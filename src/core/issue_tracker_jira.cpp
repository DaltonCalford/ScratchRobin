/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "issue_tracker_jira.h"
#include "simple_json.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// Jira Adapter Implementation
// ============================================================================
bool JiraAdapter::Initialize(const TrackerConfig& config) {
    config_ = config;
    
    // Validate configuration
    if (config_.base_url.empty()) {
        return false;
    }
    
    // Normalize base URL
    base_url_ = config_.base_url;
    if (base_url_.back() == '/') {
        base_url_.pop_back();
    }
    
    // Build auth header
    auth_header_ = BuildAuthHeader(config_.auth);
    if (auth_header_.empty()) {
        return false;
    }
    
    return true;
}

bool JiraAdapter::TestConnection() {
    // Try to get server info
    auto response = HttpGet("/rest/api/3/serverInfo");
    return response.status_code == 200;
}

std::string JiraAdapter::BuildAuthHeader(const TrackerConfig::Auth& auth) {
    if (auth.type == "api_token") {
        // Jira Cloud: email + API token
        std::string credentials = auth.email + ":" + auth.token;
        // Base64 encode
        static const char base64_chars[] = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        int i = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        
        for (size_t in_len = credentials.length(), pos = 0; pos < in_len; ++pos) {
            char_array_3[i++] = credentials[pos];
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for (int j = 0; j < 4; j++)
                    encoded += base64_chars[char_array_4[j]];
                i = 0;
            }
        }
        
        if (i) {
            for (int j = i; j < 3; j++)
                char_array_3[j] = '\0';
            
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            
            for (int j = 0; j < (i + 1); j++)
                encoded += base64_chars[char_array_4[j]];
            
            while ((i++ < 3))
                encoded += '=';
        }
        
        return "Basic " + encoded;
    } else if (auth.type == "personal_token") {
        // Jira Server/Data Center: Personal Access Token
        return "Bearer " + auth.token;
    }
    return "";
}

JiraAdapter::HttpResponse JiraAdapter::HttpGet(const std::string& path) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-H \"Authorization: " << auth_header_ << "\" ";
    cmd << "-H \"Accept: application/json\" ";
    cmd << "\"" << base_url_ << path << "\"";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        response.error = "Failed to execute HTTP request";
        return response;
    }
    
    char buffer[8192];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    pclose(pipe);
    
    // Parse response
    size_t last_newline = output.rfind('\n');
    if (last_newline != std::string::npos && last_newline + 1 < output.length()) {
        std::string code_str = output.substr(last_newline + 1);
        code_str.erase(code_str.find_last_not_of(" \t\n\r") + 1);
        try {
            response.status_code = std::stoi(code_str);
        } catch (...) {
            response.status_code = 0;
        }
        response.body = output.substr(0, last_newline);
    } else {
        response.body = output;
        response.status_code = 200;
    }
    
    return response;
}

JiraAdapter::HttpResponse JiraAdapter::HttpPost(const std::string& path, 
                                                 const std::string& body) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-X POST ";
    cmd << "-H \"Authorization: " << auth_header_ << "\" ";
    cmd << "-H \"Content-Type: application/json\" ";
    cmd << "-H \"Accept: application/json\" ";
    
    // Escape body
    std::string escaped = body;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    
    cmd << "-d \"" << escaped << "\" ";
    cmd << "\"" << base_url_ << path << "\"";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        response.error = "Failed to execute HTTP request";
        return response;
    }
    
    char buffer[8192];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    pclose(pipe);
    
    size_t last_newline = output.rfind('\n');
    if (last_newline != std::string::npos && last_newline + 1 < output.length()) {
        std::string code_str = output.substr(last_newline + 1);
        code_str.erase(code_str.find_last_not_of(" \t\n\r") + 1);
        try {
            response.status_code = std::stoi(code_str);
        } catch (...) {
            response.status_code = 0;
        }
        response.body = output.substr(0, last_newline);
    } else {
        response.body = output;
        response.status_code = 200;
    }
    
    return response;
}

JiraAdapter::HttpResponse JiraAdapter::HttpPut(const std::string& path,
                                                const std::string& body) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-X PUT ";
    cmd << "-H \"Authorization: " << auth_header_ << "\" ";
    cmd << "-H \"Content-Type: application/json\" ";
    
    std::string escaped = body;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    
    cmd << "-d \"" << escaped << "\" ";
    cmd << "\"" << base_url_ << path << "\"";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        response.error = "Failed to execute HTTP request";
        return response;
    }
    
    char buffer[8192];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    pclose(pipe);
    
    size_t last_newline = output.rfind('\n');
    if (last_newline != std::string::npos && last_newline + 1 < output.length()) {
        std::string code_str = output.substr(last_newline + 1);
        code_str.erase(code_str.find_last_not_of(" \t\n\r") + 1);
        try {
            response.status_code = std::stoi(code_str);
        } catch (...) {
            response.status_code = 0;
        }
        response.body = output.substr(0, last_newline);
    } else {
        response.body = output;
        response.status_code = 200;
    }
    
    return response;
}

