#!/usr/bin/env python3
# Copyright Ashot Barkhudaryan. All Rights Reserved.
"""
ProjectCleaner dev tool - build, test and package the plugin against every installed Unreal Engine.

  python Scripts/pjc.py engines                     list discovered engines
  python Scripts/pjc.py setup   [-e 5.4,5.5]        create/refresh host projects and link the plugin into them
  python Scripts/pjc.py build   [-e ...] [-j N]     compile the plugin in every host project
  python Scripts/pjc.py test    [-e ...] [-j N]     run automation tests and commandlet (CLI) tests
  python Scripts/pjc.py cli     -e 5.5 -- -scan_only    run ProjectCleaner commandlet with given switches
  python Scripts/pjc.py package [-e ...]            RunUAT BuildPlugin (same build Fab/Marketplace does)
  python Scripts/pjc.py lint                        include path case check (Linux/Mac) + clang-format check
  python Scripts/pjc.py clean   [-e ...]            delete host Binaries/Intermediate/Saved/DerivedDataCache

Engine versions for -e: "all" (default), "5.4", "5.4,5.6", "5.0-5.3", "4.27,5.5-".

Nothing is hard-coded. Engines are discovered from, in priority order:
  1. PJC_ENGINES env var        - engine roots (or folders containing UE_* roots), separated by os.pathsep
  2. Scripts/pjc.local.json     - {"engines": [...]} (gitignored)
  3. Epic launcher manifest     - LauncherInstalled.dat (Windows, Mac)
  4. Windows registry           - launcher installs and source builds
  5. ~/.config/Epic/UnrealEngine/Install.ini - source builds (Linux)
  6. default install folders    - <ProgramFiles>/Epic Games/UE_*, /Users/Shared/Epic Games/UE_*

Host projects live in PJC_HOSTS_DIR, {"hosts_dir": ...} in pjc.local.json, or <repo>/../hosts.
Each host links Source/Resources/Config of this repo into Plugins/ProjectCleaner (junction on Windows,
symlink elsewhere), so every engine compiles the same working tree with its own Binaries/Intermediate.
"""

import argparse
import concurrent.futures
import configparser
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import time

IS_WIN = sys.platform.startswith("win")
IS_MAC = sys.platform == "darwin"
IS_LINUX = sys.platform.startswith("linux")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.dirname(SCRIPT_DIR)
PLUGIN_NAME = "ProjectCleaner"
PLUGIN_FILE = os.path.join(REPO_DIR, PLUGIN_NAME + ".uplugin")
PLUGIN_LINK_DIRS = ["Source", "Resources", "Config", "Content", "Shaders"]
TEST_CONTENT_PLUGIN = "PjcTestContent"
# Lyra like layout: game feature plugin in Plugins/GameFeatures, content plugin that exists but is not enabled
TEST_FEATURE_PLUGIN = "PjcFeature"
TEST_DISABLED_PLUGIN = "PjcDisabled"
TEST_FILTER = "ProjectCleaner"
FIXTURE_TEST = "PjcFixture.Build"
LOCAL_CONFIG = os.path.join(SCRIPT_DIR, "pjc.local.json")

USE_COLOR = sys.stdout.isatty() and os.environ.get("NO_COLOR") is None


def _c(code, text):
    return "\033[{}m{}\033[0m".format(code, text) if USE_COLOR else text


def green(text):
    return _c("92", text)


def red(text):
    return _c("91", text)


def yellow(text):
    return _c("93", text)


def cyan(text):
    return _c("96", text)


def die(msg):
    print(red("error: " + msg))
    sys.exit(2)


# ----------------------------------------------------------------------------------------------------------------------
# config
# ----------------------------------------------------------------------------------------------------------------------


def load_local_config():
    if not os.path.isfile(LOCAL_CONFIG):
        return {}
    with open(LOCAL_CONFIG, encoding="utf-8") as f:
        return json.load(f)


def hosts_dir(cfg):
    path = os.environ.get("PJC_HOSTS_DIR") or cfg.get("hosts_dir") or os.path.join(os.path.dirname(REPO_DIR), "hosts")
    return os.path.abspath(os.path.expanduser(path))


# ----------------------------------------------------------------------------------------------------------------------
# engine discovery
# ----------------------------------------------------------------------------------------------------------------------


class Engine:
    def __init__(self, root, major, minor, patch, source):
        self.root = root
        self.major = major
        self.minor = minor
        self.patch = patch
        self.source = source

    @property
    def version(self):
        return "{}.{}".format(self.major, self.minor)

    @property
    def key(self):
        return (self.major, self.minor)

    @property
    def host_name(self):
        return "Hub{}{}".format(self.major, self.minor)

    @property
    def editor_name(self):
        return "UnrealEditor" if self.major >= 5 else "UE4Editor"

    @property
    def platform(self):
        if IS_WIN:
            return "Win64"
        return "Mac" if IS_MAC else "Linux"

    @property
    def build_script(self):
        batch = os.path.join(self.root, "Engine", "Build", "BatchFiles")
        if IS_WIN:
            return os.path.join(batch, "Build.bat")
        return os.path.join(batch, "Mac" if IS_MAC else "Linux", "Build.sh")

    @property
    def uat_script(self):
        return os.path.join(self.root, "Engine", "Build", "BatchFiles", "RunUAT.bat" if IS_WIN else "RunUAT.sh")

    @property
    def editor_cmd(self):
        binaries = os.path.join(self.root, "Engine", "Binaries")
        if IS_WIN:
            return os.path.join(binaries, "Win64", self.editor_name + "-Cmd.exe")
        if IS_MAC:
            return os.path.join(binaries, "Mac", self.editor_name + ".app", "Contents", "MacOS", self.editor_name)
        return os.path.join(binaries, "Linux", self.editor_name)

    def __str__(self):
        return "{}.{}.{}".format(self.major, self.minor, self.patch)


