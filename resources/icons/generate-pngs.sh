#!/bin/bash
# Generate PNG icons from SVG sources at various sizes

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check for ImageMagick's convert or rsvg-convert
if command -v rsvg-convert &> /dev/null; then
    CONVERT_CMD="rsvg-convert"
    CONVERT_OPTS=""
elif command -v convert &> /dev/null; then
    CONVERT_CMD="convert"
    CONVERT_OPTS=""
else
    echo "Error: No SVG converter found. Install librsvg (rsvg-convert) or ImageMagick."
    exit 1
fi

# Sizes to generate
SIZES=(16 24 32 48 64 128 256)

# Icons to process
ICONS=(
    "scratchrobin"
    "connect"
    "disconnect"
    "execute"
    "stop"
    "commit"
    "rollback"
    "table"
    "view"
    "diagram"
    "index"
    "users"
    "settings"
)

echo "Generating PNG icons from SVG sources..."

for icon in "${ICONS[@]}"; do
    svg_file="${icon}.svg"
    
    if [ ! -f "$svg_file" ]; then
        echo "Warning: $svg_file not found, skipping..."
        continue
    fi
    
    echo "Processing $svg_file..."
    
    for size in "${SIZES[@]}"; do
        png_file="${icon}@${size}.png"
        
        if [ "$CONVERT_CMD" = "rsvg-convert" ]; then
            rsvg-convert -w "$size" -h "$size" "$svg_file" -o "$png_file"
        else
            convert "$svg_file" -resize "${size}x${size}" "$png_file"
        fi
        
        echo "  Generated $png_file"
    done
done

echo "Done!"
