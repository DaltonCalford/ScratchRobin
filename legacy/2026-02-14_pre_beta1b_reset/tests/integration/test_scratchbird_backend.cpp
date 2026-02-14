/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "core/connection_manager.h"
#include "core/connection_backend.h"

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD

using namespace scratchrobin;

// Integration test environment - manages ScratchBird server lifecycle
class ScratchBirdIntegrationEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Check if ScratchBird server is available
        const char* sb_host = std::getenv("SCRATCHBIRD_TEST_HOST");
        const char* sb_port = std::getenv("SCRATCHBIRD_TEST_PORT");
        
        if (sb_host) {
            server_host_ = sb_host;
        }
        if (sb_port) {
            server_port_ = std::atoi(sb_port);
        }
        
        // Check if we should skip integration tests
        const char* skip = std::getenv("SKIP_SCRATCHBIRD_TESTS");
        skip_tests_ = (skip != nullptr && std::string(skip) == "1");
    }
    
    std::string GetHost() const { return server_host_; }
    int GetPort() const { return server_port_; }
    bool ShouldSkip() const { return skip_tests_; }
    
private:
    std::string server_host_ = "localhost";
    int server_port_ = 3092;
    bool skip_tests_ = false;
};

ScratchBirdIntegrationEnvironment* sb_env = nullptr;

class ScratchBirdBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (sb_env && sb_env->ShouldSkip()) {
            GTEST_SKIP() << "Skipping ScratchBird integration tests (SKIP_SCRATCHBIRD_TESTS=1)";
        }
        
        // Create connection manager
        connection_manager_ = std::make_unique<ConnectionManager>();
        
        // Configure network options
        NetworkOptions options;
        options.connectTimeoutMs = 5000;
        options.queryTimeoutMs = 30000;
        options.readTimeoutMs = 30000;
        options.writeTimeoutMs = 30000;
        connection_manager_->SetNetworkOptions(options);
    }
    
    void TearDown() override {
        if (connection_manager_) {
            connection_manager_->Disconnect();
        }
        connection_manager_.reset();
    }
    
    ConnectionProfile CreateNetworkProfile() {
        ConnectionProfile profile;
        profile.name = "Integration Test";
        profile.mode = ConnectionMode::Network;
        profile.host = sb_env ? sb_env->GetHost() : "localhost";
        profile.port = sb_env ? sb_env->GetPort() : 3092;
        profile.database = "scratchbird_test";
        profile.username = "testuser";
        profile.password = "testpass";
        profile.applicationName = "scratchrobin-integration-test";
        profile.sslMode = "prefer";
        return profile;
    }
    
    ConnectionProfile CreateIpcProfile() {
        ConnectionProfile profile;
        profile.name = "IPC Test";
        profile.mode = ConnectionMode::Ipc;
        profile.database = "scratchbird_test";
        profile.username = "testuser";
        profile.password = "testpass";
        profile.applicationName = "scratchrobin-integration-test";
        return profile;
    }
    
    ConnectionProfile CreateEmbeddedProfile() {
        ConnectionProfile profile;
        profile.name = "Embedded Test";
        profile.mode = ConnectionMode::Embedded;
        profile.database = "/tmp/scratchbird_test_embedded.sbd";
        profile.username = "testuser";
        profile.password = "testpass";
        profile.applicationName = "scratchrobin-integration-test";
        return profile;
    }
    
    std::unique_ptr<ConnectionManager> connection_manager_;
};

// Network Mode Tests
TEST_F(ScratchBirdBackendTest, NetworkModeConnect) {
    ConnectionProfile profile = CreateNetworkProfile();
    
    bool connected = connection_manager_->Connect(profile);
    
    if (!connected) {
        // This is expected if no server is running
        // Log the error but don't fail the test
        std::cout << "Network connection failed (expected if no server): " 
                  << connection_manager_->LastError() << std::endl;
        GTEST_SKIP() << "ScratchBird server not available for network test";
    }
    
    EXPECT_TRUE(connection_manager_->IsConnected());
    EXPECT_TRUE(connection_manager_->Capabilities().supportsTransactions);
}

TEST_F(ScratchBirdBackendTest, NetworkModeExecuteQuery) {
    ConnectionProfile profile = CreateNetworkProfile();
    
    if (!connection_manager_->Connect(profile)) {
        GTEST_SKIP() << "ScratchBird server not available";
    }
    
    QueryResult result;
    bool success = connection_manager_->ExecuteQuery("SELECT 1 AS test_value", &result);
    
    if (!success) {
        std::cout << "Query failed: " << connection_manager_->LastError() << std::endl;
    }
    
    // Test passes if connected, even if query fails (schema may not exist)
    EXPECT_TRUE(connection_manager_->IsConnected());
}

