# ScratchRobin Implementation Roadmap

## Overview

This document outlines the phased implementation of the comprehensive project system, expanded diagramming, and design workflow features.

**Note**: The living execution tracker is maintained in `docs/planning/IMPLEMENTATION_ROADMAP.md`.

---

## Phase 1: Core Project Infrastructure (Months 1-2)

### 1.1 Project Data Model
```cpp
// New classes to implement:

class Project {
    UUID id;
    string name;
    string description;
    User owner;
    vector<User> members;
    ProjectConfig config;
    ProjectState state;
    VersionInfo version;
    Timestamp created_at;
    Timestamp modified_at;
};

class ProjectObject {
    UUID id;
    UUID project_id;
    string kind;           // table, view, design, whiteboard, etc.
    string name;
    string path;           // Hierarchical path
    DesignState state;     // extracted, draft, modified, approved, etc.
    MetadataNode source;   // If extracted from DB
    MetadataNode design;   // Current design
    vector<Comment> comments;
    vector<Change> history;
};

class DesignState {
    enum State {
        EXTRACTED,     // Read-only from source
        NEW,           // New object in design
        MODIFIED,      // Modified from source
        DELETED,       // Marked for deletion
        PENDING,       // Awaiting review
        APPROVED,      // Approved for deploy
        REJECTED,      // Changes rejected
        IMPLEMENTED,   // Deployed
        CONFLICTED     // Merge conflict
    };
    State state;
    User changed_by;
    Timestamp changed_at;
    string reason;
};
```

### 1.2 Project Storage
- [ ] Project file format (JSON/YAML)
- [ ] Project repository (local filesystem)
- [ ] Project metadata index
- [ ] Auto-save functionality
- [ ] Backup/versioning

### 1.3 UI Framework
- [ ] Project explorer panel
- [ ] Property panels for different object types
- [ ] Tabbed interface for open objects
- [ ] Breadcrumb navigation
- [ ] Search and filter

---

## Phase 2: Design Workflow Engine (Months 2-3)

### 2.1 State Management
```cpp
class DesignWorkflowEngine {
    // State transitions
    void extractObject(ProjectObject& obj, const MetadataNode& source);
    void createObject(ProjectObject& obj, const MetadataNode& design);
    void modifyObject(ProjectObject& obj, const MetadataNode& changes);
    void deleteObject(ProjectObject& obj);
    void approveObject(ProjectObject& obj, User approver);
    void rejectObject(ProjectObject& obj, const string& reason);
    void deployObject(ProjectObject& obj);
    
    // Validation
    ValidationResult validateStateTransition(ProjectObject& obj, State newState);
    vector<string> getAllowedTransitions(ProjectObject& obj);
};
```

### 2.2 Diff Engine
- [ ] Schema comparison (table structure)
- [ ] SQL diff generation
- [ ] Visual diff display
- [ ] Merge conflict detection
- [ ] Three-way merge support

### 2.3 Change Tracking
- [ ] Audit log for all changes
- [ ] Change history viewer
- [ ] Rollback to previous version
- [ ] Compare arbitrary versions

---

## Phase 3: Expanded Diagramming (Months 3-4)

### 3.1 Diagram Canvas Framework
```cpp
class DiagramCanvas : public wxScrolledCanvas {
    // Core functionality
    void addNode(NodeType type, Point position);
    void addConnection(Node* from, Node* to);
    void autoLayout(LayoutAlgorithm algo);
    void exportTo(string format);
    
    // Interaction
    void onMouseDown(Point pos);
    void onDrag(Point delta);
    void onSelect(Node* node);
    void onContextMenu(Node* node);
};
```

### 3.2 Diagram Types

**ERD (Priority 1):**
- [ ] Table nodes with columns
- [ ] Relationship connectors
- [ ] Cardinality markers
- [ ] Auto-layout
- [ ] Sync with catalog

**Whiteboard (Priority 2):**
- [ ] Freehand drawing
- [ ] Sticky notes
- [ ] Shapes and text
- [ ] Image import
- [ ] Layers

**Mind Map (Priority 2):**
- [ ] Central node
- [ ] Branching nodes
- [ ] Collapsible branches
- [ ] Color coding

