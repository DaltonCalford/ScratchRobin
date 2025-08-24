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

#include "database/database_driver_manager.h"

namespace scratchrobin {

struct TriggerDefinition {
    QString name;
    QString tableName;
    QString schema;
    QString timing; // "BEFORE", "AFTER"
    QString event; // "INSERT", "UPDATE", "DELETE"
    QString body;
    QString condition; // WHEN condition
    QString definer; // User who defined the trigger
    QString comment;
    bool isEnabled = true;
    QMap<QString, QVariant> options;
};

class TriggerManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit TriggerManagerDialog(QWidget* parent = nullptr);
    ~TriggerManagerDialog() override = default;

    // Public interface
    void setTriggerDefinition(const TriggerDefinition& definition);
    TriggerDefinition getTriggerDefinition() const;
    void setEditMode(bool isEdit);
    void setDatabaseType(DatabaseType type);
    void setTableInfo(const QString& schema, const QString& tableName);
    void loadExistingTrigger(const QString& schema, const QString& tableName, const QString& triggerName);

signals:
    void triggerSaved(const TriggerDefinition& definition);
    void triggerCreated(const QString& sql);
    void triggerAltered(const QString& sql);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Editor actions
    void onFormatSQL();
    void onValidateSQL();
    void onPreviewSQL();
    void onGenerateTemplate();

    // Settings
    void onTriggerNameChanged(const QString& name);
    void onTimingChanged(int index);
    void onEventChanged(int index);
    void onConditionChanged();

    // Template management
    void onLoadTemplate();
    void onSaveTemplate();

private:
    void setupUI();
    void setupBasicTab();
    void setupEditorTab();
    void setupAdvancedTab();
    void setupSQLTab();

    void setupCodeEditor();
    void populateTemplates();
    void populateTables();

    bool validateTrigger();
    QString generateCreateSQL() const;
    QString generateDropSQL() const;
    QString generateAlterSQL() const;

    void applyTemplate(const QString& templateName);
    void saveAsTemplate(const QString& templateName);

    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QLineEdit* triggerNameEdit_;
    QLineEdit* tableNameEdit_;
    QLineEdit* schemaEdit_;
    QComboBox* timingCombo_;
    QComboBox* eventCombo_;
    QLineEdit* conditionEdit_;
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

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* optionsGroup_;
    QFormLayout* optionsLayout_;
    QLineEdit* definerEdit_;
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
    TriggerDefinition currentDefinition_;
    DatabaseType currentDatabaseType_;
    bool isEditMode_;
    QString originalTriggerName_;
    QStringList availableTables_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
