#pragma once
#include <QDialog>
#include <QHash>
#include <memory>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QTableView;
class QStandardItemModel;
class QPlainTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QListWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Table Designer Dialog (4 tabs)
 * 
 * Implements FORM_SPECIFICATION.md Table Designer section:
 * - Columns tab: Field name, type, length, default, null, PK
 * - Constraints tab: Foreign keys, check constraints
 * - Indexes tab: Index name, type, columns
 * - SQL Preview tab: Generated CREATE/ALTER TABLE
 */
class TableDesignerDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode {
        Create,  // Create new table
        Alter    // Alter existing table
    };

    explicit TableDesignerDialog(Mode mode, QWidget* parent = nullptr);
    ~TableDesignerDialog() override;

    void setTableName(const QString& name);
    void setExistingTable(const QString& tableName, const QString& schema = QString());

    QString generatedSql() const;

public slots:
    void accept() override;

private slots:
    void onTabChanged(int index);
    void updateSqlPreview();

private:
    void setupUi();
    void setupTabs();

    Mode mode_;
    QString tableName_;
    QString schema_;
    
    QTabWidget* tabWidget_ = nullptr;
    QPlainTextEdit* sqlPreview_ = nullptr;
    QPushButton* applyBtn_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
};

// ============================================================================
// Columns Tab
// ============================================================================
class ColumnsTab : public QWidget {
    Q_OBJECT

public:
    explicit ColumnsTab(QWidget* parent = nullptr);

    struct ColumnDef {
        QString name;
        QString type;
        int length;
        int precision;
        int scale;
        QString defaultValue;
        bool nullable;
        bool primaryKey;
        bool autoIncrement;
        bool unique;
        QString collation;
        QString comment;
    };

    QList<ColumnDef> columns() const;
    void setColumns(const QList<ColumnDef>& columns);
    
    void addColumn();
    void removeSelectedColumn();
    void moveColumnUp();
    void moveColumnDown();

signals:
    void columnsChanged();

private slots:
    void onCellChanged();
    void onSelectionChanged();
    void onTypeChanged(int row);

private:
    void setupUi();
    void populateTypeCombo(QComboBox* combo);
    void updateButtonStates();

    QTableView* tableView_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    
    QPushButton* addBtn_ = nullptr;
    QPushButton* removeBtn_ = nullptr;
    QPushButton* upBtn_ = nullptr;
    QPushButton* downBtn_ = nullptr;
    
    // Row -> type combo mapping
    QHash<int, QComboBox*> typeCombos_;
};

// ============================================================================
// Constraints Tab
// ============================================================================
class ConstraintsTab : public QWidget {
    Q_OBJECT

public:
    explicit ConstraintsTab(QWidget* parent = nullptr);

    struct ForeignKey {
        QString name;
        QStringList columns;
        QString refTable;
        QStringList refColumns;
        QString onDelete;
        QString onUpdate;
    };

    struct CheckConstraint {
        QString name;
        QString expression;
    };

    QList<ForeignKey> foreignKeys() const;
    QList<CheckConstraint> checkConstraints() const;

    void setForeignKeys(const QList<ForeignKey>& keys);
    void setCheckConstraints(const QList<CheckConstraint>& constraints);

signals:
    void constraintsChanged();

private slots:
    void onAddForeignKey();
    void onEditForeignKey();
    void onRemoveForeignKey();
    void onAddCheck();
    void onEditCheck();
    void onRemoveCheck();

private:
    void setupUi();

    QTableView* fkTable_ = nullptr;
    QStandardItemModel* fkModel_ = nullptr;
    
    QTableView* checkTable_ = nullptr;
    QStandardItemModel* checkModel_ = nullptr;
};

// ============================================================================
// Indexes Tab
// ============================================================================
class IndexesTab : public QWidget {
    Q_OBJECT

public:
    explicit IndexesTab(QWidget* parent = nullptr);

    struct IndexDef {
        QString name;
        QString type;  // UNIQUE, INDEX, FULLTEXT, SPATIAL
        bool clustered;
        QList<QPair<QString, bool>> columns;  // column name, ascending
    };

    QList<IndexDef> indexes() const;
    void setIndexes(const QList<IndexDef>& indexes);

signals:
    void indexesChanged();

private slots:
    void onAddIndex();
    void onEditIndex();
    void onRemoveIndex();
    void onIndexSelectionChanged();

private:
    void setupUi();

    QTableView* indexTable_ = nullptr;
    QStandardItemModel* indexModel_ = nullptr;
    
    QPushButton* addBtn_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* removeBtn_ = nullptr;
};

// ============================================================================
// SQL Preview Tab
// ============================================================================
class SqlPreviewTab : public QWidget {
    Q_OBJECT

public:
    explicit SqlPreviewTab(QWidget* parent = nullptr);

    void setSql(const QString& sql);
    QString sql() const;

    void setReadOnly(bool readOnly);

private:
    void setupUi();

    QPlainTextEdit* sqlEdit_ = nullptr;
};

// ============================================================================
// Index Editor Dialog
// ============================================================================
class IndexEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit IndexEditorDialog(QWidget* parent = nullptr);

    void setIndex(const IndexesTab::IndexDef& index);
    IndexesTab::IndexDef getIndex() const;

    void setAvailableColumns(const QStringList& columns);

private:
    void setupUi();
    void updateColumnList();

    QLineEdit* nameEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QCheckBox* clusteredCheck_ = nullptr;
    QTableView* columnsTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    
    QStringList availableColumns_;
};

// ============================================================================
// Foreign Key Editor Dialog
// ============================================================================
class ForeignKeyEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ForeignKeyEditorDialog(QWidget* parent = nullptr);

    void setForeignKey(const ConstraintsTab::ForeignKey& fk);
    ConstraintsTab::ForeignKey foreignKey() const;

    void setAvailableColumns(const QStringList& columns);
    void setAvailableTables(const QStringList& tables);

private:
    void setupUi();

    QLineEdit* nameEdit_ = nullptr;
    QListWidget* columnList_ = nullptr;
    QComboBox* refTableCombo_ = nullptr;
    QListWidget* refColumnList_ = nullptr;
    QComboBox* onDeleteCombo_ = nullptr;
    QComboBox* onUpdateCombo_ = nullptr;
};

} // namespace scratchrobin::ui
