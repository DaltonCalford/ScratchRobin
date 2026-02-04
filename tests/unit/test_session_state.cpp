/**
 * @file test_session_state.cpp
 * @brief Unit tests for session state persistence
 */

#include <gtest/gtest.h>
#include "core/session_state.h"
#include <filesystem>
#include <fstream>

using namespace scratchrobin;

class SessionStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        state_ = std::make_unique<SessionState>();
        temp_file_ = std::filesystem::temp_directory_path() / "test_session.json";
    }
    
    void TearDown() override {
        if (std::filesystem::exists(temp_file_)) {
            std::filesystem::remove(temp_file_);
        }
    }
    
    std::unique_ptr<SessionState> state_;
    std::filesystem::path temp_file_;
};

TEST_F(SessionStateTest, InitialStateIsEmpty) {
    EXPECT_TRUE(state_->GetOpenEditors().empty());
    EXPECT_TRUE(state_->GetRecentConnections().empty());
}

TEST_F(SessionStateTest, AddOpenEditor) {
    EditorState editor;
    editor.id = "editor1";
    editor.title = "Query 1";
    editor.content = "SELECT * FROM users";
    editor.cursor_position = 20;
    
    state_->AddOpenEditor(editor);
    
    auto editors = state_->GetOpenEditors();
    ASSERT_EQ(editors.size(), 1);
    EXPECT_EQ(editors[0].title, "Query 1");
    EXPECT_EQ(editors[0].content, "SELECT * FROM users");
}

TEST_F(SessionStateTest, RemoveOpenEditor) {
    EditorState editor;
    editor.id = "editor1";
    state_->AddOpenEditor(editor);
    
    state_->RemoveOpenEditor("editor1");
    
    EXPECT_TRUE(state_->GetOpenEditors().empty());
}

TEST_F(SessionStateTest, UpdateEditorContent) {
    EditorState editor;
    editor.id = "editor1";
    editor.content = "SELECT 1";
    state_->AddOpenEditor(editor);
    
    state_->UpdateEditorContent("editor1", "SELECT * FROM orders");
    
    auto editors = state_->GetOpenEditors();
    EXPECT_EQ(editors[0].content, "SELECT * FROM orders");
}

TEST_F(SessionStateTest, AddRecentConnection) {
    ConnectionState conn;
    conn.profile_name = "Production DB";
    conn.connection_string = "host=prod db=mydb";
    conn.last_connected = std::chrono::system_clock::now();
    
    state_->AddRecentConnection(conn);
    
    auto recent = state_->GetRecentConnections();
    ASSERT_EQ(recent.size(), 1);
    EXPECT_EQ(recent[0].profile_name, "Production DB");
}

TEST_F(SessionStateTest, RecentConnectionsLimit) {
    // Add more than the limit
    for (int i = 0; i < 15; ++i) {
        ConnectionState conn;
        conn.profile_name = "Conn " + std::to_string(i);
        state_->AddRecentConnection(conn);
    }
    
    auto recent = state_->GetRecentConnections();
    EXPECT_LE(recent.size(), 10);  // Limit is 10
}

TEST_F(SessionStateTest, RecentConnectionsOrderedByTime) {
    ConnectionState conn1;
    conn1.profile_name = "First";
    conn1.last_connected = std::chrono::system_clock::now();
    state_->AddRecentConnection(conn1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    ConnectionState conn2;
    conn2.profile_name = "Second";
    conn2.last_connected = std::chrono::system_clock::now();
    state_->AddRecentConnection(conn2);
    
    auto recent = state_->GetRecentConnections();
    ASSERT_EQ(recent.size(), 2);
    EXPECT_EQ(recent[0].profile_name, "Second");  // Most recent first
}

TEST_F(SessionStateTest, SaveAndLoad) {
    // Set up state
    EditorState editor;
    editor.id = "editor1";
    editor.title = "Test Query";
    editor.content = "SELECT 1";
    state_->AddOpenEditor(editor);
    
    ConnectionState conn;
    conn.profile_name = "Test DB";
    state_->AddRecentConnection(conn);
    
    state_->SetLastWindowGeometry(100, 200, 800, 600);
    
    // Save
    EXPECT_TRUE(state_->SaveToFile(temp_file_));
    
    // Load into new state
    SessionState new_state;
    EXPECT_TRUE(new_state.LoadFromFile(temp_file_));
    
    auto editors = new_state.GetOpenEditors();
    ASSERT_EQ(editors.size(), 1);
    EXPECT_EQ(editors[0].title, "Test Query");
    
    auto recent = new_state.GetRecentConnections();
    ASSERT_EQ(recent.size(), 1);
    EXPECT_EQ(recent[0].profile_name, "Test DB");
    
    int x, y, w, h;
    new_state.GetLastWindowGeometry(x, y, w, h);
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 200);
    EXPECT_EQ(w, 800);
    EXPECT_EQ(h, 600);
}

