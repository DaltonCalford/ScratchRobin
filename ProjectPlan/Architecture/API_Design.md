# ScratchRobin API Design

## Overview

This document defines the comprehensive API design for ScratchRobin, covering REST APIs, C++ interfaces, client library APIs, and internal service interfaces.

## REST API Design

### API Principles

1. **RESTful Design**: Follow REST architectural constraints
2. **Resource-Oriented**: Everything is a resource with a unique URI
3. **Standard HTTP Methods**: GET, POST, PUT, DELETE, PATCH
4. **Content Negotiation**: JSON, XML, CSV support
5. **Versioning**: API versioning in URL path
6. **Pagination**: Consistent pagination for collections
7. **Filtering**: Query parameter-based filtering
8. **Sorting**: Query parameter-based sorting
9. **Rate Limiting**: Built-in rate limiting and quotas
10. **Caching**: HTTP caching headers and ETags

### Base URL Structure

```
https://api.scratchrobin.org/v1
```

### Resource Endpoints

#### 1. Database Connections

```http
GET    /v1/connections              # List connections
POST   /v1/connections              # Create connection
GET    /v1/connections/{id}         # Get connection details
PUT    /v1/connections/{id}         # Update connection
DELETE /v1/connections/{id}         # Delete connection
POST   /v1/connections/{id}/test    # Test connection
GET    /v1/connections/{id}/status  # Get connection status
```

#### 2. Database Objects

```http
GET    /v1/databases/{db}/schemas           # List schemas
GET    /v1/databases/{db}/schemas/{schema}/tables        # List tables
GET    /v1/databases/{db}/schemas/{schema}/tables/{table} # Get table details
GET    /v1/databases/{db}/schemas/{schema}/views         # List views
GET    /v1/databases/{db}/schemas/{schema}/functions     # List functions
GET    /v1/databases/{db}/schemas/{schema}/procedures    # List procedures
```

#### 3. Query Execution

```http
POST   /v1/queries/execute          # Execute SQL query
POST   /v1/queries/explain          # Explain query plan
GET    /v1/queries/history          # Get query history
GET    /v1/queries/{id}             # Get query details
POST   /v1/queries/{id}/cancel      # Cancel running query
```

#### 4. Schema Management

```http
POST   /v1/schemas                  # Create schema
GET    /v1/schemas/{name}           # Get schema details
PUT    /v1/schemas/{name}           # Update schema
DELETE /v1/schemas/{name}           # Delete schema

POST   /v1/schemas/{schema}/tables              # Create table
PUT    /v1/schemas/{schema}/tables/{table}      # Alter table
DELETE /v1/schemas/{schema}/tables/{table}      # Drop table

POST   /v1/schemas/{schema}/tables/{table}/columns       # Add column
PUT    /v1/schemas/{schema}/tables/{table}/columns/{col} # Alter column
DELETE /v1/schemas/{schema}/tables/{table}/columns/{col} # Drop column
```

#### 5. User Management

```http
GET    /v1/users                    # List users
POST   /v1/users                    # Create user
GET    /v1/users/{id}               # Get user details
PUT    /v1/users/{id}               # Update user
DELETE /v1/users/{id}               # Delete user

GET    /v1/users/{id}/permissions   # Get user permissions
PUT    /v1/users/{id}/permissions   # Update user permissions
```

#### 6. Monitoring

```http
GET    /v1/metrics                  # Get system metrics
GET    /v1/metrics/connections      # Get connection metrics
GET    /v1/metrics/queries          # Get query metrics
GET    /v1/health                  # Get system health
GET    /v1/health/database         # Get database health
```

### Request/Response Format

#### Standard Response Format

```json
{
  "success": true,
  "data": {
    // Response data
  },
  "meta": {
    "timestamp": "2023-12-07T10:30:00Z",
    "requestId": "req-12345",
    "version": "v1"
  },
  "links": {
    "self": "/v1/connections",
    "next": "/v1/connections?page=2",
    "prev": null
  }
}
```

#### Error Response Format

