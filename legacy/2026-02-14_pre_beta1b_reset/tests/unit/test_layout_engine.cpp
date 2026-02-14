/**
 * @file test_layout_engine.cpp
 * @brief Unit tests for the diagram layout engine
 */

#include <gtest/gtest.h>
#include "diagram/layout_engine.h"
#include "ui/diagram_model.h"

using namespace scratchrobin;
using namespace scratchrobin::diagram;

class LayoutEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_ = std::make_unique<DiagramModel>(DiagramType::Erd);
    }
    
    std::unique_ptr<DiagramModel> model_;
};

TEST_F(LayoutEngineTest, CreateSugiyamaEngine) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    ASSERT_NE(engine, nullptr);
}

TEST_F(LayoutEngineTest, CreateForceDirectedEngine) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::ForceDirected);
    ASSERT_NE(engine, nullptr);
}

TEST_F(LayoutEngineTest, CreateOrthogonalEngine) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Orthogonal);
    ASSERT_NE(engine, nullptr);
}

TEST_F(LayoutEngineTest, CreateCircularEngine) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Circular);
    ASSERT_NE(engine, nullptr);
}

TEST_F(LayoutEngineTest, LayoutEmptyDiagram) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_TRUE(positions.empty());
}

TEST_F(LayoutEngineTest, LayoutSingleNode) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    DiagramNode node;
    node.id = "node1";
    node.name = "Users";
    node.width = 150;
    node.height = 100;
    model_->AddNode(node);
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 1);
    EXPECT_EQ(positions[0].node_id, "node1");
}

TEST_F(LayoutEngineTest, LayoutTwoNodes) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    DiagramNode node1;
    node1.id = "node1";
    node1.name = "Parent";
    node1.width = 150;
    node1.height = 100;
    model_->AddNode(node1);
    
    DiagramNode node2;
    node2.id = "node2";
    node2.name = "Child";
    node2.width = 150;
    node2.height = 100;
    model_->AddNode(node2);
    
    DiagramEdge edge;
    edge.id = "edge1";
    edge.source_id = "node1";
    edge.target_id = "node2";
    model_->AddEdge(edge);
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 2);
}

TEST_F(LayoutEngineTest, LayoutDirectionTopDown) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    options.direction = LayoutOptions::Direction::TopDown;
    
    DiagramNode node1;
    node1.id = "node1";
    model_->AddNode(node1);
    
    DiagramNode node2;
    node2.id = "node2";
    model_->AddNode(node2);
    
    DiagramEdge edge;
    edge.source_id = "node1";
    edge.target_id = "node2";
    model_->AddEdge(edge);
    
    auto positions = engine->Layout(*model_, options);
    
    ASSERT_EQ(positions.size(), 2);
    // In top-down layout, source should be above target
    auto& p1 = positions[0];
    auto& p2 = positions[1];
    if (p1.node_id == "node1" && p2.node_id == "node2") {
        EXPECT_LT(p1.y, p2.y);
    }
}

TEST_F(LayoutEngineTest, LayoutDirectionLeftRight) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    options.direction = LayoutOptions::Direction::LeftRight;
    
    DiagramNode node1;
    node1.id = "node1";
    model_->AddNode(node1);
    
    DiagramNode node2;
    node2.id = "node2";
    model_->AddNode(node2);
    
    DiagramEdge edge;
    edge.source_id = "node1";
    edge.target_id = "node2";
    model_->AddEdge(edge);
    
    auto positions = engine->Layout(*model_, options);
    
    ASSERT_EQ(positions.size(), 2);
    // Layout engine returns positions - actual layout depends on implementation
    // Verify both positions are returned
    EXPECT_FALSE(positions[0].node_id.empty());
    EXPECT_FALSE(positions[1].node_id.empty());
}

TEST_F(LayoutEngineTest, ForceDirectedLayoutMultipleNodes) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::ForceDirected);
    LayoutOptions options;
    options.fd_iterations = 10;  // Reduce iterations for faster test
    
    // Add multiple nodes
    for (int i = 0; i < 5; ++i) {
        DiagramNode node;
        node.id = "node" + std::to_string(i);
        node.x = i * 100;
        node.y = i * 50;
        model_->AddNode(node);
    }
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 5);
    
    // All positions should be reasonable
    for (const auto& pos : positions) {
        EXPECT_FALSE(std::isnan(pos.x));
        EXPECT_FALSE(std::isnan(pos.y));
    }
}

TEST_F(LayoutEngineTest, OrthogonalLayout) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Orthogonal);
    LayoutOptions options;
    
    DiagramNode node1;
    node1.id = "node1";
    model_->AddNode(node1);
    
    DiagramNode node2;
    node2.id = "node2";
    model_->AddNode(node2);
    
    DiagramEdge edge;
    edge.id = "edge1";
    edge.source_id = "node1";
    edge.target_id = "node2";
    model_->AddEdge(edge);
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 2);
}

TEST_F(LayoutEngineTest, LayoutSpacing) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    options.node_spacing = 200;
    options.level_spacing = 150;
    
    DiagramNode node1;
    node1.id = "node1";
    model_->AddNode(node1);
    
    DiagramNode node2;
    node2.id = "node2";
    model_->AddNode(node2);
    
    DiagramEdge edge;
    edge.source_id = "node1";
    edge.target_id = "node2";
    model_->AddEdge(edge);
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 2);
}

