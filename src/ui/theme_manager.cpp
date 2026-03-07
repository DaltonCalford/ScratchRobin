#include "ui/theme_manager.h"

#include <QApplication>
#include <QWidget>
#include <QPalette>
#include <QSettings>
#include <QDebug>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTreeView>
#include <QTabWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QStyleFactory>

namespace scratchrobin::ui {

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() {
    loadThemeSettings();
}

ThemeColors ThemeManager::lightTheme() {
    ThemeColors c;
    // Window backgrounds
    c.windowBackground = QColor(240, 240, 240);
    c.widgetBackground = QColor(255, 255, 255);
    c.alternateBackground = QColor(245, 245, 245);
    
    // Text colors
    c.text = QColor(33, 33, 33);
    c.textDisabled = QColor(128, 128, 128);
    c.textHighlight = QColor(255, 255, 255);
    
    // Accent colors
    c.primary = QColor(0, 120, 215);
    c.primaryLight = QColor(100, 170, 235);
    c.primaryDark = QColor(0, 80, 160);
    c.secondary = QColor(108, 117, 125);
    
    // Border colors
    c.border = QColor(200, 200, 200);
    c.borderFocus = QColor(0, 120, 215);
    c.borderHover = QColor(160, 160, 160);
    
    // Status colors
    c.success = QColor(40, 167, 69);
    c.warning = QColor(255, 193, 7);
    c.error = QColor(220, 53, 69);
    c.info = QColor(23, 162, 184);
    
    // Selection
    c.selection = QColor(0, 120, 215);
    c.selectionText = QColor(255, 255, 255);
    c.link = QColor(0, 102, 204);
    c.linkVisited = QColor(128, 0, 128);
    
    // Editor colors
    c.editorBackground = QColor(255, 255, 255);
    c.editorLineNumber = QColor(245, 245, 245);
    c.editorCurrentLine = QColor(240, 248, 255);
    c.editorKeyword = QColor(0, 0, 255);
    c.editorString = QColor(163, 21, 21);
    c.editorComment = QColor(0, 128, 0);
    c.editorNumber = QColor(9, 134, 88);
    c.editorFunction = QColor(121, 94, 38);
    c.editorOperator = QColor(0, 0, 0);
    
    // Grid colors
    c.gridBackground = QColor(255, 255, 255);
    c.gridAlternateRow = QColor(245, 245, 245);
    c.gridSelection = QColor(0, 120, 215);
    c.gridBorder = QColor(200, 200, 200);
    
    return c;
}

ThemeColors ThemeManager::darkTheme() {
    ThemeColors c;
    // Window backgrounds
    c.windowBackground = QColor(45, 45, 48);
    c.widgetBackground = QColor(55, 55, 58);
    c.alternateBackground = QColor(50, 50, 53);
    
    // Text colors
    c.text = QColor(230, 230, 230);
    c.textDisabled = QColor(128, 128, 128);
    c.textHighlight = QColor(255, 255, 255);
    
    // Accent colors
    c.primary = QColor(0, 150, 255);
    c.primaryLight = QColor(80, 180, 255);
    c.primaryDark = QColor(0, 100, 200);
    c.secondary = QColor(108, 117, 125);
    
    // Border colors
    c.border = QColor(80, 80, 80);
    c.borderFocus = QColor(0, 150, 255);
    c.borderHover = QColor(100, 100, 100);
    
    // Status colors
    c.success = QColor(72, 187, 120);
    c.warning = QColor(236, 201, 75);
    c.error = QColor(245, 101, 101);
    c.info = QColor(66, 153, 225);
    
    // Selection
    c.selection = QColor(0, 150, 255);
    c.selectionText = QColor(255, 255, 255);
    c.link = QColor(100, 180, 255);
    c.linkVisited = QColor(200, 100, 200);
    
    // Editor colors
    c.editorBackground = QColor(30, 30, 30);
    c.editorLineNumber = QColor(45, 45, 45);
    c.editorCurrentLine = QColor(50, 50, 50);
    c.editorKeyword = QColor(86, 156, 214);
    c.editorString = QColor(206, 145, 120);
    c.editorComment = QColor(106, 153, 85);
    c.editorNumber = QColor(181, 206, 168);
    c.editorFunction = QColor(220, 220, 170);
    c.editorOperator = QColor(212, 212, 212);
    
    // Grid colors
    c.gridBackground = QColor(45, 45, 48);
    c.gridAlternateRow = QColor(55, 55, 58);
    c.gridSelection = QColor(0, 150, 255);
    c.gridBorder = QColor(80, 80, 80);
    
    return c;
}

ThemeColors ThemeManager::nightTheme() {
    ThemeColors c = darkTheme();
    // Even darker with warmer tones for night use
    c.windowBackground = QColor(30, 30, 35);
    c.widgetBackground = QColor(38, 38, 43);
    c.alternateBackground = QColor(35, 35, 40);
    
    c.editorBackground = QColor(25, 25, 28);
    c.editorLineNumber = QColor(35, 35, 38);
    c.editorCurrentLine = QColor(40, 40, 43);
    
    // Reduced blue light
    c.primary = QColor(100, 150, 200);
    c.editorKeyword = QColor(120, 170, 220);
    
    return c;
}

ThemeColors ThemeManager::colors() const {
    switch (current_theme_) {
        case ThemeType::Dark:
            return darkTheme();
        case ThemeType::Night:
            return nightTheme();
        case ThemeType::Custom:
            return custom_colors_;
        case ThemeType::Light:
        default:
            return lightTheme();
    }
}

void ThemeManager::setTheme(ThemeType type) {
    current_theme_ = type;
    applyTheme();
    emit themeChanged(type);
}

void ThemeManager::setCustomTheme(const ThemeColors& colors) {
    custom_colors_ = colors;
    current_theme_ = ThemeType::Custom;
    applyTheme();
    emit themeChanged(ThemeType::Custom);
}

void ThemeManager::applyTheme() {
    ThemeColors c = colors();
    
    // Generate and apply palette
    QPalette palette = ThemePalette::createPalette(c);
    qApp->setPalette(palette);
    
    // Apply stylesheet for more detailed styling
    QString styleSheet = ThemePalette::createStyleSheet(c);
    qApp->setStyleSheet(styleSheet);
    
    // Apply fonts
    if (!fonts_.application.family().isEmpty()) {
        qApp->setFont(fonts_.application);
    }
    
    // Update all existing widgets
    for (auto* widget : qApp->allWidgets()) {
        applyToWidget(widget);
    }
    
    emit colorsChanged();
}

void ThemeManager::applyToWidget(QWidget* widget) {
    if (!widget) return;
    
    // Apply specific styling based on widget type
    if (qobject_cast<QMenuBar*>(widget)) {
        applyToMenuBar(widget);
    } else if (qobject_cast<QToolBar*>(widget)) {
        applyToToolBar(widget);
    } else if (qobject_cast<QStatusBar*>(widget)) {
        applyToStatusBar(widget);
    } else if (qobject_cast<QPushButton*>(widget)) {
        applyToButton(widget);
    } else if (qobject_cast<QLineEdit*>(widget) || qobject_cast<QTextEdit*>(widget)) {
        applyToInput(widget);
    } else if (qobject_cast<QTreeView*>(widget)) {
        applyToTreeView(widget);
    } else if (qobject_cast<QTabWidget*>(widget)) {
        applyToTabWidget(widget);
    } else if (qobject_cast<QGroupBox*>(widget)) {
        applyToGroupBox(widget);
    }
}

void ThemeManager::applyToMenuBar(QWidget* widget) {
    ThemeColors c = colors();
    widget->setStyleSheet(QString(
        "QMenuBar {"
        "    background-color: %1;"
        "    color: %2;"
        "    border-bottom: 1px solid %3;"
        "}"
        "QMenuBar::item {"
        "    background: transparent;"
        "    padding: 4px 12px;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: %4;"
        "    border-radius: 3px;"
        "}"
    ).arg(c.windowBackground.name())
     .arg(c.text.name())
     .arg(c.border.name())
     .arg(c.primary.name()));
}

void ThemeManager::applyToToolBar(QWidget* widget) {
    ThemeColors c = colors();
    widget->setStyleSheet(QString(
        "QToolBar {"
        "    background-color: %1;"
        "    border-bottom: 1px solid %2;"
        "    spacing: 4px;"
        "    padding: 4px;"
        "}"
        "QToolButton {"
        "    background: transparent;"
        "    border: 1px solid transparent;"
        "    border-radius: 3px;"
        "    padding: 4px;"
        "}"
        "QToolButton:hover {"
        "    background-color: %3;"
        "    border-color: %4;"
        "}"
        "QToolButton:pressed {"
        "    background-color: %5;"
        "}"
    ).arg(c.windowBackground.name())
     .arg(c.border.name())
     .arg(c.alternateBackground.name())
     .arg(c.borderHover.name())
     .arg(c.border.name()));
}

void ThemeManager::applyToStatusBar(QWidget* widget) {
    ThemeColors c = colors();
    widget->setStyleSheet(QString(
        "QStatusBar {"
        "    background-color: %1;"
        "    color: %2;"
        "    border-top: 1px solid %3;"
        "}"
        "QStatusBar::item {"
        "    border-right: 1px solid %3;"
        "    padding: 2px 8px;"
        "}"
    ).arg(c.windowBackground.name())
     .arg(c.text.name())
     .arg(c.border.name()));
}

void ThemeManager::applyToButton(QWidget* widget) {
    // Handled by stylesheet
}

void ThemeManager::applyToInput(QWidget* widget) {
    // Handled by stylesheet
}

void ThemeManager::applyToTreeView(QWidget* widget) {
    // Handled by stylesheet
}

void ThemeManager::applyToTabWidget(QWidget* widget) {
    // Handled by stylesheet
}

void ThemeManager::applyToGroupBox(QWidget* widget) {
    // Handled by stylesheet
}

void ThemeManager::setApplicationFont(const QFont& font) {
    fonts_.application = font;
    qApp->setFont(font);
    emit fontsChanged();
}

void ThemeManager::setEditorFont(const QFont& font) {
    fonts_.editor = font;
    emit fontsChanged();
}

void ThemeManager::setDataGridFont(const QFont& font) {
    fonts_.dataGrid = font;
    emit fontsChanged();
}

void ThemeManager::setFontSize(int size) {
    base_font_size_ = size;
    QFont font = qApp->font();
    font.setPointSize(size);
    qApp->setFont(font);
    emit fontsChanged();
}

void ThemeManager::increaseFontSize() {
    setFontSize(base_font_size_ + 1);
}

void ThemeManager::decreaseFontSize() {
    setFontSize(qMax(6, base_font_size_ - 1));
}

void ThemeManager::resetFontSize() {
    setFontSize(10);
}

void ThemeManager::setCustomColor(const QString& name, const QColor& color) {
    custom_color_overrides_[name] = color;
    if (current_theme_ != ThemeType::Custom) {
        custom_colors_ = colors();
        current_theme_ = ThemeType::Custom;
    }
    emit colorsChanged();
}

QColor ThemeManager::getColor(const QString& name) const {
    if (custom_color_overrides_.contains(name)) {
        return custom_color_overrides_[name];
    }
    
    ThemeColors c = colors();
    // Map string names to color fields
    if (name == "windowBackground") return c.windowBackground;
    if (name == "text") return c.text;
    if (name == "primary") return c.primary;
    if (name == "editorBackground") return c.editorBackground;
    // ... etc
    return QColor();
}

void ThemeManager::saveThemeSettings() {
    QSettings settings;
    settings.beginGroup("Theme");
    settings.setValue("themeType", static_cast<int>(current_theme_));
    settings.setValue("fontSize", base_font_size_);
    settings.setValue("appFont", fonts_.application.toString());
    settings.setValue("editorFont", fonts_.editor.toString());
    settings.endGroup();
}

void ThemeManager::loadThemeSettings() {
    QSettings settings;
    settings.beginGroup("Theme");
    current_theme_ = static_cast<ThemeType>(settings.value("themeType", 0).toInt());
    base_font_size_ = settings.value("fontSize", 10).toInt();
    
    QString appFontStr = settings.value("appFont").toString();
    if (!appFontStr.isEmpty()) {
        fonts_.application.fromString(appFontStr);
    }
    
    QString editorFontStr = settings.value("editorFont").toString();
    if (!editorFontStr.isEmpty()) {
        fonts_.editor.fromString(editorFontStr);
    }
    
    settings.endGroup();
}

// ThemePalette implementation
QPalette ThemePalette::createPalette(const ThemeColors& c) {
    QPalette palette;
    
    palette.setColor(QPalette::Window, c.windowBackground);
    palette.setColor(QPalette::WindowText, c.text);
    palette.setColor(QPalette::Base, c.widgetBackground);
    palette.setColor(QPalette::AlternateBase, c.alternateBackground);
    palette.setColor(QPalette::Text, c.text);
    palette.setColor(QPalette::Button, c.widgetBackground);
    palette.setColor(QPalette::ButtonText, c.text);
    palette.setColor(QPalette::Highlight, c.selection);
    palette.setColor(QPalette::HighlightedText, c.selectionText);
    palette.setColor(QPalette::Link, c.link);
    palette.setColor(QPalette::LinkVisited, c.linkVisited);
    
    // Disabled colors
    palette.setColor(QPalette::Disabled, QPalette::WindowText, c.textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, c.textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, c.textDisabled);
    
    return palette;
}

QString ThemePalette::createStyleSheet(const ThemeColors& c) {
    QString ss;
    
    // Global widget styles
    ss += QString(
        "QWidget {"
        "    background-color: %1;"
        "    color: %2;"
        "}"
        "QFrame {"
        "    border-color: %3;"
        "}"
    ).arg(c.windowBackground.name())
     .arg(c.text.name())
     .arg(c.border.name());
    
    // Buttons
    ss += QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 4px;"
        "    padding: 6px 16px;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %4;"
        "    border-color: %5;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %6;"
        "}"
        "QPushButton:default {"
        "    background-color: %7;"
        "    color: white;"
        "    border-color: %8;"
        "}"
    ).arg(c.widgetBackground.name())
     .arg(c.text.name())
     .arg(c.border.name())
     .arg(c.alternateBackground.name())
     .arg(c.borderHover.name())
     .arg(c.border.name())
     .arg(c.primary.name())
     .arg(c.primaryDark.name());
    
