/**
 * @file test_result_exporter.cpp
 * @brief Unit tests for the result exporter
 */

#include <gtest/gtest.h>
#include "core/result_exporter.h"
#include <sstream>
#include <fstream>
#include <cstdio>

using namespace scratchrobin;

class ResultExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test result set
        result_set_.columns = {
            {"id", "integer"},
            {"name", "text"},
            {"active", "boolean"}
        };
        
        // Add some rows
        result_set_.rows.push_back({
            QueryValue{false, "1", {}},
            QueryValue{false, "Alice", {}},
            QueryValue{false, "true", {}}
        });
        result_set_.rows.push_back({
            QueryValue{false, "2", {}},
            QueryValue{false, "Bob", {}},
            QueryValue{false, "false", {}}
        });
        result_set_.rows.push_back({
            QueryValue{false, "3", {}},
            QueryValue{false, "Charlie", {}},
            QueryValue{true, "", {}}  // NULL value
        });
    }
    
    void TearDown() override {
        // Clean up test files
        std::remove("/tmp/test_export.csv");
        std::remove("/tmp/test_export.json");
    }
    
    QueryResult result_set_;
};

TEST_F(ResultExporterTest, ExportToCsvBasic) {
    std::string error;
    ExportOptions options;
    
    bool success = ExportResultToCsv(result_set_, "/tmp/test_export.csv", &error, options);
    
    EXPECT_TRUE(success);
    
    // Read back and verify
    std::ifstream file("/tmp/test_export.csv");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_NE(content.find("id"), std::string::npos);
    EXPECT_NE(content.find("Alice"), std::string::npos);
    EXPECT_NE(content.find("Bob"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToCsvWithNull) {
    std::string error;
    ExportOptions options;
    
    bool success = ExportResultToCsv(result_set_, "/tmp/test_export.csv", &error, options);
    
    EXPECT_TRUE(success);
    
    std::ifstream file("/tmp/test_export.csv");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // Should contain empty string or NULL representation for null value
    EXPECT_FALSE(content.empty());
}

TEST_F(ResultExporterTest, ExportEmptyResult) {
    QueryResult empty_result;
    empty_result.columns = result_set_.columns;
    // No rows
    
    std::string error;
    ExportOptions options;
    
    bool csv_success = ExportResultToCsv(empty_result, "/tmp/test_export.csv", &error, options);
    EXPECT_TRUE(csv_success);
    
    bool json_success = ExportResultToJson(empty_result, "/tmp/test_export.json", &error, options);
    EXPECT_TRUE(json_success);
}

TEST_F(ResultExporterTest, ExportToJsonBasic) {
    std::string error;
    ExportOptions options;
    
    bool success = ExportResultToJson(result_set_, "/tmp/test_export.json", &error, options);
    
    EXPECT_TRUE(success);
    
    // Read back and verify
    std::ifstream file("/tmp/test_export.json");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_NE(content.find("["), std::string::npos);
    EXPECT_NE(content.find("]"), std::string::npos);
    EXPECT_NE(content.find("Alice"), std::string::npos);
    EXPECT_NE(content.find("Bob"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToJsonWithNull) {
    std::string error;
    ExportOptions options;
    
    bool success = ExportResultToJson(result_set_, "/tmp/test_export.json", &error, options);
    
    EXPECT_TRUE(success);
    
    std::ifstream file("/tmp/test_export.json");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // Should have null representation for null value
    EXPECT_NE(content.find("null"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportSpecialCharacters) {
    result_set_.rows[0][1] = QueryValue{false, "Line1\nLine2\tTab\"Quote", {}};
    
    std::string error;
    ExportOptions options;
    
    bool success = ExportResultToCsv(result_set_, "/tmp/test_export.csv", &error, options);
    EXPECT_TRUE(success);
    
    success = ExportResultToJson(result_set_, "/tmp/test_export.json", &error, options);
    EXPECT_TRUE(success);
    
    // JSON should have escaped characters
    std::ifstream file("/tmp/test_export.json");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_NE(content.find("\\n"), std::string::npos);
    EXPECT_NE(content.find("\\t"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportUtf8) {
    result_set_.rows[0][1] = QueryValue{false, "日本語テキスト", {}};
    
    std::string error;
    ExportOptions options;
    
    bool success = ExportResultToCsv(result_set_, "/tmp/test_export.csv", &error, options);
    EXPECT_TRUE(success);
    
    std::ifstream file("/tmp/test_export.csv");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_NE(content.find("日本語"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportLargeResult) {
    // Add many rows
    for (int i = 0; i < 1000; ++i) {
        result_set_.rows.push_back({
            QueryValue{false, std::to_string(i + 100), {}},
            QueryValue{false, "User" + std::to_string(i), {}},
            QueryValue{false, (i % 2 == 0) ? "true" : "false", {}}
        });
    }
    
    std::string error;
    ExportOptions options;
    
    bool success = ExportResultToCsv(result_set_, "/tmp/test_export.csv", &error, options);
    EXPECT_TRUE(success);
    
    std::ifstream file("/tmp/test_export.csv");
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    
    EXPECT_GT(size, 1000);
}

TEST_F(ResultExporterTest, ExportWithoutHeaders) {
    std::string error;
    ExportOptions options;
    options.includeHeaders = false;
    
    bool success = ExportResultToCsv(result_set_, "/tmp/test_export.csv", &error, options);
    EXPECT_TRUE(success);
    
    std::ifstream file("/tmp/test_export.csv");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // Should not have column headers
    EXPECT_EQ(content.find("id,name,active"), std::string::npos);
    EXPECT_NE(content.find("Alice"), std::string::npos);
}
