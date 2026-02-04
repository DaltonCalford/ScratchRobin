/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include <gtest/gtest.h>

#include <chrono>
#include <random>

#include "diagram/diagram_document.h"
#include "diagram/layout_engine.h"

namespace scratchrobin {

using namespace std::chrono;

class Timer {
public:
    void Start() {
        start_ = high_resolution_clock::now();
    }
    
    void Stop() {
        end_ = high_resolution_clock::now();
    }
    
    double ElapsedMs() const {
        return duration_cast<microseconds>(end_ - start_).count() / 1000.0;
    }
    
private:
    high_resolution_clock::time_point start_;
    high_resolution_clock::time_point end_;
};

class DiagramPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_unique<DiagramDocument>();
    }
    
    void GenerateEntities(int count) {
        std::mt19937 rng(42);  // Fixed seed for reproducibility
        std::uniform_real_distribution<double> pos_dist(0, 1000);
        
        for (int i = 0; i < count; ++i) {
            Entity entity;
            entity.id = "entity_" + std::to_string(i);
            entity.name = "Table_" + std::to_string(i);
            entity.position = Point2D(pos_dist(rng), pos_dist(rng));
            entity.size = Size2D(150, 80 + (i % 5) * 20);
            
            // Add attributes
            int attr_count = 3 + (i % 7);
            for (int j = 0; j < attr_count; ++j) {
                EntityAttribute attr;
                attr.name = "attr_" + std::to_string(j);
                attr.type = (j % 3 == 0) ? "INTEGER" : ((j % 3 == 1) ? "VARCHAR" : "TIMESTAMP");
                attr.isPrimaryKey = (j == 0);
                attr.isForeignKey = (j == 1);
                entity.attributes.push_back(attr);
            }
            
            doc_->AddEntity(entity);
        }
    }
    
    void GenerateRelationships(int count) {
        std::mt19937 rng(42);
        auto entities = doc_->GetEntities();
        
        if (entities.size() < 2) return;
        
        std::uniform_int_distribution<int> entity_dist(0, entities.size() - 1);
        
        for (int i = 0; i < count; ++i) {
            Relationship rel;
            rel.id = "rel_" + std::to_string(i);
            
            int from_idx = entity_dist(rng);
            int to_idx = entity_dist(rng);
            while (to_idx == from_idx) {
                to_idx = entity_dist(rng);
            }
            
            rel.fromEntity = entities[from_idx].id;
            rel.toEntity = entities[to_idx].id;
            rel.name = "rel_" + std::to_string(i);
            rel.cardinalityFrom = Relationship::Cardinality::ONE;
            rel.cardinalityTo = Relationship::Cardinality::MANY;
            
            doc_->AddRelationship(rel);
        }
    }
    
    std::unique_ptr<DiagramDocument> doc_;
};

// Performance thresholds (milliseconds)
constexpr double kMaxLoad50Entities = 1000.0;
constexpr double kMaxLoad200Entities = 3000.0;
constexpr double kMaxLoad500Entities = 10000.0;
constexpr double kMaxLayout50Entities = 2000.0;
constexpr double kMaxLayout200Entities = 5000.0;
constexpr double kMaxSerialize500Entities = 3000.0;

TEST_F(DiagramPerformanceTest, Load50Entities) {
    GenerateEntities(50);
    GenerateRelationships(25);
    
    Timer timer;
    timer.Start();
    
    // Serialize and reload
    std::string xml = doc_->ToXml();
    auto doc2 = std::make_unique<DiagramDocument>();
    doc2->FromXml(xml);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Load 50 entities: " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLoad50Entities);
    EXPECT_EQ(doc2->GetEntities().size(), 50);
}

TEST_F(DiagramPerformanceTest, Load200Entities) {
    GenerateEntities(200);
    GenerateRelationships(100);
    
    Timer timer;
    timer.Start();
    
    std::string xml = doc_->ToXml();
    auto doc2 = std::make_unique<DiagramDocument>();
    doc2->FromXml(xml);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Load 200 entities: " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLoad200Entities);
    EXPECT_EQ(doc2->GetEntities().size(), 200);
}

TEST_F(DiagramPerformanceTest, Load500Entities) {
    GenerateEntities(500);
    GenerateRelationships(250);
    
    Timer timer;
    timer.Start();
    
    std::string xml = doc_->ToXml();
    auto doc2 = std::make_unique<DiagramDocument>();
    doc2->FromXml(xml);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Load 500 entities: " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLoad500Entities);
    EXPECT_EQ(doc2->GetEntities().size(), 500);
}

TEST_F(DiagramPerformanceTest, Layout50EntitiesSugiyama) {
    GenerateEntities(50);
    GenerateRelationships(25);
    
    LayoutEngine engine;
    LayoutOptions options;
    options.algorithm = LayoutAlgorithm::SUGIYAMA;
    
    Timer timer;
    timer.Start();
    
    engine.Layout(doc_.get(), options);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Layout 50 entities (Sugiyama): " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLayout50Entities);
}