    // Input fields
    ss += QString(
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 3px;"
        "    padding: 4px;"
        "    selection-background-color: %4;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {"
        "    border-color: %5;"
        "}"
    ).arg(c.widgetBackground.name())
     .arg(c.text.name())
     .arg(c.border.name())
     .arg(c.selection.name())
     .arg(c.borderFocus.name());
    
    // Tables and Lists
    ss += QString(
        "QTableView, QTreeView, QListView {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    selection-background-color: %4;"
        "    selection-color: %5;"
        "    alternate-background-color: %6;"
        "}"
        "QTableView::item:selected, QTreeView::item:selected {"
        "    background-color: %4;"
        "}"
        "QHeaderView::section {"
        "    background-color: %7;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    padding: 4px;"
        "}"
    ).arg(c.gridBackground.name())
     .arg(c.text.name())
     .arg(c.border.name())
     .arg(c.gridSelection.name())
     .arg(c.selectionText.name())
     .arg(c.gridAlternateRow.name())
     .arg(c.alternateBackground.name());
    
    // Tabs
    ss += QString(
        "QTabWidget::pane {"
        "    border: 1px solid %1;"
        "    background-color: %2;"
        "}"
        "QTabBar::tab {"
        "    background-color: %3;"
        "    color: %4;"
        "    border: 1px solid %1;"
        "    padding: 8px 16px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: %5;"
        "    border-bottom-color: %5;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background-color: %6;"
        "}"
    ).arg(c.border.name())
     .arg(c.widgetBackground.name())
     .arg(c.windowBackground.name())
     .arg(c.text.name())
     .arg(c.widgetBackground.name())
     .arg(c.alternateBackground.name());
    
