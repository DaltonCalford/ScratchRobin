/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include "diagram/trace_util.h"

using scratchrobin::ParseTraceRef;
using scratchrobin::TraceTarget;

TEST(DiagramTraceUtilTest, ParsesDiagramPrefix) {
    TraceTarget target = ParseTraceRef("diagram:Customer");
    EXPECT_TRUE(target.diagram_path.empty());
    EXPECT_EQ(target.node_name, "Customer");
}

TEST(DiagramTraceUtilTest, ParsesErdPrefix) {
    TraceTarget target = ParseTraceRef("erd:Order");
    EXPECT_TRUE(target.diagram_path.empty());
    EXPECT_EQ(target.node_name, "Order");
}

TEST(DiagramTraceUtilTest, ParsesDiagramPathAndNode) {
    TraceTarget target = ParseTraceRef("diagrams/core.sberd#Invoice");
    EXPECT_EQ(target.diagram_path, "diagrams/core.sberd");
    EXPECT_EQ(target.node_name, "Invoice");
}
