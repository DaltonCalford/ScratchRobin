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
        handler_ = std::make_unique<ErrorHandler>();
    }
    
    std::unique_ptr<ErrorHandler> handler_;
};

TEST_F(ErrorHandlerTest, CreateSimpleError) {
    auto error = handler_->CreateError(
        ErrorCode::ConnectionFailed,
        "Could not connect to database"
    );
    
    EXPECT_EQ(error.code, ErrorCode::ConnectionFailed);
    EXPECT_EQ(error.message, "Could not connect to database");
    EXPECT_FALSE(error.has_detail());
}

TEST_F(ErrorHandlerTest, CreateErrorWithDetail) {
    auto error = handler_->CreateError(
        ErrorCode::QueryFailed,
        "Query execution failed",
        "Syntax error at line 42"
    );
    
    EXPECT_EQ(error.message, "Query execution failed");
    EXPECT_TRUE(error.has_detail());
    EXPECT_EQ(error.detail(), "Syntax error at line 42");
}

TEST_F(ErrorHandlerTest, CreateErrorWithSqlState) {
    auto error = handler_->CreateError(
        ErrorCode::QueryFailed,
        "Query failed",
        "",
        "42601"  // PostgreSQL syntax error
    );
    
    EXPECT_EQ(error.sql_state(), "42601");
}

TEST_F(ErrorHandlerTest, ErrorSeverity) {
    auto fatal = handler_->CreateError(ErrorCode::ConnectionFailed, "Fatal");
    auto warning = handler_->CreateError(ErrorCode::TransactionConflict, "Warning");
    auto info = handler_->CreateError(ErrorCode::OperationCancelled, "Info");
    
    EXPECT_EQ(fatal.severity(), ErrorSeverity::Fatal);
    EXPECT_EQ(warning.severity(), ErrorSeverity::Warning);
    EXPECT_EQ(info.severity(), ErrorSeverity::Error);
}

TEST_F(ErrorHandlerTest, ErrorToString) {
    auto error = handler_->CreateError(
        ErrorCode::ConnectionFailed,
        "Connection failed",
        "Network timeout"
    );
    
    std::string str = error.ToString();
    EXPECT_NE(str.find("Connection failed"), std::string::npos);
    EXPECT_NE(str.find("Network timeout"), std::string::npos);
}

TEST_F(ErrorHandlerTest, MapPostgresqlError) {
    // PostgreSQL connection failure
    auto error = handler_->MapBackendError(
        BackendType::PostgreSQL,
        "08001",  // sqlclient_unable_to_establish_sqlconnection
        "could not connect to server"
    );
    
    EXPECT_EQ(error.code, ErrorCode::ConnectionFailed);
    EXPECT_EQ(error.severity(), ErrorSeverity::Fatal);
}

TEST_F(ErrorHandlerTest, MapMysqlError) {
    // MySQL access denied
    auto error = handler_->MapBackendError(
        BackendType::MySQL,
        "1045",  // ER_ACCESS_DENIED_ERROR
        "Access denied for user"
    );
    
    EXPECT_EQ(error.code, ErrorCode::AuthenticationFailed);
}

TEST_F(ErrorHandlerTest, MapFirebirdError) {
    // Firebird connection error
    auto error = handler_->MapBackendError(
        BackendType::Firebird,
        "335544721",  // isc_network_error
        "Unable to complete network request"
    );
    
    EXPECT_EQ(error.code, ErrorCode::ConnectionFailed);
}

TEST_F(ErrorHandlerTest, MapSyntaxError) {
    auto pg_error = handler_->MapBackendError(
        BackendType::PostgreSQL,
        "42601",  // syntax_error
        "syntax error at or near 'SELEC'"
    );
    
    EXPECT_EQ(pg_error.code, ErrorCode::QuerySyntaxError);
    
    auto mysql_error = handler_->MapBackendError(
        BackendType::MySQL,
        "1064",  // ER_PARSE_ERROR
        "You have an error in your SQL syntax"
    );
    
    EXPECT_EQ(mysql_error.code, ErrorCode::QuerySyntaxError);
}

