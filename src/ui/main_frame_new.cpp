/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/main_frame_new.h"

#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "ui/window_framework/action_catalog.h"
#include "ui/window_framework/project_navigator_new.h"
#include "ui/dialogs/preferences_dialog.h"
#include "ui/dialogs/connection_dialog.h"
#include "ui/dialogs/query_builder_dialog.h"
#include "ui/dialogs/query_profiler_dialog.h"
#include "ui/dialogs/query_plan_visualizer.h"
#include "ui/dialogs/snippet_browser.h"
#include "ui/dialogs/performance_monitor_dialog.h"
#include "ui/dialogs/backup_restore_dialog.h"
#include "ui/dialogs/data_import_export_dialog.h"
#include "ui/dialogs/database_migration_wizard.h"
#include "ui/dialogs/schema_comparison_dialog.h"
#include "ui/dialogs/schema_documentation_dialog.h"
#include "ui/dialogs/admin_dialog.h"
#include "ui/dialogs/connection_manager_dialog.h"
#include "sql_editor_frame_new.h"
#include "ui/dialogs/object_metadata_dialog_new.h"
#include "ui/dialogs/query_history_dialog.h"
#include "core/settings.h"

namespace scratchrobin::ui {

// Event table
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
  // File menu
  EVT_MENU(wxID_NEW, MainFrame::OnFileNewConnection)
  EVT_MENU(wxID_OPEN, MainFrame::OnFileOpenSql)
  EVT_MENU(wxID_SAVE, MainFrame::OnFileSave)
  EVT_MENU(wxID_SAVEAS, MainFrame::OnFileSaveAs)
  EVT_MENU(wxID_EXIT, MainFrame::OnFileExit)

  // Edit menu
  EVT_MENU(wxID_UNDO, MainFrame::OnEditUndo)
  EVT_MENU(wxID_REDO, MainFrame::OnEditRedo)
  EVT_MENU(wxID_CUT, MainFrame::OnEditCut)
  EVT_MENU(wxID_COPY, MainFrame::OnEditCopy)
  EVT_MENU(wxID_PASTE, MainFrame::OnEditPaste)
  EVT_MENU(wxID_PREFERENCES, MainFrame::OnEditPreferences)

  // View menu
  EVT_MENU(wxID_VIEW_DETAILS, MainFrame::OnViewProjectNavigator)
  EVT_MENU(wxID_REFRESH, MainFrame::OnViewRefresh)
  EVT_MENU(wxID_FILE8, MainFrame::OnViewFullscreen)

  // Database menu
  EVT_MENU(1001, MainFrame::OnDbConnect)
  EVT_MENU(1002, MainFrame::OnDbDisconnect)
  EVT_MENU(1003, MainFrame::OnDbNewDatabase)
  EVT_MENU(1004, MainFrame::OnDbProperties)

  // Help menu
  EVT_MENU(wxID_HELP, MainFrame::OnHelpDocumentation)
  EVT_MENU(wxID_ABOUT, MainFrame::OnHelpAbout)

