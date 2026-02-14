/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include "diagram/mindmap_util.h"

using scratchrobin::DiagramModel;
using scratchrobin::DiagramNode;
using scratchrobin::DiagramType;
using scratchrobin::MindMapCountDescendants;
using scratchrobin::MindMapHasChildren;

namespace {

DiagramNode MakeNode(const std::string& id, const std::string& name, const std::string& parent) {
    DiagramNode node;
    node.id = id;
    node.name = name;
    node.parent_id = parent;
    node.x = 0;
    node.y = 0;
    node.width = 100;
    node.height = 60;
    return node;
}

} // namespace

TEST(MindMapUtilTest, CountsDescendants) {
    DiagramModel model(DiagramType::MindMap);
    model.AddNode(MakeNode("a", "Root", ""));
    model.AddNode(MakeNode("b", "ChildOne", "a"));
    model.AddNode(MakeNode("c", "ChildTwo", "a"));
    model.AddNode(MakeNode("d", "Grandchild", "b"));

    EXPECT_TRUE(MindMapHasChildren(model, "a"));
    EXPECT_TRUE(MindMapHasChildren(model, "b"));
    EXPECT_FALSE(MindMapHasChildren(model, "c"));

    EXPECT_EQ(MindMapCountDescendants(model, "a"), 3);
    EXPECT_EQ(MindMapCountDescendants(model, "b"), 1);
    EXPECT_EQ(MindMapCountDescendants(model, "c"), 0);
}
