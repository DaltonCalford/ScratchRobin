/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "issue_tracker_gitlab.h"
#include "simple_json.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// GitLab Adapter Implementation
// ============================================================================
bool GitLabAdapter::Initialize(const TrackerConfig& config) {
    config_ = config;
    
    // Validate configuration
    if (config_.project_id == 0 && (config_.owner.empty() || config_.repo.empty())) {
        return false;
    }
    
    base_url_ = config_.base_url.empty()
        ? "https://gitlab.com/api/v4"
        : config_.base_url;
    
    // Remove trailing slash
    if (base_url_.back() == '/') {
        base_url_.pop_back();
    }
    
    // Build project path
    if (config_.project_id != 0) {
        project_path_ = "/projects/" + std::to_string(config_.project_id);
    } else {
        // URL encode the path
        std::string encoded_path = config_.owner + "/" + config_.repo;
        // Replace / with %2F
        size_t pos = 0;
        while ((pos = encoded_path.find('/', pos)) != std::string::npos) {
            encoded_path.replace(pos, 1, "%2F");
            pos += 3;
        }
        project_path_ = "/projects/" + encoded_path;
    }
    
    private_token_ = config_.auth.token;
    
    return !private_token_.empty();
}

bool GitLabAdapter::TestConnection() {
    auto response = HttpGet("/user");
    return response.status_code == 200;
}

GitLabAdapter::HttpResponse GitLabAdapter::HttpGet(const std::string& path) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-H \"PRIVATE-TOKEN: " << private_token_ << "\" ";
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

GitLabAdapter::HttpResponse GitLabAdapter::HttpPost(const std::string& path,
                                                     const std::string& body) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-X POST ";
    cmd << "-H \"PRIVATE-TOKEN: " << private_token_ << "\" ";
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

GitLabAdapter::HttpResponse GitLabAdapter::HttpPut(const std::string& path,
                                                     const std::string& body) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-X PUT ";
    cmd << "-H \"PRIVATE-TOKEN: " << private_token_ << "\" ";
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

IssueReference GitLabAdapter::CreateIssue(const IssueCreateRequest& request) {
    IssueReference issue;
    
    std::ostringstream oss;
    oss << "{";
    oss << "\"title\":\"" << request.title << "\",";
    oss << "\"description\":\"" << request.description << "\"";
    
    if (!request.labels.empty()) {
        oss << ",\"labels\":\"";
        for (size_t i = 0; i < request.labels.size(); ++i) {
            if (i > 0) oss << ",";
            oss << request.labels[i];
        }
        oss << "\"";
    }
    
    if (!request.assignee.empty()) {
        oss << ",\"assignee_ids\":[\"" << request.assignee << "\"]";
    }
    
    oss << "}";
    
    auto response = HttpPost(project_path_ + "/issues", oss.str());
    
    if (response.status_code == 201) {
        return ParseIssue(response.body);
    }
    
    return issue;
}

bool GitLabAdapter::UpdateIssue(const std::string& issue_id,
                                 const IssueUpdateRequest& request) {
    std::ostringstream oss;
    oss << "{";
    
    bool first = true;
    if (request.title.has_value()) {
        if (!first) oss << ",";
        first = false;
        oss << "\"title\":\"" << *request.title << "\"";
    }
    
    if (request.description.has_value()) {
        if (!first) oss << ",";
        first = false;
        oss << "\"description\":\"" << *request.description << "\"";
    }
    
    if (request.status.has_value()) {
        if (!first) oss << ",";
        first = false;
        std::string state = StatusToGitLab(*request.status);
        if (!state.empty()) {
            oss << "\"state_event\":\"" << state << "\"";
        }
    }
    
    oss << "}";
    
    auto response = HttpPut(project_path_ + "/issues/" + issue_id, oss.str());
    return response.status_code == 200;
}

bool GitLabAdapter::DeleteIssue(const std::string& issue_id) {
    // GitLab doesn't allow deleting issues via API
    // Close it instead
    IssueUpdateRequest request;
    request.status = IssueStatus::CLOSED;
    return UpdateIssue(issue_id, request);
}

std::optional<IssueReference> GitLabAdapter::GetIssue(const std::string& issue_id) {
    auto response = HttpGet(project_path_ + "/issues/" + issue_id);
    
    if (response.status_code == 200) {
        return ParseIssue(response.body);
    }
    
    return std::nullopt;
}

