# ScratchRobin i18n Tools

Automated internationalization (i18n) tools for ScratchRobin. These tools extract hardcoded English strings from the C++ source code, translate them to multiple languages, and update the source code to use the translation system.

## Supported Languages

| Language | Locale Code | Status |
|----------|-------------|--------|
| English (Canadian) | `en_CA` | ✅ Complete |
| French (Canadian) | `fr_CA` | ✅ Complete |
| Spanish | `es_ES` | ✅ Complete |
| Portuguese | `pt_PT` | ⚠️ Beta |
| German | `de_DE` | ⚠️ Beta |
| Italian | `it_IT` | ⚠️ Beta |

## Quick Start

```bash
cd /path/to/ScratchRobin

# 1. Check current i18n status (dry run)
python tools/i18n/i18n_manager.py --source src

# 2. Review the generated report
cat i18n_update_report.md

# 3. Apply changes to source code
python tools/i18n/i18n_manager.py --source src --apply
```

## Tools Overview

### 1. i18n_manager.py (Master Orchestrator)

Runs the complete i18n workflow in one command.

```bash
# Full workflow with mock translations (for testing)
python tools/i18n/i18n_manager.py --source src

# Use local LLM for better translations (requires Ollama)
python tools/i18n/i18n_manager.py --source src --backend ollama --ollama-model llama3.2

# Apply changes to source code
python tools/i18n/i18n_manager.py --source src --apply
```

### 2. i18n_extractor.py (String Extraction)

Extracts hardcoded English strings from C++ source files.

```bash
# Extract strings to JSON
python tools/i18n/i18n_extractor.py \
    --source src \
    --output translations/extracted_strings.json \
    --template translations/template.json
```

Extracts from:
- `wxString("text")` literals
- `SetLabel()`, `SetTitle()`, `SetToolTip()` calls
- Menu `Append()` calls
- Notebook `AddPage()` calls
- List column headers
- Button labels
- Message box text

### 3. i18n_translator.py (Translation)

Translates strings using various backends.

#### Backends

**Mock (Default)** - Returns placeholder text for testing
```bash
python tools/i18n/i18n_translator.py batch --backend mock
```

**Ollama (Recommended)** - Uses local LLM for quality translations
```bash
# Install Ollama first: https://ollama.com
ollama pull llama3.2

# Run translation
python tools/i18n/i18n_translator.py batch --backend ollama --ollama-model llama3.2
```

**LibreTranslate** - Uses public or self-hosted LibreTranslate API
```bash
python tools/i18n/i18n_translator.py batch --backend libretranslate
```

#### Commands

```bash
# Create new translation file for a language
python tools/i18n/i18n_translator.py translate \
    --source translations/en_CA.json \
    --target fr_CA \
    --output translations/fr_CA.json \
    --backend ollama

# Update existing translation with new keys
python tools/i18n/i18n_translator.py update \
    --source translations/en_CA.json \
    --target fr_CA \
    --backend ollama

# Batch update all languages
python tools/i18n/i18n_translator.py batch \
    --source translations/en_CA.json \
    --translations-dir translations \
    --backend ollama
```

### 4. i18n_code_updater.py (Source Code Update)

Replaces hardcoded strings with `_()` macro calls.

```bash
# Dry run - see what would change
python tools/i18n/i18n_code_updater.py \
    --source src \
    --translations translations/en_CA.json

# Apply changes (creates .bak backups)
python tools/i18n/i18n_code_updater.py \
    --source src \
    --translations translations/en_CA.json \
    --apply
```

## Translation File Format

Translation files are JSON with the following structure:

```json
{
  "version": "1.0.0",
  "language": "French (Canadian)",
  "locale": "fr_CA",
  "translator": "Auto-translated via Ollama",
  "translations": {
    "app.name": "ScratchRobin",
    "menu.file": "Fichier",
    "menu.file.new": "Nouveau",
    "button.save": "Enregistrer",
    "dialog.confirm": "Êtes-vous certain?"
  }
}
```

## How It Works

1. **Extraction**: The extractor scans C++ source files for UI string patterns and generates a template with all strings mapped to translation keys.

2. **Translation**: The translator reads the English template and uses the selected backend to translate each string to target languages. Results are cached to avoid re-translating.

3. **Code Update**: The updater reads the translation file and replaces hardcoded strings in source code with `_("key")` macro calls.

## Best Practices

1. **Always run dry-run first** - Review `i18n_update_report.md` before applying changes
2. **Use version control** - Commit before running `--apply` so you can revert if needed
3. **Test after applying** - Build and run the application to verify changes
4. **Review translations** - Machine translations may need manual review for context
5. **Keep translations in sync** - Run the update workflow when adding new UI strings

## Manual Translation Workflow

If you prefer to translate manually:

```bash
# 1. Extract strings
python tools/i18n/i18n_extractor.py --template translations/template.json

# 2. Copy template to new language
cp translations/template.json translations/your_lang.json

# 3. Edit your_lang.json and translate all values

# 4. Update source code
python tools/i18n/i18n_code_updater.py --source src --translations translations/en_CA.json --apply
```

## Adding a New Language

1. Add the language to `LANGUAGES` dict in `i18n_translator.py`:
```python
LANGUAGES = {
    'fr_CA': {'name': 'French (Canadian)', 'locale': 'fr_CA', 'deepl_code': 'FR'},
    # Add yours:
    'ja_JP': {'name': 'Japanese', 'locale': 'ja_JP', 'deepl_code': 'JA'},
}
```

2. Run batch translation:
```bash
python tools/i18n/i18n_translator.py batch --backend ollama
```

3. The new language file will be created automatically.

## Troubleshooting

### "No replacements found"
- Ensure translation keys match the hardcoded strings exactly
- Check that strings aren't already using `_()` macro

### Translation quality issues
- Use Ollama with a larger model (e.g., `llama3.3`, `mistral`)
- Manually review and edit translation files
- Add context comments in source code

### Build errors after applying
- Ensure `#include "i18n/localization_manager.h"` is present
- Check that the `_()` macro is defined
- Verify translation keys exist in JSON files

## Integration with Build System

Add to your CI/CD pipeline:

```yaml
# Example GitHub Actions step
- name: Check i18n completeness
  run: |
    python tools/i18n/i18n_extractor.py --source src --template /tmp/template.json
    python tools/i18n/i18n_translator.py batch --backend mock
    # Check for missing translations
    python tools/i18n/check_translations.py
```
