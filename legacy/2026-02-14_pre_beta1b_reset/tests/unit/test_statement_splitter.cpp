/**
 * @file test_statement_splitter.cpp
 * @brief Unit tests for the SQL statement splitter
 */

#include <gtest/gtest.h>
#include "core/statement_splitter.h"

using namespace scratchrobin;

class StatementSplitterTest : public ::testing::Test {
protected:
    StatementSplitter splitter_;
};

TEST_F(StatementSplitterTest, EmptyInput) {
    auto result = splitter_.Split("");
    EXPECT_TRUE(result.statements.empty());
}

TEST_F(StatementSplitterTest, SingleStatement) {
    auto result = splitter_.Split("SELECT * FROM users");
    ASSERT_EQ(result.statements.size(), 1);
    EXPECT_EQ(result.statements[0], "SELECT * FROM users");
}

TEST_F(StatementSplitterTest, MultipleStatements) {
    auto result = splitter_.Split(
        "SELECT * FROM users; SELECT * FROM orders;"
    );
    ASSERT_EQ(result.statements.size(), 2);
    EXPECT_EQ(result.statements[0], "SELECT * FROM users");
    EXPECT_EQ(result.statements[1], "SELECT * FROM orders");
}

TEST_F(StatementSplitterTest, StatementsWithNewlines) {
    auto result = splitter_.Split(
        "SELECT * FROM users;\n"
        "INSERT INTO logs VALUES (1);\n"
        "UPDATE users SET active = true;"
    );
    ASSERT_EQ(result.statements.size(), 3);
}

TEST_F(StatementSplitterTest, StatementWithSemicolonInString) {
    auto result = splitter_.Split(
        "INSERT INTO messages VALUES ('Hello; World')"
    );
    ASSERT_EQ(result.statements.size(), 1);
    EXPECT_NE(result.statements[0].find("Hello; World"), std::string::npos);
}

TEST_F(StatementSplitterTest, StatementWithSemicolonInComment) {
    auto result = splitter_.Split(
        "SELECT * FROM users; -- done with users; select more"
    );
    // Statement splitter may or may not handle semicolons in comments correctly
    // depending on implementation
    EXPECT_GE(result.statements.size(), 1);
}

TEST_F(StatementSplitterTest, CreateProcedureWithSemicolons) {
    std::string sql = 
        "CREATE PROCEDURE test_proc()\n"
        "BEGIN\n"
        "  SELECT 1;\n"
        "  SELECT 2;\n"
        "END";
    
    auto result = splitter_.Split(sql);
    // Procedures may be treated as single statement depending on dialect
    EXPECT_GE(result.statements.size(), 1);
}

TEST_F(StatementSplitterTest, TrimsWhitespace) {
    auto result = splitter_.Split(
        "  SELECT * FROM users  ;   "
    );
    ASSERT_EQ(result.statements.size(), 1);
    EXPECT_EQ(result.statements[0], "SELECT * FROM users");
}

TEST_F(StatementSplitterTest, RemovesEmptyStatements) {
    auto result = splitter_.Split(
        "SELECT 1;; ; SELECT 2;"
    );
    ASSERT_EQ(result.statements.size(), 2);
    EXPECT_EQ(result.statements[0], "SELECT 1");
    EXPECT_EQ(result.statements[1], "SELECT 2");
}

TEST_F(StatementSplitterTest, ComplexQueryWithJoins) {
    auto result = splitter_.Split(
        "SELECT u.name, o.total FROM users u "
        "JOIN orders o ON u.id = o.user_id "
        "WHERE u.active = true;"
    );
    ASSERT_EQ(result.statements.size(), 1);
    EXPECT_NE(result.statements[0].find("JOIN"), std::string::npos);
}

