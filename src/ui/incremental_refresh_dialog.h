/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_INCREMENTAL_REFRESH_DIALOG_H
#define SCRATCHROBIN_INCREMENTAL_REFRESH_DIALOG_H

#include "diagram/reverse_engineer.h"

#include <memory>
#include <vector>

#include <wx/dialog.h>

class wxListCtrl;
class wxStaticText;
class wxButton;
class wxChoice;

namespace scratchrobin {

class ConnectionManager;
class ConnectionProfile;
class DiagramModel;

// Change types for incremental refresh
enum class SchemaChangeType {
    Added,
    Removed,
    Modified
};

struct SchemaChange {
    SchemaChangeType type;
    std::string object_type;  // "table", "column", "index", "constraint"
    std::string object_name;
    std::string parent_name;  // For columns/indexes - the table name
    std::string details;
    bool apply = true;  // Whether to apply this change
};

// Dialog for incremental refresh from database
class IncrementalRefreshDialog : public wxDialog {
public:
    IncrementalRefreshDialog(wxWindow* parent,
                             ConnectionManager* connection_manager,
                             const std::vector<ConnectionProfile>* connections,
                             DiagramModel* model);

    // Run the refresh process
    bool RunRefresh();

private:
    void BuildLayout();
    void OnConnectionChanged(wxCommandEvent& event);
    void OnAnalyze(wxCommandEvent& event);
    void OnApplySelected(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnDeselectAll(wxCommandEvent& event);
    
    // Analyze differences between diagram and database
    std::vector<SchemaChange> AnalyzeDifferences();
    
    // Apply changes to diagram
    bool ApplyChanges(const std::vector<SchemaChange>& changes);
    
    void PopulateChangesList(const std::vector<SchemaChange>& changes);

    ConnectionManager* connection_manager_;
    const std::vector<ConnectionProfile>* connections_;
    DiagramModel* model_;
    
    wxChoice* connection_choice_ = nullptr;
    wxChoice* schema_choice_ = nullptr;
    wxStaticText* status_text_ = nullptr;
    wxListCtrl* changes_list_ = nullptr;
    wxButton* analyze_btn_ = nullptr;
    wxButton* apply_btn_ = nullptr;
    
    std::vector<SchemaChange> current_changes_;
    bool has_analyzed_ = false;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_INCREMENTAL_REFRESH_DIALOG_H
