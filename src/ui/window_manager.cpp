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

} // namespace scratchrobin
