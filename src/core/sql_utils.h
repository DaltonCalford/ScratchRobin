/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#pragma once

#include <string>
#include <string_view>

namespace scratchrobin::core {

/**
 * SQL Utility Functions
 *
 * Provides safe SQL string construction to prevent SQL injection attacks.
 * All user-provided identifiers and string literals should be escaped
 * using these functions before inclusion in SQL queries.
 */

/**
 * Escape SQL identifiers (table names, column names, schema names, etc.)
 *
 * Wraps the identifier in double quotes and escapes embedded quotes.
 * Example: my_table -> "my_table"
 *          my"table -> "my""table"
 *
 * @param identifier The identifier to escape
 * @return Properly quoted and escaped identifier
 */
std::string escapeIdentifier(std::string_view identifier);

/**
 * Escape SQL string literals
 *
 * Wraps the string in single quotes and escapes embedded quotes.
 * Example: hello -> 'hello'
 *          it's -> 'it''s'
 *
 * @param value The string literal to escape
 * @return Properly quoted and escaped string literal
 */
std::string escapeStringLiteral(std::string_view value);

/**
 * Validate identifier format
 *
 * Checks if an identifier contains only valid characters:
 * - Letters (a-z, A-Z)
 * - Digits (0-9)
 * - Underscore (_)
 * - Dollar sign ($) - for some databases
 * - Must not start with a digit
 *
 * @param identifier The identifier to validate
 * @return true if the identifier is valid
 */
bool isValidIdentifier(std::string_view identifier);

/**
 * Sanitize identifier by removing/replacing invalid characters
 *
 * @param identifier The identifier to sanitize
 * @param replacement Character to replace invalid chars with
 * @return Sanitized identifier
 */
std::string sanitizeIdentifier(std::string_view identifier, char replacement = '_');

/**
 * Escape SQL LIKE pattern special characters
 *
 * Escapes % and _ characters for use in LIKE patterns.
 * Example: 100% -> 100\%
 *          my_table -> my\_table
 *
 * @param pattern The pattern to escape
 * @param escape_char The escape character (default: backslash)
 * @return Escaped pattern
 */
std::string escapeLikePattern(std::string_view pattern, char escape_char = '\\');

/**
 * Build a qualified table name (schema.table)
 *
 * Properly escapes both schema and table identifiers.
 *
 * @param schema Schema name (may be empty)
 * @param table Table name
 * @return Qualified and escaped name like "schema"."table"
 */
std::string qualifiedTableName(std::string_view schema, std::string_view table);

}  // namespace scratchrobin::core
