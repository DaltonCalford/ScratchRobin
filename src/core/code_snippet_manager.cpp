/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/code_snippet_manager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

CodeSnippetManager& CodeSnippetManager::Instance() {
  static CodeSnippetManager instance;
  return instance;
}

CodeSnippetManager::CodeSnippetManager() {
  InitializeBuiltinSnippets();
}

// ============================================================================
// Snippet CRUD
// ============================================================================

int64_t CodeSnippetManager::AddSnippet(const CodeSnippet& snippet) {
  CodeSnippet new_snippet = snippet;
  new_snippet.id = next_id_++;
  
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
  new_snippet.created_at = ss.str();
  new_snippet.modified_at = ss.str();
  
  snippets_.push_back(new_snippet);
  return new_snippet.id;
}

bool CodeSnippetManager::UpdateSnippet(const CodeSnippet& snippet) {
  for (auto& existing : snippets_) {
    if (existing.id == snippet.id) {
      existing = snippet;
      
      auto now = std::chrono::system_clock::now();
      auto time_t_now = std::chrono::system_clock::to_time_t(now);
      std::stringstream ss;
      ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
      existing.modified_at = ss.str();
      
      return true;
    }
  }
  return false;
}

bool CodeSnippetManager::DeleteSnippet(int64_t id) {
  auto it = std::remove_if(snippets_.begin(), snippets_.end(),
                           [id](const CodeSnippet& s) { return s.id == id; });
  if (it != snippets_.end()) {
    snippets_.erase(it, snippets_.end());
    return true;
  }
  return false;
}

std::optional<CodeSnippet> CodeSnippetManager::GetSnippet(int64_t id) {
  for (const auto& snippet : snippets_) {
    if (snippet.id == id) {
      return snippet;
    }
  }
  return std::nullopt;
}

std::optional<CodeSnippet> CodeSnippetManager::GetSnippetByName(
    const std::string& name) {
  for (const auto& snippet : snippets_) {
    if (snippet.name == name) {
      return snippet;
    }
  }
  return std::nullopt;
}

// ============================================================================
// Snippet Retrieval
// ============================================================================

std::vector<CodeSnippet> CodeSnippetManager::GetAllSnippets() {
  return snippets_;
}

std::vector<CodeSnippet> CodeSnippetManager::GetSnippets(
    const SnippetFilter& filter) {
  std::vector<CodeSnippet> result;
  std::copy_if(snippets_.begin(), snippets_.end(), std::back_inserter(result),
               [&filter, this](const CodeSnippet& s) {
                 return MatchesFilter(s, filter);
               });
  return result;
}

std::vector<CodeSnippet> CodeSnippetManager::GetRecentSnippets(int limit) {
  std::vector<CodeSnippet> sorted = snippets_;
  std::sort(sorted.begin(), sorted.end(),
            [](const CodeSnippet& a, const CodeSnippet& b) {
              return a.modified_at > b.modified_at;
            });
  if (static_cast<int>(sorted.size()) > limit) {
    sorted.resize(limit);
  }
  return sorted;
}

std::vector<CodeSnippet> CodeSnippetManager::GetFavorites() {
  SnippetFilter filter;
  filter.favorites_only = true;
  return GetSnippets(filter);
}

// ============================================================================
// Search
// ============================================================================

std::vector<CodeSnippet> CodeSnippetManager::Search(const std::string& query) {
  SnippetFilter filter;
  filter.search_text = query;
  return GetSnippets(filter);
}

std::vector<CodeSnippet> CodeSnippetManager::SearchByTag(
    const std::string& tag) {
  SnippetFilter filter;
  filter.tag = tag;
  return GetSnippets(filter);
}

// ============================================================================
// Categories
// ============================================================================

std::vector<SnippetCategory> CodeSnippetManager::GetCategories() {
  return categories_;
}

std::vector<SnippetCategory> CodeSnippetManager::GetCategories(
    const std::string& parent_id) {
  std::vector<SnippetCategory> result;
  std::copy_if(categories_.begin(), categories_.end(), std::back_inserter(result),
               [&parent_id](const SnippetCategory& c) {
                 return c.parent_id == parent_id;
               });
  return result;
}

std::optional<SnippetCategory> CodeSnippetManager::GetCategory(
    const std::string& category_id) {
  for (const auto& cat : categories_) {
    if (cat.id == category_id) {
      return cat;
    }
  }
  return std::nullopt;
}

bool CodeSnippetManager::AddCategory(const SnippetCategory& category) {
  categories_.push_back(category);
  return true;
}

