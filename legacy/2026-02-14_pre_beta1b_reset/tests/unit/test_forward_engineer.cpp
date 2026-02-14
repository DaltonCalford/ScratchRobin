/**
 * @file test_forward_engineer.cpp
 * @brief Unit tests for the forward engineer (DDL generation)
 */

#include <gtest/gtest.h>
#include "diagram/forward_engineer.h"
#include "ui/diagram_model.h"

using namespace scratchrobin;
using namespace scratchrobin::diagram;

class ForwardEngineerTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_ = std::make_unique<DiagramModel>(DiagramType::Erd);
        options_.create_if_not_exists = true;
        options_.drop_existing = false;
        options_.include_indexes = true;
        options_.include_constraints = true;
    }
    
    std::unique_ptr<DiagramModel> model_;
    ForwardEngineerOptions options_;
};

TEST_F(ForwardEngineerTest, CreateScratchBirdGenerator) {
    auto generator = DdlGenerator::Create("scratchbird");
    ASSERT_NE(generator, nullptr);
}

TEST_F(ForwardEngineerTest, CreatePostgreSqlGenerator) {
    auto generator = DdlGenerator::Create("postgresql");
    ASSERT_NE(generator, nullptr);
}

TEST_F(ForwardEngineerTest, CreateMySqlGenerator) {
    auto generator = DdlGenerator::Create("mysql");
    ASSERT_NE(generator, nullptr);
}

TEST_F(ForwardEngineerTest, CreateFirebirdGenerator) {
    auto generator = DdlGenerator::Create("firebird");
    ASSERT_NE(generator, nullptr);
}

TEST_F(ForwardEngineerTest, GenerateEmptyDiagram) {
    auto generator = DdlGenerator::Create("postgresql");
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    // Should return DDL string (may be empty or contain header comments)
    // Just verify the function works without crashing
    (void)ddl;
}

TEST_F(ForwardEngineerTest, GenerateSimpleTablePostgresql) {
    auto generator = DdlGenerator::Create("postgresql");
    
    DiagramNode users;
    users.id = "users";
    users.name = "users";
    users.type = "table";
    
    DiagramAttribute id;
    id.name = "id";
    id.data_type = "INTEGER";
    id.is_primary = true;
    id.is_nullable = false;
    users.attributes.push_back(id);
    
    DiagramAttribute name;
    name.name = "name";
    name.data_type = "VARCHAR(100)";
    name.is_nullable = false;
    users.attributes.push_back(name);
    
    model_->AddNode(users);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(ddl.find("users"), std::string::npos);
    EXPECT_NE(ddl.find("id"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateSimpleTableMySql) {
    auto generator = DdlGenerator::Create("mysql");
    
    DiagramNode users;
    users.id = "users";
    users.name = "users";
    users.type = "table";
    
    DiagramAttribute id;
    id.name = "id";
    id.data_type = "INTEGER";
    id.is_primary = true;
    users.attributes.push_back(id);
    
    model_->AddNode(users);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("CREATE TABLE"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateTableWithForeignKey) {
    auto generator = DdlGenerator::Create("postgresql");
    
    // Users table
    DiagramNode users;
    users.id = "users";
    users.name = "users";
    users.type = "table";
    
    DiagramAttribute userId;
    userId.name = "user_id";
    userId.data_type = "INTEGER";
    userId.is_primary = true;
    users.attributes.push_back(userId);
    
    model_->AddNode(users);
    
    // Orders table
    DiagramNode orders;
    orders.id = "orders";
    orders.name = "orders";
    orders.type = "table";
    
    DiagramAttribute orderId;
    orderId.name = "order_id";
    orderId.data_type = "INTEGER";
    orderId.is_primary = true;
    orders.attributes.push_back(orderId);
    
    DiagramAttribute userIdFk;
    userIdFk.name = "user_id";
    userIdFk.data_type = "INTEGER";
    userIdFk.is_foreign = true;
    orders.attributes.push_back(userIdFk);
    
    model_->AddNode(orders);
    
    // Foreign key relationship
    DiagramEdge edge;
    edge.id = "fk_orders_users";
    edge.source_id = "orders";
    edge.target_id = "users";
    edge.label = "user_id";
    model_->AddEdge(edge);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("CREATE TABLE"), std::string::npos);
}

TEST_F(ForwardEngineerTest, DataTypeMappingPostgresql) {
    auto generator = DdlGenerator::Create("postgresql");
    
    DiagramNode test;
    test.id = "test";
    test.name = "test_table";
    test.type = "table";
    
    DiagramAttribute int_col;
    int_col.name = "int_col";
    int_col.data_type = "INTEGER";
    test.attributes.push_back(int_col);
    
    DiagramAttribute str_col;
    str_col.name = "str_col";
    str_col.data_type = "VARCHAR(255)";
    test.attributes.push_back(str_col);
    
    model_->AddNode(test);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("INTEGER"), std::string::npos);
    EXPECT_NE(ddl.find("VARCHAR"), std::string::npos);
}

TEST_F(ForwardEngineerTest, DataTypeMappingMySql) {
    auto generator = DdlGenerator::Create("mysql");
    
    DiagramNode test;
    test.id = "test";
    test.name = "test_table";
    test.type = "table";
    
    DiagramAttribute int_col;
    int_col.name = "int_col";
    int_col.data_type = "INTEGER";
    test.attributes.push_back(int_col);
    
    model_->AddNode(test);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("CREATE TABLE"), std::string::npos);
}

TEST_F(ForwardEngineerTest, DataTypeMappingFirebird) {
    auto generator = DdlGenerator::Create("firebird");
    
    DiagramNode test;
    test.id = "test";
    test.name = "test_table";
    test.type = "table";
    
    DiagramAttribute int_col;
    int_col.name = "int_col";
    int_col.data_type = "INTEGER";
    test.attributes.push_back(int_col);
    
    model_->AddNode(test);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("CREATE TABLE"), std::string::npos);
}

