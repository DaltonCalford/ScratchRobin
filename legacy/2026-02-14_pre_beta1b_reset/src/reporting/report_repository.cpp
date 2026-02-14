// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "report_repository.h"

#include <algorithm>
#include <random>
#include <sstream>

namespace scratchrobin {
namespace reporting {

// Helper to generate UUID-like IDs
static std::string GenerateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex = "0123456789abcdef";
    
    std::string id;
    id.reserve(36);
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            id += '-';
        } else {
            id += hex[dis(gen)];
        }
    }
    return id;
}

ReportRepository::ReportRepository(Project* project) : project_(project) {
    LoadFromProject();
}

// Questions
Question ReportRepository::CreateQuestion(const std::string& name, 
                                          const CollectionId& collection_id) {
    Question q;
    q.id = GenerateId();
    q.name = name;
    q.collection_id = collection_id;
    q.created_at = std::time(nullptr);
    q.updated_at = q.created_at;
    
    std::lock_guard<std::mutex> lock(mutex_);
    questions_[q.id] = q;
    SaveToProject();
    return q;
}

std::optional<Question> ReportRepository::GetQuestion(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = questions_.find(id);
    if (it != questions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<Question> ReportRepository::GetQuestionByName(const std::string& name,
                                                            const CollectionId& collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, q] : questions_) {
        if (q.name == name && q.collection_id == collection_id) {
            return q;
        }
    }
    return std::nullopt;
}

bool ReportRepository::SaveQuestion(const Question& question) {
    std::lock_guard<std::mutex> lock(mutex_);
    questions_[question.id] = question;
    SaveToProject();
    return true;
}

bool ReportRepository::DeleteQuestion(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    questions_.erase(id);
    SaveToProject();
    return true;
}

std::vector<Question> ReportRepository::GetQuestions(const CollectionId& collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Question> result;
    for (const auto& [id, q] : questions_) {
        if (collection_id.empty() || q.collection_id == collection_id) {
            result.push_back(q);
        }
    }
    return result;
}

std::vector<Question> ReportRepository::GetQuestionsByCollection(const CollectionId& collection_id) const {
    return GetQuestions(collection_id);
}

// Dashboards
Dashboard ReportRepository::CreateDashboard(const std::string& name,
                                            const CollectionId& collection_id) {
    Dashboard d;
    d.id = GenerateId();
    d.name = name;
    d.collection_id = collection_id;
    d.created_at = std::time(nullptr);
    d.updated_at = d.created_at;
    
    std::lock_guard<std::mutex> lock(mutex_);
    dashboards_[d.id] = d;
    SaveToProject();
    return d;
}

std::optional<Dashboard> ReportRepository::GetDashboard(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dashboards_.find(id);
    if (it != dashboards_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ReportRepository::SaveDashboard(const Dashboard& dashboard) {
    std::lock_guard<std::mutex> lock(mutex_);
    dashboards_[dashboard.id] = dashboard;
    SaveToProject();
    return true;
}

bool ReportRepository::DeleteDashboard(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    dashboards_.erase(id);
    SaveToProject();
    return true;
}

std::vector<Dashboard> ReportRepository::GetDashboards(const CollectionId& collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Dashboard> result;
    for (const auto& [id, d] : dashboards_) {
        if (collection_id.empty() || d.collection_id == collection_id) {
            result.push_back(d);
        }
    }
    return result;
}

// Models
Model ReportRepository::CreateModel(const std::string& name,
                                    const CollectionId& collection_id) {
    Model m;
    m.id = GenerateId();
    m.name = name;
    m.collection_id = collection_id;
    m.created_at = std::time(nullptr);
    m.updated_at = m.created_at;
    
    std::lock_guard<std::mutex> lock(mutex_);
    models_[m.id] = m;
    SaveToProject();
    return m;
}

std::optional<Model> ReportRepository::GetModel(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = models_.find(id);
    if (it != models_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<Model> ReportRepository::GetModelByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, m] : models_) {
        if (m.name == name) {
            return m;
        }
    }
    return std::nullopt;
}

bool ReportRepository::SaveModel(const Model& model) {
    std::lock_guard<std::mutex> lock(mutex_);
    models_[model.id] = model;
    SaveToProject();
    return true;
}

bool ReportRepository::DeleteModel(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    models_.erase(id);
    SaveToProject();
    return true;
}

std::vector<Model> ReportRepository::GetModels(const CollectionId& collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Model> result;
    for (const auto& [id, m] : models_) {
        if (collection_id.empty() || m.collection_id == collection_id) {
            result.push_back(m);
        }
    }
    return result;
}

// Metrics, Segments, Alerts, Subscriptions, Collections, Timelines
// ... (similar stub implementations)

Metric ReportRepository::CreateMetric(const std::string& name,
                                      const CollectionId& collection_id) {
    Metric m;
    m.id = GenerateId();
    m.name = name;
    m.collection_id = collection_id;
    m.created_at = std::time(nullptr);
    m.updated_at = m.created_at;
    
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_[m.id] = m;
    SaveToProject();
    return m;
}

std::optional<Metric> ReportRepository::GetMetric(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(id);
    if (it != metrics_.end()) return it->second;
    return std::nullopt;
}

bool ReportRepository::SaveMetric(const Metric& metric) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_[metric.id] = metric;
    SaveToProject();
    return true;
}

