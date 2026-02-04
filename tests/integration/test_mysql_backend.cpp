/**
 * @file test_mysql_backend.cpp
 * @brief Integration tests for MySQL backend
 * 
 * These tests require a running MySQL/MariaDB server.
 * Set SCRATCHROBIN_TEST_MYSQL_DSN environment variable to enable.
 */

#include <gtest/gtest.h>
#include "core/mysql_backend.h"
#include "core/connection_parameters.h"
#include <cstdlib>

using namespace scratchrobin;

class MySQLBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* dsn = std::getenv("SCRATCHROBIN_TEST_MYSQL_DSN");
        if (!dsn) {
            GTEST_SKIP() << "MySQL tests skipped. Set SCRATCHROBIN_TEST_MYSQL_DSN to enable.";
        }
        
        backend_ = std::make_unique<MySQLBackend>();
        
        ConnectionParameters params;
        params.connection_string = dsn;
        
        if (!backend_->Connect(params)) {
            GTEST_SKIP() << "Could not connect to MySQL server";
        }
    }
    
    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
    }
    
    std::unique_ptr<MySQLBackend> backend_;
};

TEST_F(MySQLBackendTest, IsConnected) {
    EXPECT_TRUE(backend_->IsConnected());
}

TEST_F(MySQLBackendTest, ExecuteSimpleQuery) {
    auto result = backend_->Query("SELECT 1 as num, 'hello' as str");
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    EXPECT_EQ(result->rows.size(), 1);
}

TEST_F(MySQLBackendTest, ExecuteCreateAndDropTable) {
    // Use temporary table (auto-cleanup)
    auto create_result = backend_->Execute(
        "CREATE TEMPORARY TABLE test_table (id INT PRIMARY KEY, name VARCHAR(100))"
    );
    EXPECT_TRUE(create_result.success);
    
    // Insert data
    auto insert_result = backend_->Execute(
        "INSERT INTO test_table VALUES (1, 'Alice'), (2, 'Bob')"
    );
    EXPECT_TRUE(insert_result.success);
    EXPECT_EQ(insert_result.rows_affected, 2);
    
    // Query data
    auto query_result = backend_->Query("SELECT * FROM test_table ORDER BY id");
    ASSERT_NE(query_result, nullptr);
    EXPECT_EQ(query_result->rows.size(), 2);
}

TEST_F(MySQLBackendTest, QueryWithParameters) {
    auto result = backend_->Query(
        "SELECT ? as num, ? as str",
        {CellValue::FromInt(42), CellValue::FromString("test")}
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    ASSERT_EQ(result->rows.size(), 1);
    EXPECT_EQ(result->rows[0][0].int_value, 42);
}

TEST_F(MySQLBackendTest, TransactionCommit) {
    backend_->Execute("CREATE TEMPORARY TABLE trans_test (id INT)");
    
    EXPECT_TRUE(backend_->BeginTransaction());
    
    auto insert = backend_->Execute("INSERT INTO trans_test VALUES (1)");
    EXPECT_TRUE(insert.success);
    
    EXPECT_TRUE(backend_->Commit());
    
    auto query = backend_->Query("SELECT COUNT(*) FROM trans_test");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->rows[0][0].int_value, 1);
}

TEST_F(MySQLBackendTest, TransactionRollback) {
    backend_->Execute("CREATE TEMPORARY TABLE trans_test (id INT)");
    backend_->Execute("INSERT INTO trans_test VALUES (1)");
    
    EXPECT_TRUE(backend_->BeginTransaction());
    backend_->Execute("INSERT INTO trans_test VALUES (2)");
    EXPECT_TRUE(backend_->Rollback());
    
    auto query = backend_->Query("SELECT COUNT(*) FROM trans_test");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->rows[0][0].int_value, 1);
}

