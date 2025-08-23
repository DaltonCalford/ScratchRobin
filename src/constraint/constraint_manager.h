#ifndef SCRATCHROBIN_CONSTRAINT_MANAGER_H
#define SCRATCHROBIN_CONSTRAINT_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <functional>
#include <QObject>
#include <QDateTime>
#include <QJsonObject>
#include <QSqlDatabase>
#include <variant>

namespace scratchrobin {

class IMetadataManager;
class ISQLExecutor;
class IConnectionManager;
class IIndexManager;
class ITableDesigner;

enum class ConstraintType {
    PRIMARY_KEY,
    FOREIGN_KEY,
    UNIQUE,
    CHECK,
    NOT_NULL,
    DEFAULT,
    EXCLUDE,
    DOMAIN
};

enum class ConstraintStatus {
    VALID,
    INVALID,
    ENFORCED,
    NOT_ENFORCED,
    DEFERRED,
    CHECKING
};

enum class ConstraintDeferrable {
    NOT_DEFERRABLE,
    DEFERRABLE_INITIALLY_IMMEDIATE,
    DEFERRABLE_INITIALLY_DEFERRED
};

enum class ForeignKeyAction {
    NO_ACTION,
    RESTRICT,
    CASCADE,
    SET_NULL,
    SET_DEFAULT
};

enum class ConstraintOperation {
    CREATE,
    DROP,
    ENABLE,
    DISABLE,
    VALIDATE,
    DEFER,
    IMMEDIATE
};

struct ConstraintColumn {
    std::string columnName;
    std::string expression;
    bool isAscending = true;
    std::string collation;
    std::string operatorClass;
};

struct PrimaryKeyConstraint {
    std::string name;
    std::string tableName;
    std::vector<ConstraintColumn> columns;
    bool isDeferrable = false;
    ConstraintDeferrable deferrableMode = ConstraintDeferrable::NOT_DEFERRABLE;
    std::string comment;
};

struct ForeignKeyConstraint {
    std::string name;
    std::string tableName;
    std::string referencedTable;
    std::vector<std::pair<std::string, std::string>> columnMappings;
    ForeignKeyAction onDelete = ForeignKeyAction::NO_ACTION;
    ForeignKeyAction onUpdate = ForeignKeyAction::NO_ACTION;
    bool isDeferrable = false;
    ConstraintDeferrable deferrableMode = ConstraintDeferrable::NOT_DEFERRABLE;
    std::string comment;
};

struct UniqueConstraint {
    std::string name;
    std::string tableName;
    std::vector<ConstraintColumn> columns;
    bool isDeferrable = false;
    ConstraintDeferrable deferrableMode = ConstraintDeferrable::NOT_DEFERRABLE;
    std::string comment;
};

struct CheckConstraint {
    std::string name;
    std::string tableName;
    std::string expression;
    bool isDeferrable = false;
    ConstraintDeferrable deferrableMode = ConstraintDeferrable::NOT_DEFERRABLE;
    std::string comment;
};

struct NotNullConstraint {
    std::string columnName;
    std::string tableName;
    std::string comment;
};

struct DefaultConstraint {
    std::string columnName;
    std::string tableName;
    std::string expression;
    std::string comment;
};

struct ConstraintDefinition {
    std::string name;
    std::string schema;
    ConstraintType type;
    ConstraintStatus status;
    std::string tableName;
    std::string definition;
    std::string comment;
    QDateTime createdAt;
    QDateTime lastModified;
    std::unordered_map<std::string, std::string> properties;

    // Type-specific data
    std::variant<PrimaryKeyConstraint, ForeignKeyConstraint, UniqueConstraint,
                 CheckConstraint, NotNullConstraint, DefaultConstraint> constraintData;
};

struct ConstraintViolation {
    std::string constraintName;
    ConstraintType constraintType;
    std::string tableName;
    std::vector<std::string> violatedColumns;
    std::vector<QVariant> violatedValues;
    std::string violationMessage;
    QDateTime occurredAt;
};

struct ConstraintStatistics {
    std::string constraintName;
    std::string tableName;
    ConstraintType type;
    ConstraintStatus status;
    qint64 violationCount = 0;
    qint64 checkCount = 0;
    double performanceImpact = 0.0;
    QDateTime lastViolation;
    QDateTime lastCheck;
    QDateTime collectedAt;
};

struct ConstraintDependency {
    std::string constraintName;
    std::string dependentConstraint;
    std::string dependencyType;  // "BLOCKS", "DEPENDS_ON", "RELATED"
    std::string relationship;
    bool isBlocking = false;
    QDateTime createdAt;
};

struct ConstraintTemplate {
    std::string templateId;
    std::string name;
    std::string description;
    ConstraintType type;
    std::string templateDefinition;
    std::vector<std::string> parameters;
    std::vector<std::string> applicableDataTypes;
    std::string category;
    bool isSystemTemplate = false;
    QDateTime createdAt;
    QDateTime lastUsed;
};

struct ConstraintValidationResult {
    bool isValid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> suggestions;
    std::string validationMessage;
};

struct ConstraintAnalysisReport {
    std::string reportId;
    std::string databaseName;
    std::string tableName;
    QDateTime generatedAt;
    std::chrono::milliseconds analysisDuration{0};

