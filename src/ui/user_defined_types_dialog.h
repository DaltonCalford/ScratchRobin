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
#include <QTreeWidget>

#include "database/database_driver_manager.h"

namespace scratchrobin {

// User-defined type definitions
struct CompositeField {
    QString name;
    QString dataType;
    int length = 0;
    int precision = 0;
    int scale = 0;
    QString defaultValue;
    QString comment;
};

struct EnumValue {
    QString value;
    QString comment;
};

struct UserDefinedType {
    QString name;
    QString schema;
    QString typeCategory; // "COMPOSITE", "ENUM", "BASE", "ARRAY", "DOMAIN"
    QString description;

    // For composite types
    QList<CompositeField> fields;

    // For enum types
    QList<EnumValue> enumValues;

    // For base types (extending existing types)
    QString baseType;
    QString inputFunction;
    QString outputFunction;

    // For array types
    QString elementType;

    // For domain types
    QString underlyingType;
    QString checkConstraint;
    QString domainDefault;
    bool notNull = false;

    QMap<QString, QVariant> options;
};

struct DomainDefinition {
    QString name;
    QString schema;
    QString baseType;
    QString checkConstraint;
    QString defaultValue;
    bool notNull = false;
    QString collation;
    QString comment;
    QMap<QString, QVariant> options;
};

class UserDefinedTypesDialog : public QDialog {
    Q_OBJECT

public:
    explicit UserDefinedTypesDialog(QWidget* parent = nullptr);
    ~UserDefinedTypesDialog() override = default;

    // Public interface
    void setUserDefinedType(const UserDefinedType& type);
    UserDefinedType getUserDefinedType() const;
    void setEditMode(bool isEdit);
    void setDatabaseType(DatabaseType type);
    void loadExistingType(const QString& schema, const QString& typeName);
    void refreshTypesList();

signals:
    void typeSaved(const UserDefinedType& type);
    void typeCreated(const QString& sql);
    void typeAltered(const QString& sql);
    void typeDropped(const QString& sql);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Type management
    void onCreateType();
    void onEditType();
    void onDeleteType();
    void onRefreshTypes();

    // Type selection
    void onTypeCategoryChanged(int index);
    void onBaseTypeChanged(int index);
    void onElementTypeChanged(int index);

    // Composite type fields
    void onAddField();
    void onEditField();
    void onRemoveField();
    void onMoveFieldUp();
    void onMoveFieldDown();

    // Enum values
    void onAddEnumValue();
    void onEditEnumValue();
    void onRemoveEnumValue();
    void onMoveEnumValueUp();
    void onMoveEnumValueDown();

    // Actions
    void onGenerateSQL();
    void onPreviewSQL();
    void onValidateType();
    void onAnalyzeUsage();

private:
    void setupUI();
    void setupBasicTab();
    void setupCompositeTab();
    void setupEnumTab();
    void setupAdvancedTab();
    void setupSQLTab();

    void setupFieldsTable();
    void setupEnumTable();
    void setupTypesTree();

    void updateUIForTypeCategory();
    void populateTypeCategories();
    void populateBaseTypes();
    void populateElementTypes();

    bool validateType();
    QString generateCreateSQL() const;
    QString generateDropSQL() const;
    QString generateAlterSQL() const;

    void loadFieldToDialog(int row);
    void saveFieldFromDialog();
    void clearFieldDialog();

    void loadEnumToDialog(int row);
    void saveEnumFromDialog();
    void clearEnumDialog();

    void updateButtonStates();

    // Additional setup methods
    void setupFieldDialog();
    void setupEnumDialog();

    // Update methods
    void updateFieldsTable();
    void updateEnumTable();

    // Utility methods
    void setTableInfo(const QString& schema, const QString& typeName);

    // UI Components
    QVBoxLayout* mainLayout_;
    QSplitter* mainSplitter_;

    // Left side - Types tree
    QWidget* leftWidget_;
    QVBoxLayout* leftLayout_;
    QHBoxLayout* toolbarLayout_;
    QPushButton* createTypeButton_;
    QPushButton* refreshButton_;
    QTreeWidget* typesTree_;

    // Right side - Type editor
    QWidget* rightWidget_;
    QVBoxLayout* rightLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QLineEdit* typeNameEdit_;
    QLineEdit* schemaEdit_;
    QComboBox* typeCategoryCombo_;
    QTextEdit* descriptionEdit_;

    // Composite tab
    QWidget* compositeTab_;
    QVBoxLayout* compositeLayout_;
    QTableWidget* fieldsTable_;
    QHBoxLayout* fieldsButtonLayout_;
    QPushButton* addFieldButton_;
    QPushButton* editFieldButton_;
    QPushButton* removeFieldButton_;
    QPushButton* moveFieldUpButton_;
    QPushButton* moveFieldDownButton_;

    // Field edit dialog
    QGroupBox* fieldGroup_;
    QFormLayout* fieldLayout_;
    QLineEdit* fieldNameEdit_;
    QComboBox* fieldTypeCombo_;
    QSpinBox* fieldLengthSpin_;
    QSpinBox* fieldPrecisionSpin_;
    QSpinBox* fieldScaleSpin_;
    QLineEdit* fieldDefaultEdit_;
    QTextEdit* fieldCommentEdit_;

    // Enum tab
    QWidget* enumTab_;
    QVBoxLayout* enumTabLayout_;
    QTableWidget* enumTable_;
    QHBoxLayout* enumButtonLayout_;
    QPushButton* addEnumButton_;
    QPushButton* editEnumButton_;
    QPushButton* removeEnumButton_;
    QPushButton* moveEnumUpButton_;
    QPushButton* moveEnumDownButton_;

    // Enum edit dialog
    QGroupBox* enumGroup_;
    QFormLayout* enumFormLayout_;
    QLineEdit* enumValueEdit_;
    QTextEdit* enumCommentEdit_;

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* advancedGroup_;
    QFormLayout* advancedFormLayout_;
    QComboBox* baseTypeCombo_;
    QComboBox* elementTypeCombo_;
    QLineEdit* inputFunctionEdit_;
    QLineEdit* outputFunctionEdit_;
    QTextEdit* checkConstraintEdit_;
    QLineEdit* domainDefaultEdit_;
    QCheckBox* notNullCheck_;

    // SQL tab
    QWidget* sqlTab_;
    QVBoxLayout* sqlLayout_;
    QTextEdit* sqlPreviewEdit_;
    QPushButton* generateSqlButton_;
    QPushButton* validateButton_;
    QPushButton* validateSqlButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    UserDefinedType currentDefinition_;
    DatabaseType currentDatabaseType_;
    bool isEditMode_;
    QString currentSchema_;
    QString currentTypeName_;
    QString originalTypeName_;
    QString originalSchema_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
