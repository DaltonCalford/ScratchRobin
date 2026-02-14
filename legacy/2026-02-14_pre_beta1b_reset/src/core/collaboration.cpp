/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "collaboration.h"

#include <algorithm>
#include <sstream>
#include <thread>

namespace scratchrobin {

// ============================================================================
// User Presence Implementation
// ============================================================================
bool UserPresence::IsActive() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_activity).count();
    return elapsed < 300;  // 5 minutes
}

std::string UserPresence::GetStatusString() const {
    switch (status) {
        case Status::ACTIVE: return "active";
        case Status::IDLE: return "idle";
        case Status::AWAY: return "away";
        default: return "unknown";
    }
}

// ============================================================================
// Operation Factory Methods
// ============================================================================
Operation Operation::Insert(const std::string& user_id, const std::string& doc_id,
                            int line, int column, const std::string& content) {
    Operation op;
    op.id = "op_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    op.user_id = user_id;
    op.document_id = doc_id;
    op.type = OperationType::INSERT;
    op.line = line;
    op.column = column;
    op.content = content;
    op.timestamp = std::chrono::steady_clock::now();
    return op;
}

Operation Operation::Delete(const std::string& user_id, const std::string& doc_id,
                            int line, int column, int length) {
    Operation op;
    op.id = "op_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    op.user_id = user_id;
    op.document_id = doc_id;
    op.type = OperationType::DELETE;
    op.line = line;
    op.column = column;
    op.length = length;
    op.timestamp = std::chrono::steady_clock::now();
    return op;
}

Operation Operation::Replace(const std::string& user_id, const std::string& doc_id,
                             int line, int column, int length, const std::string& content) {
    Operation op;
    op.id = "op_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    op.user_id = user_id;
    op.document_id = doc_id;
    op.type = OperationType::REPLACE;
    op.line = line;
    op.column = column;
    op.length = length;
    op.content = content;
    op.timestamp = std::chrono::steady_clock::now();
    return op;
}

Operation Operation::CursorMove(const std::string& user_id, const std::string& doc_id,
                                int line, int column) {
    Operation op;
    op.id = "op_cursor_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    op.user_id = user_id;
    op.document_id = doc_id;
    op.type = OperationType::CURSOR_MOVE;
    op.line = line;
    op.column = column;
    op.timestamp = std::chrono::steady_clock::now();
    return op;
}

// ============================================================================
// Operational Transform Implementation
// ============================================================================
std::pair<Operation, Operation> OperationalTransform::Transform(const Operation& op_a,
                                                                 const Operation& op_b) {
    Operation transformed_a = op_a;
    Operation transformed_b = op_b;
    
    // Different documents - no transformation needed
    if (op_a.document_id != op_b.document_id) {
        return {transformed_a, transformed_b};
    }
    
    // Transform based on operation types
    if (op_a.type == OperationType::INSERT && op_b.type == OperationType::INSERT) {
        transformed_a = TransformInsertInsert(op_a, op_b);
        transformed_b = TransformInsertInsert(op_b, op_a);
    } else if (op_a.type == OperationType::INSERT && op_b.type == OperationType::DELETE) {
        transformed_a = TransformInsertDelete(op_a, op_b);
        transformed_b = TransformDeleteInsert(op_b, op_a);
    } else if (op_a.type == OperationType::DELETE && op_b.type == OperationType::INSERT) {
        transformed_a = TransformDeleteInsert(op_a, op_b);
        transformed_b = TransformInsertDelete(op_b, op_a);
    } else if (op_a.type == OperationType::DELETE && op_b.type == OperationType::DELETE) {
        transformed_a = TransformDeleteDelete(op_a, op_b);
        transformed_b = TransformDeleteDelete(op_b, op_a);
    }
    
    return {transformed_a, transformed_b};
}

Operation OperationalTransform::TransformAgainst(const Operation& op,
                                                  const std::vector<Operation>& ops) {
    Operation result = op;
    
    for (const auto& other : ops) {
        auto [transformed, _] = Transform(result, other);
        result = transformed;
    }
    
    return result;
}

Operation OperationalTransform::TransformInsertInsert(const Operation& a, 
                                                       const Operation& b) {
    Operation result = a;
    
    // If both insert at same position, use user ID for tiebreaker
    if (a.line == b.line && a.column == b.column) {
        if (a.user_id > b.user_id) {
            result.column += static_cast<int>(b.content.length());
        }
    } else if (a.line == b.line && a.column > b.column) {
        result.column += static_cast<int>(b.content.length());
    } else if (a.line > b.line) {
        // Count newlines in b's content
        int newlines = std::count(b.content.begin(), b.content.end(), '\n');
        result.line += newlines;
    }
    
    return result;
}

