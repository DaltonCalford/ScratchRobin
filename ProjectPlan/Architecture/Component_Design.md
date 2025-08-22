# ScratchRobin Component Design

## Overview

This document provides detailed design specifications for the core components of ScratchRobin, following the architectural principles outlined in the System Architecture document.

## Component Design Patterns

### 1. Factory Pattern
Used for component creation and dependency injection:

```cpp
class ComponentFactory {
public:
    static std::unique_ptr<IConnectionManager> createConnectionManager();
    static std::unique_ptr<IMetadataManager> createMetadataManager(IConnectionManager* connMgr);
    static std::unique_ptr<IQueryExecutor> createQueryExecutor(IConnectionManager* connMgr);
};
```

### 2. Observer Pattern
Used for event handling and notifications:

```cpp
class IObserver {
public:
    virtual void onEvent(const Event& event) = 0;
};

class ISubject {
public:
    virtual void attach(IObserver* observer) = 0;
    virtual void detach(IObserver* observer) = 0;
    virtual void notify(const Event& event) = 0;
};
```

### 3. Strategy Pattern
Used for configurable algorithms:

```cpp
class IConnectionStrategy {
public:
    virtual bool connect(const ConnectionInfo& info) = 0;
};

class DirectConnectionStrategy : public IConnectionStrategy {
public:
    bool connect(const ConnectionInfo& info) override;
};

class PooledConnectionStrategy : public IConnectionStrategy {
public:
    bool connect(const ConnectionInfo& info) override;
};
```

## Core Component Specifications

### ConnectionManager Component

#### Class Diagram
```
┌─────────────────────────────────────────────────┐
│           ConnectionManager                     │
├─────────────────────────────────────────────────┤
│ - connections_: map<ConnectionId, Connection>   │
│ - pool_: ConnectionPool                         │
│ - authManager_: IAuthManager*                   │
│ - eventDispatcher_: IEventDispatcher*           │
├─────────────────────────────────────────────────┤
│ + connect(info: ConnectionInfo): ConnectionId   │
│ + disconnect(id: ConnectionId): bool           │
│ + getConnection(id: ConnectionId): Connection*  │
│ + getPoolStats(): PoolStats                     │
│ + setPoolSize(size: int): void                 │
│ + healthCheck(): bool                          │
└─────────────────────────────────────────────────┘
```

#### State Diagram
```
┌─────────┐    connect()    ┌─────────┐   success    ┌─────────┐
│ DISCONN │ ──────────────→ │CONNECTING│ ───────────→ │CONNECTED│
│ ECTED   │                  │         │              │         │
└─────────┘                  └─────────┘              └─────────┘
    ↑                              │                      │
    │                   timeout/  │        disconnect()  │
    │                    failure  │                      │
    │                              ▼                      ▼
    │                       ┌─────────┐           ┌─────────┐
    └───────────────────────┤  ERROR   │           │CLOSED    │
                            │         │           │         │
                            └─────────┘           └─────────┘
```

#### Key Methods Implementation

```cpp
ConnectionId ConnectionManager::connect(const ConnectionInfo& info) {
    // Validate connection info
    if (!validateConnectionInfo(info)) {
        throw ConnectionException("Invalid connection information");
    }

    // Check connection pool limits
    if (connections_.size() >= maxPoolSize_) {
        cleanupStaleConnections();
        if (connections_.size() >= maxPoolSize_) {
            throw ConnectionException("Connection pool exhausted");
        }
    }

    // Generate unique connection ID
    ConnectionId id = generateConnectionId();

    // Create connection object
    auto connection = std::make_unique<Connection>(info, id);

    // Attempt connection
    if (connection->connect()) {
        connections_[id] = std::move(connection);
        notifyConnectionEstablished(id);
        return id;
    } else {
        throw ConnectionException("Failed to establish connection");
    }
}
```

### MetadataManager Component

#### Class Diagram
```
┌─────────────────────────────────────────────────┐
│            MetadataManager                      │
├─────────────────────────────────────────────────┤
│ - connectionManager_: IConnectionManager*       │
│ - cache_: MetadataCache                         │
│ - schemaLoader_: ISchemaLoader*                 │
│ - refreshTimer_: ITimer*                       │
├─────────────────────────────────────────────────┤
│ + getSchemas(connId): vector<SchemaInfo>        │
│ + getTables(connId, schema): vector<TableInfo>  │
│ + getColumns(connId, schema, table): vector<ColInfo>
│ + refreshCache(connId): void                   │
│ + clearCache(connId): void                     │
│ + setCacheTimeout(seconds: int): void          │
└─────────────────────────────────────────────────┘
```

