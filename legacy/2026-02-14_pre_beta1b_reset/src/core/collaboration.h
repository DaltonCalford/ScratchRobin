/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_COLLABORATION_H
#define SCRATCHROBIN_COLLABORATION_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <optional>

namespace scratchrobin {

// Forward declarations
class CollaborationClient;
class CollaborationServer;
class CollaborationDocument;

// ============================================================================
// User Presence
// ============================================================================
struct UserPresence {
    std::string user_id;
    std::string user_name;
    std::string user_color;  // Hex color for cursor/selection
    
    // Cursor position
    struct Cursor {
        std::string document_id;
        int line = 0;
        int column = 0;
        std::chrono::steady_clock::time_point updated_at;
    };
    Cursor cursor;
    
    // Selection
    struct Selection {
        std::string document_id;
        int start_line = 0;
        int start_column = 0;
        int end_line = 0;
        int end_column = 0;
    };
    std::optional<Selection> selection;
    
    // Status
    enum class Status { ACTIVE, IDLE, AWAY };
    Status status = Status::ACTIVE;
    std::chrono::steady_clock::time_point last_activity;
    
    bool IsActive() const;
    std::string GetStatusString() const;
};

// ============================================================================
// Operation Types
// ============================================================================
enum class OperationType {
    INSERT,
    DELETE,
    REPLACE,
    CURSOR_MOVE,
    SELECTION_CHANGE,
    LOCK_ACQUIRE,
    LOCK_RELEASE,
    COMMENT_ADD,
    COMMENT_DELETE,
    COMMENT_UPDATE,
    DOCUMENT_SYNC
};

// ============================================================================
// Operation
// ============================================================================
struct Operation {
    std::string id;
    std::string user_id;
    std::string document_id;
    OperationType type;
    
    // Position
    int line = 0;
    int column = 0;
    
    // Content
    std::string content;           // For INSERT
    std::string old_content;       // For REPLACE/DELETE
    int length = 0;                // For DELETE
    
    // Timestamp
    std::chrono::steady_clock::time_point timestamp;
    int sequence_number = 0;       // For ordering
    
    // For conflict resolution
    std::vector<std::string> depends_on;  // Operation IDs this depends on
    
    static Operation Insert(const std::string& user_id, const std::string& doc_id,
                           int line, int column, const std::string& content);
    static Operation Delete(const std::string& user_id, const std::string& doc_id,
                           int line, int column, int length);
    static Operation Replace(const std::string& user_id, const std::string& doc_id,
                            int line, int column, int length, const std::string& content);
    static Operation CursorMove(const std::string& user_id, const std::string& doc_id,
                               int line, int column);
};

// ============================================================================
// Operational Transformation
// ============================================================================
class OperationalTransform {
public:
    // Transform operation A against operation B
    // Returns pair of (transformed A, transformed B)
    static std::pair<Operation, Operation> Transform(const Operation& op_a,
                                                      const Operation& op_b);
    
    // Transform an operation against a list of operations
    static Operation TransformAgainst(const Operation& op,
                                       const std::vector<Operation>& ops);
    
    // Compose two operations into one
    static Operation Compose(const Operation& op_a, const Operation& op_b);
    
private:
    static Operation TransformInsertInsert(const Operation& a, const Operation& b);
    static Operation TransformInsertDelete(const Operation& a, const Operation& b);
    static Operation TransformDeleteDelete(const Operation& a, const Operation& b);
    static Operation TransformDeleteInsert(const Operation& a, const Operation& b);
};

// ============================================================================
// Conflict Resolution
// ============================================================================
class ConflictResolver {
public:
    struct Conflict {
        Operation local_op;
        Operation remote_op;
        std::string description;
    };
    
    struct Resolution {
        std::vector<Operation> operations;
        bool resolved = false;
        std::string resolution_strategy;
    };
    
    // Detect conflicts between local and remote operations
    static std::vector<Conflict> DetectConflicts(
        const std::vector<Operation>& local_ops,
        const std::vector<Operation>& remote_ops);
    
