#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class QDateTimeEdit;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Schema Migration Tool - Versioned database migrations
 * 
 * Manage database schema changes with version control:
 * - Create migration scripts
 * - Version tracking
 * - Up/Down migrations
 * - Migration history
 * - Rollback support
 */

// ============================================================================
// Migration Info
// ============================================================================
struct MigrationInfo {
    QString id;
    QString name;
    QString description;
    QDateTime created;
    QDateTime applied;
    QString checksum;
    bool isApplied = false;
    bool isReverted = false;
    int executionTime = 0;
    QString appliedBy;
};

// ============================================================================
// Schema Migration Tool Panel
// ============================================================================
class SchemaMigrationToolPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SchemaMigrationToolPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Schema Migrations"); }
    QString panelCategory() const override { return "database"; }

public slots:
    void onInitMigrations();
    void onCreateMigration();
    void onEditMigration();
    void onDeleteMigration();
    void onRunMigrations();
    void onRollbackMigration();
    void onValidateMigrations();
    void onRepairMigrations();
    void onViewHistory();
    void onExportMigrations();
    void onImportMigrations();
    void onRefresh();
    void onMigrationSelected(const QModelIndex& index);

signals:
    void migrationsApplied(int count);
    void migrationsRolledBack(int count);

private:
    void setupUi();
    void setupModel();
    void loadMigrations();
    void updateStats();
    
    backend::SessionClient* client_;
    
    // Migration table
    QTreeView* migrationTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    
    // Details
    QTabWidget* detailsTabs_ = nullptr;
    QTextEdit* upScriptEdit_ = nullptr;
    QTextEdit* downScriptEdit_ = nullptr;
    QTextEdit* infoEdit_ = nullptr;
    
    // Stats
    QLabel* totalLabel_ = nullptr;
    QLabel* pendingLabel_ = nullptr;
    QLabel* appliedLabel_ = nullptr;
    QLabel* currentVersionLabel_ = nullptr;
    
    // Buttons
    QPushButton* runBtn_ = nullptr;
    QPushButton* rollbackBtn_ = nullptr;
};

// ============================================================================
// Create Migration Dialog
// ============================================================================
class CreateMigrationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateMigrationDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onGenerateFromDiff();
    void onPreview();
    void onSave();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QTextEdit* upScriptEdit_ = nullptr;
    QTextEdit* downScriptEdit_ = nullptr;
    QCheckBox* autoGenerateCheck_ = nullptr;
};

// ============================================================================
// Run Migrations Dialog
// ============================================================================
class RunMigrationsDialog : public QDialog {
    Q_OBJECT

public:
    explicit RunMigrationsDialog(const QList<MigrationInfo>& pendingMigrations, 
                                 QWidget* parent = nullptr);

public slots:
    void onRun();
    void onDryRun();

private:
    void setupUi();
    
    QList<MigrationInfo> pendingMigrations_;
    
    QTreeView* migrationTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QCheckBox* validateCheck_ = nullptr;
    QCheckBox* backupCheck_ = nullptr;
};

// ============================================================================
// Rollback Dialog
// ============================================================================
class RollbackDialog : public QDialog {
    Q_OBJECT

public:
    explicit RollbackDialog(const QList<MigrationInfo>& appliedMigrations,
                           QWidget* parent = nullptr);

public slots:
    void onRollback();
    void onRollbackToVersion();

private:
    void setupUi();
    
    QList<MigrationInfo> appliedMigrations_;
    
    QComboBox* targetVersionCombo_ = nullptr;
    QCheckBox* validateCheck_ = nullptr;
    QCheckBox* backupCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Migration History Dialog
// ============================================================================
class MigrationHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit MigrationHistoryDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExport();
    void onFilterByDate();

private:
    void setupUi();
    void loadHistory();
    
    backend::SessionClient* client_;
    
    QTableView* historyTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QDateTimeEdit* fromDateEdit_ = nullptr;
    QDateTimeEdit* toDateEdit_ = nullptr;
};

} // namespace scratchrobin::ui
