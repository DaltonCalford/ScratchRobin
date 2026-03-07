#include "ui/user_role_manager.h"
#include "backend/session_client.h"
#include "backend/scratchbird_metadata_provider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTableView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QListWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QClipboard>
#include <QApplication>
#include <QRandomGenerator>
#include <QDateTime>

namespace scratchrobin::ui {

// ============================================================================
// UserRoleManagerPanel
// ============================================================================

UserRoleManagerPanel::UserRoleManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("user_role_manager", parent), client_(client) {
    setupUi();
    setupModel();
}

void UserRoleManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Search users, roles, groups..."));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    createUserBtn_ = new QPushButton(tr("New User"), this);
    createRoleBtn_ = new QPushButton(tr("New Role"), this);
    createGroupBtn_ = new QPushButton(tr("New Group"), this);
    editBtn_ = new QPushButton(tr("Edit"), this);
    dropBtn_ = new QPushButton(tr("Drop"), this);
    passwordBtn_ = new QPushButton(tr("Password"), this);
    membershipBtn_ = new QPushButton(tr("Memberships"), this);
    privilegesBtn_ = new QPushButton(tr("Privileges"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createUserBtn_);
    toolbar->addWidget(createRoleBtn_);
    toolbar->addWidget(createGroupBtn_);
    toolbar->addSeparator();
    toolbar->addWidget(editBtn_);
    toolbar->addWidget(dropBtn_);
    toolbar->addSeparator();
    toolbar->addWidget(passwordBtn_);
    toolbar->addWidget(membershipBtn_);
    toolbar->addWidget(privilegesBtn_);
    toolbar->addSeparator();
    toolbar->addWidget(refreshBtn);
    
    mainLayout->addWidget(toolbar);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Principal tree
    principalTree_ = new QTreeView(this);
    principalTree_->setHeaderHidden(false);
    principalTree_->setAlternatingRowColors(true);
    principalTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    splitter->addWidget(principalTree_);
    
    // Details
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setFont(QFont("Consolas", 9));
    detailsEdit_->setMaximumHeight(150);
    detailsEdit_->setPlaceholderText(tr("Select a user, role, or group to see details..."));
    splitter->addWidget(detailsEdit_);
    
    splitter->setSizes({300, 100});
    mainLayout->addWidget(splitter);
    
    // Connections
    connect(createUserBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onCreateUser);
    connect(createRoleBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onCreateRole);
    connect(createGroupBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onCreateGroup);
    connect(editBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onEditPrincipal);
    connect(dropBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onDropPrincipal);
    connect(passwordBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onChangePassword);
    connect(membershipBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onManageMemberships);
    connect(privilegesBtn_, &QPushButton::clicked, this, &UserRoleManagerPanel::onGrantPrivileges);
    connect(refreshBtn, &QPushButton::clicked, this, &UserRoleManagerPanel::refresh);
    connect(filterEdit_, &QLineEdit::textChanged, this, &UserRoleManagerPanel::onFilterChanged);
    connect(principalTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &UserRoleManagerPanel::updateDetails);
}

void UserRoleManagerPanel::setupModel() {
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Login"), tr("Superuser"), tr("CreatedDB"), tr("CreateRole")});
    principalTree_->setModel(model_);
    
    loadPrincipals();
}

void UserRoleManagerPanel::loadPrincipals() {
    model_->removeRows(0, model_->rowCount());
    
    // Mock data - in production, query from backend
    struct MockPrincipal {
        QString name;
        QString type;
        bool login;
        bool superuser;
        bool createdb;
        bool createrole;
    };
    
    QList<MockPrincipal> principals = {
        {"postgres", "User", true, true, true, true},
        {"admin", "Role", false, true, true, true},
        {"app_user", "User", true, false, false, false},
        {"readonly", "Role", false, false, false, false},
        {"developers", "Group", false, false, true, false},
        {"analysts", "Group", false, false, false, false},
        {"backup_user", "User", true, false, false, false},
        {"replication", "Role", false, false, false, false},
    };
    
    for (const auto& p : principals) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(p.name));
        row.append(new QStandardItem(p.type));
        row.append(new QStandardItem(p.login ? tr("Yes") : tr("No")));
        row.append(new QStandardItem(p.superuser ? tr("Yes") : tr("No")));
        row.append(new QStandardItem(p.createdb ? tr("Yes") : tr("No")));
        row.append(new QStandardItem(p.createrole ? tr("Yes") : tr("No")));
        
