#include "ui/tools_menu.h"

#include <QMenu>
#include <QAction>
#include <QWidget>

namespace scratchrobin::ui {

ToolsMenu::ToolsMenu(QWidget* parent)
    : QObject(parent)
    , parent_(parent) {
    setupActions();
}

ToolsMenu::~ToolsMenu() = default;

void ToolsMenu::setupActions() {
    menu_ = new QMenu(tr("&Tools"), parent_);

    // DDL/Compare group
    actionGenerateDdl_ = menu_->addAction(tr("&Generate DDL..."), this, &ToolsMenu::onGenerateDdl);
    actionGenerateDdl_->setStatusTip(tr("Generate DDL for selected objects"));

    actionCompareSchemas_ = menu_->addAction(tr("Compare &Schemas..."), this, &ToolsMenu::onCompareSchemas);
    actionCompareSchemas_->setStatusTip(tr("Compare database schemas"));

    actionCompareData_ = menu_->addAction(tr("Compare &Data..."), this, &ToolsMenu::onCompareData);
    actionCompareData_->setStatusTip(tr("Compare table data"));

    menu_->addSeparator();

    // Import/Export submenus
    createImportSubmenu();
    createExportSubmenu();

    menu_->addSeparator();

    // Migration and Scheduler
    actionMigration_ = menu_->addAction(tr("Database &Migration..."), this, &ToolsMenu::onMigration);
    actionMigration_->setStatusTip(tr("Migrate data between databases"));

    actionScheduler_ = menu_->addAction(tr("Job &Scheduler..."), this, &ToolsMenu::onScheduler);
    actionScheduler_->setStatusTip(tr("Manage scheduled jobs"));

    menu_->addSeparator();

    // Monitor submenu
    createMonitorSubmenu();

    updateActionStates();
}

void ToolsMenu::createImportSubmenu() {
    importMenu_ = new QMenu(tr("&Import Data"), menu_);

    auto* actionImportSql = importMenu_->addAction(tr("Import &SQL..."), this, &ToolsMenu::onImportSql);
    actionImportSql->setStatusTip(tr("Import SQL script file"));

    auto* actionImportCsv = importMenu_->addAction(tr("Import &CSV..."), this, &ToolsMenu::onImportCsv);
    actionImportCsv->setStatusTip(tr("Import CSV file"));

    auto* actionImportJson = importMenu_->addAction(tr("Import &JSON..."), this, &ToolsMenu::onImportJson);
    actionImportJson->setStatusTip(tr("Import JSON file"));

    auto* actionImportExcel = importMenu_->addAction(tr("Import E&xcel..."), this, &ToolsMenu::onImportExcel);
    actionImportExcel->setStatusTip(tr("Import Excel file"));

    menu_->addMenu(importMenu_);
}

void ToolsMenu::createExportSubmenu() {
    exportMenu_ = new QMenu(tr("&Export Data"), menu_);

    auto* actionExportSql = exportMenu_->addAction(tr("Export &SQL..."), this, &ToolsMenu::onExportSql);
    actionExportSql->setStatusTip(tr("Export as SQL script"));

    auto* actionExportCsv = exportMenu_->addAction(tr("Export &CSV..."), this, &ToolsMenu::onExportCsv);
    actionExportCsv->setStatusTip(tr("Export as CSV file"));

    auto* actionExportJson = exportMenu_->addAction(tr("Export &JSON..."), this, &ToolsMenu::onExportJson);
    actionExportJson->setStatusTip(tr("Export as JSON file"));

    auto* actionExportExcel = exportMenu_->addAction(tr("Export &Excel..."), this, &ToolsMenu::onExportExcel);
    actionExportExcel->setStatusTip(tr("Export as Excel file"));

    auto* actionExportXml = exportMenu_->addAction(tr("Export &XML..."), this, &ToolsMenu::onExportXml);
    actionExportXml->setStatusTip(tr("Export as XML file"));

    auto* actionExportHtml = exportMenu_->addAction(tr("Export &HTML..."), this, &ToolsMenu::onExportHtml);
    actionExportHtml->setStatusTip(tr("Export as HTML file"));

    menu_->addMenu(exportMenu_);
}

void ToolsMenu::createMonitorSubmenu() {
    monitorMenu_ = new QMenu(tr("&Monitor"), menu_);

    auto* actionConnections = monitorMenu_->addAction(tr("&Connections"), this, &ToolsMenu::onMonitorConnections);
    actionConnections->setStatusTip(tr("Monitor active connections"));

    auto* actionTransactions = monitorMenu_->addAction(tr("&Transactions"), this, &ToolsMenu::onMonitorTransactions);
    actionTransactions->setStatusTip(tr("Monitor active transactions"));

    auto* actionStatements = monitorMenu_->addAction(tr("&Active Statements"), this, &ToolsMenu::onMonitorStatements);
    actionStatements->setStatusTip(tr("Monitor running statements"));

    auto* actionLocks = monitorMenu_->addAction(tr("&Locks"), this, &ToolsMenu::onMonitorLocks);
    actionLocks->setStatusTip(tr("Monitor database locks"));

    auto* actionPerformance = monitorMenu_->addAction(tr("&Performance"), this, &ToolsMenu::onMonitorPerformance);
    actionPerformance->setStatusTip(tr("Monitor database performance"));

    menu_->addMenu(monitorMenu_);
}

void ToolsMenu::updateActionStates() {
    actionGenerateDdl_->setEnabled(hasSelection_);
    actionCompareSchemas_->setEnabled(connected_);
    actionCompareData_->setEnabled(connected_);
    actionMigration_->setEnabled(connected_);
    actionScheduler_->setEnabled(connected_);
    monitorMenu_->setEnabled(connected_);
}

void ToolsMenu::setConnected(bool connected) {
    connected_ = connected;
    updateActionStates();
}

void ToolsMenu::setHasSelection(bool hasSelection) {
    hasSelection_ = hasSelection;
    updateActionStates();
}

// Slot implementations
void ToolsMenu::onGenerateDdl() { emit generateDdlRequested(); }
void ToolsMenu::onCompareSchemas() { emit compareSchemasRequested(); }
void ToolsMenu::onCompareData() { emit compareDataRequested(); }
void ToolsMenu::onImportSql() { emit importSqlRequested(); }
void ToolsMenu::onImportCsv() { emit importCsvRequested(); }
void ToolsMenu::onImportJson() { emit importJsonRequested(); }
void ToolsMenu::onImportExcel() { emit importExcelRequested(); }
void ToolsMenu::onExportSql() { emit exportSqlRequested(); }
void ToolsMenu::onExportCsv() { emit exportCsvRequested(); }
void ToolsMenu::onExportJson() { emit exportJsonRequested(); }
void ToolsMenu::onExportExcel() { emit exportExcelRequested(); }
void ToolsMenu::onExportXml() { emit exportXmlRequested(); }
void ToolsMenu::onExportHtml() { emit exportHtmlRequested(); }
void ToolsMenu::onMigration() { emit migrationRequested(); }
void ToolsMenu::onScheduler() { emit schedulerRequested(); }
void ToolsMenu::onMonitorConnections() { emit monitorConnectionsRequested(); }
void ToolsMenu::onMonitorTransactions() { emit monitorTransactionsRequested(); }
void ToolsMenu::onMonitorStatements() { emit monitorStatementsRequested(); }
void ToolsMenu::onMonitorLocks() { emit monitorLocksRequested(); }
void ToolsMenu::onMonitorPerformance() { emit monitorPerformanceRequested(); }

} // namespace scratchrobin::ui
