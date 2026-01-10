#include "main_frame.h"
#include "sql_editor_frame.h"

#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/treectrl.h>

namespace scratchrobin {

namespace {
constexpr int kMenuNewSqlEditor = wxID_HIGHEST + 1;
} // namespace

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(kMenuNewSqlEditor, MainFrame::OnNewSqlEditor)
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "ScratchRobin", wxDefaultPosition, wxSize(1024, 768)) {
    BuildMenu();
    BuildLayout();
    CreateStatusBar();
    SetStatusText("Ready");
}

void MainFrame::BuildMenu() {
    auto* fileMenu = new wxMenu();
    fileMenu->Append(kMenuNewSqlEditor, "New SQL Editor\tCtrl+N");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "Quit\tCtrl+Q");

    auto* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "File");

    SetMenuBar(menuBar);
}

void MainFrame::BuildLayout() {
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* treePanel = new wxPanel(splitter, wxID_ANY);
    auto* treeSizer = new wxBoxSizer(wxVERTICAL);
    auto* tree = new wxTreeCtrl(treePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_DEFAULT_STYLE);
    treeSizer->Add(tree, 1, wxEXPAND | wxALL, 8);
    treePanel->SetSizer(treeSizer);

    auto* detailsPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailsSizer = new wxBoxSizer(wxVERTICAL);
    auto* label = new wxStaticText(detailsPanel, wxID_ANY,
                                   "Object details will appear here.");
    detailsSizer->Add(label, 0, wxALL, 8);
    detailsPanel->SetSizer(detailsSizer);

    splitter->SplitVertically(treePanel, detailsPanel, 320);

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(splitter, 1, wxEXPAND);
    SetSizer(rootSizer);
}

void MainFrame::OnNewSqlEditor(wxCommandEvent&) {
    auto* editor = new SqlEditorFrame();
    editor->Show(true);
}

void MainFrame::OnQuit(wxCommandEvent&) {
    Close(true);
}

} // namespace scratchrobin
