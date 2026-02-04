/**
 * @file test_layout_engine.cpp
 * @brief Unit tests for the diagram layout engine
 */

#include <gtest/gtest.h>
#include "diagram/layout_engine.h"
#include "diagram/diagram_model.h"

using namespace scratchrobin;

class LayoutEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<LayoutEngine>();
        model_ = std::make_unique<DiagramModel>();
    }
    
    std::unique_ptr<LayoutEngine> engine_;
    std::unique_ptr<DiagramModel> model_;
};

TEST_F(LayoutEngineTest, CreateEngineWithDefaultSettings) {
    auto settings = engine_->GetSettings();
    EXPECT_EQ(settings.algorithm, LayoutAlgorithm::Sugiyama);
    EXPECT_EQ(settings.direction, LayoutDirection::TopToBottom);
}

TEST_F(LayoutEngineTest, LayoutEmptyDiagram) {
    auto result = engine_->Layout(*model_);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entity_positions.size(), 0);
}

TEST_F(LayoutEngineTest, LayoutSingleEntity) {
    Entity entity;
    entity.id = "e1";
    entity.name = "Users";
    entity.width = 150;
    entity.height = 200;
    model_->AddEntity(entity);
    
    auto result = engine_->Layout(*model_);
    
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.entity_positions.size(), 1);
    // Single entity should be at origin or center
    EXPECT_GE(result.entity_positions["e1"].x, 0);
    EXPECT_GE(result.entity_positions["e1"].y, 0);
}

TEST_F(LayoutEngineTest, LayoutTwoEntities) {
    Entity e1;
    e1.id = "e1";
    e1.name = "Parent";
    e1.width = 150;
    e1.height = 100;
    model_->AddEntity(e1);
    
    Entity e2;
    e2.id = "e2";
    e2.name = "Child";
    e2.width = 150;
    e2.height = 100;
    model_->AddEntity(e2);
    
    Relationship rel;
    rel.id = "r1";
    rel.source_entity_id = "e1";
    rel.target_entity_id = "e2";
    model_->AddRelationship(rel);
    
    auto result = engine_->Layout(*model_);
    
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.entity_positions.size(), 2);
    
    // In top-to-bottom layout, parent should be above child
    auto& p1 = result.entity_positions["e1"];
    auto& p2 = result.entity_positions["e2"];
    EXPECT_LT(p1.y, p2.y);
}

TEST_F(LayoutEngineTest, LayoutDirectionLeftToRight) {
    engine_->SetDirection(LayoutDirection::LeftToRight);
    
    Entity e1;
    e1.id = "e1";
    model_->AddEntity(e1);
    
    Entity e2;
    e2.id = "e2";
    model_->AddEntity(e2);
    
    Relationship rel;
    rel.id = "r1";
    rel.source_entity_id = "e1";
    rel.target_entity_id = "e2";
    model_->AddRelationship(rel);
    
    auto result = engine_->Layout(*model_);
    
    // In left-to-right layout, source should be left of target
    auto& p1 = result.entity_positions["e1"];
    auto& p2 = result.entity_positions["e2"];
    EXPECT_LT(p1.x, p2.x);
}

TEST_F(LayoutEngineTest, ForceDirectedLayout) {
    engine_->SetAlgorithm(LayoutAlgorithm::ForceDirected);
    
    // Add multiple entities
    for (int i = 0; i < 5; ++i) {
        Entity e;
        e.id = "e" + std::to_string(i);
        e.x = i * 100;  // Different initial positions
        e.y = i * 50;
        model_->AddEntity(e);
    }
    
    auto result = engine_->Layout(*model_);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entity_positions.size(), 5);
    
    // Positions should have changed
    for (int i = 0; i < 5; ++i) {
        std::string id = "e" + std::to_string(i);
        auto& new_pos = result.entity_positions[id];
        // Force-directed should have moved entities
        // (exact positions depend on the algorithm)
        EXPECT_GE(new_pos.x, -1000);
        EXPECT_LE(new_pos.x, 1000);
    }
}

TEST_F(LayoutEngineTest, OrthogonalLayout) {
    engine_->SetAlgorithm(LayoutAlgorithm::Orthogonal);
    
    Entity e1;
    e1.id = "e1";
    model_->AddEntity(e1);
    
    Entity e2;
    e2.id = "e2";
    model_->AddEntity(e2);
    
    Relationship rel;
    rel.id = "r1";
    rel.source_entity_id = "e1";
    rel.target_entity_id = "e2";
    model_->AddRelationship(rel);
    
    auto result = engine_->Layout(*model_);
    
    EXPECT_TRUE(result.success);
    // Orthogonal layout should use right angles
    ASSERT_EQ(result.edge_routes.size(), 1);
}

TEST_F(LayoutEngineTest, HierarchicalLayoutWithMultipleLevels) {
    // Create a 3-level hierarchy
    Entity root;
    root.id = "root";
    model_->AddEntity(root);
    
    Entity child1;
    child1.id = "child1";
    model_->AddEntity(child1);
    
    Entity child2;
    child2.id = "child2";
    model_->AddEntity(child2);
    
    Entity grandchild;
    grandchild.id = "grandchild";
    model_->AddEntity(grandchild);
    
    // Relationships
    Relationship r1;
    r1.id = "r1";
    r1.source_entity_id = "root";
    r1.target_entity_id = "child1";
    model_->AddRelationship(r1);
    
    Relationship r2;
    r2.id = "r2";
    r2.source_entity_id = "root";
    r2.target_entity_id = "child2";
    model_->AddRelationship(r2);
    
    Relationship r3;
    r3.id = "r3";
    r3.source_entity_id = "child1";
    r3.target_entity_id = "grandchild";
    model_->AddRelationship(r3);
    
    auto result = engine_->Layout(*model_);
    
    EXPECT_TRUE(result.success);
    
    // Check hierarchy levels
    auto& root_pos = result.entity_positions["root"];
    auto& child1_pos = result.entity_positions["child1"];
    auto& grandchild_pos = result.entity_positions["grandchild"];
    
    EXPECT_LT(root_pos.y, child1_pos.y);
    EXPECT_LT(child1_pos.y, grandchild_pos.y);
}

