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
class QListWidget;
class QTabWidget;
class QLabel;
class QSpinBox;
class QCheckBox;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Replication Manager
 * 
 * Manages database replication (master/slave, streaming replication).
 * - View replication status
 * - Configure streaming replication
 * - Manage replication slots
 * - Monitor replication lag
 * - Failover management
 */

// ============================================================================
// Replication Info Structures
// ============================================================================
struct ReplicationSlotInfo {
    QString name;
    QString plugin;
    QString slotType;
    bool active = false;
    qint64 restartLsn = 0;
    qint64 confirmedFlushLsn = 0;
    QDateTime created;
};

struct ReplicationConnectionInfo {
    int pid = 0;
    QString applicationName;
    QString clientAddress;
    QString state;
    qint64 sentLsn = 0;
    qint64 writeLsn = 0;
    qint64 flushLsn = 0;
    qint64 replayLsn = 0;
    int writeLag = 0;
    int flushLag = 0;
    int replayLag = 0;
    bool isSync = false;
};

struct PublicationInfo {
    QString name;
    QStringList tables;
    bool allTables = false;
    bool insert = true;
    bool update = true;
    bool delete_ = true;
    bool truncate = false;
};

struct SubscriptionInfo {
    QString name;
    QString host;
    int port = 5432;
    QString database;
    QString publication;
    bool enabled = true;
    QDateTime created;
};

// ============================================================================
// Replication Manager Panel
// ============================================================================
class ReplicationManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ReplicationManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Replication Manager"); }
    QString panelCategory() const override { return "administration"; }
    
    void refresh();

public slots:
    void onCreateReplicationSlot();
    void onDropReplicationSlot();
    void onCreatePublication();
    void onCreateSubscription();
    void onEnableSubscription();
    void onDisableSubscription();
    void onRefreshSubscription();
    void onPerformFailover();
    void onFilterChanged(const QString& filter);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModels();
    void loadReplicationStatus();
    void loadReplicationSlots();
    void loadPublications();
    void loadSubscriptions();
    
    backend::SessionClient* client_;
    
    QTabWidget* tabWidget_ = nullptr;
    
    // Status tab
    QTreeView* statusTree_ = nullptr;
    QStandardItemModel* statusModel_ = nullptr;
    
    // Replication slots tab
    QTreeView* slotTree_ = nullptr;
    QStandardItemModel* slotModel_ = nullptr;
    
    // Publications tab
    QTreeView* publicationTree_ = nullptr;
    QStandardItemModel* publicationModel_ = nullptr;
    
    // Subscriptions tab
    QTreeView* subscriptionTree_ = nullptr;
    QStandardItemModel* subscriptionModel_ = nullptr;
    
    QLineEdit* filterEdit_ = nullptr;
};

// ============================================================================
// Create Replication Slot Dialog
// ============================================================================
class CreateReplicationSlotDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateReplicationSlotDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onCreate();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* pluginCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
};

// ============================================================================
// Create Publication Dialog
// ============================================================================
class CreatePublicationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreatePublicationDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();

private:
    void setupUi();
    void loadTables();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QCheckBox* allTablesCheck_ = nullptr;
    QListWidget* tableList_ = nullptr;
    QCheckBox* insertCheck_ = nullptr;
    QCheckBox* updateCheck_ = nullptr;
    QCheckBox* deleteCheck_ = nullptr;
    QCheckBox* truncateCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Create Subscription Dialog
// ============================================================================
class CreateSubscriptionDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateSubscriptionDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();
    void onTestConnection();

private:
    void setupUi();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* hostEdit_ = nullptr;
    QSpinBox* portSpin_ = nullptr;
    QLineEdit* databaseEdit_ = nullptr;
    QLineEdit* userEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLineEdit* publicationEdit_ = nullptr;
    QCheckBox* enabledCheck_ = nullptr;
    QCheckBox* copyDataCheck_ = nullptr;
    QCheckBox* createSlotCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Failover Dialog
// ============================================================================
class FailoverDialog : public QDialog {
    Q_OBJECT

public:
    explicit FailoverDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPerformFailover();

private:
    void setupUi();
    void loadStandbyServers();
    
    backend::SessionClient* client_;
    
    QComboBox* targetServerCombo_ = nullptr;
    QCheckBox* forceCheck_ = nullptr;
    QTextEdit* statusEdit_ = nullptr;
};

} // namespace scratchrobin::ui
