#include "ui/privilege_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTableView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSplitter>
#include <QGroupBox>
#include <QCheckBox>
#include <QListWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QToolBar>
#include <QTabWidget>

namespace scratchrobin::ui {

// ============================================================================
// Privilege Manager Panel
// ============================================================================
PrivilegeManagerPanel::PrivilegeManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("privilege_manager", parent)
    , client_(client)
{
    setupUi();
    setupModel();
    refresh();
}

void PrivilegeManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* grantBtn = new QPushButton(tr("Grant"), this);
    auto* revokeBtn = new QPushButton(tr("Revoke"), this);
    auto* effectiveBtn = new QPushButton(tr("Effective Permissions"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(grantBtn);
    toolbar->addWidget(revokeBtn);
    toolbar->addWidget(effectiveBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(grantBtn, &QPushButton::clicked, this, &PrivilegeManagerPanel::onGrant);
    connect(revokeBtn, &QPushButton::clicked, this, &PrivilegeManagerPanel::onRevoke);
    connect(effectiveBtn, &QPushButton::clicked, this, &PrivilegeManagerPanel::onShowEffective);
    connect(refreshBtn, &QPushButton::clicked, this, &PrivilegeManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &PrivilegeManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Tabs
    tabWidget_ = new QTabWidget(this);
    
    // By Object tab
    auto* byObjectWidget = new QWidget(this);
    auto* byObjectLayout = new QVBoxLayout(byObjectWidget);
    byObjectLayout->setContentsMargins(0, 0, 0, 0);
    
    byObjectTree_ = new QTreeView(this);
    byObjectTree_->setAlternatingRowColors(true);
    byObjectTree_->setSortingEnabled(true);
    byObjectLayout->addWidget(byObjectTree_);
    
    tabWidget_->addTab(byObjectWidget, tr("By Object"));
    
    // By Grantee tab
    auto* byGranteeWidget = new QWidget(this);
    auto* byGranteeLayout = new QVBoxLayout(byGranteeWidget);
    byGranteeLayout->setContentsMargins(0, 0, 0, 0);
    
    byGranteeTree_ = new QTreeView(this);
    byGranteeTree_->setAlternatingRowColors(true);
    byGranteeTree_->setSortingEnabled(true);
    byGranteeLayout->addWidget(byGranteeTree_);
    
    tabWidget_->addTab(byGranteeWidget, tr("By Grantee"));
    
    mainLayout->addWidget(tabWidget_);
}

void PrivilegeManagerPanel::setupModel()
{
    byObjectModel_ = new QStandardItemModel(this);
    byObjectModel_->setColumnCount(4);
    byObjectModel_->setHorizontalHeaderLabels({
        tr("Object"), tr("Type"), tr("Grantee"), tr("Privileges")
    });
    byObjectTree_->setModel(byObjectModel_);
    byObjectTree_->header()->setStretchLastSection(false);
    byObjectTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    byObjectTree_->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    
    byGranteeModel_ = new QStandardItemModel(this);
    byGranteeModel_->setColumnCount(4);
    byGranteeModel_->setHorizontalHeaderLabels({
        tr("Grantee"), tr("Object"), tr("Type"), tr("Privileges")
    });
    byGranteeTree_->setModel(byGranteeModel_);
    byGranteeTree_->header()->setStretchLastSection(false);
    byGranteeTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    byGranteeTree_->header()->setSectionResizeMode(3, QHeaderView::Stretch);
}

void PrivilegeManagerPanel::refresh()
{
    loadObjects();
    loadGrantees();
}

void PrivilegeManagerPanel::panelActivated()
{
    refresh();
}

void PrivilegeManagerPanel::loadObjects()
{
    byObjectModel_->removeRows(0, byObjectModel_->rowCount());
    
    // TODO: Load from SessionClient when API is available
}

void PrivilegeManagerPanel::loadGrantees()
{
    byGranteeModel_->removeRows(0, byGranteeModel_->rowCount());
    
    // TODO: Load from SessionClient when API is available
}

void PrivilegeManagerPanel::onGrant()
{
    GrantDialog dialog(client_, this);
    dialog.exec();
    refresh();
}

void PrivilegeManagerPanel::onRevoke()
{
    RevokeDialog dialog(client_, this);
    dialog.exec();
    refresh();
}

void PrivilegeManagerPanel::onShowEffective()
{
    EffectivePermissionsDialog dialog(client_, this);
    dialog.exec();
}

void PrivilegeManagerPanel::loadPrivilegesByObject(const QString& objectName)
{
    // TODO: Load from SessionClient when API is available
    Q_UNUSED(objectName)
}

void PrivilegeManagerPanel::loadPrivilegesByGrantee(const QString& grantee)
{
    // TODO: Load from SessionClient when API is available
    Q_UNUSED(grantee)
}

void PrivilegeManagerPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < byObjectModel_->rowCount(); ++i) {
        QString name = byObjectModel_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        byObjectTree_->setRowHidden(i, QModelIndex(), !match);
    }
    for (int i = 0; i < byGranteeModel_->rowCount(); ++i) {
        QString name = byGranteeModel_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        byGranteeTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

// ============================================================================
// Grant Dialog
// ============================================================================
GrantDialog::GrantDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Grant Privileges"));
    setMinimumSize(500, 600);
    setupUi();
}

void GrantDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Object selection
    auto* objectGroup = new QGroupBox(tr("Select Object"), this);
    auto* objectLayout = new QFormLayout(objectGroup);
    
    objectTypeCombo_ = new QComboBox(this);
    objectTypeCombo_->addItems({"TABLE", "VIEW", "SEQUENCE", "FUNCTION", "PROCEDURE"});
    objectLayout->addRow(tr("Object Type:"), objectTypeCombo_);
    
    objectNameCombo_ = new QComboBox(this);
    objectNameCombo_->setEditable(true);
    objectLayout->addRow(tr("Object Name:"), objectNameCombo_);
    
    mainLayout->addWidget(objectGroup);
    
    // Grantee selection
    auto* granteeGroup = new QGroupBox(tr("Grant To"), this);
    auto* granteeLayout = new QFormLayout(granteeGroup);
    
    granteeTypeCombo_ = new QComboBox(this);
    granteeTypeCombo_->addItems({"USER", "ROLE", "GROUP", "PUBLIC"});
    granteeLayout->addRow(tr("Grantee Type:"), granteeTypeCombo_);
    
    granteeEdit_ = new QLineEdit(this);
    granteeLayout->addRow(tr("Grantee Name:"), granteeEdit_);
    
    mainLayout->addWidget(granteeGroup);
    
    // Privileges
    auto* privGroup = new QGroupBox(tr("Privileges"), this);
    auto* privLayout = new QVBoxLayout(privGroup);
    
    selectCheck_ = new QCheckBox(tr("SELECT"), this);
    insertCheck_ = new QCheckBox(tr("INSERT"), this);
    updateCheck_ = new QCheckBox(tr("UPDATE"), this);
    deleteCheck_ = new QCheckBox(tr("DELETE"), this);
    allCheck_ = new QCheckBox(tr("ALL PRIVILEGES"), this);
    
    privLayout->addWidget(selectCheck_);
    privLayout->addWidget(insertCheck_);
    privLayout->addWidget(updateCheck_);
    privLayout->addWidget(deleteCheck_);
    privLayout->addWidget(allCheck_);
    
    withGrantOption_ = new QCheckBox(tr("WITH GRANT OPTION"), this);
    privLayout->addWidget(withGrantOption_);
    
    mainLayout->addWidget(privGroup);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("SQL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(previewBtn, &QPushButton::clicked, this, &GrantDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GrantDialog::onGrant);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void GrantDialog::onGrant()
{
    // TODO: Execute via SessionClient when API is available
    accept();
}

void GrantDialog::onPreview()
{
    QString privs;
    if (allCheck_->isChecked()) {
        privs = "ALL PRIVILEGES";
    } else {
        QStringList list;
        if (selectCheck_->isChecked()) list.append("SELECT");
        if (insertCheck_->isChecked()) list.append("INSERT");
        if (updateCheck_->isChecked()) list.append("UPDATE");
        if (deleteCheck_->isChecked()) list.append("DELETE");
        privs = list.join(", ");
    }
    
    QString sql = QString("GRANT %1 ON %2 %3 TO %4 %5%6;")
        .arg(privs)
        .arg(objectTypeCombo_->currentText())
        .arg(objectNameCombo_->currentText())
        .arg(granteeTypeCombo_->currentText())
        .arg(granteeEdit_->text())
        .arg(withGrantOption_->isChecked() ? " WITH GRANT OPTION" : "");
    
    previewEdit_->setPlainText(sql);
}

// ============================================================================
// Revoke Dialog
// ============================================================================
RevokeDialog::RevokeDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Revoke Privileges"));
    setMinimumSize(500, 400);
    setupUi();
}

void RevokeDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    objectTypeCombo_ = new QComboBox(this);
    objectTypeCombo_->addItems({"TABLE", "VIEW", "SEQUENCE", "FUNCTION", "PROCEDURE"});
    formLayout->addRow(tr("Object Type:"), objectTypeCombo_);
    
    objectNameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Object Name:"), objectNameEdit_);
    
    granteeTypeCombo_ = new QComboBox(this);
    granteeTypeCombo_->addItems({"USER", "ROLE", "GROUP", "PUBLIC"});
    formLayout->addRow(tr("From:"), granteeTypeCombo_);
    
    granteeEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Grantee Name:"), granteeEdit_);
    
    privilegesEdit_ = new QLineEdit(this);
    privilegesEdit_->setPlaceholderText(tr("e.g., SELECT, INSERT or ALL PRIVILEGES"));
    formLayout->addRow(tr("Privileges:"), privilegesEdit_);
    
    mainLayout->addLayout(formLayout);
    
    cascadeCheck_ = new QCheckBox(tr("CASCADE (also revoke dependent privileges)"), this);
    mainLayout->addWidget(cascadeCheck_);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("SQL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(previewBtn, &QPushButton::clicked, this, &RevokeDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RevokeDialog::onRevoke);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void RevokeDialog::onRevoke()
{
    // TODO: Execute via SessionClient when API is available
    accept();
}

void RevokeDialog::onPreview()
{
    QString sql = QString("REVOKE %1 ON %2 %3 FROM %4 %5%6;")
        .arg(privilegesEdit_->text())
        .arg(objectTypeCombo_->currentText())
        .arg(objectNameEdit_->text())
        .arg(granteeTypeCombo_->currentText())
        .arg(granteeEdit_->text())
        .arg(cascadeCheck_->isChecked() ? " CASCADE" : " RESTRICT");
    
    previewEdit_->setPlainText(sql);
}

// ============================================================================
// Effective Permissions Dialog
// ============================================================================
EffectivePermissionsDialog::EffectivePermissionsDialog(backend::SessionClient* client,
                                                       QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Effective Permissions"));
    setMinimumSize(600, 400);
    setupUi();
}

void EffectivePermissionsDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    userCombo_ = new QComboBox(this);
    userCombo_->setEditable(true);
    formLayout->addRow(tr("User/Role:"), userCombo_);
    
    objectEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Object (optional):"), objectEdit_);
    
    mainLayout->addLayout(formLayout);
    
    resultsTree_ = new QTreeView(this);
    resultsModel_ = new QStandardItemModel(this);
    resultsModel_->setColumnCount(3);
    resultsModel_->setHorizontalHeaderLabels({tr("Object"), tr("Privilege"), tr("Source")});
    resultsTree_->setModel(resultsModel_);
    resultsTree_->header()->setStretchLastSection(true);
    mainLayout->addWidget(resultsTree_);
    
    auto* calcBtn = new QPushButton(tr("Calculate"), this);
    mainLayout->addWidget(calcBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);
    
    connect(calcBtn, &QPushButton::clicked, this, &EffectivePermissionsDialog::onCalculate);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void EffectivePermissionsDialog::onCalculate()
{
    resultsModel_->removeRows(0, resultsModel_->rowCount());
    
    // TODO: Calculate via SessionClient when API is available
    
    QMessageBox::information(this, tr("Effective Permissions"),
        tr("Effective permissions calculated for user: %1").arg(userCombo_->currentText()));
}

} // namespace scratchrobin::ui