    std::vector<ConstraintStatistics> statistics;
    std::vector<ConstraintViolation> violations;
    std::vector<ConstraintDependency> dependencies;
    std::vector<ConstraintTemplate> recommendedTemplates;

    // Summary statistics
    int totalConstraints = 0;
    int enabledConstraints = 0;
    int disabledConstraints = 0;
    int invalidConstraints = 0;
    int violationsToday = 0;
    int violationsThisWeek = 0;
    int violationsThisMonth = 0;

    double averagePerformanceImpact = 0.0;
    std::string overallHealth;  // "Excellent", "Good", "Fair", "Poor", "Critical"
    std::vector<std::string> recommendations;
    std::vector<std::string> criticalIssues;
};

struct ConstraintMaintenanceOperation {
    std::string operationId;
    std::string constraintName;
    ConstraintOperation operation;
    std::string sqlStatement;
    ConstraintStatus statusBefore;
    ConstraintStatus statusAfter;
    QDateTime startedAt;
    QDateTime completedAt;
    bool success = false;
    std::string errorMessage;
    std::string outputMessage;
    qint64 affectedRows = 0;
};

class IConstraintManager {
public:
    virtual ~IConstraintManager() = default;

    virtual void initialize() = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;
    virtual void setIndexManager(std::shared_ptr<IIndexManager> indexManager) = 0;
    virtual void setTableDesigner(std::shared_ptr<ITableDesigner> tableDesigner) = 0;

