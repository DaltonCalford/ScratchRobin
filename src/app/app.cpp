#include "app.h"
#include "ui/main_frame.h"

namespace scratchrobin {

bool ScratchRobinApp::OnInit() {
    if (!wxApp::OnInit()) {
        return false;
    }

    auto* frame = new MainFrame();
    frame->Show(true);
    return true;
}

int ScratchRobinApp::OnExit() {
    return wxApp::OnExit();
}

} // namespace scratchrobin
