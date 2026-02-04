/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "main_frame.h"
#include "backup_dialog.h"
#include "backup_history_dialog.h"
#include "backup_schedule_dialog.h"
#include "cluster_manager_frame.h"
#include "connection_editor_dialog.h"
#include "database_manager_frame.h"
#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "etl_manager_frame.h"
#include "git_integration_frame.h"
#include "help_browser.h"
#include "icon_bar.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "package_manager_frame.h"
#include "preferences_dialog.h"
#include "procedure_manager_frame.h"
#include "replication_manager_frame.h"
#include "restore_dialog.h"
#include "schema_manager_frame.h"
#include "sequence_manager_frame.h"
#include "shortcuts_cheat_sheet.h"
#include "shortcuts_dialog.h"
#include "sql_editor_frame.h"
#include "storage_manager_frame.h"
#include "table_designer_frame.h"
#include "trigger_manager_frame.h"
#include "users_roles_frame.h"
#include "view_manager_frame.h"
#include "window_manager.h"

#include "core/session_state.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

namespace {
constexpr int kFilterTextCtrl = wxID_HIGHEST + 2;
constexpr int kFilterClearButton = wxID_HIGHEST + 3;
constexpr int kMenuTreeOpenEditor = wxID_HIGHEST + 4;
constexpr int kMenuTreeCopyName = wxID_HIGHEST + 5;
constexpr int kMenuTreeCopyDdl = wxID_HIGHEST + 6;
constexpr int kMenuTreeShowDeps = wxID_HIGHEST + 7;
constexpr int kMenuTreeRefresh = wxID_HIGHEST + 8;

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

class MetadataNodeData : public wxTreeItemData {
public:
    explicit MetadataNodeData(const MetadataNode* node) : node_(node) {}

