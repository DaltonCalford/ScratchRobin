# ScratchRobin Design Patterns

## Overview

This document outlines the design patterns used throughout the ScratchRobin codebase to ensure consistency, maintainability, and scalability.

## Architectural Patterns

### 1. Layered Architecture Pattern

ScratchRobin follows a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────┐
│                Presentation Layer               │
│  - Web UI (React)                              │
│  - Desktop UI (Qt)                             │
│  - CLI Interface (Go)                          │
└─────────────────────────────────────────────────┘
                        │
┌─────────────────────────────────────────────────┐
│                 Service Layer                   │
│  - Connection Management                        │
│  - Metadata Management                          │
│  - Query Execution                              │
│  - Schema Management                            │
└─────────────────────────────────────────────────┘
                        │
┌─────────────────────────────────────────────────┐
│                 Data Access Layer               │
│  - Connection Pool                              │
│  - Database Drivers                             │
│  - Result Processors                            │
└─────────────────────────────────────────────────┘
                        │
┌─────────────────────────────────────────────────┐
│              Infrastructure Layer               │
│  - Logging System                               │
│  - Configuration Management                     │
│  - Security Manager                             │
│  - Plugin System                                │
└─────────────────────────────────────────────────┘
```

#### Implementation Example
```cpp
// Service Layer - Connection Manager
class ConnectionManager {
private:
    std::unique_ptr<IConnectionPool> connectionPool_;
    std::unique_ptr<IMetadataManager> metadataManager_;

public:
    ConnectionManager() {
        // Initialize lower layer components
        connectionPool_ = createConnectionPool();
        metadataManager_ = createMetadataManager(connectionPool_.get());
    }

    std::shared_ptr<IConnection> getConnection(const ConnectionInfo& info) {
        // Delegate to lower layer
        return connectionPool_->acquireConnection(info);
    }
};
```

### 2. Repository Pattern

Used for data access abstraction:

```cpp
// Repository Interface
class ISchemaRepository {
public:
    virtual ~ISchemaRepository() = default;
    virtual std::vector<SchemaInfo> getAllSchemas() = 0;
    virtual SchemaInfo getSchema(const std::string& name) = 0;
    virtual bool createSchema(const SchemaInfo& schema) = 0;
    virtual bool updateSchema(const SchemaInfo& schema) = 0;
    virtual bool deleteSchema(const std::string& name) = 0;
};

// Concrete Implementation
class ScratchBirdSchemaRepository : public ISchemaRepository {
private:
    std::shared_ptr<IConnection> connection_;

public:
    explicit ScratchBirdSchemaRepository(std::shared_ptr<IConnection> connection)
        : connection_(connection) {}

    std::vector<SchemaInfo> getAllSchemas() override {
        // Implementation specific to ScratchBird
        auto result = connection_->executeQuery("SELECT * FROM information_schema.schemata");
        return parseSchemaResults(result);
    }
};
```

### 3. Factory Pattern

Used for object creation and dependency injection:

```cpp
// Abstract Factory
class IDatabaseComponentFactory {
public:
    virtual ~IDatabaseComponentFactory() = default;
    virtual std::unique_ptr<IConnectionManager> createConnectionManager() = 0;
    virtual std::unique_ptr<IMetadataManager> createMetadataManager(IConnectionManager* connMgr) = 0;
    virtual std::unique_ptr<IQueryExecutor> createQueryExecutor(IConnectionManager* connMgr) = 0;
};

// Concrete Factory
class ScratchBirdComponentFactory : public IDatabaseComponentFactory {
public:
    std::unique_ptr<IConnectionManager> createConnectionManager() override {
        return std::make_unique<ConnectionManager>();
    }

    std::unique_ptr<IMetadataManager> createMetadataManager(IConnectionManager* connMgr) override {
        return std::make_unique<MetadataManager>(connMgr);
    }

    std::unique_ptr<IQueryExecutor> createQueryExecutor(IConnectionManager* connMgr) override {
        return std::make_unique<QueryExecutor>(connMgr);
    }
};

// Factory Usage
class Application {
private:
    std::unique_ptr<IDatabaseComponentFactory> factory_;
    std::unique_ptr<IConnectionManager> connectionManager_;
    std::unique_ptr<IMetadataManager> metadataManager_;

public:
    Application(std::unique_ptr<IDatabaseComponentFactory> factory)
        : factory_(std::move(factory)) {

        connectionManager_ = factory_->createConnectionManager();
        metadataManager_ = factory_->createMetadataManager(connectionManager_.get());
    }
};
```

## Creational Patterns

### 1. Singleton Pattern

Used for shared resources that should have only one instance:

```cpp
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

