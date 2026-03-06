/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <memory>

#include <wx/app.h>
#include <wx/frame.h>

namespace scratchrobin::ui {
  class MainFrame;
}

#include "backend/native_adapter_gateway.h"
#include "backend/native_parser_compiler.h"
#include "backend/server_session_gateway.h"
#include "backend/parser_port_registry.h"
#include "backend/session_client.h"

namespace scratchrobin::ui {



class ScratchbirdWxApp final : public wxApp {
 public:
  bool OnInit() override;
  int OnExit() override;

 private:
  std::unique_ptr<backend::ParserPortRegistry> registry_;
  std::unique_ptr<backend::NativeParserCompiler> compiler_;
  std::unique_ptr<backend::ServerSessionGateway> session_;
  std::unique_ptr<backend::NativeAdapterGateway> adapter_;
  std::unique_ptr<backend::SessionClient> session_client_;
  MainFrame* frame_{nullptr};
};

}  // namespace scratchrobin::ui
