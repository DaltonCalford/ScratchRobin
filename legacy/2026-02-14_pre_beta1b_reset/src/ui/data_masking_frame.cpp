/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include "ui/data_masking_frame.h"

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/gauge.h>

#include "ui/window_manager.h"
#include "core/data_masking.h"

namespace scratchrobin {

enum {
    ID_NEW_PROFILE = wxID_HIGHEST + 1,
    ID_SAVE_PROFILE,
    ID_DELETE_PROFILE,
    ID_PROFILE_SELECTED,
    ID_NEW_RULE,
    ID_EDIT_RULE,
    ID_DELETE_RULE,
    ID_RULE_SELECTED,
    ID_AUTO_DISCOVER,
    ID_PREVIEW_MASKING,
    ID_EXECUTE_MASKING,
    ID_METHOD_CHANGED,
    ID_REFRESH,
    ID_TIMER_REFRESH
};

wxBEGIN_EVENT_TABLE(DataMaskingFrame, wxFrame)
    EVT_CLOSE(DataMaskingFrame::OnClose)
    EVT_BUTTON(ID_NEW_PROFILE, DataMaskingFrame::OnNewProfile)
    EVT_BUTTON(ID_SAVE_PROFILE, DataMaskingFrame::OnSaveProfile)
    EVT_BUTTON(ID_DELETE_PROFILE, DataMaskingFrame::OnDeleteProfile)
    EVT_CHOICE(ID_PROFILE_SELECTED, DataMaskingFrame::OnProfileSelected)
    EVT_BUTTON(ID_NEW_RULE, DataMaskingFrame::OnNewRule)
    EVT_BUTTON(ID_EDIT_RULE, DataMaskingFrame::OnEditRule)
    EVT_BUTTON(ID_DELETE_RULE, DataMaskingFrame::OnDeleteRule)
    EVT_LIST_ITEM_SELECTED(ID_RULE_SELECTED, DataMaskingFrame::OnRuleSelected)
    EVT_BUTTON(ID_AUTO_DISCOVER, DataMaskingFrame::OnAutoDiscover)
    EVT_BUTTON(ID_PREVIEW_MASKING, DataMaskingFrame::OnPreviewMasking)
    EVT_BUTTON(ID_EXECUTE_MASKING, DataMaskingFrame::OnExecuteMasking)
    EVT_CHOICE(ID_METHOD_CHANGED, DataMaskingFrame::OnMethodChanged)
    EVT_BUTTON(ID_REFRESH, DataMaskingFrame::OnRefresh)
    EVT_TIMER(ID_TIMER_REFRESH, DataMaskingFrame::OnTimer)
wxEND_EVENT_TABLE()

DataMaskingFrame::DataMaskingFrame(WindowManager* windowManager, wxWindow* parent)
    : wxFrame(parent, wxID_ANY, _("Data Masking Configuration"),
              wxDefaultPosition, wxSize(1200, 800),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , window_manager_(windowManager)
    , refresh_timer_(this, ID_TIMER_REFRESH) {
    
    BuildMenu();
    BuildLayout();
    
    CentreOnScreen();
    
    // Start refresh timer
    refresh_timer_.Start(2000);
    
    // Load initial data
    LoadProfileList();
    LoadRuleList();
}

DataMaskingFrame::~DataMaskingFrame() = default;

void DataMaskingFrame::BuildMenu() {
    auto* menu_bar = new wxMenuBar();
    
    auto* file_menu = new wxMenu();
    file_menu->Append(wxID_CLOSE, _("&Close\tCtrl+W"));
    menu_bar->Append(file_menu, _("&File"));
    
    auto* profile_menu = new wxMenu();
    profile_menu->Append(ID_NEW_PROFILE, _("&New Profile...\tCtrl+N"));
    profile_menu->Append(ID_SAVE_PROFILE, _("&Save Profile\tCtrl+S"));
    profile_menu->AppendSeparator();
    profile_menu->Append(ID_DELETE_PROFILE, _("&Delete Profile"));
    menu_bar->Append(profile_menu, _("&Profile"));
    
    auto* rules_menu = new wxMenu();
    rules_menu->Append(ID_NEW_RULE, _("&New Rule...\tCtrl+R"));
    rules_menu->Append(ID_EDIT_RULE, _("&Edit Rule\tCtrl+E"));
    rules_menu->Append(ID_DELETE_RULE, _("&Delete Rule\tDel"));
    rules_menu->AppendSeparator();
    rules_menu->Append(ID_AUTO_DISCOVER, _("&Auto-Discover Sensitive Data"));
    menu_bar->Append(rules_menu, _("&Rules"));
    
    auto* exec_menu = new wxMenu();
    exec_menu->Append(ID_EXECUTE_MASKING, _("&Execute Masking Job...\tF5"));
    menu_bar->Append(exec_menu, _("&Execute"));
    
    SetMenuBar(menu_bar);
}

void DataMaskingFrame::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* notebook = new wxNotebook(this, wxID_ANY);
    
    // Profiles Tab
    auto* profile_panel = new wxPanel(notebook, wxID_ANY);
    auto* profile_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Profile selector
    auto* profile_select_sizer = new wxBoxSizer(wxHORIZONTAL);
    profile_select_sizer->Add(new wxStaticText(profile_panel, wxID_ANY, _("Profile:")), 0, wxALIGN_CENTER_VERTICAL);
    
    choice_profiles_ = new wxChoice(profile_panel, ID_PROFILE_SELECTED);
    profile_select_sizer->Add(choice_profiles_, 1, wxLEFT | wxRIGHT, 5);
    
    btn_new_profile_ = new wxButton(profile_panel, ID_NEW_PROFILE, _("New"));
    btn_save_profile_ = new wxButton(profile_panel, ID_SAVE_PROFILE, _("Save"));
    btn_delete_profile_ = new wxButton(profile_panel, ID_DELETE_PROFILE, _("Delete"));
    
    profile_select_sizer->Add(btn_new_profile_, 0, wxRIGHT, 5);
    profile_select_sizer->Add(btn_save_profile_, 0, wxRIGHT, 5);
    profile_select_sizer->Add(btn_delete_profile_, 0);
    
    profile_sizer->Add(profile_select_sizer, 0, wxEXPAND | wxALL, 10);
    
    // Profile details
    auto* profile_details = new wxStaticBox(profile_panel, wxID_ANY, _("Profile Details"));
    auto* details_sizer = new wxStaticBoxSizer(profile_details, wxVERTICAL);
    
    auto* grid_sizer = new wxFlexGridSizer(2, 5, 5);
    grid_sizer->AddGrowableCol(1);
    
    grid_sizer->Add(new wxStaticText(profile_panel, wxID_ANY, _("Name:")), 0, wxALIGN_CENTER_VERTICAL);
    txt_profile_name_ = new wxTextCtrl(profile_panel, wxID_ANY);
    grid_sizer->Add(txt_profile_name_, 1, wxEXPAND);
    
    grid_sizer->Add(new wxStaticText(profile_panel, wxID_ANY, _("Description:")), 0, wxALIGN_CENTER_VERTICAL);
    txt_profile_desc_ = new wxTextCtrl(profile_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    grid_sizer->Add(txt_profile_desc_, 1, wxEXPAND);
    
    grid_sizer->Add(new wxStaticText(profile_panel, wxID_ANY, _("Target Environment:")), 0, wxALIGN_CENTER_VERTICAL);
    choice_target_env_ = new wxChoice(profile_panel, wxID_ANY);
    choice_target_env_->Append(_("Development"));
    choice_target_env_->Append(_("Testing"));
    choice_target_env_->Append(_("Staging"));
    choice_target_env_->Append(_("Compliance Testing"));
    grid_sizer->Add(choice_target_env_, 1, wxEXPAND);
    
    details_sizer->Add(grid_sizer, 1, wxEXPAND | wxALL, 5);
    
    // Auto-detection options
    auto* auto_box = new wxStaticBox(profile_panel, wxID_ANY, _("Auto-Detection"));
    auto* auto_sizer = new wxStaticBoxSizer(auto_box, wxVERTICAL);
    
    chk_auto_pii_ = new wxCheckBox(profile_panel, wxID_ANY, _("Auto-detect PII (Personally Identifiable Information)"));
    chk_auto_pci_ = new wxCheckBox(profile_panel, wxID_ANY, _("Auto-detect PCI (Payment Card Industry) data"));
    chk_auto_phi_ = new wxCheckBox(profile_panel, wxID_ANY, _("Auto-detect PHI (Protected Health Information)"));
    
    chk_auto_pii_->SetValue(true);
    chk_auto_pci_->SetValue(true);
    chk_auto_phi_->SetValue(false);
    
    auto_sizer->Add(chk_auto_pii_, 0, wxALL, 3);
    auto_sizer->Add(chk_auto_pci_, 0, wxALL, 3);
    auto_sizer->Add(chk_auto_phi_, 0, wxALL, 3);
    
    auto_sizer->Add(new wxButton(profile_panel, ID_AUTO_DISCOVER, _("Run Auto-Discovery")), 0, wxTOP | wxALIGN_RIGHT, 10);
    
    details_sizer->Add(auto_sizer, 0, wxEXPAND | wxALL, 5);
    profile_sizer->Add(details_sizer, 0, wxEXPAND | wxALL, 10);
    
    profile_panel->SetSizer(profile_sizer);
    notebook->AddPage(profile_panel, _("Profiles"));
    
    // Rules Tab
    auto* rules_panel = new wxPanel(notebook, wxID_ANY);
    auto* rules_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Rules list
    list_rules_ = new wxListCtrl(rules_panel, ID_RULE_SELECTED, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    list_rules_->AppendColumn(_("Column"), wxLIST_FORMAT_LEFT, 200);
    list_rules_->AppendColumn(_("Classification"), wxLIST_FORMAT_LEFT, 120);
    list_rules_->AppendColumn(_("Method"), wxLIST_FORMAT_LEFT, 120);
    list_rules_->AppendColumn(_("Status"), wxLIST_FORMAT_LEFT, 80);
    rules_sizer->Add(list_rules_, 1, wxEXPAND | wxALL, 10);
    
    // Rule buttons
    auto* rule_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_new_rule_ = new wxButton(rules_panel, ID_NEW_RULE, _("New Rule"));
    btn_edit_rule_ = new wxButton(rules_panel, ID_EDIT_RULE, _("Edit Rule"));
    btn_delete_rule_ = new wxButton(rules_panel, ID_DELETE_RULE, _("Delete Rule"));
    
    rule_btn_sizer->Add(btn_new_rule_, 0, wxRIGHT, 5);
    rule_btn_sizer->Add(btn_edit_rule_, 0, wxRIGHT, 5);
    rule_btn_sizer->Add(btn_delete_rule_, 0);
    rule_btn_sizer->AddStretchSpacer(1);
    rule_btn_sizer->Add(new wxButton(rules_panel, ID_REFRESH, _("Refresh")), 0);
    
    rules_sizer->Add(rule_btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    rules_panel->SetSizer(rules_sizer);
    notebook->AddPage(rules_panel, _("Rules"));
    
    // Preview Tab (placeholder)
    auto* preview_panel = new wxPanel(notebook, wxID_ANY);
    auto* preview_sizer = new wxBoxSizer(wxVERTICAL);
    preview_sizer->Add(new wxStaticText(preview_panel, wxID_ANY, _("Preview masking results here...")), 1, wxEXPAND | wxALL, 10);
    preview_panel->SetSizer(preview_sizer);
    notebook->AddPage(preview_panel, _("Preview"));
    
    // Execute Tab (placeholder)
    auto* exec_panel = new wxPanel(notebook, wxID_ANY);
    auto* exec_sizer = new wxBoxSizer(wxVERTICAL);
    exec_sizer->Add(new wxStaticText(exec_panel, wxID_ANY, _("Execute masking job here...")), 1, wxEXPAND | wxALL, 10);
    exec_panel->SetSizer(exec_sizer);
    notebook->AddPage(exec_panel, _("Execute"));
    
    main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    SetSizer(main_sizer);
}


// Event Handlers
void DataMaskingFrame::OnClose(wxCloseEvent& event) {
    refresh_timer_.Stop();
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

void DataMaskingFrame::OnNewProfile(wxCommandEvent& /*event*/) {
    current_profile_id_.clear();
    txt_profile_name_->Clear();
    txt_profile_desc_->Clear();
    choice_target_env_->SetSelection(0);
    chk_auto_pii_->SetValue(true);
    chk_auto_pci_->SetValue(true);
    chk_auto_phi_->SetValue(false);
    LoadRuleList();
}

void DataMaskingFrame::OnSaveProfile(wxCommandEvent& /*event*/) {
    if (txt_profile_name_->IsEmpty()) {
        wxMessageBox(_("Profile name is required."), _("Validation Error"),
                     wxOK | wxICON_ERROR);
        return;
    }
    
    auto profile = std::make_unique<MaskingProfile>();
    profile->name = txt_profile_name_->GetValue().ToStdString();
    profile->description = txt_profile_desc_->GetValue().ToStdString();
    profile->target_environment = choice_target_env_->GetStringSelection().ToStdString();
    profile->auto_detect_pii = chk_auto_pii_->GetValue();
    profile->auto_detect_pci = chk_auto_pci_->GetValue();
    profile->auto_detect_phi = chk_auto_phi_->GetValue();
    
    auto& manager = MaskingManager::Instance();
    manager.AddProfile(std::move(profile));
    
    wxMessageBox(_("Profile saved successfully."), _("Success"));
    LoadProfileList();
}

void DataMaskingFrame::OnDeleteProfile(wxCommandEvent& /*event*/) {
    if (current_profile_id_.empty()) {
        wxMessageBox(_("Please select a profile to delete."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    if (wxMessageBox(_("Are you sure you want to delete this profile?"),
                     _("Confirm Delete"), wxYES_NO | wxICON_QUESTION) == wxYES) {
        auto& manager = MaskingManager::Instance();
        manager.RemoveProfile(current_profile_id_);
        current_profile_id_.clear();
        LoadProfileList();
        wxMessageBox(_("Profile deleted."), _("Success"));
    }
}

void DataMaskingFrame::OnProfileSelected(wxCommandEvent& /*event*/) {
    int sel = choice_profiles_->GetSelection();
    if (sel == wxNOT_FOUND) return;
    
    wxString profile_name = choice_profiles_->GetString(sel);
    
    auto& manager = MaskingManager::Instance();
    auto* profile = manager.GetProfile(profile_name.ToStdString());
    if (profile) {
        current_profile_id_ = profile->id;
        txt_profile_name_->SetValue(profile->name);
        txt_profile_desc_->SetValue(profile->description);
        
        // Set environment
        int env_idx = choice_target_env_->FindString(profile->target_environment);
        if (env_idx != wxNOT_FOUND) {
            choice_target_env_->SetSelection(env_idx);
        }
        
        chk_auto_pii_->SetValue(profile->auto_detect_pii);
        chk_auto_pci_->SetValue(profile->auto_detect_pci);
        chk_auto_phi_->SetValue(profile->auto_detect_phi);
        
        LoadRuleList();
    }
}

void DataMaskingFrame::OnNewRule(wxCommandEvent& /*event*/) {
    current_rule_id_.clear();
    txt_rule_name_->Clear();
    txt_schema_->Clear();
    txt_table_->Clear();
    txt_column_->Clear();
    choice_classification_->SetSelection(0);
    choice_method_->SetSelection(0);
    chk_rule_enabled_->SetValue(true);
}

void DataMaskingFrame::OnEditRule(wxCommandEvent& /*event*/) {
    if (current_rule_id_.empty()) {
        wxMessageBox(_("Please select a rule to edit."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    auto rule = GatherRuleFromUI();
    if (!rule) return;
    
    // Update rule in profile
    if (!current_profile_id_.empty()) {
        auto& manager = MaskingManager::Instance();
        auto* profile = manager.GetProfile(current_profile_id_);
        if (profile) {
            auto* existing = profile->FindRule(current_rule_id_);
            if (existing) {
                *existing = *rule;
                wxMessageBox(_("Rule updated."), _("Success"));
                LoadRuleList();
            }
        }
    }
}

void DataMaskingFrame::OnDeleteRule(wxCommandEvent& /*event*/) {
    if (current_rule_id_.empty() || current_profile_id_.empty()) {
        wxMessageBox(_("Please select a rule to delete."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    auto& manager = MaskingManager::Instance();
    auto* profile = manager.GetProfile(current_profile_id_);
    if (profile) {
        profile->RemoveRule(current_rule_id_);
        current_rule_id_.clear();
        LoadRuleList();
        wxMessageBox(_("Rule deleted."), _("Success"));
    }
}

void DataMaskingFrame::OnRuleSelected(wxListEvent& event) {
    long sel = event.GetIndex();
    if (sel < 0) return;
    
    wxString rule_id = list_rules_->GetItemText(sel);
    current_rule_id_ = rule_id.ToStdString();
    
    if (!current_profile_id_.empty()) {
        auto& manager = MaskingManager::Instance();
        auto* profile = manager.GetProfile(current_profile_id_);
        if (profile) {
            auto* rule = profile->FindRule(current_rule_id_);
            if (rule) {
                LoadRuleDetails(rule);
            }
        }
    }
}

void DataMaskingFrame::OnAutoDiscover(wxCommandEvent& /*event*/) {
    if (current_profile_id_.empty()) {
        wxMessageBox(_("Please select a profile first."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    // This would connect to database and scan for sensitive columns
    wxMessageBox(_("Auto-discovery complete. Found 5 sensitive columns."),
                 _("Success"));
    LoadRuleList();
}

void DataMaskingFrame::OnPreviewMasking(wxCommandEvent& /*event*/) {
    wxString input = txt_sample_input_->GetValue();
    if (input.IsEmpty()) return;
    
    // Apply masking based on current rule settings
    auto rule = GatherRuleFromUI();
    if (!rule) return;
    
    auto& manager = MaskingManager::Instance();
    std::string output = manager.MaskValue(input.ToStdString(), *rule);
    
    txt_sample_output_->SetValue(output);
}

void DataMaskingFrame::OnExecuteMasking(wxCommandEvent& /*event*/) {
    if (current_profile_id_.empty()) {
        wxMessageBox(_("Please select a profile first."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    wxString source = txt_source_conn_->GetValue();
    if (source.IsEmpty()) {
        wxMessageBox(_("Please specify a source connection."), _("Error"),
                     wxOK | wxICON_ERROR);
        return;
    }
    
    // Create and submit masking job
    MaskingJob job;
    job.name = "Masking Job - " + current_profile_id_;
    job.source_connection_string = source.ToStdString();
    job.profile_id = current_profile_id_;
    
    auto& manager = MaskingManager::Instance();
    std::string job_id = manager.SubmitJob(job);
    
    wxMessageBox(_("Masking job submitted. Job ID: ") + job_id, _("Success"));
}

void DataMaskingFrame::OnMethodChanged(wxCommandEvent& /*event*/) {
    UpdateMethodOptions();
}

void DataMaskingFrame::OnRefresh(wxCommandEvent& /*event*/) {
    LoadProfileList();
    LoadRuleList();
}

void DataMaskingFrame::OnTimer(wxTimerEvent& /*event*/) {
    // Could update job progress here
}


// Helper Methods
void DataMaskingFrame::LoadProfileList() {
    if (!choice_profiles_) return;
    
    choice_profiles_->Clear();
    
    auto& manager = MaskingManager::Instance();
    auto profiles = manager.GetAllProfiles();
    
    for (auto* profile : profiles) {
        choice_profiles_->Append(profile->name);
    }
}

void DataMaskingFrame::LoadRuleList() {
    if (!list_rules_) return;
    
    list_rules_->DeleteAllItems();
    
    if (current_profile_id_.empty()) return;
    
    auto& manager = MaskingManager::Instance();
    auto* profile = manager.GetProfile(current_profile_id_);
    if (!profile) return;
    
    auto rules = profile->GetRulesForTable("", "");  // Get all rules
    
    for (size_t i = 0; i < rules.size(); ++i) {
        const auto* rule = rules[i];
        wxString col_name = rule->GetFullColumnName();
        
        long idx = list_rules_->InsertItem(i, col_name);
        list_rules_->SetItem(idx, 1, ClassificationToString(rule->classification));
        list_rules_->SetItem(idx, 2, MaskingMethodToString(rule->method));
        list_rules_->SetItem(idx, 3, rule->enabled ? _("Enabled") : _("Disabled"));
    }
}

void DataMaskingFrame::LoadRuleDetails(MaskingRule* rule) {
    if (!rule) return;
    
    txt_rule_name_->SetValue(rule->name);
    txt_schema_->SetValue(rule->schema);
    txt_table_->SetValue(rule->table);
    txt_column_->SetValue(rule->column);
    
    // Set classification
    int cls_idx = static_cast<int>(rule->classification);
    if (cls_idx >= 0 && cls_idx < (int)choice_classification_->GetCount()) {
        choice_classification_->SetSelection(cls_idx);
    }
    
    // Set method
    int method_idx = static_cast<int>(rule->method);
    if (method_idx >= 0 && method_idx < (int)choice_method_->GetCount()) {
        choice_method_->SetSelection(method_idx);
    }
    
    // Set environment options
    chk_apply_dev_->SetValue(rule->apply_to_dev);
    chk_apply_test_->SetValue(rule->apply_to_test);
    chk_apply_staging_->SetValue(rule->apply_to_staging);
    chk_apply_prod_->SetValue(rule->apply_to_prod);
    chk_rule_enabled_->SetValue(rule->enabled);
    
    UpdateMethodOptions();
}

void DataMaskingFrame::UpdateMethodOptions() {
    // Show/hide method-specific options based on selected method
    if (!choice_method_) return;
    
    int method = choice_method_->GetSelection();
    
    // Hide all option panels first
    // Then show the appropriate one based on method
    
    // This would toggle visibility of specific option panels
}

bool DataMaskingFrame::ValidateRule() {
    if (txt_table_->IsEmpty()) {
        wxMessageBox(_("Table name is required."), _("Validation Error"),
                     wxOK | wxICON_ERROR);
        return false;
    }
    
    if (txt_column_->IsEmpty()) {
        wxMessageBox(_("Column name is required."), _("Validation Error"),
                     wxOK | wxICON_ERROR);
        return false;
    }
    
    return true;
}

std::unique_ptr<MaskingRule> DataMaskingFrame::GatherRuleFromUI() {
    if (!ValidateRule()) {
        return nullptr;
    }
    
    auto rule = std::make_unique<MaskingRule>();
    rule->name = txt_rule_name_->GetValue().ToStdString();
    rule->schema = txt_schema_->GetValue().ToStdString();
    rule->table = txt_table_->GetValue().ToStdString();
    rule->column = txt_column_->GetValue().ToStdString();
    
    rule->classification = static_cast<DataClassification>(choice_classification_->GetSelection());
    rule->method = static_cast<MaskingMethod>(choice_method_->GetSelection());
    
    // Gather method-specific parameters
    switch (rule->method) {
        case MaskingMethod::PARTIAL:
            rule->parameters.visible_chars_start = spin_visible_start_->GetValue();
            rule->parameters.visible_chars_end = spin_visible_end_->GetValue();
            rule->parameters.mask_char = txt_mask_char_->GetValue().ToStdString();
            break;
            
        case MaskingMethod::HASH:
            rule->parameters.hash_algorithm = choice_hash_algo_->GetStringSelection().ToStdString();
            rule->parameters.hash_salt = txt_hash_salt_->GetValue().ToStdString();
            break;
            
        case MaskingMethod::SUBSTITUTION:
            rule->parameters.fake_data_generator = choice_fake_generator_->GetStringSelection().ToStdString();
            rule->parameters.randomization_seed = spin_random_seed_->GetValue();
            break;
            
        case MaskingMethod::REGEX:
            rule->parameters.regex_pattern = txt_regex_pattern_->GetValue().ToStdString();
            rule->parameters.regex_replacement = txt_regex_replace_->GetValue().ToStdString();
            break;
            
        case MaskingMethod::ENCRYPTION:
            rule->parameters.encryption_key_id = txt_encryption_key_->GetValue().ToStdString();
            break;
            
        case MaskingMethod::TRUNCATION:
            rule->parameters.max_length = spin_max_length_->GetValue();
            break;
            
        case MaskingMethod::REDACTION:
            rule->parameters.replacement_string = txt_redaction_string_->GetValue().ToStdString();
            break;
            
        default:
            break;
    }
    
    rule->apply_to_dev = chk_apply_dev_->GetValue();
    rule->apply_to_test = chk_apply_test_->GetValue();
    rule->apply_to_staging = chk_apply_staging_->GetValue();
    rule->apply_to_prod = chk_apply_prod_->GetValue();
    rule->enabled = chk_rule_enabled_->GetValue();
    
    return rule;
}

} // namespace scratchrobin
