// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "locales.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace scratchrobin {
namespace i18n {

// Language definitions
static const std::map<Language, LanguageInfo> kLanguageInfo = {
    {Language::English, {
        Language::English,
        "en_CA",
        "English",
        "English",
        "🇨🇦",
        false,  // is_beta
        false,  // is_rtl
        "%Y-%m-%d",      // ISO date format
        "%H:%M:%S",
        ".,",
        ".",
        ","
    }},
    {Language::French, {
        Language::French,
        "fr_CA",
        "Français",
        "French",
        "🇫🇷",
        false,
        false,
        "%d/%m/%Y",      // European date format
        "%H:%M:%S",
        ", ",
        ",",
        " "
    }},
    {Language::Spanish, {
        Language::Spanish,
        "es_ES",
        "Español",
        "Spanish",
        "🇪🇸",
        false,
        false,
        "%d/%m/%Y",
        "%H:%M:%S",
        ",.",
        ",",
        "."
    }},
    {Language::Portuguese, {
        Language::Portuguese,
        "pt_PT",
        "Português",
        "Portuguese",
        "🇵🇹",
        false,
        false,
        "%d/%m/%Y",
        "%H:%M:%S",
        ",.",
        ",",
        "."
    }},
    {Language::German, {
        Language::German,
        "de_DE",
        "Deutsch",
        "German",
        "🇩🇪",
        false,
        false,
        "%d.%m.%Y",      // German uses dots
        "%H:%M:%S",
        ",.",
        ",",
        "."
    }},
    {Language::Italian, {
        Language::Italian,
        "it_IT",
        "Italiano",
        "Italian",
        "🇮🇹",
        false,
        false,
        "%d/%m/%Y",
        "%H:%M:%S",
        ",.",
        ",",
        "."
    }},
    // Beta languages (for future)
    {Language::Dutch, {
        Language::Dutch,
        "nl_NL",
        "Nederlands",
        "Dutch",
        "🇳🇱",
        true,   // is_beta
        false,
        "%d-%m-%Y",
        "%H:%M:%S",
        ",.",
        ",",
        "."
    }},
    {Language::Polish, {
        Language::Polish,
        "pl_PL",
        "Polski",
        "Polish",
        "🇵🇱",
        true,
        false,
        "%d.%m.%Y",
        "%H:%M:%S",
        ", ",
        ",",
        " "
    }},
    {Language::Russian, {
        Language::Russian,
        "ru_RU",
        "Русский",
        "Russian",
        "🇷🇺",
        true,
        false,
        "%d.%m.%Y",
        "%H:%M:%S",
        ", ",
        ",",
        " "
    }},
    {Language::Japanese, {
        Language::Japanese,
        "ja_JP",
        "日本語",
        "Japanese",
        "🇯🇵",
        true,
        false,
        "%Y/%m/%d",
        "%H:%M:%S",
        ".,",
        ".",
        ","
    }},
    {Language::Chinese, {
        Language::Chinese,
        "zh_CN",
        "中文",
        "Chinese",
        "🇨🇳",
        true,
        false,
        "%Y-%m-%d",
        "%H:%M:%S",
        ".,",
        ".",
        ","
    }},
    {Language::Korean, {
        Language::Korean,
        "ko_KR",
        "한국어",
        "Korean",
        "🇰🇷",
        true,
        false,
        "%Y-%m-%d",
        "%H:%M:%S",
        ".,",
        ".",
        ","
    }}
};

const LanguageInfo& GetLanguageInfo(Language lang) {
    auto it = kLanguageInfo.find(lang);
    if (it != kLanguageInfo.end()) {
        return it->second;
    }
    // Return English as fallback
    return kLanguageInfo.at(Language::English);
}

std::vector<LanguageInfo> GetSupportedLanguages() {
    std::vector<LanguageInfo> result;
    for (const auto& [code, info] : kLanguageInfo) {
        if (!info.is_beta) {
            result.push_back(info);
        }
    }
    // Sort by English name
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) {
                  return a.english_name < b.english_name;
              });
    return result;
}

