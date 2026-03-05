/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/project_navigator_new.h"

#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

// Additional menu IDs
enum {
  ID_MENU_GENERATE_SELECT = 2100,
  ID_MENU_GENERATE_INSERT,
  ID_MENU_GENERATE_UPDATE,
  ID_MENU_GENERATE_DELETE,
};

namespace scratchrobin::ui {

// Note: TreeNodeData is a public nested type in ProjectNavigator

// Menu IDs for context menus
enum {
  ID_MENU_CONNECT = 2000,
  ID_MENU_DISCONNECT,
  ID_MENU_NEW_DATABASE,
  ID_MENU_BROWSE_DATA,
  ID_MENU_EDIT_OBJECT,
  ID_MENU_GENERATE_DDL,
  ID_MENU_PROPERTIES,
  ID_MENU_REFRESH
};

wxBEGIN_EVENT_TABLE(ProjectNavigator, wxPanel)
  EVT_TREE_ITEM_EXPANDING(wxID_ANY, ProjectNavigator::OnItemExpanding)
  EVT_TREE_ITEM_COLLAPSING(wxID_ANY, ProjectNavigator::OnItemCollapsing)
  EVT_TREE_SEL_CHANGED(wxID_ANY, ProjectNavigator::OnSelectionChanged)
  EVT_TREE_ITEM_ACTIVATED(wxID_ANY, ProjectNavigator::OnItemActivated)
  EVT_TREE_ITEM_MENU(wxID_ANY, ProjectNavigator::OnContextMenu)
  EVT_TEXT(wxID_ANY, ProjectNavigator::OnFilterTextChanged)
  EVT_TIMER(wxID_ANY, ProjectNavigator::OnFilterTimer)

  // Context menu handlers
  EVT_MENU(ID_MENU_CONNECT, ProjectNavigator::OnConnect)
  EVT_MENU(ID_MENU_DISCONNECT, ProjectNavigator::OnDisconnect)
  EVT_MENU(ID_MENU_NEW_DATABASE, ProjectNavigator::OnNewDatabase)
  EVT_MENU(ID_MENU_BROWSE_DATA, ProjectNavigator::OnBrowseData)
  EVT_MENU(ID_MENU_EDIT_OBJECT, ProjectNavigator::OnEditObject)
  EVT_MENU(ID_MENU_GENERATE_DDL, ProjectNavigator::OnGenerateDDL)
  EVT_MENU(ID_MENU_PROPERTIES, ProjectNavigator::OnProperties)
  EVT_MENU(ID_MENU_REFRESH, ProjectNavigator::OnRefresh)
wxEND_EVENT_TABLE()

ProjectNavigator::ProjectNavigator(wxWindow* parent, const std::string& instance_id)
    : DockableWindow(parent, "project_navigator", instance_id),
      filter_timer_(this) {
}

ProjectNavigator::~ProjectNavigator() {
  // Cleanup
}

void ProjectNavigator::onWindowCreated() {
  CreateControls();
  SetupLayout();
  BindEvents();
  PopulateRoot();
}

wxSize ProjectNavigator::getDefaultSize() const {
  return wxSize(300, 600);
}

void ProjectNavigator::CreateControls() {
  // Filter control
  filter_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, 
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_PROCESS_ENTER);
  filter_ctrl_->SetHint(_("Filter objects..."));

  // Tree control
  long tree_style = wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT |
                    wxTR_SINGLE | wxBORDER_NONE;
  tree_ctrl_ = new wxGenericTreeCtrl(this, wxID_ANY, wxDefaultPosition, 
                                      wxDefaultSize, tree_style);
}

void ProjectNavigator::SetupLayout() {
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  
  sizer->Add(filter_ctrl_, 0, wxEXPAND | wxALL, 2);
  sizer->Add(tree_ctrl_, 1, wxEXPAND);
  
  SetSizer(sizer);
}

void ProjectNavigator::BindEvents() {
  // Additional event bindings if needed
}

