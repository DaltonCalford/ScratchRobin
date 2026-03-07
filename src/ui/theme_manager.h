#pragma once

#include <QObject>
#include <QColor>
#include <QFont>
#include <QString>
#include <QPalette>
#include <QMap>

namespace scratchrobin::ui {

enum class ThemeType {
    Light,
    Dark,
    Night,    // Darker with warmer tones for night use
    Custom
};

struct ThemeColors {
    // Window backgrounds
    QColor windowBackground;
    QColor widgetBackground;
    QColor alternateBackground;
    
    // Text colors
    QColor text;
    QColor textDisabled;
    QColor textHighlight;
    
    // Accent colors
    QColor primary;
    QColor primaryLight;
    QColor primaryDark;
    QColor secondary;
    
    // Border colors
    QColor border;
    QColor borderFocus;
    QColor borderHover;
    
    // Status colors
    QColor success;
    QColor warning;
    QColor error;
    QColor info;
    
    // Special UI colors
    QColor selection;
    QColor selectionText;
    QColor link;
    QColor linkVisited;
    
    // SQL Editor specific
    QColor editorBackground;
    QColor editorLineNumber;
    QColor editorCurrentLine;
    QColor editorKeyword;
    QColor editorString;
    QColor editorComment;
    QColor editorNumber;
    QColor editorFunction;
    QColor editorOperator;
    
    // Grid colors
    QColor gridBackground;
    QColor gridAlternateRow;
    QColor gridSelection;
    QColor gridBorder;
};

struct ThemeFonts {
    QFont application;
    QFont menu;
    QFont toolbar;
    QFont editor;
    QFont editorLineNumbers;
    QFont dataGrid;
    QFont statusBar;
    QFont dialog;
    QFont fixedWidth;  // For code/monospace
};

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();
    
    // Theme management
    void setTheme(ThemeType type);
    void setCustomTheme(const ThemeColors& colors);
    ThemeType currentTheme() const { return current_theme_; }
    
    // Apply theme to application
    void applyTheme();
    void applyToWidget(QWidget* widget);
    
    // Get current colors
    ThemeColors colors() const;
    ThemeFonts fonts() const;
    
    // Font management
    void setApplicationFont(const QFont& font);
    void setEditorFont(const QFont& font);
    void setDataGridFont(const QFont& font);
    void setFontSize(int size);
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();
    
    // Getters for fonts
    QFont applicationFont() const { return fonts_.application; }
    QFont editorFont() const { return fonts_.editor; }
    QFont dataGridFont() const { return fonts_.dataGrid; }
    int baseFontSize() const { return base_font_size_; }
    ThemeType currentThemeType() const { return current_theme_; }
    
    // Color customization
    void setCustomColor(const QString& name, const QColor& color);
    QColor getColor(const QString& name) const;
    
    // Editor color scheme
    void setEditorColorScheme(const QString& schemeName);
    QStringList availableEditorSchemes() const;
    
    // Save/Load
    void saveThemeSettings();
    void loadThemeSettings();
    
    // Get predefined themes
    static ThemeColors lightTheme();
    static ThemeColors darkTheme();
    static ThemeColors nightTheme();
    
signals:
    void themeChanged(ThemeType type);
    void colorsChanged();
    void fontsChanged();

private:
    ThemeManager();
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    
    void applyToMenuBar(QWidget* widget);
    void applyToToolBar(QWidget* widget);
    void applyToStatusBar(QWidget* widget);
    void applyToEditor(QWidget* widget);
    void applyToDataGrid(QWidget* widget);
    void applyToButton(QWidget* widget);
    void applyToInput(QWidget* widget);
    void applyToTreeView(QWidget* widget);
    void applyToTabWidget(QWidget* widget);
    void applyToGroupBox(QWidget* widget);
    
    QString generateStyleSheet() const;
    QPalette generatePalette() const;
    
    ThemeType current_theme_ = ThemeType::Light;
    ThemeColors custom_colors_;
    ThemeFonts fonts_;
    int base_font_size_ = 10;
    bool use_system_font_ = true;
    
    QMap<QString, QColor> custom_color_overrides_;
};

// Theme-aware palette generator
class ThemePalette {
public:
    static QPalette createPalette(const ThemeColors& colors);
    static QString createStyleSheet(const ThemeColors& colors);
};

} // namespace scratchrobin::ui
