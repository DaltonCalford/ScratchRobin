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
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMap>
#include <QVariant>
#include <QTimer>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QListWidget>
#include <QTreeWidget>

#include "database/database_driver_manager.h"

namespace scratchrobin {

struct FunctionDefinition {
    QString name;
    QString schema;
    QString returnType;
    QList<ParameterDefinition> parameters;
    QString body;
    QString language; // "SQL", "PLSQL", "T-SQL", etc.
    QString comment;
    bool isDeterministic = false;
    QString securityType; // "DEFINER", "INVOKER"
    QString sqlMode;
    QMap<QString, QVariant> options;
};

struct ParameterDefinition {
    QString name;
    QString dataType;
    int length = 0;
    int precision = 0;
    int scale = 0;
    QString direction; // "IN", "OUT", "INOUT"
    QString defaultValue;
    QString comment;
};

class FunctionEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit FunctionEditorDialog(QWidget* parent = nullptr);
    ~FunctionEditorDialog() override = default;

    // Public interface
    void setFunctionDefinition(const FunctionDefinition& definition);
    FunctionDefinition getFunctionDefinition() const;
    void setEditMode(bool isEdit);
    void setDatabaseType(DatabaseType type);
    void loadExistingFunction(const QString& schema, const QString& functionName);

signals:
    void functionSaved(const FunctionDefinition& definition);
    void functionCreated(const QString& sql);
    void functionAltered(const QString& sql);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Parameter management
    void onAddParameter();
    void onEditParameter();
    void onDeleteParameter();
    void onMoveParameterUp();
    void onMoveParameterDown();
    void onParameterSelectionChanged();

    // Editor actions
    void onFormatSQL();
    void onValidateSQL();
    void onPreviewSQL();
    void onGenerateTemplate();

    // Template management
    void onLoadTemplate();
    void onSaveTemplate();

    // Settings
    void onLanguageChanged(int index);
    void onSecurityTypeChanged(int index);
    void onFunctionNameChanged(const QString& name);
    void onReturnTypeChanged(int index);

private:
    void setupUI();
    void setupBasicTab();
    void setupParametersTab();
    void setupEditorTab();
    void setupAdvancedTab();
    void setupSQLTab();

    void setupParameterTable();
    void setupParameterDialog();
    void setupCodeEditor();

    void updateParameterTable();
    void populateDataTypes();
    void populateLanguages();
    void populateTemplates();

    bool validateFunction();
    QString generateCreateSQL() const;
    QString generateAlterSQL() const;
    QString generateDropSQL() const;

    void loadParameterToDialog(int row);
    void saveParameterFromDialog();
    void clearParameterDialog();

    void applyTemplate(const QString& templateName);
    void saveAsTemplate(const QString& templateName);

    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QLineEdit* functionNameEdit_;
    QLineEdit* schemaEdit_;
    QComboBox* returnTypeCombo_;
    QComboBox* languageCombo_;
    QTextEdit* commentEdit_;

    // Parameters tab
    QWidget* parametersTab_;
    QVBoxLayout* parametersLayout_;
    QTableWidget* parametersTable_;
    QHBoxLayout* parametersButtonLayout_;
    QPushButton* addParameterButton_;
    QPushButton* editParameterButton_;
    QPushButton* deleteParameterButton_;
    QPushButton* moveUpButton_;
    QPushButton* moveDownButton_;

    // Parameter edit dialog (embedded)
    QGroupBox* parameterGroup_;
    QFormLayout* parameterLayout_;
    QLineEdit* paramNameEdit_;
    QComboBox* paramDataTypeCombo_;
    QSpinBox* paramLengthSpin_;
    QSpinBox* paramPrecisionSpin_;
    QSpinBox* paramScaleSpin_;
    QComboBox* paramDirectionCombo_;
    QLineEdit* paramDefaultEdit_;
    QTextEdit* paramCommentEdit_;

    // Editor tab
    QWidget* editorTab_;
    QVBoxLayout* editorLayout_;
    QHBoxLayout* editorToolbar_;
    QPushButton* formatButton_;
    QPushButton* validateButton_;
    QPushButton* previewButton_;
    QPushButton* templateButton_;
    QMenu* templateMenu_;
    QPlainTextEdit* codeEditor_;

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* optionsGroup_;
    QFormLayout* optionsLayout_;
    QCheckBox* deterministicCheck_;
    QComboBox* securityTypeCombo_;
    QLineEdit* sqlModeEdit_;

    // SQL tab
    QWidget* sqlTab_;
    QVBoxLayout* sqlLayout_;
    QTextEdit* sqlPreviewEdit_;
    QPushButton* generateSqlButton_;
    QPushButton* validateSqlButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    FunctionDefinition currentDefinition_;
    DatabaseType currentDatabaseType_;
    bool isEditMode_;
    QString originalFunctionName_;
    QString originalSchema_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
