/**
 * @file test_diagram_model.cpp
 * @brief Unit tests for the diagram model
 */

#include <gtest/gtest.h>
#include "ui/diagram_model.h"

using namespace scratchrobin;

class DiagramModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_ = std::make_unique<DiagramModel>(DiagramType::Erd);
    }
    
    std::unique_ptr<DiagramModel> model_;
};

TEST_F(DiagramModelTest, CreateEmptyDiagram) {
    EXPECT_TRUE(model_->nodes().empty());
    EXPECT_TRUE(model_->edges().empty());
}

TEST_F(DiagramModelTest, AddNode) {
    DiagramNode node;
    node.id = "node1";
    node.name = "Users";
    node.x = 100.0;
    node.y = 200.0;
    node.width = 150.0;
    node.height = 200.0;
    
    model_->AddNode(node);
    
    auto& nodes = model_->nodes();
    ASSERT_EQ(nodes.size(), 1);
    EXPECT_EQ(nodes[0].name, "Users");
    EXPECT_EQ(nodes[0].x, 100.0);
    EXPECT_EQ(nodes[0].y, 200.0);
}

TEST_F(DiagramModelTest, AddEdge) {
    // Add two nodes first
    DiagramNode node1;
    node1.id = "node1";
    node1.name = "Users";
    model_->AddNode(node1);
    
    DiagramNode node2;
    node2.id = "node2";
    node2.name = "Orders";
    model_->AddNode(node2);
    
    // Add edge between them
    DiagramEdge edge;
    edge.id = "edge1";
    edge.source_id = "node1";
    edge.target_id = "node2";
    edge.label = "has many";
    edge.source_cardinality = Cardinality::One;
    edge.target_cardinality = Cardinality::ZeroOrMany;
    
    model_->AddEdge(edge);
    
    auto& edges = model_->edges();
    ASSERT_EQ(edges.size(), 1);
    EXPECT_EQ(edges[0].source_id, "node1");
    EXPECT_EQ(edges[0].target_id, "node2");
    EXPECT_EQ(edges[0].label, "has many");
}

TEST_F(DiagramModelTest, RemoveNodeByClearing) {
    DiagramNode node;
    node.id = "node1";
    model_->AddNode(node);
    
    // Clear all nodes
    model_->nodes().clear();
    
    EXPECT_TRUE(model_->nodes().empty());
}

TEST_F(DiagramModelTest, DiagramType) {
    EXPECT_EQ(model_->type(), DiagramType::Erd);
    
    model_->set_type(DiagramType::Silverston);
    EXPECT_EQ(model_->type(), DiagramType::Silverston);
}

TEST_F(DiagramModelTest, ErdNotation) {
    // Default notation
    EXPECT_EQ(model_->notation(), ErdNotation::CrowsFoot);
    
    // Set different notations
    model_->set_notation(ErdNotation::IDEF1X);
    EXPECT_EQ(model_->notation(), ErdNotation::IDEF1X);
    
    model_->set_notation(ErdNotation::UML);
    EXPECT_EQ(model_->notation(), ErdNotation::UML);
    
    model_->set_notation(ErdNotation::Chen);
    EXPECT_EQ(model_->notation(), ErdNotation::Chen);
}

TEST_F(DiagramModelTest, NextNodeIndex) {
    int idx1 = model_->NextNodeIndex();
    int idx2 = model_->NextNodeIndex();
    int idx3 = model_->NextNodeIndex();
    
    EXPECT_EQ(idx2, idx1 + 1);
    EXPECT_EQ(idx3, idx2 + 1);
}

TEST_F(DiagramModelTest, NextEdgeIndex) {
    int idx1 = model_->NextEdgeIndex();
    int idx2 = model_->NextEdgeIndex();
    
    EXPECT_EQ(idx2, idx1 + 1);
}

TEST_F(DiagramModelTest, NodeWithAttributes) {
    DiagramNode node;
    node.id = "table1";
    node.name = "users";
    
    DiagramAttribute attr1;
    attr1.name = "id";
    attr1.data_type = "INTEGER";
    attr1.is_primary = true;
    attr1.is_nullable = false;
    node.attributes.push_back(attr1);
    
    DiagramAttribute attr2;
    attr2.name = "email";
    attr2.data_type = "VARCHAR(255)";
    attr2.is_primary = false;
    attr2.is_nullable = true;
    node.attributes.push_back(attr2);
    
    model_->AddNode(node);
    
    auto& nodes = model_->nodes();
    ASSERT_EQ(nodes[0].attributes.size(), 2);
    EXPECT_EQ(nodes[0].attributes[0].name, "id");
    EXPECT_TRUE(nodes[0].attributes[0].is_primary);
    EXPECT_FALSE(nodes[0].attributes[0].is_nullable);
}

