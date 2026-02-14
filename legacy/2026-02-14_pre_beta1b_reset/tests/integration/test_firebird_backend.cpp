/**
 * @file test_firebird_backend.cpp
 * @brief Integration tests for Firebird backend
 *
 * These tests require a running Firebird server.
 * Set SCRATCHROBIN_TEST_FB_DSN environment variable to enable.
 */

#include <gtest/gtest.h>

#include "core/firebird_backend.h"
#include "integration/connection_test_utils.h"

#include <cstdlib>

using namespace scratchrobin;

class FirebirdBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* dsn = std::getenv("SCRATCHROBIN_TEST_FB_DSN");
        if (!dsn) {
            GTEST_SKIP() << "Firebird tests skipped. Set SCRATCHROBIN_TEST_FB_DSN to enable.";
        }

        backend_ = CreateFirebirdBackend();
        if (!backend_) {
            GTEST_SKIP() << "Firebird backend not available (client libs not enabled).";
        }

        BackendConfig config = test_utils::ParseBackendConfigFromDsn(dsn);
        std::string error;
        if (!backend_->Connect(config, &error)) {
            GTEST_SKIP() << "Could not connect to Firebird server: " << error;
        }
    }

    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
    }

    std::unique_ptr<ConnectionBackend> backend_;
};

TEST_F(FirebirdBackendTest, IsConnected) {
    EXPECT_TRUE(backend_->IsConnected());
}

TEST_F(FirebirdBackendTest, ExecuteSimpleQuery) {
    QueryResult result;
    std::string error;
    ASSERT_TRUE(backend_->ExecuteQuery("SELECT 1 FROM RDB$DATABASE", &result, &error))
        << error;
    EXPECT_EQ(result.rows.size(), 1u);
}

TEST_F(FirebirdBackendTest, TransactionCommitRollback) {
    auto caps = backend_->Capabilities();
    if (!caps.supportsTransactions) {
        GTEST_SKIP() << "Transactions not supported by backend.";
    }

    QueryResult result;
    std::string error;
    ASSERT_TRUE(backend_->ExecuteQuery(
                    "CREATE GLOBAL TEMPORARY TABLE trans_test (id INTEGER)"
                    " ON COMMIT PRESERVE ROWS",
                    &result, &error))
        << error;

    ASSERT_TRUE(backend_->BeginTransaction(&error)) << error;
    ASSERT_TRUE(backend_->ExecuteQuery(
                    "INSERT INTO trans_test VALUES (1)", &result, &error))
        << error;
    ASSERT_TRUE(backend_->Commit(&error)) << error;

    ASSERT_TRUE(backend_->BeginTransaction(&error)) << error;
    ASSERT_TRUE(backend_->ExecuteQuery(
                    "INSERT INTO trans_test VALUES (2)", &result, &error))
        << error;
    ASSERT_TRUE(backend_->Rollback(&error)) << error;

    ASSERT_TRUE(backend_->ExecuteQuery("SELECT COUNT(*) FROM trans_test", &result, &error))
        << error;
    EXPECT_EQ(result.rows.size(), 1u);
}
