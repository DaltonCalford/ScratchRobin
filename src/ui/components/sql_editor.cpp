/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/components/sql_editor.h"

#include <wx/msgdlg.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(SqlEditor, wxStyledTextCtrl)
  EVT_STC_CHARADDED(wxID_ANY, SqlEditor::OnCharAdded)
  EVT_STC_MARGINCLICK(wxID_ANY, SqlEditor::OnMarginClick)
  EVT_STC_MODIFIED(wxID_ANY, SqlEditor::OnModified)
wxEND_EVENT_TABLE()

SqlEditor::SqlEditor(wxWindow* parent, wxWindowID id)
    : wxStyledTextCtrl(parent, id) {
  InitializeStyles();
  SetupKeyBindings();
  BindEvents();
}

SqlEditor::~SqlEditor() {
  // Cleanup handled by wxWidgets
}

void SqlEditor::InitializeStyles() {
  // Set up SQL syntax highlighting
  SetLexer(wxSTC_LEX_SQL);

  // Set keywords
  const char* keywords = 
    "select from where and or not insert update delete create drop table "
    "index view trigger procedure function package domain charset collation "
    "primary key foreign references unique constraint default null "
    "join inner outer left right full on group by having order asc desc "
    "union all distinct as into values set begin end if then else "
    "while do loop for return returns declare variable constant cursor "
    "exception when case commit rollback savepoint transaction isolation "
    "read committed uncommitted serializable repeatable";
  
  SetKeyWords(0, keywords);

  // Set default style
  StyleSetForeground(wxSTC_STYLE_DEFAULT, wxColor(0, 0, 0));
  StyleSetBackground(wxSTC_STYLE_DEFAULT, wxColor(255, 255, 255));
  StyleSetFont(wxSTC_STYLE_DEFAULT, wxFont(10, wxFONTFAMILY_TELETYPE, 
                                           wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
  StyleClearAll();

  // SQL-specific styles
  StyleSetForeground(wxSTC_SQL_WORD, wxColor(0, 0, 255));      // Keywords - blue
  StyleSetForeground(wxSTC_SQL_STRING, wxColor(255, 0, 0));    // Strings - red
  StyleSetForeground(wxSTC_SQL_CHARACTER, wxColor(255, 0, 0)); // Characters - red
  StyleSetForeground(wxSTC_SQL_NUMBER, wxColor(0, 128, 0));    // Numbers - green
  StyleSetForeground(wxSTC_SQL_COMMENT, wxColor(128, 128, 128)); // Comments - gray
  StyleSetForeground(wxSTC_SQL_COMMENTLINE, wxColor(128, 128, 128));

  // Set up margins for line numbers
  SetMarginType(0, wxSTC_MARGIN_NUMBER);
  SetMarginWidth(0, 40);
}

void SqlEditor::SetupKeyBindings() {
  // Set up keyboard shortcuts
  CmdKeyAssign(wxSTC_KEY_TAB, wxSTC_SCMOD_CTRL, wxSTC_CMD_TAB);
  CmdKeyAssign(wxSTC_KEY_TAB, wxSTC_SCMOD_CTRL | wxSTC_SCMOD_SHIFT, wxSTC_CMD_BACKTAB);
}

void SqlEditor::BindEvents() {
  // Additional event bindings if needed
}

void SqlEditor::SetSqlText(const wxString& text) {
  SetText(text);
  EmptyUndoBuffer();
  SetSavePoint();
}

wxString SqlEditor::GetSqlText() const {
  return GetText();
}

wxString SqlEditor::GetSelectedOrAllText() {
  wxString selection = GetSelectedText();
  if (selection.IsEmpty()) {
    return GetText();
  }
  return selection;
}

void SqlEditor::Clear() {
  ClearAll();
}

bool SqlEditor::IsModified() const {
  return GetModify();
}

void SqlEditor::MarkSaved() {
  SetSavePoint();
}

void SqlEditor::SetEditorFont(const wxFont& font) {
  StyleSetFont(wxSTC_STYLE_DEFAULT, font);
  StyleClearAll();
}

void SqlEditor::EnableCodeFolding(bool enable) {
  if (enable) {
    SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    SetMarginMask(1, wxSTC_MASK_FOLDERS);
    SetMarginWidth(1, 16);
    SetMarginSensitive(1, true);
    SetProperty(wxT("fold"), wxT("1"));
  } else {
    SetMarginWidth(1, 0);
    SetProperty(wxT("fold"), wxT("0"));
  }
}

void SqlEditor::EnableWordWrap(bool enable) {
  SetWrapMode(enable ? wxSTC_WRAP_WORD : wxSTC_WRAP_NONE);
}

bool SqlEditor::Find(const wxString& text, bool match_case, bool whole_word) {
  int flags = 0;
  if (match_case) flags |= wxSTC_FIND_MATCHCASE;
  if (whole_word) flags |= wxSTC_FIND_WHOLEWORD;
  
  SetSearchFlags(flags);
  return SearchNext(flags, text) != -1;
}

bool SqlEditor::Replace(const wxString& find_text, const wxString& replace_text) {
  if (GetSelectedText() == find_text) {
    ReplaceSelection(replace_text);
    return true;
  }
  return Find(find_text);
}

int SqlEditor::ReplaceAll(const wxString& find_text, const wxString& replace_text) {
  int count = 0;
  SetTargetStart(0);
  SetTargetEnd(GetTextLength());
  
  while (SearchInTarget(find_text) != -1) {
    ReplaceTarget(replace_text);
    count++;
    SetTargetStart(GetTargetStart() + replace_text.Length());
    SetTargetEnd(GetTextLength());
  }
  
  return count;
}

void SqlEditor::FormatSql() {
  // TODO: Implement SQL formatting
  wxMessageBox(_("SQL formatting not yet implemented"),
               _("Format"), wxOK | wxICON_INFORMATION);
}

void SqlEditor::OnCharAdded(wxStyledTextEvent& event) {
  // TODO: Handle auto-indentation, brace matching, etc.
  event.Skip();
}

void SqlEditor::OnMarginClick(wxStyledTextEvent& event) {
  // TODO: Handle code folding
  event.Skip();
}

void SqlEditor::OnModified(wxStyledTextEvent& event) {
  // TODO: Track modifications
  event.Skip();
}

}  // namespace scratchrobin::ui
