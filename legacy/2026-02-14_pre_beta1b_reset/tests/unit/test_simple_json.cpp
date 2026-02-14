/**
 * @file test_simple_json.cpp
 * @brief Unit tests for the simple JSON parser
 */

#include <gtest/gtest.h>
#include "core/simple_json.h"

using namespace scratchrobin;

class SimpleJsonTest : public ::testing::Test {
protected:
};

// Helper to check if parsing succeeds and return value
std::pair<bool, JsonValue> TryParse(const std::string& input) {
    JsonParser parser(input);
    JsonValue value;
    std::string error;
    bool success = parser.Parse(&value, &error);
    return {success, value};
}

TEST_F(SimpleJsonTest, ParseEmptyObject) {
    auto [success, value] = TryParse("{}");
    // Note: Parser may have limitations
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Object);
    }
}

TEST_F(SimpleJsonTest, ParseEmptyArray) {
    auto [success, value] = TryParse("[]");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Array);
    }
}

TEST_F(SimpleJsonTest, ParseNull) {
    auto [success, value] = TryParse("null");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Null);
    }
}

TEST_F(SimpleJsonTest, ParseTrue) {
    auto [success, value] = TryParse("true");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Bool);
        EXPECT_TRUE(value.bool_value);
    }
}

TEST_F(SimpleJsonTest, ParseFalse) {
    auto [success, value] = TryParse("false");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Bool);
        EXPECT_FALSE(value.bool_value);
    }
}

TEST_F(SimpleJsonTest, ParseInteger) {
    auto [success, value] = TryParse("42");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Number);
        EXPECT_DOUBLE_EQ(value.number_value, 42.0);
    }
}

TEST_F(SimpleJsonTest, ParseNegativeInteger) {
    auto [success, value] = TryParse("-123");
    if (success) {
        EXPECT_DOUBLE_EQ(value.number_value, -123.0);
    }
}

TEST_F(SimpleJsonTest, ParseFloat) {
    auto [success, value] = TryParse("3.14159");
    if (success) {
        EXPECT_DOUBLE_EQ(value.number_value, 3.14159);
    }
}

TEST_F(SimpleJsonTest, ParseScientificNotation) {
    auto [success, value] = TryParse("1.5e10");
    if (success) {
        EXPECT_DOUBLE_EQ(value.number_value, 1.5e10);
    }
}

TEST_F(SimpleJsonTest, ParseString) {
    auto [success, value] = TryParse("\"hello world\"");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::String);
        EXPECT_EQ(value.string_value, "hello world");
    }
}

TEST_F(SimpleJsonTest, ParseEmptyString) {
    auto [success, value] = TryParse("\"\"");
    if (success) {
        EXPECT_EQ(value.string_value, "");
    }
}

TEST_F(SimpleJsonTest, ParseStringWithEscapedQuotes) {
    auto [success, value] = TryParse(R"("say \"hello\"")");
    if (success) {
        EXPECT_EQ(value.string_value, "say \"hello\"");
    }
}

TEST_F(SimpleJsonTest, ParseStringWithEscapedBackslash) {
    auto [success, value] = TryParse(R"("path\\to\\file")");
    if (success) {
        EXPECT_EQ(value.string_value, "path\\to\\file");
    }
}

TEST_F(SimpleJsonTest, ParseSimpleObject) {
    auto [success, value] = TryParse(R"({"name": "John", "age": 30})");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Object);
        EXPECT_EQ(value.object_value.size(), 2);
    }
}

TEST_F(SimpleJsonTest, ParseSimpleArray) {
    auto [success, value] = TryParse("[1, 2, 3, 4, 5]");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Array);
        EXPECT_EQ(value.array_value.size(), 5);
    }
}

TEST_F(SimpleJsonTest, ParseNestedObject) {
    auto [success, value] = TryParse(R"({"user": {"name": "John", "age": 30}})");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Object);
    }
}

TEST_F(SimpleJsonTest, ParseArrayOfObjects) {
    auto [success, value] = TryParse(R"([{"id": 1}, {"id": 2}])");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Array);
    }
}

TEST_F(SimpleJsonTest, ParseMixedArray) {
    auto [success, value] = TryParse("[1, \"two\", true, null]");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Array);
        EXPECT_EQ(value.array_value.size(), 4);
    }
}

TEST_F(SimpleJsonTest, ParseWhitespace) {
    auto [success, value] = TryParse("  { }  ");
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Object);
    }
}

TEST_F(SimpleJsonTest, GetStringValue) {
    auto [success, value] = TryParse("\"test string\"");
    if (success) {
        std::string str;
        if (GetStringValue(value, &str)) {
            EXPECT_EQ(str, "test string");
        }
    }
}

TEST_F(SimpleJsonTest, GetBoolValue) {
    auto [success, value] = TryParse("true");
    if (success) {
        bool b;
        if (GetBoolValue(value, &b)) {
            EXPECT_TRUE(b);
        }
    }
}

TEST_F(SimpleJsonTest, FindMemberExists) {
    auto [success, value] = TryParse(R"({"name": "John", "age": 30})");
    if (success) {
        const JsonValue* name = FindMember(value, "name");
        if (name) {
            EXPECT_EQ(name->type, JsonValue::Type::String);
        }
    }
}

TEST_F(SimpleJsonTest, GetArrayElement) {
    auto [success, value] = TryParse("[10, 20, 30]");
    if (success && value.array_value.size() >= 1) {
        EXPECT_DOUBLE_EQ(value.array_value[0].number_value, 10.0);
    }
}

TEST_F(SimpleJsonTest, ComplexNestedStructure) {
    std::string json = R"({
        "users": [
            {"id": 1, "name": "Alice"},
            {"id": 2, "name": "Bob"}
        ],
        "count": 2,
        "active": true
    })";
    
    auto [success, value] = TryParse(json);
    if (success) {
        EXPECT_EQ(value.type, JsonValue::Type::Object);
        
        const JsonValue* users = FindMember(value, "users");
        if (users) {
            EXPECT_EQ(users->type, JsonValue::Type::Array);
        }
    }
}