bool CodeSnippetManager::UpdateCategory(const SnippetCategory& category) {
  for (auto& existing : categories_) {
    if (existing.id == category.id) {
      existing = category;
      return true;
    }
  }
  return false;
}

bool CodeSnippetManager::DeleteCategory(const std::string& category_id) {
  auto it = std::remove_if(categories_.begin(), categories_.end(),
                           [&category_id](const SnippetCategory& c) {
                             return c.id == category_id;
                           });
  if (it != categories_.end()) {
    categories_.erase(it, categories_.end());
    return true;
  }
  return false;
}

// ============================================================================
// Favorites
// ============================================================================

void CodeSnippetManager::SetFavorite(int64_t id, bool is_favorite) {
  for (auto& snippet : snippets_) {
    if (snippet.id == id) {
      snippet.is_favorite = is_favorite;
      break;
    }
  }
}

bool CodeSnippetManager::IsFavorite(int64_t id) {
  for (const auto& snippet : snippets_) {
    if (snippet.id == id) {
      return snippet.is_favorite;
    }
  }
  return false;
}

// ============================================================================
// Tags
// ============================================================================

std::vector<std::string> CodeSnippetManager::GetAllTags() {
  std::vector<std::string> all_tags;
  for (const auto& snippet : snippets_) {
    for (const auto& tag : snippet.tags) {
      if (std::find(all_tags.begin(), all_tags.end(), tag) == all_tags.end()) {
        all_tags.push_back(tag);
      }
    }
  }
  std::sort(all_tags.begin(), all_tags.end());
  return all_tags;
}

void CodeSnippetManager::AddTag(int64_t snippet_id, const std::string& tag) {
  for (auto& snippet : snippets_) {
    if (snippet.id == snippet_id) {
      if (std::find(snippet.tags.begin(), snippet.tags.end(), tag) == snippet.tags.end()) {
        snippet.tags.push_back(tag);
      }
      break;
    }
  }
}

void CodeSnippetManager::RemoveTag(int64_t snippet_id, const std::string& tag) {
  for (auto& snippet : snippets_) {
    if (snippet.id == snippet_id) {
      auto it = std::remove(snippet.tags.begin(), snippet.tags.end(), tag);
      snippet.tags.erase(it, snippet.tags.end());
      break;
    }
  }
}

// ============================================================================
// Variables
// ============================================================================

std::vector<SnippetVariable> CodeSnippetManager::ExtractVariables(
    const std::string& code) {
  std::vector<SnippetVariable> variables;
  
  // Simple variable extraction: look for ${variable_name} or {{variable_name}}
  size_t pos = 0;
  while ((pos = code.find("${", pos)) != std::string::npos) {
    size_t end = code.find("}", pos);
    if (end != std::string::npos) {
      SnippetVariable var;
      var.name = code.substr(pos + 2, end - pos - 2);
      variables.push_back(var);
      pos = end + 1;
    } else {
      break;
    }
  }
  
  return variables;
}

std::string CodeSnippetManager::ReplaceVariables(
    const std::string& code,
    const std::vector<std::pair<std::string, std::string>>& values) {
  std::string result = code;
  
  for (const auto& value : values) {
    std::string placeholder = "${" + value.first + "}";
    size_t pos = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos) {
      result.replace(pos, placeholder.length(), value.second);
      pos += value.second.length();
    }
  }
  
  return result;
}

// ============================================================================
// Insert Snippet
// ============================================================================

std::string CodeSnippetManager::PrepareSnippet(
    int64_t snippet_id,
    const std::vector<std::pair<std::string, std::string>>& variable_values) {
  auto snippet = GetSnippet(snippet_id);
  if (!snippet.has_value()) {
    return "";
  }
  
  IncrementUsage(snippet_id);
  return ReplaceVariables(snippet->code, variable_values);
}

// ============================================================================
// Usage Tracking
// ============================================================================

void CodeSnippetManager::IncrementUsage(int64_t snippet_id) {
  for (auto& snippet : snippets_) {
    if (snippet.id == snippet_id) {
      snippet.usage_count++;
      break;
    }
  }
}

std::vector<CodeSnippet> CodeSnippetManager::GetMostUsedSnippets(int limit) {
  std::vector<CodeSnippet> sorted = snippets_;
  std::sort(sorted.begin(), sorted.end(),
            [](const CodeSnippet& a, const CodeSnippet& b) {
              return a.usage_count > b.usage_count;
            });
  if (static_cast<int>(sorted.size()) > limit) {
    sorted.resize(limit);
  }
  return sorted;
}

