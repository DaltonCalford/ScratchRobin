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

class wxNotebook;
class wxPanel;

namespace scratchrobin {

class WindowManager;

class DiagramFrame : public wxFrame {
public:
    DiagramFrame(WindowManager* windowManager, const AppConfig* config);
    void AddDiagramTab(const wxString& title = "Diagram");

private:
    void OnNewDiagram(wxCommandEvent& event);
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

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_FRAME_H
