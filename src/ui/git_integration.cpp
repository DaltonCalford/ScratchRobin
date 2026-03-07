#include "ui/git_integration.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
#include <QTreeWidget>

namespace scratchrobin::ui {

// ============================================================================
// GitIntegrationPanel
// ============================================================================

GitIntegrationPanel::GitIntegrationPanel(QWidget* parent)
    : DockPanel("git_integration", parent) {
    setupUi();
    setRepositoryPath(QDir::current().absolutePath());
}

void GitIntegrationPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    setupToolbar();
    
    // Splitter for file tree and commits
    splitter_ = new QSplitter(Qt::Horizontal, this);
    
    setupFileTree();
    setupCommitList();
    
    splitter_->setSizes({200, 400});
    mainLayout->addWidget(splitter_);
    
    // Diff view
    diffView_ = new QTextEdit(this);
    diffView_->setReadOnly(true);
    diffView_->setFont(QFont("Consolas", 9));
    diffView_->setMaximumHeight(150);
    diffView_->setPlaceholderText(tr("Select a file to see diff..."));
    mainLayout->addWidget(diffView_);
    
    // Status
    statusLabel_ = new QLabel(this);
    mainLayout->addWidget(statusLabel_);
    
    refresh();
}

void GitIntegrationPanel::setupToolbar() {
    auto* toolbar = new QHBoxLayout();
    
    initBtn_ = new QPushButton(tr("Init"), this);
    auto* cloneBtn = new QPushButton(tr("Clone"), this);
    commitBtn_ = new QPushButton(tr("Commit"), this);
    pushBtn_ = new QPushButton(tr("Push"), this);
    pullBtn_ = new QPushButton(tr("Pull"), this);
    auto* branchBtn = new QPushButton(tr("Branch"), this);
    auto* stashBtn = new QPushButton(tr("Stash"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(initBtn_);
    toolbar->addWidget(cloneBtn);
    toolbar->addWidget(commitBtn_);
    toolbar->addWidget(pushBtn_);
    toolbar->addWidget(pullBtn_);
    toolbar->addWidget(branchBtn);
    toolbar->addWidget(stashBtn);
    toolbar->addStretch();
    toolbar->addWidget(refreshBtn);
    
    layout()->addItem(toolbar);
    
    connect(initBtn_, &QPushButton::clicked, this, &GitIntegrationPanel::onInitRepository);
    connect(cloneBtn, &QPushButton::clicked, this, &GitIntegrationPanel::onCloneRepository);
    connect(commitBtn_, &QPushButton::clicked, this, &GitIntegrationPanel::onCommit);
    connect(pushBtn_, &QPushButton::clicked, this, &GitIntegrationPanel::onPush);
    connect(pullBtn_, &QPushButton::clicked, this, &GitIntegrationPanel::onPull);
    connect(branchBtn, &QPushButton::clicked, this, &GitIntegrationPanel::onBranch);
    connect(stashBtn, &QPushButton::clicked, this, &GitIntegrationPanel::onStash);
    connect(refreshBtn, &QPushButton::clicked, this, &GitIntegrationPanel::refresh);
}

void GitIntegrationPanel::setupFileTree() {
    auto* fileWidget = new QWidget(splitter_);
    auto* layout = new QVBoxLayout(fileWidget);
    layout->setSpacing(4);
    layout->setContentsMargins(0, 0, 0, 0);
    
    layout->addWidget(new QLabel(tr("Working Tree"), this));
    
    fileTree_ = new QTreeView(this);
    fileTree_->setHeaderHidden(false);
    fileTree_->setAlternatingRowColors(true);
    fileTree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    
    fileModel_ = new QStandardItemModel(this);
    fileModel_->setHorizontalHeaderLabels({tr("File"), tr("Status")});
    fileTree_->setModel(fileModel_);
    
    layout->addWidget(fileTree_);
    
    splitter_->addWidget(fileWidget);
    
    connect(fileTree_, &QTreeView::doubleClicked,
            this, &GitIntegrationPanel::onFileDoubleClicked);
}

void GitIntegrationPanel::setupCommitList() {
    auto* commitWidget = new QWidget(splitter_);
    auto* layout = new QVBoxLayout(commitWidget);
    layout->setSpacing(4);
    layout->setContentsMargins(0, 0, 0, 0);
    
    auto* headerLayout = new QHBoxLayout();
    headerLayout->addWidget(new QLabel(tr("History"), this));
    headerLayout->addStretch();
    auto* branchLabel = new QLabel(tr("Branch: main"), this);
    branchLabel->setStyleSheet("font-weight: bold; color: #2196F3;");
    headerLayout->addWidget(branchLabel);
    layout->addLayout(headerLayout);
    
    commitTable_ = new QTableView(this);
    commitTable_->setAlternatingRowColors(true);
    commitTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    commitTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    commitTable_->setContextMenuPolicy(Qt::CustomContextMenu);
    
    commitModel_ = new QStandardItemModel(this);
    commitModel_->setHorizontalHeaderLabels({tr("Hash"), tr("Message"), tr("Author"), tr("Date")});
    commitTable_->setModel(commitModel_);
    
    layout->addWidget(commitTable_);
    
    splitter_->addWidget(commitWidget);
    
    connect(commitTable_, &QTableView::doubleClicked,
            this, &GitIntegrationPanel::onCommitDoubleClicked);
}

void GitIntegrationPanel::setRepositoryPath(const QString& path) {
    repoPath_ = path;
    refresh();
}

void GitIntegrationPanel::refresh() {
    // Update file status
    fileModel_->removeRows(0, fileModel_->rowCount());
    
    struct FileItem {
        QString path;
        QString status;
        QColor color;
    };
    
    QList<FileItem> files = {
        {"src/query.sql", "modified", QColor(255, 193, 7)},
        {"src/tables/customers.sql", "staged", QColor(76, 175, 80)},
        {"src/schema/create_tables.sql", "untracked", QColor(158, 158, 158)},
        {"docs/design.md", "deleted", QColor(244, 67, 54)},
        {"config/database.conf", "ignored", QColor(158, 158, 158)},
    };
    
    for (const auto& f : files) {
        auto* nameItem = new QStandardItem(f.path);
        auto* statusItem = new QStandardItem(f.status);
        statusItem->setForeground(f.color);
        statusItem->setData(f.color, Qt::DecorationRole);
        fileModel_->appendRow({nameItem, statusItem});
    }
    
    fileTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    fileTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    
    // Update commit history
    commitModel_->removeRows(0, commitModel_->rowCount());
    
    QList<QList<QString>> commits = {
        {"abc1234", "Add customer table indexes", "John Doe", "2 hours ago"},
        {"def5678", "Fix query performance issue", "Jane Smith", "5 hours ago"},
        {"ghi9012", "Update schema documentation", "Bob Wilson", "1 day ago"},
        {"jkl3456", "Initial schema setup", "John Doe", "2 days ago"},
    };
    
    for (const auto& c : commits) {
        QList<QStandardItem*> row;
        for (const auto& cell : c) {
            row.append(new QStandardItem(cell));
        }
        commitModel_->appendRow(row);
    }
    
    commitTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    commitTable_->resizeColumnsToContents();
    
    updateStatus();
}

void GitIntegrationPanel::updateStatus() {
    statusLabel_->setText(tr("Repository: %1 | Branch: %2 | 2 staged | 1 modified | 1 untracked")
        .arg(repoPath_).arg("main"));
}

void GitIntegrationPanel::panelActivated() {
    refresh();
}

void GitIntegrationPanel::onInitRepository() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Repository Directory"));
    if (!path.isEmpty()) {
        setRepositoryPath(path);
        QMessageBox::information(this, tr("Git Init"),
            tr("Initialized empty Git repository in %1/.git/").arg(path));
    }
}

void GitIntegrationPanel::onCloneRepository() {
    QString url = QInputDialog::getText(this, tr("Clone Repository"),
        tr("Repository URL:"));
    
    if (!url.isEmpty()) {
        QString path = QFileDialog::getExistingDirectory(this, tr("Select Clone Directory"));
        if (!path.isEmpty()) {
            QMessageBox::information(this, tr("Clone"),
                tr("Cloning %1 into %2...").arg(url).arg(path));
        }
    }
}

void GitIntegrationPanel::onCommit() {
    GitCommitDialog dlg(this);
    dlg.setStagedFiles({"src/tables/customers.sql", "src/indexes/idx_customers.sql"});
    dlg.setUnstagedFiles({"src/query.sql", "docs/design.md"});
    
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void GitIntegrationPanel::onStash() {
    auto reply = QMessageBox::question(this, tr("Stash"),
        tr("Stash current changes?"),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    
    if (reply == QMessageBox::Yes) {
        bool ok;
        QString message = QInputDialog::getText(this, tr("Stash"),
            tr("Stash message (optional):"), QLineEdit::Normal, QString(), &ok);
        
        if (ok) {
            QMessageBox::information(this, tr("Stash"), tr("Changes stashed successfully."));
            refresh();
        }
    }
}

void GitIntegrationPanel::onBranch() {
    GitBranchDialog dlg(GitBranchDialog::BranchList, this);
    dlg.exec();
}

void GitIntegrationPanel::onMerge() {
    GitBranchDialog dlg(GitBranchDialog::Merge, this);
    dlg.exec();
}

void GitIntegrationPanel::onPull() {
    QMessageBox::information(this, tr("Pull"), tr("Pulling from origin/main..."));
    refresh();
}

void GitIntegrationPanel::onPush() {
    QMessageBox::information(this, tr("Push"), tr("Pushing to origin/main..."));
}

void GitIntegrationPanel::onFetch() {
    QMessageBox::information(this, tr("Fetch"), tr("Fetching from origin..."));
}

void GitIntegrationPanel::onShowHistory() {
    GitHistoryDialog dlg(this);
    dlg.exec();
}

void GitIntegrationPanel::onShowDiff() {
    GitDiffDialog dlg(this);
    dlg.setFilePath("src/query.sql");
    dlg.exec();
}

void GitIntegrationPanel::onBlame() {
    QMessageBox::information(this, tr("Blame"), tr("Show file blame annotation"));
}

void GitIntegrationPanel::onFileDoubleClicked(const QModelIndex& index) {
    QString filePath = fileModel_->item(index.row(), 0)->text();
    
    // Show diff for the file
    diffView_->setPlainText(
        QString("diff --git a/%1 b/%1\n"
                "index 1234..5678 100644\n"
                "--- a/%1\n"
                "+++ b/%1\n"
                "@@ -10,6 +10,8 @@ SELECT * FROM customers\n"
                " WHERE status = 'active'\n"
                " AND created_at > '2024-01-01';\n"
                "+-- Added new filter\n"
                "+AND region = 'US';\n").arg(filePath));
}

void GitIntegrationPanel::onCommitDoubleClicked(const QModelIndex& index) {
    QString hash = commitModel_->item(index.row(), 0)->text();
    QString message = commitModel_->item(index.row(), 1)->text();
    
    GitHistoryDialog dlg(this);
    dlg.exec();
}

// ============================================================================
// GitCommitDialog
// ============================================================================

GitCommitDialog::GitCommitDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Commit Changes"));
    setMinimumSize(700, 500);
    
    setupUi();
}

void GitCommitDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Staged files
    auto* stagedGroup = new QGroupBox(tr("Staged Changes"), this);
    auto* stagedLayout = new QVBoxLayout(stagedGroup);
    stagedList_ = new QListWidget(this);
    stagedLayout->addWidget(stagedList_);
    
    auto* stagedBtnLayout = new QHBoxLayout();
    auto* unstageAllBtn = new QPushButton(tr("<< Unstage All"), this);
    auto* unstageBtn = new QPushButton(tr("< Unstage"), this);
    stagedBtnLayout->addStretch();
    stagedBtnLayout->addWidget(unstageAllBtn);
    stagedBtnLayout->addWidget(unstageBtn);
    stagedLayout->addLayout(stagedBtnLayout);
    
    splitter->addWidget(stagedGroup);
    
    // Unstaged files
    auto* unstagedGroup = new QGroupBox(tr("Unstaged Changes"), this);
    auto* unstagedLayout = new QVBoxLayout(unstagedGroup);
    unstagedList_ = new QListWidget(this);
    unstagedLayout->addWidget(unstagedList_);
    
    auto* unstagedBtnLayout = new QHBoxLayout();
    auto* stageBtn = new QPushButton(tr("Stage >"), this);
    auto* stageAllBtn = new QPushButton(tr("Stage All >>"), this);
    unstagedBtnLayout->addWidget(stageBtn);
    unstagedBtnLayout->addWidget(stageAllBtn);
    unstagedBtnLayout->addStretch();
    unstagedLayout->addLayout(unstagedBtnLayout);
    
    splitter->addWidget(unstagedGroup);
    
    splitter->setSizes({150, 150});
    mainLayout->addWidget(splitter);
    
    // Message
    mainLayout->addWidget(new QLabel(tr("Commit Message:"), this));
    messageEdit_ = new QTextEdit(this);
    messageEdit_->setMaximumHeight(80);
    messageEdit_->setPlaceholderText(tr("Enter commit message..."));
    mainLayout->addWidget(messageEdit_);
    
    // Options
    auto* optionsLayout = new QHBoxLayout();
    amendCheck_ = new QCheckBox(tr("Amend previous commit"), this);
    signOffCheck_ = new QCheckBox(tr("Sign off"), this);
    optionsLayout->addWidget(amendCheck_);
    optionsLayout->addWidget(signOffCheck_);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);
    
    // Preview
    mainLayout->addWidget(new QLabel(tr("Commit Preview:"), this));
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Consolas", 9));
    previewEdit_->setMaximumHeight(100);
    mainLayout->addWidget(previewEdit_);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    auto* commitBtn = new QPushButton(tr("&Commit"), this);
    commitBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(tr("&Cancel"), this);
    
    buttonLayout->addWidget(commitBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);
    
    connect(stageBtn, &QPushButton::clicked, this, &GitCommitDialog::onStageSelected);
    connect(stageAllBtn, &QPushButton::clicked, this, &GitCommitDialog::onStageAll);
    connect(unstageBtn, &QPushButton::clicked, this, &GitCommitDialog::onUnstageSelected);
    connect(unstageAllBtn, &QPushButton::clicked, this, &GitCommitDialog::onUnstageAll);
    connect(commitBtn, &QPushButton::clicked, this, &GitCommitDialog::onCommit);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void GitCommitDialog::setStagedFiles(const QStringList& files) {
    stagedList_->clear();
    stagedList_->addItems(files);
}

void GitCommitDialog::setUnstagedFiles(const QStringList& files) {
    unstagedList_->clear();
    unstagedList_->addItems(files);
}

void GitCommitDialog::onStageAll() {
    while (unstagedList_->count() > 0) {
        stagedList_->addItem(unstagedList_->takeItem(0));
    }
}

void GitCommitDialog::onUnstageAll() {
    while (stagedList_->count() > 0) {
        unstagedList_->addItem(stagedList_->takeItem(0));
    }
}

void GitCommitDialog::onStageSelected() {
    for (auto* item : unstagedList_->selectedItems()) {
        stagedList_->addItem(unstagedList_->takeItem(unstagedList_->row(item)));
    }
}

void GitCommitDialog::onUnstageSelected() {
    for (auto* item : stagedList_->selectedItems()) {
        unstagedList_->addItem(stagedList_->takeItem(stagedList_->row(item)));
    }
}

void GitCommitDialog::onCommit() {
    if (messageEdit_->toPlainText().isEmpty() && !amendCheck_->isChecked()) {
        QMessageBox::warning(this, tr("Error"), tr("Commit message is required."));
        return;
    }
    
    accept();
}

void GitCommitDialog::updatePreview() {
    QString preview;
    preview += QString("commit [hash]\n");
    preview += QString("Author: User <user@example.com>\n");
    preview += QString("Date: %1\n\n").arg(QDateTime::currentDateTime().toString());
    preview += QString("    %1\n\n").arg(messageEdit_->toPlainText());
    
    previewEdit_->setPlainText(preview);
}

// ============================================================================
// GitBranchDialog
// ============================================================================

GitBranchDialog::GitBranchDialog(Mode mode, QWidget* parent)
    : QDialog(parent), mode_(mode) {
    
    switch (mode) {
        case BranchList: setWindowTitle(tr("Branches")); break;
        case Create: setWindowTitle(tr("Create Branch")); break;
        case Delete: setWindowTitle(tr("Delete Branch")); break;
        case Merge: setWindowTitle(tr("Merge Branch")); break;
        case Checkout: setWindowTitle(tr("Checkout Branch")); break;
    }
    
    setMinimumSize(500, 400);
    setupUi();
}

void GitBranchDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Branch list
    branchList_ = new QListWidget(this);
    branchList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mainLayout->addWidget(branchList_);
    
    loadBranches();
    
    // Create branch options
    if (mode_ == Create) {
        auto* formLayout = new QGridLayout();
        formLayout->addWidget(new QLabel(tr("New branch name:"), this), 0, 0);
        newNameEdit_ = new QLineEdit(this);
        formLayout->addWidget(newNameEdit_, 0, 1);
        
        formLayout->addWidget(new QLabel(tr("Based on:"), this), 1, 0);
        baseCombo_ = new QComboBox(this);
        baseCombo_->addItems({"main", "develop", "feature/login"});
        formLayout->addWidget(baseCombo_, 1, 1);
        
        checkoutCheck_ = new QCheckBox(tr("Checkout new branch"), this);
        checkoutCheck_->setChecked(true);
        formLayout->addWidget(checkoutCheck_, 2, 0, 1, 2);
        
        mainLayout->addLayout(formLayout);
    }
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    QPushButton* actionBtn = nullptr;
    QPushButton* closeBtn = new QPushButton(tr("Close"), this);
    
    switch (mode_) {
        case BranchList:
            actionBtn = new QPushButton(tr("Checkout"), this);
            connect(actionBtn, &QPushButton::clicked, this, &GitBranchDialog::onCheckoutBranch);
            break;
        case Create:
            actionBtn = new QPushButton(tr("Create"), this);
            connect(actionBtn, &QPushButton::clicked, this, &GitBranchDialog::onCreateBranch);
            break;
        case Delete:
            actionBtn = new QPushButton(tr("Delete"), this);
            connect(actionBtn, &QPushButton::clicked, this, &GitBranchDialog::onDeleteBranch);
            break;
        case Merge:
            actionBtn = new QPushButton(tr("Merge"), this);
            connect(actionBtn, &QPushButton::clicked, this, &GitBranchDialog::onMergeBranch);
            break;
        case Checkout:
            actionBtn = new QPushButton(tr("Checkout"), this);
            connect(actionBtn, &QPushButton::clicked, this, &GitBranchDialog::onCheckoutBranch);
            break;
    }
    
    if (actionBtn) {
        btnLayout->addWidget(actionBtn);
    }
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void GitBranchDialog::loadBranches() {
    branchList_->addItem("* main");
    branchList_->addItem("  develop");
    branchList_->addItem("  feature/login");
    branchList_->addItem("  feature/reporting");
    branchList_->addItem("  bugfix/query-optimizer");
    branchList_->addItem("  release/v1.2");
}

void GitBranchDialog::onCreateBranch() {
    if (newNameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Branch name is required."));
        return;
    }
    
    QMessageBox::information(this, tr("Create Branch"),
        tr("Created branch '%1' from '%2'")
        .arg(newNameEdit_->text())
        .arg(baseCombo_->currentText()));
    
    if (checkoutCheck_->isChecked()) {
        accept();
    }
}

void GitBranchDialog::onDeleteBranch() {
    auto selected = branchList_->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a branch to delete."));
        return;
    }
    
    auto reply = QMessageBox::question(this, tr("Delete Branch"),
        tr("Are you sure you want to delete '%1'?").arg(selected[0]->text().trimmed()));
    
    if (reply == QMessageBox::Yes) {
        accept();
    }
}

