/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include <gtest/gtest.h>

#include "core/ipc_backend.h"
#include "core/connection_backend.h"

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD

using namespace scratchrobin;

class IpcBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend_ = CreateIpcBackend();
    }

    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
        backend_.reset();
    }

    std::unique_ptr<ConnectionBackend> backend_;
};

TEST_F(IpcBackendTest, BackendCreated) {
    ASSERT_NE(backend_, nullptr);
}

TEST_F(IpcBackendTest, BackendName) {
    ASSERT_NE(backend_, nullptr);
    EXPECT_EQ(backend_->BackendName(), "ScratchBird-IPC");
}

TEST_F(IpcBackendTest, NotConnectedInitially) {
    ASSERT_NE(backend_, nullptr);
    EXPECT_FALSE(backend_->IsConnected());
}

TEST_F(IpcBackendTest, CapabilitiesAvailable) {
    ASSERT_NE(backend_, nullptr);
    
    BackendCapabilities caps = backend_->Capabilities();
    
    // IPC backend should support all core features
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
}

TEST_F(IpcBackendTest, ConnectWithDefaultSocketPath) {
    ASSERT_NE(backend_, nullptr);
    
    BackendConfig config;
    // Use default socket path resolution
    config.database = "test_ipc";
    config.username = "testuser";
    config.password = "testpass";
    config.connectTimeoutMs = 2000;  // Short timeout for tests
    
    std::string error;
    bool result = backend_->Connect(config, &error);
    
    // Without actual ScratchBird server, this will fail
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
    
    // Error should mention IPC or socket
    bool hasIpcHint = error.find("IPC") != std::string::npos ||
                      error.find("socket") != std::string::npos ||
                      error.find("running") != std::string::npos ||
                      error.find("No such file") != std::string::npos ||
                      error.find("refused") != std::string::npos;
    EXPECT_TRUE(hasIpcHint) << "Error message: " << error;
}

TEST_F(IpcBackendTest, ConnectWithCustomSocketPath) {
    ASSERT_NE(backend_, nullptr);
    
    BackendConfig config;
    config.host = "/tmp/test_scratchbird.sock";
    config.database = "test_ipc";
    config.username = "testuser";
    config.connectTimeoutMs = 2000;
    
    std::string error;
    bool result = backend_->Connect(config, &error);
    
    // Should fail (no server at this path)
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
}

TEST_F(IpcBackendTest, TransactionOperationsNotConnected) {
    ASSERT_NE(backend_, nullptr);
    
    std::string error;
    
    EXPECT_FALSE(backend_->BeginTransaction(&error));
    EXPECT_FALSE(error.empty());
    
    error.clear();
    EXPECT_FALSE(backend_->Commit(&error));
    EXPECT_FALSE(error.empty());
    
    error.clear();
    EXPECT_FALSE(backend_->Rollback(&error));
    EXPECT_FALSE(error.empty());
}

TEST_F(IpcBackendTest, ExecuteQueryNotConnected) {
    ASSERT_NE(backend_, nullptr);
    
    QueryResult result;
    std::string error;
    
    bool success = backend_->ExecuteQuery("SELECT 1", &result, &error);
    
    EXPECT_FALSE(success);
    EXPECT_FALSE(error.empty());
}

TEST_F(IpcBackendTest, DisconnectWhenNotConnected) {
    ASSERT_NE(backend_, nullptr);
    
    // Should be safe to call disconnect when not connected
    EXPECT_NO_THROW(backend_->Disconnect());
    EXPECT_FALSE(backend_->IsConnected());
}

TEST_F(IpcBackendTest, DoubleDisconnectSafe) {
    ASSERT_NE(backend_, nullptr);
    
    // Should be safe to call disconnect multiple times
    EXPECT_NO_THROW(backend_->Disconnect());
    EXPECT_NO_THROW(backend_->Disconnect());
    EXPECT_NO_THROW(backend_->Disconnect());
    
    EXPECT_FALSE(backend_->IsConnected());
}

// Platform-specific socket path tests
#ifdef __linux__
TEST_F(IpcBackendTest, LinuxSocketPathResolution) {
    ASSERT_NE(backend_, nullptr);
    
    BackendConfig config;
    config.database = "mydb";
    config.connectTimeoutMs = 1000;
    
    // On Linux, should try to connect to Unix socket
    std::string error;
    backend_->Connect(config, &error);
    
    // The error should contain useful information
    EXPECT_FALSE(error.empty());
}
#endif

#ifdef _WIN32
TEST_F(IpcBackendTest, WindowsNamedPipeResolution) {
    ASSERT_NE(backend_, nullptr);
    
    BackendConfig config;
    config.database = "mydb";
    config.connectTimeoutMs = 1000;
    
    // On Windows, should try to connect to named pipe
    std::string error;
    backend_->Connect(config, &error);
    
    EXPECT_FALSE(error.empty());
}
#endif

#endif // SCRATCHROBIN_USE_SCRATCHBIRD
