/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ISSUE_TRACKER_JIRA_H
#define SCRATCHROBIN_ISSUE_TRACKER_JIRA_H

#include "issue_tracker.h"

namespace scratchrobin {

// ============================================================================
// Jira Issue Tracker Adapter
// ============================================================================
class JiraAdapter : public IssueTrackerAdapter {
public:
    static constexpr const char* PROVIDER_NAME = "jira";
    
    ~JiraAdapter() override = default;
    
    // Lifecycle
    bool Initialize(const TrackerConfig& config) override;
    bool TestConnection() override;
    std::string GetProviderName() const override { return PROVIDER_NAME; }
    
    // Issue CRUD
    IssueReference CreateIssue(const IssueCreateRequest& request) override;
    bool UpdateIssue(const std::string& issue_id, 
                      const IssueUpdateRequest& request) override;
    bool DeleteIssue(const std::string& issue_id) override;
    std::optional<IssueReference> GetIssue(const std::string& issue_id) override;
    
    // Search/Query
    std::vector<IssueReference> SearchIssues(const SearchQuery& query) override;
    std::vector<IssueReference> GetRecentIssues(int count = 10) override;
    std::vector<IssueReference> GetIssuesByLabel(const std::string& label) override;
    
    // Comments
    IssueComment AddComment(const std::string& issue_id, 
                             const std::string& text) override;
    std::vector<IssueComment> GetComments(const std::string& issue_id) override;
    
    // Attachments
    IssueAttachment AttachFile(const std::string& issue_id,
                                const std::string& file_path,
                                const std::string& description) override;
    
    // Metadata
    std::vector<std::string> GetLabels() override;
    std::vector<std::string> GetIssueTypes() override;
    std::vector<std::string> GetUsers() override;
    
    // Webhook management
    std::string RegisterWebhook(const WebhookConfig& config) override;
    bool UnregisterWebhook(const std::string& webhook_id) override;
    
private:
    TrackerConfig config_;
    std::string base_url_;
    std::string auth_header_;
    
    // HTTP helpers
    struct HttpResponse {
        int status_code = 0;
        std::string body;
        std::string error;
    };
    
    HttpResponse HttpGet(const std::string& path);
    HttpResponse HttpPost(const std::string& path, const std::string& body);
    HttpResponse HttpPut(const std::string& path, const std::string& body);
    HttpResponse HttpDelete(const std::string& path);
    
    // Response parsing
    IssueReference ParseIssue(const std::string& json_response);
    IssueStatus ParseJiraStatus(const std::string& jira_status);
    std::string StatusToJira(IssueStatus status);
    
    std::string BuildAuthHeader(const TrackerConfig::Auth& auth);
    std::string EscapeJql(const std::string& text);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_ISSUE_TRACKER_JIRA_H
