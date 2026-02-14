// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "report_types.h"
#include "../core/project.h"

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace scratchrobin {
namespace reporting {

/**
 * @brief Report repository for managing reporting artifacts
 * 
 * Provides CRUD operations for questions, dashboards, models, metrics,
 * segments, alerts, subscriptions, and collections.
 */
class ReportRepository {
public:
    explicit ReportRepository(Project* project);
    
    // ========================================================================
    // Questions
    // ========================================================================
    Question CreateQuestion(const std::string& name, const CollectionId& collection_id);
    std::optional<Question> GetQuestion(const ReportId& id) const;
    std::optional<Question> GetQuestionByName(const std::string& name, 
                                               const CollectionId& collection_id) const;
    bool SaveQuestion(const Question& question);
    bool DeleteQuestion(const ReportId& id);
    std::vector<Question> GetQuestions(const CollectionId& collection_id = {}) const;
    std::vector<Question> GetQuestionsByCollection(const CollectionId& collection_id) const;
    
    // ========================================================================
    // Dashboards
    // ========================================================================
    Dashboard CreateDashboard(const std::string& name, const CollectionId& collection_id);
    std::optional<Dashboard> GetDashboard(const ReportId& id) const;
    bool SaveDashboard(const Dashboard& dashboard);
    bool DeleteDashboard(const ReportId& id);
    std::vector<Dashboard> GetDashboards(const CollectionId& collection_id = {}) const;
    
    // ========================================================================
    // Models (Semantic Layer)
    // ========================================================================
    Model CreateModel(const std::string& name, const CollectionId& collection_id);
    std::optional<Model> GetModel(const ReportId& id) const;
    std::optional<Model> GetModelByName(const std::string& name) const;
    bool SaveModel(const Model& model);
    bool DeleteModel(const ReportId& id);
    std::vector<Model> GetModels(const CollectionId& collection_id = {}) const;
    
    // ========================================================================
    // Metrics
    // ========================================================================
    Metric CreateMetric(const std::string& name, const CollectionId& collection_id);
    std::optional<Metric> GetMetric(const ReportId& id) const;
    bool SaveMetric(const Metric& metric);
    bool DeleteMetric(const ReportId& id);
    std::vector<Metric> GetMetrics(const CollectionId& collection_id = {}) const;
    
    // ========================================================================
    // Segments
    // ========================================================================
    Segment CreateSegment(const std::string& name, const CollectionId& collection_id);
    std::optional<Segment> GetSegment(const ReportId& id) const;
    bool SaveSegment(const Segment& segment);
    bool DeleteSegment(const ReportId& id);
    std::vector<Segment> GetSegments(const CollectionId& collection_id = {}) const;
    
    // ========================================================================
    // Alerts
    // ========================================================================
    Alert CreateAlert(const std::string& name, const ReportId& question_id);
    std::optional<Alert> GetAlert(const ReportId& id) const;
    bool SaveAlert(const Alert& alert);
    bool DeleteAlert(const ReportId& id);
    std::vector<Alert> GetAlerts() const;
    std::vector<Alert> GetAlertsForQuestion(const ReportId& question_id) const;
    std::vector<Alert> GetEnabledAlerts() const;
    
    // ========================================================================
    // Subscriptions
    // ========================================================================
    Subscription CreateSubscription(const std::string& name, 
                                    const std::string& target_type,
                                    const ReportId& target_id);
    std::optional<Subscription> GetSubscription(const ReportId& id) const;
    bool SaveSubscription(const Subscription& subscription);
    bool DeleteSubscription(const ReportId& id);
    std::vector<Subscription> GetSubscriptions() const;
    std::vector<Subscription> GetDueSubscriptions(const Timestamp& before) const;
    
    // ========================================================================
    // Collections
    // ========================================================================
    Collection CreateCollection(const std::string& name, 
                                 const std::optional<CollectionId>& parent_id = std::nullopt);
    std::optional<Collection> GetCollection(const CollectionId& id) const;
    bool SaveCollection(const Collection& collection);
    bool DeleteCollection(const CollectionId& id);
    std::vector<Collection> GetCollections() const;
    std::vector<Collection> GetRootCollections() const;
    std::vector<Collection> GetChildCollections(const CollectionId& parent_id) const;
    
    // ========================================================================
    // Timelines
    // ========================================================================
    Timeline CreateTimeline(const std::string& name, const CollectionId& collection_id);
    std::optional<Timeline> GetTimeline(const ReportId& id) const;
    bool SaveTimeline(const Timeline& timeline);
    bool DeleteTimeline(const ReportId& id);
    std::vector<Timeline> GetTimelines(const CollectionId& collection_id = {}) const;
    bool AddTimelineEvent(const ReportId& timeline_id, const TimelineEvent& event);
    
    // ========================================================================
    // Import/Export
    // ========================================================================
    std::string ExportToJson() const;
    bool ImportFromJson(const std::string& json);
    
    // Export individual artifacts
    std::string ExportQuestion(const ReportId& id) const;
    std::string ExportDashboard(const ReportId& id) const;
    bool ImportQuestion(const std::string& json);
    bool ImportDashboard(const std::string& json);
    
    // ========================================================================
    // Utilities
    // ========================================================================
    ReportId GenerateId() const;
    bool Exists(const ReportId& id) const;
    std::string GetArtifactType(const ReportId& id) const;
    
    // Move between collections
    bool MoveToCollection(const ReportId& id, const CollectionId& new_collection_id);
    
    // Duplicate
    std::optional<Question> DuplicateQuestion(const ReportId& id, const std::string& new_name);
    std::optional<Dashboard> DuplicateDashboard(const ReportId& id, const std::string& new_name);
    
private:
    Project* project_;
    mutable std::mutex mutex_;
    
    // In-memory indices (persisted to project)
    std::map<ReportId, Question> questions_;
    std::map<ReportId, Dashboard> dashboards_;
    std::map<ReportId, Model> models_;
    std::map<ReportId, Metric> metrics_;
    std::map<ReportId, Segment> segments_;
    std::map<ReportId, Alert> alerts_;
    std::map<ReportId, Subscription> subscriptions_;
    std::map<CollectionId, Collection> collections_;
    std::map<ReportId, Timeline> timelines_;
    
    // Persistence helpers
    void LoadFromProject();
    void SaveToProject();
    
    template<typename T>
    bool SaveArtifact(const T& artifact, const std::string& kind);
    
    template<typename T>
    std::optional<T> GetArtifact(const ReportId& id, const std::map<ReportId, T>& index) const;
};

/**
 * @brief Repository access for a specific collection scope
 */
class ScopedRepository {
public:
    ScopedRepository(ReportRepository* repository, const CollectionId& collection_id);
    
    std::vector<Question> GetQuestions() const;
    std::vector<Dashboard> GetDashboards() const;
    std::vector<Model> GetModels() const;
    
    Question CreateQuestion(const std::string& name);
    Dashboard CreateDashboard(const std::string& name);
    
private:
    ReportRepository* repository_;
    CollectionId collection_id_;
};

} // namespace reporting
} // namespace scratchrobin
