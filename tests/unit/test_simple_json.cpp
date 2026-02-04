/**
 * @file test_simple_json.cpp
 * @brief Unit tests for the simple JSON parser/serializer
 */

#include <gtest/gtest.h>
#include "core/simple_json.h"

using namespace scratchrobin;

class SimpleJsonTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser_ = std::make_unique<SimpleJson>();
    }
    
    std::unique_ptr<SimpleJson> parser_;
};

TEST_F(SimpleJsonTest, ParseEmptyObject) {
    auto result = parser_->Parse("{}");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->object_value.empty());
}

TEST_F(SimpleJsonTest, ParseEmptyArray) {
    auto result = parser_->Parse("[]");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->array_value.empty());
}

TEST_F(SimpleJsonTest, ParseNull) {
    auto result = parser_->Parse("null");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, JsonValue::Type::Null);
}

TEST_F(SimpleJsonTest, ParseTrue) {
    auto result = parser_->Parse("true");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, JsonValue::Type::Boolean);
    EXPECT_TRUE(result->bool_value);
}

TEST_F(SimpleJsonTest, ParseFalse) {
    auto result = parser_->Parse("false");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, JsonValue::Type::Boolean);
    EXPECT_FALSE(result->bool_value);
}

TEST_F(SimpleJsonTest, ParseInteger) {
    auto result = parser_->Parse("42");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, JsonValue::Type::Number);
    EXPECT_DOUBLE_EQ(result->number_value, 42.0);
}

TEST_F(SimpleJsonTest, ParseNegativeInteger) {
    auto result = parser_->Parse("-123");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->number_value, -123.0);
}

TEST_F(SimpleJsonTest, ParseFloat) {
    auto result = parser_->Parse("3.14159");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->number_value, 3.14159);
}

TEST_F(SimpleJsonTest, ParseScientificNotation) {
    auto result = parser_->Parse("1.5e10");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->number_value, 1.5e10);
}

TEST_F(SimpleJsonTest, ParseString) {
    auto result = parser_->Parse("\"hello world\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, JsonValue::Type::String);
    EXPECT_EQ(result->string_value, "hello world");
}

TEST_F(SimpleJsonTest, ParseEmptyString) {
    auto result = parser_->Parse("\"\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->string_value, "");
}

TEST_F(SimpleJsonTest, ParseStringWithEscapedQuotes) {
    auto result = parser_->Parse(R"("say \"hello\"")");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->string_value, "say \"hello\"");
}

TEST_F(SimpleJsonTest, ParseStringWithEscapedBackslash) {
    auto result = parser_->Parse(R"("path\\to\\file")");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->string_value, "path\\to\\file");
}

TEST_F(SimpleJsonTest, ParseStringWithNewline) {
    auto result = parser_->Parse("\"line1\\nline2\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->string_value, "line1\nline2");
}

TEST_F(SimpleJsonTest, ParseStringWithTab) {
    auto result = parser_->Parse("\"col1\\tcol2\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->string_value, "col1\tcol2");
}

TEST_F(SimpleJsonTest, ParseSimpleObject) {
    auto result = parser_->Parse(R"({"name": "John", "age": 30})");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, JsonValue::Type::Object);
    EXPECT_EQ(result->object_value.size(), 2);
    EXPECT_EQ(result->object_value["name"].string_value, "John");
    EXPECT_DOUBLE_EQ(result->object_value["age"].number_value, 30);
}

TEST_F(SimpleJsonTest, ParseNestedObject) {
    auto result = parser_->Parse(R"({"person": {"name": "Jane", "age": 25}})");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->object_value["person"].object_value["name"].string_value, "Jane");
    EXPECT_DOUBLE_EQ(result->object_value["person"].object_value["age"].number_value, 25);
}

TEST_F(SimpleJsonTest, ParseSimpleArray) {
    auto result = parser_->Parse("[1, 2, 3]");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, JsonValue::Type::Array);
    EXPECT_EQ(result->array_value.size(), 3);
    EXPECT_DOUBLE_EQ(result->array_value[0].number_value, 1);
    EXPECT_DOUBLE_EQ(result->array_value[1].number_value, 2);
    EXPECT_DOUBLE_EQ(result->array_value[2].number_value, 3);
}

