/**
 * @file test_mysql_backend.cpp
 * @brief Integration tests for MySQL backend
 *
 * These tests require a running MySQL/MariaDB server.
 * Set SCRATCHROBIN_TEST_MYSQL_DSN environment variable to enable.
 */

#include <gtest/gtest.h>

#include "core/mysql_backend.h"
#include "integration/connection_test_utils.h"

#include <cstdlib>

using namespace scratchrobin;

class MySQLBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* dsn = std::getenv("SCRATCHROBIN_TEST_MYSQL_DSN");
        if (!dsn) {
            GTEST_SKIP() << "MySQL tests skipped. Set SCRATCHROBIN_TEST_MYSQL_DSN to enable.";
        }

        backend_ = CreateMySqlBackend();
        if (!backend_) {
            GTEST_SKIP() << "MySQL backend not available (libmysql not enabled).";
        }

        BackendConfig config = test_utils::ParseBackendConfigFromDsn(dsn);
        std::string error;
        if (!backend_->Connect(config, &error)) {
            GTEST_SKIP() << "Could not connect to MySQL server: " << error;
        }
    }

    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
    }

    std::unique_ptr<ConnectionBackend> backend_;
};

TEST_F(MySQLBackendTest, IsConnected) {
    EXPECT_TRUE(backend_->IsConnected());
}

TEST_F(MySQLBackendTest, ExecuteSimpleQuery) {
    QueryResult result;
    std::string error;
    ASSERT_TRUE(backend_->ExecuteQuery("SELECT 1 as num, 'hello' as str", &result, &error))
        << error;
    EXPECT_EQ(result.rows.size(), 1u);
}

TEST_F(MySQLBackendTest, TransactionCommitRollback) {
    auto caps = backend_->Capabilities();
    if (!caps.supportsTransactions) {
        GTEST_SKIP() << "Transactions not supported by backend.";
    }

    QueryResult result;
    std::string error;
    ASSERT_TRUE(backend_->ExecuteQuery(
                    "CREATE TEMPORARY TABLE trans_test (id INT PRIMARY KEY, name VARCHAR(50))",
                    &result, &error))
        << error;

    ASSERT_TRUE(backend_->BeginTransaction(&error)) << error;
    ASSERT_TRUE(backend_->ExecuteQuery(
                    "INSERT INTO trans_test VALUES (1, 'alpha')", &result, &error))
        << error;
    ASSERT_TRUE(backend_->Commit(&error)) << error;

    ASSERT_TRUE(backend_->BeginTransaction(&error)) << error;
    ASSERT_TRUE(backend_->ExecuteQuery(
                    "INSERT INTO trans_test VALUES (2, 'beta')", &result, &error))
        << error;
    ASSERT_TRUE(backend_->Rollback(&error)) << error;

    ASSERT_TRUE(backend_->ExecuteQuery("SELECT COUNT(*) FROM trans_test", &result, &error))
        << error;
    EXPECT_EQ(result.rows.size(), 1u);
}
