/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "diagram/command.h"

#include <algorithm>

namespace scratchrobin {

// CommandManager implementation
CommandManager::CommandManager(size_t max_history) : max_history_(max_history) {}

void CommandManager::Execute(std::unique_ptr<DiagramCommand> command) {
    command->Execute();
    undo_stack_.push_back(std::move(command));
    
    // Clear redo stack when new command is executed
    redo_stack_.clear();
    
    // Limit history size
    if (undo_stack_.size() > max_history_) {
        undo_stack_.erase(undo_stack_.begin());
        if (saved_index_ > 0) {
            saved_index_--;
        }
    }
}

bool CommandManager::Undo() {
    if (undo_stack_.empty()) {
        return false;
    }
    
    auto command = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    command->Undo();
    redo_stack_.push_back(std::move(command));
    return true;
}

bool CommandManager::Redo() {
    if (redo_stack_.empty()) {
        return false;
    }
    
    auto command = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    command->Execute();
    undo_stack_.push_back(std::move(command));
    return true;
}

bool CommandManager::CanUndo() const {
    return !undo_stack_.empty();
}

bool CommandManager::CanRedo() const {
    return !redo_stack_.empty();
}

std::string CommandManager::GetUndoDescription() const {
    if (undo_stack_.empty()) {
        return "";
    }
    return undo_stack_.back()->GetDescription();
}

std::string CommandManager::GetRedoDescription() const {
    if (redo_stack_.empty()) {
        return "";
    }
    return redo_stack_.back()->GetDescription();
}

void CommandManager::Clear() {
    undo_stack_.clear();
    redo_stack_.clear();
    saved_index_ = 0;
}

void CommandManager::MarkSaved() {
    saved_index_ = undo_stack_.size();
}

bool CommandManager::IsModified() const {
    return undo_stack_.size() != saved_index_;
}

// AddNodeCommand implementation
AddNodeCommand::AddNodeCommand(DiagramModel* model, const DiagramNode& node)
    : model_(model), node_(node) {}

void AddNodeCommand::Execute() {
    if (!executed_) {
        model_->AddNode(node_);
        executed_ = true;
    }
}

void AddNodeCommand::Undo() {
    if (executed_) {
        auto& nodes = model_->nodes();
        auto it = std::find_if(nodes.begin(), nodes.end(),
                               [this](const DiagramNode& n) { return n.id == node_.id; });
        if (it != nodes.end()) {
            nodes.erase(it);
        }
        executed_ = false;
    }
}

// DeleteNodeCommand implementation
DeleteNodeCommand::DeleteNodeCommand(DiagramModel* model, const std::string& node_id)
    : model_(model), node_id_(node_id) {}

void DeleteNodeCommand::Execute() {
    if (executed_) return;
    
    // Find and backup node
    auto& nodes = model_->nodes();
    auto node_it = std::find_if(nodes.begin(), nodes.end(),
                                [this](const DiagramNode& n) { return n.id == node_id_; });
    if (node_it == nodes.end()) return;
    
    node_backup_ = *node_it;
    
    // Backup connected edges
    auto& edges = model_->edges();
    for (const auto& edge : edges) {
        if (edge.source_id == node_id_ || edge.target_id == node_id_) {
            edges_backup_.push_back(edge);
        }
    }
    
    // Remove connected edges
    edges.erase(std::remove_if(edges.begin(), edges.end(),
                               [this](const DiagramEdge& e) {
                                   return e.source_id == node_id_ || e.target_id == node_id_;
                               }), edges.end());
    
    // Remove node
    nodes.erase(node_it);
    executed_ = true;
}

void DeleteNodeCommand::Undo() {
    if (!executed_) return;
    
    // Restore node
    model_->AddNode(node_backup_);
    
    // Restore edges
    auto& edges = model_->edges();
    for (const auto& edge : edges_backup_) {
        edges.push_back(edge);
    }
    
    executed_ = false;
}

// MoveNodeCommand implementation
MoveNodeCommand::MoveNodeCommand(DiagramModel* model, const std::string& node_id,
                                  double old_x, double old_y, double new_x, double new_y)
    : model_(model), node_id_(node_id),
      old_x_(old_x), old_y_(old_y), new_x_(new_x), new_y_(new_y) {}

void MoveNodeCommand::Execute() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->x = new_x_;
        it->y = new_y_;
    }
}

