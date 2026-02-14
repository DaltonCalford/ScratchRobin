/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_COPY_DIALOG_H
#define SCRATCHROBIN_COPY_DIALOG_H

#include <string>

#include <wx/dialog.h>

#include "core/connection_manager.h"

class wxButton;
class wxChoice;
class wxTextCtrl;
class wxStaticText;
class wxCheckBox;

namespace scratchrobin {

class CopyDialog : public wxDialog {
public:
    CopyDialog(wxWindow* parent, ConnectionManager* connectionManager,
               const std::string& initialSql);

private:
    void OnBrowseInput(wxCommandEvent& event);
    void OnBrowseOutput(wxCommandEvent& event);
    void OnRun(wxCommandEvent& event);
    void OnModeChanged(wxCommandEvent& event);
    void OnInputClipboardToggle(wxCommandEvent& event);
    void OnOutputClipboardToggle(wxCommandEvent& event);
    CopyOptions BuildOptions() const;
    void UpdateControlStates();

    ConnectionManager* connection_manager_ = nullptr;
    wxChoice* mode_choice_ = nullptr;
    wxTextCtrl* sql_ctrl_ = nullptr;
    wxTextCtrl* input_path_ctrl_ = nullptr;
    wxTextCtrl* output_path_ctrl_ = nullptr;
    wxCheckBox* input_clipboard_check_ = nullptr;
    wxCheckBox* output_clipboard_check_ = nullptr;
    wxTextCtrl* clipboard_ctrl_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxButton* run_button_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_COPY_DIALOG_H
