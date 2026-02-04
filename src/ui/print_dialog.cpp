/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "print_dialog.h"

#include "diagram_canvas.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dc.h>
#include <wx/msgdlg.h>
#include <wx/printdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace scratchrobin {

// ============================================================================
// DiagramPrintout
// ============================================================================

DiagramPrintout::DiagramPrintout(DiagramCanvas* canvas, const wxString& title)
    : wxPrintout(title), canvas_(canvas) {}

bool DiagramPrintout::OnPrintPage(int page) {
    if (!canvas_) return false;
    
    wxDC* dc = GetDC();
    if (!dc) return false;
    
    // Get page size in logical units
    int pw, ph;
    GetPageSizePixels(&pw, &ph);
    wxSize pageSize(pw, ph);
    
    // Get the size of the canvas content
    const auto& model = canvas_->model();
    const auto& nodes = model.nodes();
    
    if (nodes.empty()) {
        dc->DrawText("No content to print", 100, 100);
        return true;
    }
    
    // Calculate bounds
    double minX = nodes[0].x, minY = nodes[0].y;
    double maxX = nodes[0].x + nodes[0].width;
    double maxY = nodes[0].y + nodes[0].height;
    
    for (const auto& node : nodes) {
        minX = std::min(minX, node.x);
        minY = std::min(minY, node.y);
        maxX = std::max(maxX, node.x + node.width);
        maxY = std::max(maxY, node.y + node.height);
    }
    
    // Add padding
    minX -= 50; minY -= 50;
    maxX += 50; maxY += 50;
    
    double contentWidth = maxX - minX;
    double contentHeight = maxY - minY;
    
    // Calculate scale to fit page
    double scaleX = pageSize.GetWidth() / contentWidth;
    double scaleY = pageSize.GetHeight() / contentHeight;
    double scale = std::min(scaleX, scaleY) * 0.9;  // 90% to leave margin
    
    // Set up DC
    dc->SetUserScale(scale, scale);
    
    // Offset to center content
    double offsetX = (pageSize.GetWidth() / scale - contentWidth) / 2 - minX;
    double offsetY = (pageSize.GetHeight() / scale - contentHeight) / 2 - minY;
    
    // Draw background
    dc->SetBrush(wxBrush(wxColour(255, 255, 255)));
    dc->SetPen(*wxTRANSPARENT_PEN);
    dc->DrawRectangle(0, 0, pageSize.GetWidth() / scale, pageSize.GetHeight() / scale);
    
    // Draw edges
    dc->SetPen(wxPen(wxColour(0, 0, 0), 2));
    for (const auto& edge : model.edges()) {
        auto sourceIt = std::find_if(nodes.begin(), nodes.end(),
            [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
        auto targetIt = std::find_if(nodes.begin(), nodes.end(),
            [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
        
        if (sourceIt != nodes.end() && targetIt != nodes.end()) {
            double x1 = sourceIt->x + sourceIt->width / 2 + offsetX;
            double y1 = sourceIt->y + sourceIt->height / 2 + offsetY;
            double x2 = targetIt->x + targetIt->width / 2 + offsetX;
            double y2 = targetIt->y + targetIt->height / 2 + offsetY;
            dc->DrawLine(static_cast<int>(x1), static_cast<int>(y1),
                        static_cast<int>(x2), static_cast<int>(y2));
        }
    }
    
    // Draw nodes
    for (const auto& node : nodes) {
        int x = static_cast<int>(node.x + offsetX);
        int y = static_cast<int>(node.y + offsetY);
        int w = static_cast<int>(node.width);
        int h = static_cast<int>(node.height);
        
        // Fill
        dc->SetBrush(wxBrush(wxColour(220, 220, 230)));
        dc->SetPen(wxPen(wxColour(0, 0, 0), 2));
        dc->DrawRectangle(x, y, w, h);
        
        // Title
        dc->SetTextForeground(wxColour(0, 0, 0));
        dc->DrawText(node.name, x + 5, y + 5);
        
        // Type
        dc->DrawText(node.type, x + 5, y + 25);
        
        // Attributes
        int rowY = y + 50;
        for (const auto& attr : node.attributes) {
            wxString text = attr.name + " : " + attr.data_type;
            dc->DrawText(text, x + 5, rowY);
            rowY += 18;
        }
    }
    
    return true;
}

bool DiagramPrintout::HasPage(int page) {
    return page == 1;  // Single page printout for now
}

bool DiagramPrintout::OnBeginDocument(int startPage, int endPage) {
    return wxPrintout::OnBeginDocument(startPage, endPage);
}

void DiagramPrintout::GetPageInfo(int* minPage, int* maxPage, int* pageFrom, int* pageTo) {
    *minPage = 1;
    *maxPage = 1;
    *pageFrom = 1;
    *pageTo = 1;
}

// ============================================================================
// DiagramPrintDialog
// ============================================================================

wxBEGIN_EVENT_TABLE(DiagramPrintDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, DiagramPrintDialog::OnPrintSetup)
wxEND_EVENT_TABLE()

DiagramPrintDialog::DiagramPrintDialog(wxWindow* parent, DiagramCanvas* canvas)
    : wxDialog(parent, wxID_ANY, "Print Diagram",
               wxDefaultPosition, wxSize(400, 350)),
      canvas_(canvas) {
    BuildLayout();
}

void DiagramPrintDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Print scope
    wxArrayString scopes;
    scopes.Add("Current page");
    scopes.Add("All pages");
    scopes.Add("Selection only");
    scope_choice_ = new wxChoice(this, wxID_ANY);
    scope_choice_->Append("Current view");
    scope_choice_->Append("All pages");
    scope_choice_->Append("Selection");
    scope_choice_->SetSelection(0);
    
    auto* scopeSizer = new wxBoxSizer(wxHORIZONTAL);
    scopeSizer->Add(new wxStaticText(this, wxID_ANY, "Print:"), 0,
                    wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    scopeSizer->Add(scope_choice_, 1, wxEXPAND);
    root->Add(scopeSizer, 0, wxEXPAND | wxALL, 12);
    
    // Copies
    auto* copiesSizer = new wxBoxSizer(wxHORIZONTAL);
    copiesSizer->Add(new wxStaticText(this, wxID_ANY, "Copies:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    copies_spin_ = new wxSpinCtrl(this, wxID_ANY);
    copies_spin_->SetRange(1, 99);
    copies_spin_->SetValue(1);
    copiesSizer->Add(copies_spin_, 0);
    root->Add(copiesSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Orientation
    wxArrayString orientations;
    orientations.Add("Portrait");
    orientations.Add("Landscape");
    orientation_radio_ = new wxRadioBox(this, wxID_ANY, "Orientation",
                                         wxDefaultPosition, wxDefaultSize,
                                         orientations, 2);
    orientation_radio_->SetSelection(1);  // Landscape is better for diagrams
    root->Add(orientation_radio_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Options
    color_chk_ = new wxCheckBox(this, wxID_ANY, "Print in color");
    color_chk_->SetValue(true);
    root->Add(color_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    fit_to_page_chk_ = new wxCheckBox(this, wxID_ANY, "Fit to page");
    fit_to_page_chk_->SetValue(true);
    root->Add(fit_to_page_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* setupBtn = new wxButton(this, wxID_ANY, "Page Setup...");
    auto* previewBtn = new wxButton(this, wxID_ANY, "Preview...");
    auto* printBtn = new wxButton(this, wxID_ANY, "Print...");
    
    btnSizer->Add(setupBtn, 0, wxRIGHT, 8);
    btnSizer->Add(previewBtn, 0, wxRIGHT, 8);
    btnSizer->Add(printBtn, 0, wxRIGHT, 8);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
    
    // Bind buttons
    setupBtn->Bind(wxEVT_BUTTON, &DiagramPrintDialog::OnPrintSetup, this);
    previewBtn->Bind(wxEVT_BUTTON, &DiagramPrintDialog::OnPreview, this);
    printBtn->Bind(wxEVT_BUTTON, &DiagramPrintDialog::OnPrint, this);
}

void DiagramPrintDialog::OnPrintSetup(wxCommandEvent&) {
    wxPageSetupDialog dialog(this, &page_setup_data_);
    if (dialog.ShowModal() == wxID_OK) {
        page_setup_data_ = dialog.GetPageSetupData();
    }
}

void DiagramPrintDialog::OnPreview(wxCommandEvent&) {
    wxPrintPreview preview(new DiagramPrintout(canvas_), 
                           new DiagramPrintout(canvas_),
                           &print_data_);
    if (!preview.IsOk()) {
        wxMessageBox("Could not create print preview", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    wxPreviewFrame frame(&preview, this, "Print Preview", wxDefaultPosition, wxSize(800, 600));
    frame.Initialize();
    frame.Show(true);
}

void DiagramPrintDialog::OnPrint(wxCommandEvent&) {
    wxPrintDialogData dialogData(print_data_);
    wxPrinter printer(&dialogData);
    DiagramPrintout printout(canvas_, "Diagram Print");
    
    if (printer.Print(this, &printout, true)) {
        print_data_ = printer.GetPrintDialogData().GetPrintData();
        EndModal(wxID_OK);
    }
}

bool DiagramPrintDialog::ShowPrintDialog() {
    return ShowModal() == wxID_OK;
}

bool DiagramPrintDialog::DoPrint() {
    wxPrintDialogData dialogData(print_data_);
    wxPrinter printer(&dialogData);
    DiagramPrintout printout(canvas_, "Diagram");
    return printer.Print(this, &printout, true);
}

} // namespace scratchrobin
