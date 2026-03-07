#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTableWidget;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QSplitter;
class QLabel;
class QSpinBox;
class QCheckBox;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Partition Manager - Table partitioning administration
 * 
 * Supports range, list, and hash partitioning strategies.
 * - Create partitioned tables
 * - Manage partitions (add, drop, split, merge)
 * - Partition pruning and statistics
 */

// ============================================================================
// Partition Info
// ============================================================================
struct PartitionInfo {
    QString name;
    QString parentTable;
    QString schema;
    QString partitionType;  // RANGE, LIST, HASH
    QString partitionKey;
    QString partitionExpression;
    QVariant fromValue;
    QVariant toValue;
    QStringList listValues;
    int hashModulus = 0;
    int hashRemainder = 0;
    qint64 rowCount = 0;
    qint64 sizeBytes = 0;
    bool isDefault = false;
    bool isAttached = true;
    QDateTime created;
};

// ============================================================================
// Partition Manager Panel
// ============================================================================
class PartitionManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit PartitionManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Partition Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreatePartitionedTable();
    void onAddPartition();
    void onDropPartition();
    void onSplitPartition();
    void onMergePartitions();
    void onAttachPartition();
    void onDetachPartition();
    void onAnalyzePartition();
    void onFilterChanged(const QString& filter);
    void onPartitionSelected(const QModelIndex& index);

signals:
    void partitionSelected(const QString& tableName, const QString& partitionName);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadPartitionedTables();
    void loadPartitions(const QString& tableName);
    void updateDetails(const PartitionInfo& info);
    
    backend::SessionClient* client_;
    
    QTreeView* tableTree_ = nullptr;
    QTreeView* partitionTree_ = nullptr;
    QStandardItemModel* tableModel_ = nullptr;
    QStandardItemModel* partitionModel_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    
    // Details panel
    QLabel* tableLabel_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLabel* keyLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QLabel* sizeLabel_ = nullptr;
    QTextEdit* ddlPreview_ = nullptr;
};

// ============================================================================
// Create Partitioned Table Dialog
// ============================================================================
class CreatePartitionedTableDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreatePartitionedTableDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onAddPartition();
    void onRemovePartition();
    void onPartitionTypeChanged(const QString& type);
    void onPreview();
    void onCreate();

private:
    void setupUi();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* tableNameEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* partitionTypeCombo_ = nullptr;
    QLineEdit* partitionKeyEdit_ = nullptr;
    QTableWidget* columnsTable_ = nullptr;
    QTableWidget* partitionsTable_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Add Partition Dialog
// ============================================================================
class AddPartitionDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddPartitionDialog(backend::SessionClient* client,
                               const QString& parentTable,
                               const QString& partitionType,
                               QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onAdd();

private:
    void setupUi();
    QString generateDdl();
    
    backend::SessionClient* client_;
    QString parentTable_;
    QString partitionType_;
    
    QLineEdit* partitionNameEdit_ = nullptr;
    QLineEdit* fromValueEdit_ = nullptr;
    QLineEdit* toValueEdit_ = nullptr;
    QLineEdit* listValuesEdit_ = nullptr;
    QCheckBox* defaultCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Split Partition Dialog
// ============================================================================
class SplitPartitionDialog : public QDialog {
    Q_OBJECT

public:
    explicit SplitPartitionDialog(backend::SessionClient* client,
                                 const QString& tableName,
                                 const QString& partitionName,
                                 const QString& partitionType,
                                 QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onSplit();

private:
    void setupUi();
    QString generateDdl();
    
    backend::SessionClient* client_;
    QString tableName_;
    QString partitionName_;
    QString partitionType_;
    
    QLineEdit* newPartition1Edit_ = nullptr;
    QLineEdit* newPartition2Edit_ = nullptr;
    QLineEdit* splitAtEdit_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

} // namespace scratchrobin::ui
