#ifndef SCRATCHROBIN_OBJECT_HIERARCHY_H
#define SCRATCHROBIN_OBJECT_HIERARCHY_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <functional>
#include <mutex>
#include "metadata/schema_collector.h"

namespace scratchrobin {

enum class HierarchyTraversal {
    DEPTH_FIRST,
    BREADTH_FIRST,
    TOPOLOGICAL
};

enum class DependencyType {
    HARD_DEPENDENCY,      // Object cannot exist without dependency
    SOFT_DEPENDENCY,      // Object can exist but functionality is limited
    REFERENCE_DEPENDENCY, // Object references another object
    INHERITANCE,          // Object inherits from another object
    COMPOSITION,          // Object is composed of other objects
    ASSOCIATION           // Object is associated with other objects
};

struct ObjectReference {
    std::string fromSchema;
    std::string fromObject;
    SchemaObjectType fromType;
    std::string toSchema;
    std::string toObject;
    SchemaObjectType toType;
    DependencyType dependencyType;
    std::string referenceName;
    std::string description;
    bool isCircular = false;
    int dependencyLevel = 0;
};

struct ObjectHierarchyInfo {
    std::string rootSchema;
    std::string rootObject;
    SchemaObjectType rootType;
    std::vector<ObjectReference> directDependencies;
    std::vector<ObjectReference> directDependents;
    std::unordered_map<std::string, int> dependencyLevels;
    std::unordered_map<std::string, std::vector<ObjectReference>> dependencyGraph;
    std::unordered_map<std::string, std::vector<ObjectReference>> dependentGraph;
    bool hasCircularReferences = false;
    std::vector<std::vector<ObjectReference>> circularReferenceChains;
    int maxDepth = 0;
    int totalObjects = 0;
};

struct HierarchyTraversalOptions {
    HierarchyTraversal traversalType = HierarchyTraversal::DEPTH_FIRST;
    bool includeIndirectDependencies = true;
    bool includeSoftDependencies = false;
    bool includeReferenceDependencies = false;
    bool followInheritance = true;
    bool followComposition = true;
    int maxDepth = 10;
    int maxObjects = 1000;
    bool detectCircularReferences = true;
    bool includeSystemObjects = false;
    std::vector<DependencyType> includedDependencyTypes;
    std::vector<DependencyType> excludedDependencyTypes;
};

struct ImpactAnalysis {
    std::string targetSchema;
    std::string targetObject;
    SchemaObjectType targetType;
    std::vector<ObjectReference> affectedObjects;
    std::vector<ObjectReference> cascadingEffects;
    int impactLevel = 0;
    bool hasBreakingChanges = false;
    bool requiresMigration = false;
    std::vector<std::string> migrationSteps;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
};

class IObjectHierarchy {
public:
    virtual ~IObjectHierarchy() = default;

    virtual Result<ObjectHierarchyInfo> buildHierarchy(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options = {}) = 0;

    virtual Result<std::vector<ObjectReference>> getDirectDependencies(
        const std::string& schema, const std::string& object, SchemaObjectType type) = 0;

    virtual Result<std::vector<ObjectReference>> getDirectDependents(
        const std::string& schema, const std::string& object, SchemaObjectType type) = 0;

    virtual Result<std::vector<ObjectReference>> getAllDependencies(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options = {}) = 0;

    virtual Result<std::vector<ObjectReference>> getAllDependents(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options = {}) = 0;

    virtual Result<bool> hasCircularReference(
        const std::string& schema, const std::string& object, SchemaObjectType type) = 0;

    virtual Result<std::vector<std::vector<ObjectReference>>> findCircularReferences(
        const std::string& schema, const std::string& object, SchemaObjectType type) = 0;

    virtual Result<ImpactAnalysis> analyzeImpact(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const std::string& operation) = 0;

    virtual Result<std::vector<std::string>> getDependencyChain(
        const std::string& fromSchema, const std::string& fromObject, SchemaObjectType fromType,
        const std::string& toSchema, const std::string& toObject, SchemaObjectType toType) = 0;

    virtual Result<void> refreshHierarchyCache() = 0;
    virtual Result<ObjectHierarchyInfo> getCachedHierarchy(
        const std::string& schema, const std::string& object, SchemaObjectType type) = 0;