```json
{
  "success": false,
  "error": {
    "code": "CONNECTION_FAILED",
    "message": "Failed to connect to database",
    "details": "Connection timeout after 30 seconds",
    "category": "CONNECTION",
    "timestamp": "2023-12-07T10:30:00Z",
    "requestId": "req-12345"
  },
  "meta": {
    "timestamp": "2023-12-07T10:30:00Z",
    "requestId": "req-12345",
    "version": "v1"
  }
}
```

### Authentication & Authorization

#### API Key Authentication

```http
Authorization: Bearer <api-key>
X-API-Key: <api-key>
```

#### JWT Authentication

```http
Authorization: Bearer <jwt-token>
```

#### Basic Authentication

```http
Authorization: Basic <base64-encoded-credentials>
```

#### OAuth 2.0 Flow

```http
POST /v1/oauth/token
Content-Type: application/x-www-form-urlencoded

grant_type=authorization_code&
code=<authorization-code>&
client_id=<client-id>&
client_secret=<client-secret>&
redirect_uri=<redirect-uri>
```

### Rate Limiting

#### Rate Limit Headers

```http
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 950
X-RateLimit-Reset: 1607328000
X-RateLimit-Retry-After: 3600
```

#### Rate Limit Response

```json
{
  "success": false,
  "error": {
    "code": "RATE_LIMIT_EXCEEDED",
    "message": "Rate limit exceeded",
    "details": "Too many requests. Try again in 3600 seconds.",
    "category": "RATE_LIMITING"
  }
}
```

## C++ Interface Design

### Core Interfaces

#### 1. IApplication Interface

```cpp
class IApplication {
public:
    virtual ~IApplication() = default;

    // Lifecycle management
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isRunning() const = 0;

    // Application information
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getBuildInfo() const = 0;

    // Component access
    virtual IConnectionManager* getConnectionManager() = 0;
    virtual IMetadataManager* getMetadataManager() = 0;
    virtual IQueryExecutor* getQueryExecutor() = 0;
    virtual IConfiguration* getConfiguration() = 0;
    virtual ILogger* getLogger() = 0;

    // Event handling
    virtual void addEventListener(IEventListener* listener) = 0;
    virtual void removeEventListener(IEventListener* listener) = 0;
};
```

#### 2. IConnectionManager Interface

```cpp
class IConnectionManager {
public:
    virtual ~IConnectionManager() = default;

    // Connection management
    virtual ConnectionId connect(const ConnectionInfo& info) = 0;
    virtual bool disconnect(ConnectionId id) = 0;
    virtual bool isConnected(ConnectionId id) const = 0;

    // Connection retrieval
    virtual std::shared_ptr<IConnection> getConnection(ConnectionId id) = 0;
    virtual std::vector<ConnectionInfo> getAllConnections() const = 0;

    // Connection pool management
    virtual PoolStatistics getPoolStatistics() const = 0;
    virtual void setPoolConfiguration(const PoolConfiguration& config) = 0;
    virtual PoolConfiguration getPoolConfiguration() const = 0;

    // Health and monitoring
    virtual bool performHealthCheck(ConnectionId id = INVALID_CONNECTION_ID) = 0;
    virtual ConnectionHealthStatus getHealthStatus(ConnectionId id) const = 0;

    // Event callbacks
    using ConnectionEventCallback = std::function<void(ConnectionId, ConnectionEventType)>;
    virtual void setConnectionEventCallback(ConnectionEventCallback callback) = 0;
};
```

#### 3. IMetadataManager Interface

