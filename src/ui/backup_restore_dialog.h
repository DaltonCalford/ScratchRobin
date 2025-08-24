#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTableWidget>

namespace scratchrobin {

struct BackupOptions {
    QString backupType = "Full"; // Full, Schema, Data
    QString filePath;
    QString format = "Custom"; // SQL, Custom, Compressed
    bool includeData = true;
    bool includeIndexes = true;
    bool includeConstraints = true;
    bool includeTriggers = true;
    bool includeViews = true;
    bool compressBackup = true;
    QString compressionLevel = "Medium"; // Low, Medium, High
    bool encryptBackup = false;
    QString password;
    bool verifyBackup = true;
    QString comment;
    QStringList selectedTables;
    QStringList selectedSchemas;
};

struct RestoreOptions {
    QString filePath;
    QString restoreMode = "Full"; // Full, Schema, Data
    bool dropExistingObjects = false;
    bool createSchemas = true;
    bool createTables = true;
    bool createIndexes = true;
    bool createConstraints = true;
    bool createTriggers = true;
    bool createViews = true;
    QString conflictResolution = "Skip"; // Skip, Overwrite, Rename
    bool ignoreErrors = false;
    bool previewOnly = false;
    QString password; // For encrypted backups
};

struct BackupInfo {
    QString fileName;
    QString filePath;
    QDateTime createdAt;
    qint64 fileSize;
    QString backupType;
    QString databaseName;
    QString comment;
    bool isCompressed;
    bool isEncrypted;
    bool isVerified;
};

class BackupRestoreDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackupRestoreDialog(QWidget* parent = nullptr);
    ~BackupRestoreDialog() override = default;

    void setCurrentDatabase(const QString& databaseName);
    void setAvailableTables(const QStringList& tables);
    void setAvailableSchemas(const QStringList& schemas);
    void setBackupHistory(const QList<BackupInfo>& history);

signals:
    void backupRequested(const BackupOptions& options);
    void restoreRequested(const RestoreOptions& options);
    void backupVerified(const QString& filePath);
    void backupDeleted(const QString& filePath);

private slots:
    void onTabChanged(int index);
    void onBackupTypeChanged(const QString& type);
    void onBrowseBackupFile();
    void onBrowseRestoreFile();
    void onCreateBackup();
    void onRestoreBackup();
    void onVerifyBackup();
    void onDeleteBackup();
    void onBackupSelected();
    void onRefreshHistory();

private:
    void setupUI();
    void setupBackupTab();
    void setupRestoreTab();
    void setupHistoryTab();
    void setupScheduleTab();

    void populateBackupHistory();
    void showBackupDetails(const BackupInfo& info);
    void updateBackupFileExtension();
    void validateBackupOptions();

    // UI Components
    QTabWidget* tabWidget_;

    // Backup tab
    QWidget* backupTab_;
    QComboBox* backupTypeCombo_;
    QLineEdit* backupFilePathEdit_;
    QPushButton* backupBrowseButton_;
    QComboBox* backupFormatCombo_;
    QCheckBox* backupIncludeDataCheck_;
    QCheckBox* backupIncludeIndexesCheck_;
    QCheckBox* backupIncludeConstraintsCheck_;
    QCheckBox* backupIncludeTriggersCheck_;
    QCheckBox* backupIncludeViewsCheck_;
    QCheckBox* backupCompressCheck_;
    QComboBox* backupCompressionLevelCombo_;
    QCheckBox* backupEncryptCheck_;
    QLineEdit* backupPasswordEdit_;
    QCheckBox* backupVerifyCheck_;
    QTextEdit* backupCommentEdit_;
    QPushButton* createBackupButton_;

    // Restore tab
    QWidget* restoreTab_;
    QLineEdit* restoreFilePathEdit_;
    QPushButton* restoreBrowseButton_;
    QComboBox* restoreModeCombo_;
    QCheckBox* restoreDropExistingCheck_;
    QCheckBox* restoreCreateSchemasCheck_;
    QCheckBox* restoreCreateTablesCheck_;
    QCheckBox* restoreCreateIndexesCheck_;
    QCheckBox* restoreCreateConstraintsCheck_;
    QCheckBox* restoreCreateTriggersCheck_;
    QCheckBox* restoreCreateViewsCheck_;
    QComboBox* restoreConflictCombo_;
    QCheckBox* restoreIgnoreErrorsCheck_;
    QCheckBox* restorePreviewOnlyCheck_;
    QLineEdit* restorePasswordEdit_;
    QTextEdit* restorePreviewText_;
    QPushButton* restoreButton_;

    // History tab
    QWidget* historyTab_;
    QTableWidget* backupHistoryTable_;
    QPushButton* verifyBackupButton_;
    QPushButton* deleteBackupButton_;
    QPushButton* refreshHistoryButton_;

    // Schedule tab
    QWidget* scheduleTab_;
    QCheckBox* scheduleEnabledCheck_;
    QSpinBox* scheduleIntervalSpin_;
    QComboBox* scheduleUnitCombo_;
    QTimeEdit* scheduleTimeEdit_;
    QLineEdit* schedulePathEdit_;
    QPushButton* scheduleBrowseButton_;
    QPushButton* scheduleSaveButton_;

    // Common
    QLabel* databaseLabel_;
    QProgressBar* progressBar_;

    // Data
    QString currentDatabase_;
    QStringList availableTables_;
    QStringList availableSchemas_;
    QList<BackupInfo> backupHistory_;
    BackupInfo currentBackupInfo_;

    QStringList backupTypes_ = {"Full Database", "Schema Only", "Data Only", "Custom Selection"};
    QStringList backupFormats_ = {"SQL Script", "Custom Format", "Compressed Archive"};
    QStringList compressionLevels_ = {"Low", "Medium", "High", "Maximum"};
    QStringList conflictResolutions_ = {"Skip existing objects", "Overwrite existing objects", "Rename conflicting objects"};

private:
    void loadSampleHistory();
};

} // namespace scratchrobin