TEST_F(DiagramPerformanceTest, Layout50EntitiesForceDirected) {
    GenerateEntities(50);
    GenerateRelationships(25);
    
    LayoutEngine engine;
    LayoutOptions options;
    options.algorithm = LayoutAlgorithm::FORCE_DIRECTED;
    options.iterations = 100;
    
    Timer timer;
    timer.Start();
    
    engine.Layout(doc_.get(), options);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Layout 50 entities (Force-Directed): " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLayout50Entities * 1.5);  // Allow more time for FD
}

TEST_F(DiagramPerformanceTest, Layout200EntitiesSugiyama) {
    GenerateEntities(200);
    GenerateRelationships(100);
    
    LayoutEngine engine;
    LayoutOptions options;
    options.algorithm = LayoutAlgorithm::SUGIYAMA;
    
    Timer timer;
    timer.Start();
    
    engine.Layout(doc_.get(), options);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Layout 200 entities (Sugiyama): " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLayout200Entities);
}

TEST_F(DiagramPerformanceTest, Layout200EntitiesOrthogonal) {
    GenerateEntities(200);
    GenerateRelationships(100);
    
    LayoutEngine engine;
    LayoutOptions options;
    options.algorithm = LayoutAlgorithm::ORTHOGONAL;
    
    Timer timer;
    timer.Start();
    
    engine.Layout(doc_.get(), options);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Layout 200 entities (Orthogonal): " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLayout200Entities * 1.2);
}

TEST_F(DiagramPerformanceTest, Serialize500Entities) {
    GenerateEntities(500);
    GenerateRelationships(250);
    
    Timer timer;
    timer.Start();
    
    std::string xml = doc_->ToXml();
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Serialize 500 entities: " << elapsed << " ms (" 
              << (xml.size() / 1024) << " KB)" << std::endl;
    
    EXPECT_LT(elapsed, kMaxSerialize500Entities);
    EXPECT_GT(xml.size(), 10000);  // Should be substantial
}

TEST_F(DiagramPerformanceTest, Deserialize500Entities) {
    GenerateEntities(500);
    GenerateRelationships(250);
    
    std::string xml = doc_->ToXml();
    
    Timer timer;
    timer.Start();
    
    auto doc2 = std::make_unique<DiagramDocument>();
    doc2->FromXml(xml);
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Deserialize 500 entities: " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, kMaxLoad500Entities);
    EXPECT_EQ(doc2->GetEntities().size(), 500);
}

TEST_F(DiagramPerformanceTest, EntityLookupPerformance) {
    GenerateEntities(500);
    
    // Test ID lookup performance
    Timer timer;
    timer.Start();
    
    for (int i = 0; i < 1000; ++i) {
        std::string id = "entity_" + std::to_string(i % 500);
        auto entity = doc_->FindEntityById(id);
        EXPECT_NE(entity, nullptr);
    }
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "1000 entity lookups: " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, 100.0);  // Should be very fast
}

TEST_F(DiagramPerformanceTest, BulkEntityModification) {
    GenerateEntities(100);
    
    Timer timer;
    timer.Start();
    
    // Modify all entities
    for (auto& entity : doc_->GetEntities()) {
        entity.position.x += 10;
        entity.position.y += 10;
        entity.size.width += 5;
    }
    
    // Add attributes to all
    for (auto& entity : doc_->GetEntities()) {
        EntityAttribute attr;
        attr.name = "new_attr";
        attr.type = "BOOLEAN";
        entity.attributes.push_back(attr);
    }
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "Bulk modify 100 entities: " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, 100.0);
}

TEST_F(DiagramPerformanceTest, MemoryUsageLargeDiagram) {
    GenerateEntities(500);
    GenerateRelationships(250);
    
    // Get memory usage before serialization
    std::string xml = doc_->ToXml();
    
    // XML should be reasonable size
    EXPECT_LT(xml.size(), 10 * 1024 * 1024);  // Less than 10 MB
    
    // Parse it back
    auto doc2 = std::make_unique<DiagramDocument>();
    doc2->FromXml(xml);
    
    // Both documents should have same entities
    EXPECT_EQ(doc_->GetEntities().size(), doc2->GetEntities().size());
    EXPECT_EQ(doc_->GetRelationships().size(), doc2->GetRelationships().size());
}

// Stress test - many small operations
TEST_F(DiagramPerformanceTest, RapidSmallOperations) {
    GenerateEntities(50);
    
    Timer timer;
    timer.Start();
    
    // Perform many small operations
    for (int i = 0; i < 1000; ++i) {
        // Add and remove entity
        Entity entity;
        entity.id = "temp_" + std::to_string(i);
        entity.name = "Temp";
        entity.position = Point2D(i, i);
        
        doc_->AddEntity(entity);
        doc_->RemoveEntity(entity.id);
    }
    
    timer.Stop();
    
    double elapsed = timer.ElapsedMs();
    std::cout << "1000 add/remove operations: " << elapsed << " ms" << std::endl;
    
    EXPECT_LT(elapsed, 500.0);  // Should be fast
    EXPECT_EQ(doc_->GetEntities().size(), 50);  // Original count restored
}

} // namespace scratchrobin
