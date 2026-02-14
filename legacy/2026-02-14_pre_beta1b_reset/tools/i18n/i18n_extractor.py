#!/usr/bin/env python3
"""
i18n String Extractor for ScratchRobin
Extracts hardcoded English strings from C++ source files for translation.
"""

import re
import json
import argparse
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List, Dict, Set, Tuple, Optional
from collections import defaultdict


@dataclass
class ExtractedString:
    """Represents a string that needs translation."""
    key: str
    text: str
    context: str  # Category like "menu", "dialog", "button", etc.
    file: str
    line: int
    snippet: str  # Code snippet for context
    
    def to_dict(self) -> dict:
        return asdict(self)


class StringExtractor:
    """Extracts translatable strings from C++ source files."""
    
    # Patterns for finding UI strings
    PATTERNS = {
        # wxString with literal
        'wxstring_literal': re.compile(r'wxString\s*\(\s*"([^"]+)"\s*\)'),
        
        # _T() macro
        't_macro': re.compile(r'_T\s*\(\s*"([^"]+)"\s*\)'),
        
        # wxT() macro  
        'wxt_macro': re.compile(r'wxT\s*\(\s*"([^"]+)"\s*\)'),
        
        # SetLabel, SetTitle, SetToolTip
        'set_methods': re.compile(r'(SetLabel|SetTitle|SetToolTip|SetStatusText)\s*\(\s*"([^"]+)"\s*\)'),
        
        # Menu Append
        'menu_append': re.compile(r'Append\s*\(\s*[^,]+\s*,\s*"([^"]+)"'),
        
        # Notebook AddPage
        'notebook_addpage': re.compile(r'AddPage\s*\(\s*[^,]+\s*,\s*"([^"]+)"'),
        
        # InsertColumn
        'list_column': re.compile(r'InsertColumn\s*\([^)]*"([^"]+)"'),
        
        # StaticText Create
        'static_text': re.compile(r'wxStaticText\s*\([^)]*"([^"]+)"'),
        
        # Button Create
        'button_create': re.compile(r'wxButton\s*\([^)]*"([^"]+)"'),
        
        # Choice/ComboBox Append
        'choice_append': re.compile(r'->Append\s*\(\s*"([^"]+)"\s*\)'),
        
        # wxMessageBox / wxMessageDialog
        'message_box': re.compile(r'wxMessageBox\s*\(\s*"([^"]+)"'),
        
        # wxTextEntryDialog
        'text_entry': re.compile(r'wxTextEntryDialog\s*\([^)]*"([^"]+)"'),
    }
    
    # File patterns to exclude
    EXCLUDE_PATTERNS = [
        r'\.h$',
        r'test_',
        r'_test\.',
        r'json',
        r'xml',
        r'cmake',
        r'md$',
    ]
    
    def __init__(self, source_dir: Path):
        self.source_dir = Path(source_dir)
        self.strings: List[ExtractedString] = []
        self.seen: Set[Tuple[str, str]] = set()  # (key, text) to avoid duplicates
        
    def should_process_file(self, filepath: Path) -> bool:
        """Check if file should be processed."""
        if filepath.suffix not in ['.cpp', '.cc', '.cxx']:
            return False
            
        for pattern in self.EXCLUDE_PATTERNS:
            if re.search(pattern, str(filepath)):
                return False
                
        return True
    
    def categorize(self, text: str, file: str, pattern_name: str) -> str:
        """Categorize a string based on content and context."""
        text_lower = text.lower()
        file_lower = file.lower()
        
        # Check file context
        if 'menu' in file_lower or pattern_name == 'menu_append':
            return 'menu'
        elif 'dialog' in file_lower:
            return 'dialog'
        elif 'button' in pattern_name:
            return 'button'
        elif 'message' in pattern_name:
            return 'message'
        elif 'column' in pattern_name:
            return 'column_header'
        elif 'notebook' in pattern_name or 'page' in pattern_name:
            return 'tab'
        elif 'tooltip' in pattern_name:
            return 'tooltip'
        elif 'status' in pattern_name:
            return 'status'
            
        # Check content patterns
        if text.endswith('...') or text.endswith('…'):
            return 'menu'
        elif text in ['OK', 'Cancel', 'Save', 'Close', 'Apply', 'Delete', 'Edit', 'Add', 'Remove',
                      'Yes', 'No', 'Retry', 'Abort', 'Ignore']:
            return 'button'
        elif text.endswith('?') or 'error' in text_lower or 'failed' in text_lower:
            return 'message'
        elif text.endswith(':'):
            return 'label'
        else:
            return 'general'
    
    def generate_key(self, text: str, context: str, file: str) -> str:
        """Generate a translation key from text and context."""
        # Clean the text
        key_text = re.sub(r'[^\w\s]', '', text).lower().strip()
        key_text = re.sub(r'\s+', '_', key_text)
        
        # Shorten if too long
        if len(key_text) > 40:
            key_text = key_text[:40]
            
        # Add context prefix
        file_prefix = Path(file).stem.replace('_', '.')
        
        # Try to use existing key patterns
        if context == 'menu':
            return f"menu.{file_prefix}.{key_text}"
        elif context == 'button':
            return f"button.{key_text}"
        elif context == 'dialog':
            return f"dialog.{key_text}"
        elif context == 'message':
            return f"msg.{key_text}"
        elif context == 'column_header':
            return f"column.{key_text}"
        elif context == 'tab':
            return f"tab.{key_text}"
        else:
            return f"ui.{file_prefix}.{key_text}"
    
    def extract_from_file(self, filepath: Path) -> List[ExtractedString]:
        """Extract strings from a single file."""
        strings = []
        relative_path = filepath.relative_to(self.source_dir)
        
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            return strings
        
        for line_num, line in enumerate(lines, 1):
            for pattern_name, pattern in self.PATTERNS.items():
                for match in pattern.finditer(line):
                    # Get the captured group with the actual text
                    if pattern_name in ['set_methods', 'menu_append', 'notebook_addpage', 
                                        'list_column', 'static_text', 'button_create',
                                        'choice_append', 'message_box', 'text_entry']:
                        text = match.group(2) if pattern_name == 'set_methods' else match.group(1)
                    else:
                        text = match.group(1)
                    
                    if not text or len(text.strip()) < 2:
                        continue
                        
                    # Skip likely non-user strings
                    if self._should_skip(text):
                        continue
                    
                    context = self.categorize(text, str(relative_path), pattern_name)
                    key = self.generate_key(text, context, str(relative_path))
                    
                    # Avoid duplicates
                    identifier = (key, text)
                    if identifier in self.seen:
                        continue
                    self.seen.add(identifier)
                    
                    # Get code snippet for context
                    snippet = line.strip()[:100]
                    
                    strings.append(ExtractedString(
                        key=key,
                        text=text,
                        context=context,
                        file=str(relative_path),
                        line=line_num,
                        snippet=snippet
                    ))
        
        return strings
    
    def _should_skip(self, text: str) -> bool:
        """Determine if a string should be skipped."""
        # Skip SQL, code, file paths, debug messages
        skip_patterns = [
            r'^\s*$',  # Empty
            r'^[\s\*\/\-]*$',  # Just symbols
            r'^SELECT\s+',  # SQL queries
            r'^INSERT\s+',
            r'^UPDATE\s+',
            r'^DELETE\s+',
            r'^CREATE\s+',
            r'^DROP\s+',
            r'^ALTER\s+',
            r'\.cpp$',
            r'\.h$',
            r'\.hpp$',
            r'\.sql$',
            r'\.json$',
            r'\.xml$',
            r'^\s*[\{\[\(]',  # Starts with bracket
            r'^[a-z_][a-z0-9_]*$',  # Just variable names
            r'^[A-Z_][A-Z0-9_]*$',  # Just constants
            r'^\d+$',  # Just numbers
            r'^0x[0-9a-fA-F]+$',  # Hex
            r'^#',  # Preprocessor
            r'//',  # Comments
            r'/\*',
            r'^\*',
            r'\\[ntr]',  # Escape sequences
            r'\*%',  # Format strings
            r'^%[dfsu]',  # Format specifiers
        ]
        
        for pattern in skip_patterns:
            if re.search(pattern, text, re.IGNORECASE):
                return True
                
        # Skip if mostly non-letters
        letters = sum(1 for c in text if c.isalpha())
        if letters < len(text) * 0.3:
            return True
            
        return False
    
    def extract_all(self) -> List[ExtractedString]:
        """Extract strings from all source files."""
        print(f"Scanning {self.source_dir}...")
        
        cpp_files = list(self.source_dir.rglob('*.cpp'))
        cpp_files.extend(self.source_dir.rglob('*.cc'))
        cpp_files.extend(self.source_dir.rglob('*.cxx'))
        
        total_files = len(cpp_files)
        processed = 0
        
        for filepath in cpp_files:
            if not self.should_process_file(filepath):
                continue
                
            strings = self.extract_from_file(filepath)
            self.strings.extend(strings)
            
            processed += 1
            if processed % 10 == 0:
                print(f"  Processed {processed}/{total_files} files, found {len(self.strings)} strings...")
        
        print(f"\nExtraction complete: {len(self.strings)} unique strings from {processed} files")
        return self.strings
    
    def export_json(self, output_path: Path) -> None:
        """Export extracted strings to JSON."""
        data = {
            'metadata': {
                'total_strings': len(self.strings),
                'source_directory': str(self.source_dir),
            },
            'strings_by_context': defaultdict(list),
            'all_strings': [s.to_dict() for s in self.strings]
        }
        
        # Group by context
        for s in self.strings:
            data['strings_by_context'][s.context].append(s.to_dict())
        
        # Convert defaultdict to regular dict for JSON serialization
        data['strings_by_context'] = dict(data['strings_by_context'])
        
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        
        print(f"Exported to {output_path}")
    
    def export_translation_template(self, output_path: Path) -> None:
        """Export as translation template (just key: text)."""
        translations = {}
        
        for s in self.strings:
            # Use existing key or generate one
            key = s.key
            # Ensure uniqueness
            counter = 1
            original_key = key
            while key in translations:
                key = f"{original_key}_{counter}"
                counter += 1
            translations[key] = s.text
        
        template = {
            'version': '1.0.0',
            'language': 'Template',
            'locale': 'template',
            'translator': 'Auto-extracted',
            'translations': translations
        }
        
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(template, f, indent=2, ensure_ascii=False)
        
        print(f"Translation template exported to {output_path}")
        print(f"Total entries: {len(translations)}")


def main():
    parser = argparse.ArgumentParser(description='Extract translatable strings from ScratchRobin source')
    parser.add_argument('-s', '--source', type=Path, default=Path('src'),
                        help='Source directory to scan (default: src)')
    parser.add_argument('-o', '--output', type=Path, default=Path('extracted_strings.json'),
                        help='Output JSON file (default: extracted_strings.json)')
    parser.add_argument('-t', '--template', type=Path,
                        help='Output translation template file')
    
    args = parser.parse_args()
    
    extractor = StringExtractor(args.source)
    extractor.extract_all()
    extractor.export_json(args.output)
    
    if args.template:
        extractor.export_translation_template(args.template)


if __name__ == '__main__':
    main()
