/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include <gtest/gtest.h>

#include "ui/diagram_containment.h"

using namespace scratchrobin;

// ============================================================================
// Containment Rules Tests
// ============================================================================

TEST(DiagramContainmentTest, SchemaCanContainTable) {
    EXPECT_TRUE(CanAcceptChild("Schema", "Table"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Schema, DiagramNodeType::Table));
}

TEST(DiagramContainmentTest, SchemaCanContainView) {
    EXPECT_TRUE(CanAcceptChild("Schema", "View"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Schema, DiagramNodeType::View));
}

TEST(DiagramContainmentTest, SchemaCanContainProcedure) {
    EXPECT_TRUE(CanAcceptChild("Schema", "Procedure"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Schema, DiagramNodeType::Procedure));
}

TEST(DiagramContainmentTest, SchemaCanContainFunction) {
    EXPECT_TRUE(CanAcceptChild("Schema", "Function"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Schema, DiagramNodeType::Function));
}

TEST(DiagramContainmentTest, SchemaCanContainTrigger) {
    EXPECT_TRUE(CanAcceptChild("Schema", "Trigger"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Schema, DiagramNodeType::Trigger));
}

TEST(DiagramContainmentTest, TableCanContainColumn) {
    EXPECT_TRUE(CanAcceptChild("Table", "Column"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Table, DiagramNodeType::Column));
}

TEST(DiagramContainmentTest, TableCanContainIndex) {
    EXPECT_TRUE(CanAcceptChild("Table", "Index"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Table, DiagramNodeType::Index));
}

TEST(DiagramContainmentTest, TableCanContainTrigger) {
    EXPECT_TRUE(CanAcceptChild("Table", "Trigger"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Table, DiagramNodeType::Trigger));
}

TEST(DiagramContainmentTest, TableCannotContainSchema) {
    EXPECT_FALSE(CanAcceptChild("Table", "Schema"));
    EXPECT_FALSE(CanAcceptChild(DiagramNodeType::Table, DiagramNodeType::Schema));
}

TEST(DiagramContainmentTest, ColumnCannotContainAnything) {
    EXPECT_FALSE(CanAcceptChild("Column", "Table"));
    EXPECT_FALSE(CanAcceptChild("Column", "Column"));
    EXPECT_FALSE(CanAcceptChild("Column", "Index"));
    EXPECT_FALSE(CanAcceptChild(DiagramNodeType::Column, DiagramNodeType::Table));
    EXPECT_FALSE(CanAcceptChild(DiagramNodeType::Column, DiagramNodeType::Column));
}

TEST(DiagramContainmentTest, IndexCannotContainAnything) {
    EXPECT_FALSE(CanAcceptChild("Index", "Table"));
    EXPECT_FALSE(CanAcceptChild("Index", "Column"));
    EXPECT_FALSE(CanAcceptChild(DiagramNodeType::Index, DiagramNodeType::Table));
}

TEST(DiagramContainmentTest, DatabaseCanContainSchema) {
    EXPECT_TRUE(CanAcceptChild("Database", "Schema"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Database, DiagramNodeType::Schema));
}

TEST(DiagramContainmentTest, DatabaseCanContainTable) {
    EXPECT_TRUE(CanAcceptChild("Database", "Table"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Database, DiagramNodeType::Table));
}

TEST(DiagramContainmentTest, DatabaseCanContainView) {
    EXPECT_TRUE(CanAcceptChild("Database", "View"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Database, DiagramNodeType::View));
}

TEST(DiagramContainmentTest, ClusterCanContainDatabase) {
    EXPECT_TRUE(CanAcceptChild("Cluster", "Database"));
    EXPECT_TRUE(CanAcceptChild(DiagramNodeType::Cluster, DiagramNodeType::Database));
}

TEST(DiagramContainmentTest, ViewCannotContainChildren) {
    EXPECT_FALSE(CanAcceptChild("View", "Table"));
    EXPECT_FALSE(CanAcceptChild("View", "Column"));
    EXPECT_FALSE(CanAcceptChild(DiagramNodeType::View, DiagramNodeType::Table));
}

// ============================================================================
// Container Type Tests
// ============================================================================

TEST(DiagramContainmentTest, SchemaIsContainer) {
    EXPECT_TRUE(IsContainerType("Schema"));
    EXPECT_TRUE(IsContainerType(DiagramNodeType::Schema));
}

TEST(DiagramContainmentTest, TableIsContainer) {
    EXPECT_TRUE(IsContainerType("Table"));
    EXPECT_TRUE(IsContainerType(DiagramNodeType::Table));
}

TEST(DiagramContainmentTest, DatabaseIsContainer) {
    EXPECT_TRUE(IsContainerType("Database"));
    EXPECT_TRUE(IsContainerType(DiagramNodeType::Database));
}

TEST(DiagramContainmentTest, ClusterIsContainer) {
    EXPECT_TRUE(IsContainerType("Cluster"));
    EXPECT_TRUE(IsContainerType(DiagramNodeType::Cluster));
}

TEST(DiagramContainmentTest, ColumnIsNotContainer) {
    EXPECT_FALSE(IsContainerType("Column"));
    EXPECT_FALSE(IsContainerType(DiagramNodeType::Column));
}

TEST(DiagramContainmentTest, IndexIsNotContainer) {
    EXPECT_FALSE(IsContainerType("Index"));
    EXPECT_FALSE(IsContainerType(DiagramNodeType::Index));
}

TEST(DiagramContainmentTest, TriggerIsNotContainer) {
    EXPECT_FALSE(IsContainerType("Trigger"));
    EXPECT_FALSE(IsContainerType(DiagramNodeType::Trigger));
}

// ============================================================================
// Type Conversion Tests
// ============================================================================

TEST(DiagramContainmentTest, StringToTypeConversion) {
    EXPECT_EQ(StringToDiagramNodeType("Schema"), DiagramNodeType::Schema);
    EXPECT_EQ(StringToDiagramNodeType("schema"), DiagramNodeType::Schema);
    EXPECT_EQ(StringToDiagramNodeType("SCHEMA"), DiagramNodeType::Schema);
    EXPECT_EQ(StringToDiagramNodeType("Table"), DiagramNodeType::Table);
    EXPECT_EQ(StringToDiagramNodeType("Column"), DiagramNodeType::Column);
    EXPECT_EQ(StringToDiagramNodeType("Unknown"), DiagramNodeType::Generic);
}

TEST(DiagramContainmentTest, TypeToStringConversion) {
    EXPECT_EQ(DiagramNodeTypeToString(DiagramNodeType::Schema), "Schema");
    EXPECT_EQ(DiagramNodeTypeToString(DiagramNodeType::Table), "Table");
    EXPECT_EQ(DiagramNodeTypeToString(DiagramNodeType::Column), "Column");
    EXPECT_EQ(DiagramNodeTypeToString(DiagramNodeType::Generic), "Generic");
}

// ============================================================================
// Valid Child Types Tests
// ============================================================================

TEST(DiagramContainmentTest, GetValidChildTypesForSchema) {
    auto valid = GetValidChildTypes("Schema");
    EXPECT_FALSE(valid.empty());
    
    // Check that expected types are in the list
    bool has_table = false;
    bool has_view = false;
    for (const auto& type : valid) {
        if (type == "Table") has_table = true;
        if (type == "View") has_view = true;
    }
    EXPECT_TRUE(has_table);
    EXPECT_TRUE(has_view);
}

TEST(DiagramContainmentTest, GetValidChildTypesForColumn) {
    auto valid = GetValidChildTypes("Column");
    EXPECT_TRUE(valid.empty());
}
