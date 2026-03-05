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

#include <functional>
#include <string>

namespace scratchrobin::ui {

// Forward declarations
class ProjectNavigator;

/**
 * Callback functions for Project Navigator actions
 * These allow loose coupling between the navigator and main frame
 */
struct ProjectNavigatorCallbacks {
  std::function<void(const std::string& server_name)> on_connect;
  std::function<void(const std::string& server_name)> on_disconnect;
  std::function<void()> on_new_connection;
  std::function<void(const std::string& database, const std::string& table)> on_browse_data;
  std::function<void(const std::string& database, const std::string& table)> on_browse_table;
  std::function<void(const std::string& database, const std::string& view)> on_browse_view;
  std::function<void(const std::string& database, const std::string& object, const std::string& type)> on_edit_object;
  std::function<void(const std::string& database, const std::string& object, const std::string& type)> on_show_metadata;
  std::function<void()> on_refresh;
  std::function<void(const std::string& database, const std::string& table)> on_generate_select;
  std::function<void(const std::string& database, const std::string& table)> on_generate_insert;
  std::function<void(const std::string& database, const std::string& table)> on_generate_update;
  std::function<void(const std::string& database, const std::string& table)> on_generate_delete;
  std::function<void(const std::string& database, const std::string& object, const std::string& type)> on_generate_ddl;
};

/**
 * Action handler interface for Project Navigator
 * Alternative to callbacks for more complex scenarios
 */
class ProjectNavigatorActions {
 public:
  virtual ~ProjectNavigatorActions() = default;
  
  virtual void OnConnectToServer(const std::string& server_id) {}
  virtual void OnDisconnectFromServer(const std::string& server_id) {}
  virtual void OnNewConnection() {}
  virtual void OnBrowseTableData(const std::string& table_name, 
                                  const std::string& database) {}
  virtual void OnBrowseViewData(const std::string& view_name,
                                 const std::string& database) {}
  virtual void OnEditTable(const std::string& table_name,
                            const std::string& database) {}
  virtual void OnEditView(const std::string& view_name,
                           const std::string& database) {}
  virtual void OnEditProcedure(const std::string& proc_name,
                                const std::string& database) {}
  virtual void OnEditFunction(const std::string& func_name,
                               const std::string& database) {}
  virtual void OnShowObjectMetadata(const std::string& object_name,
                                     const std::string& object_type,
                                     const std::string& database) {}
  virtual void OnGenerateSelectSQL(const std::string& table_name,
                                    const std::string& database) {}
  virtual void OnGenerateInsertSQL(const std::string& table_name,
                                    const std::string& database) {}
  virtual void OnGenerateUpdateSQL(const std::string& table_name,
                                    const std::string& database) {}
  virtual void OnGenerateDeleteSQL(const std::string& table_name,
                                    const std::string& database) {}
  virtual void OnDragObjectName(const std::string& object_name) {}
};

}  // namespace scratchrobin::ui
