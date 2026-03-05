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

#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>

#include "core/settings.h"

namespace scratchrobin::ui {

// Settings dialog with tabs for Interface customization
// Features live preview - changes apply immediately to the main application
class SettingsDialog : public wxDialog {
public:
    SettingsDialog(wxWindow* parent);
    ~SettingsDialog();
    
    // Show dialog with live preview and confirmation
    bool ShowWithLivePreview();

private:
    enum class ControlId {
        kNotebook = wxID_HIGHEST + 300,
        kThemeLight,
        kThemeDark,
        kThemeHighContrast,
        kThemeNight,
        kMasterScaleSlider,
        kMasterScaleText,
        kLinkScales,
        kMenuFontSlider,
        kMenuFontText,
        kTreeTextSlider,
        kTreeTextText,
        kTreeIconSlider,
        kTreeIconText,
        kDialogFontSlider,
        kDialogFontText,
        kEditorFontSlider,
        kEditorFontText,
        kStatusBarSlider,
        kStatusBarText,
        kResetDefaults,
        kApply,
        kPreview,
    };
    
    // Page creation
    wxPanel* createAppearancePage(wxWindow* parent);
    wxPanel* createScalingPage(wxWindow* parent);
    
    // Event handlers
    void onThemeChanged(wxCommandEvent& event);
    void onMasterScaleChanged(wxCommandEvent& event);
    void onLinkedScaleChanged(wxCommandEvent& event);
    void onIndividualScaleChanged(wxCommandEvent& event);
    void onLinkScalesChanged(wxCommandEvent& event);
    void onResetDefaults(wxCommandEvent& event);
    void onApply(wxCommandEvent& event);
    void onPreview(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    
    // Helper methods
    void updateMasterScaleDisplay();
    void updateIndividualScaleDisplays();
    void syncLinkedScales(int master_value);
    void applyCurrentSettingsToApp();
    void applyCurrentSettingsToDialog();
    void restoreOriginalSettings();
    void showConfirmationAndAccept();
    
    // Live preview - apply to main app immediately
    void doLivePreview();
    
    // Controls
    wxNotebook* notebook_{nullptr};
    
    // Appearance page
    wxRadioButton* theme_light_{nullptr};
    wxRadioButton* theme_dark_{nullptr};
    wxRadioButton* theme_high_contrast_{nullptr};
    wxRadioButton* theme_night_{nullptr};
    
    // Scaling page
    wxSlider* master_scale_slider_{nullptr};
    wxStaticText* master_scale_text_{nullptr};
    wxCheckBox* link_scales_check_{nullptr};
    
    wxSlider* menu_font_slider_{nullptr};
    wxStaticText* menu_font_text_{nullptr};
    
    wxSlider* tree_text_slider_{nullptr};
    wxStaticText* tree_text_text_{nullptr};
    
    wxSlider* tree_icon_slider_{nullptr};
    wxStaticText* tree_icon_text_{nullptr};
    
    wxSlider* dialog_font_slider_{nullptr};
    wxStaticText* dialog_font_text_{nullptr};
    
    wxSlider* editor_font_slider_{nullptr};
    wxStaticText* editor_font_text_{nullptr};
    
    wxSlider* status_bar_slider_{nullptr};
    wxStaticText* status_bar_text_{nullptr};
    
    // Current working settings (for live preview)
    core::Theme current_theme_{core::Theme::kLight};
    core::ScaleSettings current_scales_;
    bool scales_linked_{true};
    
    // Original settings (to restore if cancelled)
    core::Theme original_theme_;
    core::ScaleSettings original_scales_;
    bool original_scales_linked_;
    
    // Track if changes have been applied
    bool changes_applied_{false};
    
    DECLARE_EVENT_TABLE()
};

}  // namespace scratchrobin::ui
