/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AI_SETTINGS_DIALOG_H
#define SCRATCHROBIN_AI_SETTINGS_DIALOG_H

#include <wx/wx.h>
#include <wx/spinctrl.h>

namespace scratchrobin {

// AI Provider Settings Dialog
// Allows users to configure API keys and settings for different AI providers
class AiSettingsDialog : public wxDialog {
public:
    AiSettingsDialog(wxWindow* parent);
    
    // Load/save settings
    void LoadSettings();
    void SaveSettings();
    
private:
    void CreateControls();
    void BindEvents();
    void UpdateProviderUI();
    void TestConnection();
    
    void OnProviderChanged(wxCommandEvent& event);
    void OnTestConnection(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);
    
    // UI Controls
    wxChoice* provider_choice_;
    wxTextCtrl* api_key_ctrl_;
    wxTextCtrl* api_endpoint_ctrl_;
    wxTextCtrl* model_name_ctrl_;
    wxSlider* temperature_slider_;
    wxStaticText* temperature_label_;
    wxSpinCtrl* max_tokens_ctrl_;
    wxSpinCtrl* timeout_ctrl_;
    wxCheckBox* enable_schema_design_;
    wxCheckBox* enable_query_optimization_;
    wxCheckBox* enable_code_generation_;
    wxCheckBox* enable_documentation_;
    
    // Status
    wxStaticText* status_label_;
    wxButton* test_button_;
    
    enum {
        ID_PROVIDER_CHOICE = wxID_HIGHEST + 1,
        ID_TEST_CONNECTION,
        ID_API_KEY,
        ID_TEMPERATURE_SLIDER
    };
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AI_SETTINGS_DIALOG_H