        // Store principal name for retrieval
        row[0]->setData(p.name, Qt::UserRole);
        row[0]->setData(p.type, Qt::UserRole + 1);
        
        model_->appendRow(row);
    }
    
    principalTree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void UserRoleManagerPanel::refresh() {
    loadPrincipals();
}

void UserRoleManagerPanel::panelActivated() {
    refresh();
}

void UserRoleManagerPanel::onCreateUser() {
    UserRoleEditorDialog dlg(client_, UserRoleEditorDialog::CreateUser, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void UserRoleManagerPanel::onCreateRole() {
    UserRoleEditorDialog dlg(client_, UserRoleEditorDialog::CreateRole, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void UserRoleManagerPanel::onCreateGroup() {
    UserRoleEditorDialog dlg(client_, UserRoleEditorDialog::CreateGroup, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void UserRoleManagerPanel::onEditPrincipal() {
    auto index = principalTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    UserRoleEditorDialog dlg(client_, UserRoleEditorDialog::Edit, name, this);
    dlg.exec();
    refresh();
}

void UserRoleManagerPanel::onDropPrincipal() {
    auto index = principalTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    QString type = model_->item(index.row(), 1)->text();
    
    auto reply = QMessageBox::warning(this, tr("Drop %1").arg(type),
        tr("Are you sure you want to drop %1 '%2'?\n\nThis action cannot be undone.")
            .arg(type.toLower()).arg(name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Execute DROP
        refresh();
    }
}

void UserRoleManagerPanel::onChangePassword() {
    auto index = principalTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    QString type = model_->item(index.row(), 1)->text();
    
    if (type != "User") {
        QMessageBox::information(this, tr("Info"), tr("Only users can have passwords."));
        return;
    }
    
    PasswordChangeDialog dlg(client_, name, this);
    dlg.exec();
}

void UserRoleManagerPanel::onManageMemberships() {
    auto index = principalTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    MembershipManagerDialog dlg(client_, name, this);
    dlg.exec();
    refresh();
}

void UserRoleManagerPanel::onGrantPrivileges() {
    auto index = principalTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    emit privilegesRequested(name);
}

void UserRoleManagerPanel::onFilterChanged(const QString& filter) {
    // Filter the model
    for (int i = 0; i < model_->rowCount(); ++i) {
        QString name = model_->item(i, 0)->text();
        bool visible = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        principalTree_->setRowHidden(i, model_->indexFromItem(model_->item(i, 0)).parent(), !visible);
    }
}

void UserRoleManagerPanel::updateDetails() {
    auto index = principalTree_->currentIndex();
    if (!index.isValid()) {
        detailsEdit_->clear();
        return;
    }
    
    QString name = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    QString type = model_->item(index.row(), 1)->text();
    
    QString details = QString("Name: %1\nType: %2\n\n").arg(name).arg(type);
    
    // Mock details
    if (type == "User") {
        details += "Login: Yes\n";
        details += "Connection Limit: -1 (unlimited)\n";
        details += "Member of: developers, readonly\n";
    } else if (type == "Role") {
        details += "Login: No\n";
        details += "Inherit: Yes\n";
        details += "Members: app_user, backup_user\n";
    } else if (type == "Group") {
        details += "Login: No\n";
        details += "Member count: 5\n";
    }
    
    detailsEdit_->setPlainText(details);
}

// ============================================================================
// UserRoleEditorDialog
// ============================================================================

UserRoleEditorDialog::UserRoleEditorDialog(backend::SessionClient* client,
                                          Mode mode,
                                          const QString& principalName,
                                          QWidget* parent)
    : QDialog(parent), client_(client), mode_(mode), originalName_(principalName) {
    
    isEditing_ = !principalName.isEmpty();
    
    switch (mode) {
        case CreateUser: setWindowTitle(tr("Create New User")); break;
        case CreateRole: setWindowTitle(tr("Create New Role")); break;
        case CreateGroup: setWindowTitle(tr("Create New Group")); break;
        case Edit: setWindowTitle(tr("Edit %1").arg(principalName)); break;
    }
    
    setMinimumSize(600, 500);
    setupUi();
    
    if (isEditing_) {
        loadPrincipal(principalName);
    }
}

void UserRoleEditorDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* tabWidget = new QTabWidget(this);
    
    // Identity tab
    auto* identityTab = new QWidget(this);
    auto* identityLayout = new QFormLayout(identityTab);
    
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("Enter name..."));
    identityLayout->addRow(tr("Name:"), nameEdit_);
    
    if (!isEditing_) {
        typeCombo_ = new QComboBox(this);
        typeCombo_->addItems({tr("USER"), tr("ROLE"), tr("GROUP")});
        identityLayout->addRow(tr("Type:"), typeCombo_);
        
        // Set default based on mode
        if (mode_ == CreateUser) typeCombo_->setCurrentText("USER");
        else if (mode_ == CreateRole) typeCombo_->setCurrentText("ROLE");
        else if (mode_ == CreateGroup) typeCombo_->setCurrentText("GROUP");
    }
    
    tabWidget->addTab(identityTab, tr("&Identity"));
    
    // Privileges tab
    auto* privTab = new QWidget(this);
    auto* privLayout = new QVBoxLayout(privTab);
    
    auto* privGroup = new QGroupBox(tr("System Privileges"), this);
    auto* privForm = new QFormLayout(privGroup);
    
    superuserCheck_ = new QCheckBox(tr("Superuser"), this);
    superuserCheck_->setToolTip(tr("Bypasses all access restrictions"));
    privForm->addRow(superuserCheck_);
    
    createdbCheck_ = new QCheckBox(tr("Create Database"), this);
    privForm->addRow(createdbCheck_);
    
    createroleCheck_ = new QCheckBox(tr("Create Role"), this);
    privForm->addRow(createroleCheck_);
    
    inheritCheck_ = new QCheckBox(tr("Inherit Privileges"), this);
    inheritCheck_->setChecked(true);
    privForm->addRow(inheritCheck_);
    
    loginCheck_ = new QCheckBox(tr("Can Login"), this);
    privForm->addRow(loginCheck_);
    
    replicationCheck_ = new QCheckBox(tr("Replication"), this);
    privForm->addRow(replicationCheck_);
    
    bypassrlsCheck_ = new QCheckBox(tr("Bypass RLS"), this);
    privForm->addRow(bypassrlsCheck_);
    
    privLayout->addWidget(privGroup);
    privLayout->addStretch();
    
    tabWidget->addTab(privTab, tr("&Privileges"));
    
    // Connection tab
    auto* connTab = new QWidget(this);
    auto* connLayout = new QFormLayout(connTab);
    
    connLimitSpin_ = new QSpinBox(this);
    connLimitSpin_->setRange(-1, 1000);
    connLimitSpin_->setValue(-1);
    connLimitSpin_->setSpecialValueText(tr("Unlimited"));
    connLayout->addRow(tr("Connection Limit:"), connLimitSpin_);
    
    if (!isEditing_ || mode_ == CreateUser) {
        auto* pwdLayout = new QHBoxLayout();
        passwordEdit_ = new QLineEdit(this);
        passwordEdit_->setEchoMode(QLineEdit::Password);
        pwdLayout->addWidget(passwordEdit_);
        
        generatePwdBtn_ = new QPushButton(tr("Generate"), this);
        copyPwdBtn_ = new QPushButton(tr("Copy"), this);
        pwdLayout->addWidget(generatePwdBtn_);
        pwdLayout->addWidget(copyPwdBtn_);
        
        connLayout->addRow(tr("Password:"), pwdLayout);
        
        connect(generatePwdBtn_, &QPushButton::clicked, this, &UserRoleEditorDialog::onGeneratePassword);
        connect(copyPwdBtn_, &QPushButton::clicked, this, &UserRoleEditorDialog::onCopyPassword);
    }
    
    validUntilEdit_ = new QDateTimeEdit(this);
    validUntilEdit_->setCalendarPopup(true);
    validUntilEdit_->setDateTime(QDateTime::currentDateTime().addYears(1));
    validUntilEdit_->setSpecialValueText(tr("Never"));
    connLayout->addRow(tr("Valid Until:"), validUntilEdit_);
    
    tabWidget->addTab(connTab, tr("&Connection"));
    
    // Memberships tab
    if (isEditing_) {
        auto* memberTab = new QWidget(this);
        auto* memberLayout = new QHBoxLayout(memberTab);
        
        auto* currentGroup = new QGroupBox(tr("Member Of"), this);
        auto* currentLayout = new QVBoxLayout(currentGroup);
        memberOfList_ = new QListWidget(this);
        currentLayout->addWidget(memberOfList_);
        memberLayout->addWidget(currentGroup);
        
        auto* availableGroup = new QGroupBox(tr("Available Roles"), this);
        auto* availableLayout = new QVBoxLayout(availableGroup);
        availableRolesList_ = new QListWidget(this);
        availableRolesList_->addItems({"admin", "developers", "readonly", "analysts"});
        availableLayout->addWidget(availableRolesList_);
        memberLayout->addWidget(availableGroup);
        
        tabWidget->addTab(memberTab, tr("&Memberships"));
    }
    
    // SQL Preview tab
    auto* previewTab = new QWidget(this);
    auto* previewLayout = new QVBoxLayout(previewTab);
    sqlPreview_ = new QTextEdit(this);
    sqlPreview_->setReadOnly(true);
    sqlPreview_->setFont(QFont("Consolas", 10));
    previewLayout->addWidget(sqlPreview_);
    tabWidget->addTab(previewTab, tr("&SQL Preview"));
    
    mainLayout->addWidget(tabWidget);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* validateBtn = new QPushButton(tr("&Validate"), this);
    auto* saveBtn = new QPushButton(tr("&Save"), this);
    saveBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(tr("&Cancel"), this);
    
    btnLayout->addWidget(validateBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(validateBtn, &QPushButton::clicked, this, &UserRoleEditorDialog::onValidate);
    connect(saveBtn, &QPushButton::clicked, this, &UserRoleEditorDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    // Update preview when values change
    if (typeCombo_) connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                           [this]() { sqlPreview_->setPlainText(generateDdl()); });
    if (nameEdit_) connect(nameEdit_, &QLineEdit::textChanged, 
                          [this]() { sqlPreview_->setPlainText(generateDdl()); });
}

void UserRoleEditorDialog::loadPrincipal(const QString& name) {
    nameEdit_->setText(name);
    nameEdit_->setEnabled(false);
    
    // Mock load
    superuserCheck_->setChecked(false);
    createdbCheck_->setChecked(false);
    createroleCheck_->setChecked(false);
    inheritCheck_->setChecked(true);
    loginCheck_->setChecked(true);
    
    if (memberOfList_) {
        memberOfList_->addItem("developers");
        memberOfList_->addItem("readonly");
    }
}

QString UserRoleEditorDialog::generateDdl() {
    QString ddl;
    
    QString name = nameEdit_->text();
    if (name.isEmpty()) return QString();
    
    QString type = typeCombo_ ? typeCombo_->currentText().toUpper() : "USER";
    
    if (isEditing_) {
        ddl = QString("ALTER %1 %2\n").arg(type).arg(name);
        // Add alter statements for changed attributes
    } else {
        ddl = QString("CREATE %1 %2\n").arg(type).arg(name);
        ddl += "WITH\n";
        
        if (superuserCheck_->isChecked()) ddl += "  SUPERUSER\n";
        else ddl += "  NOSUPERUSER\n";
        
        if (createdbCheck_->isChecked()) ddl += "  CREATEDB\n";
        else ddl += "  NOCREATEDB\n";
        
        if (createroleCheck_->isChecked()) ddl += "  CREATEROLE\n";
        else ddl += "  NOCREATEROLE\n";
        
        if (inheritCheck_->isChecked()) ddl += "  INHERIT\n";
        else ddl += "  NOINHERIT\n";
        
        if (loginCheck_->isChecked()) ddl += "  LOGIN\n";
        else ddl += "  NOLOGIN\n";
        
        if (replicationCheck_->isChecked()) ddl += "  REPLICATION\n";
        else ddl += "  NOREPLICATION\n";
        
        if (bypassrlsCheck_->isChecked()) ddl += "  BYPASSRLS\n";
        else ddl += "  NOBYPASSRLS\n";
        
        if (connLimitSpin_->value() >= 0) {
            ddl += QString("  CONNECTION LIMIT %1\n").arg(connLimitSpin_->value());
        }
        
        if (passwordEdit_ && !passwordEdit_->text().isEmpty()) {
            ddl += QString("  ENCRYPTED PASSWORD '***'\n");
        }
    }
    
    return ddl;
}

void UserRoleEditorDialog::onValidate() {
    sqlPreview_->setPlainText(generateDdl());
    QMessageBox::information(this, tr("Validation"), tr("DDL syntax is valid."));
}

void UserRoleEditorDialog::onSave() {
    if (nameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Name is required."));
        return;
    }
    accept();
}

void UserRoleEditorDialog::onGeneratePassword() {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;
    for (int i = 0; i < 16; ++i) {
        password.append(chars.at(QRandomGenerator::global()->bounded(chars.length())));
    }
    generatedPassword_ = password;
    passwordEdit_->setText(password);
}

void UserRoleEditorDialog::onCopyPassword() {
    if (!generatedPassword_.isEmpty()) {
        QApplication::clipboard()->setText(generatedPassword_);
    }
}

// ============================================================================
// MembershipManagerDialog
// ============================================================================

MembershipManagerDialog::MembershipManagerDialog(backend::SessionClient* client,
                                                const QString& principalName,
                                                QWidget* parent)
    : QDialog(parent), client_(client), principalName_(principalName) {
    
    setWindowTitle(tr("Manage Memberships - %1").arg(principalName));
    setMinimumSize(700, 400);
    
    setupUi();
    loadMemberships();
}

void MembershipManagerDialog::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    
    // Member Of section
    auto* memberOfGroup = new QGroupBox(tr("Member Of (Roles)"), this);
    auto* memberOfLayout = new QVBoxLayout(memberOfGroup);
    
    memberOfList_ = new QListWidget(this);
    memberOfLayout->addWidget(memberOfList_);
    
    auto* memberOfBtnLayout = new QHBoxLayout();
    auto* removeFromRoleBtn = new QPushButton(tr("<< Remove"), this);
    memberOfBtnLayout->addStretch();
    memberOfBtnLayout->addWidget(removeFromRoleBtn);
    memberOfLayout->addLayout(memberOfBtnLayout);
    
    mainLayout->addWidget(memberOfGroup);
    
    // Available Roles section
    auto* availableGroup = new QGroupBox(tr("Available Roles"), this);
    auto* availableLayout = new QVBoxLayout(availableGroup);
    
    availableRolesList_ = new QListWidget(this);
    availableLayout->addWidget(availableRolesList_);
    
    auto* addToRoleBtnLayout = new QHBoxLayout();
    auto* addToRoleBtn = new QPushButton(tr("Add >>"), this);
    addToRoleBtnLayout->addWidget(addToRoleBtn);
    addToRoleBtnLayout->addStretch();
    availableLayout->addLayout(addToRoleBtnLayout);
    
    mainLayout->addWidget(availableGroup);
    
    // Members section (if this is a role)
    auto* membersGroup = new QGroupBox(tr("Members of this Role"), this);
    auto* membersLayout = new QVBoxLayout(membersGroup);
    
    membersList_ = new QListWidget(this);
    membersLayout->addWidget(membersList_);
    
    auto* membersBtnLayout = new QHBoxLayout();
    auto* removeMemberBtn = new QPushButton(tr("<< Remove"), this);
    membersBtnLayout->addStretch();
    membersBtnLayout->addWidget(removeMemberBtn);
    membersLayout->addLayout(membersBtnLayout);
    
    mainLayout->addWidget(membersGroup);
    
    // Available Members section
    auto* availableMembersGroup = new QGroupBox(tr("Available Principals"), this);
    auto* availableMembersLayout = new QVBoxLayout(availableMembersGroup);
    
    availableMembersList_ = new QListWidget(this);
    availableMembersLayout->addWidget(availableMembersList_);
    
    auto* addMemberBtnLayout = new QHBoxLayout();
    auto* addMemberBtn = new QPushButton(tr("Add >>"), this);
    addMemberBtnLayout->addWidget(addMemberBtn);
    addMemberBtnLayout->addStretch();
    availableMembersLayout->addLayout(addMemberBtnLayout);
    
    mainLayout->addWidget(availableMembersGroup);
    
    // Bottom buttons
    auto* bottomLayout = new QVBoxLayout();
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* closeBtn = new QPushButton(tr("Close"), this);
    bottomLayout->addWidget(refreshBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(bottomLayout);
    
    connect(addToRoleBtn, &QPushButton::clicked, this, &MembershipManagerDialog::onAddToRole);
    connect(removeFromRoleBtn, &QPushButton::clicked, this, &MembershipManagerDialog::onRemoveFromRole);
    connect(addMemberBtn, &QPushButton::clicked, this, &MembershipManagerDialog::onAddMember);
    connect(removeMemberBtn, &QPushButton::clicked, this, &MembershipManagerDialog::onRemoveMember);
    connect(refreshBtn, &QPushButton::clicked, this, &MembershipManagerDialog::onRefresh);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void MembershipManagerDialog::loadMemberships() {
    // Mock data
    memberOfList_->addItem("developers");
    memberOfList_->addItem("readonly");
    
    availableRolesList_->addItem("admin");
    availableRolesList_->addItem("analysts");
    availableRolesList_->addItem("backup");
    
    membersList_->addItem("app_user");
    membersList_->addItem("backup_user");
    
    availableMembersList_->addItem("analyst_1");
    availableMembersList_->addItem("analyst_2");
}

void MembershipManagerDialog::onAddToRole() {
    auto* item = availableRolesList_->currentItem();
    if (item) {
        memberOfList_->addItem(item->text());
        delete item;
    }
}

void MembershipManagerDialog::onRemoveFromRole() {
    auto* item = memberOfList_->currentItem();
    if (item) {
        availableRolesList_->addItem(item->text());
        delete item;
    }
}

void MembershipManagerDialog::onAddMember() {
    auto* item = availableMembersList_->currentItem();
    if (item) {
        membersList_->addItem(item->text());
        delete item;
    }
}

void MembershipManagerDialog::onRemoveMember() {
    auto* item = membersList_->currentItem();
    if (item) {
        availableMembersList_->addItem(item->text());
        delete item;
    }
}

void MembershipManagerDialog::onRefresh() {
    memberOfList_->clear();
    availableRolesList_->clear();
    membersList_->clear();
    availableMembersList_->clear();
    loadMemberships();
}

// ============================================================================
// PasswordChangeDialog
// ============================================================================

PasswordChangeDialog::PasswordChangeDialog(backend::SessionClient* client,
                                          const QString& userName,
                                          QWidget* parent)
    : QDialog(parent), client_(client), userName_(userName) {
    
    setWindowTitle(tr("Change Password - %1").arg(userName));
    setMinimumSize(400, 200);
    
    setupUi();
}

void PasswordChangeDialog::setupUi() {
    auto* mainLayout = new QFormLayout(this);
    
    // Note: Current password might not be verifiable, but good for UX
    currentPwdEdit_ = new QLineEdit(this);
    currentPwdEdit_->setEchoMode(QLineEdit::Password);
    mainLayout->addRow(tr("Current Password:"), currentPwdEdit_);
    
    auto* newPwdLayout = new QHBoxLayout();
    newPwdEdit_ = new QLineEdit(this);
    newPwdEdit_->setEchoMode(QLineEdit::Password);
    newPwdLayout->addWidget(newPwdEdit_);
    
    generateBtn_ = new QPushButton(tr("Generate"), this);
    copyBtn_ = new QPushButton(tr("Copy"), this);
    newPwdLayout->addWidget(generateBtn_);
    newPwdLayout->addWidget(copyBtn_);
    
    mainLayout->addRow(tr("New Password:"), newPwdLayout);
    
    confirmPwdEdit_ = new QLineEdit(this);
    confirmPwdEdit_->setEchoMode(QLineEdit::Password);
    mainLayout->addRow(tr("Confirm Password:"), confirmPwdEdit_);
    
    expireCheck_ = new QCheckBox(tr("Expire password immediately (user must change on next login)"), this);
    mainLayout->addRow(expireCheck_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* saveBtn = new QPushButton(tr("&Save"), this);
    saveBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(tr("&Cancel"), this);
    
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addRow(btnLayout);
    
    connect(generateBtn_, &QPushButton::clicked, this, &PasswordChangeDialog::onGenerate);
    connect(copyBtn_, &QPushButton::clicked, this, &PasswordChangeDialog::onCopy);
    connect(saveBtn, &QPushButton::clicked, this, &PasswordChangeDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString PasswordChangeDialog::generateSecurePassword() {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;
    for (int i = 0; i < 16; ++i) {
        password.append(chars.at(QRandomGenerator::global()->bounded(chars.length())));
    }
    return password;
}

void PasswordChangeDialog::onGenerate() {
    QString pwd = generateSecurePassword();
    newPwdEdit_->setText(pwd);
    confirmPwdEdit_->setText(pwd);
}

void PasswordChangeDialog::onCopy() {
    if (!newPwdEdit_->text().isEmpty()) {
        QApplication::clipboard()->setText(newPwdEdit_->text());
    }
}

void PasswordChangeDialog::onSave() {
    if (newPwdEdit_->text() != confirmPwdEdit_->text()) {
        QMessageBox::warning(this, tr("Error"), tr("Passwords do not match."));
        return;
    }
    
    if (newPwdEdit_->text().length() < 8) {
        QMessageBox::warning(this, tr("Error"), tr("Password must be at least 8 characters."));
        return;
    }
    
    accept();
}

// ============================================================================
// EffectivePrivilegesDialog
// ============================================================================

EffectivePrivilegesDialog::EffectivePrivilegesDialog(backend::SessionClient* client,
                                                     const QString& principalName,
                                                     QWidget* parent)
    : QDialog(parent), client_(client), principalName_(principalName) {
    
    setWindowTitle(tr("Effective Privileges - %1").arg(principalName));
    setMinimumSize(700, 500);
    
    setupUi();
    loadPrivileges();
}

void EffectivePrivilegesDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Table
    privilegesTable_ = new QTableView(this);
    privilegesTable_->setAlternatingRowColors(true);
    privilegesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Object"), tr("Type"), tr("Privileges"), tr("Granted Via")});
    privilegesTable_->setModel(model_);
    
    mainLayout->addWidget(privilegesTable_);
    
    // Summary
    sqlPreview_ = new QTextEdit(this);
    sqlPreview_->setReadOnly(true);
    sqlPreview_->setFont(QFont("Consolas", 9));
    sqlPreview_->setMaximumHeight(100);
    sqlPreview_->setPlaceholderText(tr("Effective privilege grants..."));
    mainLayout->addWidget(sqlPreview_);
    
    // Close button
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
}

void EffectivePrivilegesDialog::loadPrivileges() {
    // Mock data
    QList<QList<QString>> privileges = {
        {"public.*", "Schema", "USAGE, CREATE", "PUBLIC"},
        {"public.customers", "Table", "SELECT, INSERT, UPDATE", "developers"},
        {"public.orders", "Table", "SELECT", "readonly"},
        {"public.calculate_total", "Function", "EXECUTE", "developers"},
        {"sales.*", "Schema", "USAGE", "sales_team"},
    };
    
    for (const auto& row : privileges) {
        QList<QStandardItem*> items;
        for (const auto& cell : row) {
            items.append(new QStandardItem(cell));
        }
        model_->appendRow(items);
    }
    
    privilegesTable_->horizontalHeader()->setStretchLastSection(true);
    privilegesTable_->resizeColumnsToContents();
    
    sqlPreview_->setPlainText(QString("-- Effective privileges for %1\n"
                                      "-- Direct grants + inherited from roles\n"
                                      "GRANT USAGE, CREATE ON SCHEMA public TO %1;\n"
                                      "GRANT SELECT, INSERT, UPDATE ON TABLE public.customers TO %1;\n"
                                      "GRANT SELECT ON TABLE public.orders TO %1;\n")
                                  .arg(principalName_));
}

} // namespace scratchrobin::ui