#### Cache Strategy
```cpp
class MetadataCache {
private:
    struct CacheEntry {
        std::chrono::system_clock::time_point timestamp;
        std::variant<Schemas, Tables, Columns> data;
        size_t accessCount;
    };

    std::unordered_map<CacheKey, CacheEntry> cache_;
    std::chrono::seconds defaultTimeout_{300}; // 5 minutes

public:
    template<typename T>
    std::optional<T> get(const CacheKey& key) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (!isExpired(it->second)) {
                it->second.accessCount++;
                return std::get<T>(it->second.data);
            } else {
                cache_.erase(it);
            }
        }
        return std::nullopt;
    }

    template<typename T>
    void put(const CacheKey& key, const T& data) {
        CacheEntry entry{
            std::chrono::system_clock::now(),
            data,
            0
        };
        cache_[key] = entry;
    }
};
```

### QueryExecutor Component

#### Class Diagram
```
┌─────────────────────────────────────────────────┐
│             QueryExecutor                       │
├─────────────────────────────────────────────────┤
│ - connectionManager_: IConnectionManager*       │
│ - sqlParser_: ISQLParser*                       │
│ - resultProcessor_: IResultProcessor*           │
│ - queryHistory_: QueryHistory                   │
├─────────────────────────────────────────────────┤
│ + executeQuery(sql, connId): QueryResult        │
│ + executeNonQuery(sql, connId): int             │
│ + explainQuery(sql, connId): QueryPlan          │
│ + getQueryHistory(): vector<QueryInfo>          │
│ + clearHistory(): void                         │
└─────────────────────────────────────────────────┘
```

#### Query Execution Flow
```cpp
QueryResult QueryExecutor::executeQuery(const std::string& sql, ConnectionId connId) {
    QueryResult result;

    try {
        // Parse SQL
        auto parsedQuery = sqlParser_->parse(sql);
        if (!parsedQuery) {
            result.success = false;
            result.errorMessage = "SQL syntax error";
            return result;
        }

        // Validate query
        if (!validateQuery(*parsedQuery)) {
            result.success = false;
            result.errorMessage = "Query validation failed";
            return result;
        }

        // Get connection
        auto connection = connectionManager_->getConnection(connId);
        if (!connection) {
            result.success = false;
            result.errorMessage = "Invalid connection";
            return result;
        }

        // Execute query
        auto startTime = std::chrono::high_resolution_clock::now();
        auto rawResult = connection->executeQuery(sql);
        auto endTime = std::chrono::high_resolution_clock::now();

        // Process results
        result = resultProcessor_->process(rawResult);
        result.executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime);

        // Add to history
        addToHistory(sql, connId, result);

        result.success = true;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }

    return result;
}
```

## UI Component Specifications

### MainWindow Component

#### Class Diagram
```
┌─────────────────────────────────────────────────┐
│               MainWindow                        │
├─────────────────────────────────────────────────┤
│ - menuBar_: IMenuBar*                           │
│ - toolBar_: IToolBar*                           │
│ - statusBar_: IStatusBar*                       │
│ - centralWidget_: QWidget*                      │
│ - layoutManager_: ILayoutManager*               │
├─────────────────────────────────────────────────┤
│ + show(): void                                 │
│ + hide(): void                                 │
│ + close(): void                                │
│ + setCentralWidget(widget): void               │
│ + addDockWidget(widget, area): void            │
│ + setWindowTitle(title): void                  │
└─────────────────────────────────────────────────┘
```

#### Layout Management
```cpp
void MainWindow::initializeLayout() {
    // Create main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal);

    // Left panel - Object browser
    objectBrowser_ = new ObjectBrowserWidget();
    leftPanel_ = new QDockWidget("Object Browser");
    leftPanel_->setWidget(objectBrowser_);

    // Right panel - Properties
    propertiesPanel_ = new QDockWidget("Properties");
    propertiesPanel_->setWidget(new PropertiesWidget());

    // Central area - Query editor and results
    centralTabs_ = new QTabWidget();
    queryEditor_ = new QueryEditorWidget();
    resultViewer_ = new ResultViewerWidget();

    centralTabs_->addTab(queryEditor_, "Query");
    centralTabs_->addTab(resultViewer_, "Results");

    // Layout assembly
    mainSplitter_->addWidget(leftPanel_);
    mainSplitter_->addWidget(centralTabs_);
    mainSplitter_->addWidget(propertiesPanel_);

    setCentralWidget(mainSplitter_);

    // Set initial sizes
    mainSplitter_->setSizes({200, 600, 200});
}
```