TEST_F(SimpleJsonTest, ParseMixedArray) {
    auto result = parser_->Parse(R"([1, "two", true, null])");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->array_value.size(), 4);
    EXPECT_EQ(result->array_value[0].type, JsonValue::Type::Number);
    EXPECT_EQ(result->array_value[1].type, JsonValue::Type::String);
    EXPECT_EQ(result->array_value[2].type, JsonValue::Type::Boolean);
    EXPECT_EQ(result->array_value[3].type, JsonValue::Type::Null);
}

TEST_F(SimpleJsonTest, ParseArrayOfObjects) {
    auto result = parser_->Parse(R"([{"id": 1}, {"id": 2}])");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->array_value.size(), 2);
    EXPECT_DOUBLE_EQ(result->array_value[0].object_value["id"].number_value, 1);
    EXPECT_DOUBLE_EQ(result->array_value[1].object_value["id"].number_value, 2);
}

TEST_F(SimpleJsonTest, ParseComplexStructure) {
    std::string json = R"({
        "users": [
            {"id": 1, "name": "Alice", "active": true},
            {"id": 2, "name": "Bob", "active": false}
        ],
        "total": 2,
        "page": 1
    })";
    
    auto result = parser_->Parse(json);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->object_value["users"].array_value.size(), 2);
    EXPECT_EQ(result->object_value["users"].array_value[0].object_value["name"].string_value, "Alice");
    EXPECT_TRUE(result->object_value["users"].array_value[0].object_value["active"].bool_value);
    EXPECT_DOUBLE_EQ(result->object_value["total"].number_value, 2);
}

TEST_F(SimpleJsonTest, ParseInvalidJson) {
    auto result = parser_->Parse("{invalid}");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SimpleJsonTest, ParseUnclosedObject) {
    auto result = parser_->Parse("{\"key\": \"value\"");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SimpleJsonTest, ParseUnclosedArray) {
    auto result = parser_->Parse("[1, 2, 3");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SimpleJsonTest, ParseUnclosedString) {
    auto result = parser_->Parse("\"unclosed string");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SimpleJsonTest, SerializeNull) {
    JsonValue value;
    value.type = JsonValue::Type::Null;
    EXPECT_EQ(value.ToString(), "null");
}

TEST_F(SimpleJsonTest, SerializeTrue) {
    JsonValue value;
    value.type = JsonValue::Type::Boolean;
    value.bool_value = true;
    EXPECT_EQ(value.ToString(), "true");
}

TEST_F(SimpleJsonTest, SerializeFalse) {
    JsonValue value;
    value.type = JsonValue::Type::Boolean;
    value.bool_value = false;
    EXPECT_EQ(value.ToString(), "false");
}

TEST_F(SimpleJsonTest, SerializeInteger) {
    JsonValue value;
    value.type = JsonValue::Type::Number;
    value.number_value = 42;
    EXPECT_EQ(value.ToString(), "42");
}

TEST_F(SimpleJsonTest, SerializeFloat) {
    JsonValue value;
    value.type = JsonValue::Type::Number;
    value.number_value = 3.14159;
    auto str = value.ToString();
    EXPECT_NE(str.find("3.14"), std::string::npos);
}

TEST_F(SimpleJsonTest, SerializeString) {
    JsonValue value;
    value.type = JsonValue::Type::String;
    value.string_value = "hello";
    EXPECT_EQ(value.ToString(), "\"hello\"");
}

TEST_F(SimpleJsonTest, SerializeStringWithQuotes) {
    JsonValue value;
    value.type = JsonValue::Type::String;
    value.string_value = "say \"hello\"";
    EXPECT_EQ(value.ToString(), "\"say \\\"hello\\\"\"");
}

TEST_F(SimpleJsonTest, SerializeStringWithNewline) {
    JsonValue value;
    value.type = JsonValue::Type::String;
    value.string_value = "line1\nline2";
    EXPECT_EQ(value.ToString(), "\"line1\\nline2\"");
}