void ProjectNavigator::PopulateRoot() {
  tree_ctrl_->DeleteAllItems();
  
  // Create hidden root
  root_item_ = tree_ctrl_->AddRoot(_("Servers"), -1, -1,
                                    new TreeNodeData(TreeNodeData::NodeType::kRoot));
  
  // Add sample servers (in real implementation, load from registry)
  wxTreeItemId server1 = tree_ctrl_->AppendItem(root_item_, _("localhost"),
                                                 -1, -1,
                                                 new TreeNodeData(TreeNodeData::NodeType::kServer));
  
  wxTreeItemId server2 = tree_ctrl_->AppendItem(root_item_, _("Production DB"),
                                                 -1, -1,
                                                 new TreeNodeData(TreeNodeData::NodeType::kServer));
  
  tree_ctrl_->Expand(root_item_);
}

void ProjectNavigator::LoadChildren(wxTreeItemId parent) {
  TreeNodeData* data = dynamic_cast<TreeNodeData*>(tree_ctrl_->GetItemData(parent));
  if (!data || data->loaded) {
    return;
  }
  
  // Show loading indicator
  wxString original_text = tree_ctrl_->GetItemText(parent);
  tree_ctrl_->SetItemText(parent, original_text + _(" (loading...)"));
  
  // In real implementation, this would be async with callback
  // For now, simulate loading based on node type
  switch (data->type) {
    case TreeNodeData::NodeType::kServer: {
      // Add databases
      wxTreeItemId db = tree_ctrl_->AppendItem(parent, _("scratchbird_db"),
                                                -1, -1,
                                                new TreeNodeData(TreeNodeData::NodeType::kDatabase));
      break;
    }
      
    case TreeNodeData::NodeType::kDatabase: {
      // Add schema folder
      wxTreeItemId schemas = tree_ctrl_->AppendItem(parent, _("Schemas"),
                                                     -1, -1,
                                                     new TreeNodeData(TreeNodeData::NodeType::kSchema));
      
      // Add system objects
      wxTreeItemId system = tree_ctrl_->AppendItem(parent, _("System Objects"),
                                                    -1, -1,
                                                    new TreeNodeData(TreeNodeData::NodeType::kSchema));
      break;
    }
      
    case TreeNodeData::NodeType::kSchema: {
      // Add object type folders
      wxTreeItemId tables = tree_ctrl_->AppendItem(parent, _("Tables"),
                                                    -1, -1,
                                                    new TreeNodeData(TreeNodeData::NodeType::kTablesFolder));
      
      wxTreeItemId views = tree_ctrl_->AppendItem(parent, _("Views"),
                                                   -1, -1,
                                                   new TreeNodeData(TreeNodeData::NodeType::kViewsFolder));
      
      wxTreeItemId procedures = tree_ctrl_->AppendItem(parent, _("Procedures"),
                                                        -1, -1,
                                                        new TreeNodeData(TreeNodeData::NodeType::kProceduresFolder));
      
      wxTreeItemId functions = tree_ctrl_->AppendItem(parent, _("Functions"),
                                                       -1, -1,
                                                       new TreeNodeData(TreeNodeData::NodeType::kFunctionsFolder));
      break;
    }
      
    case TreeNodeData::NodeType::kTablesFolder: {
      // Add sample tables
      const char* tables[] = {"users", "orders", "order_items", "products", "customers"};
      for (const auto& table : tables) {
        wxTreeItemId item = tree_ctrl_->AppendItem(parent, wxString::FromUTF8(table),
                                                    -1, -1,
                                                    new TreeNodeData(TreeNodeData::NodeType::kTable));
      }
      break;
    }
      
    case TreeNodeData::NodeType::kViewsFolder: {
      const char* views[] = {"active_users", "monthly_sales", "product_summary"};
      for (const auto& view : views) {
        tree_ctrl_->AppendItem(parent, wxString::FromUTF8(view),
                                -1, -1,
                                new TreeNodeData(TreeNodeData::NodeType::kView));
      }
      break;
    }
      
    case TreeNodeData::NodeType::kProceduresFolder: {
      const char* procs[] = {"calculate_total", "process_order", "update_inventory"};
      for (const auto& proc : procs) {
        tree_ctrl_->AppendItem(parent, wxString::FromUTF8(proc),
                                -1, -1,
                                new TreeNodeData(TreeNodeData::NodeType::kProcedure));
      }
      break;
    }
      
    case TreeNodeData::NodeType::kFunctionsFolder: {
      const char* funcs[] = {"get_user_name", "calculate_tax", "format_currency"};
      for (const auto& func : funcs) {
        tree_ctrl_->AppendItem(parent, wxString::FromUTF8(func),
                                -1, -1,
                                new TreeNodeData(TreeNodeData::NodeType::kFunction));
      }
      break;
    }
      
    case TreeNodeData::NodeType::kTable: {
      // Add columns folder
      wxTreeItemId columns = tree_ctrl_->AppendItem(parent, _("Columns"),
                                                     -1, -1,
                                                     new TreeNodeData(TreeNodeData::NodeType::kColumnsFolder));
      
      // Add sample columns
      const char* cols[] = {"id (INTEGER)", "name (VARCHAR)", "created_at (TIMESTAMP)"};
      for (const auto& col : cols) {
        tree_ctrl_->AppendItem(columns, wxString::FromUTF8(col),
                                -1, -1,
                                new TreeNodeData(TreeNodeData::NodeType::kColumn));
      }
      
      // Add indexes folder
      wxTreeItemId indexes = tree_ctrl_->AppendItem(parent, _("Indexes"),
                                                     -1, -1,
                                                     new TreeNodeData(TreeNodeData::NodeType::kIndexesFolder));
      
      // Add triggers folder
      wxTreeItemId triggers = tree_ctrl_->AppendItem(parent, _("Triggers"),
                                                      -1, -1,
                                                      new TreeNodeData(TreeNodeData::NodeType::kTriggersFolder));
      break;
    }
      
    default:
      break;
  }
  
  // Restore original text and mark as loaded
  tree_ctrl_->SetItemText(parent, original_text);
  data->loaded = true;
}

