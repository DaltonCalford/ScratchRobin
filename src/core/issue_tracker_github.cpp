/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "issue_tracker_github.h"
#include "simple_json.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// GitHub Adapter Implementation
// ============================================================================
bool GitHubAdapter::Initialize(const TrackerConfig& config) {
    config_ = config;
    
    // Validate configuration
    if (config_.owner.empty() || config_.repo.empty()) {
        return false;
    }
    
    base_url_ = config_.base_url.empty() 
        ? "https://api.github.com" 
        : config_.base_url;
    
    repo_path_ = "/repos/" + config_.owner + "/" + config_.repo;
    
    // Build auth header
    if (config_.auth.type == "personal_token" || config_.auth.type == "api_token") {
        auth_header_ = "token " + config_.auth.token;
    } else if (config_.auth.type == "oauth") {
        auth_header_ = "Bearer " + config_.auth.token;
    }
    
    return !auth_header_.empty();
}

bool GitHubAdapter::TestConnection() {
    auto response = HttpGet("/user");
    return response.status_code == 200;
}

GitHubAdapter::HttpResponse GitHubAdapter::HttpGet(const std::string& path) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-H \"Authorization: " << auth_header_ << "\" ";
    cmd << "-H \"Accept: application/vnd.github+json\" ";
    cmd << "-H \"X-GitHub-Api-Version: 2022-11-28\" ";
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

GitHubAdapter::HttpResponse GitHubAdapter::HttpPost(const std::string& path,
                                                     const std::string& body) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-X POST ";
    cmd << "-H \"Authorization: " << auth_header_ << "\" ";
    cmd << "-H \"Accept: application/vnd.github+json\" ";
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

GitHubAdapter::HttpResponse GitHubAdapter::HttpPatch(const std::string& path,
                                                      const std::string& body) {
    HttpResponse response;
    
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-X PATCH ";
    cmd << "-H \"Authorization: " << auth_header_ << "\" ";
    cmd << "-H \"Accept: application/vnd.github+json\" ";
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

IssueReference GitHubAdapter::CreateIssue(const IssueCreateRequest& request) {
    IssueReference issue;
    
    std::ostringstream oss;
    oss << "{";
    oss << "\"title\":\"" << request.title << "\",";
    oss << "\"body\":\"" << request.description << "\"";
    
    if (!request.labels.empty()) {
        oss << ",\"labels\":[";
        for (size_t i = 0; i < request.labels.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << request.labels[i] << "\"";
        }
        oss << "]";
    }
    
    if (!request.assignee.empty()) {
        oss << ",\"assignees\":[\"" << request.assignee << "\"]";
    }
    
    oss << "}";
    
    auto response = HttpPost(repo_path_ + "/issues", oss.str());
    
    if (response.status_code == 201) {
        return ParseIssue(response.body);
    }
    
    return issue;
}

bool GitHubAdapter::UpdateIssue(const std::string& issue_id,
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
        oss << "\"body\":\"" << *request.description << "\"";
    }
    
    if (request.status.has_value()) {
        if (!first) oss << ",";
        first = false;
        oss << "\"state\":\"" << StatusToGitHub(*request.status) << "\"";
    }
    
    oss << "}";
    
    auto response = HttpPatch(repo_path_ + "/issues/" + issue_id, oss.str());
    return response.status_code == 200;
}

bool GitHubAdapter::DeleteIssue(const std::string& issue_id) {
    // GitHub doesn't allow deleting issues, only closing them
    IssueUpdateRequest request;
    request.status = IssueStatus::CLOSED;
    return UpdateIssue(issue_id, request);
}

std::optional<IssueReference> GitHubAdapter::GetIssue(const std::string& issue_id) {
    auto response = HttpGet(repo_path_ + "/issues/" + issue_id);
    
    if (response.status_code == 200) {
        return ParseIssue(response.body);
    }
    
    return std::nullopt;
}

std::vector<IssueReference> GitHubAdapter::SearchIssues(const SearchQuery& query) {
    std::vector<IssueReference> results;
    
    std::ostringstream path;
    path << repo_path_ << "/issues?state=all";
    
    if (query.limit > 0) {
        path << "&per_page=" << query.limit;
    }
    
    if (!query.assignee_filter.empty()) {
        path << "&assignee=" << query.assignee_filter;
    }
    
    auto response = HttpGet(path.str());
    
    if (response.status_code == 200) {
        // Parse JSON array of issues
        // Simplified implementation
    }
    
    return results;
}

