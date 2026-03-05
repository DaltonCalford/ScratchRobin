/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin::core {

// ============================================================================
// Query History Entry
// ============================================================================

struct QueryHistoryEntry {
  int64_t id{0};
  std::string sql;  // Query text (matches test API)
  std::string database;  // Database name (matches test API)
  std::string connection_name;
  std::chrono::system_clock::time_point executed_at;
  std::chrono::milliseconds execution_time{0};
  int64_t rows_affected{-1};
  bool successful{true};  // Success flag (matches test API)
  std::string error_message;
  std::string query_type;  // SELECT, INSERT, UPDATE, DELETE, etc.
  bool is_favorite{false};
  std::string tags;
};

// ============================================================================
// Query Filter
// ============================================================================

struct QueryHistoryFilter {
  std::optional<std::string> database_name;
  std::optional<std::string> connection_name;
  std::optional<std::string> query_type;
  std::optional<std::string> search_text;
  std::optional<std::chrono::system_clock::time_point> date_from;
  std::optional<std::chrono::system_clock::time_point> date_to;
  bool favorites_only{false};
  bool successful_only{false};
};

// ============================================================================
// Query History Manager
// ============================================================================

class QueryHistory {
 public:
  static QueryHistory& Instance();
  static QueryHistory& GetInstance() { return Instance(); }  // Alias for compatibility

  // Add entry
  int64_t AddEntry(const QueryHistoryEntry& entry);
  int AddEntry(QueryHistoryEntry& entry) { entry.id = AddEntry(static_cast<const QueryHistoryEntry&>(entry)); return static_cast<int>(entry.id); }  // Compatible version

  // Retrieve entries
  std::vector<QueryHistoryEntry> GetAllEntries(int limit = 1000);
  std::vector<QueryHistoryEntry> GetEntries(const QueryHistoryFilter& filter, int limit = 1000);
  std::optional<QueryHistoryEntry> GetEntry(int64_t id);

  // Recent queries
  std::vector<QueryHistoryEntry> GetRecentQueries(int limit = 50);
  std::vector<QueryHistoryEntry> GetRecentQueriesForDatabase(
      const std::string& database_name, int limit = 50);

  // Search
  std::vector<QueryHistoryEntry> Search(const std::string& search_text, int limit = 100);
  std::vector<QueryHistoryEntry> SearchEntries(const std::string& search_text, int limit = 100) { return Search(search_text, limit); }  // Alias for compatibility

  // Favorites
  void SetFavorite(int64_t id, bool is_favorite);
  std::vector<QueryHistoryEntry> GetFavorites(int limit = 100);
  std::vector<QueryHistoryEntry> GetFavoriteEntries(int limit = 100) { return GetFavorites(limit); }  // Alias for compatibility
  int GetFavoriteCount() const;  // Get count of favorites

  // Tags
  void AddTag(int64_t id, const std::string& tag);
  void RemoveTag(int64_t id, const std::string& tag);
  std::vector<std::string> GetAllTags();
  std::vector<QueryHistoryEntry> GetEntriesByTag(const std::string& tag, int limit = 100);

  // Delete
  bool DeleteEntry(int64_t id);
  int DeleteEntriesOlderThan(const std::chrono::system_clock::time_point& cutoff);
  int DeleteAllEntries();
  void ClearHistory() { DeleteAllEntries(); }  // Alias for compatibility

  // Statistics
  int GetTotalEntryCount() const;
  int GetTotalCount() const { return GetTotalEntryCount(); }  // Alias for compatibility
  int GetEntryCountForDatabase(const std::string& database) const;
  std::chrono::milliseconds GetAverageExecutionTime(const std::string& database_name = "");

  // Persistence
  bool LoadFromFile(const std::string& filepath);
  bool SaveToFile(const std::string& filepath);
  void SetStoragePath(const std::string& path) { current_filepath_ = path; }  // Alias for compatibility

  // Settings
  void SetMaxHistorySize(int size) { max_history_size_ = size; }
  void SetMaxEntries(int size) { SetMaxHistorySize(size); }  // Alias for compatibility
  int GetMaxHistorySize() const { return max_history_size_; }

  void SetAutoSave(bool enabled) { auto_save_ = enabled; }
  bool GetAutoSave() const { return auto_save_; }

 private:
  QueryHistory() = default;
  ~QueryHistory() = default;

  QueryHistory(const QueryHistory&) = delete;
  QueryHistory& operator=(const QueryHistory&) = delete;

  std::vector<QueryHistoryEntry> entries_;
  int64_t next_id_{1};
  int max_history_size_{10000};
  bool auto_save_{true};
  std::string current_filepath_;

  void TrimHistoryIfNeeded();
  bool MatchesFilter(const QueryHistoryEntry& entry, const QueryHistoryFilter& filter);
};

}  // namespace scratchrobin::core
