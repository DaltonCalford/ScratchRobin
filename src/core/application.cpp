#include "application.h"
#include "connection_manager.h"
#include "metadata_manager.h"
#include "utils/logger.h"

#include <iostream>
#include <thread>
#include <chrono>

namespace scratchrobin {

class Application::Impl {
public:
    Impl(ConnectionManager* connectionManager, MetadataManager* metadataManager)
        : connectionManager_(connectionManager)
        , metadataManager_(metadataManager)
        , isRunning_(false)
        , applicationName_("ScratchRobin")
        , applicationVersion_("0.1.0")
    {}

    ConnectionManager* connectionManager_;
    MetadataManager* metadataManager_;
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

int Application::run() {
    Logger::info("Starting application main loop");

    impl_->isRunning_ = true;

    try {
        // Application main loop
        while (impl_->isRunning_) {
            // Process events, update UI, etc.
            // This would be replaced with actual UI framework integration
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Check for shutdown signal
            // In a real implementation, this would be handled by the UI framework
        }

        Logger::info("Application main loop ended");
        return 0;

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