```cpp
class IMetadataManager {
public:
    virtual ~IMetadataManager() = default;

    // Schema operations
    virtual std::vector<SchemaInfo> getSchemas(ConnectionId id) = 0;
    virtual Result<SchemaInfo> getSchema(ConnectionId id, const std::string& name) = 0;
    virtual Result<void> createSchema(ConnectionId id, const SchemaInfo& schema) = 0;
    virtual Result<void> updateSchema(ConnectionId id, const SchemaInfo& schema) = 0;
    virtual Result<void> deleteSchema(ConnectionId id, const std::string& name) = 0;

    // Table operations
    virtual std::vector<TableInfo> getTables(ConnectionId id, const std::string& schema = "") = 0;
    virtual Result<TableInfo> getTable(ConnectionId id, const std::string& schema, const std::string& name) = 0;
    virtual Result<void> createTable(ConnectionId id, const TableInfo& table) = 0;
    virtual Result<void> updateTable(ConnectionId id, const TableInfo& table) = 0;
    virtual Result<void> deleteTable(ConnectionId id, const std::string& schema, const std::string& name) = 0;

    // Column operations
    virtual std::vector<ColumnInfo> getColumns(ConnectionId id, const std::string& schema, const std::string& table) = 0;
    virtual Result<void> addColumn(ConnectionId id, const std::string& schema, const std::string& table, const ColumnInfo& column) = 0;
    virtual Result<void> updateColumn(ConnectionId id, const std::string& schema, const std::string& table, const ColumnInfo& column) = 0;
    virtual Result<void> deleteColumn(ConnectionId id, const std::string& schema, const std::string& table, const std::string& column) = 0;

    // Index operations
    virtual std::vector<IndexInfo> getIndexes(ConnectionId id, const std::string& schema = "", const std::string& table = "") = 0;
    virtual Result<void> createIndex(ConnectionId id, const IndexInfo& index) = 0;
    virtual Result<void> deleteIndex(ConnectionId id, const std::string& schema, const std::string& name) = 0;

    // Constraint operations
    virtual std::vector<ConstraintInfo> getConstraints(ConnectionId id, const std::string& schema = "", const std::string& table = "") = 0;
    virtual Result<void> addConstraint(ConnectionId id, const ConstraintInfo& constraint) = 0;
    virtual Result<void> deleteConstraint(ConnectionId id, const std::string& schema, const std::string& table, const std::string& name) = 0;

    // Cache management
    virtual void refreshCache(ConnectionId id = INVALID_CONNECTION_ID) = 0;
    virtual void clearCache(ConnectionId id = INVALID_CONNECTION_ID) = 0;
    virtual CacheStatistics getCacheStatistics() const = 0;
};
```

#### 4. IQueryExecutor Interface

```cpp
class IQueryExecutor {
public:
    virtual ~IQueryExecutor() = default;

    // Query execution
    virtual Result<QueryResult> executeQuery(ConnectionId connectionId, const std::string& sql) = 0;
    virtual Result<int> executeNonQuery(ConnectionId connectionId, const std::string& sql) = 0;
    virtual Result<QueryPlan> explainQuery(ConnectionId connectionId, const std::string& sql) = 0;

    // Query management
    virtual QueryId executeAsync(ConnectionId connectionId, const std::string& sql, QueryCallback callback) = 0;
    virtual bool cancelQuery(QueryId queryId) = 0;
    virtual QueryStatus getQueryStatus(QueryId queryId) const = 0;

    // Query history
    virtual std::vector<QueryInfo> getQueryHistory(ConnectionId connectionId = INVALID_CONNECTION_ID) const = 0;
    virtual Result<void> saveQueryToHistory(const QueryInfo& query) = 0;
    virtual Result<void> clearQueryHistory(ConnectionId connectionId = INVALID_CONNECTION_ID) = 0;

    // Query templates
    virtual std::vector<QueryTemplate> getQueryTemplates() const = 0;
    virtual Result<void> saveQueryTemplate(const QueryTemplate& template) = 0;
    virtual Result<void> deleteQueryTemplate(const std::string& name) = 0;

    // Performance monitoring
    virtual QueryMetrics getQueryMetrics(ConnectionId connectionId = INVALID_CONNECTION_ID) const = 0;
    virtual Result<void> resetQueryMetrics(ConnectionId connectionId = INVALID_CONNECTION_ID) = 0;
};
```

### Error Handling Interfaces

#### 1. Result<T, E> Pattern

