#pragma once

#include <string>

namespace scratchrobin::ui {

class MainWindow {
 public:
  [[nodiscard]] std::string GetTitle() const;
  [[nodiscard]] std::string GetLayoutModel() const;
};

}  // namespace scratchrobin::ui