  // Window events
  EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(backend::SessionClient* session_client)
    : wxFrame(nullptr, wxID_ANY, _("ScratchRobin - Database Administration Tool"),
              wxDefaultPosition, wxSize(1280, 800)),
      session_client_(session_client) {
  runtime_config_.mode = backend::TransportMode::kPreview;

  SetupFrame();
  CreateMenuBar();
  CreateMainToolBar();
  CreateMainStatusBar();
  CreateLayout();

  // Load and apply settings
  core::Settings::get().load();
  
  // Initialize connection state
  SetConnectionStatus(false);
}

MainFrame::~MainFrame() {
  // Cleanup
}

void MainFrame::SetupFrame() {
  // Set minimum size
  SetMinSize(wxSize(1024, 600));

  // Set icon
  SetIcon(wxArtProvider::GetIcon(wxART_HARDDISK, wxART_FRAME_ICON));

  // Center on screen
  Centre(wxBOTH);
}

void MainFrame::CreateMenuBar() {
  menu_bar_ = new wxMenuBar();

  // File Menu
  wxMenu* file_menu = new wxMenu();
  file_menu->Append(wxID_NEW, _("&New Connection...\tCtrl+N"), _("Create a new database connection"));
  file_menu->Append(wxID_OPEN, _("&Open SQL Script...\tCtrl+O"), _("Open a SQL script file"));
  file_menu->AppendSeparator();
  file_menu->Append(wxID_SAVE, _("&Save\tCtrl+S"), _("Save current SQL script"));
  file_menu->Append(wxID_SAVEAS, _("Save &As...\tCtrl+Shift+S"), _("Save SQL script with a new name"));
  file_menu->AppendSeparator();
  file_menu->Append(wxID_PRINT, _("&Print...\tCtrl+P"), _("Print current document"));
  file_menu->AppendSeparator();
  file_menu->Append(wxID_EXIT, _("E&xit\tCtrl+Q"), _("Exit the application"));
  menu_bar_->Append(file_menu, _("&File"));

  // Edit Menu
  wxMenu* edit_menu = new wxMenu();
  edit_menu->Append(wxID_UNDO, _("&Undo\tCtrl+Z"), _("Undo last action"));
  edit_menu->Append(wxID_REDO, _("&Redo\tCtrl+Shift+Z"), _("Redo last undone action"));
  edit_menu->AppendSeparator();
  edit_menu->Append(wxID_CUT, _("Cu&t\tCtrl+X"), _("Cut selection to clipboard"));
  edit_menu->Append(wxID_COPY, _("&Copy\tCtrl+C"), _("Copy selection to clipboard"));
  edit_menu->Append(wxID_PASTE, _("&Paste\tCtrl+V"), _("Paste from clipboard"));
  edit_menu->Append(wxID_SELECTALL, _("Select &All\tCtrl+A"), _("Select all content"));
  edit_menu->AppendSeparator();
  edit_menu->Append(wxID_PREFERENCES, _("&Preferences...\tCtrl+Comma"), _("Edit application preferences"));
  menu_bar_->Append(edit_menu, _("&Edit"));

  // View Menu
  wxMenu* view_menu = new wxMenu();
  item_project_navigator_ = view_menu->AppendCheckItem(wxID_VIEW_DETAILS, _("&Project Navigator"), _("Show/hide project navigator"));
  item_project_navigator_->Check(true);
  item_properties_ = view_menu->AppendCheckItem(wxID_VIEW_LIST, _("&Properties Panel"), _("Show/hide properties panel"));
  item_properties_->Check(true);
  item_output_ = view_menu->AppendCheckItem(wxID_VIEW_LARGEICONS, _("&Output Panel"), _("Show/hide output panel"));
  item_output_->Check(false);
  view_menu->AppendSeparator();
  view_menu->Append(wxID_REFRESH, _("&Refresh\tF5"), _("Refresh current view"));
  view_menu->AppendSeparator();
  view_menu->Append(wxID_FILE8, _("&Fullscreen\tF11"), _("Toggle fullscreen mode"));
  menu_bar_->Append(view_menu, _("&View"));

  // Database Menu
  wxMenu* db_menu = new wxMenu();
  item_db_connect_ = db_menu->Append(1001, _("&Connect"), _("Connect to database"));
  item_db_disconnect_ = db_menu->Append(1002, _("&Disconnect"), _("Disconnect from database"));
  db_menu->AppendSeparator();
  db_menu->Append(1003, _("&New Database..."), _("Create a new database"));
  db_menu->Append(1004, _("&Properties..."), _("Database properties"));
  db_menu->AppendSeparator();
  db_menu->Append(1005, _("&Backup/Restore..."), _("Backup or restore database"));
  db_menu->Append(1006, _("&Data Import/Export..."), _("Import or export data"));
  db_menu->AppendSeparator();
  db_menu->Append(1007, _("&Migration Wizard..."), _("Database migration tools"));
  db_menu->Append(1008, _("&Schema Comparison..."), _("Compare database schemas"));
  db_menu->Append(1009, _("&Schema Documentation..."), _("Generate schema docs with ER diagrams"));
  db_menu->AppendSeparator();
  db_menu->Append(1010, _("&Admin Tools..."), _("Server administration"));
  db_menu->Append(1011, _("&Connection Manager..."), _("Manage saved connections"));
  menu_bar_->Append(db_menu, _("&Database"));

  // Tools Menu
  wxMenu* tools_menu = new wxMenu();
  tools_menu->Append(2001, _("&Query Builder..."), _("Visual query builder"));
  tools_menu->Append(2002, _("&Query History...\tCtrl+H"), _("Browse query history"));
  tools_menu->AppendSeparator();
  tools_menu->Append(2003, _("&SQL Editor...\tCtrl+E"), _("Open SQL editor"));
  tools_menu->Append(2004, _("Query &Profiler..."), _("Query performance profiling"));
  tools_menu->Append(2005, _("Query &Plan Visualizer..."), _("Visualize query execution plans"));
  tools_menu->AppendSeparator();
  tools_menu->Append(2006, _("&Snippet Browser..."), _("Browse SQL code snippets"));
  tools_menu->Append(2007, _("&Performance Monitor..."), _("Monitor database performance"));
  menu_bar_->Append(tools_menu, _("&Tools"));

  // Help Menu
  wxMenu* help_menu = new wxMenu();
  help_menu->Append(wxID_HELP, _("&Documentation\tF1"), _("Open documentation"));
  help_menu->AppendSeparator();
  help_menu->Append(wxID_ABOUT, _("&About..."), _("About this application"));
  menu_bar_->Append(help_menu, _("&Help"));

  SetMenuBar(menu_bar_);
  
  // Bind Tools menu events
  Bind(wxEVT_MENU, &MainFrame::OnToolsQueryBuilder, this, 2001);
  Bind(wxEVT_MENU, &MainFrame::OnToolsQueryHistory, this, 2002);
  Bind(wxEVT_MENU, &MainFrame::OpenSqlEditorFromMenu, this, 2003);
  Bind(wxEVT_MENU, &MainFrame::OnToolsQueryProfiler, this, 2004);
  Bind(wxEVT_MENU, &MainFrame::OnToolsQueryPlanVisualizer, this, 2005);
  Bind(wxEVT_MENU, &MainFrame::OnToolsSnippetBrowser, this, 2006);
  Bind(wxEVT_MENU, &MainFrame::OnToolsPerformanceMonitor, this, 2007);
  
  // Bind Database menu events
  Bind(wxEVT_MENU, &MainFrame::OnDbBackupRestore, this, 1005);
  Bind(wxEVT_MENU, &MainFrame::OnDbDataImportExport, this, 1006);
  Bind(wxEVT_MENU, &MainFrame::OnDbMigrationWizard, this, 1007);
  Bind(wxEVT_MENU, &MainFrame::OnDbSchemaComparison, this, 1008);
  Bind(wxEVT_MENU, &MainFrame::OnDbSchemaDocumentation, this, 1009);
  Bind(wxEVT_MENU, &MainFrame::OnDbAdminTools, this, 1010);
  Bind(wxEVT_MENU, &MainFrame::OnDbConnectionManager, this, 1011);
}

void MainFrame::CreateMainToolBar() {
  tool_bar_ = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL, wxID_ANY);

