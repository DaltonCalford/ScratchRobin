#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QTreeView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QSplitter;
class QProgressBar;
class QRadioButton;
class QSpinBox;
class QListWidget;
class QDateTimeEdit;
class QStackedWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Backup and Restore Management
 * 
 * Comprehensive backup and restore functionality:
 * - Full, incremental, and differential backups
 * - Point-in-time restore
 * - Backup scheduling and retention policies
 * - Backup verification and integrity checks
 */

// ============================================================================
// Backup Types
// ============================================================================
enum class BackupType {
    Full,
    Incremental,
    Differential,
    SchemaOnly,
    DataOnly
};

enum class BackupFormat {
    Custom,
    Plain,
    Directory,
    Tar
};

struct BackupJob {
    QString id;
    QString name;
    BackupType type;
    BackupFormat format;
    QString sourceDatabase;
    QString destinationPath;
    QDateTime scheduledTime;
    QDateTime completedTime;
    qint64 size = 0;
    QString status; // pending, running, completed, failed
    QString errorMessage;
    int compressionLevel = 6;
    bool includeBlobs = true;
    bool includeOids = false;
    bool clean = false;
    bool createDatabase = true;
    bool noOwner = false;
    bool noPrivileges = false;
    bool verbose = true;
    QStringList excludeTables;
    QStringList includeTables;
};

struct RestoreJob {
    QString id;
    QString backupId;
    QString targetDatabase;
    QString sourcePath;
    QDateTime restorePoint; // For point-in-time recovery
    bool createDatabase = true;
    bool clean = false;
    bool dataOnly = false;
    bool schemaOnly = false;
    bool singleTransaction = true;
    bool noDataForFailedTables = false;
    bool useSetSessionAuthorization = false;
    bool exitOnError = true;
    int jobs = 1; // Parallel jobs
    QString status;
    QString errorMessage;
    QDateTime startTime;
    QDateTime endTime;
};

// ============================================================================
// Backup Manager Panel
// ============================================================================
class BackupManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit BackupManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Backup & Restore"); }
    QString panelCategory() const override { return "maintenance"; }

public slots:
    // Backup operations
    void onCreateBackup();
    void onEditBackup();
    void onDeleteBackup();
    void onRunBackup();
    void onStopBackup();
    void onVerifyBackup();
    
    // Restore operations
    void onRestoreBackup();
    void onPointInTimeRestore();
    void onValidateBackup();
    
    // Backup list
    void onRefreshBackups();
    void onBackupSelected(const QModelIndex& index);
    void onSearchBackups(const QString& text);
    
    // Actions
    void onExportBackupList();
    void onImportBackup();
    void onShowBackupDetails();
    void onDeleteOldBackups();

signals:
    void backupStarted(const QString& backupId);
    void backupCompleted(const QString& backupId, bool success);
    void restoreStarted(const QString& restoreId);
    void restoreCompleted(const QString& restoreId, bool success);

private:
    void setupUi();
    void setupBackupList();
    void setupRestorePanel();
    void loadBackups();
    void updateBackupList();
    void updateBackupDetails(const BackupJob& backup);
    
    backend::SessionClient* client_;
    QList<BackupJob> backups_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QTableView* backupTable_ = nullptr;
    QStandardItemModel* backupModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    // Backup details
    QLabel* nameLabel_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLabel* databaseLabel_ = nullptr;
    QLabel* sizeLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* createdLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
};

