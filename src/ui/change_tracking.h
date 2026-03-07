#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QTableWidget;
class QListWidget;
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
class QTreeView;
class QTreeWidget;
class QTreeWidgetItem;
class QDateTimeEdit;
class QPlainTextEdit;
class QTextBrowser;
class QSpinBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Change Tracking - Audit history
 * 
 * Track and review changes:
 * - Object modification history
 * - DDL change tracking
 * - Data change audit
 * - Before/after comparison
 * - Rollback capabilities
 * - Change notifications
 */

// ============================================================================
// Change Types
// ============================================================================
enum class ChangeType {
    Create,
    Modify,
    Delete,
    Rename,
    Grant,
    Revoke,
    DataInsert,
    DataUpdate,
    DataDelete
};

enum class ChangeObjectType {
    Table,
    View,
    Procedure,
    Function,
    Trigger,
    Index,
    Constraint,
    Column,
    Schema,
    Database,
    User,
    Role,
    Permission
};

enum class ChangeSeverity {
    Info,
    Warning,
    Critical
};

// ============================================================================
// Change Record Structure
// ============================================================================
struct ChangeRecord {
    QString id;
    QString sessionId;
    QString transactionId;
    ChangeType changeType;
    ChangeObjectType objectType;
    ChangeSeverity severity;
    QString objectName;
    QString schemaName;
    QString databaseName;
    QString author;
    QString authorId;
    QDateTime timestamp;
    QString sqlBefore;
    QString sqlAfter;
    QString description;
    QString reason;  // Change reason/description
    QStringList affectedObjects;
    int rowsAffected = -1;  // For data changes
    bool isReversible = false;
    QString checksum;
};

struct ChangeSession {
    QString id;
    QString description;
    QString author;
    QDateTime startTime;
    QDateTime endTime;
    QList<ChangeRecord> changes;
    bool isCommitted = true;
    bool canRollback = false;
};

struct ChangeComparison {
    ChangeRecord oldVersion;
    ChangeRecord newVersion;
    QStringList addedLines;
    QStringList removedLines;
    QStringList modifiedLines;
};

// ============================================================================
// Change Tracking Panel
// ============================================================================
class ChangeTrackingPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ChangeTrackingPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Change Tracking"); }
    QString panelCategory() const override { return "audit"; }

public slots:
    // Navigation
    void onChangeSelected(const QModelIndex& index);
    void onFilterByType(int type);
    void onFilterByObject(const QString& object);
    void onFilterByDateRange(const QDateTime& from, const QDateTime& to);
    void onSearchTextChanged(const QString& text);
    
    // Actions
    void onViewDetails();
    void onCompareVersions();
    void onRollbackChange();
    void onExportChange();
    void onGenerateReport();
    
    // Session management
    void onViewSession(const QString& sessionId);
    void onRollbackSession();
    void onTagSession();
    
    // Refresh
    void onRefreshChanges();
    void onStartMonitoring();
    void onStopMonitoring();

signals:
    void changeSelected(const ChangeRecord& change);
    void rollbackRequested(const QString& changeId);
    void compareRequested(const QString& oldId, const QString& newId);

private:
    void setupUi();
    void setupChangesList();
    void setupDetailsPanel();
    void setupMonitoringPanel();
    void loadChanges();
    void updateChangesList();
    void updateChangeDetails(const ChangeRecord& change);
    void filterChanges();
    
    backend::SessionClient* client_;
    QList<ChangeRecord> changes_;
    ChangeRecord currentChange_;
    bool isMonitoring_ = false;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    
    // Changes list
    QTableView* changesTable_ = nullptr;
    QStandardItemModel* changesModel_ = nullptr;
    QComboBox* typeFilterCombo_ = nullptr;
    QComboBox* objectFilterCombo_ = nullptr;
    QDateTimeEdit* dateFromEdit_ = nullptr;
    QDateTimeEdit* dateToEdit_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    // Details panel
    QStackedWidget* detailsStack_ = nullptr;
    QLabel* changeTypeLabel_ = nullptr;
    QLabel* changeObjectLabel_ = nullptr;
    QLabel* changeAuthorLabel_ = nullptr;
    QLabel* changeTimeLabel_ = nullptr;
    QTextBrowser* changeDescriptionBrowser_ = nullptr;
    QPlainTextEdit* sqlBeforeEdit_ = nullptr;
    QPlainTextEdit* sqlAfterEdit_ = nullptr;
    QLabel* rowsAffectedLabel_ = nullptr;
    QCheckBox* reversibleCheck_ = nullptr;
    
    // Actions
    QPushButton* detailsBtn_ = nullptr;
    QPushButton* compareBtn_ = nullptr;
    QPushButton* rollbackBtn_ = nullptr;
    QPushButton* exportBtn_ = nullptr;
    
    // Monitoring
    QLabel* monitoringStatusLabel_ = nullptr;
    QPushButton* monitorBtn_ = nullptr;
};

