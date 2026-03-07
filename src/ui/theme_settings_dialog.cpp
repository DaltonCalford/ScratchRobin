#include "ui/theme_settings_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QFontDialog>
#include <QColorDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QScrollArea>
#include <QFontDatabase>

namespace scratchrobin::ui {

ThemeSettingsDialog::ThemeSettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Theme Settings"));
    setMinimumSize(700, 600);
    resize(800, 650);
    
    setupUi();
    loadCurrentSettings();
}

ThemeSettingsDialog::~ThemeSettingsDialog() = default;

void ThemeSettingsDialog::setupUi() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(12);
    
    auto* tab_widget = new QTabWidget(this);
    
    // Theme tab
    auto* theme_tab = new QWidget(this);
    auto* theme_layout = new QVBoxLayout(theme_tab);
    theme_layout->setSpacing(12);
    
    // Theme selection
    auto* theme_group = new QGroupBox(tr("Theme"), this);
    auto* theme_select_layout = new QGridLayout(theme_group);
    
    theme_select_layout->addWidget(new QLabel(tr("Color Theme:")), 0, 0);
    theme_combo_ = new QComboBox(this);
    theme_combo_->addItem(tr("Light"), static_cast<int>(ThemeType::Light));
    theme_combo_->addItem(tr("Dark"), static_cast<int>(ThemeType::Dark));
    theme_combo_->addItem(tr("Night (Low Blue Light)"), static_cast<int>(ThemeType::Night));
    theme_combo_->addItem(tr("Custom"), static_cast<int>(ThemeType::Custom));
    theme_select_layout->addWidget(theme_combo_, 0, 1);
    
    theme_select_layout->addWidget(new QLabel(tr("Accent Color:")), 1, 0);
    accent_combo_ = new QComboBox(this);
    accent_combo_->addItem(tr("Blue"), "blue");
    accent_combo_->addItem(tr("Green"), "green");
    accent_combo_->addItem(tr("Orange"), "orange");
    accent_combo_->addItem(tr("Purple"), "purple");
    accent_combo_->addItem(tr("Red"), "red");
    accent_combo_->addItem(tr("Teal"), "teal");
    theme_select_layout->addWidget(accent_combo_, 1, 1);
    
    follow_os_check_ = new QCheckBox(tr("Follow operating system theme"), this);
    theme_select_layout->addWidget(follow_os_check_, 2, 0, 1, 2);
    
    theme_layout->addWidget(theme_group);
    
    // Colors customization
    auto* colors_group = new QGroupBox(tr("Custom Colors"), this);
    auto* colors_layout = new QGridLayout(colors_group);
    
    colors_layout->addWidget(new QLabel(tr("Background:"), this), 0, 0);
    bg_color_btn_ = new QPushButton(this);
    bg_color_btn_->setMinimumWidth(100);
    colors_layout->addWidget(bg_color_btn_, 0, 1);
    
    colors_layout->addWidget(new QLabel(tr("Text:"), this), 0, 2);
    text_color_btn_ = new QPushButton(this);
    text_color_btn_->setMinimumWidth(100);
    colors_layout->addWidget(text_color_btn_, 0, 3);
    
    colors_layout->addWidget(new QLabel(tr("Primary:"), this), 1, 0);
    primary_color_btn_ = new QPushButton(this);
    primary_color_btn_->setMinimumWidth(100);
    colors_layout->addWidget(primary_color_btn_, 1, 1);
    
    colors_layout->addWidget(new QLabel(tr("Border:"), this), 1, 2);
    border_color_btn_ = new QPushButton(this);
    border_color_btn_->setMinimumWidth(100);
    colors_layout->addWidget(border_color_btn_, 1, 3);
    
    colors_layout->addWidget(new QLabel(tr("Editor BG:"), this), 2, 0);
    editor_bg_btn_ = new QPushButton(this);
    editor_bg_btn_->setMinimumWidth(100);
    colors_layout->addWidget(editor_bg_btn_, 2, 1);
    
    colors_layout->addWidget(new QLabel(tr("Success:"), this), 2, 2);
    success_color_btn_ = new QPushButton(this);
    success_color_btn_->setMinimumWidth(100);
    colors_layout->addWidget(success_color_btn_, 2, 3);
    
    reset_colors_btn_ = new QPushButton(tr("Reset to Theme Defaults"), this);
    colors_layout->addWidget(reset_colors_btn_, 3, 0, 1, 4);
    
    theme_layout->addWidget(colors_group);
    theme_layout->addStretch();
    
    tab_widget->addTab(theme_tab, tr("Theme"));
    
    // Fonts tab
    auto* font_tab = new QWidget(this);
    auto* font_layout = new QVBoxLayout(font_tab);
    font_layout->setSpacing(12);
    
    // Application font
    auto* app_font_group = new QGroupBox(tr("Application Font"), this);
    auto* app_font_layout = new QHBoxLayout(app_font_group);
    
    app_font_label_ = new QLabel(this);
    app_font_layout->addWidget(app_font_label_, 1);
    
    change_app_font_btn_ = new QPushButton(tr("Change..."), this);
    app_font_layout->addWidget(change_app_font_btn_);
    
    reset_app_font_btn_ = new QPushButton(tr("Reset"), this);
    app_font_layout->addWidget(reset_app_font_btn_);
    
    font_layout->addWidget(app_font_group);
    
    // Editor font
    auto* editor_font_group = new QGroupBox(tr("Editor Font"), this);
    auto* editor_font_layout = new QHBoxLayout(editor_font_group);
    
    editor_font_label_ = new QLabel(this);
    editor_font_layout->addWidget(editor_font_label_, 1);
    
    change_editor_font_btn_ = new QPushButton(tr("Change..."), this);
    editor_font_layout->addWidget(change_editor_font_btn_);
    
    reset_editor_font_btn_ = new QPushButton(tr("Reset"), this);
    editor_font_layout->addWidget(reset_editor_font_btn_);
    
    font_layout->addWidget(editor_font_group);
    
    // Data grid font
    auto* grid_font_group = new QGroupBox(tr("Data Grid Font"), this);
    auto* grid_font_layout = new QHBoxLayout(grid_font_group);
    
    grid_font_label_ = new QLabel(this);
    grid_font_layout->addWidget(grid_font_label_, 1);
    
    change_grid_font_btn_ = new QPushButton(tr("Change..."), this);
    grid_font_layout->addWidget(change_grid_font_btn_);
    
    reset_grid_font_btn_ = new QPushButton(tr("Reset"), this);
    grid_font_layout->addWidget(reset_grid_font_btn_);
    
    font_layout->addWidget(grid_font_group);
    
    // Font size scaling
    auto* font_size_group = new QGroupBox(tr("Font Size Scaling"), this);
    auto* font_size_layout = new QHBoxLayout(font_size_group);
    
    font_size_layout->addWidget(new QLabel(tr("Base Font Size:"), this));
    font_size_spin_ = new QSpinBox(this);
    font_size_spin_->setRange(6, 20);
    font_size_layout->addWidget(font_size_spin_);
    font_size_layout->addStretch();
    
    font_layout->addWidget(font_size_group);
    font_layout->addStretch();
    
    tab_widget->addTab(font_tab, tr("Fonts"));
    
    main_layout->addWidget(tab_widget);
    
    // Dialog buttons
    auto* btn_layout = new QHBoxLayout();
    btn_layout->addStretch();
    
    apply_btn_ = new QPushButton(tr("Apply"), this);
    btn_layout->addWidget(apply_btn_);
    
    ok_btn_ = new QPushButton(tr("OK"), this);
    ok_btn_->setDefault(true);
    btn_layout->addWidget(ok_btn_);
    
    cancel_btn_ = new QPushButton(tr("Cancel"), this);
    btn_layout->addWidget(cancel_btn_);
    
    main_layout->addLayout(btn_layout);
    
    // Connections
    connect(theme_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ThemeSettingsDialog::onThemeChanged);
    connect(accent_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ThemeSettingsDialog::onAccentChanged);
    connect(follow_os_check_, &QCheckBox::stateChanged,
            this, &ThemeSettingsDialog::onFollowOsChanged);
    connect(bg_color_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onBackgroundColorClicked);
    connect(text_color_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onTextColorClicked);
    connect(primary_color_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onPrimaryColorClicked);
    connect(border_color_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onBorderColorClicked);
    connect(editor_bg_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onEditorBgClicked);
    connect(success_color_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onSuccessColorClicked);
    connect(reset_colors_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onResetColors);
    connect(change_app_font_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onChangeAppFont);
    connect(reset_app_font_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onResetAppFont);
    connect(change_editor_font_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onChangeEditorFont);
    connect(reset_editor_font_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onResetEditorFont);
    connect(change_grid_font_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onChangeGridFont);
    connect(reset_grid_font_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onResetGridFont);
    connect(font_size_spin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ThemeSettingsDialog::onFontSizeChanged);
    connect(apply_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onApply);
    connect(ok_btn_, &QPushButton::clicked,
            this, &ThemeSettingsDialog::onOk);
    connect(cancel_btn_, &QPushButton::clicked,
            this, &QDialog::reject);
}