TEST_F(SessionStateTest, LoadNonExistentFile) {
    std::filesystem::path nonexistent = "/nonexistent/path/session.json";
    EXPECT_FALSE(state_->LoadFromFile(nonexistent));
}

TEST_F(SessionStateTest, LoadInvalidJson) {
    // Write invalid JSON
    {
        std::ofstream file(temp_file_);
        file << "{invalid json";
    }
    
    EXPECT_FALSE(state_->LoadFromFile(temp_file_));
}

TEST_F(SessionStateTest, ClearSession) {
    EditorState editor;
    editor.id = "editor1";
    state_->AddOpenEditor(editor);
    
    ConnectionState conn;
    conn.profile_name = "Test";
    state_->AddRecentConnection(conn);
    
    state_->Clear();
    
    EXPECT_TRUE(state_->GetOpenEditors().empty());
    EXPECT_TRUE(state_->GetRecentConnections().empty());
}

TEST_F(SessionStateTest, WindowState) {
    WindowState window;
    window.id = "main";
    window.x = 100;
    window.y = 200;
    window.width = 1024;
    window.height = 768;
    window.maximized = false;
    
    state_->SetWindowState(window);
    
    auto retrieved = state_->GetWindowState("main");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->x, 100);
    EXPECT_EQ(retrieved->width, 1024);
}

TEST_F(SessionStateTest, TreeExpansionState) {
    state_->SetTreeNodeExpanded("public.users", true);
    state_->SetTreeNodeExpanded("public.orders", true);
    state_->SetTreeNodeExpanded("public.orders.indexes", false);
    
    EXPECT_TRUE(state_->IsTreeNodeExpanded("public.users"));
    EXPECT_TRUE(state_->IsTreeNodeExpanded("public.orders"));
    EXPECT_FALSE(state_->IsTreeNodeExpanded("public.orders.indexes"));
    EXPECT_FALSE(state_->IsTreeNodeExpanded("unknown.node"));  // Default
}

TEST_F(SessionStateTest, SelectedInspectorTab) {
    state_->SetSelectedInspectorTab("DDL");
    EXPECT_EQ(state_->GetSelectedInspectorTab(), "DDL");
}

TEST_F(SessionStateTest, EditorPositionAndSelection) {
    EditorState editor;
    editor.id = "editor1";
    editor.cursor_position = 100;
    editor.selection_start = 50;
    editor.selection_end = 100;
    state_->AddOpenEditor(editor);
    
    auto editors = state_->GetOpenEditors();
    ASSERT_EQ(editors.size(), 1);
    EXPECT_EQ(editors[0].cursor_position, 100);
    EXPECT_EQ(editors[0].selection_start, 50);
    EXPECT_EQ(editors[0].selection_end, 100);
}

TEST_F(SessionStateTest, StatementHistory) {
    state_->AddToStatementHistory("SELECT * FROM users");
    state_->AddToStatementHistory("SELECT * FROM orders");
    state_->AddToStatementHistory("SELECT * FROM users");  // Duplicate
    
    auto history = state_->GetStatementHistory();
    // Should deduplicate and limit
    EXPECT_LE(history.size(), 100);
    EXPECT_EQ(history[0], "SELECT * FROM users");  // Most recent
}

TEST_F(SessionStateTest, SaveWithUnsavedChanges) {
    EditorState editor;
    editor.id = "editor1";
    editor.content = "SELECT 1";
    editor.has_unsaved_changes = true;
    state_->AddOpenEditor(editor);
    
    EXPECT_TRUE(state_->SaveToFile(temp_file_));
    
    SessionState new_state;
    EXPECT_TRUE(new_state.LoadFromFile(temp_file_));
    
    auto editors = new_state.GetOpenEditors();
    ASSERT_EQ(editors.size(), 1);
    EXPECT_TRUE(editors[0].has_unsaved_changes);
}

TEST_F(SessionStateTest, LastActiveConnection) {
    state_->SetLastActiveConnection("Production DB");
    EXPECT_EQ(state_->GetLastActiveConnection(), "Production DB");
}

TEST_F(SessionStateTest, GridColumnWidths) {
    state_->SetGridColumnWidth("users", "id", 50);
    state_->SetGridColumnWidth("users", "name", 150);
    
    EXPECT_EQ(state_->GetGridColumnWidth("users", "id"), 50);
    EXPECT_EQ(state_->GetGridColumnWidth("users", "name"), 150);
    EXPECT_EQ(state_->GetGridColumnWidth("users", "unknown"), -1);  // Default
}
