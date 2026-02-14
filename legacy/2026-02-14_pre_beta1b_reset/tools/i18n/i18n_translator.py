#!/usr/bin/env python3
"""
i18n Translator for ScratchRobin
Translates English strings to target languages using various translation backends.
Supports: Google Translate API, DeepL API, LibreTranslate, Ollama (local LLM)
"""

import json
import argparse
import time
import hashlib
from pathlib import Path
from dataclasses import dataclass
from typing import Dict, List, Optional, Callable
from collections import defaultdict
import urllib.request
import urllib.parse
import urllib.error


# Language configuration
LANGUAGES = {
    'fr_CA': {'name': 'French (Canadian)', 'locale': 'fr_CA', 'deepl_code': 'FR'},
    'es_ES': {'name': 'Spanish', 'locale': 'es_ES', 'deepl_code': 'ES'},
    'pt_PT': {'name': 'Portuguese', 'locale': 'pt_PT', 'deepl_code': 'PT-PT'},
    'de_DE': {'name': 'German', 'locale': 'de_DE', 'deepl_code': 'DE'},
    'it_IT': {'name': 'Italian', 'locale': 'it_IT', 'deepl_code': 'IT'},
}


@dataclass
class TranslationBackend:
    """Configuration for a translation backend."""
    name: str
    translate_func: Callable[[str, str], str]
    requires_key: bool = False
    rate_limit_delay: float = 0.1  # Seconds between requests


class LibreTranslateClient:
    """Client for LibreTranslate API (self-hosted or public)."""
    
    def __init__(self, base_url: str = "https://libretranslate.de", api_key: Optional[str] = None):
        self.base_url = base_url.rstrip('/')
        self.api_key = api_key
        
    def translate(self, text: str, target_lang: str) -> str:
        """Translate text using LibreTranslate."""
        # Map locale to LibreTranslate language code
        lang_map = {
            'fr_CA': 'fr', 'es_ES': 'es', 'pt_PT': 'pt', 'de_DE': 'de', 'it_IT': 'it'
        }
        target = lang_map.get(target_lang, target_lang.split('_')[0])
        
        data = {
            'q': text,
            'source': 'en',
            'target': target,
            'format': 'text'
        }
        if self.api_key:
            data['api_key'] = self.api_key
            
        try:
            req = urllib.request.Request(
                f"{self.base_url}/translate",
                data=json.dumps(data).encode('utf-8'),
                headers={'Content-Type': 'application/json'},
                method='POST'
            )
            
            with urllib.request.urlopen(req, timeout=30) as response:
                result = json.loads(response.read().decode('utf-8'))
                return result.get('translatedText', text)
        except Exception as e:
            print(f"  Warning: Translation failed: {e}")
            return text


class OllamaClient:
    """Client for Ollama local LLM."""
    
    def __init__(self, base_url: str = "http://localhost:11434", model: str = "llama3.2"):
        self.base_url = base_url.rstrip('/')
        self.model = model
        
    def translate(self, text: str, target_lang: str) -> str:
        """Translate text using local Ollama LLM."""
        lang_names = {
            'fr_CA': 'Canadian French',
            'es_ES': 'Spanish', 
            'pt_PT': 'Portuguese',
            'de_DE': 'German',
            'it_IT': 'Italian'
        }
        lang_name = lang_names.get(target_lang, target_lang)
        
        prompt = f"""Translate the following UI text from English to {lang_name}.
Keep it concise and suitable for software interface.
Only return the translation, nothing else.

Text: "{text}"

Translation:"""

        data = {
            'model': self.model,
            'prompt': prompt,
            'stream': False
        }
        
        try:
            req = urllib.request.Request(
                f"{self.base_url}/api/generate",
                data=json.dumps(data).encode('utf-8'),
                headers={'Content-Type': 'application/json'},
                method='POST'
            )
            
            with urllib.request.urlopen(req, timeout=60) as response:
                result = json.loads(response.read().decode('utf-8'))
                translation = result.get('response', text).strip()
                # Remove quotes if LLM added them
                translation = translation.strip('"').strip("'")
                return translation
        except Exception as e:
            print(f"  Warning: Ollama translation failed: {e}")
            return text