def read_engine_version(root):
    path = os.path.join(root, "Engine", "Build", "Build.version")
    try:
        with open(path, encoding="utf-8-sig") as f:
            data = json.load(f)
        return int(data["MajorVersion"]), int(data["MinorVersion"]), int(data.get("PatchVersion", 0))
    except (OSError, ValueError, KeyError):
        return None


def _launcher_manifest_roots():
    manifests = []
    if IS_WIN:
        manifests.append(os.path.join(os.environ.get("PROGRAMDATA", "C:\\ProgramData"), "Epic", "UnrealEngineLauncher", "LauncherInstalled.dat"))
    if IS_MAC:
        manifests.append(os.path.expanduser("~/Library/Application Support/Epic/UnrealEngineLauncher/LauncherInstalled.dat"))
    for manifest in manifests:
        try:
            with open(manifest, encoding="utf-8-sig") as f:
                data = json.load(f)
        except (OSError, ValueError):
            continue
        for item in data.get("InstallationList", []):
            if item.get("AppName", "").startswith("UE_") and item.get("InstallLocation"):
                yield item["InstallLocation"]


def _registry_roots():
    if not IS_WIN:
        return
    import winreg

    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\EpicGames\Unreal Engine") as key:
            for i in range(winreg.QueryInfoKey(key)[0]):
                try:
                    with winreg.OpenKey(key, winreg.EnumKey(key, i)) as sub:
                        yield winreg.QueryValueEx(sub, "InstalledDirectory")[0]
                except OSError:
                    continue
    except OSError:
        pass
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Epic Games\Unreal Engine\Builds") as key:
            for i in range(winreg.QueryInfoKey(key)[1]):
                yield winreg.EnumValue(key, i)[1]
    except OSError:
        pass


def _linux_install_ini_roots():
    ini = os.path.expanduser("~/.config/Epic/UnrealEngine/Install.ini")
    if not os.path.isfile(ini):
        return
    parser = configparser.ConfigParser(strict=False)
    parser.optionxform = str
    try:
        parser.read(ini, encoding="utf-8")
    except configparser.Error:
        return
    if parser.has_section("Installations"):
        for _, path in parser.items("Installations"):
            yield path


def _default_folder_roots():
    parents = []
    if IS_WIN:
        parents.append(os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "Epic Games"))
    if IS_MAC:
        parents.append("/Users/Shared/Epic Games")
    for parent in parents:
        for path in sorted(glob.glob(os.path.join(parent, "UE_*"))):
            yield path


def _expand_root(path):
    """Accepts an engine root or a folder that contains UE_* engine roots."""
    path = os.path.abspath(os.path.expanduser(path.strip().strip('"')))
    if read_engine_version(path):
        return [path]
    return [p for p in sorted(glob.glob(os.path.join(path, "UE_*"))) if read_engine_version(p)]


def discover_engines(cfg):
    sources = [
        ("env", (os.environ.get("PJC_ENGINES") or "").split(os.pathsep)),
        ("local config", cfg.get("engines", [])),
        ("launcher", _launcher_manifest_roots()),
        ("registry", _registry_roots()),
        ("install.ini", _linux_install_ini_roots()),
        ("default dir", _default_folder_roots()),
    ]
    found = {}
    for source, roots in sources:
        for root in roots:
            if not root:
                continue
            for path in _expand_root(root):
                major, minor, patch = read_engine_version(path)
                if (major, minor) not in found:
                    found[(major, minor)] = Engine(path, major, minor, patch, source)
    return [found[k] for k in sorted(found)]


def _parse_version(text):
    parts = text.strip().split(".")
    if len(parts) != 2 or not all(p.isdigit() for p in parts):
        die("bad engine version '{}', expected like 5.4".format(text))
    return int(parts[0]), int(parts[1])


def select_engines(engines, spec):
    if not spec or spec == "all":
        return engines
    selected = []
    for token in spec.split(","):
        token = token.strip()
        if "-" in token:
            lo, hi = token.split("-", 1)
            lo = _parse_version(lo) if lo else (0, 0)
            hi = _parse_version(hi) if hi else (99, 99)
            selected.extend(e for e in engines if lo <= e.key <= hi)
        else:
            key = _parse_version(token)
            match = [e for e in engines if e.key == key]
            if not match:
                die("engine {} not found (see: pjc.py engines)".format(token))
            selected.extend(match)
    result = []
    for e in selected:
        if e not in result:
            result.append(e)
    return sorted(result, key=lambda e: e.key)


# ----------------------------------------------------------------------------------------------------------------------
# file helpers
# ----------------------------------------------------------------------------------------------------------------------


def write_if_changed(path, content):
    if os.path.isfile(path):
        with open(path, encoding="utf-8", newline="") as f:
            if f.read() == content:
                return False
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    return True


def is_link(path):
    if os.path.islink(path):
        return True
    isjunction = getattr(os.path, "isjunction", None)
    if isjunction and isjunction(path):
        return True
    if IS_WIN and os.path.isdir(path):
        try:
            return os.path.normcase(os.readlink(path)) != os.path.normcase(path)
        except (OSError, ValueError):
            return False
    return False


def link_target(path):
    try:
        target = os.readlink(path)
    except OSError:
        return None
    if target.startswith("\\\\?\\"):
        target = target[4:]
    return os.path.normcase(os.path.abspath(target))


def remove_link(path):
    # never use rmtree on a link: on older Pythons it follows junctions and deletes the repo files
    try:
        os.unlink(path)
    except OSError:
        os.rmdir(path)


def make_dir_link(link, target):
    try:
        os.symlink(target, link, target_is_directory=True)
        return
    except (OSError, NotImplementedError):
        if not IS_WIN:
            raise
    # Windows without symlink privilege: directory junctions need no admin rights
    result = subprocess.run(["cmd", "/c", "mklink", "/J", link, target], capture_output=True, text=True)
    if result.returncode != 0:
        raise OSError("mklink /J failed: " + (result.stdout + result.stderr).strip())


