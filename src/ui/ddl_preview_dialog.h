/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DDL_PREVIEW_DIALOG_H
#define SCRATCHROBIN_DDL_PREVIEW_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxTextCtrl;
class wxChoice;
class wxCheckBox;

namespace scratchrobin {

class DiagramModel;

enum class DdlTarget {
    ScratchBird,
    PostgreSQL,
    MySQL,
    Firebird
};

class DdlPreviewDialog : public wxDialog {
public:
    DdlPreviewDialog(wxWindow* parent, const DiagramModel& model);

    void SetTarget(DdlTarget target);
    void GenerateDdl();
    
    std::string GetDdl() const;
    DdlTarget GetTarget() const;

private:
    void BuildLayout();
    void OnTargetChanged(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnExecute(wxCommandEvent& event);
    
    std::string GenerateDdlForTarget(DdlTarget target) const;

    const DiagramModel& model_;
    DdlTarget current_target_ = DdlTarget::ScratchBird;
    
    wxChoice* target_choice_ = nullptr;
    wxTextCtrl* ddl_text_ = nullptr;
    wxCheckBox* include_drops_chk_ = nullptr;
    wxCheckBox* include_comments_chk_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DDL_PREVIEW_DIALOG_H
