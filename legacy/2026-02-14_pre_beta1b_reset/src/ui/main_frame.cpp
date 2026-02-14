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
#include "connection_database_manager.h"
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
#include "toolbar/toolbar_editor_form.h"
#include "toolbar/toolbar_manager.h"
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

#include <wx/aui/aui.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
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
#include <wx/artprov.h>
#include <wx/filename.h>

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
    EVT_MENU(ID_MENU_NEW_ERD_DIAGRAM, MainFrame::OnNewERDDiagram)
    EVT_MENU(ID_MENU_NEW_DFD_DIAGRAM, MainFrame::OnNewDFDDiagram)
    EVT_MENU(ID_MENU_NEW_UML_DIAGRAM, MainFrame::OnNewUMLDiagram)
    EVT_MENU(ID_MENU_NEW_MINDMAP, MainFrame::OnNewMindMap)
    EVT_MENU(ID_MENU_NEW_WHITEBOARD, MainFrame::OnNewWhiteboard)
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
    EVT_MENU(ID_MENU_CUSTOMIZE_TOOLBARS, MainFrame::OnCustomizeToolbars)
    EVT_MENU(ID_MENU_HELP_WINDOW, MainFrame::OnHelp)
    EVT_MENU(ID_MENU_HELP_COMMAND, MainFrame::OnHelp)
    EVT_MENU(ID_MENU_HELP_LANGUAGE, MainFrame::OnHelpLanguage)
    EVT_MENU(ID_CONN_MANAGE, MainFrame::OnManageConnections)
    EVT_MENU(ID_CONN_SERVER_CREATE, MainFrame::OnServerCreate)
    EVT_MENU(ID_CONN_SERVER_CONNECT, MainFrame::OnServerConnect)
    EVT_MENU(ID_CONN_SERVER_DISCONNECT, MainFrame::OnServerDisconnect)
    EVT_MENU(ID_CONN_SERVER_DROP, MainFrame::OnServerDrop)
    EVT_MENU(ID_CONN_SERVER_REMOVE, MainFrame::OnServerRemove)
    EVT_MENU(ID_CONN_CLUSTER_CREATE, MainFrame::OnClusterCreate)
    EVT_MENU(ID_CONN_CLUSTER_CONNECT, MainFrame::OnClusterConnect)
    EVT_MENU(ID_CONN_CLUSTER_DISCONNECT, MainFrame::OnClusterDisconnect)
    EVT_MENU(ID_CONN_CLUSTER_DROP, MainFrame::OnClusterDrop)
    EVT_MENU(ID_CONN_CLUSTER_REMOVE, MainFrame::OnClusterRemove)
    EVT_MENU(ID_CONN_DATABASE_CREATE, MainFrame::OnDatabaseCreate)
    EVT_MENU(ID_CONN_DATABASE_CONNECT, MainFrame::OnDatabaseConnect)
    EVT_MENU(ID_CONN_DATABASE_DISCONNECT, MainFrame::OnDatabaseDisconnect)
    EVT_MENU(ID_CONN_DATABASE_DROP, MainFrame::OnDatabaseDrop)
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
    : wxFrame(nullptr, wxID_ANY, "ScratchRobin", wxDefaultPosition, wxSize(1280, 900)),
      window_manager_(windowManager),
      metadata_model_(metadataModel),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig),
      layout_manager_(std::make_unique<LayoutManager>(this)) {
    BuildMenu();
    if (app_config_ && app_config_->chrome.mainWindow.showIconBar) {
        BuildIconBar(this, IconBarType::Main, 24);
    }
    BuildLayout();
    SetupLayoutManager();
    CreateStatusBar();
    SetStatusText("Ready");

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
    if (metadata_model_) {
        metadata_model_->AddObserver(this);
        PopulateTree(metadata_model_->GetSnapshot());
    }
    
    // Try to restore auto-saved layout, otherwise apply default
    if (layout_manager_) {
        layout_manager_->RestoreState();
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
    
    // Add Layout menu
    wxMenu* layoutMenu = new wxMenu();
    layoutMenu->Append(wxID_HIGHEST + 100, "&Default\tCtrl+Shift+1", "Default layout");
    layoutMenu->Append(wxID_HIGHEST + 101, "&Single Monitor\tCtrl+Shift+2", "Single monitor optimized");
    layoutMenu->Append(wxID_HIGHEST + 102, "&Dual Monitor\tCtrl+Shift+3", "Dual monitor optimized");
    layoutMenu->Append(wxID_HIGHEST + 103, "&Wide Screen\tCtrl+Shift+4", "Ultrawide optimized");
    layoutMenu->Append(wxID_HIGHEST + 104, "&Compact\tCtrl+Shift+5", "Minimal layout");
    layoutMenu->AppendSeparator();
    layoutMenu->Append(wxID_HIGHEST + 105, "&Save Current Layout...", "Save current layout as preset");
    layoutMenu->Append(wxID_HIGHEST + 106, "&Manage Layouts...", "Manage layout presets");
    
    Bind(wxEVT_MENU, &MainFrame::OnLayoutDefault, this, wxID_HIGHEST + 100);
    Bind(wxEVT_MENU, &MainFrame::OnLayoutSingleMonitor, this, wxID_HIGHEST + 101);
    Bind(wxEVT_MENU, &MainFrame::OnLayoutDualMonitor, this, wxID_HIGHEST + 102);
    Bind(wxEVT_MENU, &MainFrame::OnLayoutWideScreen, this, wxID_HIGHEST + 103);
    Bind(wxEVT_MENU, &MainFrame::OnLayoutCompact, this, wxID_HIGHEST + 104);
    Bind(wxEVT_MENU, &MainFrame::OnLayoutSaveCurrent, this, wxID_HIGHEST + 105);
    Bind(wxEVT_MENU, &MainFrame::OnLayoutManage, this, wxID_HIGHEST + 106);
    
    // Insert Layout menu before Help
    int helpIndex = -1;
    for (size_t i = 0; i < menu_bar->GetMenuCount(); ++i) {
        if (menu_bar->GetMenuLabelText(i) == "&Help") {
            helpIndex = static_cast<int>(i);
            break;
        }
    }
    if (helpIndex >= 0) {
        menu_bar->Insert(helpIndex, layoutMenu, "&Layout");
    } else {
        menu_bar->Append(layoutMenu, "&Layout");
    }
    
    SetMenuBar(menu_bar);
}

void MainFrame::BuildLayout() {
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetMinimumPaneSize(200);  // Prevent tree panel from disappearing

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
    
    InitializeTreeIcons();

    auto* detailsPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailsSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create FormContainer for inspector (replaces wxNotebook)
    ui::FormContainer::Config containerConfig;
    containerConfig.containerId = "inspector";
    containerConfig.acceptedCategory = ui::FormCategory::Unknown;  // Accept any
    containerConfig.allowMultipleForms = true;
    containerConfig.showCloseButtons = false;  // Inspector tabs shouldn't close
    containerConfig.defaultTitle = "Inspector";
    
    inspector_container_ = new ui::FormContainer(detailsPanel, containerConfig);
    
    // Create panels for Overview, DDL, Dependencies
    auto* overviewPanel = new wxPanel(inspector_container_, wxID_ANY);
    auto* overviewSizer = new wxBoxSizer(wxVERTICAL);
    overview_text_ = new wxTextCtrl(overviewPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    overviewSizer->Add(overview_text_, 1, wxEXPAND | wxALL, 8);
    overviewPanel->SetSizer(overviewSizer);

    auto* ddlPanel = new wxPanel(inspector_container_, wxID_ANY);
    auto* ddlSizer = new wxBoxSizer(wxVERTICAL);
    ddl_text_ = new wxTextCtrl(ddlPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    ddlSizer->Add(ddl_text_, 1, wxEXPAND | wxALL, 8);
    ddlPanel->SetSizer(ddlSizer);

    auto* depsPanel = new wxPanel(inspector_container_, wxID_ANY);
    auto* depsSizer = new wxBoxSizer(wxVERTICAL);
    deps_text_ = new wxTextCtrl(depsPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    depsSizer->Add(deps_text_, 1, wxEXPAND | wxALL, 8);
    depsPanel->SetSizer(depsSizer);

    // Add panels to container
    inspector_container_->AddWindow(overviewPanel, "Overview", "overview");
    inspector_container_->AddWindow(ddlPanel, "DDL", "ddl");
    inspector_container_->AddWindow(depsPanel, "Dependencies", "dependencies");

    detailsSizer->Add(inspector_container_, 1, wxEXPAND | wxALL, 4);
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
    OnNewDiagramOfType(DiagramType::Erd);
}

void MainFrame::OnNewERDDiagram(wxCommandEvent&) {
    OnNewDiagramOfType(DiagramType::Erd);
}

void MainFrame::OnNewDFDDiagram(wxCommandEvent&) {
    OnNewDiagramOfType(DiagramType::DataFlow);
}

void MainFrame::OnNewUMLDiagram(wxCommandEvent&) {
    OnNewDiagramOfType(DiagramType::Erd);  // UML uses ERD base
}

void MainFrame::OnNewMindMap(wxCommandEvent&) {
    OnNewDiagramOfType(DiagramType::MindMap);
}

void MainFrame::OnNewWhiteboard(wxCommandEvent&) {
    OnNewDiagramOfType(DiagramType::Whiteboard);
}

void MainFrame::OnNewDiagramOfType(DiagramType type) {
    if (window_manager_) {
        if (auto* host = dynamic_cast<DiagramFrame*>(window_manager_->GetDiagramHost())) {
            host->AddDiagramTabOfType(type);
            host->Raise();
            host->Show(true);
            return;
        }
    }
    auto* diagram = new DiagramFrame(window_manager_, app_config_);
    diagram->SetDefaultDiagramType(type);
    diagram->Show(true);
}

void MainFrame::OnQuit(wxCommandEvent&) {
    Close(true);
}

void MainFrame::OnClose(wxCloseEvent& event) {
    // Close all child windows first
    if (window_manager_) {
        window_manager_->CloseAll();
    }
    
    // Shutdown toolbar manager
    ui::toolbar::ToolbarManager::GetInstance().Shutdown();
    
    // Layout state is auto-saved in destructor, but we can trigger it explicitly
    if (layout_manager_) {
        layout_manager_->SaveState();
    }
    
    if (metadata_model_) {
        metadata_model_->RemoveObserver(this);
    }
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    
    event.Skip();
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

void MainFrame::InitializeTreeIcons() {
    if (!tree_) {
        return;
    }
    
    // Create image list for 16x16 icons
    tree_images_ = new wxImageList(16, 16);
    
    // Helper lambda to load icon from resources
    auto loadIcon = [&](const wxString& name, const char* fallbackArtId) -> int {
        wxString iconPath = wxString::Format("resources/icons/%s@16.png", name);
        if (wxFileName::FileExists(iconPath)) {
            wxImage image(iconPath, wxBITMAP_TYPE_PNG);
            if (image.IsOk()) {
                if (image.GetWidth() != 16 || image.GetHeight() != 16) {
                    image = image.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
                }
                return tree_images_->Add(wxBitmap(image));
            }
        }
        // Fallback to wxArtProvider
        wxBitmap fallback = wxArtProvider::GetBitmap(fallbackArtId, wxART_OTHER, wxSize(16, 16));
        if (fallback.IsOk()) {
            return tree_images_->Add(fallback);
        }
        return -1;
    };
    
    // =========================================================================
    // COMPREHENSIVE ICON INDEX DEFINITIONS
    // Must match GetIconIndexForNode() exactly
    // =========================================================================
    //
    // 000-009: Application & Navigation
    // 010-024: Database Objects (Tables, Views, Programmability)
    // 025-029: Schema Organization
    // 030-034: Security & Admin
    // 035-044: Project Objects
    // 045-049: Version Control
    // 050-059: Maintenance & Operations
    // 060-069: Infrastructure (Servers, Networks, Clusters)
    // 070-079: Design & Planning (Whiteboards, Mindmaps, Drafts)
    // 080-089: Design States (Implemented, Pending, Modified, etc)
    // 090-099: Synchronization (Sync, Diff, Deploy, Migrate)
    // 100-109: Collaboration (Shared, Team)
    // 110-119: Security & Audit (Lock, History)
    // 120:    Default/Unknown
    //
    // Total: 121 icon slots (0-120)
    // =========================================================================
    
    // == Application & Navigation (0-9) ==
    loadIcon("scratchrobin", wxART_HELP);             //  0 - root
    loadIcon("connect", wxART_HARDDISK);              //  1 - connection
    loadIcon("settings", wxART_HELP_SETTINGS);        //  2 - settings/config
    loadIcon("stop", wxART_ERROR);                    //  3 - error
    loadIcon("diagram", wxART_REPORT_VIEW);           //  4 - diagram/erd
    loadIcon("bookmark", wxART_ADD_BOOKMARK);         //  5 - bookmark/favorite
    loadIcon("tag", wxART_FIND);                      //  6 - tag/label
    
    // == Database Objects (10-24) ==
    loadIcon("table", wxART_NORMAL_FILE);             // 10 - table
    loadIcon("view", wxART_LIST_VIEW);                // 11 - view
    loadIcon("column", wxART_HELP_BOOK);              // 12 - column
    loadIcon("index", wxART_TIP);                     // 13 - index
    loadIcon("sequence", wxART_ADD_BOOKMARK);         // 14 - sequence
    loadIcon("trigger", wxART_WARNING);               // 15 - trigger
    loadIcon("constraint", wxART_TICK_MARK);          // 16 - constraint
    loadIcon("procedure", wxART_EXECUTABLE_FILE);     // 17 - procedure
    loadIcon("function", wxART_EDIT);                 // 18 - function
    loadIcon("package", wxART_NEW);                   // 19 - package
    loadIcon("domain", wxART_CROSS_MARK);             // 20 - domain
    loadIcon("collation", wxART_FIND);                // 21 - collation
    loadIcon("tablespace", wxART_CDROM);              // 22 - tablespace
    
    // == Schema Organization (25-29) ==
    loadIcon("database", wxART_REPORT_VIEW);          // 25 - database/catalog
    loadIcon("catalog", wxART_GO_HOME);               // 26 - catalog
    loadIcon("schema", wxART_FOLDER);                 // 27 - schema
    loadIcon("folder", wxART_FOLDER_OPEN);            // 28 - folder/group
    
    // == Security & Admin (30-34) ==
    loadIcon("users", wxART_NEW);                     // 30 - user/role
    loadIcon("host", wxART_GO_TO_PARENT);             // 31 - host/server
    loadIcon("permission", wxART_TICK_MARK);          // 32 - permission/grant
    loadIcon("audit", wxART_FIND_AND_REPLACE);        // 33 - audit
    loadIcon("history", wxART_GOTO_LAST);             // 34 - history
    
    // == Project Objects (35-44) ==
    loadIcon("project", wxART_HELP_PAGE);             // 35 - project/workspace
    loadIcon("sql", wxART_EDIT);                      // 36 - sql/script/query
    loadIcon("note", wxART_HELP_PAGE);                // 37 - note/documentation
    loadIcon("timeline", wxART_GOTO_FIRST);           // 38 - timeline/workflow
    loadIcon("job", wxART_REFRESH);                   // 39 - job/scheduled task
    
    // == Version Control (45-49) ==
    loadIcon("git", wxART_COPY);                      // 45 - git repository
    loadIcon("branch", wxART_GO_DOWN);                // 46 - git branch
    
    // == Maintenance & Operations (50-54) ==
    loadIcon("backup", wxART_FILE_SAVE);              // 50 - backup
    loadIcon("restore", wxART_FILE_OPEN);             // 51 - restore
    
    // == Infrastructure (60-69) ==
    loadIcon("server", wxART_HARDDISK);               // 60 - server
    loadIcon("client", wxART_NORMAL_FILE);            // 61 - client
    loadIcon("filespace", wxART_FOLDER_OPEN);         // 62 - filespace/storage
    loadIcon("network", wxART_GO_TO_PARENT);          // 63 - network
    loadIcon("cluster", wxART_CDROM);                 // 64 - cluster
    loadIcon("instance", wxART_EXECUTABLE_FILE);      // 65 - database instance
    loadIcon("replica", wxART_COPY);                  // 66 - replica/slave
    loadIcon("shard", wxART_PASTE);                   // 67 - shard/partition
    
    // == Design & Planning (70-79) ==
    loadIcon("whiteboard", wxART_HELP_BROWSER);       // 70 - whiteboard
    loadIcon("mindmap", wxART_LIST_VIEW);              // 71 - mindmap
    loadIcon("design", wxART_EDIT);                   // 72 - design
    loadIcon("draft", wxART_HELP_SETTINGS);           // 73 - draft/concept
    loadIcon("template", wxART_NEW_DIR);              // 74 - template
    loadIcon("blueprint", wxART_REPORT_VIEW);         // 75 - blueprint/plan
    loadIcon("concept", wxART_TIP);                   // 76 - concept/idea
    loadIcon("plan", wxART_LIST_VIEW);                // 77 - implementation plan
    
    // == Design States (80-89) ==
    loadIcon("implemented", wxART_TICK_MARK);         // 80 - implemented/deployed
    loadIcon("pending", wxART_WARNING);               // 81 - pending/staged
    loadIcon("modified", wxART_EDIT);                 // 82 - modified/changed
    loadIcon("deleted", wxART_DELETE);                // 83 - deleted/removed
    loadIcon("newitem", wxART_PLUS);                  // 84 - new item
    
    // == Synchronization (90-99) ==
    loadIcon("sync", wxART_REFRESH);                  // 90 - sync
    loadIcon("diff", wxART_FIND_AND_REPLACE);         // 91 - diff/compare
    loadIcon("compare", wxART_FIND);                  // 92 - compare
    loadIcon("migrate", wxART_GO_FORWARD);            // 93 - migrate
    loadIcon("deploy", wxART_FILE_SAVE_AS);           // 94 - deploy
    
    // == Collaboration (100-109) ==
    loadIcon("shared", wxART_COPY);                   // 100 - shared
    loadIcon("collaboration", wxART_PASTE);           // 101 - collaboration
    loadIcon("team", wxART_NEW);                      // 102 - team
    
    // == Security States (110-119) ==
    loadIcon("lock", wxART_MISSING_IMAGE);            // 110 - locked/protected
    loadIcon("unlock", wxART_MISSING_IMAGE);          // 111 - unlocked
    
    // == Default (120) ==
    loadIcon("table", wxART_MISSING_IMAGE);           // 120 - default/unknown
    
    tree_->SetImageList(tree_images_);
}

int MainFrame::GetIconIndexForNode(const MetadataNode& node) const {
    const std::string& kind = node.kind;
    
    // == Application & Navigation (0-9) ==
    if (kind == "root" || kind == "app" || kind == "application") return 0;
    if (kind == "connection" || kind == "connect") return 1;
    if (kind == "settings" || kind == "config" || kind == "configuration" || 
        kind == "preference" || kind == "option") return 2;
    if (kind == "error" || kind == "warning" || kind == "alert" || 
        kind == "critical" || kind == "fatal") return 3;
    if (kind == "diagram" || kind == "erd" || kind == "chart" || 
        kind == "visualization" || kind == "graph") return 4;
    if (kind == "bookmark" || kind == "favorite" || kind == "star") return 5;
    if (kind == "tag" || kind == "label" || kind == "marker") return 6;
    
    // == Database Objects (10-24) ==
    if (kind == "table" || kind == "tbl") return 10;
    if (kind == "view" || kind == "materialized_view" || kind == "mview" || 
        kind == "virtual_table") return 11;
    if (kind == "column" || kind == "field" || kind == "attribute" || 
        kind == "property") return 12;
    if (kind == "index" || kind == "key" || kind == "idx") return 13;
    if (kind == "sequence" || kind == "seq" || kind == "generator" || 
        kind == "auto_increment") return 14;
    if (kind == "trigger" || kind == "event" || kind == "callback") return 15;
    if (kind == "constraint" || kind == "check" || kind == "foreign_key" || 
        kind == "primary_key" || kind == "unique" || kind == "fk" || 
        kind == "pk" || kind == "uk" || kind == "not_null") return 16;
    if (kind == "procedure" || kind == "proc" || kind == "stored_procedure" || 
        kind == "sp") return 17;
    if (kind == "function" || kind == "func" || kind == "udf" || 
        kind == "routine") return 18;
    if (kind == "package" || kind == "pkg" || kind == "module" || 
        kind == "library") return 19;
    if (kind == "domain" || kind == "type" || kind == "datatype" || 
        kind == "enum" || kind == "data_type") return 20;
    if (kind == "collation" || kind == "charset" || kind == "character_set" || 
        kind == "encoding") return 21;
    if (kind == "tablespace" || kind == "table_space" || kind == "ts" || 
        kind == "filegroup") return 22;
    
    // == Schema Organization (25-29) ==
    if (kind == "database" || kind == "db") return 25;
    if (kind == "catalog") return 26;
    if (kind == "schema" || kind == "namespace" || kind == "owner" || 
        kind == "authorization") return 27;
    if (kind == "folder" || kind == "group" || kind == "category" || 
        kind == "directory" || kind == "path" || kind == "container") return 28;
    
    // == Security & Admin (30-34) ==
    if (kind == "user" || kind == "account" || kind == "login" || 
        kind == "principal") return 30;
    if (kind == "role" || kind == "group_role" || kind == "security_group") return 30;
    if (kind == "host" || kind == "server" || kind == "endpoint" || 
        kind == "machine") return 31;
    if (kind == "permission" || kind == "grant" || kind == "privilege" || 
        kind == "access" || kind == "acl" || kind == "right") return 32;
    if (kind == "audit" || kind == "log" || kind == "trace") return 33;
    if (kind == "history" || kind == "archive" || kind == "snapshot") return 34;
    
    // == Project Objects (35-44) ==
    if (kind == "project" || kind == "workspace") return 35;
    if (kind == "sql" || kind == "script" || kind == "query" || 
        kind == "statement" || kind == "command" || kind == "batch") return 36;
    if (kind == "note" || kind == "comment" || kind == "documentation" || 
        kind == "readme" || kind == "text" || kind == "memo") return 37;
    if (kind == "timeline" || kind == "workflow" || kind == "pipeline" || 
        kind == "stage" || kind == "phase" || kind == "process") return 38;
    if (kind == "job" || kind == "task" || kind == "schedule" || 
        kind == "cron" || kind == "scheduler" || kind == "batch_job") return 39;
    
    // == Version Control (45-49) ==
    if (kind == "git" || kind == "repository" || kind == "repo" || 
        kind == "vcs" || kind == "svn" || kind == "mercurial" || kind == "source") return 45;
    if (kind == "branch" || kind == "tag" || kind == "commit" || 
        kind == "revision" || kind == "version" || kind == "changeset") return 46;
    
    // == Maintenance & Operations (50-59) ==
    if (kind == "backup" || kind == "dump" || kind == "export") return 50;
    if (kind == "restore" || kind == "import" || kind == "load") return 51;
    
    // == Infrastructure (60-69) ==
    if (kind == "server" || kind == "srv" || kind == "host_server") return 60;
    if (kind == "client" || kind == "workstation" || kind == "terminal") return 61;
    if (kind == "filespace" || kind == "storage" || kind == "volume" || 
        kind == "disk" || kind == "mount" || kind == "filesystem") return 62;
    if (kind == "network" || kind == "subnet" || kind == "vlan" || 
        kind == "connection_pool") return 63;
    if (kind == "cluster" || kind == "failover" || kind == "ha_group") return 64;
    if (kind == "instance" || kind == "db_instance" || kind == "service") return 65;
    if (kind == "replica" || kind == "slave" || kind == "standby" || 
        kind == "mirror" || kind == "secondary") return 66;
    if (kind == "shard" || kind == "partition" || kind == "slice" || 
        kind == "segment") return 67;
    
    // == Design & Planning (70-79) ==
    // For unmapped design elements - extracted from DB but not yet implemented
    if (kind == "whiteboard" || kind == "canvas" || kind == "board") return 70;
    if (kind == "mindmap" || kind == "concept_map" || kind == "brainstorm") return 71;
    if (kind == "design" || kind == "model" || kind == "specification" || 
        kind == "spec") return 72;
    if (kind == "draft" || kind == "sketch" || kind == "wip" || 
        kind == "work_in_progress") return 73;
    if (kind == "template" || kind == "boilerplate" || kind == "pattern") return 74;
    if (kind == "blueprint" || kind == "schema_design" || kind == "architecture") return 75;
    if (kind == "concept" || kind == "idea" || kind == "proposal") return 76;
    if (kind == "plan" || kind == "implementation_plan" || kind == "migration_plan" || 
        kind == "deployment_plan") return 77;
    
    // == Design States (80-89) ==
    // Track state of objects in design vs implementation
    if (kind == "implemented" || kind == "deployed" || kind == "production" || 
        kind == "live" || kind == "active") return 80;
    if (kind == "pending" || kind == "staged" || kind == "ready" || 
        kind == "approved") return 81;
    if (kind == "modified" || kind == "changed" || kind == "edited" || 
        kind == "altered" || kind == "updated") return 82;
    if (kind == "deleted" || kind == "removed" || kind == "dropped" || 
        kind == "obsolete" || kind == "deprecated") return 83;
    if (kind == "newitem" || kind == "new" || kind == "added" || 
        kind == "created" || kind == "fresh") return 84;
    
    // == Synchronization (90-99) ==
    if (kind == "sync" || kind == "synchronize" || kind == "reconcile") return 90;
    if (kind == "diff" || kind == "difference" || kind == "delta" || 
        kind == "change_set") return 91;
    if (kind == "compare" || kind == "comparison" || kind == "contrast") return 92;
    if (kind == "migrate" || kind == "migration" || kind == "transform") return 93;
    if (kind == "deploy" || kind == "publish" || kind == "release" || 
        kind == "apply") return 94;
    
    // == Collaboration (100-109) ==
    if (kind == "shared" || kind == "public" || kind == "common") return 100;
    if (kind == "collaboration" || kind == "cooperation" || kind == "joint") return 101;
    if (kind == "team" || kind == "group" || kind == "organization" || 
        kind == "department") return 102;
    
    // == Security States (110-119) ==
    if (kind == "lock" || kind == "locked" || kind == "protected" || 
        kind == "secured" || kind == "frozen") return 110;
    if (kind == "unlock" || kind == "unlocked" || kind == "open" || 
        kind == "editable" || kind == "mutable") return 111;
    
    // == Default (120) ==
    // Any unknown type gets the default icon
    return 120;
}

void MainFrame::PopulateTree(const MetadataSnapshot& snapshot) {
    if (!tree_) {
        return;
    }

    std::string filter = ToLowerCopy(Trim(filter_text_));
    bool has_filter = !filter.empty();

    tree_->Freeze();
    tree_->DeleteAllItems();

    wxTreeItemId root = tree_->AddRoot("ScratchRobin", 0, 0);
    if (snapshot.roots.empty()) {
        tree_->AppendItem(root, "No connections configured", 120, 120);
        tree_->Expand(root);
        tree_->Thaw();
        return;
    }

    std::function<void(const wxTreeItemId&, const MetadataNode&)> addNode;
    addNode = [&](const wxTreeItemId& parent, const MetadataNode& node) {
        int iconIndex = GetIconIndexForNode(node);
        wxTreeItemId id = tree_->AppendItem(parent, node.label, iconIndex, iconIndex,
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
        tree_->AppendItem(root, has_filter ? "No matches for filter" : "No metadata available", 120, 120);
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

bool MainFrame::SelectMetadataPath(const std::string& path) {
    if (!tree_ || path.empty()) {
        return false;
    }
    wxTreeItemId root = tree_->GetRootItem();
    if (!root.IsOk()) {
        return false;
    }
    wxTreeItemId found;
    std::function<void(const wxTreeItemId&)> dfs = [&](const wxTreeItemId& item) {
        if (found.IsOk()) return;
        auto* data = dynamic_cast<MetadataNodeData*>(tree_->GetItemData(item));
        if (data) {
            const MetadataNode* node = data->GetNode();
            if (node) {
                if (node->path == path || node->label == path || node->name == path) {
                    found = item;
                    return;
                }
            }
        }
        wxTreeItemIdValue cookie;
        wxTreeItemId child = tree_->GetFirstChild(item, cookie);
        while (child.IsOk()) {
            dfs(child);
            child = tree_->GetNextChild(item, cookie);
        }
    };
    dfs(root);
    if (!found.IsOk()) {
        return false;
    }
    tree_->SelectItem(found);
    tree_->EnsureVisible(found);
    auto* data = dynamic_cast<MetadataNodeData*>(tree_->GetItemData(found));
    context_node_ = data ? data->GetNode() : nullptr;
    UpdateInspector(context_node_);
    SetStatusText("Selected: " + path);
    return true;
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
    auto* frame = new ConnectionDatabaseManager(this, window_manager_, connection_manager_, connections_, app_config_);
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

void MainFrame::OnCustomizeToolbars(wxCommandEvent&) {
    ui::toolbar::ToolbarEditorForm dialog(this);
    dialog.ShowEditor();
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
    if (inspector_container_) {
        inspector_container_->ActivateForm("dependencies");
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
    // Open the unified Connection & Database Manager
    auto* frame = new ConnectionDatabaseManager(this, window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

// =============================================================================
// Connection Submenu Handlers
// =============================================================================

void MainFrame::OnServerCreate(wxCommandEvent&) {
    wxMessageBox("Server creation will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnServerConnect(wxCommandEvent&) {
    wxMessageBox("Server connection will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnServerDisconnect(wxCommandEvent&) {
    wxMessageBox("Server disconnection will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnServerDrop(wxCommandEvent&) {
    wxMessageBox("Server drop will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnServerRemove(wxCommandEvent&) {
    wxMessageBox("Server removal will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnClusterCreate(wxCommandEvent&) {
    wxMessageBox("Cluster creation will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnClusterConnect(wxCommandEvent&) {
    wxMessageBox("Cluster connection will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnClusterDisconnect(wxCommandEvent&) {
    wxMessageBox("Cluster disconnection will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnClusterDrop(wxCommandEvent&) {
    wxMessageBox("Cluster drop will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnClusterRemove(wxCommandEvent&) {
    wxMessageBox("Cluster removal will be implemented in a future release.",
                 "Not Implemented", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnDatabaseCreate(wxCommandEvent&) {
    // Open the unified Connection & Database Manager
    auto* frame = new ConnectionDatabaseManager(this, window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnDatabaseConnect(wxCommandEvent&) {
    // Open the unified Connection & Database Manager
    auto* frame = new ConnectionDatabaseManager(this, window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void MainFrame::OnDatabaseDisconnect(wxCommandEvent&) {
    if (connection_manager_ && connection_manager_->IsConnected()) {
        connection_manager_->Disconnect();
        SetStatusText("Disconnected");
        // Refresh tree to show disconnected state
        if (metadata_model_) {
            metadata_model_->Refresh();
        }
    } else {
        wxMessageBox("Not currently connected to a database.",
                     "Not Connected", wxOK | wxICON_INFORMATION, this);
    }
}

void MainFrame::OnDatabaseDrop(wxCommandEvent&) {
    // Open the unified Connection & Database Manager
    auto* frame = new ConnectionDatabaseManager(this, window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
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

// ============================================================================
// Layout Management Implementation
// ============================================================================

void MainFrame::SetupLayoutManager() {
    if (!layout_manager_) {
        return;
    }
    
    // Initialize toolbar manager
    ui::toolbar::ToolbarManager::GetInstance().Initialize();
    
    layout_manager_->Initialize();
    
    // Subscribe to layout changes
    layout_manager_->AddObserver([this](const LayoutChangeEvent& evt) {
        switch (evt.type) {
            case LayoutChangeEvent::WindowRegistered:
                SetStatusText("Window added: " + evt.window_id);
                break;
            case LayoutChangeEvent::LayoutLoaded:
                SetStatusText("Layout loaded: " + evt.layout_name);
                break;
            default:
                break;
        }
    });
    
    // Default presets are automatically available via GetPresets()
}

void MainFrame::CreateDockablePanels() {
    // This is where dockable panels would be created
    // For now, the existing BuildLayout handles the basic UI
}

void MainFrame::SwitchLayout(const std::string& preset_name) {
    if (layout_manager_) {
        layout_manager_->LoadPreset(preset_name);
    }
}

void MainFrame::SaveCurrentLayout(const std::string& name) {
    if (layout_manager_) {
        layout_manager_->SaveCurrentAsPreset(name, "User custom layout");
    }
}

void MainFrame::OnLayoutDefault(wxCommandEvent&) {
    SwitchLayout("Default");
}

void MainFrame::OnLayoutSingleMonitor(wxCommandEvent&) {
    SwitchLayout("Single Monitor");
}

void MainFrame::OnLayoutDualMonitor(wxCommandEvent&) {
    SwitchLayout("Dual Monitor");
}

void MainFrame::OnLayoutWideScreen(wxCommandEvent&) {
    SwitchLayout("Wide Screen");
}

void MainFrame::OnLayoutCompact(wxCommandEvent&) {
    SwitchLayout("Compact");
}

void MainFrame::OnLayoutSaveCurrent(wxCommandEvent&) {
    wxTextEntryDialog dialog(this, "Enter a name for this layout:", "Save Layout", "My Layout");
    if (dialog.ShowModal() == wxID_OK) {
        wxString name = dialog.GetValue();
        if (!name.IsEmpty()) {
            SaveCurrentLayout(name.ToStdString());
            SetStatusText("Layout saved: " + name.ToStdString());
        }
    }
}

void MainFrame::OnLayoutManage(wxCommandEvent&) {
    wxMessageBox("Layout management dialog will be implemented here.\n\n"
                 "Available layouts:\n"
                 "- Default\n"
                 "- Single Monitor\n"
                 "- Dual Monitor\n"
                 "- Wide Screen\n"
                 "- Compact\n\n"
                 "Custom layouts can be saved and restored.", 
                 "Manage Layouts", wxOK | wxICON_INFORMATION, this);
}

} // namespace scratchrobin