def remove_tree(path):
    """Deletes a real directory tree. Refuses links so it can never reach into the repo."""
    if not os.path.exists(path):
        return
    if is_link(path):
        remove_link(path)
        return

    def on_error(func, p, _):
        os.chmod(p, 0o777)
        func(p)

    for root, dirs, _ in os.walk(path):
        for d in list(dirs):
            full = os.path.join(root, d)
            if is_link(full):
                remove_link(full)
                dirs.remove(d)
    shutil.rmtree(path, onerror=on_error)


# ----------------------------------------------------------------------------------------------------------------------
# host projects
# ----------------------------------------------------------------------------------------------------------------------


class Host:
    def __init__(self, engine, root_dir):
        self.engine = engine
        self.name = engine.host_name
        self.dir = os.path.join(root_dir, self.name)
        self.uproject = os.path.join(self.dir, self.name + ".uproject")
        self.plugin_dir = os.path.join(self.dir, "Plugins", PLUGIN_NAME)
        self.logs_dir = os.path.join(root_dir, "_logs", self.name)

    def log(self, name):
        os.makedirs(self.logs_dir, exist_ok=True)
        return os.path.join(self.logs_dir, name + ".log")


def _uproject_json(host):
    data = {
        "FileVersion": 3,
        "EngineAssociation": host.engine.version,
        "Category": "",
        "Description": "ProjectCleaner test host, generated by Scripts/pjc.py",
        "Modules": [{"Name": host.name, "Type": "Runtime", "LoadingPhase": "Default"}],
        "Plugins": [
            {"Name": PLUGIN_NAME, "Enabled": True},
            {"Name": TEST_CONTENT_PLUGIN, "Enabled": True},
            {"Name": "GameFeatures", "Enabled": True},
            {"Name": "ModularGameplay", "Enabled": True},
            {"Name": TEST_FEATURE_PLUGIN, "Enabled": True},
        ],
    }
    return json.dumps(data, indent="\t") + "\n"


def _test_content_uplugin(name=TEST_CONTENT_PLUGIN, description="Content only plugin used by ProjectCleaner tests (multi root scanning)", **extra):
    data = {
        "FileVersion": 3,
        "Version": 1,
        "VersionName": "1.0",
        "FriendlyName": name,
        "Description": description,
        "Category": "Testing",
        "CanContainContent": True,
        "Installed": False,
    }
    data.update(extra)
    return json.dumps(data, indent="\t") + "\n"


# Toolchain workarounds for the host target only, the plugin itself stays untouched.
# UE 5.0-5.4 + MSVC 14.38+: ConcurrentLinearAllocator.h evaluates __has_feature when the MSVC asan header exists
# (C4668/C4067 errors). 5.1+ Platform.h respects a predefined PLATFORM_HAS_ASAN_INCLUDE, 5.0 checks the header
# directly, so there __has_feature itself gets defined (same result as MSVC not supporting the feature).
HOST_TARGET_PATCHES = {(5, minor): ["bOverrideBuildEnvironment = true;", 'GlobalDefinitions.Add("PLATFORM_HAS_ASAN_INCLUDE=0");'] for minor in range(1, 5)}
HOST_TARGET_PATCHES[(5, 0)] = ["bOverrideBuildEnvironment = true;", 'GlobalDefinitions.Add("__has_feature(x)=0");']


def msvc_workaround_env(engine, root):
    """
    Same fix as HOST_TARGET_PATCHES for builds that do not use our host targets (RunUAT BuildPlugin makes its own
    host project). MSVC prepends the CL environment variable to every cl.exe command line.
    """
    if not IS_WIN or (engine.major, engine.minor) not in HOST_TARGET_PATCHES:
        return {}
    if engine.minor >= 1:
        return {"CL": "/DPLATFORM_HAS_ASAN_INCLUDE=0"}
    # 5.0: /D cannot define function like macros, force include a header instead
    header = os.path.join(root, "_toolchain", "pjc_has_feature.h")
    write_if_changed(header, "#pragma once\n#ifndef __has_feature\n#define __has_feature(x) 0\n#endif\n")
    return {"CL": "/FI" + header}


def _patch_target(engine, content):
    lines = HOST_TARGET_PATCHES.get(engine.key) if IS_WIN else None
    if not lines:
        return content
    anchor = re.search(r"^(\s*)Type = TargetType\.\w+;\s*$", content, re.M)
    if not anchor:
        return content
    indent = anchor.group(1)
    patch = "".join("\n{}{}".format(indent, line) for line in lines)
    return content[: anchor.end()] + patch + content[anchor.end():]


