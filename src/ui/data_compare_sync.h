#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTreeView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QProgressBar;
class QLabel;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class QSplitter;
class QTabWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Data Compare/Sync Manager - Table comparison and synchronization
 * 
 * Compare and synchronize data between tables or databases.
 * - Schema comparison
 * - Row-level data comparison
 * - Generate sync scripts
 * - Bidirectional sync
 * - Conflict resolution
 */

// ============================================================================
// Compare Result Info
// ============================================================================
struct CompareResult {
    enum DiffType { Equal, Different, SourceOnly, TargetOnly };
    
    QString tableName;
    QString keyValue;
    DiffType type;
    QString columnName;
    QString sourceValue;
    QString targetValue;
};

// ============================================================================
// Data Compare/Sync Panel
// ============================================================================
class DataCompareSyncPanel : public DockPanel {
    Q_OBJECT

public:
    explicit DataCompareSyncPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Data Compare/Sync"); }
    QString panelCategory() const override { return "database"; }

public slots:
    void onSelectSourceTable();
    void onSelectTargetTable();
    void onRunComparison();
    void onGenerateSyncScript();
    void onApplySync();
    void onExportResults();
    void onSaveComparison();
    void onLoadComparison();
    void onFilterChanged(const QString& filter);

signals:
    void comparisonStarted();
    void comparisonProgress(int current, int total);
    void comparisonFinished(int differences, int onlyInSource, int onlyInTarget);

private:
    void setupUi();
    void setupModels();
    void runComparison();
    void displayResults();
    
    backend::SessionClient* client_;
    
    QTabWidget* tabWidget_ = nullptr;
    
    // Source/Target selection
    QComboBox* sourceTableCombo_ = nullptr;
    QComboBox* sourceSchemaCombo_ = nullptr;
    QComboBox* targetTableCombo_ = nullptr;
    QComboBox* targetSchemaCombo_ = nullptr;
    
    // Key columns
    QLineEdit* keyColumnsEdit_ = nullptr;
    QCheckBox* compareAllColumnsCheck_ = nullptr;
    QLineEdit* compareColumnsEdit_ = nullptr;
    
    // Options
    QCheckBox* ignoreCaseCheck_ = nullptr;
    QCheckBox* ignoreWhitespaceCheck_ = nullptr;
    QCheckBox* ignoreNullsCheck_ = nullptr;
    
    // Results
    QTreeView* resultTree_ = nullptr;
    QStandardItemModel* resultModel_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    
    // Stats
    QLabel* equalCountLabel_ = nullptr;
    QLabel* differentCountLabel_ = nullptr;
    QLabel* sourceOnlyCountLabel_ = nullptr;
    QLabel* targetOnlyCountLabel_ = nullptr;
    
    // Sync script
    QTextEdit* scriptEdit_ = nullptr;
    
    QList<CompareResult> compareResults_;
};

// ============================================================================
// Compare Options Dialog
// ============================================================================
class CompareOptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CompareOptionsDialog(QWidget* parent = nullptr);

public slots:
    void onSave();

private:
    void setupUi();
    
    QCheckBox* ignoreCaseCheck_ = nullptr;
    QCheckBox* ignoreWhitespaceCheck_ = nullptr;
    QCheckBox* ignoreNullsCheck_ = nullptr;
    QCheckBox* trimStringsCheck_ = nullptr;
    QCheckBox* roundNumbersCheck_ = nullptr;
    QSpinBox* decimalPlacesSpin_ = nullptr;
    QCheckBox* compareTimestampsCheck_ = nullptr;
    QSpinBox* timestampToleranceSpin_ = nullptr;
};

// ============================================================================
// Sync Script Preview Dialog
// ============================================================================
class SyncScriptPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit SyncScriptPreviewDialog(const QString& script, QWidget* parent = nullptr);

public slots:
    void onApply();
    void onSave();
    void onCopy();

private:
    void setupUi();
    
    QTextEdit* scriptEdit_ = nullptr;
};

// ============================================================================
// Table Mapping Dialog
// ============================================================================
class TableMappingDialog : public QDialog {
    Q_OBJECT

public:
    explicit TableMappingDialog(const QString& sourceTable, 
                                const QString& targetTable,
                                QWidget* parent = nullptr);

public slots:
    void onAutoMap();
    void onClearMapping();
    void onSave();

private:
    void setupUi();
    void loadColumns();
    
    QString sourceTable_;
    QString targetTable_;
    
    QTableView* mappingTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
};

} // namespace scratchrobin::ui
