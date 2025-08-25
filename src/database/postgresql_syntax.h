#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <memory>
#include <vector>
#include "postgresql_features.h"

namespace scratchrobin {

// PostgreSQL Syntax Elements
struct PostgreSQLSyntaxElement {
    QString name;
    QString pattern;
    QString description;
    bool isKeyword = false;
    bool isFunction = false;
    bool isOperator = false;
    bool isDataType = false;

    PostgreSQLSyntaxElement() = default;
    PostgreSQLSyntaxElement(const QString& name, const QString& pattern, const QString& description = "",
                          bool keyword = false, bool function = false, bool op = false, bool datatype = false)
        : name(name), pattern(pattern), description(description), isKeyword(keyword),
          isFunction(function), isOperator(op), isDataType(datatype) {}
};

// PostgreSQL Syntax Patterns
struct PostgreSQLSyntaxPatterns {
    // Keywords
    static const QStringList RESERVED_KEYWORDS;
    static const QStringList NON_RESERVED_KEYWORDS;

    // Data types
    static const QStringList DATA_TYPES;

    // Built-in functions
    static const QStringList BUILTIN_FUNCTIONS;

    // Operators
    static const QStringList OPERATORS;

    // Comments
    static const QString SINGLE_LINE_COMMENT;
    static const QString MULTI_LINE_COMMENT_START;
    static const QString MULTI_LINE_COMMENT_END;

    // Strings and identifiers
    static const QString STRING_LITERAL;
    static const QString IDENTIFIER;
    static const QString QUOTED_IDENTIFIER;

    // Numbers
    static const QString NUMBER_LITERAL;

    // Special constructs
    static const QString VARIABLE;
    static const QString SYSTEM_VARIABLE;
    static const QString ARRAY_LITERAL;
    static const QString JSON_LITERAL;

    // Get all keywords (reserved + non-reserved)
    static QStringList getAllKeywords();

    // Get all syntax elements
    static QList<PostgreSQLSyntaxElement> getAllSyntaxElements();
};

// PostgreSQL Parser for SQL validation and analysis
class PostgreSQLParser {
public:
    // Parse SQL text and return syntax elements
    static QList<PostgreSQLSyntaxElement> parseSQL(const QString& sql);

    // Validate SQL syntax (basic validation)
    static bool validateSQLSyntax(const QString& sql, QStringList& errors, QStringList& warnings);

    // Extract objects from SQL (tables, columns, etc.)
    static QStringList extractTableNames(const QString& sql);
    static QStringList extractColumnNames(const QString& sql);
    static QStringList extractFunctionNames(const QString& sql);
    static QStringList extractVariableNames(const QString& sql);
    static QStringList extractSchemaNames(const QString& sql);

    // Format SQL (basic formatting)
    static QString formatSQL(const QString& sql);

    // Get suggestions for auto-completion
    static QStringList getCompletionSuggestions(const QString& partialText, const QString& context = "");

    // Check if identifier needs quoting
    static bool needsQuoting(const QString& identifier);

    // Escape identifier if needed
    static QString escapeIdentifier(const QString& identifier);

    // Parse CREATE statements
    static bool parseCreateTable(const QString& sql, QString& tableName, QStringList& columns, QString& options);
    static QString buildCreateTableQuery(const QString& tableName, const QList<QPair<QString, QString>>& columns,
                                       const QString& schema = "", const QString& database = "");
    static bool parseCreateIndex(const QString& sql, QString& indexName, QString& tableName, QStringList& columns);
    static bool parseCreateView(const QString& sql, QString& viewName, QString& definition);
    static bool parseCreateFunction(const QString& sql, QString& functionName, QStringList& parameters, QString& returnType);
    static bool parseCreateExtension(const QString& sql, QString& extensionName, QString& version);

    // Parse SELECT statements
    static bool parseSelectStatement(const QString& sql, QStringList& columns, QStringList& tables, QString& whereClause);

