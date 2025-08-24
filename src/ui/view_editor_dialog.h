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

#include "database/database_driver_manager.h"

namespace scratchrobin {

struct ViewDefinition {
    QString name;
    QString schema;
    QString definition;
    QList<QString> referencedTables;
    QString checkOption; // "CASCADE", "LOCAL", "NONE"
    QString securityType; // "DEFINER", "INVOKER"
    QString algorithm; // "MERGE", "TEMPTABLE", "UNDEFINED"
    QString sqlSecurity; // "DEFINER", "INVOKER"
    QString comment;
    QMap<QString, QVariant> options;
};

class ViewEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ViewEditorDialog(QWidget* parent = nullptr);
    ~ViewEditorDialog() override = default;

    // Public interface
    void setViewDefinition(const ViewDefinition& definition);
    ViewDefinition getViewDefinition() const;
    void setEditMode(bool isEdit);
    void setDatabaseType(DatabaseType type);
    void loadExistingView(const QString& schema, const QString& viewName);

signals:
    void viewSaved(const ViewDefinition& definition);
    void viewCreated(const QString& sql);
    void viewAltered(const QString& sql);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Editor actions
    void onFormatSQL();
    void onValidateSQL();
    void onPreviewSQL();
    void onGenerateTemplate();

    // Analysis
    void onAnalyzeDependencies();
    void onShowReferencedTables();

    // Settings
    void onViewNameChanged(const QString& name);
    void onAlgorithmChanged(int index);
    void onSecurityTypeChanged(int index);
    void onCheckOptionChanged(int index);

private:
    void setupUI();
    void setupBasicTab();
    void setupEditorTab();
    void setupAdvancedTab();
    void setupDependenciesTab();
    void setupSQLTab();

    void setupCodeEditor();
    void setupDependenciesTable();
    void populateTemplates();

    bool validateView();
    QString generateCreateSQL() const;
    QString generateAlterSQL() const;
    QString generateDropSQL() const;

    void parseViewDefinition();
    void analyzeDependencies();

    void applyTemplate(const QString& templateName);
    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QLineEdit* viewNameEdit_;
    QLineEdit* schemaEdit_;
    QComboBox* algorithmCombo_;
    QComboBox* securityTypeCombo_;
    QComboBox* checkOptionCombo_;
    QTextEdit* commentEdit_;

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
    QTextEdit* definitionEdit_; // Alternative to code editor

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* optionsGroup_;
    QFormLayout* optionsLayout_;

    // Dependencies tab
    QWidget* dependenciesTab_;
    QVBoxLayout* dependenciesLayout_;
    QListWidget* referencedTablesList_;
    QPushButton* analyzeButton_;
    QPushButton* showTablesButton_;
    QLabel* dependencyStatusLabel_;

    // SQL tab
    QWidget* sqlTab_;
    QVBoxLayout* sqlLayout_;
    QTextEdit* sqlPreviewEdit_;
    QPushButton* generateSqlButton_;
    QPushButton* validateSqlButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    ViewDefinition currentDefinition_;
    DatabaseType currentDatabaseType_;
    bool isEditMode_;
    QString originalViewName_;
    QString originalSchema_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