def setup_host(host):
    """Creates the host C++ project from the engine's own TP_Blank template, so it always matches that engine."""
    engine = host.engine
    template = os.path.join(engine.root, "Templates", "TP_Blank", "Source")
    if not os.path.isdir(template):
        raise RuntimeError("engine template not found: " + template)

    os.makedirs(host.dir, exist_ok=True)
    write_if_changed(host.uproject, _uproject_json(host))

    for root, _, files in os.walk(template):
        for name in files:
            src = os.path.join(root, name)
            rel = os.path.relpath(src, template).replace("TP_Blank", host.name)
            with open(src, encoding="utf-8-sig", errors="replace") as f:
                content = f.read().replace("TP_Blank", host.name).replace("TP_BLANK", host.name.upper())
            if name.endswith(".Target.cs"):
                content = _patch_target(engine, content)
            write_if_changed(os.path.join(host.dir, "Source", rel), content)

    # PjcTestPrimary: blueprint based primary type used by fixture tests (Blueprints/BP_Primary)
    # GameFeatureData: required by game feature plugins (same rule as Lyra)
    # staged folder: raw files shipped with packaged builds, never "external files"
    write_if_changed(
        os.path.join(host.dir, "Config", "DefaultGame.ini"),
        "[/Script/EngineSettings.GeneralProjectSettings]\n"
        "ProjectName={}\n"
        "\n"
        "[/Script/Engine.AssetManagerSettings]\n"
        '+PrimaryAssetTypesToScan=(PrimaryAssetType="PjcTestPrimary",AssetBaseClass=/Script/Engine.PlayerController,'
        'bHasBlueprintClasses=True,bIsEditorOnly=True,Directories=((Path="/Game/PjcTests/Blueprints")),'
        "Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=Unknown))\n"
        '+PrimaryAssetTypesToScan=(PrimaryAssetType="GameFeatureData",AssetBaseClass="/Script/GameFeatures.GameFeatureData",'
        "bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=,SpecificAssets=,"
        "Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))\n"
        "\n"
        "[/Script/UnrealEd.ProjectPackagingSettings]\n"
        '+DirectoriesToAlwaysStageAsNonUFS=(Path="PjcTests/Staged")\n'.format(host.name),
    )
    write_if_changed(os.path.join(host.dir, "Config", "DefaultEngine.ini"), "[/Script/Engine.Engine]\n")

    plugin_root = os.path.join(host.dir, "Plugins", TEST_CONTENT_PLUGIN)
    write_if_changed(os.path.join(plugin_root, TEST_CONTENT_PLUGIN + ".uplugin"), _test_content_uplugin())
    os.makedirs(os.path.join(plugin_root, "Content"), exist_ok=True)
    os.makedirs(os.path.join(host.dir, "Content"), exist_ok=True)

    feature_root = os.path.join(host.dir, "Plugins", "GameFeatures", TEST_FEATURE_PLUGIN)
    write_if_changed(
        os.path.join(feature_root, TEST_FEATURE_PLUGIN + ".uplugin"),
        _test_content_uplugin(
            TEST_FEATURE_PLUGIN,
            "Game feature plugin used by ProjectCleaner tests (Lyra like layout)",
            Category="Game Features",
            EnabledByDefault=False,
            ExplicitlyLoaded=True,
            BuiltInInitialFeatureState="Registered",
        ),
    )
    os.makedirs(os.path.join(feature_root, "Content"), exist_ok=True)
    os.makedirs(os.path.join(feature_root, "Config"), exist_ok=True)

    # present in Plugins but not enabled (like Lyra GreenRoom/RedRoom): not mounted, must never be touched
    disabled_root = os.path.join(host.dir, "Plugins", TEST_DISABLED_PLUGIN)
    write_if_changed(
        os.path.join(disabled_root, TEST_DISABLED_PLUGIN + ".uplugin"),
        _test_content_uplugin(TEST_DISABLED_PLUGIN, "Disabled content plugin used by ProjectCleaner tests", ExplicitlyLoaded=True, EnabledByDefault=False),
    )
    os.makedirs(os.path.join(disabled_root, "Content"), exist_ok=True)

    sync_host(host)


def sync_host(host):
    """Links repo folders into host Plugins/ProjectCleaner. Binaries/Intermediate stay per host."""
    os.makedirs(host.plugin_dir, exist_ok=True)

    if is_link(host.plugin_dir):
        raise RuntimeError("{} is a link, expected a real folder".format(host.plugin_dir))

    for name in PLUGIN_LINK_DIRS:
        target = os.path.join(REPO_DIR, name)
        link = os.path.join(host.plugin_dir, name)
        wanted = os.path.isdir(target)

        if is_link(link):
            if wanted and link_target(link) == os.path.normcase(os.path.abspath(target)):
                continue
            remove_link(link)
        elif os.path.exists(link):
            remove_tree(link)

        if wanted:
            make_dir_link(link, target)

    dst = os.path.join(host.plugin_dir, PLUGIN_NAME + ".uplugin")
    with open(PLUGIN_FILE, encoding="utf-8", newline="") as f:
        write_if_changed(dst, f.read())


def _pid_alive(pid):
    if IS_WIN:
        out = subprocess.run(["tasklist", "/FI", "PID eq %d" % pid, "/NH"], capture_output=True, text=True).stdout
        return str(pid) in out
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


