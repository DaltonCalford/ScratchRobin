/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "incremental_refresh_dialog.h"

#include "core/connection_manager.h"
#include "ui/diagram_model.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace scratchrobin {

namespace {

std::string ProfileLabel(const ConnectionProfile& profile) {
    if (!profile.name.empty()) return profile.name;
    if (!profile.database.empty()) return profile.database;
    std::string label = profile.host.empty() ? "localhost" : profile.host;
    if (profile.port != 0) label += ":" + std::to_string(profile.port);
    return label;
}

std::string NormalizeBackend(const std::string& backend) {
    std::string value = backend;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    if (value == "postgres" || value == "pg") return "postgresql";
    if (value == "mariadb") return "mysql";
    if (value == "fb") return "firebird";
    if (value.empty() || value == "scratchbird") return "native";
    return value;
}

} // namespace

wxBEGIN_EVENT_TABLE(IncrementalRefreshDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, IncrementalRefreshDialog::OnAnalyze)
wxEND_EVENT_TABLE()

IncrementalRefreshDialog::IncrementalRefreshDialog(wxWindow* parent,
                                                   ConnectionManager* connection_manager,
                                                   const std::vector<ConnectionProfile>* connections,
                                                   DiagramModel* model)
    : wxDialog(parent, wxID_ANY, "Incremental Refresh from Database",
               wxDefaultPosition, wxSize(700, 500)),
      connection_manager_(connection_manager),
      connections_(connections),
      model_(model) {
    BuildLayout();
}

void IncrementalRefreshDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Connection selection
    auto* connSizer = new wxBoxSizer(wxHORIZONTAL);
    connSizer->Add(new wxStaticText(this, wxID_ANY, "Connection:"), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    connection_choice_ = new wxChoice(this, wxID_ANY);
    if (connections_) {
        for (const auto& profile : *connections_) {
            connection_choice_->Append(ProfileLabel(profile));
        }
    }
    connSizer->Add(connection_choice_, 1, wxEXPAND);
    root->Add(connSizer, 0, wxEXPAND | wxALL, 12);
    
    // Schema selection
    auto* schemaSizer = new wxBoxSizer(wxHORIZONTAL);
    schemaSizer->Add(new wxStaticText(this, wxID_ANY, "Schema:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    schema_choice_ = new wxChoice(this, wxID_ANY);
    schema_choice_->Append("public");
    schema_choice_->SetSelection(0);
    schemaSizer->Add(schema_choice_, 1, wxEXPAND);
    root->Add(schemaSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Status
    status_text_ = new wxStaticText(this, wxID_ANY, 
        "Click Analyze to compare the diagram with the database schema");
    root->Add(status_text_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Changes list
    changes_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_HRULES);
    changes_list_->AppendColumn("Type", wxLIST_FORMAT_LEFT, 80);
    changes_list_->AppendColumn("Object", wxLIST_FORMAT_LEFT, 150);
    changes_list_->AppendColumn("Parent", wxLIST_FORMAT_LEFT, 150);
    changes_list_->AppendColumn("Change", wxLIST_FORMAT_LEFT, 200);
    root->Add(changes_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons row
    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
    analyze_btn_ = new wxButton(this, wxID_ANY, "Analyze");
    apply_btn_ = new wxButton(this, wxID_ANY, "Apply Selected");
    apply_btn_->Enable(false);
    auto* selectAllBtn = new wxButton(this, wxID_ANY, "Select All");
    auto* deselectAllBtn = new wxButton(this, wxID_ANY, "Deselect All");
    
    btnRow->Add(analyze_btn_, 0, wxRIGHT, 8);
    btnRow->Add(apply_btn_, 0, wxRIGHT, 8);
    btnRow->AddStretchSpacer(1);
    btnRow->Add(selectAllBtn, 0, wxRIGHT, 8);
    btnRow->Add(deselectAllBtn, 0);
    root->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Bottom buttons
    auto* bottomBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    bottomBtnSizer->AddStretchSpacer(1);
    bottomBtnSizer->Add(new wxButton(this, wxID_CANCEL, "Close"), 0);
    root->Add(bottomBtnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
    
    // Bind button events
    Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
        auto* btn = dynamic_cast<wxButton*>(evt.GetEventObject());
        if (!btn) return;
        
        wxString label = btn->GetLabel();
        if (label == "Analyze") {
            OnAnalyze(evt);
        } else if (label == "Apply Selected") {
            OnApplySelected(evt);
        } else if (label == "Select All") {
            OnSelectAll(evt);
        } else if (label == "Deselect All") {
            OnDeselectAll(evt);
        }
    });
}

void IncrementalRefreshDialog::OnAnalyze(wxCommandEvent&) {
    if (!connection_manager_ || !connections_) return;
    
    int sel = connection_choice_->GetSelection();
    if (sel < 0 || sel >= static_cast<int>(connections_->size())) {
        wxMessageBox("Please select a connection", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    const auto& profile = (*connections_)[sel];
    
    // Connect
    connection_manager_->Disconnect();
    if (!connection_manager_->Connect(profile)) {
        status_text_->SetLabel("Connection failed: " + connection_manager_->LastError());
        return;
    }
    
    status_text_->SetLabel("Analyzing differences...");
    
    current_changes_ = AnalyzeDifferences();
    PopulateChangesList(current_changes_);
    
    has_analyzed_ = true;
    apply_btn_->Enable(!current_changes_.empty());
    
    std::string msg = "Found " + std::to_string(current_changes_.size()) + " differences";
    status_text_->SetLabel(msg);
}

std::vector<SchemaChange> IncrementalRefreshDialog::AnalyzeDifferences() {
    std::vector<SchemaChange> changes;
    
    if (!model_ || !connection_manager_) return changes;
    
    // Get current tables from diagram
    const auto& diagramNodes = model_->nodes();
    std::string schema = schema_choice_->GetStringSelection().ToStdString();
    
    // Query database tables
    QueryResult dbTables;
    std::string sql = "SELECT table_name FROM information_schema.tables "
                      "WHERE table_schema = '" + schema + "' AND table_type = 'BASE TABLE';";
    
    if (!connection_manager_->ExecuteQuery(sql, &dbTables)) {
        return changes;
    }
    
    // Build sets of tables
    std::vector<std::string> dbTableNames;
    for (const auto& row : dbTables.rows) {
        if (!row.empty() && !row[0].isNull) {
            dbTableNames.push_back(row[0].text);
        }
    }
    
    // Find added tables (in DB but not in diagram)
    for (const auto& dbTable : dbTableNames) {
        auto it = std::find_if(diagramNodes.begin(), diagramNodes.end(),
            [&dbTable](const DiagramNode& n) { return n.name == dbTable; });
        
        if (it == diagramNodes.end()) {
            SchemaChange change;
            change.type = SchemaChangeType::Added;
            change.object_type = "table";
            change.object_name = dbTable;
            change.details = "Table exists in database but not in diagram";
            changes.push_back(change);
        } else {
            // Table exists in both - check for column changes
            // Query columns from database
            QueryResult dbColumns;
            std::string colSql = "SELECT column_name, data_type, is_nullable "
                                 "FROM information_schema.columns "
                                 "WHERE table_schema = '" + schema + "' "
                                 "AND table_name = '" + dbTable + "';";
            
            if (connection_manager_->ExecuteQuery(colSql, &dbColumns)) {
                // Compare with diagram attributes
                for (const auto& dbCol : dbColumns.rows) {
                    if (dbCol.size() < 3) continue;
                    
                    std::string colName = dbCol[0].isNull ? "" : dbCol[0].text;
                    std::string colType = dbCol[1].isNull ? "" : dbCol[1].text;
                    
                    auto attrIt = std::find_if(it->attributes.begin(), it->attributes.end(),
                        [&colName](const DiagramAttribute& a) { return a.name == colName; });
                    
                    if (attrIt == it->attributes.end()) {
                        SchemaChange change;
                        change.type = SchemaChangeType::Added;
                        change.object_type = "column";
                        change.object_name = colName;
                        change.parent_name = dbTable;
                        change.details = "Column exists in DB but not in diagram: " + colType;
                        changes.push_back(change);
                    } else if (attrIt->data_type != colType) {
                        SchemaChange change;
                        change.type = SchemaChangeType::Modified;
                        change.object_type = "column";
                        change.object_name = colName;
                        change.parent_name = dbTable;
                        change.details = "Type changed from " + attrIt->data_type + " to " + colType;
                        changes.push_back(change);
                    }
                }
            }
        }
    }
    
    // Find removed tables (in diagram but not in DB)
    for (const auto& diagramNode : diagramNodes) {
        auto it = std::find(dbTableNames.begin(), dbTableNames.end(), diagramNode.name);
        if (it == dbTableNames.end()) {
            SchemaChange change;
            change.type = SchemaChangeType::Removed;
            change.object_type = "table";
            change.object_name = diagramNode.name;
            change.details = "Table exists in diagram but not in database";
            changes.push_back(change);
        }
    }
    
    return changes;
}

void IncrementalRefreshDialog::PopulateChangesList(const std::vector<SchemaChange>& changes) {
    changes_list_->DeleteAllItems();
    
    for (size_t i = 0; i < changes.size(); ++i) {
        const auto& change = changes[i];
        
        std::string typeStr;
        switch (change.type) {
            case SchemaChangeType::Added: typeStr = "Added"; break;
            case SchemaChangeType::Removed: typeStr = "Removed"; break;
            case SchemaChangeType::Modified: typeStr = "Modified"; break;
        }
        
        long index = changes_list_->InsertItem(static_cast<long>(i), typeStr);
        changes_list_->SetItem(index, 1, change.object_type + ": " + change.object_name);
        changes_list_->SetItem(index, 2, change.parent_name);
        changes_list_->SetItem(index, 3, change.details);
        
        // Check the item by default
        changes_list_->SetItemImage(index, 0);  // Use image for checked state if needed
    }
}

void IncrementalRefreshDialog::OnApplySelected(wxCommandEvent&) {
    if (!has_analyzed_ || current_changes_.empty()) return;
    
    // Collect selected changes (use current changes for now since we don't have checkboxes)
    // In a full implementation, we'd track which items are selected via a checkbox column
    std::vector<SchemaChange> selectedChanges = current_changes_;
    
    if (ApplyChanges(selectedChanges)) {
        wxMessageBox("Changes applied successfully", "Success", wxOK | wxICON_INFORMATION, this);
        EndModal(wxID_OK);
    } else {
        wxMessageBox("Failed to apply some changes", "Warning", wxOK | wxICON_WARNING, this);
    }
}

bool IncrementalRefreshDialog::ApplyChanges(const std::vector<SchemaChange>& changes) {
    if (!model_) return false;
    
    bool success = true;
    
    for (const auto& change : changes) {
        if (change.object_type == "table" && change.type == SchemaChangeType::Added) {
            // Add new node to diagram
            DiagramNode node;
            node.id = "node_" + std::to_string(model_->NextNodeIndex());
            node.name = change.object_name;
            node.type = "TABLE";
            node.x = 100 + (rand() % 400);
            node.y = 100 + (rand() % 300);
            node.width = 180;
            node.height = 120;
            
            // Query columns for this table
            if (connection_manager_) {
                std::string sql = "SELECT column_name, data_type FROM information_schema.columns "
                                  "WHERE table_name = '" + change.object_name + "';";
                QueryResult result;
                if (connection_manager_->ExecuteQuery(sql, &result)) {
                    for (const auto& row : result.rows) {
                        if (row.size() >= 2) {
                            DiagramAttribute attr;
                            attr.name = row[0].isNull ? "" : row[0].text;
                            attr.data_type = row[1].isNull ? "" : row[1].text;
                            node.attributes.push_back(attr);
                        }
                    }
                }
            }
            
            model_->AddNode(node);
        }
        else if (change.object_type == "table" && change.type == SchemaChangeType::Removed) {
            // Remove node from diagram
            auto& nodes = model_->nodes();
            auto it = std::find_if(nodes.begin(), nodes.end(),
                [&change](const DiagramNode& n) { return n.name == change.object_name; });
            
            if (it != nodes.end()) {
                size_t index = it - nodes.begin();
                // Remove edges first
                auto& edges = model_->edges();
                edges.erase(std::remove_if(edges.begin(), edges.end(),
                    [&it](const DiagramEdge& e) {
                        return e.source_id == it->id || e.target_id == it->id;
                    }), edges.end());
                // Remove node
                nodes.erase(nodes.begin() + index);
            }
        }
        else if (change.object_type == "column" && change.type == SchemaChangeType::Added) {
            // Add column to existing table
            auto& nodes = model_->nodes();
            auto it = std::find_if(nodes.begin(), nodes.end(),
                [&change](const DiagramNode& n) { return n.name == change.parent_name; });
            
            if (it != nodes.end()) {
                DiagramAttribute attr;
                attr.name = change.object_name;
                attr.data_type = "UNKNOWN";
                it->attributes.push_back(attr);
            }
        }
    }
    
    return success;
}

void IncrementalRefreshDialog::OnSelectAll(wxCommandEvent&) {
    // Selection tracking would require a separate mechanism
    // For now, we'll just highlight all items
    for (long i = 0; i < changes_list_->GetItemCount(); ++i) {
        changes_list_->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
}

void IncrementalRefreshDialog::OnDeselectAll(wxCommandEvent&) {
    for (long i = 0; i < changes_list_->GetItemCount(); ++i) {
        changes_list_->SetItemState(i, 0, wxLIST_STATE_SELECTED);
    }
}

} // namespace scratchrobin