    // PostgreSQL-specific parsing
    static bool parseArrayLiteral(const QString& sql, QStringList& elements);
    static bool parseJsonPath(const QString& sql, QStringList& pathElements);

private:
    // Tokenization
    static QStringList tokenize(const QString& sql);
    static bool isKeyword(const QString& token);
    static bool isOperator(const QString& token);
    static bool isDataType(const QString& token);
    static bool isFunction(const QString& token);

    // Parsing helpers
    static QString extractIdentifier(const QStringList& tokens, int& currentIndex);
    static QString extractStringLiteral(const QStringList& tokens, int& currentIndex);
    static QString extractExpression(const QStringList& tokens, int& currentIndex);
};

// PostgreSQL Query Analyzer for performance and optimization
class PostgreSQLQueryAnalyzer {
public:
    // Analyze query for potential issues
    static void analyzeQuery(const QString& sql, QStringList& issues, QStringList& suggestions);

    // Estimate query complexity
    static int estimateComplexity(const QString& sql);

    // Check for best practices violations
    static QStringList checkBestPractices(const QString& sql);

    // Identify missing indexes
    static QStringList suggestIndexes(const QString& sql);

    // Check for potential security issues
    static QStringList checkSecurityIssues(const QString& sql);

    // PostgreSQL-specific analysis
    static QStringList checkPostgreSQLSpecificIssues(const QString& sql);
    static QStringList suggestPostgreSQLOptimizations(const QString& sql);

    // Analyze EXPLAIN output
    static QStringList analyzeExplainPlan(const QString& explainOutput);

private:
    static bool hasSelectStar(const QString& sql);
    static bool hasImplicitJoins(const QString& sql);
    static bool hasCartesianProduct(const QString& sql);
    static bool hasUnnecessaryJoins(const QString& sql);
    static bool usesFunctionsInWhere(const QString& sql);
    static bool hasSuboptimalLike(const QString& sql);
    static bool hasMissingIndexes(const QString& sql);
    static bool hasSeqScanInsteadOfIndex(const QString& sql);
    static bool hasHighCostOperations(const QString& sql);
};

// PostgreSQL Code Formatter
class PostgreSQLCodeFormatter {
public:
    // Format SQL code with proper indentation and spacing
    static QString formatCode(const QString& sql, int indentSize = 4);

    // Remove extra whitespace
    static QString compressCode(const QString& sql);

    // Expand compressed code
    static QString expandCode(const QString& sql);

    // Convert case (upper/lower/keyword case)
    static QString convertCase(const QString& sql, bool upperKeywords = true, bool upperFunctions = false);

    // PostgreSQL-specific formatting
    static QString formatArrayLiterals(const QString& sql);
    static QString formatJsonPaths(const QString& sql);
    static QString formatCTEs(const QString& sql);
    static QString formatWindowFunctions(const QString& sql);

private:
    static QString indentCode(const QString& sql, int indentSize);
    static QString formatKeywords(const QString& sql, bool uppercase);
    static QString formatFunctions(const QString& sql, bool uppercase);
    static QString addNewlines(const QString& sql);
    static QString alignClauses(const QString& sql);
};

// PostgreSQL Syntax Validator
class PostgreSQLSyntaxValidator {
public:
    // Comprehensive syntax validation
    static bool validateSyntax(const QString& sql, QStringList& errors, QStringList& warnings);

    // Check specific syntax elements
    static bool validateIdentifiers(const QString& sql, QStringList& errors);
    static bool validateDataTypes(const QString& sql, QStringList& errors);
    static bool validateFunctions(const QString& sql, QStringList& errors);
    static bool validateOperators(const QString& sql, QStringList& errors);

    // Check for PostgreSQL-specific syntax
    static bool validatePostgreSQLExtensions(const QString& sql, QStringList& errors, QStringList& warnings);

    // Check for deprecated features
    static QStringList checkDeprecatedFeatures(const QString& sql);

