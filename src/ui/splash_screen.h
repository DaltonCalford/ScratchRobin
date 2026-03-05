/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/bitmap.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/timer.h>

namespace scratchrobin::ui {

// Splash screen shown during application startup
// Displays loading progress and status messages
class SplashScreen : public wxDialog {
public:
    explicit SplashScreen(wxWindow* parent = nullptr);
    
    // Show the splash screen
    void ShowSplash();
    
    // Update loading progress (0-100)
    void SetProgress(int percentage);
    
    // Update status message
    void SetStatusMessage(const wxString& message);
    
    // Loading steps
    void BeginLoading();
    void SetStepConfigLoad();
    void SetStepDecryptPasswords();
    void SetStepInitUI();
    void SetStepComplete();
    
    // Close the splash
    void CloseSplash();

private:
    wxGauge* progress_bar_{nullptr};
    wxStaticText* status_text_{nullptr};
    wxStaticText* title_text_{nullptr};
    wxStaticText* version_text_{nullptr};
    
    void buildUI();
    
    DECLARE_EVENT_TABLE()
};

}  // namespace scratchrobin::ui