**Data Flow (Priority 3):**
- [ ] Process nodes
- [ ] Data stores
- [ ] Flow arrows
- [ ] Stencil library

### 3.3 Stencil System
```cpp
class StencilLibrary {
    map<string, Stencil> categories;
    
    struct Stencil {
        string name;
        SVG icon;
        vector<Shape> shapes;
        map<string, Property> properties;
    };
};
```

---

## Phase 4: Collaboration System (Months 4-5)

### 4.1 Real-Time Sync
```cpp
class CollaborationClient {
    WebSocket connection;
    
    void connect(Project& project);
    void sendChange(Change& change);
    void receiveChange(Change& change);
    void updateCursor(User& user, Point position);
    void lockObject(ProjectObject& obj);
    void unlockObject(ProjectObject& obj);
};
```

### 4.2 Comment System
- [ ] Comments on objects
- [ ] Comments on diagrams
- [ ] Comment threads
- [ ] @mentions
- [ ] Notifications
- [ ] Comment resolution

### 4.3 Permissions
```cpp
class PermissionManager {
    enum Permission {
        VIEW,
        COMMENT,
        EDIT_DESIGN,
        EDIT_DIAGRAM,
        DEPLOY,
        ADMIN
    };
    
    bool checkPermission(User& user, ProjectObject& obj, Permission perm);
    void grantPermission(User& user, Permission perm);
    void revokePermission(User& user, Permission perm);
};
```

---

## Phase 5: Testing Framework (Months 5-6)

### 5.1 Test Definition Language
```yaml
# Test file format
test_suite: Customer Portal Tests

tests:
  - name: users_table_constraints
    type: unit
    object: users
    steps:
      - action: insert
        data: {name: "A", email: "invalid"}
        expect: fail
      - action: insert
        data: {name: "Valid", email: "test@test.com"}
        expect: success
        
  - name: order_workflow
    type: integration
    steps:
      - action: execute
        sql: "INSERT INTO orders..."
      - action: assert
        sql: "SELECT COUNT(*) FROM order_items"
        expect: 1
```

### 5.2 Test Runner
- [ ] Parse test definitions
- [ ] Execute in transaction
- [ ] Collect results
- [ ] Generate reports
- [ ] Performance benchmarking

### 5.3 Test Coverage
- [ ] Track which objects have tests
- [ ] Coverage reporting
- [ ] Gap analysis

---

## Phase 6: Deployment System (Months 6-7)

### 6.1 Deployment Plan
```cpp
class DeploymentPlan {
    string name;
    string version;
    vector<DeploymentStep> steps;
    DeploymentConfig config;
    
    struct DeploymentStep {
        int sequence;
        string name;
        vector<ProjectObject> objects;
        string rollback_sql;
        bool requires_approval;
    };
};
```

### 6.2 Migration Generator
- [ ] DDL generation
- [ ] Dependency ordering
- [ ] Rollback script generation
- [ ] Change preview

### 6.3 Deployment Execution
- [ ] Pre-deployment checks
- [ ] Backup creation
- [ ] Step-by-step execution
- [ ] Progress monitoring
- [ ] Rollback on failure
- [ ] Post-deployment validation

### 6.4 Deployment Targets
- [ ] Multiple environment support
- [ ] Environment configuration
- [ ] Promotion pipeline (dev → test → staging → prod)

---

## Phase 7: Resource Tracking (Months 7-8)

### 7.1 Time Tracking
```cpp
class TimeTracker {
    void startTask(ProjectObject& obj, ActivityType type);
    void stopTask();
    void logHours(ProjectObject& obj, float hours, string description);
    
    TimeReport generateReport(DateRange range);
    float getTotalHours(ProjectObject& obj);
    float getBudgetRemaining(Project& project);
};
```

### 7.2 Cost Analysis
- [ ] Hourly rate configuration
- [ ] Infrastructure cost tracking
- [ ] Cost projection
- [ ] Budget alerts

### 7.3 Reporting
- [ ] Time by person
- [ ] Time by object
- [ ] Burn-down charts
- [ ] Cost summaries

