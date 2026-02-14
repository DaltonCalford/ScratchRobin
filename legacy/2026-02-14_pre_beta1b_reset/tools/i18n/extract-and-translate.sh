#!/bin/bash
# extract-and-translate.sh - Complete i18n workflow

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BACKEND="${1:-mock}"

echo "==================================="
echo "ScratchRobin i18n Workflow"
echo "Backend: $BACKEND"
echo "==================================="

cd "$PROJECT_DIR"

# Step 1: Extract strings
echo ""
echo "Step 1: Extracting strings from source code..."
python3 "$SCRIPT_DIR/i18n_extractor.py" \
    --source src \
    --output translations/extracted_strings.json \
    --template translations/template.json

# Step 2: Update English template
echo ""
echo "Step 2: Merging with existing translations..."
python3 -c "
import json
from pathlib import Path

# Load extracted template
with open('translations/template.json', 'r') as f:
    template = json.load(f)

# Merge with existing en_CA.json
en_file = Path('translations/en_CA.json')
if en_file.exists():
    with open(en_file, 'r') as f:
        existing = json.load(f)
    merged = existing['translations'].copy()
    merged.update(template['translations'])
else:
    merged = template['translations']

updated = {
    'version': '1.0.0',
    'language': 'English (Canadian)',
    'locale': 'en_CA',
    'translator': 'ScratchRobin Team',
    'translations': merged
}

with open(en_file, 'w', encoding='utf-8') as f:
    json.dump(updated, f, indent=2, ensure_ascii=False)

print(f'Updated {en_file} with {len(merged)} strings')
"

# Step 3: Translate to other languages
echo ""
echo "Step 3: Translating to other languages..."
python3 "$SCRIPT_DIR/i18n_translator.py" batch \
    --source translations/en_CA.json \
    --translations-dir translations \
    --backend "$BACKEND"

# Step 4: Report
echo ""
echo "==================================="
echo "i18n Workflow Complete!"
echo "==================================="
echo ""
echo "Files updated:"
for file in translations/*.json; do
    count=$(python3 -c "import json; print(len(json.load(open('$file'))['translations']))")
    echo "  - $(basename $file): $count strings"
done

echo ""
echo "Next steps:"
if [ "$BACKEND" = "mock" ]; then
    echo "  1. Review translations (currently using mock/placeholder text)"
    echo "  2. Run with Ollama for real translations:"
    echo "     ./tools/i18n/translate_with_ollama.sh"
else
    echo "  1. Review translation quality"
    echo "  2. Build and test the application"
fi