    const MetadataNode* GetNode() const { return node_; }

private:
    const MetadataNode* node_ = nullptr;
};
} // namespace

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, MainFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, MainFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, MainFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, MainFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, MainFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, MainFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, MainFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, MainFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, MainFrame::OnOpenIndexDesigner)
    EVT_MENU(ID_MENU_SEQUENCE_MANAGER, MainFrame::OnOpenSequenceManager)
    EVT_MENU(ID_MENU_VIEW_MANAGER, MainFrame::OnOpenViewManager)
    EVT_MENU(ID_MENU_TRIGGER_MANAGER, MainFrame::OnOpenTriggerManager)
    EVT_MENU(ID_MENU_PROCEDURE_MANAGER, MainFrame::OnOpenProcedureManager)
    EVT_MENU(ID_MENU_PACKAGE_MANAGER, MainFrame::OnOpenPackageManager)
    EVT_MENU(ID_MENU_STORAGE_MANAGER, MainFrame::OnOpenStorageManager)
    EVT_MENU(ID_MENU_DATABASE_MANAGER, MainFrame::OnOpenDatabaseManager)
    EVT_MENU(ID_MENU_BACKUP, MainFrame::OnBackup)
    EVT_MENU(ID_MENU_RESTORE, MainFrame::OnRestore)
    EVT_MENU(ID_MENU_BACKUP_HISTORY, MainFrame::OnBackupHistory)
    EVT_MENU(ID_MENU_BACKUP_SCHEDULE, MainFrame::OnBackupSchedule)
    EVT_MENU(ID_MENU_PREFERENCES, MainFrame::OnPreferences)
    EVT_MENU(ID_MENU_CLUSTER_MANAGER, MainFrame::OnOpenClusterManager)
    EVT_MENU(ID_MENU_REPLICATION_MANAGER, MainFrame::OnOpenReplicationManager)
    EVT_MENU(ID_MENU_ETL_MANAGER, MainFrame::OnOpenEtlManager)
    EVT_MENU(ID_MENU_GIT_INTEGRATION, MainFrame::OnOpenGitIntegration)
    EVT_MENU(ID_MENU_SHORTCUTS, MainFrame::OnShortcuts)
    EVT_MENU(ID_MENU_CHEAT_SHEET, MainFrame::OnShortcutsCheatSheet)
    EVT_MENU(ID_MENU_HELP_WINDOW, MainFrame::OnHelp)
    EVT_MENU(ID_MENU_HELP_COMMAND, MainFrame::OnHelp)
    EVT_MENU(ID_MENU_HELP_LANGUAGE, MainFrame::OnHelpLanguage)
    EVT_MENU(ID_CONN_MANAGE, MainFrame::OnManageConnections)
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_TREE_SEL_CHANGED(wxID_ANY, MainFrame::OnTreeSelection)
    EVT_TREE_ITEM_MENU(wxID_ANY, MainFrame::OnTreeItemMenu)
    EVT_TEXT(kFilterTextCtrl, MainFrame::OnFilterChanged)
    EVT_BUTTON(kFilterClearButton, MainFrame::OnFilterClear)
    EVT_MENU(kMenuTreeOpenEditor, MainFrame::OnTreeOpenEditor)
    EVT_MENU(kMenuTreeCopyName, MainFrame::OnTreeCopyName)
    EVT_MENU(kMenuTreeCopyDdl, MainFrame::OnTreeCopyDdl)
    EVT_MENU(kMenuTreeShowDeps, MainFrame::OnTreeShowDependencies)
    EVT_MENU(kMenuTreeRefresh, MainFrame::OnTreeRefresh)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(WindowManager* windowManager,
                     MetadataModel* metadataModel,
                     ConnectionManager* connectionManager,
                     std::vector<ConnectionProfile>* connections,
                     const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "ScratchRobin", wxDefaultPosition, wxSize(1024, 768)),
      window_manager_(windowManager),
      metadata_model_(metadataModel),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    BuildMenu();
    if (app_config_ && app_config_->chrome.mainWindow.showIconBar) {
        BuildIconBar(this, IconBarType::Main, 24);
    }
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
    WindowChromeConfig chrome;
    if (app_config_) {
        chrome = app_config_->chrome.mainWindow;
    }

    if (!chrome.showMenu) {
        return;
    }

    MenuBuildOptions options;
    options.includeConnections = true;
    options.includeEdit = true;
    options.includeView = true;
    options.includeWindow = true;
    options.includeHelp = true;
    auto* menu_bar = BuildMenuBar(options, window_manager_, this);
    SetMenuBar(menu_bar);
}

void MainFrame::BuildLayout() {
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* treePanel = new wxPanel(splitter, wxID_ANY);
    auto* treeSizer = new wxBoxSizer(wxVERTICAL);
    auto* filterSizer = new wxBoxSizer(wxHORIZONTAL);
    filterSizer->Add(new wxStaticText(treePanel, wxID_ANY, "Filter:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    filter_ctrl_ = new wxTextCtrl(treePanel, kFilterTextCtrl);
    filter_ctrl_->SetHint("Filter catalog objects");
    filterSizer->Add(filter_ctrl_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    filter_clear_button_ = new wxButton(treePanel, kFilterClearButton, "Clear");
    filterSizer->Add(filter_clear_button_, 0, wxALIGN_CENTER_VERTICAL);
    treeSizer->Add(filterSizer, 0, wxEXPAND | wxALL, 8);
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

    overview_page_index_ = inspector_notebook_->GetPageCount();
    inspector_notebook_->AddPage(overviewPanel, "Overview");
    ddl_page_index_ = inspector_notebook_->GetPageCount();
    inspector_notebook_->AddPage(ddlPanel, "DDL");
    deps_page_index_ = inspector_notebook_->GetPageCount();
    inspector_notebook_->AddPage(depsPanel, "Dependencies");

    detailsSizer->Add(inspector_notebook_, 1, wxEXPAND | wxALL, 4);
    detailsPanel->SetSizer(detailsSizer);

    splitter->SplitVertically(treePanel, detailsPanel, 320);

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(splitter, 1, wxEXPAND);
    SetSizer(rootSizer);
}

void MainFrame::OnNewSqlEditor(wxCommandEvent&) {
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, metadata_model_);
    editor->Show(true);
}

void MainFrame::OnNewDiagram(wxCommandEvent&) {
    if (window_manager_) {
        if (auto* host = dynamic_cast<DiagramFrame*>(window_manager_->GetDiagramHost())) {
            host->AddDiagramTab();
            host->Raise();
            host->Show(true);
            return;
        }
    }
    auto* diagram = new DiagramFrame(window_manager_, app_config_);
    diagram->Show(true);
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
        context_node_ = nullptr;
        UpdateInspector(nullptr);
        return;
    }
    context_node_ = data->GetNode();
    UpdateInspector(context_node_);
}

void MainFrame::PopulateTree(const MetadataSnapshot& snapshot) {
    if (!tree_) {
        return;
    }

    std::string filter = ToLowerCopy(Trim(filter_text_));
    bool has_filter = !filter.empty();

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
            if (HasMatch(child, filter)) {
                addNode(id, child);
            }
        }
    };

    bool added_any = false;
    for (const auto& node : snapshot.roots) {
        if (HasMatch(node, filter)) {
            addNode(root, node);
            added_any = true;
        }
    }

    if (!added_any) {
        tree_->AppendItem(root, has_filter ? "No matches for filter" : "No metadata available");
        tree_->Expand(root);
        tree_->Thaw();
        UpdateInspector(nullptr);
        return;
    }

    if (has_filter) {
        tree_->ExpandAll();
    } else {
        tree_->Expand(root);
    }
    tree_->Thaw();

    UpdateInspector(nullptr);
}

