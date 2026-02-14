/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_LAYOUT_INSPECTOR_PANEL_H
#define SCRATCHROBIN_LAYOUT_INSPECTOR_PANEL_H

#include "ui/layout/dockable_window.h"
#include "core/metadata_model.h"
#include <wx/panel.h>
#include <wx/notebook.h>

class wxTextCtrl;

namespace scratchrobin {

// Inspector panel - shows details of selected database object
class InspectorPanel : public wxPanel, public IDockableWindow {
public:
    InspectorPanel(wxWindow* parent);
    
    // IDockableWindow implementation
    std::string GetWindowId() const override { return "inspector"; }
    std::string GetWindowTitle() const override { return "Inspector"; }
    std::string GetWindowType() const override { return "inspector"; }
    wxWindow* GetWindow() override { return this; }
    const wxWindow* GetWindow() const override { return this; }
    
    // Display
    void ShowNode(const MetadataNode* node);
    void Clear();
    
private:
    void BuildLayout();
    void UpdateOverview(const MetadataNode* node);
    void UpdateDDL(const MetadataNode* node);
    void UpdateDependencies(const MetadataNode* node);
    std::string BuildSeedSql(const MetadataNode* node) const;
    
    wxNotebook* notebook_ = nullptr;
    wxTextCtrl* overview_text_ = nullptr;
    wxTextCtrl* ddl_text_ = nullptr;
    wxTextCtrl* deps_text_ = nullptr;
    
    int overview_page_index_ = 0;
    int ddl_page_index_ = 1;
    int deps_page_index_ = 2;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LAYOUT_INSPECTOR_PANEL_H
