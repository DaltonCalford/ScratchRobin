/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_WINDOW_MANAGER_H
#define SCRATCHROBIN_WINDOW_MANAGER_H

#include <cstddef>
#include <unordered_set>
#include <vector>

#include <wx/frame.h>

namespace scratchrobin {

class WindowManager {
public:
    void RegisterWindow(wxFrame* frame);
    void UnregisterWindow(wxFrame* frame);
    size_t WindowCount() const;
    void CloseAll();
    void RegisterDiagramHost(wxFrame* frame);
    void UnregisterDiagramHost(wxFrame* frame);
    wxFrame* GetDiagramHost() const;
    std::vector<wxFrame*> GetWindows() const;

private:
    std::unordered_set<wxFrame*> windows_;
    wxFrame* diagram_host_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_WINDOW_MANAGER_H