void ThemeSettingsDialog::loadCurrentSettings() {
    ThemeManager& tm = ThemeManager::instance();
    
    ThemeType currentType = tm.currentThemeType();
    int index = 0;
    switch (currentType) {
        case ThemeType::Dark: index = 1; break;
        case ThemeType::Night: index = 2; break;
        case ThemeType::Custom: index = 3; break;
        default: index = 0; break;
    }
    theme_combo_->setCurrentIndex(index);
    
    ThemeColors c = tm.colors();
    updateColorButton(bg_color_btn_, c.windowBackground);
    updateColorButton(text_color_btn_, c.text);
    updateColorButton(primary_color_btn_, c.primary);
    updateColorButton(border_color_btn_, c.border);
    updateColorButton(editor_bg_btn_, c.editorBackground);
    updateColorButton(success_color_btn_, c.success);
    
    app_font_label_->setText(QString("%1, %2pt")
        .arg(tm.applicationFont().family())
        .arg(tm.applicationFont().pointSize()));
    editor_font_label_->setText(QString("%1, %2pt")
        .arg(tm.editorFont().family())
        .arg(tm.editorFont().pointSize()));
    grid_font_label_->setText(QString("%1, %2pt")
        .arg(tm.dataGridFont().family())
        .arg(tm.dataGridFont().pointSize()));
    
    font_size_spin_->setValue(tm.baseFontSize());
    
    enableColorEditing(currentType == ThemeType::Custom);
}

