/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "shortcuts_cheat_sheet.h"

#include <sstream>

#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>

namespace scratchrobin {

// =============================================================================
// ShortcutsCheatSheet
// =============================================================================

wxBEGIN_EVENT_TABLE(ShortcutsCheatSheet, wxDialog)
    EVT_KEY_DOWN(ShortcutsCheatSheet::OnKeyDown)
wxEND_EVENT_TABLE()

ShortcutsCheatSheet::ShortcutsCheatSheet(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Keyboard Shortcuts Cheat Sheet",
               wxDefaultPosition, wxSize(700, 600),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX) {
    InitializeCategories();
    BuildLayout();
}

void ShortcutsCheatSheet::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Title
    auto* title = new wxStaticText(this, wxID_ANY, "Keyboard Shortcuts Reference");
    title->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                          wxFONTWEIGHT_BOLD));
    root->Add(title, 0, wxALIGN_CENTER | wxALL, 16);
    
    // Scrolled content window
    scrolled_window_ = new wxScrolledWindow(this, wxID_ANY);
    scrolled_window_->SetScrollRate(10, 10);
    
    auto* contentSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add categories
    for (const auto& category : categories_) {
        // Category header
        auto* catLabel = new wxStaticText(scrolled_window_, wxID_ANY, category.name);
        catLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_BOLD));
        catLabel->SetForegroundColour(wxColour(0, 80, 120));
        contentSizer->Add(catLabel, 0, wxEXPAND | wxTOP | wxBOTTOM, 8);
        
        // Add a line under category
        auto* line = new wxStaticText(scrolled_window_, wxID_ANY, 
                                      wxString(50, wxUniChar('-')));
        line->SetForegroundColour(wxColour(180, 180, 180));
        contentSizer->Add(line, 0, wxEXPAND | wxBOTTOM, 8);
        
        // Shortcut entries in two columns
        auto* entrySizer = new wxFlexGridSizer(2, 2, 8, 32);
        entrySizer->AddGrowableCol(1);
        
        for (const auto& entry : category.entries) {
            // Shortcut key (bold, with border effect using spaces)
            auto* shortcutLabel = new wxStaticText(scrolled_window_, wxID_ANY,
                                                   "  " + entry.shortcut + "  ");
            shortcutLabel->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                                          wxFONTWEIGHT_BOLD));
            shortcutLabel->SetBackgroundColour(wxColour(240, 240, 240));
            shortcutLabel->SetForegroundColour(wxColour(0, 0, 0));
            entrySizer->Add(shortcutLabel, 0, wxALIGN_CENTER_VERTICAL);
            
            // Description
            auto* descLabel = new wxStaticText(scrolled_window_, wxID_ANY,
                                               entry.description);
            descLabel->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                      wxFONTWEIGHT_NORMAL));
            entrySizer->Add(descLabel, 0, wxALIGN_CENTER_VERTICAL);
        }
        
        contentSizer->Add(entrySizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);
    }
    
    scrolled_window_->SetSizer(contentSizer);
    root->Add(scrolled_window_, 1, wxEXPAND | wxLEFT | wxRIGHT, 16);
    
    // Button row
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer(1);
    
    auto* printBtn = new wxButton(this, wxID_ANY, "Print...");
    auto* closeBtn = new wxButton(this, wxID_OK, "Close");
    closeBtn->SetDefault();
    
    buttonSizer->Add(printBtn, 0, wxRIGHT, 8);
    buttonSizer->Add(closeBtn, 0);
    
    root->Add(buttonSizer, 0, wxEXPAND | wxALL, 16);
    
    SetSizer(root);
    
    // Bind events
    printBtn->Bind(wxEVT_BUTTON, &ShortcutsCheatSheet::OnPrint, this);
    closeBtn->Bind(wxEVT_BUTTON, &ShortcutsCheatSheet::OnClose, this);
    
    // Allow Escape to close
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& evt) {
        if (evt.GetKeyCode() == WXK_ESCAPE) {
            EndModal(wxID_OK);
        } else {
            evt.Skip();
        }
    });
}

