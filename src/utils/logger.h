#ifndef SCRATCHROBIN_LOGGER_H
#define SCRATCHROBIN_LOGGER_H

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <chrono>

namespace scratchrobin {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static void init(const std::string& filename = "scratchrobin.log");
    static void shutdown();

    static void setLevel(LogLevel level);
    static LogLevel getLevel();

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);

    static void setMaxFileSize(size_t size);
    static void setMaxFiles(size_t count);

private:
    Logger() = default;
    ~Logger() = default;

    static Logger& getInstance();

    void initImpl(const std::string& filename);
    void shutdownImpl();
    void logImpl(LogLevel level, const std::string& message);
    void rotateLogIfNeeded();

    std::unique_ptr<std::ofstream> logFile_;
    LogLevel currentLevel_ = LogLevel::INFO;
    std::string filename_;
    size_t maxFileSize_ = 10 * 1024 * 1024; // 10MB
    size_t maxFiles_ = 5;
    mutable std::mutex mutex_;

    // Disable copy and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LOGGER_H
