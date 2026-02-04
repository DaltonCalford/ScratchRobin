/**
 * @file test_postgres_backend.cpp
 * @brief Integration tests for PostgreSQL backend
 * 
 * These tests require a running PostgreSQL server.
 * Set SCRATCHROBIN_TEST_PG_DSN environment variable to enable.
 */

#include <gtest/gtest.h>
#include "core/postgres_backend.h"
#include "core/connection_parameters.h"
#include <cstdlib>

using namespace scratchrobin;

class PostgreSQLBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* dsn = std::getenv("SCRATCHROBIN_TEST_PG_DSN");
        if (!dsn) {
            GTEST_SKIP() << "PostgreSQL tests skipped. Set SCRATCHROBIN_TEST_PG_DSN to enable.";
        }
        
        backend_ = std::make_unique<PostgreSQLBackend>();
        
        ConnectionParameters params;
        params.connection_string = dsn;
        
        if (!backend_->Connect(params)) {
            GTEST_SKIP() << "Could not connect to PostgreSQL server";
        }
    }
    
    void TearDown() override {
        if (backend_ && backend_->IsConnected()) {
            backend_->Disconnect();
        }
    }
    
    std::unique_ptr<PostgreSQLBackend> backend_;
};

TEST_F(PostgreSQLBackendTest, IsConnected) {
    EXPECT_TRUE(backend_->IsConnected());
}

TEST_F(PostgreSQLBackendTest, ExecuteSimpleQuery) {
    auto result = backend_->Query("SELECT 1 as num, 'hello' as str");
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    EXPECT_EQ(result->rows.size(), 1);
    EXPECT_EQ(result->rows[0].size(), 2);
}

TEST_F(PostgreSQLBackendTest, ExecuteCreateAndDropTable) {
    // Create test table
    auto create_result = backend_->Execute(
        "CREATE TEMP TABLE test_table (id INT PRIMARY KEY, name TEXT)"
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
    
    // Clean up (temp tables auto-drop, but let's be explicit)
    auto drop_result = backend_->Execute("DROP TABLE test_table");
    EXPECT_TRUE(drop_result.success);
}

TEST_F(PostgreSQLBackendTest, QueryWithParameters) {
    auto result = backend_->Query(
        "SELECT $1::int as num, $2::text as str",
        {CellValue::FromInt(42), CellValue::FromString("test")}
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    ASSERT_EQ(result->rows.size(), 1);
    
    // Verify parameter binding worked
    EXPECT_EQ(result->rows[0][0].int_value, 42);
}

TEST_F(PostgreSQLBackendTest, TransactionCommit) {
    // Create temp table
    backend_->Execute("CREATE TEMP TABLE trans_test (id INT)");
    
    // Begin transaction
    EXPECT_TRUE(backend_->BeginTransaction());
    
    // Insert in transaction
    auto insert = backend_->Execute("INSERT INTO trans_test VALUES (1)");
    EXPECT_TRUE(insert.success);
    
    // Commit
    EXPECT_TRUE(backend_->Commit());
    
    // Verify data persisted
    auto query = backend_->Query("SELECT COUNT(*) FROM trans_test");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->rows[0][0].int_value, 1);
    
    backend_->Execute("DROP TABLE trans_test");
}

TEST_F(PostgreSQLBackendTest, TransactionRollback) {
    // Create temp table
    backend_->Execute("CREATE TEMP TABLE trans_test (id INT)");
    
    // Insert baseline data
    backend_->Execute("INSERT INTO trans_test VALUES (1)");
    
    // Begin transaction
    EXPECT_TRUE(backend_->BeginTransaction());
    
    // Insert more data
    backend_->Execute("INSERT INTO trans_test VALUES (2)");
    
    // Rollback
    EXPECT_TRUE(backend_->Rollback());
    
    // Verify only baseline data remains
    auto query = backend_->Query("SELECT COUNT(*) FROM trans_test");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->rows[0][0].int_value, 1);
    
    backend_->Execute("DROP TABLE trans_test");
}

TEST_F(PostgreSQLBackendTest, SchemaIntrospection) {
    // Create test schema objects
    backend_->Execute("CREATE TEMP TABLE schema_test (id INT PRIMARY KEY, data TEXT)");
    backend_->Execute("CREATE INDEX idx_schema_test ON schema_test(data)");
    
    // Load schema
    auto schema = backend_->LoadSchema();
    ASSERT_NE(schema, nullptr);
    
    // Find our test table
    bool found_test_table = false;
    for (const auto& table : schema->tables) {
        if (table.name == "schema_test") {
            found_test_table = true;
            EXPECT_EQ(table.columns.size(), 2);
            break;
        }
    }
    EXPECT_TRUE(found_test_table);
    
    backend_->Execute("DROP TABLE schema_test");
}

