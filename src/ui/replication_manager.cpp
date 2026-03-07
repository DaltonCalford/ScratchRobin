#include "ui/replication_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTableView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QToolBar>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QTabWidget>

namespace scratchrobin::ui {

// ============================================================================
// Replication Manager Panel
// ============================================================================
ReplicationManagerPanel::ReplicationManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("replication_manager", parent)
    , client_(client)
{
    setupUi();
    setupModels();
    refresh();
}

void ReplicationManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* createSlotBtn = new QPushButton(tr("Create Slot"), this);
    auto* createPubBtn = new QPushButton(tr("Create Publication"), this);
    auto* createSubBtn = new QPushButton(tr("Create Subscription"), this);
    auto* enableBtn = new QPushButton(tr("Enable"), this);
    auto* disableBtn = new QPushButton(tr("Disable"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* failoverBtn = new QPushButton(tr("Failover"), this);
    
    toolbar->addWidget(createSlotBtn);
    toolbar->addWidget(createPubBtn);
    toolbar->addWidget(createSubBtn);
    toolbar->addWidget(enableBtn);
    toolbar->addWidget(disableBtn);
    toolbar->addWidget(refreshBtn);
    toolbar->addWidget(failoverBtn);
    
    connect(createSlotBtn, &QPushButton::clicked, this, &ReplicationManagerPanel::onCreateReplicationSlot);
    connect(createPubBtn, &QPushButton::clicked, this, &ReplicationManagerPanel::onCreatePublication);
    connect(createSubBtn, &QPushButton::clicked, this, &ReplicationManagerPanel::onCreateSubscription);
    connect(enableBtn, &QPushButton::clicked, this, &ReplicationManagerPanel::onEnableSubscription);
    connect(disableBtn, &QPushButton::clicked, this, &ReplicationManagerPanel::onDisableSubscription);
    connect(refreshBtn, &QPushButton::clicked, this, &ReplicationManagerPanel::refresh);
    connect(failoverBtn, &QPushButton::clicked, this, &ReplicationManagerPanel::onPerformFailover);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &ReplicationManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Tabs
    tabWidget_ = new QTabWidget(this);
    
    statusTree_ = new QTreeView(this);
    statusTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(statusTree_, tr("Status"));
    
    slotTree_ = new QTreeView(this);
    slotTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(slotTree_, tr("Replication Slots"));
    
    publicationTree_ = new QTreeView(this);
    publicationTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(publicationTree_, tr("Publications"));
    
    subscriptionTree_ = new QTreeView(this);
    subscriptionTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(subscriptionTree_, tr("Subscriptions"));
    
    mainLayout->addWidget(tabWidget_);
}

void ReplicationManagerPanel::setupModels()
{
    statusModel_ = new QStandardItemModel(this);
    statusTree_->setModel(statusModel_);
    
    slotModel_ = new QStandardItemModel(this);
    slotTree_->setModel(slotModel_);
    
    publicationModel_ = new QStandardItemModel(this);
    publicationTree_->setModel(publicationModel_);
    
    subscriptionModel_ = new QStandardItemModel(this);
    subscriptionTree_->setModel(subscriptionModel_);
}

void ReplicationManagerPanel::refresh()
{
    loadReplicationStatus();
    loadReplicationSlots();
    loadPublications();
    loadSubscriptions();
}

void ReplicationManagerPanel::panelActivated()
{
    refresh();
}

void ReplicationManagerPanel::loadReplicationStatus()
{
    statusModel_->removeRows(0, statusModel_->rowCount());
}

void ReplicationManagerPanel::loadReplicationSlots()
{
    slotModel_->removeRows(0, slotModel_->rowCount());
}

void ReplicationManagerPanel::loadPublications()
{
    publicationModel_->removeRows(0, publicationModel_->rowCount());
}

void ReplicationManagerPanel::loadSubscriptions()
{
    subscriptionModel_->removeRows(0, subscriptionModel_->rowCount());
}

void ReplicationManagerPanel::onCreateReplicationSlot()
{
    CreateReplicationSlotDialog dialog(client_, this);
    dialog.exec();
}

void ReplicationManagerPanel::onDropReplicationSlot()
{
    auto reply = QMessageBox::question(this, tr("Drop Slot"),
        tr("Drop replication slot?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        refresh();
    }
}

void ReplicationManagerPanel::onCreatePublication()
{
    CreatePublicationDialog dialog(client_, this);
    dialog.exec();
}

void ReplicationManagerPanel::onCreateSubscription()
{
    CreateSubscriptionDialog dialog(client_, this);
    dialog.exec();
}

void ReplicationManagerPanel::onEnableSubscription()
{
    // TODO: Enable subscription
    refresh();
}

void ReplicationManagerPanel::onDisableSubscription()
{
    // TODO: Disable subscription
    refresh();
}

void ReplicationManagerPanel::onRefreshSubscription()
{
    // TODO: Refresh subscription
    refresh();
}

void ReplicationManagerPanel::onPerformFailover()
{
    FailoverDialog dialog(client_, this);
    dialog.exec();
}

void ReplicationManagerPanel::onFilterChanged(const QString& filter)
{
    Q_UNUSED(filter)
}

// ============================================================================
// Create Replication Slot Dialog
// ============================================================================
CreateReplicationSlotDialog::CreateReplicationSlotDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create Replication Slot"));
    setMinimumSize(400, 200);
    setupUi();
}

void CreateReplicationSlotDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    nameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Slot Name:"), nameEdit_);
    pluginCombo_ = new QComboBox(this);
    pluginCombo_->addItem("pgoutput");
    formLayout->addRow(tr("Plugin:"), pluginCombo_);
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItems({"physical", "logical"});
    formLayout->addRow(tr("Type:"), typeCombo_);
    mainLayout->addLayout(formLayout);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateReplicationSlotDialog::onCreate()
{
    accept();
}

// ============================================================================
// Create Publication Dialog
// ============================================================================
CreatePublicationDialog::CreatePublicationDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create Publication"));
    setMinimumSize(500, 450);
    setupUi();
}

void CreatePublicationDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    nameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Publication Name:"), nameEdit_);
    
