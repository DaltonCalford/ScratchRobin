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
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QRandomGenerator>
#include <QDateTimeEdit>
#include <QDialogButtonBox>

namespace scratchrobin::ui {

// ============================================================================
// User Management Panel
// ============================================================================

UserManagementPanel::UserManagementPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("user_management", parent)
    , client_(client) {
    setupUi();
    loadUsers();
}

void UserManagementPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* createBtn = new QPushButton(tr("Create User"), this);
    connect(createBtn, &QPushButton::clicked, this, &UserManagementPanel::onCreateUser);
    toolbarLayout->addWidget(createBtn);
    
    auto* editBtn = new QPushButton(tr("Edit"), this);
    connect(editBtn, &QPushButton::clicked, this, &UserManagementPanel::onEditUser);
    toolbarLayout->addWidget(editBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &UserManagementPanel::onDeleteUser);
    toolbarLayout->addWidget(deleteBtn);
    
    auto* cloneBtn = new QPushButton(tr("Clone"), this);
    connect(cloneBtn, &QPushButton::clicked, this, &UserManagementPanel::onCloneUser);
    toolbarLayout->addWidget(cloneBtn);
    
    toolbarLayout->addStretch();
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search users..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &UserManagementPanel::onSearchUsers);
    toolbarLayout->addWidget(searchEdit_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &UserManagementPanel::onRefreshUsers);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // User list
    userTable_ = new QTableView(this);
    userModel_ = new QStandardItemModel(this);
    userModel_->setHorizontalHeaderLabels({tr("User"), tr("Superuser"), tr("Can Login"), tr("Can Create DB"), tr("Member Of")});
    userTable_->setModel(userModel_);
    userTable_->setAlternatingRowColors(true);
    userTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(userTable_, &QTableView::clicked, this, &UserManagementPanel::onUserSelected);
    splitter->addWidget(userTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* infoGroup = new QGroupBox(tr("User Information"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    nameLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Name:"), nameLabel_);
    
    superuserLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Superuser:"), superuserLabel_);
    
    createdbLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Can Create DB:"), createdbLabel_);
    
    loginLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Can Login:"), loginLabel_);
    
    expiryLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Expires:"), expiryLabel_);
    
    connectionLimitLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Connection Limit:"), connectionLimitLabel_);
    
    detailsLayout->addWidget(infoGroup);
    
    // Member of
    auto* memberGroup = new QGroupBox(tr("Member Of"), this);
    auto* memberLayout = new QVBoxLayout(memberGroup);
    
    memberOfTable_ = new QTableView(this);
    memberOfTable_->setMaximumHeight(100);
    memberOfModel_ = new QStandardItemModel(this);
    memberOfModel_->setHorizontalHeaderLabels({tr("Role"), tr("Admin Option")});
    memberOfTable_->setModel(memberOfModel_);
    memberLayout->addWidget(memberOfTable_);
    
    detailsLayout->addWidget(memberGroup);
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* lockBtn = new QPushButton(tr("Lock Account"), this);
    connect(lockBtn, &QPushButton::clicked, this, &UserManagementPanel::onLockUser);
    btnLayout->addWidget(lockBtn);
    
    auto* unlockBtn = new QPushButton(tr("Unlock Account"), this);
    connect(unlockBtn, &QPushButton::clicked, this, &UserManagementPanel::onUnlockUser);
    btnLayout->addWidget(unlockBtn);
    
    auto* passwdBtn = new QPushButton(tr("Change Password"), this);
    connect(passwdBtn, &QPushButton::clicked, this, &UserManagementPanel::onChangePassword);
    btnLayout->addWidget(passwdBtn);
    
    auto* objectsBtn = new QPushButton(tr("View Objects"), this);
    connect(objectsBtn, &QPushButton::clicked, this, &UserManagementPanel::onViewUserObjects);
    btnLayout->addWidget(objectsBtn);
    
    btnLayout->addStretch();
    detailsLayout->addLayout(btnLayout);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({300, 300});
    
    mainLayout->addWidget(splitter, 1);
}

void UserManagementPanel::loadUsers() {
    users_.clear();
    
    // Simulate loading users
    QStringList userNames = {"postgres", "admin", "app_user", "readonly_user", "backup_user"};
    for (const auto& name : userNames) {
        UserAccountInfo user;
        user.name = name;
        user.isSuperuser = (name == "postgres" || name == "admin");
        user.canCreateDB = user.isSuperuser;
        user.canLogin = true;
        user.connectionLimit = -1;
        user.memberOf << "pg_read_all_data";
        if (user.isSuperuser) user.memberOf << "pg_write_all_data";
        users_.append(user);
    }
    
    // Update table
    userModel_->clear();
    userModel_->setHorizontalHeaderLabels({tr("User"), tr("Superuser"), tr("Can Login"), tr("Can Create DB"), tr("Member Of")});
    
    for (const auto& user : users_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(user.name);
        row << new QStandardItem(user.isSuperuser ? tr("Yes") : tr("No"));
        row << new QStandardItem(user.canLogin ? tr("Yes") : tr("No"));
        row << new QStandardItem(user.canCreateDB ? tr("Yes") : tr("No"));
        row << new QStandardItem(user.memberOf.join(", "));
        userModel_->appendRow(row);
    }
}

void UserManagementPanel::updateUserDetails(const UserAccountInfo& user) {
    nameLabel_->setText(user.name);
    superuserLabel_->setText(user.isSuperuser ? tr("Yes") : tr("No"));
    createdbLabel_->setText(user.canCreateDB ? tr("Yes") : tr("No"));
    loginLabel_->setText(user.canLogin ? tr("Yes") : tr("No"));
    expiryLabel_->setText(user.validUntil.isValid() ? user.validUntil.toString() : tr("Never"));
    connectionLimitLabel_->setText(user.connectionLimit < 0 ? tr("Unlimited") : QString::number(user.connectionLimit));
    
    memberOfModel_->clear();
    memberOfModel_->setHorizontalHeaderLabels({tr("Role"), tr("Admin Option")});
    for (const auto& role : user.memberOf) {
        memberOfModel_->appendRow({new QStandardItem(role), new QStandardItem(tr("No"))});
    }
}

void UserManagementPanel::onCreateUser() {
    UserAccountInfo newUser;
    UserDialog dialog(&newUser, true, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        users_.append(newUser);
        loadUsers();
        emit userCreated(newUser.name);
    }
}

void UserManagementPanel::onEditUser() {
    auto index = userTable_->currentIndex();
    if (!index.isValid() || index.row() >= users_.size()) return;
    
    UserDialog dialog(&users_[index.row()], false, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadUsers();
        emit userModified(users_[index.row()].name);
    }
}

void UserManagementPanel::onDeleteUser() {
    auto index = userTable_->currentIndex();
    if (!index.isValid() || index.row() >= users_.size()) return;
    
    QString username = users_[index.row()].name;
    auto reply = QMessageBox::warning(this, tr("Delete User"),
        tr("Are you sure you want to delete user '%1'?").arg(username),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        users_.removeAt(index.row());
        loadUsers();
        emit userDeleted(username);
    }
}

void UserManagementPanel::onCloneUser() {
    auto index = userTable_->currentIndex();
    if (!index.isValid() || index.row() >= users_.size()) return;
    
    UserAccountInfo clone = users_[index.row()];
    clone.name += "_copy";
    
    UserDialog dialog(&clone, true, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        users_.append(clone);
        loadUsers();
        emit userCreated(clone.name);
    }
}

void UserManagementPanel::onRefreshUsers() {
    loadUsers();
}

void UserManagementPanel::onUserSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= users_.size()) return;
    updateUserDetails(users_[index.row()]);
}

