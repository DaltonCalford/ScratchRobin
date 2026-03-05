/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/query_history.h"

#include <algorithm>
#include <fstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

QueryHistory& QueryHistory::Instance() {
  static QueryHistory instance;
  return instance;
}

// ============================================================================
// Add Entry
// ============================================================================

int64_t QueryHistory::AddEntry(const QueryHistoryEntry& entry) {
  QueryHistoryEntry new_entry = entry;
  new_entry.id = next_id_++;
  if (new_entry.executed_at == std::chrono::system_clock::time_point{}) {
    new_entry.executed_at = std::chrono::system_clock::now();
  }
  entries_.push_back(new_entry);
  TrimHistoryIfNeeded();
  return new_entry.id;
}

// ============================================================================
// Retrieve Entries
// ============================================================================

std::vector<QueryHistoryEntry> QueryHistory::GetAllEntries(int limit) {
  std::vector<QueryHistoryEntry> result;
  int count = 0;
  for (auto it = entries_.rbegin(); it != entries_.rend() && count < limit; ++it, ++count) {
    result.push_back(*it);
  }
  return result;
}

std::vector<QueryHistoryEntry> QueryHistory::GetEntries(
    const QueryHistoryFilter& filter, int limit) {
  std::vector<QueryHistoryEntry> result;
  int count = 0;
  for (auto it = entries_.rbegin(); it != entries_.rend() && count < limit; ++it) {
    if (MatchesFilter(*it, filter)) {
      result.push_back(*it);
      ++count;
    }
  }
  return result;
}

std::optional<QueryHistoryEntry> QueryHistory::GetEntry(int64_t id) {
  for (const auto& entry : entries_) {
    if (entry.id == id) {
      return entry;
    }
  }
  return std::nullopt;
}

// ============================================================================
// Recent Queries
// ============================================================================

std::vector<QueryHistoryEntry> QueryHistory::GetRecentQueries(int limit) {
  return GetAllEntries(limit);
}

std::vector<QueryHistoryEntry> QueryHistory::GetRecentQueriesForDatabase(
    const std::string& database_name, int limit) {
  QueryHistoryFilter filter;
  filter.database_name = database_name;
  return GetEntries(filter, limit);
}

// ============================================================================
// Search
// ============================================================================

std::vector<QueryHistoryEntry> QueryHistory::Search(
    const std::string& search_text, int limit) {
  QueryHistoryFilter filter;
  filter.search_text = search_text;
  return GetEntries(filter, limit);
}

// ============================================================================
// Favorites
// ============================================================================

void QueryHistory::SetFavorite(int64_t id, bool is_favorite) {
  for (auto& entry : entries_) {
    if (entry.id == id) {
      entry.is_favorite = is_favorite;
      break;
    }
  }
}

std::vector<QueryHistoryEntry> QueryHistory::GetFavorites(int limit) {
  QueryHistoryFilter filter;
  filter.favorites_only = true;
  return GetEntries(filter, limit);
}

int QueryHistory::GetFavoriteCount() const {
  return std::count_if(entries_.begin(), entries_.end(),
                       [](const QueryHistoryEntry& e) { return e.is_favorite; });
}

// ============================================================================
// Tags
// ============================================================================

void QueryHistory::AddTag(int64_t id, const std::string& tag) {
  // Implementation would parse tags string and add new tag
}

void QueryHistory::RemoveTag(int64_t id, const std::string& tag) {
  // Implementation would parse tags string and remove tag
}

std::vector<std::string> QueryHistory::GetAllTags() {
  std::vector<std::string> all_tags;
  // Implementation would collect unique tags from all entries
  return all_tags;
}

std::vector<QueryHistoryEntry> QueryHistory::GetEntriesByTag(
    const std::string& tag, int limit) {
  std::vector<QueryHistoryEntry> result;
  // Implementation would filter entries by tag
  return result;
}

// ============================================================================
// Delete
// ============================================================================

bool QueryHistory::DeleteEntry(int64_t id) {
  auto it = std::remove_if(entries_.begin(), entries_.end(),
                           [id](const QueryHistoryEntry& e) { return e.id == id; });
  if (it != entries_.end()) {
    entries_.erase(it, entries_.end());
    return true;
  }
  return false;
}

int QueryHistory::DeleteEntriesOlderThan(
    const std::chrono::system_clock::time_point& cutoff) {
  auto it = std::remove_if(entries_.begin(), entries_.end(),
                           [&cutoff](const QueryHistoryEntry& e) {
                             return e.executed_at < cutoff;
                           });
  int count = static_cast<int>(std::distance(it, entries_.end()));
  entries_.erase(it, entries_.end());
  return count;
}

int QueryHistory::DeleteAllEntries() {
  int count = static_cast<int>(entries_.size());
  entries_.clear();
  return count;
}

// ============================================================================
// Statistics
// ============================================================================

int QueryHistory::GetTotalEntryCount() const {
  return static_cast<int>(entries_.size());
}

int QueryHistory::GetEntryCountForDatabase(const std::string& database_name) const {
  return std::count_if(entries_.begin(), entries_.end(),
                       [&database_name](const QueryHistoryEntry& e) {
                         return e.database == database_name;
                       });
}

std::chrono::milliseconds QueryHistory::GetAverageExecutionTime(
    const std::string& database_name) {
  int64_t total_ms = 0;
  int count = 0;
  for (const auto& entry : entries_) {
    if (database_name.empty() || entry.database == database_name) {
      total_ms += entry.execution_time.count();
      ++count;
    }
  }
  if (count == 0) {
    return std::chrono::milliseconds(0);
  }
  return std::chrono::milliseconds(total_ms / count);
}

// ============================================================================
// Persistence
// ============================================================================

bool QueryHistory::LoadFromFile(const std::string& filepath) {
  // Implementation would load from JSON or similar format
  current_filepath_ = filepath;
  return true;
}

bool QueryHistory::SaveToFile(const std::string& filepath) {
  // Implementation would save to JSON or similar format
  current_filepath_ = filepath;
  return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

void QueryHistory::TrimHistoryIfNeeded() {
  while (static_cast<int>(entries_.size()) > max_history_size_) {
    entries_.erase(entries_.begin());
  }
}

bool QueryHistory::MatchesFilter(const QueryHistoryEntry& entry,
                                 const QueryHistoryFilter& filter) {
  if (filter.database_name.has_value() &&
      entry.database != filter.database_name.value()) {
    return false;
  }
  if (filter.connection_name.has_value() &&
      entry.connection_name != filter.connection_name.value()) {
    return false;
  }
  if (filter.query_type.has_value() &&
      entry.query_type != filter.query_type.value()) {
    return false;
  }
  if (filter.search_text.has_value() &&
      entry.sql.find(filter.search_text.value()) == std::string::npos) {
    return false;
  }
  if (filter.favorites_only && !entry.is_favorite) {
    return false;
  }
  if (filter.successful_only && !entry.successful) {
    return false;
  }
  return true;
}

}  // namespace scratchrobin::core