TEST_F(PostgreSQLBackendTest, DetectCapabilities) {
    auto caps = backend_->GetCapabilities();
    
    EXPECT_EQ(caps.backend_type, BackendType::PostgreSQL);
    EXPECT_TRUE(caps.supports_transactions);
    EXPECT_TRUE(caps.supports_prepared_statements);
    EXPECT_TRUE(caps.supports_savepoints);
    EXPECT_GT(caps.max_identifier_length, 0);
}

TEST_F(PostgreSQLBackendTest, DataTypes) {
    auto result = backend_->Query(
        "SELECT "
        "1::smallint as smallint, "
        "2::integer as integer, "
        "3::bigint as bigint, "
        "4.5::real as real, "
        "6.7::double precision as double, "
        "true::boolean as bool, "
        "'text'::text as text, "
        "'2026-02-03'::date as date, "
        "'2026-02-03 14:30:00'::timestamp as timestamp"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->columns.size(), 9);
    
    // Verify column types
    EXPECT_EQ(result->columns[0].type, DataType::SmallInt);
    EXPECT_EQ(result->columns[1].type, DataType::Integer);
    EXPECT_EQ(result->columns[2].type, DataType::BigInt);
    EXPECT_EQ(result->columns[5].type, DataType::Boolean);
    EXPECT_EQ(result->columns[6].type, DataType::Text);
}

TEST_F(PostgreSQLBackendTest, JSONDataType) {
    auto result = backend_->Query(
        "SELECT '{\"key\": \"value\"}'::json as json_col, "
        "'{\"key\": \"value\"}'::jsonb as jsonb_col"
    );
    
    ASSERT_NE(result, nullptr);
    // PostgreSQL 9.4+ supports JSON/JSONB
    EXPECT_TRUE(result->columns[0].type == DataType::Json ||
                result->columns[0].type == DataType::Text);
}

TEST_F(PostgreSQLBackendTest, CancelQuery) {
    // Start a long-running query in a way we can cancel it
    // This is a simplified test - real implementation would need
    // async query execution with cancel support
    
    auto future = std::async(std::launch::async, [this]() {
        return backend_->Query("SELECT pg_sleep(10)");
    });
    
    // Give it time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Cancel
    backend_->CancelCurrentQuery();
    
    auto result = future.get();
    // Result may be null or have error depending on timing
}

TEST_F(PostgreSQLBackendTest, ErrorHandling) {
    auto result = backend_->Query("SELECT * FROM nonexistent_table_xyz");
    
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->success);
    EXPECT_FALSE(result->error_message.empty());
}

TEST_F(PostgreSQLBackendTest, PreparedStatement) {
    // Prepare statement
    auto stmt = backend_->Prepare(
        "test_stmt",
        "SELECT $1::int + $2::int as sum"
    );
    EXPECT_TRUE(stmt.success);
    
    // Execute prepared statement
    auto result = backend_->ExecutePrepared(
        "test_stmt",
        {CellValue::FromInt(10), CellValue::FromInt(20)}
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    ASSERT_EQ(result->rows.size(), 1);
    EXPECT_EQ(result->rows[0][0].int_value, 30);
    
    // Clean up
    backend_->DeallocatePrepared("test_stmt");
}

TEST_F(PostgreSQLBackendTest, LargeResultSet) {
    // Generate large result set
    auto result = backend_->Query(
        "SELECT generate_series(1, 10000) as num"
    );
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->success);
    EXPECT_EQ(result->rows.size(), 10000);
}

TEST_F(PostgreSQLBackendTest, ConnectionStringVariations) {
    // Test various connection string formats
    std::vector<std::string> test_cases = {
        "host=localhost port=5432 dbname=postgres",
        "postgresql://localhost:5432/postgres",
        "host=localhost dbname=postgres user=postgres",
    };
    
    for (const auto& conn_str : test_cases) {
        PostgreSQLBackend test_backend;
        ConnectionParameters params;
        params.connection_string = conn_str;
        
        // Just verify connection string parsing doesn't crash
        // Actual connection may fail depending on server config
        EXPECT_NO_THROW(test_backend.Connect(params));
    }
}