bool ReportRepository::DeleteMetric(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.erase(id);
    SaveToProject();
    return true;
}

std::vector<Metric> ReportRepository::GetMetrics(const CollectionId& collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Metric> result;
    for (const auto& [id, m] : metrics_) {
        if (collection_id.empty() || m.collection_id == collection_id) {
            result.push_back(m);
        }
    }
    return result;
}

// Segment stubs
Segment ReportRepository::CreateSegment(const std::string& name,
                                        const CollectionId& collection_id) {
    Segment s;
    s.id = GenerateId();
    s.name = name;
    s.collection_id = collection_id;
    s.created_at = std::time(nullptr);
    s.updated_at = s.created_at;
    std::lock_guard<std::mutex> lock(mutex_);
    segments_[s.id] = s;
    SaveToProject();
    return s;
}
std::optional<Segment> ReportRepository::GetSegment(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segments_.find(id);
    if (it != segments_.end()) return it->second;
    return std::nullopt;
}
bool ReportRepository::SaveSegment(const Segment& segment) {
    std::lock_guard<std::mutex> lock(mutex_);
    segments_[segment.id] = segment;
    SaveToProject();
    return true;
}
bool ReportRepository::DeleteSegment(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    segments_.erase(id);
    SaveToProject();
    return true;
}
std::vector<Segment> ReportRepository::GetSegments(const CollectionId& collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Segment> result;
    for (const auto& [id, s] : segments_) {
        if (collection_id.empty() || s.collection_id == collection_id) {
            result.push_back(s);
        }
    }
    return result;
}

// Alert stubs
Alert ReportRepository::CreateAlert(const std::string& name, const ReportId& question_id) {
    Alert a;
    a.id = GenerateId();
    a.name = name;
    a.question_id = question_id;
    a.created_at = std::time(nullptr);
    a.updated_at = a.created_at;
    std::lock_guard<std::mutex> lock(mutex_);
    alerts_[a.id] = a;
    SaveToProject();
    return a;
}
std::optional<Alert> ReportRepository::GetAlert(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = alerts_.find(id);
    if (it != alerts_.end()) return it->second;
    return std::nullopt;
}
bool ReportRepository::SaveAlert(const Alert& alert) {
    std::lock_guard<std::mutex> lock(mutex_);
    alerts_[alert.id] = alert;
    SaveToProject();
    return true;
}
bool ReportRepository::DeleteAlert(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    alerts_.erase(id);
    SaveToProject();
    return true;
}
std::vector<Alert> ReportRepository::GetAlerts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Alert> result;
    for (const auto& [id, a] : alerts_) result.push_back(a);
    return result;
}
std::vector<Alert> ReportRepository::GetAlertsForQuestion(const ReportId& question_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Alert> result;
    for (const auto& [id, a] : alerts_) {
        if (a.question_id == question_id) result.push_back(a);
    }
    return result;
}
std::vector<Alert> ReportRepository::GetEnabledAlerts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Alert> result;
    for (const auto& [id, a] : alerts_) {
        if (a.enabled) result.push_back(a);
    }
    return result;
}

