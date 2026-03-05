/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/components/sql_code_completion.h"

#include <algorithm>
#include <cctype>

namespace scratchrobin::ui {

namespace {

// Default SQL keywords
const char* kDefaultKeywords =
    "SELECT FROM WHERE AND OR NOT INSERT UPDATE DELETE CREATE ALTER DROP "
    "TABLE INDEX VIEW TRIGGER PROCEDURE FUNCTION PACKAGE DOMAIN "
    "PRIMARY KEY FOREIGN REFERENCES UNIQUE CONSTRAINT DEFAULT NULL "
    "JOIN INNER OUTER LEFT RIGHT FULL ON GROUP BY HAVING ORDER ASC DESC "
    "UNION ALL DISTINCT AS INTO VALUES SET BEGIN END IF THEN ELSE "
    "WHILE DO LOOP FOR RETURN RETURNS DECLARE VARIABLE CONSTANT CURSOR "
    "EXCEPTION WHEN CASE COMMIT ROLLBACK SAVEPOINT TRANSACTION ISOLATION "
    "LEVEL READ UNCOMMITTED COMMITTED REPEATABLE SERIALIZABLE SNAPSHOT "
    "EXISTS IN BETWEEN LIKE IS NULL TRUE FALSE CAST GENERATED ALWAYS "
    "IDENTITY START WITH INCREMENT BY CACHE";

std::vector<std::string> SplitKeywords(const std::string& text) {
  std::vector<std::string> result;
  std::string current;
  for (char c : text) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!current.empty()) {
        result.push_back(current);
        current.clear();
      }
    } else {
      current += c;
    }
  }
  if (!current.empty()) {
    result.push_back(current);
  }
  return result;
}

std::string ToLower(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

}  // namespace

SqlCodeCompletion::SqlCodeCompletion() {
  // Initialize with default keywords
  keywords_ = SplitKeywords(kDefaultKeywords);
}

SqlCodeCompletion::~SqlCodeCompletion() {
  if (is_attached_ && stc_) {
    Detach();
  }
}

void SqlCodeCompletion::Attach(wxStyledTextCtrl* stc) {
  if (is_attached_) {
    Detach();
  }
  
  stc_ = stc;
  if (stc_) {
    stc_->Bind(wxEVT_STC_CHARADDED, &SqlCodeCompletion::OnCharAdded, this);
    stc_->Bind(wxEVT_KEY_DOWN, &SqlCodeCompletion::OnKeyDown, this);
    is_attached_ = true;
  }
}

void SqlCodeCompletion::Detach() {
  if (stc_ && is_attached_) {
    stc_->Unbind(wxEVT_STC_CHARADDED, &SqlCodeCompletion::OnCharAdded, this);
    stc_->Unbind(wxEVT_KEY_DOWN, &SqlCodeCompletion::OnKeyDown, this);
    stc_ = nullptr;
    is_attached_ = false;
  }
}

void SqlCodeCompletion::SetSchema(const std::vector<std::string>& tables,
                                  const std::vector<std::vector<std::string>>& columns) {
  tables_ = tables;
  columns_ = columns;
}

void SqlCodeCompletion::AddKeywords(const std::vector<std::string>& keywords) {
  keywords_.insert(keywords_.end(), keywords.begin(), keywords.end());
  
  // Remove duplicates
  std::sort(keywords_.begin(), keywords_.end());
  keywords_.erase(std::unique(keywords_.begin(), keywords_.end()), keywords_.end());
}

void SqlCodeCompletion::Clear() {
  keywords_.clear();
  tables_.clear();
  columns_.clear();
}

void SqlCodeCompletion::TriggerCompletion() {
  ShowCompletionList();
}

void SqlCodeCompletion::CancelCompletion() {
  if (stc_) {
    stc_->AutoCompCancel();
  }
}

void SqlCodeCompletion::OnCharAdded(wxStyledTextEvent& event) {
  if (!auto_completion_enabled_ || !stc_) {
    event.Skip();
    return;
  }

  int ch = event.GetKey();
  
  // Trigger completion on letter or when typing after a period
  if (std::isalpha(ch) || ch == '.') {
    wxString line = stc_->GetCurLine();
    int pos = stc_->GetCurrentPos();
    int line_start = stc_->PositionFromLine(stc_->GetCurrentLine());
    int col = pos - line_start;
    
    // Get word before cursor
    wxString prefix;
    for (int i = col - 1; i >= 0; --i) {
      wxChar c = line[i];
      if (std::isalnum(c) || c == '_') {
        prefix = c + prefix;
      } else {
        break;
      }
    }
    
    if (prefix.Length() >= 2 || ch == '.') {
      ShowCompletionList();
    }
  }
  
  event.Skip();
}

void SqlCodeCompletion::OnKeyDown(wxKeyEvent& event) {
  // Handle special keys for completion
  if (event.GetKeyCode() == WXK_ESCAPE) {
    CancelCompletion();
  }
  event.Skip();
}

void SqlCodeCompletion::ShowCompletionList() {
  if (!stc_) return;

  // Get current word prefix
  int pos = stc_->GetCurrentPos();
  int line_start = stc_->PositionFromLine(stc_->GetCurrentLine());
  int col = pos - line_start;
  wxString line = stc_->GetCurLine();
  
  wxString prefix;
  for (int i = col - 1; i >= 0; --i) {
    wxChar c = line[i];
    if (std::isalnum(c) || c == '_') {
      prefix = c + prefix;
    } else {
      break;
    }
  }
  
  std::string prefix_std = prefix.ToStdString();
  auto completions = GetCompletions(prefix_std);
  
  if (completions.empty()) {
    return;
  }
  
  // Build completion list string
  wxString list;
  for (const auto& item : completions) {
    if (!list.IsEmpty()) {
      list += " ";
    }
    list += wxString::FromUTF8(item.display_text.c_str());
  }
  
  stc_->AutoCompSetSeparator(' ');
  stc_->AutoCompShow(prefix.length(), list);
}

std::vector<CompletionItem> SqlCodeCompletion::GetCompletions(const std::string& prefix) {
  std::vector<CompletionItem> result;
  std::string prefix_lower = ToLower(prefix);
  
  // Add matching keywords
  for (const auto& kw : keywords_) {
    std::string kw_lower = ToLower(kw);
    if (kw_lower.find(prefix_lower) == 0) {
      CompletionItem item;
      item.display_text = kw;
      item.insert_text = kw;
      item.icon_type = 0;  // keyword
      result.push_back(item);
    }
  }
  
  // Add matching tables
  for (const auto& table : tables_) {
    std::string table_lower = ToLower(table);
    if (table_lower.find(prefix_lower) == 0) {
      CompletionItem item;
      item.display_text = table;
      item.insert_text = table;
      item.icon_type = 1;  // table
      result.push_back(item);
    }
  }
  
  // Limit results
  if (result.size() > 50) {
    result.resize(50);
  }
  
  return result;
}

}  // namespace scratchrobin::ui