    // Group Boxes
    ss += QString(
        "QGroupBox {"
        "    background-color: transparent;"
        "    color: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 4px;"
        "    margin-top: 12px;"
        "    padding-top: 12px;"
        "    font-weight: bold;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 8px;"
        "    padding: 0 4px;"
        "}"
    ).arg(c.text.name())
     .arg(c.border.name());
    
    // Combo Boxes
    ss += QString(
        "QComboBox {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 3px;"
        "    padding: 4px;"
        "    min-width: 60px;"
        "}"
        "QComboBox:hover {"
        "    border-color: %4;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    selection-background-color: %5;"
        "}"
    ).arg(c.widgetBackground.name())
     .arg(c.text.name())
     .arg(c.border.name())
     .arg(c.borderHover.name())
     .arg(c.selection.name());
    
    // Scrollbars
    ss += QString(
        "QScrollBar:vertical {"
        "    background-color: %1;"
        "    width: 12px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background-color: %2;"
        "    border-radius: 6px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background-color: %3;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "    background-color: %1;"
        "    height: 12px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background-color: %2;"
        "    border-radius: 6px;"
        "    min-width: 20px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background-color: %3;"
        "}"
    ).arg(c.alternateBackground.name())
     .arg(c.border.name())
     .arg(c.borderHover.name());
    
    return ss;
}

} // namespace scratchrobin::ui
