#include "ui/main_window.h"

namespace scratchrobin::ui {

std::string MainWindow::GetTitle() const {
  return "robin-migrate (ScratchBird Native)";
}

std::string MainWindow::GetLayoutModel() const {
  return "left tree + top tabs + sql editor + result grid";
}

}  // namespace scratchrobin::ui
