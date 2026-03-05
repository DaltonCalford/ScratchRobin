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

#include <optional>
#include <string>
#include <vector>

namespace scratchrobin::core {

// ============================================================================
// Snippet Category
// ============================================================================

struct SnippetCategory {
  std::string id;
  std::string name;
  std::string description;
  std::string parent_id;  // For nested categories
  int sort_order{0};
};

// ============================================================================
// Code Snippet
// ============================================================================

struct CodeSnippet {
  int64_t id{0};
  std::string name;
  std::string description;
  std::string code;
  std::string language;  // sql, pgsql, python, etc.
  std::string category_id;
  std::vector<std::string> tags;
  bool is_favorite{false};
  int usage_count{0};
  std::string created_at;
  std::string modified_at;
  std::string author;
};

// ============================================================================
// Snippet Variable
// ============================================================================

struct SnippetVariable {
  std::string name;
  std::string description;
  std::string default_value;
  bool required{false};
};

// ============================================================================
// Snippet Filter
// ============================================================================

struct SnippetFilter {
  std::optional<std::string> category_id;
  std::optional<std::string> language;
  std::optional<std::string> search_text;
  std::optional<std::string> tag;
  bool favorites_only{false};
};

// ============================================================================
// Code Snippet Manager
// ============================================================================

class CodeSnippetManager {
 public:
  static CodeSnippetManager& Instance();

  // Snippet CRUD
  int64_t AddSnippet(const CodeSnippet& snippet);
  bool UpdateSnippet(const CodeSnippet& snippet);
  bool DeleteSnippet(int64_t id);
  std::optional<CodeSnippet> GetSnippet(int64_t id);
  std::optional<CodeSnippet> GetSnippetByName(const std::string& name);

  // Snippet retrieval
  std::vector<CodeSnippet> GetAllSnippets();
  std::vector<CodeSnippet> GetSnippets(const SnippetFilter& filter);
  std::vector<CodeSnippet> GetRecentSnippets(int limit = 10);
  std::vector<CodeSnippet> GetFavorites();

  // Search
  std::vector<CodeSnippet> Search(const std::string& query);
  std::vector<CodeSnippet> SearchByTag(const std::string& tag);

  // Categories
  std::vector<SnippetCategory> GetCategories();
  std::vector<SnippetCategory> GetCategories(const std::string& parent_id);
  std::optional<SnippetCategory> GetCategory(const std::string& category_id);
  bool AddCategory(const SnippetCategory& category);
  bool UpdateCategory(const SnippetCategory& category);
  bool DeleteCategory(const std::string& category_id);

  // Favorites
  void SetFavorite(int64_t id, bool is_favorite);
  bool IsFavorite(int64_t id);

  // Tags
  std::vector<std::string> GetAllTags();
  void AddTag(int64_t snippet_id, const std::string& tag);
  void RemoveTag(int64_t snippet_id, const std::string& tag);

  // Variables - parse and replace placeholders in snippets
  std::vector<SnippetVariable> ExtractVariables(const std::string& code);
  std::string ReplaceVariables(
      const std::string& code,
      const std::vector<std::pair<std::string, std::string>>& values);

  // Insert snippet with variables
  std::string PrepareSnippet(
      int64_t snippet_id,
      const std::vector<std::pair<std::string, std::string>>& variable_values);

  // Usage tracking
  void IncrementUsage(int64_t snippet_id);
  std::vector<CodeSnippet> GetMostUsedSnippets(int limit = 10);

  // Import/Export
  bool ExportSnippets(const std::string& filepath,
                      const std::vector<int64_t>& snippet_ids);
  bool ImportSnippets(const std::string& filepath);

  // Persistence
  bool LoadFromFile(const std::string& filepath);
  bool SaveToFile(const std::string& filepath);

  // Built-in snippets
  void InitializeBuiltinSnippets();
  void ResetToDefaults();

 private:
  CodeSnippetManager();
  ~CodeSnippetManager() = default;

  CodeSnippetManager(const CodeSnippetManager&) = delete;
  CodeSnippetManager& operator=(const CodeSnippetManager&) = delete;

  std::vector<CodeSnippet> snippets_;
  std::vector<SnippetCategory> categories_;
  int64_t next_id_{1};
  std::string current_filepath_;

  void AddBuiltinSnippet(const std::string& name,
                         const std::string& code,
                         const std::string& category,
                         const std::string& description = "");
  std::string GenerateId();
  bool MatchesFilter(const CodeSnippet& snippet, const SnippetFilter& filter);
};

}  // namespace scratchrobin::core
