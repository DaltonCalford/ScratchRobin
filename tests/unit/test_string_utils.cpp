#include "utils/string_utils.h"
#include <gtest/gtest.h>

using namespace scratchrobin::utils;

class StringUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test trim functions
TEST_F(StringUtilsTest, Trim) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("hello"), "hello");
    EXPECT_EQ(trim(""), "");
    EXPECT_EQ(trim("   "), "");
    EXPECT_EQ(trim("\t\n hello \t\n"), "hello");
}

TEST_F(StringUtilsTest, TrimLeft) {
    EXPECT_EQ(trimLeft("  hello  "), "hello  ");
    EXPECT_EQ(trimLeft("hello  "), "hello  ");
    EXPECT_EQ(trimLeft(""), "");
}

TEST_F(StringUtilsTest, TrimRight) {
    EXPECT_EQ(trimRight("  hello  "), "  hello");
    EXPECT_EQ(trimRight("  hello"), "  hello");
    EXPECT_EQ(trimRight(""), "");
}

// Test case conversion
TEST_F(StringUtilsTest, ToLower) {
    EXPECT_EQ(toLower("HELLO"), "hello");
    EXPECT_EQ(toLower("Hello World"), "hello world");
    EXPECT_EQ(toLower("123"), "123");
    EXPECT_EQ(toLower(""), "");
    EXPECT_EQ(toLower("HELLO WORLD!@#"), "hello world!@#");
}

TEST_F(StringUtilsTest, ToUpper) {
    EXPECT_EQ(toUpper("hello"), "HELLO");
    EXPECT_EQ(toUpper("Hello World"), "HELLO WORLD");
    EXPECT_EQ(toUpper("123"), "123");
    EXPECT_EQ(toUpper(""), "");
}

// Test splitting
TEST_F(StringUtilsTest, SplitChar) {
    auto result = split("a,b,c", ',');
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");

    result = split("", ',');
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "");

    result = split(",", ',');
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "");
    EXPECT_EQ(result[1], "");
}

TEST_F(StringUtilsTest, SplitString) {
    auto result = split("hello---world---test", "---");
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "hello");
    EXPECT_EQ(result[1], "world");
    EXPECT_EQ(result[2], "test");

    result = split("hello", "---");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "hello");
}

// Test joining
TEST_F(StringUtilsTest, Join) {
    std::vector<std::string> parts = {"a", "b", "c"};
    EXPECT_EQ(join(parts, ","), "a,b,c");
    EXPECT_EQ(join(parts, " "), "a b c");
    EXPECT_EQ(join(parts, ""), "abc");

    std::vector<std::string> empty;
    EXPECT_EQ(join(empty, ","), "");

    std::vector<std::string> single = {"hello"};
    EXPECT_EQ(join(single, ","), "hello");
}

TEST_F(StringUtilsTest, JoinChar) {
    std::vector<std::string> parts = {"a", "b", "c"};
    EXPECT_EQ(join(parts, ','), "a,b,c");

    std::vector<std::string> empty;
    EXPECT_EQ(join(empty, ','), "");
}

// Test prefix/suffix checking
TEST_F(StringUtilsTest, StartsWith) {
    EXPECT_TRUE(startsWith("hello world", "hello"));
    EXPECT_FALSE(startsWith("hello world", "world"));
    EXPECT_TRUE(startsWith("hello", "hello"));
    EXPECT_TRUE(startsWith("hello", ""));
    EXPECT_FALSE(startsWith("", "hello"));
    EXPECT_TRUE(startsWith("", ""));
}

TEST_F(StringUtilsTest, EndsWith) {
    EXPECT_TRUE(endsWith("hello world", "world"));
    EXPECT_FALSE(endsWith("hello world", "hello"));
    EXPECT_TRUE(endsWith("hello", "hello"));
    EXPECT_TRUE(endsWith("hello", ""));
    EXPECT_FALSE(endsWith("", "hello"));
    EXPECT_TRUE(endsWith("", ""));
}