// ============================================================================
// Backup Wizard
// ============================================================================
class BackupWizard : public QDialog {
    Q_OBJECT

public:
    explicit BackupWizard(BackupJob* job, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onNext();
    void onPrevious();
    void onBrowseDestination();
    void onToggleAllTables(bool checked);
    void onTableSelectionChanged();
    void onUpdateCommand();
    void onFinish();
    void onCancel();

private:
    void setupUi();
    void createWelcomePage();
    void createTypePage();
    void createOptionsPage();
    void createTablesPage();
    void createCommandPage();
    void updateCommandPreview();
    void loadTables();
    
    BackupJob* job_;
    backend::SessionClient* client_;
    
    QStackedWidget* pages_ = nullptr;
    QPushButton* nextBtn_ = nullptr;
    QPushButton* prevBtn_ = nullptr;
    
    // Type page
    QRadioButton* fullRadio_ = nullptr;
    QRadioButton* incrementalRadio_ = nullptr;
    QRadioButton* differentialRadio_ = nullptr;
    QRadioButton* schemaOnlyRadio_ = nullptr;
    QRadioButton* dataOnlyRadio_ = nullptr;
    
    // Options page
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* databaseCombo_ = nullptr;
    QLineEdit* destinationEdit_ = nullptr;
    QComboBox* formatCombo_ = nullptr;
    QSpinBox* compressionSpin_ = nullptr;
    QCheckBox* includeBlobsCheck_ = nullptr;
    QCheckBox* includeOidsCheck_ = nullptr;
    QCheckBox* noOwnerCheck_ = nullptr;
    QCheckBox* noPrivilegesCheck_ = nullptr;
    QCheckBox* verboseCheck_ = nullptr;
    
    // Tables page
    QListWidget* tablesList_ = nullptr;
    QListWidget* excludeList_ = nullptr;
    QCheckBox* allTablesCheck_ = nullptr;
    
    // Command page
    QTextEdit* commandEdit_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Restore Wizard
// ============================================================================
class RestoreWizard : public QDialog {
    Q_OBJECT

public:
    explicit RestoreWizard(const QString& backupPath, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onNext();
    void onPrevious();
    void onBrowseBackup();
    void onBrowseTarget();
    void onUpdateCommand();
    void onStartRestore();
    void onCancel();

private:
    void setupUi();
    void createSourcePage();
    void createTargetPage();
    void createOptionsPage();
    void createProgressPage();
    void updateCommandPreview();
    void loadBackupInfo();
    
    QString backupPath_;
    backend::SessionClient* client_;
    RestoreJob job_;
    
    QStackedWidget* pages_ = nullptr;
    QPushButton* nextBtn_ = nullptr;
    QPushButton* prevBtn_ = nullptr;
    
    // Source page
    QLineEdit* sourceEdit_ = nullptr;
    QLabel* backupInfoLabel_ = nullptr;
    QDateTimeEdit* pointInTimeEdit_ = nullptr;
    QCheckBox* pitrCheck_ = nullptr;
    
    // Target page
    QComboBox* targetDatabaseCombo_ = nullptr;
    QLineEdit* newDatabaseEdit_ = nullptr;
    QRadioButton* existingDbRadio_ = nullptr;
    QRadioButton* newDbRadio_ = nullptr;
    
    // Options page
    QCheckBox* cleanCheck_ = nullptr;
    QCheckBox* createDbCheck_ = nullptr;
    QCheckBox* dataOnlyCheck_ = nullptr;
    QCheckBox* schemaOnlyCheck_ = nullptr;
    QCheckBox* singleTransactionCheck_ = nullptr;
    QCheckBox* exitOnErrorCheck_ = nullptr;
    QSpinBox* jobsSpin_ = nullptr;
    QTextEdit* commandEdit_ = nullptr;
    
    // Progress page
    QProgressBar* progressBar_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Point-in-Time Recovery Dialog
// ============================================================================
class PointInTimeRecoveryDialog : public QDialog {
    Q_OBJECT

public:
    explicit PointInTimeRecoveryDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onBrowseWalArchive();
    void onSelectBackup();
    void onUpdateTimeline();
    void onStartRecovery();
    void onCancel();

private:
    void setupUi();
    void loadBackups();
    void validateRecovery();
    
    backend::SessionClient* client_;
    
    QComboBox* backupCombo_ = nullptr;
    QDateTimeEdit* recoveryTimeEdit_ = nullptr;
    QLineEdit* walArchiveEdit_ = nullptr;
    QLineEdit* targetDatabaseEdit_ = nullptr;
    QTextEdit* timelineEdit_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Backup Verification Dialog
// ============================================================================
class BackupVerificationDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackupVerificationDialog(const QString& backupPath, QWidget* parent = nullptr);

public slots:
    void onStartVerification();
    void onCancel();

private:
    void setupUi();
    void runVerification();
    
    QString backupPath_;
    
    QLabel* backupLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    QPushButton* verifyBtn_ = nullptr;
};

} // namespace scratchrobin::ui
