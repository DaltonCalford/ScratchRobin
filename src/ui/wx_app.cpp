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

#include "backend/scratchbird_runtime_config.h"
#include "core/app_config.h"
#include "ui/main_frame_new.h"
#include "ui/splash_screen.h"
#include "ui/unlock_dialog.h"

namespace scratchrobin::ui {

bool ScratchbirdWxApp::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }

  // Show splash screen immediately
  splash_ = new SplashScreen(nullptr);
  splash_->ShowSplash();
  splash_->BeginLoading();

  // Initialize backend
  splash_->SetStepConfigLoad();
  registry_ = std::make_unique<backend::ParserPortRegistry>();
  compiler_ = std::make_unique<backend::NativeParserCompiler>();
  session_ = std::make_unique<backend::ServerSessionGateway>();
  adapter_ = std::make_unique<backend::NativeAdapterGateway>(registry_.get(), compiler_.get(), session_.get());
  session_client_ = std::make_unique<backend::SessionClient>(adapter_.get());

  const auto register_status = registry_->Register(4044, "scratchbird-native");
  if (!register_status.ok) {
    splash_->CloseSplash();
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
    // Load config without decrypting passwords yet
    if (!config.load()) {
      // Config file exists but failed to load - create default
      config.resetToDefaults();
    }
  } else {
    // First run - create default config
    config.resetToDefaults();
  }

  // Check if we have encrypted passwords that need decryption
  splash_->SetStepDecryptPasswords();
  
  if (config.hasEncryptedPasswords() || config_exists) {
    // Show unlock dialog
    splash_->Hide();  // Hide splash while unlock dialog is shown
    
    UnlockDialog unlockDlg(nullptr, config.hasEncryptedPasswords());
    wxString key = unlockDlg.ShowAndGetKey();
    
    if (!key.IsEmpty()) {
      // User provided a key - try to decrypt
      if (!config.decryptPasswords(key.ToStdString())) {
        // Decryption failed - wrong key
        wxMessageBox(wxT("The decryption key is incorrect. Passwords will not be available."),
                     wxT("Decryption Failed"), wxOK | wxICON_WARNING);
      }
    } else {
      // User cancelled or skipped - no auto-connect
      // Continue without password decryption
    }
    
    splash_->ShowSplash();
  }

  // Initialize UI
  splash_->SetStepInitUI();
  
  frame_ = new MainFrame(session_client_.get());
  
  // Apply saved window position from config
  core::ScreenLayout* layout = config.getCurrentLayout();
  if (layout) {
    auto main_win = layout->findWindow("main");
    if (main_win) {
      // Apply position and size
      if (main_win->maximized) {
        frame_->Maximize(true);
      } else {
        frame_->SetSize(main_win->x, main_win->y, 
                       main_win->width, main_win->height);
      }
    }
  }

  // Complete loading
  splash_->SetStepComplete();
  splash_->CloseSplash();
  
  // Show main frame
  frame_->Show(true);
  frame_->Raise();
  
  // Cleanup splash
  delete splash_;
  splash_ = nullptr;
  
  return true;
}

int ScratchbirdWxApp::OnExit() {
  // Save window position before exiting
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
        
        // Determine which display this is on
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
  
  // Save configuration
  config.save();
  
  return wxApp::OnExit();
}

}  // namespace scratchrobin::ui
