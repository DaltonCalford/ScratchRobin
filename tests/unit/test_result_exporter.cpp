/**
 * @file test_result_exporter.cpp
 * @brief Unit tests for the result exporter
 */

#include <gtest/gtest.h>
#include "core/result_exporter.h"
#include <sstream>

using namespace scratchrobin;

class ResultExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        exporter_ = std::make_unique<ResultExporter>();
        
        // Set up test result set
        result_set_.columns = {
            {"id", DataType::Integer, 4, 0, 0, false, true},
            {"name", DataType::Text, 100, 0, 0, false, false},
            {"active", DataType::Boolean, 1, 0, 0, true, false}
        };
        
        // Add some rows
        result_set_.rows.push_back({
            CellValue::FromInt(1),
            CellValue::FromString("Alice"),
            CellValue::FromBool(true)
        });
        result_set_.rows.push_back({
            CellValue::FromInt(2),
            CellValue::FromString("Bob"),
            CellValue::FromBool(false)
        });
        result_set_.rows.push_back({
            CellValue::FromInt(3),
            CellValue::FromString("Charlie"),
            CellValue::Null()
        });
    }
    
    std::unique_ptr<ResultExporter> exporter_;
    QueryResult result_set_;
};

TEST_F(ResultExporterTest, ExportToCsvBasic) {
    std::ostringstream output;
    
    bool success = exporter_->ExportToCsv(result_set_, output);
    
    EXPECT_TRUE(success);
    std::string result = output.str();
    EXPECT_NE(result.find("id,name,active"), std::string::npos);
    EXPECT_NE(result.find("1,Alice,true"), std::string::npos);
    EXPECT_NE(result.find("2,Bob,false"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToCsvWithNull) {
    std::ostringstream output;
    
    exporter_->ExportToCsv(result_set_, output);
    
    std::string result = output.str();
    EXPECT_NE(result.find("NULL"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToCsvCustomDelimiter) {
    std::ostringstream output;
    
    CsvOptions options;
    options.delimiter = ';';
    exporter_->ExportToCsv(result_set_, output, options);
    
    std::string result = output.str();
    EXPECT_NE(result.find("id;name;active"), std::string::npos);
    EXPECT_NE(result.find("1;Alice;true"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToCsvNoHeader) {
    std::ostringstream output;
    
    CsvOptions options;
    options.include_header = false;
    exporter_->ExportToCsv(result_set_, output, options);
    
    std::string result = output.str();
    EXPECT_EQ(result.find("id,name"), std::string::npos);
    EXPECT_NE(result.find("1,Alice"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToCsvWithCommaInData) {
    result_set_.rows[0][1] = CellValue::FromString("Smith, John");
    
    std::ostringstream output;
    exporter_->ExportToCsv(result_set_, output);
    
    std::string result = output.str();
    EXPECT_NE(result.find("\"Smith, John\""), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToJsonBasic) {
    std::ostringstream output;
    
    bool success = exporter_->ExportToJson(result_set_, output);
    
    EXPECT_TRUE(success);
    std::string result = output.str();
    EXPECT_NE(result.find("["), std::string::npos);
    EXPECT_NE(result.find("]"), std::string::npos);
    EXPECT_NE(result.find("Alice"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToJsonPretty) {
    std::ostringstream output;
    
    JsonOptions options;
    options.pretty = true;
    exporter_->ExportToJson(result_set_, output, options);
    
    std::string result = output.str();
    EXPECT_NE(result.find("\n"), std::string::npos);
    EXPECT_NE(result.find("  "), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToJsonMinified) {
    std::ostringstream output;
    
    JsonOptions options;
    options.pretty = false;
    exporter_->ExportToJson(result_set_, output, options);
    
    std::string result = output.str();
    // Should not have newlines or extra spaces in minified mode
    EXPECT_EQ(result.find("\n  "), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToJsonWithNull) {
    std::ostringstream output;
    
    exporter_->ExportToJson(result_set_, output);
    
    std::string result = output.str();
    EXPECT_NE(result.find("null"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToJsonWithMetadata) {
    std::ostringstream output;
    
    JsonOptions options;
    options.include_metadata = true;
    exporter_->ExportToJson(result_set_, output, options);
    
    std::string result = output.str();
    EXPECT_NE(result.find("columns"), std::string::npos);
    EXPECT_NE(result.find("rowCount"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportEmptyResult) {
    QueryResult empty_result;
    empty_result.columns = result_set_.columns;
    // No rows
    
    std::ostringstream csv_output;
    bool csv_success = exporter_->ExportToCsv(empty_result, csv_output);
    EXPECT_TRUE(csv_success);
    EXPECT_NE(csv_output.str().find("id,name,active"), std::string::npos);
    
    std::ostringstream json_output;
    bool json_success = exporter_->ExportToJson(empty_result, json_output);
    EXPECT_TRUE(json_success);
    EXPECT_NE(json_output.str().find("[]"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportLargeResult) {
    // Add many rows
    for (int i = 0; i < 10000; ++i) {
        result_set_.rows.push_back({
            CellValue::FromInt(i + 100),
            CellValue::FromString("User" + std::to_string(i)),
            CellValue::FromBool(i % 2 == 0)
        });
    }
    
    std::ostringstream output;
    bool success = exporter_->ExportToCsv(result_set_, output);
    
    EXPECT_TRUE(success);
    std::string result = output.str();
    EXPECT_GT(result.size(), 100000);
}

TEST_F(ResultExporterTest, ExportSpecialCharacters) {
    result_set_.rows[0][1] = CellValue::FromString("Line1\nLine2\tTab\"Quote");
    
    std::ostringstream csv_output;
    exporter_->ExportToCsv(result_set_, csv_output);
    std::string csv = csv_output.str();
    // Special chars should be properly escaped
    EXPECT_NE(csv.find("\""), std::string::npos);
    
    std::ostringstream json_output;
    exporter_->ExportToJson(result_set_, json_output);
    std::string json = json_output.str();
    // Should have escaped newlines, tabs, quotes
    EXPECT_NE(json.find("\\n"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportUtf8) {
    result_set_.rows[0][1] = CellValue::FromString("日本語テキスト");
    
    std::ostringstream output;
    bool success = exporter_->ExportToCsv(result_set_, output);
    
    EXPECT_TRUE(success);
    std::string result = output.str();
    EXPECT_NE(result.find("日本語"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToHtml) {
    std::ostringstream output;
    
    bool success = exporter_->ExportToHtml(result_set_, output);
    
    EXPECT_TRUE(success);
    std::string result = output.str();
    EXPECT_NE(result.find("<table>"), std::string::npos);
    EXPECT_NE(result.find("<th>"), std::string::npos);
    EXPECT_NE(result.find("<td>"), std::string::npos);
    EXPECT_NE(result.find("</table>"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToMarkdown) {
    std::ostringstream output;
    
    bool success = exporter_->ExportToMarkdown(result_set_, output);
    
    EXPECT_TRUE(success);
    std::string result = output.str();
    EXPECT_NE(result.find("| id |"), std::string::npos);
    EXPECT_NE(result.find("|---|"), std::string::npos);
    EXPECT_NE(result.find("| 1 |"), std::string::npos);
}

TEST_F(ResultExporterTest, ExportToXml) {
    std::ostringstream output;
    
    bool success = exporter_->ExportToXml(result_set_, output);
    
    EXPECT_TRUE(success);
    std::string result = output.str();
    EXPECT_NE(result.find("<?xml"), std::string::npos);
    EXPECT_NE(result.find("<row>"), std::string::npos);
    EXPECT_NE(result.find("<id>"), std::string::npos);
}

TEST_F(ResultExporterTest, CsvEscapeQuotes) {
    result_set_.rows[0][1] = CellValue::FromString("He said \"Hello\"");
    
    std::ostringstream output;
    exporter_->ExportToCsv(result_set_, output);
    
    std::string result = output.str();
    EXPECT_NE(result.find("\"\""), std::string::npos);
}

TEST_F(ResultExporterTest, DetectFormatFromExtension) {
    EXPECT_EQ(exporter_->DetectFormat("data.csv"), ExportFormat::CSV);
    EXPECT_EQ(exporter_->DetectFormat("data.json"), ExportFormat::JSON);
    EXPECT_EQ(exporter_->DetectFormat("data.html"), ExportFormat::HTML);
    EXPECT_EQ(exporter_->DetectFormat("data.md"), ExportFormat::MARKDOWN);
    EXPECT_EQ(exporter_->DetectFormat("data.xml"), ExportFormat::XML);
    EXPECT_EQ(exporter_->DetectFormat("data.unknown"), ExportFormat::CSV);  // Default
}
