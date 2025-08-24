#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTabWidget>
#include <QTableWidget>

namespace scratchrobin {

enum class ColumnDataType {
    VARCHAR,
    TEXT,
    INTEGER,
    BIGINT,
    SMALLINT,
    DECIMAL,
    NUMERIC,
    FLOAT,
    DOUBLE,
    BOOLEAN,
    DATE,
    TIME,
    DATETIME,
    TIMESTAMP,
    BINARY,
    BLOB,
    JSON,
    JSONB,
    UUID,
    SERIAL,
    BIGSERIAL,
    SMALLSERIAL
};

struct ColumnEditorDefinition {
    QString name;
    ColumnDataType dataType;
    int length = -1;           // For VARCHAR, BINARY, etc.
    int precision = -1;        // For DECIMAL, NUMERIC
    int scale = -1;            // For DECIMAL, NUMERIC
    bool nullable = true;
    QString defaultValue;
    bool autoIncrement = false;
    QString collation;
    QString comment;

    // Constraints
    bool primaryKey = false;
    bool unique = false;
    QString checkConstraint;
    QString foreignKeyTable;
    QString foreignKeyColumn;
    QString onDeleteAction = "NO ACTION";
    QString onUpdateAction = "NO ACTION";

    // Advanced options
    bool compressed = false;
    QString storageType = "DEFAULT";
    QString characterSet;
};

struct ColumnEditorResult {
    ColumnEditorDefinition definition;
    bool isNewColumn = true;
    bool applyToAll = false;
};

class ColumnEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColumnEditorDialog(QWidget* parent = nullptr);
    ~ColumnEditorDialog() override = default;

    void setColumnDefinition(const ColumnEditorDefinition& definition, bool isNewColumn = true);
    ColumnEditorDefinition getColumnDefinition() const;
    bool isNewColumn() const { return isNewColumn_; }

    static ColumnEditorResult showColumnEditor(QWidget* parent,
                                             const ColumnEditorDefinition& initial = ColumnEditorDefinition(),
                                             bool isNew = true);

signals:
    void columnDefinitionChanged(const ColumnEditorDefinition& definition);

private slots:
    void onDataTypeChanged(int index);
    void onColumnNameChanged(const QString& text);
    void onNullableChanged(bool checked);
    void onPrimaryKeyChanged(bool checked);
    void onForeignKeyChanged(bool checked);
    void onDefaultValueChanged(const QString& text);
    void onLengthChanged(int value);
    void onPrecisionChanged(int value);
    void onScaleChanged(int value);
    void onValidateColumn();
    void onGenerateSQL();
    void onSave();
    void onCancel();

private:
    void setupUI();
    void setupBasicTab();
    void setupAdvancedTab();
    void setupConstraintsTab();
    void setupPreviewTab();

    void updateDataTypeOptions();
    void updateUIForDataType(ColumnDataType dataType);
    void updatePreview();
    bool validateColumnDefinition();
    QString generateColumnSQL() const;
public:
    QString getDataTypeString(ColumnDataType type) const;

    // UI Components
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QLineEdit* columnNameEdit_;
    QComboBox* dataTypeCombo_;
    QSpinBox* lengthSpin_;
    QSpinBox* precisionSpin_;
    QSpinBox* scaleSpin_;
    QCheckBox* nullableCheck_;
    QLineEdit* defaultValueEdit_;
    QLineEdit* commentEdit_;

    // Advanced tab
    QWidget* advancedTab_;
    QComboBox* collationCombo_;
    QComboBox* characterSetCombo_;
    QComboBox* storageTypeCombo_;
    QCheckBox* compressedCheck_;
    QCheckBox* autoIncrementCheck_;

    // Constraints tab
    QWidget* constraintsTab_;
    QCheckBox* primaryKeyCheck_;
    QCheckBox* uniqueCheck_;
    QLineEdit* checkConstraintEdit_;
    QGroupBox* foreignKeyGroup_;
    QLineEdit* foreignKeyTableEdit_;
    QLineEdit* foreignKeyColumnEdit_;
    QComboBox* onDeleteActionCombo_;
    QComboBox* onUpdateActionCombo_;

    // Preview tab
    QWidget* previewTab_;
    QTextEdit* previewTextEdit_;
    QLabel* validationLabel_;

    // Action buttons
    QPushButton* saveButton_;
    QPushButton* cancelButton_;
    QPushButton* validateButton_;
    QPushButton* generateSQLButton_;

    // Current state
    ColumnEditorDefinition currentDefinition_;
    bool isNewColumn_;
    bool isValid_ = false;
};

} // namespace scratchrobin
