#include "ui/qt_app.h"

int main(int argc, char* argv[]) {
  scratchrobin::ui::QtApp app(argc, argv);
  
  if (!app.init()) {
    return 1;
  }
  
  return app.run();
}
