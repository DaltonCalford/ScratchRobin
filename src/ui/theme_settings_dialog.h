#pragma once
#include <QDialog>
#include <QPushButton>
#include "ui/theme_manager.h"

QT_BEGIN_NAMESPACE
class QTabWidget;
class QComboBox;
class QLabel;
class QSpinBox;
class QSlider;
class QCheckBox;
class QFontComboBox;
class QListWidget;
class QStackedWidget;
class QRadioButton;
class QColorDialog;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::ui {

class ThemeSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ThemeSettingsDialog(QWidget* parent = nullptr);
    ~ThemeSettingsDialog() override;

signals:
    void previewThemeRequested(ThemeType type);
    void settingsApplied();

private slots:
    // Theme selection
    void onThemeChanged(int index);
    
    // Font settings
    void onFontSizeChanged(int size);
    
    // Color settings
    void onResetColors();
    
    // Apply/Save
    void onApply();
    void onOk();
    void onCancel();
    void applySettings();
    
    // Additional font methods
    void onChangeAppFont();
    void onResetAppFont();
    void onChangeEditorFont();
    void onResetEditorFont();
    void onChangeGridFont();
    void onResetGridFont();
    
    // Theme methods
    void onFollowOsChanged(int state);
    void onAccentChanged(int index);
    
    // Color methods  
    void onBackgroundColorClicked();
    void onTextColorClicked();
    void onPrimaryColorClicked();
    void onBorderColorClicked();
    void onEditorBgClicked();
    void onSuccessColorClicked();

private:
    void setupUi();
    void createThemeTab();
    void createFontsTab();
    void createColorsTab();
    void createEditorTab();
    void createPreviewTab();
    
    void loadCurrentSettings();
    void updateColorButtons();
    void applyThemeToPreview();
    void enableColorEditing(bool enable);
    void updateColorButton(QPushButton* btn, const QColor& color);
    
    // Theme tab
    QComboBox* theme_combo_;
    QComboBox* accent_combo_;
    QCheckBox* follow_os_check_;
    QRadioButton* light_radio_;
    QRadioButton* dark_radio_;
    QRadioButton* night_radio_;
    QRadioButton* custom_radio_;
    QLabel* theme_preview_label_;
    
    // Color buttons
    QPushButton* bg_color_btn_;
    QPushButton* text_color_btn_;
    QPushButton* primary_color_btn_;
    QPushButton* border_color_btn_;
    QPushButton* editor_bg_btn_;
    QPushButton* success_color_btn_;
    QPushButton* reset_colors_btn_;
    
    // Font labels
    QLabel* app_font_label_;
    QLabel* editor_font_label_;
    QLabel* grid_font_label_;
    
    // Font buttons
    QPushButton* change_app_font_btn_;
    QPushButton* reset_app_font_btn_;
    QPushButton* change_editor_font_btn_;
    QPushButton* reset_editor_font_btn_;
    QPushButton* change_grid_font_btn_;
    QPushButton* reset_grid_font_btn_;
    
    // Dialog buttons
    QPushButton* apply_btn_;
    QPushButton* ok_btn_;
    QPushButton* cancel_btn_;
    
    // Font size
    QSpinBox* font_size_spin_;
    
    ThemeColors current_colors_;
    ThemeFonts current_fonts_;
    ThemeType selected_theme_;
};

} // namespace scratchrobin::ui
