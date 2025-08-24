#include <iostream>
#include <memory>
#include <exception>
#include <string>

#include <QApplication>

#include "core/application.h"
#include "core/connection_manager.h"
#include "core/metadata_manager.h"
#include "ui/main_window.h"
// #include "ui/splash_screen.h"
#include "utils/logger.h"

namespace scratchrobin {

struct Config {
    bool showHelp = false;
    bool showVersion = false;
};

Config parseCommandLine(int argc, char* argv[]) {
    Config config;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            config.showHelp = true;
        } else if (arg == "--version" || arg == "-v") {
            config.showVersion = true;
        }
    }
    return config;
}

void printUsage() {
    std::cout << "ScratchRobin v0.1.0 - Database Management Interface" << std::endl;
    std::cout << "Usage: scratchrobin [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help     Show this help message" << std::endl;
    std::cout << "  -v, --version  Show version information" << std::endl;
}

} // namespace scratchrobin

int main(int argc, char* argv[]) {
    try {
        // Initialize logger
        scratchrobin::Logger::init("scratchrobin.log");
        scratchrobin::Logger::info("Starting ScratchRobin v0.1.0");

        // Parse command line arguments
        auto config = scratchrobin::parseCommandLine(argc, argv);
        if (config.showHelp) {
            scratchrobin::printUsage();
            return 0;
        }

        if (config.showVersion) {
            std::cout << "ScratchRobin v0.1.0" << std::endl;
            return 0;
        }

        // Initialize core components
        scratchrobin::Logger::info("Initializing core components...");

        auto connectionManager = std::make_unique<scratchrobin::ConnectionManager>();
        auto metadataManager = std::make_unique<scratchrobin::MetadataManager>(connectionManager.get());
        auto application = std::make_unique<scratchrobin::Application>(connectionManager.get(), metadataManager.get());

        // Initialize Qt application first
        scratchrobin::Logger::info("Initializing Qt application...");
        application->initializeQt();

        // Initialize UI
        scratchrobin::Logger::info("Initializing user interface...");
        auto mainWindow = std::make_unique<scratchrobin::MainWindow>(application.get());

        // Set the main window in the application
        application->setMainWindow(mainWindow.get());

        // Start the application
        scratchrobin::Logger::info("Starting application...");
        const int result = application->run();

        scratchrobin::Logger::info("Application shutdown complete");
        return result;

    } catch (const std::exception& e) {
        scratchrobin::Logger::error("Fatal error: " + std::string(e.what()));
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        scratchrobin::Logger::error("Unknown fatal error occurred");
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}
