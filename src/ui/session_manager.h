#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QTimer;
class QSplitter;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Session Manager
 * 
 * Views and manages database sessions.
 * - View active sessions
 * - View session statistics
 * - Kill sessions
 * - Filter by user/database/application
 * - Session history
 */

// ============================================================================
// Session Info Structures
// ============================================================================
struct SessionInfo {
    int pid = 0;
    QString username;
    QString application;
    QString clientHost;
    QString clientPort;
    QString database;
    QDateTime backendStart;
    QDateTime transactionStart;
    QDateTime queryStart;
    QDateTime stateChange;
    QString state;
    bool waiting = false;
    QString waitEventType;
    QString waitEvent;
    QString currentQuery;
    qint64 queryDuration = 0;  // milliseconds
};

// ============================================================================
// Session Manager Panel
// ============================================================================
class SessionManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SessionManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    ~SessionManagerPanel() override;
    
    QString panelTitle() const override { return tr("Session Manager"); }
    QString panelCategory() const override { return "monitoring"; }
    
    void refresh();

public slots:
    void onAutoRefreshToggled(bool enabled);
    void onRefreshIntervalChanged(int seconds);
    void onKillSession();
    void onKillAllUserSessions();
    void onCancelQuery();
    void onViewQueryDetails();
    void onViewSessionHistory();
    void onFilterChanged(const QString& filter);

protected:
    void panelActivated() override;
    void panelDeactivated() override;

private:
    void setupUi();
    void setupModel();
    void loadSessions();
    void startAutoRefresh();
    void stopAutoRefresh();
    
    backend::SessionClient* client_;
    QTimer* refreshTimer_ = nullptr;
    
    // Stats panel
    QLabel* totalLabel_ = nullptr;
    QLabel* activeLabel_ = nullptr;
    QLabel* idleLabel_ = nullptr;
    QLabel* waitingLabel_ = nullptr;
    
    // Filters
    QLineEdit* filterEdit_ = nullptr;
    QComboBox* userFilter_ = nullptr;
    QComboBox* databaseFilter_ = nullptr;
    QComboBox* stateFilter_ = nullptr;
    
    // Sessions table
    QTableView* sessionTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
};

// ============================================================================
// Session Details Dialog
// ============================================================================
class SessionDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SessionDetailsDialog(backend::SessionClient* client,
                                 const SessionInfo& session,
                                 QWidget* parent = nullptr);

private:
    void setupUi();
    void loadSessionDetails();
    
    backend::SessionClient* client_;
    SessionInfo session_;
    
    QTextEdit* detailsEdit_ = nullptr;
};

// ============================================================================
// Session History Dialog
// ============================================================================
class SessionHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit SessionHistoryDialog(backend::SessionClient* client,
                                 int pid,
                                 QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExport();

private:
    void setupUi();
    void loadHistory();
    
    backend::SessionClient* client_;
    int pid_;
    
    QTableView* historyTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QSpinBox* limitSpin_ = nullptr;
};

} // namespace scratchrobin::ui
