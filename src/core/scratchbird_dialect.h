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
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace scratchrobin {
namespace core {

// ============================================================================
// Feature Levels
// ============================================================================

enum class FeatureLevel {
  kStandard,    // Standard SQL compliance
  kExtended,    // Extended features
  kVendor       // Vendor-specific extensions
};

// ============================================================================
// DialectFeature
// ============================================================================

struct DialectFeature {
  std::string name;
  std::string description;
  FeatureLevel level = FeatureLevel::kStandard;
  bool is_supported = false;
  
  DialectFeature();
  DialectFeature(const std::string& name_val, const std::string& description_val,
                 FeatureLevel level_val);
};

// ============================================================================
// DialectCapabilities
// ============================================================================

class DialectCapabilities {
 public:
  DialectCapabilities();
  
  bool IsSupported(const std::string& feature_name) const;
  void SetSupported(const std::string& feature_name, bool supported);
  std::optional<DialectFeature> GetFeature(const std::string& feature_name) const;
  
  std::vector<std::string> GetSupportedFeatures() const;
  std::vector<std::string> GetUnsupportedFeatures() const;
  
 private:
  void InitializeDefaults();
  
  std::unordered_map<std::string, DialectFeature> features_;
};

// ============================================================================
// ReservedWords
// ============================================================================

class ReservedWords {
 public:
  ReservedWords();
  
  bool IsReserved(const std::string& word) const;
  bool IsReservedStrict(const std::string& word) const;
  std::string QuoteIdentifier(const std::string& identifier) const;
  bool NeedsQuoting(const std::string& identifier) const;
  
  std::vector<std::string> GetAllReservedWords() const;
  void AddReservedWord(const std::string& word);
  
 private:
  void InitializeReservedWords();
  
  std::unordered_set<std::string> reserved_words_;
};

// ============================================================================
// DialectSyntax
// ============================================================================

class DialectSyntax {
 public:
  DialectSyntax();
  
  // String quoting
  std::string QuoteString(const std::string& value) const;
  std::string QuoteIdentifier(const std::string& identifier) const;
  
  // Parameter formatting
  std::string FormatPositionalParam(int index) const;
  std::string FormatNamedParam(const std::string& name) const;
  
  // Comments
  std::string MakeLineComment(const std::string& text) const;
  std::string MakeBlockComment(const std::string& text) const;
  
  // Accessors
  char GetIdentifierQuote() const { return identifier_quote_; }
  const std::string& GetLineComment() const { return line_comment_; }
  
 private:
  void InitializeDefaults();
  
  std::string string_quote_;
  std::string escape_string_;
  char identifier_quote_;
  std::string line_comment_;
  std::string block_comment_start_;
  std::string block_comment_end_;
  std::string statement_terminator_;
  std::string positional_param_;
  std::string named_param_prefix_;
  std::string catalog_separator_;
  
  bool supports_unicode_identifiers_;
  bool case_sensitive_identifiers_;
  bool case_sensitive_strings_;
  bool supports_hex_escapes_;
  bool supports_unicode_escapes_;
  bool supports_octal_escapes_;
};

// ============================================================================
// TypeMapping
// ============================================================================

enum class TypeMappingKind {
  kDirect,      // Direct 1:1 mapping
  kCast,        // Requires cast
  kFunction,    // Requires function conversion
  kUnsupported  // Not supported
};

struct TypeMapping {
  std::string target_type;
  TypeMappingKind kind = TypeMappingKind::kDirect;
};

// ============================================================================
// DialectTypeMapping
// ============================================================================

class DialectTypeMapping {
 public:
  DialectTypeMapping();
  
  void AddMapping(const std::string& source_type, const std::string& target_type,
                  TypeMappingKind kind);
  std::optional<TypeMapping> GetMapping(const std::string& source_type) const;
  std::string MapType(const std::string& source_type) const;
  bool HasMapping(const std::string& source_type) const;
  
  std::vector<std::pair<std::string, TypeMapping>> GetAllMappings() const;
  
 private:
  void InitializeBuiltinMappings();
  
  std::unordered_map<std::string, TypeMapping> mappings_;
};

// ============================================================================
// Dialect
// ============================================================================

class Dialect {
 public:
  Dialect();
  ~Dialect();
  
  // Factory method for ScratchBird dialect
  static Dialect CreateScratchBirdDialect();
  
  // Accessors
  const std::string& GetName() const;
  const std::string& GetVersion() const;
  const std::string& GetDescription() const;
  
  DialectCapabilities& GetCapabilities();
  const DialectCapabilities& GetCapabilities() const;
  
  ReservedWords& GetReservedWords();
  const ReservedWords& GetReservedWords() const;
  
  DialectSyntax& GetSyntax();
  const DialectSyntax& GetSyntax() const;
  
  DialectTypeMapping& GetTypeMapping();
  const DialectTypeMapping& GetTypeMapping() const;
  
  // Convenience methods
  bool SupportsFeature(const std::string& feature_name) const;
  std::string QuoteIdentifier(const std::string& identifier) const;
  std::string QuoteString(const std::string& value) const;
  std::string MapType(const std::string& source_type) const;
  
 private:
  std::string name_;
  std::string version_;
  std::string description_;
  
  DialectCapabilities capabilities_;
  ReservedWords reserved_words_;
  DialectSyntax syntax_;
  DialectTypeMapping type_mapping_;
};

// ============================================================================
// DialectRegistry
// ============================================================================

class DialectRegistry {
 public:
  static DialectRegistry& Instance();
  
  void RegisterDialect(const std::string& name, const Dialect& dialect);
  std::optional<Dialect> GetDialect(const std::string& name) const;
  bool HasDialect(const std::string& name) const;
  std::vector<std::string> GetDialectNames() const;
  Dialect GetDefaultDialect() const;
  
 private:
  DialectRegistry();
  
  std::unordered_map<std::string, Dialect> dialects_;
};

}  // namespace core
}  // namespace scratchrobin
