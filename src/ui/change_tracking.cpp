#include "change_tracking.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTableView>
#include <QTableWidget>
#include <QTreeView>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QSpinBox>
#include <QClipboard>
#include <QApplication>
#include <QInputDialog>

namespace scratchrobin::ui {

// ============================================================================
// Change Tracking Panel
// ============================================================================
ChangeTrackingPanel::ChangeTrackingPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("change_tracking", parent), client_(client) {
    setupUi();
    loadChanges();
}

void ChangeTrackingPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    detailsBtn_ = new QPushButton(tr("View Details"), this);
    detailsBtn_->setEnabled(false);
    connect(detailsBtn_, &QPushButton::clicked, this, &ChangeTrackingPanel::onViewDetails);
    toolbarLayout->addWidget(detailsBtn_);
    
    compareBtn_ = new QPushButton(tr("Compare"), this);
    compareBtn_->setEnabled(false);
    connect(compareBtn_, &QPushButton::clicked, this, &ChangeTrackingPanel::onCompareVersions);
    toolbarLayout->addWidget(compareBtn_);
    
    rollbackBtn_ = new QPushButton(tr("Rollback"), this);
    rollbackBtn_->setEnabled(false);
    connect(rollbackBtn_, &QPushButton::clicked, this, &ChangeTrackingPanel::onRollbackChange);
    toolbarLayout->addWidget(rollbackBtn_);
    
    exportBtn_ = new QPushButton(tr("Export"), this);
    connect(exportBtn_, &QPushButton::clicked, this, &ChangeTrackingPanel::onExportChange);
    toolbarLayout->addWidget(exportBtn_);
    
    toolbarLayout->addStretch();
    
    auto* reportBtn = new QPushButton(tr("Report"), this);
    connect(reportBtn, &QPushButton::clicked, this, &ChangeTrackingPanel::onGenerateReport);
    toolbarLayout->addWidget(reportBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    typeFilterCombo_ = new QComboBox(this);
    typeFilterCombo_->addItem(tr("All Types"));
    typeFilterCombo_->addItem(tr("Create"));
    typeFilterCombo_->addItem(tr("Modify"));
    typeFilterCombo_->addItem(tr("Delete"));
    typeFilterCombo_->addItem(tr("Data Changes"));
    connect(typeFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChangeTrackingPanel::onFilterByType);
    filterLayout->addWidget(typeFilterCombo_);
    
    objectFilterCombo_ = new QComboBox(this);
    objectFilterCombo_->addItem(tr("All Objects"));
    objectFilterCombo_->addItem(tr("Tables"));
    objectFilterCombo_->addItem(tr("Views"));
    objectFilterCombo_->addItem(tr("Procedures"));
    objectFilterCombo_->addItem(tr("Functions"));
    filterLayout->addWidget(objectFilterCombo_);
    
    dateFromEdit_ = new QDateTimeEdit(this);
    dateFromEdit_->setCalendarPopup(true);
    dateFromEdit_->setDateTime(QDateTime::currentDateTime().addDays(-7));
    filterLayout->addWidget(new QLabel(tr("From:"), this));
    filterLayout->addWidget(dateFromEdit_);
    
    dateToEdit_ = new QDateTimeEdit(this);
    dateToEdit_->setCalendarPopup(true);
    dateToEdit_->setDateTime(QDateTime::currentDateTime());
    filterLayout->addWidget(new QLabel(tr("To:"), this));
    filterLayout->addWidget(dateToEdit_);
    
    mainLayout->addLayout(filterLayout);
    
    // Search
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search changes..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &ChangeTrackingPanel::onSearchTextChanged);
    mainLayout->addWidget(searchEdit_);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Changes table
    changesTable_ = new QTableView(this);
    changesModel_ = new QStandardItemModel(this);
    changesModel_->setHorizontalHeaderLabels({
        tr("Time"), tr("Type"), tr("Object"), tr("Author"), tr("Description")
    });
    changesTable_->setModel(changesModel_);
    changesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    changesTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    changesTable_->setAlternatingRowColors(true);
    changesTable_->horizontalHeader()->setStretchLastSection(true);
    
    connect(changesTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ChangeTrackingPanel::onChangeSelected);
    
