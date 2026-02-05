/**
 * @file test_error_handler.cpp
 * @brief Unit tests for the error handler
 */

#include <gtest/gtest.h>
#include "core/error_handler.h"

using namespace scratchrobin;

class ErrorHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // ErrorMapper uses static methods, no setup needed
    }
};

TEST_F(ErrorHandlerTest, MapPostgresqlConnectionError) {
    auto error = ErrorMapper::MapBackendError(
        "postgresql",
        "08001",
        "08001",  // sqlclient_unable_to_establish_sqlconnection
        "could not connect to server"
    );
    
    EXPECT_EQ(error.category, ErrorCategory::Connection);
    EXPECT_EQ(error.code, "SR-1001");
    EXPECT_EQ(error.severity, ErrorSeverity::Error);
}

TEST_F(ErrorHandlerTest, MapPostgresqlAuthError) {
    auto error = ErrorMapper::MapBackendError(
        "postgresql",
        "28P01",
        "28P01",
        "password authentication failed"
    );
    
    EXPECT_EQ(error.category, ErrorCategory::Connection);
    EXPECT_EQ(error.code, "SR-1003");
    EXPECT_EQ(error.message, "Authentication failed");
}

TEST_F(ErrorHandlerTest, MapPostgresqlSyntaxError) {
    auto error = ErrorMapper::MapBackendError(
        "postgresql",
        "42601",
        "42601",  // syntax_error
        "syntax error at or near 'SELEC'"
    );
    
    EXPECT_EQ(error.category, ErrorCategory::Query);
    EXPECT_EQ(error.code, "SR-1101");
    EXPECT_EQ(error.message, "SQL syntax error");
}

TEST_F(ErrorHandlerTest, MapPostgresqlTableNotFound) {
    auto error = ErrorMapper::MapBackendError(
        "postgresql",
        "42P01",
        "42P01",
        "relation 'users' does not exist"
    );
    
    EXPECT_EQ(error.category, ErrorCategory::Query);
    EXPECT_EQ(error.code, "SR-1102");
}

TEST_F(ErrorHandlerTest, MapPostgresqlConstraintViolation) {
    auto error = ErrorMapper::MapBackendError(
        "postgresql",
        "23505",
        "23505",  // unique_violation
        "duplicate key value violates unique constraint"
    );
    
    EXPECT_EQ(error.category, ErrorCategory::Query);
    EXPECT_EQ(error.code, "SR-1104");
    EXPECT_EQ(error.message, "Constraint violation");
}

TEST_F(ErrorHandlerTest, MapPostgresqlDeadlock) {
    auto error = ErrorMapper::MapBackendError(
        "postgresql",
        "40P01",
        "40P01",  // deadlock_detected
        "deadlock detected"
    );
    
    EXPECT_EQ(error.category, ErrorCategory::Transaction);
    EXPECT_EQ(error.code, "SR-1105");
    EXPECT_EQ(error.severity, ErrorSeverity::Warning);
    EXPECT_TRUE(error.IsRetryable());
}

TEST_F(ErrorHandlerTest, MapMysqlError) {
    // MySQL access denied
    auto error = ErrorMapper::MapBackendError(
        "mysql",
        "1045",  // ER_ACCESS_DENIED_ERROR
        "",
        "Access denied for user"
    );
    
    EXPECT_EQ(error.code, "SR-1003");
    EXPECT_EQ(error.category, ErrorCategory::Connection);
}

TEST_F(ErrorHandlerTest, MapMysqlSyntaxError) {
    auto error = ErrorMapper::MapBackendError(
        "mysql",
        "1064",  // ER_PARSE_ERROR
        "",
        "You have an error in your SQL syntax"
    );
    
    EXPECT_EQ(error.code, "SR-1101");
    EXPECT_EQ(error.category, ErrorCategory::Query);
}

TEST_F(ErrorHandlerTest, MapMysqlDeadlock) {
    auto error = ErrorMapper::MapBackendError(
        "mysql",
        "1213",  // Deadlock
        "",
        "Deadlock found when trying to get lock"
    );
    
    EXPECT_EQ(error.code, "SR-1105");
    EXPECT_TRUE(error.IsRetryable());
}

