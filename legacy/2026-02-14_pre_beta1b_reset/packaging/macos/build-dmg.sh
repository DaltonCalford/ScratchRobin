#!/bin/bash
# Build ScratchRobin macOS DMG

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-macos"
APP_BUNDLE="$BUILD_DIR/ScratchRobin.app"
DMG_NAME="ScratchRobin-1.0.0.dmg"
VOLUME_NAME="ScratchRobin"

# Clean previous builds
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Build ScratchRobin
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DSCRATCHROBIN_USE_LIBSECRET=OFF \
    -DSCRATCHROBIN_USE_LIBPQ=OFF \
    -DSCRATCHROBIN_USE_MYSQL=OFF

cmake --build "$BUILD_DIR" -j$(sysctl -n hw.ncpu)

# Create app bundle structure
mkdir -p "$APP_BUNDLE/Contents/MacOS"
mkdir -p "$APP_BUNDLE/Contents/Resources"
mkdir -p "$APP_BUNDLE/Contents/Frameworks"

# Copy executable
cp "$BUILD_DIR/scratchrobin" "$APP_BUNDLE/Contents/MacOS/"

# Create Info.plist
cat > "$APP_BUNDLE/Contents/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
    <key>CFBundleExecutable</key>
    <string>scratchrobin</string>
    <key>CFBundleIdentifier</key>
    <string>ca.stacktrace.scratchrobin</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>ScratchRobin</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>12.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeName</key>
            <string>ScratchRobin ERD Document</string>
            <key>CFBundleTypeRole</key>
            <string>Editor</string>
            <key>CFBundleTypeExtensions</key>
            <array>
                <string>sberd</string>
            </array>
        </dict>
    </array>
</dict>
</plist>
EOF

# Copy icon (if exists)
ICON_PATH="$PROJECT_ROOT/resources/icons/scratchrobin.icns"
if [ -f "$ICON_PATH" ]; then
    cp "$ICON_PATH" "$APP_BUNDLE/Contents/Resources/"
fi

# Sign the app (if certificate available)
if [ -n "$APPLE_DEVELOPER_ID" ]; then
    codesign --force --deep --sign "$APPLE_DEVELOPER_ID" "$APP_BUNDLE"
fi

# Create DMG
TEMP_DMG="$BUILD_DIR/temp.dmg"
STAGING_DIR="$BUILD_DIR/staging"

rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR"

cp -R "$APP_BUNDLE" "$STAGING_DIR/"

# Create symbolic link to Applications
ln -s /Applications "$STAGING_DIR/Applications"

# Create the DMG
hdiutil create -srcfolder "$STAGING_DIR" -volname "$VOLUME_NAME" \
    -fs HFS+ -format UDRW -o "$TEMP_DMG"

# Convert to compressed read-only DMG
rm -f "$PROJECT_ROOT/$DMG_NAME"
hdiutil convert "$TEMP_DMG" -format UDZO -o "$PROJECT_ROOT/$DMG_NAME"

# Clean up
rm -rf "$STAGING_DIR" "$TEMP_DMG"

echo "DMG created: $PROJECT_ROOT/$DMG_NAME"