void MoveNodeCommand::Undo() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->x = old_x_;
        it->y = old_y_;
    }
}

// ResizeNodeCommand implementation
ResizeNodeCommand::ResizeNodeCommand(DiagramModel* model, const std::string& node_id,
                                      double old_w, double old_h, double new_w, double new_h)
    : model_(model), node_id_(node_id),
      old_w_(old_w), old_h_(old_h), new_w_(new_w), new_h_(new_h) {}

void ResizeNodeCommand::Execute() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->width = new_w_;
        it->height = new_h_;
    }
}

void ResizeNodeCommand::Undo() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->width = old_w_;
        it->height = old_h_;
    }
}

// AddEdgeCommand implementation
AddEdgeCommand::AddEdgeCommand(DiagramModel* model, const DiagramEdge& edge)
    : model_(model), edge_(edge) {}

void AddEdgeCommand::Execute() {
    if (!executed_) {
        model_->AddEdge(edge_);
        executed_ = true;
    }
}

void AddEdgeCommand::Undo() {
    if (executed_) {
        auto& edges = model_->edges();
        auto it = std::find_if(edges.begin(), edges.end(),
                               [this](const DiagramEdge& e) { return e.id == edge_.id; });
        if (it != edges.end()) {
            edges.erase(it);
        }
        executed_ = false;
    }
}

// DeleteEdgeCommand implementation
DeleteEdgeCommand::DeleteEdgeCommand(DiagramModel* model, const std::string& edge_id)
    : model_(model), edge_id_(edge_id) {}

void DeleteEdgeCommand::Execute() {
    if (executed_) return;
    
    auto& edges = model_->edges();
    auto it = std::find_if(edges.begin(), edges.end(),
                           [this](const DiagramEdge& e) { return e.id == edge_id_; });
    if (it == edges.end()) return;
    
    edge_backup_ = *it;
    edges.erase(it);
    executed_ = true;
}

void DeleteEdgeCommand::Undo() {
    if (!executed_) return;
    
    model_->AddEdge(edge_backup_);
    executed_ = false;
}

// EditNodeNameCommand implementation
EditNodeNameCommand::EditNodeNameCommand(DiagramModel* model, const std::string& node_id,
                                          const std::string& old_name, const std::string& new_name)
    : model_(model), node_id_(node_id), old_name_(old_name), new_name_(new_name) {}

void EditNodeNameCommand::Execute() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->name = new_name_;
    }
}

void EditNodeNameCommand::Undo() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->name = old_name_;
    }
}

// CompoundCommand implementation
void CompoundCommand::AddCommand(std::unique_ptr<DiagramCommand> command) {
    commands_.push_back(std::move(command));
}

void CompoundCommand::Execute() {
    for (auto& cmd : commands_) {
        cmd->Execute();
    }
}

void CompoundCommand::Undo() {
    // Undo in reverse order
    for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
        (*it)->Undo();
    }
}

std::string CompoundCommand::GetDescription() const {
    if (commands_.empty()) {
        return "Multiple changes";
    }
    if (commands_.size() == 1) {
        return commands_[0]->GetDescription();
    }
    return std::to_string(commands_.size()) + " changes";
}

// DeleteMultipleNodesCommand implementation
DeleteMultipleNodesCommand::DeleteMultipleNodesCommand(DiagramModel* model, 
                                                        const std::vector<std::string>& node_ids)
    : model_(model), node_ids_(node_ids) {}