    // Validate PostgreSQL-specific constructs
    static QStringList validateArraySyntax(const QString& sql);
    static QStringList validateJsonSyntax(const QString& sql);
    static QStringList validateFulltextSyntax(const QString& sql);

private:
    static bool isValidIdentifier(const QString& identifier);
    static bool isValidDataType(const QString& dataType);
    static bool isValidFunction(const QString& function);
    static bool isValidOperator(const QString& op);

    static bool hasUnclosedComments(const QString& sql);
    static bool hasUnclosedStrings(const QString& sql);
    static bool hasUnclosedBrackets(const QString& sql);
    static bool hasInvalidArraySyntax(const QString& sql);
    static bool hasInvalidJsonSyntax(const QString& sql);
};

// PostgreSQL IntelliSense Provider
class PostgreSQLIntelliSense {
public:
    // Get completion items for current context
    static QStringList getCompletions(const QString& text, int cursorPosition);

    // Get context-aware suggestions
    static QStringList getContextSuggestions(const QString& text, int cursorPosition);

    // Get table/column suggestions
    static QStringList getTableSuggestions(const QString& partialName = "");
    static QStringList getColumnSuggestions(const QString& tableName, const QString& partialName = "");

    // Get keyword suggestions
    static QStringList getKeywordSuggestions(const QString& partialName = "");

    // Get function suggestions
    static QStringList getFunctionSuggestions(const QString& partialName = "");

    // Get operator suggestions
    static QStringList getOperatorSuggestions();

    // PostgreSQL-specific suggestions
    static QStringList getSchemaSuggestions(const QString& partialName = "");
    static QStringList getExtensionSuggestions(const QString& partialName = "");
    static QStringList getArraySuggestions();
    static QStringList getJsonSuggestions();

private:
    static QString getCurrentContext(const QString& text, int cursorPosition);
    static QString getCurrentWord(const QString& text, int cursorPosition);
    static bool isInString(const QString& text, int position);
    static bool isInComment(const QString& text, int position);
    static bool isInArrayLiteral(const QString& text, int position);
    static bool isInJsonPath(const QString& text, int position);
};

// PostgreSQL Script Executor with error handling
class PostgreSQLScriptExecutor {
public:
    // Execute SQL script with proper error handling
    static bool executeScript(const QString& script, QStringList& results, QStringList& errors);

    // Execute individual statements
    static bool executeStatement(const QString& statement, QStringList& results, QString& error);

    // Batch execution with transaction support
    static bool executeBatch(const QStringList& statements, QStringList& results, QStringList& errors, bool useTransaction = true);

    // Parse script into individual statements
    static QStringList parseScript(const QString& script);

    // PostgreSQL-specific execution
    static bool executeWithCopy(const QString& copyCommand, const QString& data, QString& error);
    static bool executeWithCursor(const QString& sql, int fetchSize, QStringList& results, QString& error);

private:
    static QStringList splitStatements(const QString& script);
    static bool isCompleteStatement(const QString& statement);
    static QString cleanStatement(const QString& statement);
};

// PostgreSQL EXPLAIN Plan Analyzer
class PostgreSQLExplainAnalyzer {
public:
    // Analyze EXPLAIN output
    static QStringList analyzeExplainOutput(const QString& explainOutput);

    // Get query execution plan
    static QString getQueryPlan(const QString& sql);

    // Identify performance bottlenecks
    static QStringList identifyBottlenecks(const QString& explainOutput);

    // Suggest optimizations
    static QStringList suggestOptimizations(const QString& explainOutput);

private:
    static bool hasSequentialScan(const QString& explainOutput);
    static bool hasIndexScan(const QString& explainOutput);
    static bool hasNestedLoop(const QString& explainOutput);
    static bool hasHashJoin(const QString& explainOutput);
    static bool hasMergeJoin(const QString& explainOutput);
    static bool hasSortOperation(const QString& explainOutput);
    static bool hasHighCostOperation(const QString& explainOutput);
    static int getEstimatedCost(const QString& explainOutput);
    static int getEstimatedRows(const QString& explainOutput);
};

} // namespace scratchrobin