void ProjectNavigator::OnItemExpanding(wxTreeEvent& event) {
  wxTreeItemId item = event.GetItem();
  LoadChildren(item);
}

void ProjectNavigator::OnItemCollapsing(wxTreeEvent& event) {
  // Could unload children here if memory is a concern
}

void ProjectNavigator::OnSelectionChanged(wxTreeEvent& event) {
  wxTreeItemId item = event.GetItem();
  TreeNodeData* data = dynamic_cast<TreeNodeData*>(tree_ctrl_->GetItemData(item));
  
  if (data) {
    // Notify main frame or other components about selection change
    // This could fire an event or call a callback
  }
}

void ProjectNavigator::OnItemActivated(wxTreeEvent& event) {
  wxTreeItemId item = event.GetItem();
  TreeNodeData* data = dynamic_cast<TreeNodeData*>(tree_ctrl_->GetItemData(item));
  
  if (!data) return;
  
  // Double-click action based on type
  switch (data->type) {
    case TreeNodeData::NodeType::kTable:
      OnBrowseData(event);
      break;
    case TreeNodeData::NodeType::kView:
      OnBrowseData(event);
      break;
    case TreeNodeData::NodeType::kProcedure:
    case TreeNodeData::NodeType::kFunction:
      OnEditObject(event);
      break;
    default:
      // Toggle expansion
      if (tree_ctrl_->IsExpanded(item)) {
        tree_ctrl_->Collapse(item);
      } else {
        tree_ctrl_->Expand(item);
      }
      break;
  }
}

void ProjectNavigator::OnContextMenu(wxTreeEvent& event) {
  wxTreeItemId item = event.GetItem();
  context_menu_item_ = item;
  
  TreeNodeData* data = dynamic_cast<TreeNodeData*>(tree_ctrl_->GetItemData(item));
  if (!data) return;
  
  // Show appropriate menu based on node type
  switch (data->type) {
    case TreeNodeData::NodeType::kServer:
      ShowServerMenu(item);
      break;
    case TreeNodeData::NodeType::kDatabase:
      ShowDatabaseMenu(item);
      break;
    case TreeNodeData::NodeType::kTablesFolder:
      ShowTableFolderMenu(item);
      break;
    case TreeNodeData::NodeType::kTable:
      ShowTableMenu(item);
      break;
    case TreeNodeData::NodeType::kView:
      ShowViewMenu(item);
      break;
    case TreeNodeData::NodeType::kProcedure:
      ShowProcedureMenu(item);
      break;
    default:
      break;
  }
}