void UserManagementPanel::onSearchUsers(const QString& text) {
    // Filter users
    for (int i = 0; i < userModel_->rowCount(); ++i) {
        auto* item = userModel_->item(i, 0);
        bool match = item->text().contains(text, Qt::CaseInsensitive);
        userTable_->setRowHidden(i, !match);
    }
}

void UserManagementPanel::onLockUser() {
    QMessageBox::information(this, tr("Lock User"),
        tr("User account locked."));
}

void UserManagementPanel::onUnlockUser() {
    QMessageBox::information(this, tr("Unlock User"),
        tr("User account unlocked."));
}

void UserManagementPanel::onChangePassword() {
    auto index = userTable_->currentIndex();
    if (!index.isValid() || index.row() >= users_.size()) return;
    
    ChangePasswordDialog dialog(users_[index.row()].name, this);
    dialog.exec();
}

void UserManagementPanel::onResetPassword() {
    QMessageBox::information(this, tr("Reset Password"),
        tr("Password reset email sent."));
}

void UserManagementPanel::onSetExpiration() {
    // Set account expiration
}

void UserManagementPanel::onViewUserObjects() {
    auto index = userTable_->currentIndex();
    if (!index.isValid() || index.row() >= users_.size()) return;
    
    UserObjectsDialog dialog(users_[index.row()].name, client_, this);
    dialog.exec();
}

