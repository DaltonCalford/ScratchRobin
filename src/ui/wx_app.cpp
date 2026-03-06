/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/wx_app.h"

#include <memory>

#include <wx/init.h>
#include <wx/display.h>
#include <wx/msgdlg.h>
#include <wx/frame.h>

#include "backend/scratchbird_runtime_config.h"
#include "core/app_config.h"
#include "ui/unlock_dialog.h"
#include "ui/main_frame_new.h"

namespace scratchrobin::ui {

bool ScratchbirdWxApp::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }

  // Initialize backend
  registry_ = std::make_unique<backend::ParserPortRegistry>();
  compiler_ = std::make_unique<backend::NativeParserCompiler>();
  session_ = std::make_unique<backend::ServerSessionGateway>();
  adapter_ = std::make_unique<backend::NativeAdapterGateway>(registry_.get(), compiler_.get(), session_.get());
  session_client_ = std::make_unique<backend::SessionClient>(adapter_.get());

  const auto register_status = registry_->Register(4044, "scratchbird-native");
  if (!register_status.ok) {
    return false;
  }

  backend::ScratchbirdRuntimeConfig runtime;
  runtime.mode = backend::TransportMode::kPreview;
  runtime.host = "127.0.0.1";
  runtime.port = 4044;
  runtime.database = "default";
  session_client_->ConfigureRuntime(runtime);

  // Load application configuration
  core::AppConfig& config = core::AppConfig::get();
  bool config_exists = config.configExists();
  
  if (config_exists) {
    if (!config.load()) {
      config.resetToDefaults();
    }
  } else {
    config.resetToDefaults();
  }

  // Check if we have encrypted passwords that need decryption
  if (config.hasEncryptedPasswords() || config_exists) {
    UnlockDialog unlockDlg(nullptr, config.hasEncryptedPasswords());
    wxString key = unlockDlg.ShowAndGetKey();
    
    if (!key.IsEmpty()) {
      if (!config.decryptPasswords(key.ToStdString())) {
        wxMessageBox(wxT("The decryption key is incorrect. Passwords will not be available."),
                     wxT("Decryption Failed"), wxOK | wxICON_WARNING);
      }
    }
  }

  // Create main frame
  frame_ = new MainFrame(session_client_.get());
  frame_->Show(true);
  frame_->Raise();
  
  return true;
}

int ScratchbirdWxApp::OnExit() {
  core::AppConfig& config = core::AppConfig::get();
  
  if (config.shouldAutoSavePositions() && frame_) {
    core::ScreenLayout* layout = config.getCurrentLayout();
    if (layout) {
      core::WindowLayout main_win;
      main_win.window_id = "main";
      main_win.maximized = frame_->IsMaximized();
      
      if (!main_win.maximized) {
        wxRect rect = frame_->GetRect();
        main_win.x = rect.x;
        main_win.y = rect.y;
        main_win.width = rect.width;
        main_win.height = rect.height;
        
        int display_count = wxDisplay::GetCount();
        for (int i = 0; i < display_count; ++i) {
          wxDisplay display(i);
          if (display.GetGeometry().Contains(rect.GetPosition())) {
            main_win.display_index = i;
            break;
          }
        }
      }
      
      layout->updateWindow(main_win);
      config.setModified(true);
    }
  }
  
  config.save();
  
  return wxApp::OnExit();
}

}  // namespace scratchrobin::ui