  // File operations
  tool_bar_->AddTool(wxID_NEW, _("New"), wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR, wxSize(24, 24)),
                     _("New Connection"));
  tool_bar_->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Open SQL Script"));
  tool_bar_->AddTool(wxID_SAVE, _("Save"), wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Save"));
  tool_bar_->AddSeparator();

  // Edit operations
  tool_bar_->AddTool(wxID_CUT, _("Cut"), wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Cut"));
  tool_bar_->AddTool(wxID_COPY, _("Copy"), wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Copy"));
  tool_bar_->AddTool(wxID_PASTE, _("Paste"), wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Paste"));
  tool_bar_->AddSeparator();

  // Database operations
  tool_bar_->AddTool(1001, _("Connect"), wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Connect"));
  tool_bar_->AddTool(1002, _("Disconnect"), wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Disconnect"));
  tool_bar_->AddSeparator();

  // View operations
  tool_bar_->AddTool(wxID_REFRESH, _("Refresh"), wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR, wxSize(24, 24)),
                     _("Refresh"));

  tool_bar_->Realize();
}

void MainFrame::CreateMainStatusBar() {
  status_bar_ = CreateStatusBar(4);

  // Panel 0: Connection status
  // Panel 1: Current user/schema
  // Panel 2: Operation status
  // Panel 3: Mode

  int widths[] = {200, 150, 100, 100};
  SetStatusWidths(4, widths);

  SetStatusText(_("Not connected"), 0);
  SetStatusText(wxEmptyString, 1);
  SetStatusText(_("Ready"), 2);
  SetStatusText(_("Preview"), 3);
}

void MainFrame::CreateLayout() {
  main_panel_ = new wxPanel(this, wxID_ANY);

  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

  // Create splitter for navigator and content
  wxSplitterWindow* splitter = new wxSplitterWindow(main_panel_, wxID_ANY, 
                                                     wxDefaultPosition, wxDefaultSize,
                                                     wxSP_3D | wxSP_LIVE_UPDATE);

  // Project Navigator (left)
  project_navigator_ = new ProjectNavigator(splitter, "main_navigator");
  
  // Set up callbacks for navigator actions
  ProjectNavigatorCallbacks callbacks;
  callbacks.on_connect = [this](const std::string& server) { OnConnectToServer(server); };
  callbacks.on_disconnect = [this](const std::string& server) { OnDisconnectFromServer(server); };
  callbacks.on_new_connection = [this]() { OnNewConnection(); };
  callbacks.on_browse_table = [this](const std::string& table, const std::string& db) {
    OnBrowseTableData(table, db);
  };
  callbacks.on_browse_view = [this](const std::string& view, const std::string& db) {
    OnBrowseViewData(view, db);
  };
  callbacks.on_show_metadata = [this](const std::string& name, const std::string& type,
                                      const std::string& db) {
    OnShowObjectMetadata(name, type, db);
  };
  callbacks.on_generate_select = [this](const std::string& table, const std::string& db) {
    OnGenerateSelectSQL(table, db);
  };
  project_navigator_->SetCallbacks(callbacks);

  // Content area (right) - will hold editors
  wxPanel* content_panel = new wxPanel(splitter, wxID_ANY);
  wxBoxSizer* content_sizer = new wxBoxSizer(wxVERTICAL);
  
  wxString placeholder_text = _("Select a database object from the navigator\n");
  placeholder_text += _("or create a new connection to get started.");
  wxStaticText* placeholder = new wxStaticText(content_panel, wxID_ANY, 
                                                placeholder_text,
                                                wxDefaultPosition, wxDefaultSize,
                                                wxALIGN_CENTER_HORIZONTAL);
  content_sizer->AddStretchSpacer(1);
  content_sizer->Add(placeholder, 0, wxALIGN_CENTER);
  content_sizer->AddStretchSpacer(1);
  content_panel->SetSizer(content_sizer);

  // Split the window
  splitter->SplitVertically(project_navigator_, content_panel, 300);
  splitter->SetMinimumPaneSize(200);

  main_sizer->Add(splitter, 1, wxEXPAND);
  main_panel_->SetSizer(main_sizer);
}

void MainFrame::SetConnectionStatus(bool connected, const wxString& connection_name) {
  is_connected_ = connected;
  current_connection_name_ = connection_name;

  // Update status bar
  if (connected) {
    SetStatusText(wxString::Format(_("Connected: %s"), connection_name), 0);
  } else {
    SetStatusText(_("Not connected"), 0);
  }

  // Update menu states
  UpdateMenuStates();
}

void MainFrame::UpdateMenuStates() {
  // Database menu
  item_db_connect_->Enable(!is_connected_);
  item_db_disconnect_->Enable(is_connected_);
}

// Event Handlers - File
void MainFrame::OnFileNewConnection(wxCommandEvent& event) {
  ConnectionDialog dlg(this, session_client_);
  if (dlg.ShowModal() == wxID_OK) {
    ConnectionConfig config = dlg.GetConnectionConfig();
    // TODO: Initiate connection via session_client_
    wxMessageBox(wxString::Format("Connecting to %s:%d/%s as %s",
                                  config.host, config.port, 
                                  config.database, config.username),
                 "Connecting...", wxOK | wxICON_INFORMATION);
  }
}

void MainFrame::OnFileOpenSql(wxCommandEvent& event) {
  // TODO: Open file dialog and load SQL
  OpenSqlEditor();
}

void MainFrame::OnFileSave(wxCommandEvent& event) {
  // TODO: Save current editor content
}

void MainFrame::OnFileSaveAs(wxCommandEvent& event) {
  // TODO: Save with new filename
}

void MainFrame::OnFileExit(wxCommandEvent& event) {
  Close(true);
}

// Event Handlers - Edit
void MainFrame::OnEditUndo(wxCommandEvent& event) {
  // TODO: Undo in active editor
}

void MainFrame::OnEditRedo(wxCommandEvent& event) {
  // TODO: Redo in active editor
}

void MainFrame::OnEditCut(wxCommandEvent& event) {
  // TODO: Cut from active editor
}

void MainFrame::OnEditCopy(wxCommandEvent& event) {
  // TODO: Copy from active editor
}

void MainFrame::OnEditPaste(wxCommandEvent& event) {
  // TODO: Paste to active editor
}

void MainFrame::OnEditPreferences(wxCommandEvent& event) {
  PreferencesDialog dlg(this);
  dlg.ShowModal();
}

// Event Handlers - View
void MainFrame::OnViewProjectNavigator(wxCommandEvent& event) {
  // TODO: Toggle navigator visibility
}

void MainFrame::OnViewProperties(wxCommandEvent& event) {
  // TODO: Toggle properties panel
}

void MainFrame::OnViewOutput(wxCommandEvent& event) {
  // TODO: Toggle output panel
}

void MainFrame::OnViewRefresh(wxCommandEvent& event) {
  if (project_navigator_) {
    project_navigator_->refreshTree();
  }
}

void MainFrame::OnViewFullscreen(wxCommandEvent& event) {
  ShowFullScreen(!IsFullScreen());
}

// Event Handlers - Database
void MainFrame::OnDbConnect(wxCommandEvent& event) {
  // TODO: Show connection dialog
  SetConnectionStatus(true, _("test_db"));
}

void MainFrame::OnDbDisconnect(wxCommandEvent& event) {
  // TODO: Disconnect
  SetConnectionStatus(false);
}

void MainFrame::OnDbNewDatabase(wxCommandEvent& event) {
  // TODO: New database dialog
}

void MainFrame::OnDbProperties(wxCommandEvent& event) {
  // TODO: Database properties dialog
}

// Event Handlers - Help
void MainFrame::OnHelpDocumentation(wxCommandEvent& event) {
  // TODO: Open documentation
  wxMessageBox(_("Documentation would open in browser"), _("Help"), wxOK | wxICON_INFORMATION);
}

void MainFrame::OnHelpAbout(wxCommandEvent& event) {
  wxString msg = _("Robin-Migrate v1.0.0\n\n");
  msg += _("Database Administration Tool\n");
  msg += _("for ScratchBird Databases\n\n");
  msg += _("2026 Robin-Migrate Contributors");
  wxMessageBox(msg, _("About Robin-Migrate"), wxOK | wxICON_INFORMATION);
}

void MainFrame::OnClose(wxCloseEvent& event) {
  // TODO: Check for unsaved changes
  // Save window state
  event.Skip();
}

// Event Handlers - Tools
void MainFrame::OnToolsQueryBuilder(wxCommandEvent& event) {
  ShowQueryBuilder();
}

void MainFrame::OnToolsQueryHistory(wxCommandEvent& event) {
  // TODO: Show query history dialog
  wxMessageBox(_("Query History dialog would open"), _("Query History"), wxOK | wxICON_INFORMATION);
}

// ProjectNavigatorActions Implementation
void MainFrame::OnConnectToServer(const std::string& server_id) {
  wxMessageBox(wxString::Format(_("Connect to server: %s"), server_id),
               _("Connect"), wxOK | wxICON_INFORMATION);
}

void MainFrame::OnDisconnectFromServer(const std::string& server_id) {
  SetConnectionStatus(false);
  wxMessageBox(wxString::Format(_("Disconnected from: %s"), server_id),
               _("Disconnect"), wxOK | wxICON_INFORMATION);
}

void MainFrame::OnNewConnection() {
  wxCommandEvent dummy_event;
  OnFileNewConnection(dummy_event);
}

void MainFrame::OnBrowseTableData(const std::string& table_name,
                                   const std::string& database) {
  wxString sql = wxString::Format("SELECT * FROM %s;", table_name);
  OpenSqlEditorWithQuery(sql.ToStdString());
}

void MainFrame::OnBrowseViewData(const std::string& view_name,
                                  const std::string& database) {
  wxString sql = wxString::Format("SELECT * FROM %s;", view_name);
  OpenSqlEditorWithQuery(sql.ToStdString());
}

void MainFrame::OnEditTable(const std::string& table_name,
                             const std::string& database) {
  OnShowObjectMetadata(table_name, "TABLE", database);
}

void MainFrame::OnEditView(const std::string& view_name,
                            const std::string& database) {
  OnShowObjectMetadata(view_name, "VIEW", database);
}

void MainFrame::OnEditProcedure(const std::string& proc_name,
                                 const std::string& database) {
  OnShowObjectMetadata(proc_name, "PROCEDURE", database);
}

void MainFrame::OnEditFunction(const std::string& func_name,
                                const std::string& database) {
  OnShowObjectMetadata(func_name, "FUNCTION", database);
}

void MainFrame::OnShowObjectMetadata(const std::string& object_name,
                                      const std::string& object_type,
                                      const std::string& database) {
  ObjectMetadataDialogNew* dlg = new ObjectMetadataDialogNew(
      this, session_client_, object_name, object_type);
  dlg->LoadMetadata();
  dlg->Show();
}

void MainFrame::OnGenerateSelectSQL(const std::string& table_name,
                                     const std::string& database) {
  wxString sql = wxString::Format("SELECT *\nFROM %s;", table_name);
  OpenSqlEditorWithQuery(sql.ToStdString());
}

void MainFrame::OnGenerateInsertSQL(const std::string& table_name,
                                     const std::string& database) {
  wxString sql = wxString::Format("INSERT INTO %s (col1, col2, col3)\nVALUES (val1, val2, val3);",
                                   table_name);
  OpenSqlEditorWithQuery(sql.ToStdString());
}

void MainFrame::OnGenerateUpdateSQL(const std::string& table_name,
                                     const std::string& database) {
  wxString sql = wxString::Format("UPDATE %s\nSET col1 = val1\nWHERE condition;",
                                   table_name);
  OpenSqlEditorWithQuery(sql.ToStdString());
}

void MainFrame::OnGenerateDeleteSQL(const std::string& table_name,
                                     const std::string& database) {
  wxString sql = wxString::Format("DELETE FROM %s\nWHERE condition;", table_name);
  OpenSqlEditorWithQuery(sql.ToStdString());
}

void MainFrame::OnDragObjectName(const std::string& object_name) {
  // TODO: Handle drag and drop to SQL editor
}

// Helper Methods
void MainFrame::OpenSqlEditor(const std::string& sql) {
  SqlEditorFrameNew* frame = new SqlEditorFrameNew(this, session_client_, &runtime_config_);
  if (!sql.empty()) {
    frame->SetSqlText(sql);
  }
  frame->Show();
  sql_editor_frames_.push_back(frame);
  editor_counter_++;
}

void MainFrame::OpenSqlEditorWithQuery(const std::string& sql) {
  OpenSqlEditor(sql);
}

void MainFrame::OpenSqlEditorFromMenu(wxCommandEvent& event) {
  OpenSqlEditor();
}

void MainFrame::ShowQueryBuilder() {
  QueryBuilderDialog dlg(this);
  if (dlg.ShowModal() == wxID_OK) {
    std::string sql = dlg.GetGeneratedSQL();
    if (!sql.empty()) {
      OpenSqlEditorWithQuery(sql);
    }
  }
}

// Tools Menu Handlers
void MainFrame::OnToolsQueryProfiler(wxCommandEvent& event) {
  QueryProfilerDialog dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnToolsQueryPlanVisualizer(wxCommandEvent& event) {
  QueryPlanVisualizer dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnToolsSnippetBrowser(wxCommandEvent& event) {
  SnippetBrowser dlg(this);
  if (dlg.ShowModal() == wxID_OK) {
    wxString snippet = dlg.GetSelectedSnippet();
    if (!snippet.IsEmpty()) {
      OpenSqlEditorWithQuery(snippet.ToStdString());
    }
  }
}

void MainFrame::OnToolsPerformanceMonitor(wxCommandEvent& event) {
  PerformanceMonitorDialog dlg(this);
  dlg.ShowModal();
}

// Database Menu Handlers
void MainFrame::OnDbBackupRestore(wxCommandEvent& event) {
  BackupRestoreDialog dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnDbDataImportExport(wxCommandEvent& event) {
  DataImportExportDialog dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnDbMigrationWizard(wxCommandEvent& event) {
  DatabaseMigrationWizard dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnDbSchemaComparison(wxCommandEvent& event) {
  SchemaComparisonDialog dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnDbSchemaDocumentation(wxCommandEvent& event) {
  SchemaDocumentationDialog dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnDbAdminTools(wxCommandEvent& event) {
  AdminDialog dlg(this);
  dlg.ShowModal();
}

void MainFrame::OnDbConnectionManager(wxCommandEvent& event) {
  ConnectionManagerDialog dlg(this);
  dlg.ShowModal();
}

}  // namespace scratchrobin::ui