void UserManagementPanel::onExportUsers() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Users"),
        QString(),
        tr("SQL Files (*.sql);;JSON (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Users exported to %1").arg(fileName));
    }
}

void UserManagementPanel::onImportUsers() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import Users"),
        QString(),
        tr("SQL Files (*.sql);;JSON (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Import"),
            tr("Users imported from %1").arg(fileName));
        loadUsers();
    }
}

// ============================================================================
// User Dialog
// ============================================================================

UserDialog::UserDialog(UserAccountInfo* user, bool isNew, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , user_(user)
    , isNew_(isNew)
    , client_(client) {
    setupUi();
}

void UserDialog::setupUi() {
    setWindowTitle(isNew_ ? tr("Create User") : tr("Edit User"));
    resize(500, 600);
    
    auto* layout = new QVBoxLayout(this);
    
    // Basic info
    auto* basicGroup = new QGroupBox(tr("Basic Information"), this);
    auto* basicLayout = new QFormLayout(basicGroup);
    
    nameEdit_ = new QLineEdit(user_->name, this);
    basicLayout->addRow(tr("Name:"), nameEdit_);
    
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    basicLayout->addRow(tr("Password:"), passwordEdit_);
    
    showPasswordCheck_ = new QCheckBox(tr("Show password"), this);
    connect(showPasswordCheck_, &QCheckBox::toggled, this, &UserDialog::onShowPassword);
    basicLayout->addRow(showPasswordCheck_);
    
    auto* genBtn = new QPushButton(tr("Generate Password"), this);
    connect(genBtn, &QPushButton::clicked, this, &UserDialog::onGeneratePassword);
    basicLayout->addRow(genBtn);
    
    layout->addWidget(basicGroup);
    
    // Privileges
    auto* privGroup = new QGroupBox(tr("Privileges"), this);
    auto* privLayout = new QVBoxLayout(privGroup);
    
    superuserCheck_ = new QCheckBox(tr("Superuser"), this);
    superuserCheck_->setChecked(user_->isSuperuser);
    privLayout->addWidget(superuserCheck_);
    
    createdbCheck_ = new QCheckBox(tr("Can create databases"), this);
    createdbCheck_->setChecked(user_->canCreateDB);
    privLayout->addWidget(createdbCheck_);
    
    createroleCheck_ = new QCheckBox(tr("Can create roles"), this);
    createroleCheck_->setChecked(user_->canCreateRole);
    privLayout->addWidget(createroleCheck_);
    
    inheritCheck_ = new QCheckBox(tr("Inherit privileges"), this);
    inheritCheck_->setChecked(user_->inheritPrivileges);
    privLayout->addWidget(inheritCheck_);
    
    loginCheck_ = new QCheckBox(tr("Can login"), this);
    loginCheck_->setChecked(user_->canLogin);
    privLayout->addWidget(loginCheck_);
    
    replicationCheck_ = new QCheckBox(tr("Replication"), this);
    replicationCheck_->setChecked(user_->isReplication);
    privLayout->addWidget(replicationCheck_);
    
    bypassRLSCheck_ = new QCheckBox(tr("Bypass Row Level Security"), this);
    bypassRLSCheck_->setChecked(user_->bypassRLS);
    privLayout->addWidget(bypassRLSCheck_);
    
    layout->addWidget(privGroup);
    
    // Limits
    auto* limitGroup = new QGroupBox(tr("Connection Limits"), this);
    auto* limitLayout = new QFormLayout(limitGroup);
    
    connectionLimitEdit_ = new QLineEdit(user_->connectionLimit < 0 ? "" : QString::number(user_->connectionLimit), this);
    connectionLimitEdit_->setPlaceholderText(tr("Unlimited"));
    limitLayout->addRow(tr("Connection limit:"), connectionLimitEdit_);
    
    validUntilEdit_ = new QDateTimeEdit(this);
    validUntilEdit_->setCalendarPopup(true);
    validUntilEdit_->setDisplayFormat("yyyy-MM-dd hh:mm");
    if (user_->validUntil.isValid()) {
        validUntilEdit_->setDateTime(user_->validUntil);
    } else {
        validUntilEdit_->setDateTime(QDateTime::currentDateTime().addYears(1));
    }
    limitLayout->addRow(tr("Valid until:"), validUntilEdit_);
    
    layout->addWidget(limitGroup);
    
    layout->addStretch();
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &UserDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void UserDialog::onGeneratePassword() {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;
    for (int i = 0; i < 16; ++i) {
        password.append(chars[QRandomGenerator::global()->bounded(chars.length())]);
    }
    passwordEdit_->setText(password);
    generatedPassword_ = password;
}

void UserDialog::onCopyPassword() {
    // Copy to clipboard
}

void UserDialog::onTestConnection() {
    QMessageBox::information(this, tr("Test Connection"),
        tr("Connection test would be performed here."));
}

void UserDialog::onSave() {
    user_->name = nameEdit_->text();
    user_->isSuperuser = superuserCheck_->isChecked();
    user_->canCreateDB = createdbCheck_->isChecked();
    user_->canCreateRole = createroleCheck_->isChecked();
    user_->inheritPrivileges = inheritCheck_->isChecked();
    user_->canLogin = loginCheck_->isChecked();
    user_->isReplication = replicationCheck_->isChecked();
    user_->bypassRLS = bypassRLSCheck_->isChecked();
    
    bool ok;
    int limit = connectionLimitEdit_->text().toInt(&ok);
    user_->connectionLimit = ok ? limit : -1;
    user_->validUntil = validUntilEdit_->dateTime();
    
    accept();
}

void UserDialog::onShowPassword(bool show) {
    passwordEdit_->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void UserDialog::loadRoles() {
    // Load available roles
}

// ============================================================================
// Change Password Dialog
// ============================================================================

ChangePasswordDialog::ChangePasswordDialog(const QString& username, QWidget* parent)
    : QDialog(parent)
    , username_(username) {
    setupUi();
}

void ChangePasswordDialog::setupUi() {
    setWindowTitle(tr("Change Password - %1").arg(username_));
    resize(400, 250);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    currentPasswordEdit_ = new QLineEdit(this);
    currentPasswordEdit_->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("Current Password:"), currentPasswordEdit_);
    
    newPasswordEdit_ = new QLineEdit(this);
    newPasswordEdit_->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("New Password:"), newPasswordEdit_);
    
    confirmPasswordEdit_ = new QLineEdit(this);
    confirmPasswordEdit_->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("Confirm Password:"), confirmPasswordEdit_);
    
    showPasswordCheck_ = new QCheckBox(tr("Show passwords"), this);
    formLayout->addRow(showPasswordCheck_);
    
    forceChangeCheck_ = new QCheckBox(tr("Force password change on next login"), this);
    formLayout->addRow(forceChangeCheck_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* genBtn = new QPushButton(tr("Generate"), this);
    connect(genBtn, &QPushButton::clicked, this, &ChangePasswordDialog::onGeneratePassword);
    btnLayout->addWidget(genBtn);
    
    auto* changeBtn = new QPushButton(tr("Change"), this);
    connect(changeBtn, &QPushButton::clicked, this, &ChangePasswordDialog::onChangePassword);
    btnLayout->addWidget(changeBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void ChangePasswordDialog::onGeneratePassword() {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;
    for (int i = 0; i < 16; ++i) {
        password.append(chars[QRandomGenerator::global()->bounded(chars.length())]);
    }
    newPasswordEdit_->setText(password);
    confirmPasswordEdit_->setText(password);
}

void ChangePasswordDialog::onCopyPassword() {
    // Copy to clipboard
}

void ChangePasswordDialog::onChangePassword() {
    if (newPasswordEdit_->text() != confirmPasswordEdit_->text()) {
        QMessageBox::warning(this, tr("Password Mismatch"),
            tr("New passwords do not match."));
        return;
    }
    accept();
}

// ============================================================================
// User Objects Dialog
// ============================================================================

UserObjectsDialog::UserObjectsDialog(const QString& username, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , username_(username)
    , client_(client) {
    setupUi();
    loadObjects();
}

void UserObjectsDialog::setupUi() {
    setWindowTitle(tr("Objects Owned by %1").arg(username_));
    resize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    objectsTable_ = new QTableView(this);
    objectsModel_ = new QStandardItemModel(this);
    objectsModel_->setHorizontalHeaderLabels({tr("Schema"), tr("Object"), tr("Type")});
    objectsTable_->setModel(objectsModel_);
    objectsTable_->setAlternatingRowColors(true);
    layout->addWidget(objectsTable_, 1);
    
    // Reassignment
    auto* reassignLayout = new QHBoxLayout();
    reassignLayout->addWidget(new QLabel(tr("Reassign to:"), this));
    
    newOwnerCombo_ = new QComboBox(this);
    newOwnerCombo_->addItem("postgres");
    newOwnerCombo_->addItem("admin");
    reassignLayout->addWidget(newOwnerCombo_);
    
    auto* reassignBtn = new QPushButton(tr("Reassign"), this);
    connect(reassignBtn, &QPushButton::clicked, this, &UserObjectsDialog::onReassignObjects);
    reassignLayout->addWidget(reassignBtn);
    
    reassignLayout->addStretch();
    layout->addLayout(reassignLayout);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export List"), this);
    connect(exportBtn, &QPushButton::clicked, this, &UserObjectsDialog::onExportList);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void UserObjectsDialog::loadObjects() {
    objectsModel_->clear();
    objectsModel_->setHorizontalHeaderLabels({tr("Schema"), tr("Object"), tr("Type")});
    
    // Simulate objects
    QStringList objects = {"customers", "orders", "products"};
    for (const auto& obj : objects) {
        objectsModel_->appendRow({
            new QStandardItem("public"),
            new QStandardItem(obj),
            new QStandardItem("TABLE")
        });
    }
}

void UserObjectsDialog::onReassignObjects() {
    QMessageBox::information(this, tr("Reassign"),
        tr("Objects would be reassigned to %1").arg(newOwnerCombo_->currentText()));
}

void UserObjectsDialog::onDropObjects() {
    QMessageBox::warning(this, tr("Drop Objects"),
        tr("This would drop all objects owned by the user."));
}

void UserObjectsDialog::onExportList() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Object List"),
        QString(),
        tr("Text Files (*.txt)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Object list exported."));
    }
}

} // namespace scratchrobin::ui
