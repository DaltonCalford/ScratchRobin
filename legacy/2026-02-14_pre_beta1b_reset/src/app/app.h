/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_APP_H
#define SCRATCHROBIN_APP_H

#include <memory>
#include <vector>

#include <wx/app.h>

#include "core/config.h"
#include "core/metadata_model.h"
#include "ui/window_manager.h"

namespace scratchrobin {

class StartupFrame;

class ScratchRobinApp : public wxApp {
public:
    ~ScratchRobinApp() override;
    bool OnInit() override;
    int OnExit() override;
    const std::vector<ConnectionProfile>& GetConnections() const { return connections_; }
    ConnectionManager* GetConnectionManager() { return &connection_manager_; }
    const AppConfig& GetConfig() const { return app_config_; }

private:
    void LoadConfiguration();

    AppConfig app_config_;
    std::vector<ConnectionProfile> connections_;
    ConnectionManager connection_manager_;
    std::unique_ptr<WindowManager> window_manager_;
    std::unique_ptr<MetadataModel> metadata_model_;
    StartupFrame* startup_frame_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_APP_H