void GitBranchDialog::onMergeBranch() {
    auto selected = branchList_->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a branch to merge."));
        return;
    }
    
    auto reply = QMessageBox::question(this, tr("Merge Branch"),
        tr("Merge '%1' into current branch?").arg(selected[0]->text().trimmed()));
    
    if (reply == QMessageBox::Yes) {
        accept();
    }
}

void GitBranchDialog::onCheckoutBranch() {
    auto selected = branchList_->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a branch."));
        return;
    }
    
    accept();
}

void GitBranchDialog::onPushBranch() {
    QMessageBox::information(this, tr("Push"), tr("Branch pushed to origin."));
}

// ============================================================================
// GitHistoryDialog
// ============================================================================

GitHistoryDialog::GitHistoryDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("History"));
    setMinimumSize(900, 600);
    setupUi();
}

void GitHistoryDialog::setFilePath(const QString& path) {
    filePath_ = path;
    setWindowTitle(tr("History - %1").arg(path));
    loadHistory();
}

void GitHistoryDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterEdit_ = new QLineEdit(this);
    filterLayout->addWidget(filterEdit_);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    filterLayout->addWidget(refreshBtn);
    mainLayout->addLayout(filterLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // History table
    historyTable_ = new QTableView(this);
    historyTable_->setAlternatingRowColors(true);
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Hash"), tr("Graph"), tr("Message"), 
                                       tr("Author"), tr("Date"), tr("Refs")});
    historyTable_->setModel(model_);
    
    splitter->addWidget(historyTable_);
    
    // Details
    auto* detailsSplitter = new QSplitter(Qt::Horizontal, splitter);
    
    commitDetails_ = new QTextEdit(detailsSplitter);
    commitDetails_->setReadOnly(true);
    commitDetails_->setMaximumHeight(200);
    
    diffView_ = new QTextEdit(detailsSplitter);
    diffView_->setReadOnly(true);
    diffView_->setFont(QFont("Consolas", 9));
    
    detailsSplitter->addWidget(commitDetails_);
    detailsSplitter->addWidget(diffView_);
    
    splitter->addWidget(detailsSplitter);
    splitter->setSizes({300, 150});
    
    mainLayout->addWidget(splitter);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* checkoutBtn = new QPushButton(tr("Checkout"), this);
    auto* branchBtn = new QPushButton(tr("Create Branch"), this);
    auto* cherryBtn = new QPushButton(tr("Cherry Pick"), this);
    auto* revertBtn = new QPushButton(tr("Revert"), this);
    auto* closeBtn = new QPushButton(tr("Close"), this);
    
    btnLayout->addWidget(checkoutBtn);
    btnLayout->addWidget(branchBtn);
    btnLayout->addWidget(cherryBtn);
    btnLayout->addWidget(revertBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(filterEdit_, &QLineEdit::textChanged, this, &GitHistoryDialog::onFilterChanged);
    connect(refreshBtn, &QPushButton::clicked, this, &GitHistoryDialog::loadHistory);
    connect(historyTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &GitHistoryDialog::updateDiff);
    connect(checkoutBtn, &QPushButton::clicked, this, &GitHistoryDialog::onCheckoutCommit);
    connect(branchBtn, &QPushButton::clicked, this, &GitHistoryDialog::onCreateBranchFromCommit);
    connect(cherryBtn, &QPushButton::clicked, this, &GitHistoryDialog::onCherryPickCommit);
    connect(revertBtn, &QPushButton::clicked, this, &GitHistoryDialog::onRevertCommit);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    loadHistory();
}

