/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "core/simple_json.h"
#include "core/metadata_model.h"
#include "diagram/diagram_document.h"

namespace scratchrobin {

// Platform-specific memory measurement
#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>

size_t GetCurrentMemoryUsage() {
    // Read from /proc/self/status
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("VmRSS:") == 0) {
            // Parse: "VmRSS:    12345 kB"
            std::istringstream iss(line);
            std::string label;
            size_t value;
            std::string unit;
            iss >> label >> value >> unit;
            return value * 1024;  // Convert kB to bytes
        }
    }
    return 0;
}
#elif __APPLE__
#include <mach/mach.h>

size_t GetCurrentMemoryUsage() {
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS) {
        return info.resident_size;
    }
    return 0;
}
#elif _WIN32
#include <windows.h>
#include <psapi.h>

size_t GetCurrentMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}
#else
size_t GetCurrentMemoryUsage() {
    return 0;  // Not supported on this platform
}
#endif

// Memory threshold constants (in bytes)
constexpr size_t kMaxBaseMemory = 100 * 1024 * 1024;        // 100 MB
constexpr size_t kMaxMetadataModelMemory = 10 * 1024 * 1024; // 10 MB
constexpr size_t kMaxPerDiagramEntity = 500 * 1024;          // 500 KB per entity
constexpr size_t kMaxJsonDocumentMemory = 5 * 1024 * 1024;   // 5 MB

class MemoryUsageTest : public ::testing::Test {
protected:
    void SetUp() override {
        initial_memory_ = GetCurrentMemoryUsage();
    }
    
    size_t GetMemoryDelta() {
        size_t current = GetCurrentMemoryUsage();
        return current > initial_memory_ ? current - initial_memory_ : 0;
    }
    
    size_t initial_memory_ = 0;
};

TEST_F(MemoryUsageTest, MetadataModelMemoryUsage) {
    // Create a metadata model with various objects
    MetadataModel model;
    
    // Add schemas
    for (int i = 0; i < 10; ++i) {
        SchemaMetadata schema;
        schema.name = "schema_" + std::to_string(i);
        
        // Add tables to schema
        for (int j = 0; j < 50; ++j) {
            TableMetadata table;
            table.name = "table_" + std::to_string(j);
            table.schema = schema.name;
            
            // Add columns
            for (int k = 0; k < 10; ++k) {
                ColumnMetadata column;
                column.name = "column_" + std::to_string(k);
                column.type = (k % 2 == 0) ? "INTEGER" : "VARCHAR";
                column.nullable = true;
                table.columns.push_back(column);
            }
            
            schema.tables.push_back(table);
        }
        
        model.AddSchema(schema);
    }
    
    size_t memory_delta = GetMemoryDelta();
    
    // Should use reasonable memory for 10 schemas x 50 tables x 10 columns
    EXPECT_LT(memory_delta, kMaxMetadataModelMemory) 
        << "Memory delta: " << (memory_delta / 1024 / 1024) << " MB";
}

TEST_F(MemoryUsageTest, SimpleJsonMemoryUsage) {
    // Create a large JSON document
    SimpleJson::Object root;
    
    for (int i = 0; i < 1000; ++i) {
        SimpleJson::Object item;
        item["id"] = i;
        item["name"] = "Item " + std::to_string(i);
        item["description"] = "This is a description for item " + std::to_string(i);
        item["active"] = (i % 2 == 0);
        
        SimpleJson::Array tags;
        for (int j = 0; j < 5; ++j) {
            tags.push_back("tag_" + std::to_string(j));
        }
        item["tags"] = tags;
        
        root["item_" + std::to_string(i)] = item;
    }
    
    size_t memory_delta = GetMemoryDelta();
    
    // Should use reasonable memory
    EXPECT_LT(memory_delta, kMaxJsonDocumentMemory)
        << "JSON memory delta: " << (memory_delta / 1024 / 1024) << " MB";
    
    // Serialize and verify
    std::string json_str = SimpleJson::Serialize(root);
    EXPECT_FALSE(json_str.empty());
    
    // Parse back
    auto parsed = SimpleJson::Parse(json_str);
    EXPECT_TRUE(parsed.IsObject());
}