---

## Phase 8: Documentation System (Months 8-9)

### 8.1 Auto-Documentation
- [ ] Data dictionary generation
- [ ] ERD documentation
- [ ] Change log generation
- [ ] API docs from comments

### 8.2 Documentation Editor
- [ ] Rich text editing
- [ ] Markdown support
- [ ] Embedded diagrams
- [ ] Template system

### 8.3 Publishing
- [ ] HTML export
- [ ] PDF generation
- [ ] Wiki integration
- [ ] Scheduled updates

---

## Phase 9: Integration & Polish (Months 9-10)

### 9.1 External Integrations
- [ ] Git provider (GitHub, GitLab, Bitbucket)
- [ ] CI/CD pipelines
- [ ] Monitoring systems
- [ ] Ticketing systems

### 9.2 API
- [ ] REST API
- [ ] Webhook system
- [ ] API documentation

### 9.3 Performance Optimization
- [ ] Large project handling
- [ ] Diagram rendering optimization
- [ ] Query optimization
- [ ] Caching

### 9.4 Security Hardening
- [ ] Encryption at rest
- [ ] Audit logging
- [ ] Access control review
- [ ] Penetration testing

---

## Phase 10: Advanced Features (Months 11-12)

### 10.1 AI-Assisted Features
- [ ] Schema optimization suggestions
- [ ] Auto-completion for designs
- [ ] Natural language to SQL
- [ ] Anomaly detection

### 10.2 Advanced Diagramming
- [ ] 3D visualization
- [ ] Animation
- [ ] Interactive prototypes
- [ ] Custom visualizations

### 10.3 Enterprise Features
- [ ] LDAP/SSO integration
- [ ] SAML support
- [ ] Audit compliance reports
- [ ] Data masking

---

## Implementation Priority Matrix

| Feature | User Value | Complexity | Priority |
|---------|------------|------------|----------|
| Project Infrastructure | High | Medium | P1 |
| Design State Management | High | Medium | P1 |
| ERD Diagramming | High | Medium | P1 |
| Basic Collaboration | High | High | P1 |
| Test Framework | High | High | P2 |
| Deployment System | High | High | P2 |
| Whiteboard/Mind Map | Medium | Medium | P2 |
| Time Tracking | Medium | Low | P3 |
| Documentation System | Medium | Medium | P3 |
| Advanced Diagramming | Low | High | P4 |
| AI Features | Medium | Very High | P4 |

---

## Technical Requirements

### Backend
- **Database**: SQLite for local, PostgreSQL for team server
- **Sync**: WebSocket for real-time
- **Storage**: Local filesystem + cloud options
- **Cache**: Redis for team features

### Frontend
- **Framework**: wxWidgets (current)
- **Canvas**: Custom wxCanvas for diagrams
- **Editor**: Monaco/Scintilla for SQL
- **Rich Text**: Custom or wxRichTextCtrl

### Infrastructure
- **Team Server**: Node.js/Go for sync
- **API**: REST + GraphQL
- **Auth**: JWT + OAuth providers
- **File Storage**: S3-compatible

---

## Success Metrics

### Phase 1-2 (Core)
- [ ] Can create and save projects
- [ ] Can extract and modify objects
- [ ] State changes work correctly
- [ ] Basic diff viewer functional

### Phase 3-4 (Design & Collab)
- [ ] ERD diagrams render correctly
- [ ] Can collaborate with 2+ users
- [ ] Comments work
- [ ] Real-time sync < 500ms

### Phase 5-6 (Test & Deploy)
- [ ] Can define and run tests
- [ ] Deployment plans generate correctly
- [ ] Can deploy to target DB
- [ ] Rollback works

### Phase 7-10 (Advanced)
- [ ] Time tracking accurate
- [ ] Documentation generates
- [ ] Git integration works
- [ ] Performance acceptable

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Scope creep | Strict prioritization, MVP approach |
| Performance issues | Early profiling, optimization sprints |
| Collaboration complexity | Start with simple lock-based, evolve to OT |
| Database compatibility | Abstract DB layer, extensive testing |
| Security concerns | Security audit, encryption, access control |
