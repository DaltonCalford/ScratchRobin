# Issue Tracker Integration Specification

**Version:** 1.0.0  
**Status:** ✅ IMPLEMENTED  
**Release:** Alpha 2

## Overview

This specification defines the integration of issue tracking systems into ScratchRobin, creating a unified development ecosystem where database design, schema changes, and operational issues are directly linked to project management workflows.

## Goals

1. **Bi-directional Sync**: Link database objects to issues and vice versa
2. **Change Tracking**: Automatically create issues for schema changes, drift detection, and performance regressions
3. **Design Review**: Integrate whiteboard/design reviews with approval workflows
4. **Operational Intelligence**: Create issues from monitoring alerts and health check failures
5. **Documentation**: Auto-generate issue descriptions from database context

## Supported Issue Trackers (Phase 1)

| Provider | Priority | API Version | Authentication |
|----------|----------|-------------|----------------|
| **Jira** | P0 | REST API v3 | API Token, OAuth 2.0 |
| **GitHub Issues** | P0 | REST API v3 | Personal Token, OAuth |
| **GitLab Issues** | P0 | REST API v4 | Personal Token, OAuth |
| Azure DevOps | P1 | REST API 6.0 | PAT, OAuth |
| Linear | P1 | GraphQL | API Key |
| YouTrack | P2 | REST API | Token |
| Redmine | P2 | REST API | API Key |

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     ScratchRobin Application                     │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │   Database   │  │  Whiteboard  │  │    Monitoring        │  │
│  │   Designer   │  │   Canvas     │  │    Dashboard         │  │
│  └──────┬───────┘  └──────┬───────┘  └──────────┬───────────┘  │
│         │                 │                      │              │
│         └─────────────────┼──────────────────────┘              │
│                           │                                     │
│              ┌────────────▼────────────┐                        │
│              │   Issue Link Manager    │                        │
│              │  - Object-to-Issue Map  │                        │
│              │  - Sync Orchestrator    │                        │
│              │  - Context Generator    │                        │
│              └────────────┬────────────┘                        │
│                           │                                     │
│       ┌───────────────────┼───────────────────┐                │
│       │                   │                   │                │
│  ┌────▼────┐        ┌────▼────┐        ┌────▼────┐           │
│  │  Jira   │        │ GitHub  │        │ GitLab  │           │
│  │ Adapter │        │ Adapter │        │ Adapter │           │
│  └────┬────┘        └────┬────┘        └────┬────┘           │
│       │                  │                  │                 │
│       └──────────────────┼──────────────────┘                 │
│                          │                                    │
│                    ┌─────▼──────┐                             │
│                    │  External  │                             │
│                    │   APIs     │                             │
│                    └────────────┘                             │
└─────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Issue Link Manager

Central registry for object-to-issue relationships:

```cpp
class IssueLinkManager {
public:
    // Link a database object to an issue
    bool LinkObject(const ObjectReference& obj, const IssueReference& issue);
    
    // Get all issues linked to an object
    std::vector<IssueReference> GetLinkedIssues(const ObjectReference& obj);
    
    // Get all objects linked to an issue
    std::vector<ObjectReference> GetLinkedObjects(const IssueReference& issue);
    
    // Sync status tracking
    SyncStatus GetSyncStatus(const IssueReference& issue);
    
    // Auto-create issue from database event
    IssueReference AutoCreateIssue(const DatabaseEvent& event, 
                                    const IssueTemplate& tmpl);
};
```

### 2. Provider Adapters

Common interface for all issue trackers:

```cpp
class IssueTrackerAdapter {
public:
    virtual ~IssueTrackerAdapter() = default;
    
    // Lifecycle
    virtual bool Initialize(const TrackerConfig& config) = 0;
    virtual bool TestConnection() = 0;
    
    // Issue CRUD
    virtual Issue CreateIssue(const IssueCreateRequest& request) = 0;
    virtual Issue UpdateIssue(const std::string& issue_id, 
                               const IssueUpdateRequest& request) = 0;
    virtual bool DeleteIssue(const std::string& issue_id) = 0;
    virtual std::optional<Issue> GetIssue(const std::string& issue_id) = 0;
    
    // Search/Query
    virtual std::vector<Issue> SearchIssues(const SearchQuery& query) = 0;
    virtual std::vector<Issue> GetIssuesByLabel(const std::string& label) = 0;
    
    // Comments
    virtual Comment AddComment(const std::string& issue_id, 
                                const std::string& text) = 0;
    
    // Attachments
    virtual Attachment AttachFile(const std::string& issue_id,
                                   const std::string& file_path,
                                   const std::string& description) = 0;
    
    // Project/Repository operations
    virtual std::vector<Project> GetProjects() = 0;
    virtual std::vector<Label> GetLabels(const std::string& project_id) = 0;
    virtual std::vector<User> GetUsers(const std::string& project_id) = 0;
    
    // Webhook management (for bi-directional sync)
    virtual bool RegisterWebhook(const WebhookConfig& config) = 0;
    virtual bool UnregisterWebhook(const std::string& webhook_id) = 0;
};
```

