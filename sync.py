import os
import subprocess
import shutil
import stat

# ANSI escape codes for colors
class Colors:
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    CYAN = '\033[96m'
    RESET = '\033[0m'

# The main plugin directory that serves as the source of truth
source_plugin_dir = r"C:\dev\ue_projects\sandbox\Hub427\Plugins\ProjectCleaner"

# The destination projects where the symlink should be created
target_projects = [
    "Hub500",
    "Hub510",
    "Hub520",
    "Hub530",
    # "Hub540",
    # "Hub550",
    # "Hub560",
    # "Hub570",
]

# Items we DO NOT want to sync to other engine versions
EXCLUDED_ITEMS = {'.git', 'Binaries', 'Intermediate', '.vs', '.idea'}

def remove_readonly(func, path, _):
    """Clear the readonly bit and reattempt the removal. This fixes the .git Access Denied error."""
    try:
        os.chmod(path, stat.S_IWRITE)
        func(path)
    except Exception:
        pass

def main():
    print(f"{Colors.CYAN}Starting symlink synchronization...{Colors.RESET}\n")

    if not os.path.exists(source_plugin_dir):
        print(f"{Colors.RED}ERROR: Source plugin directory does not exist: {source_plugin_dir}{Colors.RESET}")
        return

    # Get the list of files/folders to link
    items_to_sync = [item for item in os.listdir(source_plugin_dir) if item not in EXCLUDED_ITEMS]

    for proj in target_projects:
        dest_plugins_dir = rf"C:\dev\ue_projects\sandbox\{proj}\Plugins"
        dest_plugin_dir = os.path.join(dest_plugins_dir, "ProjectCleaner")
        
        # 1. Ensure the Plugins directory exists
        os.makedirs(dest_plugins_dir, exist_ok=True)
            
        # 2. Remove the existing ProjectCleaner directory/link completely
        if os.path.exists(dest_plugin_dir) or os.path.islink(dest_plugin_dir):
            print(f"{Colors.YELLOW}[{proj}] Cleaning existing directory...{Colors.RESET}")
            try:
                if os.path.islink(dest_plugin_dir) or os.path.isfile(dest_plugin_dir):
                    os.remove(dest_plugin_dir)
                else:
                    # Using onerror=remove_readonly to bypass the Windows read-only block on .git objects
                    shutil.rmtree(dest_plugin_dir, onerror=remove_readonly)
            except Exception as e:
                print(f"{Colors.RED}[{proj}] Failed to remove existing path: {e}{Colors.RESET}")
                continue
                
        # 3. Create a fresh, real ProjectCleaner directory
        os.makedirs(dest_plugin_dir, exist_ok=True)
            
        # 4. Symlink individual contents into the new directory
        success_count = 0
        for item in items_to_sync:
            src_item = os.path.join(source_plugin_dir, item)
            dest_item = os.path.join(dest_plugin_dir, item)
            
            # Using Git Bash's ln -s
            # Note: We use relative paths for the symlink target so the links remain valid 
            # even if the drive letter changes, but absolute paths work fine too.
            command = ["ln", "-s", src_item, dest_item]
            
            try:
                subprocess.run(command, check=True, capture_output=True, text=True)
                success_count += 1
            except subprocess.CalledProcessError as e:
                print(f"{Colors.RED}[{proj}] ERROR: Failed to create link for {item}.")
                print(f"     {e.stderr.strip()}{Colors.RESET}")

        print(f"{Colors.GREEN}[{proj}] Successfully synced {success_count} items (Excluded: {', '.join(EXCLUDED_ITEMS)}).{Colors.RESET}")

    print(f"\n{Colors.GREEN}Sync complete!{Colors.RESET}")

if __name__ == "__main__":
    main()