```cpp
template<typename T, typename E = std::string>
class Result {
public:
    // Factory methods
    static Result<T, E> success(const T& value);
    static Result<T, E> success(T&& value);
    static Result<T, E> failure(const E& error);
    static Result<T, E> failure(E&& error);

    // State checking
    bool isSuccess() const;
    bool isFailure() const;

    // Value access
    const T& getValue() const;
    const E& getError() const;

    // Functional programming style
    template<typename Func>
    auto map(Func func) const -> Result<decltype(func(std::declval<T>())), E>;

    template<typename Func>
    auto flatMap(Func func) const -> decltype(func(std::declval<T>()));

    // Error handling
    template<typename Func>
    Result<T, E> onSuccess(Func func) const;

    template<typename Func>
    Result<T, E> onFailure(Func func) const;
};
```

#### 2. Exception Hierarchy

```cpp
class ScratchRobinException : public std::runtime_error {
public:
    explicit ScratchRobinException(const std::string& message,
                                 ErrorCode code = ErrorCode::GENERIC,
                                 const std::string& component = "");
    virtual ~ScratchRobinException() = default;

    ErrorCode getErrorCode() const;
    std::string getComponent() const;
    std::chrono::system_clock::time_point getTimestamp() const;
    virtual std::string getErrorCategory() const = 0;
};

class ConnectionException : public ScratchRobinException {
public:
    using ScratchRobinException::ScratchRobinException;
    std::string getErrorCategory() const override { return "CONNECTION"; }
};

class QueryException : public ScratchRobinException {
public:
    using ScratchRobinException::ScratchRobinException;
    std::string getErrorCategory() const override { return "QUERY"; }
};

class MetadataException : public ScratchRobinException {
public:
    using ScratchRobinException::ScratchRobinException;
    std::string getErrorCategory() const override { return "METADATA"; }
};

class ConfigurationException : public ScratchRobinException {
public:
    using ScratchRobinException::ScratchRobinException;
    std::string getErrorCategory() const override { return "CONFIGURATION"; }
};
```

## Client Library APIs

### Python Client Library

```python
import scratchrobin

# Connection management
client = scratchrobin.connect(
    host="localhost",
    port=5432,
    database="scratchbird",
    user="admin",
    password="secret"
)

# Query execution
result = client.execute("SELECT * FROM users WHERE active = ?", True)
for row in result:
    print(f"User: {row['name']}, Email: {row['email']}")

# Metadata operations
schemas = client.get_schemas()
tables = client.get_tables("public")

# Transaction management
with client.transaction() as tx:
    tx.execute("INSERT INTO users (name, email) VALUES (?, ?)", "John Doe", "john@example.com")
    tx.execute("UPDATE user_stats SET count = count + 1")
    # Transaction automatically commits on exit

# Connection pooling
pool = scratchrobin.ConnectionPool(
    host="localhost",
    port=5432,
    database="scratchbird",
    user="admin",
    password="secret",
    max_connections=20,
    min_connections=5
)

with pool.get_connection() as conn:
    result = conn.execute("SELECT COUNT(*) FROM users")
    print(f"Total users: {result[0][0]}")
```

### Java Client Library

```java
import org.scratchrobin.*;

public class ScratchRobinExample {
    public static void main(String[] args) {
        // Connection management
        ScratchRobinClient client = new ScratchRobinClient.Builder()
            .host("localhost")
            .port(5432)
            .database("scratchbird")
            .username("admin")
            .password("secret")
            .build();

        // Query execution
        QueryResult result = client.executeQuery("SELECT * FROM users WHERE active = ?", true);
        for (Row row : result.getRows()) {
            System.out.println("User: " + row.getString("name") +
                             ", Email: " + row.getString("email"));
        }

        // Metadata operations
        List<SchemaInfo> schemas = client.getMetadataManager().getSchemas();
        List<TableInfo> tables = client.getMetadataManager().getTables("public");

        // Transaction management
        try (Transaction tx = client.beginTransaction()) {
            tx.execute("INSERT INTO users (name, email) VALUES (?, ?)",
                      "John Doe", "john@example.com");
            tx.execute("UPDATE user_stats SET count = count + 1");
            tx.commit();
        } catch (Exception e) {
            // Transaction automatically rolls back on exception
        }

        // Connection pooling
        ConnectionPool pool = new ConnectionPool.Builder()
            .host("localhost")
            .port(5432)
            .database("scratchbird")
            .username("admin")
            .password("secret")
            .maxConnections(20)
            .minConnections(5)
            .build();

        try (PooledConnection conn = pool.getConnection()) {
            QueryResult countResult = conn.execute("SELECT COUNT(*) FROM users");
            System.out.println("Total users: " + countResult.getRows().get(0).getLong(0));
        }

        client.close();
    }
}
```