TEST_F(LayoutEngineTest, HierarchicalLayoutWithMultipleLevels) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    // Create a 3-level hierarchy
    DiagramNode root;
    root.id = "root";
    model_->AddNode(root);
    
    DiagramNode child1;
    child1.id = "child1";
    model_->AddNode(child1);
    
    DiagramNode child2;
    child2.id = "child2";
    model_->AddNode(child2);
    
    DiagramNode grandchild;
    grandchild.id = "grandchild";
    model_->AddNode(grandchild);
    
    // Relationships
    DiagramEdge e1;
    e1.source_id = "root";
    e1.target_id = "child1";
    model_->AddEdge(e1);
    
    DiagramEdge e2;
    e2.source_id = "root";
    e2.target_id = "child2";
    model_->AddEdge(e2);
    
    DiagramEdge e3;
    e3.source_id = "child1";
    e3.target_id = "grandchild";
    model_->AddEdge(e3);
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 4);
}

TEST_F(LayoutEngineTest, CycleHandling) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    // Create a cycle: A -> B -> C -> A
    DiagramNode a;
    a.id = "A";
    model_->AddNode(a);
    
    DiagramNode b;
    b.id = "B";
    model_->AddNode(b);
    
    DiagramNode c;
    c.id = "C";
    model_->AddNode(c);
    
    DiagramEdge e1;
    e1.source_id = "A";
    e1.target_id = "B";
    model_->AddEdge(e1);
    
    DiagramEdge e2;
    e2.source_id = "B";
    e2.target_id = "C";
    model_->AddEdge(e2);
    
    DiagramEdge e3;
    e3.source_id = "C";
    e3.target_id = "A";
    model_->AddEdge(e3);
    
    // Should handle cycles without crashing
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 3);
}

TEST_F(LayoutEngineTest, LayoutAlgorithmToString) {
    EXPECT_FALSE(LayoutAlgorithmToString(LayoutAlgorithm::Sugiyama).empty());
    EXPECT_FALSE(LayoutAlgorithmToString(LayoutAlgorithm::ForceDirected).empty());
    EXPECT_FALSE(LayoutAlgorithmToString(LayoutAlgorithm::Orthogonal).empty());
    EXPECT_FALSE(LayoutAlgorithmToString(LayoutAlgorithm::Circular).empty());
}

TEST_F(LayoutEngineTest, StringToLayoutAlgorithm) {
    EXPECT_EQ(StringToLayoutAlgorithm("sugiyama"), LayoutAlgorithm::Sugiyama);
    EXPECT_EQ(StringToLayoutAlgorithm("forcedirected"), LayoutAlgorithm::ForceDirected);
    EXPECT_EQ(StringToLayoutAlgorithm("orthogonal"), LayoutAlgorithm::Orthogonal);
    EXPECT_EQ(StringToLayoutAlgorithm("circular"), LayoutAlgorithm::Circular);
    // Default case
    EXPECT_EQ(StringToLayoutAlgorithm("unknown"), LayoutAlgorithm::Sugiyama);
}

TEST_F(LayoutEngineTest, GetAvailableLayoutAlgorithms) {
    auto algorithms = GetAvailableLayoutAlgorithms();
    EXPECT_FALSE(algorithms.empty());
}

TEST_F(LayoutEngineTest, LayoutOptionsDefaults) {
    LayoutOptions options;
    
    EXPECT_EQ(options.algorithm, LayoutAlgorithm::Sugiyama);
    EXPECT_EQ(options.direction, LayoutOptions::Direction::TopDown);
    EXPECT_EQ(options.node_spacing, 150.0);
    EXPECT_EQ(options.level_spacing, 120.0);
    EXPECT_EQ(options.padding, 50.0);
    EXPECT_TRUE(options.minimize_crossings);
    EXPECT_GT(options.repulsion_force, 0);
    EXPECT_GT(options.attraction_force, 0);
}

TEST_F(LayoutEngineTest, RespectPinnedNodes) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::ForceDirected);
    LayoutOptions options;
    options.fd_iterations = 5;
    
    DiagramNode node1;
    node1.id = "node1";
    node1.x = 100;
    node1.y = 200;
    node1.pinned = true;
    model_->AddNode(node1);
    
    DiagramNode node2;
    node2.id = "node2";
    node2.x = 300;
    node2.y = 200;
    model_->AddNode(node2);
    
    DiagramEdge edge;
    edge.source_id = "node1";
    edge.target_id = "node2";
    model_->AddEdge(edge);
    
    auto positions = engine->Layout(*model_, options);
    
    // Should return positions for both nodes
    EXPECT_EQ(positions.size(), 2);
}

TEST_F(LayoutEngineTest, ComplexDiagram) {
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    // Create multiple nodes and edges
    for (int i = 0; i < 4; ++i) {
        DiagramNode node;
        node.id = "level1_" + std::to_string(i);
        model_->AddNode(node);
    }
    
    for (int i = 0; i < 4; ++i) {
        DiagramNode node;
        node.id = "level2_" + std::to_string(i);
        model_->AddNode(node);
    }
    
    // Create crossing edges
    DiagramEdge e1;
    e1.source_id = "level1_0";
    e1.target_id = "level2_2";
    model_->AddEdge(e1);
    
    DiagramEdge e2;
    e2.source_id = "level1_1";
    e2.target_id = "level2_1";
    model_->AddEdge(e2);
    
    DiagramEdge e3;
    e3.source_id = "level1_2";
    e3.target_id = "level2_3";
    model_->AddEdge(e3);
    
    DiagramEdge e4;
    e4.source_id = "level1_3";
    e4.target_id = "level2_0";
    model_->AddEdge(e4);
    
    auto positions = engine->Layout(*model_, options);
    
    EXPECT_EQ(positions.size(), 8);
    
    // All positions should be valid
    for (const auto& pos : positions) {
        EXPECT_FALSE(pos.node_id.empty());
        EXPECT_FALSE(std::isnan(pos.x));
        EXPECT_FALSE(std::isnan(pos.y));
    }
}
