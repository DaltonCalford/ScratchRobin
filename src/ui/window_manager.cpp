/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "window_manager.h"

#include <vector>

namespace scratchrobin {

void WindowManager::RegisterWindow(wxFrame* frame) {
    if (!frame) {
        return;
    }
    windows_.insert(frame);
}

void WindowManager::UnregisterWindow(wxFrame* frame) {
    if (!frame) {
        return;
    }
    windows_.erase(frame);
}

size_t WindowManager::WindowCount() const {
    return windows_.size();
}

void WindowManager::CloseAll() {
    std::vector<wxFrame*> to_close(windows_.begin(), windows_.end());
    for (auto* frame : to_close) {
        if (frame) {
            frame->Close(true);
        }
    }
}

void WindowManager::RegisterDiagramHost(wxFrame* frame) {
    diagram_host_ = frame;
}

void WindowManager::UnregisterDiagramHost(wxFrame* frame) {
    if (diagram_host_ == frame) {
        diagram_host_ = nullptr;
    }
}

wxFrame* WindowManager::GetDiagramHost() const {
    return diagram_host_;
}

std::vector<wxFrame*> WindowManager::GetWindows() const {
    return std::vector<wxFrame*>(windows_.begin(), windows_.end());
}

} // namespace scratchrobin
