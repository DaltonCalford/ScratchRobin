#include "metadata/object_hierarchy.h"
#include "metadata/schema_collector.h"
#include <algorithm>
#include <queue>
#include <stack>
#include <iostream>

namespace scratchrobin {

class ObjectHierarchy::Impl {
public:
    Impl(std::shared_ptr<ISchemaCollector> schemaCollector)
        : schemaCollector_(schemaCollector) {}

    Result<ObjectHierarchyInfo> buildHierarchyInternal(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options) {

        ObjectHierarchyInfo hierarchy;
        hierarchy.rootSchema = schema;
        hierarchy.rootObject = object;
        hierarchy.rootType = type;

        try {
            // Collect direct dependencies
            auto dependenciesResult = collectDependencies(schema, object, type, options);
            if (!dependenciesResult.isSuccess()) {
                return Result<ObjectHierarchyInfo>::error(dependenciesResult.error().code, dependenciesResult.error().message);
            }
            hierarchy.directDependencies = dependenciesResult.value();

            // Collect direct dependents
            auto dependentsResult = collectDependents(schema, object, type, options);
            if (!dependentsResult.isSuccess()) {
                return Result<ObjectHierarchyInfo>::error(dependentsResult.error().code, dependentsResult.error().message);
            }
            hierarchy.directDependents = dependentsResult.value();

            // Build dependency graphs
            buildDependencyGraph(hierarchy, hierarchy.directDependencies, hierarchy.directDependents);

            // Detect circular references
            if (options.detectCircularReferences) {
                detectCircularReferences(hierarchy);
            }

            // Calculate dependency levels
            calculateDependencyLevels(hierarchy);

            // Calculate hierarchy statistics
            hierarchy.totalObjects = hierarchy.dependencyGraph.size() + hierarchy.dependentGraph.size() + 1;
            hierarchy.maxDepth = calculateMaxDepth(hierarchy);

        } catch (const std::exception& e) {
            return Result<ObjectHierarchyInfo>::error(ErrorCode::UNKNOWN_ERROR, std::string("Failed to build hierarchy: ") + e.what());
        }

        return Result<ObjectHierarchyInfo>::success(hierarchy);
    }

    Result<std::vector<ObjectReference>> collectDependencies(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        try {
            // Get object details to understand its structure
            auto objectResult = schemaCollector_->getObjectDetails(schema, object, type);
            if (!objectResult.isSuccess()) {
                return Result<std::vector<ObjectReference>>::error(objectResult.error().code, objectResult.error().message);
            }

            SchemaObject obj = objectResult.value();

            // Based on object type, collect different types of dependencies
            switch (type) {
                case SchemaObjectType::TABLE:
                    dependencies = collectTableDependencies(schema, object, obj, options);
                    break;
                case SchemaObjectType::VIEW:
                    dependencies = collectViewDependencies(schema, object, obj, options);
                    break;
                case SchemaObjectType::COLUMN:
                    dependencies = collectColumnDependencies(schema, object, obj, options);
                    break;
                case SchemaObjectType::INDEX:
                    dependencies = collectIndexDependencies(schema, object, obj, options);
                    break;
                case SchemaObjectType::CONSTRAINT:
                    dependencies = collectConstraintDependencies(schema, object, obj, options);
                    break;
                case SchemaObjectType::FUNCTION:
                    dependencies = collectFunctionDependencies(schema, object, obj, options);
                    break;
                case SchemaObjectType::PROCEDURE:
                    dependencies = collectProcedureDependencies(schema, object, obj, options);
                    break;
                default:
                    // For other object types, return empty dependencies
                    break;
            }

            // Filter dependencies based on options
            dependencies.erase(
                std::remove_if(dependencies.begin(), dependencies.end(),
                    [&](const ObjectReference& ref) {
                        return !shouldIncludeDependency(ref, options);
                    }),
                dependencies.end());

        } catch (const std::exception& e) {
            return Result<std::vector<ObjectReference>>::error(
                std::string("Failed to collect dependencies: ") + e.what());
        }

        return Result<std::vector<ObjectReference>>::success(dependencies);
    }