### Node.js Client Library

```javascript
const scratchrobin = require('scratchrobin');

// Connection management
const client = scratchrobin.connect({
    host: 'localhost',
    port: 5432,
    database: 'scratchbird',
    user: 'admin',
    password: 'secret'
});

// Query execution
client.execute('SELECT * FROM users WHERE active = ?', [true])
    .then(result => {
        result.rows.forEach(row => {
            console.log(`User: ${row.name}, Email: ${row.email}`);
        });
    })
    .catch(err => console.error('Query failed:', err));

// Metadata operations
client.getSchemas()
    .then(schemas => console.log('Schemas:', schemas))
    .catch(err => console.error('Failed to get schemas:', err));

client.getTables('public')
    .then(tables => console.log('Tables:', tables))
    .catch(err => console.error('Failed to get tables:', err));

// Transaction management
const transaction = client.beginTransaction();

transaction.execute('INSERT INTO users (name, email) VALUES (?, ?)', ['John Doe', 'john@example.com'])
    .then(() => transaction.execute('UPDATE user_stats SET count = count + 1'))
    .then(() => transaction.commit())
    .then(() => console.log('Transaction committed'))
    .catch(err => {
        transaction.rollback();
        console.error('Transaction failed:', err);
    });

// Connection pooling
const pool = scratchrobin.createPool({
    host: 'localhost',
    port: 5432,
    database: 'scratchbird',
    user: 'admin',
    password: 'secret',
    maxConnections: 20,
    minConnections: 5
});

pool.getConnection()
    .then(conn => {
        return conn.execute('SELECT COUNT(*) FROM users')
            .then(result => {
                console.log('Total users:', result.rows[0][0]);
                conn.release();
            });
    })
    .catch(err => console.error('Pool operation failed:', err));

// Event handling
client.on('connected', () => console.log('Connected to database'));
client.on('disconnected', () => console.log('Disconnected from database'));
client.on('error', (err) => console.error('Database error:', err));

// Cleanup
process.on('SIGINT', () => {
    client.close();
    pool.close();
    process.exit(0);
});
```

### Go Client Library

```go
package main

import (
    "context"
    "fmt"
    "log"
    "time"

    "github.com/scratchbird/scratchrobin-go"
)

func main() {
    // Connection management
    client, err := scratchrobin.Connect(scratchrobin.Config{
        Host:     "localhost",
        Port:     5432,
        Database: "scratchbird",
        User:     "admin",
        Password: "secret",
    })
    if err != nil {
        log.Fatal("Failed to connect:", err)
    }
    defer client.Close()

    // Query execution
    rows, err := client.Query("SELECT * FROM users WHERE active = ?", true)
    if err != nil {
        log.Fatal("Query failed:", err)
    }
    defer rows.Close()

    for rows.Next() {
        var name, email string
        if err := rows.Scan(&name, &email); err != nil {
            log.Fatal("Scan failed:", err)
        }
        fmt.Printf("User: %s, Email: %s\n", name, email)
    }

    // Metadata operations
    schemas, err := client.GetSchemas()
    if err != nil {
        log.Fatal("Failed to get schemas:", err)
    }
    fmt.Println("Schemas:", schemas)

    tables, err := client.GetTables("public")
    if err != nil {
        log.Fatal("Failed to get tables:", err)
    }
    fmt.Println("Tables:", tables)

    // Transaction management
    tx, err := client.BeginTx(context.Background())
    if err != nil {
        log.Fatal("Failed to begin transaction:", err)
    }

    _, err = tx.Exec("INSERT INTO users (name, email) VALUES ($1, $2)", "John Doe", "john@example.com")
    if err != nil {
        tx.Rollback()
        log.Fatal("Insert failed:", err)
    }

    _, err = tx.Exec("UPDATE user_stats SET count = count + 1")
    if err != nil {
        tx.Rollback()
        log.Fatal("Update failed:", err)
    }

    if err := tx.Commit(); err != nil {
        log.Fatal("Commit failed:", err)
    }

    // Connection pooling
    pool, err := scratchrobin.NewPool(scratchrobin.PoolConfig{
        Host:           "localhost",
        Port:           5432,
        Database:       "scratchbird",
        User:           "admin",
        Password:       "secret",
        MaxConnections: 20,
        MinConnections: 5,
        MaxIdleTime:    5 * time.Minute,
    })
    if err != nil {
        log.Fatal("Failed to create pool:", err)
    }
    defer pool.Close()

    conn, err := pool.GetConnection(context.Background())
    if err != nil {
        log.Fatal("Failed to get connection from pool:", err)
    }
    defer conn.Release()

    var count int
    err = conn.QueryRow("SELECT COUNT(*) FROM users").Scan(&count)
    if err != nil {
        log.Fatal("Count query failed:", err)
    }
    fmt.Println("Total users:", count)

    // Event handling
    client.OnConnected(func() {
        fmt.Println("Connected to database")
    })

    client.OnDisconnected(func() {
        fmt.Println("Disconnected from database")
    })

    client.OnError(func(err error) {
        fmt.Println("Database error:", err)
    })
}
```

