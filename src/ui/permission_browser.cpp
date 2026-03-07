#include "permission_browser.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QHeaderView>

namespace scratchrobin::ui {

// ============================================================================
// Permission Browser Panel
// ============================================================================

PermissionBrowserPanel::PermissionBrowserPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("permission_browser", parent)
    , client_(client) {
    setupUi();
    loadPermissions();
}

void PermissionBrowserPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* grantBtn = new QPushButton(tr("Grant"), this);
    connect(grantBtn, &QPushButton::clicked, this, &PermissionBrowserPanel::onGrantPermission);
    toolbarLayout->addWidget(grantBtn);
    
    auto* revokeBtn = new QPushButton(tr("Revoke"), this);
    connect(revokeBtn, &QPushButton::clicked, this, &PermissionBrowserPanel::onRevokePermission);
    toolbarLayout->addWidget(revokeBtn);
    
    toolbarLayout->addStretch();
    
    filterCombo_ = new QComboBox(this);
    filterCombo_->addItem(tr("All Objects"));
    filterCombo_->addItem(tr("Tables"));
    filterCombo_->addItem(tr("Schemas"));
    filterCombo_->addItem(tr("Functions"));
    filterCombo_->addItem(tr("Sequences"));
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PermissionBrowserPanel::onFilterChanged);
    toolbarLayout->addWidget(filterCombo_);
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &PermissionBrowserPanel::onSearch);
    toolbarLayout->addWidget(searchEdit_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &PermissionBrowserPanel::onRefreshPermissions);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Object tree
    objectTree_ = new QTreeView(this);
    objectModel_ = new QStandardItemModel(this);
    objectTree_->setModel(objectModel_);
    objectTree_->setHeaderHidden(true);
    connect(objectTree_, &QTreeView::clicked, this, &PermissionBrowserPanel::onObjectSelected);
    splitter->addWidget(objectTree_);
    
    // Permissions table
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* permGroup = new QGroupBox(tr("Permissions"), this);
    auto* permLayout = new QVBoxLayout(permGroup);
    
    permTable_ = new QTableView(this);
    permModel_ = new QStandardItemModel(this);
    permModel_->setHorizontalHeaderLabels({tr("Grantee"), tr("Privilege"), tr("Grantable"), tr("With Hierarchy")});
    permTable_->setModel(permModel_);
    permTable_->setAlternatingRowColors(true);
    permTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    permLayout->addWidget(permTable_, 1);
    
    rightLayout->addWidget(permGroup);
    
    // Details
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setMaximumHeight(100);
    rightLayout->addWidget(new QLabel(tr("Details:"), this));
    rightLayout->addWidget(detailsEdit_);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({250, 450});
    
    mainLayout->addWidget(splitter, 1);
}

void PermissionBrowserPanel::loadObjects() {
    objectModel_->clear();
    
    // Simulate object hierarchy
    auto* databaseItem = new QStandardItem("scratchbird");
    
    auto* schemaItem = new QStandardItem("public");
    schemaItem->appendRow(new QStandardItem("customers"));
    schemaItem->appendRow(new QStandardItem("orders"));
    schemaItem->appendRow(new QStandardItem("products"));
    databaseItem->appendRow(schemaItem);
    
    auto* functionsItem = new QStandardItem("Functions");
    functionsItem->appendRow(new QStandardItem("get_customer_orders"));
    databaseItem->appendRow(functionsItem);
    
    objectModel_->appendRow(databaseItem);
    objectTree_->expandAll();
}

void PermissionBrowserPanel::loadPermissions() {
    loadObjects();
    
    permissions_.clear();
    
    // Simulate permissions
    QStringList perms = {
        "postgres:SELECT:Yes:Yes",
        "admin:ALL:Yes:Yes",
        "app_user:SELECT:No:Yes",
        "app_user:INSERT:No:No",
        "app_user:UPDATE:No:No"
    };
    
    for (const auto& perm : perms) {
        auto parts = perm.split(':');
        PermissionInfo info;
        info.grantee = parts[0];
        info.privilege = parts[1];
        info.isGrantable = parts[2] == "Yes";
        info.withHierarchy = parts[3] == "Yes";
        permissions_.append(info);
    }
    
    updatePermissionsTable();
}

void PermissionBrowserPanel::updatePermissionsTable() {
    permModel_->clear();
    permModel_->setHorizontalHeaderLabels({tr("Grantee"), tr("Privilege"), tr("Grantable"), tr("With Hierarchy")});
    
    for (const auto& perm : permissions_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(perm.grantee);
        row << new QStandardItem(perm.privilege);
        row << new QStandardItem(perm.isGrantable ? tr("Yes") : tr("No"));
        row << new QStandardItem(perm.withHierarchy ? tr("Yes") : tr("No"));
        permModel_->appendRow(row);
    }
}

