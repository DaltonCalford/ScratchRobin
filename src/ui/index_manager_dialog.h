#pragma once

#include <QObject>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMap>
#include <QVariant>
#include <QTimer>
#include <QProgressBar>
#include <QScrollArea>
#include <QSplitter>
#include <QListWidget>

#include "database/database_driver_manager.h"

namespace scratchrobin {

struct IndexManagerColumn {
    QString column;
    QString expression;
    int length = 0;
    QString sortOrder; // "ASC", "DESC"
};

struct IndexManagerDefinition {
    QString name;
    QString tableName;
    QString schema;
    QString type; // "INDEX", "UNIQUE", "PRIMARY KEY", "FULLTEXT", "SPATIAL"
    QString method; // "BTREE", "HASH", "RTREE", etc.
    QList<IndexManagerColumn> columns;
    QString parser; // For FULLTEXT indexes
    QString comment;
    bool isVisible = true;
    QMap<QString, QVariant> options;
};

class IndexManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit IndexManagerDialog(QWidget* parent = nullptr);
    ~IndexManagerDialog() override = default;

    // Public interface
    void setIndexDefinition(const IndexManagerDefinition& definition);
    IndexManagerDefinition getIndexDefinition() const;
    void setEditMode(bool isEdit);
    void setDatabaseType(DatabaseType type);
    void setTableInfo(const QString& schema, const QString& tableName);
    void loadExistingIndex(const QString& schema, const QString& tableName, const QString& indexName);

signals:
    void indexSaved(const IndexManagerDefinition& definition);
    void indexCreated(const QString& sql);
    void indexAltered(const QString& sql);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Column management
    void onAddColumn();
    void onEditColumn();
    void onRemoveColumn();
    void onMoveColumnUp();
    void onMoveColumnDown();
    void onColumnSelectionChanged();

    // Index settings
    void onIndexNameChanged(const QString& name);
    void onIndexTypeChanged(int index);
    void onMethodChanged(int index);

    // Actions
    void onGenerateSQL();
    void onPreviewSQL();
    void onValidateIndex();
    void onAnalyzeIndex();

private:
    void setupUI();
    void setupBasicTab();
    void setupColumnsTab();
    void setupAdvancedTab();
    void setupAnalysisTab();
    void setupSQLTab();

    void setupColumnTable();
    void setupColumnDialog();

    void updateColumnTable();
    void populateIndexTypes();
    void populateMethods();
    void populateColumns();
    void populateParsers();

    bool validateIndex();
    QString generateCreateSQL() const;
    QString generateDropSQL() const;
    QString generateAlterSQL() const;

    void loadColumnToDialog(int row);
    void saveColumnFromDialog();
    void clearColumnDialog();

    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QLineEdit* indexNameEdit_;
    QLineEdit* tableNameEdit_;
    QLineEdit* schemaEdit_;
    QComboBox* indexTypeCombo_;
    QComboBox* methodCombo_;
    QTextEdit* commentEdit_;

    // Columns tab
    QWidget* columnsTab_;
    QVBoxLayout* columnsLayout_;
    QTableWidget* columnsTable_;
    QHBoxLayout* columnsButtonLayout_;
    QPushButton* addColumnButton_;
    QPushButton* editColumnButton_;
    QPushButton* removeColumnButton_;
    QPushButton* moveUpButton_;
    QPushButton* moveDownButton_;

    // Column edit dialog (embedded)
    QGroupBox* columnGroup_;
    QFormLayout* columnLayout_;
    QComboBox* columnCombo_;
    QLineEdit* expressionEdit_;
    QSpinBox* lengthSpin_;
    QComboBox* sortOrderCombo_;

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* optionsGroup_;
    QFormLayout* optionsLayout_;
    QComboBox* parserCombo_;
    QCheckBox* visibleCheck_;

    // Analysis tab
    QWidget* analysisTab_;
    QVBoxLayout* analysisLayout_;
    QPushButton* analyzeButton_;
    QTextEdit* analysisResultEdit_;
    QLabel* analysisStatusLabel_;

    // SQL tab
    QWidget* sqlTab_;
    QVBoxLayout* sqlLayout_;
    QTextEdit* sqlPreviewEdit_;
    QPushButton* generateSqlButton_;
    QPushButton* validateButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    IndexManagerDefinition currentDefinition_;
    DatabaseType currentDatabaseType_;
    bool isEditMode_;
    QString originalIndexName_;
    QStringList availableColumns_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
