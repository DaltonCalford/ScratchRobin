// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include <ctime>
#include <string>
#include <vector>
#include <map>

namespace scratchrobin {
namespace i18n {

/**
 * @brief Supported languages
 */
enum class Language {
    English,        // en_CA - English (Canadian)
    French,         // fr_CA - French (Canadian)
    Spanish,        // es_ES - Spanish
    Portuguese,     // pt_PT - Portuguese
    German,         // de_DE - German
    Italian,        // it_IT - Italian
    
    // Future beta languages
    Dutch,          // nl_NL
    Polish,         // pl_PL
    Russian,        // ru_RU
    Japanese,       // ja_JP
    Chinese,        // zh_CN
    Korean,         // ko_KR
    
    Count,          // Number of supported languages
    Default = English
};

/**
 * @brief Language information structure
 */
struct LanguageInfo {
    Language code;
    std::string locale_code;        // e.g., "en_CA", "fr_CA"
    std::string name;               // Native name: "English", "Français"
    std::string english_name;       // English name: "English", "French"
    std::string flag_emoji;         // 🇨🇦, 🇫🇷, etc.
    bool is_beta = false;           // Available in beta?
    bool is_rtl = false;            // Right-to-left language?
    std::string date_format;        // Date format string
    std::string time_format;        // Time format string
    std::string number_format;      // Number format: ",.", ".,", " "
    std::string decimal_separator;
    std::string thousands_separator;
};

/**
 * @brief Get information about a language
 */
const LanguageInfo& GetLanguageInfo(Language lang);

/**
 * @brief Get all supported languages (release languages only)
 */
std::vector<LanguageInfo> GetSupportedLanguages();

/**
 * @brief Get all languages including beta
 */
std::vector<LanguageInfo> GetAllLanguages();

/**
 * @brief Convert locale code to Language enum
 */
Language LocaleCodeToLanguage(const std::string& code);

/**
 * @brief Convert Language enum to locale code
 */
std::string LanguageToLocaleCode(Language lang);

/**
 * @brief Get system default language
 */
Language GetSystemDefaultLanguage();

/**
 * @brief Check if language is supported (not beta)
 */
bool IsLanguageSupported(Language lang);

/**
 * @brief Format a date according to language conventions
 */
std::string FormatDate(std::time_t timestamp, Language lang);

/**
 * @brief Format a number according to language conventions
 */
std::string FormatNumber(double value, int decimals, Language lang);

/**
 * @brief Format currency according to language conventions
 */
std::string FormatCurrency(double value, const std::string& currency_code, Language lang);

} // namespace i18n
} // namespace scratchrobin
