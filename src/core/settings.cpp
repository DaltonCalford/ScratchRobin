#include "core/settings.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>

namespace scratchrobin::core {

std::string themeToString(Theme theme) {
    switch (theme) {
        case Theme::kLight: return "light";
        case Theme::kDark: return "dark";
        case Theme::kSystem: return "system";
    }
    return "system";
}

Theme stringToTheme(const std::string& str) {
    if (str == "light") return Theme::kLight;
    if (str == "dark") return Theme::kDark;
    return Theme::kSystem;
}

Settings& Settings::get() {
    static Settings instance;
    return instance;
}

void Settings::load() {
    // Stub - would load from QSettings
    theme_ = Theme::kSystem;
    scales_.master_scale = 100;
    scales_.menu_font_scale = 100;
    scales_.tree_text_scale = 100;
}

void Settings::save() {
    // Stub - would save to QSettings
}

void Settings::resetToDefaults() {
    theme_ = Theme::kSystem;
    scales_ = {};
    scales_.master_scale = 100;
}

std::string Settings::getConfigPath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return (configDir + "/scratchrobin-settings.ini").toStdString();
}

}  // namespace scratchrobin::core
