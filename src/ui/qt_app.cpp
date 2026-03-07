#include "ui/qt_app.h"
#include "ui/main_window.h"
#include "ui/splash_screen.h"
#include "backend/scratchbird_runtime_config.h"
#include "backend/parser_port_registry.h"
#include "backend/native_parser_compiler.h"
#include "backend/server_session_gateway.h"
#include "backend/native_adapter_gateway.h"
#include "backend/session_client.h"
#include "core/app_config.h"

#include <QMessageBox>
#include <QThread>
#include <QEventLoop>

namespace scratchrobin::ui {

QtApp::QtApp(int& argc, char** argv)
    : QApplication(argc, argv)
    , main_window_(nullptr)
    , splash_screen_(nullptr) {
  setApplicationName("ScratchRobin");
  setApplicationVersion("0.1.0");
  setOrganizationName("ScratchBird");
}

QtApp::~QtApp() = default;

bool QtApp::init() {
  splash_screen_ = new SplashScreen();
  splash_screen_->showSplash();
  splash_screen_->beginLoading();

  splash_screen_->setStepConfigLoad();
  if (!initializeBackend()) {
    splash_screen_->closeSplash();
    delete splash_screen_;
    splash_screen_ = nullptr;
    return false;
  }

  auto& config = core::AppConfig::get();
  bool config_exists = config.configExists();
  
  if (config_exists) {
    if (!config.load()) config.resetToDefaults();
  } else {
    config.resetToDefaults();
  }

  if (config.hasEncryptedPasswords() || config_exists) {
    splash_screen_->setStepDecryptPasswords();
    QThread::msleep(100);
  }

  splash_screen_->setStepInitUI();
  if (!createMainWindow()) {
    splash_screen_->closeSplash();
    delete splash_screen_;
    splash_screen_ = nullptr;
    return false;
  }

  splash_screen_->setStepComplete();
  splash_screen_->closeSplash();
  delete splash_screen_;
  splash_screen_ = nullptr;
  
  return true;
}

bool QtApp::initializeBackend() {
  registry_ = std::make_unique<backend::ParserPortRegistry>();
  compiler_ = std::make_unique<backend::NativeParserCompiler>();
  session_ = std::make_unique<backend::ServerSessionGateway>();
  adapter_ = std::make_unique<backend::NativeAdapterGateway>(registry_.get(), compiler_.get(), session_.get());
  session_client_ = std::make_unique<backend::SessionClient>(adapter_.get());

  auto register_status = registry_->Register(4044, "scratchbird-native");
  if (!register_status.ok) {
    QMessageBox::critical(nullptr, tr("Error"), tr("Failed to register parser port: %1").arg(QString::fromStdString(register_status.message)));
    return false;
  }

  backend::ScratchbirdRuntimeConfig runtime;
  runtime.mode = backend::TransportMode::kPreview;
  runtime.host = "127.0.0.1";
  runtime.port = 4044;
  runtime.database = "default";
  session_client_->ConfigureRuntime(runtime);

  return true;
}

bool QtApp::createMainWindow() {
  main_window_ = new MainWindow(session_client_.get());
  
  auto& config = core::AppConfig::get();
  auto* layout = config.getCurrentLayout();
  if (layout) {
    auto main_win_opt = layout->findWindow("main");
    if (main_win_opt.has_value()) {
      const auto& main_win = main_win_opt.value();
      if (main_win.maximized) {
        main_window_->showMaximized();
      } else {
        main_window_->setGeometry(main_win.x, main_win.y, main_win.width, main_win.height);
        main_window_->show();
      }
    } else {
      main_window_->show();
    }
  } else {
    // No saved layout, show normally
    main_window_->show();
  }
  
  main_window_->raise();
  main_window_->activateWindow();
  
  // Process events to ensure window is fully created and visible
  QApplication::processEvents();
  
  return true;
}

int QtApp::run() {
  return exec();
}

}  // namespace scratchrobin::ui