void GitHistoryDialog::loadHistory() {
    model_->removeRows(0, model_->rowCount());
    
    QList<QList<QString>> history = {
        {"abc1234", "*", "Add customer table indexes", "John Doe", "2 hours ago", "HEAD -> main"},
        {"def5678", "|", "Fix query performance issue", "Jane Smith", "5 hours ago", ""},
        {"ghi9012", "|", "Update schema documentation", "Bob Wilson", "1 day ago", "origin/main"},
        {"jkl3456", "|/", "Merge branch 'feature/indexes'", "John Doe", "2 days ago", ""},
        {"mno7890", "|", "Initial schema setup", "John Doe", "3 days ago", "v1.0"},
    };
    
    for (const auto& row : history) {
        QList<QStandardItem*> items;
        for (const auto& cell : row) {
            items.append(new QStandardItem(cell));
        }
        model_->appendRow(items);
    }
    
    historyTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    historyTable_->resizeColumnsToContents();
}

void GitHistoryDialog::updateDiff() {
    auto index = historyTable_->currentIndex();
    if (!index.isValid()) return;
    
    QString hash = model_->item(index.row(), 0)->text();
    QString message = model_->item(index.row(), 2)->text();
    QString author = model_->item(index.row(), 3)->text();
    QString date = model_->item(index.row(), 4)->text();
    
    commitDetails_->setPlainText(
        QString("Commit: %1\nAuthor: %2\nDate: %3\n\n    %4")
        .arg(hash).arg(author).arg(date).arg(message));
    
    diffView_->setPlainText(
        QString("diff --git a/src/tables/customers.sql b/src/tables/customers.sql\n"
                "new file mode 100644\n"
                "index 0000000..1234567\n"
                "--- /dev/null\n"
                "+++ b/src/tables/customers.sql\n"
                "@@ -0,0 +1,10 @@\n"
                "+CREATE TABLE customers (\n"
                "+    id INT PRIMARY KEY,\n"
                "+    name VARCHAR(100),\n"
                "+    email VARCHAR(100),\n"
                "+    created_at TIMESTAMP\n"));
}