    Result<std::vector<ObjectReference>> collectDependents(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependents;

        try {
            // Collect all objects that might depend on this object
            SchemaCollectionOptions collectionOptions;
            collectionOptions.includedSchemas = {schema};
            collectionOptions.includeSystemObjects = options.includeSystemObjects;

            auto collectionResult = schemaCollector_->collectSchema(collectionOptions);
            if (!collectionResult.isSuccess()) {
                return Result<std::vector<ObjectReference>>::error(collectionResult.error().code, collectionResult.error().message);
            }

            for (const auto& obj : collectionResult.value().objects) {
                // Check if this object depends on our target object
                auto depsResult = collectDependencies(obj.schema, obj.name, obj.type, options);
                if (depsResult.isSuccess()) {
                    for (const auto& dep : depsResult.value()) {
                        if (dep.toSchema == schema && dep.toObject == object && dep.toType == type) {
                            // Create reverse reference (dependent)
                            ObjectReference dependentRef;
                            dependentRef.fromSchema = obj.schema;
                            dependentRef.fromObject = obj.name;
                            dependentRef.fromType = obj.type;
                            dependentRef.toSchema = schema;
                            dependentRef.toObject = object;
                            dependentRef.toType = type;
                            dependentRef.dependencyType = dep.dependencyType;
                            dependentRef.referenceName = dep.referenceName;
                            dependentRef.description = "Reverse dependency: " + obj.name + " depends on " + object;

                            dependents.push_back(dependentRef);
                        }
                    }
                }
            }

        } catch (const std::exception& e) {
            return Result<std::vector<ObjectReference>>::error(
                std::string("Failed to collect dependents: ") + e.what());
        }

        return Result<std::vector<ObjectReference>>::success(dependents);
    }

private:
    std::shared_ptr<ISchemaCollector> schemaCollector_;

    // Object type-specific dependency collection methods
    std::vector<ObjectReference> collectTableDependencies(
        const std::string& schema, const std::string& table, const SchemaObject& obj,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        // Tables typically depend on schemas
        ObjectReference schemaDep;
        schemaDep.fromSchema = schema;
        schemaDep.fromObject = table;
        schemaDep.fromType = SchemaObjectType::TABLE;
        schemaDep.toSchema = schema;
        schemaDep.toObject = schema;
        schemaDep.toType = SchemaObjectType::SCHEMA;
        schemaDep.dependencyType = DependencyType::HARD_DEPENDENCY;
        schemaDep.referenceName = "table_schema";
        schemaDep.description = "Table " + table + " belongs to schema " + schema;
        dependencies.push_back(schemaDep);

        // Add dependencies on sequences if table has serial columns
        if (obj.properties.count("has_serial_columns") &&
            obj.properties.at("has_serial_columns") == "true") {
            // In a real implementation, this would query for actual sequences
            // For now, we'll add a placeholder dependency
        }

        return dependencies;
    }

    std::vector<ObjectReference> collectViewDependencies(
        const std::string& schema, const std::string& view, const SchemaObject& obj,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        // Views depend on schemas
        ObjectReference schemaDep;
        schemaDep.fromSchema = schema;
        schemaDep.fromObject = view;
        schemaDep.fromType = SchemaObjectType::VIEW;
        schemaDep.toSchema = schema;
        schemaDep.toObject = schema;
        schemaDep.toType = SchemaObjectType::SCHEMA;
        schemaDep.dependencyType = DependencyType::HARD_DEPENDENCY;
        schemaDep.referenceName = "view_schema";
        schemaDep.description = "View " + view + " belongs to schema " + schema;
        dependencies.push_back(schemaDep);

        // Views depend on the tables they reference
        // In a real implementation, this would parse the view definition
        // For now, we'll add placeholder dependencies based on common patterns

        return dependencies;
    }