### ObjectBrowser Component

#### Tree Model Design
```cpp
class DatabaseObject {
public:
    enum Type {
        SERVER,
        DATABASE,
        SCHEMA,
        TABLE,
        VIEW,
        COLUMN,
        INDEX,
        CONSTRAINT,
        FUNCTION,
        PROCEDURE
    };

    std::string name;
    std::string schema;
    Type type;
    std::vector<std::unique_ptr<DatabaseObject>> children;
    DatabaseObject* parent;

    virtual std::string getDisplayName() const = 0;
    virtual std::string getIconName() const = 0;
    virtual bool hasChildren() const = 0;
};

class TableObject : public DatabaseObject {
public:
    TableInfo tableInfo;

    std::string getDisplayName() const override {
        return tableInfo.name;
    }

    std::string getIconName() const override {
        return tableInfo.type == "VIEW" ? "view.png" : "table.png";
    }

    bool hasChildren() const override {
        return true; // Has columns
    }
};
```

#### Lazy Loading Implementation
```cpp
void ObjectBrowser::onItemExpanded(QTreeWidgetItem* item) {
    if (item->childCount() > 0) {
        return; // Already loaded
    }

    DatabaseObject* dbObject = getDatabaseObject(item);
    if (!dbObject || !dbObject->hasChildren()) {
        return;
    }

    // Show loading indicator
    auto loadingItem = new QTreeWidgetItem();
    loadingItem->setText(0, "Loading...");
    item->addChild(loadingItem);

    // Load children asynchronously
    QtConcurrent::run([this, item, dbObject]() {
        auto children = loadChildren(dbObject);

        // Update UI on main thread
        QMetaObject::invokeMethod(this, [this, item, loadingItem, children]() {
            item->removeChild(loadingItem);

            for (auto& child : children) {
                auto childItem = createTreeItem(child.get());
                item->addChild(childItem);
            }
        });
    });
}
```

## Data Structures

### Connection Types
```cpp
struct ConnectionInfo {
    std::string id;
    std::string name;
    std::string host;
    int port = 5432;
    std::string database;
    std::string username;
    std::string password;
    std::string connectionString;
    std::unordered_map<std::string, std::string> properties;
    std::chrono::milliseconds timeout{30000};
    bool autoReconnect = true;
    int maxReconnectAttempts = 3;
};

struct ConnectionStats {
    std::chrono::system_clock::time_point connectedAt;
    std::chrono::system_clock::time_point lastActivity;
    size_t queriesExecuted = 0;
    size_t bytesSent = 0;
    size_t bytesReceived = 0;
    double averageResponseTime = 0.0;
    ConnectionState state = ConnectionState::DISCONNECTED;
};
```

### Query Types
```cpp
struct QueryInfo {
    std::string id;
    std::string sql;
    QueryType type;
    QueryState state = QueryState::PENDING;
    std::chrono::system_clock::time_point submittedAt;
    std::chrono::system_clock::time_point startedAt;
    std::chrono::system_clock::time_point completedAt;
    std::chrono::milliseconds executionTime{0};
    int rowsAffected = 0;
    int rowsReturned = 0;
    std::string errorMessage;
    std::unordered_map<std::string, std::string> parameters;
    ConnectionId connectionId;
};

struct QueryResult {
    bool success = false;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    int rowCount = 0;
    int affectedRows = 0;
    std::string errorMessage;
    std::chrono::milliseconds executionTime{0};
    QueryPlan plan;
};
```

## Error Handling Design

### Exception Hierarchy
```cpp
class ScratchRobinException : public std::runtime_error {
public:
    ScratchRobinException(const std::string& message, ErrorCode code = ErrorCode::GENERIC)
        : std::runtime_error(message), code_(code) {}

    ErrorCode getErrorCode() const { return code_; }
    virtual std::string getErrorCategory() const = 0;

private:
    ErrorCode code_;
};

class ConnectionException : public ScratchRobinException {
public:
    std::string getErrorCategory() const override { return "CONNECTION"; }
};

class QueryException : public ScratchRobinException {
public:
    std::string getErrorCategory() const override { return "QUERY"; }
};

class MetadataException : public ScratchRobinException {
public:
    std::string getErrorCategory() const override { return "METADATA"; }
};
```

### Error Recovery Patterns
```cpp
template<typename Func, typename... Args>
auto executeWithRetry(Func func, int maxRetries, Args&&... args) {
    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            return func(std::forward<Args>(args)...);
        } catch (const ConnectionException& e) {
            attempt++;
            if (attempt >= maxRetries) {
                throw;
            }

            // Exponential backoff
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100 * (1 << attempt)));
        }
    }
}
```

