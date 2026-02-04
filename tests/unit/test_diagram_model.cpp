/**
 * @file test_diagram_model.cpp
 * @brief Unit tests for the diagram model
 */

#include <gtest/gtest.h>
#include "diagram/diagram_model.h"

using namespace scratchrobin;

class DiagramModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_ = std::make_unique<DiagramModel>();
    }
    
    std::unique_ptr<DiagramModel> model_;
};

TEST_F(DiagramModelTest, CreateEmptyDiagram) {
    EXPECT_TRUE(model_->GetEntities().empty());
    EXPECT_TRUE(model_->GetRelationships().empty());
}

TEST_F(DiagramModelTest, AddEntity) {
    Entity entity;
    entity.id = "entity1";
    entity.name = "Users";
    entity.x = 100;
    entity.y = 200;
    entity.width = 150;
    entity.height = 200;
    
    model_->AddEntity(entity);
    
    auto entities = model_->GetEntities();
    ASSERT_EQ(entities.size(), 1);
    EXPECT_EQ(entities[0].name, "Users");
    EXPECT_EQ(entities[0].x, 100);
}

TEST_F(DiagramModelTest, RemoveEntity) {
    Entity entity;
    entity.id = "entity1";
    model_->AddEntity(entity);
    
    model_->RemoveEntity("entity1");
    
    EXPECT_TRUE(model_->GetEntities().empty());
}

TEST_F(DiagramModelTest, UpdateEntityPosition) {
    Entity entity;
    entity.id = "entity1";
    entity.x = 0;
    entity.y = 0;
    model_->AddEntity(entity);
    
    model_->UpdateEntityPosition("entity1", 500, 300);
    
    auto e = model_->GetEntity("entity1");
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->x, 500);
    EXPECT_EQ(e->y, 300);
}

TEST_F(DiagramModelTest, AddEntityAttribute) {
    Entity entity;
    entity.id = "entity1";
    entity.name = "Users";
    
    Attribute attr;
    attr.name = "id";
    attr.type = "INTEGER";
    attr.is_primary_key = true;
    attr.is_nullable = false;
    
    entity.attributes.push_back(attr);
    model_->AddEntity(entity);
    
    auto e = model_->GetEntity("entity1");
    ASSERT_TRUE(e.has_value());
    ASSERT_EQ(e->attributes.size(), 1);
    EXPECT_EQ(e->attributes[0].name, "id");
    EXPECT_TRUE(e->attributes[0].is_primary_key);
}

TEST_F(DiagramModelTest, AddRelationship) {
    // Create two entities
    Entity users;
    users.id = "users";
    users.name = "Users";
    model_->AddEntity(users);
    
    Entity orders;
    orders.id = "orders";
    orders.name = "Orders";
    model_->AddEntity(orders);
    
    // Create relationship
    Relationship rel;
    rel.id = "rel1";
    rel.source_entity_id = "users";
    rel.target_entity_id = "orders";
    rel.source_cardinality = Cardinality::One;
    rel.target_cardinality = Cardinality::Many;
    rel.identifying = false;
    
    model_->AddRelationship(rel);
    
    auto relationships = model_->GetRelationships();
    ASSERT_EQ(relationships.size(), 1);
    EXPECT_EQ(relationships[0].source_entity_id, "users");
    EXPECT_EQ(relationships[0].target_cardinality, Cardinality::Many);
}

TEST_F(DiagramModelTest, RemoveRelationship) {
    Relationship rel;
    rel.id = "rel1";
    rel.source_entity_id = "entity1";
    rel.target_entity_id = "entity2";
    model_->AddRelationship(rel);
    
    model_->RemoveRelationship("rel1");
    
    EXPECT_TRUE(model_->GetRelationships().empty());
}

TEST_F(DiagramModelTest, RelationshipCascadingDelete) {
    Entity users;
    users.id = "users";
    model_->AddEntity(users);
    
    Entity orders;
    orders.id = "orders";
    model_->AddEntity(orders);
    
    Relationship rel;
    rel.id = "rel1";
    rel.source_entity_id = "users";
    rel.target_entity_id = "orders";
    model_->AddRelationship(rel);
    
    // Delete users entity - relationship should be removed too
    model_->RemoveEntity("users");
    
    EXPECT_TRUE(model_->GetRelationships().empty());
}

TEST_F(DiagramModelTest, SetNotation) {
    model_->SetNotation(ErdNotation::CrowsFoot);
    EXPECT_EQ(model_->GetNotation(), ErdNotation::CrowsFoot);
    
    model_->SetNotation(ErdNotation::IDEF1X);
    EXPECT_EQ(model_->GetNotation(), ErdNotation::IDEF1X);
}

TEST_F(DiagramModelTest, SerializeToJson) {
    Entity entity;
    entity.id = "users";
    entity.name = "Users";
    entity.x = 100;
    entity.y = 200;
    
    Attribute attr;
    attr.name = "id";
    attr.type = "INTEGER";
    attr.is_primary_key = true;
    entity.attributes.push_back(attr);
    
    model_->AddEntity(entity);
    model_->SetNotation(ErdNotation::CrowsFoot);
    
    std::string json = model_->ToJson();
    
    EXPECT_NE(json.find("\"name\":\"Users\""), std::string::npos);
    EXPECT_NE(json.find("\"notation\":\"crowsfoot\""), std::string::npos);
    EXPECT_NE(json.find("\"id\":\"INTEGER\""), std::string::npos);
}