    std::vector<ObjectReference> collectColumnDependencies(
        const std::string& schema, const std::string& column, const SchemaObject& obj,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        // Columns depend on their parent tables
        if (obj.properties.count("table_name")) {
            ObjectReference tableDep;
            tableDep.fromSchema = schema;
            tableDep.fromObject = column;
            tableDep.fromType = SchemaObjectType::COLUMN;
            tableDep.toSchema = schema;
            tableDep.toObject = obj.properties.at("table_name");
            tableDep.toType = SchemaObjectType::TABLE;
            tableDep.dependencyType = DependencyType::HARD_DEPENDENCY;
            tableDep.referenceName = "parent_table";
            tableDep.description = "Column " + column + " belongs to table " + obj.properties.at("table_name");
            dependencies.push_back(tableDep);
        }

        // Columns may depend on domains or types
        if (obj.properties.count("domain_name")) {
            ObjectReference domainDep;
            domainDep.fromSchema = schema;
            domainDep.fromObject = column;
            domainDep.fromType = SchemaObjectType::COLUMN;
            domainDep.toSchema = schema;
            domainDep.toObject = obj.properties.at("domain_name");
            domainDep.toType = SchemaObjectType::DOMAIN;
            domainDep.dependencyType = DependencyType::HARD_DEPENDENCY;
            domainDep.referenceName = "column_domain";
            domainDep.description = "Column " + column + " uses domain " + obj.properties.at("domain_name");
            dependencies.push_back(domainDep);
        }

        return dependencies;
    }

    std::vector<ObjectReference> collectIndexDependencies(
        const std::string& schema, const std::string& index, const SchemaObject& obj,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        // Indexes depend on their parent tables
        if (obj.properties.count("table_name")) {
            ObjectReference tableDep;
            tableDep.fromSchema = schema;
            tableDep.fromObject = index;
            tableDep.fromType = SchemaObjectType::INDEX;
            tableDep.toSchema = schema;
            tableDep.toObject = obj.properties.at("table_name");
            tableDep.toType = SchemaObjectType::TABLE;
            tableDep.dependencyType = DependencyType::HARD_DEPENDENCY;
            tableDep.referenceName = "parent_table";
            tableDep.description = "Index " + index + " is defined on table " + obj.properties.at("table_name");
            dependencies.push_back(tableDep);
        }

        return dependencies;
    }

    std::vector<ObjectReference> collectConstraintDependencies(
        const std::string& schema, const std::string& constraint, const SchemaObject& obj,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        // Constraints depend on their parent tables
        if (obj.properties.count("table_name")) {
            ObjectReference tableDep;
            tableDep.fromSchema = schema;
            tableDep.fromObject = constraint;
            tableDep.fromType = SchemaObjectType::CONSTRAINT;
            tableDep.toSchema = schema;
            tableDep.toObject = obj.properties.at("table_name");
            tableDep.toType = SchemaObjectType::TABLE;
            tableDep.dependencyType = DependencyType::HARD_DEPENDENCY;
            tableDep.referenceName = "parent_table";
            tableDep.description = "Constraint " + constraint + " is defined on table " + obj.properties.at("table_name");
            dependencies.push_back(tableDep);
        }

        return dependencies;
    }

    std::vector<ObjectReference> collectFunctionDependencies(
        const std::string& schema, const std::string& function, const SchemaObject& obj,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        // Functions depend on schemas
        ObjectReference schemaDep;
        schemaDep.fromSchema = schema;
        schemaDep.fromObject = function;
        schemaDep.fromType = SchemaObjectType::FUNCTION;
        schemaDep.toSchema = schema;
        schemaDep.toObject = schema;
        schemaDep.toType = SchemaObjectType::SCHEMA;
        schemaDep.dependencyType = DependencyType::HARD_DEPENDENCY;
        schemaDep.referenceName = "function_schema";
        schemaDep.description = "Function " + function + " belongs to schema " + schema;
        dependencies.push_back(schemaDep);

        return dependencies;
    }

