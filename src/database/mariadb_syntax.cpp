#include "mariadb_syntax.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

namespace scratchrobin {

// MariaDB Syntax Patterns Implementation

const QStringList MariaDBSyntaxPatterns::RESERVED_KEYWORDS = {
    "ACCESSIBLE", "ADD", "ALL", "ALTER", "ANALYZE", "AND", "AS", "ASC", "ASENSITIVE",
    "BEFORE", "BETWEEN", "BIGINT", "BINARY", "BLOB", "BOTH", "BY", "CALL", "CASCADE",
    "CASE", "CHANGE", "CHAR", "CHARACTER", "CHECK", "COLLATE", "COLUMN", "CONDITION",
    "CONSTRAINT", "CONTINUE", "CONVERT", "CREATE", "CROSS", "CURRENT_DATE", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "DATABASE", "DATABASES", "DAY_HOUR",
    "DAY_MICROSECOND", "DAY_MINUTE", "DAY_SECOND", "DEC", "DECIMAL", "DECLARE",
    "DEFAULT", "DELAYED", "DELETE", "DESC", "DESCRIBE", "DETERMINISTIC", "DISTINCT",
    "DISTINCTROW", "DIV", "DOUBLE", "DROP", "DUAL", "EACH", "ELSE", "ELSEIF", "ENCLOSED",
    "ESCAPED", "EXISTS", "EXIT", "EXPLAIN", "FALSE", "FETCH", "FLOAT", "FOR", "FORCE",
    "FOREIGN", "FROM", "FULLTEXT", "GRANT", "GROUP", "HAVING", "HIGH_PRIORITY",
    "HOUR_MICROSECOND", "HOUR_MINUTE", "HOUR_SECOND", "IF", "IGNORE", "IN", "INDEX",
    "INFILE", "INNER", "INOUT", "INSENSITIVE", "INSERT", "INT", "INTEGER", "INTERVAL",
    "INTO", "IS", "ITERATE", "JOIN", "KEY", "KEYS", "KILL", "LEADING", "LEAVE", "LEFT",
    "LIKE", "LIMIT", "LINEAR", "LINES", "LOAD", "LOCALTIME", "LOCALTIMESTAMP", "LOCK",
    "LONG", "LONGBLOB", "LONGTEXT", "LOOP", "LOW_PRIORITY", "MATCH", "MEDIUMBLOB",
    "MEDIUMINT", "MEDIUMTEXT", "MIDDLEINT", "MINUTE_MICROSECOND", "MINUTE_SECOND",
    "MOD", "MODIFIES", "NATURAL", "NOT", "NO_WRITE_TO_BINLOG", "NULL", "NUMERIC",
    "ON", "OPTIMIZE", "OPTION", "OPTIONALLY", "OR", "ORDER", "OUT", "OUTER", "OUTFILE",
    "PRECISION", "PRIMARY", "PROCEDURE", "PURGE", "RANGE", "READ", "READS", "REAL",
    "REFERENCES", "REGEXP", "RELEASE", "RENAME", "REPEAT", "REPLACE", "REQUIRE",
    "RESTRICT", "RETURN", "REVOKE", "RIGHT", "RLIKE", "SCHEMA", "SCHEMAS", "SECOND_MICROSECOND",
    "SELECT", "SENSITIVE", "SEPARATOR", "SET", "SHOW", "SMALLINT", "SOME", "SONAME",
    "SPATIAL", "SPECIFIC", "SQL", "SQLEXCEPTION", "SQLSTATE", "SQLWARNING", "SQL_BIG_RESULT",
    "SQL_CALC_FOUND_ROWS", "SQL_SMALL_RESULT", "SSL", "STARTING", "STRAIGHT_JOIN",
    "TABLE", "TERMINATED", "THEN", "TINYBLOB", "TINYINT", "TINYTEXT", "TO", "TRAILING",
    "TRIGGER", "TRUE", "UNDO", "UNION", "UNIQUE", "UNLOCK", "UNSIGNED", "UPDATE", "USAGE",
    "USE", "USING", "UTC_DATE", "UTC_TIME", "UTC_TIMESTAMP", "VALUES", "VARBINARY",
    "VARCHAR", "VARYING", "WHEN", "WHERE", "WHILE", "WITH", "WRITE", "XOR", "YEAR_MONTH",
    "ZEROFILL"
};

const QStringList MariaDBSyntaxPatterns::NON_RESERVED_KEYWORDS = {
    "ABORT", "ABSOLUTE", "ACCESS", "ACTION", "ADD", "ADMIN", "AFTER", "AGGREGATE",
    "ALGORITHM", "ALWAYS", "ANALYSE", "ANALYZE", "ANY", "ARRAY", "ASENSITIVE", "ASSERTION",
    "ASSIGNMENT", "AT", "AUTHORIZATION", "BACKWARD", "BEFORE", "BEGIN", "BIGSERIAL",
    "BINARY", "BOOLEAN", "BY", "CACHE", "CALLED", "CASCADE", "CASCADED", "CATALOG",
    "CHAIN", "CHARACTERISTICS", "CHECKPOINT", "CLASS", "CLOSE", "CLUSTER", "COALESCE",
    "COLLATION", "COLUMN_NAME", "COMMENT", "COMMENTS", "COMMIT", "COMMITTED", "COMPLETION",
    "CONCURRENTLY", "CONFIGURATION", "CONNECTION", "CONSTRAINTS", "CONTENT", "CONTINUE",
    "CONVERSION", "COPY", "COST", "CREATEDB", "CREATEROLE", "CREATEUSER", "CSV", "CUBE",
    "CURRENT", "CURSOR_NAME", "CYCLE", "DATA", "DATABASE", "DAY", "DEALLOCATE", "DEC",
    "DEFAULTS", "DEFERRABLE", "DEFERRED", "DEFINER", "DELETE", "DELIMITER", "DELIMITERS",
    "DESC", "DICTIONARY", "DISABLE", "DISCARD", "DOCUMENT", "DOMAIN", "DROP", "EACH",
    "ENABLE", "ENCODING", "ENCRYPTED", "ENUM", "ESCAPE", "EVENT", "EXCLUDE", "EXCLUDING",
    "EXCLUSIVE", "EXECUTE", "EXPLAIN", "EXTENSION", "EXTERNAL", "FAMILY", "FILTER",
    "FIRST", "FOLLOWING", "FORCE", "FORWARD", "FREEZE", "FUNCTION", "GENEVA", "GLOBAL",
    "GRANT", "GRANTED", "GREATEST", "HANDLER", "HEADER", "HOLD", "HOUR", "IDENTITY",
    "IF", "ILIKE", "IMMEDIATE", "IMMUTABLE", "IMPLICIT", "IMPORT", "INCLUDING", "INCREMENT",
    "INDEX", "INDEXES", "INHERIT", "INHERITS", "INITIALLY", "INLINE", "INPUT", "INSENSITIVE",
    "INSERT", "INSTEAD", "INVOKER", "ISNULL", "ISOLATION", "KEY", "LABEL", "LANGUAGE",
    "LARGE", "LAST", "LEAKPROOF", "LEAST", "LEVEL", "LISTEN", "LOAD", "LOCAL", "LOCATION",
    "LOCK", "LOGGED", "MAPPING", "MATCH", "MATERIALIZED", "MAXVALUE", "MINUTE", "MINVALUE",
    "MODE", "MONTH", "MOVE", "NAME", "NAMES", "NEXT", "NO", "NONE", "NOTHING", "NOTIFY",
    "NOTNULL", "NOWAIT", "NULLS", "OBJECT", "OF", "OFF", "OIDS", "ONLY", "OPTIONS", "ORDINALITY",
    "OUT", "OVER", "OVERLAPS", "OWNED", "OWNER", "PARSER", "PARTIAL", "PARTITION", "PASSING",
    "PASSWORD", "PLANS", "POLICY", "PRECEDING", "PREPARE", "PREPARED", "PRESERVE", "PRIOR",
    "PRIVILEGES", "PROCEDURAL", "PROCEDURE", "PROGRAM", "QUOTE", "RANGE", "READ", "REASSIGN",
    "RECHECK", "RECURSIVE", "REF", "REFRESH", "REINDEX", "RELATIVE", "RELEASE", "RENAME",
    "REPEATABLE", "REPLACE", "REPLICA", "RESET", "RESTART", "RESTRICT", "RETURNING", "RETURNS",
    "REVOKE", "ROLE", "ROLLBACK", "ROLLUP", "ROW", "ROWS", "RULE", "SAVEPOINT", "SCHEMA",
    "SCROLL", "SEARCH", "SECOND", "SECURITY", "SELECTIVE", "SEQUENCE", "SEQUENCES", "SERIALIZABLE",
    "SERVER", "SESSION", "SETOF", "SETS", "SHARE", "SHOW", "SIMILAR", "SIMPLE", "SNAPSHOT",
    "SOME", "SQL", "STABLE", "STANDALONE", "START", "STATEMENT", "STATISTICS", "STDIN", "STDOUT",
    "STORAGE", "STRICT", "STRIP", "SUBSTRING", "SYMMETRIC", "SYSID", "SYSTEM", "TABLES", "TABLESPACE",
    "TEMP", "TEMPLATE", "TEMPORARY", "TEXT", "TRANSACTION", "TRANSFORM", "TREAT", "TRIGGER",
    "TRIM", "TRUNCATE", "TRUSTED", "TYPE", "TYPES", "UESCAPE", "UNBOUNDED", "UNCOMMITTED",
    "UNKNOWN", "UNLISTEN", "UNLOGGED", "UNTIL", "UPDATE", "VACUUM", "VALID", "VALIDATE",
    "VALIDATOR", "VALUE", "VARYING", "VERBOSE", "VERSION", "VIEW", "VIEWS", "VOLATILE",
    "WHITESPACE", "WORK", "WRAPPER", "WRITE", "XML", "XMLATTRIBUTES", "XMLCONCAT", "XMLELEMENT",
    "XMLEXISTS", "XMLFOREST", "XMLPARSE", "XMLPI", "XMLROOT", "XMLSERIALIZE", "YEAR", "YES",
    "ZONE"
};

const QStringList MariaDBSyntaxPatterns::DATA_TYPES = {
    "tinyint", "smallint", "mediumint", "int", "integer", "bigint",
    "decimal", "dec", "numeric", "float", "double", "real", "bit",
    "serial", "bigserial", "char", "varchar", "tinytext", "text", "mediumtext", "longtext",
    "binary", "varbinary", "tinyblob", "blob", "mediumblob", "longblob",
    "enum", "set", "date", "datetime", "timestamp", "time", "year",
    "geometry", "point", "linestring", "polygon", "multipoint",
    "multilinestring", "multipolygon", "geometrycollection", "json"
};

const QStringList MariaDBSyntaxPatterns::BUILTIN_FUNCTIONS = {
    // String functions
    "ASCII", "BIN", "BIT_LENGTH", "CHAR", "CHAR_LENGTH", "CHARACTER_LENGTH",
    "CONCAT", "CONCAT_WS", "ELT", "EXPORT_SET", "FIELD", "FIND_IN_SET", "FORMAT",
    "FROM_BASE64", "HEX", "INSTR", "LCASE", "LEFT", "LENGTH", "LOAD_FILE", "LOCATE",
    "LOWER", "LPAD", "LTRIM", "MAKE_SET", "MID", "OCT", "OCTET_LENGTH", "ORD",
    "POSITION", "QUOTE", "REPEAT", "REPLACE", "REVERSE", "RIGHT", "RPAD", "RTRIM",
    "SOUNDEX", "SPACE", "STRCMP", "SUBSTR", "SUBSTRING", "SUBSTRING_INDEX", "TO_BASE64",
    "TRIM", "UCASE", "UNHEX", "UPPER", "WEIGHT_STRING",

    // Numeric functions
    "ABS", "ACOS", "ASIN", "ATAN", "ATAN2", "CEIL", "CEILING", "CONV", "COS", "COT",
    "CRC32", "DEGREES", "DIV", "EXP", "FLOOR", "GREATEST", "LEAST", "LN", "LOG", "LOG10",
    "LOG2", "MOD", "PI", "POW", "POWER", "RADIANS", "RAND", "ROUND", "SIGN", "SIN",
    "SQRT", "TAN", "TRUNCATE",

    // Date/Time functions
    "ADDDATE", "ADDTIME", "CONVERT_TZ", "CURDATE", "CURRENT_DATE", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "CURTIME", "DATE", "DATE_ADD", "DATE_FORMAT", "DATE_SUB",
    "DATEDIFF", "DAY", "DAYNAME", "DAYOFMONTH", "DAYOFWEEK", "DAYOFYEAR", "EXTRACT",
    "FROM_DAYS", "FROM_UNIXTIME", "GET_FORMAT", "HOUR", "LAST_DAY", "LOCALTIME",
    "LOCALTIMESTAMP", "MAKEDATE", "MAKETIME", "MICROSECOND", "MINUTE", "MONTH",
    "MONTHNAME", "NOW", "PERIOD_ADD", "PERIOD_DIFF", "QUARTER", "SECOND", "SEC_TO_TIME",
    "STR_TO_DATE", "SUBDATE", "SUBTIME", "SYSDATE", "TIME", "TIME_FORMAT", "TIME_TO_SEC",
    "TIMEDIFF", "TIMESTAMP", "TIMESTAMPADD", "TIMESTAMPDIFF", "TO_DAYS", "TO_SECONDS",
    "UNIX_TIMESTAMP", "UTC_DATE", "UTC_TIME", "UTC_TIMESTAMP", "WEEK", "WEEKDAY",
    "WEEKOFYEAR", "YEAR", "YEARWEEK",

    // Aggregate functions
    "AVG", "BIT_AND", "BIT_OR", "BIT_XOR", "COUNT", "GROUP_CONCAT", "MAX", "MIN",
    "STD", "STDDEV", "STDDEV_POP", "STDDEV_SAMP", "SUM", "VAR_POP", "VAR_SAMP", "VARIANCE",

    // JSON functions
    "JSON_ARRAY", "JSON_ARRAY_APPEND", "JSON_ARRAY_INSERT", "JSON_COMPACT", "JSON_CONTAINS",
    "JSON_CONTAINS_PATH", "JSON_DEPTH", "JSON_EXTRACT", "JSON_INSERT", "JSON_KEYS",
    "JSON_LENGTH", "JSON_MERGE", "JSON_MERGE_PATCH", "JSON_MERGE_PRESERVE", "JSON_OBJECT",
    "JSON_PRETTY", "JSON_QUOTE", "JSON_REMOVE", "JSON_REPLACE", "JSON_SEARCH", "JSON_SET",
    "JSON_TYPE", "JSON_UNQUOTE", "JSON_VALID",

    // Dynamic column functions
    "COLUMN_ADD", "COLUMN_CHECK", "COLUMN_CREATE", "COLUMN_DELETE", "COLUMN_EXISTS",
    "COLUMN_GET", "COLUMN_JSON", "COLUMN_LIST",

    // Other functions
    "AES_DECRYPT", "AES_ENCRYPT", "COMPRESS", "DECODE", "DES_DECRYPT", "DES_ENCRYPT",
    "ENCODE", "ENCRYPT", "MD5", "OLD_PASSWORD", "PASSWORD", "SHA", "SHA1", "SHA2",
    "UNCOMPRESS", "UNCOMPRESSED_LENGTH", "UUID", "UUID_SHORT", "BENCHMARK", "SLEEP"
};

const QStringList MariaDBSyntaxPatterns::OPERATORS = {
    // Comparison operators
    "=", ">", "<", ">=", "<=", "<>", "!=", "!<", "!>",

    // Arithmetic operators
    "+", "-", "*", "/", "%", "DIV", "MOD",

    // Logical operators
    "AND", "&&", "OR", "||", "NOT", "!", "XOR",

    // Bit operators
    "&", "|", "^", "~", "<<", ">>",

    // String operators
    "LIKE", "NOT LIKE", "REGEXP", "NOT REGEXP", "RLIKE", "NOT RLIKE",

    // Set operators
    "UNION", "UNION ALL", "INTERSECT", "EXCEPT", "MINUS",

    // Assignment operator
    ":=",

    // Special operators
    ".", "->", "->>", "IN", "NOT IN", "BETWEEN", "NOT BETWEEN", "IS", "IS NOT",
    "NULL", "NOT NULL", "EXISTS", "NOT EXISTS"
};

const QString MSSQLSyntaxPatterns::SINGLE_LINE_COMMENT = "--";
const QString MSSQLSyntaxPatterns::MULTI_LINE_COMMENT_START = "/*";
const QString MSSQLSyntaxPatterns::MULTI_LINE_COMMENT_END = "*/";
const QString MSSQLSyntaxPatterns::STRING_LITERAL = "'[^']*'";
const QString MSSQLSyntaxPatterns::IDENTIFIER = "[a-zA-Z_][a-zA-Z0-9_]*";
const QString MSSQLSyntaxPatterns::BRACKETED_IDENTIFIER = "`[^`]*`";
const QString MSSQLSyntaxPatterns::NUMBER_LITERAL = "\\b\\d+\\.?\\d*\\b";
const QString MSSQLSyntaxPatterns::VARIABLE = "@[a-zA-Z_][a-zA-Z0-9_]*";
const QString MSSQLSyntaxPatterns::TEMP_TABLE = "#[a-zA-Z_][a-zA-Z0-9_]*";
const QString MSSQLSyntaxPatterns::SYSTEM_OBJECT = "[a-zA-Z_][a-zA-Z0-9_]*\\.[a-zA-Z_][a-zA-Z0-9_]*";

QStringList MariaDBSyntaxPatterns::getAllKeywords() {
    QStringList allKeywords;
    allKeywords << RESERVED_KEYWORDS << NON_RESERVED_KEYWORDS;
    return allKeywords;
}

QList<MariaDBSyntaxElement> MariaDBSyntaxPatterns::getAllSyntaxElements() {
    QList<MariaDBSyntaxElement> elements;

    // Add keywords
    for (const QString& keyword : RESERVED_KEYWORDS) {
        elements << MariaDBSyntaxElement(keyword, "\\b" + keyword + "\\b", "Reserved keyword", true, false, false, false);
    }

    for (const QString& keyword : NON_RESERVED_KEYWORDS) {
        elements << MariaDBSyntaxElement(keyword, "\\b" + keyword + "\\b", "Non-reserved keyword", true, false, false, false);
    }

    // Add data types
    for (const QString& dataType : DATA_TYPES) {
        elements << MariaDBSyntaxElement(dataType, "\\b" + dataType + "\\b", "Data type", false, false, false, true);
    }

    // Add functions
    for (const QString& function : BUILTIN_FUNCTIONS) {
        elements << MariaDBSyntaxElement(function, "\\b" + function + "\\b", "Built-in function", false, true, false, false);
    }

    // Add operators
    for (const QString& op : OPERATORS) {
        elements << MariaDBSyntaxElement(op, "\\b" + QRegularExpression::escape(op) + "\\b", "Operator", false, false, true, false);
    }

    // Add other patterns
    elements << MariaDBSyntaxElement("Single-line comment", "--.*", "Single-line comment");
    elements << MariaDBSyntaxElement("Multi-line comment", "/\\*.*?\\*/", "Multi-line comment");
    elements << MariaDBSyntaxElement("String literal", "'[^']*'", "String literal");
    elements << MariaDBSyntaxElement("Identifier", IDENTIFIER, "Regular identifier");
    elements << MariaDBSyntaxElement("Backtick identifier", "`[^`]*`", "Backtick identifier");
    elements << MariaDBSyntaxElement("Number", NUMBER_LITERAL, "Numeric literal");
    elements << MariaDBSyntaxElement("Variable", "@[a-zA-Z_][a-zA-Z0-9_]*", "User variable");
    elements << MariaDBSyntaxElement("System variable", "@@([a-zA-Z_][a-zA-Z0-9_]*|\\w+)", "System variable");
    elements << MariaDBSyntaxElement("Temporary table", "#[a-zA-Z_][a-zA-Z0-9_]*", "Temporary table");

    return elements;
}

// MariaDB Parser Implementation

QList<MariaDBSyntaxElement> MariaDBParser::parseSQL(const QString& sql) {
    QList<MariaDBSyntaxElement> elements;
    QList<MariaDBSyntaxElement> patterns = MariaDBSyntaxPatterns::getAllSyntaxElements();

    for (const MariaDBSyntaxElement& pattern : patterns) {
        QRegularExpression regex(pattern.pattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = regex.globalMatch(sql);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            MariaDBSyntaxElement element = pattern;
            element.name = match.captured();
            elements << element;
        }
    }

    return elements;
}

bool MariaDBParser::validateSQLSyntax(const QString& sql, QStringList& errors, QStringList& warnings) {
    errors.clear();
    warnings.clear();

    // Basic validation using MariaDBSyntaxValidator
    return MariaDBSyntaxValidator::validateSyntax(sql, errors, warnings);
}

QStringList MariaDBParser::extractTableNames(const QString& sql) {
    QStringList tableNames;
    QRegularExpression tableRegex("\\bFROM\\s+([`\\w\\.]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = tableRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString tableName = match.captured(1);
        if (!tableNames.contains(tableName)) {
            tableNames << tableName;
        }
    }

    // Also check JOIN clauses
    QRegularExpression joinRegex("\\bJOIN\\s+([`\\w\\.]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator joinIt = joinRegex.globalMatch(sql);

    while (joinIt.hasNext()) {
        QRegularExpressionMatch match = joinIt.next();
        QString tableName = match.captured(1);
        if (!tableNames.contains(tableName)) {
            tableNames << tableName;
        }
    }

    return tableNames;
}

QStringList MariaDBParser::extractColumnNames(const QString& sql) {
    QStringList columnNames;

    // Extract from SELECT clause
    QRegularExpression selectRegex("\\bSELECT\\s+(.*?)\\s+FROM\\s+", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch selectMatch = selectRegex.match(sql);

    if (selectMatch.hasMatch()) {
        QString selectClause = selectMatch.captured(1);
        QRegularExpression columnRegex("([`\\w\\.]+)");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(selectClause);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString columnName = match.captured(1);
            if (!columnName.contains("SELECT") && !columnName.contains("FROM") && !columnNames.contains(columnName)) {
                columnNames << columnName;
            }
        }
    }

    return columnNames;
}

QStringList MariaDBParser::extractFunctionNames(const QString& sql) {
    QStringList functionNames;
    QStringList functions = MariaDBSyntaxPatterns::BUILTIN_FUNCTIONS;

    for (const QString& function : functions) {
        if (sql.contains(function, Qt::CaseInsensitive)) {
            functionNames << function;
        }
    }

    return functionNames;
}

QStringList MariaDBParser::extractVariableNames(const QString& sql) {
    QStringList variableNames;
    QRegularExpression varRegex("@[a-zA-Z_][a-zA-Z0-9_]*");
    QRegularExpressionMatchIterator it = varRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString variableName = match.captured();
        if (!variableNames.contains(variableName)) {
            variableNames << variableName;
        }
    }

    return variableNames;
}

QString MariaDBParser::formatSQL(const QString& sql) {
    return MariaDBCodeFormatter::formatCode(sql);
}

QStringList MariaDBParser::getCompletionSuggestions(const QString& partialText, const QString& context) {
    return MariaDBIntelliSense::getCompletions(partialText, partialText.length());
}

bool MariaDBParser::needsQuoting(const QString& identifier) {
    // Check if identifier contains special characters or is a reserved word
    QRegularExpression specialCharRegex("[^a-zA-Z0-9_]");
    return specialCharRegex.match(identifier).hasMatch() ||
           MariaDBSyntaxPatterns::RESERVED_KEYWORDS.contains(identifier, Qt::CaseInsensitive) ||
           MariaDBSyntaxPatterns::NON_RESERVED_KEYWORDS.contains(identifier, Qt::CaseInsensitive);
}

QString MariaDBParser::escapeIdentifier(const QString& identifier) {
    if (needsQuoting(identifier)) {
        return "`" + identifier + "`";
    }
    return identifier;
}

bool MariaDBParser::parseCreateTable(const QString& sql, QString& tableName, QStringList& columns, QString& engine) {
    QRegularExpression createTableRegex("\\bCREATE\\s+TABLE\\s+([`\\w\\.]+)\\s*\\((.*)\\)",
                                       QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = createTableRegex.match(sql);

    if (match.hasMatch()) {
        tableName = match.captured(1);
        QString columnsDef = match.captured(2);

        // Parse column definitions (simplified)
        QRegularExpression columnRegex("([`\\w]+)\\s+([\\w\\[\\]]+)[^,]*");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(columnsDef);

        while (it.hasNext()) {
            QRegularExpressionMatch columnMatch = it.next();
            columns << columnMatch.captured(1) + " " + columnMatch.captured(2);
        }

        // Extract engine if specified
        QRegularExpression engineRegex("\\bENGINE\\s*=\\s*(\\w+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch engineMatch = engineRegex.match(sql);
        if (engineMatch.hasMatch()) {
            engine = engineMatch.captured(1);
        }

        return true;
    }

    return false;
}

bool MariaDBParser::parseCreateIndex(const QString& sql, QString& indexName, QString& tableName, QStringList& columns) {
    QRegularExpression createIndexRegex("\\bCREATE\\s+(?:UNIQUE\\s+)?(?:FULLTEXT\\s+|SPATIAL\\s+)?INDEX\\s+([`\\w]+)\\s+ON\\s+([`\\w\\.]+)\\s*\\(([^)]+)\\)",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = createIndexRegex.match(sql);

    if (match.hasMatch()) {
        indexName = match.captured(1);
        tableName = match.captured(2);
        QString columnsDef = match.captured(3);

        QRegularExpression columnRegex("([`\\w]+)");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(columnsDef);

        while (it.hasNext()) {
            QRegularExpressionMatch columnMatch = it.next();
            columns << columnMatch.captured(1);
        }

        return true;
    }

    return false;
}

bool MariaDBParser::parseCreateView(const QString& sql, QString& viewName, QString& definition) {
    QRegularExpression createViewRegex("\\bCREATE\\s+VIEW\\s+([`\\w\\.]+)\\s+AS\\s+(.*)",
                                      QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = createViewRegex.match(sql);

    if (match.hasMatch()) {
        viewName = match.captured(1);
        definition = match.captured(2).trimmed();
        return true;
    }

    return false;
}

bool MariaDBParser::parseCreateSequence(const QString& sql, QString& sequenceName, QString& options) {
    QRegularExpression createSequenceRegex("\\bCREATE\\s+SEQUENCE\\s+([`\\w\\.]+)\\s*(.*)",
                                          QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = createSequenceRegex.match(sql);

    if (match.hasMatch()) {
        sequenceName = match.captured(1);
        options = match.captured(2).trimmed();
        return true;
    }

    return false;
}

bool MariaDBParser::parseSelectStatement(const QString& sql, QStringList& columns, QStringList& tables, QString& whereClause) {
    // Extract columns
    columns = extractColumnNames(sql);

    // Extract tables
    tables = extractTableNames(sql);

    // Extract WHERE clause
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|\\bHAVING\\b|\\bLIMIT\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        whereClause = whereMatch.captured(1).trimmed();
    }

    return !columns.isEmpty() || !tables.isEmpty();
}

// MariaDB Query Analyzer Implementation

void MariaDBQueryAnalyzer::analyzeQuery(const QString& sql, QStringList& issues, QStringList& suggestions) {
    issues.clear();
    suggestions.clear();

    // Check for common issues
    if (hasSelectStar(sql)) {
        issues << "Using SELECT * is not recommended for production code";
        suggestions << "Specify explicit column names in SELECT clause";
    }

    if (hasCartesianProduct(sql)) {
        issues << "Query may produce Cartesian product";
        suggestions << "Verify JOIN conditions are correct";
    }

    if (hasImplicitConversion(sql)) {
        issues << "Implicit data type conversion may cause performance issues";
        suggestions << "Use explicit CAST/CONVERT functions";
    }

    if (usesFunctionsInWhere(sql)) {
        issues << "Using functions in WHERE clause may prevent index usage";
        suggestions << "Avoid functions on indexed columns in WHERE clause";
    }

    if (hasSuboptimalLike(sql)) {
        issues << "LIKE pattern without wildcard at start may still be slow";
        suggestions << "Consider full-text search for text pattern matching";
    }

    if (hasNonOptimalEngine(sql)) {
        issues << "Table may be using non-optimal storage engine";
        suggestions << "Consider using InnoDB for transactional tables";
    }

    if (hasMissingEngineOptions(sql)) {
        issues << "Storage engine options may be missing";
        suggestions << "Specify appropriate engine options for better performance";
    }
}

int MariaDBQueryAnalyzer::estimateComplexity(const QString& sql) {
    int complexity = 1;

    // Count keywords that indicate complexity
    QStringList complexityKeywords = {"JOIN", "UNION", "GROUP BY", "ORDER BY", "HAVING", "DISTINCT", "EXISTS", "IN", "NOT IN"};
    for (const QString& keyword : complexityKeywords) {
        QRegularExpression regex("\\b" + keyword + "\\b", QRegularExpression::CaseInsensitiveOption);
        complexity += regex.globalMatch(sql).size();
    }

    // Count subqueries
    QRegularExpression subqueryRegex("\\(\\s*SELECT\\s+", QRegularExpression::CaseInsensitiveOption);
    complexity += subqueryRegex.globalMatch(sql).size() * 2;

    // Count CTEs
    QRegularExpression cteRegex("\\bWITH\\s+\\w+\\s+AS\\s*\\(", QRegularExpression::CaseInsensitiveOption);
    complexity += cteRegex.globalMatch(sql).size() * 3;

    // Count window functions
    QRegularExpression windowRegex("\\bOVER\\s*\\(", QRegularExpression::CaseInsensitiveOption);
    complexity += windowRegex.globalMatch(sql).size() * 2;

    return complexity;
}

QStringList MariaDBQueryAnalyzer::checkBestPractices(const QString& sql) {
    QStringList suggestions;

    // Check for SELECT * usage
    if (hasSelectStar(sql)) {
        suggestions << "Avoid using SELECT * in production code";
    }

    // Check for implicit transactions
    if (sql.contains("START TRANSACTION", Qt::CaseInsensitive) && !sql.contains("COMMIT", Qt::CaseInsensitive)) {
        suggestions << "Ensure all transactions are properly committed or rolled back";
    }

    // Check for proper storage engine
    if (hasNonOptimalEngine(sql)) {
        suggestions << "Use InnoDB for transactional tables, MyISAM for read-heavy non-transactional tables";
    }

    // Check for missing indexes in WHERE clause
    if (sql.contains("WHERE", Qt::CaseInsensitive) && !sql.contains("INDEX", Qt::CaseInsensitive)) {
        suggestions << "Consider adding indexes for frequently queried columns";
    }

    return suggestions;
}

QStringList MariaDBQueryAnalyzer::suggestIndexes(const QString& sql) {
    QStringList suggestions;

    // Analyze WHERE clause for potential indexes
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|\\bHAVING\\b|\\bLIMIT\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        QString whereClause = whereMatch.captured(1);
        QStringList conditions = whereClause.split(QRegularExpression("\\b(AND|OR)\\b"), Qt::SkipEmptyParts);

        for (const QString& condition : conditions) {
            if (condition.contains("=") || condition.contains("LIKE") || condition.contains("BETWEEN")) {
                QRegularExpression columnRegex("([`\\w\\.]+)\\s*[=<>]");
                QRegularExpressionMatch columnMatch = columnRegex.match(condition);
                if (columnMatch.hasMatch()) {
                    QString column = columnMatch.captured(1);
                    suggestions << QString("Consider creating an index on column: %1").arg(column);
                }
            }
        }
    }

    return suggestions;
}

QStringList MariaDBQueryAnalyzer::checkSecurityIssues(const QString& sql) {
    QStringList issues;

    // Check for SQL injection vulnerabilities
    if (sql.contains("EXEC", Qt::CaseInsensitive) || sql.contains("EXECUTE", Qt::CaseInsensitive)) {
        issues << "Dynamic SQL execution detected - ensure proper parameterization";
    }

    // Check for system table access without proper filtering
    if (sql.contains("information_schema", Qt::CaseInsensitive) && !sql.contains("WHERE", Qt::CaseInsensitive)) {
        issues << "Accessing system tables without WHERE clause may expose sensitive information";
    }

    // Check for LOAD DATA LOCAL INFILE usage
    if (sql.contains("LOAD DATA LOCAL INFILE", Qt::CaseInsensitive)) {
        issues << "LOAD DATA LOCAL INFILE detected - ensure proper security controls";
    }

    return issues;
}

QStringList MariaDBQueryAnalyzer::checkMariaDBSpecificIssues(const QString& sql) {
    QStringList issues;

    // Check for MyISAM usage on transactional tables
    if (sql.contains("ENGINE=MyISAM", Qt::CaseInsensitive) && sql.contains("FOREIGN KEY", Qt::CaseInsensitive)) {
        issues << "MyISAM engine does not support foreign keys - use InnoDB instead";
    }

    // Check for missing charset specification
    if (sql.contains("CREATE TABLE", Qt::CaseInsensitive) && !sql.contains("CHARSET", Qt::CaseInsensitive)) {
        issues << "Missing character set specification in CREATE TABLE";
    }

    // Check for Aria engine usage without proper options
    if (sql.contains("ENGINE=Aria", Qt::CaseInsensitive) && !sql.contains("PAGE_CHECKSUM", Qt::CaseInsensitive)) {
        issues << "Consider enabling PAGE_CHECKSUM for Aria tables";
    }

    return issues;
}

QStringList MariaDBQueryAnalyzer::suggestMariaDBOptimizations(const QString& sql) {
    QStringList suggestions;

    // Suggest Aria engine for temporary tables
    if (sql.contains("TEMPORARY TABLE", Qt::CaseInsensitive) && !sql.contains("ENGINE", Qt::CaseInsensitive)) {
        suggestions << "Use Aria engine for temporary tables: ENGINE=Aria";
    }

    // Suggest compression for large tables
    if (sql.contains("ENGINE=InnoDB", Qt::CaseInsensitive) && sql.contains("TEXT", Qt::CaseInsensitive)) {
        suggestions << "Consider using ROW_FORMAT=COMPRESSED for large InnoDB tables with TEXT/BLOB columns";
    }

    // Suggest partitioning for large tables
    if (sql.contains("CREATE TABLE", Qt::CaseInsensitive) && sql.contains("BIGINT", Qt::CaseInsensitive)) {
        suggestions << "Consider partitioning large tables based on date or ID ranges";
    }

    return suggestions;
}

bool MariaDBQueryAnalyzer::hasSelectStar(const QString& sql) {
    QRegularExpression selectStarRegex("\\bSELECT\\s+\\*", QRegularExpression::CaseInsensitiveOption);
    return selectStarRegex.match(sql).hasMatch();
}

bool MariaDBQueryAnalyzer::hasImplicitConversion(const QString& sql) {
    // Look for patterns that might cause implicit conversions
    return sql.contains("varchar", Qt::CaseInsensitive) && sql.contains("int", Qt::CaseInsensitive);
}

bool MariaDBQueryAnalyzer::hasCartesianProduct(const QString& sql) {
    QRegularExpression fromRegex("\\bFROM\\s+([`\\w\\.]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression joinRegex("\\bJOIN\\s+", QRegularExpression::CaseInsensitiveOption);

    int tableCount = fromRegex.globalMatch(sql).size();
    int joinCount = joinRegex.globalMatch(sql).size();

    return tableCount > 1 && (tableCount - 1) > joinCount;
}

bool MariaDBQueryAnalyzer::usesFunctionsInWhere(const QString& sql) {
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|\\bHAVING\\b|\\bLIMIT\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        QString whereClause = whereMatch.captured(1);
        QStringList functions = MariaDBSyntaxPatterns::BUILTIN_FUNCTIONS;

        for (const QString& function : functions) {
            if (whereClause.contains(function, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }

    return false;
}

bool MariaDBQueryAnalyzer::hasSuboptimalLike(const QString& sql) {
    QRegularExpression likeRegex("\\bLIKE\\s+['\"][^%]", QRegularExpression::CaseInsensitiveOption);
    return likeRegex.match(sql).hasMatch();
}

bool MariaDBQueryAnalyzer::hasNonOptimalEngine(const QString& sql) {
    // Check for MyISAM with foreign keys (not supported)
    if (sql.contains("ENGINE=MyISAM", Qt::CaseInsensitive) && sql.contains("FOREIGN KEY", Qt::CaseInsensitive)) {
        return true;
    }

    // Check for InnoDB without proper configuration
    if (sql.contains("ENGINE=InnoDB", Qt::CaseInsensitive) && !sql.contains("ROW_FORMAT", Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}

bool MariaDBQueryAnalyzer::hasMissingEngineOptions(const QString& sql) {
    // Check for missing important engine options
    if (sql.contains("ENGINE=", Qt::CaseInsensitive) && !sql.contains("DEFAULT CHARSET", Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}

} // namespace scratchrobin
