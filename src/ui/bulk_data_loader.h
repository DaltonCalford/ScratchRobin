#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QProgressBar;
class QLabel;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class QSplitter;
class QTimer;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Bulk Data Loader - High-performance data loading
 * 
 * Optimized for loading large datasets efficiently.
 * - Multi-threaded loading
 * - Batch insert optimization
 * - Progress tracking with ETA
 * - Resume capability
 * - Error handling and recovery
 */

// ============================================================================
// Bulk Load Info
// ============================================================================
struct BulkLoadInfo {
    QString sourceFile;
    QString targetTable;
    QString format;
    qint64 fileSize = 0;
    qint64 totalRows = 0;
    qint64 processedRows = 0;
    qint64 errorCount = 0;
    int batchSize = 1000;
    int threadCount = 4;
    bool useTransactions = true;
    bool truncateFirst = false;
    bool validateData = true;
    QDateTime started;
    QDateTime estimatedEnd;
};

// ============================================================================
// Bulk Data Loader Panel
// ============================================================================
class BulkDataLoaderPanel : public DockPanel {
    Q_OBJECT

public:
    explicit BulkDataLoaderPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    ~BulkDataLoaderPanel() override;
    
    QString panelTitle() const override { return tr("Bulk Data Loader"); }
    QString panelCategory() const override { return "database"; }

public slots:
    void onSelectSource();
    void onSelectTarget();
    void onStartLoad();
    void onPauseLoad();
    void onResumeLoad();
    void onCancelLoad();
    void onConfigureOptions();
    void onViewErrors();
    void onExportReport();

signals:
    void loadStarted(const QString& fileName);
    void loadProgress(qint64 processed, qint64 total, int percentage);
    void loadFinished(qint64 rowsLoaded, qint64 errors);
    void loadError(const QString& error);

private:
    void setupUi();
    void setupModel();
    void updateStats();
    void updateProgress();
    
    backend::SessionClient* client_;
    
    // Source section
    QLineEdit* sourceEdit_ = nullptr;
    QPushButton* sourceBtn_ = nullptr;
    QLabel* sourceInfoLabel_ = nullptr;
    
    // Target section
    QComboBox* targetTableCombo_ = nullptr;
    QComboBox* targetSchemaCombo_ = nullptr;
    QCheckBox* truncateCheck_ = nullptr;
    
    // Options
    QSpinBox* batchSizeSpin_ = nullptr;
    QSpinBox* threadCountSpin_ = nullptr;
    QCheckBox* useTransactionsCheck_ = nullptr;
    QCheckBox* validateDataCheck_ = nullptr;
    
    // Progress
    QProgressBar* progressBar_ = nullptr;
    QLabel* progressLabel_ = nullptr;
    QLabel* etaLabel_ = nullptr;
    QLabel* statsLabel_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    
    // Buttons
    QPushButton* startBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    
    BulkLoadInfo currentLoad_;
    bool isLoading_ = false;
    bool isPaused_ = false;
};

// ============================================================================
// Load Configuration Dialog
// ============================================================================
class LoadConfigurationDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoadConfigurationDialog(BulkLoadInfo* info, QWidget* parent = nullptr);

public slots:
    void onSave();
    void onReset();

private:
    void setupUi();
    void loadSettings();
    
    BulkLoadInfo* info_;
    
    QSpinBox* batchSizeSpin_ = nullptr;
    QSpinBox* threadCountSpin_ = nullptr;
    QSpinBox* commitIntervalSpin_ = nullptr;
    QCheckBox* useTransactionsCheck_ = nullptr;
    QCheckBox* skipErrorsCheck_ = nullptr;
    QSpinBox* maxErrorsSpin_ = nullptr;
    QCheckBox* validateCheck_ = nullptr;
    QCheckBox* trimWhitespaceCheck_ = nullptr;
    QCheckBox* ignoreDuplicatesCheck_ = nullptr;
};

// ============================================================================
// Error Viewer Dialog
// ============================================================================
class ErrorViewerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ErrorViewerDialog(const QStringList& errors, QWidget* parent = nullptr);

public slots:
    void onExport();
    void onClear();

private:
    void setupUi();
    
    QTableView* errorTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QStringList errors_;
};

} // namespace scratchrobin::ui