    std::vector<ObjectReference> collectProcedureDependencies(
        const std::string& schema, const std::string& procedure, const SchemaObject& obj,
        const HierarchyTraversalOptions& options) {

        std::vector<ObjectReference> dependencies;

        // Procedures depend on schemas
        ObjectReference schemaDep;
        schemaDep.fromSchema = schema;
        schemaDep.fromObject = procedure;
        schemaDep.fromType = SchemaObjectType::PROCEDURE;
        schemaDep.toSchema = schema;
        schemaDep.toObject = schema;
        schemaDep.toType = SchemaObjectType::SCHEMA;
        schemaDep.dependencyType = DependencyType::HARD_DEPENDENCY;
        schemaDep.referenceName = "procedure_schema";
        schemaDep.description = "Procedure " + procedure + " belongs to schema " + schema;
        dependencies.push_back(schemaDep);

        return dependencies;
    }

    void buildDependencyGraph(ObjectHierarchyInfo& hierarchy,
                            const std::vector<ObjectReference>& dependencies,
                            const std::vector<ObjectReference>& dependents) {

        // Build dependency graph (what this object depends on)
        for (const auto& dep : dependencies) {
            std::string key = generateObjectKey(dep.toSchema, dep.toObject, dep.toType);
            hierarchy.dependencyGraph[key].push_back(dep);
        }

        // Build dependent graph (what depends on this object)
        for (const auto& dep : dependents) {
            std::string key = generateObjectKey(dep.fromSchema, dep.fromObject, dep.fromType);
            hierarchy.dependentGraph[key].push_back(dep);
        }
    }

    void detectCircularReferences(ObjectHierarchyInfo& hierarchy) {
        std::unordered_set<std::string> visited;
        std::vector<ObjectReference> currentChain;

        std::string rootKey = generateObjectKey(hierarchy.rootSchema, hierarchy.rootObject, hierarchy.rootType);

        findCircularReferenceChains(hierarchy, rootKey, visited, currentChain);

        hierarchy.hasCircularReferences = !hierarchy.circularReferenceChains.empty();
    }

    void findCircularReferenceChains(ObjectHierarchy& hierarchy,
                                   const std::string& currentKey,
                                   std::unordered_set<std::string>& visited,
                                   std::vector<ObjectReference>& currentChain) {

        if (visited.count(currentKey)) {
            // Check if this creates a circular reference
            auto it = std::find_if(currentChain.begin(), currentChain.end(),
                [&](const ObjectReference& ref) {
                    std::string key = generateObjectKey(ref.toSchema, ref.toObject, ref.toType);
                    return key == currentKey;
                });

            if (it != currentChain.end()) {
                // Found a circular reference
                std::vector<ObjectReference> circularChain(it, currentChain.end());
                hierarchy.circularReferenceChains.push_back(circularChain);

                // Mark references in the chain as circular
                for (auto& ref : circularChain) {
                    ref.isCircular = true;
                }
            }
            return;
        }

        visited.insert(currentKey);

        // Explore all dependencies of the current object
        if (hierarchy.dependencyGraph.count(currentKey)) {
            for (const auto& dep : hierarchy.dependencyGraph.at(currentKey)) {
                currentChain.push_back(dep);
                std::string nextKey = generateObjectKey(dep.toSchema, dep.toObject, dep.toType);
                findCircularReferenceChains(hierarchy, nextKey, visited, currentChain);
                currentChain.pop_back();
            }
        }

        visited.erase(currentKey);
    }

    void calculateDependencyLevels(ObjectHierarchyInfo& hierarchy) {
        std::string rootKey = generateObjectKey(hierarchy.rootSchema, hierarchy.rootObject, hierarchy.rootType);

        // Use breadth-first search to calculate dependency levels
        std::queue<std::pair<std::string, int>> queue;
        std::unordered_set<std::string> visited;

        queue.push({rootKey, 0});
        visited.insert(rootKey);

        while (!queue.empty()) {
            auto [currentKey, level] = queue.front();
            queue.pop();

            hierarchy.dependencyLevels[currentKey] = level;

            // Add all dependencies to the queue
            if (hierarchy.dependencyGraph.count(currentKey)) {
                for (const auto& dep : hierarchy.dependencyGraph.at(currentKey)) {
                    std::string nextKey = generateObjectKey(dep.toSchema, dep.toObject, dep.toType);
                    if (visited.find(nextKey) == visited.end()) {
                        visited.insert(nextKey);
                        queue.push({nextKey, level + 1});
                    }
                }
            }
        }
    }

