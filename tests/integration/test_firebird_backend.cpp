/**
 * @file test_firebird_backend.cpp
 * @brief Integration tests for Firebird backend
 * 
 * These tests require a running Firebird server.
 * Set SCRATCHROBIN_TEST_FB_DSN environment variable to enable.
 */

#include <gtest/gtest.h>
#include "core/firebird_backend.h"
#include "core/connection_parameters.h"
#include <cstdlib>

using namespace scratchrobin;

class FirebirdBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* dsn = std::getenv("SCRATCHROBIN_TEST_FB_DSN");
        if (!dsn) {
            GTEST_SKIP() << "Firebird tests skipped. Set SCRATCHROBIN_TEST_FB_DSN to enable.";
        }
        
        backend_ = std::make_unique<FirebirdBackend>();
        
        ConnectionParameters params;
        params.connection_string = dsn;
        
        if (!backend_->Connect(params)) {
            GTEST_SKIP() << "Could not connect to Firebird server";
        }
    }
    
    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
    }
    
    std::unique_ptr<FirebirdBackend> backend_;
};

TEST_F(FirebirdBackendTest, IsConnected) {
    EXPECT_TRUE(backend_->IsConnected());
}

TEST_F(FirebirdBackendTest, ExecuteSimpleQuery) {
    auto result = backend_->Query("SELECT 1 as num, 'hello' as str FROM RDB$DATABASE");
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    EXPECT_EQ(result->rows.size(), 1);
}

TEST_F(FirebirdBackendTest, ExecuteCreateAndDropTable) {
    // Create GTT (Global Temporary Table) for testing
    auto create_result = backend_->Execute(
        "CREATE GLOBAL TEMPORARY TABLE test_table ("
        "id INTEGER PRIMARY KEY, "
        "name VARCHAR(100)"
        ") ON COMMIT PRESERVE ROWS"
    );
    EXPECT_TRUE(create_result.success);
    
    // Insert data
    auto insert_result = backend_->Execute(
        "INSERT INTO test_table VALUES (1, 'Alice')"
    );
    EXPECT_TRUE(insert_result.success);
    
    // Query data
    auto query_result = backend_->Query("SELECT * FROM test_table");
    ASSERT_NE(query_result, nullptr);
    EXPECT_EQ(query_result->rows.size(), 1);
    
    // Clean up
    backend_->Execute("DROP TABLE test_table");
}

TEST_F(FirebirdBackendTest, QueryWithParameters) {
    auto result = backend_->Query(
        "SELECT CAST(? AS INTEGER) as num, CAST(? AS VARCHAR(100)) as str FROM RDB$DATABASE",
        {CellValue::FromInt(42), CellValue::FromString("test")}
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    ASSERT_EQ(result->rows.size(), 1);
}

TEST_F(FirebirdBackendTest, TransactionCommit) {
    backend_->Execute(
        "CREATE GLOBAL TEMPORARY TABLE trans_test (id INTEGER) ON COMMIT PRESERVE ROWS"
    );
    
    EXPECT_TRUE(backend_->BeginTransaction());
    
    auto insert = backend_->Execute("INSERT INTO trans_test VALUES (1)");
    EXPECT_TRUE(insert.success);
    
    EXPECT_TRUE(backend_->Commit());
    
    auto query = backend_->Query("SELECT COUNT(*) FROM trans_test");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->rows[0][0].int_value, 1);
    
    backend_->Execute("DROP TABLE trans_test");
}

TEST_F(FirebirdBackendTest, TransactionRollback) {
    backend_->Execute(
        "CREATE GLOBAL TEMPORARY TABLE trans_test (id INTEGER) ON COMMIT PRESERVE ROWS"
    );
    backend_->Execute("INSERT INTO trans_test VALUES (1)");
    
    EXPECT_TRUE(backend_->BeginTransaction());
    backend_->Execute("INSERT INTO trans_test VALUES (2)");
    EXPECT_TRUE(backend_->Rollback());
    
    auto query = backend_->Query("SELECT COUNT(*) FROM trans_test");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->rows[0][0].int_value, 1);
    
    backend_->Execute("DROP TABLE trans_test");
}

TEST_F(FirebirdBackendTest, SchemaIntrospection) {
    auto schema = backend_->LoadSchema();
    ASSERT_NE(schema, nullptr);
    
    // Should find system tables
    bool found_rdb_relations = false;
    for (const auto& table : schema->tables) {
        if (table.name == "RDB$RELATIONS") {
            found_rdb_relations = true;
            break;
        }
    }
    EXPECT_TRUE(found_rdb_relations);
}

TEST_F(FirebirdBackendTest, DetectCapabilities) {
    auto caps = backend_->GetCapabilities();
    
    EXPECT_EQ(caps.backend_type, BackendType::Firebird);
    EXPECT_TRUE(caps.supports_transactions);
    EXPECT_TRUE(caps.supports_prepared_statements);
    EXPECT_TRUE(caps.supports_savepoints);
    EXPECT_GT(caps.max_identifier_length, 0);
}