class HostsLock:
    """One pjc.py run per hosts folder: parallel runs delete each other's test content."""

    def __init__(self, root):
        self.path = os.path.join(root, ".pjc.lock")

    def __enter__(self):
        os.makedirs(os.path.dirname(self.path), exist_ok=True)
        for _ in range(2):
            try:
                fd = os.open(self.path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            except FileExistsError:
                try:
                    with open(self.path, encoding="utf-8") as f:
                        pid = int(f.read().strip() or 0)
                except (OSError, ValueError):
                    pid = 0
                if pid and _pid_alive(pid):
                    die("another pjc.py run (pid {}) is using {}".format(pid, os.path.dirname(self.path)))
                os.remove(self.path)  # stale lock
                continue
            os.write(fd, str(os.getpid()).encode())
            os.close(fd)
            return self
        die("could not lock " + self.path)

    def __exit__(self, *_):
        try:
            os.remove(self.path)
        except OSError:
            pass


# ----------------------------------------------------------------------------------------------------------------------
# process helpers
# ----------------------------------------------------------------------------------------------------------------------


def bundled_dotnet(engine):
    """Engine shipped dotnet folder, if any: DotNet/<ver>/<platform> (5.1+) or DotNet/<Platform> (5.0)."""
    if engine is None:
        return None
    sub = "windows" if IS_WIN else ("linux" if IS_LINUX else ("mac-arm64" if os.uname().machine == "arm64" else "mac-x64"))
    exe = "dotnet.exe" if IS_WIN else "dotnet"
    root = os.path.join(engine.root, "Engine", "Binaries", "ThirdParty", "DotNet")
    candidates = glob.glob(os.path.join(root, "*", sub, exe))
    if not candidates:
        legacy = os.path.join(root, "Windows" if IS_WIN else ("Linux" if IS_LINUX else "Mac"), exe)
        return os.path.dirname(legacy) if os.path.isfile(legacy) else None

    def version_key(path):
        ver = os.path.basename(os.path.dirname(os.path.dirname(path)))
        return [int(p) if p.isdigit() else 0 for p in ver.split(".")]

    return os.path.dirname(max(candidates, key=version_key))


def child_env(engine=None):
    env = os.environ.copy()
    # set by some shells (e.g. Git Bash); UE4 RunUAT.bat starts AutomationTool.exe from current directory
    for key in [k for k in env if k.upper() == "NODEFAULTCURRENTDIRECTORYINEXEPATH"]:
        del env[key]
    # Same as engine GetDotnetPath script. UE 5.1 installed builds skip it and run UBT on whatever system dotnet
    # exists, which fails on machines with only .NET 8+ (BinaryFormatter removed). Use the engine own runtime.
    dotnet = bundled_dotnet(engine)
    if dotnet:
        env["PATH"] = dotnet + os.pathsep + env.get("PATH", "")
        env["DOTNET_ROOT"] = dotnet
        env["DOTNET_MULTILEVEL_LOOKUP"] = "0"
        env["DOTNET_ROLL_FORWARD"] = "LatestMajor"
    return env


def run_logged(cmd, log_path, timeout=None, cwd=None, engine=None, extra_env=None):
    """Runs a command with all output written to a log file. Returns exit code (or None on timeout)."""
    env = child_env(engine)
    env.update(extra_env or {})
    with open(log_path, "w", encoding="utf-8", errors="replace") as f:
        f.write("$ " + subprocess.list2cmdline(cmd) + "\n")
        for key, value in sorted((extra_env or {}).items()):
            f.write("$ env {}={}\n".format(key, value))
        f.write("\n")
        f.flush()
        try:
            result = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, timeout=timeout, cwd=cwd, env=env)
            return result.returncode
        except subprocess.TimeoutExpired:
            f.write("\n\nTIMEOUT after {} sec\n".format(timeout))
            return None


def run_ubt_logged(cmd, log, engine, extra_env=None):
    """UBT/UAT run. Engine versions share some per-user UBT files (e.g. MarketplaceRules.dll), retry on collision."""
    code = None
    for attempt in range(3):
        code = run_logged(cmd, log, timeout=3600, engine=engine, extra_env=extra_env)
        if code == 0 or not log_lines(log, r"being used by another process", 1):
            break
        time.sleep(10 + attempt * 10)
    return code


