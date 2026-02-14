/**
 * @file test_metadata_model.cpp
 * @brief Unit tests for the metadata model
 */

#include <gtest/gtest.h>
#include "core/metadata_model.h"

using namespace scratchrobin;

class MetadataModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_ = std::make_unique<MetadataModel>();
    }
    
    std::unique_ptr<MetadataModel> model_;
};

TEST_F(MetadataModelTest, InitialStateIsEmpty) {
    auto snapshot = model_->GetSnapshot();
    EXPECT_TRUE(snapshot.nodes.empty());
    // Timestamp is only set when UpdateNode is called or when loaded from fixture
}

TEST_F(MetadataModelTest, AddSingleNode) {
    MetadataNode node;
    node.id = 1;
    node.type = MetadataType::Table;
    node.name = "users";
    node.schema = "public";
    node.path = "public.users";
    
    model_->UpdateNode(node);
    
    auto snapshot = model_->GetSnapshot();
    EXPECT_EQ(snapshot.nodes.size(), 1);
    EXPECT_EQ(snapshot.nodes[0].name, "users");
}

TEST_F(MetadataModelTest, AddMultipleNodesWithHierarchy) {
    // Add schema
    MetadataNode schema;
    schema.id = 1;
    schema.type = MetadataType::Schema;
    schema.name = "public";
    schema.path = "public";
    model_->UpdateNode(schema);
    
    // Add tables under schema
    MetadataNode table1;
    table1.id = 2;
    table1.type = MetadataType::Table;
    table1.name = "users";
    table1.schema = "public";
    table1.path = "public.users";
    table1.parent_id = 1;
    model_->UpdateNode(table1);
    
    MetadataNode table2;
    table2.id = 3;
    table2.type = MetadataType::Table;
    table2.name = "orders";
    table2.schema = "public";
    table2.path = "public.orders";
    table2.parent_id = 1;
    model_->UpdateNode(table2);
    
    auto snapshot = model_->GetSnapshot();
    EXPECT_EQ(snapshot.nodes.size(), 3);
}

TEST_F(MetadataModelTest, UpdateExistingNode) {
    MetadataNode node;
    node.id = 1;
    node.type = MetadataType::Table;
    node.name = "users";
    node.row_count = 100;
    model_->UpdateNode(node);
    
    // Update the same node
    node.row_count = 200;
    model_->UpdateNode(node);
    
    auto snapshot = model_->GetSnapshot();
    EXPECT_EQ(snapshot.nodes.size(), 1);
    EXPECT_EQ(snapshot.nodes[0].row_count, 200);
}

TEST_F(MetadataModelTest, RemoveNode) {
    MetadataNode node;
    node.id = 1;
    node.type = MetadataType::Table;
    node.name = "temp_table";
    model_->UpdateNode(node);
    
    EXPECT_EQ(model_->GetSnapshot().nodes.size(), 1);
    
    model_->RemoveNode(1);
    
    EXPECT_EQ(model_->GetSnapshot().nodes.size(), 0);
}

TEST_F(MetadataModelTest, FindNodeByPath) {
    MetadataNode node;
    node.id = 1;
    node.type = MetadataType::Table;
    node.name = "users";
    node.schema = "public";
    node.path = "public.users";
    model_->UpdateNode(node);
    
    auto found = model_->FindNodeByPath("public.users");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "users");
    
    auto not_found = model_->FindNodeByPath("nonexistent");
    EXPECT_FALSE(not_found.has_value());
}

TEST_F(MetadataModelTest, FindNodesByType) {
    // Add mixed types
    MetadataNode schema;
    schema.id = 1;
    schema.type = MetadataType::Schema;
    schema.name = "public";
    model_->UpdateNode(schema);
    
    MetadataNode table;
    table.id = 2;
    table.type = MetadataType::Table;
    table.name = "users";
    model_->UpdateNode(table);
    
    MetadataNode view;
    view.id = 3;
    view.type = MetadataType::View;
    view.name = "active_users";
    model_->UpdateNode(view);
    
    auto tables = model_->FindNodesByType(MetadataType::Table);
    EXPECT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].name, "users");
    
    auto schemas = model_->FindNodesByType(MetadataType::Schema);
    EXPECT_EQ(schemas.size(), 1);
}

TEST_F(MetadataModelTest, NodeDependencies) {
    MetadataNode table;
    table.id = 1;
    table.type = MetadataType::Table;
    table.name = "orders";
    table.dependencies = {"public.users", "public.products"};
    model_->UpdateNode(table);
    
    auto snapshot = model_->GetSnapshot();
    ASSERT_EQ(snapshot.nodes.size(), 1);
    EXPECT_EQ(snapshot.nodes[0].dependencies.size(), 2);
    EXPECT_TRUE(snapshot.nodes[0].HasDependency("public.users"));
}

TEST_F(MetadataModelTest, ClearAllNodes) {
    for (int i = 0; i < 5; ++i) {
        MetadataNode node;
        node.id = i;
        node.type = MetadataType::Table;
        node.name = "table" + std::to_string(i);
        model_->UpdateNode(node);
    }
    
    EXPECT_EQ(model_->GetSnapshot().nodes.size(), 5);
    
    model_->Clear();
    
    EXPECT_EQ(model_->GetSnapshot().nodes.size(), 0);
}

TEST_F(MetadataModelTest, ObserverPattern) {
    class TestObserver : public MetadataObserver {
    public:
        int update_count = 0;
        void OnMetadataUpdated(const MetadataSnapshot& snapshot) override {
            ++update_count;
        }
    };
    
    TestObserver observer;
    model_->AddObserver(&observer);
    
    MetadataNode node;
    node.id = 1;
    node.type = MetadataType::Table;
    model_->UpdateNode(node);
    
    EXPECT_EQ(observer.update_count, 1);
    
    model_->RemoveObserver(&observer);
    
    node.id = 2;
    model_->UpdateNode(node);
    
    // Should not have incremented after removal
    EXPECT_EQ(observer.update_count, 1);
}

TEST_F(MetadataModelTest, NodeComparison) {
    MetadataNode node1;
    node1.id = 1;
    node1.name = "alpha";
    
    MetadataNode node2;
    node2.id = 2;
    node2.name = "beta";
    
    MetadataNode node3;
    node3.id = 1;  // Same ID as node1
    node3.name = "alpha";
    
    EXPECT_TRUE(node1 == node3);
    EXPECT_FALSE(node1 == node2);
}

TEST_F(MetadataModelTest, LargeMetadataSet) {
    // Test performance with many nodes
    constexpr int NUM_NODES = 1000;
    
    for (int i = 0; i < NUM_NODES; ++i) {
        MetadataNode node;
        node.id = i;
        node.type = MetadataType::Table;
        node.name = "table_" + std::to_string(i);
        node.schema = "schema_" + std::to_string(i % 10);
        node.path = node.schema + "." + node.name;
        model_->UpdateNode(node);
    }
    
    auto snapshot = model_->GetSnapshot();
    EXPECT_EQ(snapshot.nodes.size(), NUM_NODES);
    
    // Verify lookup performance
    auto found = model_->FindNodeByPath("schema_5.table_505");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, 505);
}
