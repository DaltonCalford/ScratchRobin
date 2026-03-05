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

#include <wx/stc/stc.h>

namespace scratchrobin::ui {

/**
 * SqlEditor - A syntax-highlighted SQL editor based on wxStyledTextCtrl
 */
class SqlEditor : public wxStyledTextCtrl {
 public:
  SqlEditor(wxWindow* parent, wxWindowID id = wxID_ANY);
  ~SqlEditor() override;

  // Set the SQL text
  void SetSqlText(const wxString& text);

  // Get the SQL text
  wxString GetSqlText() const;

  // Get selected text or all text if no selection
  wxString GetSelectedOrAllText();

  // Clear the editor
  void Clear();

  // Check if text has been modified
  bool IsModified() const;

  // Mark as saved (clear modified flag)
  void MarkSaved();

  // Set the editor font
  void SetEditorFont(const wxFont& font);

  // Enable/disable code folding
  void EnableCodeFolding(bool enable);

  // Enable/disable word wrap
  void EnableWordWrap(bool enable);

  // Find and replace
  bool Find(const wxString& text, bool match_case = true, bool whole_word = false);
  bool Replace(const wxString& find_text, const wxString& replace_text);
  int ReplaceAll(const wxString& find_text, const wxString& replace_text);

  // Format SQL
  void FormatSql();

 private:
  void InitializeStyles();
  void SetupKeyBindings();
  void BindEvents();

  void OnCharAdded(wxStyledTextEvent& event);
  void OnMarginClick(wxStyledTextEvent& event);
  void OnModified(wxStyledTextEvent& event);

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