void ProjectNavigator::ShowServerMenu(wxTreeItemId item) {
  wxMenu menu;
  menu.Append(ID_MENU_CONNECT, _("&Connect"), _("Connect to this server"));
  menu.Append(ID_MENU_DISCONNECT, _("&Disconnect"), _("Disconnect from this server"));
  menu.AppendSeparator();
  menu.Append(ID_MENU_NEW_DATABASE, _("&New Database..."), _("Create a new database"));
  menu.AppendSeparator();
  menu.Append(ID_MENU_REFRESH, _("&Refresh"), _("Refresh server contents"));
  menu.AppendSeparator();
  menu.Append(ID_MENU_PROPERTIES, _("&Properties..."), _("Edit server properties"));
  
  PopupMenu(&menu);
}

void ProjectNavigator::ShowDatabaseMenu(wxTreeItemId item) {
  wxMenu menu;
  menu.Append(ID_MENU_CONNECT, _("&Connect"), _("Connect to this database"));
  menu.Append(ID_MENU_DISCONNECT, _("&Disconnect"), _("Disconnect from this database"));
  menu.AppendSeparator();
  menu.Append(ID_MENU_REFRESH, _("&Refresh"), _("Refresh database contents"));
  menu.AppendSeparator();
  menu.Append(ID_MENU_GENERATE_DDL, _("&Generate DDL"), _("Generate DDL script"));
  menu.Append(ID_MENU_PROPERTIES, _("&Properties..."), _("Database properties"));
  
  PopupMenu(&menu);
}

void ProjectNavigator::ShowTableFolderMenu(wxTreeItemId item) {
  wxMenu menu;
  menu.Append(ID_MENU_NEW_DATABASE, _("&New Table..."), _("Create a new table"));
  menu.Append(ID_MENU_REFRESH, _("&Refresh"), _("Refresh table list"));
  
  PopupMenu(&menu);
}

void ProjectNavigator::ShowTableMenu(wxTreeItemId item) {
  wxMenu menu;
  menu.Append(ID_MENU_BROWSE_DATA, _("&Browse Data"), _("Open table data editor"));
  menu.AppendSeparator();
  menu.Append(ID_MENU_EDIT_OBJECT, _("&Edit Table..."), _("Modify table structure"));
  menu.Append(ID_MENU_GENERATE_DDL, _("&Generate DDL"), _("Generate CREATE TABLE"));
  
  // SQL Generation submenu
  wxMenu* sql_menu = new wxMenu();
  sql_menu->Append(ID_MENU_GENERATE_SELECT, _("SELECT * FROM..."), _("Generate SELECT statement"));
  sql_menu->Append(ID_MENU_GENERATE_INSERT, _("INSERT INTO..."), _("Generate INSERT statement"));
  sql_menu->Append(ID_MENU_GENERATE_UPDATE, _("UPDATE..."), _("Generate UPDATE statement"));
  sql_menu->Append(ID_MENU_GENERATE_DELETE, _("DELETE FROM..."), _("Generate DELETE statement"));
  menu.AppendSubMenu(sql_menu, _("&Generate SQL"), _("Generate SQL statements"));
  
  menu.AppendSeparator();
  menu.Append(ID_MENU_REFRESH, _("&Refresh"), _("Refresh table information"));
  menu.Append(ID_MENU_PROPERTIES, _("&Properties..."), _("Table properties"));
  
  // Bind SQL generation events
  Bind(wxEVT_MENU, &ProjectNavigator::OnGenerateSelect, this, ID_MENU_GENERATE_SELECT);
  Bind(wxEVT_MENU, &ProjectNavigator::OnGenerateInsert, this, ID_MENU_GENERATE_INSERT);
  Bind(wxEVT_MENU, &ProjectNavigator::OnGenerateUpdate, this, ID_MENU_GENERATE_UPDATE);
  Bind(wxEVT_MENU, &ProjectNavigator::OnGenerateDelete, this, ID_MENU_GENERATE_DELETE);
  
  PopupMenu(&menu);
}