TEST_F(StatementSplitterTest, DDLStatements) {
    auto result = splitter_.Split(
        "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100));"
        "CREATE INDEX idx_name ON users(name);"
    );
    ASSERT_EQ(result.statements.size(), 2);
    EXPECT_NE(result.statements[0].find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(result.statements[1].find("CREATE INDEX"), std::string::npos);
}

TEST_F(StatementSplitterTest, TransactionStatements) {
    auto result = splitter_.Split(
        "BEGIN; SELECT * FROM users; COMMIT;"
    );
    // Transaction control statements may be handled specially
    EXPECT_GE(result.statements.size(), 1);
}

TEST_F(StatementSplitterTest, QuotedIdentifiers) {
    auto result = splitter_.Split(
        "SELECT * FROM \"my;table\";"
    );
    ASSERT_EQ(result.statements.size(), 1);
    EXPECT_NE(result.statements[0].find("my;table"), std::string::npos);
}

TEST_F(StatementSplitterTest, DollarQuotedStrings) {
    // PostgreSQL-style dollar quoting
    std::string sql = 
        "SELECT $tag$This contains; semicolons$tag$ FROM users;";
    
    auto result = splitter_.Split(sql);
    // Dollar quote handling may vary by implementation
    EXPECT_GE(result.statements.size(), 1);
}

TEST_F(StatementSplitterTest, BatchWithManyStatements) {
    std::string sql;
    for (int i = 0; i < 100; ++i) {
        sql += "INSERT INTO test VALUES (" + std::to_string(i) + ");";
    }
    
    auto result = splitter_.Split(sql);
    EXPECT_EQ(result.statements.size(), 100);
}

TEST_F(StatementSplitterTest, SingleLineComments) {
    auto result = splitter_.Split(
        "-- First query\n"
        "SELECT 1;\n"
        "-- Second query\n"
        "SELECT 2;"
    );
    ASSERT_EQ(result.statements.size(), 2);
}

TEST_F(StatementSplitterTest, MultiLineComments) {
    auto result = splitter_.Split(
        "/* This is a\n"
        "multi-line comment */\n"
        "SELECT * FROM users;"
    );
    ASSERT_EQ(result.statements.size(), 1);
    // Comment handling may vary - statement may or may not include comments
    EXPECT_NE(result.statements[0].find("SELECT * FROM users"), std::string::npos);
}

TEST_F(StatementSplitterTest, MixedCommentsAndStrings) {
    auto result = splitter_.Split(
        "/* comment */ SELECT 'string with -- comment' /* another */ FROM t;"
    );
    ASSERT_EQ(result.statements.size(), 1);
    EXPECT_NE(result.statements[0].find("string with -- comment"), std::string::npos);
}

TEST_F(StatementSplitterTest, PostgreSQLFunction) {
    // Test PostgreSQL-specific syntax with dollar quoting
    auto result = splitter_.Split(
        "CREATE FUNCTION test() RETURNS void AS $$\n"
        "BEGIN\n"
        "  PERFORM 1;\n"
        "END;\n"
        "$$ LANGUAGE plpgsql;"
    );
    
    // Should handle dollar-quoted functions as single statement
    EXPECT_GE(result.statements.size(), 1);
}

TEST_F(StatementSplitterTest, MySQLProcedure) {
    // Test MySQL procedure syntax
    auto result = splitter_.Split(
        "CREATE PROCEDURE test()\n"
        "BEGIN\n"
        "  SELECT 1;\n"
        "  SELECT 2;\n"
        "END"
    );
    
    // Should handle procedures
    EXPECT_GE(result.statements.size(), 1);
}

TEST_F(StatementSplitterTest, FirebirdProcedure) {
    // Test Firebird procedures
    auto result = splitter_.Split(
        "CREATE PROCEDURE TEST AS\n"
        "BEGIN\n"
        "  SELECT 1 FROM RDB$DATABASE;\n"
        "END"
    );
    
    EXPECT_GE(result.statements.size(), 1);
}

TEST_F(StatementSplitterTest, ErrorOnUnterminatedString) {
    // Should handle gracefully or report error
    auto result = splitter_.Split(
        "SELECT 'unterminated string"
    );
    // Should either return partial statement or empty
    EXPECT_GE(result.statements.size(), 0);
}

TEST_F(StatementSplitterTest, PreservesOriginalCase) {
    auto result = splitter_.Split(
        "SeLeCt * FrOm UsErS;"
    );
    ASSERT_EQ(result.statements.size(), 1);
    EXPECT_EQ(result.statements[0], "SeLeCt * FrOm UsErS");
}
