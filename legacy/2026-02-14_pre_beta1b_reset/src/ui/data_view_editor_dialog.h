/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DATA_VIEW_EDITOR_DIALOG_H
#define SCRATCHROBIN_DATA_VIEW_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxTextCtrl;

namespace scratchrobin {

class DataViewEditorDialog : public wxDialog {
public:
    DataViewEditorDialog(wxWindow* parent, const std::string& json_payload);

    std::string json_payload() const;

private:
    void BuildLayout();
    void OnOk(wxCommandEvent& event);

    wxTextCtrl* json_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATA_VIEW_EDITOR_DIALOG_H
