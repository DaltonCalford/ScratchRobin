# Installation and Setup

## System Requirements

### Minimum Requirements
- **OS**: Linux (Ubuntu 20.04+), Windows 10+, macOS 12+
- **CPU**: x86_64, ARM64
- **RAM**: 4 GB
- **Disk**: 500 MB free space

### Recommended
- **RAM**: 8 GB or more
- **Display**: 1920x1080 or higher
- **For large diagrams**: 16 GB RAM recommended

## Installation

### Linux

#### Debian/Ubuntu (.deb package)
```bash
sudo dpkg -i scratchrobin_1.0.0_amd64.deb
sudo apt-get install -f  # Install dependencies
```

#### Fedora/RHEL (.rpm package)
```bash
sudo rpm -i scratchrobin-1.0.0.x86_64.rpm
```

#### AppImage (Universal)
```bash
chmod +x ScratchRobin-1.0.0-x86_64.AppImage
./ScratchRobin-1.0.0-x86_64.AppImage
```

To integrate with system:
```bash
./ScratchRobin-1.0.0-x86_64.AppImage --appimage-extract-and-run
```

#### Build from Source
See [BUILD.md](../BUILD.md) for detailed instructions.

### Windows

#### Installer (.msi or .exe)
1. Download `ScratchRobin-1.0.0-win64.exe`
2. Run the installer
3. Follow setup wizard
4. Launch from Start Menu or Desktop

#### Portable
1. Download `ScratchRobin-1.0.0-win64-portable.zip`
2. Extract to desired location
3. Run `scratchrobin.exe`

### macOS

#### DMG Package
1. Download `ScratchRobin-1.0.0.dmg`
2. Open the DMG
3. Drag ScratchRobin to Applications
4. Launch from Applications folder

**Note**: On first launch, you may need to right-click and select "Open" to bypass Gatekeeper.

#### Homebrew (Coming Soon)
```bash
brew install scratchrobin
```

## First Launch

On first launch, ScratchRobin will:
1. Create configuration directory (`~/.config/scratchrobin/`)
2. Set up default preferences
3. Create sample connection profile

## Post-Installation

### Configure Database Connections

1. Launch ScratchRobin
2. Go to **Window → Connections** or press **Ctrl+Shift+C**
3. Click **New** to create a connection profile
4. Select your connection mode (Embedded, IPC, or Network)
5. Enter connection details
6. Click **Test Connection**
7. Click **Save**

### Optional: Install System Integration

#### Linux Desktop Entry
For AppImage or portable installs:
```bash
# Create desktop entry
cat > ~/.local/share/applications/scratchrobin.desktop << EOF
[Desktop Entry]
Name=ScratchRobin
Exec=/path/to/scratchrobin
Icon=/path/to/scratchrobin.png
Type=Application
Categories=Development;Database;
EOF
```

#### Windows File Associations
During installation, you can optionally associate `.sberd` (ScratchRobin ERD files) with ScratchRobin.

## Updating

### Automatic Updates
ScratchRobin checks for updates on startup (can be disabled in preferences).

### Manual Update
Download the latest version from the website and install over the existing installation. Your settings and connections will be preserved.

## Uninstallation

### Linux
```bash
# Debian/Ubuntu
sudo apt remove scratchrobin

# Fedora/RHEL
sudo rpm -e scratchrobin

# AppImage
# Just delete the file
```

### Windows
Use "Add or Remove Programs" in Settings, or run the uninstaller from the installation directory.

### macOS
Drag ScratchRobin from Applications to Trash.

## Getting Help

- [Connection Troubleshooting](troubleshooting.md)
- [FAQ](../FAQ.md)
- [Report Issues](https://github.com/DaltonCalford/ScratchRobin/issues)
