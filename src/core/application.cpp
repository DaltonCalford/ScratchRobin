#include "application.h"
#include "connection_manager.h"
#include "metadata_manager.h"
#include "utils/logger.h"
#include "ui/main_window.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <QApplication>
#include <QTimer>

namespace scratchrobin {

class Application::Impl {
public:
    Impl(ConnectionManager* connectionManager, MetadataManager* metadataManager)
        : connectionManager_(connectionManager)
        , metadataManager_(metadataManager)
        , mainWindow_(nullptr)
        , isRunning_(false)
        , applicationName_("ScratchRobin")
        , applicationVersion_("0.1.0")
    {}

    ConnectionManager* connectionManager_;
    MetadataManager* metadataManager_;
    MainWindow* mainWindow_;
    bool isRunning_;
    std::string applicationName_;
    std::string applicationVersion_;
};

Application::Application(ConnectionManager* connectionManager, MetadataManager* metadataManager)
    : impl_(std::make_unique<Impl>(connectionManager, metadataManager)) {
    Logger::info("Application initialized: " + impl_->applicationName_ + " v" + impl_->applicationVersion_);
}

Application::~Application() {
    if (impl_->isRunning_) {
        shutdown();
    }
    Logger::info("Application destroyed");
}

void Application::setMainWindow(MainWindow* mainWindow) {
    impl_->mainWindow_ = mainWindow;
    Logger::info("MainWindow set for application");
}

void Application::initializeQt() {
    if (!qApp) {
        Logger::info("Creating QApplication");
        static int argc = 0;
        static char* argv[] = {nullptr};
        new QApplication(argc, argv);
    }
}

int Application::run() {
    Logger::info("Starting application main loop");

    impl_->isRunning_ = true;

    try {
        // Ensure Qt application is initialized
        initializeQt();

        // Show the main window if available
        if (impl_->mainWindow_) {
            Logger::info("Showing main window");
            impl_->mainWindow_->show();
        } else {
            Logger::warn("No main window set for application");
        }

        // Start Qt application event loop
        Logger::info("Starting Qt application event loop");
        QApplication* app = qApp;
        if (app) {
            int result = app->exec();
            Logger::info("Qt application event loop ended");
            return result;
        } else {
            Logger::error("No QApplication available");
            return 1;
        }

    } catch (const std::exception& e) {
        Logger::error("Exception in application main loop: " + std::string(e.what()));
        return 1;
    } catch (...) {
        Logger::error("Unknown exception in application main loop");
        return 1;
    }
}

void Application::shutdown() {
    Logger::info("Application shutdown initiated");

    impl_->isRunning_ = false;

    // Clean up resources
    if (impl_->connectionManager_) {
        impl_->connectionManager_->shutdown();
    }

    Logger::info("Application shutdown complete");
}

bool Application::isRunning() const {
    return impl_->isRunning_;
}

void Application::setApplicationName(const std::string& name) {
    impl_->applicationName_ = name;
    Logger::info("Application name set to: " + name);
}

std::string Application::getApplicationName() const {
    return impl_->applicationName_;
}

void Application::setApplicationVersion(const std::string& version) {
    impl_->applicationVersion_ = version;
    Logger::info("Application version set to: " + version);
}

std::string Application::getApplicationVersion() const {
    return impl_->applicationVersion_;
}

ConnectionManager* Application::getConnectionManager() const {
    return impl_->connectionManager_;
}

MetadataManager* Application::getMetadataManager() const {
    return impl_->metadataManager_;
}

} // namespace scratchrobin
