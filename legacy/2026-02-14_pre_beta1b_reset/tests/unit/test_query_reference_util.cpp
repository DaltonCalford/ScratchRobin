/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include "core/query_reference_util.h"

using scratchrobin::ExtractQueryReferences;
using scratchrobin::QueryReferencesObject;

TEST(QueryReferenceUtilTest, HandlesSchemaQualifiedTable) {
    EXPECT_TRUE(QueryReferencesObject("SELECT * FROM public.orders", "public", "orders"));
    EXPECT_TRUE(QueryReferencesObject("SELECT * FROM sales.orders", "sales", "orders"));
    EXPECT_FALSE(QueryReferencesObject("SELECT * FROM sales.orders", "public", "orders"));
}

TEST(QueryReferenceUtilTest, HandlesAliasesAndMultiJoin) {
    std::string query = "SELECT * FROM public.orders o "
                        "JOIN public.customers c ON o.customer_id = c.id "
                        "LEFT JOIN public.items i ON i.id = o.item_id";
    auto refs = ExtractQueryReferences(query);
    EXPECT_TRUE(refs.parsed);
    EXPECT_GE(refs.identifiers.size(), 3u);
    EXPECT_TRUE(QueryReferencesObject(query, "public", "orders"));
    EXPECT_TRUE(QueryReferencesObject(query, "public", "customers"));
    EXPECT_TRUE(QueryReferencesObject(query, "public", "items"));
}

TEST(QueryReferenceUtilTest, HandlesQuotedIdentifiers) {
    std::string query = "SELECT * FROM \"Sales\".\"Order Items\" oi "
                        "JOIN `Customers` c ON c.id = oi.customer_id "
                        "JOIN [Audit.Log] al ON al.order_id = oi.id";
    EXPECT_TRUE(QueryReferencesObject(query, "Sales", "Order Items"));
    EXPECT_TRUE(QueryReferencesObject(query, "", "customers"));
    EXPECT_TRUE(QueryReferencesObject(query, "", "audit.log"));
}

TEST(QueryReferenceUtilTest, HandlesQuotedIdentifiersWithDotsInside) {
    std::string query = "SELECT * FROM \"Sales\".\"Order.Items\" oi";
    EXPECT_TRUE(QueryReferencesObject(query, "Sales", "Order.Items"));
    EXPECT_FALSE(QueryReferencesObject(query, "Sales", "Order"));
}

TEST(QueryReferenceUtilTest, HandlesQuotedIdentifiersWithSpaces) {
    std::string query = "SELECT * FROM \"Sales Data\".\"Order Items\" oi "
                        "JOIN \"Customer Accounts\" ca ON ca.id = oi.customer_id";
    EXPECT_TRUE(QueryReferencesObject(query, "Sales Data", "Order Items"));
    EXPECT_TRUE(QueryReferencesObject(query, "", "Customer Accounts"));
}

TEST(QueryReferenceUtilTest, HandlesNestedJoinsInSubqueries) {
    std::string query =
        "SELECT * FROM (SELECT * FROM sales.orders o "
        "JOIN sales.order_lines ol ON ol.order_id = o.id) sub "
        "JOIN sales.customers c ON c.id = sub.customer_id";
    EXPECT_TRUE(QueryReferencesObject(query, "sales", "orders"));
    EXPECT_TRUE(QueryReferencesObject(query, "sales", "order_lines"));
    EXPECT_TRUE(QueryReferencesObject(query, "sales", "customers"));
}

TEST(QueryReferenceUtilTest, HandlesSubqueries) {
    std::string query =
        "SELECT * FROM (SELECT * FROM public.orders) sub "
        "JOIN public.customers c ON c.id = sub.customer_id";
    EXPECT_TRUE(QueryReferencesObject(query, "public", "orders"));
    EXPECT_TRUE(QueryReferencesObject(query, "public", "customers"));
}

TEST(QueryReferenceUtilTest, HandlesUnqualifiedTableNames) {
    std::string query = "SELECT * FROM orders JOIN customers ON orders.customer_id = customers.id";
    EXPECT_TRUE(QueryReferencesObject(query, "", "orders"));
    EXPECT_TRUE(QueryReferencesObject(query, "", "customers"));
    EXPECT_FALSE(QueryReferencesObject(query, "public", "products"));
}

TEST(QueryReferenceUtilTest, IgnoresStopTokens) {
    std::string query = "SELECT * FROM orders WHERE status = 'OPEN' ORDER BY created_at";
    auto refs = ExtractQueryReferences(query);
    EXPECT_TRUE(refs.parsed);
    EXPECT_EQ(refs.identifiers.size(), 1u);
    EXPECT_EQ(refs.identifiers[0], "orders");
}

TEST(QueryReferenceUtilTest, ReturnsFalseWhenNoIdentifiers) {
    std::string query = "SELECT 1";
    auto refs = ExtractQueryReferences(query);
    EXPECT_TRUE(refs.parsed);
    EXPECT_TRUE(refs.identifiers.empty());
    EXPECT_FALSE(QueryReferencesObject(query, "public", "orders"));
}