TEST_F(MemoryUsageTest, DiagramDocumentMemoryUsage) {
    // Create a diagram with entities
    auto doc = std::make_unique<DiagramDocument>();
    
    size_t entity_count = 100;
    for (size_t i = 0; i < entity_count; ++i) {
        Entity entity;
        entity.id = "entity_" + std::to_string(i);
        entity.name = "Table_" + std::to_string(i);
        entity.position = Point2D(i * 10.0, i * 5.0);
        entity.size = Size2D(150, 100);
        
        // Add attributes
        for (int j = 0; j < 5; ++j) {
            EntityAttribute attr;
            attr.name = "attr_" + std::to_string(j);
            attr.type = "INTEGER";
            attr.isPrimaryKey = (j == 0);
            entity.attributes.push_back(attr);
        }
        
        doc->AddEntity(entity);
    }
    
    size_t memory_delta = GetMemoryDelta();
    size_t per_entity = memory_delta / entity_count;
    
    // Should use reasonable memory per entity
    EXPECT_LT(per_entity, kMaxPerDiagramEntity)
        << "Per-entity memory: " << (per_entity / 1024) << " KB";
    
    // Serialize to XML
    std::string xml = doc->ToXml();
    EXPECT_FALSE(xml.empty());
    
    // Parse back
    auto doc2 = std::make_unique<DiagramDocument>();
    EXPECT_TRUE(doc2->FromXml(xml));
    EXPECT_EQ(doc2->GetEntities().size(), entity_count);
}

TEST_F(MemoryUsageTest, MemoryGrowthCheck) {
    // Simulate multiple operations and check for leaks
    std::vector<std::unique_ptr<MetadataModel>> models;
    
    // Record memory after each batch
    std::vector<size_t> memory_readings;
    
    for (int batch = 0; batch < 10; ++batch) {
        // Create and destroy models
        for (int i = 0; i < 10; ++i) {
            auto model = std::make_unique<MetadataModel>();
            
            SchemaMetadata schema;
            schema.name = "test_schema";
            
            for (int j = 0; j < 20; ++j) {
                TableMetadata table;
                table.name = "table_" + std::to_string(j);
                table.schema = schema.name;
                schema.tables.push_back(table);
            }
            
            model->AddSchema(schema);
            models.push_back(std::move(model));
        }
        
        // Clear half of them
        if (models.size() > 50) {
            models.erase(models.begin(), models.begin() + 50);
        }
        
        memory_readings.push_back(GetCurrentMemoryUsage());
    }
    
    // Clear all
    models.clear();
    
    // Memory should not grow unboundedly
    // Check that last reading isn't too much higher than early readings
    if (memory_readings.size() >= 2) {
        size_t growth = memory_readings.back() > memory_readings[0] ? 
                        memory_readings.back() - memory_readings[0] : 0;
        
        // Allow 50MB growth for test overhead
        EXPECT_LT(growth, 50 * 1024 * 1024)
            << "Memory growth detected: " << (growth / 1024 / 1024) << " MB";
    }
}

TEST_F(MemoryUsageTest, LargeResultSetHandling) {
    // Simulate handling a large result set
    constexpr int row_count = 10000;
    constexpr int col_count = 10;
    
    QueryResult result;
    
    // Create columns
    for (int c = 0; c < col_count; ++c) {
        QueryColumn col;
        col.name = "column_" + std::to_string(c);
        col.type = "VARCHAR";
        result.columns.push_back(col);
    }
    
    // Create rows
    for (int r = 0; r < row_count; ++r) {
        std::vector<QueryValue> row;
        for (int c = 0; c < col_count; ++c) {
            QueryValue val;
            val.value = "Value_" + std::to_string(r) + "_" + std::to_string(c);
            row.push_back(val);
        }
        result.rows.push_back(row);
    }
    
    size_t memory_delta = GetMemoryDelta();
    
    // Should be reasonable for 10K rows x 10 columns
    // Allow up to 100 MB for this test
    EXPECT_LT(memory_delta, 100 * 1024 * 1024)
        << "Large result set memory: " << (memory_delta / 1024 / 1024) << " MB";
    
    // Verify data integrity
    EXPECT_EQ(result.rows.size(), row_count);
    EXPECT_EQ(result.columns.size(), col_count);
}

// Skip tests if memory measurement not available
TEST_F(MemoryUsageTest, MemoryMeasurementAvailable) {
    size_t memory = GetCurrentMemoryUsage();
    
    if (memory == 0) {
        GTEST_SKIP() << "Memory measurement not available on this platform";
    }
    
    EXPECT_GT(memory, 0);
    
    // Should be reasonable for a test process
    EXPECT_LT(memory, 500 * 1024 * 1024)  // 500 MB
        << "Initial memory usage: " << (memory / 1024 / 1024) << " MB";
}

} // namespace scratchrobin
