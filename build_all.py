import subprocess
import os
import sys

# ANSI escape codes for colors
class Colors:
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    CYAN = '\033[96m'
    RESET = '\033[0m'

# Define your engine versions and paths
versions = [
    {"engine": "4.27", "project": "Hub427"},
    {"engine": "5.0", "project": "Hub500"},
    {"engine": "5.1", "project": "Hub510"},
    {"engine": "5.2", "project": "Hub520"},
    {"engine": "5.3", "project": "Hub530"},
    {"engine": "5.4", "project": "Hub540"},
    {"engine": "5.5", "project": "Hub550"},
    {"engine": "5.6", "project": "Hub560"},
    {"engine": "5.7", "project": "Hub570"},
]

def main():
    print(f"{Colors.CYAN}Starting build process for all versions...{Colors.RESET}")

    all_success = True
    skipped_or_failed = []

    for item in versions:
        engine_ver = item["engine"]
        proj_name = item["project"]

        print(f"\n{Colors.CYAN}" + "=" * 60)
        print(f" Compiling {proj_name} (UE {engine_ver})...")
        print("=" * 60 + f"{Colors.RESET}")

        ubt_path = rf"C:\ue_versions\UE_{engine_ver}\Engine\Build\BatchFiles\Build.bat"
        uproject_path = rf"C:\dev\ue_projects\sandbox\{proj_name}\{proj_name}.uproject"

        if not os.path.exists(ubt_path):
            print(f"{Colors.YELLOW}WARNING: Could not find UBT at {ubt_path}. Skipping.{Colors.RESET}")
            all_success = False
            skipped_or_failed.append(f"{proj_name} (Missing UBT)")
            continue

        if not os.path.exists(uproject_path):
            print(f"{Colors.YELLOW}WARNING: Could not find uproject at {uproject_path}. Skipping.{Colors.RESET}")
            all_success = False
            skipped_or_failed.append(f"{proj_name} (Missing project)")
            continue

        # Run UBT to compile the Editor target for Win64 in Development mode
        command = [
            ubt_path,
            f"{proj_name}Editor",
            "Win64",
            "Development",
            f"-Project={uproject_path}",
            "-WaitMutex"
        ]

        print(f"Executing: {' '.join(command)}")

        result = subprocess.run(command)

        if result.returncode == 0:
            print(f"\n{Colors.GREEN}Successfully compiled {proj_name}!{Colors.RESET}")
        else:
            print(f"\n{Colors.RED}ERROR: Compilation FAILED for {proj_name} (Engine {engine_ver}). Stopping.")
            print(f"Exit code: {result.returncode}{Colors.RESET}")
            sys.exit(result.returncode)

    print("\n" + "=" * 60)
    if all_success:
        print(f"{Colors.GREEN}All Hub projects compiled successfully!{Colors.RESET}")
    else:
        print(f"{Colors.YELLOW}Finished, but some projects were skipped or encountered warnings:")
        for issue in skipped_or_failed:
            print(f" - {issue}{Colors.RESET}")
    print("=" * 60 + "\n")

if __name__ == "__main__":
    main()