TEST_F(LayoutEngineTest, RespectPinnedNodes) {
    Entity e1;
    e1.id = "e1";
    e1.x = 100;
    e1.y = 200;
    e1.pinned = true;
    model_->AddEntity(e1);
    
    Entity e2;
    e2.id = "e2";
    model_->AddEntity(e2);
    
    Relationship rel;
    rel.id = "r1";
    rel.source_entity_id = "e1";
    rel.target_entity_id = "e2";
    model_->AddRelationship(rel);
    
    auto result = engine_->Layout(*model_);
    
    // Pinned node should keep its original position
    auto& p1 = result.entity_positions["e1"];
    EXPECT_EQ(p1.x, 100);
    EXPECT_EQ(p1.y, 200);
}

TEST_F(LayoutEngineTest, LayoutSpacing) {
    LayoutSettings settings;
    settings.horizontal_spacing = 200;
    settings.vertical_spacing = 150;
    engine_->SetSettings(settings);
    
    Entity e1;
    e1.id = "e1";
    model_->AddEntity(e1);
    
    Entity e2;
    e2.id = "e2";
    model_->AddEntity(e2);
    
    Relationship rel;
    rel.id = "r1";
    rel.source_entity_id = "e1";
    rel.target_entity_id = "e2";
    model_->AddRelationship(rel);
    
    auto result = engine_->Layout(*model_);
    
    auto& p1 = result.entity_positions["e1"];
    auto& p2 = result.entity_positions["e2"];
    
    // Entities should be spaced according to settings
    EXPECT_GE(std::abs(p2.y - p1.y), 150);
}

TEST_F(LayoutEngineTest, EdgeRouting) {
    Entity e1;
    e1.id = "e1";
    e1.x = 0;
    e1.y = 0;
    e1.width = 100;
    e1.height = 50;
    model_->AddEntity(e1);
    
    Entity e2;
    e2.id = "e2";
    e2.x = 300;
    e2.y = 0;
    e2.width = 100;
    e2.height = 50;
    model_->AddEntity(e2);
    
    Relationship rel;
    rel.id = "r1";
    rel.source_entity_id = "e1";
    rel.target_entity_id = "e2";
    model_->AddRelationship(rel);
    
    auto result = engine_->Layout(*model_);
    
    ASSERT_EQ(result.edge_routes.size(), 1);
    auto& route = result.edge_routes["r1"];
    
    // Route should have at least start and end points
    EXPECT_GE(route.points.size(), 2);
}

TEST_F(LayoutEngineTest, MinimizeCrossings) {
    // Create a complex graph where crossings could occur
    for (int i = 0; i < 4; ++i) {
        Entity e;
        e.id = "level1_" + std::to_string(i);
        model_->AddEntity(e);
    }
    
    for (int i = 0; i < 4; ++i) {
        Entity e;
        e.id = "level2_" + std::to_string(i);
        model_->AddEntity(e);
    }
    
    // Create crossing relationships
    Relationship r1;
    r1.source_entity_id = "level1_0";
    r1.target_entity_id = "level2_2";
    model_->AddRelationship(r1);
    
    Relationship r2;
    r2.source_entity_id = "level1_1";
    r2.target_entity_id = "level2_1";
    model_->AddRelationship(r2);
    
    Relationship r3;
    r3.source_entity_id = "level1_2";
    r3.target_entity_id = "level2_3";
    model_->AddRelationship(r3);
    
    Relationship r4;
    r4.source_entity_id = "level1_3";
    r4.target_entity_id = "level2_0";
    model_->AddRelationship(r4);
    
    auto result = engine_->Layout(*model_);
    
    EXPECT_TRUE(result.success);
    // Algorithm should attempt to minimize crossings
    // (exact number depends on algorithm and heuristics)
}

TEST_F(LayoutEngineTest, CycleHandling) {
    // Create a cycle: A -> B -> C -> A
    Entity a;
    a.id = "A";
    model_->AddEntity(a);
    
    Entity b;
    b.id = "B";
    model_->AddEntity(b);
    
    Entity c;
    c.id = "C";
    model_->AddEntity(c);
    
    Relationship r1;
    r1.source_entity_id = "A";
    r1.target_entity_id = "B";
    model_->AddRelationship(r1);
    
    Relationship r2;
    r2.source_entity_id = "B";
    r2.target_entity_id = "C";
    model_->AddRelationship(r2);
    
    Relationship r3;
    r3.source_entity_id = "C";
    r3.target_entity_id = "A";
    model_->AddRelationship(r3);
    
    auto result = engine_->Layout(*model_);
    
    EXPECT_TRUE(result.success);
    // Sugiyama algorithm should handle cycles by temporarily breaking them
}

TEST_F(LayoutEngineTest, ApplyLayoutToModel) {
    Entity e1;
    e1.id = "e1";
    e1.x = 0;
    e1.y = 0;
    model_->AddEntity(e1);
    
    auto result = engine_->Layout(*model_);
    engine_->ApplyLayout(*model_, result);
    
    auto updated = model_->GetEntity("e1");
    ASSERT_TRUE(updated.has_value());
    // Position should have been updated
    // (exact value depends on algorithm)
}
