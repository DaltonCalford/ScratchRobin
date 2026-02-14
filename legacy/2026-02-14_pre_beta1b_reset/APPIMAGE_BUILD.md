# ScratchRobin Static/AppImage Build Guide

This document describes how to build ScratchRobin as a portable, mostly-static executable suitable for AppImage distribution - perfect for IT disaster recovery scenarios where dependencies may be broken.

## Overview

The static build creates a self-contained executable that:
- Links wxWidgets statically (no libwx_gtk3u_core-3.2.so.0 dependency)
- Links C++ runtime statically (no libstdc++ version conflicts)
- Bundles all resources (translations, icons)
- Only depends on system GTK/X11 (for theme compatibility)

## Quick Start

### Option 1: Use the Build Script (Recommended)

```bash
cd /path/to/ScratchRobin
./tools/build_static.sh
```

This creates:
- `build-static/scratchrobin` - The static executable
- `build-static/AppDir/` - AppImage-ready directory structure

### Option 2: Manual CMake Configuration

```bash
mkdir build-static && cd build-static
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSCRATCHROBIN_STATIC_BUILD=ON \
    -DSCRATCHROBIN_STATIC_WX=ON \
    -DSCRATCHROBIN_STATIC_CXX=ON \
    -DSCRATCHROBIN_BUILD_TESTS=OFF

make -j$(nproc)
```

## Creating the AppImage

### Prerequisites

Download the AppImage tool:
```bash
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage
```

### Build AppImage

After running `build_static.sh`:

```bash
./appimagetool-x86_64.AppImage build-static/AppDir
```

This creates: `ScratchRobin-x86_64.AppImage`

### Testing the AppImage

```bash
# Make executable
chmod +x ScratchRobin-x86_64.AppImage

# Run
./ScratchRobin-x86_64.AppImage

# Or extract and run (for debugging)
./ScratchRobin-x86_64.AppImage --appimage-extract
squashfs-root/AppRun
```

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `SCRATCHROBIN_STATIC_BUILD` | Enable all static linking options | OFF |
| `SCRATCHROBIN_STATIC_WX` | Statically link wxWidgets | OFF |
| `SCRATCHROBIN_STATIC_CXX` | Statically link libstdc++ and libgcc | OFF |
| `SCRATCHROBIN_BUILD_TESTS` | Build test suite | ON |
| `SCRATCHROBIN_USE_LIBSECRET` | Enable libsecret integration | ON |

### Recommended Combinations

**Maximum portability (AppImage):**
```bash
cmake .. -DSCRATCHROBIN_STATIC_BUILD=ON -DSCRATCHROBIN_BUILD_TESTS=OFF
```

**Semi-static (development):**
```bash
cmake .. -DSCRATCHROBIN_STATIC_WX=ON -DSCRATCHROBIN_STATIC_CXX=OFF
```

**Fully static (specialized systems):**
```bash
cmake .. -DSCRATCHROBIN_STATIC_BUILD=ON -DSCRATCHROBIN_USE_LIBSECRET=OFF
```

## Resource Handling

The application now uses `ResourcePaths` utility to locate resources:

1. **Environment variable:** `SCRATCHROBIN_RESOURCES=/path/to/resources`
2. **AppImage layout:** `../share/scratchrobin/` relative to executable
3. **Development:** `./assets/` and `./translations/` in working directory
4. **Fallback:** Current working directory

This allows the same binary to work in:
- AppImage (read-only squashfs)
- Portable USB drive
- System installation (/usr/share/scratchrobin)
- Development tree

## Dependencies

### Fully Static Dependencies (Bundled)

- wxWidgets 3.2.x (core, base, aui, html)
- C++ Standard Library (libstdc++)
- GCC Runtime (libgcc)

### Dynamic Dependencies (System)

These remain dynamic for compatibility:

- GTK3 (libgtk-3.so.0) - Required for theming
- GLib (libglib-2.0.so.0) - Core system library
- X11/XCB - Graphics subsystem
- Database clients (optional):
  - libpq.so.5 (PostgreSQL)
  - libmysqlclient.so.21 (MySQL)
  - libfbclient.so.2 (Firebird)

### Checking Dependencies

```bash
# Check what libraries are still dynamic
ldd scratchrobin | grep -E "libwx|libstdc|libgcc"
# Should show nothing for fully static build

# Check required system libraries
ldd scratchrobin | grep -E "libgtk|libglib|libX"
# These are expected and OK
```

## Troubleshooting

### Binary Too Large

Static linking increases size (~20-30MB). To reduce:

```bash
# Strip debug symbols
strip scratchrobin

# Use UPX compression (optional)
upx --best scratchrobin
```

### GTK Theme Issues

If the app looks different from system apps:
- GTK must remain dynamic for theme support
- Ensure `gtk-theme-name` is properly set
- May need to bundle GTK theme in AppImage for older systems

### Database Drivers Not Found

Database client libraries (libpq, libmysqlclient) remain dynamic.
For fully portable AppImage:
- Include them in AppDir/usr/lib/
- Or use bundled SQLite-only mode

### wxWidgets Static Build

If your system's wxWidgets is only shared:

```bash
# Build static wxWidgets first
cd wxWidgets-3.2.9
mkdir build-static && cd build-static
../configure --disable-shared --enable-static
make -j$(nproc)
sudo make install

# Then build ScratchRobin
export wxWidgets_ROOT=/usr/local
cmake .. -DSCRATCHROBIN_STATIC_WX=ON
```

## File Sizes

| Build Type | Executable | AppImage |
|------------|-----------|----------|
| Dynamic (current) | ~86 MB | ~88 MB |
| Semi-static | ~105 MB | ~107 MB |
| Fully static | ~115 MB | ~117 MB |
| Stripped static | ~95 MB | ~97 MB |

## Distribution

### For IT Teams

1. **Single AppImage file** - Users download and run
2. **Portable USB** - Extract AppImage, copy AppDir to USB
3. **Network share** - Mount shared AppImage or extract once
4. **Emergency recovery** - Boot live ISO, run from /tmp

### Recommended Deployment

```bash
# Create versioned AppImage
mv ScratchRobin-x86_64.AppImage "ScratchRobin-0.1.0-x86_64.AppImage"

# For internal distribution
cp "ScratchRobin-0.1.0-x86_64.AppImage" /shared/tools/
ln -s "ScratchRobin-0.1.0-x86_64.AppImage" /shared/tools/scratchrobin
```

## CI/CD Integration

```yaml
# GitHub Actions example
- name: Build AppImage
  run: |
    ./tools/build_static.sh
    wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x appimagetool-x86_64.AppImage
    ./appimagetool-x86_64.AppImage build-static/AppDir
    
- name: Upload Artifact
  uses: actions/upload-artifact@v4
  with:
    name: ScratchRobin-AppImage
    path: ScratchRobin-x86_64.AppImage
```

## Summary

The static build provides:
- ✅ Single-file portability
- ✅ No dependency hell
- ✅ Works on broken systems
- ✅ Same binary for all Linux distros
- ✅ Easy emergency deployment

Perfect for IT professionals who need reliable database tools when everything else is failing.