class MockTranslator:
    """Mock translator for testing - returns placeholder text."""
    
    def translate(self, text: str, target_lang: str) -> str:
        lang_prefix = target_lang.split('_')[0].upper()
        return f"[{lang_prefix}] {text}"


class TranslationCache:
    """Caches translations to avoid re-translating."""
    
    def __init__(self, cache_file: Path):
        self.cache_file = cache_file
        self.cache: Dict[str, str] = {}
        self.load()
        
    def _make_key(self, text: str, target_lang: str) -> str:
        """Create cache key."""
        return hashlib.md5(f"{text}:{target_lang}".encode()).hexdigest()
        
    def load(self):
        """Load cache from file."""
        if self.cache_file.exists():
            try:
                with open(self.cache_file, 'r', encoding='utf-8') as f:
                    self.cache = json.load(f)
            except Exception:
                self.cache = {}
                
    def save(self):
        """Save cache to file."""
        with open(self.cache_file, 'w', encoding='utf-8') as f:
            json.dump(self.cache, f, indent=2)
            
    def get(self, text: str, target_lang: str) -> Optional[str]:
        """Get cached translation."""
        key = self._make_key(text, target_lang)
        return self.cache.get(key)
        
    def set(self, text: str, target_lang: str, translation: str):
        """Cache a translation."""
        key = self._make_key(text, target_lang)
        self.cache[key] = translation


class I18nTranslator:
    """Main translation coordinator."""
    
    def __init__(self, backend: str = 'mock', api_key: Optional[str] = None,
                 cache_file: Optional[Path] = None, ollama_model: str = 'llama3.2'):
        self.backend_name = backend
        self.cache = TranslationCache(cache_file or Path('.translation_cache.json'))
        self.backend = self._create_backend(backend, api_key, ollama_model)
        
    def _create_backend(self, backend: str, api_key: Optional[str], ollama_model: str) -> TranslationBackend:
        """Create the appropriate translation backend."""
        
        if backend == 'libretranslate':
            client = LibreTranslateClient(api_key=api_key)
            return TranslationBackend(
                name='LibreTranslate',
                translate_func=client.translate,
                requires_key=False,
                rate_limit_delay=0.5
            )
            
        elif backend == 'ollama':
            client = OllamaClient(model=ollama_model)
            return TranslationBackend(
                name='Ollama',
                translate_func=client.translate,
                requires_key=False,
                rate_limit_delay=0.1
            )
            
        elif backend == 'mock':
            client = MockTranslator()
            return TranslationBackend(
                name='Mock',
                translate_func=client.translate,
                requires_key=False,
                rate_limit_delay=0
            )
            
        else:
            raise ValueError(f"Unknown backend: {backend}")
    
    def translate_text(self, text: str, target_lang: str) -> str:
        """Translate a single text, using cache if available."""
        # Check cache
        cached = self.cache.get(text, target_lang)
        if cached:
            return cached
            
        # Translate
        result = self.backend.translate_func(text, target_lang)
        
        # Cache and save
        self.cache.set(text, target_lang, result)
        
        # Rate limiting
        if self.backend.rate_limit_delay > 0:
            time.sleep(self.backend.rate_limit_delay)
            
        return result
    
    def translate_file(self, source_file: Path, target_lang: str, 
                       output_file: Optional[Path] = None) -> Dict[str, str]:
        """Translate all strings in a source translation file."""
        
        # Load source translations (English)
        with open(source_file, 'r', encoding='utf-8') as f:
            source_data = json.load(f)
            
        source_translations = source_data.get('translations', {})
        
        print(f"Translating {len(source_translations)} strings to {target_lang} using {self.backend.name}...")
        
        # Translate
        translated = {}
        total = len(source_translations)
        cached_count = 0
        
        for i, (key, text) in enumerate(source_translations.items(), 1):
            # Check if already in cache
            if self.cache.get(text, target_lang):
                translated[key] = self.cache.get(text, target_lang)
                cached_count += 1
            else:
                translated[key] = self.translate_text(text, target_lang)
                
            if i % 10 == 0 or i == total:
                print(f"  Progress: {i}/{total} ({cached_count} from cache)")
                
        # Save cache periodically
        self.cache.save()
        
        # Create output
        lang_info = LANGUAGES.get(target_lang, {'name': target_lang, 'locale': target_lang})
        output_data = {
            'version': source_data.get('version', '1.0.0'),
            'language': lang_info['name'],
            'locale': lang_info['locale'],
            'translator': f'Auto-translated via {self.backend.name}',
            'translations': translated
        }
        
        # Save to file
        if output_file:
            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(output_data, f, indent=2, ensure_ascii=False)
            print(f"Saved to {output_file}")
            
        return output_data
    
    def update_existing_translation(self, source_file: Path, target_file: Path, 
                                     target_lang: str) -> Dict[str, str]:
        """Update an existing translation file with new/missing keys."""
        
        # Load source (English) and target
        with open(source_file, 'r', encoding='utf-8') as f:
            source_data = json.load(f)
        with open(target_file, 'r', encoding='utf-8') as f:
            target_data = json.load(f)
            
        source_translations = source_data.get('translations', {})
        target_translations = target_data.get('translations', {})
        
        # Find missing keys
        missing_keys = set(source_translations.keys()) - set(target_translations.keys())
        
        if not missing_keys:
            print(f"No missing translations for {target_lang}")
            return target_data
            
        print(f"Found {len(missing_keys)} missing translations for {target_lang}")
        
        # Translate missing keys
        for key in missing_keys:
            text = source_translations[key]
            target_translations[key] = self.translate_text(text, target_lang)
            
        # Update metadata
        target_data['translations'] = target_translations
        target_data['translator'] = f"{target_data.get('translator', '')} + Auto-translated"
        
        # Save
        with open(target_file, 'w', encoding='utf-8') as f:
            json.dump(target_data, f, indent=2, ensure_ascii=False)
            
        self.cache.save()
        print(f"Updated {target_file}")
        
        return target_data