    // Resolve conflicts automatically
    static Resolution ResolveAutomatic(const Conflict& conflict);
    
    // Resolution strategies
    static Resolution LastWriteWins(const Conflict& conflict);
    static Resolution FirstWriteWins(const Conflict& conflict);
    static Resolution Merge(const Conflict& conflict);
    static Resolution ManualResolution(const Conflict& conflict);
};

// ============================================================================
// Lock Manager
// ============================================================================
class LockManager {
public:
    enum class LockType { NONE, READ, WRITE };
    
    struct Lock {
        std::string resource_id;  // Document, table, row, etc.
        std::string user_id;
        LockType type;
        std::chrono::steady_clock::time_point acquired_at;
        std::chrono::seconds timeout{30};
    };
    
    // Try to acquire a lock
    bool AcquireLock(const std::string& resource_id, const std::string& user_id,
                     LockType type);
    
    // Release a lock
    void ReleaseLock(const std::string& resource_id, const std::string& user_id);
    
    // Check if resource is locked
    bool IsLocked(const std::string& resource_id) const;
    
    // Get lock holder
    std::optional<Lock> GetLock(const std::string& resource_id) const;
    
    // Get all locks for a user
    std::vector<Lock> GetUserLocks(const std::string& user_id) const;
    
    // Cleanup expired locks
    void CleanupExpired();
    
private:
    std::map<std::string, Lock> locks_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Collaboration Document
// ============================================================================
class CollaborationDocument {
public:
    std::string id;
    std::string name;
    std::string owner_id;
    
    // Content
    std::vector<std::string> lines;
    
    // Operation history
    std::vector<Operation> history;
    int current_sequence = 0;
    
    // Active users
    std::map<std::string, UserPresence> active_users;
    
    // Comments
    struct Comment {
        std::string id;
        std::string user_id;
        int line;
        int column;
        std::string text;
        std::chrono::steady_clock::time_point created_at;
        std::vector<Comment> replies;
    };
    std::vector<Comment> comments;
    
    // Apply operation
    bool ApplyOperation(const Operation& op);
    
    // Transform and apply remote operation
    bool ApplyRemoteOperation(const Operation& op);
    
    // Get document state at sequence number
    std::vector<std::string> GetStateAt(int sequence) const;
    
    // Get operations since sequence
    std::vector<Operation> GetOperationsSince(int sequence) const;
    
    // Sync with server
    struct SyncResult {
        std::vector<Operation> server_operations;
        std::vector<Operation> client_operations;
        int server_sequence;
        bool success = false;
    };
    SyncResult Sync(int client_sequence, const std::vector<Operation>& pending_ops);
    
    // Serialization
    void ToJson(std::ostream& out) const;
    static std::unique_ptr<CollaborationDocument> FromJson(const std::string& json);
};

// ============================================================================
// Collaboration Client
// ============================================================================
class CollaborationClient {
public:
    using ConnectionCallback = std::function<void(bool connected)>;
    using OperationCallback = std::function<void(const Operation& op)>;
    using PresenceCallback = std::function<void(const UserPresence& presence)>;
    using CommentCallback = std::function<void(const CollaborationDocument::Comment& comment)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    CollaborationClient();
    ~CollaborationClient();
    
    // Connection
    void Connect(const std::string& server_url, const std::string& user_id,
                 const std::string& user_name);
    void Disconnect();
    bool IsConnected() const { return connected_; }
    
    // Document operations
    void JoinDocument(const std::string& document_id);
    void LeaveDocument(const std::string& document_id);
    void SendOperation(const Operation& op);
    void SendCursorMove(int line, int column);
    void SendSelection(int start_line, int start_col, int end_line, int end_col);
    
    // Comments
    void AddComment(int line, int column, const std::string& text);
    void DeleteComment(const std::string& comment_id);
    void ReplyToComment(const std::string& parent_id, const std::string& text);
    
    // Locks
    bool RequestLock(const std::string& resource_id, LockManager::LockType type);
    void ReleaseLock(const std::string& resource_id);
    