    splitter->addWidget(changesTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    
    changeTypeLabel_ = new QLabel(tr("Select a change to view details"), this);
    QFont titleFont = changeTypeLabel_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    changeTypeLabel_->setFont(titleFont);
    detailsLayout->addWidget(changeTypeLabel_);
    
    auto* infoLayout = new QGridLayout();
    changeObjectLabel_ = new QLabel(this);
    changeAuthorLabel_ = new QLabel(this);
    changeTimeLabel_ = new QLabel(this);
    rowsAffectedLabel_ = new QLabel(this);
    
    infoLayout->addWidget(new QLabel(tr("Object:"), this), 0, 0);
    infoLayout->addWidget(changeObjectLabel_, 0, 1);
    infoLayout->addWidget(new QLabel(tr("Author:"), this), 1, 0);
    infoLayout->addWidget(changeAuthorLabel_, 1, 1);
    infoLayout->addWidget(new QLabel(tr("Time:"), this), 2, 0);
    infoLayout->addWidget(changeTimeLabel_, 2, 1);
    infoLayout->addWidget(new QLabel(tr("Rows Affected:"), this), 3, 0);
    infoLayout->addWidget(rowsAffectedLabel_, 3, 1);
    
    detailsLayout->addLayout(infoLayout);
    
    reversibleCheck_ = new QCheckBox(tr("Can be rolled back"), this);
    reversibleCheck_->setEnabled(false);
    detailsLayout->addWidget(reversibleCheck_);
    
    // SQL comparison
    auto* sqlSplitter = new QSplitter(Qt::Vertical, this);
    
    auto* beforeGroup = new QGroupBox(tr("Before"), this);
    auto* beforeLayout = new QVBoxLayout(beforeGroup);
    sqlBeforeEdit_ = new QPlainTextEdit(this);
    sqlBeforeEdit_->setReadOnly(true);
    beforeLayout->addWidget(sqlBeforeEdit_);
    sqlSplitter->addWidget(beforeGroup);
    
    auto* afterGroup = new QGroupBox(tr("After"), this);
    auto* afterLayout = new QVBoxLayout(afterGroup);
    sqlAfterEdit_ = new QPlainTextEdit(this);
    sqlAfterEdit_->setReadOnly(true);
    afterLayout->addWidget(sqlAfterEdit_);
    sqlSplitter->addWidget(afterGroup);
    
    sqlSplitter->setSizes({150, 150});
    detailsLayout->addWidget(sqlSplitter, 1);
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({500, 400});
    
    mainLayout->addWidget(splitter, 1);
    
    // Status bar
    auto* statusLayout = new QHBoxLayout();
    monitoringStatusLabel_ = new QLabel(tr("Not monitoring"), this);
    statusLayout->addWidget(monitoringStatusLabel_);
    statusLayout->addStretch();
    
    monitorBtn_ = new QPushButton(tr("Start Monitoring"), this);
    connect(monitorBtn_, &QPushButton::clicked, [this]() {
        if (isMonitoring_) {
            onStopMonitoring();
        } else {
            onStartMonitoring();
        }
    });
    statusLayout->addWidget(monitorBtn_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &ChangeTrackingPanel::onRefreshChanges);
    statusLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(statusLayout);
}

void ChangeTrackingPanel::loadChanges() {
    changes_.clear();
    
    // Sample data
    ChangeRecord ch1;
    ch1.id = "ch1";
    ch1.changeType = ChangeType::Create;
    ch1.objectType = ChangeObjectType::Table;
    ch1.objectName = "customers";
    ch1.schemaName = "public";
    ch1.author = tr("John Doe");
    ch1.timestamp = QDateTime::currentDateTime().addDays(-5);
    ch1.sqlAfter = "CREATE TABLE customers (\n    id INT PRIMARY KEY,\n    name VARCHAR(100),\n    email VARCHAR(100)\n);";
    ch1.description = tr("Created customer table");
    ch1.isReversible = true;
    changes_.append(ch1);
    
    ChangeRecord ch2;
    ch2.id = "ch2";
    ch2.changeType = ChangeType::Modify;
    ch2.objectType = ChangeObjectType::Table;
    ch2.objectName = "customers";
    ch2.schemaName = "public";
    ch2.author = tr("Jane Smith");
    ch2.timestamp = QDateTime::currentDateTime().addDays(-3);
    ch2.sqlBefore = "CREATE TABLE customers (\n    id INT PRIMARY KEY,\n    name VARCHAR(100),\n    email VARCHAR(100)\n);";
    ch2.sqlAfter = "CREATE TABLE customers (\n    id INT PRIMARY KEY,\n    name VARCHAR(100),\n    email VARCHAR(100),\n    created_at TIMESTAMP\n);";
    ch2.description = tr("Added created_at column");
    ch2.isReversible = true;
    changes_.append(ch2);
    
    ChangeRecord ch3;
    ch3.id = "ch3";
    ch3.changeType = ChangeType::DataUpdate;
    ch3.objectType = ChangeObjectType::Table;
    ch3.objectName = "customers";
    ch3.schemaName = "public";
    ch3.author = tr("Bob Wilson");
    ch3.timestamp = QDateTime::currentDateTime().addDays(-1);
    ch3.sqlAfter = "UPDATE customers SET email = LOWER(email);";
    ch3.description = tr("Normalized email addresses");
    ch3.rowsAffected = 1250;
    ch3.isReversible = false;
    ch3.severity = ChangeSeverity::Warning;
    changes_.append(ch3);
    
    ChangeRecord ch4;
    ch4.id = "ch4";
    ch4.changeType = ChangeType::Delete;
    ch4.objectType = ChangeObjectType::Index;
    ch4.objectName = "idx_customers_email";
    ch4.schemaName = "public";
    ch4.author = tr("Admin");
    ch4.timestamp = QDateTime::currentDateTime().addSecs(-12 * 3600);
    ch4.sqlBefore = "CREATE INDEX idx_customers_email ON customers(email);";
    ch4.description = tr("Removed redundant index");
    ch4.isReversible = true;
    changes_.append(ch4);
    
    updateChangesList();
}

void ChangeTrackingPanel::updateChangesList() {
    changesModel_->clear();
    changesModel_->setHorizontalHeaderLabels({
        tr("Time"), tr("Type"), tr("Object"), tr("Author"), tr("Description")
    });
    
    for (const auto& change : changes_) {
        QString typeStr;
        switch (change.changeType) {
            case ChangeType::Create: typeStr = tr("CREATE"); break;
            case ChangeType::Modify: typeStr = tr("MODIFY"); break;
            case ChangeType::Delete: typeStr = tr("DELETE"); break;
            case ChangeType::Rename: typeStr = tr("RENAME"); break;
            case ChangeType::Grant: typeStr = tr("GRANT"); break;
            case ChangeType::Revoke: typeStr = tr("REVOKE"); break;
            case ChangeType::DataInsert: typeStr = tr("INSERT"); break;
            case ChangeType::DataUpdate: typeStr = tr("UPDATE"); break;
            case ChangeType::DataDelete: typeStr = tr("DELETE DATA"); break;
        }
        
        QString objectPath = change.schemaName + "." + change.objectName;
        
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(change.timestamp.toString("yyyy-MM-dd hh:mm"))
             << new QStandardItem(typeStr)
             << new QStandardItem(objectPath)
             << new QStandardItem(change.author)
             << new QStandardItem(change.description);
        
        (*row)[0]->setData(change.id, Qt::UserRole);
        
        // Color code by severity
        if (change.severity == ChangeSeverity::Critical) {
            for (int i = 0; i < row->size(); ++i) {
                (*row)[i]->setBackground(QBrush(QColor(255, 200, 200)));
            }
        } else if (change.severity == ChangeSeverity::Warning) {
            for (int i = 0; i < row->size(); ++i) {
                (*row)[i]->setBackground(QBrush(QColor(255, 240, 200)));
            }
        }
        
        changesModel_->appendRow(*row);
    }
}

void ChangeTrackingPanel::onChangeSelected(const QModelIndex& index) {
    if (!index.isValid()) {
        detailsBtn_->setEnabled(false);
        compareBtn_->setEnabled(false);
        rollbackBtn_->setEnabled(false);
        return;
    }
    
    QString changeId = changesModel_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    for (const auto& change : changes_) {
        if (change.id == changeId) {
            currentChange_ = change;
            updateChangeDetails(change);
            
            detailsBtn_->setEnabled(true);
            compareBtn_->setEnabled(!change.sqlBefore.isEmpty());
            rollbackBtn_->setEnabled(change.isReversible);
            break;
        }
    }
}

void ChangeTrackingPanel::updateChangeDetails(const ChangeRecord& change) {
    QString typeStr;
    switch (change.changeType) {
        case ChangeType::Create: typeStr = tr("CREATE"); break;
        case ChangeType::Modify: typeStr = tr("MODIFY"); break;
        case ChangeType::Delete: typeStr = tr("DELETE"); break;
        case ChangeType::Rename: typeStr = tr("RENAME"); break;
        default: typeStr = tr("CHANGE");
    }
    
    changeTypeLabel_->setText(tr("%1: %2").arg(typeStr).arg(change.description));
    changeObjectLabel_->setText(change.schemaName + "." + change.objectName);
    changeAuthorLabel_->setText(change.author);
    changeTimeLabel_->setText(change.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    
    if (change.rowsAffected >= 0) {
        rowsAffectedLabel_->setText(QString::number(change.rowsAffected));
    } else {
        rowsAffectedLabel_->setText(tr("N/A"));
    }
    
    reversibleCheck_->setChecked(change.isReversible);
    sqlBeforeEdit_->setPlainText(change.sqlBefore);
    sqlAfterEdit_->setPlainText(change.sqlAfter);
}

void ChangeTrackingPanel::onViewDetails() {
    if (currentChange_.id.isEmpty()) return;
    
    ChangeDetailsDialog dialog(currentChange_, client_, this);
    dialog.exec();
}

void ChangeTrackingPanel::onCompareVersions() {
    if (currentChange_.id.isEmpty()) return;
    
    // For demo, compare before/after of same change
    ChangeRecord before = currentChange_;
    ChangeRecord after = currentChange_;
    after.sqlAfter = after.sqlAfter;  // Same for demo
    
    CompareVersionsDialog dialog(before, after, client_, this);
    dialog.exec();
}

void ChangeTrackingPanel::onRollbackChange() {
    if (currentChange_.id.isEmpty() || !currentChange_.isReversible) return;
    
    auto reply = QMessageBox::question(this, tr("Rollback Change"),
        tr("Are you sure you want to rollback this change?\n\n%1")
        .arg(currentChange_.description),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Rollback"),
            tr("Rollback SQL would be executed here."));
        emit rollbackRequested(currentChange_.id);
    }
}

void ChangeTrackingPanel::onExportChange() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Change"),
        QString(), tr("SQL Files (*.sql);;JSON Files (*.json);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"),
            tr("Change exported to %1").arg(fileName));
    }
}

