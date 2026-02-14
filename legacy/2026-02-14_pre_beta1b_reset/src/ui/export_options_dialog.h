/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_EXPORT_OPTIONS_DIALOG_H
#define SCRATCHROBIN_EXPORT_OPTIONS_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxChoice;
class wxSpinCtrl;
class wxTextCtrl;
class wxRadioBox;

namespace scratchrobin {

enum class ExportFormat {
    Png,
    Svg,
    Pdf
};

enum class ExportScope {
    All,
    Visible,
    Selection
};

struct ExportOptions {
    ExportFormat format = ExportFormat::Png;
    ExportScope scope = ExportScope::All;
    int dpi = 96;
    int quality = 95;  // For PNG/JPEG
    std::string filename;
    bool transparent_background = false;
    bool include_grid = false;
};

class ExportOptionsDialog : public wxDialog {
public:
    ExportOptionsDialog(wxWindow* parent, ExportOptions& options);

    bool IsConfirmed() const { return confirmed_; }

private:
    void BuildLayout();
    void OnFormatChanged(wxCommandEvent& event);
    void OnBrowse(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void UpdateFilenameExtension();

    ExportOptions& options_;
    bool confirmed_ = false;
    
    wxChoice* format_choice_ = nullptr;
    wxRadioBox* scope_radio_ = nullptr;
    wxSpinCtrl* dpi_spin_ = nullptr;
    wxSpinCtrl* quality_spin_ = nullptr;
    wxTextCtrl* filename_ctrl_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_EXPORT_OPTIONS_DIALOG_H