void ThemeSettingsDialog::enableColorEditing(bool enable) {
    bg_color_btn_->setEnabled(enable);
    text_color_btn_->setEnabled(enable);
    primary_color_btn_->setEnabled(enable);
    border_color_btn_->setEnabled(enable);
    editor_bg_btn_->setEnabled(enable);
    success_color_btn_->setEnabled(enable);
    reset_colors_btn_->setEnabled(enable);
}

void ThemeSettingsDialog::updateColorButton(QPushButton* btn, const QColor& color) {
    QPixmap pixmap(24, 24);
    pixmap.fill(color);
    btn->setIcon(QIcon(pixmap));
    btn->setText(color.name());
}

void ThemeSettingsDialog::onThemeChanged(int index) {
    ThemeType type = static_cast<ThemeType>(theme_combo_->itemData(index).toInt());
    enableColorEditing(type == ThemeType::Custom);
    
    ThemeColors c;
    switch (type) {
        case ThemeType::Dark: c = ThemeManager::darkTheme(); break;
        case ThemeType::Night: c = ThemeManager::nightTheme(); break;
        case ThemeType::Custom: return; // Keep current custom colors
        default: c = ThemeManager::lightTheme(); break;
    }
    
    updateColorButton(bg_color_btn_, c.windowBackground);
    updateColorButton(text_color_btn_, c.text);
    updateColorButton(primary_color_btn_, c.primary);
    updateColorButton(border_color_btn_, c.border);
    updateColorButton(editor_bg_btn_, c.editorBackground);
    updateColorButton(success_color_btn_, c.success);
    
    emit previewThemeRequested(type);
}

void ThemeSettingsDialog::onAccentChanged(int /*index*/) {
    // Apply accent color modification
}

void ThemeSettingsDialog::onFollowOsChanged(int /*state*/) {
    // Toggle OS theme following
}

void ThemeSettingsDialog::onBackgroundColorClicked() {
    QColor color = QColorDialog::getColor(ThemeManager::instance().colors().windowBackground,
                                          this, tr("Select Background Color"));
    if (color.isValid()) {
        updateColorButton(bg_color_btn_, color);
        ThemeManager::instance().setCustomColor("windowBackground", color);
    }
}