void ShortcutsCheatSheet::InitializeCategories() {
    categories_.clear();
    
    // Global shortcuts
    CheatSheetCategory global;
    global.name = "Global Shortcuts";
    global.entries = {
        {"Ctrl+N", "New SQL Editor"},
        {"Ctrl+Shift+N", "New Diagram"},
        {"Ctrl+O", "Open Connection"},
        {"Ctrl+W", "Close Window"},
        {"Ctrl+Q", "Quit Application"},
        {"Ctrl+Shift+?", "Show Shortcuts Help"}
    };
    categories_.push_back(global);
    
    // SQL Editor shortcuts
    CheatSheetCategory editor;
    editor.name = "SQL Editor";
    editor.entries = {
        {"F5", "Execute Query"},
        {"F6", "Execute Current Statement"},
        {"Esc", "Cancel Running Query"},
        {"Ctrl+F5", "Explain Query Plan"},
        {"Ctrl+Shift+F", "Format SQL Code"},
        {"Ctrl+Space", "Show Auto-complete"},
        {"Ctrl+/", "Toggle Comment"},
        {"Ctrl+F", "Find"},
        {"Ctrl+H", "Replace"},
        {"F3", "Find Next"}
    };
    categories_.push_back(editor);
    
    // Catalog Tree shortcuts
    CheatSheetCategory catalog;
    catalog.name = "Catalog Tree";
    catalog.entries = {
        {"Enter", "Open Selected Object"},
        {"F2", "Rename Object"},
        {"Delete", "Delete Object"},
        {"Ctrl+C", "Copy Object Name"},
        {"Ctrl+Shift+C", "Copy DDL"}
    };
    categories_.push_back(catalog);
    
    // Diagram shortcuts
    CheatSheetCategory diagram;
    diagram.name = "Diagram";
    diagram.entries = {
        {"Delete", "Delete Selected"},
        {"Ctrl+A", "Select All"},
        {"Ctrl+Shift+A", "Align Selected"},
        {"Ctrl+Plus", "Zoom In"},
        {"Ctrl+Minus", "Zoom Out"},
        {"Ctrl+0", "Reset Zoom"},
        {"Ctrl+G", "Toggle Grid"},
        {"Ctrl+Shift+G", "Group Selected"}
    };
    categories_.push_back(diagram);
    
    // Results Grid shortcuts
    CheatSheetCategory results;
    results.name = "Results Grid";
    results.entries = {
        {"Ctrl+C", "Copy Selected Cells"},
        {"Ctrl+F", "Find in Results"},
        {"Ctrl+A", "Select All"},
        {"Ctrl+E", "Export Results"}
    };
    categories_.push_back(results);
}

void ShortcutsCheatSheet::OnPrint(wxCommandEvent&) {
    wxPrintDialogData printDialogData;
    wxPrinter printer(&printDialogData);
    
    class CheatSheetPrintout : public wxPrintout {
    public:
        CheatSheetPrintout(const std::vector<CheatSheetCategory>& categories)
            : wxPrintout("Keyboard Shortcuts Cheat Sheet"),
              categories_(categories) {}
        
        bool OnPrintPage(int page) override {
            if (page != 1) return false;
            
            wxDC* dc = GetDC();
            if (!dc) return false;
            
            int pageWidth, pageHeight;
            GetPageSizePixels(&pageWidth, &pageHeight);
            
            // Scale to fit
            float scaleX = (float)pageWidth / 800.0f;
            float scaleY = (float)pageHeight / 1100.0f;
            float scale = std::min(scaleX, scaleY) * 0.9f;
            dc->SetUserScale(scale, scale);
            
            int y = 50;
            int leftMargin = 50;
            
            // Title
            dc->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
            dc->DrawText("Keyboard Shortcuts Cheat Sheet", leftMargin, y);
            y += 60;
            
            // Categories
            dc->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
            
            for (const auto& category : categories_) {
                // Check if we need a new page (simplified)
                if (y > 1000) break;
                
                // Category name
                dc->SetTextForeground(wxColour(0, 80, 120));
                dc->DrawText(category.name, leftMargin, y);
                y += 30;
                
                // Line
                dc->SetPen(wxPen(wxColour(180, 180, 180), 1));
                dc->DrawLine(leftMargin, y, leftMargin + 400, y);
                y += 15;
                
                // Entries
                dc->SetFont(wxFont(11, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                                  wxFONTWEIGHT_BOLD));
                dc->SetTextForeground(wxColour(0, 0, 0));
                
                for (const auto& entry : category.entries) {
                    if (y > 1000) break;
                    
                    // Shortcut
                    dc->DrawText(entry.shortcut, leftMargin + 20, y);
                    
                    // Description
                    dc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                      wxFONTWEIGHT_NORMAL));
                    dc->DrawText(entry.description, leftMargin + 180, y);
                    dc->SetFont(wxFont(11, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                                      wxFONTWEIGHT_BOLD));
                    
                    y += 25;
                }
                
                y += 20;
                dc->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                  wxFONTWEIGHT_BOLD));
            }
            
            return true;
        }
        
        bool HasPage(int page) override { return page == 1; }
        void GetPageInfo(int* minPage, int* maxPage, int* pageFrom, int* pageTo) override {
            *minPage = 1;
            *maxPage = 1;
            *pageFrom = 1;
            *pageTo = 1;
        }
        
    private:
        const std::vector<CheatSheetCategory>& categories_;
    };
    
    CheatSheetPrintout printout(categories_);
    
    if (!printer.Print(this, &printout, true)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR) {
            wxMessageBox("There was a problem printing.", "Print Error",
                        wxOK | wxICON_ERROR);
        }
    }
}

void ShortcutsCheatSheet::OnClose(wxCommandEvent&) {
    EndModal(wxID_OK);
}

void ShortcutsCheatSheet::OnKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_ESCAPE) {
        EndModal(wxID_OK);
    } else {
        event.Skip();
    }
}

// =============================================================================
// Convenience Functions
// =============================================================================

void ShowShortcutsCheatSheet(wxWindow* parent) {
    ShortcutsCheatSheet dlg(parent);
    dlg.ShowModal();
}

} // namespace scratchrobin
