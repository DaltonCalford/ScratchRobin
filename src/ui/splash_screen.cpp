/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/splash_screen.h"

#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/settings.h>
#include <wx/frame.h>

namespace scratchrobin::ui {

BEGIN_EVENT_TABLE(SplashScreen, wxDialog)
END_EVENT_TABLE()

SplashScreen::SplashScreen(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition,
               wxSize(500, 320), wxBORDER_NONE | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP) {
    
    buildUI();
    CentreOnScreen();
}

void SplashScreen::buildUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Background color - dark blue gradient simulation
    SetBackgroundColour(wxColour(30, 50, 80));
    
    // Title section
    wxBoxSizer* title_sizer = new wxBoxSizer(wxVERTICAL);
    
    title_text_ = new wxStaticText(this, wxID_ANY, wxT("Robin Migrate"));
    wxFont title_font = title_text_->GetFont();
    title_font.SetPointSize(28);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title_text_->SetFont(title_font);
    title_text_->SetForegroundColour(wxColour(255, 255, 255));
    title_sizer->Add(title_text_, 0, wxALIGN_CENTER | wxTOP, 40);
    
    version_text_ = new wxStaticText(this, wxID_ANY, wxT("Version 1.0 - ScratchBird Client"));
    wxFont version_font = version_text_->GetFont();
    version_font.SetPointSize(10);
    version_text_->SetFont(version_font);
    version_text_->SetForegroundColour(wxColour(180, 200, 230));
    title_sizer->Add(version_text_, 0, wxALIGN_CENTER | wxTOP, 5);
    
    main_sizer->Add(title_sizer, 1, wxEXPAND);
    
    // Progress section
    wxBoxSizer* progress_sizer = new wxBoxSizer(wxVERTICAL);
    
    status_text_ = new wxStaticText(this, wxID_ANY, wxT("Initializing..."));
    status_text_->SetForegroundColour(wxColour(255, 255, 255));
    progress_sizer->Add(status_text_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    progress_bar_ = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, 
                                wxSize(460, 20), wxGA_HORIZONTAL | wxGA_SMOOTH);
    progress_bar_->SetValue(0);
    progress_sizer->Add(progress_bar_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);
    
    main_sizer->Add(progress_sizer, 0, wxEXPAND);
    
    SetSizer(main_sizer);
}

void SplashScreen::ShowSplash() {
    Show();
    Raise();
    Update();
    wxYield();
}

void SplashScreen::SetProgress(int percentage) {
    percentage = std::max(0, std::min(100, percentage));
    if (progress_bar_) {
        progress_bar_->SetValue(percentage);
    }
    Update();
    wxYield();
}

void SplashScreen::SetStatusMessage(const wxString& message) {
    if (status_text_) {
        status_text_->SetLabel(message);
    }
    Update();
    wxYield();
}

void SplashScreen::BeginLoading() {
    SetProgress(0);
    SetStatusMessage(wxT("Starting application..."));
}

void SplashScreen::SetStepConfigLoad() {
    SetProgress(20);
    SetStatusMessage(wxT("Loading configuration..."));
}

void SplashScreen::SetStepDecryptPasswords() {
    SetProgress(50);
    SetStatusMessage(wxT("Waiting for password decryption key..."));
}

void SplashScreen::SetStepInitUI() {
    SetProgress(75);
    SetStatusMessage(wxT("Initializing user interface..."));
}

void SplashScreen::SetStepComplete() {
    SetProgress(100);
    SetStatusMessage(wxT("Ready"));
    wxMilliSleep(200);  // Brief pause so user sees 100%
}

void SplashScreen::CloseSplash() {
    Hide();
}

}  // namespace scratchrobin::ui
