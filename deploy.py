#!/usr/bin/env python3
"""
Script tự động build ESP32 firmware và upload lên GitHub
Sử dụng: python auto_deploy.py 1.0.1
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

# ========== CẤU HÌNH ==========
ARDUINO_CLI = "arduino-cli"  # Hoặc đường dẫn đầy đủ
SKETCH_PATH = "."  # Thư mục chứa file .ino
BOARD_FQBN = "esp32:esp32:esp32"  # Board ESP32

GITHUB_REPO = "honghuynhit/esp32-power-monitor"  # Thay username của bạn
GITHUB_BRANCH = "main"

# File output
FIRMWARE_BIN = "firmware.bin"
VERSION_FILE = "version.txt"
# ================================

def print_header(text):
    """In header đẹp"""
    print("\n" + "="*50)
    print(f"  {text}")
    print("="*50)

def check_requirements():
    """Kiểm tra các tool cần thiết"""
    print_header("Kiểm tra môi trường")
    
    # Check arduino-cli
    try:
        result = subprocess.run([ARDUINO_CLI, "version"], 
                              capture_output=True, text=True)
        print(f"✓ Arduino CLI: {result.stdout.strip()}")
    except FileNotFoundError:
        print("✗ Arduino CLI không tìm thấy!")
        print("  Cài đặt: https://arduino.github.io/arduino-cli/")
        sys.exit(1)
    
    # Check git
    try:
        result = subprocess.run(["git", "--version"], 
                              capture_output=True, text=True)
        print(f"✓ Git: {result.stdout.strip()}")
    except FileNotFoundError:
        print("✗ Git không tìm thấy!")
        sys.exit(1)

def get_new_version():
    """Lấy version mới từ command line hoặc tự động tăng"""
    if len(sys.argv) > 1:
        return sys.argv[1]
    
    # Đọc version hiện tại
    if os.path.exists(VERSION_FILE):
        with open(VERSION_FILE, 'r') as f:
            current = f.read().strip()
        
        # Auto increment patch version
        parts = current.split('.')
        if len(parts) == 3:
            parts[2] = str(int(parts[2]) + 1)
            return '.'.join(parts)
    
    return "1.0.0"

def build_firmware():
    """Build firmware từ sketch"""
    print_header("Building Firmware")
    
    # Tìm file .ino
    ino_files = list(Path(SKETCH_PATH).glob("*.ino"))
    if not ino_files:
        print("✗ Không tìm thấy file .ino!")
        sys.exit(1)
    
    sketch = ino_files[0]
    print(f"Sketch: {sketch}")
    print(f"Board: {BOARD_FQBN}")
    
    # Build
    cmd = [
        ARDUINO_CLI, "compile",
        "--fqbn", BOARD_FQBN,
        "--output-dir", "build",
        str(sketch)
    ]
    
    print(f"Command: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode != 0:
        print("✗ Build thất bại!")
        print(result.stderr)
        sys.exit(1)
    
    print("✓ Build thành công!")
    
    # Tìm file .bin
    build_dir = Path("build")
    bin_files = list(build_dir.glob("*.bin"))
    
    if not bin_files:
        print("✗ Không tìm thấy file .bin!")
        sys.exit(1)
    
    # Copy firmware.bin ra thư mục gốc
    src_bin = bin_files[0]
    shutil.copy(src_bin, FIRMWARE_BIN)
    
    size = os.path.getsize(FIRMWARE_BIN)
    print(f"✓ Firmware: {FIRMWARE_BIN} ({size:,} bytes)")
    
    return True

def update_version_file(version):
    """Cập nhật file version.txt"""
    print_header("Updating Version")
    
    with open(VERSION_FILE, 'w') as f:
        f.write(version + '\n')
    
    print(f"✓ Version updated: {version}")

def git_commit_and_push(version):
    """Commit và push lên GitHub"""
    print_header("Deploying to GitHub")
    
    # Check git status
    result = subprocess.run(["git", "status", "--porcelain"], 
                          capture_output=True, text=True)
    
    if not result.stdout.strip():
        print("ℹ Không có thay đổi để commit")
        return
    
    # Add files
    files_to_add = [FIRMWARE_BIN, VERSION_FILE]
    for file in files_to_add:
        subprocess.run(["git", "add", file])
    
    print(f"✓ Added: {', '.join(files_to_add)}")
    
    # Commit
    commit_msg = f"Release version {version}"
    result = subprocess.run(
        ["git", "commit", "-m", commit_msg],
        capture_output=True, text=True
    )
    
    if result.returncode != 0:
        print("✗ Commit thất bại!")
        print(result.stderr)
        sys.exit(1)
    
    print(f"✓ Committed: {commit_msg}")
    
    # Push
    print(f"Pushing to {GITHUB_BRANCH}...")
    result = subprocess.run(
        ["git", "push", "origin", GITHUB_BRANCH],
        capture_output=True, text=True
    )
    
    if result.returncode != 0:
        print("✗ Push thất bại!")
        print(result.stderr)
        print("\nĐảm bảo bạn đã:")
        print("1. git remote add origin https://github.com/honghuynhit/esp32-power-monitor.git")
        print("2. git config credential.helper store")
        print("3. Có quyền push lên repo")
        sys.exit(1)
    
    print("✓ Pushed to GitHub!")
    
    # Print URLs
    print("\n" + "="*50)
    print("📦 Deployment successful!")
    print("="*50)
    print(f"Version: {version}")
    print(f"Repository: https://github.com/{GITHUB_REPO}")
    print(f"\nFirmware URL:")
    print(f"https://raw.githubusercontent.com/{GITHUB_REPO}/{GITHUB_BRANCH}/{FIRMWARE_BIN}")
    print(f"\nVersion URL:")
    print(f"https://raw.githubusercontent.com/{GITHUB_REPO}/{GITHUB_BRANCH}/{VERSION_FILE}")
    print("="*50)

def main():
    """Main function"""
    print("""
╔════════════════════════════════════════╗
║   ESP32 Firmware Auto Deploy Tool     ║
╚════════════════════════════════════════╝
    """)
    
    # Check requirements
    check_requirements()
    
    # Get version
    new_version = get_new_version()
    print(f"\n🔖 New Version: {new_version}")
    
    # Confirm
    response = input("\nContinue? (y/N): ").strip().lower()
    if response != 'y':
        print("Cancelled.")
        sys.exit(0)
    
    # Build firmware
    build_firmware()
    
    # Update version file
    update_version_file(new_version)
    
    # Deploy to GitHub
    git_commit_and_push(new_version)
    
    print("\n✅ All done! ESP32 sẽ tự động update trong 6 giờ tới.")

if __name__ == "__main__":
    main()