std::vector<IssueReference> GitHubAdapter::GetRecentIssues(int count) {
    SearchQuery query;
    query.limit = count;
    return SearchIssues(query);
}

std::vector<IssueReference> GitHubAdapter::GetIssuesByLabel(const std::string& label) {
    std::vector<IssueReference> results;
    
    auto response = HttpGet(repo_path_ + "/issues?labels=" + label);
    
    if (response.status_code == 200) {
        // Parse issues
    }
    
    return results;
}

IssueComment GitHubAdapter::AddComment(const std::string& issue_id,
                                        const std::string& text) {
    IssueComment comment;
    
    std::string body = "{\"body\":\"" + text + "\"}";
    auto response = HttpPost(repo_path_ + "/issues/" + issue_id + "/comments", body);
    
    if (response.status_code == 201) {
        // Parse comment
    }
    
    return comment;
}

std::vector<IssueComment> GitHubAdapter::GetComments(const std::string& issue_id) {
    std::vector<IssueComment> comments;
    
    auto response = HttpGet(repo_path_ + "/issues/" + issue_id + "/comments");
    
    if (response.status_code == 200) {
        // Parse comments
    }
    
    return comments;
}

IssueAttachment GitHubAdapter::AttachFile(const std::string& issue_id,
                                           const std::string& file_path,
                                           const std::string& description) {
    IssueAttachment attachment;
    // GitHub doesn't support file attachments on issues directly
    // Would need to upload to separate storage and link
    (void)issue_id;
    (void)file_path;
    (void)description;
    return attachment;
}

std::vector<std::string> GitHubAdapter::GetLabels() {
    std::vector<std::string> labels;
    
    auto response = HttpGet(repo_path_ + "/labels");
    
    if (response.status_code == 200) {
        // Parse labels
    }
    
    return labels;
}

std::vector<std::string> GitHubAdapter::GetIssueTypes() {
    // GitHub issues don't have built-in types like Jira
    // Return common label-based types
    return {"bug", "enhancement", "task", "documentation"};
}

std::vector<std::string> GitHubAdapter::GetUsers() {
    std::vector<std::string> users;
    
    auto response = HttpGet(repo_path_ + "/collaborators");
    
    if (response.status_code == 200) {
        // Parse users
    }
    
    return users;
}

std::string GitHubAdapter::RegisterWebhook(const WebhookConfig& config) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"name\":\"web\",";
    oss << "\"config\":{\"url\":\"" << config.url << "\",\"content_type\":\"json\"},";
    oss << "\"events\":[\"issues\",\"issue_comment\"]";
    oss << "}";
    
    auto response = HttpPost(repo_path_ + "/hooks", oss.str());
    
    if (response.status_code == 201) {
        // Parse webhook ID
        return "webhook_id";
    }
    
    return "";
}

bool GitHubAdapter::UnregisterWebhook(const std::string& webhook_id) {
    // Would use DELETE /repos/{owner}/{repo}/hooks/{hook_id}
    (void)webhook_id;
    return true;
}

IssueReference GitHubAdapter::ParseIssue(const std::string& json_response) {
    IssueReference issue;
    issue.provider = PROVIDER_NAME;
    
    JsonParser parser(json_response);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return issue;
    }
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* num = FindMember(root, "number");
        if (num && num->type == JsonValue::Type::Number) {
            issue.issue_id = std::to_string(static_cast<int>(num->number_value));
            issue.display_key = "#" + issue.issue_id;
        }
        
        const JsonValue* url = FindMember(root, "html_url");
        if (url && url->type == JsonValue::Type::String) {
            issue.url = url->string_value;
        }
        
        const JsonValue* title = FindMember(root, "title");
        if (title && title->type == JsonValue::Type::String) {
            issue.title = title->string_value;
        }
        
        const JsonValue* state = FindMember(root, "state");
        if (state && state->type == JsonValue::Type::String) {
            issue.status = ParseGitHubState(state->string_value);
        }
    }
    
    return issue;
}

IssueStatus GitHubAdapter::ParseGitHubState(const std::string& state) {
    if (state == "closed") {
        return IssueStatus::CLOSED;
    } else if (state == "open") {
        return IssueStatus::OPEN;
    }
    return IssueStatus::OPEN;
}

std::string GitHubAdapter::StatusToGitHub(IssueStatus status) {
    switch (status) {
        case IssueStatus::CLOSED:
        case IssueStatus::RESOLVED:
            return "closed";
        case IssueStatus::OPEN:
        case IssueStatus::IN_PROGRESS:
        case IssueStatus::BLOCKED:
        default:
            return "open";
    }
}

} // namespace scratchrobin