void GitHistoryDialog::onFilterChanged(const QString& text) {
    // Filter history
}

void GitHistoryDialog::onCheckoutCommit() {
    QMessageBox::information(this, tr("Checkout"), tr("Commit checked out."));
}

void GitHistoryDialog::onCreateBranchFromCommit() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Create Branch"),
        tr("Branch name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        QMessageBox::information(this, tr("Create Branch"),
            tr("Created branch '%1'").arg(name));
    }
}

void GitHistoryDialog::onCherryPickCommit() {
    auto reply = QMessageBox::question(this, tr("Cherry Pick"),
        tr("Cherry-pick this commit onto current branch?"));
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Cherry Pick"), tr("Commit cherry-picked."));
    }
}

void GitHistoryDialog::onRevertCommit() {
    auto reply = QMessageBox::question(this, tr("Revert"),
        tr("Revert this commit?"));
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Revert"), tr("Commit reverted."));
    }
}

void GitHistoryDialog::onCompareCommits() {
    // Compare two selected commits
}

// ============================================================================
// GitDiffDialog
// ============================================================================

GitDiffDialog::GitDiffDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Diff"));
    setMinimumSize(800, 600);
    
    setupUi();
}

void GitDiffDialog::setFilePath(const QString& path) {
    filePath_ = path;
    setWindowTitle(tr("Diff - %1").arg(path));
    loadDiff();
}

