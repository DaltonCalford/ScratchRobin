/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_COMMAND_H
#define SCRATCHROBIN_DIAGRAM_COMMAND_H

#include "ui/diagram_model.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {

// Forward declarations
class DiagramModel;
struct DiagramNode;
struct DiagramEdge;

// Abstract base class for diagram commands (Command pattern)
class DiagramCommand {
public:
    virtual ~DiagramCommand() = default;
    
    // Execute the command
    virtual void Execute() = 0;
    
    // Undo the command
    virtual void Undo() = 0;
    
    // Get command description for UI
    virtual std::string GetDescription() const = 0;
    
    // Check if command can be undone
    virtual bool CanUndo() const { return true; }
};

// Command manager for undo/redo history
class CommandManager {
public:
    explicit CommandManager(size_t max_history = 100);
    
    // Execute a new command (clears redo stack)
    void Execute(std::unique_ptr<DiagramCommand> command);
    
    // Undo last command
    bool Undo();
    
    // Redo last undone command
    bool Redo();
    
    // Check if can undo/redo
    bool CanUndo() const;
    bool CanRedo() const;
    
    // Get undo/redo descriptions for UI
    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;
    
    // Clear all history
    void Clear();
    
    // Mark current state as saved
    void MarkSaved();
    
    // Check if there are unsaved changes
    bool IsModified() const;

private:
    std::vector<std::unique_ptr<DiagramCommand>> undo_stack_;
    std::vector<std::unique_ptr<DiagramCommand>> redo_stack_;
    size_t max_history_;
    size_t saved_index_ = 0;
};

// Concrete command implementations

// Add node command
class AddNodeCommand : public DiagramCommand {
public:
    AddNodeCommand(DiagramModel* model, const DiagramNode& node);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override { return "Add " + node_.name; }

private:
    DiagramModel* model_;
    DiagramNode node_;
    bool executed_ = false;
};

// Delete node command (with its connected edges)
class DeleteNodeCommand : public DiagramCommand {
public:
    DeleteNodeCommand(DiagramModel* model, const std::string& node_id);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override { return "Delete node"; }

private:
    DiagramModel* model_;
    std::string node_id_;
    DiagramNode node_backup_;
    std::vector<DiagramEdge> edges_backup_;
    bool executed_ = false;
};

// Move node command
class MoveNodeCommand : public DiagramCommand {
public:
    MoveNodeCommand(DiagramModel* model, const std::string& node_id,
                    double old_x, double old_y, double new_x, double new_y);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override { return "Move node"; }

private:
    DiagramModel* model_;
    std::string node_id_;
    double old_x_, old_y_;
    double new_x_, new_y_;
};

// Resize node command
class ResizeNodeCommand : public DiagramCommand {
public:
    ResizeNodeCommand(DiagramModel* model, const std::string& node_id,
                      double old_w, double old_h, double new_w, double new_h);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override { return "Resize node"; }

private:
    DiagramModel* model_;
    std::string node_id_;
    double old_w_, old_h_;
    double new_w_, new_h_;
};

// Add edge command
class AddEdgeCommand : public DiagramCommand {
public:
    AddEdgeCommand(DiagramModel* model, const DiagramEdge& edge);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override { return "Add relationship"; }

private:
    DiagramModel* model_;
    DiagramEdge edge_;
    bool executed_ = false;
};

// Delete edge command
class DeleteEdgeCommand : public DiagramCommand {
public:
    DeleteEdgeCommand(DiagramModel* model, const std::string& edge_id);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override { return "Delete relationship"; }

private:
    DiagramModel* model_;
    std::string edge_id_;
    DiagramEdge edge_backup_;
    bool executed_ = false;
};

// Edit node name command
class EditNodeNameCommand : public DiagramCommand {
public:
    EditNodeNameCommand(DiagramModel* model, const std::string& node_id,
                        const std::string& old_name, const std::string& new_name);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override { return "Rename node"; }

private:
    DiagramModel* model_;
    std::string node_id_;
    std::string old_name_;
    std::string new_name_;
};

// Compound command (for grouping multiple commands)
class CompoundCommand : public DiagramCommand {
public:
    void AddCommand(std::unique_ptr<DiagramCommand> command);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    std::vector<std::unique_ptr<DiagramCommand>> commands_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_COMMAND_H
