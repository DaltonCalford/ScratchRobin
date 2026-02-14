#!/bin/bash
# Build ScratchRobin as a static/portable executable for AppImage
# Usage: ./tools/build_static.sh [build_dir]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$PROJECT_DIR/build-static}"
INSTALL_DIR="$BUILD_DIR/AppDir"

echo "=========================================="
echo "ScratchRobin Static Build for AppImage"
echo "=========================================="
echo ""
echo "Build directory: $BUILD_DIR"
echo "Install directory: $INSTALL_DIR"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with static options
echo "Configuring CMake..."
cmake "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DSCRATCHROBIN_STATIC_BUILD=ON \
    -DSCRATCHROBIN_STATIC_WX=ON \
    -DSCRATCHROBIN_STATIC_CXX=ON \
    -DSCRATCHROBIN_BUILD_TESTS=OFF \
    -DSCRATCHROBIN_USE_LIBSECRET=OFF \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++ -Wl,--as-needed"

# Build
echo ""
echo "Building..."
cmake --build . --parallel $(nproc)

# Create AppDir structure for AppImage
echo ""
echo "Creating AppDir structure..."
mkdir -p "$INSTALL_DIR/usr/bin"
mkdir -p "$INSTALL_DIR/usr/lib"
mkdir -p "$INSTALL_DIR/usr/share/applications"
mkdir -p "$INSTALL_DIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$INSTALL_DIR/usr/share/scratchrobin/translations"
mkdir -p "$INSTALL_DIR/usr/share/scratchrobin/assets/icons"

# Copy executable
cp scratchrobin "$INSTALL_DIR/usr/bin/"

# Copy resources
cp -r "$PROJECT_DIR/translations/"* "$INSTALL_DIR/usr/share/scratchrobin/translations/"
cp -r "$PROJECT_DIR/assets/icons/"* "$INSTALL_DIR/usr/share/scratchrobin/assets/icons/" 2>/dev/null || true

# Create desktop file
cat > "$INSTALL_DIR/usr/share/applications/scratchrobin.desktop" << 'EOF'
[Desktop Entry]
Name=ScratchRobin
Comment=Database Designer & Administrator
Exec=scratchrobin
Icon=scratchrobin
Type=Application
Categories=Development;Database;
Terminal=false
StartupNotify=true
EOF

# Create a wrapper script that sets up environment
cat > "$INSTALL_DIR/usr/bin/scratchrobin-wrapper" << 'EOF'
#!/bin/bash
# AppImage wrapper for ScratchRobin

# Get the directory where this script is located
APPDIR="$(dirname "$(readlink -f "$0")")/../.."

# Set environment for resources
export SCRATCHROBIN_RESOURCES="$APPDIR/usr/share/scratchrobin"

# Launch the application
exec "$APPDIR/usr/bin/scratchrobin" "$@"
EOF
chmod +x "$INSTALL_DIR/usr/bin/scratchrobin-wrapper"

# Copy icon if available
if [ -f "$PROJECT_DIR/assets/icons/scratchrobin@256.png" ]; then
    cp "$PROJECT_DIR/assets/icons/scratchrobin@256.png" "$INSTALL_DIR/usr/share/icons/hicolor/256x256/apps/scratchrobin.png"
elif [ -f "$PROJECT_DIR/../ScratchBird/docs/Artwork/ScratchRobin.png" ]; then
    cp "$PROJECT_DIR/../ScratchBird/docs/Artwork/ScratchRobin.png" "$INSTALL_DIR/usr/share/icons/hicolor/256x256/apps/scratchrobin.png"
fi

# Create AppRun script (required for AppImage)
cat > "$INSTALL_DIR/AppRun" << 'EOF'
#!/bin/bash
# AppRun - Entry point for AppImage

SELF=$(readlink -f "$0")
HERE=${SELF%/*}

export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
export SCRATCHROBIN_RESOURCES="$HERE/usr/share/scratchrobin"

# Launch
exec "$HERE/usr/bin/scratchrobin" "$@"
EOF
chmod +x "$INSTALL_DIR/AppRun"

# Symlink desktop file and icon at AppDir root for AppImage tools
ln -sf usr/share/applications/scratchrobin.desktop "$INSTALL_DIR/"
ln -sf usr/share/icons/hicolor/256x256/apps/scratchrobin.png "$INSTALL_DIR/" 2>/dev/null || true

# Check executable dependencies
echo ""
echo "Checking executable dependencies..."
echo "Note: For a truly portable AppImage, these should mostly be system libs:"
echo ""
ldd "$INSTALL_DIR/usr/bin/scratchrobin" | grep -v "linux-vdso\|/lib64/ld-linux" | head -30

echo ""
echo "=========================================="
echo "Build complete!"
echo "=========================================="
echo ""
echo "AppDir location: $INSTALL_DIR"
echo ""
echo "To create the AppImage:"
echo "  1. Download appimagetool:"
echo "     wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
echo "  2. Run:"
echo "     ./appimagetool-x86_64.AppImage $INSTALL_DIR"
echo ""
echo "To test without AppImage packaging:"
echo "  $INSTALL_DIR/AppRun"
echo ""
echo "Optional: Strip debug symbols to reduce size:"
echo "  strip $INSTALL_DIR/usr/bin/scratchrobin"
echo ""
