/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */

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
    QSettings settings(QString::fromStdString(getConfigPath()), QSettings::IniFormat);
    
    // Load theme
    theme_ = stringToTheme(settings.value("UI/theme", "system").toString().toStdString());
    
    // Load scale settings
    scales_.master_scale = settings.value("Scales/master", 100).toInt();
    scales_.menu_font_scale = settings.value("Scales/menu_font", 100).toInt();
    scales_.tree_text_scale = settings.value("Scales/tree_text", 100).toInt();
    scales_.tree_icon_scale = settings.value("Scales/tree_icon", 100).toInt();
    scales_.dialog_font_scale = settings.value("Scales/dialog_font", 100).toInt();
    scales_.editor_font_scale = settings.value("Scales/editor_font", 100).toInt();
    scales_.status_bar_scale = settings.value("Scales/status_bar", 100).toInt();
    
    // Load color settings
    colors_.background_r = settings.value("Colors/background_r", 240).toInt();
    colors_.background_g = settings.value("Colors/background_g", 240).toInt();
    colors_.background_b = settings.value("Colors/background_b", 240).toInt();
    colors_.foreground_r = settings.value("Colors/foreground_r", 0).toInt();
    colors_.foreground_g = settings.value("Colors/foreground_g", 0).toInt();
    colors_.foreground_b = settings.value("Colors/foreground_b", 0).toInt();
    
    // Validate loaded values
    if (scales_.master_scale < 50 || scales_.master_scale > 200) {
        scales_.master_scale = 100;
    }
    if (scales_.menu_font_scale < 50 || scales_.menu_font_scale > 200) {
        scales_.menu_font_scale = 100;
    }
    if (scales_.tree_text_scale < 50 || scales_.tree_text_scale > 200) {
        scales_.tree_text_scale = 100;
    }
    if (scales_.tree_icon_scale < 50 || scales_.tree_icon_scale > 200) {
        scales_.tree_icon_scale = 100;
    }
    if (scales_.dialog_font_scale < 50 || scales_.dialog_font_scale > 200) {
        scales_.dialog_font_scale = 100;
    }
    if (scales_.editor_font_scale < 50 || scales_.editor_font_scale > 200) {
        scales_.editor_font_scale = 100;
    }
    if (scales_.status_bar_scale < 50 || scales_.status_bar_scale > 200) {
        scales_.status_bar_scale = 100;
    }
}

void Settings::save() {
    QSettings settings(QString::fromStdString(getConfigPath()), QSettings::IniFormat);
    
    // Save theme
    settings.setValue("UI/theme", QString::fromStdString(themeToString(theme_)));
    
    // Save scale settings
    settings.setValue("Scales/master", scales_.master_scale);
    settings.setValue("Scales/menu_font", scales_.menu_font_scale);
    settings.setValue("Scales/tree_text", scales_.tree_text_scale);
    settings.setValue("Scales/tree_icon", scales_.tree_icon_scale);
    settings.setValue("Scales/dialog_font", scales_.dialog_font_scale);
    settings.setValue("Scales/editor_font", scales_.editor_font_scale);
    settings.setValue("Scales/status_bar", scales_.status_bar_scale);
    
    // Save color settings
    settings.setValue("Colors/background_r", colors_.background_r);
    settings.setValue("Colors/background_g", colors_.background_g);
    settings.setValue("Colors/background_b", colors_.background_b);
    settings.setValue("Colors/foreground_r", colors_.foreground_r);
    settings.setValue("Colors/foreground_g", colors_.foreground_g);
    settings.setValue("Colors/foreground_b", colors_.foreground_b);
    
    settings.sync();
}

void Settings::resetToDefaults() {
    theme_ = Theme::kSystem;
    scales_ = {};
    scales_.master_scale = 100;
    colors_ = {};
    colors_.background_r = 240;
    colors_.background_g = 240;
    colors_.background_b = 240;
    colors_.foreground_r = 0;
    colors_.foreground_g = 0;
    colors_.foreground_b = 0;
}

std::string Settings::getConfigPath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return (configDir + "/scratchrobin-settings.ini").toStdString();
}

}  // namespace scratchrobin::core
