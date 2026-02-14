# ScratchRobin Packaging

This directory contains packaging configuration for various platforms.

## Directory Structure

```
packaging/
├── debian/          # Debian/Ubuntu packaging
├── rpm/             # Fedora/RHEL packaging
├── appimage/        # Universal Linux AppImage
├── windows/         # Windows installer (NSIS)
└── macos/           # macOS DMG bundle
```

## Building Packages

### Debian/Ubuntu

```bash
cd packaging/debian
dpkg-buildpackage -us -uc -b
```

### Fedora/RHEL

```bash
cd packaging/rpm
rpmbuild -ba scratchrobin.spec
```

### AppImage

```bash
cd packaging/appimage
./build-appimage.sh
```

### Windows

Requires NSIS to be installed:

```bash
cd packaging/windows
makensis installer.nsi
```

### macOS

```bash
cd packaging/macos
./build-dmg.sh
```

## CI/CD Integration

See `.github/workflows/release.yml` for automated packaging on release.