TEST_F(DiagramModelTest, CardinalityValues) {
    DiagramEdge edge;
    edge.id = "edge1";
    edge.source_id = "node1";
    edge.target_id = "node2";
    
    // Test all cardinality types
    edge.source_cardinality = Cardinality::One;
    edge.target_cardinality = Cardinality::ZeroOrOne;
    model_->AddEdge(edge);
    
    edge.id = "edge2";
    edge.source_cardinality = Cardinality::OneOrMany;
    edge.target_cardinality = Cardinality::ZeroOrMany;
    model_->AddEdge(edge);
    
    auto& edges = model_->edges();
    EXPECT_EQ(edges.size(), 2);
}

TEST_F(DiagramModelTest, NodePositionAndSize) {
    DiagramNode node;
    node.x = 50.5;
    node.y = 100.25;
    node.width = 200.0;
    node.height = 150.0;
    
    auto& added = model_->AddNode(node);
    
    EXPECT_EQ(added.x, 50.5);
    EXPECT_EQ(added.y, 100.25);
    EXPECT_EQ(added.width, 200.0);
    EXPECT_EQ(added.height, 150.0);
}

TEST_F(DiagramModelTest, NodeFlags) {
    DiagramNode node;
    node.ghosted = true;
    node.pinned = true;
    
    auto& added = model_->AddNode(node);
    
    EXPECT_TRUE(added.ghosted);
    EXPECT_TRUE(added.pinned);
}

TEST_F(DiagramModelTest, EdgeFlags) {
    DiagramEdge edge;
    edge.id = "edge1";
    edge.source_id = "node1";
    edge.target_id = "node2";
    edge.directed = false;
    edge.identifying = true;
    
    model_->AddEdge(edge);
    
    auto& edges = model_->edges();
    EXPECT_FALSE(edges[0].directed);
    EXPECT_TRUE(edges[0].identifying);
}

TEST_F(DiagramModelTest, ErdNotationToString) {
    EXPECT_EQ(ErdNotationToString(ErdNotation::CrowsFoot), "crowsfoot");
    EXPECT_EQ(ErdNotationToString(ErdNotation::IDEF1X), "idef1x");
    EXPECT_EQ(ErdNotationToString(ErdNotation::UML), "uml");
    EXPECT_EQ(ErdNotationToString(ErdNotation::Chen), "chen");
}

TEST_F(DiagramModelTest, StringToErdNotation) {
    EXPECT_EQ(StringToErdNotation("crowsfoot"), ErdNotation::CrowsFoot);
    EXPECT_EQ(StringToErdNotation("idef1x"), ErdNotation::IDEF1X);
    EXPECT_EQ(StringToErdNotation("uml"), ErdNotation::UML);
    EXPECT_EQ(StringToErdNotation("chen"), ErdNotation::Chen);
    // Default case
    EXPECT_EQ(StringToErdNotation("unknown"), ErdNotation::CrowsFoot);
}

TEST_F(DiagramModelTest, ErdNotationLabel) {
    EXPECT_FALSE(ErdNotationLabel(ErdNotation::CrowsFoot).empty());
    EXPECT_FALSE(ErdNotationLabel(ErdNotation::IDEF1X).empty());
    EXPECT_FALSE(ErdNotationLabel(ErdNotation::UML).empty());
    EXPECT_FALSE(ErdNotationLabel(ErdNotation::Chen).empty());
}

TEST_F(DiagramModelTest, CardinalityLabel) {
    EXPECT_FALSE(CardinalityLabel(Cardinality::One).empty());
    EXPECT_FALSE(CardinalityLabel(Cardinality::ZeroOrOne).empty());
    EXPECT_FALSE(CardinalityLabel(Cardinality::OneOrMany).empty());
    EXPECT_FALSE(CardinalityLabel(Cardinality::ZeroOrMany).empty());
}

TEST_F(DiagramModelTest, DiagramTypeLabel) {
    EXPECT_FALSE(DiagramTypeLabel(DiagramType::Erd).empty());
    EXPECT_FALSE(DiagramTypeLabel(DiagramType::Silverston).empty());
}

TEST_F(DiagramModelTest, DiagramTypeKey) {
    EXPECT_FALSE(DiagramTypeKey(DiagramType::Erd).empty());
    EXPECT_FALSE(DiagramTypeKey(DiagramType::Silverston).empty());
}