JiraAdapter::HttpResponse JiraAdapter::HttpDelete(const std::string& path) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-X DELETE ";
    cmd << "-H \"Authorization: " << auth_header_ << "\" ";
    cmd << "\"" << base_url_ << path << "\"";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        response.error = "Failed to execute HTTP request";
        return response;
    }
    
    char buffer[8192];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    pclose(pipe);
    
    size_t last_newline = output.rfind('\n');
    if (last_newline != std::string::npos && last_newline + 1 < output.length()) {
        std::string code_str = output.substr(last_newline + 1);
        code_str.erase(code_str.find_last_not_of(" \t\n\r") + 1);
        try {
            response.status_code = std::stoi(code_str);
        } catch (...) {
            response.status_code = 0;
        }
        response.body = output.substr(0, last_newline);
    } else {
        response.body = output;
        response.status_code = 200;
    }
    
    return response;
}

IssueReference JiraAdapter::CreateIssue(const IssueCreateRequest& request) {
    IssueReference issue;
    
    std::ostringstream oss;
    oss << "{\"fields\":{\"project\":{\"key\":\"" << config_.project_key << "\"},";
    oss << "\"summary\":\"" << request.title << "\",";
    oss << "\"description\":\"" << request.description << "\",";
    oss << "\"issuetype\":{\"name\":\"" << config_.default_issue_type << "\"}}}";
    
    auto response = HttpPost("/rest/api/3/issue", oss.str());
    
    if (response.status_code == 201) {
        return ParseIssue(response.body);
    }
    
    return issue;
}

bool JiraAdapter::UpdateIssue(const std::string& issue_id,
                               const IssueUpdateRequest& request) {
    std::ostringstream oss;
    oss << "{\"fields\":{";
    
    bool first = true;
    if (request.title.has_value()) {
        if (!first) oss << ",";
        first = false;
        oss << "\"summary\":\"" << *request.title << "\"";
    }
    
    if (request.description.has_value()) {
        if (!first) oss << ",";
        first = false;
        oss << "\"description\":\"" << *request.description << "\"";
    }
    
    oss << "}}";
    
    auto response = HttpPut("/rest/api/3/issue/" + issue_id, oss.str());
    return response.status_code == 204;
}

bool JiraAdapter::DeleteIssue(const std::string& issue_id) {
    auto response = HttpDelete("/rest/api/3/issue/" + issue_id);
    return response.status_code == 204;
}

std::optional<IssueReference> JiraAdapter::GetIssue(const std::string& issue_id) {
    auto response = HttpGet("/rest/api/3/issue/" + issue_id);
    
    if (response.status_code == 200) {
        return ParseIssue(response.body);
    }
    
    return std::nullopt;
}

std::vector<IssueReference> JiraAdapter::SearchIssues(const SearchQuery& query) {
    std::vector<IssueReference> results;
    
    // Build JQL query
    std::ostringstream jql;
    jql << "project=" << config_.project_key;
    
    if (!query.text.empty()) {
        jql << " AND text ~ \"" << EscapeJql(query.text) << "\"";
    }
    
    if (!query.assignee_filter.empty()) {
        jql << " AND assignee = \"" << query.assignee_filter << "\"";
    }
    
    if (!query.label_filter.empty()) {
        for (const auto& label : query.label_filter) {
            jql << " AND labels = \"" << label << "\"";
        }
    }
    
    // Execute search
    std::string path = "/rest/api/3/search?jql=" + EscapeJql(jql.str()) + 
                       "&maxResults=" + std::to_string(query.limit);
    
    auto response = HttpGet(path);
    
    if (response.status_code == 200) {
        // Parse response and extract issues
        // Simplified - would parse JSON array
    }
    
    return results;
}

std::vector<IssueReference> JiraAdapter::GetRecentIssues(int count) {
    SearchQuery query;
    query.limit = count;
    return SearchIssues(query);
}

std::vector<IssueReference> JiraAdapter::GetIssuesByLabel(const std::string& label) {
    SearchQuery query;
    query.label_filter.push_back(label);
    return SearchIssues(query);
}

IssueComment JiraAdapter::AddComment(const std::string& issue_id,
                                      const std::string& text) {
    IssueComment comment;
    
    std::string body = "{\"body\":\"" + text + "\"}";
    auto response = HttpPost("/rest/api/3/issue/" + issue_id + "/comment", body);
    
    if (response.status_code == 201) {
        // Parse comment
    }
    
    return comment;
}

