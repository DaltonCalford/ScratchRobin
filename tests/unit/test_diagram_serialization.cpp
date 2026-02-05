/**
 * @file test_diagram_serialization.cpp
 * @brief Diagram serialization tests
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "diagram/diagram_serialization.h"

using namespace scratchrobin;
using namespace scratchrobin::diagram;

class DiagramSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() / "scratchrobin_diagram_test";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir_);
    }

    std::filesystem::path temp_dir_;
};

TEST_F(DiagramSerializationTest, SaveAndLoadMindMap) {
    DiagramModel model(DiagramType::MindMap);
    DiagramNode root;
    root.id = "mm_node_1";
    root.name = "Root";
    root.type = "Topic";
    root.x = 10;
    root.y = 10;
    root.width = 200;
    root.height = 90;
    root.tags = {"plan", "v1"};
    model.AddNode(root);

    DiagramNode child;
    child.id = "mm_node_2";
    child.name = "Child";
    child.type = "Idea";
    child.parent_id = root.id;
    child.x = 260;
    child.y = 20;
    child.width = 180;
    child.height = 80;
    model.AddNode(child);

    DiagramEdge edge;
    edge.id = "mm_edge_1";
    edge.source_id = root.id;
    edge.target_id = child.id;
    edge.label = "link";
    edge.edge_type = "link";
    edge.directed = true;
    model.AddEdge(edge);

    DiagramDocument doc;
    doc.diagram_id = "diagram-1";
    doc.name = "Mind Map Example";
    doc.zoom = 1.25;
    doc.pan_x = 12.0;
    doc.pan_y = 24.0;

    std::string path = (temp_dir_ / "mindmap.sbdgm").string();
    std::string error;
    ASSERT_TRUE(DiagramSerializer::SaveToFile(model, doc, path, &error));

    DiagramModel loaded(DiagramType::MindMap);
    DiagramDocument loaded_doc;
    ASSERT_TRUE(DiagramSerializer::LoadFromFile(&loaded, &loaded_doc, path, &error));

    EXPECT_EQ(loaded.type(), DiagramType::MindMap);
    EXPECT_EQ(loaded_doc.name, "Mind Map Example");
    ASSERT_EQ(loaded.nodes().size(), 2u);
    EXPECT_EQ(loaded.nodes()[0].name, "Root");
    EXPECT_EQ(loaded.nodes()[1].parent_id, "mm_node_1");
    ASSERT_EQ(loaded.edges().size(), 1u);
    EXPECT_EQ(loaded.edges()[0].edge_type, "link");
}

TEST_F(DiagramSerializationTest, SaveAndLoadDataFlow) {
    DiagramModel model(DiagramType::DataFlow);
    DiagramNode process;
    process.id = "dfd_p1";
    process.name = "Process Orders";
    process.type = "Process";
    process.trace_refs = {"public.orders", "public.order_items"};
    process.x = 50;
    process.y = 50;
    process.width = 200;
    process.height = 120;
    model.AddNode(process);

    DiagramNode store;
    store.id = "dfd_s1";
    store.name = "Orders Store";
    store.type = "Data Store";
    store.x = 320;
    store.y = 60;
    store.width = 200;
    store.height = 100;
    model.AddNode(store);

    DiagramEdge flow;
    flow.id = "dfd_f1";
    flow.source_id = process.id;
    flow.target_id = store.id;
    flow.label = "flow";
    flow.edge_type = "data_flow";
    flow.directed = true;
    model.AddEdge(flow);

    DiagramDocument doc;
    doc.diagram_id = "diagram-2";
    doc.name = "DFD Example";

    std::string path = (temp_dir_ / "dfd.sbdgm").string();
    std::string error;
    ASSERT_TRUE(DiagramSerializer::SaveToFile(model, doc, path, &error));

    DiagramModel loaded(DiagramType::DataFlow);
    DiagramDocument loaded_doc;
    ASSERT_TRUE(DiagramSerializer::LoadFromFile(&loaded, &loaded_doc, path, &error));

    EXPECT_EQ(loaded.type(), DiagramType::DataFlow);
    ASSERT_EQ(loaded.nodes().size(), 2u);
    EXPECT_EQ(loaded.nodes()[0].trace_refs.size(), 2u);
    EXPECT_EQ(loaded.edges()[0].edge_type, "data_flow");
}
