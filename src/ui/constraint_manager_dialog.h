#pragma once

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

struct ConstraintManagerDefinition {
    QString name;
    QString tableName;
    QString schema;
    QString type; // "PRIMARY KEY", "FOREIGN KEY", "UNIQUE", "CHECK"
    QString expression;
    QList<QString> columns;
    QString referencedTable;
    QString referencedColumn;
    QString onDelete; // "CASCADE", "SET NULL", "RESTRICT", etc.
    QString onUpdate;
    bool isEnabled = true;
    QString comment;
    QMap<QString, QVariant> options;
};

class ConstraintManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConstraintManagerDialog(QWidget* parent = nullptr);
    ~ConstraintManagerDialog() override = default;

    // Public interface
    void setConstraintDefinition(const ConstraintManagerDefinition& definition);
    ConstraintManagerDefinition getConstraintDefinition() const;
    void setEditMode(bool isEdit);
    void setDatabaseType(DatabaseType type);
    void setTableInfo(const QString& schema, const QString& tableName);
    void loadExistingConstraint(const QString& schema, const QString& tableName, const QString& constraintName);

signals:
    void constraintSaved(const ConstraintManagerDefinition& definition);
    void constraintCreated(const QString& sql);
    void constraintAltered(const QString& sql);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Constraint settings
    void onConstraintNameChanged(const QString& name);
    void onConstraintTypeChanged(int index);
    void onColumnSelectionChanged();

    // Column management
    void onAddColumn();
    void onAddColumn(const QString& columnName);
    void onRemoveColumn();
    void onColumnSelectionChanged(int row);

    // Foreign key settings
    void onReferencedTableChanged(const QString& table);
    void onReferencedColumnChanged(const QString& column);
    void onOnDeleteChanged(int index);
    void onOnUpdateChanged(int index);

    // Actions
    void onGenerateSQL();
    void onPreviewSQL();
    void onValidateConstraint();
    void onAnalyzeConstraint();

private:
    void setupUI();
    void setupBasicTab();
    void setupColumnsTab();
    void setupForeignKeyTab();
    void setupAdvancedTab();
    void setupSQLTab();

    void setupColumnTable();
    void setupConstraintDialog();

    void updateColumnTable();
    void populateConstraintTypes();
    void populateColumns();
    void populateTables();
    void populateActions();

    bool validateConstraint();
    QString generateCreateSQL() const;
    QString generateDropSQL() const;
    QString generateAlterSQL() const;

    void updateUIForConstraintType();
    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QLineEdit* constraintNameEdit_;
    QLineEdit* tableNameEdit_;
    QLineEdit* schemaEdit_;
    QComboBox* constraintTypeCombo_;
    QTextEdit* expressionEdit_;
    QTextEdit* commentEdit_;

    // Columns tab
    QWidget* columnsTab_;
    QVBoxLayout* columnsLayout_;
    QTableWidget* columnsTable_;
    QHBoxLayout* columnsButtonLayout_;
    QPushButton* addColumnButton_;
    QPushButton* removeColumnButton_;
    QListWidget* availableColumnsList_;

    // Foreign Key tab
    QWidget* foreignKeyTab_;
    QFormLayout* foreignKeyLayout_;
    QLineEdit* referencedTableEdit_;
    QLineEdit* referencedColumnEdit_;
    QComboBox* onDeleteCombo_;
    QComboBox* onUpdateCombo_;

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* optionsGroup_;
    QFormLayout* optionsLayout_;
    QCheckBox* enabledCheck_;

    // SQL tab
    QWidget* sqlTab_;
    QVBoxLayout* sqlLayout_;
    QTextEdit* sqlPreviewEdit_;
    QPushButton* generateSqlButton_;
    QPushButton* validateSqlButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    ConstraintManagerDefinition currentDefinition_;
    DatabaseType currentDatabaseType_;
    bool isEditMode_;
    QString originalConstraintName_;
    QStringList availableColumns_;
    QStringList availableTables_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