TEST_F(SimpleJsonTest, SerializeEmptyArray) {
    JsonValue value;
    value.type = JsonValue::Type::Array;
    EXPECT_EQ(value.ToString(), "[]");
}

TEST_F(SimpleJsonTest, SerializeSimpleArray) {
    JsonValue value;
    value.type = JsonValue::Type::Array;
    value.array_value.push_back(JsonValue::FromNumber(1));
    value.array_value.push_back(JsonValue::FromNumber(2));
    value.array_value.push_back(JsonValue::FromNumber(3));
    EXPECT_EQ(value.ToString(), "[1, 2, 3]");
}

TEST_F(SimpleJsonTest, SerializeEmptyObject) {
    JsonValue value;
    value.type = JsonValue::Type::Object;
    EXPECT_EQ(value.ToString(), "{}");
}

TEST_F(SimpleJsonTest, SerializeSimpleObject) {
    JsonValue value;
    value.type = JsonValue::Type::Object;
    value.object_value["name"] = JsonValue::FromString("John");
    value.object_value["age"] = JsonValue::FromNumber(30);
    
    auto str = value.ToString();
    EXPECT_NE(str.find("\"name\":\"John\""), std::string::npos);
    EXPECT_NE(str.find("\"age\":30"), std::string::npos);
}

TEST_F(SimpleJsonTest, SerializeNestedObject) {
    JsonValue inner;
    inner.type = JsonValue::Type::Object;
    inner.object_value["x"] = JsonValue::FromNumber(10);
    
    JsonValue outer;
    outer.type = JsonValue::Type::Object;
    outer.object_value["point"] = inner;
    
    auto str = outer.ToString();
    EXPECT_NE(str.find("\"point\":{\"x\":10"), std::string::npos);
}

TEST_F(SimpleJsonTest, RoundTripParseAndSerialize) {
    std::string original = R"({"name": "Test", "value": 123, "flag": true})";
    
    auto parsed = parser_->Parse(original);
    ASSERT_TRUE(parsed.has_value());
    
    auto serialized = parsed->ToString();
    auto reparsed = parser_->Parse(serialized);
    
    ASSERT_TRUE(reparsed.has_value());
    EXPECT_EQ(reparsed->object_value["name"].string_value, "Test");
    EXPECT_DOUBLE_EQ(reparsed->object_value["value"].number_value, 123);
    EXPECT_TRUE(reparsed->object_value["flag"].bool_value);
}

TEST_F(SimpleJsonTest, ParseWhitespace) {
    auto result = parser_->Parse("  {  \"key\"  :  \"value\"  }  ");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->object_value["key"].string_value, "value");
}

TEST_F(SimpleJsonTest, ParseUnicodeEscape) {
    auto result = parser_->Parse("\"\\u0048\\u0065\\u006c\\u006c\\u006f\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->string_value, "Hello");
}

TEST_F(SimpleJsonTest, GetFieldExists) {
    auto result = parser_->Parse(R"({"a": 1, "b": 2})");
    ASSERT_TRUE(result.has_value());
    
    auto a = result->GetField("a");
    ASSERT_TRUE(a.has_value());
    EXPECT_DOUBLE_EQ(a->number_value, 1);
}

TEST_F(SimpleJsonTest, GetFieldNotExists) {
    auto result = parser_->Parse(R"({"a": 1})");
    ASSERT_TRUE(result.has_value());
    
    auto b = result->GetField("b");
    EXPECT_FALSE(b.has_value());
}

TEST_F(SimpleJsonTest, GetArrayElement) {
    auto result = parser_->Parse("[10, 20, 30]");
    ASSERT_TRUE(result.has_value());
    
    auto elem = result->GetArrayElement(1);
    ASSERT_TRUE(elem.has_value());
    EXPECT_DOUBLE_EQ(elem->number_value, 20);
}

TEST_F(SimpleJsonTest, GetArrayElementOutOfBounds) {
    auto result = parser_->Parse("[1, 2]");
    ASSERT_TRUE(result.has_value());
    
    auto elem = result->GetArrayElement(5);
    EXPECT_FALSE(elem.has_value());
}