// ============================================================================
// Import/Export
// ============================================================================

bool CodeSnippetManager::ExportSnippets(
    const std::string& filepath,
    const std::vector<int64_t>& snippet_ids) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  
  // Export as JSON
  file << "[\n";
  bool first = true;
  for (const auto& snippet : snippets_) {
    if (snippet_ids.empty() || 
        std::find(snippet_ids.begin(), snippet_ids.end(), snippet.id) != snippet_ids.end()) {
      if (!first) file << ",\n";
      first = false;
      file << "  {\n";
      file << "    \"name\": \"" << snippet.name << "\",\n";
      file << "    \"code\": \"" << snippet.code << "\"\n";
      file << "  }";
    }
  }
  file << "\n]\n";
  
  return true;
}

bool CodeSnippetManager::ImportSnippets(const std::string& filepath) {
  // Would parse JSON and add snippets
  return true;
}

// ============================================================================
// Persistence
// ============================================================================

bool CodeSnippetManager::LoadFromFile(const std::string& filepath) {
  // Would load from JSON
  current_filepath_ = filepath;
  return true;
}

bool CodeSnippetManager::SaveToFile(const std::string& filepath) {
  // Would save to JSON
  current_filepath_ = filepath;
  return true;
}

// ============================================================================
// Built-in Snippets
// ============================================================================

void CodeSnippetManager::InitializeBuiltinSnippets() {
  // Add default categories
  categories_ = {
    {"ddl", "DDL", "Data Definition Language", "", 1},
    {"dml", "DML", "Data Manipulation Language", "", 2},
    {"queries", "Queries", "Common Queries", "", 3},
    {"procedures", "Procedures", "Stored Procedures", "", 4}
  };
  
  // Add default snippets
  AddBuiltinSnippet(
    "Create Table",
    "CREATE TABLE ${table_name} (\n  ${column_name} ${data_type} ${constraints}\n);",
    "ddl",
    "Create a new table"
  );
  
  AddBuiltinSnippet(
    "Select All",
    "SELECT * FROM ${table_name};",
    "queries",
    "Select all columns from a table"
  );
  
  AddBuiltinSnippet(
    "Insert Into",
    "INSERT INTO ${table_name} (${columns})\nVALUES (${values});",
    "dml",
    "Insert a new row"
  );
  
  AddBuiltinSnippet(
    "Update",
    "UPDATE ${table_name}\nSET ${column} = ${value}\nWHERE ${condition};",
    "dml",
    "Update rows in a table"
  );
  
  AddBuiltinSnippet(
    "Delete",
    "DELETE FROM ${table_name}\nWHERE ${condition};",
    "dml",
    "Delete rows from a table"
  );
}

void CodeSnippetManager::ResetToDefaults() {
  snippets_.clear();
  categories_.clear();
  next_id_ = 1;
  InitializeBuiltinSnippets();
}

// ============================================================================
// Helper Methods
// ============================================================================

void CodeSnippetManager::AddBuiltinSnippet(const std::string& name,
                                           const std::string& code,
                                           const std::string& category,
                                           const std::string& description) {
  CodeSnippet snippet;
  snippet.id = next_id_++;
  snippet.name = name;
  snippet.code = code;
  snippet.category_id = category;
  snippet.description = description;
  snippet.language = "sql";
  snippet.created_at = "builtin";
  snippet.modified_at = "builtin";
  snippets_.push_back(snippet);
}

std::string CodeSnippetManager::GenerateId() {
  return std::to_string(next_id_++);
}

bool CodeSnippetManager::MatchesFilter(const CodeSnippet& snippet,
                                       const SnippetFilter& filter) {
  if (filter.category_id.has_value() &&
      snippet.category_id != filter.category_id.value()) {
    return false;
  }
  if (filter.language.has_value() &&
      snippet.language != filter.language.value()) {
    return false;
  }
  if (filter.search_text.has_value()) {
    const auto& search = filter.search_text.value();
    if (snippet.name.find(search) == std::string::npos &&
        snippet.code.find(search) == std::string::npos &&
        snippet.description.find(search) == std::string::npos) {
      return false;
    }
  }
  if (filter.tag.has_value()) {
    const auto& tag = filter.tag.value();
    if (std::find(snippet.tags.begin(), snippet.tags.end(), tag) == snippet.tags.end()) {
      return false;
    }
  }
  if (filter.favorites_only && !snippet.is_favorite) {
    return false;
  }
  return true;
}

}  // namespace scratchrobin::core
