#include "main_frame.h"
#include "sql_editor_frame.h"
#include "window_manager.h"

#include <functional>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>

namespace scratchrobin {

namespace {
constexpr int kMenuNewSqlEditor = wxID_HIGHEST + 1;
} // namespace

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(kMenuNewSqlEditor, MainFrame::OnNewSqlEditor)
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
    EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(WindowManager* windowManager,
                     MetadataModel* metadataModel,
                     ConnectionManager* connectionManager,
                     const std::vector<ConnectionProfile>* connections)
    : wxFrame(nullptr, wxID_ANY, "ScratchRobin", wxDefaultPosition, wxSize(1024, 768)),
      window_manager_(windowManager),
      metadata_model_(metadataModel),
      connection_manager_(connectionManager),
      connections_(connections) {
    BuildMenu();
    BuildLayout();
    CreateStatusBar();
    SetStatusText("Ready");

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
    if (metadata_model_) {
        metadata_model_->AddObserver(this);
        PopulateTree(metadata_model_->GetSnapshot());
    }
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
    tree_ = new wxTreeCtrl(treePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_DEFAULT_STYLE);
    treeSizer->Add(tree_, 1, wxEXPAND | wxALL, 8);
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
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_);
    editor->Show(true);
}

void MainFrame::OnQuit(wxCommandEvent&) {
    Close(true);
}

void MainFrame::OnClose(wxCloseEvent&) {
    if (metadata_model_) {
        metadata_model_->RemoveObserver(this);
    }
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

void MainFrame::OnMetadataUpdated(const MetadataSnapshot& snapshot) {
    PopulateTree(snapshot);
}

void MainFrame::PopulateTree(const MetadataSnapshot& snapshot) {
    if (!tree_) {
        return;
    }

    tree_->Freeze();
    tree_->DeleteAllItems();

    wxTreeItemId root = tree_->AddRoot("ScratchRobin");
    if (snapshot.roots.empty()) {
        tree_->AppendItem(root, "No connections configured");
        tree_->Expand(root);
        tree_->Thaw();
        return;
    }

    std::function<void(const wxTreeItemId&, const MetadataNode&)> addNode;
    addNode = [&](const wxTreeItemId& parent, const MetadataNode& node) {
        wxTreeItemId id = tree_->AppendItem(parent, node.label);
        for (const auto& child : node.children) {
            addNode(id, child);
        }
    };

    for (const auto& node : snapshot.roots) {
        addNode(root, node);
    }

    tree_->Expand(root);
    tree_->Thaw();
}

} // namespace scratchrobin