Operation OperationalTransform::TransformInsertDelete(const Operation& a,
                                                       const Operation& b) {
    Operation result = a;
    
    if (a.line == b.line && a.column > b.column) {
        result.column = std::max(0, result.column - b.length);
    } else if (a.line > b.line) {
        result.line--;
    }
    
    return result;
}

Operation OperationalTransform::TransformDeleteDelete(const Operation& a,
                                                       const Operation& b) {
    Operation result = a;
    
    if (a.line == b.line) {
        if (a.column >= b.column + b.length) {
            result.column -= b.length;
        } else if (a.column + a.length <= b.column) {
            // No overlap - position unchanged
        } else {
            // Overlapping deletes - complex case
            // Simplified: reduce length
            int overlap_start = std::max(a.column, b.column);
            int overlap_end = std::min(a.column + a.length, b.column + b.length);
            result.length -= (overlap_end - overlap_start);
        }
    } else if (a.line > b.line) {
        result.line--;
    }
    
    return result;
}

Operation OperationalTransform::TransformDeleteInsert(const Operation& a,
                                                       const Operation& b) {
    Operation result = a;
    
    if (a.line == b.line && a.column >= b.column) {
        result.column += static_cast<int>(b.content.length());
    } else if (a.line >= b.line) {
        int newlines = std::count(b.content.begin(), b.content.end(), '\n');
        result.line += newlines;
    }
    
    return result;
}

Operation OperationalTransform::Compose(const Operation& op_a, const Operation& op_b) {
    // Combine two operations into one
    Operation result = op_a;
    result.id = "composed_" + op_a.id + "_" + op_b.id;
    result.timestamp = std::chrono::steady_clock::now();
    return result;
}

// ============================================================================
// Conflict Resolution Implementation
// ============================================================================
std::vector<ConflictResolver::Conflict> ConflictResolver::DetectConflicts(
    const std::vector<Operation>& local_ops,
    const std::vector<Operation>& remote_ops) {
    
    std::vector<Conflict> conflicts;
    
    for (const auto& local : local_ops) {
        for (const auto& remote : remote_ops) {
            // Check for overlapping regions
            if (local.document_id != remote.document_id) continue;
            if (local.line != remote.line) continue;
            
            bool overlap = (local.column < remote.column + remote.length &&
                          local.column + local.length > remote.column);
            
            if (overlap) {
                Conflict conflict;
                conflict.local_op = local;
                conflict.remote_op = remote;
                conflict.description = "Overlapping operations on same region";
                conflicts.push_back(conflict);
            }
        }
    }
    
    return conflicts;
}

ConflictResolver::Resolution ConflictResolver::ResolveAutomatic(const Conflict& conflict) {
    // Default to last-write-wins
    return LastWriteWins(conflict);
}

ConflictResolver::Resolution ConflictResolver::LastWriteWins(const Conflict& conflict) {
    Resolution res;
    
    // Use the operation with later timestamp
    if (conflict.local_op.timestamp > conflict.remote_op.timestamp) {
        res.operations.push_back(conflict.local_op);
    } else {
        res.operations.push_back(conflict.remote_op);
    }
    
    res.resolved = true;
    res.resolution_strategy = "last_write_wins";
    return res;
}

ConflictResolver::Resolution ConflictResolver::FirstWriteWins(const Conflict& conflict) {
    Resolution res;
    
    // Use the operation with earlier timestamp
    if (conflict.local_op.timestamp < conflict.remote_op.timestamp) {
        res.operations.push_back(conflict.local_op);
    } else {
        res.operations.push_back(conflict.remote_op);
    }
    
    res.resolved = true;
    res.resolution_strategy = "first_write_wins";
    return res;
}

ConflictResolver::Resolution ConflictResolver::Merge(const Conflict& conflict) {
    Resolution res;
    
    // For inserts, try to merge the content
    if (conflict.local_op.type == OperationType::INSERT &&
        conflict.remote_op.type == OperationType::INSERT) {
        // Simple merge: concatenate
        Operation merged = conflict.local_op;
        merged.content = conflict.local_op.content + conflict.remote_op.content;
        merged.id = "merged_" + conflict.local_op.id;
        res.operations.push_back(merged);
        res.resolved = true;
    }
    
    res.resolution_strategy = "merge";
    return res;
}

// ============================================================================
// Lock Manager Implementation
// ============================================================================
bool LockManager::AcquireLock(const std::string& resource_id, const std::string& user_id,
                              LockType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = locks_.find(resource_id);
    if (it != locks_.end()) {
        // Resource already locked
        const Lock& existing = it->second;
        
        // Check if lock expired
        auto now = std::chrono::steady_clock::now();
        if (now - existing.acquired_at > existing.timeout) {
            // Lock expired - can acquire
        } else if (existing.user_id == user_id) {
            // User already has lock - upgrade/downgrade
        } else {
            // Another user has the lock
            return false;
        }
    }
    
    Lock new_lock;
    new_lock.resource_id = resource_id;
    new_lock.user_id = user_id;
    new_lock.type = type;
    new_lock.acquired_at = std::chrono::steady_clock::now();
    
    locks_[resource_id] = new_lock;
    return true;
}

