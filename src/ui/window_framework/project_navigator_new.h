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
#include <functional>

#include <wx/treectrl.h>
#include <wx/generic/treectlg.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

#include "ui/window_framework/dockable_window.h"
#include "ui/window_framework/project_navigator_actions.h"

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * Project Navigator Panel - Phase 1 Implementation
 * 
 * Features:
 * - Lazy loading tree (load children on expand)
 * - Real-time filtering
 * - Context menus per node type
 * - Drag & drop support
 */
class ProjectNavigator : public DockableWindow {
 public:
  ProjectNavigator(wxWindow* parent, const std::string& instance_id = "");
  ~ProjectNavigator() override;

  // Tree operations
  void refreshTree();
  void ExpandNode(wxTreeItemId item);
  void CollapseNode(wxTreeItemId item);
  
  // Selection
  wxTreeItemId GetSelectedItem() const;
  void SelectItem(wxTreeItemId item);

  // Filtering
  void SetFilter(const wxString& filter);
  void ClearFilter();

  // Callbacks
  void SetCallbacks(const ProjectNavigatorCallbacks& callbacks) { callbacks_ = callbacks; }
  void SetActionHandler(ProjectNavigatorActions* handler) { action_handler_ = handler; }

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
protected:
  void onWindowCreated() override;
  wxSize getDefaultSize() const override;

  // Tree data structures (public for use in callbacks)
  struct TreeNodeData : public wxTreeItemData {
    enum class NodeType {
      kRoot,
      kServer,
      kDatabase,
      kSchema,
      kTablesFolder,
      kViewsFolder,
      kProceduresFolder,
      kFunctionsFolder,
      kTriggersFolder,
      kIndexesFolder,
      kColumnsFolder,
      kTable,
      kView,
      kProcedure,
      kFunction,
      kTrigger,
      kIndex,
      kColumn
    };

    NodeType type;
    std::string id;           // Unique identifier
    std::string name;         // Display name
    std::string parent_id;    // Parent identifier
    bool loaded{false};       // Children loaded?

    TreeNodeData(NodeType t) : type(t) {}
  };

  // Tree population
  void PopulateRoot();
  void BindEvents();
  
protected:
  // Make TreeNodeData available to implementations
  typedef TreeNodeData NodeData;
  void LoadChildren(wxTreeItemId parent);
  void OnItemExpanding(wxTreeEvent& event);
  void OnItemCollapsing(wxTreeEvent& event);
  void OnSelectionChanged(wxTreeEvent& event);
  void OnItemActivated(wxTreeEvent& event);
  void OnContextMenu(wxTreeEvent& event);

  // Filter handling
  void OnFilterTextChanged(wxCommandEvent& event);
  void OnFilterTimer(wxTimerEvent& event);
  void ApplyFilter();
  bool ShouldShowItem(wxTreeItemId item, const wxString& filter);

  // Context menus
  void ShowServerMenu(wxTreeItemId item);
  void ShowDatabaseMenu(wxTreeItemId item);
  void ShowTableFolderMenu(wxTreeItemId item);
  void ShowTableMenu(wxTreeItemId item);
  void ShowViewMenu(wxTreeItemId item);
  void ShowProcedureMenu(wxTreeItemId item);
  void ShowFunctionMenu(wxTreeItemId item);

  // Helper methods
  std::string GetItemName(wxTreeItemId item);
  std::string GetItemDatabase(wxTreeItemId item);
  std::string GetItemType(wxTreeItemId item);

  // Actions
  void OnConnect(wxCommandEvent& event);
  void OnDisconnect(wxCommandEvent& event);
  void OnNewDatabase(wxCommandEvent& event);
  void OnBrowseData(wxCommandEvent& event);
  void OnEditObject(wxCommandEvent& event);
  void OnGenerateDDL(wxCommandEvent& event);
  void OnProperties(wxCommandEvent& event);
  void OnRefresh(wxCommandEvent& event);
  void OnGenerateSelect(wxCommandEvent& event);
  void OnGenerateInsert(wxCommandEvent& event);
  void OnGenerateUpdate(wxCommandEvent& event);
  void OnGenerateDelete(wxCommandEvent& event);

private:
  // Backend (optional, can be set later)
  backend::SessionClient* session_client_{nullptr};

  // UI Components
  wxTextCtrl* filter_ctrl_{nullptr};
  wxGenericTreeCtrl* tree_ctrl_{nullptr};
  wxTimer filter_timer_;

  // State
  wxString current_filter_;
  wxTreeItemId root_item_;
  bool is_loading_{false};

  // Context menu item data
  wxTreeItemId context_menu_item_;

  // Callbacks
  ProjectNavigatorCallbacks callbacks_;
  ProjectNavigatorActions* action_handler_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
