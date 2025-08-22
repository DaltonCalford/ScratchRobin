#include "logger.h"

#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sstream>

namespace scratchrobin {

namespace fs = std::filesystem;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::init(const std::string& filename) {
    getInstance().initImpl(filename);
}

void Logger::shutdown() {
    getInstance().shutdownImpl();
}

void Logger::setLevel(LogLevel level) {
    getInstance().currentLevel_ = level;
}

LogLevel Logger::getLevel() {
    return getInstance().currentLevel_;
}

void Logger::debug(const std::string& message) {
    getInstance().logImpl(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    getInstance().logImpl(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    getInstance().logImpl(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    getInstance().logImpl(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    getInstance().logImpl(LogLevel::FATAL, message);
}

void Logger::setMaxFileSize(size_t size) {
    getInstance().maxFileSize_ = size;
}

void Logger::setMaxFiles(size_t count) {
    getInstance().maxFiles_ = count;
}

void Logger::initImpl(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (logFile_ && logFile_->is_open()) {
        logFile_->close();
    }

    filename_ = filename;

    // Create logs directory if it doesn't exist
    fs::path logPath(filename);
    if (logPath.has_parent_path()) {
        fs::create_directories(logPath.parent_path());
    }

    logFile_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    if (!logFile_->is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
        return;
    }

    // Log initialization
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time);
    timestamp = timestamp.substr(0, timestamp.length() - 1); // Remove newline

    *logFile_ << timestamp << " [INFO] Logger initialized: " << filename << std::endl;
    logFile_->flush();
}

void Logger::shutdownImpl() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (logFile_ && logFile_->is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::string timestamp = std::ctime(&time);
        timestamp = timestamp.substr(0, timestamp.length() - 1);

        *logFile_ << timestamp << " [INFO] Logger shutdown" << std::endl;
        logFile_->flush();
        logFile_->close();
    }
}

void Logger::logImpl(LogLevel level, const std::string& message) {
    // Check if we should log this level
    if (level < currentLevel_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Also output to console for certain levels
    std::ostream& console = (level >= LogLevel::ERROR) ? std::cerr : std::cout;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time);
    timestamp = timestamp.substr(0, timestamp.length() - 1);

    // Level string
    std::string levelStr;
    switch (level) {
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO:  levelStr = "INFO";  break;
        case LogLevel::WARN:  levelStr = "WARN";  break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
        case LogLevel::FATAL: levelStr = "FATAL"; break;
    }

    // Format log message
    std::string logMessage = timestamp + " [" + levelStr + "] " + message;

    // Output to console
    console << logMessage << std::endl;

    // Output to file if available
    if (logFile_ && logFile_->is_open()) {
        *logFile_ << logMessage << std::endl;
        logFile_->flush();

        // Check if we need to rotate
        rotateLogIfNeeded();
    }
}

void Logger::rotateLogIfNeeded() {
    if (!logFile_ || !logFile_->is_open()) {
        return;
    }

    // Check file size
    logFile_->seekp(0, std::ios::end);
    size_t size = logFile_->tellp();

    if (size >= maxFileSize_) {
        logFile_->close();

        // Rotate existing log files
        for (size_t i = maxFiles_; i > 0; --i) {
            std::string src = filename_ + (i == 1 ? "" : "." + std::to_string(i - 1));
            std::string dst = filename_ + "." + std::to_string(i);

            if (fs::exists(src)) {
                if (i == maxFiles_) {
                    fs::remove(src); // Remove oldest file
                } else {
                    fs::rename(src, dst);
                }
            }
        }

        // Create new log file
        logFile_ = std::make_unique<std::ofstream>(filename_, std::ios::out);
        if (logFile_->is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::string timestamp = std::ctime(&time);
            timestamp = timestamp.substr(0, timestamp.length() - 1);

            *logFile_ << timestamp << " [INFO] Log rotated" << std::endl;
        }
    }
}

} // namespace scratchrobin