    allTablesCheck_ = new QCheckBox(tr("All Tables"), this);
    formLayout->addRow(QString(), allTablesCheck_);
    mainLayout->addLayout(formLayout);
    
    tableList_ = new QListWidget(this);
    tableList_->setSelectionMode(QAbstractItemView::MultiSelection);
    mainLayout->addWidget(new QLabel(tr("Tables:")));
    mainLayout->addWidget(tableList_);
    
    auto* optsGroup = new QGroupBox(tr("Operations"), this);
    auto* optsLayout = new QHBoxLayout(optsGroup);
    insertCheck_ = new QCheckBox(tr("INSERT"), this);
    insertCheck_->setChecked(true);
    updateCheck_ = new QCheckBox(tr("UPDATE"), this);
    updateCheck_->setChecked(true);
    deleteCheck_ = new QCheckBox(tr("DELETE"), this);
    deleteCheck_->setChecked(true);
    truncateCheck_ = new QCheckBox(tr("TRUNCATE"), this);
    
    optsLayout->addWidget(insertCheck_);
    optsLayout->addWidget(updateCheck_);
    optsLayout->addWidget(deleteCheck_);
    optsLayout->addWidget(truncateCheck_);
    mainLayout->addWidget(optsGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreatePublicationDialog::loadTables()
{
}

void CreatePublicationDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreatePublicationDialog::onCreate()
{
    accept();
}

QString CreatePublicationDialog::generateDdl()
{
    QString sql = QString("CREATE PUBLICATION %1").arg(nameEdit_->text());
    
    if (allTablesCheck_->isChecked()) {
        sql += " FOR ALL TABLES";
    } else {
        // TODO: Add selected tables
        sql += " FOR TABLE table1, table2";
    }
    
    QStringList ops;
    if (insertCheck_->isChecked()) ops.append("insert");
    if (updateCheck_->isChecked()) ops.append("update");
    if (deleteCheck_->isChecked()) ops.append("delete");
    if (truncateCheck_->isChecked()) ops.append("truncate");
    
    sql += QString(" WITH (publish = '%1');").arg(ops.join(", "));
    return sql;
}

// ============================================================================
// Create Subscription Dialog
// ============================================================================
CreateSubscriptionDialog::CreateSubscriptionDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create Subscription"));
    setMinimumSize(400, 450);
    setupUi();
}

void CreateSubscriptionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    nameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Subscription Name:"), nameEdit_);
    hostEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Host:"), hostEdit_);
    portSpin_ = new QSpinBox(this);
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(5432);
    formLayout->addRow(tr("Port:"), portSpin_);
    databaseEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Database:"), databaseEdit_);
    userEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("User:"), userEdit_);
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("Password:"), passwordEdit_);
    publicationEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Publication:"), publicationEdit_);
    mainLayout->addLayout(formLayout);
    
    enabledCheck_ = new QCheckBox(tr("Enabled"), this);
    enabledCheck_->setChecked(true);
    mainLayout->addWidget(enabledCheck_);
    
    copyDataCheck_ = new QCheckBox(tr("Copy Data"), this);
    copyDataCheck_->setChecked(true);
    mainLayout->addWidget(copyDataCheck_);
    
    createSlotCheck_ = new QCheckBox(tr("Create Slot"), this);
    createSlotCheck_->setChecked(true);
    mainLayout->addWidget(createSlotCheck_);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto* testBtn = buttonBox->addButton(tr("Test"), QDialogButtonBox::ActionRole);
    connect(testBtn, &QPushButton::clicked, this, &CreateSubscriptionDialog::onTestConnection);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateSubscriptionDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateSubscriptionDialog::onCreate()
{
    accept();
}