TEST_F(ErrorHandlerTest, MapFirebirdError) {
    auto error = ErrorMapper::MapBackendError(
        "firebird",
        "335544721",  // isc_network_error
        "",
        "Unable to complete network request"
    );
    
    EXPECT_EQ(error.category, ErrorCategory::Connection);
}

TEST_F(ErrorHandlerTest, MapUnknownBackendError) {
    auto error = ErrorMapper::MapBackendError(
        "unknown_backend",
        "99999",
        "99999",
        "Something unexpected happened"
    );
    
    // Should map to generic error (SR-0000 for unknown backends)
    EXPECT_EQ(error.code, "SR-0000");
}

TEST_F(ErrorHandlerTest, IsRetryableError) {
    auto deadlock = ErrorMapper::MapBackendError(
        "postgresql", "40P01", "40P01", "deadlock"
    );
    EXPECT_TRUE(deadlock.IsRetryable());
    
    auto syntax = ErrorMapper::MapBackendError(
        "postgresql", "42601", "42601", "syntax error"
    );
    EXPECT_FALSE(syntax.IsRetryable());
}

TEST_F(ErrorHandlerTest, GetUserMessage) {
    auto message = ErrorMapper::GetUserMessage("SR-1001");
    EXPECT_FALSE(message.empty());
    EXPECT_NE(message, "An error occurred");
}

TEST_F(ErrorHandlerTest, GetSuggestedAction) {
    ErrorInfo error;
    error.category = ErrorCategory::Connection;
    error.hint = "Check your network settings";
    
    std::string action = ErrorMapper::GetSuggestedAction(error);
    EXPECT_EQ(action, "Check your network settings");
}

TEST_F(ErrorHandlerTest, ErrorInfoFullMessage) {
    ErrorInfo error;
    error.message = "Connection failed";
    error.detail = "Network timeout";
    error.hint = "Check server status";
    
    std::string full = error.GetFullMessage();
    EXPECT_NE(full.find("Connection failed"), std::string::npos);
    EXPECT_NE(full.find("Network timeout"), std::string::npos);
    EXPECT_NE(full.find("Check server status"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ErrorInfoToLogString) {
    ErrorInfo error;
    error.code = "SR-1001";
    error.category = ErrorCategory::Connection;
    error.severity = ErrorSeverity::Error;
    error.message = "Test error";
    error.backend = "postgresql";
    error.sqlState = "08006";
    
    std::string log = error.ToLogString();
    EXPECT_NE(log.find("SR-1001"), std::string::npos);
    EXPECT_NE(log.find("Test error"), std::string::npos);
    EXPECT_NE(log.find("postgresql"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ErrorLoggerSingleton) {
    auto& logger1 = ErrorLogger::Instance();
    auto& logger2 = ErrorLogger::Instance();
    
    EXPECT_EQ(&logger1, &logger2);
}

TEST_F(ErrorHandlerTest, ErrorLoggerLogLevel) {
    auto& logger = ErrorLogger::Instance();
    
    // Should not throw
    logger.SetLogLevel(ErrorSeverity::Warning);
    EXPECT_NO_THROW(logger.Log(ErrorSeverity::Notice, "This should not be logged"));
    
    logger.SetLogLevel(ErrorSeverity::Notice);
    EXPECT_NO_THROW(logger.Log(ErrorSeverity::Error, "This should be logged"));
}

TEST_F(ErrorHandlerTest, ErrorSeverityEnum) {
    // Test that enum values exist and can be compared
    EXPECT_LT(static_cast<int>(ErrorSeverity::Fatal), 
              static_cast<int>(ErrorSeverity::Error));
    EXPECT_LT(static_cast<int>(ErrorSeverity::Error), 
              static_cast<int>(ErrorSeverity::Warning));
    EXPECT_LT(static_cast<int>(ErrorSeverity::Warning), 
              static_cast<int>(ErrorSeverity::Notice));
}

TEST_F(ErrorHandlerTest, ErrorCategoryEnum) {
    // Test that all categories exist
    ErrorCategory cat;
    cat = ErrorCategory::Connection;
    cat = ErrorCategory::Query;
    cat = ErrorCategory::Transaction;
    cat = ErrorCategory::Metadata;
    cat = ErrorCategory::System;
    cat = ErrorCategory::Configuration;
    cat = ErrorCategory::Unknown;
    (void)cat; // Suppress unused warning
}