void PermissionBrowserPanel::onGrantPermission() {
    PermissionInfo newPerm;
    GrantPermissionDialog dialog(&newPerm, this);
    if (dialog.exec() == QDialog::Accepted) {
        permissions_.append(newPerm);
        updatePermissionsTable();
        emit permissionGranted(newPerm.grantee, newPerm.privilege);
    }
}

void PermissionBrowserPanel::onRevokePermission() {
    auto index = permTable_->currentIndex();
    if (!index.isValid() || index.row() >= permissions_.size()) return;
    
    auto reply = QMessageBox::warning(this, tr("Revoke Permission"),
        tr("Are you sure you want to revoke this permission?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QString grantee = permissions_[index.row()].grantee;
        QString privilege = permissions_[index.row()].privilege;
        permissions_.removeAt(index.row());
        updatePermissionsTable();
        emit permissionRevoked(grantee, privilege);
    }
}

void PermissionBrowserPanel::onRefreshPermissions() {
    loadPermissions();
}

void PermissionBrowserPanel::onObjectSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    QString objectName = objectModel_->itemFromIndex(index)->text();
    detailsEdit_->setText(tr("Permissions for object: %1").arg(objectName));
    
    // Reload permissions for selected object
    updatePermissionsTable();
}

void PermissionBrowserPanel::onSearch(const QString& text) {
    // Filter displayed permissions
    for (int i = 0; i < permModel_->rowCount(); ++i) {
        auto* item = permModel_->item(i, 0);
        bool match = item->text().contains(text, Qt::CaseInsensitive);
        permTable_->setRowHidden(i, !match);
    }
}

void PermissionBrowserPanel::onFilterChanged(int index) {
    Q_UNUSED(index)
    // Apply object type filter
    updatePermissionsTable();
}

void PermissionBrowserPanel::onEditGrant() {
    auto index = permTable_->currentIndex();
    if (!index.isValid() || index.row() >= permissions_.size()) return;
    
    GrantPermissionDialog dialog(&permissions_[index.row()], this);
    dialog.exec();
    updatePermissionsTable();
}

void PermissionBrowserPanel::onViewEffective() {
    QMessageBox::information(this, tr("Effective Permissions"),
        tr("Effective permissions visualization would display here."));
}

void PermissionBrowserPanel::onAnalyze() {
    QMessageBox::information(this, tr("Permission Analysis"),
        tr("Permission analysis report would display here."));
}

void PermissionBrowserPanel::onExportPermissions() {
    QMessageBox::information(this, tr("Export"),
        tr("Permissions exported to SQL."));
}

void PermissionBrowserPanel::onShowDDL() {
    QMessageBox::information(this, tr("DDL"),
        tr("GRANT and REVOKE statements would display here."));
}

// ============================================================================
// Grant Permission Dialog
// ============================================================================

GrantPermissionDialog::GrantPermissionDialog(PermissionInfo* perm, QWidget* parent)
    : QDialog(parent)
    , perm_(perm) {
    setupUi();
}

