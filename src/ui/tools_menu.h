#pragma once
#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
class QWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Tools menu implementation for database tools and utilities
 * 
 * Implements MENU_SPECIFICATION.md §Tools section:
 * - Generate DDL
 * - Compare Schemas/Data
 * - Import Data submenu (SQL, CSV, JSON, Excel)
 * - Export Data submenu (SQL, CSV, JSON, Excel, XML, HTML)
 * - Database Migration
 * - Job Scheduler
 * - Monitor submenu
 */
class ToolsMenu : public QObject {
    Q_OBJECT

public:
    explicit ToolsMenu(QWidget* parent = nullptr);
    ~ToolsMenu() override;

    QMenu* menu() const { return menu_; }

    // State management
    void setConnected(bool connected);
    void setHasSelection(bool hasSelection);

signals:
    // DDL
    void generateDdlRequested();
    void compareSchemasRequested();
    void compareDataRequested();
    
    // Import
    void importSqlRequested();
    void importCsvRequested();
    void importJsonRequested();
    void importExcelRequested();
    
    // Export
    void exportSqlRequested();
    void exportCsvRequested();
    void exportJsonRequested();
    void exportExcelRequested();
    void exportXmlRequested();
    void exportHtmlRequested();
    
    // Tools
    void migrationRequested();
    void schedulerRequested();
    
    // Monitor
    void monitorConnectionsRequested();
    void monitorTransactionsRequested();
    void monitorStatementsRequested();
    void monitorLocksRequested();
    void monitorPerformanceRequested();

private slots:
    void onGenerateDdl();
    void onCompareSchemas();
    void onCompareData();
    void onImportSql();
    void onImportCsv();
    void onImportJson();
    void onImportExcel();
    void onExportSql();
    void onExportCsv();
    void onExportJson();
    void onExportExcel();
    void onExportXml();
    void onExportHtml();
    void onMigration();
    void onScheduler();
    void onMonitorConnections();
    void onMonitorTransactions();
    void onMonitorStatements();
    void onMonitorLocks();
    void onMonitorPerformance();

private:
    void setupActions();
    void createImportSubmenu();
    void createExportSubmenu();
    void createMonitorSubmenu();
    void updateActionStates();

    QWidget* parent_;
    QMenu* menu_ = nullptr;
    QMenu* importMenu_ = nullptr;
    QMenu* exportMenu_ = nullptr;
    QMenu* monitorMenu_ = nullptr;

    // State
    bool connected_ = false;
    bool hasSelection_ = false;

    // Actions
    QAction* actionGenerateDdl_ = nullptr;
    QAction* actionCompareSchemas_ = nullptr;
    QAction* actionCompareData_ = nullptr;
    QAction* actionMigration_ = nullptr;
    QAction* actionScheduler_ = nullptr;
};

} // namespace scratchrobin::ui
