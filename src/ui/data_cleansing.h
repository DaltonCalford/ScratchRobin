#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QTableWidget;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QSplitter;
class QProgressBar;
class QListWidget;
class QStackedWidget;
class QRadioButton;
class QSpinBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Data Cleansing - Data quality and deduplication tool
 * 
 * Clean and improve data quality:
 * - Duplicate detection and removal
 * - Data formatting and standardization
 * - Null/empty value handling
 * - Data validation and correction
 * - Find and replace patterns
 * - Bulk updates with preview
 */

// ============================================================================
// Cleansing Operation Types
// ============================================================================
enum class CleansingOperation {
    RemoveDuplicates,
    RemoveNulls,
    StandardizeFormat,
    FindReplace,
    TrimWhitespace,
    ChangeCase,
    ValidateFormat,
    FixEncoding,
    MergeRecords,
    SplitColumn,
    NormalizeValues,
    CustomExpression
};

enum class DuplicateMatchType {
    Exact,
    Fuzzy,
    Soundex,
    Custom
};

struct DuplicateRule {
    QStringList columns;
    DuplicateMatchType matchType;
    int fuzzyThreshold = 80; // 0-100
    QString customFormula;
    bool caseSensitive = false;
    bool ignoreWhitespace = true;
};

struct CleansingRule {
    QString id;
    QString name;
    QString description;
    QString schema;
    QString tableName;
    CleansingOperation operation;
    QString targetColumn;
    
    // Operation-specific parameters
    QVariant value; // For find/replace, null replacement
    QString format; // For standardization
    QString pattern; // For validation
    QString replacementPattern;
    bool caseSensitive = false;
    bool useRegex = false;
    
    // Duplicate detection
    DuplicateRule duplicateRule;
    QString keepStrategy; // first, last, merge
    
    // Preview and safety
    bool previewOnly = true;
    bool backupBeforeRun = true;
    int sampleSize = 1000;
};

struct CleansingResult {
    QString ruleId;
    QDateTime startTime;
    QDateTime endTime;
    int rowsScanned = 0;
    int rowsAffected = 0;
    int duplicatesFound = 0;
    int duplicatesRemoved = 0;
    int nullsFixed = 0;
    int formatErrors = 0;
    bool success = false;
    QString errorMessage;
};

// ============================================================================
// Data Cleansing Panel
// ============================================================================
class DataCleansingPanel : public DockPanel {
    Q_OBJECT

public:
    explicit DataCleansingPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Data Cleansing"); }
    QString panelCategory() const override { return "tools"; }

public slots:
    // Rule management
    void onCreateRule();
    void onEditRule();
    void onDeleteRule();
    void onCloneRule();
    void onRuleSelected(const QModelIndex& index);
    
    // Operations
    void onPreviewChanges();
    void onApplyChanges();
    void onScheduleCleansing();
    void onExportReport();
    
    // Quick actions
    void onQuickDeduplicate();
    void onQuickTrimWhitespace();
    void onQuickStandardizeCase();
    void onQuickRemoveNulls();
    
    // Analysis
    void onAnalyzeDataQuality();
    void onFindDuplicates();
    void onGenerateReport();

signals:
    void cleansingStarted(const QString& ruleId);
    void cleansingCompleted(const QString& ruleId, const CleansingResult& result);
    void duplicatesFound(const QString& table, int count);

private:
    void setupUi();
    void setupRulesTable();
    void setupQualityPanel();
    void loadRules();
    void updateRulesTable();
    void updateRuleDetails(const CleansingRule& rule);
    
    backend::SessionClient* client_;
    QList<CleansingRule> rules_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QTableView* rulesTable_ = nullptr;
    QStandardItemModel* rulesModel_ = nullptr;
    
    // Rule details
    QLabel* ruleNameLabel_ = nullptr;
    QLabel* operationLabel_ = nullptr;
    QLabel* targetLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    
    // Quality dashboard
    QProgressBar* overallQualityBar_ = nullptr;
    QLabel* duplicatesLabel_ = nullptr;
    QLabel* nullsLabel_ = nullptr;
    QLabel* formatIssuesLabel_ = nullptr;
};

