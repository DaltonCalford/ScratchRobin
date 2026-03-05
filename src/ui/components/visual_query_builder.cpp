/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/components/visual_query_builder.h"

#include <algorithm>
#include <sstream>

#include <wx/dcbuffer.h>
#include <wx/brush.h>
#include <wx/pen.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(VisualQueryBuilder, wxPanel)
  EVT_PAINT(VisualQueryBuilder::OnPaint)
  EVT_LEFT_DOWN(VisualQueryBuilder::OnMouseLeftDown)
  EVT_LEFT_UP(VisualQueryBuilder::OnMouseLeftUp)
  EVT_MOTION(VisualQueryBuilder::OnMouseMove)
  EVT_RIGHT_DOWN(VisualQueryBuilder::OnMouseRightDown)
  EVT_SIZE(VisualQueryBuilder::OnSize)
wxEND_EVENT_TABLE()

VisualQueryBuilder::VisualQueryBuilder(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE | wxBORDER_SUNKEN) {
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(wxColor(240, 240, 240));
}

VisualQueryBuilder::~VisualQueryBuilder() {
  // Cleanup handled by wxWidgets
}

void VisualQueryBuilder::CreateControls() {
  // Controls are created in constructor
}

void VisualQueryBuilder::SetupLayout() {
  // Layout is handled by the panel itself
}

void VisualQueryBuilder::BindEvents() {
  // Events are bound via event table
}

void VisualQueryBuilder::AddTable(const std::string& table_name, int x, int y) {
  TableNode node;
  node.table_name = table_name;
  node.alias = table_name.substr(0, 1);
  node.x = x;
  node.y = y;
  
  // TODO: Fetch columns from database metadata
  node.columns = {"*"};  // Placeholder
  node.selected_columns.resize(node.columns.size(), false);
  node.selected_columns[0] = true;  // Select * by default
  
  tables_.push_back(node);
  Refresh();
}

void VisualQueryBuilder::RemoveTable(const std::string& table_name) {
  // Remove joins involving this table first
  joins_.erase(
    std::remove_if(joins_.begin(), joins_.end(),
      [&table_name](const JoinLink& join) {
        return join.from_table == table_name || join.to_table == table_name;
      }),
    joins_.end()
  );
  
  // Remove the table
  tables_.erase(
    std::remove_if(tables_.begin(), tables_.end(),
      [&table_name](const TableNode& node) {
        return node.table_name == table_name;
      }),
    tables_.end()
  );
  
  Refresh();
}

void VisualQueryBuilder::AddJoin(const std::string& from_table, 
                                  const std::string& from_column,
                                  const std::string& to_table, 
                                  const std::string& to_column,
                                  int join_type) {
  JoinLink join;
  join.from_table = from_table;
  join.from_column = from_column;
  join.to_table = to_table;
  join.to_column = to_column;
  join.join_type = join_type;
  
  joins_.push_back(join);
  Refresh();
}

void VisualQueryBuilder::RemoveJoin(size_t index) {
  if (index < joins_.size()) {
    joins_.erase(joins_.begin() + index);
    Refresh();
  }
}

void VisualQueryBuilder::Clear() {
  tables_.clear();
  joins_.clear();
  Refresh();
}

wxString VisualQueryBuilder::GenerateSql() const {
  if (tables_.empty()) {
    return wxEmptyString;
  }
  
  std::ostringstream sql;
  
  sql << "SELECT ";
  
  // Add columns
  bool first_column = true;
  for (const auto& table : tables_) {
    for (size_t i = 0; i < table.columns.size(); ++i) {
      if (table.selected_columns[i]) {
        if (!first_column) {
          sql << ", ";
        }
        if (table.columns[i] == "*") {
          sql << table.alias << ".*";
        } else {
          sql << table.alias << "." << table.columns[i];
        }
        first_column = false;
      }
    }
  }
  
  if (first_column) {
    sql << "*";
  }
  
  // Add FROM clause
  sql << " FROM ";
  bool first_table = true;
  for (const auto& table : tables_) {
    if (!first_table) {
      sql << ", ";
    }
    sql << table.table_name << " " << table.alias;
    first_table = false;
  }
  
  // Add JOIN clauses
  for (const auto& join : joins_) {
    const char* join_type_str = "INNER JOIN";
    if (join.join_type == 1) join_type_str = "LEFT JOIN";
    else if (join.join_type == 2) join_type_str = "RIGHT JOIN";
    else if (join.join_type == 3) join_type_str = "FULL JOIN";
    
    // Find aliases
    std::string from_alias, to_alias;
    for (const auto& table : tables_) {
      if (table.table_name == join.from_table) from_alias = table.alias;
      if (table.table_name == join.to_table) to_alias = table.alias;
    }
    
    sql << " " << join_type_str << " " << join.to_table << " " << to_alias
        << " ON " << from_alias << "." << join.from_column
        << " = " << to_alias << "." << join.to_column;
  }
  
  return wxString::FromUTF8(sql.str().c_str());
}

bool VisualQueryBuilder::LoadFromSql(const wxString& sql) {
  // TODO: Implement SQL parsing to visual design
  // This is a complex feature that would require a SQL parser
  Clear();
  return false;
}

std::vector<TableNode> VisualQueryBuilder::GetTables() const {
  return tables_;
}

std::vector<JoinLink> VisualQueryBuilder::GetJoins() const {
  return joins_;
}