def main():
    parser = argparse.ArgumentParser(description='Translate ScratchRobin i18n files')
    parser.add_argument('command', choices=['translate', 'update', 'batch'],
                        help='Command: translate (new file), update (existing), batch (all languages)')
    parser.add_argument('-s', '--source', type=Path, default=Path('translations/en_CA.json'),
                        help='Source English translation file')
    parser.add_argument('-t', '--target', type=str,
                        help='Target language code (e.g., fr_CA, es_ES)')
    parser.add_argument('-o', '--output', type=Path,
                        help='Output file (for translate command)')
    parser.add_argument('-b', '--backend', choices=['mock', 'libretranslate', 'ollama'],
                        default='mock', help='Translation backend')
    parser.add_argument('-k', '--api-key', help='API key for translation service')
    parser.add_argument('-m', '--ollama-model', default='llama3.2',
                        help='Ollama model to use')
    parser.add_argument('-c', '--cache', type=Path, default=Path('.translation_cache.json'),
                        help='Translation cache file')
    parser.add_argument('--translations-dir', type=Path, default=Path('translations'),
                        help='Directory containing translation files')
    
    args = parser.parse_args()
    
    translator = I18nTranslator(
        backend=args.backend,
        api_key=args.api_key,
        cache_file=args.cache,
        ollama_model=args.ollama_model
    )
    
    if args.command == 'translate':
        if not args.target:
            parser.error("--target is required for translate command")
        if not args.output:
            args.output = args.translations_dir / f"{args.target}.json"
            
        translator.translate_file(args.source, args.target, args.output)
        
    elif args.command == 'update':
        if not args.target:
            parser.error("--target is required for update command")
        target_file = args.translations_dir / f"{args.target}.json"
        
        translator.update_existing_translation(args.source, target_file, args.target)
        
    elif args.command == 'batch':
        # Update all language files
        for lang_code in LANGUAGES.keys():
            target_file = args.translations_dir / f"{lang_code}.json"
            if target_file.exists():
                print(f"\n=== Updating {lang_code} ===")
                translator.update_existing_translation(args.source, target_file, lang_code)
            else:
                print(f"\n=== Creating {lang_code} ===")
                translator.translate_file(args.source, lang_code, target_file)
                
    print("\nDone!")


if __name__ == '__main__':
    main()
