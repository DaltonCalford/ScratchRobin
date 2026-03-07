#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QDateTimeEdit;
class QTabWidget;
class QSplitter;
class QTreeView;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Audit Log Viewer - Security audit log analysis
 * 
 * View and analyze database audit logs:
 * - View audit trail with filtering
 * - Search by user, action, object
 * - Export audit logs
 * - Archive old logs
 */

// ============================================================================
// Severity Levels
// ============================================================================
enum class Severity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// ============================================================================
// Audit Log Entry
// ============================================================================
struct AuditLogEntry {
    QDateTime timestamp;
    QString userName;
    QString databaseName;
    QString action;
    QString objectName;
    QString result;
    Severity severity;
    QString commandText;
    QString sessionId;
    QString clientAddress;
    QString errorMessage;
};

// ============================================================================
// Audit Log Viewer Panel
// ============================================================================
class AuditLogViewerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit AuditLogViewerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Audit Log Viewer"); }
    QString panelCategory() const override { return "security"; }

public slots:
    // Log operations
    void onRefreshLogs();
    void onApplyFilter();
    void onClearFilters();
    
    // Selection
    void onEntrySelected(const QModelIndex& index);
    void onSearch(const QString& text);
    void onFilterChanged(int index);
    
    // Actions
    void onExportLogs();
    void onArchiveOldLogs();
    void onViewStatistics();
    void onConfigureRetention();
    void onEnableDisable();

private:
    void setupUi();
    void loadLogEntries();
    void updateLogTable();
    void updateEntryDetails(const AuditLogEntry& entry);
    Severity severityFromString(const QString& str);
    
    backend::SessionClient* client_;
    QList<AuditLogEntry> entries_;
    
    // UI
    QDateTimeEdit* fromDateEdit_ = nullptr;
    QDateTimeEdit* toDateEdit_ = nullptr;
    QComboBox* severityCombo_ = nullptr;
    QComboBox* actionTypeCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    
    QTableView* logTable_ = nullptr;
    QStandardItemModel* logModel_ = nullptr;
    
    QTextEdit* commandEdit_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
    
    // Details labels
    QLabel* timestampLabel_ = nullptr;
    QLabel* userLabel_ = nullptr;
    QLabel* databaseLabel_ = nullptr;
    QLabel* actionLabel_ = nullptr;
    QLabel* objectLabel_ = nullptr;
    QLabel* resultLabel_ = nullptr;
};

// ============================================================================
// Audit Filter Dialog
// ============================================================================
class AuditFilterDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuditFilterDialog(QWidget* parent = nullptr);

public slots:
    void onPresetToday();
    void onPresetWeek();
    void onPresetMonth();
    void onSaveFilter();

private:
    void setupUi();
    
    QDateTimeEdit* fromDateEdit_ = nullptr;
    QDateTimeEdit* toDateEdit_ = nullptr;
    QComboBox* severityCombo_ = nullptr;
    QLineEdit* userEdit_ = nullptr;
    QComboBox* actionCombo_ = nullptr;
    QLineEdit* objectEdit_ = nullptr;
};

// ============================================================================
// Audit Statistics Dialog
// ============================================================================
class AuditStatisticsDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuditStatisticsDialog(QWidget* parent = nullptr);

private:
    void setupUi();
};

} // namespace scratchrobin::ui
