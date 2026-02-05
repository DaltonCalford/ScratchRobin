/**
 * @file test_capability_detector.cpp
 * @brief Unit tests for the capability detector
 */

#include <gtest/gtest.h>
#include "core/capability_detector.h"
#include "core/connection_backend.h"

using namespace scratchrobin;

class CapabilityDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // CapabilityDetector uses static methods
    }
};

TEST_F(CapabilityDetectorTest, ParseVersionSimple) {
    int major = 0, minor = 0, patch = 0;
    
    EXPECT_TRUE(CapabilityDetector::ParseVersion("13.4", &major, &minor, &patch));
    EXPECT_EQ(major, 13);
    EXPECT_EQ(minor, 4);
    EXPECT_EQ(patch, 0);
}

TEST_F(CapabilityDetectorTest, ParseVersionWithPatch) {
    int major = 0, minor = 0, patch = 0;
    
    EXPECT_TRUE(CapabilityDetector::ParseVersion("8.0.25", &major, &minor, &patch));
    EXPECT_EQ(major, 8);
    EXPECT_EQ(minor, 0);
    EXPECT_EQ(patch, 25);
}

TEST_F(CapabilityDetectorTest, ParseVersionWithSuffix) {
    int major = 0, minor = 0, patch = 0;
    
    EXPECT_TRUE(CapabilityDetector::ParseVersion("15.2 (Debian 15.2-1)", &major, &minor, &patch));
    EXPECT_EQ(major, 15);
    EXPECT_EQ(minor, 2);
}

TEST_F(CapabilityDetectorTest, ParseVersionInvalid) {
    int major = 0, minor = 0, patch = 0;
    
    EXPECT_FALSE(CapabilityDetector::ParseVersion("", &major, &minor, &patch));
    EXPECT_FALSE(CapabilityDetector::ParseVersion("not a version", &major, &minor, &patch));
}

TEST_F(CapabilityDetectorTest, ParseVersionNullPointer) {
    // Should handle null pointers gracefully
    EXPECT_FALSE(CapabilityDetector::ParseVersion("1.0", nullptr, nullptr, nullptr));
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesPostgreSQL) {
    auto caps = CapabilityDetector::GetStaticCapabilities("postgresql");
    
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsPaging);
    EXPECT_TRUE(caps.supportsExplain);
    EXPECT_TRUE(caps.supportsDomains);
    EXPECT_TRUE(caps.supportsSequences);
    EXPECT_TRUE(caps.supportsTriggers);
    EXPECT_TRUE(caps.supportsProcedures);
    EXPECT_TRUE(caps.supportsUserAdmin);
    EXPECT_TRUE(caps.supportsRoleAdmin);
    EXPECT_TRUE(caps.supportsSchemas);
    EXPECT_TRUE(caps.supportsTablespaces);
    EXPECT_TRUE(caps.supportsMultipleDatabases);
    EXPECT_TRUE(caps.supportsSavepoints);
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesMySQL) {
    auto caps = CapabilityDetector::GetStaticCapabilities("mysql");
    
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsPaging);
    EXPECT_TRUE(caps.supportsExplain);
    EXPECT_TRUE(caps.supportsTriggers);
    EXPECT_TRUE(caps.supportsProcedures);
    EXPECT_TRUE(caps.supportsUserAdmin);
    EXPECT_TRUE(caps.supportsMultipleDatabases);
    EXPECT_TRUE(caps.supportsSavepoints);
    
    // MySQL specific
    EXPECT_FALSE(caps.supportsRoleAdmin);  // Only MySQL 8.0+
    EXPECT_FALSE(caps.supportsDomains);
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesMariaDB) {
    auto caps = CapabilityDetector::GetStaticCapabilities("mariadb");
    
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsPaging);
    EXPECT_TRUE(caps.supportsSavepoints);
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesFirebird) {
    auto caps = CapabilityDetector::GetStaticCapabilities("firebird");
    
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsPaging);
    EXPECT_TRUE(caps.supportsDomains);
    EXPECT_TRUE(caps.supportsSequences);
    EXPECT_TRUE(caps.supportsTriggers);
    EXPECT_TRUE(caps.supportsProcedures);
    EXPECT_TRUE(caps.supportsUserAdmin);
    EXPECT_TRUE(caps.supportsRoleAdmin);
    EXPECT_TRUE(caps.supportsMultipleDatabases);
    EXPECT_TRUE(caps.supportsSavepoints);
    
    // Firebird limitations
    EXPECT_FALSE(caps.supportsExplain);
    // Firebird supports schemas through the default mechanism
    EXPECT_TRUE(caps.supportsSchemas);  
    EXPECT_FALSE(caps.supportsTablespaces);
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesScratchBird) {
    auto caps = CapabilityDetector::GetStaticCapabilities("scratchbird");
    
    // ScratchBird supports everything
    EXPECT_TRUE(caps.supportsCancel);
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsExplain);
    EXPECT_TRUE(caps.supportsSblr);
    EXPECT_TRUE(caps.supportsStreaming);
    EXPECT_TRUE(caps.supportsDdlExtract);
    EXPECT_TRUE(caps.supportsDependencies);
    EXPECT_TRUE(caps.supportsUserAdmin);
    EXPECT_TRUE(caps.supportsRoleAdmin);
    EXPECT_TRUE(caps.supportsGroupAdmin);
    EXPECT_TRUE(caps.supportsJobScheduler);
    EXPECT_TRUE(caps.supportsDomains);
    EXPECT_TRUE(caps.supportsSequences);
    EXPECT_TRUE(caps.supportsTriggers);
    EXPECT_TRUE(caps.supportsProcedures);
    EXPECT_TRUE(caps.supportsViews);
    EXPECT_TRUE(caps.supportsTempTables);
    EXPECT_TRUE(caps.supportsMultipleDatabases);
    EXPECT_TRUE(caps.supportsTablespaces);
    EXPECT_TRUE(caps.supportsSchemas);
    EXPECT_TRUE(caps.supportsBackup);
    EXPECT_TRUE(caps.supportsImportExport);
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesNative) {
    // "native" is alias for scratchbird
    auto caps = CapabilityDetector::GetStaticCapabilities("native");
    
    EXPECT_TRUE(caps.supportsSblr);
    EXPECT_TRUE(caps.supportsJobScheduler);
    EXPECT_TRUE(caps.supportsGroupAdmin);
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesUnknown) {
    auto caps = CapabilityDetector::GetStaticCapabilities("unknown_backend");
    
    // Should return default capabilities
    EXPECT_FALSE(caps.supportsCancel);
    EXPECT_TRUE(caps.supportsTransactions);  // Default
}

