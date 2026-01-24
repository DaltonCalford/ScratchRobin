#include "diagram_frame.h"

#include "diagram_page.h"
#include "icon_bar.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "window_manager.h"

#include <wx/notebook.h>
#include <wx/sizer.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(DiagramFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, DiagramFrame::OnNewDiagram)
    EVT_CLOSE(DiagramFrame::OnClose)
wxEND_EVENT_TABLE()

DiagramFrame::DiagramFrame(WindowManager* windowManager, const AppConfig* config)
    : wxFrame(nullptr, wxID_ANY, "Diagrams", wxDefaultPosition, wxSize(1100, 700)),
      window_manager_(windowManager),
      app_config_(config) {
    if (window_manager_) {
        window_manager_->RegisterWindow(this);
        window_manager_->RegisterDiagramHost(this);
    }

    WindowChromeConfig chrome;
    if (app_config_) {
        chrome = app_config_->chrome.diagram;
    }

    if (chrome.showMenu) {
        MenuBuildOptions options;
        options.includeConnections = chrome.replicateMenu;
        auto* menu_bar = BuildMenuBar(options);
        SetMenuBar(menu_bar);
    }

    if (chrome.showIconBar) {
        IconBarType type = chrome.replicateIconBar ? IconBarType::Main : IconBarType::Diagram;
        BuildIconBar(this, type, 24);
    }

    notebook_ = new wxNotebook(this, wxID_ANY);
    AddDiagramTab();

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(notebook_, 1, wxEXPAND);
    SetSizer(rootSizer);
}

void DiagramFrame::AddDiagramTab(const wxString& title) {
    if (!notebook_) {
        return;
    }
    diagram_counter_++;
    wxString label = title;
    if (label.empty()) {
        label = wxString::Format("Diagram %d", diagram_counter_);
    }
    notebook_->AddPage(new DiagramPage(notebook_), label, true);
}

void DiagramFrame::OnNewDiagram(wxCommandEvent&) {
    AddDiagramTab(wxString::Format("Diagram %d", diagram_counter_ + 1));
}

void DiagramFrame::OnClose(wxCloseEvent& event) {
    if (window_manager_) {
        window_manager_->UnregisterDiagramHost(this);
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
    event.Skip();
}

} // namespace scratchrobin
