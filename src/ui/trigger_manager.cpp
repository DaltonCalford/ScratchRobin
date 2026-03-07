#include "ui/trigger_manager.h"
#include "backend/session_client.h"

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
#include <QCheckBox>
#include <QListWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>

namespace scratchrobin::ui {

// ============================================================================
// TriggerManagerPanel
// ============================================================================

TriggerManagerPanel::TriggerManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("trigger_manager", parent), client_(client) {
    setupUi();
    setupModel();
}

void TriggerManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* newBtn = new QPushButton(tr("New"), this);
    auto* editBtn = new QPushButton(tr("Edit"), this);
    auto* dropBtn = new QPushButton(tr("Drop"), this);
    enableBtn_ = new QPushButton(tr("Enable"), this);
    disableBtn_ = new QPushButton(tr("Disable"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* historyBtn = new QPushButton(tr("History"), this);
    
    toolbar->addWidget(newBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(dropBtn);
    toolbar->addSeparator();
    toolbar->addWidget(enableBtn_);
    toolbar->addWidget(disableBtn_);
    toolbar->addSeparator();
    toolbar->addWidget(refreshBtn);
    toolbar->addWidget(historyBtn);
    
    mainLayout->addWidget(toolbar);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Trigger tree
    triggerTree_ = new QTreeView(this);
    triggerTree_->setHeaderHidden(false);
    triggerTree_->setAlternatingRowColors(true);
    triggerTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    splitter->addWidget(triggerTree_);
    
    // Code preview
    codePreview_ = new QTextEdit(this);
    codePreview_->setReadOnly(true);
    codePreview_->setFont(QFont("Consolas", 9));
    codePreview_->setPlaceholderText(tr("Select a trigger to see its code..."));
    codePreview_->setMaximumHeight(150);
    splitter->addWidget(codePreview_);
    
    splitter->setSizes({300, 100});
    mainLayout->addWidget(splitter);
    
    // Connections
    connect(newBtn, &QPushButton::clicked, this, &TriggerManagerPanel::onCreateTrigger);
    connect(editBtn, &QPushButton::clicked, this, &TriggerManagerPanel::onEditTrigger);
    connect(dropBtn, &QPushButton::clicked, this, &TriggerManagerPanel::onDropTrigger);
    connect(enableBtn_, &QPushButton::clicked, this, &TriggerManagerPanel::onEnableTrigger);
    connect(disableBtn_, &QPushButton::clicked, this, &TriggerManagerPanel::onDisableTrigger);
    connect(refreshBtn, &QPushButton::clicked, this, &TriggerManagerPanel::refresh);
    connect(historyBtn, &QPushButton::clicked, this, &TriggerManagerPanel::onShowExecutionHistory);
    connect(triggerTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TriggerManagerPanel::updateTriggerDetails);
}

void TriggerManagerPanel::setupModel() {
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Trigger"), tr("Table"), tr("Timing"), tr("Event"), tr("Status")});
    triggerTree_->setModel(model_);
    
    loadTriggers();
}

void TriggerManagerPanel::loadTriggers() {
    model_->removeRows(0, model_->rowCount());
    
    // Mock data
    struct MockTrigger {
        QString name;
        QString table;
        QString timing;
        QString event;
        bool enabled;
    };
    
    QList<MockTrigger> triggers = {
        {"trg_customers_audit", "customers", "BEFORE", "INSERT/UPDATE", true},
        {"trg_orders_validate", "orders", "BEFORE", "INSERT", true},
        {"trg_inventory_update", "products", "AFTER", "UPDATE", true},
        {"trg_log_changes", "audit_log", "AFTER", "INSERT/UPDATE/DELETE", false},
        {"trg_prevent_delete", "archived_records", "BEFORE", "DELETE", true},
    };
    
    for (const auto& t : triggers) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(t.name));
        row.append(new QStandardItem(t.table));
        row.append(new QStandardItem(t.timing));
        row.append(new QStandardItem(t.event));
        
        auto* statusItem = new QStandardItem(t.enabled ? tr("Enabled") : tr("Disabled"));
        if (!t.enabled) {
            statusItem->setForeground(QColor(128, 128, 128));
        }
        row.append(statusItem);
        
        // Store trigger name
        row[0]->setData(t.name, Qt::UserRole);
        row[0]->setData(t.enabled, Qt::UserRole + 1);
        
        model_->appendRow(row);
    }
    
    triggerTree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void TriggerManagerPanel::refresh() {
    loadTriggers();
}

void TriggerManagerPanel::panelActivated() {
    refresh();
}

void TriggerManagerPanel::onCreateTrigger() {
    TriggerDesignerDialog dlg(client_, QString(), QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void TriggerManagerPanel::onEditTrigger() {
    auto index = triggerTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString triggerName = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    QString tableName = model_->item(index.row(), 1)->text();
    
    TriggerDesignerDialog dlg(client_, triggerName, tableName, this);
    dlg.exec();
    refresh();
}

void TriggerManagerPanel::onDropTrigger() {
    auto index = triggerTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString triggerName = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    auto reply = QMessageBox::warning(this, tr("Drop Trigger"),
        tr("Are you sure you want to drop trigger '%1'?").arg(triggerName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        refresh();
    }
}

void TriggerManagerPanel::onEnableTrigger() {
    auto index = triggerTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString triggerName = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    // ALTER TRIGGER ... ENABLE
    QMessageBox::information(this, tr("Enable Trigger"),
        tr("Trigger '%1' has been enabled.").arg(triggerName));
    refresh();
}

void TriggerManagerPanel::onDisableTrigger() {
    auto index = triggerTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString triggerName = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    // ALTER TRIGGER ... DISABLE
    QMessageBox::information(this, tr("Disable Trigger"),
        tr("Trigger '%1' has been disabled.").arg(triggerName));
    refresh();
}

void TriggerManagerPanel::onShowTriggerCode() {
    updateTriggerDetails();
}

void TriggerManagerPanel::onShowExecutionHistory() {
    auto index = triggerTree_->currentIndex();
    QString triggerName;
    if (index.isValid()) {
        triggerName = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    }
    
    TriggerHistoryDialog dlg(client_, triggerName, this);
    dlg.exec();
}

void TriggerManagerPanel::updateTriggerDetails() {
    auto index = triggerTree_->currentIndex();
    if (!index.isValid()) {
        codePreview_->clear();
        enableBtn_->setEnabled(false);
        disableBtn_->setEnabled(false);
        return;
    }
    
    QString triggerName = model_->item(index.row(), 0)->data(Qt::UserRole).toString();
    QString tableName = model_->item(index.row(), 1)->text();
    bool isEnabled = model_->item(index.row(), 0)->data(Qt::UserRole + 1).toBool();
    
    // Mock code preview
    QString code = QString("CREATE TRIGGER %1\n"
                          "BEFORE INSERT OR UPDATE ON %2\n"
                          "FOR EACH ROW\n"
                          "BEGIN\n"
                          "    -- Audit trail\n"
                          "    NEW.modified_at = CURRENT_TIMESTAMP;\n"
                          "    NEW.modified_by = CURRENT_USER;\n"
                          "END;").arg(triggerName).arg(tableName);
    
    codePreview_->setPlainText(code);
    
    enableBtn_->setEnabled(!isEnabled);
    disableBtn_->setEnabled(isEnabled);
    
    emit triggerSelected(triggerName);
}

// ============================================================================
// TriggerDesignerDialog
// ============================================================================

TriggerDesignerDialog::TriggerDesignerDialog(backend::SessionClient* client,
                                            const QString& triggerName,
                                            const QString& tableName,
                                            QWidget* parent)
    : QDialog(parent), client_(client), originalTriggerName_(triggerName), isEditing_(!triggerName.isEmpty()) {
    setWindowTitle(isEditing_ ? tr("Edit Trigger - %1").arg(triggerName) : tr("Create New Trigger"));
    setMinimumSize(800, 600);
    
    setupUi();
    
    if (isEditing_) {
        loadTrigger(triggerName);
    } else if (!tableName.isEmpty()) {
        tableCombo_->setCurrentText(tableName);
    }
}

void TriggerDesignerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* tabWidget = new QTabWidget(this);
    
    setupBasicTab();
    setupAdvancedTab();
    setupPreviewTab();
    
    tabWidget->addTab(findChild<QWidget*>("basicTab"), tr("&Basic"));
    tabWidget->addTab(findChild<QWidget*>("advancedTab"), tr("&Advanced"));
    tabWidget->addTab(findChild<QWidget*>("previewTab"), tr("&Preview"));
    
    mainLayout->addWidget(tabWidget);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    auto* validateBtn = new QPushButton(tr("&Validate"), this);
    auto* formatBtn = new QPushButton(tr("&Format"), this);
    auto* okBtn = new QPushButton(isEditing_ ? tr("&Alter") : tr("&Create"), this);
    okBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(tr("&Cancel"), this);
    
    buttonLayout->addWidget(validateBtn);
    buttonLayout->addWidget(formatBtn);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(validateBtn, &QPushButton::clicked, this, &TriggerDesignerDialog::onValidate);
    connect(formatBtn, &QPushButton::clicked, this, &TriggerDesignerDialog::onFormatCode);
    connect(okBtn, &QPushButton::clicked, this, &TriggerDesignerDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void TriggerDesignerDialog::setupBasicTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("basicTab");
    auto* layout = new QVBoxLayout(tab);
    
    auto* formLayout = new QGridLayout();
    
    formLayout->addWidget(new QLabel(tr("Trigger Name:"), this), 0, 0);
    nameEdit_ = new QLineEdit(this);
    formLayout->addWidget(nameEdit_, 0, 1);
    
    formLayout->addWidget(new QLabel(tr("Table:"), this), 1, 0);
    tableCombo_ = new QComboBox(this);
    tableCombo_->setEditable(true);
    tableCombo_->addItems({"customers", "orders", "products", "inventory", "audit_log"});
    formLayout->addWidget(tableCombo_, 1, 1);
    
    formLayout->addWidget(new QLabel(tr("Timing:"), this), 2, 0);
    timingCombo_ = new QComboBox(this);
    timingCombo_->addItems({"BEFORE", "AFTER", "INSTEAD OF"});
    formLayout->addWidget(timingCombo_, 2, 1);
    
    formLayout->addWidget(new QLabel(tr("Events:"), this), 3, 0);
    eventsList_ = new QListWidget(this);
    eventsList_->setSelectionMode(QAbstractItemView::MultiSelection);
    eventsList_->addItems({"INSERT", "UPDATE", "DELETE", "TRUNCATE"});
    formLayout->addWidget(eventsList_, 3, 1);
    
    layout->addLayout(formLayout);
    
    layout->addWidget(new QLabel(tr("Trigger Code:"), this));
    codeEdit_ = new QTextEdit(this);
    codeEdit_->setFont(QFont("Consolas", 10));
    codeEdit_->setPlaceholderText(tr("Enter trigger code here...\n"
                                     "Example:\n"
                                     "BEGIN\n"
                                     "    NEW.modified_at = NOW();\n"
                                     "END;"));
    layout->addWidget(codeEdit_);
}

void TriggerDesignerDialog::setupAdvancedTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("advancedTab");
    auto* layout = new QVBoxLayout(tab);
    
    forEachRowCheck_ = new QCheckBox(tr("FOR EACH ROW (row-level trigger)"), this);
    forEachRowCheck_->setChecked(true);
    layout->addWidget(forEachRowCheck_);
    
    auto* whenLayout = new QHBoxLayout();
    whenClauseCheck_ = new QCheckBox(tr("WHEN clause:"), this);
    whenLayout->addWidget(whenClauseCheck_);
    whenClauseEdit_ = new QLineEdit(this);
    whenClauseEdit_->setPlaceholderText(tr("condition"));
    whenLayout->addWidget(whenClauseEdit_);
    layout->addLayout(whenLayout);
    
    deferrableCheck_ = new QCheckBox(tr("DEFERRABLE constraint trigger"), this);
    layout->addWidget(deferrableCheck_);
    
    auto* initiallyLayout = new QHBoxLayout();
    initiallyLayout->addWidget(new QLabel(tr("Initially:"), this));
    initiallyCombo_ = new QComboBox(this);
    initiallyCombo_->addItems({"IMMEDIATE", "DEFERRED"});
    initiallyLayout->addWidget(initiallyCombo_);
    initiallyLayout->addStretch();
    layout->addLayout(initiallyLayout);
    
    layout->addWidget(new QLabel(tr("Comment:"), this));
    commentEdit_ = new QLineEdit(this);
    layout->addWidget(commentEdit_);
    
    layout->addStretch();
}

void TriggerDesignerDialog::setupPreviewTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("previewTab");
    auto* layout = new QVBoxLayout(tab);
    
    layout->addWidget(new QLabel(tr("Generated SQL:"), this));
    sqlPreview_ = new QTextEdit(this);
    sqlPreview_->setReadOnly(true);
    sqlPreview_->setFont(QFont("Consolas", 10));
    layout->addWidget(sqlPreview_);
}

void TriggerDesignerDialog::loadTrigger(const QString& triggerName) {
    nameEdit_->setText(triggerName);
    nameEdit_->setEnabled(false);
    
    // Mock load
    tableCombo_->setCurrentText("customers");
    timingCombo_->setCurrentText("BEFORE");
    
    codeEdit_->setPlainText("BEGIN\n    NEW.modified_at = CURRENT_TIMESTAMP;\nEND;");
}

QString TriggerDesignerDialog::generatedSql() const {
    QString sql;
    
    sql = QString("CREATE TRIGGER %1\n").arg(nameEdit_->text());
    sql += QString("%1 ").arg(timingCombo_->currentText());
    
    // Events
    QStringList events;
    for (auto* item : eventsList_->selectedItems()) {
        events.append(item->text());
    }
    if (events.isEmpty()) events.append("INSERT");
    sql += events.join(" OR ");
    
    sql += QString(" ON %1\n").arg(tableCombo_->currentText());
    
    if (forEachRowCheck_->isChecked()) {
        sql += "FOR EACH ROW\n";
    }
    
    if (whenClauseCheck_->isChecked() && !whenClauseEdit_->text().isEmpty()) {
        sql += QString("WHEN (%1)\n").arg(whenClauseEdit_->text());
    }
    
    sql += codeEdit_->toPlainText();
    
    if (!commentEdit_->text().isEmpty()) {
        sql += QString("\n\nCOMMENT ON TRIGGER %1 IS '%2';").arg(nameEdit_->text()).arg(commentEdit_->text());
    }
    
    return sql;
}

void TriggerDesignerDialog::updatePreview() {
    if (sqlPreview_) {
        sqlPreview_->setPlainText(generatedSql());
    }
}

void TriggerDesignerDialog::onValidate() {
    QMessageBox::information(this, tr("Validation"), tr("Trigger syntax is valid."));
}

void TriggerDesignerDialog::onSave() {
    if (nameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Trigger name is required."));
        return;
    }
    
    accept();
}

void TriggerDesignerDialog::onFormatCode() {
    // Format the trigger code
}

void TriggerDesignerDialog::onToggleAdvanced() {
    // Show/hide advanced options
}

// ============================================================================
// TriggerHistoryDialog
// ============================================================================

TriggerHistoryDialog::TriggerHistoryDialog(backend::SessionClient* client,
                                          const QString& triggerName,
                                          QWidget* parent)
    : QDialog(parent), client_(client), triggerName_(triggerName) {
    setWindowTitle(triggerName.isEmpty() ? tr("Trigger Execution History") 
                                         : tr("History - %1").arg(triggerName));
    setMinimumSize(800, 500);
    
    setupUi();
    loadHistory();
}

void TriggerHistoryDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Filter toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    toolbarLayout->addWidget(new QLabel(tr("Trigger:"), this));
    triggerFilter_ = new QComboBox(this);
    triggerFilter_->setEditable(true);
    triggerFilter_->addItem(tr("(All Triggers)"));
    triggerFilter_->addItems({"trg_customers_audit", "trg_orders_validate", 
                               "trg_inventory_update", "trg_log_changes"});
    if (!triggerName_.isEmpty()) {
        triggerFilter_->setCurrentText(triggerName_);
    }
    toolbarLayout->addWidget(triggerFilter_);
    
    toolbarLayout->addWidget(new QLabel(tr("Search:"), this));
    searchEdit_ = new QLineEdit(this);
    toolbarLayout->addWidget(searchEdit_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* exportBtn = new QPushButton(tr("Export"), this);
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    
    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addWidget(exportBtn);
    toolbarLayout->addWidget(clearBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // History table
    historyTable_ = new QTableView(this);
    historyTable_->setAlternatingRowColors(true);
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(historyTable_);
    
    // Status
    statusLabel_ = new QLabel(this);
    mainLayout->addWidget(statusLabel_);
    
    // Close button
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);
    
    connect(refreshBtn, &QPushButton::clicked, this, &TriggerHistoryDialog::onRefresh);
    connect(exportBtn, &QPushButton::clicked, this, &TriggerHistoryDialog::onExport);
    connect(clearBtn, &QPushButton::clicked, this, &TriggerHistoryDialog::onClear);
}

void TriggerHistoryDialog::loadHistory() {
    auto* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({tr("Timestamp"), tr("Trigger"), tr("Table"), 
                                      tr("Operation"), tr("Row Count"), tr("Duration")});
    
    // Mock history data
    QList<QList<QString>> history = {
        {"2026-03-06 10:15:23", "trg_customers_audit", "customers", "INSERT", "1", "2ms"},
        {"2026-03-06 10:15:25", "trg_customers_audit", "customers", "UPDATE", "1", "1ms"},
        {"2026-03-06 10:16:00", "trg_orders_validate", "orders", "INSERT", "1", "3ms"},
        {"2026-03-06 10:16:30", "trg_inventory_update", "products", "UPDATE", "5", "8ms"},
        {"2026-03-06 10:17:00", "trg_log_changes", "audit_log", "INSERT", "1", "1ms"},
    };
    
    for (const auto& row : history) {
        QList<QStandardItem*> items;
        for (const auto& cell : row) {
            items.append(new QStandardItem(cell));
        }
        model->appendRow(items);
    }
    
    historyTable_->setModel(model);
    historyTable_->horizontalHeader()->setStretchLastSection(true);
    
    statusLabel_->setText(tr("Showing %1 entries").arg(history.size()));
}

void TriggerHistoryDialog::onRefresh() {
    loadHistory();
}

void TriggerHistoryDialog::onFilter() {
    // Filter by trigger name
    loadHistory();
}

void TriggerHistoryDialog::onClear() {
    auto reply = QMessageBox::question(this, tr("Clear History"),
        tr("Are you sure you want to clear the trigger execution history?"));
    
    if (reply == QMessageBox::Yes) {
        // Clear history
        loadHistory();
    }
}

void TriggerHistoryDialog::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export History"),
        QString(), tr("CSV Files (*.csv);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        // Export to CSV
    }
}

} // namespace scratchrobin::ui