TEST_F(DiagramModelTest, DeserializeFromJson) {
    std::string json = R"({
        "version": "1.0",
        "notation": "crowsfoot",
        "entities": [
            {
                "id": "orders",
                "name": "Orders",
                "x": 300,
                "y": 400,
                "attributes": [
                    {"name": "id", "type": "INTEGER", "is_primary_key": true}
                ]
            }
        ],
        "relationships": []
    })";
    
    auto result = DiagramModel::FromJson(json);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->GetNotation(), ErdNotation::CrowsFoot);
    
    auto entities = result->GetEntities();
    ASSERT_EQ(entities.size(), 1);
    EXPECT_EQ(entities[0].name, "Orders");
    EXPECT_EQ(entities[0].x, 300);
}

TEST_F(DiagramModelTest, InvalidJsonReturnsNullopt) {
    std::string invalid = "{invalid json";
    auto result = DiagramModel::FromJson(invalid);
    EXPECT_FALSE(result.has_value());
}

TEST_F(DiagramModelTest, GetEntityBounds) {
    Entity e1;
    e1.id = "e1";
    e1.x = 0;
    e1.y = 0;
    e1.width = 100;
    e1.height = 150;
    model_->AddEntity(e1);
    
    Entity e2;
    e2.id = "e2";
    e2.x = 200;
    e2.y = 300;
    e2.width = 100;
    e2.height = 150;
    model_->AddEntity(e2);
    
    auto bounds = model_->GetBounds();
    EXPECT_EQ(bounds.x, 0);
    EXPECT_EQ(bounds.y, 0);
    EXPECT_EQ(bounds.width, 300);
    EXPECT_EQ(bounds.height, 450);
}

TEST_F(DiagramModelTest, ClearDiagram) {
    Entity entity;
    entity.id = "e1";
    model_->AddEntity(entity);
    
    Relationship rel;
    rel.id = "r1";
    model_->AddRelationship(rel);
    
    model_->Clear();
    
    EXPECT_TRUE(model_->GetEntities().empty());
    EXPECT_TRUE(model_->GetRelationships().empty());
}

TEST_F(DiagramModelTest, EntityNotFound) {
    auto entity = model_->GetEntity("nonexistent");
    EXPECT_FALSE(entity.has_value());
}

TEST_F(DiagramModelTest, UpdateEntitySize) {
    Entity entity;
    entity.id = "e1";
    entity.width = 100;
    entity.height = 100;
    model_->AddEntity(entity);
    
    model_->UpdateEntitySize("e1", 200, 250);
    
    auto e = model_->GetEntity("e1");
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->width, 200);
    EXPECT_EQ(e->height, 250);
}

TEST_F(DiagramModelTest, DiagramVersion) {
    EXPECT_EQ(model_->GetVersion(), "1.0");
}

TEST_F(DiagramModelTest, SetDiagramName) {
    model_->SetName("My ERD Diagram");
    EXPECT_EQ(model_->GetName(), "My ERD Diagram");
}

TEST_F(DiagramModelTest, ComplexDiagramRoundTrip) {
    // Create a more complex diagram
    Entity users;
    users.id = "users";
    users.name = "Users";
    users.x = 100;
    users.y = 100;
    
    Attribute userId;
    userId.name = "user_id";
    userId.type = "INTEGER";
    userId.is_primary_key = true;
    users.attributes.push_back(userId);
    
    Attribute userName;
    userName.name = "username";
    userName.type = "VARCHAR(50)";
    userName.is_nullable = false;
    users.attributes.push_back(userName);
    
    model_->AddEntity(users);
    
    Entity orders;
    orders.id = "orders";
    orders.name = "Orders";
    orders.x = 400;
    orders.y = 100;
    
    Attribute orderId;
    orderId.name = "order_id";
    orderId.type = "INTEGER";
    orderId.is_primary_key = true;
    orders.attributes.push_back(orderId);
    
    Attribute userIdFk;
    userIdFk.name = "user_id";
    userIdFk.type = "INTEGER";
    userIdFk.is_foreign_key = true;
    orders.attributes.push_back(userIdFk);
    
    model_->AddEntity(orders);
    
    Relationship rel;
    rel.id = "users_orders";
    rel.source_entity_id = "users";
    rel.target_entity_id = "orders";
    rel.source_cardinality = Cardinality::One;
    rel.target_cardinality = Cardinality::Many;
    rel.source_role = "places";
    rel.target_role = "placed_by";
    model_->AddRelationship(rel);
    
    // Serialize and deserialize
    std::string json = model_->ToJson();
    auto restored = DiagramModel::FromJson(json);
    
    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->GetEntities().size(), 2);
    EXPECT_EQ(restored->GetRelationships().size(), 1);
    
    auto users_entity = restored->GetEntity("users");
    ASSERT_TRUE(users_entity.has_value());
    EXPECT_EQ(users_entity->attributes.size(), 2);
}