def log_lines(log_path, pattern, limit=25):
    regex = re.compile(pattern)
    out = []
    with open(log_path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if regex.search(line):
                line = line.rstrip()
                if line not in out:
                    out.append(line)
    return out[:limit]


def run_for_hosts(hosts, func, jobs, title):
    """Runs func(host) -> (ok, summary) for every host, in parallel when jobs > 1."""
    print(cyan("== {} ({} engine{}, {} job{})".format(title, len(hosts), "" if len(hosts) == 1 else "s", jobs, "" if jobs == 1 else "s")))
    results = {}

    def wrapped(host):
        start = time.time()
        try:
            ok, summary = func(host)
        except Exception as ex:  # report and continue with other engines
            ok, summary = False, "exception: {}".format(ex)
        elapsed = time.time() - start
        status = green("OK  ") if ok else red("FAIL")
        print("  [{}] {:<6} {:>6.0f}s  {}".format(status, host.engine.version, elapsed, summary.splitlines()[0] if summary else ""), flush=True)
        for extra in summary.splitlines()[1:]:
            print("                        " + extra)
        return host, ok

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        for host, ok in pool.map(wrapped, hosts):
            results[host.engine.version] = ok

    failed = [v for v, ok in results.items() if not ok]
    if failed:
        print(red("== {} failed for: {}".format(title, ", ".join(failed))))
    else:
        print(green("== {} passed for all {} engine(s)".format(title, len(results))))
    return not failed


# ----------------------------------------------------------------------------------------------------------------------
# commands
# ----------------------------------------------------------------------------------------------------------------------


def cmd_engines(args, engines, cfg):
    print("hosts dir: " + hosts_dir(cfg))
    if not engines:
        print(yellow("no engines found. Set PJC_ENGINES or add 'engines' to " + LOCAL_CONFIG))
        return True
    for e in engines:
        tools = all(os.path.isfile(p) for p in (e.build_script, e.editor_cmd))
        print("  {:<8} {:<10} {:<13} {}{}".format(e.version, str(e), "[" + e.source + "]", e.root, "" if tools else yellow("  (missing build tools)")))
    return True


def cmd_setup(args, hosts):
    def work(host):
        setup_host(host)
        return True, host.dir

    return run_for_hosts(hosts, work, 1, "setup")


def cmd_sync(args, hosts):
    def work(host):
        if not os.path.isfile(host.uproject):
            setup_host(host)
        else:
            sync_host(host)
        return True, host.plugin_dir

    return run_for_hosts(hosts, work, 1, "sync")


def build_host(host, clang=False, strict=False):
    # always refresh host files (writes only what changed), so hosts follow tool updates
    setup_host(host)
    engine = host.engine
    log = host.log("build")
    # own UBT log per host, default one is shared by all engines and breaks parallel builds
    cmd = [
        engine.build_script,
        host.name + "Editor",
        engine.platform,
        "Development",
        "-Project=" + host.uproject,
        "-WaitMutex",
        "-NoHotReloadFromIDE",
        "-Log=" + host.log("ubt"),
    ]
    if clang:
        cmd.append("-Compiler=Clang")
    if strict:
        # every file compiled on its own, catches missing includes hidden by unity and precompiled headers
        cmd += ["-DisableUnity", "-NoPCH"]
    code = run_ubt_logged(cmd, log, engine)
    warnings = log_lines(log, r"[\\/](ProjectCleaner|Pjc)[\\/].*\.(cpp|h)\(\d+.*: warning")
    errors = log_lines(log, r"error [A-Z]+\d+:|: error\b|: fatal error|ERROR:|Unhandled exception") if code != 0 else []
    if code == 0:
        summary = "built" + (yellow(" with {} plugin warning(s)".format(len(warnings))) if warnings else "")
        summary += "\n".join([""] + warnings[:10]) if warnings else ""
        return True, summary
    return False, "exit {}  log: {}\n{}".format(code, log, "\n".join(errors[:15]))


def cmd_build(args, hosts):
    return run_for_hosts(hosts, lambda h: build_host(h, args.clang, args.strict), args.jobs, "build")


def editor_args(host, extra):
    return [host.engine.editor_cmd, host.uproject] + extra + [
        "-unattended",
        "-nopause",
        "-nosplash",
        "-nullrhi",
        "-NoSound",
        "-stdout",
        "-FullStdOutLogOutput",
        "-PjcTestHost",
    ]


def run_automation(host, test_filter, tag):
    report_dir = os.path.join(host.logs_dir, "report_" + tag)
    remove_tree(report_dir)
    os.makedirs(report_dir, exist_ok=True)
    cmd = editor_args(
        host,
        [
            "-ExecCmds=Automation RunTests {};Quit".format(test_filter),
            "-TestExit=Automation Test Queue Empty",
            "-ReportExportPath=" + report_dir,
        ],
    )
    log = host.log(tag)
    code = run_logged(cmd, log, timeout=2400, engine=host.engine)

    index = os.path.join(report_dir, "index.json")
    if not os.path.isfile(index):
        crash = log_lines(log, r"(?i)(fatal error|assertion failed|unhandled exception|error:)", 10)
        return False, "no test report (exit {})  log: {}\n{}".format(code, log, "\n".join(crash))

    with open(index, encoding="utf-8-sig") as f:
        report = json.load(f)
    tests = report.get("tests", [])
    failed = [t for t in tests if t.get("state") not in ("Success", "Skipped")]
    lines = []
    for t in failed:
        lines.append(red("FAIL ") + t.get("fullTestPath", t.get("testDisplayName", "?")))
        for entry in t.get("entries", []):
            event = entry.get("event", {})
            if event.get("type") == "Error":
                lines.append("     " + event.get("message", "").strip()[:300])
    if not tests:
        return False, "no tests matched '{}'  log: {}".format(test_filter, log)
    summary = "{} passed, {} failed".format(len(tests) - len(failed), len(failed))
    return not failed, "\n".join([summary] + lines)


def run_commandlet(host, switches, tag):
    cmd = editor_args(host, ["-run=Pjc"] + switches)
    log = host.log(tag)
    code = run_logged(cmd, log, timeout=1800, engine=host.engine)
    return code, log


STAT_KEYS = {
    "Assets All": "assets_all",
    "Assets Used": "assets_used",
    "Assets Unused": "assets_unused",
    "Files External": "files_external",
    "Files Corrupted": "files_corrupted",
    "Folders Empty": "folders_empty",
}


def parse_cli_stats(log):
    """Returns list of stats blocks printed by the commandlet (before and after cleanup)."""
    blocks = []
    current = None
    regex = re.compile(r"LogProjectCleanerCLI: Display: ([A-Za-z ]+?) - (\d+)")
    with open(log, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = regex.search(line)
            if not m or m.group(1) not in STAT_KEYS:
                continue
            key = STAT_KEYS[m.group(1)]
            if current is None or key in current:
                current = {}
                blocks.append(current)
            current[key] = int(m.group(2))
    return blocks


def commandlet_failed(code, log):
    """Engine turns logged errors into exit code 1. Errors about the fixture corrupted file are expected."""
    if code == 0:
        return False
    unexpected = [line for line in log_lines(log, r": Error: ", 50) if "Broken.uasset" not in line]
    return code is None or bool(unexpected) or not log_lines(log, r"Broken\.uasset", 1)


# everything fixture tests may leave behind (crashed or failed runs), same list PjcFixture.Build writes to expected.json
FIXTURE_LEFTOVERS = [
    "Content/PjcTests",
    "Content/Localization/PjcTests",
    "Config/PjcTests.ini",
    "Plugins/" + TEST_CONTENT_PLUGIN + "/Content/*",
    "Plugins/GameFeatures/" + TEST_FEATURE_PLUGIN + "/Content/*",
    "Plugins/GameFeatures/" + TEST_FEATURE_PLUGIN + "/Config/PjcFeatureTests.ini",
    "Plugins/" + TEST_DISABLED_PLUGIN + "/Content/*",
]


def remove_fixture(host, expected=None):
    expected = expected or {}
    for rel in sorted(set(expected.get("cleanup_paths", []) + FIXTURE_LEFTOVERS)):
        # "Folder/*": contents only, folder itself belongs to host setup
        paths = glob.glob(os.path.join(host.dir, rel)) if rel.endswith("/*") else [os.path.join(host.dir, rel)]
        for path in paths:
            if os.path.isdir(path):
                remove_tree(path)
            elif os.path.isfile(path):
                os.remove(path)
    if expected.get("cleanup_user_config", True):
        # exclude and scan settings saved by the fixture for the commandlet run
        for name in ("EditorPerProjectUserSettings.ini", "Editor.ini"):
            for ini in glob.glob(os.path.join(host.dir, "Saved", "Config", "*", name)):
                os.remove(ini)
        default_editor = os.path.join(host.dir, "Config", "DefaultEditor.ini")
        if os.path.isfile(default_editor):
            with open(default_editor, encoding="utf-8", errors="replace") as f:
                generated = "PjcScanSettings" in f.read()
            if generated:
                os.remove(default_editor)
    expected_file = os.path.join(host.dir, "Saved", "PjcFixture", "expected.json")
    if os.path.isfile(expected_file):
        os.remove(expected_file)


def run_cli_tests(host):
    """End to end: build fixture content in editor, then scan and clean it with the commandlet."""
    ok, summary = run_automation(host, FIXTURE_TEST, "fixture")
    expected_path = os.path.join(host.dir, "Saved", "PjcFixture", "expected.json")
    if not ok or not os.path.isfile(expected_path):
        remove_fixture(host)
        return False, "fixture build failed: " + summary
    with open(expected_path, encoding="utf-8-sig") as f:
        expected = json.load(f)

    problems = []
    try:
        for rel, content in expected.get("create_files", {}).items():
            path = os.path.join(host.dir, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8") as f:
                f.write(content)

        code, log = run_commandlet(host, ["-scan_only"], "cli_scan")
        blocks = parse_cli_stats(log)
        if commandlet_failed(code, log) or not blocks:
            problems.append("scan_only: exit {}, stats blocks {}  log: {}".format(code, len(blocks), log))
        else:
            for key, value in expected["scan"].items():
                if blocks[0].get(key) != value:
                    problems.append("scan_only: {} = {}, expected {}".format(key, blocks[0].get(key), value))

        code, log = run_commandlet(host, ["-full_cleanup", "-list"], "cli_cleanup")
        blocks = parse_cli_stats(log)
        if commandlet_failed(code, log) or len(blocks) < 2:
            problems.append("full_cleanup: exit {}, stats blocks {}  log: {}".format(code, len(blocks), log))
        else:
            for key, value in expected["after_cleanup"].items():
                if blocks[1].get(key) != value:
                    problems.append("full_cleanup: {} = {}, expected {}".format(key, blocks[1].get(key), value))

        for rel in expected.get("kept_files", []):
            if not os.path.exists(os.path.join(host.dir, rel)):
                problems.append("deleted but must be kept: " + rel)
        for rel in expected.get("deleted_files", []):
            if os.path.exists(os.path.join(host.dir, rel)):
                problems.append("not deleted: " + rel)
    finally:
        remove_fixture(host, expected)

    if problems:
        return False, "\n".join(["{} problem(s)".format(len(problems))] + problems)
    return True, "scan + full_cleanup verified ({} kept, {} deleted)".format(len(expected.get("kept_files", [])), len(expected.get("deleted_files", [])))


def cmd_test(args, hosts):
    def work(host):
        if not args.no_build:
            ok, summary = build_host(host, args.clang)
            if not ok:
                return False, "build: " + summary
        results = []
        ok_all = True
        # leftovers of crashed runs would show up as unexpected assets
        remove_fixture(host)
        if not args.cli_only:
            ok, summary = run_automation(host, args.filter, "automation")
            ok_all &= ok
            results.append("automation: " + summary)
        if not args.automation_only:
            ok, summary = run_cli_tests(host)
            ok_all &= ok
            results.append("cli: " + summary)
        return ok_all, "\n".join(results)

    return run_for_hosts(hosts, work, args.jobs, "test")


def cmd_cli(args, hosts):
    switches = [s for s in args.rest if s != "--"]
    if not switches:
        die("pass commandlet switches after --, e.g. pjc.py cli -e 5.5 -- -scan_only")

    def work(host):
        code, log = run_commandlet(host, switches, "cli")
        stats = parse_cli_stats(log)
        return code == 0, "exit {}  {}  log: {}".format(code, stats[-1] if stats else "", log)

    return run_for_hosts(hosts, work, args.jobs, "cli " + " ".join(switches))


def cmd_package(args, hosts, cfg):
    out_root = os.path.join(os.path.dirname(hosts_dir(cfg)), "packages") if not args.out else os.path.abspath(args.out)

    def work(host):
        sync_host(host)
        out = os.path.join(out_root, "{}_{}".format(PLUGIN_NAME, host.engine.version))
        remove_tree(out)
        cmd = [
            host.engine.uat_script,
            "BuildPlugin",
            "-Plugin=" + os.path.join(host.plugin_dir, PLUGIN_NAME + ".uplugin"),
            "-Package=" + out,
            "-TargetPlatforms=" + host.engine.platform,
            "-Rocket",
        ]
        log = host.log("package")
        code = run_ubt_logged(cmd, log, host.engine, msvc_workaround_env(host.engine, os.path.dirname(host.dir)))
        # some UAT batch files exit 0 even when AutomationTool could not start, check the result too
        binaries = glob.glob(os.path.join(out, "Binaries", host.engine.platform, "*Pjc*"))
        if code == 0 and binaries:
            return True, out
        if code == 0:
            code = "0 but no plugin binaries"
        return False, "exit {}  log: {}\n{}".format(code, log, "\n".join(log_lines(log, r"(?i)\berror\b", 15)))

    return run_for_hosts(hosts, work, args.jobs, "package")


def cmd_clean(args, hosts):
    def work(host):
        removed = []
        for path in (
            os.path.join(host.dir, "Binaries"),
            os.path.join(host.dir, "Intermediate"),
            os.path.join(host.dir, "Saved"),
            os.path.join(host.dir, "DerivedDataCache"),
            os.path.join(host.plugin_dir, "Binaries"),
            os.path.join(host.plugin_dir, "Intermediate"),
        ):
            if os.path.isdir(path):
                remove_tree(path)
                removed.append(os.path.relpath(path, host.dir))
        return True, "removed: " + (", ".join(removed) if removed else "nothing")

    return run_for_hosts(hosts, work, 1, "clean")


# ----------------------------------------------------------------------------------------------------------------------
# lint
# ----------------------------------------------------------------------------------------------------------------------


def _header_index(roots):
    """Maps lowercase relative-suffix lookups to real paths for every header under given roots."""
    index = {}
    for root in roots:
        for dirpath, _, files in os.walk(root):
            for name in files:
                if name.endswith((".h", ".hpp", ".inl")):
                    full = os.path.join(dirpath, name).replace("\\", "/")
                    index.setdefault(name.lower(), []).append(full)
    return index


def check_include_case(engines):
    """Windows and default Mac file systems ignore case, Linux does not. Catch mismatches before users do."""
    source_dir = os.path.join(REPO_DIR, "Source")
    roots = [source_dir]
    for e in engines:
        roots += [os.path.join(e.root, "Engine", "Source"), os.path.join(e.root, "Engine", "Plugins")]
    index = _header_index(roots)
    include_re = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
    problems = []
    for dirpath, _, files in os.walk(source_dir):
        for name in files:
            if not name.endswith((".h", ".cpp")):
                continue
            path = os.path.join(dirpath, name)
            with open(path, encoding="utf-8-sig", errors="replace") as f:
                for num, line in enumerate(f, 1):
                    m = include_re.match(line)
                    if not m or m.group(1).endswith(".generated.h"):
                        continue
                    inc = m.group(1).replace("\\", "/")
                    candidates = index.get(os.path.basename(inc).lower(), [])
                    exact = [c for c in candidates if c.endswith("/" + inc)]
                    folded = [c for c in candidates if c.lower().endswith("/" + inc.lower())]
                    if folded and not exact:
                        real = folded[0][len(folded[0]) - len(inc):]
                        problems.append("{}:{}: include \"{}\" has wrong case, file is \"{}\"".format(os.path.relpath(path, REPO_DIR), num, inc, real))
    return problems


def check_clang_format():
    exe = shutil.which("clang-format")
    if not exe:
        return None, "clang-format not found, skipped"
    files = []
    for dirpath, _, names in os.walk(os.path.join(REPO_DIR, "Source")):
        files += [os.path.join(dirpath, n) for n in names if n.endswith((".h", ".cpp"))]
    result = subprocess.run([exe, "--dry-run", "--style=file"] + files, capture_output=True, text=True, cwd=REPO_DIR)
    bad = sorted({line.split(":")[0] if not IS_WIN else ":".join(line.split(":")[:2]) for line in result.stderr.splitlines() if "warning:" in line})
    if bad:
        return False, "{} file(s) not clang-formatted:\n".format(len(bad)) + "\n".join("  " + os.path.relpath(b, REPO_DIR) for b in bad)
    return True, "clang-format clean ({} files)".format(len(files))


def cmd_lint(args, engines):
    ok = True
    newest = engines[-1:] + engines[:1] if engines else []
    problems = check_include_case(newest)
    if problems:
        ok = False
        print(red("include case problems (break Linux builds):"))
        for p in problems:
            print("  " + p)
    else:
        print(green("include case: ok") + " (checked against {})".format(", ".join(e.version for e in newest) or "plugin only"))
    fmt_ok, msg = check_clang_format()
    if fmt_ok is False:
        ok = False
        print(red(msg))
    else:
        print((green if fmt_ok else yellow)(msg))
    return ok


# ----------------------------------------------------------------------------------------------------------------------
# main
# ----------------------------------------------------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description="ProjectCleaner dev tool", formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    def add(name, help_text, jobs=1):
        p = sub.add_parser(name, help=help_text)
        p.add_argument("-e", "--engines", default="all", help="engine versions: all | 5.4 | 5.4,5.6 | 5.0-5.3 | 5.5-")
        p.add_argument("-j", "--jobs", type=int, default=jobs, help="engines processed in parallel")
        return p

    add("engines", "list discovered engines")
    add("setup", "create or refresh host projects")
    add("sync", "re-link plugin into host projects")
    p = add("build", "compile plugin for each engine")
    p.add_argument("--clang", action="store_true", help="compile with clang (Windows only, catches Linux/Mac compiler errors)")
    p.add_argument("--strict", action="store_true", help="no unity build and no precompiled headers (finds missing includes). UE 5.3 launcher builds do not support it")
    p = add("test", "build, then run automation and commandlet tests")
    p.add_argument("--filter", default=TEST_FILTER, help="automation test filter (default: %(default)s)")
    p.add_argument("--no-build", action="store_true", help="skip build step")
    p.add_argument("--clang", action="store_true", help="build with clang")
    p.add_argument("--automation-only", action="store_true", help="skip commandlet tests")
    p.add_argument("--cli-only", action="store_true", help="skip automation tests")
    p = add("cli", "run ProjectCleaner commandlet")
    p.add_argument("rest", nargs=argparse.REMAINDER, help="-- followed by commandlet switches")
    p = add("package", "RunUAT BuildPlugin for each engine")
    p.add_argument("--out", help="output folder (default: <hosts dir>/../packages)")
    add("lint", "include case and clang-format checks")
    add("clean", "delete host build products")

    args = parser.parse_args()
    cfg = load_local_config()
    engines = discover_engines(cfg)

    if args.command == "engines":
        return cmd_engines(args, engines, cfg)
    if args.command == "lint":
        return cmd_lint(args, select_engines(engines, args.engines))

    if not engines:
        die("no Unreal Engine installs found. Set PJC_ENGINES or add 'engines' to " + LOCAL_CONFIG)

    selected = select_engines(engines, args.engines)
    root = hosts_dir(cfg)
    hosts = [Host(e, root) for e in selected]

    commands = {
        "setup": lambda: cmd_setup(args, hosts),
        "sync": lambda: cmd_sync(args, hosts),
        "build": lambda: cmd_build(args, hosts),
        "test": lambda: cmd_test(args, hosts),
        "cli": lambda: cmd_cli(args, hosts),
        "package": lambda: cmd_package(args, hosts, cfg),
        "clean": lambda: cmd_clean(args, hosts),
    }
    with HostsLock(root):
        return commands[args.command]()


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
