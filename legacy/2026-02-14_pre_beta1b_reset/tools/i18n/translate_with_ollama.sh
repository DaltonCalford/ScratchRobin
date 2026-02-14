#!/bin/bash
# translate_with_ollama.sh - High-quality translation using local LLM

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
MODEL="${1:-llama3.2}"

echo "==================================="
echo "i18n Translation with Ollama"
echo "Model: $MODEL"
echo "==================================="

# Check if Ollama is installed
if ! command -v ollama &> /dev/null; then
    echo "Error: Ollama is not installed"
    echo "Please install from: https://ollama.com"
    exit 1
fi

# Check if model is available
if ! ollama list | grep -q "$MODEL"; then
    echo "Model $MODEL not found. Pulling..."
    ollama pull "$MODEL"
fi

# Run batch translation
cd "$PROJECT_DIR"
python3 "$SCRIPT_DIR/i18n_translator.py" batch \
    --source translations/en_CA.json \
    --translations-dir translations \
    --backend ollama \
    --ollama-model "$MODEL"

echo ""
echo "==================================="
echo "Translation complete!"
echo "Review the translation files in: translations/"
echo "==================================="