void MainFrame::UpdateInspector(const MetadataNode* node) {
    if (overview_text_) {
        if (!node) {
            overview_text_->SetValue("Select a catalog object to view details.");
        } else {
            std::string text = "Name: " + node->label + "\n";
            if (!node->catalog.empty()) {
                text += "Catalog: " + node->catalog + "\n";
            }
            if (!node->kind.empty()) {
                text += "Type: " + node->kind + "\n";
            }
            if (!node->path.empty()) {
                text += "Path: " + node->path + "\n";
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

const MetadataNode* MainFrame::GetSelectedNode() const {
    if (!tree_) {
        return nullptr;
    }
    auto item = tree_->GetSelection();
    if (!item.IsOk()) {
        return nullptr;
    }
    auto* data = dynamic_cast<MetadataNodeData*>(tree_->GetItemData(item));
    return data ? data->GetNode() : nullptr;
}

bool MainFrame::HasMatch(const MetadataNode& node, const std::string& filter) const {
    if (filter.empty()) {
        return true;
    }

    auto matches = [&](const std::string& value) {
        if (value.empty()) {
            return false;
        }
        return ToLowerCopy(value).find(filter) != std::string::npos;
    };

    if (matches(node.label) || matches(node.kind) || matches(node.catalog) || matches(node.path)) {
        return true;
    }

    for (const auto& child : node.children) {
        if (HasMatch(child, filter)) {
            return true;
        }
    }

    return false;
}

std::string MainFrame::BuildSeedSql(const MetadataNode* node) const {
    if (!node) {
        return {};
    }
    if (!node->ddl.empty()) {
        return node->ddl;
    }
    std::string label = node->path.empty() ? node->label : node->path;
    if (label.empty()) {
        return {};
    }
    return "-- " + label + "\n";
}

bool MainFrame::CopyToClipboard(const std::string& text) const {
    if (text.empty()) {
        return false;
    }
    if (wxTheClipboard && wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(text)));
        wxTheClipboard->Close();
        return true;
    }
    return false;
}

void MainFrame::OnTreeItemMenu(wxTreeEvent& event) {
    if (!tree_) {
        return;
    }
    auto item = event.GetItem();
    if (!item.IsOk()) {
        return;
    }
    tree_->SelectItem(item);
    auto* data = dynamic_cast<MetadataNodeData*>(tree_->GetItemData(item));
    context_node_ = data ? data->GetNode() : nullptr;
    UpdateInspector(context_node_);

    wxMenu menu;
    menu.Append(kMenuTreeOpenEditor, "Open in SQL Editor");
    menu.AppendSeparator();
    menu.Append(kMenuTreeCopyName, "Copy Name");
    menu.Append(kMenuTreeCopyDdl, "Copy DDL");
    menu.Append(kMenuTreeShowDeps, "Show Dependencies");
    menu.AppendSeparator();
    menu.Append(kMenuTreeRefresh, "Refresh Metadata");

    if (!context_node_ || context_node_->ddl.empty()) {
        menu.Enable(kMenuTreeCopyDdl, false);
    }
    if (!context_node_ || context_node_->dependencies.empty()) {
        menu.Enable(kMenuTreeShowDeps, false);
    }
    if (!metadata_model_) {
        menu.Enable(kMenuTreeRefresh, false);
    }

    PopupMenu(&menu);
}

void MainFrame::OnFilterChanged(wxCommandEvent&) {
    if (!filter_ctrl_) {
        return;
    }
    filter_text_ = filter_ctrl_->GetValue().ToStdString();
    if (metadata_model_) {
        PopulateTree(metadata_model_->GetSnapshot());
    }
}

void MainFrame::OnFilterClear(wxCommandEvent&) {
    if (filter_ctrl_) {
        filter_ctrl_->Clear();
    }
    filter_text_.clear();
    if (metadata_model_) {
        PopulateTree(metadata_model_->GetSnapshot());
    }
}

void MainFrame::OnTreeOpenEditor(wxCommandEvent&) {
    const MetadataNode* node = context_node_ ? context_node_ : GetSelectedNode();
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, metadata_model_);
    std::string seed = BuildSeedSql(node);
    if (!seed.empty()) {
        editor->LoadStatement(seed);
    }
    editor->Show(true);
}

void MainFrame::OnOpenMonitoring(wxCommandEvent&) {
    auto* frame = new MonitoringFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenUsersRoles(wxCommandEvent&) {
    auto* frame = new UsersRolesFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenJobScheduler(wxCommandEvent&) {
    auto* frame = new JobSchedulerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenDomainManager(wxCommandEvent&) {
    auto* frame = new DomainManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenSchemaManager(wxCommandEvent&) {
    auto* frame = new SchemaManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenTableDesigner(wxCommandEvent&) {
    auto* frame = new TableDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    auto* frame = new IndexDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenSequenceManager(wxCommandEvent&) {
    auto* frame = new SequenceManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenViewManager(wxCommandEvent&) {
    auto* frame = new ViewManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenTriggerManager(wxCommandEvent&) {
    auto* frame = new TriggerManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenProcedureManager(wxCommandEvent&) {
    auto* frame = new ProcedureManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenPackageManager(wxCommandEvent&) {
    auto* frame = new PackageManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenStorageManager(wxCommandEvent&) {
    auto* frame = new StorageManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnOpenDatabaseManager(wxCommandEvent&) {
    auto* frame = new DatabaseManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnBackup(wxCommandEvent&) {
    BackupDialog dialog(this, connections_, "");
    dialog.ShowModal();
}

void MainFrame::OnRestore(wxCommandEvent&) {
    RestoreDialog dialog(this, connections_);
    dialog.ShowModal();
}

void MainFrame::OnBackupHistory(wxCommandEvent&) {
    BackupHistoryDialog dialog(this, connection_manager_, connections_);
    dialog.ShowModal();
}

void MainFrame::OnBackupSchedule(wxCommandEvent&) {
    BackupScheduleDialog dialog(this, connections_);
    dialog.ShowModal();
}

void MainFrame::OnPreferences(wxCommandEvent&) {
    PreferencesDialog dialog(this, preferences_);
    if (dialog.ShowModal() == wxID_OK) {
        PreferencesDialog::SavePreferences(preferences_);
    }
}

void MainFrame::OnShortcuts(wxCommandEvent&) {
    ShowShortcutsDialog(this);
}

void MainFrame::OnShortcutsCheatSheet(wxCommandEvent&) {
    ShowShortcutsCheatSheet(this);
}

void MainFrame::OnHelp(wxCommandEvent&) {
    HelpBrowser::ShowHelp(HelpTopicId::GettingStarted);
}

void MainFrame::OnHelpLanguage(wxCommandEvent&) {
    HelpBrowser::ShowHelp(HelpTopicId::Functions);
}

void MainFrame::OnTreeCopyName(wxCommandEvent&) {
    const MetadataNode* node = context_node_ ? context_node_ : GetSelectedNode();
    if (!node) {
        return;
    }
    std::string text = node->path.empty() ? node->label : node->path;
    if (CopyToClipboard(text)) {
        SetStatusText("Copied name to clipboard");
    } else {
        SetStatusText("Unable to access clipboard");
    }
}

void MainFrame::OnTreeCopyDdl(wxCommandEvent&) {
    const MetadataNode* node = context_node_ ? context_node_ : GetSelectedNode();
    if (!node || node->ddl.empty()) {
        SetStatusText("DDL not available for selection");
        return;
    }
    if (CopyToClipboard(node->ddl)) {
        SetStatusText("Copied DDL to clipboard");
    } else {
        SetStatusText("Unable to access clipboard");
    }
}

void MainFrame::OnTreeShowDependencies(wxCommandEvent&) {
    const MetadataNode* node = context_node_ ? context_node_ : GetSelectedNode();
    if (!node || node->dependencies.empty()) {
        SetStatusText("No dependencies to display");
        return;
    }
    UpdateInspector(node);
    if (inspector_notebook_) {
        inspector_notebook_->SetSelection(deps_page_index_);
    }
}

void MainFrame::OnTreeRefresh(wxCommandEvent&) {
    if (!metadata_model_) {
        return;
    }
    metadata_model_->Refresh();
    if (!metadata_model_->LastError().empty()) {
        SetStatusText(metadata_model_->LastError());
    } else {
        SetStatusText("Metadata refreshed");
    }
}

void MainFrame::OnManageConnections(wxCommandEvent&) {
    if (!connections_) {
        return;
    }
    
    // Make a copy of connections to edit
    std::vector<ConnectionProfile> editableConnections = *connections_;
    
    ConnectionManagerDialog dialog(this, &editableConnections);
    if (dialog.ShowModal() == wxID_OK) {
        // Update the original connections
        // Note: In a real implementation, this would also save to config file
        *connections_ = editableConnections;
        
        // Refresh metadata model if available
        if (metadata_model_) {
            metadata_model_->Refresh();
        }
        
        SetStatusText("Connections updated");
    }
}

// =============================================================================
// Beta Placeholder Handlers (Phase 7)
// =============================================================================

void MainFrame::OnOpenClusterManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* frame = new ClusterManagerFrame(window_manager_, connection_manager_, 
                                          connections_, app_config_);
    window_manager_->RegisterWindow(frame);
    frame->Show(true);
    SetStatusText("Cluster Manager (Beta Preview) opened");
}

void MainFrame::OnOpenReplicationManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* frame = new ReplicationManagerFrame(window_manager_, connection_manager_, 
                                              connections_, app_config_);
    window_manager_->RegisterWindow(frame);
    frame->Show(true);
    SetStatusText("Replication Manager (Beta Preview) opened");
}

void MainFrame::OnOpenEtlManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* frame = new EtlManagerFrame(window_manager_, connection_manager_, 
                                      connections_, app_config_);
    window_manager_->RegisterWindow(frame);
    frame->Show(true);
    SetStatusText("ETL Manager (Beta Preview) opened");
}

void MainFrame::OnOpenGitIntegration(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* frame = new GitIntegrationFrame(window_manager_, connection_manager_, 
                                          connections_, app_config_);
    window_manager_->RegisterWindow(frame);
    frame->Show(true);
    SetStatusText("Git Integration (Beta Preview) opened");
}

} // namespace scratchrobin