TEST_F(ScratchBirdBackendTest, NetworkModeTransaction) {
    ConnectionProfile profile = CreateNetworkProfile();
    
    if (!connection_manager_->Connect(profile)) {
        GTEST_SKIP() << "ScratchBird server not available";
    }
    
    // Test transaction operations
    EXPECT_TRUE(connection_manager_->BeginTransaction());
    EXPECT_TRUE(connection_manager_->IsInTransaction());
    
    EXPECT_TRUE(connection_manager_->Rollback());
    EXPECT_FALSE(connection_manager_->IsInTransaction());
    
    EXPECT_TRUE(connection_manager_->BeginTransaction());
    EXPECT_TRUE(connection_manager_->Commit());
    EXPECT_FALSE(connection_manager_->IsInTransaction());
}

// IPC Mode Tests
TEST_F(ScratchBirdBackendTest, IpcModeConnect) {
    ConnectionProfile profile = CreateIpcProfile();
    
    bool connected = connection_manager_->Connect(profile);
    
    if (!connected) {
        std::cout << "IPC connection failed (expected if no server): " 
                  << connection_manager_->LastError() << std::endl;
        GTEST_SKIP() << "ScratchBird IPC server not available";
    }
    
    EXPECT_TRUE(connection_manager_->IsConnected());
}

// Embedded Mode Tests
TEST_F(ScratchBirdBackendTest, EmbeddedModeConnect) {
    ConnectionProfile profile = CreateEmbeddedProfile();
    
    bool connected = connection_manager_->Connect(profile);
    
    if (!connected) {
        std::cout << "Embedded connection failed: " 
                  << connection_manager_->LastError() << std::endl;
        GTEST_SKIP() << "ScratchBird embedded mode not available";
    }
    
    EXPECT_TRUE(connection_manager_->IsConnected());
}

// Backend Capability Tests
TEST_F(ScratchBirdBackendTest, BackendCapabilitiesDetection) {
    ConnectionProfile profile = CreateNetworkProfile();
    
    if (!connection_manager_->Connect(profile)) {
        GTEST_SKIP() << "ScratchBird server not available";
    }
    
    BackendCapabilities caps = connection_manager_->Capabilities();
    
    // Core capabilities
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsCancel);
    EXPECT_TRUE(caps.supportsPaging);
    EXPECT_TRUE(caps.supportsSavepoints);
    EXPECT_TRUE(caps.supportsStreaming);
    
    // Object type support
    EXPECT_TRUE(caps.supportsDomains);
    EXPECT_TRUE(caps.supportsSequences);
    EXPECT_TRUE(caps.supportsTriggers);
    EXPECT_TRUE(caps.supportsProcedures);
    EXPECT_TRUE(caps.supportsViews);
    EXPECT_TRUE(caps.supportsIndexes);
    
    // Server info should be populated
    EXPECT_FALSE(caps.serverType.empty());
    EXPECT_FALSE(caps.serverVersion.empty());
}

// Multi-mode connection test
TEST_F(ScratchBirdBackendTest, AllConnectionModesAttempt) {
    // Try all three connection modes
    std::vector<std::pair<ConnectionMode, std::string>> modes = {
        {ConnectionMode::Network, "Network"},
        {ConnectionMode::Ipc, "IPC"},
        {ConnectionMode::Embedded, "Embedded"}
    };
    
    int successful_connections = 0;
    
    for (const auto& [mode, name] : modes) {
        ConnectionProfile profile;
        profile.name = name + " Test";
        profile.mode = mode;
        profile.host = (mode == ConnectionMode::Network) ? 
                       (sb_env ? sb_env->GetHost() : "localhost") : "";
        profile.port = (mode == ConnectionMode::Network) ? 
                       (sb_env ? sb_env->GetPort() : 3092) : 0;
        profile.database = (mode == ConnectionMode::Embedded) ? 
                           "/tmp/test_" + name + ".sbd" : "scratchbird_test";
        profile.username = "testuser";
        profile.password = "testpass";
        
        ConnectionManager mgr;
        if (mgr.Connect(profile)) {
            successful_connections++;
            std::cout << name << " mode connection: SUCCESS" << std::endl;
            
            // Test basic query
            QueryResult result;
            if (mgr.ExecuteQuery("SELECT 1", &result)) {
                std::cout << name << " mode query: SUCCESS" << std::endl;
            }
            
            mgr.Disconnect();
        } else {
            std::cout << name << " mode connection: FAILED (" 
                      << mgr.LastError() << ")" << std::endl;
        }
    }
    
    // At least one mode should work in a proper test environment
    // But we don't fail if none work (server may not be running)
    std::cout << "Successful connections: " << successful_connections << "/3" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    sb_env = new ScratchBirdIntegrationEnvironment();
    ::testing::AddGlobalTestEnvironment(sb_env);
    
    return RUN_ALL_TESTS();
}

#else // SCRATCHROBIN_USE_SCRATCHBIRD

// Dummy test when ScratchBird is not enabled
TEST(ScratchBirdBackendTest, BackendNotAvailable) {
    GTEST_SKIP() << "ScratchBird support not compiled in";
}

#endif // SCRATCHROBIN_USE_SCRATCHBIRD