TEST_F(ErrorHandlerTest, MapTimeoutError) {
    auto error = handler_->MapBackendError(
        BackendType::PostgreSQL,
        "57014",  // query_canceled
        "canceling statement due to statement timeout"
    );
    
    EXPECT_EQ(error.code, ErrorCode::QueryTimeout);
}

TEST_F(ErrorHandlerTest, MapConstraintViolation) {
    auto error = handler_->MapBackendError(
        BackendType::PostgreSQL,
        "23505",  // unique_violation
        "duplicate key value violates unique constraint"
    );
    
    EXPECT_EQ(error.code, ErrorCode::ConstraintViolation);
}

TEST_F(ErrorHandlerTest, MapDeadlockError) {
    auto error = handler_->MapBackendError(
        BackendType::PostgreSQL,
        "40P01",  // deadlock_detected
        "deadlock detected"
    );
    
    EXPECT_EQ(error.code, ErrorCode::TransactionConflict);
    EXPECT_TRUE(error.is_retryable());
}

TEST_F(ErrorHandlerTest, IsRetryableError) {
    auto deadlock = handler_->MapBackendError(
        BackendType::PostgreSQL, "40P01", "deadlock"
    );
    EXPECT_TRUE(deadlock.is_retryable());
    
    auto syntax = handler_->MapBackendError(
        BackendType::PostgreSQL, "42601", "syntax error"
    );
    EXPECT_FALSE(syntax.is_retryable());
}

TEST_F(ErrorHandlerTest, UserFriendlyMessage) {
    auto error = handler_->CreateError(
        ErrorCode::ConnectionFailed,
        "could not connect to server: Connection refused",
        "Is the server running on host \"localhost\"?"
    );
    
    std::string friendly = handler_->GetUserFriendlyMessage(error);
    EXPECT_NE(friendly.find("Could not connect"), std::string::npos);
    EXPECT_FALSE(friendly.empty());
}

TEST_F(ErrorHandlerTest, ErrorChain) {
    auto inner = handler_->CreateError(
        ErrorCode::NetworkError,
        "Socket timeout"
    );
    
    auto outer = handler_->CreateError(
        ErrorCode::ConnectionFailed,
        "Failed to establish connection",
        "",
        "",
        inner
    );
    
    EXPECT_TRUE(outer.has_cause());
    EXPECT_EQ(outer.cause()->code, ErrorCode::NetworkError);
}

TEST_F(ErrorHandlerTest, ErrorContext) {
    auto error = handler_->CreateError(
        ErrorCode::QueryFailed,
        "Query failed"
    );
    
    error.add_context("While executing: SELECT * FROM users");
    error.add_context("In function: LoadUserData()");
    
    auto context = error.context();
    EXPECT_EQ(context.size(), 2);
    EXPECT_NE(context[0].find("SELECT"), std::string::npos);
}

TEST_F(ErrorHandlerTest, UnknownBackendError) {
    auto error = handler_->MapBackendError(
        BackendType::PostgreSQL,
        "99999",  // Unknown error code
        "Something unexpected happened"
    );
    
    // Should map to generic error
    EXPECT_EQ(error.code, ErrorCode::UnknownError);
}

TEST_F(ErrorHandlerTest, ErrorCodeToString) {
    EXPECT_NE(ErrorCodeToString(ErrorCode::ConnectionFailed), "");
    EXPECT_NE(ErrorCodeToString(ErrorCode::QueryFailed), "");
    EXPECT_NE(ErrorCodeToString(ErrorCode::AuthenticationFailed), "");
}

TEST_F(ErrorHandlerTest, LogError) {
    auto error = handler_->CreateError(
        ErrorCode::QueryFailed,
        "Test error for logging"
    );
    
    // Should not throw
    EXPECT_NO_THROW(handler_->LogError(error));
}

TEST_F(ErrorHandlerTest, FormatForDisplay) {
    auto error = handler_->CreateError(
        ErrorCode::ConstraintViolation,
        "Unique constraint failed",
        "Key (email)=(test@example.com) already exists",
        "23505"
    );
    
    std::string display = handler_->FormatForDisplay(error, true);
    EXPECT_NE(display.find("Unique constraint failed"), std::string::npos);
    EXPECT_NE(display.find("test@example.com"), std::string::npos);
    EXPECT_NE(display.find("23505"), std::string::npos);
}