void DeleteMultipleNodesCommand::Execute() {
    if (executed_) return;
    
    // Backup all nodes to be deleted
    auto& nodes = model_->nodes();
    for (const auto& node_id : node_ids_) {
        auto it = std::find_if(nodes.begin(), nodes.end(),
                               [&node_id](const DiagramNode& n) { return n.id == node_id; });
        if (it != nodes.end()) {
            nodes_backup_.push_back(*it);
        }
    }
    
    // Backup all edges connected to any of the nodes being deleted
    auto& edges = model_->edges();
    for (const auto& edge : edges) {
        bool source_deleted = std::find(node_ids_.begin(), node_ids_.end(), edge.source_id) != node_ids_.end();
        bool target_deleted = std::find(node_ids_.begin(), node_ids_.end(), edge.target_id) != node_ids_.end();
        if (source_deleted || target_deleted) {
            edges_backup_.push_back(edge);
        }
    }
    
    // Remove connected edges
    edges.erase(std::remove_if(edges.begin(), edges.end(),
                               [this](const DiagramEdge& e) {
                                   bool source_deleted = std::find(node_ids_.begin(), node_ids_.end(), e.source_id) != node_ids_.end();
                                   bool target_deleted = std::find(node_ids_.begin(), node_ids_.end(), e.target_id) != node_ids_.end();
                                   return source_deleted || target_deleted;
                               }), edges.end());
    
    // Remove nodes
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                               [this](const DiagramNode& n) {
                                   return std::find(node_ids_.begin(), node_ids_.end(), n.id) != node_ids_.end();
                               }), nodes.end());
    
    executed_ = true;
}

void DeleteMultipleNodesCommand::Undo() {
    if (!executed_) return;
    
    // Restore all nodes
    for (const auto& node : nodes_backup_) {
        model_->AddNode(node);
    }
    
    // Restore all edges
    auto& edges = model_->edges();
    for (const auto& edge : edges_backup_) {
        edges.push_back(edge);
    }
    
    executed_ = false;
}

std::string DeleteMultipleNodesCommand::GetDescription() const {
    if (node_ids_.size() == 1) {
        return "Delete node";
    }
    return "Delete " + std::to_string(node_ids_.size()) + " nodes";
}

// ProjectDeleteCommand implementation
ProjectDeleteCommand::ProjectDeleteCommand(DiagramModel* model, 
                                            const std::vector<std::string>& node_ids,
                                            DeleteCallback delete_callback)
    : model_(model), node_ids_(node_ids), delete_callback_(delete_callback) {}

void ProjectDeleteCommand::Execute() {
    if (!delete_callback_) {
        error_message_ = "No delete callback provided";
        success_ = false;
        return;
    }
    
    // Perform the actual database deletion
    success_ = delete_callback_(node_ids_, &error_message_);
    
    if (success_) {
        // Also remove from diagram (but don't backup since this is permanent)
        auto& nodes = model_->nodes();
        auto& edges = model_->edges();
        
        // Remove edges connected to deleted nodes
        edges.erase(std::remove_if(edges.begin(), edges.end(),
                                   [this](const DiagramEdge& e) {
                                       bool source_deleted = std::find(node_ids_.begin(), node_ids_.end(), e.source_id) != node_ids_.end();
                                       bool target_deleted = std::find(node_ids_.begin(), node_ids_.end(), e.target_id) != node_ids_.end();
                                       return source_deleted || target_deleted;
                                   }), edges.end());
        
        // Remove nodes
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                                   [this](const DiagramNode& n) {
                                       return std::find(node_ids_.begin(), node_ids_.end(), n.id) != node_ids_.end();
                                   }), nodes.end());
    }
}

void ProjectDeleteCommand::Undo() {
    // Project deletion cannot be undone - this is intentional
    // The command reports CanUndo() = false to indicate this
}

// ============================================================================
// ReparentNodeCommand Implementation (Drag & Drop Containment)
// ============================================================================

ReparentNodeCommand::ReparentNodeCommand(DiagramModel* model, const std::string& node_id,
                                          const std::string& old_parent_id, const std::string& new_parent_id,
                                          double old_x, double old_y, double new_x, double new_y)
    : model_(model), node_id_(node_id),
      old_parent_id_(old_parent_id), new_parent_id_(new_parent_id),
      old_x_(old_x), old_y_(old_y), new_x_(new_x), new_y_(new_y) {}

void ReparentNodeCommand::Execute() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->parent_id = new_parent_id_;
        it->x = new_x_;
        it->y = new_y_;
    }
}

void ReparentNodeCommand::Undo() {
    auto& nodes = model_->nodes();
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [this](const DiagramNode& n) { return n.id == node_id_; });
    if (it != nodes.end()) {
        it->parent_id = old_parent_id_;
        it->x = old_x_;
        it->y = old_y_;
    }
}

} // namespace scratchrobin
