#include "constraint/constraint_manager.h"
#include "metadata/metadata_manager.h"
#include "execution/sql_executor.h"
#include "core/connection_manager.h"
#include "index/index_manager.h"
#include "table/table_designer.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <random>
#include <chrono>
#include "utils/string_utils.h"

namespace scratchrobin {

// Helper functions


ConstraintType stringToConstraintType(const std::string& str) {
    if (str == "PRIMARY KEY" || str == "p") return ConstraintType::PRIMARY_KEY;
    if (str == "FOREIGN KEY" || str == "f") return ConstraintType::FOREIGN_KEY;
    if (str == "UNIQUE" || str == "u") return ConstraintType::UNIQUE;
    if (str == "CHECK" || str == "c") return ConstraintType::CHECK;
    if (str == "NOT NULL" || str == "n") return ConstraintType::NOT_NULL;
    if (str == "DEFAULT" || str == "d") return ConstraintType::DEFAULT;
    if (str == "EXCLUDE" || str == "x") return ConstraintType::EXCLUDE;
    if (str == "DOMAIN") return ConstraintType::DOMAIN;
    return ConstraintType::CHECK;
}

std::string constraintStatusToString(ConstraintStatus status) {
    switch (status) {
        case ConstraintStatus::VALID: return "VALID";
        case ConstraintStatus::INVALID: return "INVALID";
        case ConstraintStatus::ENFORCED: return "ENFORCED";
        case ConstraintStatus::NOT_ENFORCED: return "NOT_ENFORCED";
        case ConstraintStatus::DEFERRED: return "DEFERRED";
        case ConstraintStatus::CHECKING: return "CHECKING";
        default: return "UNKNOWN";
    }
}

ConstraintStatus stringToConstraintStatus(const std::string& str) {
    if (str == "INVALID") return ConstraintStatus::INVALID;
    if (str == "ENFORCED") return ConstraintStatus::ENFORCED;
    if (str == "NOT_ENFORCED") return ConstraintStatus::NOT_ENFORCED;
    if (str == "DEFERRED") return ConstraintStatus::DEFERRED;
    if (str == "CHECKING") return ConstraintStatus::CHECKING;
    return ConstraintStatus::VALID;
}

std::string deferrableToString(ConstraintDeferrable deferrable) {
    switch (deferrable) {
        case ConstraintDeferrable::NOT_DEFERRABLE: return "NOT DEFERRABLE";
        case ConstraintDeferrable::DEFERRABLE_INITIALLY_IMMEDIATE: return "DEFERRABLE INITIALLY IMMEDIATE";
        case ConstraintDeferrable::DEFERRABLE_INITIALLY_DEFERRED: return "DEFERRABLE INITIALLY DEFERRED";
        default: return "NOT DEFERRABLE";
    }
}

std::string foreignKeyActionToString(ForeignKeyAction action) {
    switch (action) {
        case ForeignKeyAction::NO_ACTION: return "NO ACTION";
        case ForeignKeyAction::RESTRICT: return "RESTRICT";
        case ForeignKeyAction::CASCADE: return "CASCADE";
        case ForeignKeyAction::SET_NULL: return "SET NULL";
        case ForeignKeyAction::SET_DEFAULT: return "SET DEFAULT";
        default: return "NO ACTION";
    }
}

std::string operationToString(ConstraintOperation op) {
    switch (op) {
        case ConstraintOperation::CREATE: return "CREATE";
        case ConstraintOperation::DROP: return "DROP";
        case ConstraintOperation::ENABLE: return "ENABLE";
        case ConstraintOperation::DISABLE: return "DISABLE";
        case ConstraintOperation::VALIDATE: return "VALIDATE";
        case ConstraintOperation::DEFER: return "DEFER";
        case ConstraintOperation::IMMEDIATE: return "IMMEDIATE";
        default: return "UNKNOWN";
    }
}

std::string generateConstraintId() {
    static std::atomic<int> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "constraint_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
}



class ConstraintManager::Impl {
public:
    Impl(ConstraintManager* parent) : parent_(parent) {}

    void initialize() {
        // Initialize constraint templates
        initializeSystemTemplates();
    }