std::vector<IssueReference> GitLabAdapter::SearchIssues(const SearchQuery& query) {
    std::vector<IssueReference> results;
    
    std::ostringstream path;
    path << project_path_ << "/issues?state=all";
    
    if (query.limit > 0) {
        path << "&per_page=" << query.limit;
    }
    
    if (!query.text.empty()) {
        path << "&search=" << query.text;
    }
    
    auto response = HttpGet(path.str());
    
    if (response.status_code == 200) {
        // Parse JSON array
    }
    
    return results;
}

std::vector<IssueReference> GitLabAdapter::GetRecentIssues(int count) {
    SearchQuery query;
    query.limit = count;
    return SearchIssues(query);
}

std::vector<IssueReference> GitLabAdapter::GetIssuesByLabel(const std::string& label) {
    std::vector<IssueReference> results;
    
    auto response = HttpGet(project_path_ + "/issues?labels=" + label);
    
    if (response.status_code == 200) {
        // Parse issues
    }
    
    return results;
}

IssueComment GitLabAdapter::AddComment(const std::string& issue_id,
                                        const std::string& text) {
    IssueComment comment;
    
    std::string body = "{\"body\":\"" + text + "\"}";
    auto response = HttpPost(project_path_ + "/issues/" + issue_id + "/notes", body);
    
    if (response.status_code == 201) {
        // Parse comment
    }
    
    return comment;
}

std::vector<IssueComment> GitLabAdapter::GetComments(const std::string& issue_id) {
    std::vector<IssueComment> comments;
    
    auto response = HttpGet(project_path_ + "/issues/" + issue_id + "/notes");
    
    if (response.status_code == 200) {
        // Parse comments
    }
    
    return comments;
}

IssueAttachment GitLabAdapter::AttachFile(const std::string& issue_id,
                                           const std::string& file_path,
                                           const std::string& description) {
    IssueAttachment attachment;
    (void)issue_id;
    (void)file_path;
    (void)description;
    return attachment;
}

std::vector<std::string> GitLabAdapter::GetLabels() {
    std::vector<std::string> labels;
    
    auto response = HttpGet(project_path_ + "/labels");
    
    if (response.status_code == 200) {
        // Parse labels
    }
    
    return labels;
}

std::vector<std::string> GitLabAdapter::GetIssueTypes() {
    // GitLab uses labels for issue types
    return {"bug", "feature", "task", "documentation"};
}

std::vector<std::string> GitLabAdapter::GetUsers() {
    std::vector<std::string> users;
    
    auto response = HttpGet(project_path_ + "/members");
    
    if (response.status_code == 200) {
        // Parse users
    }
    
    return users;
}

std::string GitLabAdapter::RegisterWebhook(const WebhookConfig& config) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"url\":\"" << config.url << "\",";
    oss << "\"issues_events\":true,";
    oss << "\"note_events\":true";
    oss << "}";
    
    auto response = HttpPost(project_path_ + "/hooks", oss.str());
    
    if (response.status_code == 201) {
        return "webhook_id";
    }
    
    return "";
}

bool GitLabAdapter::UnregisterWebhook(const std::string& webhook_id) {
    (void)webhook_id;
    return true;
}

IssueReference GitLabAdapter::ParseIssue(const std::string& json_response) {
    IssueReference issue;
    issue.provider = PROVIDER_NAME;
    
    JsonParser parser(json_response);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return issue;
    }
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* iid = FindMember(root, "iid");
        if (iid && iid->type == JsonValue::Type::Number) {
            issue.issue_id = std::to_string(static_cast<int>(iid->number_value));
            issue.display_key = "#" + issue.issue_id;
        }
        
        const JsonValue* web_url = FindMember(root, "web_url");
        if (web_url && web_url->type == JsonValue::Type::String) {
            issue.url = web_url->string_value;
        }
        
        const JsonValue* title = FindMember(root, "title");
        if (title && title->type == JsonValue::Type::String) {
            issue.title = title->string_value;
        }
        
        const JsonValue* state = FindMember(root, "state");
        if (state && state->type == JsonValue::Type::String) {
            issue.status = ParseGitLabState(state->string_value);
        }
    }
    
    return issue;
}

IssueStatus GitLabAdapter::ParseGitLabState(const std::string& state) {
    if (state == "closed") {
        return IssueStatus::CLOSED;
    } else if (state == "opened") {
        return IssueStatus::OPEN;
    }
    return IssueStatus::OPEN;
}

std::string GitLabAdapter::StatusToGitLab(IssueStatus status) {
    switch (status) {
        case IssueStatus::CLOSED:
        case IssueStatus::RESOLVED:
            return "close";
        case IssueStatus::OPEN:
            return "reopen";
        default:
            return "";
    }
}

} // namespace scratchrobin
