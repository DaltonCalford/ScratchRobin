/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_FRAME_H
#define SCRATCHROBIN_DIAGRAM_FRAME_H

#include <wx/frame.h>

#include "core/config.h"
#include "diagram_model.h"
#include "form_container.h"

class wxNotebook;
class wxPanel;

namespace scratchrobin {

class WindowManager;
class DiagramPage;

class DiagramFrame : public wxFrame, public ui::IFormWindow {
public:
    DiagramFrame(WindowManager* windowManager, const AppConfig* config);
    
    // IFormWindow implementation
    ui::FormCategory GetFormCategory() const override { return ui::FormCategory::Diagram; }
    std::string GetFormId() const override;
    wxString GetFormTitle() const override;
    wxWindow* GetWindow() override { return this; }
    const wxWindow* GetWindow() const override { return this; }
    void OnFormActivated() override;
    void OnFormDeactivated() override;
    bool CanAcceptChild(ui::IFormWindow* child) const override;
    void AddChildForm(ui::IFormWindow* child) override;
    void RemoveChildForm(ui::IFormWindow* child) override;
    
    // Child diagram management
    void AddEmbeddedDiagram(DiagramFrame* childDiagram, int x, int y);
    void RemoveEmbeddedDiagram(const std::string& diagramId);
    bool HasEmbeddedDiagrams() const { return !childDiagrams_.empty(); }
    void AddDiagramTab(const wxString& title = "Diagram");
    void AddDiagramTabOfType(DiagramType type, const wxString& title = "");
    void SetDefaultDiagramType(DiagramType type) { default_type_ = type; }
    DiagramPage* GetActiveDiagramPage() const;
    bool FocusNodeInOpenDiagrams(const std::string& ref, DiagramType preferred_type);
    bool OpenDiagramFromTrace(const std::string& ref);
    bool OpenDiagramFile(const std::string& path, const std::string& focus_node);

private:
    void OnNewDiagram(wxCommandEvent& event);
    void OnOpenDiagram(wxCommandEvent& event);
    void OnSaveDiagram(wxCommandEvent& event);
    void OnSaveDiagramAs(wxCommandEvent& event);
    void OnNewSqlEditor(wxCommandEvent& event);
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    WindowManager* window_manager_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    wxNotebook* notebook_ = nullptr;
    int diagram_counter_ = 0;
    DiagramType default_type_ = DiagramType::Erd;
    
    // Embedded child diagrams (diagram containment)
    struct EmbeddedDiagram {
        std::string diagramId;
        wxString title;
        int x, y;
        class DiagramMiniView* miniView;
    };
    std::vector<EmbeddedDiagram> childDiagrams_;
    std::string formId_;  // Unique ID for this form instance
    static int nextFormId_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_FRAME_H