TEST_F(MySQLBackendTest, SchemaIntrospection) {
    backend_->Execute("CREATE TEMPORARY TABLE schema_test (id INT PRIMARY KEY, data VARCHAR(100))");
    backend_->Execute("CREATE INDEX idx_schema_test ON schema_test(data)");
    
    auto schema = backend_->LoadSchema();
    ASSERT_NE(schema, nullptr);
    
    // Schema should contain test_table
    bool found_test = false;
    for (const auto& table : schema->tables) {
        if (table.name == "schema_test") {
            found_test = true;
            break;
        }
    }
    EXPECT_TRUE(found_test);
}

TEST_F(MySQLBackendTest, DetectCapabilities) {
    auto caps = backend_->GetCapabilities();
    
    EXPECT_EQ(caps.backend_type, BackendType::MySQL);
    EXPECT_TRUE(caps.supports_transactions);
    EXPECT_TRUE(caps.supports_prepared_statements);
    EXPECT_GT(caps.max_identifier_length, 0);
}

TEST_F(MySQLBackendTest, DataTypes) {
    auto result = backend_->Query(
        "SELECT "
        "CAST(1 AS TINYINT) as tinyint, "
        "CAST(2 AS SMALLINT) as smallint, "
        "CAST(3 AS INT) as int_col, "
        "CAST(4 AS BIGINT) as bigint, "
        "CAST(5.5 AS FLOAT) as float_col, "
        "CAST(6.6 AS DOUBLE) as double_col, "
        "CAST(TRUE AS BOOLEAN) as bool_col, "
        "'text' as text_col"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->columns.size(), 8);
}

TEST_F(MySQLBackendTest, JSONDataType) {
    // MySQL 5.7+ supports JSON
    auto result = backend_->Query(
        "SELECT CAST('{\"key\": \"value\"}' AS JSON) as json_col"
    );
    
    // May fail on older MySQL versions
    if (result && result->success) {
        EXPECT_EQ(result->columns[0].type, DataType::Json);
    }
}

TEST_F(MySQLBackendTest, ErrorHandling) {
    auto result = backend_->Query("SELECT * FROM nonexistent_table_xyz");
    
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->success);
    EXPECT_FALSE(result->error_message.empty());
}

TEST_F(MySQLBackendTest, AutoIncrement) {
    backend_->Execute(
        "CREATE TEMPORARY TABLE auto_test ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "name VARCHAR(100)"
        ")"
    );
    
    auto insert = backend_->Execute("INSERT INTO auto_test (name) VALUES ('Alice')");
    EXPECT_TRUE(insert.success);
    EXPECT_EQ(insert.last_insert_id, "1");
    
    auto query = backend_->Query("SELECT * FROM auto_test");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->rows[0][0].int_value, 1);
}

TEST_F(MySQLBackendTest, LargeResultSet) {
    auto result = backend_->Query(
        "SELECT seq as num FROM ("
        "SELECT @row := @row + 1 as seq "
        "FROM (SELECT 0 UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3) t1, "
        "     (SELECT 0 UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3) t2, "
        "     (SELECT 0 UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3) t3, "
        "     (SELECT 0 UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3) t4, "
        "     (SELECT 0 UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3) t5, "
        "     (SELECT @row := 0) init "
        "LIMIT 10000"
        ") sub"
    );
    
    if (result && result->success) {
        EXPECT_EQ(result->rows.size(), 10000);
    }
}

TEST_F(MySQLBackendTest, ShowDatabases) {
    auto result = backend_->Query("SHOW DATABASES");
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    EXPECT_GT(result->rows.size(), 0);
    
    // Should contain at least 'information_schema'
    bool found_info_schema = false;
    for (const auto& row : result->rows) {
        if (row[0].string_value == "information_schema") {
            found_info_schema = true;
            break;
        }
    }
    EXPECT_TRUE(found_info_schema);
}

TEST_F(MySQLBackendTest, PreparedStatement) {
    auto result = backend_->Query(
        "SELECT ? + ? as sum",
        {CellValue::FromInt(10), CellValue::FromInt(20)}
    );
    
    ASSERT_NE(result, nullptr);
    if (result->success) {
        EXPECT_EQ(result->rows[0][0].int_value, 30);
    }
}