    std::vector<ConstraintDefinition> getTableConstraints(const std::string& tableName, const std::string& connectionId) {
        std::vector<ConstraintDefinition> constraints;

        if (!sqlExecutor_) {
            return constraints;
        }

        try {
            // Get primary key constraints
            std::string pkQuery = R"(
                SELECT
                    conname as constraint_name,
                    'PRIMARY KEY' as constraint_type,
                    pg_get_constraintdef(c.oid) as definition,
                    obj_description(c.oid, 'pg_constraint') as comment,
                    c.condeferrable,
                    c.condeferred
                FROM pg_constraint c
                JOIN pg_class t ON t.oid = c.conrelid
                JOIN pg_namespace n ON n.oid = t.relnamespace
                WHERE t.relname = ')" + tableName + R"('
                AND c.contype = 'p'
                ORDER BY constraint_name;
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult pkResult = sqlExecutor_->executeQuery(pkQuery, context);

            if (pkResult.success && !pkResult.rows.empty()) {
                for (const auto& row : pkResult.rows) {
                    ConstraintDefinition constraint;
                    constraint.name = row[0].toString().toStdString();
                    constraint.type = ConstraintType::PRIMARY_KEY;
                    constraint.tableName = tableName;
                    constraint.definition = row[2].toString().toStdString();
                    constraint.comment = row[3].toString().toStdString();
                    constraint.createdAt = QDateTime::currentDateTime();

                    PrimaryKeyConstraint pk;
                    pk.name = constraint.name;
                    pk.tableName = tableName;
                    pk.comment = constraint.comment;
                    pk.isDeferrable = row[4].toBool();
                    pk.deferrableMode = pk.isDeferrable ?
                        (row[5].toBool() ? ConstraintDeferrable::DEFERRABLE_INITIALLY_DEFERRED :
                                           ConstraintDeferrable::DEFERRABLE_INITIALLY_IMMEDIATE) :
                        ConstraintDeferrable::NOT_DEFERRABLE;

                    constraint.constraintData = pk;
                    constraints.push_back(constraint);
                }
            }

            // Get foreign key constraints
            std::string fkQuery = R"(
                SELECT
                    conname as constraint_name,
                    'FOREIGN KEY' as constraint_type,
                    pg_get_constraintdef(c.oid) as definition,
                    obj_description(c.oid, 'pg_constraint') as comment,
                    c.condeferrable,
                    c.condeferred
                FROM pg_constraint c
                JOIN pg_class t ON t.oid = c.conrelid
                JOIN pg_namespace n ON n.oid = t.relnamespace
                WHERE t.relname = ')" + tableName + R"('
                AND c.contype = 'f'
                ORDER BY constraint_name;
            )";

            QueryResult fkResult = sqlExecutor_->executeQuery(fkQuery, context);

            if (fkResult.success && !fkResult.rows.empty()) {
                for (const auto& row : fkResult.rows) {
                    ConstraintDefinition constraint;
                    constraint.name = row[0].toString().toStdString();
                    constraint.type = ConstraintType::FOREIGN_KEY;
                    constraint.tableName = tableName;
                    constraint.definition = row[2].toString().toStdString();
                    constraint.comment = row[3].toString().toStdString();
                    constraint.createdAt = QDateTime::currentDateTime();

                    ForeignKeyConstraint fk;
                    fk.name = constraint.name;
                    fk.tableName = tableName;
                    fk.comment = constraint.comment;
                    fk.isDeferrable = row[4].toBool();
                    fk.deferrableMode = fk.isDeferrable ?
                        (row[5].toBool() ? ConstraintDeferrable::DEFERRABLE_INITIALLY_DEFERRED :
                                           ConstraintDeferrable::DEFERRABLE_INITIALLY_IMMEDIATE) :
                        ConstraintDeferrable::NOT_DEFERRABLE;

                    // Parse foreign key definition to extract referenced table and columns
                    parseForeignKeyDefinition(constraint.definition, fk);

                    constraint.constraintData = fk;
                    constraints.push_back(constraint);
                }
            }

            // Get unique constraints
            std::string uniqueQuery = R"(
                SELECT
                    conname as constraint_name,
                    'UNIQUE' as constraint_type,
                    pg_get_constraintdef(c.oid) as definition,
                    obj_description(c.oid, 'pg_constraint') as comment,
                    c.condeferrable,
                    c.condeferred
                FROM pg_constraint c
                JOIN pg_class t ON t.oid = c.conrelid
                JOIN pg_namespace n ON n.oid = t.relnamespace
                WHERE t.relname = ')" + tableName + R"('
                AND c.contype = 'u'
                ORDER BY constraint_name;
            )";

            QueryResult uniqueResult = sqlExecutor_->executeQuery(uniqueQuery, context);

            if (uniqueResult.success && !uniqueResult.rows.empty()) {
                for (const auto& row : uniqueResult.rows) {
                    ConstraintDefinition constraint;
                    constraint.name = row[0].toString().toStdString();
                    constraint.type = ConstraintType::UNIQUE;
                    constraint.tableName = tableName;
                    constraint.definition = row[2].toString().toStdString();
                    constraint.comment = row[3].toString().toStdString();
                    constraint.createdAt = QDateTime::currentDateTime();

                    UniqueConstraint unique;
                    unique.name = constraint.name;
                    unique.tableName = tableName;
                    unique.comment = constraint.comment;
                    unique.isDeferrable = row[4].toBool();
                    unique.deferrableMode = unique.isDeferrable ?
                        (row[5].toBool() ? ConstraintDeferrable::DEFERRABLE_INITIALLY_DEFERRED :
                                           ConstraintDeferrable::DEFERRABLE_INITIALLY_IMMEDIATE) :
                        ConstraintDeferrable::NOT_DEFERRABLE;

                    constraint.constraintData = unique;
                    constraints.push_back(constraint);
                }
            }

            // Get check constraints
            std::string checkQuery = R"(
                SELECT
                    conname as constraint_name,
                    'CHECK' as constraint_type,
                    pg_get_constraintdef(c.oid) as definition,
                    obj_description(c.oid, 'pg_constraint') as comment,
                    c.condeferrable,
                    c.condeferred
                FROM pg_constraint c
                JOIN pg_class t ON t.oid = c.conrelid
                JOIN pg_namespace n ON n.oid = t.relnamespace
                WHERE t.relname = ')" + tableName + R"('
                AND c.contype = 'c'
                ORDER BY constraint_name;
            )";

            QueryResult checkResult = sqlExecutor_->executeQuery(checkQuery, context);

            if (checkResult.success && !checkResult.rows.empty()) {
                for (const auto& row : checkResult.rows) {
                    ConstraintDefinition constraint;
                    constraint.name = row[0].toString().toStdString();
                    constraint.type = ConstraintType::CHECK;
                    constraint.tableName = tableName;
                    constraint.definition = row[2].toString().toStdString();
                    constraint.comment = row[3].toString().toStdString();
                    constraint.createdAt = QDateTime::currentDateTime();

                    CheckConstraint check;
                    check.name = constraint.name;
                    check.tableName = tableName;
                    check.comment = constraint.comment;
                    check.isDeferrable = row[4].toBool();
                    check.deferrableMode = check.isDeferrable ?
                        (row[5].toBool() ? ConstraintDeferrable::DEFERRABLE_INITIALLY_DEFERRED :
                                           ConstraintDeferrable::DEFERRABLE_INITIALLY_IMMEDIATE) :
                        ConstraintDeferrable::NOT_DEFERRABLE;

                    // Extract expression from definition
                    std::regex exprRegex(R"(CHECK\s*\((.*)\))", std::regex::icase);
                    std::smatch exprMatch;
                    if (std::regex_search(constraint.definition, exprMatch, exprRegex)) {
                        check.expression = exprMatch[1].str();
                    }

                    constraint.constraintData = check;
                    constraints.push_back(constraint);
                }
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to get table constraints:" << e.what();
        }