private:
    Logger() {
        // Initialize logger
        init("scratchrobin.log");
    }

    ~Logger() {
        // Cleanup
        shutdown();
    }

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

// Usage
Logger::getInstance().info("Application started");
```

### 2. Builder Pattern

Used for complex object construction:

```cpp
class QueryBuilder {
private:
    std::string table_;
    std::vector<std::string> columns_;
    std::vector<std::string> whereConditions_;
    std::vector<std::string> orderBy_;
    std::optional<size_t> limit_;
    std::optional<size_t> offset_;

public:
    QueryBuilder& select(const std::vector<std::string>& columns) {
        columns_ = columns;
        return *this;
    }

    QueryBuilder& from(const std::string& table) {
        table_ = table;
        return *this;
    }

    QueryBuilder& where(const std::string& condition) {
        whereConditions_.push_back(condition);
        return *this;
    }

    QueryBuilder& orderBy(const std::string& column, const std::string& direction = "ASC") {
        orderBy_.push_back(column + " " + direction);
        return *this;
    }

    QueryBuilder& limit(size_t count) {
        limit_ = count;
        return *this;
    }

    QueryBuilder& offset(size_t count) {
        offset_ = count;
        return *this;
    }

    std::string build() const {
        std::stringstream sql;

        // SELECT clause
        if (columns_.empty()) {
            sql << "SELECT *";
        } else {
            sql << "SELECT " << join(columns_, ", ");
        }

        // FROM clause
        sql << " FROM " << table_;

        // WHERE clause
        if (!whereConditions_.empty()) {
            sql << " WHERE " << join(whereConditions_, " AND ");
        }

        // ORDER BY clause
        if (!orderBy_.empty()) {
            sql << " ORDER BY " << join(orderBy_, ", ");
        }

        // LIMIT clause
        if (limit_.has_value()) {
            sql << " LIMIT " << *limit_;
        }

        // OFFSET clause
        if (offset_.has_value()) {
            sql << " OFFSET " << *offset_;
        }

        return sql.str();
    }
};

// Usage
std::string query = QueryBuilder()
    .select({"id", "name", "email"})
    .from("users")
    .where("active = true")
    .where("created_at > '2023-01-01'")
    .orderBy("name")
    .limit(100)
    .build();
```

## Structural Patterns

### 1. Adapter Pattern

Used for integrating different interfaces:

```cpp
// Target interface
class IDatabaseConnection {
public:
    virtual ~IDatabaseConnection() = default;
    virtual bool connect(const ConnectionInfo& info) = 0;
    virtual QueryResult executeQuery(const std::string& sql) = 0;
    virtual void disconnect() = 0;
};

// Adaptee (third-party library)
class PostgreSQLConnection {
public:
    void open(const std::string& connectionString) {
        // PostgreSQL specific connection logic
    }

    PGresult* execute(const std::string& sql) {
        // PostgreSQL specific query execution
        return nullptr;
    }

    void close() {
        // PostgreSQL specific disconnect logic
    }
};

// Adapter
class PostgreSQLConnectionAdapter : public IDatabaseConnection {
private:
    PostgreSQLConnection* pgConnection_;

public:
    explicit PostgreSQLConnectionAdapter(PostgreSQLConnection* pg)
        : pgConnection_(pg) {}