void VisualQueryBuilder::SetAvailableTables(const std::vector<std::string>& tables) {
  available_tables_ = tables;
}

void VisualQueryBuilder::EnableDragDrop(bool enable) {
  drag_drop_enabled_ = enable;
}

void VisualQueryBuilder::OnPaint(wxPaintEvent& event) {
  wxAutoBufferedPaintDC dc(this);
  
  // Clear background
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
  
  // Draw grid lines
  dc.SetPen(wxPen(wxColor(220, 220, 220)));
  for (int x = 0; x < GetClientSize().x; x += 20) {
    dc.DrawLine(x, 0, x, GetClientSize().y);
  }
  for (int y = 0; y < GetClientSize().y; y += 20) {
    dc.DrawLine(0, y, GetClientSize().x, y);
  }
  
  // Draw joins first (so they appear behind tables)
  for (const auto& join : joins_) {
    DrawJoin(dc, join);
  }
  
  // Draw tables
  for (const auto& table : tables_) {
    DrawTable(dc, table);
  }
}

void VisualQueryBuilder::OnMouseLeftDown(wxMouseEvent& event) {
  int x = event.GetX();
  int y = event.GetY();
  
  int index = FindTableIndexAt(x, y);
  if (index >= 0) {
    dragged_table_index_ = index;
    drag_offset_x_ = x - tables_[index].x;
    drag_offset_y_ = y - tables_[index].y;
    CaptureMouse();
  }
}

void VisualQueryBuilder::OnMouseLeftUp(wxMouseEvent& event) {
  if (dragged_table_index_ >= 0) {
    dragged_table_index_ = -1;
    if (HasCapture()) {
      ReleaseMouse();
    }
    Refresh();
  }
}

void VisualQueryBuilder::OnMouseMove(wxMouseEvent& event) {
  if (dragged_table_index_ >= 0 && dragged_table_index_ < static_cast<int>(tables_.size())) {
    int x = event.GetX();
    int y = event.GetY();
    
    tables_[dragged_table_index_].x = x - drag_offset_x_;
    tables_[dragged_table_index_].y = y - drag_offset_y_;
    
    Refresh();
  }
}

void VisualQueryBuilder::OnMouseRightDown(wxMouseEvent& event) {
  // TODO: Show context menu for tables/joins
  event.Skip();
}

void VisualQueryBuilder::OnSize(wxSizeEvent& event) {
  Refresh();
  event.Skip();
}

void VisualQueryBuilder::DrawTable(wxDC& dc, const TableNode& table) {
  // Draw table box
  dc.SetPen(wxPen(wxColor(100, 100, 100)));
  dc.SetBrush(wxBrush(wxColor(255, 255, 255)));
  dc.DrawRectangle(table.x, table.y, table.width, table.height);
  
  // Draw table header
  dc.SetBrush(wxBrush(wxColor(200, 200, 220)));
  dc.DrawRectangle(table.x, table.y, table.width, 25);
  
  // Draw table name
  dc.SetTextForeground(wxColor(0, 0, 0));
  dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, 
                    wxFONTWEIGHT_BOLD));
  dc.DrawLabel(wxString::FromUTF8(table.table_name.c_str()),
               wxRect(table.x + 5, table.y + 3, table.width - 10, 20));
  
  // Draw columns
  dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, 
                    wxFONTWEIGHT_NORMAL));
  int y = table.y + 30;
  for (size_t i = 0; i < table.columns.size() && y < table.y + table.height - 5; ++i) {
    if (table.selected_columns[i]) {
      dc.SetTextForeground(wxColor(0, 0, 255));
    } else {
      dc.SetTextForeground(wxColor(100, 100, 100));
    }
    dc.DrawText(wxString::FromUTF8(table.columns[i].c_str()), 
                table.x + 10, y);
    y += 18;
  }
}

void VisualQueryBuilder::DrawJoin(wxDC& dc, const JoinLink& join) {
  // Find table positions
  TableNode* from_table = nullptr;
  TableNode* to_table = nullptr;
  
  for (auto& table : tables_) {
    if (table.table_name == join.from_table) from_table = &table;
    if (table.table_name == join.to_table) to_table = &table;
  }
  
  if (!from_table || !to_table) return;
  
  // Draw join line
  int x1 = from_table->x + from_table->width / 2;
  int y1 = from_table->y + from_table->height / 2;
  int x2 = to_table->x + to_table->width / 2;
  int y2 = to_table->y + to_table->height / 2;
  
  // Set color based on join type
  wxColor color(100, 100, 100);
  if (join.join_type == 1) color = wxColor(0, 0, 255);    // Left - blue
  else if (join.join_type == 2) color = wxColor(255, 0, 0);   // Right - red
  else if (join.join_type == 3) color = wxColor(0, 128, 0);   // Full - green
  
  dc.SetPen(wxPen(color, 2));
  dc.DrawLine(x1, y1, x2, y2);
}

TableNode* VisualQueryBuilder::FindTableAt(int x, int y) {
  int index = FindTableIndexAt(x, y);
  if (index >= 0) {
    return &tables_[index];
  }
  return nullptr;
}

int VisualQueryBuilder::FindTableIndexAt(int x, int y) {
  for (int i = static_cast<int>(tables_.size()) - 1; i >= 0; --i) {
    const auto& table = tables_[i];
    if (x >= table.x && x <= table.x + table.width &&
        y >= table.y && y <= table.y + table.height) {
      return i;
    }
  }
  return -1;
}

}  // namespace scratchrobin::ui
