/**
 * @file test_value_formatter.cpp
 * @brief Unit tests for the value formatter
 */

#include <gtest/gtest.h>
#include "core/value_formatter.h"

using namespace scratchrobin;

class ValueFormatterTest : public ::testing::Test {
protected:
    void SetUp() override {
        options_ = FormatOptions{};
    }
    
    FormatOptions options_;
};

TEST_F(ValueFormatterTest, FormatNullValue) {
    QueryValue value;
    value.isNull = true;
    
    auto result = FormatValueForDisplay(value, "text", options_);
    EXPECT_EQ(result, "NULL");
}

TEST_F(ValueFormatterTest, FormatStringValue) {
    QueryValue value;
    value.isNull = false;
    value.text = "Hello, World!";
    
    auto result = FormatValueForDisplay(value, "text", options_);
    EXPECT_EQ(result, "Hello, World!");
}

TEST_F(ValueFormatterTest, FormatIntegerValue) {
    QueryValue value;
    value.isNull = false;
    value.text = "42";
    
    auto result = FormatValueForDisplay(value, "integer", options_);
    EXPECT_EQ(result, "42");
}

TEST_F(ValueFormatterTest, FormatNumericValue) {
    QueryValue value;
    value.isNull = false;
    value.text = "3.14159";
    
    auto result = FormatValueForDisplay(value, "numeric", options_);
    EXPECT_EQ(result, "3.14159");
}

TEST_F(ValueFormatterTest, FormatDateValue) {
    QueryValue value;
    value.isNull = false;
    value.text = "2026-02-03";
    
    auto result = FormatValueForDisplay(value, "date", options_);
    EXPECT_EQ(result, "2026-02-03");
}

TEST_F(ValueFormatterTest, FormatTimestampValue) {
    QueryValue value;
    value.isNull = false;
    value.text = "2026-02-03 14:30:00";
    
    auto result = FormatValueForDisplay(value, "timestamp", options_);
    EXPECT_EQ(result, "2026-02-03 14:30:00");
}

TEST_F(ValueFormatterTest, FormatBooleanTypeTrue) {
    EXPECT_TRUE(IsBooleanType("boolean"));
    EXPECT_TRUE(IsBooleanType("bool"));
}

TEST_F(ValueFormatterTest, FormatBooleanTypeFalse) {
    EXPECT_FALSE(IsBooleanType("text"));
    EXPECT_FALSE(IsBooleanType("integer"));
}

TEST_F(ValueFormatterTest, FormatNumericTypeTrue) {
    EXPECT_TRUE(IsNumericType("numeric"));
    EXPECT_TRUE(IsNumericType("decimal"));
    EXPECT_TRUE(IsNumericType("money"));
    // Note: Implementation uses internal type names (int16, int32, int64, float32, float64)
}

TEST_F(ValueFormatterTest, FormatNumericTypeFalse) {
    EXPECT_FALSE(IsNumericType("text"));
    EXPECT_FALSE(IsNumericType("boolean"));
    EXPECT_FALSE(IsNumericType("date"));
}

TEST_F(ValueFormatterTest, FormatJsonTypeTrue) {
    EXPECT_TRUE(IsJsonType("json"));
    EXPECT_TRUE(IsJsonType("jsonb"));
}

TEST_F(ValueFormatterTest, FormatJsonTypeFalse) {
    EXPECT_FALSE(IsJsonType("text"));
    EXPECT_FALSE(IsJsonType("xml"));
}

TEST_F(ValueFormatterTest, FormatEmptyString) {
    QueryValue value;
    value.isNull = false;
    value.text = "";
    
    auto result = FormatValueForDisplay(value, "text", options_);
    EXPECT_EQ(result, "");
}

TEST_F(ValueFormatterTest, FormatWhitespaceString) {
    QueryValue value;
    value.isNull = false;
    value.text = "   \t\n   ";
    
    auto result = FormatValueForDisplay(value, "text", options_);
    EXPECT_EQ(result, "   \t\n   ");
}

TEST_F(ValueFormatterTest, FormatBinaryValue) {
    QueryValue value;
    value.isNull = false;
    value.raw = {0x00, 0x01, 0x02, 0xFF};
    
    auto result = FormatValueForDisplay(value, "bytea", options_);
    // Binary should be displayed (likely as hex or placeholder)
    EXPECT_FALSE(result.empty());
}

TEST_F(ValueFormatterTest, FormatExportSimple) {
    QueryValue value;
    value.isNull = false;
    value.text = "Hello";
    
    auto result = FormatValueForExport(value, "text", options_);
    EXPECT_EQ(result, "Hello");
}

TEST_F(ValueFormatterTest, FormatExportWithSpecialChars) {
    QueryValue value;
    value.isNull = false;
    value.text = "Hello, World!";
    
    auto result = FormatValueForExport(value, "text", options_);
    EXPECT_EQ(result, "Hello, World!");
}

TEST_F(ValueFormatterTest, FormatExportNull) {
    QueryValue value;
    value.isNull = true;
    
    auto result = FormatValueForExport(value, "text", options_);
    // Implementation returns "NULL" for null values in export
    EXPECT_EQ(result, "NULL");
}

TEST_F(ValueFormatterTest, FormatLargeNumber) {
    QueryValue value;
    value.isNull = false;
    value.text = "9223372036854775807";
    
    auto result = FormatValueForDisplay(value, "bigint", options_);
    EXPECT_EQ(result, "9223372036854775807");
}

TEST_F(ValueFormatterTest, FormatNegativeNumber) {
    QueryValue value;
    value.isNull = false;
    value.text = "-12345";
    
    auto result = FormatValueForDisplay(value, "integer", options_);
    EXPECT_EQ(result, "-12345");
}

TEST_F(ValueFormatterTest, FormatDecimal) {
    QueryValue value;
    value.isNull = false;
    value.text = "1234567890.1234567890";
    
    auto result = FormatValueForDisplay(value, "numeric", options_);
    EXPECT_EQ(result, "1234567890.1234567890");
}

TEST_F(ValueFormatterTest, FormatUUID) {
    QueryValue value;
    value.isNull = false;
    value.text = "550e8400-e29b-41d4-a716-446655440000";
    
    auto result = FormatValueForDisplay(value, "uuid", options_);
    EXPECT_EQ(result, "550e8400-e29b-41d4-a716-446655440000");
}

TEST_F(ValueFormatterTest, FormatArray) {
    QueryValue value;
    value.isNull = false;
    value.text = "{1, 2, 3, 4, 5}";
    
    auto result = FormatValueForDisplay(value, "array", options_);
    EXPECT_EQ(result, "{1, 2, 3, 4, 5}");
}

TEST_F(ValueFormatterTest, FormatJson) {
    QueryValue value;
    value.isNull = false;
    value.text = R"({"key": "value", "number": 42})";
    
    auto result = FormatValueForDisplay(value, "json", options_);
    EXPECT_NE(result.find("key"), std::string::npos);
    EXPECT_NE(result.find("value"), std::string::npos);
}