void CreateSubscriptionDialog::onTestConnection()
{
    QMessageBox::information(this, tr("Test"), tr("Connection test not implemented."));
}

QString CreateSubscriptionDialog::generateDdl()
{
    QString conninfo = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5")
        .arg(hostEdit_->text())
        .arg(portSpin_->value())
        .arg(databaseEdit_->text())
        .arg(userEdit_->text())
        .arg(passwordEdit_->text());
    
    QString sql = QString("CREATE SUBSCRIPTION %1 CONNECTION '%2' PUBLICATION %3")
        .arg(nameEdit_->text())
        .arg(conninfo)
        .arg(publicationEdit_->text());
    
    QStringList opts;
    if (!enabledCheck_->isChecked()) opts.append("enabled = false");
    if (!copyDataCheck_->isChecked()) opts.append("copy_data = false");
    if (!createSlotCheck_->isChecked()) opts.append("create_slot = false");
    
    if (!opts.isEmpty()) {
        sql += QString(" WITH (%1)").arg(opts.join(", "));
    }
    
    sql += ";";
    return sql;
}

// ============================================================================
// Failover Dialog
// ============================================================================
FailoverDialog::FailoverDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Perform Failover"));
    setMinimumSize(400, 300);
    setupUi();
}

void FailoverDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* warningLabel = new QLabel(
        tr("<b>Warning:</b> This will promote a standby server to become the new primary. "
           "This operation cannot be undone."), this);
    warningLabel->setWordWrap(true);
    warningLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(warningLabel);
    
    auto* formLayout = new QFormLayout();
    targetServerCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Target Standby:"), targetServerCombo_);
    forceCheck_ = new QCheckBox(tr("Force failover"), this);
    formLayout->addRow(QString(), forceCheck_);
    mainLayout->addLayout(formLayout);
    
    statusEdit_ = new QTextEdit(this);
    statusEdit_->setReadOnly(true);
    statusEdit_->setPlaceholderText(tr("Status will appear here..."));
    mainLayout->addWidget(statusEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Failover"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FailoverDialog::onPerformFailover);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void FailoverDialog::loadStandbyServers()
{
}

void FailoverDialog::onPerformFailover()
{
    auto reply = QMessageBox::warning(this, tr("Confirm Failover"),
        tr("Are you sure you want to promote '%1' to primary?")
            .arg(targetServerCombo_->currentText()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        statusEdit_->setPlainText(tr("Failover initiated..."));
        // TODO: Execute failover via SessionClient
        accept();
    }
}

} // namespace scratchrobin::ui
