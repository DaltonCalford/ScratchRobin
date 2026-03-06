#pragma once

#include <string>

namespace scratchrobin::core {

enum class Theme { kLight, kDark, kSystem };

std::string themeToString(Theme theme);
Theme stringToTheme(const std::string& str);

struct ScaleSettings {
    int master_scale = 100;
    int menu_font_scale = 100;
    int tree_text_scale = 100;
    int tree_icon_scale = 100;
    int dialog_font_scale = 100;
    int editor_font_scale = 100;
    int status_bar_scale = 100;
};

struct ColorSettings {
    int background_r = 240, background_g = 240, background_b = 240;
    int foreground_r = 0, foreground_g = 0, foreground_b = 0;
};

class Settings {
public:
    static Settings& get();
    
    void load();
    void save();
    void resetToDefaults();
    
    Theme getTheme() const { return theme_; }
    const ScaleSettings& getScales() const { return scales_; }
    const ColorSettings& getColors() const { return colors_; }
    
    void setTheme(Theme theme) { theme_ = theme; }
    void setScales(const ScaleSettings& scales) { scales_ = scales; }
    void setColors(const ColorSettings& colors) { colors_ = colors; }

private:
    Settings() = default;
    std::string getConfigPath() const;
    
    Theme theme_ = Theme::kSystem;
    ScaleSettings scales_;
    ColorSettings colors_;
};

}  // namespace scratchrobin::core