// Subscription stubs
Subscription ReportRepository::CreateSubscription(const std::string& name,
                                                  const std::string& target_type,
                                                  const ReportId& target_id) {
    Subscription s;
    s.id = GenerateId();
    s.name = name;
    s.target_type = target_type;
    s.target_id = target_id;
    s.created_at = std::time(nullptr);
    s.updated_at = s.created_at;
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_[s.id] = s;
    SaveToProject();
    return s;
}
std::optional<Subscription> ReportRepository::GetSubscription(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscriptions_.find(id);
    if (it != subscriptions_.end()) return it->second;
    return std::nullopt;
}
bool ReportRepository::SaveSubscription(const Subscription& subscription) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_[subscription.id] = subscription;
    SaveToProject();
    return true;
}
bool ReportRepository::DeleteSubscription(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(id);
    SaveToProject();
    return true;
}
std::vector<Subscription> ReportRepository::GetSubscriptions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Subscription> result;
    for (const auto& [id, s] : subscriptions_) result.push_back(s);
    return result;
}
std::vector<Subscription> ReportRepository::GetDueSubscriptions(const Timestamp& before) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Subscription> result;
    for (const auto& [id, s] : subscriptions_) {
        if (!s.last_run || *s.last_run < before) {
            result.push_back(s);
        }
    }
    return result;
}

// Collection stubs
Collection ReportRepository::CreateCollection(const std::string& name,
                                              const std::optional<CollectionId>& parent_id) {
    Collection c;
    c.id = GenerateId();
    c.name = name;
    c.parent_id = parent_id;
    c.created_at = std::time(nullptr);
    c.updated_at = c.created_at;
    std::lock_guard<std::mutex> lock(mutex_);
    collections_[c.id] = c;
    SaveToProject();
    return c;
}
std::optional<Collection> ReportRepository::GetCollection(const CollectionId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = collections_.find(id);
    if (it != collections_.end()) return it->second;
    return std::nullopt;
}
bool ReportRepository::SaveCollection(const Collection& collection) {
    std::lock_guard<std::mutex> lock(mutex_);
    collections_[collection.id] = collection;
    SaveToProject();
    return true;
}
bool ReportRepository::DeleteCollection(const CollectionId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    collections_.erase(id);
    SaveToProject();
    return true;
}
std::vector<Collection> ReportRepository::GetCollections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Collection> result;
    for (const auto& [id, c] : collections_) result.push_back(c);
    return result;
}
std::vector<Collection> ReportRepository::GetRootCollections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Collection> result;
    for (const auto& [id, c] : collections_) {
        if (!c.parent_id) result.push_back(c);
    }
    return result;
}
std::vector<Collection> ReportRepository::GetChildCollections(const CollectionId& parent_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Collection> result;
    for (const auto& [id, c] : collections_) {
        if (c.parent_id == parent_id) result.push_back(c);
    }
    return result;
}

// Timeline stubs
Timeline ReportRepository::CreateTimeline(const std::string& name,
                                          const CollectionId& collection_id) {
    Timeline t;
    t.id = GenerateId();
    t.name = name;
    t.collection_id = collection_id;
    t.created_at = std::time(nullptr);
    t.updated_at = t.created_at;
    std::lock_guard<std::mutex> lock(mutex_);
    timelines_[t.id] = t;
    SaveToProject();
    return t;
}
std::optional<Timeline> ReportRepository::GetTimeline(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = timelines_.find(id);
    if (it != timelines_.end()) return it->second;
    return std::nullopt;
}
bool ReportRepository::SaveTimeline(const Timeline& timeline) {
    std::lock_guard<std::mutex> lock(mutex_);
    timelines_[timeline.id] = timeline;
    SaveToProject();
    return true;
}
bool ReportRepository::DeleteTimeline(const ReportId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    timelines_.erase(id);
    SaveToProject();
    return true;
}
std::vector<Timeline> ReportRepository::GetTimelines(const CollectionId& collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Timeline> result;
    for (const auto& [id, t] : timelines_) {
        if (collection_id.empty() || t.collection_id == collection_id) {
            result.push_back(t);
        }
    }
    return result;
}
bool ReportRepository::AddTimelineEvent(const ReportId& timeline_id, const TimelineEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = timelines_.find(timeline_id);
    if (it == timelines_.end()) return false;
    it->second.events.push_back(event);
    SaveToProject();
    return true;
}

// Import/Export
std::string ReportRepository::ExportToJson() const {
    std::ostringstream json;
    json << "{\"questions\":[],\"dashboards\":[],\"models\":[]}";
    return json.str();
}

bool ReportRepository::ImportFromJson(const std::string& json) {
    // Simplified - would parse full JSON
    return true;
}