std::vector<LanguageInfo> GetAllLanguages() {
    std::vector<LanguageInfo> result;
    for (const auto& [code, info] : kLanguageInfo) {
        result.push_back(info);
    }
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) {
                  return a.english_name < b.english_name;
              });
    return result;
}

Language LocaleCodeToLanguage(const std::string& code) {
    std::string lower = code;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    for (const auto& [lang, info] : kLanguageInfo) {
        std::string info_lower = info.locale_code;
        std::transform(info_lower.begin(), info_lower.end(), info_lower.begin(), ::tolower);
        if (info_lower == lower) {
            return lang;
        }
    }
    
    // Try matching just the language part (e.g., "en" matches "en_CA")
    if (lower.length() >= 2) {
        std::string lang_part = lower.substr(0, 2);
        for (const auto& [lang, info] : kLanguageInfo) {
            std::string info_lower = info.locale_code;
            std::transform(info_lower.begin(), info_lower.end(), info_lower.begin(), ::tolower);
            if (info_lower.substr(0, 2) == lang_part) {
                return lang;
            }
        }
    }
    
    return Language::Default;
}

std::string LanguageToLocaleCode(Language lang) {
    return GetLanguageInfo(lang).locale_code;
}

Language GetSystemDefaultLanguage() {
    // Get system locale
    const char* locale = std::getenv("LANG");
    if (!locale) {
        locale = std::getenv("LC_ALL");
    }
    if (!locale) {
        return Language::Default;
    }
    
    std::string locale_str(locale);
    // Extract language code (e.g., "en_CA.UTF-8" -> "en_CA")
    size_t dot_pos = locale_str.find('.');
    if (dot_pos != std::string::npos) {
        locale_str = locale_str.substr(0, dot_pos);
    }
    
    return LocaleCodeToLanguage(locale_str);
}

bool IsLanguageSupported(Language lang) {
    return !GetLanguageInfo(lang).is_beta;
}

std::string FormatDate(std::time_t timestamp, Language lang) {
    const auto& info = GetLanguageInfo(lang);
    
    std::tm* tm = std::localtime(&timestamp);
    if (!tm) return "";
    
    std::ostringstream oss;
    oss << std::put_time(tm, info.date_format.c_str());
    return oss.str();
}

std::string FormatNumber(double value, int decimals, Language lang) {
    const auto& info = GetLanguageInfo(lang);
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value;
    std::string result = oss.str();
    
    // Replace decimal point if needed
    if (info.decimal_separator != ".") {
        size_t dot_pos = result.find('.');
        if (dot_pos != std::string::npos) {
            result.replace(dot_pos, 1, info.decimal_separator);
        }
    }
    
    // Add thousands separator
    if (!info.thousands_separator.empty() && info.thousands_separator != "") {
        size_t sep_pos = result.find(info.decimal_separator);
        if (sep_pos == std::string::npos) {
            sep_pos = result.length();
        }
        
        int count = 0;
        for (int i = static_cast<int>(sep_pos) - 1; i > 0; --i) {
            if (++count == 3) {
                result.insert(i, info.thousands_separator);
                count = 0;
            }
        }
    }
    
    return result;
}

std::string FormatCurrency(double value, const std::string& currency_code, Language lang) {
    const auto& info = GetLanguageInfo(lang);
    
    std::string formatted = FormatNumber(value, 2, lang);
    
    // Currency symbols
    static const std::map<std::string, std::string> kCurrencySymbols = {
        {"USD", "$"},
        {"EUR", "€"},
        {"GBP", "£"},
        {"CAD", "C$"},
        {"JPY", "¥"},
        {"CNY", "¥"},
        {"KRW", "₩"},
        {"BRL", "R$"},
        {"MXN", "$"},
        {"AUD", "A$"}
    };
    
    auto it = kCurrencySymbols.find(currency_code);
    std::string symbol = (it != kCurrencySymbols.end()) ? it->second : currency_code + " ";
    
    // Position symbol based on language
    if (lang == Language::French || lang == Language::German || lang == Language::Spanish ||
        lang == Language::Portuguese || lang == Language::Italian) {
        // Symbol after amount in many European locales
        return formatted + " " + symbol;
    }
    
    // Symbol before amount (default)
    return symbol + formatted;
}

} // namespace i18n
} // namespace scratchrobin