void ThemeSettingsDialog::onTextColorClicked() {
    QColor color = QColorDialog::getColor(ThemeManager::instance().colors().text,
                                          this, tr("Select Text Color"));
    if (color.isValid()) {
        updateColorButton(text_color_btn_, color);
        ThemeManager::instance().setCustomColor("text", color);
    }
}

void ThemeSettingsDialog::onPrimaryColorClicked() {
    QColor color = QColorDialog::getColor(ThemeManager::instance().colors().primary,
                                          this, tr("Select Primary Color"));
    if (color.isValid()) {
        updateColorButton(primary_color_btn_, color);
        ThemeManager::instance().setCustomColor("primary", color);
    }
}

void ThemeSettingsDialog::onBorderColorClicked() {
    QColor color = QColorDialog::getColor(ThemeManager::instance().colors().border,
                                          this, tr("Select Border Color"));
    if (color.isValid()) {
        updateColorButton(border_color_btn_, color);
        ThemeManager::instance().setCustomColor("border", color);
    }
}

void ThemeSettingsDialog::onEditorBgClicked() {
    QColor color = QColorDialog::getColor(ThemeManager::instance().colors().editorBackground,
                                          this, tr("Select Editor Background"));
    if (color.isValid()) {
        updateColorButton(editor_bg_btn_, color);
        ThemeManager::instance().setCustomColor("editorBackground", color);
    }
}

void ThemeSettingsDialog::onSuccessColorClicked() {
    QColor color = QColorDialog::getColor(ThemeManager::instance().colors().success,
                                          this, tr("Select Success Color"));
    if (color.isValid()) {
        updateColorButton(success_color_btn_, color);
        ThemeManager::instance().setCustomColor("success", color);
    }
}

void ThemeSettingsDialog::onResetColors() {
    int index = theme_combo_->currentIndex();
    onThemeChanged(index);
}

void ThemeSettingsDialog::onChangeAppFont() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, ThemeManager::instance().applicationFont(), this);
    if (ok) {
        app_font_label_->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
        ThemeManager::instance().setApplicationFont(font);
    }
}

void ThemeSettingsDialog::onResetAppFont() {
    QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    app_font_label_->setText(QString("%1, %2pt").arg(defaultFont.family()).arg(defaultFont.pointSize()));
    ThemeManager::instance().setApplicationFont(defaultFont);
}

void ThemeSettingsDialog::onChangeEditorFont() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, ThemeManager::instance().editorFont(), this,
                                      tr("Select Editor Font"),
                                      QFontDialog::MonospacedFonts);
    if (ok) {
        editor_font_label_->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
        ThemeManager::instance().setEditorFont(font);
    }
}

void ThemeSettingsDialog::onResetEditorFont() {
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    editor_font_label_->setText(QString("%1, %2pt").arg(mono.family()).arg(mono.pointSize()));
    ThemeManager::instance().setEditorFont(mono);
}

void ThemeSettingsDialog::onChangeGridFont() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, ThemeManager::instance().dataGridFont(), this);
    if (ok) {
        grid_font_label_->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
        ThemeManager::instance().setDataGridFont(font);
    }
}

void ThemeSettingsDialog::onResetGridFont() {
    QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    grid_font_label_->setText(QString("%1, %2pt").arg(defaultFont.family()).arg(defaultFont.pointSize()));
    ThemeManager::instance().setDataGridFont(defaultFont);
}

void ThemeSettingsDialog::onFontSizeChanged(int size) {
    ThemeManager::instance().setFontSize(size);
}

void ThemeSettingsDialog::onApply() {
    applySettings();
}

void ThemeSettingsDialog::onOk() {
    applySettings();
    accept();
}

void ThemeSettingsDialog::onCancel() {
    reject();
}

void ThemeSettingsDialog::applySettings() {
    ThemeManager& tm = ThemeManager::instance();
    
    int typeIndex = theme_combo_->currentIndex();
    ThemeType type = static_cast<ThemeType>(theme_combo_->itemData(typeIndex).toInt());
    tm.setTheme(type);
    
    // Save settings
    tm.saveThemeSettings();
    
    emit settingsApplied();
}

} // namespace scratchrobin::ui
