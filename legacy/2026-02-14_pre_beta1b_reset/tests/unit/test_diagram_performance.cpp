/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include <gtest/gtest.h>

#include <chrono>
#include <random>
#include <cstdlib>

#include "ui/diagram_model.h"
#include "diagram/layout_engine.h"

namespace scratchrobin {

using namespace std::chrono;
using namespace diagram;

bool ShouldRunPerfTests() {
    return std::getenv("SCRATCHROBIN_RUN_PERF_TESTS") != nullptr;
}

double PerfMsFactor() {
    const char* value = std::getenv("SCRATCHROBIN_PERF_MS_FACTOR");
    if (!value || *value == '\0') {
        return 1.0;
    }
    char* end = nullptr;
    double factor = std::strtod(value, &end);
    if (end == value || factor <= 0.0) {
        return 1.0;
    }
    return factor;
}

double ScaledMs(double base_ms) {
    return base_ms * PerfMsFactor();
}

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
        model_ = std::make_unique<DiagramModel>(DiagramType::Erd);
    }
    
    void GenerateNodes(int count) {
        std::mt19937 rng(42);  // Fixed seed for reproducibility
        std::uniform_real_distribution<double> pos_dist(0, 1000);
        
        for (int i = 0; i < count; ++i) {
            DiagramNode node;
            node.id = "node_" + std::to_string(i);
            node.name = "Table_" + std::to_string(i);
            node.x = pos_dist(rng);
            node.y = pos_dist(rng);
            node.width = 150;
            node.height = 80 + (i % 5) * 20;
            
            // Add attributes
            int attr_count = 3 + (i % 7);
            for (int j = 0; j < attr_count; ++j) {
                DiagramAttribute attr;
                attr.name = "attr_" + std::to_string(j);
                attr.data_type = (j % 3 == 0) ? "INTEGER" : ((j % 3 == 1) ? "VARCHAR" : "TIMESTAMP");
                attr.is_primary = (j == 0);
                attr.is_nullable = (j > 0);
                node.attributes.push_back(attr);
            }
            
            model_->AddNode(node);
        }
    }
    
    void GenerateEdges(int count) {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> node_dist(0, static_cast<int>(model_->nodes().size()) - 1);
        
        for (int i = 0; i < count; ++i) {
            int source = node_dist(rng);
            int target = node_dist(rng);
            
            if (source != target) {
                DiagramEdge edge;
                edge.id = "edge_" + std::to_string(i);
                edge.source_id = model_->nodes()[source].id;
                edge.target_id = model_->nodes()[target].id;
                edge.label = "rel_" + std::to_string(i);
                model_->AddEdge(edge);
            }
        }
    }
    
    std::unique_ptr<DiagramModel> model_;
    Timer timer_;
};

TEST_F(DiagramPerformanceTest, SmallModel_Operations) {
    GenerateNodes(10);
    GenerateEdges(15);
    
    EXPECT_EQ(model_->nodes().size(), 10);
    EXPECT_GE(model_->edges().size(), 10);  // Some edges may be skipped if source == target
}

TEST_F(DiagramPerformanceTest, MediumModel_Operations) {
    GenerateNodes(50);
    GenerateEdges(75);
    
    EXPECT_EQ(model_->nodes().size(), 50);
    EXPECT_GE(model_->edges().size(), 50);  // Some may have been skipped
}

TEST_F(DiagramPerformanceTest, LargeModel_Operations) {
    GenerateNodes(200);
    GenerateEdges(300);
    
    EXPECT_EQ(model_->nodes().size(), 200);
    EXPECT_GE(model_->edges().size(), 200);
}

TEST_F(DiagramPerformanceTest, SugiyamaLayout_Small) {
    if (!ShouldRunPerfTests()) {
        GTEST_SKIP() << "Performance tests disabled. Set SCRATCHROBIN_RUN_PERF_TESTS=1 to enable.";
    }
    GenerateNodes(10);
    GenerateEdges(15);
    
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    timer_.Start();
    auto positions = engine->Layout(*model_, options);
    timer_.Stop();
    
    EXPECT_EQ(positions.size(), 10);
    EXPECT_LT(timer_.ElapsedMs(), ScaledMs(1000.0));  // Should complete within 1 second (scaled)
}

TEST_F(DiagramPerformanceTest, SugiyamaLayout_Medium) {
    if (!ShouldRunPerfTests()) {
        GTEST_SKIP() << "Performance tests disabled. Set SCRATCHROBIN_RUN_PERF_TESTS=1 to enable.";
    }
    GenerateNodes(50);
    GenerateEdges(75);
    
    auto engine = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
    LayoutOptions options;
    
    timer_.Start();
    auto positions = engine->Layout(*model_, options);
    timer_.Stop();
    
    EXPECT_EQ(positions.size(), 50);
    EXPECT_LT(timer_.ElapsedMs(), ScaledMs(5000.0));  // Should complete within 5 seconds (scaled)
}

TEST_F(DiagramPerformanceTest, ForceDirectedLayout_Small) {
    if (!ShouldRunPerfTests()) {
        GTEST_SKIP() << "Performance tests disabled. Set SCRATCHROBIN_RUN_PERF_TESTS=1 to enable.";
    }
    GenerateNodes(10);
    GenerateEdges(15);
    
    auto engine = LayoutEngine::Create(LayoutAlgorithm::ForceDirected);
    LayoutOptions options;
    options.fd_iterations = 50;  // Reduce for faster tests
    
    timer_.Start();
    auto positions = engine->Layout(*model_, options);
    timer_.Stop();
    
    EXPECT_EQ(positions.size(), 10);
    EXPECT_LT(timer_.ElapsedMs(), ScaledMs(2000.0));
}

TEST_F(DiagramPerformanceTest, NodeIterationPerformance) {
    GenerateNodes(1000);
    
    timer_.Start();
    int total_attrs = 0;
    for (const auto& node : model_->nodes()) {
        total_attrs += static_cast<int>(node.attributes.size());
    }
    timer_.Stop();
    
    EXPECT_GT(total_attrs, 0);
    EXPECT_LT(timer_.ElapsedMs(), ScaledMs(100.0));  // Should be very fast (scaled)
}

TEST_F(DiagramPerformanceTest, EdgeLookupPerformance) {
    GenerateNodes(100);
    GenerateEdges(150);
    
    timer_.Start();
    // Access edges multiple times
    for (int i = 0; i < 1000; ++i) {
        volatile size_t count = model_->edges().size();
        (void)count;
    }
    timer_.Stop();
    
    EXPECT_LT(timer_.ElapsedMs(), ScaledMs(100.0));
}

TEST_F(DiagramPerformanceTest, ModelCreationPerformance) {
    timer_.Start();
    
    auto model = std::make_unique<DiagramModel>(DiagramType::Erd);
    
    for (int i = 0; i < 100; ++i) {
        DiagramNode node;
        node.id = "node_" + std::to_string(i);
        node.name = "Table_" + std::to_string(i);
        model->AddNode(node);
    }
    
    timer_.Stop();
    
    EXPECT_EQ(model->nodes().size(), 100);
    EXPECT_LT(timer_.ElapsedMs(), ScaledMs(100.0));
}

} // namespace scratchrobin