        return constraints;
    }

    ConstraintDefinition getConstraintDefinition(const std::string& constraintName, const std::string& connectionId) {
        ConstraintDefinition definition;

        if (!sqlExecutor_) {
            return definition;
        }

        try {
            std::string query = R"(
                SELECT
                    conname,
                    contype,
                    pg_get_constraintdef(c.oid) as definition,
                    obj_description(c.oid, 'pg_constraint') as comment,
                    t.relname as table_name,
                    c.condeferrable,
                    c.condeferred
                FROM pg_constraint c
                JOIN pg_class t ON t.oid = c.conrelid
                WHERE conname = ')" + constraintName + R"(';
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success && !result.rows.empty()) {
                const auto& row = result.rows[0];
                definition.name = row[0].toString().toStdString();
                definition.type = stringToConstraintType(row[1].toString().toStdString());
                definition.definition = row[2].toString().toStdString();
                definition.comment = row[3].toString().toStdString();
                definition.tableName = row[4].toString().toStdString();
                definition.createdAt = QDateTime::currentDateTime();

                // Parse constraint-specific data
                parseConstraintData(definition, row);
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to get constraint definition:" << e.what();
        }

        return definition;
    }

    std::vector<ConstraintStatistics> getConstraintStatistics(const std::string& constraintName, const std::string& connectionId) {
        std::vector<ConstraintStatistics> statistics;

        if (!sqlExecutor_) {
            return statistics;
        }

        try {
            // This would typically query system catalogs for constraint statistics
            // For now, return basic statistics
            ConstraintStatistics stats;
            stats.constraintName = constraintName;
            stats.collectedAt = QDateTime::currentDateTime();
            stats.status = ConstraintStatus::VALID;
            statistics.push_back(stats);

        } catch (const std::exception& e) {
            qWarning() << "Failed to get constraint statistics:" << e.what();
        }

        return statistics;
    }

    bool createConstraint(const ConstraintDefinition& definition, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = generateConstraintDDL(definition, connectionId);

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000); // 5 minutes for constraint creation

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->constraintCreated(definition.name, connectionId);
                return true;
            } else {
                qWarning() << "Failed to create constraint:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to create constraint:" << e.what();
            return false;
        }
    }

    bool dropConstraint(const std::string& constraintName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "ALTER TABLE " + getTableForConstraint(constraintName, connectionId) +
                             " DROP CONSTRAINT IF EXISTS " + constraintName + ";";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(30000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->constraintDropped(constraintName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to drop constraint:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to drop constraint:" << e.what();
            return false;
        }
    }

    bool enableConstraint(const std::string& constraintName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "ALTER TABLE " + getTableForConstraint(constraintName, connectionId) +
                             " VALIDATE CONSTRAINT " + constraintName + ";";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->constraintModified(constraintName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to enable constraint:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to enable constraint:" << e.what();
            return false;
        }
    }

    bool disableConstraint(const std::string& constraintName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "ALTER TABLE " + getTableForConstraint(constraintName, connectionId) +
                             " DROP CONSTRAINT IF EXISTS " + constraintName + " RESTRICT;";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(30000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->constraintModified(constraintName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to disable constraint:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to disable constraint:" << e.what();
            return false;
        }
    }

    bool validateConstraint(const std::string& constraintName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "ALTER TABLE " + getTableForConstraint(constraintName, connectionId) +
                             " VALIDATE CONSTRAINT " + constraintName + ";";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->constraintModified(constraintName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to validate constraint:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to validate constraint:" << e.what();
            return false;
        }
    }

    bool deferConstraint(const std::string& constraintName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "SET CONSTRAINTS " + constraintName + " DEFERRED;";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->constraintModified(constraintName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to defer constraint:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to defer constraint:" << e.what();
            return false;
        }
    }

    bool immediateConstraint(const std::string& constraintName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "SET CONSTRAINTS " + constraintName + " IMMEDIATE;";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->constraintModified(constraintName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to make constraint immediate:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to make constraint immediate:" << e.what();
            return false;
        }
    }

    std::vector<ConstraintMaintenanceOperation> getMaintenanceHistory(const std::string& constraintName, const std::string& connectionId) {
        // This would require a maintenance log table
        // For now, return empty vector
        return std::vector<ConstraintMaintenanceOperation>();
    }

    ConstraintMaintenanceOperation performMaintenance(const std::string& constraintName, ConstraintOperation operation, const std::string& connectionId) {
        ConstraintMaintenanceOperation maintenanceOp;
        maintenanceOp.operationId = generateOperationId();
        maintenanceOp.constraintName = constraintName;
        maintenanceOp.operation = operation;
        maintenanceOp.startedAt = QDateTime::currentDateTime();

        if (!sqlExecutor_) {
            maintenanceOp.success = false;
            maintenanceOp.errorMessage = "SQL executor not available";
            maintenanceOp.completedAt = QDateTime::currentDateTime();
            return maintenanceOp;
        }

        try {
            std::string sqlStatement;

            switch (operation) {
                case ConstraintOperation::VALIDATE:
                    sqlStatement = "ALTER TABLE " + getTableForConstraint(constraintName, connectionId) +
                                 " VALIDATE CONSTRAINT " + constraintName + ";";
                    break;
                case ConstraintOperation::ENABLE:
                    sqlStatement = "ALTER TABLE " + getTableForConstraint(constraintName, connectionId) +
                                 " VALIDATE CONSTRAINT " + constraintName + ";";
                    break;
                case ConstraintOperation::DISABLE:
                    sqlStatement = "ALTER TABLE " + getTableForConstraint(constraintName, connectionId) +
                                 " DROP CONSTRAINT IF EXISTS " + constraintName + " RESTRICT;";
                    break;
                case ConstraintOperation::DEFER:
                    sqlStatement = "SET CONSTRAINTS " + constraintName + " DEFERRED;";
                    break;
                case ConstraintOperation::IMMEDIATE:
                    sqlStatement = "SET CONSTRAINTS " + constraintName + " IMMEDIATE;";
                    break;
                default:
                    maintenanceOp.success = false;
                    maintenanceOp.errorMessage = "Unsupported maintenance operation";
                    maintenanceOp.completedAt = QDateTime::currentDateTime();
                    return maintenanceOp;
            }

            maintenanceOp.sqlStatement = sqlStatement;

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000);

            QueryResult result = sqlExecutor_->executeQuery(sqlStatement, context);

            maintenanceOp.success = result.success;
            if (!result.success) {
                maintenanceOp.errorMessage = result.errorMessage;
            } else {
                maintenanceOp.outputMessage = "Operation completed successfully";
            }

            maintenanceOp.completedAt = QDateTime::currentDateTime();

            if (maintenanceOp.success) {
                parent_->constraintModified(constraintName, connectionId);
            }

            parent_->maintenanceCompleted(maintenanceOp);

        } catch (const std::exception& e) {
            maintenanceOp.success = false;
            maintenanceOp.errorMessage = std::string("Maintenance failed: ") + e.what();
            maintenanceOp.completedAt = QDateTime::currentDateTime();
        }

        return maintenanceOp;
    }

    ConstraintValidationResult validateConstraintDefinition(const ConstraintDefinition& definition, const std::string& connectionId) {
        ConstraintValidationResult result;
        result.isValid = true;

        // Validate constraint name
        if (definition.name.empty()) {
            result.isValid = false;
            result.errors.push_back("Constraint name is required");
        } else if (!std::regex_match(definition.name, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            result.isValid = false;
            result.errors.push_back("Constraint name contains invalid characters");
        }

        // Validate table name
        if (definition.tableName.empty()) {
            result.isValid = false;
            result.errors.push_back("Table name is required");
        }

        // Type-specific validation
        std::visit([&](auto&& constraint) {
            using T = std::decay_t<decltype(constraint)>;
            if constexpr (std::is_same_v<T, PrimaryKeyConstraint>) {
                if (constraint.columns.empty()) {
                    result.isValid = false;
                    result.errors.push_back("Primary key must have at least one column");
                }
            } else if constexpr (std::is_same_v<T, ForeignKeyConstraint>) {
                if (constraint.columnMappings.empty()) {
                    result.isValid = false;
                    result.errors.push_back("Foreign key must have at least one column mapping");
                }
                if (constraint.referencedTable.empty()) {
                    result.isValid = false;
                    result.errors.push_back("Referenced table is required for foreign key");
                }
            } else if constexpr (std::is_same_v<T, CheckConstraint>) {
                if (constraint.expression.empty()) {
                    result.isValid = false;
                    result.errors.push_back("Check constraint must have an expression");
                }
            }
        }, definition.constraintData);

        return result;
    }

    std::vector<ConstraintViolation> checkConstraintViolations(const std::string& constraintName, const std::string& connectionId) {
        std::vector<ConstraintViolation> violations;

        // This would implement constraint violation checking
        // For now, return empty vector
        return violations;
    }

    std::vector<ConstraintDependency> getConstraintDependencies(const std::string& constraintName, const std::string& connectionId) {
        std::vector<ConstraintDependency> dependencies;

        // This would analyze constraint dependencies
        // For now, return empty vector
        return dependencies;
    }

    std::vector<ConstraintTemplate> getAvailableTemplates(const std::string& connectionId) {
        return systemTemplates_;
    }

    ConstraintDefinition applyTemplate(const std::string& templateId, const std::unordered_map<std::string, std::string>& parameters, const std::string& connectionId) {
        ConstraintDefinition definition;

        auto it = std::find_if(systemTemplates_.begin(), systemTemplates_.end(),
                              [&templateId](const ConstraintTemplate& t) { return t.templateId == templateId; });

        if (it != systemTemplates_.end()) {
            std::string ddl = it->templateDefinition;

            // Replace parameters
            for (const auto& param : parameters) {
                std::string placeholder = "${" + param.first + "}";
                size_t pos = ddl.find(placeholder);
                while (pos != std::string::npos) {
                    ddl.replace(pos, placeholder.length(), param.second);
                    pos = ddl.find(placeholder, pos + param.second.length());
                }
            }

            definition.name = "new_" + std::to_string(static_cast<int>(it->type)) + "_constraint";
            definition.type = it->type;
            definition.definition = ddl;
        }

        return definition;
    }

    void saveTemplate(const ConstraintTemplate& template_, const std::string& connectionId) {
        systemTemplates_.push_back(template_);
    }

    ConstraintAnalysisReport analyzeTableConstraints(const std::string& tableName, const std::string& connectionId) {
        ConstraintAnalysisReport report;
        report.reportId = generateConstraintId();
        report.tableName = tableName;
        report.generatedAt = QDateTime::currentDateTime();
        report.analysisDuration = std::chrono::milliseconds(0);

        auto startTime = std::chrono::high_resolution_clock::now();

        // Get constraints
        report.statistics = getConstraintStatisticsForTable(tableName, connectionId);

        // Calculate summary statistics
        for (const auto& stat : report.statistics) {
            report.totalConstraints++;
            if (stat.status == ConstraintStatus::ENFORCED) report.enabledConstraints++;
            else if (stat.status == ConstraintStatus::NOT_ENFORCED) report.disabledConstraints++;
            else if (stat.status == ConstraintStatus::INVALID) report.invalidConstraints++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        report.analysisDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // Determine overall health
        if (report.invalidConstraints > 0) {
            report.overallHealth = "Critical";
            report.criticalIssues.push_back("Invalid constraints found");
        } else if (report.disabledConstraints > 0) {
            report.overallHealth = "Poor";
            report.criticalIssues.push_back("Disabled constraints found");
        } else if (report.enabledConstraints == report.totalConstraints) {
            report.overallHealth = "Excellent";
        } else {
            report.overallHealth = "Good";
        }

        return report;
    }

    ConstraintAnalysisReport analyzeDatabaseConstraints(const std::string& connectionId) {
        ConstraintAnalysisReport report;
        report.reportId = generateConstraintId();
        report.generatedAt = QDateTime::currentDateTime();
        report.analysisDuration = std::chrono::milliseconds(0);

        // This would analyze all constraints in the database
        // For now, return basic report
        return report;
    }

    std::vector<ConstraintTemplate> getRecommendedTemplates(const std::string& tableName, const std::string& connectionId) {
        std::vector<ConstraintTemplate> recommendations;

        // This would analyze the table structure and suggest appropriate constraint templates
        return recommendations;
    }

    std::string generateConstraintDDL(const ConstraintDefinition& definition, const std::string& connectionId) {
        std::stringstream ddl;

        switch (definition.type) {
            case ConstraintType::PRIMARY_KEY:
                ddl << "ALTER TABLE " << definition.tableName << " ADD CONSTRAINT " << definition.name << " PRIMARY KEY (";
                if (std::holds_alternative<PrimaryKeyConstraint>(definition.constraintData)) {
                    const auto& pk = std::get<PrimaryKeyConstraint>(definition.constraintData);
                    for (size_t i = 0; i < pk.columns.size(); ++i) {
                        ddl << pk.columns[i].columnName;
                        if (i < pk.columns.size() - 1) ddl << ", ";
                    }
                }
                ddl << ")";
                break;

            case ConstraintType::FOREIGN_KEY:
                ddl << "ALTER TABLE " << definition.tableName << " ADD CONSTRAINT " << definition.name << " FOREIGN KEY (";
                if (std::holds_alternative<ForeignKeyConstraint>(definition.constraintData)) {
                    const auto& fk = std::get<ForeignKeyConstraint>(definition.constraintData);
                    for (size_t i = 0; i < fk.columnMappings.size(); ++i) {
                        ddl << fk.columnMappings[i].first;
                        if (i < fk.columnMappings.size() - 1) ddl << ", ";
                    }
                    ddl << ") REFERENCES " << fk.referencedTable << " (";
                    for (size_t i = 0; i < fk.columnMappings.size(); ++i) {
                        ddl << fk.columnMappings[i].second;
                        if (i < fk.columnMappings.size() - 1) ddl << ", ";
                    }
                    ddl << ")";
                    if (fk.onDelete != ForeignKeyAction::NO_ACTION) {
                        ddl << " ON DELETE " << foreignKeyActionToString(fk.onDelete);
                    }
                    if (fk.onUpdate != ForeignKeyAction::NO_ACTION) {
                        ddl << " ON UPDATE " << foreignKeyActionToString(fk.onUpdate);
                    }
                }
                ddl << ")";
                break;

            case ConstraintType::UNIQUE:
                ddl << "ALTER TABLE " << definition.tableName << " ADD CONSTRAINT " << definition.name << " UNIQUE (";
                if (std::holds_alternative<UniqueConstraint>(definition.constraintData)) {
                    const auto& unique = std::get<UniqueConstraint>(definition.constraintData);
                    for (size_t i = 0; i < unique.columns.size(); ++i) {
                        ddl << unique.columns[i].columnName;
                        if (i < unique.columns.size() - 1) ddl << ", ";
                    }
                }
                ddl << ")";
                break;

            case ConstraintType::CHECK:
                ddl << "ALTER TABLE " << definition.tableName << " ADD CONSTRAINT " << definition.name << " CHECK (";
                if (std::holds_alternative<CheckConstraint>(definition.constraintData)) {
                    const auto& check = std::get<CheckConstraint>(definition.constraintData);
                    ddl << check.expression;
                }
                ddl << ")";
                break;

            case ConstraintType::NOT_NULL:
                if (std::holds_alternative<NotNullConstraint>(definition.constraintData)) {
                    const auto& notNull = std::get<NotNullConstraint>(definition.constraintData);
                    ddl << "ALTER TABLE " << definition.tableName << " ALTER COLUMN " << notNull.columnName << " SET NOT NULL";
                }
                break;

            case ConstraintType::DEFAULT:
                if (std::holds_alternative<DefaultConstraint>(definition.constraintData)) {
                    const auto& def = std::get<DefaultConstraint>(definition.constraintData);
                    ddl << "ALTER TABLE " << definition.tableName << " ALTER COLUMN " << def.columnName << " SET DEFAULT " << def.expression;
                }
                break;

            default:
                ddl << "-- Unsupported constraint type: " << constraintTypeToString(definition.type);
                break;
        }

        return ddl.str();
    }

    bool isConstraintNameAvailable(const std::string& name, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string query = R"(
                SELECT COUNT(*) as count
                FROM pg_constraint
                WHERE conname = ')" + name + R"(';
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success && !result.rows.empty()) {
                return result.rows[0][0].toInt() == 0;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to check constraint name availability:" << e.what();
        }

        return false;
    }

    std::vector<std::string> getAvailableConstraintTypes(const std::string& connectionId) {
        return {
            "PRIMARY KEY",
            "FOREIGN KEY",
            "UNIQUE",
            "CHECK",
            "NOT NULL",
            "DEFAULT",
            "EXCLUDE"
        };
    }

    void collectConstraintStatistics(const std::string& constraintName, const std::string& connectionId) {
        // This would collect statistics for a specific constraint
    }

    void collectAllConstraintStatistics(const std::string& connectionId) {
        // This would collect statistics for all constraints
    }

public:
    ConstraintManager* parent_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IIndexManager> indexManager_;
    std::shared_ptr<ITableDesigner> tableDesigner_;

    std::vector<ConstraintTemplate> systemTemplates_;

    void initializeSystemTemplates() {
        // Primary Key Template
        ConstraintTemplate pkTemplate;
        pkTemplate.templateId = "primary_key_template";
        pkTemplate.name = "Primary Key Constraint";
        pkTemplate.description = "Standard primary key constraint";
        pkTemplate.type = ConstraintType::PRIMARY_KEY;
        pkTemplate.templateDefinition = "ALTER TABLE ${table_name} ADD CONSTRAINT ${constraint_name} PRIMARY KEY (${column_names})";
        pkTemplate.parameters = {"table_name", "constraint_name", "column_names"};
        pkTemplate.applicableDataTypes = {"integer", "bigint", "uuid", "text", "varchar"};
        pkTemplate.category = "Identity";
        pkTemplate.isSystemTemplate = true;
        pkTemplate.createdAt = QDateTime::currentDateTime();
        systemTemplates_.push_back(pkTemplate);

        // Foreign Key Template
        ConstraintTemplate fkTemplate;
        fkTemplate.templateId = "foreign_key_template";
        fkTemplate.name = "Foreign Key Constraint";
        fkTemplate.description = "Standard foreign key constraint with cascade delete";
        fkTemplate.type = ConstraintType::FOREIGN_KEY;
        fkTemplate.templateDefinition = "ALTER TABLE ${table_name} ADD CONSTRAINT ${constraint_name} FOREIGN KEY (${local_columns}) REFERENCES ${referenced_table} (${referenced_columns}) ON DELETE CASCADE ON UPDATE NO ACTION";
        fkTemplate.parameters = {"table_name", "constraint_name", "local_columns", "referenced_table", "referenced_columns"};
        fkTemplate.applicableDataTypes = {"integer", "bigint", "uuid"};
        fkTemplate.category = "Relationship";
        fkTemplate.isSystemTemplate = true;
        fkTemplate.createdAt = QDateTime::currentDateTime();
        systemTemplates_.push_back(fkTemplate);

        // Unique Constraint Template
        ConstraintTemplate uniqueTemplate;
        uniqueTemplate.templateId = "unique_constraint_template";
        uniqueTemplate.name = "Unique Constraint";
        uniqueTemplate.description = "Unique constraint for business key columns";
        uniqueTemplate.type = ConstraintType::UNIQUE;
        uniqueTemplate.templateDefinition = "ALTER TABLE ${table_name} ADD CONSTRAINT ${constraint_name} UNIQUE (${column_names})";
        uniqueTemplate.parameters = {"table_name", "constraint_name", "column_names"};
        uniqueTemplate.applicableDataTypes = {"text", "varchar", "integer", "bigint", "uuid", "date", "timestamp"};
        uniqueTemplate.category = "Business Rule";
        uniqueTemplate.isSystemTemplate = true;
        uniqueTemplate.createdAt = QDateTime::currentDateTime();
        systemTemplates_.push_back(uniqueTemplate);

        // Check Constraint Template - Email Validation
        ConstraintTemplate emailTemplate;
        emailTemplate.templateId = "email_check_template";
        emailTemplate.name = "Email Validation Check";
        emailTemplate.description = "Check constraint for valid email format";
        emailTemplate.type = ConstraintType::CHECK;
        emailTemplate.templateDefinition = "ALTER TABLE ${table_name} ADD CONSTRAINT ${constraint_name} CHECK (${column_name} ~* '^[A-Za-z0-9._+%-]+@[A-Za-z0-9.-]+[.][A-Za-z]+$')";
        emailTemplate.parameters = {"table_name", "constraint_name", "column_name"};
        emailTemplate.applicableDataTypes = {"text", "varchar"};
        emailTemplate.category = "Data Validation";
        emailTemplate.isSystemTemplate = true;
        emailTemplate.createdAt = QDateTime::currentDateTime();
        systemTemplates_.push_back(emailTemplate);

        // Check Constraint Template - Positive Number
        ConstraintTemplate positiveTemplate;
        positiveTemplate.templateId = "positive_number_check_template";
        positiveTemplate.name = "Positive Number Check";
        positiveTemplate.description = "Check constraint for positive numbers";
        positiveTemplate.type = ConstraintType::CHECK;
        positiveTemplate.templateDefinition = "ALTER TABLE ${table_name} ADD CONSTRAINT ${constraint_name} CHECK (${column_name} > 0)";
        positiveTemplate.parameters = {"table_name", "constraint_name", "column_name"};
        positiveTemplate.applicableDataTypes = {"integer", "bigint", "numeric", "decimal", "real", "double precision"};
        positiveTemplate.category = "Data Validation";
        positiveTemplate.isSystemTemplate = true;
        positiveTemplate.createdAt = QDateTime::currentDateTime();
        systemTemplates_.push_back(positiveTemplate);

        // Not Null Template
        ConstraintTemplate notNullTemplate;
        notNullTemplate.templateId = "not_null_template";
        notNullTemplate.name = "Not Null Constraint";
        notNullTemplate.description = "Not null constraint for required columns";
        notNullTemplate.type = ConstraintType::NOT_NULL;
        notNullTemplate.templateDefinition = "ALTER TABLE ${table_name} ALTER COLUMN ${column_name} SET NOT NULL";
        notNullTemplate.parameters = {"table_name", "column_name"};
        notNullTemplate.applicableDataTypes = {"all"};
        notNullTemplate.category = "Data Integrity";
        notNullTemplate.isSystemTemplate = true;
        notNullTemplate.createdAt = QDateTime::currentDateTime();
        systemTemplates_.push_back(notNullTemplate);

        // Default Value Template
        ConstraintTemplate defaultTemplate;
        defaultTemplate.templateId = "default_value_template";
        defaultTemplate.name = "Default Value Constraint";
        defaultTemplate.description = "Default value constraint for optional columns";
        defaultTemplate.type = ConstraintType::DEFAULT;
        defaultTemplate.templateDefinition = "ALTER TABLE ${table_name} ALTER COLUMN ${column_name} SET DEFAULT ${default_value}";
        defaultTemplate.parameters = {"table_name", "column_name", "default_value"};
        defaultTemplate.applicableDataTypes = {"all"};
        defaultTemplate.category = "Data Integrity";
        defaultTemplate.isSystemTemplate = true;
        defaultTemplate.createdAt = QDateTime::currentDateTime();
        systemTemplates_.push_back(defaultTemplate);
    }

    void parseConstraintData(ConstraintDefinition& definition, const std::vector<QVariant>& row) {
        switch (definition.type) {
            case ConstraintType::PRIMARY_KEY:
                parsePrimaryKeyData(definition, row);
                break;
            case ConstraintType::FOREIGN_KEY:
                parseForeignKeyData(definition, row);
                break;
            case ConstraintType::UNIQUE:
                parseUniqueData(definition, row);
                break;
            case ConstraintType::CHECK:
                parseCheckData(definition, row);
                break;
            default:
                break;
        }
    }

    void parsePrimaryKeyData(ConstraintDefinition& definition, const std::vector<QVariant>& row) {
        PrimaryKeyConstraint pk;
        pk.name = definition.name;
        pk.tableName = definition.tableName;
        pk.comment = definition.comment;

        // Parse columns from definition
        std::regex columnRegex(R"(PRIMARY KEY\s*\(([^)]+)\))", std::regex::icase);
        std::smatch columnMatch;
        if (std::regex_search(definition.definition, columnMatch, columnRegex)) {
            std::string columnsStr = columnMatch[1].str();
            std::stringstream ss(columnsStr);
            std::string column;
            while (std::getline(ss, column, ',')) {
                // Remove whitespace
                column.erase(column.begin(), std::find_if(column.begin(), column.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                column.erase(std::find_if(column.rbegin(), column.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), column.end());

                ConstraintColumn col;
                col.columnName = column;
                pk.columns.push_back(col);
            }
        }

        definition.constraintData = pk;
    }

    void parseForeignKeyData(ConstraintDefinition& definition, const std::vector<QVariant>& row) {
        ForeignKeyConstraint fk;
        fk.name = definition.name;
        fk.tableName = definition.tableName;
        fk.comment = definition.comment;

        parseForeignKeyDefinition(definition.definition, fk);
        definition.constraintData = fk;
    }

    void parseUniqueData(ConstraintDefinition& definition, const std::vector<QVariant>& row) {
        UniqueConstraint unique;
        unique.name = definition.name;
        unique.tableName = definition.tableName;
        unique.comment = definition.comment;

        // Parse columns from definition (similar to primary key)
        std::regex columnRegex(R"(UNIQUE\s*\(([^)]+)\))", std::regex::icase);
        std::smatch columnMatch;
        if (std::regex_search(definition.definition, columnMatch, columnRegex)) {
            std::string columnsStr = columnMatch[1].str();
            std::stringstream ss(columnsStr);
            std::string column;
            while (std::getline(ss, column, ',')) {
                column.erase(column.begin(), std::find_if(column.begin(), column.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                column.erase(std::find_if(column.rbegin(), column.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), column.end());

                ConstraintColumn col;
                col.columnName = column;
                unique.columns.push_back(col);
            }
        }

        definition.constraintData = unique;
    }

    void parseCheckData(ConstraintDefinition& definition, const std::vector<QVariant>& row) {
        CheckConstraint check;
        check.name = definition.name;
        check.tableName = definition.tableName;
        check.comment = definition.comment;

        // Extract expression from definition
        std::regex exprRegex(R"(CHECK\s*\((.*)\))", std::regex::icase);
        std::smatch exprMatch;
        if (std::regex_search(definition.definition, exprMatch, exprRegex)) {
            check.expression = exprMatch[1].str();
        }

        definition.constraintData = check;
    }

    void parseForeignKeyDefinition(const std::string& definition, ForeignKeyConstraint& fk) {
        // Parse foreign key definition to extract referenced table and columns
        std::regex fkRegex(R"(FOREIGN KEY\s*\(([^)]+)\)\s*REFERENCES\s*(\w+)\s*\(([^)]+)\))", std::regex::icase);
        std::smatch fkMatch;
        if (std::regex_search(definition, fkMatch, fkRegex)) {
            std::string localColumnsStr = fkMatch[1].str();
            fk.referencedTable = fkMatch[2].str();
            std::string referencedColumnsStr = fkMatch[3].str();

            // Parse local columns
            std::stringstream ss(localColumnsStr);
            std::string column;
            std::vector<std::string> localColumns;
            while (std::getline(ss, column, ',')) {
                column.erase(column.begin(), std::find_if(column.begin(), column.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                column.erase(std::find_if(column.rbegin(), column.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), column.end());
                localColumns.push_back(column);
            }

            // Parse referenced columns
            std::stringstream ss2(referencedColumnsStr);
            std::vector<std::string> referencedColumns;
            while (std::getline(ss2, column, ',')) {
                column.erase(column.begin(), std::find_if(column.begin(), column.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                column.erase(std::find_if(column.rbegin(), column.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), column.end());
                referencedColumns.push_back(column);
            }

            // Create column mappings
            for (size_t i = 0; i < localColumns.size() && i < referencedColumns.size(); ++i) {
                fk.columnMappings.emplace_back(localColumns[i], referencedColumns[i]);
            }
        }
    }

    std::string getTableForConstraint(const std::string& constraintName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return "";
        }

        try {
            std::string query = R"(
                SELECT t.relname
                FROM pg_constraint c
                JOIN pg_class t ON t.oid = c.conrelid
                WHERE c.conname = ')" + constraintName + R"(';
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success && !result.rows.empty()) {
                return result.rows[0][0].toString().toStdString();
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to get table for constraint:" << e.what();
        }

        return "";
    }

    std::vector<ConstraintStatistics> getConstraintStatisticsForTable(const std::string& tableName, const std::string& connectionId) {
        std::vector<ConstraintStatistics> statistics;

        auto constraints = getTableConstraints(tableName, connectionId);

        for (const auto& constraint : constraints) {
            ConstraintStatistics stat;
            stat.constraintName = constraint.name;
            stat.tableName = constraint.tableName;
            stat.type = constraint.type;
            stat.status = constraint.status;
            stat.collectedAt = QDateTime::currentDateTime();
            statistics.push_back(stat);
        }

        return statistics;
    }
};

// ConstraintManager implementation

ConstraintManager::ConstraintManager(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>(this)) {

    // Set up connections
    connect(this, &ConstraintManager::constraintCreated, [this](const std::string& constraintName, const std::string& connectionId) {
        if (constraintCreatedCallback_) constraintCreatedCallback_(constraintName, connectionId);
    });

    connect(this, &ConstraintManager::constraintDropped, [this](const std::string& constraintName, const std::string& connectionId) {
        if (constraintDroppedCallback_) constraintDroppedCallback_(constraintName, connectionId);
    });

    connect(this, &ConstraintManager::constraintModified, [this](const std::string& constraintName, const std::string& connectionId) {
        if (constraintModifiedCallback_) constraintModifiedCallback_(constraintName, connectionId);
    });

    connect(this, &ConstraintManager::violationDetected, [this](const ConstraintViolation& violation) {
        if (violationDetectedCallback_) violationDetectedCallback_(violation);
    });

    connect(this, &ConstraintManager::maintenanceCompleted, [this](const ConstraintMaintenanceOperation& op) {
        if (maintenanceCompletedCallback_) maintenanceCompletedCallback_(op);
    });
}

ConstraintManager::~ConstraintManager() = default;

void ConstraintManager::initialize() {
    impl_->initialize();
}

void ConstraintManager::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    metadataManager_ = metadataManager;
    impl_->metadataManager_ = metadataManager;
}

void ConstraintManager::setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) {
    sqlExecutor_ = sqlExecutor;
    impl_->sqlExecutor_ = sqlExecutor;
}

void ConstraintManager::setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) {
    connectionManager_ = connectionManager;
    impl_->connectionManager_ = connectionManager;
}

void ConstraintManager::setIndexManager(std::shared_ptr<IIndexManager> indexManager) {
    indexManager_ = indexManager;
    impl_->indexManager_ = indexManager;
}

void ConstraintManager::setTableDesigner(std::shared_ptr<ITableDesigner> tableDesigner) {
    tableDesigner_ = tableDesigner;
    impl_->tableDesigner_ = tableDesigner;
}

std::vector<ConstraintDefinition> ConstraintManager::getTableConstraints(const std::string& tableName, const std::string& connectionId) {
    return impl_->getTableConstraints(tableName, connectionId);
}

ConstraintDefinition ConstraintManager::getConstraintDefinition(const std::string& constraintName, const std::string& connectionId) {
    return impl_->getConstraintDefinition(constraintName, connectionId);
}

std::vector<ConstraintStatistics> ConstraintManager::getConstraintStatistics(const std::string& constraintName, const std::string& connectionId) {
    return impl_->getConstraintStatistics(constraintName, connectionId);
}

bool ConstraintManager::createConstraint(const ConstraintDefinition& definition, const std::string& connectionId) {
    return impl_->createConstraint(definition, connectionId);
}

bool ConstraintManager::dropConstraint(const std::string& constraintName, const std::string& connectionId) {
    return impl_->dropConstraint(constraintName, connectionId);
}

bool ConstraintManager::enableConstraint(const std::string& constraintName, const std::string& connectionId) {
    return impl_->enableConstraint(constraintName, connectionId);
}

bool ConstraintManager::disableConstraint(const std::string& constraintName, const std::string& connectionId) {
    return impl_->disableConstraint(constraintName, connectionId);
}

bool ConstraintManager::validateConstraint(const std::string& constraintName, const std::string& connectionId) {
    return impl_->validateConstraint(constraintName, connectionId);
}

bool ConstraintManager::deferConstraint(const std::string& constraintName, const std::string& connectionId) {
    return impl_->deferConstraint(constraintName, connectionId);
}

bool ConstraintManager::immediateConstraint(const std::string& constraintName, const std::string& connectionId) {
    return impl_->immediateConstraint(constraintName, connectionId);
}

std::vector<ConstraintMaintenanceOperation> ConstraintManager::getMaintenanceHistory(const std::string& constraintName, const std::string& connectionId) {
    return impl_->getMaintenanceHistory(constraintName, connectionId);
}

ConstraintMaintenanceOperation ConstraintManager::performMaintenance(const std::string& constraintName, ConstraintOperation operation, const std::string& connectionId) {
    return impl_->performMaintenance(constraintName, operation, connectionId);
}

ConstraintValidationResult ConstraintManager::validateConstraintDefinition(const ConstraintDefinition& definition, const std::string& connectionId) {
    return impl_->validateConstraintDefinition(definition, connectionId);
}

std::vector<ConstraintViolation> ConstraintManager::checkConstraintViolations(const std::string& constraintName, const std::string& connectionId) {
    return impl_->checkConstraintViolations(constraintName, connectionId);
}

std::vector<ConstraintDependency> ConstraintManager::getConstraintDependencies(const std::string& constraintName, const std::string& connectionId) {
    return impl_->getConstraintDependencies(constraintName, connectionId);
}

std::vector<ConstraintTemplate> ConstraintManager::getAvailableTemplates(const std::string& connectionId) {
    return impl_->getAvailableTemplates(connectionId);
}

ConstraintDefinition ConstraintManager::applyTemplate(const std::string& templateId, const std::unordered_map<std::string, std::string>& parameters, const std::string& connectionId) {
    return impl_->applyTemplate(templateId, parameters, connectionId);
}

void ConstraintManager::saveTemplate(const ConstraintTemplate& template_, const std::string& connectionId) {
    impl_->saveTemplate(template_, connectionId);
}

ConstraintAnalysisReport ConstraintManager::analyzeTableConstraints(const std::string& tableName, const std::string& connectionId) {
    return impl_->analyzeTableConstraints(tableName, connectionId);
}

ConstraintAnalysisReport ConstraintManager::analyzeDatabaseConstraints(const std::string& connectionId) {
    return impl_->analyzeDatabaseConstraints(connectionId);
}

std::vector<ConstraintTemplate> ConstraintManager::getRecommendedTemplates(const std::string& tableName, const std::string& connectionId) {
    return impl_->getRecommendedTemplates(tableName, connectionId);
}

std::string ConstraintManager::generateConstraintDDL(const ConstraintDefinition& definition, const std::string& connectionId) {
    return impl_->generateConstraintDDL(definition, connectionId);
}

bool ConstraintManager::isConstraintNameAvailable(const std::string& name, const std::string& connectionId) {
    return impl_->isConstraintNameAvailable(name, connectionId);
}

std::vector<std::string> ConstraintManager::getAvailableConstraintTypes(const std::string& connectionId) {
    return impl_->getAvailableConstraintTypes(connectionId);
}

void ConstraintManager::collectConstraintStatistics(const std::string& constraintName, const std::string& connectionId) {
    impl_->collectConstraintStatistics(constraintName, connectionId);
}

void ConstraintManager::collectAllConstraintStatistics(const std::string& connectionId) {
    impl_->collectAllConstraintStatistics(connectionId);
}

void ConstraintManager::setConstraintCreatedCallback(ConstraintCreatedCallback callback) {
    constraintCreatedCallback_ = callback;
}

void ConstraintManager::setConstraintDroppedCallback(ConstraintDroppedCallback callback) {
    constraintDroppedCallback_ = callback;
}

void ConstraintManager::setConstraintModifiedCallback(ConstraintModifiedCallback callback) {
    constraintModifiedCallback_ = callback;
}

void ConstraintManager::setViolationDetectedCallback(ViolationDetectedCallback callback) {
    violationDetectedCallback_ = callback;
}

void ConstraintManager::setMaintenanceCompletedCallback(MaintenanceCompletedCallback callback) {
    maintenanceCompletedCallback_ = callback;
}

} // namespace scratchrobin
