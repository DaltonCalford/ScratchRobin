#!/usr/bin/env python3
"""
i18n Manager for ScratchRobin
Master script to orchestrate the complete internationalization workflow.
"""

import argparse
import subprocess
import sys
from pathlib import Path


SCRIPTS_DIR = Path(__file__).parent


def run_extractor(source_dir: Path, output_dir: Path):
    """Run string extraction."""
    print("=" * 60)
    print("STEP 1: Extracting strings from source code")
    print("=" * 60)
    
    cmd = [
        sys.executable, str(SCRIPTS_DIR / 'i18n_extractor.py'),
        '-s', str(source_dir),
        '-o', str(output_dir / 'extracted_strings.json'),
        '-t', str(output_dir / 'translation_template.json')
    ]
    
    result = subprocess.run(cmd, capture_output=False)
    return result.returncode == 0


def run_translator(source_file: Path, translations_dir: Path, backend: str = 'mock'):
    """Run translation for all languages."""
    print("\n" + "=" * 60)
    print("STEP 2: Translating strings")
    print("=" * 60)
    
    cmd = [
        sys.executable, str(SCRIPTS_DIR / 'i18n_translator.py'),
        'batch',
        '-s', str(source_file),
        '--translations-dir', str(translations_dir),
        '-b', backend
    ]
    
    result = subprocess.run(cmd, capture_output=False)
    return result.returncode == 0


def run_updater(source_dir: Path, translations_file: Path, apply: bool = False):
    """Run code updater."""
    print("\n" + "=" * 60)
    print("STEP 3: Updating source code")
    print("=" * 60)
    
    cmd = [
        sys.executable, str(SCRIPTS_DIR / 'i18n_code_updater.py'),
        '-s', str(source_dir),
        '-t', str(translations_file),
        '-r', 'i18n_update_report.md'
    ]
    
    if apply:
        cmd.append('--apply')
    
    result = subprocess.run(cmd, capture_output=False)
    return result.returncode == 0


def run_full_workflow(args):
    """Run the complete i18n workflow."""
    source_dir = Path(args.source).resolve()
    translations_dir = Path(args.translations_dir).resolve()
    translations_dir.mkdir(parents=True, exist_ok=True)
    
    # Step 1: Extract
    if not run_extractor(source_dir, translations_dir):
        print("ERROR: Extraction failed")
        return 1
    
    # Step 2: Translate
    source_file = translations_dir / 'translation_template.json'
    if source_file.exists() and not args.skip_translate:
        if not run_translator(source_file, translations_dir, args.backend):
            print("WARNING: Translation had issues, continuing...")
    
    # Step 3: Update code
    english_file = translations_dir / 'en_CA.json'
    if english_file.exists() and not args.skip_update:
        if not run_updater(source_dir, english_file, args.apply):
            print("ERROR: Code update failed")
            return 1
    
    print("\n" + "=" * 60)
    print("i18n workflow complete!")
    print("=" * 60)
    
    if not args.apply:
        print("\nThis was a dry run. Review i18n_update_report.md and then run with --apply")
        print("to apply the changes to source code.")
    
    return 0


def main():
    parser = argparse.ArgumentParser(
        description='i18n Manager for ScratchRobin - Complete internationalization workflow',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Workflow:
  1. Extract hardcoded English strings from C++ source files
  2. Translate strings to all supported languages
  3. Update source code to use _() macro instead of hardcoded strings

Examples:
  # Dry run (recommended first step)
  python i18n_manager.py --source src

  # Apply changes to source code
  python i18n_manager.py --source src --apply

  # Use local LLM for translation (requires Ollama)
  python i18n_manager.py --source src --backend ollama --ollama-model llama3.2

  # Skip translation step (use existing translation files)
  python i18n_manager.py --source src --skip-translate
        """
    )
    
    parser.add_argument('--source', type=Path, default=Path('src'),
                        help='Source directory (default: src)')
    parser.add_argument('--translations-dir', type=Path, default=Path('translations'),
                        help='Directory for translation files (default: translations)')
    parser.add_argument('--apply', action='store_true',
                        help='Apply changes to source code (default is dry-run)')
    parser.add_argument('--backend', choices=['mock', 'libretranslate', 'ollama'],
                        default='mock', help='Translation backend (default: mock)')
    parser.add_argument('--ollama-model', default='llama3.2',
                        help='Ollama model for translation (default: llama3.2)')
    parser.add_argument('--skip-translate', action='store_true',
                        help='Skip translation step')
    parser.add_argument('--skip-update', action='store_true',
                        help='Skip code update step')
    parser.add_argument('--check', action='store_true',
                        help='Check i18n status without making changes')
    
    args = parser.parse_args()
    
    if args.check:
        # Just run extraction and report
        return run_extractor(args.source, Path(args.translations_dir))
    
    return run_full_workflow(args)


if __name__ == '__main__':
    sys.exit(main())
