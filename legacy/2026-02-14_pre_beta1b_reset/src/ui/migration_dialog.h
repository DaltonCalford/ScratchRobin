/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_MIGRATION_DIALOG_H
#define SCRATCHROBIN_MIGRATION_DIALOG_H

#include <memory>

#include <wx/dialog.h>

class wxTextCtrl;
class wxChoice;
class wxListBox;
class wxNotebook;
class wxRadioBox;

namespace scratchrobin {

class DiagramModel;

namespace diagram {
class MigrationGenerator;
}

class MigrationDialog : public wxDialog {
public:
    MigrationDialog(wxWindow* parent, const DiagramModel* model);
    ~MigrationDialog();

private:
    void BuildLayout();
    void OnGenerate(wxCommandEvent& event);
    void OnSaveScript(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnDialectChanged(wxCommandEvent& event);

    const DiagramModel* model_;
    std::unique_ptr<diagram::MigrationGenerator> generator_;
    
    wxChoice* dialect_choice_ = nullptr;
    wxRadioBox* direction_radio_ = nullptr;
    wxListBox* warnings_list_ = nullptr;
    wxTextCtrl* script_text_ = nullptr;
    wxNotebook* notebook_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MIGRATION_DIALOG_H
