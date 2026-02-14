/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DATA_MASKING_FRAME_H
#define SCRATCHROBIN_DATA_MASKING_FRAME_H

#include <memory>
#include <string>
#include <vector>

#include <wx/frame.h>
#include <wx/timer.h>

class wxNotebook;
class wxListCtrl;
class wxListEvent;
class wxTextCtrl;
class wxChoice;
class wxCheckBox;
class wxSpinCtrl;
class wxButton;
class wxStaticText;
class wxComboBox;
class wxPanel;
class wxGauge;

namespace scratchrobin {

class WindowManager;
class MaskingProfile;
class MaskingRule;

/**
 * @brief Data Masking Configuration Frame
 * 
 * Provides UI for:
 * - Creating/managing masking profiles
 * - Defining masking rules per column
 * - Auto-discovering sensitive data
 * - Previewing masking results
 * - Executing masking jobs
 */
class DataMaskingFrame : public wxFrame {
public:
    DataMaskingFrame(WindowManager* windowManager,
                     wxWindow* parent = nullptr);
    
    ~DataMaskingFrame();

private:
    void BuildMenu();
    void BuildLayout();
    
    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnNewProfile(wxCommandEvent& event);
    void OnSaveProfile(wxCommandEvent& event);
    void OnDeleteProfile(wxCommandEvent& event);
    void OnProfileSelected(wxCommandEvent& event);
    void OnNewRule(wxCommandEvent& event);
    void OnEditRule(wxCommandEvent& event);
    void OnDeleteRule(wxCommandEvent& event);
    void OnRuleSelected(wxListEvent& event);
    void OnAutoDiscover(wxCommandEvent& event);
    void OnPreviewMasking(wxCommandEvent& event);
    void OnExecuteMasking(wxCommandEvent& event);
    void OnMethodChanged(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    
    // Helper methods
    void LoadProfileList();
    void LoadRuleList();
    void LoadRuleDetails(MaskingRule* rule);
    void UpdatePreview();
    bool ValidateRule();
    std::unique_ptr<MaskingRule> GatherRuleFromUI();
    void UpdateMethodOptions();
    
    WindowManager* window_manager_ = nullptr;
    
    // Current state
    std::string current_profile_id_;
    std::string current_rule_id_;
    
    // Auto-refresh timer
    wxTimer refresh_timer_;
    
    // UI Elements - Profiles
    wxChoice* choice_profiles_ = nullptr;
    wxButton* btn_new_profile_ = nullptr;
    wxButton* btn_save_profile_ = nullptr;
    wxButton* btn_delete_profile_ = nullptr;
    wxTextCtrl* txt_profile_name_ = nullptr;
    wxTextCtrl* txt_profile_desc_ = nullptr;
    wxChoice* choice_target_env_ = nullptr;
    wxCheckBox* chk_auto_pii_ = nullptr;
    wxCheckBox* chk_auto_pci_ = nullptr;
    wxCheckBox* chk_auto_phi_ = nullptr;
    
    // UI Elements - Rules
    wxListCtrl* list_rules_ = nullptr;
    wxButton* btn_new_rule_ = nullptr;
    wxButton* btn_edit_rule_ = nullptr;
    wxButton* btn_delete_rule_ = nullptr;
    wxButton* btn_auto_discover_ = nullptr;
    
    // UI Elements - Rule Details
    wxTextCtrl* txt_rule_name_ = nullptr;
    wxTextCtrl* txt_schema_ = nullptr;
    wxTextCtrl* txt_table_ = nullptr;
    wxTextCtrl* txt_column_ = nullptr;
    wxChoice* choice_classification_ = nullptr;
    wxChoice* choice_method_ = nullptr;
    
    // Method-specific options
    wxPanel* panel_method_options_ = nullptr;
    
    // Partial masking options
    wxSpinCtrl* spin_visible_start_ = nullptr;
    wxSpinCtrl* spin_visible_end_ = nullptr;
    wxTextCtrl* txt_mask_char_ = nullptr;
    
    // Hash options
    wxChoice* choice_hash_algo_ = nullptr;
    wxTextCtrl* txt_hash_salt_ = nullptr;
    
    // Substitution options
    wxChoice* choice_fake_generator_ = nullptr;
    wxSpinCtrl* spin_random_seed_ = nullptr;
    
    // Regex options
    wxTextCtrl* txt_regex_pattern_ = nullptr;
    wxTextCtrl* txt_regex_replace_ = nullptr;
    
    // Encryption options
    wxTextCtrl* txt_encryption_key_ = nullptr;
    
    // Truncation options
    wxSpinCtrl* spin_max_length_ = nullptr;
    
    // Redaction options
    wxTextCtrl* txt_redaction_string_ = nullptr;
    
    // Environment options
    wxCheckBox* chk_apply_dev_ = nullptr;
    wxCheckBox* chk_apply_test_ = nullptr;
    wxCheckBox* chk_apply_staging_ = nullptr;
    wxCheckBox* chk_apply_prod_ = nullptr;
    wxCheckBox* chk_rule_enabled_ = nullptr;
    
    // UI Elements - Preview
    wxTextCtrl* txt_sample_input_ = nullptr;
    wxTextCtrl* txt_sample_output_ = nullptr;
    wxButton* btn_preview_ = nullptr;
    
    // UI Elements - Execution
    wxTextCtrl* txt_source_conn_ = nullptr;
    wxTextCtrl* txt_target_conn_ = nullptr;
    wxTextCtrl* txt_schemas_ = nullptr;
    wxCheckBox* chk_dry_run_ = nullptr;
    wxCheckBox* chk_truncate_target_ = nullptr;
    wxSpinCtrl* spin_batch_size_ = nullptr;
    wxSpinCtrl* spin_workers_ = nullptr;
    wxButton* btn_execute_ = nullptr;
    wxButton* btn_cancel_ = nullptr;
    
    // Status
    wxStaticText* lbl_job_status_ = nullptr;
    wxStaticText* lbl_progress_ = nullptr;
    wxGauge* gauge_progress_ = nullptr;
    wxListCtrl* list_job_log_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATA_MASKING_FRAME_H
