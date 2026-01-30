/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "app.h"

#include "core/metadata_model.h"
#include "ui/main_frame.h"
#include "ui/startup_frame.h"
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
    if (app_config_.startup.enabled) {
        startup_frame_ = new StartupFrame(app_config_.startup);
        startup_frame_->Show(true);
    }
    connection_manager_.SetNetworkOptions(app_config_.network);
    if (!connections_.empty()) {
        metadata_model_->UpdateConnections(connections_);
        if (!connections_.front().fixturePath.empty()) {
            metadata_model_->SetFixturePath(connections_.front().fixturePath);
        }
        metadata_model_->Refresh();
    } else {
        metadata_model_->LoadStub();
    }

    auto* frame = new MainFrame(window_manager_.get(), metadata_model_.get(),
                                &connection_manager_, &connections_, &app_config_);
    frame->Show(true);

    if (startup_frame_) {
        startup_frame_->Hide();
        startup_frame_->Destroy();
        startup_frame_ = nullptr;
    }
    return true;
}

int ScratchRobinApp::OnExit() {
    if (window_manager_) {
        window_manager_->CloseAll();
    }
    if (startup_frame_) {
        startup_frame_->Destroy();
        startup_frame_ = nullptr;
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
