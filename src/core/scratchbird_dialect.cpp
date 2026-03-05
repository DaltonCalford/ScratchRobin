/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "scratchbird_dialect.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace scratchrobin {
namespace core {

namespace {

// Case-insensitive comparison helper
bool IEquals(const std::string& a, const std::string& b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                    [](char ca, char cb) {
                      return std::tolower(static_cast<unsigned char>(ca)) ==
                             std::tolower(static_cast<unsigned char>(cb));
                    });
}

// Normalize identifier (lowercase)
std::string NormalizeIdentifier(const std::string& name) {
  std::string result = name;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

}  // anonymous namespace

// ============================================================================
// DialectFeature Implementation
// ============================================================================

DialectFeature::DialectFeature() = default;

DialectFeature::DialectFeature(const std::string& name_val, 
                               const std::string& description_val,
                               FeatureLevel level_val)
    : name(name_val),
      description(description_val),
      level(level_val),
      is_supported(false) {}

// ============================================================================
// DialectCapabilities Implementation
// ============================================================================

DialectCapabilities::DialectCapabilities() {
  InitializeDefaults();
}

void DialectCapabilities::InitializeDefaults() {
  // Transaction features
  features_["savepoints"] = DialectFeature("savepoints", "Support for SAVEPOINT", 
                                           FeatureLevel::kStandard);
  features_["savepoints"].is_supported = true;
  
  features_["nested_transactions"] = DialectFeature("nested_transactions", 
                                                    "Nested transaction blocks",
                                                    FeatureLevel::kExtended);
  features_["nested_transactions"].is_supported = false;
  
  features_["two_phase_commit"] = DialectFeature("two_phase_commit", 
                                                 "Two-phase commit protocol",
                                                 FeatureLevel::kExtended);
  features_["two_phase_commit"].is_supported = true;
  
  // DDL features
  features_["alter_table"] = DialectFeature("alter_table", 
                                           "ALTER TABLE support",
                                           FeatureLevel::kStandard);
  features_["alter_table"].is_supported = true;
  
  features_["create_index_concurrently"] = DialectFeature("create_index_concurrently",
                                                         "CREATE INDEX CONCURRENTLY",
                                                         FeatureLevel::kExtended);
  features_["create_index_concurrently"].is_supported = true;
  
  features_["if_not_exists"] = DialectFeature("if_not_exists",
                                             "IF NOT EXISTS clause",
                                             FeatureLevel::kStandard);
  features_["if_not_exists"].is_supported = true;
  
  features_["if_exists"] = DialectFeature("if_exists",
                                         "IF EXISTS clause",
                                         FeatureLevel::kStandard);
  features_["if_exists"].is_supported = true;
  
  // DML features
  features_["returning"] = DialectFeature("returning",
                                         "RETURNING clause",
                                         FeatureLevel::kStandard);
  features_["returning"].is_supported = true;
  
  features_["upsert"] = DialectFeature("upsert",
                                      "UPSERT/ON CONFLICT",
                                      FeatureLevel::kStandard);
  features_["upsert"].is_supported = true;
  
  features_["cte"] = DialectFeature("cte",
                                   "Common Table Expressions",
                                   FeatureLevel::kStandard);
  features_["cte"].is_supported = true;
  
  features_["recursive_cte"] = DialectFeature("recursive_cte",
                                             "Recursive CTEs",
                                             FeatureLevel::kStandard);
  features_["recursive_cte"].is_supported = true;
  
  features_["window_functions"] = DialectFeature("window_functions",
                                                "Window functions",
                                                FeatureLevel::kStandard);
  features_["window_functions"].is_supported = true;
  
  features_["lateral_joins"] = DialectFeature("lateral_joins",
                                             "LATERAL joins",
                                             FeatureLevel::kStandard);
  features_["lateral_joins"].is_supported = true;
  
  // Type features
  features_["arrays"] = DialectFeature("arrays",
                                      "Array types",
                                      FeatureLevel::kStandard);
  features_["arrays"].is_supported = true;
  
  features_["composite_types"] = DialectFeature("composite_types",
                                               "Composite types",
                                               FeatureLevel::kStandard);
  features_["composite_types"].is_supported = true;
  
  features_["range_types"] = DialectFeature("range_types",
                                           "Range types",
                                           FeatureLevel::kExtended);
  features_["range_types"].is_supported = true;
  
  features_["domain_types"] = DialectFeature("domain_types",
                                            "Domain types",
                                            FeatureLevel::kStandard);
  features_["domain_types"].is_supported = true;
  
  // Function features
  features_["user_defined_functions"] = DialectFeature("user_defined_functions",
                                                      "User-defined functions",
                                                      FeatureLevel::kStandard);
  features_["user_defined_functions"].is_supported = true;
  
  features_["aggregate_functions"] = DialectFeature("aggregate_functions",
                                                   "Aggregate functions",
                                                   FeatureLevel::kStandard);
  features_["aggregate_functions"].is_supported = true;
  
  features_["window_aggregate"] = DialectFeature("window_aggregate",
                                                "Window aggregates",
                                                FeatureLevel::kStandard);
  features_["window_aggregate"].is_supported = true;
  
  // Security features
  features_["row_level_security"] = DialectFeature("row_level_security",
                                                  "Row-level security",
                                                  FeatureLevel::kExtended);
  features_["row_level_security"].is_supported = true;
  
  features_["column_level_security"] = DialectFeature("column_level_security",
                                                     "Column-level security",
                                                     FeatureLevel::kExtended);
  features_["column_level_security"].is_supported = false;
  
  // Index features
  features_["btree_index"] = DialectFeature("btree_index",
                                           "B-tree indexes",
                                           FeatureLevel::kStandard);
  features_["btree_index"].is_supported = true;
  
  features_["hash_index"] = DialectFeature("hash_index",
                                          "Hash indexes",
                                          FeatureLevel::kStandard);
  features_["hash_index"].is_supported = true;
  
  features_["gin_index"] = DialectFeature("gin_index",
                                         "GIN indexes",
                                         FeatureLevel::kExtended);
  features_["gin_index"].is_supported = true;
  
  features_["gist_index"] = DialectFeature("gist_index",
                                          "GiST indexes",
                                          FeatureLevel::kExtended);
  features_["gist_index"].is_supported = true;
  
  features_["spgist_index"] = DialectFeature("spgist_index",
                                            "SP-GiST indexes",
                                            FeatureLevel::kExtended);
  features_["spgist_index"].is_supported = true;
  
  features_["brin_index"] = DialectFeature("brin_index",
                                          "BRIN indexes",
                                          FeatureLevel::kExtended);
  features_["brin_index"].is_supported = true;
  
  features_["partial_index"] = DialectFeature("partial_index",
                                             "Partial indexes",
                                             FeatureLevel::kStandard);
  features_["partial_index"].is_supported = true;
  
  features_["expression_index"] = DialectFeature("expression_index",
                                                "Expression indexes",
                                                FeatureLevel::kStandard);
  features_["expression_index"].is_supported = true;
}

bool DialectCapabilities::IsSupported(const std::string& feature_name) const {
  auto it = features_.find(feature_name);
  if (it != features_.end()) {
    return it->second.is_supported;
  }
  return false;
}

void DialectCapabilities::SetSupported(const std::string& feature_name, bool supported) {
  features_[feature_name].is_supported = supported;
}

std::optional<DialectFeature> DialectCapabilities::GetFeature(
    const std::string& feature_name) const {
  auto it = features_.find(feature_name);
  if (it != features_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<std::string> DialectCapabilities::GetSupportedFeatures() const {
  std::vector<std::string> result;
  for (const auto& [name, feature] : features_) {
    if (feature.is_supported) {
      result.push_back(name);
    }
  }
  return result;
}

std::vector<std::string> DialectCapabilities::GetUnsupportedFeatures() const {
  std::vector<std::string> result;
  for (const auto& [name, feature] : features_) {
    if (!feature.is_supported) {
      result.push_back(name);
    }
  }
  return result;
}

// ============================================================================
// ReservedWords Implementation
// ============================================================================

ReservedWords::ReservedWords() {
  InitializeReservedWords();
}

void ReservedWords::InitializeReservedWords() {
  // SQL standard reserved words
  const char* sql_reserved[] = {
    "all", "analyse", "analyze", "and", "any", "array", "as", "asc",
    "asymmetric", "authorization", "binary", "both", "case", "cast",
    "check", "collate", "collation", "column", "concurrently", "constraint",
    "create", "cross", "current_catalog", "current_date", "current_role",
    "current_schema", "current_time", "current_timestamp", "current_user",
    "default", "deferrable", "desc", "distinct", "do", "else", "end",
    "except", "false", "fetch", "for", "foreign", "freeze", "from", "full",
    "grant", "group", "having", "ilike", "in", "initially", "inner",
    "intersect", "into", "is", "isnull", "join", "lateral", "leading",
    "left", "like", "limit", "localtime", "localtimestamp", "natural",
    "not", "notnull", "null", "offset", "on", "only", "or", "order",
    "outer", "overlaps", "placing", "primary", "references", "returning",
    "right", "select", "session_user", "similar", "some", "symmetric",
    "table", "tablesample", "then", "to", "trailing", "true", "union",
    "unique", "user", "using", "variadic", "verbose", "when", "where",
    "window", "with"
  };
  
  for (const auto* word : sql_reserved) {
    reserved_words_.insert(word);
  }
  
  // ScratchBird-specific reserved words
  const char* sb_reserved[] = {
    "sblr", "scratchbird", "emulated", "native", "wire", "protocol",
    "sandbox", "domain", "policy", "registry"
  };
  
  for (const auto* word : sb_reserved) {
    reserved_words_.insert(word);
  }
}

bool ReservedWords::IsReserved(const std::string& word) const {
  return reserved_words_.find(NormalizeIdentifier(word)) != reserved_words_.end();
}

bool ReservedWords::IsReservedStrict(const std::string& word) const {
  return reserved_words_.find(word) != reserved_words_.end();
}

std::string ReservedWords::QuoteIdentifier(const std::string& identifier) const {
  if (IsReserved(identifier) || NeedsQuoting(identifier)) {
    return "\"" + identifier + "\"";
  }
  return identifier;
}

bool ReservedWords::NeedsQuoting(const std::string& identifier) const {
  // Check for non-identifier characters
  for (char c : identifier) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
      return true;
    }
  }
  
  // Check if starts with digit
  if (!identifier.empty() && std::isdigit(static_cast<unsigned char>(identifier[0]))) {
    return true;
  }
  
  // Check if mixed case (requires quoting to preserve case)
  bool has_upper = false;
  bool has_lower = false;
  for (char c : identifier) {
    if (std::isupper(static_cast<unsigned char>(c))) has_upper = true;
    if (std::islower(static_cast<unsigned char>(c))) has_lower = true;
  }
  
  return has_upper && has_lower;
}

std::vector<std::string> ReservedWords::GetAllReservedWords() const {
  return std::vector<std::string>(reserved_words_.begin(), reserved_words_.end());
}

void ReservedWords::AddReservedWord(const std::string& word) {
  reserved_words_.insert(NormalizeIdentifier(word));
}

// ============================================================================
// DialectSyntax Implementation
// ============================================================================

DialectSyntax::DialectSyntax() {
  InitializeDefaults();
}

void DialectSyntax::InitializeDefaults() {
  // String literals
  string_quote_ = "'";
  escape_string_ = "E";
  
  // Identifiers
  identifier_quote_ = '"';
  
  // Comments
  line_comment_ = "--";
  block_comment_start_ = "/*";
  block_comment_end_ = "*/";
  
  // Statement terminator
  statement_terminator_ = ";";
  
  // Parameter placeholder
  positional_param_ = "$";
  named_param_prefix_ = ":";
  
  // Catalog separator
  catalog_separator_ = ".";
  
  // Quoting rules
  supports_unicode_identifiers_ = true;
  case_sensitive_identifiers_ = false;
  case_sensitive_strings_ = true;
  
  // Escape sequences
  supports_hex_escapes_ = true;
  supports_unicode_escapes_ = true;
  supports_octal_escapes_ = false;
}

std::string DialectSyntax::QuoteString(const std::string& value) const {
  std::string result = string_quote_;
  
  for (char c : value) {
    if (c == '\'') {
      result += "''";
    } else if (c == '\\' && supports_unicode_escapes_) {
      result += "\\\\";
    } else {
      result += c;
    }
  }
  
  result += string_quote_;
  return result;
}

std::string DialectSyntax::QuoteIdentifier(const std::string& identifier) const {
  std::string result(1, identifier_quote_);
  
  for (char c : identifier) {
    if (c == identifier_quote_) {
      result += identifier_quote_;
      result += identifier_quote_;
    } else {
      result += c;
    }
  }
  
  result += identifier_quote_;
  return result;
}

std::string DialectSyntax::FormatPositionalParam(int index) const {
  return positional_param_ + std::to_string(index);
}

std::string DialectSyntax::FormatNamedParam(const std::string& name) const {
  return named_param_prefix_ + name;
}

std::string DialectSyntax::MakeLineComment(const std::string& text) const {
  return line_comment_ + " " + text;
}

std::string DialectSyntax::MakeBlockComment(const std::string& text) const {
  return block_comment_start_ + " " + text + " " + block_comment_end_;
}

// ============================================================================
// DialectTypeMapping Implementation
// ============================================================================

DialectTypeMapping::DialectTypeMapping() {
  InitializeBuiltinMappings();
}

void DialectTypeMapping::InitializeBuiltinMappings() {
  // Standard SQL type mappings to ScratchBird native types
  AddMapping("boolean", "bool", TypeMappingKind::kDirect);
  AddMapping("bool", "bool", TypeMappingKind::kDirect);
  
  AddMapping("smallint", "int2", TypeMappingKind::kDirect);
  AddMapping("int2", "int2", TypeMappingKind::kDirect);
  AddMapping("integer", "int4", TypeMappingKind::kDirect);
  AddMapping("int", "int4", TypeMappingKind::kDirect);
  AddMapping("int4", "int4", TypeMappingKind::kDirect);
  AddMapping("bigint", "int8", TypeMappingKind::kDirect);
  AddMapping("int8", "int8", TypeMappingKind::kDirect);
  
  AddMapping("real", "float4", TypeMappingKind::kDirect);
  AddMapping("float4", "float4", TypeMappingKind::kDirect);
  AddMapping("double precision", "float8", TypeMappingKind::kDirect);
  AddMapping("float8", "float8", TypeMappingKind::kDirect);
  AddMapping("float", "float8", TypeMappingKind::kDirect);
  
  AddMapping("numeric", "numeric", TypeMappingKind::kDirect);
  AddMapping("decimal", "numeric", TypeMappingKind::kDirect);
  
  AddMapping("char", "char", TypeMappingKind::kDirect);
  AddMapping("character", "char", TypeMappingKind::kDirect);
  AddMapping("varchar", "varchar", TypeMappingKind::kDirect);
  AddMapping("character varying", "varchar", TypeMappingKind::kDirect);
  AddMapping("text", "text", TypeMappingKind::kDirect);
  
  AddMapping("bytea", "bytea", TypeMappingKind::kDirect);
  AddMapping("blob", "bytea", TypeMappingKind::kCast);
  
  AddMapping("date", "date", TypeMappingKind::kDirect);
  AddMapping("time", "time", TypeMappingKind::kDirect);
  AddMapping("time with time zone", "timetz", TypeMappingKind::kDirect);
  AddMapping("timetz", "timetz", TypeMappingKind::kDirect);
  AddMapping("timestamp", "timestamp", TypeMappingKind::kDirect);
  AddMapping("timestamp without time zone", "timestamp", TypeMappingKind::kDirect);
  AddMapping("timestamp with time zone", "timestamptz", TypeMappingKind::kDirect);
  AddMapping("timestamptz", "timestamptz", TypeMappingKind::kDirect);
  AddMapping("interval", "interval", TypeMappingKind::kDirect);
  
  AddMapping("uuid", "uuid", TypeMappingKind::kDirect);
  AddMapping("xml", "xml", TypeMappingKind::kDirect);
  AddMapping("json", "json", TypeMappingKind::kDirect);
  AddMapping("jsonb", "jsonb", TypeMappingKind::kDirect);
  
  AddMapping("oid", "oid", TypeMappingKind::kDirect);
  AddMapping("regclass", "regclass", TypeMappingKind::kDirect);
  AddMapping("regtype", "regtype", TypeMappingKind::kDirect);
  AddMapping("regproc", "regproc", TypeMappingKind::kDirect);
  
  AddMapping("inet", "inet", TypeMappingKind::kDirect);
  AddMapping("cidr", "cidr", TypeMappingKind::kDirect);
  AddMapping("macaddr", "macaddr", TypeMappingKind::kDirect);
  
  AddMapping("point", "point", TypeMappingKind::kDirect);
  AddMapping("line", "line", TypeMappingKind::kDirect);
  AddMapping("lseg", "lseg", TypeMappingKind::kDirect);
  AddMapping("box", "box", TypeMappingKind::kDirect);
  AddMapping("path", "path", TypeMappingKind::kDirect);
  AddMapping("polygon", "polygon", TypeMappingKind::kDirect);
  AddMapping("circle", "circle", TypeMappingKind::kDirect);
  
  AddMapping("bit", "bit", TypeMappingKind::kDirect);
  AddMapping("bit varying", "varbit", TypeMappingKind::kDirect);
  AddMapping("varbit", "varbit", TypeMappingKind::kDirect);
}

void DialectTypeMapping::AddMapping(const std::string& source_type,
                                    const std::string& target_type,
                                    TypeMappingKind kind) {
  mappings_[NormalizeIdentifier(source_type)] = {target_type, kind};
}

std::optional<TypeMapping> DialectTypeMapping::GetMapping(
    const std::string& source_type) const {
  auto it = mappings_.find(NormalizeIdentifier(source_type));
  if (it != mappings_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::string DialectTypeMapping::MapType(const std::string& source_type) const {
  auto mapping = GetMapping(source_type);
  if (mapping) {
    return mapping->target_type;
  }
  return source_type;
}

bool DialectTypeMapping::HasMapping(const std::string& source_type) const {
  return mappings_.find(NormalizeIdentifier(source_type)) != mappings_.end();
}

std::vector<std::pair<std::string, TypeMapping>> DialectTypeMapping::GetAllMappings() const {
  std::vector<std::pair<std::string, TypeMapping>> result;
  for (const auto& [source, mapping] : mappings_) {
    result.emplace_back(source, mapping);
  }
  return result;
}

// ============================================================================
// Dialect Implementation
// ============================================================================

Dialect::Dialect() = default;

Dialect::~Dialect() = default;

Dialect Dialect::CreateScratchBirdDialect() {
  Dialect dialect;
  
  dialect.name_ = "ScratchBird";
  dialect.version_ = "1.0";
  dialect.description_ = "Native ScratchBird SQL dialect";
  
  // Capabilities are already initialized with ScratchBird-compatible defaults
  
  return dialect;
}

const std::string& Dialect::GetName() const {
  return name_;
}

const std::string& Dialect::GetVersion() const {
  return version_;
}

const std::string& Dialect::GetDescription() const {
  return description_;
}

DialectCapabilities& Dialect::GetCapabilities() {
  return capabilities_;
}

const DialectCapabilities& Dialect::GetCapabilities() const {
  return capabilities_;
}

ReservedWords& Dialect::GetReservedWords() {
  return reserved_words_;
}

const ReservedWords& Dialect::GetReservedWords() const {
  return reserved_words_;
}

DialectSyntax& Dialect::GetSyntax() {
  return syntax_;
}

const DialectSyntax& Dialect::GetSyntax() const {
  return syntax_;
}

DialectTypeMapping& Dialect::GetTypeMapping() {
  return type_mapping_;
}

const DialectTypeMapping& Dialect::GetTypeMapping() const {
  return type_mapping_;
}

bool Dialect::SupportsFeature(const std::string& feature_name) const {
  return capabilities_.IsSupported(feature_name);
}

std::string Dialect::QuoteIdentifier(const std::string& identifier) const {
  return reserved_words_.QuoteIdentifier(identifier);
}

std::string Dialect::QuoteString(const std::string& value) const {
  return syntax_.QuoteString(value);
}

std::string Dialect::MapType(const std::string& source_type) const {
  return type_mapping_.MapType(source_type);
}

// ============================================================================
// DialectRegistry Implementation
// ============================================================================

DialectRegistry::DialectRegistry() {
  // Register default ScratchBird dialect
  RegisterDialect("scratchbird", Dialect::CreateScratchBirdDialect());
}

DialectRegistry& DialectRegistry::Instance() {
  static DialectRegistry instance;
  return instance;
}

void DialectRegistry::RegisterDialect(const std::string& name, const Dialect& dialect) {
  dialects_[NormalizeIdentifier(name)] = dialect;
}

std::optional<Dialect> DialectRegistry::GetDialect(const std::string& name) const {
  auto it = dialects_.find(NormalizeIdentifier(name));
  if (it != dialects_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool DialectRegistry::HasDialect(const std::string& name) const {
  return dialects_.find(NormalizeIdentifier(name)) != dialects_.end();
}

std::vector<std::string> DialectRegistry::GetDialectNames() const {
  std::vector<std::string> names;
  for (const auto& [name, _] : dialects_) {
    names.push_back(name);
  }
  return names;
}

Dialect DialectRegistry::GetDefaultDialect() const {
  auto it = dialects_.find("scratchbird");
  if (it != dialects_.end()) {
    return it->second;
  }
  return Dialect::CreateScratchBirdDialect();
}

}  // namespace core
}  // namespace scratchrobin
