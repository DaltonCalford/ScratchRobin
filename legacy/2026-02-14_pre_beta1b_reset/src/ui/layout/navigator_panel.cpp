/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "ui/layout/navigator_panel.h"
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/artprov.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(NavigatorPanel, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_ANY, NavigatorPanel::OnTreeSelection)
    EVT_TREE_ITEM_MENU(wxID_ANY, NavigatorPanel::OnTreeItemMenu)
    EVT_TREE_ITEM_ACTIVATED(wxID_ANY, NavigatorPanel::OnTreeActivate)
wxEND_EVENT_TABLE()

NavigatorPanel::NavigatorPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    BuildLayout();
}

void NavigatorPanel::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Filter controls
    auto* filter_sizer = new wxBoxSizer(wxHORIZONTAL);
    filter_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    filter_ctrl_->SetHint("Filter...");
    filter_clear_button_ = new wxButton(this, wxID_ANY, "X", wxDefaultPosition, wxSize(24, 24));
    filter_sizer->Add(filter_ctrl_, 1, wxEXPAND | wxRIGHT, 4);
    filter_sizer->Add(filter_clear_button_, 0);
    sizer->Add(filter_sizer, 0, wxEXPAND | wxALL, 4);
    
    // Tree control
    tree_ = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_DEFAULT_STYLE);
    
    // Create image list
    tree_images_ = new wxImageList(16, 16);
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16)));
    tree_->SetImageList(tree_images_);
    
    sizer->Add(tree_, 1, wxEXPAND | wxALL, 4);
    SetSizer(sizer);
}

void NavigatorPanel::PopulateTree(const MetadataSnapshot& snapshot) {
    tree_->DeleteAllItems();
    node_map_.clear();
    
    if (snapshot.nodes.empty()) return;
    
    // Add root
    wxTreeItemId root = tree_->AddRoot("Database", 0);
    
    // Add nodes
    for (const auto& node : snapshot.nodes) {
        AddNodeToTree(root, &node);
    }
    
    tree_->Expand(root);
}

wxTreeItemId NavigatorPanel::AddNodeToTree(wxTreeItemId parent, const MetadataNode* node) {
    if (!node) return wxTreeItemId();
    
    wxString label = wxString::FromUTF8(node->name);
    int icon = GetIconForNode(*node);
    
    wxTreeItemId item = tree_->AppendItem(parent, label, icon);
    node_map_[node->path] = item;
    
    // Add children
    for (const auto& child : node->children) {
        AddNodeToTree(item, &child);
    }
    
    return item;
}

int NavigatorPanel::GetIconForNode(const MetadataNode& node) const {
    if (node.kind == "table") return 0;
    if (node.kind == "view") return 1;
    if (node.kind == "column") return 1;
    return 0;
}

void NavigatorPanel::RefreshContent() {
    // Refresh tree from current model
}

void NavigatorPanel::SetFilter(const std::string& filter) {
    filter_text_ = filter;
    // Apply filter to tree
}

std::string NavigatorPanel::GetSelectedPath() const {
    wxTreeItemId sel = tree_->GetSelection();
    if (!sel.IsOk()) return "";
    
    for (const auto& [path, item] : node_map_) {
        if (item == sel) return path;
    }
    return "";
}

void NavigatorPanel::SelectPath(const std::string& path) {
    auto it = node_map_.find(path);
    if (it != node_map_.end()) {
        tree_->SelectItem(it->second);
    }
}

const MetadataNode* NavigatorPanel::GetSelectedNode() const {
    // Would need access to MetadataModel
    return nullptr;
}

void NavigatorPanel::OnTreeSelection(wxTreeEvent& event) {
    event.Skip();
}

void NavigatorPanel::OnTreeItemMenu(wxTreeEvent& event) {
    event.Skip();
}

void NavigatorPanel::OnTreeActivate(wxTreeEvent& event) {
    event.Skip();
}

void NavigatorPanel::OnFilterChanged(wxCommandEvent& event) {
    filter_text_ = filter_ctrl_->GetValue().ToStdString();
    event.Skip();
}

void NavigatorPanel::OnFilterClear(wxCommandEvent& event) {
    filter_ctrl_->Clear();
    filter_text_.clear();
    event.Skip();
}

} // namespace scratchrobin
