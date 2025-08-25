#include "utils/logger.h"
#include <gtest/gtest.h>

using namespace scratchrobin;

TEST(LoggerTest, Initialization) {
    Logger::init("test.log");
    EXPECT_NO_THROW(Logger::info("Test message"));
    Logger::shutdown();
}

TEST(LoggerTest, LogLevels) {
    Logger::init("test.log");
    Logger::setLevel(LogLevel::DEBUG);

    EXPECT_NO_THROW(Logger::debug("Debug message"));
    EXPECT_NO_THROW(Logger::info("Info message"));
    EXPECT_NO_THROW(Logger::warn("Warning message"));
    EXPECT_NO_THROW(Logger::error("Error message"));

    Logger::shutdown();
}
