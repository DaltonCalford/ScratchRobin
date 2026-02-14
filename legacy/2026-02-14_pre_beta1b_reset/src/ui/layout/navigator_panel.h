/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_LAYOUT_NAVIGATOR_PANEL_H
#define SCRATCHROBIN_LAYOUT_NAVIGATOR_PANEL_H

#include "ui/layout/dockable_window.h"
#include "core/metadata_model.h"
#include <wx/panel.h>
#include <wx/treectrl.h>
#include <map>

class wxTextCtrl;
class wxButton;

namespace scratchrobin {

// Navigator panel - dockable tree control for database navigation
class NavigatorPanel : public wxPanel, public INavigatorWindow {
public:
    NavigatorPanel(wxWindow* parent);
    
    // IDockableWindow implementation
    std::string GetWindowId() const override { return "navigator"; }
    std::string GetWindowTitle() const override { return "Navigator"; }
    std::string GetWindowType() const override { return "navigator"; }
    wxWindow* GetWindow() override { return this; }
    const wxWindow* GetWindow() const override { return this; }
    
    // INavigatorWindow implementation
    void RefreshContent() override;
    void SetFilter(const std::string& filter) override;
    std::string GetSelectedPath() const override;
    void SelectPath(const std::string& path) override;
    
    // Tree population
    void PopulateTree(const MetadataSnapshot& snapshot);
    
    // Tree access
    wxTreeCtrl* GetTree() { return tree_; }
    const MetadataNode* GetSelectedNode() const;
    
    // Menu handlers
    void OnTreeSelection(wxTreeEvent& event);
    void OnTreeItemMenu(wxTreeEvent& event);
    void OnTreeActivate(wxTreeEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnFilterClear(wxCommandEvent& event);
    
private:
    void BuildLayout();
    wxTreeItemId AddNodeToTree(wxTreeItemId parent, const MetadataNode* node);
    int GetIconForNode(const MetadataNode& node) const;
    
    wxTreeCtrl* tree_ = nullptr;
    wxTextCtrl* filter_ctrl_ = nullptr;
    wxButton* filter_clear_button_ = nullptr;
    wxImageList* tree_images_ = nullptr;
    
    std::string filter_text_;
    std::map<std::string, wxTreeItemId> node_map_;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LAYOUT_NAVIGATOR_PANEL_H
