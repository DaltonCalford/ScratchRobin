#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <memory>
#include <vector>
#include "mssql_features.h"

namespace scratchrobin {

// MSSQL Syntax Elements
struct MSSQLSyntaxElement {
    QString name;
    QString pattern;
    QString description;
    bool isKeyword = false;
    bool isFunction = false;
    bool isOperator = false;
    bool isDataType = false;

    MSSQLSyntaxElement() = default;
    MSSQLSyntaxElement(const QString& name, const QString& pattern, const QString& description = "",
                      bool keyword = false, bool function = false, bool op = false, bool datatype = false)
        : name(name), pattern(pattern), description(description), isKeyword(keyword),
          isFunction(function), isOperator(op), isDataType(datatype) {}
};

// MSSQL Syntax Highlighter Patterns
struct MSSQLSyntaxPatterns {
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
    static const QString BRACKETED_IDENTIFIER;

    // Numbers
    static const QString NUMBER_LITERAL;

    // Special constructs
    static const QString VARIABLE;
    static const QString TEMP_TABLE;
    static const QString SYSTEM_OBJECT;

    // Get all keywords (reserved + non-reserved)
    static QStringList getAllKeywords();

    // Get all syntax elements
    static QList<MSSQLSyntaxElement> getAllSyntaxElements();
};

// MSSQL Parser for SQL validation and analysis
class MSSQLParser {
public:
    // Parse SQL text and return syntax elements
    static QList<MSSQLSyntaxElement> parseSQL(const QString& sql);

    // Validate SQL syntax (basic validation)
    static bool validateSQLSyntax(const QString& sql, QStringList& errors, QStringList& warnings);

    // Extract objects from SQL (tables, columns, etc.)
    static QStringList extractTableNames(const QString& sql);
    static QStringList extractColumnNames(const QString& sql);
    static QStringList extractFunctionNames(const QString& sql);
    static QStringList extractVariableNames(const QString& sql);

    // Format SQL (basic formatting)
    static QString formatSQL(const QString& sql);

    // Get suggestions for auto-completion
    static QStringList getCompletionSuggestions(const QString& partialText, const QString& context = "");

    // Check if identifier needs quoting
    static bool needsQuoting(const QString& identifier);

    // Escape identifier if needed
    static QString escapeIdentifier(const QString& identifier);

    // Parse CREATE statements
    static bool parseCreateTable(const QString& sql, QString& tableName, QStringList& columns);
    static bool parseCreateIndex(const QString& sql, QString& indexName, QString& tableName, QStringList& columns);
    static bool parseCreateView(const QString& sql, QString& viewName, QString& definition);

    // Parse SELECT statements
    static bool parseSelectStatement(const QString& sql, QStringList& columns, QStringList& tables, QString& whereClause);

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

// MSSQL Query Analyzer for performance and optimization
class MSSQLQueryAnalyzer {
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

private:
    static bool hasSelectStar(const QString& sql);
    static bool hasImplicitConversion(const QString& sql);
    static bool hasCartesianProduct(const QString& sql);
    static bool hasUnnecessaryJoins(const QString& sql);
    static bool usesFunctionsInWhere(const QString& sql);
    static bool hasSuboptimalLike(const QString& sql);
};

// MSSQL Code Formatter
class MSSQLCodeFormatter {
public:
    // Format SQL code with proper indentation and spacing
    static QString formatCode(const QString& sql, int indentSize = 4);

    // Remove extra whitespace
    static QString compressCode(const QString& sql);

    // Expand compressed code
    static QString expandCode(const QString& sql);

    // Convert case (upper/lower/keyword case)
    static QString convertCase(const QString& sql, bool upperKeywords = true, bool upperFunctions = false);

private:
    static QString indentCode(const QString& sql, int indentSize);
    static QString formatKeywords(const QString& sql, bool uppercase);
    static QString formatFunctions(const QString& sql, bool uppercase);
    static QString addNewlines(const QString& sql);
    static QString alignClauses(const QString& sql);
};

// MSSQL Syntax Validator
class MSSQLSyntaxValidator {
public:
    // Comprehensive syntax validation
    static bool validateSyntax(const QString& sql, QStringList& errors, QStringList& warnings);

    // Check specific syntax elements
    static bool validateIdentifiers(const QString& sql, QStringList& errors);
    static bool validateDataTypes(const QString& sql, QStringList& errors);
    static bool validateFunctions(const QString& sql, QStringList& errors);
    static bool validateOperators(const QString& sql, QStringList& errors);

    // Check for MSSQL-specific syntax
    static bool validateTsqlExtensions(const QString& sql, QStringList& errors, QStringList& warnings);

    // Check for deprecated features
    static QStringList checkDeprecatedFeatures(const QString& sql);

private:
    static bool isValidIdentifier(const QString& identifier);
    static bool isValidDataType(const QString& dataType);
    static bool isValidFunction(const QString& function);
    static bool isValidOperator(const QString& op);

    static bool hasUnclosedComments(const QString& sql);
    static bool hasUnclosedStrings(const QString& sql);
    static bool hasUnclosedBrackets(const QString& sql);
};

// MSSQL IntelliSense Provider
class MSSQLIntelliSense {
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

private:
    static QString getCurrentContext(const QString& text, int cursorPosition);
    static QString getCurrentWord(const QString& text, int cursorPosition);
    static bool isInString(const QString& text, int position);
    static bool isInComment(const QString& text, int position);
};

// MSSQL Script Executor with error handling
class MSSQLScriptExecutor {
public:
    // Execute SQL script with proper error handling
    static bool executeScript(const QString& script, QStringList& results, QStringList& errors);

    // Execute individual statements
    static bool executeStatement(const QString& statement, QStringList& results, QString& error);

    // Batch execution with transaction support
    static bool executeBatch(const QStringList& statements, QStringList& results, QStringList& errors, bool useTransaction = true);

    // Parse script into individual statements
    static QStringList parseScript(const QString& script);

private:
    static QStringList splitStatements(const QString& script);
    static bool isCompleteStatement(const QString& statement);
    static QString cleanStatement(const QString& statement);
};

} // namespace scratchrobin