## Internal Service Interfaces

### 1. Plugin Interface

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Plugin lifecycle
    virtual bool initialize(const PluginConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Plugin information
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual PluginType getType() const = 0;

    // Capabilities
    virtual std::vector<std::string> getCapabilities() const = 0;
    virtual bool supportsCapability(const std::string& capability) const = 0;
};

class IQueryPlugin : public IPlugin {
public:
    virtual Result<QueryResult> executeQuery(ConnectionId connectionId, const std::string& sql) = 0;
    virtual Result<QueryPlan> explainQuery(ConnectionId connectionId, const std::string& sql) = 0;
};

class IMetadataPlugin : public IPlugin {
public:
    virtual Result<std::vector<SchemaInfo>> getSchemas(ConnectionId connectionId) = 0;
    virtual Result<std::vector<TableInfo>> getTables(ConnectionId connectionId, const std::string& schema) = 0;
};

class IUIPPlugin : public IPlugin {
public:
    virtual QWidget* createWidget(const std::string& widgetType, QWidget* parent = nullptr) = 0;
    virtual bool registerMenuItem(const MenuItem& item) = 0;
};
```

### 2. Event System Interface

```cpp
enum class EventType {
    CONNECTION_ESTABLISHED,
    CONNECTION_LOST,
    QUERY_STARTED,
    QUERY_COMPLETED,
    QUERY_FAILED,
    SCHEMA_CHANGED,
    TABLE_CREATED,
    TABLE_DROPPED,
    USER_CREATED,
    USER_DELETED,
    APPLICATION_STARTING,
    APPLICATION_STARTED,
    APPLICATION_STOPPING,
    APPLICATION_STOPPED
};

struct Event {
    EventType type;
    std::string source;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> data;
};

class IEventListener {
public:
    virtual ~IEventListener() = default;
    virtual void onEvent(const Event& event) = 0;
    virtual std::vector<EventType> getSupportedEvents() const = 0;
};

class IEventDispatcher {
public:
    virtual ~IEventDispatcher() = default;

    virtual void dispatch(const Event& event) = 0;
    virtual void addListener(IEventListener* listener) = 0;
    virtual void removeListener(IEventListener* listener) = 0;
    virtual void addListenerForEvent(IEventListener* listener, EventType eventType) = 0;
    virtual void removeListenerForEvent(IEventListener* listener, EventType eventType) = 0;
};
```

### 3. Monitoring Interface

```cpp
struct SystemMetrics {
    double cpuUsage;
    double memoryUsage;
    double diskUsage;
    double networkRx;
    double networkTx;
    std::chrono::system_clock::time_point timestamp;
};

struct ConnectionMetrics {
    size_t activeConnections;
    size_t idleConnections;
    size_t totalConnections;
    size_t pendingRequests;
    size_t failedConnections;
    size_t successfulConnections;
    double averageConnectionTime;
    double averageWaitTime;
    std::chrono::system_clock::time_point timestamp;
};

