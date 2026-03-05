/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <wx/colour.h>
#include <wx/font.h>
#include <wx/gdicmn.h>

// Forward declaration
class wxWindow;

namespace scratchrobin::core {

// Theme types
enum class Theme {
    kLight,
    kDark,
    kHighContrast,
    kNight
};

// Scale factors (percentage-based, 100 = normal)
struct ScaleSettings {
    int master_scale{100};           // Master scaler (affects all)
    int menu_font_scale{100};        // Menu bar font size
    int tree_text_scale{100};        // Tree item text size
    int tree_icon_scale{100};        // Tree icon size (base 16px)
    int dialog_font_scale{100};      // Dialog text size
    int editor_font_scale{100};      // SQL editor font size
    int status_bar_scale{100};       // Status bar text size
    
    // Calculate effective scale for an element (master * element / 100)
    int getEffectiveScale(int element_scale) const {
        return (master_scale * element_scale) / 100;
    }
    
    // Get tree icon size in pixels (base 16px)
    int getTreeIconSize() const {
        return (16 * getEffectiveScale(tree_icon_scale)) / 100;
    }
    
    // Get font point size with scaling (base size)
    int getFontSize(int base_size, int element_scale) const {
        return (base_size * getEffectiveScale(element_scale)) / 100;
    }
};

// Color customization for themes
struct ThemeColors {
    wxColour background;
    wxColour foreground;
    wxColour accent;
    wxColour selection_bg;
    wxColour selection_fg;
    wxColour border;
    wxColour tree_bg;
    wxColour tree_fg;
    wxColour tree_lines;
    wxColour button_bg;
    wxColour button_fg;
    wxColour input_bg;
    wxColour input_fg;
    wxColour status_bg;
    wxColour status_fg;
};

// Complete application settings
class Settings {
public:
    static Settings& get();
    
    // Load/save from config file
    void load();
    void save() const;
    void resetToDefaults();
    
    // Theme
    Theme getTheme() const { return theme_; }
    void setTheme(Theme theme);
    std::string getThemeName() const;
    static std::string themeToString(Theme theme);
    static Theme stringToTheme(const std::string& str);
    
    // Colors for current theme
    ThemeColors getThemeColors() const;
    
    // Scaling
    const ScaleSettings& getScaleSettings() const { return scales_; }
    void setScaleSettings(const ScaleSettings& scales);
    void setMasterScale(int percentage);  // 50-300
    void setMenuFontScale(int percentage);
    void setTreeTextScale(int percentage);
    void setTreeIconScale(int percentage);
    void setDialogFontScale(int percentage);
    void setEditorFontScale(int percentage);
    void setStatusBarScale(int percentage);
    
    // Apply settings to a window and its children
    void applyToWindow(wxWindow* window) const;
    
    // Events - called when settings change
    using ChangeCallback = std::function<void()>;
    void addChangeCallback(ChangeCallback cb);
    void notifyChanged();

private:
    Settings() = default;
    
    Theme theme_{Theme::kLight};
    ScaleSettings scales_;
    std::vector<ChangeCallback> callbacks_;
    
    // Config file path
    std::string getConfigPath() const;
};

// Helper to get scaled font
wxFont getScaledFont(int base_point_size, int scale_percentage, 
                     wxFontFamily family = wxFONTFAMILY_DEFAULT,
                     wxFontStyle style = wxFONTSTYLE_NORMAL,
                     wxFontWeight weight = wxFONTWEIGHT_NORMAL);

// Helper to apply theme to a window
void applyThemeToWindow(wxWindow* window, const ThemeColors& colors);

}  // namespace scratchrobin::core
