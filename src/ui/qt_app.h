#pragma once
#include <QApplication>
#include <memory>

namespace scratchrobin::backend {
class ParserPortRegistry;
class NativeParserCompiler;
class ServerSessionGateway;
class NativeAdapterGateway;
class SessionClient;
}

namespace scratchrobin::ui {

class MainWindow;
class SplashScreen;

class QtApp : public QApplication {
  Q_OBJECT

 public:
  QtApp(int& argc, char** argv);
  ~QtApp() override;

  bool init();
  int run();

 private:
  bool initializeBackend();
  bool createMainWindow();

  std::unique_ptr<backend::ParserPortRegistry> registry_;
  std::unique_ptr<backend::NativeParserCompiler> compiler_;
  std::unique_ptr<backend::ServerSessionGateway> session_;
  std::unique_ptr<backend::NativeAdapterGateway> adapter_;
  std::unique_ptr<backend::SessionClient> session_client_;
  
  MainWindow* main_window_;
  SplashScreen* splash_screen_;
};

}  // namespace scratchrobin::ui