## Threading and Concurrency

### Thread Safety Design
```cpp
class ThreadSafeConnectionManager : public IConnectionManager {
public:
    ThreadSafeConnectionManager()
        : mutex_(std::make_unique<std::shared_mutex>()) {}

    ConnectionId connect(const ConnectionInfo& info) override {
        std::unique_lock lock(*mutex_);
        return connectImpl(info);
    }

    bool disconnect(ConnectionId id) override {
        std::unique_lock lock(*mutex_);
        return disconnectImpl(id);
    }

    // Read operations use shared lock
    bool isConnected(ConnectionId id) const override {
        std::shared_lock lock(*mutex_);
        return isConnectedImpl(id);
    }

private:
    std::unique_ptr<std::shared_mutex> mutex_;
};
```

### Asynchronous Operations
```cpp
class AsyncQueryExecutor {
public:
    using QueryCallback = std::function<void(const QueryResult&)>;

    void executeAsync(const std::string& sql, ConnectionId connId, QueryCallback callback) {
        QtConcurrent::run([this, sql, connId, callback]() {
            QueryResult result = executeQuery(sql, connId);
            callback(result);
        });
    }

    // Progress reporting
    void executeWithProgress(const std::string& sql, ConnectionId connId,
                           ProgressCallback progressCallback,
                           QueryCallback completionCallback) {
        // Implementation for progress tracking
    }
};
```

## Memory Management

### Smart Pointer Usage
```cpp
class ComponentManager {
private:
    std::unique_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::weak_ptr<IQueryExecutor> queryExecutor_;

public:
    ComponentManager() {
        connectionManager_ = ComponentFactory::createConnectionManager();
        metadataManager_ = std::make_shared<MetadataManager>(connectionManager_.get());
        queryExecutor_ = ComponentFactory::createQueryExecutor(connectionManager_.get());
    }

    // Observer pattern for component lifecycle
    void addComponentObserver(IComponentObserver* observer) {
        observers_.push_back(observer);
    }
};
```

### Resource Cleanup
```cpp
class ResourceManager {
public:
    ~ResourceManager() {
        cleanup();
    }

    void registerResource(std::function<void()> cleanupFunc) {
        cleanupFunctions_.push_back(cleanupFunc);
    }

    void cleanup() {
        for (auto& func : cleanupFunctions_) {
            try {
                func();
            } catch (const std::exception& e) {
                // Log cleanup errors but don't throw
            }
        }
        cleanupFunctions_.clear();
    }

private:
    std::vector<std::function<void()>> cleanupFunctions_;
};
```

## Configuration Design

### Configuration Management
```cpp
class ConfigurationManager {
private:
    struct ConfigValue {
        std::string value;
        std::string defaultValue;
        bool isModified = false;
        std::chrono::system_clock::time_point lastModified;
    };

    std::unordered_map<std::string, ConfigValue> config_;
    std::string configFile_;
    bool autoSave_ = true;

public:
    void loadFromFile(const std::string& filename) {
        // Load configuration from file
    }

    void saveToFile(const std::string& filename) {
        // Save configuration to file
    }

    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        // Type-safe configuration retrieval
    }

    template<typename T>
    void set(const std::string& key, const T& value) {
        // Type-safe configuration storage
    }
};
```

## Testing Infrastructure

### Mock Objects Design
```cpp
class MockConnection : public IConnection {
public:
    MOCK_METHOD(bool, connect, (), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(QueryResult, executeQuery, (const std::string&), (override));
    MOCK_METHOD(int, executeNonQuery, (const std::string&), (override));
};

class MockMetadataManager : public IMetadataManager {
public:
    MOCK_METHOD(std::vector<SchemaInfo>, getSchemas, (ConnectionId), (override));
    MOCK_METHOD(std::vector<TableInfo>, getTables, (ConnectionId, const std::string&), (override));
    MOCK_METHOD(TableInfo, getTableInfo, (ConnectionId, const std::string&, const std::string&), (override));
};
```

### Test Fixtures
```cpp
class DatabaseTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        connectionManager_ = std::make_unique<MockConnectionManager>();
        metadataManager_ = std::make_unique<MetadataManager>(connectionManager_.get());
    }

    void TearDown() override {
        // Cleanup test data
    }

    std::unique_ptr<MockConnectionManager> connectionManager_;
    std::unique_ptr<MetadataManager> metadataManager_;
};
```

This component design document provides the detailed specifications needed to implement ScratchRobin's core functionality. Each component is designed with clear interfaces, error handling, threading considerations, and testing in mind.