    int calculateMaxDepth(const ObjectHierarchyInfo& hierarchy) {
        int maxDepth = 0;
        for (const auto& [key, level] : hierarchy.dependencyLevels) {
            maxDepth = std::max(maxDepth, level);
        }
        return maxDepth;
    }

    std::string generateObjectKey(const std::string& schema,
                                const std::string& object,
                                SchemaObjectType type) {
        return schema + "." + object + "." + std::to_string(static_cast<int>(type));
    }

    bool shouldIncludeDependency(const ObjectReference& ref,
                               const HierarchyTraversalOptions& options) {
        // Check if dependency type is included
        if (!options.includedDependencyTypes.empty()) {
            if (std::find(options.includedDependencyTypes.begin(),
                         options.includedDependencyTypes.end(),
                         ref.dependencyType) == options.includedDependencyTypes.end()) {
                return false;
            }
        }

        // Check if dependency type is excluded
        if (!options.excludedDependencyTypes.empty()) {
            if (std::find(options.excludedDependencyTypes.begin(),
                         options.excludedDependencyTypes.end(),
                         ref.dependencyType) != options.excludedDependencyTypes.end()) {
                return false;
            }
        }

        // Check system objects
        if (!options.includeSystemObjects) {
            if (isSystemObject(ref.toSchema, ref.toObject)) {
                return false;
            }
        }

        return true;
    }

