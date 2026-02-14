// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "report_types.h"

#include <sstream>

namespace scratchrobin {
namespace reporting {

// Question serialization
std::string Question::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    if (description) json << "\"description\":\"" << *description << "\",";
    json << "\"collection_id\":\"" << collection_id << "\",";
    json << "\"sql_mode\":" << (sql_mode ? "true" : "false") << ",";
    json << "\"created_at\":" << created_at << ",";
    json << "\"updated_at\":" << updated_at;
    json << "}";
    return json.str();
}

Question Question::FromJson(const std::string& json) {
    Question q;
    // Simplified parsing - full implementation would use proper JSON parser
    return q;
}

// Dashboard serialization
std::string Dashboard::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    json << "\"collection_id\":\"" << collection_id << "\",";
    json << "\"cards\":[";
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) json << ",";
        json << "{\"id\":\"" << cards[i].id << "\",";
        json << "\"question_id\":\"" << cards[i].question_id << "\"}";
    }
    json << "],";
    json << "\"created_at\":" << created_at << ",";
    json << "\"updated_at\":" << updated_at;
    json << "}";
    return json.str();
}

Dashboard Dashboard::FromJson(const std::string& json) {
    Dashboard d;
    return d;
}

// Model serialization
std::string Model::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    json << "\"collection_id\":\"" << collection_id << "\",";
    json << "\"source\":\"" << source << "\"";
    json << "}";
    return json.str();
}

Model Model::FromJson(const std::string& json) {
    Model m;
    return m;
}

// Metric serialization
std::string Metric::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    json << "\"expression\":\"" << expression << "\"";
    json << "}";
    return json.str();
}

Metric Metric::FromJson(const std::string& json) {
    Metric m;
    return m;
}

// Segment serialization
std::string Segment::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    json << "\"expression\":\"" << expression << "\"";
    json << "}";
    return json.str();
}

Segment Segment::FromJson(const std::string& json) {
    Segment s;
    return s;
}

// Alert serialization
std::string Alert::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    json << "\"question_id\":\"" << question_id << "\",";
    json << "\"enabled\":" << (enabled ? "true" : "false") << ",";
    json << "\"schedule\":\"" << schedule << "\"";
    json << "}";
    return json.str();
}

Alert Alert::FromJson(const std::string& json) {
    Alert a;
    return a;
}

// Subscription serialization
std::string Subscription::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    json << "\"target_type\":\"" << target_type << "\",";
    json << "\"target_id\":\"" << target_id << "\",";
    json << "\"enabled\":" << (enabled ? "true" : "false");
    json << "}";
    return json.str();
}

Subscription Subscription::FromJson(const std::string& json) {
    Subscription s;
    return s;
}

// Collection serialization
std::string Collection::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    if (parent_id) json << "\"parent_id\":\"" << *parent_id << "\",";
    json << "\"status\":\"" << (status == Status::Official ? "official" : 
                                  status == Status::Community ? "community" : "archived") << "\"";
    json << "}";
    return json.str();
}

Collection Collection::FromJson(const std::string& json) {
    Collection c;
    return c;
}

// Timeline serialization
std::string Timeline::ToJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\"";
    json << "}";
    return json.str();
}

Timeline Timeline::FromJson(const std::string& json) {
    Timeline t;
    return t;
}

} // namespace reporting
} // namespace scratchrobin
