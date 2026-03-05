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

#include <string>
#include <unordered_map>

#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/frame.h>
#include <wx/imaglist.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>
#include <wx/generic/treectlg.h>

#include "core/settings.h"

#include "backend/preview_object_metadata_store.h"
#include "backend/preview_metadata_store.h"
#include "backend/scratchbird_runtime_config.h"
#include "backend/session_client.h"

namespace scratchrobin::ui {

class SqlEditorFrame;

class MainFrame final : public wxFrame {
 public:
  explicit MainFrame(backend::SessionClient* session_client);

 private:
  enum class ControlId {
    kMenuOpenSqlEditor = wxID_HIGHEST + 200,
    kMenuRegisterDatabase,
    kMenuCreateDatabase,
    kMenuRegisterServer,
    kMenuServerProperties,
    kMenuNewTable,
    kMenuNewView,
    kMenuNewProcedure,
    kMenuNewFunction,
    kMenuNewSequence,
    kMenuNewTrigger,
    kMenuNewDomain,
    kMenuNewPackage,
    kMenuNewJob,
    kMenuNewSchedule,
    kMenuNewUser,
    kMenuNewRole,
    kMenuNewGroup,
    kMenuNewObject,
    kMenuObjectProperties,
    kMenuToggleStatusBar,
    kMenuToggleSearchBar,
    kMenuToggleDebugMode,
    kMenuSettings,
    kMenuModePreview,
    kMenuModeManaged,
    kMenuModeListenerOnly,
    kMenuModeIpcOnly,
    kMenuModeEmbedded,

    kSearchBox,
    kSearchPrev,
    kSearchNext,
    kSearchAdvanced,
  };

  void BuildMainMenu();
  void BuildMainLayout();
  void BuildTreeImageList();
  void PopulateTree();
  void BindEvents();
  void UpdateStatusbarText();
  void ApplyModeSelection(backend::TransportMode mode);
  void EnsureSqlEditor();
  void ApplySearchNavigation(bool next);
  void ApplySearchFilter();
  void ApplySettings();
  void OnSettingsChanged();

  void OnMenuOpenSqlEditor(wxCommandEvent& event);
  void OnMenuToggleStatusBar(wxCommandEvent& event);
  void OnMenuToggleSearchBar(wxCommandEvent& event);
  void OnMenuToggleDebugMode(wxCommandEvent& event);
  void OnMenuSettings(wxCommandEvent& event);
  void OnMenuModePreview(wxCommandEvent& event);
  void OnMenuModeManaged(wxCommandEvent& event);
  void OnMenuModeListenerOnly(wxCommandEvent& event);
  void OnMenuModeIpcOnly(wxCommandEvent& event);
  void OnMenuModeEmbedded(wxCommandEvent& event);
  void OnMenuNewTable(wxCommandEvent& event);
  void OnMenuNewView(wxCommandEvent& event);
  void OnMenuNewProcedure(wxCommandEvent& event);
  void OnMenuNewFunction(wxCommandEvent& event);
  void OnMenuNewSequence(wxCommandEvent& event);
  void OnMenuNewTrigger(wxCommandEvent& event);
  void OnMenuNewDomain(wxCommandEvent& event);
  void OnMenuNewPackage(wxCommandEvent& event);
  void OnMenuNewJob(wxCommandEvent& event);
  void OnMenuNewSchedule(wxCommandEvent& event);
  void OnMenuNewUser(wxCommandEvent& event);
  void OnMenuNewRole(wxCommandEvent& event);
  void OnMenuNewGroup(wxCommandEvent& event);
  void OnMenuNewObject(wxCommandEvent& event);
  void OnMenuObjectProperties(wxCommandEvent& event);
  void OnMenuPlaceholder(wxCommandEvent& event);
  void OnMenuAbout(wxCommandEvent& event);

  void OnTreeSelectionChanged(wxTreeEvent& event);
  void OnTreeItemActivated(wxTreeEvent& event);

  void OnSearchTextChange(wxCommandEvent& event);
  void OnSearchEnter(wxCommandEvent& event);
  void OnSearchPrev(wxCommandEvent& event);
  void OnSearchNext(wxCommandEvent& event);
  void OnSearchAdvanced(wxCommandEvent& event);

  bool GetSelectedTableIdentity(std::string* schema_path, std::string* table_name) const;
  bool GetTableIdentityForItem(const wxTreeItemId& item, std::string* schema_path,
                               std::string* table_name) const;
  bool GetSelectedObjectIdentity(std::string* schema_path, std::string* object_name,
                                 std::string* object_kind) const;
  std::string GetSelectedSchemaPath() const;
  void OpenTableMetadataDialog(bool create_new, bool view_only);
  void OpenObjectMetadataDialog(bool create_new, bool view_only,
                                const std::string& create_kind = {});
  bool SavePreviewTableMetadataRecord(const backend::PreviewTableMetadata& metadata);
  bool SavePreviewObjectMetadataRecord(const backend::PreviewObjectMetadata& metadata);
  static std::string MakeTableMetadataKey(const std::string& schema_path,
                                          const std::string& table_name);
  static std::string MakeObjectMetadataKey(const std::string& schema_path,
                                           const std::string& object_name,
                                           const std::string& object_kind);

  backend::SessionClient* session_client_;
  backend::ScratchbirdRuntimeConfig runtime_config_;

  wxMenuBar* menu_bar_{nullptr};
  wxMenu* view_menu_{nullptr};
  wxMenuItem* toggle_status_item_{nullptr};
  wxMenuItem* toggle_search_item_{nullptr};
  wxMenuItem* toggle_debug_mode_item_{nullptr};
  wxMenuItem* mode_preview_item_{nullptr};
  wxMenuItem* mode_managed_item_{nullptr};
  wxMenuItem* mode_listener_only_item_{nullptr};
  wxMenuItem* mode_ipc_only_item_{nullptr};
  wxMenuItem* mode_embedded_item_{nullptr};

  wxPanel* main_panel_{nullptr};
  wxGenericTreeCtrl* object_tree_{nullptr};
  wxPanel* search_panel_{nullptr};
  wxBoxSizer* search_panel_sizer_{nullptr};
  wxComboBox* search_box_{nullptr};
  wxBitmapButton* button_prev_{nullptr};
  wxBitmapButton* button_next_{nullptr};
  wxBitmapButton* button_advanced_{nullptr};
  wxImageList* tree_image_list_{nullptr};

  int icon_server_{-1};
  int icon_database_{-1};
  int icon_folder_{-1};
  int icon_schema_{-1};
  int icon_table_{-1};
  int icon_view_{-1};
  int icon_procedure_{-1};
  int icon_function_{-1};
  int icon_generator_{-1};
  int icon_trigger_{-1};
  int icon_domain_{-1};
  int icon_package_{-1};
  int icon_job_{-1};
  int icon_schedule_{-1};
  int icon_user_{-1};
  int icon_role_{-1};
  int icon_group_{-1};
  int icon_branch_columns_{-1};
  int icon_branch_indexes_{-1};
  int icon_branch_triggers_{-1};
  int icon_column_{-1};
  int icon_column_key_{-1};
  int icon_index_{-1};
  int icon_table_trigger_{-1};

  wxTreeItemId tree_root_;
  wxTreeItemId tree_database_;
  std::unordered_map<std::string, backend::PreviewTableMetadata> table_metadata_by_key_;
  std::unordered_map<std::string, backend::PreviewObjectMetadata> object_metadata_by_key_;
  bool debug_mode_enabled_{true};

  SqlEditorFrame* sql_editor_frame_{nullptr};
};

}  // namespace scratchrobin::ui