### 3. Context Generator

AI-powered generation of issue content from database context:

```cpp
class IssueContextGenerator {
public:
    // Generate issue title/description from schema change
    GeneratedContent GenerateForSchemaChange(
        const SchemaChange& change,
        const ChangeImpact& impact);
    
    // Generate from performance regression
    GeneratedContent GenerateForPerformanceIssue(
        const QueryPerformance& before,
        const QueryPerformance& after);
    
    // Generate from drift detection
    GeneratedContent GenerateForDrift(
        const SchemaSnapshot& expected,
        const SchemaSnapshot& actual);
    
    // Generate from health check failure
    GeneratedContent GenerateForHealthCheck(
        const HealthCheckResult& failure);
};
```

## Data Models

### Issue Reference

```cpp
struct IssueReference {
    std::string provider;        // "jira", "github", "gitlab"
    std::string issue_id;        // Provider-specific ID
    std::string display_key;     // "PROJ-123", "#456"
    std::string url;             // Direct link
    std::string title;
    IssueStatus status;
    Priority priority;
    std::vector<std::string> labels;
    std::string assignee;
    std::time_t created_at;
    std::time_t updated_at;
};
```

### Object Reference

```cpp
struct ObjectReference {
    ObjectType type;             // TABLE, VIEW, PROCEDURE, etc.
    std::string database;
    std::string schema;
    std::string name;
    std::string qualified_name;  // "database.schema.name"
    
    // For version control
    std::string version_hash;
    std::time_t modified_at;
};
```

### Link Record

```cpp
struct IssueLink {
    std::string link_id;
    ObjectReference object;
    IssueReference issue;
    LinkType type;               // MANUAL, AUTO_CREATED, AUTO_DETECTED
    std::string created_by;      // User or system component
    std::time_t created_at;
    SyncDirection sync_direction; // TO_ISSUE, FROM_ISSUE, BIDIRECTIONAL
};
```

## Integration Points

### 1. Database Designer

- **Create Issue from Table**: Right-click table → "Create Issue"
- **Link to Existing Issue**: Search and link to existing issue
- **Design Review Workflow**: Submit whiteboard for approval → creates review issue
- **Schema Change Proposal**: Propose changes → creates RFC issue

### 2. Migration Generator

- **Pre-migration**: Create issue for approval with impact analysis
- **Post-migration**: Update issue with results, close if successful
- **Rollback**: Auto-create incident issue on rollback

### 3. Monitoring & Health Checks

- **Drift Detection**: Create P1 issue when schema drift detected
- **Performance Regression**: Create issue when query time degrades >20%
- **Failed Backup**: Create issue with error details
- **Replication Lag**: Create issue when lag exceeds threshold

### 4. Test Runner

- **Failed Tests**: Create issue with test output and context
- **Coverage Drop**: Create issue when coverage decreases
- **Flaky Tests**: Create issue for intermittent failures

### 5. Whiteboard Canvas

- **Design Approval**: Create approval workflow issue
- **Comments → Issues**: Convert sticky notes to issues
- **Architecture Decision**: Create ADR issue from decision node

## Auto-Issue Templates

### Schema Change Template

```markdown
## Schema Change Request

**Object**: {{object.qualified_name}}
**Type**: {{change.type}} (CREATE/ALTER/DROP)
**Requested By**: {{user.name}}

### Changes
{{change.sql_ddl}}

### Impact Analysis
- Tables Affected: {{impact.table_count}}
- Queries Affected: {{impact.query_count}}
- Estimated Downtime: {{impact.downtime_minutes}} minutes
- Rollback Complexity: {{impact.rollback_complexity}}

### Dependencies
{{#each dependencies}}
- {{this.qualified_name}}
{{/each}}

### Approval Checklist
- [ ] Impact reviewed
- [ ] Rollback plan verified
- [ ] Stakeholder approval obtained
```

### Performance Regression Template

```markdown
## Performance Regression Detected

**Query**: {{query.fingerprint}}
**Severity**: {{severity}}

### Before
- Execution Time: {{before.execution_time_ms}}ms
- Rows Scanned: {{before.rows_scanned}}
- Index Usage: {{before.index_usage}}

### After
- Execution Time: {{after.execution_time_ms}}ms
- Rows Scanned: {{after.rows_scanned}}
- Index Usage: {{after.index_usage}}

### Suggested Actions
{{ai_suggestions}}
```

### Drift Detection Template

