/**
 * @file test_forward_engineer.cpp
 * @brief Unit tests for the forward engineer (DDL generation)
 */

#include <gtest/gtest.h>
#include "diagram/forward_engineer.h"
#include "diagram/diagram_model.h"

using namespace scratchrobin;

class ForwardEngineerTest : public ::testing::Test {
protected:
    void SetUp() override {
        engineer_ = std::make_unique<ForwardEngineer>();
        model_ = std::make_unique<DiagramModel>();
    }
    
    std::unique_ptr<ForwardEngineer> engineer_;
    std::unique_ptr<DiagramModel> model_;
};

TEST_F(ForwardEngineerTest, GenerateEmptyDiagram) {
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    EXPECT_TRUE(ddl.empty());
}

TEST_F(ForwardEngineerTest, GenerateSimpleTablePostgresql) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute id;
    id.name = "id";
    id.type = "INTEGER";
    id.is_primary_key = true;
    id.is_nullable = false;
    users.attributes.push_back(id);
    
    Attribute name;
    name.name = "name";
    name.type = "VARCHAR(100)";
    name.is_nullable = false;
    users.attributes.push_back(name);
    
    model_->AddEntity(users);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    EXPECT_NE(ddl.find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(ddl.find("users"), std::string::npos);
    EXPECT_NE(ddl.find("id"), std::string::npos);
    EXPECT_NE(ddl.find("PRIMARY KEY"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateTableWithForeignKey) {
    // Users table
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute userId;
    userId.name = "user_id";
    userId.type = "INTEGER";
    userId.is_primary_key = true;
    users.attributes.push_back(userId);
    
    model_->AddEntity(users);
    
    // Orders table with FK
    Entity orders;
    orders.id = "orders";
    orders.name = "orders";
    
    Attribute orderId;
    orderId.name = "order_id";
    orderId.type = "INTEGER";
    orderId.is_primary_key = true;
    orders.attributes.push_back(orderId);
    
    Attribute userIdFk;
    userIdFk.name = "user_id";
    userIdFk.type = "INTEGER";
    userIdFk.is_foreign_key = true;
    userIdFk.referenced_table = "users";
    userIdFk.referenced_column = "user_id";
    orders.attributes.push_back(userIdFk);
    
    model_->AddEntity(orders);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    EXPECT_NE(ddl.find("FOREIGN KEY"), std::string::npos);
    EXPECT_NE(ddl.find("REFERENCES users"), std::string::npos);
}

TEST_F(ForwardEngineerTest, DataTypeMappingPostgresql) {
    Entity test;
    test.id = "test";
    test.name = "test_table";
    
    struct TypeTest {
        std::string logical_type;
        std::string expected_pg;
    };
    
    std::vector<TypeTest> tests = {
        {"INT32", "INTEGER"},
        {"INT64", "BIGINT"},
        {"FLOAT32", "REAL"},
        {"FLOAT64", "DOUBLE PRECISION"},
        {"BOOLEAN", "BOOLEAN"},
        {"TEXT", "TEXT"},
        {"BLOB", "BYTEA"},
    };
    
    for (const auto& t : tests) {
        Attribute attr;
        attr.name = "col_" + t.logical_type;
        attr.type = t.logical_type;
        test.attributes.push_back(attr);
    }
    
    model_->AddEntity(test);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    for (const auto& t : tests) {
        EXPECT_NE(ddl.find(t.expected_pg), std::string::npos)
            << "Expected to find " << t.expected_pg << " for type " << t.logical_type;
    }
}

TEST_F(ForwardEngineerTest, DataTypeMappingMySQL) {
    Entity test;
    test.id = "test";
    test.name = "test_table";
    
    Attribute textAttr;
    textAttr.name = "description";
    textAttr.type = "TEXT";
    test.attributes.push_back(textAttr);
    
    Attribute blobAttr;
    blobAttr.name = "data";
    blobAttr.type = "BLOB";
    test.attributes.push_back(blobAttr);
    
    model_->AddEntity(test);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::MySQL);
    
    EXPECT_NE(ddl.find("LONGTEXT"), std::string::npos);  // TEXT maps to LONGTEXT in MySQL
    EXPECT_NE(ddl.find("LONGBLOB"), std::string::npos);  // BLOB maps to LONGBLOB
}

TEST_F(ForwardEngineerTest, DataTypeMappingFirebird) {
    Entity test;
    test.id = "test";
    test.name = "test_table";
    
    Attribute textAttr;
    textAttr.name = "description";
    textAttr.type = "TEXT";
    test.attributes.push_back(textAttr);
    
    model_->AddEntity(test);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::Firebird);
    
    EXPECT_NE(ddl.find("BLOB SUB_TYPE TEXT"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateWithIfNotExists) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute id;
    id.name = "id";
    id.type = "INTEGER";
    users.attributes.push_back(id);
    
    model_->AddEntity(users);
    
    DDLGenerationOptions options;
    options.use_if_not_exists = true;
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL, options);
    
    EXPECT_NE(ddl.find("IF NOT EXISTS"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateWithoutDrop) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute id;
    id.name = "id";
    id.type = "INTEGER";
    users.attributes.push_back(id);
    
    model_->AddEntity(users);
    
    DDLGenerationOptions options;
    options.include_drops = false;
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL, options);
    
    EXPECT_EQ(ddl.find("DROP TABLE"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateWithDrop) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute id;
    id.name = "id";
    id.type = "INTEGER";
    users.attributes.push_back(id);
    
    model_->AddEntity(users);
    
    DDLGenerationOptions options;
    options.include_drops = true;
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL, options);
    
    EXPECT_NE(ddl.find("DROP TABLE IF EXISTS"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateIndex) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute name;
    name.name = "name";
    name.type = "VARCHAR(100)";
    name.is_indexed = true;
    users.attributes.push_back(name);
    
    model_->AddEntity(users);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    EXPECT_NE(ddl.find("CREATE INDEX"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateUniqueConstraint) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute email;
    email.name = "email";
    email.type = "VARCHAR(255)";
    email.is_unique = true;
    users.attributes.push_back(email);
    
    model_->AddEntity(users);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    EXPECT_NE(ddl.find("UNIQUE"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateDefaultValues) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute created;
    created.name = "created_at";
    created.type = "TIMESTAMP";
    created.default_value = "NOW()";
    users.attributes.push_back(created);
    
    Attribute active;
    active.name = "active";
    active.type = "BOOLEAN";
    active.default_value = "true";
    users.attributes.push_back(active);
    
    model_->AddEntity(users);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    EXPECT_NE(ddl.find("DEFAULT NOW()"), std::string::npos);
    EXPECT_NE(ddl.find("DEFAULT true"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateMultipleTables) {
    // Create multiple entities
    for (int i = 0; i < 3; ++i) {
        Entity e;
        e.id = "table" + std::to_string(i);
        e.name = "table_" + std::to_string(i);
        
        Attribute id;
        id.name = "id";
        id.type = "INTEGER";
        id.is_primary_key = true;
        e.attributes.push_back(id);
        
        model_->AddEntity(e);
    }
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    EXPECT_NE(ddl.find("table_0"), std::string::npos);
    EXPECT_NE(ddl.find("table_1"), std::string::npos);
    EXPECT_NE(ddl.find("table_2"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateInDependencyOrder) {
    // Child table first (to test ordering)
    Entity orders;
    orders.id = "orders";
    orders.name = "orders";
    
    Attribute orderId;
    orderId.name = "order_id";
    orderId.type = "INTEGER";
    orderId.is_primary_key = true;
    orders.attributes.push_back(orderId);
    
    Attribute userIdFk;
    userIdFk.name = "user_id";
    userIdFk.type = "INTEGER";
    userIdFk.is_foreign_key = true;
    userIdFk.referenced_table = "users";
    orders.attributes.push_back(userIdFk);
    
    model_->AddEntity(orders);
    
    // Parent table second
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute userId;
    userId.name = "user_id";
    userId.type = "INTEGER";
    userId.is_primary_key = true;
    users.attributes.push_back(userId);
    
    model_->AddEntity(users);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    // Users table should come before orders (due to FK dependency)
    size_t users_pos = ddl.find("CREATE TABLE users");
    size_t orders_pos = ddl.find("CREATE TABLE orders");
    
    EXPECT_LT(users_pos, orders_pos);
}

TEST_F(ForwardEngineerTest, EscapeReservedWords) {
    Entity order;
    order.id = "order";
    order.name = "order";  // Reserved word in SQL
    
    Attribute select;
    select.name = "select";  // Reserved word
    select.type = "INTEGER";
    order.attributes.push_back(select);
    
    model_->AddEntity(order);
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    
    // Should quote reserved words
    EXPECT_NE(ddl.find("\"order\""), std::string::npos);
    EXPECT_NE(ddl.find("\"select\""), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateAutoIncrement) {
    Entity users;
    users.id = "users";
    users.name = "users";
    
    Attribute id;
    id.name = "id";
    id.type = "INTEGER";
    id.is_primary_key = true;
    id.is_auto_increment = true;
    users.attributes.push_back(id);
    
    model_->AddEntity(users);
    
    auto pg_ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL);
    EXPECT_NE(pg_ddl.find("SERIAL"), std::string::npos);
    
    auto mysql_ddl = engineer_->GenerateDDL(*model_, SqlDialect::MySQL);
    EXPECT_NE(mysql_ddl.find("AUTO_INCREMENT"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateComment) {
    Entity users;
    users.id = "users";
    users.name = "users";
    users.comment = "Stores user account information";
    
    Attribute email;
    email.name = "email";
    email.type = "VARCHAR(255)";
    email.comment = "User's email address";
    users.attributes.push_back(email);
    
    model_->AddEntity(users);
    
    DDLGenerationOptions options;
    options.include_comments = true;
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL, options);
    
    EXPECT_NE(ddl.find("COMMENT ON"), std::string::npos);
    EXPECT_NE(ddl.find("user account information"), std::string::npos);
}

TEST_F(ForwardEngineerTest, GenerateSchemaPrefix) {
    Entity users;
    users.id = "users";
    users.name = "users";
    users.schema = "app_data";
    
    Attribute id;
    id.name = "id";
    id.type = "INTEGER";
    users.attributes.push_back(id);
    
    model_->AddEntity(users);
    
    DDLGenerationOptions options;
    options.use_schema_prefix = true;
    
    auto ddl = engineer_->GenerateDDL(*model_, SqlDialect::PostgreSQL, options);
    
    EXPECT_NE(ddl.find("app_data.users"), std::string::npos);
}