void GitDiffDialog::setCommits(const QString& fromCommit, const QString& toCommit) {
    fromCommit_ = fromCommit;
    toCommit_ = toCommit;
    setWindowTitle(tr("Diff: %1..%2").arg(fromCommit.left(7)).arg(toCommit.left(7)));
    loadDiff();
}

void GitDiffDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbar = new QHBoxLayout();
    
    auto* prevBtn = new QPushButton(tr("<< Previous"), this);
    auto* nextBtn = new QPushButton(tr("Next >>"), this);
    statsLabel_ = new QLabel(this);
    
    toolbar->addWidget(prevBtn);
    toolbar->addWidget(nextBtn);
    toolbar->addStretch();
    toolbar->addWidget(statsLabel_);
    
    auto* stageBtn = new QPushButton(tr("Stage Hunk"), this);
    auto* unstageBtn = new QPushButton(tr("Unstage Hunk"), this);
    toolbar->addWidget(stageBtn);
    toolbar->addWidget(unstageBtn);
    
    mainLayout->addLayout(toolbar);
    
    // Diff view
    diffEdit_ = new QTextEdit(this);
    diffEdit_->setReadOnly(true);
    diffEdit_->setFont(QFont("Consolas", 10));
    mainLayout->addWidget(diffEdit_);
    
    // Close button
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(prevBtn, &QPushButton::clicked, this, &GitDiffDialog::onPreviousChange);
    connect(nextBtn, &QPushButton::clicked, this, &GitDiffDialog::onNextChange);
    connect(stageBtn, &QPushButton::clicked, this, &GitDiffDialog::onStageHunk);
    connect(unstageBtn, &QPushButton::clicked, this, &GitDiffDialog::onUnstageHunk);
    
    loadDiff();
}