    virtual std::vector<ConstraintDefinition> getTableConstraints(const std::string& tableName, const std::string& connectionId) = 0;
    virtual ConstraintDefinition getConstraintDefinition(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual std::vector<ConstraintStatistics> getConstraintStatistics(const std::string& constraintName, const std::string& connectionId) = 0;

    virtual bool createConstraint(const ConstraintDefinition& definition, const std::string& connectionId) = 0;
    virtual bool dropConstraint(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual bool enableConstraint(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual bool disableConstraint(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual bool validateConstraint(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual bool deferConstraint(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual bool immediateConstraint(const std::string& constraintName, const std::string& connectionId) = 0;

    virtual std::vector<ConstraintMaintenanceOperation> getMaintenanceHistory(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual ConstraintMaintenanceOperation performMaintenance(const std::string& constraintName, ConstraintOperation operation, const std::string& connectionId) = 0;

    virtual ConstraintValidationResult validateConstraintDefinition(const ConstraintDefinition& definition, const std::string& connectionId) = 0;
    virtual std::vector<ConstraintViolation> checkConstraintViolations(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual std::vector<ConstraintDependency> getConstraintDependencies(const std::string& constraintName, const std::string& connectionId) = 0;

    virtual std::vector<ConstraintTemplate> getAvailableTemplates(const std::string& connectionId) = 0;
    virtual ConstraintDefinition applyTemplate(const std::string& templateId, const std::unordered_map<std::string, std::string>& parameters, const std::string& connectionId) = 0;
    virtual void saveTemplate(const ConstraintTemplate& template_, const std::string& connectionId) = 0;

    virtual ConstraintAnalysisReport analyzeTableConstraints(const std::string& tableName, const std::string& connectionId) = 0;
    virtual ConstraintAnalysisReport analyzeDatabaseConstraints(const std::string& connectionId) = 0;
    virtual std::vector<ConstraintTemplate> getRecommendedTemplates(const std::string& tableName, const std::string& connectionId) = 0;

    virtual std::string generateConstraintDDL(const ConstraintDefinition& definition, const std::string& connectionId) = 0;
    virtual bool isConstraintNameAvailable(const std::string& name, const std::string& connectionId) = 0;
    virtual std::vector<std::string> getAvailableConstraintTypes(const std::string& connectionId) = 0;

    virtual void collectConstraintStatistics(const std::string& constraintName, const std::string& connectionId) = 0;
    virtual void collectAllConstraintStatistics(const std::string& connectionId) = 0;

    using ConstraintCreatedCallback = std::function<void(const std::string&, const std::string&)>;
    using ConstraintDroppedCallback = std::function<void(const std::string&, const std::string&)>;
    using ConstraintModifiedCallback = std::function<void(const std::string&, const std::string&)>;
    using ViolationDetectedCallback = std::function<void(const ConstraintViolation&)>;
    using MaintenanceCompletedCallback = std::function<void(const ConstraintMaintenanceOperation&)>;

    virtual void setConstraintCreatedCallback(ConstraintCreatedCallback callback) = 0;
    virtual void setConstraintDroppedCallback(ConstraintDroppedCallback callback) = 0;
    virtual void setConstraintModifiedCallback(ConstraintModifiedCallback callback) = 0;
    virtual void setViolationDetectedCallback(ViolationDetectedCallback callback) = 0;
    virtual void setMaintenanceCompletedCallback(MaintenanceCompletedCallback callback) = 0;
};

class ConstraintManager : public QObject, public IConstraintManager {
    Q_OBJECT

public:
    ConstraintManager(QObject* parent = nullptr);
    ~ConstraintManager() override;

    // IConstraintManager implementation
    void initialize() override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;
    void setIndexManager(std::shared_ptr<IIndexManager> indexManager) override;
    void setTableDesigner(std::shared_ptr<ITableDesigner> tableDesigner) override;

    std::vector<ConstraintDefinition> getTableConstraints(const std::string& tableName, const std::string& connectionId) override;
    ConstraintDefinition getConstraintDefinition(const std::string& constraintName, const std::string& connectionId) override;
    std::vector<ConstraintStatistics> getConstraintStatistics(const std::string& constraintName, const std::string& connectionId) override;

    bool createConstraint(const ConstraintDefinition& definition, const std::string& connectionId) override;
    bool dropConstraint(const std::string& constraintName, const std::string& connectionId) override;
    bool enableConstraint(const std::string& constraintName, const std::string& connectionId) override;
    bool disableConstraint(const std::string& constraintName, const std::string& connectionId) override;
    bool validateConstraint(const std::string& constraintName, const std::string& connectionId) override;
    bool deferConstraint(const std::string& constraintName, const std::string& connectionId) override;
    bool immediateConstraint(const std::string& constraintName, const std::string& connectionId) override;

    std::vector<ConstraintMaintenanceOperation> getMaintenanceHistory(const std::string& constraintName, const std::string& connectionId) override;
    ConstraintMaintenanceOperation performMaintenance(const std::string& constraintName, ConstraintOperation operation, const std::string& connectionId) override;

    ConstraintValidationResult validateConstraintDefinition(const ConstraintDefinition& definition, const std::string& connectionId) override;
    std::vector<ConstraintViolation> checkConstraintViolations(const std::string& constraintName, const std::string& connectionId) override;
    std::vector<ConstraintDependency> getConstraintDependencies(const std::string& constraintName, const std::string& connectionId) override;

    std::vector<ConstraintTemplate> getAvailableTemplates(const std::string& connectionId) override;
    ConstraintDefinition applyTemplate(const std::string& templateId, const std::unordered_map<std::string, std::string>& parameters, const std::string& connectionId) override;
    void saveTemplate(const ConstraintTemplate& template_, const std::string& connectionId) override;

    ConstraintAnalysisReport analyzeTableConstraints(const std::string& tableName, const std::string& connectionId) override;
    ConstraintAnalysisReport analyzeDatabaseConstraints(const std::string& connectionId) override;
    std::vector<ConstraintTemplate> getRecommendedTemplates(const std::string& tableName, const std::string& connectionId) override;

    std::string generateConstraintDDL(const ConstraintDefinition& definition, const std::string& connectionId) override;
    bool isConstraintNameAvailable(const std::string& name, const std::string& connectionId) override;
    std::vector<std::string> getAvailableConstraintTypes(const std::string& connectionId) override;

    void collectConstraintStatistics(const std::string& constraintName, const std::string& connectionId) override;
    void collectAllConstraintStatistics(const std::string& connectionId) override;

    void setConstraintCreatedCallback(ConstraintCreatedCallback callback) override;
    void setConstraintDroppedCallback(ConstraintDroppedCallback callback) override;
    void setConstraintModifiedCallback(ConstraintModifiedCallback callback) override;
    void setViolationDetectedCallback(ViolationDetectedCallback callback) override;
    void setMaintenanceCompletedCallback(MaintenanceCompletedCallback callback) override;

signals:
    void constraintCreated(const std::string& constraintName, const std::string& connectionId);
    void constraintDropped(const std::string& constraintName, const std::string& connectionId);
    void constraintModified(const std::string& constraintName, const std::string& connectionId);
    void violationDetected(const ConstraintViolation& violation);
    void maintenanceCompleted(const ConstraintMaintenanceOperation& operation);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IIndexManager> indexManager_;
    std::shared_ptr<ITableDesigner> tableDesigner_;

    // Callbacks
    ConstraintCreatedCallback constraintCreatedCallback_;
    ConstraintDroppedCallback constraintDroppedCallback_;
    ConstraintModifiedCallback constraintModifiedCallback_;
    ViolationDetectedCallback violationDetectedCallback_;
    MaintenanceCompletedCallback maintenanceCompletedCallback_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONSTRAINT_MANAGER_H
