/**
 * @file test_postgres_backend.cpp
 * @brief Integration tests for PostgreSQL backend
 *
 * These tests require a running PostgreSQL server.
 * Set SCRATCHROBIN_TEST_PG_DSN environment variable to enable.
 */

#include <gtest/gtest.h>

#include "core/postgres_backend.h"
#include "integration/connection_test_utils.h"

#include <cstdlib>

using namespace scratchrobin;

class PostgreSQLBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* dsn = std::getenv("SCRATCHROBIN_TEST_PG_DSN");
        if (!dsn) {
            GTEST_SKIP() << "PostgreSQL tests skipped. Set SCRATCHROBIN_TEST_PG_DSN to enable.";
        }

        backend_ = CreatePostgresBackend();
        if (!backend_) {
            GTEST_SKIP() << "PostgreSQL backend not available (libpq not enabled).";
        }

        BackendConfig config = test_utils::ParseBackendConfigFromDsn(dsn);
        std::string error;
        if (!backend_->Connect(config, &error)) {
            GTEST_SKIP() << "Could not connect to PostgreSQL server: " << error;
        }
    }

    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
    }

    std::unique_ptr<ConnectionBackend> backend_;
};

TEST_F(PostgreSQLBackendTest, IsConnected) {
    EXPECT_TRUE(backend_->IsConnected());
}

TEST_F(PostgreSQLBackendTest, ExecuteSimpleQuery) {
    QueryResult result;
    std::string error;
    ASSERT_TRUE(backend_->ExecuteQuery("SELECT 1 as num, 'hello' as str", &result, &error))
        << error;
    EXPECT_EQ(result.rows.size(), 1u);
    EXPECT_EQ(result.rows[0].size(), 2u);
}

TEST_F(PostgreSQLBackendTest, TransactionCommitRollback) {
    auto caps = backend_->Capabilities();
    if (!caps.supportsTransactions) {
        GTEST_SKIP() << "Transactions not supported by backend.";
    }

    QueryResult result;
    std::string error;
    ASSERT_TRUE(backend_->ExecuteQuery(
                    "CREATE TEMP TABLE trans_test (id INT PRIMARY KEY, name TEXT)",
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

    ASSERT_TRUE(backend_->ExecuteQuery("DROP TABLE trans_test", &result, &error))
        << error;
}