void GrantPermissionDialog::setupUi() {
    setWindowTitle(tr("Grant Permission"));
    resize(400, 350);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    objectTypeCombo_ = new QComboBox(this);
    objectTypeCombo_->addItems({tr("Table"), tr("Schema"), tr("Function"), tr("Sequence"), tr("Database")});
    formLayout->addRow(tr("Object Type:"), objectTypeCombo_);
    
    objectNameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Object Name:"), objectNameEdit_);
    
    granteeCombo_ = new QComboBox(this);
    granteeCombo_->setEditable(true);
    granteeCombo_->addItems({"postgres", "admin", "app_user"});
    formLayout->addRow(tr("Grant To:"), granteeCombo_);
    
    privilegeCombo_ = new QComboBox(this);
    privilegeCombo_->addItems({"ALL", "SELECT", "INSERT", "UPDATE", "DELETE", "TRUNCATE", "REFERENCES", "TRIGGER"});
    formLayout->addRow(tr("Privilege:"), privilegeCombo_);
    
    grantableCheck_ = new QCheckBox(tr("With grant option"), this);
    formLayout->addRow(grantableCheck_);
    
    hierarchyCheck_ = new QCheckBox(tr("With hierarchy"), this);
    formLayout->addRow(hierarchyCheck_);
    
    layout->addLayout(formLayout);
    
    // Preview
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setMaximumHeight(80);
    previewEdit_->setPlaceholderText(tr("SQL preview will appear here..."));
    layout->addWidget(new QLabel(tr("Preview:"), this));
    layout->addWidget(previewEdit_);
    
    layout->addStretch();
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &GrantPermissionDialog::onPreview);
    btnLayout->addWidget(previewBtn);
    
    btnLayout->addStretch();
    
    auto* grantBtn = new QPushButton(tr("Grant"), this);
    connect(grantBtn, &QPushButton::clicked, this, &GrantPermissionDialog::onGrant);
    btnLayout->addWidget(grantBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void GrantPermissionDialog::onPreview() {
    QString sql = QString("GRANT %1 ON %2 %3 TO %4%5%6;")
        .arg(privilegeCombo_->currentText())
        .arg(objectTypeCombo_->currentText().toUpper())
        .arg(objectNameEdit_->text())
        .arg(granteeCombo_->currentText())
        .arg(grantableCheck_->isChecked() ? " WITH GRANT OPTION" : "")
        .arg(hierarchyCheck_->isChecked() ? " WITH HIERARCHY OPTION" : "");
    
    previewEdit_->setText(sql);
}

void GrantPermissionDialog::onGrant() {
    perm_->grantee = granteeCombo_->currentText();
    perm_->privilege = privilegeCombo_->currentText();
    perm_->isGrantable = grantableCheck_->isChecked();
    perm_->withHierarchy = hierarchyCheck_->isChecked();
    accept();
}

void GrantPermissionDialog::onShowHelp() {
    QMessageBox::information(this, tr("Help"),
        tr("Grant permission help would display here."));
}

// ============================================================================
// Revoke Permission Dialog
// ============================================================================

RevokePermissionDialog::RevokePermissionDialog(PermissionInfo* perm, QWidget* parent)
    : QDialog(parent)
    , perm_(perm) {
    setupUi();
}

void RevokePermissionDialog::setupUi() {
    setWindowTitle(tr("Revoke Permission"));
    resize(400, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    // Warning
    auto* warningLabel = new QLabel(tr("<b>Warning:</b> This will remove the specified permission."), this);
    warningLabel->setStyleSheet("color: red;");
    layout->addWidget(warningLabel);
    
    auto* formLayout = new QFormLayout();
    
    objectTypeCombo_ = new QComboBox(this);
    objectTypeCombo_->addItems({tr("Table"), tr("Schema"), tr("Function"), tr("Sequence"), tr("Database")});
    formLayout->addRow(tr("Object Type:"), objectTypeCombo_);
    
    objectNameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Object Name:"), objectNameEdit_);
    
    granteeCombo_ = new QComboBox(this);
    granteeCombo_->setEditable(true);
    formLayout->addRow(tr("From:"), granteeCombo_);
    
    privilegeCombo_ = new QComboBox(this);
    privilegeCombo_->addItems({"ALL", "SELECT", "INSERT", "UPDATE", "DELETE"});
    formLayout->addRow(tr("Privilege:"), privilegeCombo_);
    
    cascadeCheck_ = new QCheckBox(tr("CASCADE (also revoke dependent permissions)"), this);
    formLayout->addRow(cascadeCheck_);
    
    layout->addLayout(formLayout);
    
    // Preview
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setMaximumHeight(60);
    layout->addWidget(previewEdit_);
    
    layout->addStretch();
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &RevokePermissionDialog::onPreview);
    btnLayout->addWidget(previewBtn);
    
    btnLayout->addStretch();
    
    auto* revokeBtn = new QPushButton(tr("Revoke"), this);
    revokeBtn->setStyleSheet("color: red;");
    connect(revokeBtn, &QPushButton::clicked, this, &RevokePermissionDialog::onRevoke);
    btnLayout->addWidget(revokeBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void RevokePermissionDialog::onPreview() {
    QString sql = QString("REVOKE %1 ON %2 %3 FROM %4%5;")
        .arg(privilegeCombo_->currentText())
        .arg(objectTypeCombo_->currentText().toUpper())
        .arg(objectNameEdit_->text())
        .arg(granteeCombo_->currentText())
        .arg(cascadeCheck_->isChecked() ? " CASCADE" : "");
    
    previewEdit_->setText(sql);
}

void RevokePermissionDialog::onRevoke() {
    perm_->grantee = granteeCombo_->currentText();
    perm_->privilege = privilegeCombo_->currentText();
    accept();
}

void RevokePermissionDialog::onShowHelp() {
    QMessageBox::information(this, tr("Help"),
        tr("Revoke permission help would display here."));
}

} // namespace scratchrobin::ui