    bool isSystemObject(const std::string& schema, const std::string& object) {
        // Common PostgreSQL system schemas
        static const std::unordered_set<std::string> systemSchemas = {
            "information_schema", "pg_catalog", "pg_toast"
        };

        return systemSchemas.count(schema) > 0;
    }
};

// ObjectHierarchy implementation

ObjectHierarchy::ObjectHierarchy(std::shared_ptr<ISchemaCollector> schemaCollector)
    : impl_(std::make_unique<Impl>(schemaCollector)),
      schemaCollector_(schemaCollector) {}

ObjectHierarchy::~ObjectHierarchy() = default;

Result<ObjectHierarchy> ObjectHierarchy::buildHierarchy(
    const std::string& schema, const std::string& object, SchemaObjectType type,
    const HierarchyTraversalOptions& options) {

    return impl_->buildHierarchyInternal(schema, object, type, options);
}

Result<std::vector<ObjectReference>> ObjectHierarchy::getDirectDependencies(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    HierarchyTraversalOptions options;
    options.includeIndirectDependencies = false;
    return impl_->collectDependencies(schema, object, type, options);
}

Result<std::vector<ObjectReference>> ObjectHierarchy::getDirectDependents(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    HierarchyTraversalOptions options;
    options.includeIndirectDependencies = false;
    return impl_->collectDependents(schema, object, type, options);
}

Result<std::vector<ObjectReference>> ObjectHierarchy::getAllDependencies(
    const std::string& schema, const std::string& object, SchemaObjectType type,
    const HierarchyTraversalOptions& options) {

    auto hierarchyResult = buildHierarchy(schema, object, type, options);
    if (!hierarchyResult.isSuccess()) {
        return Result<std::vector<ObjectReference>>::error(hierarchyResult.error().code, hierarchyResult.error().message);
    }

    std::vector<ObjectReference> allDependencies;
    const auto& hierarchy = hierarchyResult.value();

    // Collect all dependencies from the dependency graph
    for (const auto& [key, refs] : hierarchy.dependencyGraph) {
        allDependencies.insert(allDependencies.end(), refs.begin(), refs.end());
    }

    return Result<std::vector<ObjectReference>>::success(allDependencies);
}

Result<std::vector<ObjectReference>> ObjectHierarchy::getAllDependents(
    const std::string& schema, const std::string& object, SchemaObjectType type,
    const HierarchyTraversalOptions& options) {

    auto hierarchyResult = buildHierarchy(schema, object, type, options);
    if (!hierarchyResult.isSuccess()) {
        return Result<std::vector<ObjectReference>>::error(hierarchyResult.error().code, hierarchyResult.error().message);
    }

    std::vector<ObjectReference> allDependents;
    const auto& hierarchy = hierarchyResult.value();

    // Collect all dependents from the dependent graph
    for (const auto& [key, refs] : hierarchy.dependentGraph) {
        allDependents.insert(allDependents.end(), refs.begin(), refs.end());
    }

    return Result<std::vector<ObjectReference>>::success(allDependents);
}

Result<bool> ObjectHierarchy::hasCircularReference(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    auto hierarchyResult = buildHierarchy(schema, object, type);
    if (!hierarchyResult.isSuccess()) {
        return Result<bool>::error(hierarchyResult.error().code, hierarchyResult.error().message);
    }

    return Result<bool>::success(hierarchyResult.value().hasCircularReferences);
}

Result<std::vector<std::vector<ObjectReference>>> ObjectHierarchy::findCircularReferences(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    auto hierarchyResult = buildHierarchy(schema, object, type);
    if (!hierarchyResult.isSuccess()) {
        return Result<std::vector<std::vector<ObjectReference>>>::error(hierarchyResult.error().code, hierarchyResult.error().message);
    }

    return Result<std::vector<std::vector<ObjectReference>>>::success(
        hierarchyResult.value().circularReferenceChains);
}

Result<ImpactAnalysis> ObjectHierarchy::analyzeImpact(
    const std::string& schema, const std::string& object, SchemaObjectType type,
    const std::string& operation) {

    if (operation == "DELETE") {
        return analyzeDeletionImpact(schema, object, type);
    } else if (operation == "MODIFY") {
        return analyzeModificationImpact(schema, object, type);
    } else if (operation == "CREATE") {
        return analyzeCreationImpact(schema, object, type);
    } else {
        ImpactAnalysis analysis;
        analysis.targetSchema = schema;
        analysis.targetObject = object;
        analysis.targetType = type;
        analysis.warnings.push_back("Unknown operation: " + operation);
        return Result<ImpactAnalysis>::success(analysis);
    }
}

Result<std::vector<std::string>> ObjectHierarchy::getDependencyChain(
    const std::string& fromSchema, const std::string& fromObject, SchemaObjectType fromType,
    const std::string& toSchema, const std::string& toObject, SchemaObjectType toType) {

    // This is a simplified implementation
    // A full implementation would use graph traversal algorithms to find the shortest path
    std::vector<std::string> chain;

    chain.push_back(fromSchema + "." + fromObject + " (" +
                   std::to_string(static_cast<int>(fromType)) + ")");
    chain.push_back(toSchema + "." + toObject + " (" +
                   std::to_string(static_cast<int>(toType)) + ")");

    return Result<std::vector<std::string>>::success(chain);
}

Result<void> ObjectHierarchy::refreshHierarchyCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    hierarchyCache_.clear();
    cacheTimestamps_.clear();
    return Result<void>::success();
}

Result<ObjectHierarchy> ObjectHierarchy::getCachedHierarchy(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    std::string cacheKey = generateHierarchyCacheKey(schema, object, type);

    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = hierarchyCache_.find(cacheKey);
    if (it != hierarchyCache_.end()) {
        HierarchyTraversalOptions defaultOptions;
        if (isHierarchyCacheValid(cacheKey, defaultOptions)) {
            return Result<ObjectHierarchy>::success(it->second);
        }
    }

    // Cache miss - build fresh hierarchy
    auto result = buildHierarchy(schema, object, type);
    if (result.isSuccess()) {
        updateHierarchyCache(cacheKey, result.value());
    }

    return result;
}