// ============================================================================
// Cleansing Rule Dialog
// ============================================================================
class CleansingRuleDialog : public QDialog {
    Q_OBJECT

public:
    explicit CleansingRuleDialog(CleansingRule* rule, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onOperationChanged(int index);
    void onTableChanged(int index);
    void onPreview();
    void onSave();
    void onCancel();

private:
    void setupUi();
    void setupBasicPage();
    void setupOperationPage();
    void setupSafetyPage();
    void updateOperationOptions();
    void loadTables();
    void loadColumns(const QString& table);
    
    CleansingRule* rule_;
    backend::SessionClient* client_;
    
    QStackedWidget* pages_ = nullptr;
    
    // Basic page
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* tableCombo_ = nullptr;
    QComboBox* operationCombo_ = nullptr;
    
    // Operation-specific widgets
    QStackedWidget* operationStack_ = nullptr;
    
    // Duplicate detection
    QListWidget* duplicateColumnsList_ = nullptr;
    QComboBox* matchTypeCombo_ = nullptr;
    QSpinBox* thresholdSpin_ = nullptr;
    QComboBox* keepStrategyCombo_ = nullptr;
    
    // Find/replace
    QLineEdit* findEdit_ = nullptr;
    QLineEdit* replaceEdit_ = nullptr;
    QCheckBox* regexCheck_ = nullptr;
    QCheckBox* caseSensitiveCheck_ = nullptr;
    
    // Format standardization
    QComboBox* formatTypeCombo_ = nullptr;
    QLineEdit* customFormatEdit_ = nullptr;
    
    // Change case
    QComboBox* caseTypeCombo_ = nullptr;
    
    // Validation
    QComboBox* validationTypeCombo_ = nullptr;
    QLineEdit* validationPatternEdit_ = nullptr;
    QComboBox* invalidActionCombo_ = nullptr;
    
    // Null handling
    QComboBox* nullActionCombo_ = nullptr;
    QLineEdit* nullReplacementEdit_ = nullptr;
    
    // Safety page
    QCheckBox* previewCheck_ = nullptr;
    QCheckBox* backupCheck_ = nullptr;
    QSpinBox* sampleSizeSpin_ = nullptr;
};

// ============================================================================
// Duplicate Finder Dialog
// ============================================================================
class DuplicateFinderDialog : public QDialog {
    Q_OBJECT

public:
    explicit DuplicateFinderDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onTableChanged(int index);
    void onStartScan();
    void onStopScan();
    void onSelectAll();
    void onDeselectAll();
    void onKeepFirst();
    void onKeepLast();
    void onMergeSelected();
    void onDeleteSelected();

private:
    void setupUi();
    void performScan();
    void updateResults();
    
    backend::SessionClient* client_;
    bool scanning_ = false;
    
    QComboBox* tableCombo_ = nullptr;
    QListWidget* columnsList_ = nullptr;
    QComboBox* matchTypeCombo_ = nullptr;
    QSpinBox* thresholdSpin_ = nullptr;
    
    QTableView* resultsTable_ = nullptr;
    QStandardItemModel* resultsModel_ = nullptr;
    
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    
    struct DuplicateGroup {
        int groupId;
        QList<int> rowIds;
        QStringList values;
        int count;
    };
    QList<DuplicateGroup> duplicateGroups_;
};

// ============================================================================
// Data Quality Analyzer
// ============================================================================
class DataQualityAnalyzer : public QDialog {
    Q_OBJECT

public:
    explicit DataQualityAnalyzer(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onAnalyze();
    void onExportReport();
    void onGenerateFixRules();

private:
    void setupUi();
    void performAnalysis();
    void calculateQualityScore();
    
    backend::SessionClient* client_;
    
    QComboBox* tableCombo_ = nullptr;
    QTableWidget* resultsTable_ = nullptr;
    QTextEdit* reportEdit_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    
    struct ColumnQuality {
        QString columnName;
        QString dataType;
        int totalRows;
        int nullCount;
        int uniqueCount;
        int duplicateCount;
        int formatErrors;
        float qualityScore;
        QString recommendations;
    };
    QList<ColumnQuality> qualityResults_;
};

// ============================================================================
// Preview Changes Dialog
// ============================================================================
class PreviewChangesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreviewChangesDialog(const CleansingRule& rule, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onApply();
    void onCancel();

private:
    void setupUi();
    void loadPreviewData();
    
    CleansingRule rule_;
    backend::SessionClient* client_;
    
    QLabel* summaryLabel_ = nullptr;
    QTableView* beforeTable_ = nullptr;
    QTableView* afterTable_ = nullptr;
    QStandardItemModel* beforeModel_ = nullptr;
    QStandardItemModel* afterModel_ = nullptr;
};

// ============================================================================
// Bulk Update Dialog
// ============================================================================
class BulkUpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit BulkUpdateDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onExecute();
    void onSchedule();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QComboBox* tableCombo_ = nullptr;
    QComboBox* columnCombo_ = nullptr;
    QTextEdit* setClauseEdit_ = nullptr;
    QTextEdit* whereClauseEdit_ = nullptr;
    QLabel* affectedRowsLabel_ = nullptr;
    QTableView* previewTable_ = nullptr;
};

} // namespace scratchrobin::ui
