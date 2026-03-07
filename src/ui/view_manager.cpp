#include "ui/view_manager.h"
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
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>

namespace scratchrobin::ui {

// ============================================================================
// ViewManagerPanel
// ============================================================================

ViewManagerPanel::ViewManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("view_manager", parent), client_(client) {
    setupUi();
    setupModel();
}

void ViewManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    createBtn_ = new QPushButton(tr("New"), this);
    editBtn_ = new QPushButton(tr("Edit"), this);
    dropBtn_ = new QPushButton(tr("Drop"), this);
    refreshBtn_ = new QPushButton(tr("Refresh"), this);
    dataBtn_ = new QPushButton(tr("Data"), this);
    
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(editBtn_);
    toolbar->addWidget(dropBtn_);
    toolbar->addSeparator();
    toolbar->addWidget(refreshBtn_);
    toolbar->addWidget(dataBtn_);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter views..."));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Splitter for tree and preview
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // View tree
    viewTree_ = new QTreeView(this);
    viewTree_->setHeaderHidden(true);
    viewTree_->setAlternatingRowColors(true);
    viewTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    splitter->addWidget(viewTree_);
    
    // Definition preview
    definitionPreview_ = new QTextEdit(this);
    definitionPreview_->setReadOnly(true);
    definitionPreview_->setFont(QFont("Consolas", 9));
    definitionPreview_->setPlaceholderText(tr("Select a view to see its definition..."));
    definitionPreview_->setMaximumHeight(150);
    splitter->addWidget(definitionPreview_);
    
    splitter->setSizes({300, 100});
    mainLayout->addWidget(splitter);
    
    // Connections
    connect(createBtn_, &QPushButton::clicked, this, &ViewManagerPanel::onCreateView);
    connect(editBtn_, &QPushButton::clicked, this, &ViewManagerPanel::onEditView);
    connect(dropBtn_, &QPushButton::clicked, this, &ViewManagerPanel::onDropView);
    connect(refreshBtn_, &QPushButton::clicked, this, &ViewManagerPanel::refresh);
    connect(dataBtn_, &QPushButton::clicked, this, &ViewManagerPanel::onShowViewData);
    connect(filterEdit_, &QLineEdit::textChanged, this, &ViewManagerPanel::onFilterChanged);
    connect(viewTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ViewManagerPanel::updateViewDetails);
}

void ViewManagerPanel::setupModel() {
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("View"), tr("Schema"), tr("Type")});
    viewTree_->setModel(model_);
    
    // Load initial data
    loadViews();
}

void ViewManagerPanel::loadViews() {
    model_->clear();
    
    // Add categories
    auto* userViews = new QStandardItem(tr("User Views"));
    auto* systemViews = new QStandardItem(tr("System Views"));
    auto* materializedViews = new QStandardItem(tr("Materialized Views"));
    
    // Mock data - would query database
    QStringList userViewNames = {"active_customers", "sales_summary", "inventory_status",
                                 "employee_directory", "monthly_reports"};
    for (const auto& name : userViewNames) {
        auto* item = new QStandardItem(name);
        item->setData("VIEW", Qt::UserRole);
        item->setData(name, Qt::UserRole + 1);
        userViews->appendRow(item);
    }
    
    QStringList matViewNames = {"cached_sales", "aggregated_metrics"};
    for (const auto& name : matViewNames) {
        auto* item = new QStandardItem(name);
        item->setData("MATERIALIZED VIEW", Qt::UserRole);
        item->setData(name, Qt::UserRole + 1);
        materializedViews->appendRow(item);
    }
    
    model_->appendRow(userViews);
    model_->appendRow(materializedViews);
    model_->appendRow(systemViews);
    
    viewTree_->expandAll();
}

void ViewManagerPanel::refresh() {
    loadViews();
}

void ViewManagerPanel::panelActivated() {
    refresh();
}

void ViewManagerPanel::onCreateView() {
    ViewDesignerDialog dlg(client_, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        QString sql = dlg.generatedSql();
        // Execute CREATE VIEW
        refresh();
    }
}

void ViewManagerPanel::onEditView() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    if (viewName.isEmpty()) return;
    
    ViewDesignerDialog dlg(client_, viewName, this);
    dlg.exec();
    refresh();
}

void ViewManagerPanel::onDropView() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    if (viewName.isEmpty()) return;
    
    auto reply = QMessageBox::warning(this, tr("Drop View"),
        tr("Are you sure you want to drop view '%1'?").arg(viewName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Execute DROP VIEW
        refresh();
    }
}

void ViewManagerPanel::onRefreshView() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    if (viewName.isEmpty()) return;
    
    // Execute REFRESH MATERIALIZED VIEW for materialized views
}

