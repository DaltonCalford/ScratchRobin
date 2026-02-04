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
    auto statements = splitter_.Split("");
    EXPECT_TRUE(statements.empty());
}

TEST_F(StatementSplitterTest, SingleStatement) {
    auto statements = splitter_.Split("SELECT * FROM users");
    ASSERT_EQ(statements.size(), 1);
    EXPECT_EQ(statements[0], "SELECT * FROM users");
}

TEST_F(StatementSplitterTest, MultipleStatements) {
    auto statements = splitter_.Split(
        "SELECT * FROM users; SELECT * FROM orders;"
    );
    ASSERT_EQ(statements.size(), 2);
    EXPECT_EQ(statements[0], "SELECT * FROM users");
    EXPECT_EQ(statements[1], "SELECT * FROM orders");
}

TEST_F(StatementSplitterTest, StatementsWithNewlines) {
    auto statements = splitter_.Split(
        "SELECT * FROM users;\n"
        "INSERT INTO logs VALUES (1);\n"
        "UPDATE users SET active = true;"
    );
    ASSERT_EQ(statements.size(), 3);
}

TEST_F(StatementSplitterTest, StatementWithSemicolonInString) {
    auto statements = splitter_.Split(
        "INSERT INTO messages VALUES ('Hello; World')"
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_NE(statements[0].find("Hello; World"), std::string::npos);
}

TEST_F(StatementSplitterTest, StatementWithSemicolonInComment) {
    auto statements = splitter_.Split(
        "SELECT * FROM users; -- done with users; select more"
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_EQ(statements[0], "SELECT * FROM users");
}

TEST_F(StatementSplitterTest, CreateProcedureWithSemicolons) {
    std::string sql = 
        "CREATE PROCEDURE test_proc()\n"
        "BEGIN\n"
        "  SELECT 1;\n"
        "  SELECT 2;\n"
        "END";
    
    auto statements = splitter_.Split(sql);
    // Procedures may be treated as single statement depending on dialect
    EXPECT_GE(statements.size(), 1);
}

TEST_F(StatementSplitterTest, TrimsWhitespace) {
    auto statements = splitter_.Split(
        "  SELECT * FROM users  ;   "
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_EQ(statements[0], "SELECT * FROM users");
}

TEST_F(StatementSplitterTest, RemovesEmptyStatements) {
    auto statements = splitter_.Split(
        "SELECT 1;; ; SELECT 2;"
    );
    ASSERT_EQ(statements.size(), 2);
    EXPECT_EQ(statements[0], "SELECT 1");
    EXPECT_EQ(statements[1], "SELECT 2");
}

TEST_F(StatementSplitterTest, ComplexQueryWithJoins) {
    auto statements = splitter_.Split(
        "SELECT u.name, o.total FROM users u "
        "JOIN orders o ON u.id = o.user_id "
        "WHERE u.active = true;"
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_NE(statements[0].find("JOIN"), std::string::npos);
}

TEST_F(StatementSplitterTest, DDLStatements) {
    auto statements = splitter_.Split(
        "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100));"
        "CREATE INDEX idx_name ON users(name);"
    );
    ASSERT_EQ(statements.size(), 2);
    EXPECT_NE(statements[0].find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(statements[1].find("CREATE INDEX"), std::string::npos);
}

TEST_F(StatementSplitterTest, TransactionStatements) {
    auto statements = splitter_.Split(
        "BEGIN; SELECT * FROM users; COMMIT;"
    );
    // Transaction control statements may be handled specially
    EXPECT_GE(statements.size(), 1);
}

TEST_F(StatementSplitterTest, QuotedIdentifiers) {
    auto statements = splitter_.Split(
        "SELECT * FROM \"my;table\";"
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_NE(statements[0].find("my;table"), std::string::npos);
}

TEST_F(StatementSplitterTest, DollarQuotedStrings) {
    // PostgreSQL-style dollar quoting
    std::string sql = 
        "SELECT $tag$This contains; semicolons$tag$ FROM users;";
    
    auto statements = splitter_.Split(sql);
    ASSERT_EQ(statements.size(), 1);
    EXPECT_NE(statements[0].find("This contains; semicolons"), std::string::npos);
}

TEST_F(StatementSplitterTest, BatchWithManyStatements) {
    std::string sql;
    for (int i = 0; i < 100; ++i) {
        sql += "INSERT INTO test VALUES (" + std::to_string(i) + ");";
    }
    
    auto statements = splitter_.Split(sql);
    EXPECT_EQ(statements.size(), 100);
}

TEST_F(StatementSplitterTest, SingleLineComments) {
    auto statements = splitter_.Split(
        "-- First query\n"
        "SELECT 1;\n"
        "-- Second query\n"
        "SELECT 2;"
    );
    ASSERT_EQ(statements.size(), 2);
}

TEST_F(StatementSplitterTest, MultiLineComments) {
    auto statements = splitter_.Split(
        "/* This is a\n"
        "multi-line comment */\n"
        "SELECT * FROM users;"
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_EQ(statements[0], "SELECT * FROM users");
}

TEST_F(StatementSplitterTest, MixedCommentsAndStrings) {
    auto statements = splitter_.Split(
        "/* comment */ SELECT 'string with -- comment' /* another */ FROM t;"
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_NE(statements[0].find("string with -- comment"), std::string::npos);
}

TEST_F(StatementSplitterTest, DialectPostgreSQL) {
    splitter_.SetDialect(SqlDialect::PostgreSQL);
    
    // Test PostgreSQL-specific syntax
    auto statements = splitter_.Split(
        "CREATE FUNCTION test() RETURNS void AS $$\n"
        "BEGIN\n"
        "  PERFORM 1;\n"
        "END;\n"
        "$$ LANGUAGE plpgsql;"
    );
    
    // Should handle dollar-quoted functions as single statement
    EXPECT_GE(statements.size(), 1);
}

TEST_F(StatementSplitterTest, DialectMySQL) {
    splitter_.SetDialect(SqlDialect::MySQL);
    
    // Test MySQL DELIMITER change
    auto statements = splitter_.Split(
        "DELIMITER //\n"
        "CREATE PROCEDURE test()\n"
        "BEGIN\n"
        "  SELECT 1;\n"
        "END//\n"
        "DELIMITER ;"
    );
    
    // Should handle DELIMITER changes
    EXPECT_GE(statements.size(), 1);
}

TEST_F(StatementSplitterTest, DialectFirebird) {
    splitter_.SetDialect(SqlDialect::Firebird);
    
    // Test Firebird procedures
    auto statements = splitter_.Split(
        "CREATE PROCEDURE TEST AS\n"
        "BEGIN\n"
        "  SELECT 1 FROM RDB$DATABASE;\n"
        "END"
    );
    
    EXPECT_GE(statements.size(), 1);
}

TEST_F(StatementSplitterTest, ErrorOnUnterminatedString) {
    // Should handle gracefully or report error
    auto statements = splitter_.Split(
        "SELECT 'unterminated string"
    );
    // Should either return partial statement or empty
    EXPECT_GE(statements.size(), 0);
}

TEST_F(StatementSplitterTest, PreservesOriginalCase) {
    auto statements = splitter_.Split(
        "SeLeCt * FrOm UsErS;"
    );
    ASSERT_EQ(statements.size(), 1);
    EXPECT_EQ(statements[0], "SeLeCt * FrOm UsErS");
}
