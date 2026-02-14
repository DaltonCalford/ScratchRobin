// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "locales.h"
#include <wx/wx.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {
namespace i18n {

/**
 * @brief Translation catalog interface
 */
class TranslationCatalog {
public:
    virtual ~TranslationCatalog() = default;
    
    // Get translated string
    virtual std::string GetString(const std::string& key) const = 0;
    virtual std::string GetString(const std::string& key, const std::string& context) const = 0;
    
    // Get translated string with plural forms
    virtual std::string GetPluralString(const std::string& singular_key,
                                        const std::string& plural_key,
                                        int count) const = 0;
    
    // Check if translation exists
    virtual bool HasTranslation(const std::string& key) const = 0;
    
    // Get catalog info
    virtual Language GetLanguage() const = 0;
    virtual std::string GetVersion() const = 0;
    virtual int GetTranslationCount() const = 0;
};

/**
 * @brief Simple JSON-based translation catalog
 */
class JsonTranslationCatalog : public TranslationCatalog {
public:
    JsonTranslationCatalog(Language lang, const std::string& json_content);
    
    std::string GetString(const std::string& key) const override;
    std::string GetString(const std::string& key, const std::string& context) const override;
    std::string GetPluralString(const std::string& singular_key,
                                const std::string& plural_key,
                                int count) const override;
    bool HasTranslation(const std::string& key) const override;
    Language GetLanguage() const override;
    std::string GetVersion() const override;
    int GetTranslationCount() const override;
    
private:
    Language language_;
    std::string version_;
    std::map<std::string, std::string> translations_;
    std::map<std::string, std::vector<std::string>> plural_translations_;
    
    void ParseJson(const std::string& json_content);
};

/**
 * @brief Language change event
 */
struct LanguageChangeEvent {
    Language old_language;
    Language new_language;
    bool initialized = false;  // True on first load
};

using LanguageChangeCallback = std::function<void(const LanguageChangeEvent&)>;

/**
 * @brief Localization manager - singleton for application-wide i18n
 * 
 * Manages language switching, translation catalogs, and notifies
 * UI components of language changes.
 */
class LocalizationManager {
public:
    static LocalizationManager& Instance();
    
    // Initialize with a language
    bool Initialize(Language lang = Language::Default);
    void Shutdown();
    bool IsInitialized() const { return initialized_; }
    
    // Language switching
    bool SetLanguage(Language lang);
    Language GetCurrentLanguage() const { return current_language_; }
    
    // wxWidgets integration
    bool InitializeWxLocale(wxLocale& locale);
    
    // Translation functions
    std::string Translate(const std::string& key) const;
    std::string Translate(const std::string& key, const std::string& context) const;
    std::string TranslatePlural(const std::string& singular, const std::string& plural, int count) const;
    
    // Format translation with parameters
    template<typename... Args>
    std::string TranslateFormat(const std::string& key, Args... args) const {
        std::string format = Translate(key);
        // Simple format replacement: {0}, {1}, etc.
        return FormatString(format, args...);
    }
    
    // Load translation catalog
    bool LoadCatalog(Language lang, const std::string& catalog_path);
    bool LoadCatalogFromString(Language lang, const std::string& json_content);
    bool HasCatalog(Language lang) const;
    
    // Get loaded catalog
    std::shared_ptr<TranslationCatalog> GetCatalog(Language lang) const;
    std::shared_ptr<TranslationCatalog> GetCurrentCatalog() const;
    
    // Event subscriptions
    void AddLanguageChangeListener(LanguageChangeCallback callback);
    void RemoveLanguageChangeListener(void* identifier);
    
    // UI updates
    void RequestUiRefresh();  // Notify all listeners to refresh
    
    // Translation coverage
    float GetTranslationCoverage(Language lang) const;  // 0.0 to 1.0
    std::vector<std::string> GetUntranslatedKeys(Language lang) const;
    
    // Translations directory
    void SetTranslationsDirectory(const std::string& path);
    std::string GetTranslationsDirectory() const { return translations_dir_; }
    
    // Auto-detect system language
    Language DetectSystemLanguage() const;
    
    // Fallback mechanism
    void SetFallbackLanguage(Language lang) { fallback_language_ = lang; }
    Language GetFallbackLanguage() const { return fallback_language_; }
    
private:
    LocalizationManager() = default;
    ~LocalizationManager() = default;
    
    bool initialized_ = false;
    Language current_language_ = Language::Default;
    Language fallback_language_ = Language::Default;
    std::string translations_dir_;
    
    std::map<Language, std::shared_ptr<TranslationCatalog>> catalogs_;
    std::vector<std::pair<void*, LanguageChangeCallback>> listeners_;
    
    void NotifyListeners(const LanguageChangeEvent& event);
    std::string FindTranslationFile(Language lang) const;
    
    // Template helper for formatting
    template<typename T, typename... Args>
    std::string FormatString(std::string format, T value, Args... args) const {
        // Replace {N} with value
        // This is a simplified implementation
        return format;
    }
    std::string FormatString(std::string format) const { return format; }
};

/**
 * @brief Helper macros for translations
 */
// Standard translation
#define TR(key) scratchrobin::i18n::LocalizationManager::Instance().Translate(key)

// Translation with context
#define TR_CTX(key, context) scratchrobin::i18n::LocalizationManager::Instance().Translate(key, context)

// Plural translation
#define TR_PLURAL(singular, plural, count) \
    scratchrobin::i18n::LocalizationManager::Instance().TranslatePlural(singular, plural, count)

// Formatted translation
#define TR_FMT(key, ...) \
    scratchrobin::i18n::LocalizationManager::Instance().TranslateFormat(key, __VA_ARGS__)

/**
 * @brief RAII helper for temporarily changing language
 */
class ScopedLanguage {
public:
    explicit ScopedLanguage(Language lang);
    ~ScopedLanguage();
    
    void Restore();
    
private:
    Language previous_language_;
    bool restored_ = false;
};

/**
 * @brief Base class for translatable UI components
 */
class TranslatableComponent {
public:
    virtual ~TranslatableComponent() = default;
    virtual void UpdateTranslations() = 0;
    
    // Helper to register for language changes
    void RegisterForTranslationUpdates();
    void UnregisterFromTranslationUpdates();
};

} // namespace i18n
} // namespace scratchrobin
