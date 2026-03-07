#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QRegularExpression>

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
class QSpinBox;
class QListWidget;
class QStackedWidget;
class QRadioButton;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Data Masking - Sensitive data obfuscation tool
 * 
 * Protect sensitive data through various masking techniques:
 * - Full/partial masking (show last 4 digits only)
 * - Encryption/tokenization
 * - Data substitution/shuffling
 * - Nulling out sensitive fields
 * - Format-preserving masking
 * - Role-based masking policies
 */

// ============================================================================
// Masking Types
// ============================================================================
enum class MaskingType {
    Full,           // Replace with fixed string (e.g., "****")
    Partial,        // Show part of data (e.g., "***-**-6789")
    Random,         // Replace with random data of same type
    Shuffle,        // Shuffle values within column
    Hash,           // One-way hash
    Encryption,     // Reversible encryption
    Tokenization,   // Replace with token
    Nullify,        // Set to NULL
    Truncation,     // Remove part of data
    Rounding,       // For numeric data
    Custom          // Custom formula
};

enum class SensitiveDataType {
    SSN,
    CreditCard,
    Phone,
    Email,
    Name,
    Address,
    DateOfBirth,
    AccountNumber,
    IPAddress,
    Custom
};

struct MaskingRule {
    QString id;
    QString name;
    QString description;
    QString schema;
    QString tableName;
    QString columnName;
    QString dataType;
    SensitiveDataType sensitiveType;
    MaskingType maskingType;
    bool enabled = true;
    
    // Masking options
    int showFirst = 0;
    int showLast = 4;
    QString maskChar = "*";
    QString customMask;
    bool preserveFormat = true;
    QString encryptionKey;
    QString customFormula;
    
    // Conditions
    bool conditionalMasking = false;
    QString conditionColumn;
    QString conditionOperator;
    QVariant conditionValue;
    
    // Auditing
    bool auditAccess = true;
    QString allowedRoles;
};

struct MaskingPolicy {
    QString id;
    QString name;
    QString description;
    bool enabled = true;
    QList<MaskingRule> rules;
    QStringList applyToRoles; // Empty = all roles
    QStringList excludeRoles; // These roles see unmasked data
};

// ============================================================================
// Data Masking Panel
// ============================================================================
class DataMaskingPanel : public DockPanel {
    Q_OBJECT

public:
    explicit DataMaskingPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Data Masking"); }
    QString panelCategory() const override { return "security"; }

public slots:
    // Rule management
    void onCreateRule();
    void onEditRule();
    void onDeleteRule();
    void onDuplicateRule();
    void onToggleRule(bool enabled);
    
    // Policy management
    void onCreatePolicy();
    void onEditPolicy();
    void onDeletePolicy();
    void onApplyPolicy();
    
    // Detection
    void onScanForSensitiveData();
    void onAutoGenerateRules();
    void onRuleSelected(const QModelIndex& index);
    
    // Preview
    void onPreviewMasking();
    void onCompareBeforeAfter();
    void onExportRules();
    void onImportRules();

signals:
    void ruleCreated(const QString& ruleId);
    void ruleModified(const QString& ruleId);
    void ruleDeleted(const QString& ruleId);
    void policyApplied(const QString& policyId);

private:
    void setupUi();
    void setupRulesTable();
    void setupPoliciesPanel();
    void loadRules();
    void updateRulesTable();
    void updateRuleDetails(const MaskingRule& rule);
    
    backend::SessionClient* client_;
    QList<MaskingRule> rules_;
    QList<MaskingPolicy> policies_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QTableView* rulesTable_ = nullptr;
    QStandardItemModel* rulesModel_ = nullptr;
    
    // Rule details
    QLabel* ruleNameLabel_ = nullptr;
    QLabel* tableColumnLabel_ = nullptr;
    QLabel* dataTypeLabel_ = nullptr;
    QLabel* maskingTypeLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
};