void ViewManagerPanel::onShowViewData() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    if (viewName.isEmpty()) return;
    
    ViewDataDialog dlg(client_, viewName, this);
    dlg.exec();
}

void ViewManagerPanel::onShowViewDefinition() {
    updateViewDetails();
}

void ViewManagerPanel::onShowDependencies() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    if (viewName.isEmpty()) return;
    
    ViewDependenciesDialog dlg(client_, viewName, this);
    dlg.exec();
}

void ViewManagerPanel::onDuplicateView() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    QString newName = QInputDialog::getText(this, tr("Duplicate View"),
                                            tr("New view name:"));
    if (!newName.isEmpty()) {
        // Duplicate view
        refresh();
    }
}

void ViewManagerPanel::onRenameView() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    QString newName = QInputDialog::getText(this, tr("Rename View"),
                                            tr("New name:"),
                                            QLineEdit::Normal,
                                            viewName);
    if (!newName.isEmpty() && newName != viewName) {
        // Rename view
        refresh();
    }
}

void ViewManagerPanel::onExportView() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export View"),
        QString(), tr("SQL Files (*.sql);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << definitionPreview_->toPlainText();
            file.close();
        }
    }
}

void ViewManagerPanel::onImportView() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import View"),
        QString(), tr("SQL Files (*.sql);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QString sql = QString::fromUtf8(file.readAll());
            file.close();
            
            // Parse and create view
            ViewDesignerDialog dlg(client_, QString(), this);
            // dlg.setDefinition(sql);
            dlg.exec();
        }
    }
}

void ViewManagerPanel::onFilterChanged(const QString& filter) {
    // Filter tree model
    for (int i = 0; i < model_->rowCount(); ++i) {
        QStandardItem* parent = model_->item(i);
        bool parentVisible = false;
        
        for (int j = 0; j < parent->rowCount(); ++j) {
            QStandardItem* child = parent->child(j);
            bool match = filter.isEmpty() || 
                        child->text().contains(filter, Qt::CaseInsensitive);
            viewTree_->setRowHidden(j, parent->index(), !match);
            if (match) parentVisible = true;
        }
        
        viewTree_->setRowHidden(i, QModelIndex(), !parentVisible && !filter.isEmpty());
    }
}

void ViewManagerPanel::updateViewDetails() {
    auto index = viewTree_->currentIndex();
    if (!index.isValid()) {
        definitionPreview_->clear();
        return;
    }
    
    QString viewName = index.data(Qt::UserRole + 1).toString();
    if (viewName.isEmpty()) {
        definitionPreview_->clear();
        return;
    }
    
    // Generate mock definition
    QString definition = QString("CREATE VIEW %1 AS\n"
                                "SELECT *\n"
                                "FROM some_table\n"
                                "WHERE active = true;").arg(viewName);
    
    definitionPreview_->setPlainText(definition);
    emit viewSelected(viewName);
}

// ============================================================================
// ViewDesignerDialog
// ============================================================================

ViewDesignerDialog::ViewDesignerDialog(backend::SessionClient* client,
                                      const QString& viewName,
                                      QWidget* parent)
    : QDialog(parent), client_(client), originalViewName_(viewName), isEditing_(!viewName.isEmpty()) {
    setWindowTitle(isEditing_ ? tr("Edit View - %1").arg(viewName) : tr("Create New View"));
    setMinimumSize(800, 600);
    resize(900, 700);
    
    setupUi();
    
    if (isEditing_) {
        loadView(viewName);
    }
}

void ViewDesignerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Tabs
    auto* tabWidget = new QTabWidget(this);
    
    // Definition tab
    auto* defTab = new QWidget(this);
    auto* defLayout = new QVBoxLayout(defTab);
    
    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel(tr("Name:"), this));
    nameEdit_ = new QLineEdit(this);
    nameLayout->addWidget(nameEdit_);
    nameLayout->addWidget(new QLabel(tr("Schema:"), this));
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    nameLayout->addWidget(schemaCombo_);
    defLayout->addLayout(nameLayout);
    
    defLayout->addWidget(new QLabel(tr("View Definition (SQL):"), this));
    definitionEdit_ = new QTextEdit(this);
    definitionEdit_->setFont(QFont("Consolas", 10));
    definitionEdit_->setPlaceholderText(tr("Enter SELECT statement for view..."));
    defLayout->addWidget(definitionEdit_);
    
    tabWidget->addTab(defTab, tr("&Definition"));
    
    // Options tab
    auto* optTab = new QWidget(this);
    auto* optLayout = new QGridLayout(optTab);
    
    checkOptionCheck_ = new QCheckBox(tr("WITH CHECK OPTION"), this);
    optLayout->addWidget(checkOptionCheck_, 0, 0);
    
    readOnlyCheck_ = new QCheckBox(tr("READ ONLY"), this);
    optLayout->addWidget(readOnlyCheck_, 1, 0);
    
    materializedCheck_ = new QCheckBox(tr("MATERIALIZED VIEW"), this);
    optLayout->addWidget(materializedCheck_, 2, 0);
    
    optLayout->addWidget(new QLabel(tr("Comment:"), this), 3, 0);
    commentEdit_ = new QLineEdit(this);
    optLayout->addWidget(commentEdit_, 4, 0);
    
    tabWidget->addTab(optTab, tr("&Options"));
    
    // Preview tab
    auto* previewTab = new QWidget(this);
    auto* previewLayout = new QVBoxLayout(previewTab);
    
    sqlPreview_ = new QTextEdit(this);
    sqlPreview_->setReadOnly(true);
    sqlPreview_->setFont(QFont("Consolas", 10));
    previewLayout->addWidget(new QLabel(tr("Generated SQL:"), this));
    previewLayout->addWidget(sqlPreview_);
    
    previewGrid_ = new QTableView(this);
    previewLayout->addWidget(new QLabel(tr("Data Preview:"), this));
    previewLayout->addWidget(previewGrid_);
    
    tabWidget->addTab(previewTab, tr("&Preview"));
    
    mainLayout->addWidget(tabWidget);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    auto* validateBtn = new QPushButton(tr("&Validate"), this);
    buttonLayout->addWidget(validateBtn);
    
    auto* previewBtn = new QPushButton(tr("Pre&view"), this);
    buttonLayout->addWidget(previewBtn);
    
    auto* formatBtn = new QPushButton(tr("&Format"), this);
    buttonLayout->addWidget(formatBtn);
    
    buttonLayout->addSpacing(20);
    
    auto* okBtn = new QPushButton(isEditing_ ? tr("&Alter") : tr("&Create"), this);
    okBtn->setDefault(true);
    buttonLayout->addWidget(okBtn);
    
    auto* cancelBtn = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(validateBtn, &QPushButton::clicked, this, &ViewDesignerDialog::onValidate);
    connect(previewBtn, &QPushButton::clicked, this, &ViewDesignerDialog::onPreview);
    connect(formatBtn, &QPushButton::clicked, this, &ViewDesignerDialog::onFormatSql);
    connect(okBtn, &QPushButton::clicked, this, &ViewDesignerDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(definitionEdit_, &QTextEdit::textChanged, this, &ViewDesignerDialog::updatePreview);
    connect(materializedCheck_, &QCheckBox::toggled, this, &ViewDesignerDialog::updatePreview);
}

void ViewDesignerDialog::loadView(const QString& viewName) {
    nameEdit_->setText(viewName);
    nameEdit_->setEnabled(false);  // Can't rename in edit mode
    
    // Mock definition
    definitionEdit_->setPlainText(QString("SELECT * FROM %1_source WHERE active = true")
                                  .arg(viewName));
}

void ViewDesignerDialog::buildQuery() {
    updatePreview();
}

void ViewDesignerDialog::updatePreview() {
    QString sql = generatedSql();
    sqlPreview_->setPlainText(sql);
}

QString ViewDesignerDialog::generatedSql() const {
    QString sql;
    
    QString viewType = materializedCheck_->isChecked() ? "MATERIALIZED VIEW" : "VIEW";
    sql = QString("CREATE OR REPLACE %1 %2.%3 AS\n")
          .arg(viewType)
          .arg(schemaCombo_->currentText())
          .arg(nameEdit_->text());
    
    sql += definitionEdit_->toPlainText();
    
    if (checkOptionCheck_->isChecked()) {
        sql += "\nWITH CHECK OPTION";
    }
    
    sql += ";";
    
    if (!commentEdit_->text().isEmpty()) {
        sql += QString("\n\nCOMMENT ON VIEW %1.%2 IS '%3';")
               .arg(schemaCombo_->currentText())
               .arg(nameEdit_->text())
               .arg(commentEdit_->text());
    }
    
    return sql;
}

void ViewDesignerDialog::onValidate() {
    // Validate SQL syntax
    QMessageBox::information(this, tr("Validation"), tr("View definition is valid."));
}

void ViewDesignerDialog::onPreview() {
    // Show data preview
    tabWidget()->setCurrentIndex(2);  // Switch to preview tab
}

void ViewDesignerDialog::onSave() {
    if (nameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("View name is required."));
        return;
    }
    
    if (definitionEdit_->toPlainText().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("View definition is required."));
        return;
    }
    
    accept();
}