Result<void> ObjectHierarchy::traverseHierarchy(
    const std::string& schema, const std::string& object, SchemaObjectType type,
    const HierarchyTraversalOptions& options, TraversalCallback callback) {

    auto hierarchyResult = buildHierarchy(schema, object, type, options);
    if (!hierarchyResult.isSuccess()) {
        return Result<void>::error(hierarchyResult.error().code, hierarchyResult.error().message);
    }

    const auto& hierarchy = hierarchyResult.value();
    std::string rootKey = generateObjectKey(schema, object, type);

    switch (options.traversalType) {
        case HierarchyTraversal::DEPTH_FIRST:
            performDepthFirstTraversal(rootKey, hierarchy, callback, std::unordered_set<std::string>(), 0);
            break;
        case HierarchyTraversal::BREADTH_FIRST:
            performBreadthFirstTraversal(rootKey, hierarchy, callback);
            break;
        case HierarchyTraversal::TOPOLOGICAL:
            performTopologicalSort(hierarchy);
            break;
    }

    return Result<void>::success();
}

// Private helper methods

Result<ImpactAnalysis> ObjectHierarchy::analyzeDeletionImpact(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    ImpactAnalysis analysis;
    analysis.targetSchema = schema;
    analysis.targetObject = object;
    analysis.targetType = type;
    analysis.impactLevel = 3; // High impact
    analysis.hasBreakingChanges = true;

    // Get all dependents
    auto dependentsResult = getAllDependents(schema, object, type);
    if (dependentsResult.isSuccess()) {
        analysis.affectedObjects = dependentsResult.value();
        analysis.impactLevel = std::min(5, 3 + static_cast<int>(analysis.affectedObjects.size() / 10));
    }

    // Generate warnings and recommendations
    if (!analysis.affectedObjects.empty()) {
        analysis.warnings.push_back("Deleting this object will affect " +
                                  std::to_string(analysis.affectedObjects.size()) +
                                  " dependent objects");
        analysis.recommendations.push_back("Consider dropping dependent objects first");
    }

    if (type == SchemaObjectType::TABLE) {
        analysis.migrationSteps.push_back("Export table data before deletion");
        analysis.migrationSteps.push_back("Drop dependent views and constraints");
    }

    return Result<ImpactAnalysis>::success(analysis);
}

Result<ImpactAnalysis> ObjectHierarchy::analyzeModificationImpact(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    ImpactAnalysis analysis;
    analysis.targetSchema = schema;
    analysis.targetObject = object;
    analysis.targetType = type;
    analysis.impactLevel = 2; // Medium impact
    analysis.hasBreakingChanges = false;

    // Get all dependents
    auto dependentsResult = getAllDependents(schema, object, type);
    if (dependentsResult.isSuccess()) {
        analysis.affectedObjects = dependentsResult.value();
        if (!analysis.affectedObjects.empty()) {
            analysis.hasBreakingChanges = true;
            analysis.impactLevel = 3;
        }
    }

    if (type == SchemaObjectType::COLUMN) {
        analysis.warnings.push_back("Column modifications may require data type conversions");
        analysis.recommendations.push_back("Test with a subset of data first");
    }

    return Result<ImpactAnalysis>::success(analysis);
}

Result<ImpactAnalysis> ObjectHierarchy::analyzeCreationImpact(
    const std::string& schema, const std::string& object, SchemaObjectType type) {

    ImpactAnalysis analysis;
    analysis.targetSchema = schema;
    analysis.targetObject = object;
    analysis.targetType = type;
    analysis.impactLevel = 1; // Low impact
    analysis.hasBreakingChanges = false;

    analysis.recommendations.push_back("New object creation is generally safe");
    analysis.recommendations.push_back("Verify object name doesn't conflict with existing objects");

    return Result<ImpactAnalysis>::success(analysis);
}

std::string ObjectHierarchy::generateHierarchyCacheKey(const std::string& schema,
                                                     const std::string& object,
                                                     SchemaObjectType type) {
    return schema + "." + object + "." + std::to_string(static_cast<int>(type));
}

