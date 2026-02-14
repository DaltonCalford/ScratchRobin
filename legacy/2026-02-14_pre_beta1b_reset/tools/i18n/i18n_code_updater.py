#!/usr/bin/env python3
"""
i18n Code Updater for ScratchRobin
Replaces hardcoded English strings with _() macro calls.
"""

import re
import json
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
import shutil


@dataclass
class Replacement:
    """A string replacement to make."""
    original: str
    replacement: str
    line: int
    key: str
    context: str


class CodeUpdater:
    """Updates C++ source files to use i18n macros."""
    
    def __init__(self, translation_file: Path, dry_run: bool = True):
        self.dry_run = dry_run
        self.translations: Dict[str, str] = {}
        self.reverse_map: Dict[str, str] = {}  # text -> key
        
        # Load translations
        with open(translation_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
            self.translations = data.get('translations', {})
            # Build reverse map for lookup
            for key, text in self.translations.items():
                if text not in self.reverse_map:
                    self.reverse_map[text] = key
                    
        print(f"Loaded {len(self.translations)} translation keys")
        
    def find_translation_key(self, text: str) -> Optional[str]:
        """Find the translation key for a given text."""
        # Direct lookup
        if text in self.reverse_map:
            return self.reverse_map[text]
            
        # Fuzzy match - case insensitive
        text_lower = text.lower()
        for key, trans_text in self.translations.items():
            if trans_text.lower() == text_lower:
                return key
                
        return None
    
    def process_file(self, filepath: Path) -> List[Replacement]:
        """Process a single file and find replacements."""
        replacements = []
        
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            return replacements
        
        # Patterns to search for
        patterns = [
            # wxString with literal
            (re.compile(r'wxString\s*\(\s*"([^"]+)"\s*\)'), 'wxString(_("{key}"))'),
            
            # SetLabel with literal
            (re.compile(r'(SetLabel|SetTitle|SetToolTip|SetStatusText)\s*\(\s*"([^"]+)"\s*\)'), 
             '{method}(_("{key}"))'),
            
            # Menu Append - keep ID, replace label
            (re.compile(r'(Append)\s*\(\s*([^,]+)\s*,\s*"([^"]+)"'), 
             '{method}({id}, _("{key}")'),
             
            # Notebook AddPage
            (re.compile(r'(AddPage)\s*\(\s*([^,]+)\s*,\s*"([^"]+)"'),
             '{method}({page}, _("{key}")'),
        ]
        
        for line_num, line in enumerate(lines, 1):
            # Skip lines that already use _()
            if '_(' in line or 'wxGetTranslation' in line:
                continue
                
            # Pattern 1: wxString("...")
            for match in re.finditer(r'wxString\s*\(\s*"([^"]+)"\s*\)', line):
                text = match.group(1)
                key = self.find_translation_key(text)
                if key:
                    original = match.group(0)
                    replacement = f'_("{key}")'
                    replacements.append(Replacement(
                        original=original,
                        replacement=replacement,
                        line=line_num,
                        key=key,
                        context='wxString'
                    ))
                    
            # Pattern 2: SetLabel, SetTitle, SetToolTip, SetStatusText
            for match in re.finditer(r'(SetLabel|SetTitle|SetToolTip|SetStatusText)\s*\(\s*"([^"]+)"\s*\)', line):
                method = match.group(1)
                text = match.group(2)
                key = self.find_translation_key(text)
                if key:
                    original = match.group(0)
                    replacement = f'{method}(_("{key}"))'
                    replacements.append(Replacement(
                        original=original,
                        replacement=replacement,
                        line=line_num,
                        key=key,
                        context=method
                    ))
                    
            # Pattern 3: Append(ID, "label")
            for match in re.finditer(r'(->Append)\s*\(\s*([^,]+)\s*,\s*"([^"]+)"', line):
                method = match.group(1)
                id_part = match.group(2).strip()
                text = match.group(3)
                key = self.find_translation_key(text)
                if key:
                    original = match.group(0)
                    replacement = f'{method}({id_part}, _("{key}")'
                    replacements.append(Replacement(
                        original=original,
                        replacement=replacement,
                        line=line_num,
                        key=key,
                        context='menu'
                    ))
                    
            # Pattern 4: AddPage(page, "label")
            for match in re.finditer(r'(->AddPage)\s*\(\s*([^,]+)\s*,\s*"([^"]+)"', line):
                method = match.group(1)
                page_part = match.group(2).strip()
                text = match.group(3)
                key = self.find_translation_key(text)
                if key:
                    original = match.group(0)
                    replacement = f'{method}({page_part}, _("{key}")'
                    replacements.append(Replacement(
                        original=original,
                        replacement=replacement,
                        line=line_num,
                        key=key,
                        context='tab'
                    ))
                    
        return replacements
    
    def apply_replacements(self, filepath: Path, replacements: List[Replacement]) -> bool:
        """Apply replacements to a file."""
        if not replacements:
            return False
            
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            return False
        
        # Sort replacements by position in file (reverse order to preserve indices)
        # For now, just do string replacement
        modified = content
        for r in replacements:
            modified = modified.replace(r.original, r.replacement, 1)
            
        if self.dry_run:
            return True
            
        # Backup original
        backup_path = filepath.with_suffix(filepath.suffix + '.bak')
        shutil.copy2(filepath, backup_path)
        
        # Write modified content
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(modified)
            
        return True
    
    def add_include(self, filepath: Path) -> bool:
        """Add i18n include if not present."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            return False
            
        # Check if i18n header already included
        if 'localization_manager.h' in content or 'i18n/' in content:
            return False
            
        # Find a good place to add include
        lines = content.split('\n')
        include_idx = -1
        last_include = -1
        
        for i, line in enumerate(lines):
            if line.strip().startswith('#include'):
                last_include = i
                
        if last_include >= 0:
            # Add after last include
            lines.insert(last_include + 1, '#include "i18n/localization_manager.h"')
        else:
            # Add after pragma or at top
            for i, line in enumerate(lines):
                if line.strip().startswith('#pragma'):
                    lines.insert(i + 1, '#include "i18n/localization_manager.h"')
                    break
                    
        modified = '\n'.join(lines)
        
        if self.dry_run:
            return True
            
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(modified)
            
        return True
    
    def process_directory(self, source_dir: Path, file_pattern: str = '*.cpp') -> Dict[Path, List[Replacement]]:
        """Process all files in a directory."""
        all_replacements = {}
        
        for filepath in source_dir.rglob(file_pattern):
            replacements = self.process_file(filepath)
            if replacements:
                all_replacements[filepath] = replacements
                
        return all_replacements
    
    def generate_report(self, all_replacements: Dict[Path, List[Replacement]], output_file: Path):
        """Generate a report of all replacements."""
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("# i18n Code Update Report\n\n")
            f.write(f"Total files: {len(all_replacements)}\n")
            total_replacements = sum(len(r) for r in all_replacements.values())
            f.write(f"Total replacements: {total_replacements}\n\n")
            
            for filepath, replacements in sorted(all_replacements.items()):
                f.write(f"\n## {filepath}\n\n")
                f.write("| Line | Context | Original | Replacement | Key |\n")
                f.write("|------|---------|----------|-------------|-----|\n")
                for r in replacements:
                    orig = r.original.replace('|', '\\|')[:50]
                    repl = r.replacement.replace('|', '\\|')[:50]
                    f.write(f"| {r.line} | {r.context} | `{orig}` | `{repl}` | `{r.key}` |\n")


def main():
    parser = argparse.ArgumentParser(description='Update source code to use i18n macros')
    parser.add_argument('-s', '--source', type=Path, required=True,
                        help='Source directory containing C++ files')
    parser.add_argument('-t', '--translations', type=Path, 
                        default=Path('translations/en_CA.json'),
                        help='English translation file')
    parser.add_argument('-r', '--report', type=Path, default=Path('i18n_update_report.md'),
                        help='Output report file')
    parser.add_argument('--apply', action='store_true',
                        help='Apply changes (default is dry-run)')
    parser.add_argument('--pattern', default='*.cpp',
                        help='File pattern to match')
    
    args = parser.parse_args()
    
    updater = CodeUpdater(args.translations, dry_run=not args.apply)
    
    print(f"Scanning {args.source} for files matching {args.pattern}...")
    all_replacements = updater.process_directory(args.source, args.pattern)
    
    if not all_replacements:
        print("No replacements found.")
        return
        
    print(f"\nFound {len(all_replacements)} files with potential replacements")
    total = sum(len(r) for r in all_replacements.values())
    print(f"Total potential replacements: {total}")
    
    # Generate report
    updater.generate_report(all_replacements, args.report)
    print(f"\nReport saved to {args.report}")
    
    if args.apply:
        print("\nApplying changes...")
        for filepath, replacements in all_replacements.items():
            if updater.apply_replacements(filepath, replacements):
                print(f"  Updated {filepath} ({len(replacements)} replacements)")
        print("\nDone! Backups created with .bak extension")
    else:
        print("\nDry run - no changes made. Use --apply to apply changes.")


if __name__ == '__main__':
    main()
