#include "main_window.h"
#include "core/application.h"
#include "utils/logger.h"

namespace scratchrobin {

class MainWindow::Impl {
public:
    Impl(Application* app) : application_(app) {}
    Application* application_;
    std::string title_ = "ScratchRobin";
    int width_ = 1024;
    int height_ = 768;
    bool visible_ = false;
};

MainWindow::MainWindow(Application* application)
    : impl_(std::make_unique<Impl>(application)) {
    Logger::info("MainWindow created");
}

MainWindow::~MainWindow() {
    Logger::info("MainWindow destroyed");
}

void MainWindow::show() {
    impl_->visible_ = true;
    Logger::info("MainWindow shown: " + impl_->title_);
    // In a real implementation, this would show the actual window
}

void MainWindow::hide() {
    impl_->visible_ = false;
    Logger::info("MainWindow hidden");
}

bool MainWindow::isVisible() const {
    return impl_->visible_;
}

void MainWindow::close() {
    impl_->visible_ = false;
    Logger::info("MainWindow closed");
}

void MainWindow::setTitle(const std::string& title) {
    impl_->title_ = title;
    Logger::info("MainWindow title set to: " + title);
}

std::string MainWindow::getTitle() const {
    return impl_->title_;
}

void MainWindow::setSize(int width, int height) {
    impl_->width_ = width;
    impl_->height_ = height;
    Logger::info("MainWindow size set to: " + std::to_string(width) + "x" + std::to_string(height));
}

void MainWindow::getSize(int& width, int& height) const {
    width = impl_->width_;
    height = impl_->height_;
}

} // namespace scratchrobin