void ProjectNavigator::ShowViewMenu(wxTreeItemId item) {
  wxMenu menu;
  menu.Append(ID_MENU_BROWSE_DATA, _("&Browse Data"), _("View data"));
  menu.AppendSeparator();
  menu.Append(ID_MENU_EDIT_OBJECT, _("&Edit View..."), _("Modify view definition"));
  menu.Append(ID_MENU_GENERATE_DDL, _("&Generate DDL"), _("Generate CREATE VIEW"));
  menu.Append(ID_MENU_PROPERTIES, _("&Properties..."), _("View properties"));
  
  PopupMenu(&menu);
}

void ProjectNavigator::ShowProcedureMenu(wxTreeItemId item) {
  wxMenu menu;
  menu.Append(ID_MENU_EDIT_OBJECT, _("&Edit..."), _("Edit procedure"));
  menu.Append(ID_MENU_GENERATE_DDL, _("&Generate DDL"), _("Generate CREATE PROCEDURE"));
  menu.Append(ID_MENU_PROPERTIES, _("&Properties..."), _("Procedure properties"));
  
  PopupMenu(&menu);
}

// Filter handling
void ProjectNavigator::OnFilterTextChanged(wxCommandEvent& event) {
  // Restart timer to debounce filter input
  filter_timer_.Stop();
  filter_timer_.Start(300, wxTIMER_ONE_SHOT);
}

void ProjectNavigator::OnFilterTimer(wxTimerEvent& event) {
  current_filter_ = filter_ctrl_->GetValue();
  ApplyFilter();
}

void ProjectNavigator::ApplyFilter() {
  // In a real implementation, this would filter the tree
  // For now, just show a message
  if (!current_filter_.IsEmpty()) {
    // TODO: Implement tree filtering
  }
}

void ProjectNavigator::SetFilter(const wxString& filter) {
  filter_ctrl_->SetValue(filter);
  current_filter_ = filter;
  ApplyFilter();
}

void ProjectNavigator::ClearFilter() {
  filter_ctrl_->Clear();
  current_filter_.Clear();
  ApplyFilter();
}

// Public interface
void ProjectNavigator::refreshTree() {
  PopulateRoot();
}

void ProjectNavigator::ExpandNode(wxTreeItemId item) {
  tree_ctrl_->Expand(item);
}

void ProjectNavigator::CollapseNode(wxTreeItemId item) {
  tree_ctrl_->Collapse(item);
}

wxTreeItemId ProjectNavigator::GetSelectedItem() const {
  return tree_ctrl_->GetSelection();
}

void ProjectNavigator::SelectItem(wxTreeItemId item) {
  tree_ctrl_->SelectItem(item);
}

// Action helpers
std::string ProjectNavigator::GetItemName(wxTreeItemId item) {
  return tree_ctrl_->GetItemText(item).ToStdString();
}

std::string ProjectNavigator::GetItemDatabase(wxTreeItemId item) {
  // Walk up tree to find database node
  wxTreeItemId current = item;
  while (current.IsOk() && current != root_item_) {
    TreeNodeData* data = dynamic_cast<TreeNodeData*>(tree_ctrl_->GetItemData(current));
    if (data && data->type == TreeNodeData::NodeType::kDatabase) {
      return tree_ctrl_->GetItemText(current).ToStdString();
    }
    current = tree_ctrl_->GetItemParent(current);
  }
  return "";
}

std::string ProjectNavigator::GetItemType(wxTreeItemId item) {
  TreeNodeData* data = dynamic_cast<TreeNodeData*>(tree_ctrl_->GetItemData(item));
  if (!data) return "";
  
  switch (data->type) {
    case TreeNodeData::NodeType::kTable: return "TABLE";
    case TreeNodeData::NodeType::kView: return "VIEW";
    case TreeNodeData::NodeType::kProcedure: return "PROCEDURE";
    case TreeNodeData::NodeType::kFunction: return "FUNCTION";
    case TreeNodeData::NodeType::kTrigger: return "TRIGGER";
    default: return "";
  }
}