bool ObjectHierarchy::isHierarchyCacheValid(const std::string& cacheKey,
                                          const HierarchyTraversalOptions& options) {
    auto it = cacheTimestamps_.find(cacheKey);
    if (it == cacheTimestamps_.end()) {
        return false;
    }

    // Cache is valid for 10 minutes
    auto age = std::chrono::system_clock::now() - it->second;
    return age < std::chrono::minutes(10);
}

void ObjectHierarchy::updateHierarchyCache(const std::string& cacheKey, const ObjectHierarchy& hierarchy) {
    hierarchyCache_[cacheKey] = hierarchy;
    cacheTimestamps_[cacheKey] = std::chrono::system_clock::now();
}

void ObjectHierarchy::cleanupExpiredHierarchyCache() {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::minutes(30); // Remove cache entries older than 30 minutes

    std::lock_guard<std::mutex> lock(cacheMutex_);
    for (auto it = cacheTimestamps_.begin(); it != cacheTimestamps_.end();) {
        if (it->second < cutoff) {
            hierarchyCache_.erase(it->first);
            it = cacheTimestamps_.erase(it);
        } else {
            ++it;
        }
    }
}

void ObjectHierarchy::performDepthFirstTraversal(const std::string& key,
                                               const ObjectHierarchy& hierarchy,
                                               TraversalCallback callback,
                                               std::unordered_set<std::string>& visited,
                                               int depth) {
    if (visited.count(key)) {
        return;
    }

    visited.insert(key);

    // Call callback for current object
    if (hierarchy.dependencyGraph.count(key)) {
        for (const auto& ref : hierarchy.dependencyGraph.at(key)) {
            callback(ref, depth);
        }
    }

    // Recurse on dependencies
    if (hierarchy.dependencyGraph.count(key)) {
        for (const auto& ref : hierarchy.dependencyGraph.at(key)) {
            std::string nextKey = generateObjectKey(ref.toSchema, ref.toObject, ref.toType);
            performDepthFirstTraversal(nextKey, hierarchy, callback, visited, depth + 1);
        }
    }
}

void ObjectHierarchy::performBreadthFirstTraversal(const std::string& key,
                                                 const ObjectHierarchy& hierarchy,
                                                 TraversalCallback callback) {
    std::queue<std::pair<std::string, int>> queue;
    std::unordered_set<std::string> visited;

    queue.push({key, 0});
    visited.insert(key);

    while (!queue.empty()) {
        auto [currentKey, depth] = queue.front();
        queue.pop();

        // Call callback for current object
        if (hierarchy.dependencyGraph.count(currentKey)) {
            for (const auto& ref : hierarchy.dependencyGraph.at(currentKey)) {
                callback(ref, depth);
            }
        }

        // Add dependencies to queue
        if (hierarchy.dependencyGraph.count(currentKey)) {
            for (const auto& ref : hierarchy.dependencyGraph.at(currentKey)) {
                std::string nextKey = generateObjectKey(ref.toSchema, ref.toObject, ref.toType);
                if (visited.find(nextKey) == visited.end()) {
                    visited.insert(nextKey);
                    queue.push({nextKey, depth + 1});
                }
            }
        }
    }
}

void ObjectHierarchy::performTopologicalSort(ObjectHierarchy& hierarchy) {
    // Implementation would perform topological sorting of the dependency graph
    // This is a placeholder for the actual implementation
}

std::string ObjectHierarchy::generateObjectKey(const std::string& schema,
                                             const std::string& object,
                                             SchemaObjectType type) {
    return schema + "." + object + "." + std::to_string(static_cast<int>(type));
}

bool ObjectHierarchy::isSystemObject(const std::string& schema, const std::string& object) {
    return impl_->isSystemObject(schema, object);
}

bool ObjectHierarchy::shouldIncludeDependency(const ObjectReference& ref,
                                            const HierarchyTraversalOptions& options) {
    return impl_->shouldIncludeDependency(ref, options);
}

} // namespace scratchrobin