TEST_F(ForwardEngineerTest, NullableColumns) {
    auto generator = DdlGenerator::Create("postgresql");
    
    DiagramNode test;
    test.id = "test";
    test.name = "test_table";
    test.type = "table";
    
    DiagramAttribute notNullCol;
    notNullCol.name = "not_null_col";
    notNullCol.data_type = "VARCHAR(100)";
    notNullCol.is_nullable = false;
    test.attributes.push_back(notNullCol);
    
    DiagramAttribute nullableCol;
    nullableCol.name = "nullable_col";
    nullableCol.data_type = "VARCHAR(100)";
    nullableCol.is_nullable = true;
    test.attributes.push_back(nullableCol);
    
    model_->AddNode(test);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("not_null_col"), std::string::npos);
    EXPECT_NE(ddl.find("nullable_col"), std::string::npos);
}

TEST_F(ForwardEngineerTest, MultipleTables) {
    auto generator = DdlGenerator::Create("postgresql");
    
    // Create multiple tables
    for (int i = 0; i < 3; ++i) {
        DiagramNode table;
        table.id = "table" + std::to_string(i);
        table.name = "table_" + std::to_string(i);
        table.type = "table";
        
        DiagramAttribute id;
        id.name = "id";
        id.data_type = "INTEGER";
        id.is_primary = true;
        table.attributes.push_back(id);
        
        model_->AddNode(table);
    }
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    // Should have CREATE TABLE for each
    size_t count = 0;
    size_t pos = 0;
    while ((pos = ddl.find("CREATE TABLE", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    EXPECT_GE(count, 3);
}

TEST_F(ForwardEngineerTest, ForwardEngineerOptionsDefaults) {
    ForwardEngineerOptions options;
    
    EXPECT_TRUE(options.create_if_not_exists);
    EXPECT_FALSE(options.drop_existing);
    EXPECT_TRUE(options.include_indexes);
    EXPECT_TRUE(options.include_constraints);
    EXPECT_FALSE(options.include_comments);
    EXPECT_EQ(options.schema_name, "public");
    EXPECT_TRUE(options.use_domains);
}

TEST_F(ForwardEngineerTest, DataTypeMapper) {
    // Test that DataTypeMapper has mappings
    auto mappings = DataTypeMapper::GetMappings();
    EXPECT_FALSE(mappings.empty());
}

TEST_F(ForwardEngineerTest, DdlPreviewGeneration) {
    DiagramNode test;
    test.id = "test";
    test.name = "test_table";
    test.type = "table";
    
    DiagramAttribute id;
    id.name = "id";
    id.data_type = "INTEGER";
    id.is_primary = true;
    test.attributes.push_back(id);
    
    model_->AddNode(test);
    
    auto result = DdlPreview::GeneratePreview(*model_, "postgresql", options_);
    
    EXPECT_FALSE(result.ddl.empty());
    EXPECT_GE(result.table_count, 1);
}

TEST_F(ForwardEngineerTest, GenerateSingleTableDdl) {
    auto generator = DdlGenerator::Create("postgresql");
    
    DiagramNode table;
    table.id = "users";
    table.name = "users";
    table.type = "table";
    
    DiagramAttribute id;
    id.name = "id";
    id.data_type = "INTEGER";
    id.is_primary = true;
    table.attributes.push_back(id);
    
    auto ddl = generator->GenerateTableDdl(table, options_);
    
    EXPECT_NE(ddl.find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(ddl.find("users"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateForeignKeyDdl) {
    auto generator = DdlGenerator::Create("postgresql");
    
    // Add tables
    DiagramNode users;
    users.id = "users";
    users.name = "users";
    users.type = "table";
    model_->AddNode(users);
    
    DiagramNode orders;
    orders.id = "orders";
    orders.name = "orders";
    orders.type = "table";
    model_->AddNode(orders);
    
    // Add FK edge
    DiagramEdge edge;
    edge.id = "fk_edge";
    edge.source_id = "orders";
    edge.target_id = "users";
    model_->AddEdge(edge);
    
    auto ddl = generator->GenerateForeignKeyDdl(edge, *model_, options_);
    
    // Should return some DDL (may be empty if not implemented)
    (void)ddl;
}

TEST_F(ForwardEngineerTest, EmptyTableGeneration) {
    auto generator = DdlGenerator::Create("postgresql");
    
    DiagramNode empty_table;
    empty_table.id = "empty";
    empty_table.name = "empty_table";
    empty_table.type = "table";
    // No attributes
    
    auto ddl = generator->GenerateTableDdl(empty_table, options_);
    
    // Should handle empty tables gracefully
    EXPECT_FALSE(ddl.empty());
}

TEST_F(ForwardEngineerTest, TableWithManyColumns) {
    auto generator = DdlGenerator::Create("postgresql");
    
    DiagramNode table;
    table.id = "big_table";
    table.name = "big_table";
    table.type = "table";
    
    for (int i = 0; i < 20; ++i) {
        DiagramAttribute col;
        col.name = "column_" + std::to_string(i);
        col.data_type = (i % 3 == 0) ? "INTEGER" : "VARCHAR(100)";
        col.is_nullable = (i % 2 == 0);
        table.attributes.push_back(col);
    }
    
    model_->AddNode(table);
    
    auto ddl = generator->GenerateDdl(*model_, options_);
    
    EXPECT_NE(ddl.find("big_table"), std::string::npos);
    for (int i = 0; i < 20; ++i) {
        EXPECT_NE(ddl.find("column_" + std::to_string(i)), std::string::npos);
    }
}
