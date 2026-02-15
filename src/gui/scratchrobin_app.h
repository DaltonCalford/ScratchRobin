#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <wx/app.h>

#include "runtime/runtime_services.h"

namespace scratchrobin::gui {

class ScratchRobinApp final : public wxApp {
public:
    bool OnInit() override;
    int OnExit() override;

private:
    static std::filesystem::path FindRepoRoot(const char* argv0);

    runtime::StartupPaths startup_paths_;
    std::unique_ptr<runtime::ScratchRobinRuntime> runtime_;
};

}  // namespace scratchrobin::gui

