#include "main_frame.h"
#include "sql_editor_frame.h"
#include "window_manager.h"

#include <functional>
#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

namespace {
constexpr int kMenuNewSqlEditor = wxID_HIGHEST + 1;

class MetadataNodeData : public wxTreeItemData {
public:
    explicit MetadataNodeData(const MetadataNode* node) : node_(node) {}

    const MetadataNode* GetNode() const { return node_; }

private:
    const MetadataNode* node_ = nullptr;
};
} // namespace

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(kMenuNewSqlEditor, MainFrame::OnNewSqlEditor)
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_TREE_SEL_CHANGED(wxID_ANY, MainFrame::OnTreeSelection)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(WindowManager* windowManager,
                     MetadataModel* metadataModel,
                     ConnectionManager* connectionManager,
                     const std::vector<ConnectionProfile>* connections,
                     const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "ScratchRobin", wxDefaultPosition, wxSize(1024, 768)),
      window_manager_(windowManager),
      metadata_model_(metadataModel),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
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
    inspector_notebook_ = new wxNotebook(detailsPanel, wxID_ANY);

    auto* overviewPanel = new wxPanel(inspector_notebook_, wxID_ANY);
    auto* overviewSizer = new wxBoxSizer(wxVERTICAL);
    overview_text_ = new wxTextCtrl(overviewPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    overviewSizer->Add(overview_text_, 1, wxEXPAND | wxALL, 8);
    overviewPanel->SetSizer(overviewSizer);

    auto* ddlPanel = new wxPanel(inspector_notebook_, wxID_ANY);
    auto* ddlSizer = new wxBoxSizer(wxVERTICAL);
    ddl_text_ = new wxTextCtrl(ddlPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    ddlSizer->Add(ddl_text_, 1, wxEXPAND | wxALL, 8);
    ddlPanel->SetSizer(ddlSizer);

    auto* depsPanel = new wxPanel(inspector_notebook_, wxID_ANY);
    auto* depsSizer = new wxBoxSizer(wxVERTICAL);
    deps_text_ = new wxTextCtrl(depsPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    depsSizer->Add(deps_text_, 1, wxEXPAND | wxALL, 8);
    depsPanel->SetSizer(depsSizer);

    inspector_notebook_->AddPage(overviewPanel, "Overview");
    inspector_notebook_->AddPage(ddlPanel, "DDL");
    inspector_notebook_->AddPage(depsPanel, "Dependencies");

    detailsSizer->Add(inspector_notebook_, 1, wxEXPAND | wxALL, 4);
    detailsPanel->SetSizer(detailsSizer);

    splitter->SplitVertically(treePanel, detailsPanel, 320);

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(splitter, 1, wxEXPAND);
    SetSizer(rootSizer);
}

void MainFrame::OnNewSqlEditor(wxCommandEvent&) {
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_, app_config_);
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

void MainFrame::OnTreeSelection(wxTreeEvent& event) {
    if (!tree_) {
        return;
    }
    auto item = event.GetItem();
    if (!item.IsOk()) {
        return;
    }

    auto* data = dynamic_cast<MetadataNodeData*>(tree_->GetItemData(item));
    if (!data) {
        UpdateInspector(nullptr);
        return;
    }
    UpdateInspector(data->GetNode());
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
        wxTreeItemId id = tree_->AppendItem(parent, node.label, -1, -1,
                                            new MetadataNodeData(&node));
        for (const auto& child : node.children) {
            addNode(id, child);
        }
    };

    for (const auto& node : snapshot.roots) {
        addNode(root, node);
    }

    tree_->Expand(root);
    tree_->Thaw();

    UpdateInspector(nullptr);
}

void MainFrame::UpdateInspector(const MetadataNode* node) {
    if (overview_text_) {
        if (!node) {
            overview_text_->SetValue("Select a catalog object to view details.");
        } else {
            std::string text = "Name: " + node->label + "\n";
            if (!node->kind.empty()) {
                text += "Type: " + node->kind + "\n";
            }
            if (!node->children.empty()) {
                text += "Children: " + std::to_string(node->children.size()) + "\n";
            }
            overview_text_->SetValue(text);
        }
    }

    if (ddl_text_) {
        if (!node || node->ddl.empty()) {
            ddl_text_->SetValue("DDL extract not available for this selection.");
        } else {
            ddl_text_->SetValue(node->ddl);
        }
    }

    if (deps_text_) {
        if (!node || node->dependencies.empty()) {
            deps_text_->SetValue("No dependencies recorded for this selection.");
        } else {
            std::string text;
            for (const auto& dep : node->dependencies) {
                text += "- " + dep + "\n";
            }
            deps_text_->SetValue(text);
        }
    }
}

} // namespace scratchrobin
