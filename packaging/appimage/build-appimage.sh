#!/bin/bash
# Build ScratchRobin AppImage

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-appimage"
APPIMAGE_DIR="$BUILD_DIR/appimage"

# Clean previous builds
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR" "$APPIMAGE_DIR"

# Build ScratchRobin
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DSCRATCHROBIN_USE_LIBSECRET=ON \
    -DSCRATCHROBIN_USE_LIBPQ=ON \
    -DSCRATCHROBIN_USE_MYSQL=ON

cmake --build "$BUILD_DIR" -j$(nproc)

# Install to AppDir
DESTDIR="$APPIMAGE_DIR" cmake --install "$BUILD_DIR"

# Create AppRun
cat > "$APPIMAGE_DIR/AppRun" << 'EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
exec "${HERE}/usr/bin/scratchrobin" "$@"
EOF
chmod +x "$APPIMAGE_DIR/AppRun"

# Create desktop file
cat > "$APPIMAGE_DIR/scratchrobin.desktop" << 'EOF'
[Desktop Entry]
Name=ScratchRobin
Exec=scratchrobin
Icon=scratchrobin
Type=Application
Categories=Development;Database;
Comment=Database administration tool for ScratchBird
EOF

# Copy icon
mkdir -p "$APPIMAGE_DIR/usr/share/pixmaps"
cp "$PROJECT_ROOT/resources/icons/scratchrobin.png" \
    "$APPIMAGE_DIR/scratchrobin.png" 2>/dev/null || \
    touch "$APPIMAGE_DIR/scratchrobin.png"

# Download linuxdeploy if not present
LINUXDEPLOY="$BUILD_DIR/linuxdeploy-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOY" ]; then
    wget -q -O "$LINUXDEPLOY" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
fi

# Build AppImage
OUTPUT="$PROJECT_ROOT/ScratchRobin-1.0.0-x86_64.AppImage"
"$LINUXDEPLOY" --appdir "$APPIMAGE_DIR" --output appimage -o "$OUTPUT"

echo "AppImage created: $OUTPUT"