TEST_F(FirebirdBackendTest, DataTypes) {
    auto result = backend_->Query(
        "SELECT "
        "CAST(1 AS SMALLINT) as smallint, "
        "CAST(2 AS INTEGER) as int_col, "
        "CAST(3 AS BIGINT) as bigint, "
        "CAST(4.5 AS FLOAT) as float_col, "
        "CAST(5.5 AS DOUBLE PRECISION) as double_col, "
        "CAST('text' AS VARCHAR(100)) as varchar_col, "
        "CAST('2026-02-03' AS DATE) as date_col, "
        "CAST('14:30:00' AS TIME) as time_col "
        "FROM RDB$DATABASE"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->columns.size(), 8);
}

TEST_F(FirebirdBackendTest, BLOBDataType) {
    auto result = backend_->Query(
        "SELECT CAST('test blob data' AS BLOB SUB_TYPE TEXT) as blob_col FROM RDB$DATABASE"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
}

TEST_F(FirebirdBackendTest, ErrorHandling) {
    auto result = backend_->Query("SELECT * FROM NONEXISTENT_TABLE_XYZ FROM RDB$DATABASE");
    
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->success);
    EXPECT_FALSE(result->error_message.empty());
}

TEST_F(FirebirdBackendTest, GeneratorSequence) {
    // Create generator (sequence)
    backend_->Execute("CREATE GENERATOR test_gen");
    backend_->Execute("SET GENERATOR test_gen TO 0");
    
    // Get next value
    auto result = backend_->Query("SELECT GEN_ID(test_gen, 1) FROM RDB$DATABASE");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->rows[0][0].int_value, 1);
    
    // Get next value again
    result = backend_->Query("SELECT GEN_ID(test_gen, 1) FROM RDB$DATABASE");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->rows[0][0].int_value, 2);
    
    // Clean up
    backend_->Execute("DROP GENERATOR test_gen");
}

TEST_F(FirebirdBackendTest, ExecuteBlock) {
    // Firebird's execute block for anonymous procedures
    auto result = backend_->Query(
        "EXECUTE BLOCK "
        "RETURNS (result INTEGER) "
        "AS "
        "BEGIN "
        "  result = 42; "
        "  SUSPEND; "
        "END"
    );
    
    if (result && result->success) {
        EXPECT_EQ(result->rows[0][0].int_value, 42);
    }
}

TEST_F(FirebirdBackendTest, SystemTablesQuery) {
    auto result = backend_->Query(
        "SELECT RDB$RELATION_NAME FROM RDB$RELATIONS "
        "WHERE RDB$SYSTEM_FLAG = 1 "
        "FETCH FIRST 10 ROWS ONLY"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    EXPECT_GT(result->rows.size(), 0);
}

TEST_F(FirebirdBackendTest, DomainsIntrospection) {
    auto result = backend_->Query(
        "SELECT RDB$FIELD_NAME, RDB$FIELD_TYPE "
        "FROM RDB$FIELDS "
        "WHERE RDB$SYSTEM_FLAG = 0 OR RDB$SYSTEM_FLAG IS NULL "
        "FETCH FIRST 10 ROWS ONLY"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
}

TEST_F(FirebirdBackendTest, ConstraintsIntrospection) {
    auto result = backend_->Query(
        "SELECT RDB$CONSTRAINT_NAME, RDB$CONSTRAINT_TYPE "
        "FROM RDB$RELATION_CONSTRAINTS "
        "FETCH FIRST 10 ROWS ONLY"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
}

TEST_F(FirebirdBackendTest, IndicesIntrospection) {
    auto result = backend_->Query(
        "SELECT RDB$INDEX_NAME FROM RDB$INDICES "
        "FETCH FIRST 10 ROWS ONLY"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
}

TEST_F(FirebirdBackendTest, PreparedStatement) {
    // Firebird supports parameters with ? or named params
    auto result = backend_->Query(
        "SELECT CAST(? AS INTEGER) + CAST(? AS INTEGER) as sum FROM RDB$DATABASE",
        {CellValue::FromInt(10), CellValue::FromInt(20)}
    );
    
    ASSERT_NE(result, nullptr);
    if (result->success) {
        EXPECT_EQ(result->rows[0][0].int_value, 30);
    }
}

TEST_F(FirebirdBackendTest, LargeResultSet) {
    // Generate large result using recursive CTE (Firebird 2.1+)
    auto result = backend_->Query(
        "WITH RECURSIVE cnt(x) AS ( "
        "  SELECT 1 FROM RDB$DATABASE "
        "  UNION ALL "
        "  SELECT x + 1 FROM cnt WHERE x < 10000 "
        ") "
        "SELECT x FROM cnt"
    );
    
    if (result && result->success) {
        EXPECT_EQ(result->rows.size(), 10000);
    }
}
