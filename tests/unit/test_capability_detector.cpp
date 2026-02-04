/**
 * @file test_capability_detector.cpp
 * @brief Unit tests for the capability detector
 */

#include <gtest/gtest.h>
#include "core/capability_detector.h"

using namespace scratchrobin;

class CapabilityDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector_ = std::make_unique<CapabilityDetector>();
    }
    
    std::unique_ptr<CapabilityDetector> detector_;
};

TEST_F(CapabilityDetectorTest, DetectPostgresqlCapabilities) {
    // Simulate PostgreSQL version info
    std::map<std::string, std::string> server_info = {
        {"server_version", "15.2"},
        {"server_version_num", "150002"},
    };
    
    auto caps = detector_->DetectCapabilities(
        BackendType::PostgreSQL,
        server_info
    );
    
    EXPECT_EQ(caps.backend_type, BackendType::PostgreSQL);
    EXPECT_EQ(caps.server_version, "15.2");
    EXPECT_TRUE(caps.supports_transactions);
    EXPECT_TRUE(caps.supports_savepoints);
    EXPECT_TRUE(caps.supports_prepared_statements);
    EXPECT_TRUE(caps.supports_cte);
    EXPECT_TRUE(caps.supports_window_functions);
    EXPECT_TRUE(caps.supports_json);
}

TEST_F(CapabilityDetectorTest, DetectOldPostgresqlCapabilities) {
    std::map<std::string, std::string> server_info = {
        {"server_version", "9.5"},
        {"server_version_num", "90500"},
    };
    
    auto caps = detector_->DetectCapabilities(
        BackendType::PostgreSQL,
        server_info
    );
    
    // Older versions may not support some features
    EXPECT_TRUE(caps.supports_transactions);
    EXPECT_FALSE(caps.supports_jsonb);  // JSONB added in 9.4
}

TEST_F(CapabilityDetectorTest, DetectMysqlCapabilities) {
    std::map<std::string, std::string> server_info = {
        {"version", "8.0.32"},
        {"version_comment", "MySQL Community Server"},
    };
    
    auto caps = detector_->DetectCapabilities(
        BackendType::MySQL,
        server_info
    );
    
    EXPECT_EQ(caps.backend_type, BackendType::MySQL);
    EXPECT_TRUE(caps.supports_transactions);
    EXPECT_TRUE(caps.supports_json);  // MySQL 5.7+
    EXPECT_TRUE(caps.supports_cte);   // MySQL 8.0+
}

TEST_F(CapabilityDetectorTest, DetectOldMysqlCapabilities) {
    std::map<std::string, std::string> server_info = {
        {"version", "5.6.40"},
    };
    
    auto caps = detector_->DetectCapabilities(
        BackendType::MySQL,
        server_info
    );
    
    EXPECT_FALSE(caps.supports_json);  // JSON added in 5.7
    EXPECT_FALSE(caps.supports_cte);   // CTE added in 8.0
}

TEST_F(CapabilityDetectorTest, DetectFirebirdCapabilities) {
    std::map<std::string, std::string> server_info = {
        {"version", "4.0.2"},
        {"ods_version", "13.0"},
    };
    
    auto caps = detector_->DetectCapabilities(
        BackendType::Firebird,
        server_info
    );
    
    EXPECT_EQ(caps.backend_type, BackendType::Firebird);
    EXPECT_TRUE(caps.supports_transactions);
    EXPECT_TRUE(caps.supports_savepoints);
}

TEST_F(CapabilityDetectorTest, DetectScratchBirdCapabilities) {
    std::map<std::string, std::string> server_info = {
        {"version", "0.1.0"},
        {"supports_jobs", "true"},
        {"supports_domains", "true"},
        {"supports_vectors", "true"},
    };
    
    auto caps = detector_->DetectCapabilities(
        BackendType::ScratchBird,
        server_info
    );
    
    EXPECT_TRUE(caps.supports_jobs);
    EXPECT_TRUE(caps.supports_domains);
    EXPECT_TRUE(caps.supports_vectors);
}

