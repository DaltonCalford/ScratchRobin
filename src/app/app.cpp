#include "app.h"

#include "core/metadata_model.h"
#include "ui/main_frame.h"
#include "ui/window_manager.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace scratchrobin {

ScratchRobinApp::~ScratchRobinApp() = default;

bool ScratchRobinApp::OnInit() {
    if (!wxApp::OnInit()) {
        return false;
    }

    window_manager_ = std::make_unique<WindowManager>();
    metadata_model_ = std::make_unique<MetadataModel>();

    LoadConfiguration();
    connection_manager_.SetNetworkOptions(app_config_.network);
    if (!connections_.empty()) {
        metadata_model_->UpdateConnections(connections_);
    } else {
        metadata_model_->LoadStub();
    }

    auto* frame = new MainFrame(window_manager_.get(), metadata_model_.get(),
                                &connection_manager_, &connections_, &app_config_);
    frame->Show(true);
    return true;
}

int ScratchRobinApp::OnExit() {
    if (window_manager_) {
        window_manager_->CloseAll();
    }
    return wxApp::OnExit();
}

void ScratchRobinApp::LoadConfiguration() {
    ConfigStore store;

    wxFileName config_root(wxStandardPaths::Get().GetUserConfigDir(), "");
    config_root.AppendDir("scratchrobin");

    wxFileName app_config_path(config_root);
    app_config_path.SetFullName("scratchrobin.toml");
    if (!store.LoadAppConfig(app_config_path.GetFullPath().ToStdString(), &app_config_)) {
        store.LoadAppConfig("config/scratchrobin.toml.example", &app_config_);
    }

    wxFileName connection_path(config_root);
    connection_path.SetFullName("connections.toml");
    if (!store.LoadConnections(connection_path.GetFullPath().ToStdString(), &connections_)) {
        store.LoadConnections("config/connections.toml.example", &connections_);
    }
}

} // namespace scratchrobin