    bool connect(const ConnectionInfo& info) override {
        std::string connectionString = buildConnectionString(info);
        try {
            pgConnection_->open(connectionString);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    QueryResult executeQuery(const std::string& sql) override {
        PGresult* result = pgConnection_->execute(sql);
        return convertToQueryResult(result);
    }

    void disconnect() override {
        pgConnection_->close();
    }

private:
    std::string buildConnectionString(const ConnectionInfo& info) {
        std::stringstream ss;
        ss << "host=" << info.host
           << " port=" << info.port
           << " dbname=" << info.database
           << " user=" << info.username
           << " password=" << info.password;
        return ss.str();
    }

    QueryResult convertToQueryResult(PGresult* pgResult) {
        QueryResult result;
        // Convert PostgreSQL result to our QueryResult format
        return result;
    }
};
```

### 2. Decorator Pattern

Used for adding functionality to objects dynamically:

```cpp
// Component interface
class IQueryExecutor {
public:
    virtual ~IQueryExecutor() = default;
    virtual QueryResult execute(const std::string& sql, const ConnectionInfo& info) = 0;
};

// Concrete component
class BasicQueryExecutor : public IQueryExecutor {
public:
    QueryResult execute(const std::string& sql, const ConnectionInfo& info) override {
        // Basic query execution logic
        return QueryResult();
    }
};

// Base decorator
class QueryExecutorDecorator : public IQueryExecutor {
protected:
    std::unique_ptr<IQueryExecutor> wrapped_;

public:
    explicit QueryExecutorDecorator(std::unique_ptr<IQueryExecutor> wrapped)
        : wrapped_(std::move(wrapped)) {}
};

// Concrete decorators
class LoggingQueryExecutor : public QueryExecutorDecorator {
public:
    explicit LoggingQueryExecutor(std::unique_ptr<IQueryExecutor> wrapped)
        : QueryExecutorDecorator(std::move(wrapped)) {}

    QueryResult execute(const std::string& sql, const ConnectionInfo& info) override {
        Logger::info("Executing query: " + sql);
        auto start = std::chrono::high_resolution_clock::now();

        QueryResult result = wrapped_->execute(sql, info);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        Logger::info("Query executed in " + std::to_string(duration.count()) + "ms");

        return result;
    }
};

class CachingQueryExecutor : public QueryExecutorDecorator {
private:
    std::unordered_map<std::string, QueryResult> cache_;
    mutable std::mutex mutex_;

public:
    explicit CachingQueryExecutor(std::unique_ptr<IQueryExecutor> wrapped)
        : QueryExecutorDecorator(std::move(wrapped)) {}

    QueryResult execute(const std::string& sql, const ConnectionInfo& info) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check cache for SELECT queries
        if (sql.find("SELECT") == 0) {
            auto it = cache_.find(sql);
            if (it != cache_.end()) {
                Logger::info("Cache hit for query: " + sql);
                return it->second;
            }
        }

        QueryResult result = wrapped_->execute(sql, info);

        // Cache SELECT results
        if (sql.find("SELECT") == 0) {
            cache_[sql] = result;
        }

        return result;
    }
};

// Usage
auto executor = std::make_unique<BasicQueryExecutor>();
executor = std::make_unique<LoggingQueryExecutor>(std::move(executor));
executor = std::make_unique<CachingQueryExecutor>(std::move(executor));

// Now we have a query executor with logging and caching capabilities
QueryResult result = executor->execute("SELECT * FROM users", connectionInfo);
```

### 3. Composite Pattern

Used for tree-like structures (e.g., database object hierarchy):

```cpp
// Component interface
class DatabaseObject {
public:
    virtual ~DatabaseObject() = default;
    virtual std::string getName() const = 0;
    virtual std::string getType() const = 0;
    virtual void addChild(std::unique_ptr<DatabaseObject> child) = 0;
    virtual const std::vector<std::unique_ptr<DatabaseObject>>& getChildren() const = 0;
    virtual void accept(DatabaseObjectVisitor* visitor) = 0;
};

// Leaf node
class TableObject : public DatabaseObject {
private:
    std::string name_;
    std::string schema_;

public:
    TableObject(const std::string& name, const std::string& schema)
        : name_(name), schema_(schema) {}

    std::string getName() const override {
        return schema_ + "." + name_;
    }

    std::string getType() const override {
        return "TABLE";
    }

    void addChild(std::unique_ptr<DatabaseObject> child) override {
        // Tables don't have children in this example
        throw std::runtime_error("Tables cannot have children");
    }

    const std::vector<std::unique_ptr<DatabaseObject>>& getChildren() const override {
        static const std::vector<std::unique_ptr<DatabaseObject>> empty;
        return empty;
    }

    void accept(DatabaseObjectVisitor* visitor) override {
        visitor->visit(this);
    }
};

// Composite node
class SchemaObject : public DatabaseObject {
private:
    std::string name_;
    std::vector<std::unique_ptr<DatabaseObject>> children_;

public:
    explicit SchemaObject(const std::string& name) : name_(name) {}

    std::string getName() const override {
        return name_;
    }

    std::string getType() const override {
        return "SCHEMA";
    }