TEST_F(CapabilityDetectorTest, CheckFeatureSupport) {
    CapabilityInfo caps;
    caps.supports_transactions = true;
    caps.supports_json = false;
    caps.server_version = "10.0";
    
    EXPECT_TRUE(caps.Supports(Feature::Transactions));
    EXPECT_FALSE(caps.Supports(Feature::Json));
}

TEST_F(CapabilityDetectorTest, VersionComparison) {
    EXPECT_TRUE(detector_->IsVersionAtLeast("10.0", "9.5"));
    EXPECT_TRUE(detector_->IsVersionAtLeast("10.0", "10.0"));
    EXPECT_FALSE(detector_->IsVersionAtLeast("9.5", "10.0"));
    
    // Multi-part versions
    EXPECT_TRUE(detector_->IsVersionAtLeast("15.2.1", "15.2"));
    EXPECT_TRUE(detector_->IsVersionAtLeast("15.2.1", "15.2.0"));
}

TEST_F(CapabilityDetectorTest, MaxIdentifierLength) {
    auto pg_caps = detector_->DetectCapabilities(
        BackendType::PostgreSQL,
        {{"server_version", "15.0"}}
    );
    EXPECT_EQ(pg_caps.max_identifier_length, 63);
    
    auto mysql_caps = detector_->DetectCapabilities(
        BackendType::MySQL,
        {{"version", "8.0"}}
    );
    EXPECT_EQ(mysql_caps.max_identifier_length, 64);
}

TEST_F(CapabilityDetectorTest, DefaultIsolationLevel) {
    auto pg_caps = detector_->DetectCapabilities(
        BackendType::PostgreSQL, {}
    );
    EXPECT_EQ(pg_caps.default_isolation_level, IsolationLevel::ReadCommitted);
}

TEST_F(CapabilityDetectorTest, SerializeToJson) {
    CapabilityInfo caps;
    caps.backend_type = BackendType::PostgreSQL;
    caps.server_version = "15.2";
    caps.supports_transactions = true;
    caps.supports_json = true;
    
    std::string json = caps.ToJson();
    
    EXPECT_NE(json.find("\"supports_transactions\":true"), std::string::npos);
    EXPECT_NE(json.find("\"server_version\":\"15.2\""), std::string::npos);
}

TEST_F(CapabilityDetectorTest, DeserializeFromJson) {
    std::string json = R"({
        "backend_type": "PostgreSQL",
        "server_version": "14.5",
        "supports_transactions": true,
        "supports_json": false,
        "max_identifier_length": 63
    })";
    
    auto caps = CapabilityInfo::FromJson(json);
    
    EXPECT_TRUE(caps.has_value());
    EXPECT_EQ(caps->backend_type, BackendType::PostgreSQL);
    EXPECT_EQ(caps->server_version, "14.5");
    EXPECT_TRUE(caps->supports_transactions);
    EXPECT_FALSE(caps->supports_json);
    EXPECT_EQ(caps->max_identifier_length, 63);
}

TEST_F(CapabilityDetectorTest, MergeCapabilities) {
    CapabilityInfo base;
    base.supports_transactions = true;
    base.supports_json = false;
    
    CapabilityInfo overlay;
    overlay.supports_json = true;
    overlay.supports_cte = true;
    
    auto merged = detector_->MergeCapabilities(base, overlay);
    
    EXPECT_TRUE(merged.supports_transactions);
    EXPECT_TRUE(merged.supports_json);
    EXPECT_TRUE(merged.supports_cte);
}

TEST_F(CapabilityDetectorTest, DetectFromConnection) {
    // Mock connection test
    // In real tests, this would use a mock connection
    ConnectionProfile profile;
    profile.backend_type = BackendType::PostgreSQL;
    
    // This would normally query the database
    // For unit tests, we just verify the API exists
    EXPECT_NO_THROW(detector_->DetectCapabilities(profile));
}

TEST_F(CapabilityDetectorTest, UnsupportedFeatureError) {
    CapabilityInfo caps;
    caps.supports_window_functions = false;
    caps.supports_cte = false;
    
    auto error = detector_->CheckFeatureSupport(caps, Feature::WindowFunctions);
    EXPECT_TRUE(error.has_value());
    
    auto ok = detector_->CheckFeatureSupport(caps, Feature::Transactions);
    // Transactions support is assumed always present
    // so this would depend on the caps setup
}