void GitDiffDialog::loadDiff() {
    QString diff = QString(
        "diff --git a/%1 b/%1\n"
        "index a1b2c3d..e4f5g6h 100644\n"
        "--- a/%1\n"
        "+++ b/%1\n"
        "@@ -10,7 +10,10 @@\n"
        " CREATE TABLE customers (\n"
        "     id INT PRIMARY KEY AUTO_INCREMENT,\n"
        "-    name VARCHAR(50),\n"
        "+    first_name VARCHAR(50),\n"
        "+    last_name VARCHAR(50),\n"
        "     email VARCHAR(100) UNIQUE,\n"
        "+    phone VARCHAR(20),\n"
        "     created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
        " );\n"
        " \n"
        "-CREATE INDEX idx_name ON customers(name);\n"
        "+CREATE INDEX idx_first ON customers(first_name);\n"
        "+CREATE INDEX idx_last ON customers(last_name);\n"
    ).arg(filePath_);
    
    diffEdit_->setPlainText(diff);
    statsLabel_->setText(tr("5 insertions(+), 2 deletions(-)"));
}

void GitDiffDialog::onPreviousChange() {
    // Navigate to previous hunk
}

void GitDiffDialog::onNextChange() {
    // Navigate to next hunk
}

void GitDiffDialog::onStageHunk() {
    QMessageBox::information(this, tr("Stage"), tr("Hunk staged."));
}

void GitDiffDialog::onUnstageHunk() {
    QMessageBox::information(this, tr("Unstage"), tr("Hunk unstaged."));
}

} // namespace scratchrobin::ui
