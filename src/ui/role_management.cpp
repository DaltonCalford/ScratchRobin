#include "role_management.h"
#include "user_management.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTableView>
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
#include <QListWidget>
#include <QDialogButtonBox>
#include <QTreeView>

namespace scratchrobin::ui {

// ============================================================================
// Role Management Panel
// ============================================================================

RoleManagementPanel::RoleManagementPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("role_management", parent)
    , client_(client) {
    setupUi();
    loadRoles();
}

void RoleManagementPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* createBtn = new QPushButton(tr("Create Role"), this);
    connect(createBtn, &QPushButton::clicked, this, &RoleManagementPanel::onCreateRole);
    toolbarLayout->addWidget(createBtn);
    
    auto* editBtn = new QPushButton(tr("Edit"), this);
    connect(editBtn, &QPushButton::clicked, this, &RoleManagementPanel::onEditRole);
    toolbarLayout->addWidget(editBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &RoleManagementPanel::onDeleteRole);
    toolbarLayout->addWidget(deleteBtn);
    
    auto* cloneBtn = new QPushButton(tr("Clone"), this);
    connect(cloneBtn, &QPushButton::clicked, this, &RoleManagementPanel::onCloneRole);
    toolbarLayout->addWidget(cloneBtn);
    
    toolbarLayout->addStretch();
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search roles..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &RoleManagementPanel::onSearchRoles);
    toolbarLayout->addWidget(searchEdit_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &RoleManagementPanel::onRefreshRoles);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Role tree
    roleTree_ = new QTreeView(this);
    roleModel_ = new QStandardItemModel(this);
    roleModel_->setHorizontalHeaderLabels({tr("Role"), tr("Members"), tr("Type")});
    roleTree_->setModel(roleModel_);
    roleTree_->setAlternatingRowColors(true);
    connect(roleTree_, &QTreeView::clicked, this, &RoleManagementPanel::onRoleSelected);
    splitter->addWidget(roleTree_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* infoGroup = new QGroupBox(tr("Role Information"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    nameLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Name:"), nameLabel_);
    
    typeLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Type:"), typeLabel_);
    
    memberCountLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Member Count:"), memberCountLabel_);
    
    descriptionLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Description:"), descriptionLabel_);
    
    detailsLayout->addWidget(infoGroup);
    
    // Members table
    auto* membersGroup = new QGroupBox(tr("Members"), this);
    auto* membersLayout = new QVBoxLayout(membersGroup);
    
    membersTable_ = new QTableView(this);
    membersModel_ = new QStandardItemModel(this);
    membersModel_->setHorizontalHeaderLabels({tr("Member"), tr("Type"), tr("Admin Option")});
    membersTable_->setModel(membersModel_);
    membersLayout->addWidget(membersTable_);
    
    auto* memberBtnLayout = new QHBoxLayout();
    
    auto* addMemberBtn = new QPushButton(tr("Add Member"), this);
    connect(addMemberBtn, &QPushButton::clicked, this, &RoleManagementPanel::onManageMembers);
    memberBtnLayout->addWidget(addMemberBtn);
    
    auto* removeMemberBtn = new QPushButton(tr("Remove Member"), this);
    connect(removeMemberBtn, &QPushButton::clicked, this, &RoleManagementPanel::onRemoveMember);
    memberBtnLayout->addWidget(removeMemberBtn);
    
    memberBtnLayout->addStretch();
    membersLayout->addLayout(memberBtnLayout);
    
    detailsLayout->addWidget(membersGroup);
    
    // Permissions
    auto* permGroup = new QGroupBox(tr("Permissions"), this);
    auto* permLayout = new QVBoxLayout(permGroup);
    
    permTree_ = new QTreeView(this);
    permTree_->setMaximumHeight(150);
    permLayout->addWidget(permTree_);
    
    auto* permBtn = new QPushButton(tr("Edit Permissions"), this);
    connect(permBtn, &QPushButton::clicked, this, &RoleManagementPanel::onEditPermissions);
    permLayout->addWidget(permBtn);
    
    detailsLayout->addWidget(permGroup);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({300, 300});
    
    mainLayout->addWidget(splitter, 1);
}

void RoleManagementPanel::loadRoles() {
    roles_.clear();
    
    // Simulate loading roles
    QStringList roleData = {
        "admin:0:User",
        "developers:3:Role",
        "analysts:2:Role",
        "readonly:5:Role"
    };
    
    for (const auto& line : roleData) {
        auto parts = line.split(':');
        RoleInfo role;
        role.name = parts[0];
        role.type = (parts[2] == "User") ? RoleType::User : RoleType::Group;
        role.members << (parts[0] == "developers" ? QStringList{"john", "jane", "bob"} : QStringList{"user1", "user2"});
        roles_.append(role);
    }
    
    updateRoleTree();
}

void RoleManagementPanel::updateRoleTree() {
    roleModel_->clear();
    roleModel_->setHorizontalHeaderLabels({tr("Role"), tr("Members"), tr("Type")});
    
    for (const auto& role : roles_) {
        auto* item = new QStandardItem(role.name);
        item->setData(role.name, Qt::UserRole);
        
        auto* membersItem = new QStandardItem(QString::number(role.members.size()));
        auto* typeItem = new QStandardItem(role.type == RoleType::User ? tr("User") : tr("Group"));
        
        roleModel_->appendRow({item, membersItem, typeItem});
    }
}

void RoleManagementPanel::updateRoleDetails(const RoleInfo& role) {
    nameLabel_->setText(role.name);
    typeLabel_->setText(role.type == RoleType::User ? tr("User") : tr("Group"));
    memberCountLabel_->setText(QString::number(role.members.size()));
    descriptionLabel_->setText(role.description.isEmpty() ? tr("No description") : role.description);
    
    membersModel_->clear();
    membersModel_->setHorizontalHeaderLabels({tr("Member"), tr("Type"), tr("Admin Option")});
    
    for (const auto& member : role.members) {
        membersModel_->appendRow({
            new QStandardItem(member),
            new QStandardItem(tr("User")),
            new QStandardItem(tr("No"))
        });
    }
}

void RoleManagementPanel::onCreateRole() {
    RoleInfo newRole;
    RoleDialog dialog(&newRole, true, this);
    if (dialog.exec() == QDialog::Accepted) {
        roles_.append(newRole);
        loadRoles();
        emit roleCreated(newRole.name);
    }
}

void RoleManagementPanel::onEditRole() {
    auto index = roleTree_->currentIndex();
    if (!index.isValid() || index.row() >= roles_.size()) return;
    
    RoleDialog dialog(&roles_[index.row()], false, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadRoles();
        emit roleModified(roles_[index.row()].name);
    }
}

void RoleManagementPanel::onDeleteRole() {
    auto index = roleTree_->currentIndex();
    if (!index.isValid() || index.row() >= roles_.size()) return;
    
    QString roleName = roles_[index.row()].name;
    auto reply = QMessageBox::warning(this, tr("Delete Role"),
        tr("Are you sure you want to delete role '%1'?").arg(roleName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        roles_.removeAt(index.row());
        loadRoles();
        emit roleDeleted(roleName);
    }
}

void RoleManagementPanel::onCloneRole() {
    auto index = roleTree_->currentIndex();
    if (!index.isValid() || index.row() >= roles_.size()) return;
    
    RoleInfo clone = roles_[index.row()];
    clone.name += "_copy";
    
    RoleDialog dialog(&clone, true, this);
    if (dialog.exec() == QDialog::Accepted) {
        roles_.append(clone);
        loadRoles();
        emit roleCreated(clone.name);
    }
}

void RoleManagementPanel::onRefreshRoles() {
    loadRoles();
}

void RoleManagementPanel::onRoleSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= roles_.size()) return;
    updateRoleDetails(roles_[index.row()]);
}

void RoleManagementPanel::onSearchRoles(const QString& text) {
    for (int i = 0; i < roleModel_->rowCount(); ++i) {
        auto* item = roleModel_->item(i, 0);
        bool match = item->text().contains(text, Qt::CaseInsensitive);
        roleTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

void RoleManagementPanel::onManageMembers() {
    auto index = roleTree_->currentIndex();
    if (!index.isValid() || index.row() >= roles_.size()) return;
    
    RoleMembershipDialog dialog(&roles_[index.row()], this);
    if (dialog.exec() == QDialog::Accepted) {
        loadRoles();
    }
}

void RoleManagementPanel::onAddMember() {
    // Add member to role
}

void RoleManagementPanel::onRemoveMember() {
    auto index = membersTable_->currentIndex();
    if (!index.isValid()) return;
    
    QString member = membersModel_->item(index.row(), 0)->text();
    QMessageBox::information(this, tr("Remove Member"),
        tr("Member '%1' would be removed.").arg(member));
}

void RoleManagementPanel::onEditPermissions() {
    auto index = roleTree_->currentIndex();
    if (!index.isValid() || index.row() >= roles_.size()) return;
    
    QMessageBox::information(this, tr("Edit Permissions"),
        tr("Permissions editor for role '%1' would open.").arg(roles_[index.row()].name));
}

void RoleManagementPanel::onViewHierarchy() {
    QMessageBox::information(this, tr("Role Hierarchy"),
        tr("Role hierarchy visualization would display here."));
}

void RoleManagementPanel::onViewPermissions() {
    auto index = roleTree_->currentIndex();
    if (!index.isValid() || index.row() >= roles_.size()) return;
    
    QMessageBox::information(this, tr("Permissions"),
        tr("Permissions for role '%1' would display here.").arg(roles_[index.row()].name));
}

// ============================================================================
// Role Dialog
// ============================================================================

RoleDialog::RoleDialog(RoleInfo* role, bool isNew, QWidget* parent)
    : QDialog(parent)
    , role_(role)
    , isNew_(isNew)
    , descriptionEdit_(nullptr) {
    setupUi();
}

void RoleDialog::setupUi() {
    setWindowTitle(isNew_ ? tr("Create Role") : tr("Edit Role"));
    resize(400, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Basic info
    auto* basicGroup = new QGroupBox(tr("Basic Information"), this);
    auto* basicLayout = new QFormLayout(basicGroup);
    
    nameEdit_ = new QLineEdit(role_->name, this);
    basicLayout->addRow(tr("Name:"), nameEdit_);
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem(tr("Group Role"), static_cast<int>(RoleType::Group));
    typeCombo_->addItem(tr("User Role"), static_cast<int>(RoleType::User));
    typeCombo_->setCurrentIndex(role_->type == RoleType::Group ? 0 : 1);
    basicLayout->addRow(tr("Type:"), typeCombo_);
    
    descriptionEdit_ = new QLineEdit(role_->description, this);
    basicLayout->addRow(tr("Description:"), descriptionEdit_);
    
    layout->addWidget(basicGroup);
    
    // Privileges
    auto* privGroup = new QGroupBox(tr("Privileges"), this);
    auto* privLayout = new QVBoxLayout(privGroup);
    
    createdbCheck_ = new QCheckBox(tr("Can create databases"), this);
    createdbCheck_->setChecked(role_->canCreateDB);
    privLayout->addWidget(createdbCheck_);
    
    createroleCheck_ = new QCheckBox(tr("Can create roles"), this);
    createroleCheck_->setChecked(role_->canCreateRole);
    privLayout->addWidget(createroleCheck_);
    
    loginCheck_ = new QCheckBox(tr("Can login"), this);
    loginCheck_->setChecked(role_->canLogin);
    privLayout->addWidget(loginCheck_);
    
    layout->addWidget(privGroup);
    
    layout->addStretch();
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &RoleDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void RoleDialog::onSave() {
    role_->name = nameEdit_->text();
    role_->type = static_cast<RoleType>(typeCombo_->currentData().toInt());
    role_->description = descriptionEdit_->text();
    role_->canCreateDB = createdbCheck_->isChecked();
    role_->canCreateRole = createroleCheck_->isChecked();
    role_->canLogin = loginCheck_->isChecked();
    
    accept();
}

// ============================================================================
// Role Membership Dialog
// ============================================================================

RoleMembershipDialog::RoleMembershipDialog(RoleInfo* role, QWidget* parent)
    : QDialog(parent)
    , role_(role) {
    setupUi();
}

void RoleMembershipDialog::setupUi() {
    setWindowTitle(tr("Role Membership - %1").arg(role_->name));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Current members
    auto* membersGroup = new QGroupBox(tr("Current Members"), this);
    auto* membersLayout = new QVBoxLayout(membersGroup);
    
    membersList_ = new QListWidget(this);
    for (const auto& member : role_->members) {
        membersList_->addItem(member);
    }
    membersLayout->addWidget(membersList_);
    
    auto* removeBtn = new QPushButton(tr("Remove Selected"), this);
    connect(removeBtn, &QPushButton::clicked, this, &RoleMembershipDialog::onRemoveMember);
    membersLayout->addWidget(removeBtn);
    
    layout->addWidget(membersGroup);
    
    // Add members
    auto* addGroup = new QGroupBox(tr("Add Members"), this);
    auto* addLayout = new QVBoxLayout(addGroup);
    
    availableList_ = new QListWidget(this);
    availableList_->addItem("user1");
    availableList_->addItem("user2");
    availableList_->addItem("user3");
    availableList_->setSelectionMode(QAbstractItemView::MultiSelection);
    addLayout->addWidget(availableList_);
    
    adminOptionCheck_ = new QCheckBox(tr("With admin option"), this);
    addLayout->addWidget(adminOptionCheck_);
    
    auto* addBtn = new QPushButton(tr("Add Selected"), this);
    connect(addBtn, &QPushButton::clicked, this, &RoleMembershipDialog::onAddMember);
    addLayout->addWidget(addBtn);
    
    layout->addWidget(addGroup);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void RoleMembershipDialog::onAddMember() {
    auto items = availableList_->selectedItems();
    for (auto* item : items) {
        if (!role_->members.contains(item->text())) {
            role_->members.append(item->text());
            membersList_->addItem(item->text());
        }
    }
}

void RoleMembershipDialog::onRemoveMember() {
    auto currentRow = membersList_->currentRow();
    if (currentRow >= 0 && currentRow < role_->members.size()) {
        role_->members.removeAt(currentRow);
        delete membersList_->takeItem(currentRow);
    }
}

void RoleMembershipDialog::onSave() {
    accept();
}

// ============================================================================
// RoleHierarchyDialog stubs
// ============================================================================

void RoleHierarchyDialog::onRefresh() {}
void RoleHierarchyDialog::onExpandAll() {}
void RoleHierarchyDialog::onCollapseAll() {}
void RoleHierarchyDialog::onExportDiagram() {}
void RoleHierarchyDialog::onPrint() {}

// ============================================================================
// GrantRoleDialog stub
// ============================================================================

void GrantRoleDialog::onGrant() {
    accept();
}

// ============================================================================
// Missing RoleManagementPanel stubs
// ============================================================================

void RoleManagementPanel::onSetAdminOption() {}
void RoleManagementPanel::onExportRoles() {}
void RoleManagementPanel::onImportRoles() {}

} // namespace scratchrobin::ui