    using TraversalCallback = std::function<void(const ObjectReference&, int)>;
    virtual Result<void> traverseHierarchy(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options, TraversalCallback callback) = 0;
};

class ObjectHierarchy : public IObjectHierarchy {
public:
    ObjectHierarchy(std::shared_ptr<ISchemaCollector> schemaCollector);
    ~ObjectHierarchy() override;

    Result<ObjectHierarchyInfo> buildHierarchy(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options = {}) override;

    Result<std::vector<ObjectReference>> getDirectDependencies(
        const std::string& schema, const std::string& object, SchemaObjectType type) override;

    Result<std::vector<ObjectReference>> getDirectDependents(
        const std::string& schema, const std::string& object, SchemaObjectType type) override;

    Result<std::vector<ObjectReference>> getAllDependencies(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options = {}) override;

    Result<std::vector<ObjectReference>> getAllDependents(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options = {}) override;

    Result<bool> hasCircularReference(
        const std::string& schema, const std::string& object, SchemaObjectType type) override;

    Result<std::vector<std::vector<ObjectReference>>> findCircularReferences(
        const std::string& schema, const std::string& object, SchemaObjectType type) override;

    Result<ImpactAnalysis> analyzeImpact(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const std::string& operation) override;

    Result<std::vector<std::string>> getDependencyChain(
        const std::string& fromSchema, const std::string& fromObject, SchemaObjectType fromType,
        const std::string& toSchema, const std::string& toObject, SchemaObjectType toType) override;

    Result<void> refreshHierarchyCache() override;
    Result<ObjectHierarchyInfo> getCachedHierarchy(
        const std::string& schema, const std::string& object, SchemaObjectType type) override;

    Result<void> traverseHierarchy(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options, TraversalCallback callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::shared_ptr<ISchemaCollector> schemaCollector_;

    mutable std::mutex cacheMutex_;
    std::unordered_map<std::string, ObjectHierarchyInfo> hierarchyCache_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> cacheTimestamps_;

    // Helper methods for building hierarchies
    Result<ObjectHierarchyInfo> buildHierarchyInternal(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options);

    Result<std::vector<ObjectReference>> collectDependencies(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options);

    Result<std::vector<ObjectReference>> collectDependents(
        const std::string& schema, const std::string& object, SchemaObjectType type,
        const HierarchyTraversalOptions& options);

    void buildDependencyGraph(ObjectHierarchyInfo& hierarchy,
                            const std::vector<ObjectReference>& dependencies,
                            const std::vector<ObjectReference>& dependents);

    void detectCircularReferences(ObjectHierarchyInfo& hierarchy);
    void findCircularReferenceChains(ObjectHierarchyInfo& hierarchy,
                                   const std::string& startKey,
                                   std::unordered_set<std::string>& visited,
                                   std::vector<ObjectReference>& currentChain);

    void calculateDependencyLevels(const ObjectHierarchyInfo& hierarchy);
    void performTopologicalSort(const ObjectHierarchyInfo& hierarchy);
    void performDepthFirstTraversal(const std::string& key,
                                  const ObjectHierarchyInfo& hierarchy,
                                  TraversalCallback callback,
                                  std::unordered_set<std::string>& visited,
                                  int depth);
    void performBreadthFirstTraversal(const std::string& key,
                                    const ObjectHierarchyInfo& hierarchy,
                                    TraversalCallback callback,
                                    const std::unordered_set<std::string>& visited);

    // Impact analysis methods
    Result<ImpactAnalysis> analyzeDeletionImpact(
        const std::string& schema, const std::string& object, SchemaObjectType type);
    Result<ImpactAnalysis> analyzeModificationImpact(
        const std::string& schema, const std::string& object, SchemaObjectType type);
    Result<ImpactAnalysis> analyzeCreationImpact(
        const std::string& schema, const std::string& object, SchemaObjectType type);

    // Cache management
    std::string generateHierarchyCacheKey(const std::string& schema,
                                        const std::string& object,
                                        SchemaObjectType type);
    bool isHierarchyCacheValid(const std::string& cacheKey,
                             const HierarchyTraversalOptions& options);
    void updateHierarchyCache(const std::string& cacheKey, const ObjectHierarchyInfo& hierarchy);
    void cleanupExpiredHierarchyCache();

    // Utility methods
    std::string generateObjectKey(const std::string& schema,
                                const std::string& object,
                                SchemaObjectType type);
    bool isSystemObject(const std::string& schema, const std::string& object);
    bool shouldIncludeDependency(const ObjectReference& ref,
                               const HierarchyTraversalOptions& options);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_OBJECT_HIERARCHY_H