std::vector<IssueComment> JiraAdapter::GetComments(const std::string& issue_id) {
    std::vector<IssueComment> comments;
    
    auto response = HttpGet("/rest/api/3/issue/" + issue_id + "/comment");
    
    if (response.status_code == 200) {
        // Parse comments array
    }
    
    return comments;
}

IssueAttachment JiraAdapter::AttachFile(const std::string& issue_id,
                                         const std::string& file_path,
                                         const std::string& description) {
    IssueAttachment attachment;
    // Would use multipart/form-data upload
    (void)issue_id;
    (void)file_path;
    (void)description;
    return attachment;
}

std::vector<std::string> JiraAdapter::GetLabels() {
    std::vector<std::string> labels;
    auto response = HttpGet("/rest/api/3/label");
    
    if (response.status_code == 200) {
        // Parse labels
    }
    
    return labels;
}

std::vector<std::string> JiraAdapter::GetIssueTypes() {
    std::vector<std::string> types;
    auto response = HttpGet("/rest/api/3/issuetype");
    
    if (response.status_code == 200) {
        // Parse issue types
    }
    
    return types;
}

std::vector<std::string> JiraAdapter::GetUsers() {
    std::vector<std::string> users;
    auto response = HttpGet("/rest/api/3/users/search");
    
    if (response.status_code == 200) {
        // Parse users
    }
    
    return users;
}

std::string JiraAdapter::RegisterWebhook(const WebhookConfig& config) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"name\":\"ScratchRobin Integration\",";
    oss << "\"url\":\"" << config.url << "\",";
    oss << "\"events\":[\"jira:issue_created\",\"jira:issue_updated\"]";
    oss << "}";
    
    auto response = HttpPost("/rest/api/3/webhook", oss.str());
    
    if (response.status_code == 201) {
        // Parse webhook ID
        return "webhook_id";
    }
    
    return "";
}

bool JiraAdapter::UnregisterWebhook(const std::string& webhook_id) {
    auto response = HttpDelete("/rest/api/3/webhook/" + webhook_id);
    return response.status_code == 204;
}

IssueReference JiraAdapter::ParseIssue(const std::string& json_response) {
    IssueReference issue;
    issue.provider = PROVIDER_NAME;
    
    JsonParser parser(json_response);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return issue;
    }
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* id = FindMember(root, "id");
        if (id && id->type == JsonValue::Type::String) {
            issue.issue_id = id->string_value;
        }
        
        const JsonValue* key = FindMember(root, "key");
        if (key && key->type == JsonValue::Type::String) {
            issue.display_key = key->string_value;
        }
        
        const JsonValue* self = FindMember(root, "self");
        if (self && self->type == JsonValue::Type::String) {
            issue.url = self->string_value;
        }
        
        const JsonValue* fields = FindMember(root, "fields");
        if (fields && fields->type == JsonValue::Type::Object) {
            const JsonValue* summary = FindMember(*fields, "summary");
            if (summary && summary->type == JsonValue::Type::String) {
                issue.title = summary->string_value;
            }
            
            const JsonValue* status = FindMember(*fields, "status");
            if (status && status->type == JsonValue::Type::Object) {
                const JsonValue* status_name = FindMember(*status, "name");
                if (status_name && status_name->type == JsonValue::Type::String) {
                    issue.status = ParseJiraStatus(status_name->string_value);
                }
            }
        }
    }
    
    return issue;
}

IssueStatus JiraAdapter::ParseJiraStatus(const std::string& jira_status) {
    std::string lower = jira_status;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "done" || lower == "closed" || lower == "resolved") {
        return IssueStatus::CLOSED;
    } else if (lower == "in progress") {
        return IssueStatus::IN_PROGRESS;
    } else if (lower == "blocked" || lower == "impeded") {
        return IssueStatus::BLOCKED;
    } else if (lower == "to do" || lower == "open") {
        return IssueStatus::OPEN;
    }
    
    return IssueStatus::OPEN;
}

std::string JiraAdapter::StatusToJira(IssueStatus status) {
    switch (status) {
        case IssueStatus::CLOSED:
        case IssueStatus::RESOLVED:
            return "Done";
        case IssueStatus::IN_PROGRESS:
            return "In Progress";
        case IssueStatus::BLOCKED:
            return "Blocked";
        case IssueStatus::OPEN:
        default:
            return "To Do";
    }
}

std::string JiraAdapter::EscapeJql(const std::string& text) {
    // URL encode special characters
    std::ostringstream escaped;
    for (char c : text) {
        if (c == ' ') {
            escaped << "%20";
        } else if (c == '"') {
            escaped << "%22";
        } else if (c == '&') {
            escaped << "%26";
        } else {
            escaped << c;
        }
    }
    return escaped.str();
}

} // namespace scratchrobin