struct QueryMetrics {
    size_t totalQueries;
    size_t successfulQueries;
    size_t failedQueries;
    size_t cancelledQueries;
    double averageQueryTime;
    double queriesPerSecond;
    std::unordered_map<std::string, size_t> queryTypeDistribution;
    std::chrono::system_clock::time_point timestamp;
};

class IMonitoringService {
public:
    virtual ~IMonitoringService() = default;

    virtual SystemMetrics getSystemMetrics() const = 0;
    virtual ConnectionMetrics getConnectionMetrics() const = 0;
    virtual QueryMetrics getQueryMetrics() const = 0;

    virtual std::vector<SystemMetrics> getSystemMetricsHistory(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const = 0;

    virtual std::vector<ConnectionMetrics> getConnectionMetricsHistory(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const = 0;

    virtual std::vector<QueryMetrics> getQueryMetricsHistory(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const = 0;

    virtual void resetMetrics() = 0;
    virtual void enableMetrics(bool enable) = 0;
    virtual bool isMetricsEnabled() const = 0;
};
```

## API Versioning Strategy

### URL Versioning

- Version in URL path: `/v1/connections`
- Major version changes: breaking changes
- Minor version additions: backward compatible
- Patch version fixes: bug fixes only

### Header Versioning

```http
Accept: application/vnd.scratchrobin.v1+json
X-API-Version: v1
```

### Query Parameter Versioning

```http
GET /api/connections?version=v1
```

### Content Type Versioning

```http
Content-Type: application/vnd.scratchrobin.v1+json
```

## API Documentation

### OpenAPI/Swagger Specification

```yaml
openapi: 3.0.3
info:
  title: ScratchRobin API
  description: Database management interface for ScratchBird
  version: 1.0.0
  contact:
    name: ScratchRobin Team
    email: team@scratchrobin.org
servers:
  - url: https://api.scratchrobin.org/v1
    description: Production server
  - url: https://staging-api.scratchrobin.org/v1
    description: Staging server

paths:
  /connections:
    get:
      summary: List database connections
      operationId: listConnections
      parameters:
        - name: page
          in: query
          schema:
            type: integer
            minimum: 1
            default: 1
        - name: limit
          in: query
          schema:
            type: integer
            minimum: 1
            maximum: 100
            default: 20
      responses:
        '200':
          description: Successful response
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/ConnectionList'
        '401':
          $ref: '#/components/responses/Unauthorized'
        '429':
          $ref: '#/components/responses/RateLimitExceeded'

components:
  schemas:
    ConnectionInfo:
      type: object
      properties:
        id:
          type: string
          format: uuid
        name:
          type: string
          example: "Production Database"
        host:
          type: string
          example: "localhost"
        port:
          type: integer
          minimum: 1
          maximum: 65535
          example: 5432
        database:
          type: string
          example: "scratchbird"
        username:
          type: string
          example: "admin"
        isConnected:
          type: boolean
          example: true
        createdAt:
          type: string
          format: date-time
        updatedAt:
          type: string
          format: date-time

    ConnectionList:
      type: object
      properties:
        success:
          type: boolean
          example: true
        data:
          type: array
          items:
            $ref: '#/components/schemas/ConnectionInfo'
        meta:
          $ref: '#/components/schemas/Metadata'
        links:
          $ref: '#/components/schemas/Links'

  responses:
    Unauthorized:
      description: Authentication required
      content:
        application/json:
          schema:
            $ref: '#/components/schemas/Error'

    RateLimitExceeded:
      description: Too many requests
      content:
        application/json:
          schema:
            $ref: '#/components/schemas/Error'

  securitySchemes:
    BearerAuth:
      type: http
      scheme: bearer
      bearerFormat: JWT
    ApiKeyAuth:
      type: apiKey
      in: header
      name: X-API-Key
```

This comprehensive API design document provides the complete specification for ScratchRobin's interfaces, covering REST APIs, C++ interfaces, client libraries, and internal service interfaces with proper error handling, versioning, authentication, and documentation.
