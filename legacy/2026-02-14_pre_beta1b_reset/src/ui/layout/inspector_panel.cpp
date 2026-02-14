/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "ui/layout/inspector_panel.h"
#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace scratchrobin {

InspectorPanel::InspectorPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    BuildLayout();
}

void InspectorPanel::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    notebook_ = new wxNotebook(this, wxID_ANY);
    
    // Overview page
    auto* overview = new wxPanel(notebook_, wxID_ANY);
    auto* overview_sizer = new wxBoxSizer(wxVERTICAL);
    overview_text_ = new wxTextCtrl(overview, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY);
    overview_sizer->Add(overview_text_, 1, wxEXPAND);
    overview->SetSizer(overview_sizer);
    overview_page_index_ = notebook_->AddPage(overview, "Overview");
    
    // DDL page
    auto* ddl = new wxPanel(notebook_, wxID_ANY);
    auto* ddl_sizer = new wxBoxSizer(wxVERTICAL);
    ddl_text_ = new wxTextCtrl(ddl, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    ddl_sizer->Add(ddl_text_, 1, wxEXPAND);
    ddl->SetSizer(ddl_sizer);
    ddl_page_index_ = notebook_->AddPage(ddl, "DDL");
    
    // Dependencies page
    auto* deps = new wxPanel(notebook_, wxID_ANY);
    auto* deps_sizer = new wxBoxSizer(wxVERTICAL);
    deps_text_ = new wxTextCtrl(deps, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY);
    deps_sizer->Add(deps_text_, 1, wxEXPAND);
    deps->SetSizer(deps_sizer);
    deps_page_index_ = notebook_->AddPage(deps, "Dependencies");
    
    sizer->Add(notebook_, 1, wxEXPAND);
    SetSizer(sizer);
}

void InspectorPanel::ShowNode(const MetadataNode* node) {
    if (!node) {
        Clear();
        return;
    }
    
    UpdateOverview(node);
    UpdateDDL(node);
    UpdateDependencies(node);
}

void InspectorPanel::Clear() {
    overview_text_->Clear();
    ddl_text_->Clear();
    deps_text_->Clear();
}

void InspectorPanel::UpdateOverview(const MetadataNode* node) {
    if (!overview_text_ || !node) return;
    
    wxString text;
    text += wxString::Format("Name: %s\n", wxString::FromUTF8(node->name));
    text += wxString::Format("Type: %s\n", wxString::FromUTF8(node->kind));
    text += wxString::Format("Path: %s\n", wxString::FromUTF8(node->path));
    
    overview_text_->SetValue(text);
}

void InspectorPanel::UpdateDDL(const MetadataNode* node) {
    if (!ddl_text_ || !node) return;
    
    std::string sql = BuildSeedSql(node);
    ddl_text_->SetValue(wxString::FromUTF8(sql));
}

void InspectorPanel::UpdateDependencies(const MetadataNode* node) {
    if (!deps_text_ || !node) return;
    
    wxString text = "Dependencies:\n";
    // Would query dependency graph
    deps_text_->SetValue(text);
}

std::string InspectorPanel::BuildSeedSql(const MetadataNode* node) const {
    if (!node) return "";
    
    if (node->kind == "table") {
        return "-- Table DDL for " + node->name + "\nCREATE TABLE " + node->name + " ();";
    }
    return "-- DDL not available for " + node->kind;
}

} // namespace scratchrobin