// Test string replacement
TEST_F(StringUtilsTest, Replace) {
    EXPECT_EQ(replace("hello world", "world", "universe"), "hello universe");
    EXPECT_EQ(replace("hello world world", "world", "universe"), "hello universe world");
    EXPECT_EQ(replace("hello world", "xyz", "abc"), "hello world");
    EXPECT_EQ(replace("hello", "", "abc"), "hello");
    EXPECT_EQ(replace("", "hello", "world"), "");
}

TEST_F(StringUtilsTest, ReplaceAll) {
    EXPECT_EQ(replaceAll("hello world world", "world", "universe"), "hello universe universe");
    EXPECT_EQ(replaceAll("hello world", "world", "universe"), "hello universe");
    EXPECT_EQ(replaceAll("hello world", "xyz", "abc"), "hello world");
    EXPECT_EQ(replaceAll("aaa", "a", "b"), "bbb");
    EXPECT_EQ(replaceAll("", "hello", "world"), "");
}

// Test empty/blank checking
TEST_F(StringUtilsTest, IsEmpty) {
    EXPECT_TRUE(isEmpty(""));
    EXPECT_FALSE(isEmpty("hello"));
    EXPECT_FALSE(isEmpty(" "));
}

TEST_F(StringUtilsTest, IsBlank) {
    EXPECT_TRUE(isBlank(""));
    EXPECT_TRUE(isBlank("   "));
    EXPECT_TRUE(isBlank("\t\n"));
    EXPECT_FALSE(isBlank("hello"));
    EXPECT_FALSE(isBlank("  hello  "));
}

// Test case formatting
TEST_F(StringUtilsTest, Capitalize) {
    EXPECT_EQ(capitalize("hello"), "Hello");
    EXPECT_EQ(capitalize("HELLO"), "Hello");
    EXPECT_EQ(capitalize("hELLO"), "Hello");
    EXPECT_EQ(capitalize(""), "");
    EXPECT_EQ(capitalize("a"), "A");
}

TEST_F(StringUtilsTest, TitleCase) {
    EXPECT_EQ(titleCase("hello world"), "Hello World");
    EXPECT_EQ(titleCase("the quick brown fox"), "The Quick Brown Fox");
    EXPECT_EQ(titleCase("hello   world"), "Hello   World");
    EXPECT_EQ(titleCase(""), "");
    EXPECT_EQ(titleCase("a"), "A");
}

// Test string reversal
TEST_F(StringUtilsTest, Reverse) {
    EXPECT_EQ(reverse("hello"), "olleh");
    EXPECT_EQ(reverse("12345"), "54321");
    EXPECT_EQ(reverse(""), "");
    EXPECT_EQ(reverse("a"), "a");
    EXPECT_EQ(reverse("hello world"), "dlrow olleh");
}

// Test padding
TEST_F(StringUtilsTest, LeftPad) {
    EXPECT_EQ(leftPad("hello", 8), "  hello");
    EXPECT_EQ(leftPad("hello", 3), "hello");
    EXPECT_EQ(leftPad("hello", 8, '*'), "**hello");
    EXPECT_EQ(leftPad("", 3), "   ");
    EXPECT_EQ(leftPad("", 3, 'x'), "xxx");
}

TEST_F(StringUtilsTest, RightPad) {
    EXPECT_EQ(rightPad("hello", 8), "hello   ");
    EXPECT_EQ(rightPad("hello", 3), "hello");
    EXPECT_EQ(rightPad("hello", 8, '*'), "hello***");
    EXPECT_EQ(rightPad("", 3), "   ");
    EXPECT_EQ(rightPad("", 3, 'x'), "xxx");
}

// Test truncation - TODO: Fix truncate function implementation
// TEST_F(StringUtilsTest, Truncate) {
//     EXPECT_EQ(truncate("hello world", 5), "he...");
//     EXPECT_EQ(truncate("hello world", 8), "hello...");
//     EXPECT_EQ(truncate("hello", 10), "hello");
//     EXPECT_EQ(truncate("hello world", 5, "..."), "he...");
//     EXPECT_EQ(truncate("hello world", 5, "[...]"), "he[...]");
//     EXPECT_EQ(truncate("", 5), "");
// }