// ============================================================================
// Masking Rule Dialog
// ============================================================================
class MaskingRuleDialog : public QDialog {
    Q_OBJECT

public:
    explicit MaskingRuleDialog(MaskingRule* rule, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onSensitiveTypeChanged(int index);
    void onMaskingTypeChanged(int index);
    void onTableChanged(int index);
    void onPreview();
    void onSave();
    void onCancel();

private:
    void setupUi();
    void setupBasicPage();
    void setupMaskingOptionsPage();
    void setupConditionsPage();
    void loadTables();
    void loadColumns(const QString& table);
    void updatePreview();
    
    MaskingRule* rule_;
    backend::SessionClient* client_;
    
    QStackedWidget* pages_ = nullptr;
    
    // Basic page
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* tableCombo_ = nullptr;
    QComboBox* columnCombo_ = nullptr;
    QComboBox* sensitiveTypeCombo_ = nullptr;
    QComboBox* maskingTypeCombo_ = nullptr;
    
    // Masking options page
    QStackedWidget* optionsStack_ = nullptr;
    
    // Partial masking
    QSpinBox* showFirstSpin_ = nullptr;
    QSpinBox* showLastSpin_ = nullptr;
    QLineEdit* maskCharEdit_ = nullptr;
    
    // Options pages
    QWidget* partialOptionsPage_ = nullptr;
    QWidget* hashOptionsPage_ = nullptr;
    
    // Random/substitution
    QComboBox* substitutionTypeCombo_ = nullptr;
    
    // Hash/encryption
    QComboBox* algorithmCombo_ = nullptr;
    QLineEdit* keyEdit_ = nullptr;
    
    // Custom
    QTextEdit* formulaEdit_ = nullptr;
    
    // Conditions page
    QCheckBox* conditionalCheck_ = nullptr;
    QComboBox* conditionColumnCombo_ = nullptr;
    QComboBox* conditionOpCombo_ = nullptr;
    QLineEdit* conditionValueEdit_ = nullptr;
    
    // Preview
    QTableView* previewTable_ = nullptr;
    QStandardItemModel* previewModel_ = nullptr;
};

// ============================================================================
// Sensitive Data Scanner
// ============================================================================
class SensitiveDataScanner : public QDialog {
    Q_OBJECT

public:
    explicit SensitiveDataScanner(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onStartScan();
    void onStopScan();
    void onSelectAll();
    void onDeselectAll();
    void onGenerateRules();
    void onExportResults();

private:
    void setupUi();
    void performScan();
    void addDetectedColumn(const QString& schema, const QString& table, 
                          const QString& column, SensitiveDataType type);
    
    backend::SessionClient* client_;
    bool scanning_ = false;
    
    QTableView* resultsTable_ = nullptr;
    QStandardItemModel* resultsModel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* scanBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    
    struct DetectedColumn {
        QString schema;
        QString table;
        QString column;
        QString dataType;
        SensitiveDataType detectedType;
        float confidence;
        bool selected;
    };
    QList<DetectedColumn> detectedColumns_;
};

// ============================================================================
// Masking Preview Dialog
// ============================================================================
class MaskingPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit MaskingPreviewDialog(const MaskingRule& rule, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExport();

private:
    void setupUi();
    void loadPreviewData();
    QString applyMasking(const QString& value);
    
    MaskingRule rule_;
    backend::SessionClient* client_;
    
    QTableView* beforeTable_ = nullptr;
    QTableView* afterTable_ = nullptr;
    QStandardItemModel* beforeModel_ = nullptr;
    QStandardItemModel* afterModel_ = nullptr;
};

// ============================================================================
// Policy Management Dialog
// ============================================================================
class PolicyManagementDialog : public QDialog {
    Q_OBJECT

public:
    explicit PolicyManagementDialog(QList<MaskingPolicy>* policies, QWidget* parent = nullptr);

public slots:
    void onCreatePolicy();
    void onEditPolicy();
    void onDeletePolicy();
    void onClonePolicy();
    void onPolicySelected(const QModelIndex& index);
    void onSaveChanges();

private:
    void setupUi();
    void loadPolicies();
    
    QList<MaskingPolicy>* policies_;
    
    QListWidget* policyList_ = nullptr;
    QTableWidget* rulesTable_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QCheckBox* enabledCheck_ = nullptr;
    QListWidget* rolesList_ = nullptr;
};

} // namespace scratchrobin::ui