    // Callbacks
    void SetConnectionCallback(ConnectionCallback cb) { on_connected_ = cb; }
    void SetOperationCallback(OperationCallback cb) { on_operation_ = cb; }
    void SetPresenceCallback(PresenceCallback cb) { on_presence_ = cb; }
    void SetCommentCallback(CommentCallback cb) { on_comment_ = cb; }
    void SetErrorCallback(ErrorCallback cb) { on_error_ = cb; }
    
    // Get current users
    std::vector<UserPresence> GetActiveUsers() const;
    
    // Get pending operations
    std::vector<Operation> GetPendingOperations() const;
    
private:
    void ProcessMessage(const std::string& message);
    void SendMessage(const std::string& message);
    void OnConnected();
    void OnDisconnected();
    void OnError(const std::string& error);
    void Heartbeat();
    
    std::string server_url_;
    std::string user_id_;
    std::string user_name_;
    std::string current_document_id_;
    bool connected_ = false;
    
    // Pending operations (not yet acknowledged by server)
    std::vector<Operation> pending_operations_;
    
    // Callbacks
    ConnectionCallback on_connected_;
    OperationCallback on_operation_;
    PresenceCallback on_presence_;
    CommentCallback on_comment_;
    ErrorCallback on_error_;
    
    // Would have WebSocket client here
    // std::unique_ptr<WebSocketClient> ws_client_;
};

// ============================================================================
// Collaboration Server
// ============================================================================
class CollaborationServer {
public:
    struct Session {
        std::string session_id;
        std::string user_id;
        std::string user_name;
        std::string current_document_id;
        std::chrono::steady_clock::time_point connected_at;
        std::chrono::steady_clock::time_point last_activity;
    };
    
    CollaborationServer();
    ~CollaborationServer();
    
    // Server lifecycle
    bool Start(int port);
    void Stop();
    bool IsRunning() const { return running_; }
    
    // Document management
    void CreateDocument(const std::string& id, const std::string& name);
    void DeleteDocument(const std::string& id);
    std::shared_ptr<CollaborationDocument> GetDocument(const std::string& id);
    std::vector<std::string> ListDocuments() const;
    
    // Session management
    void BroadcastToDocument(const std::string& document_id, 
                             const std::string& message,
                             const std::string& exclude_session);
    void BroadcastPresence(const std::string& document_id);
    
    // Statistics
    struct Stats {
        int active_sessions = 0;
        int active_documents = 0;
        int total_operations = 0;
        std::chrono::seconds uptime{0};
    };
    Stats GetStats() const;
    
private:
    void HandleNewConnection(/* WebSocket connection */);
    void HandleMessage(const std::string& session_id, const std::string& message);
    void HandleDisconnect(const std::string& session_id);
    void CleanupSessions();
    
    bool running_ = false;
    int port_ = 0;
    std::chrono::steady_clock::time_point start_time_;
    
    // Documents
    std::map<std::string, std::shared_ptr<CollaborationDocument>> documents_;
    
    // Sessions
    std::map<std::string, Session> sessions_;
    
    // Lock manager
    LockManager lock_manager_;
    
    mutable std::mutex mutex_;
};

// ============================================================================
// Collaboration Manager (singleton)
// ============================================================================
class CollaborationManager {
public:
    static CollaborationManager& Instance();
    
    // Client
    CollaborationClient* GetClient() { return client_.get(); }
    void InitializeClient(const std::string& server_url, 
                          const std::string& user_id,
                          const std::string& user_name);
    
    // Server
    CollaborationServer* GetServer() { return server_.get(); }
    void StartServer(int port);
    void StopServer();
    
    // Settings
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    
    void SetUserInfo(const std::string& user_id, const std::string& user_name,
                     const std::string& color);
    
private:
    CollaborationManager() = default;
    ~CollaborationManager() = default;
    
    bool enabled_ = false;
    std::unique_ptr<CollaborationClient> client_;
    std::unique_ptr<CollaborationServer> server_;
    
    std::string user_id_;
    std::string user_name_;
    std::string user_color_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_COLLABORATION_H
