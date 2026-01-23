#include "startup_frame.h"

#include <wx/bitmap.h>
#include <wx/filefn.h>
#include <wx/gauge.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

namespace scratchrobin {
namespace {

wxBitmap LoadLogoBitmap() {
    const wxString path = "assets/icons/scratchrobin@48.png";
    if (wxFileExists(path)) {
        wxBitmap bitmap;
        bitmap.LoadFile(path, wxBITMAP_TYPE_PNG);
        if (bitmap.IsOk()) {
            return bitmap;
        }
    }
    return wxBitmap();
}

} // namespace

StartupFrame::StartupFrame(const StartupConfig& config)
    : wxFrame(nullptr, wxID_ANY, "ScratchRobin", wxDefaultPosition, wxSize(520, 320),
              wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxBORDER_NONE),
      config_(config) {
    SetBackgroundColour(wxColour(20, 24, 28));

    auto* panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(GetBackgroundColour());

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    wxBitmap logo = LoadLogoBitmap();
    if (logo.IsOk()) {
        logo_ = new wxStaticBitmap(panel, wxID_ANY, logo);
        rootSizer->Add(logo_, 0, wxALIGN_CENTER | wxTOP, 40);
    } else {
        auto* title = new wxStaticText(panel, wxID_ANY, "ScratchRobin");
        title->SetForegroundColour(wxColour(235, 235, 235));
        rootSizer->Add(title, 0, wxALIGN_CENTER | wxTOP, 60);
    }

    status_ = new wxStaticText(panel, wxID_ANY, "Starting...");
    status_->SetForegroundColour(wxColour(200, 200, 200));
    rootSizer->Add(status_, 0, wxALIGN_CENTER | wxTOP, 16);

    if (config_.showProgress) {
        progress_ = new wxGauge(panel, wxID_ANY, 100);
        progress_->SetValue(20);
        rootSizer->Add(progress_, 0, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 32);
    }

    rootSizer->AddStretchSpacer(1);
    panel->SetSizer(rootSizer);

    Bind(wxEVT_LEFT_DOWN, &StartupFrame::OnClick, this);
    panel->Bind(wxEVT_LEFT_DOWN, &StartupFrame::OnClick, this);

    CentreOnScreen();
}

void StartupFrame::SetStatusText(const wxString& text) {
    if (status_) {
        status_->SetLabel(text);
    }
}

void StartupFrame::SetProgress(int value) {
    if (progress_) {
        progress_->SetValue(value);
    }
}

void StartupFrame::OnClick(wxMouseEvent&) {
    Hide();
}

} // namespace scratchrobin
