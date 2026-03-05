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

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <memory>
#include <string>

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * Object Metadata Dialog - New Implementation
 * Displays metadata information for database objects
 */
class ObjectMetadataDialogNew : public wxDialog {
 public:
  ObjectMetadataDialogNew(wxWindow* parent,
                          scratchrobin::backend::SessionClient* session_client,
                          const std::string& object_name,
                          const std::string& object_type);
  ~ObjectMetadataDialogNew() override;

  // Load metadata from database
  void LoadMetadata();

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnRefresh(wxCommandEvent& event);
  void OnClose(wxCommandEvent& event);
  void OnExportDDL(wxCommandEvent& event);
  void OnCopyToClipboard(wxCommandEvent& event);
  void OnTabChanged(wxBookCtrlEvent& event);

  // Metadata loading methods
  void LoadTableMetadata();
  void LoadViewMetadata();
  void LoadProcedureMetadata();
  void LoadFunctionMetadata();
  void LoadTriggerMetadata();
  void LoadIndexMetadata();
  void LoadDomainMetadata();

  // UI update methods
  void UpdateGeneralTab();
  void UpdateColumnsTab();
  void UpdateConstraintsTab();
  void UpdateIndexesTab();
  void UpdateDependenciesTab();
  void UpdateDDLTtab();
  void UpdatePermissionsTab();

  // Backend
  scratchrobin::backend::SessionClient* session_client_;

  // Object info
  std::string object_name_;
  std::string object_type_;
  std::string database_name_;

  // UI Components
  wxNotebook* notebook_{nullptr};
  
  // General tab
  wxStaticText* name_label_{nullptr};
  wxStaticText* type_label_{nullptr};
  wxStaticText* owner_label_{nullptr};
  wxStaticText* created_label_{nullptr};
  wxStaticText* modified_label_{nullptr};
  wxStaticText* description_label_{nullptr};

  // Columns tab
  wxListCtrl* columns_list_{nullptr};

  // Constraints tab
  wxListCtrl* constraints_list_{nullptr};

  // Indexes tab
  wxListCtrl* indexes_list_{nullptr};

  // Dependencies tab
  wxTreeCtrl* dependencies_tree_{nullptr};

  // DDL tab
  wxTextCtrl* ddl_text_{nullptr};

  // Permissions tab
  wxListCtrl* permissions_list_{nullptr};

  // State
  struct MetadataCache;
  std::unique_ptr<MetadataCache> cache_;
  bool is_loading_{false};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