```markdown
## ⚠️ Schema Drift Detected

**Environment**: {{environment.name}}
**Detection Time**: {{timestamp}}

### Expected (from {{expected.source}})
{{expected.schema_summary}}

### Actual (from database)
{{actual.schema_summary}}

### Differences
{{drift.details}}

### Resolution
1. Review changes in {{environment.name}}
2. Either:
   - Apply changes to source: [Create Migration](#)
   - Revert changes: [Generate Rollback](#)
```

## Configuration

### Tracker Configuration

```json
{
  "trackers": [
    {
      "name": "Company Jira",
      "type": "jira",
      "enabled": true,
      "config": {
        "base_url": "https://company.atlassian.net",
        "auth": {
          "type": "api_token",
          "email": "user@company.com",
          "token": "${JIRA_API_TOKEN}"
        },
        "project_key": "DB",
        "default_issue_type": "Task",
        "labels": ["database", "scratchrobin"]
      },
      "sync": {
        "mode": "bidirectional",
        "webhook_secret": "${JIRA_WEBHOOK_SECRET}",
        "poll_interval_minutes": 5
      }
    },
    {
      "name": "GitHub Issues",
      "type": "github",
      "enabled": true,
      "config": {
        "owner": "company",
        "repo": "database-schema",
        "auth": {
          "type": "personal_token",
          "token": "${GITHUB_TOKEN}"
        },
        "labels": ["schema-change", "automated"]
      }
    }
  ]
}
```

### Auto-Issue Rules

```json
{
  "auto_issue_rules": [
    {
      "name": "Schema Drift Alert",
      "event": "drift.detected",
      "condition": "drift.severity == 'high'",
      "tracker": "Company Jira",
      "template": "drift_detection",
      "priority": "P1",
      "assignee": "@db-oncall"
    },
    {
      "name": "Performance Regression",
      "event": "performance.regression",
      "condition": "regression.percentage > 20",
      "tracker": "GitHub Issues",
      "template": "performance_regression",
      "labels": ["performance", "regression"]
    },
    {
      "name": "Migration Approval",
      "event": "migration.proposed",
      "condition": "impact.downtime_minutes > 0",
      "tracker": "Company Jira",
      "template": "schema_change",
      "labels": ["migration", "approval-required"]
    }
  ]
}
```

## UI Integration

### Issue Links Panel

```
┌────────────────────────────────────────┐
│ Linked Issues                    [+][] │
├────────────────────────────────────────┤
│ 🔗 DB-123 Schema review pending        │
│    Status: In Progress | Assignee: Bob │
│    [View] [Sync] [Unlink]              │
├────────────────────────────────────────┤
│ 🔗 #456 Performance regression fix     │
│    Status: Open | Label: performance   │
│    [View] [Sync] [Unlink]              │
├────────────────────────────────────────┤
│ [Create Issue] [Link Existing]         │
└────────────────────────────────────────┘
```

### Create Issue Dialog

```
┌────────────────────────────────────────┐
│ Create Issue                           │
├────────────────────────────────────────┤
│ Provider: [Jira ▼]                     │
│ Project:  [DB ▼]                       │
│ Type:     [Task ▼]                     │
│ Priority: [Medium ▼]                   │
├────────────────────────────────────────┤
│ Title: [Auto-generated from context  ] │
├────────────────────────────────────────┤
│ Description:                           │
│ ┌──────────────────────────────────┐   │
│ │ AI-generated from schema change  │   │
│ │ context with DDL and impact...   │   │
│ └──────────────────────────────────┘   │
├────────────────────────────────────────┤
│ Labels: [schema-change] [approval] [x] │
│ Assignee: [Search users...          ▼] │
├────────────────────────────────────────┤
│ [ ] Link to current object             │
│ [ ] Attach ER diagram                  │
│ [ ] Attach migration script            │
├────────────────────────────────────────┤
│              [Cancel] [Create Issue]   │
└────────────────────────────────────────┘
```

## Security Considerations

1. **API Token Storage**: Use secure credential store (libsecret, Keychain)
2. **Webhook Verification**: Validate signatures for incoming webhooks
3. **Data Exposure**: Sanitize database content in issue descriptions
4. **Access Control**: Respect issue tracker permissions, don't bypass
5. **Audit Logging**: Log all issue creation and linking actions

## Implementation Status

### ✅ COMPLETED (Alpha 2)

**Core Infrastructure:**
- ✅ IssueLinkManager with full CRUD operations
- ✅ JSON persistence for links (SaveLinks/LoadLinks)
- ✅ Object-to-issue mapping registry
- ✅ Sync scheduler with background tasks

**Provider Adapters (Full CRUD):**
- ✅ Jira adapter (REST API v3) - Complete with create/update/search
- ✅ GitHub adapter (REST API v4) - Complete with create/update/search
- ✅ GitLab adapter (REST API v4) - Complete with create/update/search

