/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include <gtest/gtest.h>

#include "core/embedded_backend.h"
#include "core/connection_backend.h"

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD

using namespace scratchrobin;

class EmbeddedBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend_ = CreateEmbeddedBackend();
    }

    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
        backend_.reset();
    }

    std::unique_ptr<ConnectionBackend> backend_;
};

TEST_F(EmbeddedBackendTest, BackendCreated) {
    ASSERT_NE(backend_, nullptr);
}

TEST_F(EmbeddedBackendTest, BackendName) {
    ASSERT_NE(backend_, nullptr);
    EXPECT_EQ(backend_->BackendName(), "ScratchBird-Embedded");
}

TEST_F(EmbeddedBackendTest, NotConnectedInitially) {
    ASSERT_NE(backend_, nullptr);
    EXPECT_FALSE(backend_->IsConnected());
}

TEST_F(EmbeddedBackendTest, ConnectWithEmptyConfigFails) {
    ASSERT_NE(backend_, nullptr);
    
    BackendConfig config;
    // Empty database path should fail
    std::string error;
    bool result = backend_->Connect(config, &error);
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
    EXPECT_FALSE(backend_->IsConnected());
}

TEST_F(EmbeddedBackendTest, CapabilitiesAvailable) {
    ASSERT_NE(backend_, nullptr);
    
    BackendCapabilities caps = backend_->Capabilities();
    
    // Embedded backend should support all core features
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsCancel);
    EXPECT_TRUE(caps.supportsPaging);
    EXPECT_TRUE(caps.supportsSavepoints);
    EXPECT_TRUE(caps.supportsStreaming);
    
    // ScratchBird-specific features
    EXPECT_TRUE(caps.supportsSblr);
    EXPECT_TRUE(caps.supportsDomains);
    EXPECT_TRUE(caps.supportsSequences);
    EXPECT_TRUE(caps.supportsTriggers);
    EXPECT_TRUE(caps.supportsProcedures);
    EXPECT_TRUE(caps.supportsTablespaces);
    
    // Server info should be populated after connect
    // Before connect, these may be empty defaults
    EXPECT_EQ(caps.serverType, "");
}

TEST_F(EmbeddedBackendTest, TransactionOperationsNotConnected) {
    ASSERT_NE(backend_, nullptr);
    
    std::string error;
    
    // Should fail when not connected
    EXPECT_FALSE(backend_->BeginTransaction(&error));
    EXPECT_FALSE(error.empty());
    
    error.clear();
    EXPECT_FALSE(backend_->Commit(&error));
    EXPECT_FALSE(error.empty());
    
    error.clear();
    EXPECT_FALSE(backend_->Rollback(&error));
    EXPECT_FALSE(error.empty());
}

TEST_F(EmbeddedBackendTest, CancelNotConnected) {
    ASSERT_NE(backend_, nullptr);
    
    std::string error;
    // Cancel may succeed (no-op) or fail when not connected
    // depending on implementation
    backend_->Cancel(&error);
}

TEST_F(EmbeddedBackendTest, ExecuteQueryNotConnected) {
    ASSERT_NE(backend_, nullptr);
    
    QueryResult result;
    std::string error;
    
    bool success = backend_->ExecuteQuery("SELECT 1", &result, &error);
    
    EXPECT_FALSE(success);
    EXPECT_FALSE(error.empty());
}

// Mock connection test - tests the interface without actual server
TEST_F(EmbeddedBackendTest, MockConnectionTest) {
    ASSERT_NE(backend_, nullptr);
    
    BackendConfig config;
    config.host = "localhost";
    config.port = 3092;
    config.database = "test_embedded";
    config.username = "testuser";
    config.password = "testpass";
    config.applicationName = "scratchrobin-test";
    config.connectTimeoutMs = 5000;
    config.queryTimeoutMs = 30000;
    
    // Without actual ScratchBird server, this will fail
    // but we can verify the error handling
    std::string error;
    bool result = backend_->Connect(config, &error);
    
    // Should fail (no server running in unit test)
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
    
    // Error should mention embedded server
    EXPECT_TRUE(error.find("embedded") != std::string::npos ||
                error.find("refused") != std::string::npos ||
                error.find("No such file") != std::string::npos ||
                error.find("Cannot assign requested address") != std::string::npos);
}

TEST_F(EmbeddedBackendTest, DisconnectWhenNotConnected) {
    ASSERT_NE(backend_, nullptr);
    
    // Should be safe to call disconnect when not connected
    EXPECT_NO_THROW(backend_->Disconnect());
    EXPECT_FALSE(backend_->IsConnected());
}

TEST_F(EmbeddedBackendTest, DoubleDisconnectSafe) {
    ASSERT_NE(backend_, nullptr);
    
    // Should be safe to call disconnect multiple times
    EXPECT_NO_THROW(backend_->Disconnect());
    EXPECT_NO_THROW(backend_->Disconnect());
    EXPECT_NO_THROW(backend_->Disconnect());
    
    EXPECT_FALSE(backend_->IsConnected());
}

#endif // SCRATCHROBIN_USE_SCRATCHBIRD
