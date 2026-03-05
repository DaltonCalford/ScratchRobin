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
#include <vector>

#include <wx/stc/stc.h>

namespace scratchrobin::ui {

/**
 * CompletionItem - Represents a single code completion suggestion
 */
struct CompletionItem {
  std::string display_text;
  std::string insert_text;
  std::string description;
  int icon_type{0};  // 0=keyword, 1=table, 2=column, 3=function, etc.
};

/**
 * SqlCodeCompletion - Provides SQL code completion for wxStyledTextCtrl
 */
class SqlCodeCompletion {
 public:
  SqlCodeCompletion();
  ~SqlCodeCompletion();

  // Attach to a styled text control
  void Attach(wxStyledTextCtrl* stc);

  // Detach from control
  void Detach();

  // Enable/disable auto-completion
  void EnableAutoCompletion(bool enable) { auto_completion_enabled_ = enable; }

  // Set the database schema for completion (tables, columns, etc.)
  void SetSchema(const std::vector<std::string>& tables,
                 const std::vector<std::vector<std::string>>& columns);

  // Add custom keywords for completion
  void AddKeywords(const std::vector<std::string>& keywords);

  // Clear all completion data
  void Clear();

  // Manually trigger completion
  void TriggerCompletion();

  // Cancel any active completion
  void CancelCompletion();

 private:
  void OnCharAdded(wxStyledTextEvent& event);
  void OnKeyDown(wxKeyEvent& event);
  void ShowCompletionList();
  std::vector<CompletionItem> GetCompletions(const std::string& prefix);

  wxStyledTextCtrl* stc_{nullptr};
  bool auto_completion_enabled_{true};

  std::vector<std::string> keywords_;
  std::vector<std::string> tables_;
  std::vector<std::vector<std::string>> columns_;

  bool is_attached_{false};
};

}  // namespace scratchrobin::ui