void ViewDesignerDialog::onFormatSql() {
    // Format the SQL
    QString sql = definitionEdit_->toPlainText();
    // Apply formatting
    definitionEdit_->setPlainText(sql);
}

void ViewDesignerDialog::onCheckSyntax() {
    // Check SQL syntax
}

void ViewDesignerDialog::onToggleOptions() {
    // Show/hide options
}

QTabWidget* ViewDesignerDialog::tabWidget() const {
    return findChild<QTabWidget*>();
}

// ============================================================================
// ViewDataDialog
// ============================================================================

ViewDataDialog::ViewDataDialog(backend::SessionClient* client,
                              const QString& viewName,
                              QWidget* parent)
    : QDialog(parent), client_(client), viewName_(viewName) {
    setWindowTitle(tr("View Data - %1").arg(viewName));
    setMinimumSize(800, 500);
    
    setupUi();
    loadData();
}

void ViewDataDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    toolbarLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("WHERE clause..."));
    toolbarLayout->addWidget(filterEdit_);
    
    toolbarLayout->addWidget(new QLabel(tr("Sort:"), this));
    sortCombo_ = new QComboBox(this);
    sortCombo_->addItem(tr("(Default)"));
    toolbarLayout->addWidget(sortCombo_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    toolbarLayout->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    toolbarLayout->addWidget(exportBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Data grid
    dataGrid_ = new QTableView(this);
    dataGrid_->setAlternatingRowColors(true);
    mainLayout->addWidget(dataGrid_);
    
    // Status
    statusLabel_ = new QLabel(tr("Loading..."), this);
    mainLayout->addWidget(statusLabel_);
    
    // Connections
    connect(refreshBtn, &QPushButton::clicked, this, &ViewDataDialog::onRefresh);
    connect(exportBtn, &QPushButton::clicked, this, &ViewDataDialog::onExport);
}

void ViewDataDialog::loadData() {
    // Mock data loading
    statusLabel_->setText(tr("Showing %1 rows from view '%2'")
                         .arg(100).arg(viewName_));
}

void ViewDataDialog::onRefresh() {
    loadData();
}

void ViewDataDialog::onFilter() {
    loadData();
}

void ViewDataDialog::onSort() {
    loadData();
}

void ViewDataDialog::onExport() {
    // Export data
}

void ViewDataDialog::onPageChanged(int page) {
    currentPage_ = page;
    loadData();
}

// ============================================================================
// ViewDependenciesDialog
// ============================================================================

ViewDependenciesDialog::ViewDependenciesDialog(backend::SessionClient* client,
                                              const QString& viewName,
                                              QWidget* parent)
    : QDialog(parent), client_(client), viewName_(viewName) {
    setWindowTitle(tr("View Dependencies - %1").arg(viewName));
    setMinimumSize(600, 400);
    
    setupUi();
    loadDependencies();
    loadDependents();
}

void ViewDependenciesDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Dependencies (what this view depends on)
    auto* depsGroup = new QGroupBox(tr("Depends On"), this);
    auto* depsLayout = new QVBoxLayout(depsGroup);
    dependenciesTree_ = new QTreeView(this);
    depsLayout->addWidget(dependenciesTree_);
    splitter->addWidget(depsGroup);
    
    // Dependents (what depends on this view)
    auto* dentsGroup = new QGroupBox(tr("Used By"), this);
    auto* dentsLayout = new QVBoxLayout(dentsGroup);
    dependentsTree_ = new QTreeView(this);
    dentsLayout->addWidget(dependentsTree_);
    splitter->addWidget(dentsGroup);
    
    mainLayout->addWidget(splitter);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);
}

void ViewDependenciesDialog::loadDependencies() {
    // Mock dependencies
    auto* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({tr("Object"), tr("Type")});
    
    auto* tables = new QStandardItem(tr("Tables"));
    tables->appendRow(new QStandardItem("customers"));
    tables->appendRow(new QStandardItem("orders"));
    model->appendRow(tables);
    
    auto* views = new QStandardItem(tr("Views"));
    model->appendRow(views);
    
    auto* functions = new QStandardItem(tr("Functions"));
    model->appendRow(functions);
    
    dependenciesTree_->setModel(model);
    dependenciesTree_->expandAll();
}

void ViewDependenciesDialog::loadDependents() {
    // Mock dependents
    auto* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({tr("Object"), tr("Type")});
    
    auto* views = new QStandardItem(tr("Views"));
    views->appendRow(new QStandardItem("extended_sales_view"));
    model->appendRow(views);
    
    auto* procedures = new QStandardItem(tr("Procedures"));
    model->appendRow(procedures);
    
    dependentsTree_->setModel(model);
    dependentsTree_->expandAll();
}

} // namespace scratchrobin::ui
