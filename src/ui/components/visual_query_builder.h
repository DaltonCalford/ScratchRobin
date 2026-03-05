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
#include <vector>

#include <wx/panel.h>

namespace scratchrobin::ui {

/**
 * TableNode - Represents a table in the visual query builder
 */
struct TableNode {
  std::string table_name;
  std::string alias;
  int x{0};
  int y{0};
  int width{150};
  int height{100};
  std::vector<std::string> columns;
  std::vector<bool> selected_columns;
};

/**
 * JoinLink - Represents a join between two tables
 */
struct JoinLink {
  std::string from_table;
  std::string from_column;
  std::string to_table;
  std::string to_column;
  int join_type{0};  // 0=inner, 1=left, 2=right, 3=full
};

/**
 * VisualQueryBuilder - A visual query builder component
 */
class VisualQueryBuilder : public wxPanel {
 public:
  VisualQueryBuilder(wxWindow* parent, wxWindowID id = wxID_ANY);
  ~VisualQueryBuilder() override;

  // Add a table to the canvas
  void AddTable(const std::string& table_name, int x = 0, int y = 0);

  // Remove a table
  void RemoveTable(const std::string& table_name);

  // Add a join between tables
  void AddJoin(const std::string& from_table, const std::string& from_column,
               const std::string& to_table, const std::string& to_column,
               int join_type = 0);

  // Remove a join
  void RemoveJoin(size_t index);

  // Clear all tables and joins
  void Clear();

  // Generate SQL from the visual design
  wxString GenerateSql() const;

  // Load visual design from SQL (if possible)
  bool LoadFromSql(const wxString& sql);

  // Get current tables
  std::vector<TableNode> GetTables() const;

  // Get current joins
  std::vector<JoinLink> GetJoins() const;

  // Set available tables (for drag-drop)
  void SetAvailableTables(const std::vector<std::string>& tables);

  // Enable/disable drag-drop from external sources
  void EnableDragDrop(bool enable);

 private:
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  void OnPaint(wxPaintEvent& event);
  void OnMouseLeftDown(wxMouseEvent& event);
  void OnMouseLeftUp(wxMouseEvent& event);
  void OnMouseMove(wxMouseEvent& event);
  void OnMouseRightDown(wxMouseEvent& event);
  void OnSize(wxSizeEvent& event);

  void DrawTable(wxDC& dc, const TableNode& table);
  void DrawJoin(wxDC& dc, const JoinLink& join);
  TableNode* FindTableAt(int x, int y);
  int FindTableIndexAt(int x, int y);

  std::vector<TableNode> tables_;
  std::vector<JoinLink> joins_;
  std::vector<std::string> available_tables_;

  // Drag state
  int dragged_table_index_{-1};
  int drag_offset_x_{0};
  int drag_offset_y_{0};

  bool drag_drop_enabled_{true};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
