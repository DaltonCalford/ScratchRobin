/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_REVERSE_ENGINEER_WIZARD_H
#define SCRATCHROBIN_REVERSE_ENGINEER_WIZARD_H

#include "diagram/reverse_engineer.h"

#include <memory>
#include <vector>

#include <wx/wizard.h>

class wxCheckListBox;
class wxChoice;
class wxGauge;
class wxListBox;
class wxStaticText;
class wxCheckBox;

namespace scratchrobin {

class ConnectionManager;
class ConnectionProfile;
class DiagramModel;

// Wizard page for selecting connection and schema
class SchemaSelectionPage : public wxWizardPage {
public:
    SchemaSelectionPage(wxWizard* parent,
                        ConnectionManager* connection_manager,
                        const std::vector<ConnectionProfile>* connections);

    wxWizardPage* GetPrev() const override { return nullptr; }
    wxWizardPage* GetNext() const override { return next_page_; }
    void SetNext(wxWizardPage* next) { next_page_ = next; }
    bool TransferDataFromWindow() override;
    
    std::string GetSelectedSchema() const;
    const ConnectionProfile* GetSelectedProfile() const;

private:
    void OnConnectionChanged(wxCommandEvent& event);
    void LoadSchemas();
    void BuildLayout();

    ConnectionManager* connection_manager_;
    const std::vector<ConnectionProfile>* connections_;
    
    wxChoice* connection_choice_ = nullptr;
    wxChoice* schema_choice_ = nullptr;
    wxStaticText* status_text_ = nullptr;
    wxWizardPage* next_page_ = nullptr;
};

// Wizard page for selecting tables
class TableSelectionPage : public wxWizardPage {
public:
    TableSelectionPage(wxWizard* parent);

    wxWizardPage* GetPrev() const override { return prev_page_; }
    wxWizardPage* GetNext() const override { return next_page_; }
    void SetPrev(wxWizardPage* prev) { prev_page_ = prev; }
    void SetNext(wxWizardPage* next) { next_page_ = next; }
    bool TransferDataFromWindow() override;
    
    void SetSchema(const std::string& schema);
    void SetProfile(ConnectionManager* cm, const ConnectionProfile* profile);
    std::vector<std::string> GetSelectedTables() const;

private:
    void LoadTables();
    void OnSelectAll(wxCommandEvent& event);
    void OnDeselectAll(wxCommandEvent& event);
    void BuildLayout();

    std::string schema_;
    ConnectionManager* connection_manager_ = nullptr;
    const ConnectionProfile* profile_ = nullptr;
    
    wxCheckListBox* tables_list_ = nullptr;
    wxStaticText* status_text_ = nullptr;
    wxWizardPage* prev_page_ = nullptr;
    wxWizardPage* next_page_ = nullptr;
};

// Wizard page for import options
class ImportOptionsPage : public wxWizardPage {
public:
    ImportOptionsPage(wxWizard* parent);

    wxWizardPage* GetPrev() const override { return prev_page_; }
    wxWizardPage* GetNext() const override { return nullptr; }
    void SetPrev(wxWizardPage* prev) { prev_page_ = prev; }
    
    diagram::ReverseEngineerOptions GetOptions() const;

private:
    void BuildLayout();

    wxCheckBox* include_indexes_chk_ = nullptr;
    wxCheckBox* include_constraints_chk_ = nullptr;
    wxCheckBox* include_comments_chk_ = nullptr;
    wxCheckBox* auto_layout_chk_ = nullptr;
    wxChoice* layout_algo_choice_ = nullptr;
    wxWizardPage* prev_page_ = nullptr;
};

// Main wizard class
class ReverseEngineerWizard : public wxWizard {
public:
    ReverseEngineerWizard(wxWindow* parent,
                          ConnectionManager* connection_manager,
                          const std::vector<ConnectionProfile>* connections);

    // Run the wizard and import to the given model
    bool Run(DiagramModel* model);

private:
    void OnWizardFinished(wxWizardEvent& event);
    void OnWizardCancelled(wxWizardEvent& event);

    ConnectionManager* connection_manager_;
    const std::vector<ConnectionProfile>* connections_;
    
    SchemaSelectionPage* schema_page_ = nullptr;
    TableSelectionPage* tables_page_ = nullptr;
    ImportOptionsPage* options_page_ = nullptr;
    
    bool completed_ = false;
    DiagramModel* target_model_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

// Progress dialog for import
class ImportProgressDialog : public wxDialog {
public:
    ImportProgressDialog(wxWindow* parent, int total_tables);

    void UpdateProgress(const std::string& table_name, int current, int total);
    void SetCompleted(bool success, const std::string& message);
    bool WasCancelled() const { return cancelled_; }

private:
    void OnCancel(wxCommandEvent& event);
    void BuildLayout();

    wxStaticText* current_table_text_ = nullptr;
    wxStaticText* count_text_ = nullptr;
    wxGauge* progress_gauge_ = nullptr;
    wxButton* cancel_btn_ = nullptr;
    bool cancelled_ = false;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_REVERSE_ENGINEER_WIZARD_H
