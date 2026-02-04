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
        formatter_ = std::make_unique<ValueFormatter>();
    }
    
    std::unique_ptr<ValueFormatter> formatter_;
};

TEST_F(ValueFormatterTest, FormatNull) {
    CellValue value;
    value.type = DataType::Null;
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "NULL");
}

TEST_F(ValueFormatterTest, FormatInteger) {
    CellValue value;
    value.type = DataType::Integer;
    value.int_value = 42;
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "42");
}

TEST_F(ValueFormatterTest, FormatBigInt) {
    CellValue value;
    value.type = DataType::BigInt;
    value.bigint_value = 9223372036854775807LL;
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "9223372036854775807");
}

TEST_F(ValueFormatterTest, FormatFloat) {
    CellValue value;
    value.type = DataType::Float;
    value.float_value = 3.14159f;
    
    auto result = formatter_->Format(value);
    EXPECT_NE(result.find("3.14"), std::string::npos);
}

TEST_F(ValueFormatterTest, FormatDouble) {
    CellValue value;
    value.type = DataType::Double;
    value.double_value = 2.718281828459045;
    
    auto result = formatter_->Format(value);
    EXPECT_NE(result.find("2.718"), std::string::npos);
}

TEST_F(ValueFormatterTest, FormatString) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Hello, World!";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "Hello, World!");
}

TEST_F(ValueFormatterTest, FormatStringWithSpecialChars) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Line 1\nLine 2\tTabbed";
    
    auto result = formatter_->Format(value);
    EXPECT_NE(result.find("\n"), std::string::npos);
    EXPECT_NE(result.find("\t"), std::string::npos);
}

TEST_F(ValueFormatterTest, FormatBooleanTrue) {
    CellValue value;
    value.type = DataType::Boolean;
    value.bool_value = true;
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "true");
}

TEST_F(ValueFormatterTest, FormatBooleanFalse) {
    CellValue value;
    value.type = DataType::Boolean;
    value.bool_value = false;
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "false");
}

TEST_F(ValueFormatterTest, FormatDate) {
    CellValue value;
    value.type = DataType::Date;
    value.string_value = "2026-02-03";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "2026-02-03");
}

TEST_F(ValueFormatterTest, FormatTimestamp) {
    CellValue value;
    value.type = DataType::Timestamp;
    value.string_value = "2026-02-03 14:30:00";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "2026-02-03 14:30:00");
}

TEST_F(ValueFormatterTest, FormatTimestampWithTimezone) {
    CellValue value;
    value.type = DataType::TimestampTz;
    value.string_value = "2026-02-03 14:30:00-05:00";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "2026-02-03 14:30:00-05:00");
}

TEST_F(ValueFormatterTest, FormatDecimal) {
    CellValue value;
    value.type = DataType::Decimal;
    value.string_value = "1234567890.1234567890";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "1234567890.1234567890");
}

TEST_F(ValueFormatterTest, FormatUUID) {
    CellValue value;
    value.type = DataType::Uuid;
    value.string_value = "550e8400-e29b-41d4-a716-446655440000";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "550e8400-e29b-41d4-a716-446655440000");
}

TEST_F(ValueFormatterTest, FormatJson) {
    CellValue value;
    value.type = DataType::Json;
    value.string_value = R"({"key": "value", "number": 42})";
    
    auto result = formatter_->Format(value);
    EXPECT_NE(result.find("key"), std::string::npos);
    EXPECT_NE(result.find("value"), std::string::npos);
}

TEST_F(ValueFormatterTest, FormatBinary) {
    CellValue value;
    value.type = DataType::Binary;
    value.binary_value = {0x00, 0x01, 0x02, 0xFF};
    
    auto result = formatter_->Format(value);
    // Binary should be hex-encoded or shown as placeholder
    EXPECT_FALSE(result.empty());
}

TEST_F(ValueFormatterTest, FormatArray) {
    CellValue value;
    value.type = DataType::Array;
    value.string_value = "{1, 2, 3, 4, 5}";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "{1, 2, 3, 4, 5}");
}

