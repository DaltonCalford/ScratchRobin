/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PRINT_DIALOG_H
#define SCRATCHROBIN_PRINT_DIALOG_H

#include <wx/print.h>
#include <wx/dialog.h>

class wxChoice;
class wxSpinCtrl;
class wxRadioBox;
class wxCheckBox;

namespace scratchrobin {

class DiagramCanvas;
class SqlEditorFrame;

// Print scope options
enum class PrintScope {
    CurrentPage,
    AllPages,
    Selection
};

// Print options
struct PrintOptions {
    PrintScope scope = PrintScope::CurrentPage;
    int copies = 1;
    bool color = true;
    bool fitToPage = true;
    int orientation = wxPORTRAIT;  // wxPORTRAIT or wxLANDSCAPE
    double scale = 1.0;
};

// Print dialog for diagrams and SQL results
class DiagramPrintDialog : public wxDialog {
public:
    DiagramPrintDialog(wxWindow* parent, DiagramCanvas* canvas);

    bool ShowPrintDialog();
    bool DoPrint();

private:
    void BuildLayout();
    void OnPrintSetup(wxCommandEvent& event);
    void OnPreview(wxCommandEvent& event);
    void OnPrint(wxCommandEvent& event);

    DiagramCanvas* canvas_;
    PrintOptions options_;
    
    wxPrintData print_data_;
    wxPageSetupData page_setup_data_;
    
    wxChoice* scope_choice_ = nullptr;
    wxSpinCtrl* copies_spin_ = nullptr;
    wxRadioBox* orientation_radio_ = nullptr;
    wxCheckBox* color_chk_ = nullptr;
    wxCheckBox* fit_to_page_chk_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

// Printout class for diagrams
class DiagramPrintout : public wxPrintout {
public:
    DiagramPrintout(DiagramCanvas* canvas, const wxString& title = "Diagram");

    bool OnPrintPage(int page) override;
    bool HasPage(int page) override;
    bool OnBeginDocument(int startPage, int endPage) override;
    void GetPageInfo(int* minPage, int* maxPage, int* pageFrom, int* pageTo) override;

private:
    DiagramCanvas* canvas_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PRINT_DIALOG_H