    void addChild(std::unique_ptr<DatabaseObject> child) override {
        children_.push_back(std::move(child));
    }

    const std::vector<std::unique_ptr<DatabaseObject>>& getChildren() const override {
        return children_;
    }

    void accept(DatabaseObjectVisitor* visitor) override {
        visitor->visit(this);
        for (const auto& child : children_) {
            child->accept(visitor);
        }
    }
};

// Usage
auto publicSchema = std::make_unique<SchemaObject>("public");
publicSchema->addChild(std::make_unique<TableObject>("users", "public"));
publicSchema->addChild(std::make_unique<TableObject>("products", "public"));

auto scratchbirdSchema = std::make_unique<SchemaObject>("scratchbird");
scratchbirdSchema->addChild(std::make_unique<TableObject>("system_tables", "scratchbird"));
```

## Behavioral Patterns

### 1. Observer Pattern

Used for event handling and notifications:

```cpp
// Subject interface
class ISubject {
public:
    virtual ~ISubject() = default;
    virtual void attach(IObserver* observer) = 0;
    virtual void detach(IObserver* observer) = 0;
    virtual void notify(const Event& event) = 0;
};

// Observer interface
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void onEvent(const Event& event) = 0;
};

// Event types
enum class EventType {
    CONNECTION_ESTABLISHED,
    CONNECTION_LOST,
    QUERY_EXECUTED,
    SCHEMA_CHANGED
};

struct Event {
    EventType type;
    std::string source;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> data;
};

// Concrete subject
class ConnectionManager : public ISubject {
private:
    std::vector<IObserver*> observers_;
    mutable std::mutex observersMutex_;

public:
    void attach(IObserver* observer) override {
        std::lock_guard<std::mutex> lock(observersMutex_);
        observers_.push_back(observer);
    }

    void detach(IObserver* observer) override {
        std::lock_guard<std::mutex> lock(observersMutex_);
        observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
    }

    void notify(const Event& event) override {
        std::lock_guard<std::mutex> lock(observersMutex_);
        for (IObserver* observer : observers_) {
            observer->onEvent(event);
        }
    }

    void connect(const ConnectionInfo& info) {
        // Connection logic...

        // Notify observers
        Event event{
            EventType::CONNECTION_ESTABLISHED,
            "ConnectionManager",
            std::chrono::system_clock::now(),
            {{"database", info.database}, {"host", info.host}}
        };
        notify(event);
    }
};

// Concrete observer
class ConnectionMonitor : public IObserver {
public:
    void onEvent(const Event& event) override {
        switch (event.type) {
            case EventType::CONNECTION_ESTABLISHED:
                Logger::info("Connection established to " + event.data.at("database"));
                break;
            case EventType::CONNECTION_LOST:
                Logger::error("Connection lost to " + event.data.at("database"));
                break;
            default:
                break;
        }
    }
};

// Usage
auto connectionManager = std::make_unique<ConnectionManager>();
auto monitor = std::make_unique<ConnectionMonitor>();

connectionManager->attach(monitor.get());
connectionManager->connect(connectionInfo);
```

### 2. Strategy Pattern

Used for configurable algorithms:

```cpp
// Strategy interface
class IAuthenticationStrategy {
public:
    virtual ~IAuthenticationStrategy() = default;
    virtual bool authenticate(const std::string& username, const std::string& password) = 0;
    virtual std::string getStrategyName() const = 0;
};

// Concrete strategies
class DatabaseAuthenticationStrategy : public IAuthenticationStrategy {
private:
    std::shared_ptr<IConnection> connection_;

public:
    explicit DatabaseAuthenticationStrategy(std::shared_ptr<IConnection> connection)
        : connection_(connection) {}

    bool authenticate(const std::string& username, const std::string& password) override {
        // Query database for user credentials
        std::string sql = "SELECT password_hash FROM users WHERE username = '" + username + "'";
        auto result = connection_->executeQuery(sql);

        if (result.rows.empty()) {
            return false;
        }

        std::string storedHash = result.rows[0][0];
        return verifyPassword(password, storedHash);
    }

    std::string getStrategyName() const override {
        return "database";
    }
};

class LDAPAuthenticationStrategy : public IAuthenticationStrategy {
private:
    std::string ldapServer_;
    int ldapPort_;

public:
    LDAPAuthenticationStrategy(const std::string& server, int port)
        : ldapServer_(server), ldapPort_(port) {}