TEST_F(CapabilityDetectorTest, GetStaticCapabilitiesCaseInsensitive) {
    auto caps1 = CapabilityDetector::GetStaticCapabilities("PostgreSQL");
    auto caps2 = CapabilityDetector::GetStaticCapabilities("POSTGRESQL");
    auto caps3 = CapabilityDetector::GetStaticCapabilities("postgresql");
    
    EXPECT_EQ(caps1.supportsExplain, caps2.supportsExplain);
    EXPECT_EQ(caps2.supportsExplain, caps3.supportsExplain);
}

TEST_F(CapabilityDetectorTest, CapabilityMatrixExists) {
    const char* matrix = CapabilityMatrix::GetMarkdownTable();
    
    EXPECT_NE(matrix, nullptr);
    EXPECT_NE(std::string(matrix).find("PostgreSQL"), std::string::npos);
    EXPECT_NE(std::string(matrix).find("MySQL"), std::string::npos);
    EXPECT_NE(std::string(matrix).find("Firebird"), std::string::npos);
    EXPECT_NE(std::string(matrix).find("ScratchBird"), std::string::npos);
}

TEST_F(CapabilityDetectorTest, DetectCapabilitiesWithNull) {
    // Should return default capabilities when backend is null
    auto caps = CapabilityDetector::DetectCapabilities(nullptr);
    
    EXPECT_FALSE(caps.supportsCancel);
    EXPECT_TRUE(caps.supportsTransactions);  // Default
}

TEST_F(CapabilityDetectorTest, BackendCapabilitiesDefaultValues) {
    BackendCapabilities caps;
    
    // Check default values
    EXPECT_FALSE(caps.supportsCancel);
    EXPECT_TRUE(caps.supportsTransactions);
    EXPECT_TRUE(caps.supportsPaging);
    EXPECT_TRUE(caps.supportsSavepoints);
    EXPECT_FALSE(caps.supportsExplain);
    EXPECT_FALSE(caps.supportsSblr);
    EXPECT_TRUE(caps.supportsStreaming);
    EXPECT_FALSE(caps.supportsDdlExtract);
    EXPECT_FALSE(caps.supportsDependencies);
    EXPECT_FALSE(caps.supportsUserAdmin);
    EXPECT_FALSE(caps.supportsRoleAdmin);
    EXPECT_FALSE(caps.supportsGroupAdmin);
    EXPECT_FALSE(caps.supportsJobScheduler);
    EXPECT_FALSE(caps.supportsDomains);
    EXPECT_FALSE(caps.supportsSequences);
    EXPECT_FALSE(caps.supportsTriggers);
    EXPECT_FALSE(caps.supportsProcedures);
    EXPECT_TRUE(caps.supportsViews);
    EXPECT_TRUE(caps.supportsTempTables);
    EXPECT_TRUE(caps.supportsMultipleDatabases);
    EXPECT_FALSE(caps.supportsTablespaces);
    EXPECT_TRUE(caps.supportsSchemas);
    EXPECT_FALSE(caps.supportsBackup);
    EXPECT_TRUE(caps.supportsImportExport);
    
    // Version info defaults
    EXPECT_EQ(caps.majorVersion, 0);
    EXPECT_EQ(caps.minorVersion, 0);
    EXPECT_EQ(caps.patchVersion, 0);
}

TEST_F(CapabilityDetectorTest, VersionComparisonHelpers) {
    // Test that version parsing works correctly for comparison purposes
    int major1, minor1, patch1;
    int major2, minor2, patch2;
    
    CapabilityDetector::ParseVersion("14.5", &major1, &minor1, &patch1);
    CapabilityDetector::ParseVersion("13.8", &major2, &minor2, &patch2);
    
    EXPECT_GT(major1, major2);
    
    CapabilityDetector::ParseVersion("14.5.1", &major1, &minor1, &patch1);
    CapabilityDetector::ParseVersion("14.5", &major2, &minor2, &patch2);
    
    EXPECT_EQ(major1, major2);
    EXPECT_EQ(minor1, minor2);
    EXPECT_GT(patch1, patch2);
}