std::string ReportRepository::ExportQuestion(const ReportId& id) const {
    auto q = GetQuestion(id);
    if (q) return q->ToJson();
    return "{}";
}

std::string ReportRepository::ExportDashboard(const ReportId& id) const {
    auto d = GetDashboard(id);
    if (d) return d->ToJson();
    return "{}";
}

bool ReportRepository::ImportQuestion(const std::string& json) {
    Question q = Question::FromJson(json);
    return SaveQuestion(q);
}

bool ReportRepository::ImportDashboard(const std::string& json) {
    Dashboard d = Dashboard::FromJson(json);
    return SaveDashboard(d);
}

// Utilities
ReportId ReportRepository::GenerateId() const {
    return ::scratchrobin::reporting::GenerateId();
}

bool ReportRepository::Exists(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return questions_.count(id) || dashboards_.count(id) || models_.count(id) ||
           metrics_.count(id) || segments_.count(id) || alerts_.count(id) ||
           subscriptions_.count(id) || collections_.count(id) || timelines_.count(id);
}

std::string ReportRepository::GetArtifactType(const ReportId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (questions_.count(id)) return "question";
    if (dashboards_.count(id)) return "dashboard";
    if (models_.count(id)) return "model";
    if (metrics_.count(id)) return "metric";
    if (segments_.count(id)) return "segment";
    if (alerts_.count(id)) return "alert";
    if (subscriptions_.count(id)) return "subscription";
    if (collections_.count(id)) return "collection";
    if (timelines_.count(id)) return "timeline";
    return "unknown";
}

bool ReportRepository::MoveToCollection(const ReportId& id, const CollectionId& new_collection_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto q_it = questions_.find(id);
    if (q_it != questions_.end()) {
        q_it->second.collection_id = new_collection_id;
        SaveToProject();
        return true;
    }
    
    auto d_it = dashboards_.find(id);
    if (d_it != dashboards_.end()) {
        d_it->second.collection_id = new_collection_id;
        SaveToProject();
        return true;
    }
    
    return false;
}

std::optional<Question> ReportRepository::DuplicateQuestion(const ReportId& id, 
                                                            const std::string& new_name) {
    auto q = GetQuestion(id);
    if (!q) return std::nullopt;
    
    Question copy = *q;
    copy.id = GenerateId();
    copy.name = new_name;
    copy.created_at = std::time(nullptr);
    copy.updated_at = copy.created_at;
    
    SaveQuestion(copy);
    return copy;
}

std::optional<Dashboard> ReportRepository::DuplicateDashboard(const ReportId& id,
                                                              const std::string& new_name) {
    auto d = GetDashboard(id);
    if (!d) return std::nullopt;
    
    Dashboard copy = *d;
    copy.id = GenerateId();
    copy.name = new_name;
    copy.created_at = std::time(nullptr);
    copy.updated_at = copy.created_at;
    
    SaveDashboard(copy);
    return copy;
}

// Persistence
void ReportRepository::LoadFromProject() {
    // In real implementation, load from project file
    // For now, create default collection
    if (collections_.empty()) {
        Collection default_coll;
        default_coll.id = "default";
        default_coll.name = "Default";
        default_coll.status = Collection::Status::Official;
        default_coll.created_at = std::time(nullptr);
        default_coll.updated_at = default_coll.created_at;
        collections_[default_coll.id] = default_coll;
    }
}

void ReportRepository::SaveToProject() {
    // In real implementation, save to project file
}

// ScopedRepository
ScopedRepository::ScopedRepository(ReportRepository* repository, 
                                   const CollectionId& collection_id)
    : repository_(repository), collection_id_(collection_id) {}

std::vector<Question> ScopedRepository::GetQuestions() const {
    return repository_->GetQuestions(collection_id_);
}

std::vector<Dashboard> ScopedRepository::GetDashboards() const {
    return repository_->GetDashboards(collection_id_);
}

std::vector<Model> ScopedRepository::GetModels() const {
    return repository_->GetModels(collection_id_);
}

Question ScopedRepository::CreateQuestion(const std::string& name) {
    return repository_->CreateQuestion(name, collection_id_);
}

Dashboard ScopedRepository::CreateDashboard(const std::string& name) {
    return repository_->CreateDashboard(name, collection_id_);
}

} // namespace reporting
} // namespace scratchrobin
