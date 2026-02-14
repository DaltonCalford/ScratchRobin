// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "localization_manager.h"

#include "core/resource_paths.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace scratchrobin {
namespace i18n {

// JsonTranslationCatalog implementation

JsonTranslationCatalog::JsonTranslationCatalog(Language lang, const std::string& json_content)
    : language_(lang) {
    ParseJson(json_content);
}

std::string JsonTranslationCatalog::GetString(const std::string& key) const {
    auto it = translations_.find(key);
    if (it != translations_.end()) {
        return it->second;
    }
    return key;  // Return key as fallback
}

std::string JsonTranslationCatalog::GetString(const std::string& key, 
                                               const std::string& context) const {
    // Context-specific lookup: "context|key"
    std::string contextual_key = context + "|" + key;
    auto it = translations_.find(contextual_key);
    if (it != translations_.end()) {
        return it->second;
    }
    // Fall back to non-contextual
    return GetString(key);
}

std::string JsonTranslationCatalog::GetPluralString(const std::string& singular_key,
                                                     const std::string& plural_key,
                                                     int count) const {
    // Simplified plural handling
    if (count == 1) {
        return GetString(singular_key);
    }
    return GetString(plural_key);
}

bool JsonTranslationCatalog::HasTranslation(const std::string& key) const {
    return translations_.count(key) > 0;
}

Language JsonTranslationCatalog::GetLanguage() const {
    return language_;
}

std::string JsonTranslationCatalog::GetVersion() const {
    return version_;
}

int JsonTranslationCatalog::GetTranslationCount() const {
    return static_cast<int>(translations_.size());
}

void JsonTranslationCatalog::ParseJson(const std::string& json_content) {
    // Simplified JSON parsing - in production would use proper JSON library
    // Expected format:
    // {
    //   "version": "1.0",
    //   "translations": {
    //     "key": "translation",
    //     "context|key": "contextual translation"
    //   }
    // }
    
    // For now, add some default translations as placeholders
    // In production, these would be loaded from files
    
    // Common UI strings
    translations_["app.name"] = "ScratchRobin";
    translations_["app.title"] = "Database Designer";
    translations_["menu.file"] = "File";
    translations_["menu.edit"] = "Edit";
    translations_["menu.view"] = "View";
    translations_["menu.tools"] = "Tools";
    translations_["menu.help"] = "Help";
    translations_["button.ok"] = "OK";
    translations_["button.cancel"] = "Cancel";
    translations_["button.save"] = "Save";
    translations_["button.close"] = "Close";
    translations_["dialog.confirm"] = "Are you sure?";
    translations_["dialog.error"] = "Error";
    translations_["dialog.warning"] = "Warning";
    translations_["dialog.info"] = "Information";
    
    // Language-specific defaults would be loaded from files
}

// LocalizationManager implementation

LocalizationManager& LocalizationManager::Instance() {
    static LocalizationManager instance;
    return instance;
}

bool LocalizationManager::Initialize(Language lang) {
    if (initialized_) {
        return SetLanguage(lang);
    }
    
    // Set default translations directory
    translations_dir_ = "translations";
    
    // Try to load system language first, then requested
    Language system_lang = DetectSystemLanguage();
    if (system_lang != lang && HasCatalog(system_lang)) {
        current_language_ = system_lang;
    } else {
        current_language_ = lang;
    }
    
    // Load the catalog
    if (!HasCatalog(current_language_)) {
        // Create default catalog
        auto catalog = std::make_shared<JsonTranslationCatalog>(
            current_language_, "{}");
        catalogs_[current_language_] = catalog;
    }
    
    initialized_ = true;
    
    // Notify listeners
    LanguageChangeEvent event;
    event.old_language = current_language_;
    event.new_language = current_language_;
    event.initialized = true;
    NotifyListeners(event);
    
    return true;
}

void LocalizationManager::Shutdown() {
    catalogs_.clear();
    listeners_.clear();
    initialized_ = false;
}

bool LocalizationManager::SetLanguage(Language lang) {
    if (!initialized_) {
        return Initialize(lang);
    }
    
    if (lang == current_language_) {
        return true;
    }
    
    // Load catalog if not already loaded
    if (!HasCatalog(lang)) {
        std::string file_path = FindTranslationFile(lang);
        if (!file_path.empty()) {
            LoadCatalog(lang, file_path);
        } else {
            // Create empty catalog
            auto catalog = std::make_shared<JsonTranslationCatalog>(lang, "{}");
            catalogs_[lang] = catalog;
        }
    }
    
    Language old_lang = current_language_;
    current_language_ = lang;
    
    // Notify listeners
    LanguageChangeEvent event;
    event.old_language = old_lang;
    event.new_language = lang;
    event.initialized = false;
    NotifyListeners(event);
    
    return true;
}

bool LocalizationManager::InitializeWxLocale(wxLocale& locale) {
    if (!initialized_) return false;
    
    const auto& info = GetLanguageInfo(current_language_);
    
    // Initialize wxLocale
    int wx_lang = wxLANGUAGE_ENGLISH;
    
    // Map our language to wx language constant
    switch (current_language_) {
        case Language::English: wx_lang = wxLANGUAGE_ENGLISH_CANADA; break;
        case Language::French: wx_lang = wxLANGUAGE_FRENCH_CANADIAN; break;
        case Language::Spanish: wx_lang = wxLANGUAGE_SPANISH; break;
        case Language::Portuguese: wx_lang = wxLANGUAGE_PORTUGUESE; break;
        case Language::German: wx_lang = wxLANGUAGE_GERMAN; break;
        case Language::Italian: wx_lang = wxLANGUAGE_ITALIAN; break;
        default: wx_lang = wxLANGUAGE_ENGLISH; break;
    }
    
    return locale.Init(wx_lang);
}

std::string LocalizationManager::Translate(const std::string& key) const {
    auto catalog = GetCurrentCatalog();
    if (catalog) {
        std::string result = catalog->GetString(key);
        if (result != key) return result;
    }
    
    // Try fallback
    if (current_language_ != fallback_language_) {
        auto fallback = GetCatalog(fallback_language_);
        if (fallback) {
            return fallback->GetString(key);
        }
    }
    
    return key;
}

std::string LocalizationManager::Translate(const std::string& key, 
                                            const std::string& context) const {
    auto catalog = GetCurrentCatalog();
    if (catalog) {
        std::string result = catalog->GetString(key, context);
        if (result != key) return result;
    }
    
    // Try fallback
    if (current_language_ != fallback_language_) {
        auto fallback = GetCatalog(fallback_language_);
        if (fallback) {
            return fallback->GetString(key, context);
        }
    }
    
    return key;
}

std::string LocalizationManager::TranslatePlural(const std::string& singular,
                                                  const std::string& plural,
                                                  int count) const {
    auto catalog = GetCurrentCatalog();
    if (catalog) {
        return catalog->GetPluralString(singular, plural, count);
    }
    return (count == 1) ? singular : plural;
}

bool LocalizationManager::LoadCatalog(Language lang, const std::string& catalog_path) {
    std::ifstream file(catalog_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    return LoadCatalogFromString(lang, content);
}

bool LocalizationManager::LoadCatalogFromString(Language lang, 
                                                 const std::string& json_content) {
    try {
        auto catalog = std::make_shared<JsonTranslationCatalog>(lang, json_content);
        catalogs_[lang] = catalog;
        return true;
    } catch (...) {
        return false;
    }
}

bool LocalizationManager::HasCatalog(Language lang) const {
    return catalogs_.count(lang) > 0;
}

std::shared_ptr<TranslationCatalog> LocalizationManager::GetCatalog(Language lang) const {
    auto it = catalogs_.find(lang);
    if (it != catalogs_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<TranslationCatalog> LocalizationManager::GetCurrentCatalog() const {
    return GetCatalog(current_language_);
}

void LocalizationManager::AddLanguageChangeListener(LanguageChangeCallback callback) {
    // Use callback address as identifier
    void* id = reinterpret_cast<void*>(&callback);
    listeners_.push_back({id, callback});
}

void LocalizationManager::RemoveLanguageChangeListener(void* identifier) {
    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
            [identifier](const auto& pair) { return pair.first == identifier; }),
        listeners_.end()
    );
}

void LocalizationManager::RequestUiRefresh() {
    LanguageChangeEvent event;
    event.old_language = current_language_;
    event.new_language = current_language_;
    event.initialized = false;
    NotifyListeners(event);
}

float LocalizationManager::GetTranslationCoverage(Language lang) const {
    auto catalog = GetCatalog(lang);
    if (!catalog) return 0.0f;
    
    // Compare against English catalog as reference
    auto english = GetCatalog(Language::English);
    if (!english) return 1.0f;
    
    int total = english->GetTranslationCount();
    if (total == 0) return 1.0f;
    
    int translated = catalog->GetTranslationCount();
    return static_cast<float>(translated) / total;
}

std::vector<std::string> LocalizationManager::GetUntranslatedKeys(Language lang) const {
    std::vector<std::string> untranslated;
    
    auto catalog = GetCatalog(lang);
    auto english = GetCatalog(Language::English);
    
    if (!english) return untranslated;
    
    // In real implementation, iterate through all English keys
    // and check if they exist in target catalog
    
    return untranslated;
}

void LocalizationManager::SetTranslationsDirectory(const std::string& path) {
    translations_dir_ = path;
}

Language LocalizationManager::DetectSystemLanguage() const {
    return GetSystemDefaultLanguage();
}

void LocalizationManager::NotifyListeners(const LanguageChangeEvent& event) {
    for (const auto& [id, callback] : listeners_) {
        if (callback) {
            callback(event);
        }
    }
}

std::string LocalizationManager::FindTranslationFile(Language lang) const {
    const auto& info = GetLanguageInfo(lang);
    
    // Search for translation file in multiple locations
    std::vector<std::string> possible_names = {
        // Default translations directory
        translations_dir_ + "/" + info.locale_code + ".json",
        translations_dir_ + "/" + info.locale_code.substr(0, 2) + ".json",
        translations_dir_ + "/" + info.english_name + ".json",
        // Resource paths (for AppImage/portable installations)
        ResourcePaths::GetTranslationPath(info.locale_code),
        ResourcePaths::GetTranslationPath(info.locale_code.substr(0, 2)),
        // Current working directory fallback
        "translations/" + info.locale_code + ".json",
        "translations/" + info.locale_code.substr(0, 2) + ".json"
    };
    
    for (const auto& path : possible_names) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    return "";
}

// ScopedLanguage implementation

ScopedLanguage::ScopedLanguage(Language lang) {
    auto& manager = LocalizationManager::Instance();
    previous_language_ = manager.GetCurrentLanguage();
    manager.SetLanguage(lang);
}

ScopedLanguage::~ScopedLanguage() {
    Restore();
}

void ScopedLanguage::Restore() {
    if (!restored_) {
        LocalizationManager::Instance().SetLanguage(previous_language_);
        restored_ = true;
    }
}

// TranslatableComponent implementation

void TranslatableComponent::RegisterForTranslationUpdates() {
    auto& manager = LocalizationManager::Instance();
    // This would need a more complex implementation to handle member functions
    // For now, subclasses should manually register
}

void TranslatableComponent::UnregisterFromTranslationUpdates() {
    // Unregister from updates
}

} // namespace i18n
} // namespace scratchrobin