TEST_F(ValueFormatterTest, FormatForCsvSimple) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Hello";
    
    auto result = formatter_->FormatForCsv(value);
    EXPECT_EQ(result, "Hello");
}

TEST_F(ValueFormatterTest, FormatForCsvWithComma) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Hello, World";
    
    auto result = formatter_->FormatForCsv(value);
    // Should be quoted
    EXPECT_EQ(result, "\"Hello, World\"");
}

TEST_F(ValueFormatterTest, FormatForCsvWithQuote) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Say \"Hello\"";
    
    auto result = formatter_->FormatForCsv(value);
    // Quotes should be doubled
    EXPECT_EQ(result, "\"Say \"\"Hello\"\"\"");
}

TEST_F(ValueFormatterTest, FormatForCsvWithNewline) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Line 1\nLine 2";
    
    auto result = formatter_->FormatForCsv(value);
    // Should be quoted
    EXPECT_EQ(result, "\"Line 1\nLine 2\"");
}

TEST_F(ValueFormatterTest, FormatForJsonString) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Hello";
    
    auto result = formatter_->FormatForJson(value);
    EXPECT_EQ(result, "\"Hello\"");
}

TEST_F(ValueFormatterTest, FormatForJsonWithSpecialChars) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "Line 1\nLine 2\tTab\"Quote";
    
    auto result = formatter_->FormatForJson(value);
    EXPECT_NE(result.find("\\n"), std::string::npos);
    EXPECT_NE(result.find("\\t"), std::string::npos);
    EXPECT_NE(result.find("\\\""), std::string::npos);
}

TEST_F(ValueFormatterTest, FormatForJsonInteger) {
    CellValue value;
    value.type = DataType::Integer;
    value.int_value = 42;
    
    auto result = formatter_->FormatForJson(value);
    EXPECT_EQ(result, "42");
}

TEST_F(ValueFormatterTest, FormatForJsonNull) {
    CellValue value;
    value.type = DataType::Null;
    
    auto result = formatter_->FormatForJson(value);
    EXPECT_EQ(result, "null");
}

TEST_F(ValueFormatterTest, FormatForJsonBoolean) {
    CellValue value;
    value.type = DataType::Boolean;
    value.bool_value = true;
    
    auto result = formatter_->FormatForJson(value);
    EXPECT_EQ(result, "true");
}

TEST_F(ValueFormatterTest, FormatWithMaxLength) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "This is a very long string that should be truncated";
    
    auto result = formatter_->Format(value, ValueFormatter::Options{.max_length = 20});
    EXPECT_EQ(result.length(), 20);
}

TEST_F(ValueFormatterTest, TruncateWithEllipsis) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "This is a very long string";
    
    auto result = formatter_->Format(value, ValueFormatter::Options{
        .max_length = 15,
        .use_ellipsis = true
    });
    EXPECT_EQ(result, "This is a ve...");
}

TEST_F(ValueFormatterTest, FormatZero) {
    CellValue value;
    value.type = DataType::Integer;
    value.int_value = 0;
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "0");
}

TEST_F(ValueFormatterTest, FormatNegative) {
    CellValue value;
    value.type = DataType::Integer;
    value.int_value = -123;
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "-123");
}

TEST_F(ValueFormatterTest, FormatVeryLargeNumber) {
    CellValue value;
    value.type = DataType::Double;
    value.double_value = 1.7976931348623157e+308;
    
    auto result = formatter_->Format(value);
    EXPECT_FALSE(result.empty());
}

TEST_F(ValueFormatterTest, FormatVerySmallNumber) {
    CellValue value;
    value.type = DataType::Double;
    value.double_value = 2.2250738585072014e-308;
    
    auto result = formatter_->Format(value);
    EXPECT_FALSE(result.empty());
}

TEST_F(ValueFormatterTest, FormatEmptyString) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "");
}

TEST_F(ValueFormatterTest, FormatWhitespaceString) {
    CellValue value;
    value.type = DataType::Text;
    value.string_value = "   \t\n   ";
    
    auto result = formatter_->Format(value);
    EXPECT_EQ(result, "   \t\n   ");
}