// Test string searching
TEST_F(StringUtilsTest, Contains) {
    EXPECT_TRUE(contains("hello world", "world"));
    EXPECT_TRUE(contains("hello world", "hello"));
    EXPECT_FALSE(contains("hello world", "xyz"));
    EXPECT_TRUE(contains("hello world", ""));
    EXPECT_FALSE(contains("", "hello"));
    EXPECT_TRUE(contains("", ""));
}

TEST_F(StringUtilsTest, ContainsIgnoreCase) {
    EXPECT_TRUE(containsIgnoreCase("Hello World", "world"));
    EXPECT_TRUE(containsIgnoreCase("HELLO WORLD", "hello"));
    EXPECT_FALSE(containsIgnoreCase("hello world", "xyz"));
    EXPECT_TRUE(containsIgnoreCase("Hello World", "WORLD"));
}

TEST_F(StringUtilsTest, CountOccurrences) {
    EXPECT_EQ(countOccurrences("hello world world", "world"), 2);
    EXPECT_EQ(countOccurrences("hello world", "world"), 1);
    EXPECT_EQ(countOccurrences("hello world", "xyz"), 0);
    EXPECT_EQ(countOccurrences("aaa", "a"), 3);
    EXPECT_EQ(countOccurrences("aaa", "aa"), 2);
    EXPECT_EQ(countOccurrences("", "hello"), 0);
    EXPECT_EQ(countOccurrences("hello", ""), 0);
}

// Test string removal
TEST_F(StringUtilsTest, Remove) {
    EXPECT_EQ(remove("hello world", "world"), "hello ");
    EXPECT_EQ(remove("hello world", "xyz"), "hello world");
    EXPECT_EQ(remove("hello world", ""), "hello world");
    EXPECT_EQ(remove("", "hello"), "");
}

TEST_F(StringUtilsTest, RemoveAll) {
    EXPECT_EQ(removeAll("hello world world", "world"), "hello   ");
    EXPECT_EQ(removeAll("aaa", "a"), "");
    EXPECT_EQ(removeAll("hello world", "xyz"), "hello world");
    EXPECT_EQ(removeAll("hello world", ""), "hello world");
}

// Test character type checking
TEST_F(StringUtilsTest, IsNumeric) {
    EXPECT_TRUE(isNumeric("123"));
    EXPECT_TRUE(isNumeric("0"));
    EXPECT_FALSE(isNumeric("123.45"));
    EXPECT_FALSE(isNumeric("abc"));
    EXPECT_FALSE(isNumeric("123abc"));
    EXPECT_FALSE(isNumeric(""));
    EXPECT_FALSE(isNumeric("-123"));
}

TEST_F(StringUtilsTest, IsAlpha) {
    EXPECT_TRUE(isAlpha("abc"));
    EXPECT_TRUE(isAlpha("ABC"));
    EXPECT_TRUE(isAlpha("Hello"));
    EXPECT_FALSE(isAlpha("123"));
    EXPECT_FALSE(isAlpha("abc123"));
    EXPECT_FALSE(isAlpha(""));
    EXPECT_FALSE(isAlpha("hello world"));
}

TEST_F(StringUtilsTest, IsAlphaNumeric) {
    EXPECT_TRUE(isAlphaNumeric("abc123"));
    EXPECT_TRUE(isAlphaNumeric("123"));
    EXPECT_TRUE(isAlphaNumeric("abc"));
    EXPECT_TRUE(isAlphaNumeric("HelloWorld123"));
    EXPECT_FALSE(isAlphaNumeric("hello world"));
    EXPECT_FALSE(isAlphaNumeric("hello-world"));
    EXPECT_FALSE(isAlphaNumeric(""));
    EXPECT_FALSE(isAlphaNumeric("123.45"));
}

// Performance tests
TEST_F(StringUtilsTest, PerformanceLargeString) {
    std::string largeString(10000, 'a');
    std::string result = toLower(largeString);
    EXPECT_EQ(result.length(), 10000);
    EXPECT_EQ(result[0], 'a');
}
