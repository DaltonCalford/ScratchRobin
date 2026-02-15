#include "gui/scratchrobin_app.h"

#include <vector>

#include <wx/msgdlg.h>

#include "core/reject.h"
#include "gui/main_frame.h"

namespace scratchrobin::gui {

std::filesystem::path ScratchRobinApp::FindRepoRoot(const char* argv0) {
    namespace fs = std::filesystem;
    std::vector<fs::path> roots;
    roots.push_back(fs::current_path());
    roots.push_back(fs::current_path().parent_path());
    if (argv0 != nullptr) {
        std::error_code ec;
        fs::path exe = fs::absolute(argv0, ec);
        if (!ec) {
            roots.push_back(exe.parent_path());
            roots.push_back(exe.parent_path().parent_path());
        }
    }
    for (const auto& candidate : roots) {
        if (candidate.empty()) {
            continue;
        }
        if (fs::exists(candidate / "config/scratchrobin.toml.example") &&
            fs::exists(candidate / "config/connections.toml.example")) {
            return candidate;
        }
    }
    return fs::current_path();
}

bool ScratchRobinApp::OnInit() {
    if (!wxApp::OnInit()) {
        return false;
    }

    const auto repo_root = FindRepoRoot(nullptr);

    startup_paths_.app_config_path = (repo_root / "config/scratchrobin.toml").string();
    startup_paths_.app_config_example_path = (repo_root / "config/scratchrobin.toml.example").string();
    startup_paths_.connections_path = (repo_root / "config/connections.toml").string();
    startup_paths_.connections_example_path = (repo_root / "config/connections.toml.example").string();
    startup_paths_.session_state_path = (repo_root / "work/session_state.json").string();

    runtime_ = std::make_unique<runtime::ScratchRobinRuntime>();
    runtime::StartupReport report;
    try {
        report = runtime_->Startup(startup_paths_);
    } catch (const RejectError& ex) {
        wxMessageBox("Runtime startup failed:\n" + wxString(ex.what()),
                     "ScratchRobin Startup Error",
                     wxOK | wxICON_ERROR);
        runtime_.reset();
        return false;
    } catch (const std::exception& ex) {
        wxMessageBox("Runtime startup failed:\n" + wxString(ex.what()),
                     "ScratchRobin Startup Error",
                     wxOK | wxICON_ERROR);
        runtime_.reset();
        return false;
    }

    auto* frame = new MainFrame(report, repo_root);
    frame->Show(true);
    SetTopWindow(frame);
    return true;
}

int ScratchRobinApp::OnExit() {
    if (runtime_ != nullptr) {
        try {
            runtime_->Shutdown(startup_paths_);
        } catch (...) {
            // Never block process shutdown on cleanup failures.
        }
    }
    runtime_.reset();
    return wxApp::OnExit();
}

}  // namespace scratchrobin::gui