    bool authenticate(const std::string& username, const std::string& password) override {
        // LDAP authentication logic
        return performLDAPAuthentication(username, password);
    }

    std::string getStrategyName() const override {
        return "ldap";
    }
};

// Context using the strategy
class Authenticator {
private:
    std::unique_ptr<IAuthenticationStrategy> strategy_;

public:
    void setAuthenticationStrategy(std::unique_ptr<IAuthenticationStrategy> strategy) {
        strategy_ = std::move(strategy);
    }

    bool authenticate(const std::string& username, const std::string& password) {
        if (!strategy_) {
            throw std::runtime_error("No authentication strategy set");
        }

        Logger::info("Authenticating user '" + username + "' using " + strategy_->getStrategyName() + " strategy");
        return strategy_->authenticate(username, password);
    }
};

// Usage
auto authenticator = std::make_unique<Authenticator>();

// Use database authentication
authenticator->setAuthenticationStrategy(
    std::make_unique<DatabaseAuthenticationStrategy>(connection));

// Or switch to LDAP authentication
authenticator->setAuthenticationStrategy(
    std::make_unique<LDAPAuthenticationStrategy>("ldap.company.com", 389));

bool isAuthenticated = authenticator->authenticate(username, password);
```

### 3. Command Pattern

Used for encapsulating operations as objects:

```cpp
// Command interface
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

// Concrete commands
class CreateTableCommand : public ICommand {
private:
    std::shared_ptr<IConnection> connection_;
    std::string sql_;
    std::string undoSql_;

public:
    CreateTableCommand(std::shared_ptr<IConnection> connection,
                      const std::string& createSql,
                      const std::string& dropSql)
        : connection_(connection), sql_(createSql), undoSql_(dropSql) {}

    void execute() override {
        connection_->executeNonQuery(sql_);
        Logger::info("Executed: " + sql_);
    }

    void undo() override {
        connection_->executeNonQuery(undoSql_);
        Logger::info("Undid: " + sql_);
    }

    std::string getDescription() const override {
        return "Create table: " + sql_;
    }
};

// Command manager
class CommandManager {
private:
    std::vector<std::unique_ptr<ICommand>> commandHistory_;
    size_t currentIndex_ = 0;
    mutable std::mutex mutex_;

public:
    void executeCommand(std::unique_ptr<ICommand> command) {
        std::lock_guard<std::mutex> lock(mutex_);

        command->execute();

        // Remove any commands after current index (for redo)
        commandHistory_.resize(currentIndex_);
        commandHistory_.push_back(std::move(command));
        currentIndex_ = commandHistory_.size();
    }

    bool undo() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (currentIndex_ > 0) {
            currentIndex_--;
            commandHistory_[currentIndex_]->undo();
            return true;
        }

        return false;
    }

    bool redo() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (currentIndex_ < commandHistory_.size()) {
            commandHistory_[currentIndex_]->execute();
            currentIndex_++;
            return true;
        }

        return false;
    }

    std::vector<std::string> getCommandHistory() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> history;

        for (const auto& command : commandHistory_) {
            history.push_back(command->getDescription());
        }

        return history;
    }
};

// Usage
auto commandManager = std::make_unique<CommandManager>();

std::string createSql = "CREATE TABLE users (id SERIAL PRIMARY KEY, name TEXT)";
std::string dropSql = "DROP TABLE IF EXISTS users";

auto createCommand = std::make_unique<CreateTableCommand>(connection, createSql, dropSql);
commandManager->executeCommand(std::move(createCommand));

// Later...
commandManager->undo();  // Drops the table
commandManager->redo();  // Creates the table again
```

## Concurrency Patterns

### 1. Thread-Safe Singleton Pattern

```cpp
class ThreadSafeLogger {
public:
    static ThreadSafeLogger& getInstance() {
        static std::unique_ptr<ThreadSafeLogger> instance;
        static std::once_flag initFlag;

        std::call_once(initFlag, []() {
            instance = std::make_unique<ThreadSafeLogger>();
        });

        return *instance;
    }

    void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Thread-safe logging implementation
    }

