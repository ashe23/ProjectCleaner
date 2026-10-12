# Copyright Ashot Barkhudaryan. All Rights Reserved.
"""
Runs inside Unreal Editor (pjc.py polygon starts it with -run=pythonscript), never imports plugin code.

Writes the project package graph straight from the asset registry, so tests can verify cleanup results
independently of ProjectCleaner's own classification:
  {"mounts": {"/Game": {"dir": ..., "mounted": true}}, "packages": {"/Game/A": {"file": ..., "deps": [...], "classes": [...]}}}

Environment:
  PJC_SNAPSHOT_OUT     output json path
  PJC_SNAPSHOT_MOUNTS  json {"/Game": "<abs content dir>", "/MyPlugin": "<abs plugin content dir>", ...}
"""

import json
import os
import time

import unreal

PACKAGE_EXTENSIONS = (".uasset", ".umap")


def class_name(asset):
    path = getattr(asset, "asset_class_path", None)
    if path is not None:
        return "{}.{}".format(path.package_name, path.asset_name)
    return str(asset.asset_class)


def main():
    start = time.time()
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)

    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    mounts = json.loads(os.environ["PJC_SNAPSHOT_MOUNTS"])
    result = {"engine": unreal.SystemLibrary.get_engine_version(), "mounts": {}, "packages": {}}

    for mount, content_dir in sorted(mounts.items()):
        # plugins that are present but not enabled are not mounted, their content is not part of the project
        mounted = registry.has_assets(mount, True)
        result["mounts"][mount] = {"dir": content_dir, "mounted": bool(mounted)}
        if not mounted:
            continue

        for root, _, files in os.walk(content_dir):
            for name in files:
                stem, ext = os.path.splitext(name)
                if ext.lower() not in PACKAGE_EXTENSIONS:
                    continue
                rel_dir = os.path.relpath(root, content_dir).replace("\\", "/")
                package = mount + ("" if rel_dir == "." else "/" + rel_dir) + "/" + stem
                assets = registry.get_assets_by_package_name(package, True) or []
                deps = registry.get_dependencies(package, options) or []
                result["packages"][package] = {
                    "file": os.path.join(root, name).replace("\\", "/"),
                    "deps": sorted({str(d) for d in deps if not str(d).startswith("/Script/")}),
                    "classes": sorted({class_name(a) for a in assets}),
                    # GameplayAbilities loads cue notifies by this tag, see GameplayCueManager
                    "gameplay_cue": any(str(a.get_tag_value("GameplayCueName") or "") not in ("", "None") for a in assets),
                }

    result["seconds"] = round(time.time() - start, 1)
    with open(os.environ["PJC_SNAPSHOT_OUT"], "w", encoding="utf-8") as f:
        json.dump(result, f, indent=1, sort_keys=True)
    unreal.log("pjc_snapshot: {} packages in {} mounts, {}s".format(len(result["packages"]), len(result["mounts"]), result["seconds"]))


main()