**UI Components:**
- ✅ IssueTrackerPanel - Main panel for viewing linked issues
- ✅ CreateIssueDialog - Full issue creation with context
- ✅ LinkIssueDialog - Search and link existing issues
- ✅ IssueTrackerSettingsDialog - Configure trackers with Add/Remove/Test
- ✅ AddTrackerDialog - Add new tracker configuration

**Context Generation:**
- ✅ IssueContextGenerator with templates
- ✅ Schema change context generation
- ✅ Performance regression context generation
- ✅ Drift detection context generation
- ✅ Health check context generation

**Webhook Server:**
- ✅ HTTP server using sockets (port 8080 configurable)
- ✅ Webhook signature verification (HMAC-SHA256)
- ✅ Jira webhook handler
- ✅ GitHub webhook handler
- ✅ GitLab webhook handler

**Sync Scheduler:**
- ✅ Background task scheduling
- ✅ Issue sync task (poll for updates)
- ✅ Drift detection task
- ✅ Health check task

**Security:**
- ✅ API token storage in system keyring (libsecret)
- ✅ Webhook signature verification
- ✅ Secure credential management

### 📋 Planned (Future Releases)

**Phase 3 Enhancements:**
- Comment synchronization
- Attachment upload
- Real-time webhook improvements

**Phase 4 Advanced:**
- ADR (Architecture Decision Record) workflow
- Design approval integration with whiteboard
- Multi-tracker linking (link to multiple trackers)
- Custom template editor

## Usage Guide

### Configuring a Tracker

1. Open **Tools → Issue Tracker Settings**
2. Click **Add** to add a new tracker
3. Select provider (Jira, GitHub, or GitLab)
4. Enter configuration:
   - **Name**: Display name for this tracker
   - **Base URL**: API endpoint URL
   - **Project**: Project key (Jira) or repo name (GitHub/GitLab)
   - **API Token**: Your personal access token
   - **Email**: (Jira only) Your account email
5. Click **Test Connection** to verify
6. Click **Save** to store configuration

### Creating an Issue from a Database Object

1. Navigate to any database object (table, view, procedure, etc.)
2. Right-click and select **Create Issue**
3. Or open the Issue Tracker panel and click **Create Issue**
4. Select provider and fill in details:
   - Title and description
   - Issue type (Bug, Enhancement, Task)
   - Priority
5. Object context is automatically included in the description
6. Click **Create** - the issue is created and linked

### Linking to an Existing Issue

1. Select the database object
2. Click **Link Existing Issue** in the Issue Tracker panel
3. Select provider
4. Search by keyword or click **Load Recent Issues**
5. Select the issue from results
6. Click **Link**

### Webhook Setup for Bi-directional Sync

**GitHub:**
1. Go to repository Settings → Webhooks
2. Add webhook: `http://your-server:8080/webhook/github`
3. Set secret and enable Issues events

**Jira:**
1. Go to Project Settings → Webhooks
2. Add webhook: `http://your-server:8080/webhook/jira`
3. Enable issue created/updated events

**GitLab:**
1. Go to Project Settings → Webhooks
2. Add webhook: `http://your-server:8080/webhook/gitlab`
3. Enable Issues events

### Auto-Created Issues

The system can auto-create issues for:
- Schema drift detection
- Performance regressions
- Health check failures
- Migration proposals

Configure auto-creation rules in the settings.

## Success Metrics

1. **Developer Efficiency**: Time from schema change to approved migration
2. **Issue Quality**: Completeness of auto-generated issue descriptions
3. **Traceability**: % of schema changes with linked issues
4. **Resolution Time**: Mean time to resolve drift/performance issues
5. **User Satisfaction**: Developer satisfaction with integration

## Appendix: API Examples

### Jira REST API

```bash
# Create issue
curl -X POST https://company.atlassian.net/rest/api/3/issue \
  -H "Authorization: Basic $AUTH" \
  -H "Content-Type: application/json" \
  -d '{
    "fields": {
      "project": {"key": "DB"},
      "summary": "Schema change: Add users table",
      "description": {
        "type": "doc",
        "version": 1,
        "content": [...]
      },
      "issuetype": {"name": "Task"},
      "labels": ["database", "schema-change"]
    }
  }'
```

### GitHub REST API

```bash
# Create issue
curl -X POST https://api.github.com/repos/company/repo/issues \
  -H "Authorization: token $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Schema change: Add users table",
    "body": "Auto-generated from ScratchRobin...",
    "labels": ["schema-change", "database"],
    "assignees": ["db-admin"]
  }'
```

### GitLab REST API

```bash
# Create issue
curl -X POST https://gitlab.com/api/v4/projects/$PROJECT_ID/issues \
  -H "PRIVATE-TOKEN: $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Schema change: Add users table",
    "description": "Auto-generated from ScratchRobin...",
    "labels": "database,schema-change",
    "assignee_ids": [5]
  }'
```