private:
    ThreadSafeLogger() = default;
    ~ThreadSafeLogger() = default;

    mutable std::mutex mutex_;
    std::ofstream logFile_;

    // Prevent copying
    ThreadSafeLogger(const ThreadSafeLogger&) = delete;
    ThreadSafeLogger& operator=(const ThreadSafeLogger&) = delete;
};
```

### 2. Producer-Consumer Pattern

```cpp
template<typename T>
class BlockingQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool shutdown_ = false;

public:
    void push(const T& item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(item);
        }
        condition_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this]() {
            return !queue_.empty() || shutdown_;
        });

        if (shutdown_) {
            throw std::runtime_error("Queue has been shutdown");
        }

        T item = queue_.front();
        queue_.pop();
        return item;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        condition_.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
};

// Usage in connection pool
class AsyncConnectionPool {
private:
    BlockingQueue<ConnectionRequest> requestQueue_;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_;

public:
    AsyncConnectionPool(size_t numWorkers) : running_(true) {
        for (size_t i = 0; i < numWorkers; ++i) {
            workerThreads_.emplace_back(&AsyncConnectionPool::workerLoop, this);
        }
    }

    ~AsyncConnectionPool() {
        shutdown();
    }

    std::future<std::shared_ptr<IConnection>> getConnection(const ConnectionInfo& info) {
        auto promise = std::make_shared<std::promise<std::shared_ptr<IConnection>>>();
        std::future<std::shared_ptr<IConnection>> future = promise->get_future();

        ConnectionRequest request{info, promise};
        requestQueue_.push(request);

        return future;
    }