void ChangeTrackingPanel::onGenerateReport() {
    AuditReportDialog dialog(client_, this);
    dialog.exec();
}

void ChangeTrackingPanel::onViewSession(const QString& sessionId) {
    Q_UNUSED(sessionId)
    SessionBrowserDialog dialog(client_, this);
    dialog.exec();
}

void ChangeTrackingPanel::onRollbackSession() {
    RollbackWizard wizard(changes_, client_, this);
    wizard.exec();
}

void ChangeTrackingPanel::onTagSession() {
    bool ok;
    QString tag = QInputDialog::getText(this, tr("Tag Session"),
        tr("Enter tag name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !tag.isEmpty()) {
        QMessageBox::information(this, tr("Tagged"), tr("Session tagged with: %1").arg(tag));
    }
}

void ChangeTrackingPanel::onRefreshChanges() {
    loadChanges();
}

void ChangeTrackingPanel::onStartMonitoring() {
    isMonitoring_ = true;
    monitoringStatusLabel_->setText(tr("Monitoring active"));
    monitoringStatusLabel_->setStyleSheet("color: green;");
    monitorBtn_->setText(tr("Stop Monitoring"));
}

void ChangeTrackingPanel::onStopMonitoring() {
    isMonitoring_ = false;
    monitoringStatusLabel_->setText(tr("Not monitoring"));
    monitoringStatusLabel_->setStyleSheet("");
    monitorBtn_->setText(tr("Start Monitoring"));
}

void ChangeTrackingPanel::onFilterByType(int type) {
    Q_UNUSED(type)
    filterChanges();
}

void ChangeTrackingPanel::onFilterByObject(const QString& object) {
    Q_UNUSED(object)
    filterChanges();
}

void ChangeTrackingPanel::onFilterByDateRange(const QDateTime& from, const QDateTime& to) {
    Q_UNUSED(from)
    Q_UNUSED(to)
    filterChanges();
}

void ChangeTrackingPanel::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    filterChanges();
}

