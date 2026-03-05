/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/settings.h"

#include <wx/config.h>
#include <wx/settings.h>
#include <wx/window.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <algorithm>

namespace scratchrobin::core {

Settings& Settings::get() {
    static Settings instance;
    return instance;
}

std::string Settings::getConfigPath() const {
    wxString config_dir = wxStandardPaths::Get().GetUserConfigDir();
    if (!wxDirExists(config_dir)) {
        wxMkdir(config_dir, wxS_DIR_DEFAULT);
    }
    return (config_dir + "/scratchrobin-settings.ini").ToStdString();
}

void Settings::load() {
    wxFileName config_path(wxString::FromUTF8(getConfigPath().c_str()));
    wxConfig config(wxT("ScratchRobin"), wxEmptyString, 
                    config_path.GetFullPath(),
                    wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
    
    // Load theme
    wxString theme_str;
    if (config.Read(wxT("Theme"), &theme_str)) {
        theme_ = stringToTheme(theme_str.ToStdString());
    }
    
    // Load scales
    config.Read(wxT("MasterScale"), &scales_.master_scale, 100);
    config.Read(wxT("MenuFontScale"), &scales_.menu_font_scale, 100);
    config.Read(wxT("TreeTextScale"), &scales_.tree_text_scale, 100);
    config.Read(wxT("TreeIconScale"), &scales_.tree_icon_scale, 100);
    config.Read(wxT("DialogFontScale"), &scales_.dialog_font_scale, 100);
    config.Read(wxT("EditorFontScale"), &scales_.editor_font_scale, 100);
    config.Read(wxT("StatusBarScale"), &scales_.status_bar_scale, 100);
    
    // Clamp values
    scales_.master_scale = std::clamp(scales_.master_scale, 50, 300);
    scales_.menu_font_scale = std::clamp(scales_.menu_font_scale, 50, 300);
    scales_.tree_text_scale = std::clamp(scales_.tree_text_scale, 50, 300);
    scales_.tree_icon_scale = std::clamp(scales_.tree_icon_scale, 50, 300);
    scales_.dialog_font_scale = std::clamp(scales_.dialog_font_scale, 50, 300);
    scales_.editor_font_scale = std::clamp(scales_.editor_font_scale, 50, 300);
    scales_.status_bar_scale = std::clamp(scales_.status_bar_scale, 50, 300);
}

void Settings::save() const {
    wxFileName config_path(wxString::FromUTF8(getConfigPath().c_str()));
    wxConfig config(wxT("ScratchRobin"), wxEmptyString,
                    config_path.GetFullPath(),
                    wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
    
    config.Write(wxT("Theme"), wxString::FromUTF8(themeToString(theme_).c_str()));
    config.Write(wxT("MasterScale"), scales_.master_scale);
    config.Write(wxT("MenuFontScale"), scales_.menu_font_scale);
    config.Write(wxT("TreeTextScale"), scales_.tree_text_scale);
    config.Write(wxT("TreeIconScale"), scales_.tree_icon_scale);
    config.Write(wxT("DialogFontScale"), scales_.dialog_font_scale);
    config.Write(wxT("EditorFontScale"), scales_.editor_font_scale);
    config.Write(wxT("StatusBarScale"), scales_.status_bar_scale);
}

void Settings::resetToDefaults() {
    theme_ = Theme::kLight;
    scales_ = ScaleSettings{};  // All defaults to 100
    notifyChanged();
}

void Settings::setTheme(Theme theme) {
    if (theme_ != theme) {
        theme_ = theme;
        notifyChanged();
    }
}

std::string Settings::getThemeName() const {
    return themeToString(theme_);
}

std::string Settings::themeToString(Theme theme) {
    switch (theme) {
        case Theme::kLight: return "light";
        case Theme::kDark: return "dark";
        case Theme::kHighContrast: return "highcontrast";
        case Theme::kNight: return "night";
    }
    return "light";
}

Theme Settings::stringToTheme(const std::string& str) {
    if (str == "dark") return Theme::kDark;
    if (str == "highcontrast") return Theme::kHighContrast;
    if (str == "night") return Theme::kNight;
    return Theme::kLight;
}

ThemeColors Settings::getThemeColors() const {
    ThemeColors colors;
    
    switch (theme_) {
        case Theme::kLight:
            colors.background = wxColour(255, 255, 255);
            colors.foreground = wxColour(0, 0, 0);
            colors.accent = wxColour(0, 120, 215);
            colors.selection_bg = wxColour(0, 120, 215);
            colors.selection_fg = wxColour(255, 255, 255);
            colors.border = wxColour(200, 200, 200);
            colors.tree_bg = wxColour(255, 255, 255);
            colors.tree_fg = wxColour(0, 0, 0);
            colors.tree_lines = wxColour(160, 160, 160);
            colors.button_bg = wxColour(240, 240, 240);
            colors.button_fg = wxColour(0, 0, 0);
            colors.input_bg = wxColour(255, 255, 255);
            colors.input_fg = wxColour(0, 0, 0);
            colors.status_bg = wxColour(240, 240, 240);
            colors.status_fg = wxColour(0, 0, 0);
            break;
            
        case Theme::kDark:
            colors.background = wxColour(45, 45, 48);
            colors.foreground = wxColour(230, 230, 230);
            colors.accent = wxColour(0, 151, 251);
            colors.selection_bg = wxColour(0, 120, 215);
            colors.selection_fg = wxColour(255, 255, 255);
            colors.border = wxColour(80, 80, 80);
            colors.tree_bg = wxColour(37, 37, 38);
            colors.tree_fg = wxColour(230, 230, 230);
            colors.tree_lines = wxColour(100, 100, 100);
            colors.button_bg = wxColour(62, 62, 66);
            colors.button_fg = wxColour(230, 230, 230);
            colors.input_bg = wxColour(51, 51, 55);
            colors.input_fg = wxColour(230, 230, 230);
            colors.status_bg = wxColour(45, 45, 48);
            colors.status_fg = wxColour(230, 230, 230);
            break;
            
        case Theme::kHighContrast:
            colors.background = wxColour(0, 0, 0);
            colors.foreground = wxColour(255, 255, 255);
            colors.accent = wxColour(255, 255, 0);
            colors.selection_bg = wxColour(0, 255, 255);
            colors.selection_fg = wxColour(0, 0, 0);
            colors.border = wxColour(255, 255, 255);
            colors.tree_bg = wxColour(0, 0, 0);
            colors.tree_fg = wxColour(255, 255, 255);
            colors.tree_lines = wxColour(255, 255, 255);
            colors.button_bg = wxColour(0, 0, 0);
            colors.button_fg = wxColour(255, 255, 255);
            colors.input_bg = wxColour(0, 0, 0);
            colors.input_fg = wxColour(255, 255, 255);
            colors.status_bg = wxColour(0, 0, 0);
            colors.status_fg = wxColour(255, 255, 255);
            break;
            
        case Theme::kNight:
            colors.background = wxColour(30, 30, 46);
            colors.foreground = wxColour(205, 214, 244);
            colors.accent = wxColour(137, 180, 250);
            colors.selection_bg = wxColour(88, 91, 112);
            colors.selection_fg = wxColour(205, 214, 244);
            colors.border = wxColour(69, 71, 90);
            colors.tree_bg = wxColour(24, 24, 37);
            colors.tree_fg = wxColour(205, 214, 244);
            colors.tree_lines = wxColour(88, 91, 112);
            colors.button_bg = wxColour(49, 50, 68);
            colors.button_fg = wxColour(205, 214, 244);
            colors.input_bg = wxColour(49, 50, 68);
            colors.input_fg = wxColour(205, 214, 244);
            colors.status_bg = wxColour(30, 30, 46);
            colors.status_fg = wxColour(205, 214, 244);
            break;
    }
    
    return colors;
}

void Settings::setScaleSettings(const ScaleSettings& scales) {
    scales_ = scales;
    // Clamp all values
    scales_.master_scale = std::clamp(scales_.master_scale, 50, 300);
    scales_.menu_font_scale = std::clamp(scales_.menu_font_scale, 50, 300);
    scales_.tree_text_scale = std::clamp(scales_.tree_text_scale, 50, 300);
    scales_.tree_icon_scale = std::clamp(scales_.tree_icon_scale, 50, 300);
    scales_.dialog_font_scale = std::clamp(scales_.dialog_font_scale, 50, 300);
    scales_.editor_font_scale = std::clamp(scales_.editor_font_scale, 50, 300);
    scales_.status_bar_scale = std::clamp(scales_.status_bar_scale, 50, 300);
    notifyChanged();
}

void Settings::setMasterScale(int percentage) {
    percentage = std::clamp(percentage, 50, 300);
    if (scales_.master_scale != percentage) {
        scales_.master_scale = percentage;
        notifyChanged();
    }
}

void Settings::setMenuFontScale(int percentage) {
    percentage = std::clamp(percentage, 50, 300);
    if (scales_.menu_font_scale != percentage) {
        scales_.menu_font_scale = percentage;
        notifyChanged();
    }
}

void Settings::setTreeTextScale(int percentage) {
    percentage = std::clamp(percentage, 50, 300);
    if (scales_.tree_text_scale != percentage) {
        scales_.tree_text_scale = percentage;
        notifyChanged();
    }
}

void Settings::setTreeIconScale(int percentage) {
    percentage = std::clamp(percentage, 50, 300);
    if (scales_.tree_icon_scale != percentage) {
        scales_.tree_icon_scale = percentage;
        notifyChanged();
    }
}

void Settings::setDialogFontScale(int percentage) {
    percentage = std::clamp(percentage, 50, 300);
    if (scales_.dialog_font_scale != percentage) {
        scales_.dialog_font_scale = percentage;
        notifyChanged();
    }
}

void Settings::setEditorFontScale(int percentage) {
    percentage = std::clamp(percentage, 50, 300);
    if (scales_.editor_font_scale != percentage) {
        scales_.editor_font_scale = percentage;
        notifyChanged();
    }
}

void Settings::setStatusBarScale(int percentage) {
    percentage = std::clamp(percentage, 50, 300);
    if (scales_.status_bar_scale != percentage) {
        scales_.status_bar_scale = percentage;
        notifyChanged();
    }
}

void Settings::addChangeCallback(ChangeCallback cb) {
    callbacks_.push_back(cb);
}

void Settings::notifyChanged() {
    for (auto& cb : callbacks_) {
        cb();
    }
}

void Settings::applyToWindow(wxWindow* window) const {
    if (!window) return;
    
    const ThemeColors colors = getThemeColors();
    applyThemeToWindow(window, colors);
    
    // Apply fonts
    const int base_font_size = 9;  // Base point size
    
    wxFont font = window->GetFont();
    font.SetPointSize(scales_.getFontSize(base_font_size, scales_.dialog_font_scale));
    window->SetFont(font);
    
    // Recursively apply to children
    for (wxWindow* child : window->GetChildren()) {
        applyToWindow(child);
    }
}

wxFont getScaledFont(int base_point_size, int scale_percentage,
                     wxFontFamily family,
                     wxFontStyle style,
                     wxFontWeight weight) {
    int scaled_size = (base_point_size * scale_percentage) / 100;
    return wxFont(scaled_size, family, style, weight);
}

void applyThemeToWindow(wxWindow* window, const ThemeColors& colors) {
    if (!window) return;
    
    window->SetBackgroundColour(colors.background);
    window->SetForegroundColour(colors.foreground);
    
    // Special handling for specific window types
    if (wxStatusBar* status = wxDynamicCast(window, wxStatusBar)) {
        status->SetBackgroundColour(colors.status_bg);
        status->SetForegroundColour(colors.status_fg);
    }
}

}  // namespace scratchrobin::core