    void shutdown() {
        running_ = false;
        requestQueue_.shutdown();

        for (auto& thread : workerThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    struct ConnectionRequest {
        ConnectionInfo info;
        std::shared_ptr<std::promise<std::shared_ptr<IConnection>>> promise;
    };

    void workerLoop() {
        while (running_) {
            try {
                ConnectionRequest request = requestQueue_.pop();
                auto connection = createConnection(request.info);
                request.promise->set_value(connection);
            } catch (const std::exception& e) {
                if (running_) {
                    Logger::error("Error in connection pool worker: " + std::string(e.what()));
                }
            }
        }
    }
};
```

## Error Handling Patterns

### 1. Exception Hierarchy Pattern

```cpp
class ScratchRobinException : public std::runtime_error {
public:
    explicit ScratchRobinException(const std::string& message,
                                 ErrorCode code = ErrorCode::GENERIC,
                                 const std::string& component = "")
        : std::runtime_error(message)
        , code_(code)
        , component_(component)
        , timestamp_(std::chrono::system_clock::now()) {}

    ErrorCode getErrorCode() const { return code_; }
    std::string getComponent() const { return component_; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }

    virtual std::string getErrorCategory() const = 0;

private:
    ErrorCode code_;
    std::string component_;
    std::chrono::system_clock::time_point timestamp_;
};

class ConnectionException : public ScratchRobinException {
public:
    using ScratchRobinException::ScratchRobinException;

    std::string getErrorCategory() const override {
        return "CONNECTION";
    }
};

class QueryException : public ScratchRobinException {
public:
    using ScratchRobinException::ScratchRobinException;

    std::string getErrorCategory() const override {
        return "QUERY";
    }
};

// Usage
try {
    connectionManager->connect(connectionInfo);
} catch (const ConnectionException& e) {
    Logger::error("Connection failed in " + e.getComponent() + ": " + e.what());
    // Handle connection error
} catch (const ScratchRobinException& e) {
    Logger::error("ScratchRobin error in " + e.getComponent() + ": " + e.what());
    // Handle generic ScratchRobin error
} catch (const std::exception& e) {
    Logger::error("Unexpected error: " + std::string(e.what()));
    // Handle unexpected errors
}
```

### 2. Result Pattern

Used for operations that can fail gracefully:

```cpp
template<typename T, typename E = std::string>
class Result {
private:
    std::variant<T, E> data_;
    bool isSuccess_;

public:
    static Result<T, E> success(const T& value) {
        Result result;
        result.data_ = value;
        result.isSuccess_ = true;
        return result;
    }

    static Result<T, E> failure(const E& error) {
        Result result;
        result.data_ = error;
        result.isSuccess_ = false;
        return result;
    }

    bool isSuccess() const { return isSuccess_; }
    bool isFailure() const { return !isSuccess_; }

    const T& getValue() const {
        if (!isSuccess_) {
            throw std::runtime_error("Cannot get value from failed result");
        }
        return std::get<T>(data_);
    }

    const E& getError() const {
        if (isSuccess_) {
            throw std::runtime_error("Cannot get error from successful result");
        }
        return std::get<E>(data_);
    }

    // Functional programming style
    template<typename Func>
    auto map(Func func) const -> Result<decltype(func(std::declval<T>())), E> {
        if (isSuccess_) {
            return Result<decltype(func(std::declval<T>())), E>::success(func(getValue()));
        } else {
            return Result<decltype(func(std::declval<T>())), E>::failure(getError());
        }
    }

    template<typename Func>
    auto flatMap(Func func) const -> decltype(func(std::declval<T>())) {
        if (isSuccess_) {
            return func(getValue());
        } else {
            return decltype(func(std::declval<T>()))::failure(getError());
        }
    }
};

// Usage
Result<std::shared_ptr<IConnection>, std::string> connectToDatabase(const ConnectionInfo& info) {
    try {
        auto connection = connectionManager->connect(info);
        return Result<std::shared_ptr<IConnection>, std::string>::success(connection);
    } catch (const std::exception& e) {
        return Result<std::shared_ptr<IConnection>, std::string>::failure(e.what());
    }
}

// Usage with functional style
auto result = connectToDatabase(connectionInfo)
    .map([](auto connection) {
        return connection->executeQuery("SELECT * FROM users");
    })
    .map([](const QueryResult& result) {
        return processQueryResult(result);
    });

if (result.isSuccess()) {
    // Handle success
    auto processedData = result.getValue();
} else {
    // Handle error
    Logger::error("Database operation failed: " + result.getError());
}
```

## Configuration Patterns

### 1. Configuration Builder Pattern

```cpp
class ConfigurationBuilder {
private:
    std::unordered_map<std::string, std::string> properties_;

public:
    ConfigurationBuilder& setDatabaseHost(const std::string& host) {
        properties_["database.host"] = host;
        return *this;
    }

    ConfigurationBuilder& setDatabasePort(int port) {
        properties_["database.port"] = std::to_string(port);
        return *this;
    }

    ConfigurationBuilder& setConnectionPoolSize(size_t size) {
        properties_["pool.size"] = std::to_string(size);
        return *this;
    }

    ConfigurationBuilder& enableSSL(bool enabled) {
        properties_["ssl.enabled"] = enabled ? "true" : "false";
        return *this;
    }

    ConfigurationBuilder& setLogLevel(const std::string& level) {
        properties_["logging.level"] = level;
        return *this;
    }

    std::unique_ptr<IConfiguration> build() const {
        return std::make_unique<Configuration>(properties_);
    }
};

// Usage
auto config = ConfigurationBuilder()
    .setDatabaseHost("localhost")
    .setDatabasePort(5432)
    .setConnectionPoolSize(20)
    .enableSSL(true)
    .setLogLevel("INFO")
    .build();
```

### 2. Environment-Aware Configuration Pattern

```cpp
class EnvironmentAwareConfiguration : public IConfiguration {
private:
    std::unique_ptr<IConfiguration> baseConfig_;
    std::string environment_;

public:
    EnvironmentAwareConfiguration(std::unique_ptr<IConfiguration> baseConfig,
                                const std::string& environment)
        : baseConfig_(std::move(baseConfig))
        , environment_(environment) {}

    std::string getString(const std::string& key, const std::string& defaultValue = "") const override {
        // Try environment-specific key first
        std::string envKey = key + "." + environment_;
        std::string value = baseConfig_->getString(envKey);
        if (!value.empty()) {
            return value;
        }

        // Fall back to base key
        return baseConfig_->getString(key, defaultValue);
    }

    int getInt(const std::string& key, int defaultValue = 0) const override {
        // Similar logic for int values
        return std::stoi(getString(key, std::to_string(defaultValue)));
    }
};

// Usage
auto baseConfig = loadConfiguration("config.yaml");
auto config = std::make_unique<EnvironmentAwareConfiguration>(
    baseConfig, "production");

// This will look for:
// 1. database.host.production
// 2. database.host (if production-specific not found)
std::string host = config->getString("database.host", "localhost");
```

This comprehensive design patterns document provides the foundation for consistent, maintainable, and scalable code throughout the ScratchRobin project. The patterns cover all major aspects of software design including creational, structural, behavioral, and concurrency patterns, along with specific patterns for error handling, configuration, and domain-specific concerns.
