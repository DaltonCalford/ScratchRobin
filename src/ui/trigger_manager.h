#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QCheckBox;
class QListWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Trigger Manager - Database triggers management
 * 
 * - List all triggers
 * - Create/edit/drop triggers
 * - Enable/disable triggers
 * - View trigger execution history
 */

// ============================================================================
// Trigger Info
// ============================================================================
struct TriggerInfo {
    QString name;
    QString table;
    QString schema;
    QString timing;  // BEFORE, AFTER, INSTEAD OF
    QString event;   // INSERT, UPDATE, DELETE, TRUNCATE
    QString definition;
    bool isEnabled = true;
    bool isSystem = false;
    QDateTime created;
    QString comment;
};

// ============================================================================
// Trigger Manager Panel
// ============================================================================
class TriggerManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit TriggerManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Trigger Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateTrigger();
    void onEditTrigger();
    void onDropTrigger();
    void onEnableTrigger();
    void onDisableTrigger();
    void onShowTriggerCode();
    void onShowExecutionHistory();

signals:
    void triggerSelected(const QString& triggerName);
    void triggerEditRequested(const QString& triggerName);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadTriggers();
    void updateTriggerDetails();
    
    backend::SessionClient* client_;
    
    QTreeView* triggerTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QTextEdit* codePreview_ = nullptr;
    QPushButton* enableBtn_ = nullptr;
    QPushButton* disableBtn_ = nullptr;
};

// ============================================================================
// Trigger Designer Dialog
// ============================================================================
class TriggerDesignerDialog : public QDialog {
    Q_OBJECT

public:
    explicit TriggerDesignerDialog(backend::SessionClient* client,
                                  const QString& triggerName = QString(),
                                  const QString& tableName = QString(),
                                  QWidget* parent = nullptr);

    QString generatedSql() const;

public slots:
    void onValidate();
    void onSave();
    void onFormatCode();
    void onToggleAdvanced();
    void updatePreview();

private:
    void setupUi();
    void setupBasicTab();
    void setupAdvancedTab();
    void setupPreviewTab();
    void loadTrigger(const QString& triggerName);
    
    backend::SessionClient* client_;
    QString originalTriggerName_;
    bool isEditing_ = false;
    
    // Basic tab
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* tableCombo_ = nullptr;
    QComboBox* timingCombo_ = nullptr;  // BEFORE, AFTER, INSTEAD OF
    QListWidget* eventsList_ = nullptr;  // INSERT, UPDATE, DELETE
    QTextEdit* codeEdit_ = nullptr;
    
    // Advanced tab
    QCheckBox* forEachRowCheck_ = nullptr;
    QCheckBox* whenClauseCheck_ = nullptr;
    QLineEdit* whenClauseEdit_ = nullptr;
    QCheckBox* deferrableCheck_ = nullptr;
    QComboBox* initiallyCombo_ = nullptr;
    QLineEdit* commentEdit_ = nullptr;
    
    // Preview
    QTextEdit* sqlPreview_ = nullptr;
};

// ============================================================================
// Trigger Execution History Dialog
// ============================================================================
class TriggerHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit TriggerHistoryDialog(backend::SessionClient* client,
                                 const QString& triggerName = QString(),
                                 QWidget* parent = nullptr);

private slots:
    void onRefresh();
    void onFilter();
    void onClear();
    void onExport();

private:
    void setupUi();
    void loadHistory();
    
    backend::SessionClient* client_;
    QString triggerName_;
    
    QTableView* historyTable_ = nullptr;
    QComboBox* triggerFilter_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

} // namespace scratchrobin::ui
