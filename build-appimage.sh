#!/bin/bash
#
# ScratchRobin AppImage Build Script
# Copyright (c) 2025-2026 Dalton Calford
#
# This script builds an AppImage for ScratchRobin
#

set -e

echo "======================================"
echo "ScratchRobin AppImage Builder"
echo "======================================"

# Configuration
APP_NAME="ScratchRobin"
APP_VERSION="0.1.0"
ARCH=$(uname -m)
BUILD_DIR="build-appimage"
APPDIR="AppDir"

# Check dependencies
check_dependency() {
    if ! command -v "$1" &> /dev/null; then
        echo "Error: $1 is required but not installed"
        exit 1
    fi
}

echo "Checking dependencies..."
check_dependency cmake
check_dependency make
check_dependency gcc
check_dependency g++

# Check for linuxdeploy
echo "Checking for linuxdeploy..."
if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy..."
    wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

# Check for linuxdeploy-plugin-qt
if [ ! -f "linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy-plugin-qt..."
    wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
fi

# Clean previous build
echo "Cleaning previous build..."
rm -rf "$BUILD_DIR" "$APPDIR"
mkdir -p "$BUILD_DIR"

# Configure and build
echo "Configuring build..."
cd "$BUILD_DIR"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DSCRATCHROBIN_BUILD_TESTS=OFF \
    -DWITH_SYSTEM_WXWIDGETS=OFF \
    -DWITH_STATIC_WXWIDGETS=ON

echo "Building ScratchRobin..."
make -j$(nproc)

cd ..

# Create AppDir structure
echo "Creating AppDir structure..."
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/metainfo"

# Copy binary
echo "Copying binary..."
cp "$BUILD_DIR/ScratchRobin" "$APPDIR/usr/bin/"

# Create desktop file
echo "Creating desktop file..."
cat > "$APPDIR/usr/share/applications/scratchrobin.desktop" << 'EOF'
[Desktop Entry]
Name=ScratchRobin
Comment=Advanced Database Administration Tool
Exec=scratchrobin
Icon=scratchrobin
Type=Application
Categories=Development;Database;
Keywords=database;sql;firebird;postgresql;mysql;
Terminal=false
MimeType=application/x-scratchrobin-project;
EOF

# Create AppRun symlink
ln -sf "usr/share/applications/scratchrobin.desktop" "$APPDIR/scratchrobin.desktop"

# Copy icon (if available)
if [ -f "resources/icons/scratchrobin.png" ]; then
    cp "resources/icons/scratchrobin.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/"
    ln -sf "usr/share/icons/hicolor/256x256/apps/scratchrobin.png" "$APPDIR/scratchrobin.png"
else
    # Create a simple icon placeholder
    echo "Warning: Icon not found, using placeholder"
    touch "$APPDIR/scratchrobin.png"
fi

# Create AppStream metadata
echo "Creating AppStream metadata..."
cat > "$APPDIR/usr/share/metainfo/scratchrobin.appdata.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<component type="desktop-application">
  <id>com.dcalford.scratchrobin</id>
  <name>ScratchRobin</name>
  <summary>Advanced Database Administration Tool</summary>
  <description>
    <p>
      ScratchRobin is a modern, multi-database administration tool supporting
      Firebird, PostgreSQL, MySQL, and the native ScratchBird protocol.
    </p>
    <p>Features:</p>
    <ul>
      <li>Multi-database connection support</li>
      <li>SQL editor with syntax highlighting</li>
      <li>Visual query builder</li>
      <li>Schema comparison and synchronization</li>
      <li>Data migration tools</li>
      <li>Backup and restore</li>
      <li>Performance monitoring</li>
      <li>Docker deployment management</li>
      <li>Collaboration features</li>
    </ul>
  </description>
  <metadata_license>MIT</metadata_license>
  <project_license>IDPL-1.0</project_license>
  <developer_name>Dalton Calford</developer_name>
  <url type="homepage">https://github.com/DaltonCalford/ScratchRobin</url>
  <url type="bugtracker">https://github.com/DaltonCalford/ScratchRobin/issues</url>
  <releases>
    <release version="$APP_VERSION" date="$(date +%Y-%m-%d)"/>
  </releases>
  <content_rating type="oars-1.1">
    <content_attribute id="social-info">mild</content_attribute>
  </content_rating>
</component>
EOF

# Copy wxWidgets libraries
echo "Copying wxWidgets libraries..."
if [ -d "wxwidgets-3.2.9-mingw64" ]; then
    # Static build - libraries already linked
    echo "Using static wxWidgets build"
else
    # Copy system wxWidgets libraries
    WX_LIBS=(
        libwx_baseu-3.2.so
        libwx_gtk3u_core-3.2.so
        libwx_gtk3u_aui-3.2.so
        libwx_gtk3u_html-3.2.so
        libwx_gtk3u_qa-3.2.so
        libwx_gtk3u_xrc-3.2.so
        libwx_gtk3u_stc-3.2.so
    )
    
    for lib in "${WX_LIBS[@]}"; do
        if [ -f "/usr/lib/x86_64-linux-gnu/$lib" ]; then
            cp "/usr/lib/x86_64-linux-gnu/$lib"* "$APPDIR/usr/lib/"
        elif [ -f "/usr/lib64/$lib" ]; then
            cp "/usr/lib64/$lib"* "$APPDIR/usr/lib/"
        fi
    done
fi

# Copy other dependencies
echo "Copying other dependencies..."
# libsecret for credential storage
for lib in libsecret-1.so libgcrypt.so; do
    if ldconfig -p | grep -q "$lib"; then
        lib_path=$(ldconfig -p | grep "$lib" | head -1 | awk '{print $NF}')
        if [ -n "$lib_path" ]; then
            cp "$lib_path"* "$APPDIR/usr/lib/" 2>/dev/null || true
        fi
    fi
done

# Create AppRun script
echo "Creating AppRun..."
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash

# AppRun for ScratchRobin
SELF=$(readlink -f "$0")
HERE=${SELF%/*}

# Set library path
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"

# Set GTK theme if not set
if [ -z "$GTK_THEME" ]; then
    export GTK_THEME=Adwaita:dark
fi

# Launch the application
exec "${HERE}/usr/bin/ScratchRobin" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Build AppImage
echo "Building AppImage..."
./linuxdeploy-x86_64.AppImage \
    --appdir "$APPDIR" \
    --plugin qt \
    --output appimage \
    -d "$APPDIR/usr/share/applications/scratchrobin.desktop" \
    -i "$APPDIR/usr/share/icons/hicolor/256x256/apps/scratchrobin.png"

# Rename AppImage with version
if [ -f "ScratchRobin-x86_64.AppImage" ]; then
    mv "ScratchRobin-x86_64.AppImage" "ScratchRobin-${APP_VERSION}-x86_64.AppImage"
    echo ""
    echo "======================================"
    echo "AppImage built successfully!"
    echo "Output: ScratchRobin-${APP_VERSION}-x86_64.AppImage"
    echo "======================================"
else
    echo "Error: AppImage creation failed"
    exit 1
fi

# Make executable
chmod +x "ScratchRobin-${APP_VERSION}-x86_64.AppImage"

# Create SHA256 checksum
echo "Creating checksum..."
sha256sum "ScratchRobin-${APP_VERSION}-x86_64.AppImage" > "ScratchRobin-${APP_VERSION}-x86_64.AppImage.sha256"

# Optional: Test the AppImage
if [ "$1" == "--test" ]; then
    echo ""
    echo "Testing AppImage..."
    ./"ScratchRobin-${APP_VERSION}-x86_64.AppImage" --version || true
fi

echo ""
echo "Build complete!"