// ============================================================================
// Change Details Dialog
// ============================================================================
class ChangeDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChangeDetailsDialog(const ChangeRecord& change,
                                 backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onViewBefore();
    void onViewAfter();
    void onViewDiff();
    void onExportSQL();
    void onCopyToClipboard();
    void onRollback();

private:
    void setupUi();
    void loadChangeDetails();
    void updateDiffView();
    
    ChangeRecord change_;
    backend::SessionClient* client_;
    
    QLabel* titleLabel_ = nullptr;
    QTabWidget* tabWidget_ = nullptr;
    QTextBrowser* diffBrowser_ = nullptr;
    QPlainTextEdit* beforeEdit_ = nullptr;
    QPlainTextEdit* afterEdit_ = nullptr;
    QTextBrowser* infoBrowser_ = nullptr;
};

// ============================================================================
// Compare Versions Dialog
// ============================================================================
class CompareVersionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CompareVersionsDialog(const ChangeRecord& version1, const ChangeRecord& version2,
                                   backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onSideBySideView();
    void onUnifiedView();
    void onInlineView();
    void onExportDiff();
    void onCopyDiff();
    void onNavigateNextDiff();
    void onNavigatePrevDiff();

private:
    void setupUi();
    void generateDiff();
    void updateView();
    
    ChangeRecord version1_;
    ChangeRecord version2_;
    backend::SessionClient* client_;
    
    QTabWidget* viewTabWidget_ = nullptr;
    QPlainTextEdit* leftEdit_ = nullptr;
    QPlainTextEdit* rightEdit_ = nullptr;
    QTextBrowser* unifiedBrowser_ = nullptr;
    QListWidget* diffList_ = nullptr;
    
    QStringList diffLines_;
    int currentDiffIndex_ = -1;
};

// ============================================================================
// Rollback Wizard
// ============================================================================
class RollbackWizard : public QDialog {
    Q_OBJECT

public:
    explicit RollbackWizard(const QList<ChangeRecord>& changes,
                            backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onSelectAll();
    void onDeselectAll();
    void onPreviewRollback();
    void onExecuteRollback();
    void onCancel();

private:
    void setupUi();
    void updatePreview();
    void generateRollbackScript();
    
    QList<ChangeRecord> changes_;
    backend::SessionClient* client_;
    QSet<QString> selectedChangeIds_;
    
    QTreeWidget* changesTree_ = nullptr;
    QTextBrowser* previewBrowser_ = nullptr;
    QCheckBox* backupCheck_ = nullptr;
    QCheckBox* validateCheck_ = nullptr;
    QPushButton* executeBtn_ = nullptr;
};

// ============================================================================
// Audit Report Dialog
// ============================================================================
class AuditReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuditReportDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onGenerateReport();
    void onExportPDF();
    void onExportCSV();
    void onExportHTML();
    void onPrint();
    void onScheduleReport();

private:
    void setupUi();
    void generateSummary();
    void generateDetails();
    void updateReportView();
    
    backend::SessionClient* client_;
    
    QComboBox* reportTypeCombo_ = nullptr;
    QDateTimeEdit* dateFromEdit_ = nullptr;
    QDateTimeEdit* dateToEdit_ = nullptr;
    QComboBox* scopeCombo_ = nullptr;
    QTextBrowser* reportBrowser_ = nullptr;
    QTreeWidget* summaryTree_ = nullptr;
};

// ============================================================================
// Session Browser Dialog
// ============================================================================
class SessionBrowserDialog : public QDialog {
    Q_OBJECT

public:
    explicit SessionBrowserDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onSessionSelected(const QModelIndex& index);
    void onFilterSessions();
    void onViewSessionDetails();
    void onRollbackSession();
    void onTagSession();
    void onExportSession();

private:
    void setupUi();
    void loadSessions();
    void updateSessionsList();
    void updateSessionDetails(const ChangeSession& session);
    
    backend::SessionClient* client_;
    QList<ChangeSession> sessions_;
    ChangeSession currentSession_;
    
    QTableView* sessionsTable_ = nullptr;
    QStandardItemModel* sessionsModel_ = nullptr;
    QTextBrowser* sessionDetailsBrowser_ = nullptr;
    QTableView* sessionChangesTable_ = nullptr;
    QStandardItemModel* sessionChangesModel_ = nullptr;
};

// ============================================================================
// Real-time Monitor Widget
// ============================================================================
class ChangeMonitorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChangeMonitorWidget(backend::SessionClient* client, QWidget* parent = nullptr);
    
    bool isMonitoring() const { return isMonitoring_; }

public slots:
    void startMonitoring();
    void stopMonitoring();
    void onNewChange(const ChangeRecord& change);
    void onClearHistory();

signals:
    void changeDetected(const ChangeRecord& change);
    void monitoringStarted();
    void monitoringStopped();

private:
    void setupUi();
    void addChangeToLog(const ChangeRecord& change);
    
    backend::SessionClient* client_;
    bool isMonitoring_ = false;
    QList<ChangeRecord> recentChanges_;
    
    QTableWidget* logTable_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* startStopBtn_ = nullptr;
    QSpinBox* maxEntriesSpin_ = nullptr;
};

} // namespace scratchrobin::ui
