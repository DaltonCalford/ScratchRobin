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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/toolbar.h>

#include "backend/scratchbird_runtime_config.h"
#include "backend/session_client.h"
#include "ui/window_framework/project_navigator_actions.h"

// Forward declarations
namespace scratchrobin::ui {
class ProjectNavigator;
class DynamicMenuBar;
class SqlEditorFrameNew;
class ObjectMetadataDialogNew;
}  // namespace scratchrobin::ui

namespace scratchrobin::ui {

/**
 * Main application window - Phase 2 Implementation
 * 
 * Features:
 * - Menu bar with all standard menus
 * - Toolbar with standard actions
 * - Status bar with 4 panels
 * - Project navigator (dockable) with callbacks
 * - Central area for editors
 * - Integrated SQL editor with history
 * - Object metadata viewer
 */
class MainFrame final : public wxFrame, public ProjectNavigatorActions {
 public:
  explicit MainFrame(backend::SessionClient* session_client);
  ~MainFrame() override;

  // Menu update interface
  void UpdateMenuStates();
  void SetConnectionStatus(bool connected, const wxString& connection_name = wxEmptyString);

 private:
  // Window setup
  void SetupFrame();
  void CreateMenuBar();
  void CreateMainToolBar();
  void CreateMainStatusBar();
  void CreateLayout();

  // Menu handlers - File
  void OnFileNewConnection(wxCommandEvent& event);
  void OnFileOpenSql(wxCommandEvent& event);
  void OnFileSave(wxCommandEvent& event);
  void OnFileSaveAs(wxCommandEvent& event);
  void OnFileExit(wxCommandEvent& event);

  // Menu handlers - Edit
  void OnEditUndo(wxCommandEvent& event);
  void OnEditRedo(wxCommandEvent& event);
  void OnEditCut(wxCommandEvent& event);
  void OnEditCopy(wxCommandEvent& event);
  void OnEditPaste(wxCommandEvent& event);
  void OnEditPreferences(wxCommandEvent& event);

  // Menu handlers - View
  void OnViewProjectNavigator(wxCommandEvent& event);
  void OnViewProperties(wxCommandEvent& event);
  void OnViewOutput(wxCommandEvent& event);
  void OnViewRefresh(wxCommandEvent& event);
  void OnViewFullscreen(wxCommandEvent& event);

  // Menu handlers - Database
  void OnDbConnect(wxCommandEvent& event);
  void OnDbDisconnect(wxCommandEvent& event);
  void OnDbNewDatabase(wxCommandEvent& event);
  void OnDbProperties(wxCommandEvent& event);

  // Menu handlers - Help
  void OnHelpDocumentation(wxCommandEvent& event);
  void OnHelpAbout(wxCommandEvent& event);

  // Menu handlers - Tools
  void OnToolsQueryBuilder(wxCommandEvent& event);
  void OnToolsQueryHistory(wxCommandEvent& event);
  void OnToolsQueryProfiler(wxCommandEvent& event);
  void OnToolsQueryPlanVisualizer(wxCommandEvent& event);
  void OnToolsSnippetBrowser(wxCommandEvent& event);
  void OnToolsPerformanceMonitor(wxCommandEvent& event);
  
  // Menu handlers - Database (additional)
  void OnDbBackupRestore(wxCommandEvent& event);
  void OnDbDataImportExport(wxCommandEvent& event);
  void OnDbMigrationWizard(wxCommandEvent& event);
  void OnDbSchemaComparison(wxCommandEvent& event);
  void OnDbSchemaDocumentation(wxCommandEvent& event);
  void OnDbAdminTools(wxCommandEvent& event);
  void OnDbConnectionManager(wxCommandEvent& event);

  // ProjectNavigatorActions interface
  void OnConnectToServer(const std::string& server_id) override;
  void OnDisconnectFromServer(const std::string& server_id) override;
  void OnNewConnection() override;
  void OnBrowseTableData(const std::string& table_name, 
                         const std::string& database) override;
  void OnBrowseViewData(const std::string& view_name,
                        const std::string& database) override;
  void OnEditTable(const std::string& table_name,
                   const std::string& database) override;
  void OnEditView(const std::string& view_name,
                  const std::string& database) override;
  void OnEditProcedure(const std::string& proc_name,
                       const std::string& database) override;
  void OnEditFunction(const std::string& func_name,
                      const std::string& database) override;
  void OnShowObjectMetadata(const std::string& object_name,
                            const std::string& object_type,
                            const std::string& database) override;
  void OnGenerateSelectSQL(const std::string& table_name,
                           const std::string& database) override;
  void OnGenerateInsertSQL(const std::string& table_name,
                           const std::string& database) override;
  void OnGenerateUpdateSQL(const std::string& table_name,
                           const std::string& database) override;
  void OnGenerateDeleteSQL(const std::string& table_name,
                           const std::string& database) override;
  void OnDragObjectName(const std::string& object_name) override;

  // Helper methods
  void OpenSqlEditor(const std::string& sql = "");
  void OpenSqlEditorWithQuery(const std::string& sql);
  void OpenSqlEditorFromMenu(wxCommandEvent& event);
  void ShowQueryBuilder();

  // Event handlers
  void OnClose(wxCloseEvent& event);

  // Backend
  backend::SessionClient* session_client_{nullptr};
  backend::ScratchbirdRuntimeConfig runtime_config_;

  // UI Components
  wxMenuBar* menu_bar_{nullptr};
  wxToolBar* tool_bar_{nullptr};
  wxStatusBar* status_bar_{nullptr};

  // Menu items (for state updates)
  wxMenuItem* item_project_navigator_{nullptr};
  wxMenuItem* item_properties_{nullptr};
  wxMenuItem* item_output_{nullptr};
  wxMenuItem* item_db_connect_{nullptr};
  wxMenuItem* item_db_disconnect_{nullptr};

  // Panels
  wxPanel* main_panel_{nullptr};
  ProjectNavigator* project_navigator_{nullptr};

  // State
  bool is_connected_{false};
  wxString current_connection_name_;

  // Editor management
  std::vector<SqlEditorFrameNew*> sql_editor_frames_;
  int editor_counter_{0};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
