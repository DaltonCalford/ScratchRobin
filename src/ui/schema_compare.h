#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QCheckBox;
class QGroupBox;
class QSplitter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Schema Compare - Compare and diff database schemas
 * 
 * Compare two schemas and identify differences in:
 * - Tables (columns, constraints, indexes)
 * - Views
 * - Functions/Procedures
 * - Sequences
 * - Triggers
 * - Privileges
 * Generate synchronization scripts
 */

// ============================================================================
// Schema Diff Types
// ============================================================================
enum class DiffType {
    Identical,
    Created,      // Exists in source but not in target
    Dropped,      // Exists in target but not in source
    Modified      // Exists in both but different
};

struct SchemaObjectDiff {
    QString objectType;      // TABLE, VIEW, FUNCTION, etc.
    QString objectName;
    QString schemaName;
    DiffType diffType;
    QStringList differences; // Specific differences (columns, etc.)
    QString sourceDdl;
    QString targetDdl;
};

// ============================================================================
// Schema Compare Panel
// ============================================================================
class SchemaComparePanel : public DockPanel {
    Q_OBJECT

public:
    explicit SchemaComparePanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Schema Compare"); }
    QString panelCategory() const override { return "database"; }

public slots:
    void onSelectSourceSchema();
    void onSelectTargetSchema();
    void onRunComparison();
    void onGenerateSyncScript();
    void onApplyChanges();
    void onExportResults();
    void onSaveSnapshot();
    void onLoadSnapshot();
    void onFilterChanged(const QString& filter);
    void onObjectSelected(const QModelIndex& index);

signals:
    void comparisonStarted();
    void comparisonFinished(int differences, int onlyInSource, int onlyInTarget, int modified);

private:
    void setupUi();
    void setupModels();
    void runComparison();
    void displayResults();
    
    backend::SessionClient* client_;
    
    // Schema selection
    QComboBox* sourceConnectionCombo_ = nullptr;
    QComboBox* sourceSchemaCombo_ = nullptr;
    QComboBox* targetConnectionCombo_ = nullptr;
    QComboBox* targetSchemaCombo_ = nullptr;
    
    // Options
    QCheckBox* compareTablesCheck_ = nullptr;
    QCheckBox* compareViewsCheck_ = nullptr;
    QCheckBox* compareFunctionsCheck_ = nullptr;
    QCheckBox* compareIndexesCheck_ = nullptr;
    QCheckBox* compareConstraintsCheck_ = nullptr;
    QCheckBox* compareTriggersCheck_ = nullptr;
    QCheckBox* comparePrivilegesCheck_ = nullptr;
    QCheckBox* ignoreWhitespaceCheck_ = nullptr;
    QCheckBox* ignoreCommentsCheck_ = nullptr;
    
    // Results
    QTabWidget* tabWidget_ = nullptr;
    QTreeView* diffTree_ = nullptr;
    QStandardItemModel* diffModel_ = nullptr;
    
    // Detail views
    QTextEdit* sourceDdlEdit_ = nullptr;
    QTextEdit* targetDdlEdit_ = nullptr;
    QTextEdit* diffEdit_ = nullptr;
    QTextEdit* scriptEdit_ = nullptr;
    
    // Stats
    QLabel* summaryLabel_ = nullptr;
    QLabel* identicalLabel_ = nullptr;
    QLabel* createdLabel_ = nullptr;
    QLabel* droppedLabel_ = nullptr;
    QLabel* modifiedLabel_ = nullptr;
    
    QList<SchemaObjectDiff> differences_;
};

// ============================================================================
// Schema Comparison Options Dialog
// ============================================================================
class SchemaCompareOptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SchemaCompareOptionsDialog(QWidget* parent = nullptr);

public slots:
    void onSave();

private:
    void setupUi();
    
    QCheckBox* ignoreCaseCheck_ = nullptr;
    QCheckBox* ignoreWhitespaceCheck_ = nullptr;
    QCheckBox* ignoreCommentsCheck_ = nullptr;
    QCheckBox* ignoreTablespaceCheck_ = nullptr;
    QCheckBox* ignoreOwnershipCheck_ = nullptr;
    QCheckBox* ignoreGrantCheck_ = nullptr;
    QCheckBox* compareExtendedStatsCheck_ = nullptr;
};

// ============================================================================
// Sync Script Preview Dialog
// ============================================================================
class SchemaSyncScriptDialog : public QDialog {
    Q_OBJECT

public:
    explicit SchemaSyncScriptDialog(const QString& script, QWidget* parent = nullptr);

public slots:
    void onApply();
    void onSave();
    void onCopy();

private:
    void setupUi();
    
    QTextEdit* scriptEdit_ = nullptr;
};

} // namespace scratchrobin::ui