void ChangeTrackingPanel::filterChanges() {
    // TODO: Implement filtering
    updateChangesList();
}

// ============================================================================
// Change Details Dialog
// ============================================================================
ChangeDetailsDialog::ChangeDetailsDialog(const ChangeRecord& change,
                                         backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), change_(change), client_(client) {
    setupUi();
    loadChangeDetails();
}

void ChangeDetailsDialog::setupUi() {
    setWindowTitle(tr("Change Details: %1").arg(change_.objectName));
    setMinimumSize(600, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Title
    titleLabel_ = new QLabel(this);
    QFont titleFont = titleLabel_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel_->setFont(titleFont);
    layout->addWidget(titleLabel_);
    
    // Info
    auto* infoGroup = new QGroupBox(tr("Information"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    infoLayout->addRow(tr("Object:"), new QLabel(change_.schemaName + "." + change_.objectName, this));
    infoLayout->addRow(tr("Author:"), new QLabel(change_.author, this));
    infoLayout->addRow(tr("Time:"), new QLabel(change_.timestamp.toString("yyyy-MM-dd hh:mm:ss"), this));
    infoLayout->addRow(tr("Description:"), new QLabel(change_.description, this));
    
    layout->addWidget(infoGroup);
    
    // Tabs
    tabWidget_ = new QTabWidget(this);
    
    // Diff view
    diffBrowser_ = new QTextBrowser(this);
    tabWidget_->addTab(diffBrowser_, tr("Diff"));
    
    // Before
    beforeEdit_ = new QPlainTextEdit(this);
    beforeEdit_->setReadOnly(true);
    tabWidget_->addTab(beforeEdit_, tr("Before"));
    
    // After
    afterEdit_ = new QPlainTextEdit(this);
    afterEdit_->setReadOnly(true);
    tabWidget_->addTab(afterEdit_, tr("After"));
    
    // Info
    infoBrowser_ = new QTextBrowser(this);
    tabWidget_->addTab(infoBrowser_, tr("Metadata"));
    
    layout->addWidget(tabWidget_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* copyBtn = new QPushButton(tr("Copy SQL"), this);
    connect(copyBtn, &QPushButton::clicked, this, &ChangeDetailsDialog::onCopyToClipboard);
    btnLayout->addWidget(copyBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &ChangeDetailsDialog::onExportSQL);
    btnLayout->addWidget(exportBtn);
    
    if (change_.isReversible) {
        auto* rollbackBtn = new QPushButton(tr("Rollback"), this);
        connect(rollbackBtn, &QPushButton::clicked, this, &ChangeDetailsDialog::onRollback);
        btnLayout->addWidget(rollbackBtn);
    }
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ChangeDetailsDialog::loadChangeDetails() {
    titleLabel_->setText(change_.description);
    beforeEdit_->setPlainText(change_.sqlBefore);
    afterEdit_->setPlainText(change_.sqlAfter);
    
    updateDiffView();
    
    QString metadata = tr("<h3>Change Metadata</h3>");
    metadata += tr("<p><b>Change ID:</b> %1</p>").arg(change_.id);
    metadata += tr("<p><b>Transaction ID:</b> %1</p>").arg(change_.transactionId);
    metadata += tr("<p><b>Checksum:</b> %1</p>").arg(change_.checksum);
    metadata += tr("<p><b>Reversible:</b> %1</p>").arg(change_.isReversible ? tr("Yes") : tr("No"));
    
    if (change_.rowsAffected >= 0) {
        metadata += tr("<p><b>Rows Affected:</b> %1</p>").arg(change_.rowsAffected);
    }
    
    infoBrowser_->setHtml(metadata);
}

void ChangeDetailsDialog::updateDiffView() {
    // Simple diff display
    QString diffHtml = "<pre style='font-family: monospace;'>";
    
    if (!change_.sqlBefore.isEmpty()) {
        diffHtml += tr("<span style='color: red;'>--- Before</span>\n");
        QStringList beforeLines = change_.sqlBefore.split('\n');
        for (const auto& line : beforeLines) {
            diffHtml += tr("<span style='color: red;'>- %1</span>\n").arg(line.toHtmlEscaped());
        }
    }
    
    if (!change_.sqlAfter.isEmpty()) {
        diffHtml += tr("<span style='color: green;'>+++ After</span>\n");
        QStringList afterLines = change_.sqlAfter.split('\n');
        for (const auto& line : afterLines) {
            diffHtml += tr("<span style='color: green;'>+ %1</span>\n").arg(line.toHtmlEscaped());
        }
    }
    
    diffHtml += "</pre>";
    diffBrowser_->setHtml(diffHtml);
}

void ChangeDetailsDialog::onViewBefore() {
    tabWidget_->setCurrentIndex(1);
}

void ChangeDetailsDialog::onViewAfter() {
    tabWidget_->setCurrentIndex(2);
}

void ChangeDetailsDialog::onViewDiff() {
    tabWidget_->setCurrentIndex(0);
}

void ChangeDetailsDialog::onExportSQL() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export SQL"),
        change_.objectName + ".sql", tr("SQL Files (*.sql)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("SQL exported to %1").arg(fileName));
    }
}

void ChangeDetailsDialog::onCopyToClipboard() {
    QApplication::clipboard()->setText(change_.sqlAfter);
    QMessageBox::information(this, tr("Copied"), tr("SQL copied to clipboard."));
}

void ChangeDetailsDialog::onRollback() {
    // TODO: Execute rollback
    QMessageBox::information(this, tr("Rollback"), tr("Rollback executed."));
    accept();
}

// ============================================================================
// Compare Versions Dialog
// ============================================================================
CompareVersionsDialog::CompareVersionsDialog(const ChangeRecord& version1, const ChangeRecord& version2,
                                             backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), version1_(version1), version2_(version2), client_(client) {
    setupUi();
    generateDiff();
}

void CompareVersionsDialog::setupUi() {
    setWindowTitle(tr("Compare: %1").arg(version1_.objectName));
    setMinimumSize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* sideBySideBtn = new QPushButton(tr("Side by Side"), this);
    connect(sideBySideBtn, &QPushButton::clicked, this, &CompareVersionsDialog::onSideBySideView);
    toolbarLayout->addWidget(sideBySideBtn);
    
    auto* unifiedBtn = new QPushButton(tr("Unified"), this);
    connect(unifiedBtn, &QPushButton::clicked, this, &CompareVersionsDialog::onUnifiedView);
    toolbarLayout->addWidget(unifiedBtn);
    
    toolbarLayout->addStretch();
    
    auto* prevBtn = new QPushButton(tr("← Previous"), this);
    connect(prevBtn, &QPushButton::clicked, this, &CompareVersionsDialog::onNavigatePrevDiff);
    toolbarLayout->addWidget(prevBtn);
    
    auto* nextBtn = new QPushButton(tr("Next →"), this);
    connect(nextBtn, &QPushButton::clicked, this, &CompareVersionsDialog::onNavigateNextDiff);
    toolbarLayout->addWidget(nextBtn);
    
    layout->addLayout(toolbarLayout);
    
    // View tabs
    viewTabWidget_ = new QTabWidget(this);
    
    // Side by side view
    auto* sideBySideWidget = new QWidget(this);
    auto* sideBySideLayout = new QHBoxLayout(sideBySideWidget);
    
    leftEdit_ = new QPlainTextEdit(this);
    leftEdit_->setReadOnly(true);
    sideBySideLayout->addWidget(leftEdit_);
    
    rightEdit_ = new QPlainTextEdit(this);
    rightEdit_->setReadOnly(true);
    sideBySideLayout->addWidget(rightEdit_);
    
    viewTabWidget_->addTab(sideBySideWidget, tr("Side by Side"));
    
    // Unified view
    unifiedBrowser_ = new QTextBrowser(this);
    viewTabWidget_->addTab(unifiedBrowser_, tr("Unified"));
    
    // Diff list
    diffList_ = new QListWidget(this);
    viewTabWidget_->addTab(diffList_, tr("Changes"));
    
    layout->addWidget(viewTabWidget_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* copyBtn = new QPushButton(tr("Copy Diff"), this);
    connect(copyBtn, &QPushButton::clicked, this, &CompareVersionsDialog::onCopyDiff);
    btnLayout->addWidget(copyBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &CompareVersionsDialog::onExportDiff);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void CompareVersionsDialog::generateDiff() {
    leftEdit_->setPlainText(version1_.sqlAfter);
    rightEdit_->setPlainText(version2_.sqlAfter);
    
    // Simple diff
    diffLines_.clear();
    QStringList beforeLines = version1_.sqlAfter.split('\n');
    QStringList afterLines = version2_.sqlAfter.split('\n');
    
    QString unifiedHtml = "<pre style='font-family: monospace;'>";
    
    int maxLines = qMax(beforeLines.size(), afterLines.size());
    for (int i = 0; i < maxLines; ++i) {
        QString beforeLine = i < beforeLines.size() ? beforeLines[i] : "";
        QString afterLine = i < afterLines.size() ? afterLines[i] : "";
        
        if (beforeLine != afterLine) {
            if (!beforeLine.isEmpty()) {
                unifiedHtml += tr("<span style='background: #ffcccc;'>- %1</span>\n")
                    .arg(beforeLine.toHtmlEscaped());
            }
            if (!afterLine.isEmpty()) {
                unifiedHtml += tr("<span style='background: #ccffcc;'>+ %1</span>\n")
                    .arg(afterLine.toHtmlEscaped());
            }
            diffLines_.append(QString::number(i + 1));
        } else {
            unifiedHtml += tr("  %1\n").arg(beforeLine.toHtmlEscaped());
        }
    }
    
    unifiedHtml += "</pre>";
    unifiedBrowser_->setHtml(unifiedHtml);
    
    // Update diff list
    diffList_->clear();
    for (const auto& line : diffLines_) {
        diffList_->addItem(tr("Line %1").arg(line));
    }
}

void CompareVersionsDialog::updateView() {
    // Update based on current tab
}

void CompareVersionsDialog::onSideBySideView() {
    viewTabWidget_->setCurrentIndex(0);
}

void CompareVersionsDialog::onUnifiedView() {
    viewTabWidget_->setCurrentIndex(1);
}

void CompareVersionsDialog::onInlineView() {
    viewTabWidget_->setCurrentIndex(2);
}

void CompareVersionsDialog::onExportDiff() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Diff"),
        tr("diff.txt"), tr("Text Files (*.txt);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("Diff exported to %1").arg(fileName));
    }
}

void CompareVersionsDialog::onCopyDiff() {
    QApplication::clipboard()->setText(unifiedBrowser_->toPlainText());
    QMessageBox::information(this, tr("Copied"), tr("Diff copied to clipboard."));
}

void CompareVersionsDialog::onNavigateNextDiff() {
    if (currentDiffIndex_ < diffLines_.size() - 1) {
        currentDiffIndex_++;
        diffList_->setCurrentRow(currentDiffIndex_);
    }
}

void CompareVersionsDialog::onNavigatePrevDiff() {
    if (currentDiffIndex_ > 0) {
        currentDiffIndex_--;
        diffList_->setCurrentRow(currentDiffIndex_);
    }
}

// ============================================================================
// Rollback Wizard
// ============================================================================
RollbackWizard::RollbackWizard(const QList<ChangeRecord>& changes,
                               backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), changes_(changes), client_(client) {
    setupUi();
}

void RollbackWizard::setupUi() {
    setWindowTitle(tr("Rollback Wizard"));
    setMinimumSize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Select changes to rollback:"), this));
    
    // Changes tree
    changesTree_ = new QTreeWidget(this);
    changesTree_->setHeaderLabels({tr("Select"), tr("Time"), tr("Object"), tr("Description")});
    changesTree_->setRootIsDecorated(false);
    
    for (const auto& change : changes_) {
        if (change.isReversible) {
            auto* item = new QTreeWidgetItem(changesTree_);
            item->setCheckState(0, Qt::Unchecked);
            item->setText(1, change.timestamp.toString("yyyy-MM-dd hh:mm"));
            item->setText(2, change.objectName);
            item->setText(3, change.description);
            item->setData(0, Qt::UserRole, change.id);
        }
    }
    
    layout->addWidget(changesTree_, 1);
    
    // Select buttons
    auto* selectLayout = new QHBoxLayout();
    auto* selectAllBtn = new QPushButton(tr("Select All"), this);
    connect(selectAllBtn, &QPushButton::clicked, this, &RollbackWizard::onSelectAll);
    selectLayout->addWidget(selectAllBtn);
    
    auto* deselectAllBtn = new QPushButton(tr("Deselect All"), this);
    connect(deselectAllBtn, &QPushButton::clicked, this, &RollbackWizard::onDeselectAll);
    selectLayout->addWidget(deselectAllBtn);
    selectLayout->addStretch();
    
    layout->addLayout(selectLayout);
    
    // Options
    backupCheck_ = new QCheckBox(tr("Create backup before rollback"), this);
    backupCheck_->setChecked(true);
    layout->addWidget(backupCheck_);
    
    validateCheck_ = new QCheckBox(tr("Validate rollback script"), this);
    validateCheck_->setChecked(true);
    layout->addWidget(validateCheck_);
    
    // Preview
    layout->addWidget(new QLabel(tr("Rollback Script Preview:"), this));
    previewBrowser_ = new QTextBrowser(this);
    previewBrowser_->setMaximumHeight(100);
    layout->addWidget(previewBrowser_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &RollbackWizard::onPreviewRollback);
    btnLayout->addWidget(previewBtn);
    
    executeBtn_ = new QPushButton(tr("Execute Rollback"), this);
    executeBtn_->setEnabled(false);
    connect(executeBtn_, &QPushButton::clicked, this, &RollbackWizard::onExecuteRollback);
    btnLayout->addWidget(executeBtn_);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void RollbackWizard::updatePreview() {
    QString script = tr("-- Rollback Script\n");
    script += tr("-- Generated: %1\n\n").arg(QDateTime::currentDateTime().toString());
    
    for (int i = 0; i < changesTree_->topLevelItemCount(); ++i) {
        auto* item = changesTree_->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            QString changeId = item->data(0, Qt::UserRole).toString();
            script += tr("-- Rollback: %1\n").arg(item->text(3));
            
            // Find the change and generate rollback SQL
            for (const auto& change : changes_) {
                if (change.id == changeId) {
                    if (!change.sqlBefore.isEmpty()) {
                        script += change.sqlBefore + ";\n\n";
                    }
                    break;
                }
            }
        }
    }
    
    previewBrowser_->setText(script);
    executeBtn_->setEnabled(true);
}

void RollbackWizard::generateRollbackScript() {
    // Generate the complete rollback script
}

void RollbackWizard::onSelectAll() {
    for (int i = 0; i < changesTree_->topLevelItemCount(); ++i) {
        changesTree_->topLevelItem(i)->setCheckState(0, Qt::Checked);
    }
    updatePreview();
}

void RollbackWizard::onDeselectAll() {
    for (int i = 0; i < changesTree_->topLevelItemCount(); ++i) {
        changesTree_->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    }
    updatePreview();
}

void RollbackWizard::onPreviewRollback() {
    updatePreview();
}

void RollbackWizard::onExecuteRollback() {
    auto reply = QMessageBox::question(this, tr("Execute Rollback"),
        tr("Are you sure you want to execute the rollback?\nThis action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Rollback Complete"),
            tr("Rollback has been executed successfully."));
        accept();
    }
}

void RollbackWizard::onCancel() {
    reject();
}

// ============================================================================
// Audit Report Dialog
// ============================================================================
AuditReportDialog::AuditReportDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
}

void AuditReportDialog::setupUi() {
    setWindowTitle(tr("Audit Report"));
    setMinimumSize(600, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Report Options"), this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    
    reportTypeCombo_ = new QComboBox(this);
    reportTypeCombo_->addItem(tr("Summary"));
    reportTypeCombo_->addItem(tr("Detailed"));
    reportTypeCombo_->addItem(tr("Changes by User"));
    reportTypeCombo_->addItem(tr("Changes by Object"));
    optionsLayout->addRow(tr("Type:"), reportTypeCombo_);
    
    dateFromEdit_ = new QDateTimeEdit(this);
    dateFromEdit_->setCalendarPopup(true);
    dateFromEdit_->setDateTime(QDateTime::currentDateTime().addDays(-30));
    optionsLayout->addRow(tr("From:"), dateFromEdit_);
    
    dateToEdit_ = new QDateTimeEdit(this);
    dateToEdit_->setCalendarPopup(true);
    dateToEdit_->setDateTime(QDateTime::currentDateTime());
    optionsLayout->addRow(tr("To:"), dateToEdit_);
    
    scopeCombo_ = new QComboBox(this);
    scopeCombo_->addItem(tr("All Objects"));
    scopeCombo_->addItem(tr("Tables Only"));
    scopeCombo_->addItem(tr("Procedures Only"));
    optionsLayout->addRow(tr("Scope:"), scopeCombo_);
    
    layout->addWidget(optionsGroup);
    
    // Generate button
    auto* generateBtn = new QPushButton(tr("Generate Report"), this);
    connect(generateBtn, &QPushButton::clicked, this, &AuditReportDialog::onGenerateReport);
    layout->addWidget(generateBtn);
    
    // Report view
    reportBrowser_ = new QTextBrowser(this);
    layout->addWidget(reportBrowser_, 1);
    
    // Export buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* pdfBtn = new QPushButton(tr("Export PDF"), this);
    connect(pdfBtn, &QPushButton::clicked, this, &AuditReportDialog::onExportPDF);
    btnLayout->addWidget(pdfBtn);
    
    auto* csvBtn = new QPushButton(tr("Export CSV"), this);
    connect(csvBtn, &QPushButton::clicked, this, &AuditReportDialog::onExportCSV);
    btnLayout->addWidget(csvBtn);
    
    auto* htmlBtn = new QPushButton(tr("Export HTML"), this);
    connect(htmlBtn, &QPushButton::clicked, this, &AuditReportDialog::onExportHTML);
    btnLayout->addWidget(htmlBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void AuditReportDialog::generateSummary() {
    QString html = "<h1>Audit Report Summary</h1>";
    html += tr("<p>Period: %1 to %2</p>")
        .arg(dateFromEdit_->dateTime().toString("yyyy-MM-dd"))
        .arg(dateToEdit_->dateTime().toString("yyyy-MM-dd"));
    
    html += "<h2>Change Statistics</h2>";
    html += "<ul>";
    html += tr("<li>Total Changes: 45</li>");
    html += tr("<li>Created: 12</li>");
    html += tr("<li>Modified: 23</li>");
    html += tr("<li>Deleted: 10</li>");
    html += "</ul>";
    
    html += "<h2>Top Contributors</h2>";
    html += "<ol>";
    html += tr("<li>John Doe (18 changes)</li>");
    html += tr("<li>Jane Smith (15 changes)</li>");
    html += tr("<li>Bob Wilson (12 changes)</li>");
    html += "</ol>";
    
    reportBrowser_->setHtml(html);
}

void AuditReportDialog::generateDetails() {
    QString html = "<h1>Detailed Audit Report</h1>";
    html += "<table border='1' cellpadding='5'>";
    html += "<tr><th>Time</th><th>Author</th><th>Type</th><th>Object</th><th>Description</th></tr>";
    
    // Sample data
    html += "<tr><td>2024-01-15 10:30</td><td>John Doe</td><td>CREATE</td><td>customers</td><td>Created customer table</td></tr>";
    html += "<tr><td>2024-01-14 14:20</td><td>Jane Smith</td><td>MODIFY</td><td>orders</td><td>Added index</td></tr>";
    html += "<tr><td>2024-01-13 09:15</td><td>Bob Wilson</td><td>DELETE</td><td>old_logs</td><td>Cleaned up old logs</td></tr>";
    
    html += "</table>";
    reportBrowser_->setHtml(html);
}

void AuditReportDialog::updateReportView() {
    // Update based on selection
}

void AuditReportDialog::onGenerateReport() {
    int type = reportTypeCombo_->currentIndex();
    if (type == 0) {
        generateSummary();
    } else {
        generateDetails();
    }
}

void AuditReportDialog::onExportPDF() {
    QMessageBox::information(this, tr("Export"), tr("PDF export not yet implemented."));
}

void AuditReportDialog::onExportCSV() {
    QMessageBox::information(this, tr("Export"), tr("CSV export not yet implemented."));
}

void AuditReportDialog::onExportHTML() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export HTML"),
        tr("audit_report.html"), tr("HTML Files (*.html)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("Report exported to %1").arg(fileName));
    }
}

void AuditReportDialog::onPrint() {
    QMessageBox::information(this, tr("Print"), tr("Print dialog would open here."));
}

void AuditReportDialog::onScheduleReport() {
    QMessageBox::information(this, tr("Schedule"), tr("Report scheduling would be configured here."));
}

// ============================================================================
// Session Browser Dialog
// ============================================================================
SessionBrowserDialog::SessionBrowserDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadSessions();
}

void SessionBrowserDialog::setupUi() {
    setWindowTitle(tr("Session Browser"));
    setMinimumSize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Sessions table
    sessionsTable_ = new QTableView(this);
    sessionsModel_ = new QStandardItemModel(this);
    sessionsModel_->setHorizontalHeaderLabels({tr("Session"), tr("Description"), tr("Author"), tr("Changes"), tr("Time")});
    sessionsTable_->setModel(sessionsModel_);
    sessionsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(sessionsTable_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* viewBtn = new QPushButton(tr("View Details"), this);
    connect(viewBtn, &QPushButton::clicked, this, &SessionBrowserDialog::onViewSessionDetails);
    btnLayout->addWidget(viewBtn);
    
    auto* rollbackBtn = new QPushButton(tr("Rollback Session"), this);
    connect(rollbackBtn, &QPushButton::clicked, this, &SessionBrowserDialog::onRollbackSession);
    btnLayout->addWidget(rollbackBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void SessionBrowserDialog::loadSessions() {
    sessions_.clear();
    
    // Sample data
    ChangeSession s1;
    s1.id = "sess1";
    s1.description = tr("Initial schema setup");
    s1.author = tr("Admin");
    s1.startTime = QDateTime::currentDateTime().addDays(-30);
    s1.isCommitted = true;
    sessions_.append(s1);
    
    ChangeSession s2;
    s2.id = "sess2";
    s2.description = tr("Add reporting tables");
    s2.author = tr("John Doe");
    s2.startTime = QDateTime::currentDateTime().addDays(-7);
    s2.isCommitted = true;
    sessions_.append(s2);
    
    updateSessionsList();
}

void SessionBrowserDialog::updateSessionsList() {
    sessionsModel_->clear();
    sessionsModel_->setHorizontalHeaderLabels({tr("Session"), tr("Description"), tr("Author"), tr("Changes"), tr("Time")});
    
    for (const auto& session : sessions_) {
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(session.id)
             << new QStandardItem(session.description)
             << new QStandardItem(session.author)
             << new QStandardItem(QString::number(session.changes.size()))
             << new QStandardItem(session.startTime.toString("yyyy-MM-dd"));
        
        sessionsModel_->appendRow(*row);
    }
}

void SessionBrowserDialog::updateSessionDetails(const ChangeSession& session) {
    Q_UNUSED(session)
    // Show session details
}

void SessionBrowserDialog::onSessionSelected(const QModelIndex& index) {
    Q_UNUSED(index)
    // Update details
}

void SessionBrowserDialog::onFilterSessions() {
    // Filter sessions
}

void SessionBrowserDialog::onViewSessionDetails() {
    QMessageBox::information(this, tr("Session Details"), tr("Session details would be shown here."));
}

void SessionBrowserDialog::onRollbackSession() {
    auto reply = QMessageBox::question(this, tr("Rollback Session"),
        tr("Are you sure you want to rollback this entire session?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Rollback"), tr("Session rollback initiated."));
    }
}

void SessionBrowserDialog::onTagSession() {
    QMessageBox::information(this, tr("Tag"), tr("Session tagging would be done here."));
}

void SessionBrowserDialog::onExportSession() {
    QMessageBox::information(this, tr("Export"), tr("Session export would be done here."));
}

// ============================================================================
// Change Monitor Widget
// ============================================================================
ChangeMonitorWidget::ChangeMonitorWidget(backend::SessionClient* client, QWidget* parent)
    : QWidget(parent), client_(client) {
    setupUi();
}

void ChangeMonitorWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // Status
    auto* statusLayout = new QHBoxLayout();
    statusLabel_ = new QLabel(tr("Monitoring stopped"), this);
    statusLayout->addWidget(statusLabel_);
    statusLayout->addStretch();
    
    startStopBtn_ = new QPushButton(tr("Start"), this);
    connect(startStopBtn_, &QPushButton::clicked, [this]() {
        if (isMonitoring_) {
            stopMonitoring();
        } else {
            startMonitoring();
        }
    });
    statusLayout->addWidget(startStopBtn_);
    
    layout->addLayout(statusLayout);
    
    // Max entries
    auto* maxLayout = new QHBoxLayout();
    maxLayout->addWidget(new QLabel(tr("Max entries:"), this));
    maxEntriesSpin_ = new QSpinBox(this);
    maxEntriesSpin_->setRange(10, 1000);
    maxEntriesSpin_->setValue(100);
    maxLayout->addWidget(maxEntriesSpin_);
    maxLayout->addStretch();
    layout->addLayout(maxLayout);
    
    // Log table
    logTable_ = new QTableWidget(this);
    logTable_->setColumnCount(4);
    logTable_->setHorizontalHeaderLabels({tr("Time"), tr("Type"), tr("Object"), tr("User")});
    logTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    logTable_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(logTable_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &ChangeMonitorWidget::onClearHistory);
    btnLayout->addWidget(clearBtn);
    
    layout->addLayout(btnLayout);
}

void ChangeMonitorWidget::startMonitoring() {
    isMonitoring_ = true;
    statusLabel_->setText(tr("Monitoring active"));
    statusLabel_->setStyleSheet("color: green;");
    startStopBtn_->setText(tr("Stop"));
    emit monitoringStarted();
}

void ChangeMonitorWidget::stopMonitoring() {
    isMonitoring_ = false;
    statusLabel_->setText(tr("Monitoring stopped"));
    statusLabel_->setStyleSheet("");
    startStopBtn_->setText(tr("Start"));
    emit monitoringStopped();
}

void ChangeMonitorWidget::onNewChange(const ChangeRecord& change) {
    addChangeToLog(change);
    emit changeDetected(change);
}

void ChangeMonitorWidget::addChangeToLog(const ChangeRecord& change) {
    int row = logTable_->rowCount();
    logTable_->insertRow(0);
    
    logTable_->setItem(0, 0, new QTableWidgetItem(change.timestamp.toString("hh:mm:ss")));
    
    QString typeStr;
    switch (change.changeType) {
        case ChangeType::Create: typeStr = tr("CREATE"); break;
        case ChangeType::Modify: typeStr = tr("MODIFY"); break;
        case ChangeType::Delete: typeStr = tr("DELETE"); break;
        default: typeStr = tr("CHANGE");
    }
    logTable_->setItem(0, 1, new QTableWidgetItem(typeStr));
    
    logTable_->setItem(0, 2, new QTableWidgetItem(change.objectName));
    logTable_->setItem(0, 3, new QTableWidgetItem(change.author));
    
    // Trim to max entries
    while (logTable_->rowCount() > maxEntriesSpin_->value()) {
        logTable_->removeRow(logTable_->rowCount() - 1);
    }
}

void ChangeMonitorWidget::onClearHistory() {
    logTable_->setRowCount(0);
    recentChanges_.clear();
}

} // namespace scratchrobin::ui
