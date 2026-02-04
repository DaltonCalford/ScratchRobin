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
    # Application & Navigation (0-9)
    "scratchrobin"
    "connect"
    "disconnect"
    "execute"
    "stop"
    "commit"
    "rollback"
    "settings"
    # Database Objects (10-24)
    "table"
    "view"
    "index"
    "column"
    "sequence"
    "trigger"
    "procedure"
    "function"
    "package"
    "constraint"
    "domain"
    "collation"
    "tablespace"
    # Schema Organization (25-29)
    "schema"
    "folder"
    "database"
    "catalog"
    # Security & Admin (30-34)
    "users"
    # Project Objects (35-44)
    "sql"
    "note"
    "diagram"
    "project"
    "timeline"
    "job"
    # Version Control (45-49)
    "git"
    "branch"
    # Maintenance (50-54)
    "backup"
    "restore"
    # Infrastructure (60-69)
    "server"
    "client"
    "filespace"
    "network"
    "cluster"
    "instance"
    "replica"
    "shard"
    # Design & Planning (70-79)
    "whiteboard"
    "mindmap"
    "design"
    "draft"
    "template"
    "blueprint"
    "concept"
    "plan"
    # Design States (80-89)
    "implemented"
    "pending"
    "modified"
    "deleted"
    "newitem"
    # Synchronization (90-99)
    "sync"
    "diff"
    "compare"
    "migrate"
    "deploy"
    # Collaboration (100-109)
    "shared"
    "collaboration"
    "team"
    # Security & Audit (110-119)
    "lock"
    "unlock"
    "history"
    "audit"
    "tag"
    "bookmark"
    # Additional Features (125-130)
    "cube"
    "test"
    "lineage"
    "mask"
    "api"
    "stream"
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
