#include "ui/qt_app.h"
#include <QMessageBox>

int main(int argc, char* argv[]) {
  try {
    scratchrobin::ui::QtApp app(argc, argv);
    
    if (!app.init()) {
      return 1;
    }
    
    return app.run();
  } catch (const std::exception& e) {
    QMessageBox::critical(nullptr, "Fatal Error", 
                          QString("Application crashed: %1").arg(e.what()));
    return 1;
  } catch (...) {
    QMessageBox::critical(nullptr, "Fatal Error", "Unknown application error");
    return 1;
  }
}