void LockManager::ReleaseLock(const std::string& resource_id, const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = locks_.find(resource_id);
    if (it != locks_.end() && it->second.user_id == user_id) {
        locks_.erase(it);
    }
}

bool LockManager::IsLocked(const std::string& resource_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = locks_.find(resource_id);
    if (it == locks_.end()) return false;
    
    // Check if expired
    auto now = std::chrono::steady_clock::now();
    return (now - it->second.acquired_at) <= it->second.timeout;
}

std::optional<LockManager::Lock> LockManager::GetLock(
    const std::string& resource_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = locks_.find(resource_id);
    if (it != locks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<LockManager::Lock> LockManager::GetUserLocks(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Lock> result;
    for (const auto& [resource, l] : locks_) {
        if (l.user_id == user_id) {
            result.push_back(l);
        }
    }
    return result;
}

void LockManager::CleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = locks_.begin(); it != locks_.end();) {
        if (now - it->second.acquired_at > it->second.timeout) {
            it = locks_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Collaboration Document Implementation
// ============================================================================
bool CollaborationDocument::ApplyOperation(const Operation& op) {
    // Transform against pending operations
    Operation transformed = op;
    
    // Apply to document
    switch (op.type) {
        case OperationType::INSERT:
            if (op.line >= 0 && op.line < static_cast<int>(lines.size())) {
                std::string& line = lines[op.line];
                if (op.column >= 0 && op.column <= static_cast<int>(line.length())) {
                    line.insert(op.column, op.content);
                }
            }
            break;
            
        case OperationType::DELETE:
            if (op.line >= 0 && op.line < static_cast<int>(lines.size())) {
                std::string& line = lines[op.line];
                if (op.column >= 0 && op.column + op.length <= static_cast<int>(line.length())) {
                    line.erase(op.column, op.length);
                }
            }
            break;
            
        case OperationType::REPLACE:
            if (op.line >= 0 && op.line < static_cast<int>(lines.size())) {
                std::string& line = lines[op.line];
                if (op.column >= 0 && op.column + op.length <= static_cast<int>(line.length())) {
                    line.replace(op.column, op.length, op.content);
                }
            }
            break;
            
        default:
            break;
    }
    
    // Add to history
    history.push_back(op);
    current_sequence++;
    
    return true;
}

bool CollaborationDocument::ApplyRemoteOperation(const Operation& op) {
    // Transform against local operations
    std::vector<Operation> local_ops;
    for (const auto& h : history) {
        if (h.user_id != op.user_id) {
            local_ops.push_back(h);
        }
    }
    
    Operation transformed = OperationalTransform::TransformAgainst(op, local_ops);
    return ApplyOperation(transformed);
}

std::vector<Operation> CollaborationDocument::GetOperationsSince(int sequence) const {
    std::vector<Operation> result;
    for (size_t i = sequence; i < history.size(); ++i) {
        result.push_back(history[i]);
    }
    return result;
}

CollaborationDocument::SyncResult CollaborationDocument::Sync(
    int client_sequence, const std::vector<Operation>& pending_ops) {
    
    SyncResult result;
    
    // Get server operations client hasn't seen
    result.server_operations = GetOperationsSince(client_sequence);
    
    // Transform and apply client operations
    for (const auto& op : pending_ops) {
        Operation transformed = OperationalTransform::TransformAgainst(op, history);
        ApplyOperation(transformed);
        result.client_operations.push_back(transformed);
    }
    
    result.server_sequence = current_sequence;
    result.success = true;
    
    return result;
}

// ============================================================================
// Collaboration Client Implementation
// ============================================================================
CollaborationClient::CollaborationClient() = default;
CollaborationClient::~CollaborationClient() {
    Disconnect();
}

void CollaborationClient::Connect(const std::string& server_url, 
                                   const std::string& user_id,
                                   const std::string& user_name) {
    server_url_ = server_url;
    user_id_ = user_id;
    user_name_ = user_name;
    
    // Would establish WebSocket connection here
    connected_ = true;
    
    if (on_connected_) {
        on_connected_(true);
    }
}

void CollaborationClient::Disconnect() {
    connected_ = false;
    
    if (on_connected_) {
        on_connected_(false);
    }
}

void CollaborationClient::JoinDocument(const std::string& document_id) {
    current_document_id_ = document_id;
    
    // Would send join message to server
}

void CollaborationClient::LeaveDocument(const std::string& document_id) {
    // Would send leave message to server
    if (current_document_id_ == document_id) {
        current_document_id_.clear();
    }
}

void CollaborationClient::SendOperation(const Operation& op) {
    pending_operations_.push_back(op);
    
    // Would send to server
    SendMessage("{\"type\":\"operation\",\"data\":{}}");
}

void CollaborationClient::SendCursorMove(int line, int column) {
    // Would send cursor position to server
}

void CollaborationClient::SendSelection(int start_line, int start_col,
                                        int end_line, int end_col) {
    // Would send selection to server
}

void CollaborationClient::AddComment(int line, int column, const std::string& text) {
    // Would send comment to server
}

void CollaborationClient::DeleteComment(const std::string& comment_id) {
    // Would send delete comment to server
}

void CollaborationClient::ReplyToComment(const std::string& parent_id,
                                          const std::string& text) {
    // Would send reply to server
}

bool CollaborationClient::RequestLock(const std::string& resource_id,
                                       LockManager::LockType type) {
    // Would request lock from server
    return true;
}

void CollaborationClient::ReleaseLock(const std::string& resource_id) {
    // Would release lock on server
}

std::vector<UserPresence> CollaborationClient::GetActiveUsers() const {
    // Would return cached user list
    return {};
}

std::vector<Operation> CollaborationClient::GetPendingOperations() const {
    return pending_operations_;
}

void CollaborationClient::ProcessMessage(const std::string& message) {
    // Parse and handle incoming message
}

void CollaborationClient::SendMessage(const std::string& message) {
    // Send to server
}

void CollaborationClient::Heartbeat() {
    // Send keepalive
}

// ============================================================================
// Collaboration Server Implementation
// ============================================================================
CollaborationServer::CollaborationServer() = default;
CollaborationServer::~CollaborationServer() {
    Stop();
}

bool CollaborationServer::Start(int port) {
    port_ = port;
    running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    
    // Would start WebSocket server here
    
    return true;
}

void CollaborationServer::Stop() {
    running_ = false;
    sessions_.clear();
}

void CollaborationServer::CreateDocument(const std::string& id, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto doc = std::make_shared<CollaborationDocument>();
    doc->id = id;
    doc->name = name;
    documents_[id] = doc;
}

void CollaborationServer::DeleteDocument(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    documents_.erase(id);
}

std::shared_ptr<CollaborationDocument> CollaborationServer::GetDocument(
    const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = documents_.find(id);
    if (it != documents_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> CollaborationServer::ListDocuments() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    for (const auto& [id, doc] : documents_) {
        result.push_back(id);
    }
    return result;
}

void CollaborationServer::BroadcastToDocument(const std::string& document_id,
                                               const std::string& message,
                                               const std::string& exclude_session) {
    // Would broadcast to all sessions in document
}

void CollaborationServer::BroadcastPresence(const std::string& document_id) {
    // Would broadcast updated presence to all users
}

CollaborationServer::Stats CollaborationServer::GetStats() const {
    Stats stats;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    stats.active_sessions = static_cast<int>(sessions_.size());
    stats.active_documents = static_cast<int>(documents_.size());
    
    for (const auto& [id, doc] : documents_) {
        stats.total_operations += static_cast<int>(doc->history.size());
    }
    
    auto now = std::chrono::steady_clock::now();
    stats.uptime = std::chrono::duration_cast<std::chrono::seconds>(
        now - start_time_);
    
    return stats;
}

void CollaborationServer::HandleMessage(const std::string& session_id,
                                         const std::string& message) {
    // Parse and handle message
}

void CollaborationServer::HandleDisconnect(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
}

void CollaborationServer::CleanupSessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
            now - it->second.last_activity).count();
        if (elapsed > 30) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Collaboration Manager Implementation
// ============================================================================
CollaborationManager& CollaborationManager::Instance() {
    static CollaborationManager instance;
    return instance;
}

void CollaborationManager::InitializeClient(const std::string& server_url,
                                             const std::string& user_id,
                                             const std::string& user_name) {
    client_ = std::make_unique<CollaborationClient>();
    client_->Connect(server_url, user_id, user_name);
}

void CollaborationManager::StartServer(int port) {
    server_ = std::make_unique<CollaborationServer>();
    server_->Start(port);
}

void CollaborationManager::StopServer() {
    if (server_) {
        server_->Stop();
        server_.reset();
    }
}

void CollaborationManager::SetUserInfo(const std::string& user_id,
                                        const std::string& user_name,
                                        const std::string& color) {
    user_id_ = user_id;
    user_name_ = user_name;
    user_color_ = color;
}

} // namespace scratchrobin