// Action handlers
void ProjectNavigator::OnConnect(wxCommandEvent& event) {
  if (callbacks_.on_connect) {
    callbacks_.on_connect(GetItemName(context_menu_item_));
  } else if (action_handler_) {
    action_handler_->OnConnectToServer(GetItemName(context_menu_item_));
  } else {
    wxMessageBox(_("Connect dialog would open"), _("Connect"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnDisconnect(wxCommandEvent& event) {
  if (callbacks_.on_disconnect) {
    callbacks_.on_disconnect(GetItemName(context_menu_item_));
  } else if (action_handler_) {
    action_handler_->OnDisconnectFromServer(GetItemName(context_menu_item_));
  } else {
    wxMessageBox(_("Disconnect would happen"), _("Disconnect"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnNewDatabase(wxCommandEvent& event) {
  if (callbacks_.on_new_connection) {
    callbacks_.on_new_connection();
  } else if (action_handler_) {
    action_handler_->OnNewConnection();
  } else {
    wxMessageBox(_("New database dialog would open"), _("New Database"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnBrowseData(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  std::string type = GetItemType(context_menu_item_);
  
  if (type == "TABLE" && callbacks_.on_browse_table) {
    callbacks_.on_browse_table(name, database);
  } else if (type == "VIEW" && callbacks_.on_browse_view) {
    callbacks_.on_browse_view(name, database);
  } else if (action_handler_) {
    if (type == "TABLE") {
      action_handler_->OnBrowseTableData(name, database);
    } else if (type == "VIEW") {
      action_handler_->OnBrowseViewData(name, database);
    }
  } else {
    wxMessageBox(wxString::Format(_("Browse data for: %s"), name),
                 _("Browse Data"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnEditObject(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  std::string type = GetItemType(context_menu_item_);
  
  if (action_handler_) {
    if (type == "TABLE") {
      action_handler_->OnEditTable(name, database);
    } else if (type == "VIEW") {
      action_handler_->OnEditView(name, database);
    } else if (type == "PROCEDURE") {
      action_handler_->OnEditProcedure(name, database);
    } else if (type == "FUNCTION") {
      action_handler_->OnEditFunction(name, database);
    }
  } else {
    wxMessageBox(_("Edit dialog would open"), _("Edit Object"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnGenerateDDL(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  std::string type = GetItemType(context_menu_item_);
  
  if (action_handler_) {
    action_handler_->OnShowObjectMetadata(name, type, database);
  } else {
    wxMessageBox(_("DDL would be generated"), _("Generate DDL"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnProperties(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  std::string type = GetItemType(context_menu_item_);
  
  if (callbacks_.on_show_metadata) {
    callbacks_.on_show_metadata(name, type, database);
  } else if (action_handler_) {
    action_handler_->OnShowObjectMetadata(name, type, database);
  } else {
    wxMessageBox(_("Properties dialog would open"), _("Properties"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnRefresh(wxCommandEvent& event) {
  if (!tree_ctrl_ || !context_menu_item_.IsOk()) return;
  TreeNodeData* data = dynamic_cast<TreeNodeData*>(tree_ctrl_->GetItemData(context_menu_item_));
  if (data) {
    data->loaded = false;  // Force reload
    LoadChildren(context_menu_item_);
  }
}

void ProjectNavigator::OnGenerateSelect(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  
  if (callbacks_.on_generate_select) {
    callbacks_.on_generate_select(name, database);
  } else if (action_handler_) {
    action_handler_->OnGenerateSelectSQL(name, database);
  } else {
    wxMessageBox(wxString::Format(_("Generate SELECT for: %s"), name),
                 _("Generate SQL"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnGenerateInsert(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  
  if (action_handler_) {
    action_handler_->OnGenerateInsertSQL(name, database);
  } else {
    wxMessageBox(wxString::Format(_("Generate INSERT for: %s"), name),
                 _("Generate SQL"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnGenerateUpdate(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  
  if (action_handler_) {
    action_handler_->OnGenerateUpdateSQL(name, database);
  } else {
    wxMessageBox(wxString::Format(_("Generate UPDATE for: %s"), name),
                 _("Generate SQL"), wxOK | wxICON_INFORMATION);
  }
}

void ProjectNavigator::OnGenerateDelete(wxCommandEvent& event) {
  std::string name = GetItemName(context_menu_item_);
  std::string database = GetItemDatabase(context_menu_item_);
  
  if (action_handler_) {
    action_handler_->OnGenerateDeleteSQL(name, database);
  } else {
    wxMessageBox(wxString::Format(_("Generate DELETE for: %s"), name),
                 _("Generate SQL"), wxOK | wxICON_INFORMATION);
  }
}

}  // namespace scratchrobin::ui
