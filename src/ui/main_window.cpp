#include "ui/main_window.h"
#include "ui/project_navigator.h"
#include "ui/sql_editor.h"
#include "ui/connection_dialog.h"
#include "ui/data_grid.h"
#include "ui/csv_import_dialog.h"
#include "ui/preferences_dialog.h"

#ifdef SCRATCHROBIN_WITH_QXLSX
#include "xlsxdocument.h"
#include "xlsxworksheet.h"
#include "xlsxcellrange.h"
#include "xlsxformat.h"
#endif

// For Excel import dialog
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include "ui/preferences_dialog.h"
#include "ui/er_diagram_widget.h"
#include "ui/doc_generator_dialog.h"
#include "ui/toolbar_editor_dialog.h"
#include "ui/menu_editor_dialog.h"
#include "ui/theme_settings_dialog.h"
#include "ui/theme_manager.h"
#include "ui/query_menu.h"
#include "ui/transaction_menu.h"
#include "ui/window_menu.h"
#include "ui/tools_menu.h"
#include "ui/find_replace_dialog.h"
#include "ui/about_dialog.h"
#include "ui/shortcuts_dialog.h"
#include "ui/tip_of_day.h"
#include "ui/dock_workspace.h"
#include "ui/view_manager.h"
#include "ui/trigger_manager.h"
#include "ui/git_integration.h"
#include "ui/reporting_system.h"
#include "ui/dashboard_builder.h"
#include "ui/table_designer_dialog.h"
#include "ui/query_history.h"
#include "ui/schema_compare.h"
#include "ui/schema_migration_tool.h"
#include "ui/scheduled_jobs.h"
#include "ui/data_modeler.h"
#include "ui/query_builder.h"
#include "ui/query_plan_visualizer.h"
#include "ui/sql_debugger.h"
#include "ui/procedure_debugger.h"
#include "ui/sql_profiler.h"
#include "ui/code_formatter.h"
#include "ui/performance_dashboard.h"
#include "ui/slow_query_log_viewer.h"
#include "ui/database_health_check.h"
#include "ui/alert_notification_system.h"
#include "ui/user_management.h"
#include "ui/role_management.h"
#include "ui/permission_browser.h"
#include "ui/audit_log_viewer.h"
#include "ui/ssl_certificate_manager.h"
#include "ui/backup_restore.h"
#include "ui/scheduled_jobs.h"
#include "ui/vacuum_analyze_tool.h"
#include "ui/data_generator.h"
#include "ui/data_masking.h"
#include "ui/data_cleansing.h"
#include "ui/data_lineage.h"
#include "ui/team_sharing.h"
#include "ui/query_comments.h"
#include "ui/change_tracking.h"
#include "ui/plugin_system.h"
#include "ui/macro_recording.h"
#include "ui/scripting_console.h"
#include "ui/custom_keybindings.h"
#include "ui/monitoring_panels.h"
#include "backend/session_client.h"
#include "backend/query_response.h"
#include "backend/scratchbird_connection.h"
#include "core/window_state_manager.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QCloseEvent>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QProgressBar>
#include <QThread>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QXmlStreamWriter>
#include <QTextStream>

namespace scratchrobin::ui {

MainWindow::MainWindow(backend::SessionClient* session_client, QWidget* parent)
    : QMainWindow(parent)
    , session_client_(session_client)
    , db_connection_(std::make_unique<backend::ScratchbirdConnection>())
    , query_running_(false)
    , dock_workspace_(nullptr)
    , view_manager_panel_(nullptr)
    , trigger_manager_panel_(nullptr)
    , git_integration_panel_(nullptr)
    , report_manager_panel_(nullptr)
    , dashboard_manager_panel_(nullptr)
    , navigator_dock_(nullptr)
    , results_dock_(nullptr)
    , editor_tabs_(nullptr)
    , navigator_(nullptr)
    , results_grid_(nullptr)
    , status_label_(nullptr)
    , connection_label_(nullptr)
    , row_count_label_(nullptr) {
  setWindowTitle(tr("ScratchRobin - Database Administration Tool"));
  setMinimumSize(1024, 768);
  resize(1280, 800);
  
  setupUi();
  createMenus();
  createToolbars();
  createStatusBar();
  setupConnections();
  
  // Initialize async query executor
  async_executor_.Initialize(4);  // 4 worker threads
  
  // Initialize DockWorkspace after basic UI setup
  setupDockWorkspace();
  
  showStatusMessage(tr("Ready"));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
  // Central widget - SQL editor tabs
  auto* central = new QWidget(this);
  auto* central_layout = new QVBoxLayout(central);
  central_layout->setContentsMargins(0, 0, 0, 0);
  
  editor_tabs_ = new QTabWidget(central);
  editor_tabs_->setTabsClosable(true);
  editor_tabs_->setMovable(true);
  central_layout->addWidget(editor_tabs_);
  
  setCentralWidget(central);
  
  // Create dock windows for floating capability
  createDockWindows();
}

void MainWindow::createDockWindows() {
  // Navigator dock (left side)
  navigator_dock_ = new QDockWidget(tr("Project Navigator"), this);
  navigator_dock_->setObjectName("navigatorDock");
  navigator_dock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  navigator_dock_->setFeatures(QDockWidget::DockWidgetMovable | 
                                QDockWidget::DockWidgetFloatable |
                                QDockWidget::DockWidgetClosable);
  
  navigator_ = new ProjectNavigator(navigator_dock_);
  navigator_->setMinimumWidth(200);
  navigator_dock_->setWidget(navigator_);
  addDockWidget(Qt::LeftDockWidgetArea, navigator_dock_);
  
  // Results dock (bottom)
  results_dock_ = new QDockWidget(tr("Query Results"), this);
  results_dock_->setObjectName("resultsDock");
  results_dock_->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
  results_dock_->setFeatures(QDockWidget::DockWidgetMovable | 
                              QDockWidget::DockWidgetFloatable |
                              QDockWidget::DockWidgetClosable);
  
  results_grid_ = new DataGrid(results_dock_);
  results_grid_->setMinimumHeight(150);
  results_dock_->setWidget(results_grid_);
  addDockWidget(Qt::BottomDockWidgetArea, results_dock_);
}

void MainWindow::setupDockWorkspace() {
  // Create DockWorkspace for advanced panel management
  dock_workspace_ = new DockWorkspace(this, this);
  
  // Register existing panels with DockWorkspace
  DockPanelInfo navigator_info;
  navigator_info.id = "project_navigator";
  navigator_info.title = tr("Project Navigator");
  navigator_info.category = "core";
  navigator_info.defaultArea = DockAreaType::Left;
  navigator_info.allowFloating = true;
  navigator_info.allowClose = true;
  dock_workspace_->registerPanel(navigator_info, navigator_);
  
  DockPanelInfo results_info;
  results_info.id = "query_results";
  results_info.title = tr("Query Results");
  results_info.category = "core";
  results_info.defaultArea = DockAreaType::Bottom;
  results_info.allowFloating = true;
  results_info.allowClose = true;
  dock_workspace_->registerPanel(results_info, results_grid_);
  
  // Create and register new manager panels
  view_manager_panel_ = new ViewManagerPanel(session_client_, this);
  DockPanelInfo view_info;
  view_info.id = "view_manager";
  view_info.title = tr("View Manager");
  view_info.category = "database";
  view_info.defaultArea = DockAreaType::Left;
  view_info.allowFloating = true;
  view_info.allowClose = true;
  dock_workspace_->registerPanel(view_info, view_manager_panel_);
  
  trigger_manager_panel_ = new TriggerManagerPanel(session_client_, this);
  DockPanelInfo trigger_info;
  trigger_info.id = "trigger_manager";
  trigger_info.title = tr("Trigger Manager");
  trigger_info.category = "database";
  trigger_info.defaultArea = DockAreaType::Left;
  trigger_info.allowFloating = true;
  trigger_info.allowClose = true;
  dock_workspace_->registerPanel(trigger_info, trigger_manager_panel_);
  
  git_integration_panel_ = new GitIntegrationPanel(this);
  DockPanelInfo git_info;
  git_info.id = "git_integration";
  git_info.title = tr("Git Integration");
  git_info.category = "version";
  git_info.defaultArea = DockAreaType::Right;
  git_info.allowFloating = true;
  git_info.allowClose = true;
  dock_workspace_->registerPanel(git_info, git_integration_panel_);
  
  report_manager_panel_ = new ReportManagerPanel(session_client_, this);
  DockPanelInfo report_info;
  report_info.id = "report_manager";
  report_info.title = tr("Report Manager");
  report_info.category = "reporting";
  report_info.defaultArea = DockAreaType::Right;
  report_info.allowFloating = true;
  report_info.allowClose = true;
  dock_workspace_->registerPanel(report_info, report_manager_panel_);
  
  dashboard_manager_panel_ = new DashboardManagerPanel(session_client_, this);
  DockPanelInfo dashboard_info;
  dashboard_info.id = "dashboard_manager";
  dashboard_info.title = tr("Dashboard Manager");
  dashboard_info.category = "reporting";
  dashboard_info.defaultArea = DockAreaType::Right;
  dashboard_info.allowFloating = true;
  dashboard_info.allowClose = true;
  dock_workspace_->registerPanel(dashboard_info, dashboard_manager_panel_);
  
  // Connect dashboard manager signals
  connect(dashboard_manager_panel_, &DashboardManagerPanel::dashboardOpenRequested,
          this, &MainWindow::onDashboardOpenRequested);
  
  // Schema Compare panel
  auto* schema_compare_panel = new SchemaComparePanel(session_client_, this);
  DockPanelInfo schema_compare_info;
  schema_compare_info.id = "schema_compare";
  schema_compare_info.title = tr("Schema Compare");
  schema_compare_info.category = "database";
  schema_compare_info.defaultArea = DockAreaType::Right;
  schema_compare_info.allowFloating = true;
  schema_compare_info.allowClose = true;
  dock_workspace_->registerPanel(schema_compare_info, schema_compare_panel);
  
  // Schema Migration Tool panel
  auto* schema_migration_panel = new SchemaMigrationToolPanel(session_client_, this);
  DockPanelInfo schema_migration_info;
  schema_migration_info.id = "schema_migration";
  schema_migration_info.title = tr("Schema Migrations");
  schema_migration_info.category = "database";
  schema_migration_info.defaultArea = DockAreaType::Right;
  schema_migration_info.allowFloating = true;
  schema_migration_info.allowClose = true;
  dock_workspace_->registerPanel(schema_migration_info, schema_migration_panel);
  
  // Data Modeler panel
  auto* data_modeler_panel = new DataModelerPanel(session_client_, this);
  DockPanelInfo data_modeler_info;
  data_modeler_info.id = "data_modeler";
  data_modeler_info.title = tr("Data Modeler");
  data_modeler_info.category = "database";
  data_modeler_info.defaultArea = DockAreaType::Center;
  data_modeler_info.allowFloating = true;
  data_modeler_info.allowClose = true;
  dock_workspace_->registerPanel(data_modeler_info, data_modeler_panel);
  
  // ER Diagram Designer panel (enhanced version of existing ER diagram)
  auto* er_diagram_designer = new ERDiagramWidget(this);
  DockPanelInfo er_diagram_info;
  er_diagram_info.id = "er_diagram";
  er_diagram_info.title = tr("ER Diagram Designer");
  er_diagram_info.category = "database";
  er_diagram_info.defaultArea = DockAreaType::Center;
  er_diagram_info.allowFloating = true;
  er_diagram_info.allowClose = true;
  dock_workspace_->registerPanel(er_diagram_info, er_diagram_designer);
  
  // Query Builder panel
  auto* query_builder_panel = new QueryBuilderPanel(session_client_, this);
  DockPanelInfo query_builder_info;
  query_builder_info.id = "query_builder";
  query_builder_info.title = tr("Query Builder");
  query_builder_info.category = "query";
  query_builder_info.defaultArea = DockAreaType::Center;
  query_builder_info.allowFloating = true;
  query_builder_info.allowClose = true;
  dock_workspace_->registerPanel(query_builder_info, query_builder_panel);
  
  // Query Plan Visualizer panel
  auto* query_plan_panel = new QueryPlanVisualizerPanel(session_client_, this);
  DockPanelInfo query_plan_info;
  query_plan_info.id = "query_plan";
  query_plan_info.title = tr("Query Plan Visualizer");
  query_plan_info.category = "query";
  query_plan_info.defaultArea = DockAreaType::Right;
  query_plan_info.allowFloating = true;
  query_plan_info.allowClose = true;
  dock_workspace_->registerPanel(query_plan_info, query_plan_panel);
  
  // SQL Debugger panel
  auto* sql_debugger_panel = new SqlDebuggerPanel(session_client_, this);
  DockPanelInfo sql_debugger_info;
  sql_debugger_info.id = "sql_debugger";
  sql_debugger_info.title = tr("SQL Debugger");
  sql_debugger_info.category = "debug";
  sql_debugger_info.defaultArea = DockAreaType::Center;
  sql_debugger_info.allowFloating = true;
  sql_debugger_info.allowClose = true;
  dock_workspace_->registerPanel(sql_debugger_info, sql_debugger_panel);
  
  // Procedure Debugger panel
  auto* proc_debugger_panel = new ProcedureDebuggerPanel(session_client_, this);
  DockPanelInfo proc_debugger_info;
  proc_debugger_info.id = "procedure_debugger";
  proc_debugger_info.title = tr("Procedure Debugger");
  proc_debugger_info.category = "debug";
  proc_debugger_info.defaultArea = DockAreaType::Center;
  proc_debugger_info.allowFloating = true;
  proc_debugger_info.allowClose = true;
  dock_workspace_->registerPanel(proc_debugger_info, proc_debugger_panel);
  
  // SQL Profiler panel
  auto* sql_profiler_panel = new SqlProfilerPanel(session_client_, this);
  DockPanelInfo sql_profiler_info;
  sql_profiler_info.id = "sql_profiler";
  sql_profiler_info.title = tr("SQL Profiler");
  sql_profiler_info.category = "performance";
  sql_profiler_info.defaultArea = DockAreaType::Right;
  sql_profiler_info.allowFloating = true;
  sql_profiler_info.allowClose = true;
  dock_workspace_->registerPanel(sql_profiler_info, sql_profiler_panel);
  
  // Code Formatter panel
  auto* code_formatter_panel = new CodeFormatterPanel(session_client_, this);
  DockPanelInfo code_formatter_info;
  code_formatter_info.id = "code_formatter";
  code_formatter_info.title = tr("Code Formatter");
  code_formatter_info.category = "tools";
  code_formatter_info.defaultArea = DockAreaType::Center;
  code_formatter_info.allowFloating = true;
  code_formatter_info.allowClose = true;
  dock_workspace_->registerPanel(code_formatter_info, code_formatter_panel);
  
  // Performance Dashboard panel
  auto* perf_dashboard_panel = new PerformanceDashboardPanel(session_client_, this);
  DockPanelInfo perf_dashboard_info;
  perf_dashboard_info.id = "performance_dashboard";
  perf_dashboard_info.title = tr("Performance Dashboard");
  perf_dashboard_info.category = "performance";
  perf_dashboard_info.defaultArea = DockAreaType::Right;
  perf_dashboard_info.allowFloating = true;
  perf_dashboard_info.allowClose = true;
  dock_workspace_->registerPanel(perf_dashboard_info, perf_dashboard_panel);
  
  // Slow Query Log Viewer panel
  auto* slow_query_panel = new SlowQueryLogViewerPanel(session_client_, this);
  DockPanelInfo slow_query_info;
  slow_query_info.id = "slow_query_log";
  slow_query_info.title = tr("Slow Query Log");
  slow_query_info.category = "performance";
  slow_query_info.defaultArea = DockAreaType::Right;
  slow_query_info.allowFloating = true;
  slow_query_info.allowClose = true;
  dock_workspace_->registerPanel(slow_query_info, slow_query_panel);
  
  // Database Health Check panel
  auto* health_check_panel = new DatabaseHealthCheckPanel(session_client_, this);
  DockPanelInfo health_check_info;
  health_check_info.id = "health_check";
  health_check_info.title = tr("Health Check");
  health_check_info.category = "maintenance";
  health_check_info.defaultArea = DockAreaType::Right;
  health_check_info.allowFloating = true;
  health_check_info.allowClose = true;
  dock_workspace_->registerPanel(health_check_info, health_check_panel);
  
  // Alert Notification System panel
  auto* alert_panel = new AlertNotificationSystemPanel(session_client_, this);
  DockPanelInfo alert_info;
  alert_info.id = "alert_system";
  alert_info.title = tr("Alert System");
  alert_info.category = "monitoring";
  alert_info.defaultArea = DockAreaType::Right;
  alert_info.allowFloating = true;
  alert_info.allowClose = true;
  dock_workspace_->registerPanel(alert_info, alert_panel);
}

void MainWindow::createMenus() {
  // File menu
  file_menu_ = menuBar()->addMenu(tr("&File"));
  action_new_connection_ = file_menu_->addAction(tr("&New Connection..."), this, &MainWindow::onFileNewConnection, QKeySequence::New);
  action_open_sql_ = file_menu_->addAction(tr("&Open SQL Script..."), this, &MainWindow::onFileOpenSql, QKeySequence::Open);
  file_menu_->addSeparator();
  action_save_ = file_menu_->addAction(tr("&Save"), this, &MainWindow::onFileSave, QKeySequence::Save);
  file_menu_->addAction(tr("Save &As..."), this, &MainWindow::onFileSaveAs, QKeySequence::SaveAs);
  file_menu_->addSeparator();
  action_exit_ = file_menu_->addAction(tr("E&xit"), this, &MainWindow::onFileExit, QKeySequence::Quit);

  // Edit menu
  edit_menu_ = menuBar()->addMenu(tr("&Edit"));
  action_undo_ = edit_menu_->addAction(tr("&Undo"), this, &MainWindow::onEditUndo, QKeySequence::Undo);
  action_redo_ = edit_menu_->addAction(tr("&Redo"), this, &MainWindow::onEditRedo, QKeySequence::Redo);
  edit_menu_->addSeparator();
  action_cut_ = edit_menu_->addAction(tr("Cu&t"), this, &MainWindow::onEditCut, QKeySequence::Cut);
  action_copy_ = edit_menu_->addAction(tr("&Copy"), this, &MainWindow::onEditCopy, QKeySequence::Copy);
  action_paste_ = edit_menu_->addAction(tr("&Paste"), this, &MainWindow::onEditPaste, QKeySequence::Paste);
  edit_menu_->addSeparator();
  edit_menu_->addAction(tr("&Find..."), this, &MainWindow::onEditFind, QKeySequence::Find);
  edit_menu_->addAction(tr("Find and &Replace..."), this, &MainWindow::onEditFindReplace, QKeySequence::Replace);
  edit_menu_->addAction(tr("Find &Next"), this, &MainWindow::onEditFindNext, QKeySequence("F3"));
  edit_menu_->addAction(tr("Find &Previous"), this, &MainWindow::onEditFindPrevious, QKeySequence("Shift+F3"));
  edit_menu_->addSeparator();
  edit_menu_->addAction(tr("&Preferences..."), this, &MainWindow::onEditPreferences);

  // View menu
  view_menu_ = menuBar()->addMenu(tr("&View"));
  action_refresh_ = view_menu_->addAction(tr("&Refresh"), this, &MainWindow::onViewRefresh, QKeySequence::Refresh);
  view_menu_->addSeparator();
  
  // Database Object Managers
  auto* db_managers_menu = view_menu_->addMenu(tr("&Database Managers"));
  db_managers_menu->addAction(tr("&View Manager"), this, &MainWindow::onViewViewManager, QKeySequence("Ctrl+Shift+V"));
  db_managers_menu->addAction(tr("&Trigger Manager"), this, &MainWindow::onViewTriggerManager, QKeySequence("Ctrl+Shift+T"));
  db_managers_menu->addAction(tr("&Table Designer"), this, &MainWindow::onViewTableDesigner);
  view_menu_->addSeparator();
  
  // Version Control
  view_menu_->addAction(tr("&Git Integration"), this, &MainWindow::onViewGitIntegration, QKeySequence("Ctrl+Shift+G"));
  view_menu_->addSeparator();
  
  // Reporting and Analytics
  auto* reporting_menu = view_menu_->addMenu(tr("&Reporting"));
  reporting_menu->addAction(tr("&Report Manager"), this, &MainWindow::onViewReportManager);
  reporting_menu->addAction(tr("&Dashboard Manager"), this, &MainWindow::onViewDashboardManager);
  view_menu_->addSeparator();
  
  // Query Utilities
  auto* query_utils_menu = view_menu_->addMenu(tr("&Query Utilities"));
  query_utils_menu->addAction(tr("&SQL History"), this, &MainWindow::onViewSqlHistory);
  query_utils_menu->addAction(tr("Query &Favorites"), this, &MainWindow::onViewQueryFavorites);
  view_menu_->addSeparator();
  
  // Query menu (new)
  query_menu_obj_ = new QueryMenu(this);
  menuBar()->addMenu(query_menu_obj_->menu());
  connect(query_menu_obj_, &QueryMenu::executeRequested, this, &MainWindow::onQueryExecute);
  connect(query_menu_obj_, &QueryMenu::executeSelectionRequested, this, &MainWindow::onQueryExecuteSelection);
  connect(query_menu_obj_, &QueryMenu::executeScriptRequested, this, &MainWindow::onQueryExecuteScript);
  connect(query_menu_obj_, &QueryMenu::stopRequested, this, &MainWindow::onQueryStop);
  connect(query_menu_obj_, &QueryMenu::explainRequested, this, &MainWindow::onQueryExplain);
  connect(query_menu_obj_, &QueryMenu::explainAnalyzeRequested, this, &MainWindow::onQueryExplainAnalyze);
  connect(query_menu_obj_, &QueryMenu::formatSqlRequested, this, &MainWindow::onQueryFormatSql);
  connect(query_menu_obj_, &QueryMenu::commentRequested, this, &MainWindow::onQueryComment);
  connect(query_menu_obj_, &QueryMenu::uncommentRequested, this, &MainWindow::onQueryUncomment);
  connect(query_menu_obj_, &QueryMenu::toggleCommentRequested, this, &MainWindow::onQueryToggleComment);
  connect(query_menu_obj_, &QueryMenu::uppercaseRequested, this, &MainWindow::onQueryUppercase);
  connect(query_menu_obj_, &QueryMenu::lowercaseRequested, this, &MainWindow::onQueryLowercase);
  
  // Transaction menu (new)
  transaction_menu_obj_ = new TransactionMenu(this);
  menuBar()->addMenu(transaction_menu_obj_->menu());
  connect(transaction_menu_obj_, &TransactionMenu::startTransactionRequested, this, &MainWindow::onTransactionStart);
  connect(transaction_menu_obj_, &TransactionMenu::commitRequested, this, &MainWindow::onTransactionCommit);
  connect(transaction_menu_obj_, &TransactionMenu::rollbackRequested, this, &MainWindow::onTransactionRollback);
  connect(transaction_menu_obj_, &TransactionMenu::setSavepointRequested, this, &MainWindow::onTransactionSavepoint);
  connect(transaction_menu_obj_, &TransactionMenu::autoCommitToggled, this, &MainWindow::onTransactionAutoCommit);
  connect(transaction_menu_obj_, &TransactionMenu::isolationLevelChanged, this, &MainWindow::onTransactionIsolationLevel);
  connect(transaction_menu_obj_, &TransactionMenu::readOnlyToggled, this, &MainWindow::onTransactionReadOnly);

  // Database menu
  db_menu_ = menuBar()->addMenu(tr("&Database"));
  action_connect_ = db_menu_->addAction(tr("&Connect..."), this, &MainWindow::onDbConnect);
  action_disconnect_ = db_menu_->addAction(tr("&Disconnect"), this, &MainWindow::onDbDisconnect);
  db_menu_->addSeparator();
  action_backup_ = db_menu_->addAction(tr("&Backup..."), this, &MainWindow::onDbBackup);
  db_menu_->addAction(tr("&Restore..."), this, &MainWindow::onDbRestore);
  db_menu_->addSeparator();
  db_menu_->addAction(tr("E&xecute Query"), this, [this]() { executeCurrentEditor(); }, QKeySequence("F5"));

  // Tools menu (enhanced)
  tools_menu_ext_ = new ToolsMenu(this);
  menuBar()->addMenu(tools_menu_ext_->menu());
  connect(tools_menu_ext_, &ToolsMenu::generateDdlRequested, this, &MainWindow::onToolsGenerateDdl);
  connect(tools_menu_ext_, &ToolsMenu::compareSchemasRequested, this, &MainWindow::onToolsCompareSchemas);
  connect(tools_menu_ext_, &ToolsMenu::compareDataRequested, this, &MainWindow::onToolsCompareData);
  connect(tools_menu_ext_, &ToolsMenu::importSqlRequested, this, &MainWindow::onToolsImportSql);
  connect(tools_menu_ext_, &ToolsMenu::importCsvRequested, this, &MainWindow::onToolsImportCsv);
  connect(tools_menu_ext_, &ToolsMenu::importJsonRequested, this, &MainWindow::onToolsImportJson);
  connect(tools_menu_ext_, &ToolsMenu::importExcelRequested, this, &MainWindow::onToolsImportExcel);
  connect(tools_menu_ext_, &ToolsMenu::exportSqlRequested, this, &MainWindow::onToolsExportSql);
  connect(tools_menu_ext_, &ToolsMenu::exportCsvRequested, this, &MainWindow::onToolsExportCsv);
  connect(tools_menu_ext_, &ToolsMenu::exportJsonRequested, this, &MainWindow::onToolsExportJson);
  connect(tools_menu_ext_, &ToolsMenu::exportExcelRequested, this, &MainWindow::onToolsExportExcel);
  connect(tools_menu_ext_, &ToolsMenu::exportXmlRequested, this, &MainWindow::onToolsExportXml);
  connect(tools_menu_ext_, &ToolsMenu::exportHtmlRequested, this, &MainWindow::onToolsExportHtml);
  connect(tools_menu_ext_, &ToolsMenu::migrationRequested, this, &MainWindow::onToolsMigration);
  connect(tools_menu_ext_, &ToolsMenu::schedulerRequested, this, &MainWindow::onToolsScheduler);
  connect(tools_menu_ext_, &ToolsMenu::monitorConnectionsRequested, this, &MainWindow::onToolsMonitorConnections);
  connect(tools_menu_ext_, &ToolsMenu::monitorTransactionsRequested, this, &MainWindow::onToolsMonitorTransactions);
  connect(tools_menu_ext_, &ToolsMenu::monitorStatementsRequested, this, &MainWindow::onToolsMonitorStatements);
  connect(tools_menu_ext_, &ToolsMenu::monitorLocksRequested, this, &MainWindow::onToolsMonitorLocks);
  connect(tools_menu_ext_, &ToolsMenu::monitorPerformanceRequested, this, &MainWindow::onToolsMonitorPerformance);
  
  // Legacy Tools menu (keeping for compatibility)
  tools_menu_ = menuBar()->addMenu(tr("&Legacy Tools"));
  tools_menu_->addAction(tr("&Migration Wizard..."), this, &MainWindow::onToolsMigrationWizard);
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("&Schema Compare..."), this, &MainWindow::onToolsSchemaCompare, QKeySequence("Ctrl+Shift+C"));
  tools_menu_->addAction(tr("Schema &Migration Tool..."), this, &MainWindow::onToolsSchemaMigration);
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("&ER Diagram..."), this, &MainWindow::onToolsERDiagram);
  tools_menu_->addAction(tr("ER &Diagram Designer..."), this, &MainWindow::onToolsERDiagramDesigner);
  tools_menu_->addAction(tr("&Data Modeler..."), this, &MainWindow::onToolsDataModeler);
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("&Query Builder..."), this, &MainWindow::onToolsQueryBuilder, QKeySequence("Ctrl+Shift+B"));
  tools_menu_->addAction(tr("Query &Plan Visualizer..."), this, &MainWindow::onToolsQueryPlanVisualizer);
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("SQL &Debugger..."), this, &MainWindow::onToolsSqlDebugger);
  tools_menu_->addAction(tr("&Procedure Debugger..."), this, &MainWindow::onToolsProcedureDebugger);
  tools_menu_->addAction(tr("SQL &Profiler..."), this, &MainWindow::onToolsSqlProfiler);
  tools_menu_->addAction(tr("&Code Formatter..."), this, &MainWindow::onToolsCodeFormatter);
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("Performance &Dashboard..."), this, &MainWindow::onToolsPerformanceDashboard);
  tools_menu_->addAction(tr("&Slow Query Log..."), this, &MainWindow::onToolsSlowQueryLog);
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("Database &Health Check..."), this, &MainWindow::onToolsHealthCheck);
  tools_menu_->addAction(tr("&Alert System..."), this, &MainWindow::onToolsAlertSystem);
  tools_menu_->addSeparator();
  
  // Security management submenu
  auto* security_menu = tools_menu_->addMenu(tr("&Security"));
  security_menu->addAction(tr("&User Management..."), this, &MainWindow::onToolsUserManagement, QKeySequence("Ctrl+Shift+U"));
  security_menu->addAction(tr("&Role Management..."), this, &MainWindow::onToolsRoleManagement, QKeySequence("Ctrl+Shift+R"));
  security_menu->addAction(tr("&Permission Browser..."), this, &MainWindow::onToolsPermissionBrowser, QKeySequence("Ctrl+Shift+P"));
  security_menu->addAction(tr("&Audit Log Viewer..."), this, &MainWindow::onToolsAuditLogViewer, QKeySequence("Ctrl+Shift+A"));
  security_menu->addAction(tr("&SSL Certificate Manager..."), this, &MainWindow::onToolsSSLCertificateManager, QKeySequence("Ctrl+Shift+S"));
  
  // Maintenance submenu
  auto* maintenance_menu = tools_menu_->addMenu(tr("&Maintenance"));
  maintenance_menu->addAction(tr("&Backup & Restore..."), this, &MainWindow::onToolsBackupManager, QKeySequence("Ctrl+Shift+B"));
  maintenance_menu->addAction(tr("&Scheduled Jobs..."), this, &MainWindow::onToolsScheduledJobs, QKeySequence("Ctrl+Shift+J"));
  maintenance_menu->addAction(tr("&Vacuum/Analyze..."), this, &MainWindow::onToolsVacuumAnalyze, QKeySequence("Ctrl+Shift+V"));
  
  // Data Management submenu
  auto* data_menu = tools_menu_->addMenu(tr("&Data Management"));
  data_menu->addAction(tr("&Data Generator..."), this, &MainWindow::onToolsDataGenerator, QKeySequence("Ctrl+Shift+G"));
  data_menu->addAction(tr("Data &Masking..."), this, &MainWindow::onToolsDataMasking, QKeySequence("Ctrl+Shift+M"));
  data_menu->addAction(tr("Data &Cleansing..."), this, &MainWindow::onToolsDataCleansing, QKeySequence("Ctrl+Shift+C"));
  data_menu->addAction(tr("Data &Lineage..."), this, &MainWindow::onToolsDataLineage, QKeySequence("Ctrl+Shift+L"));
  
  // Collaboration submenu
  auto* collab_menu = tools_menu_->addMenu(tr("&Collaboration"));
  collab_menu->addAction(tr("&Team Sharing..."), this, &MainWindow::onToolsTeamSharing, QKeySequence("Ctrl+Shift+T"));
  collab_menu->addAction(tr("&Query Comments..."), this, &MainWindow::onToolsQueryComments, QKeySequence("Ctrl+Shift+Q"));
  collab_menu->addAction(tr("&Change Tracking..."), this, &MainWindow::onToolsChangeTracking, QKeySequence("Ctrl+Shift+H"));
  
  // Extensions submenu
  auto* ext_menu = tools_menu_->addMenu(tr("E&xtensions"));
  ext_menu->addAction(tr("&Plugin Manager..."), this, &MainWindow::onToolsPluginManager, QKeySequence("Ctrl+Shift+P"));
  ext_menu->addAction(tr("&Macro Recording..."), this, &MainWindow::onToolsMacroRecording, QKeySequence("Ctrl+Shift+R"));
  ext_menu->addAction(tr("&Scripting Console..."), this, &MainWindow::onToolsScriptingConsole, QKeySequence("Ctrl+Shift+J"));
  
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("&Generate Documentation..."), this, &MainWindow::onToolsDocGenerator);
  tools_menu_->addSeparator();
  action_toolbar_editor_ = tools_menu_->addAction(tr("Customize &Toolbars..."), this, &MainWindow::onToolsToolbarEditor);
  action_menu_editor_ = tools_menu_->addAction(tr("Customize &Menus..."), this, &MainWindow::onToolsMenuEditor);
  tools_menu_->addSeparator();
  tools_menu_->addAction(tr("&Theme Settings..."), this, &MainWindow::onToolsThemeSettings);

  // Window menu (enhanced)
  window_menu_ = menuBar()->addMenu(tr("&Window"));
  action_navigator_ = window_menu_->addAction(tr("Project Navigator"), this, &MainWindow::onViewNavigator);
  action_navigator_->setCheckable(true);
  action_navigator_->setChecked(true);
  
  action_results_ = window_menu_->addAction(tr("Query Results"), this, &MainWindow::onViewResults);
  action_results_->setCheckable(true);
  action_results_->setChecked(true);
  
  window_menu_->addSeparator();
  window_menu_->addAction(tr("Close All Tabs"), this, [this]() {
    while (editor_tabs_->count() > 0) {
      editor_tabs_->removeTab(0);
    }
  });
  
  window_menu_->addSeparator();
  window_menu_->addAction(tr("&Save Layout"), this, &MainWindow::onWindowSaveLayout);
  window_menu_->addAction(tr("&Load Layout"), this, &MainWindow::onWindowLoadLayout);
  window_menu_->addAction(tr("&Reset Layout"), this, &MainWindow::onWindowResetLayout);

  // Help menu
  help_menu_ = menuBar()->addMenu(tr("&Help"));
  help_menu_->addAction(tr("&Documentation"), this, []() { QDesktopServices::openUrl(QUrl("https://scratchbird.dev/docs")); });
  help_menu_->addAction(tr("&Context Help"), this, [this]() { 
    // Show context-sensitive help based on current focus
    QString helpTopic;
    QWidget* focusWidget = QApplication::focusWidget();
    if (focusWidget) {
      QString className = focusWidget->metaObject()->className();
      if (className.contains("SqlEditor")) {
        helpTopic = tr("sql_editor");
      } else if (className.contains("DataGrid")) {
        helpTopic = tr("data_grid");
      } else if (className.contains("ProjectNavigator")) {
        helpTopic = tr("project_navigator");
      } else {
        helpTopic = tr("general");
      }
    }
    QDesktopServices::openUrl(QUrl(QString("https://scratchbird.dev/docs/%1").arg(helpTopic)));
  }, QKeySequence("Shift+F1"));
  help_menu_->addSeparator();
  help_menu_->addAction(tr("&Keyboard Shortcuts"), this, &MainWindow::onHelpShortcuts);
  help_menu_->addAction(tr("&Tip of the Day"), this, &MainWindow::onHelpTipOfDay);
  help_menu_->addSeparator();
  help_menu_->addAction(tr("&About ScratchRobin"), this, &MainWindow::onHelpAbout);
}

void MainWindow::createToolbars() {
  // Main toolbar
  main_toolbar_ = addToolBar(tr("Main"));
  main_toolbar_->setObjectName("mainToolbar");
  main_toolbar_->addAction(action_new_connection_);
  main_toolbar_->addAction(action_open_sql_);
  main_toolbar_->addSeparator();
  main_toolbar_->addAction(action_save_);
  main_toolbar_->addSeparator();
  main_toolbar_->addAction(action_undo_);
  main_toolbar_->addAction(action_redo_);
  addToolBar(Qt::TopToolBarArea, main_toolbar_);

  // Database toolbar
  db_toolbar_ = addToolBar(tr("Database"));
  db_toolbar_->setObjectName("dbToolbar");
  db_toolbar_->addAction(action_connect_);
  db_toolbar_->addAction(action_disconnect_);
  db_toolbar_->addSeparator();
  db_toolbar_->addAction(action_backup_);
  db_toolbar_->addSeparator();
  
  // Execute button
  auto* execute_action = db_toolbar_->addAction(tr("Execute"), this, [this]() { executeCurrentEditor(); });
  execute_action->setShortcut(QKeySequence("F5"));
  execute_action->setToolTip(tr("Execute Query (F5)"));
  
  db_toolbar_->addAction(action_refresh_);
  addToolBar(Qt::TopToolBarArea, db_toolbar_);
}

void MainWindow::createStatusBar() {
  // Connection status
  connection_label_ = new QLabel(tr("Disconnected"), this);
  connection_label_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addPermanentWidget(connection_label_);
  
  // Row count
  row_count_label_ = new QLabel(tr("0 rows"), this);
  row_count_label_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addPermanentWidget(row_count_label_);
  
  // Status message
  status_label_ = new QLabel(tr("Ready"), this);
  statusBar()->addWidget(status_label_);
}

void MainWindow::setupConnections() {
  // Tab close
  connect(editor_tabs_, &QTabWidget::tabCloseRequested, [this](int index) {
    editor_tabs_->removeTab(index);
  });
  
  // Dock visibility sync
  connect(navigator_dock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
    action_navigator_->setChecked(visible);
  });
  connect(results_dock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
    action_results_->setChecked(visible);
  });
  
  // Project Navigator connections
  connect(navigator_, &ProjectNavigator::serverSelected, this, [this](const QString& server) {
    showStatusMessage(tr("Server selected: %1").arg(server), 2000);
  });
  connect(navigator_, &ProjectNavigator::databaseSelected, this, [this](const QString& server, const QString& db) {
    showStatusMessage(tr("Database selected: %1.%2").arg(server, db), 2000);
  });
  connect(navigator_, &ProjectNavigator::tableDoubleClicked, this, [this](const QString& server, const QString& db, const QString& table) {
    Q_UNUSED(server)
    createSqlEditorTab(tr("Query %1").arg(table));
    if (auto* editor = qobject_cast<SqlEditor*>(editor_tabs_->currentWidget())) {
      editor->setText(QString("SELECT * FROM %1.%2;").arg(db, table));
      QTimer::singleShot(100, this, [this]() { executeCurrentEditor(); });
    }
  });
  connect(navigator_, &ProjectNavigator::queryTableRequested, this, [this](const QString& server, const QString& db, const QString& table) {
    Q_UNUSED(server)
    createSqlEditorTab(tr("Query %1").arg(table));
    if (auto* editor = qobject_cast<SqlEditor*>(editor_tabs_->currentWidget())) {
      editor->setText(QString("SELECT * FROM %1.%2;").arg(db, table));
    }
  });
  connect(navigator_, &ProjectNavigator::copyNameRequested, this, [this](const QString& name) {
    showStatusMessage(tr("Copied: %1").arg(name), 2000);
  });
  
  // Results grid connections
  connect(results_grid_, &DataGrid::rowDoubleClicked, this, [this](int row) {
    showStatusMessage(tr("Row %1 selected").arg(row + 1), 2000);
  });
  
  // Load saved window layout (delayed to ensure docks are fully created)
  QTimer::singleShot(0, this, [this]() {
    core::WindowStateManager::instance().loadWindowLayout(this, "default");
    
    // Apply saved theme
    ThemeManager::instance().loadThemeSettings();
    ThemeManager::instance().applyTheme();
  });
}

void MainWindow::closeEvent(QCloseEvent* event) {
  // Save window layout
  core::WindowStateManager::instance().saveWindowLayout(this, "default");
  event->accept();
}

// View actions
void MainWindow::onViewNavigator() {
  navigator_dock_->setVisible(action_navigator_->isChecked());
}

void MainWindow::onViewResults() {
  results_dock_->setVisible(action_results_->isChecked());
}

void MainWindow::onViewEditor() {
  // Always visible as central widget
}

// File menu
void MainWindow::onFileNewConnection() {
  ConnectionDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    auto config = dialog.connectionInfo();
    
    // Convert to backend ConnectionInfo
    backend::ConnectionInfo conn_info;
    conn_info.host = config.host.toStdString();
    conn_info.port = config.port;
    conn_info.database = config.database.toStdString();
    conn_info.username = config.username.toStdString();
    conn_info.password = config.password.toStdString();
    
    showStatusMessage(tr("Connecting to %1...").arg(config.host), 0);
    
    if (db_connection_->connect(conn_info)) {
      connection_label_->setText(tr("Connected: %1").arg(config.host));
      showStatusMessage(tr("Connected to %1").arg(config.host), 3000);
      
      // Add to navigator
      navigator_->addServer(config.name, QString("%1:%2").arg(config.host).arg(config.port));
      navigator_->addDatabase(config.name, config.database);
      
      // Load tables
      auto tables_result = db_connection_->getTables(config.database.toStdString());
      if (tables_result.success) {
        for (const auto& row : tables_result.rows) {
          if (!row.empty()) {
            navigator_->addTable(config.name, config.database, QString::fromStdString(row[0]));
          }
        }
      }
    } else {
      showError(tr("Failed to connect: %1").arg(QString::fromStdString(db_connection_->lastError())));
    }
  }
}

void MainWindow::onFileOpenSql() {
  QString filename = QFileDialog::getOpenFileName(this, tr("Open SQL Script"), 
                                                   QString(), 
                                                   tr("SQL Files (*.sql);;All Files (*)"));
  if (!filename.isEmpty()) {
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString content = QString::fromUtf8(file.readAll());
      file.close();
      
      createSqlEditorTab(QFileInfo(filename).fileName());
      if (auto* editor = qobject_cast<SqlEditor*>(editor_tabs_->currentWidget())) {
        editor->setText(content);
      }
      showStatusMessage(tr("Opened %1").arg(filename), 3000);
    }
  }
}

void MainWindow::onFileSave() {
  // Save current editor content
  if (auto* editor = currentEditor()) {
    QString filename = editor->property("filename").toString();
    if (filename.isEmpty()) {
      onFileSaveAs();
      return;
    }
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream stream(&file);
      stream << editor->toPlainText();
      file.close();
      showStatusMessage(tr("Saved %1").arg(filename), 3000);
      updateWindowTitle(filename);
    } else {
      QMessageBox::warning(this, tr("Save Error"), 
        tr("Could not save file: %1").arg(filename));
    }
  } else {
    showStatusMessage(tr("No editor to save"), 2000);
  }
}

void MainWindow::onFileSaveAs() {
  QString filename = QFileDialog::getSaveFileName(this, tr("Save SQL Script"),
                                                   QString(),
                                                   tr("SQL Files (*.sql);;All Files (*)"));
  if (!filename.isEmpty()) {
    onFileSave();
  }
}

void MainWindow::onFileExit() {
  close();
}

// Edit menu
void MainWindow::onEditUndo() { 
  if (auto* editor = currentEditor()) {
    editor->undo();
  }
}
void MainWindow::onEditRedo() { 
  if (auto* editor = currentEditor()) {
    editor->redo();
  }
}
void MainWindow::onEditCut() { 
  if (auto* editor = currentEditor()) {
    editor->cut();
  }
}
void MainWindow::onEditCopy() { 
  if (auto* editor = currentEditor()) {
    editor->copy();
  }
}
void MainWindow::onEditPaste() { 
  if (auto* editor = currentEditor()) {
    editor->paste();
  }
}

void MainWindow::onEditPreferences() {
  PreferencesDialog dialog(this);
  connect(&dialog, &PreferencesDialog::preferencesChanged, this, [this](const Preferences& prefs) {
    // Apply preferences to application
    applyPreferences(prefs);
    showStatusMessage(tr("Preferences saved"), 2000);
  });
  dialog.exec();
}

void MainWindow::applyPreferences(const Preferences& prefs) {
  // Apply font settings
  QFont editorFont(prefs.editor_font, prefs.editor_font_size);
  if (auto* editor = currentEditor()) {
    editor->setFont(editorFont);
  }
  
  // Apply SQL editor settings
  // These would be stored and applied to all SQL editors
  
  // Apply connection settings
  // Update auto-connect behavior
  
  // Apply logging settings
  // Update log level if logging system exists
  
  Q_UNUSED(prefs)
}

// View menu
void MainWindow::onViewRefresh() { 
  navigator_->refresh();
  showStatusMessage(tr("Refreshed"), 2000);
}

// Database menu
void MainWindow::onDbConnect() { 
  onFileNewConnection();
}
void MainWindow::onDbDisconnect() { 
  db_connection_->disconnect();
  connection_label_->setText(tr("Disconnected"));
  showStatusMessage(tr("Disconnected"), 2000);
}
void MainWindow::onDbBackup() { 
  showStatusMessage(tr("Backup..."), 2000);
}
void MainWindow::onDbRestore() { 
  showStatusMessage(tr("Restore..."), 2000);
}

// Tools menu
void MainWindow::onToolsMigrationWizard() { 
  showStatusMessage(tr("Migration Wizard..."), 2000);
}

void MainWindow::onToolsERDiagram() {
  auto* dock = new QDockWidget(tr("ER Diagram"), this);
  dock->setObjectName("erDiagramDock");
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
  dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  
  auto* erWidget = new ERDiagramWidget(dock);
  dock->setWidget(erWidget);
  addDockWidget(Qt::RightDockWidgetArea, dock);
  
  // Add to window menu
  auto* action = window_menu_->addAction(tr("ER Diagram"), this, [dock]() {
    dock->setVisible(!dock->isVisible());
  });
  action->setCheckable(true);
  action->setChecked(true);
  connect(dock, &QDockWidget::visibilityChanged, action, &QAction::setChecked);
  
  showStatusMessage(tr("ER Diagram opened"), 2000);
}

void MainWindow::onToolsDocGenerator() {
  DocGeneratorDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    showStatusMessage(tr("Documentation generated"), 3000);
  }
}

void MainWindow::onToolsToolbarEditor() {
  ToolbarEditorDialog dialog(this);
  dialog.loadToolbars(findChildren<QToolBar*>());
  if (dialog.exec() == QDialog::Accepted) {
    showStatusMessage(tr("Toolbars customized"), 2000);
  }
}

void MainWindow::onToolsMenuEditor() {
  MenuEditorDialog dialog(this);
  dialog.setMenuBar(menuBar());
  if (dialog.exec() == QDialog::Accepted) {
    // Menu configuration applied
    showStatusMessage(tr("Menus customized"), 2000);
  }
}

void MainWindow::onToolsThemeSettings() {
  ThemeSettingsDialog dialog(this);
  connect(&dialog, &ThemeSettingsDialog::previewThemeRequested, this, [](ThemeType type) {
    ThemeManager::instance().setTheme(type);
  });
  connect(&dialog, &ThemeSettingsDialog::settingsApplied, this, [this]() {
    ThemeManager::instance().saveThemeSettings();
    showStatusMessage(tr("Theme settings applied"), 2000);
  });
  if (dialog.exec() == QDialog::Accepted) {
    showStatusMessage(tr("Theme settings saved"), 2000);
  }
}

// Window layout actions
void MainWindow::onWindowSaveLayout() {
  core::WindowStateManager::instance().saveWindowLayout(this, "default");
  showStatusMessage(tr("Layout saved"), 2000);
}

void MainWindow::onWindowLoadLayout() {
  if (core::WindowStateManager::instance().loadWindowLayout(this, "default")) {
    showStatusMessage(tr("Layout loaded"), 2000);
  } else {
    showStatusMessage(tr("No saved layout found"), 2000);
  }
}

void MainWindow::onWindowResetLayout() {
  core::WindowStateManager::instance().resetLayout("default");
  
  // Reset docks to default positions
  removeDockWidget(navigator_dock_);
  removeDockWidget(results_dock_);
  addDockWidget(Qt::LeftDockWidgetArea, navigator_dock_);
  addDockWidget(Qt::BottomDockWidgetArea, results_dock_);
  
  navigator_dock_->setVisible(true);
  results_dock_->setVisible(true);
  action_navigator_->setChecked(true);
  action_results_->setChecked(true);
  
  showStatusMessage(tr("Layout reset to defaults"), 2000);
}

void MainWindow::onWindowManageLayouts() {
  // Simple layout management - save/restore layouts
  QStringList layouts = {"Default", "Query Focus", "Data Focus", "ER Diagram"};
  
  bool ok;
  QString choice = QInputDialog::getItem(this, tr("Manage Layouts"),
                                         tr("Select layout to apply or save current as:"),
                                         layouts, 0, true, &ok);
  
  if (ok) {
    if (choice == "Default") {
      onWindowResetLayout();
    } else if (choice == "Query Focus") {
      // Hide navigator, maximize editor
      navigator_dock_->hide();
      action_navigator_->setChecked(false);
      resizeDocks({navigator_dock_, results_dock_}, {0, 300}, Qt::Vertical);
      showStatusMessage(tr("Applied 'Query Focus' layout"), 2000);
    } else if (choice == "Data Focus") {
      // Show results larger
      resizeDocks({navigator_dock_, results_dock_}, {200, 400}, Qt::Vertical);
      showStatusMessage(tr("Applied 'Data Focus' layout"), 2000);
    } else {
      showStatusMessage(tr("Layout '%1' would be saved/loaded").arg(choice), 2000);
    }
  }
}

// Edit menu - Find/Replace
void MainWindow::onEditFind() {
  // Show find/replace dialog with find tab focused
  auto* dialog = new FindReplaceDialog(this);
  if (auto* editor = currentEditor()) {
    dialog->setTextEdit(editor);
    QString selected = editor->selectedSql();
    if (!selected.isEmpty() && selected.length() < 100) {
      dialog->setFindText(selected);
    }
  }
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();  // Non-modal
}

void MainWindow::onEditFindReplace() {
  onEditFind();  // Same dialog, just different focus
}

void MainWindow::onEditFindNext() {
  if (auto* editor = currentEditor()) {
    // Use QTextEdit's built-in find functionality
    // This requires the FindReplaceDialog to have been opened first
    // to set the search text, but we'll implement a simple version
    showStatusMessage(tr("Use Ctrl+F to open Find dialog"), 2000);
  }
}

void MainWindow::onEditFindPrevious() {
  if (auto* editor = currentEditor()) {
    showStatusMessage(tr("Use Ctrl+F to open Find dialog"), 2000);
  }
}

void MainWindow::onHelpShortcuts() {
  ShortcutsDialog dialog(this);
  dialog.exec();
}

void MainWindow::onHelpTipOfDay() {
  TipOfDayDialog dialog(this);
  dialog.exec();
}

void MainWindow::onHelpAbout() {
  AboutDialog dialog(this);
  dialog.exec();
}

// Helper functions
SqlEditor* MainWindow::currentEditor() const {
  return qobject_cast<SqlEditor*>(editor_tabs_->currentWidget());
}

void MainWindow::createSqlEditorTab(const QString& title) {
  SqlEditor* editor = new SqlEditor(session_client_, this);
  QString tabTitle = title.isEmpty() ? tr("Query %1").arg(editor_tabs_->count() + 1) : title;
  int index = editor_tabs_->addTab(editor, tabTitle);
  editor_tabs_->setCurrentIndex(index);
  editor->setFocus();
  
  // Connect execute signal
  connect(editor, &SqlEditor::executeRequested, this, [this, editor]() {
    QString sql = editor->selectedSql();
    if (!sql.isEmpty()) {
      executeSql(sql);
    }
  });
}

void MainWindow::executeCurrentEditor() {
  if (auto* editor = currentEditor()) {
    QString sql = editor->selectedSql();
    if (!sql.isEmpty()) {
      executeSql(sql);
    }
  }
}

void MainWindow::executeSql(const QString& sql) {
  showStatusMessage(tr("Executing..."), 0);
  
  if (!db_connection_->isConnected()) {
    showError(tr("Not connected to database. Please connect first."));
    return;
  }
  
  // Execute using ScratchbirdConnection
  auto result = db_connection_->execute(sql.toStdString());
  
  if (!result.success) {
    showError(QString::fromStdString(result.error_message));
    return;
  }
  
  // Convert to Qt types
  QStringList headers;
  for (const auto& col : result.columns) {
    headers.append(QString::fromStdString(col.name));
  }
  
  QList<QStringList> data;
  for (const auto& row : result.rows) {
    QStringList rowData;
    for (const auto& cell : row) {
      rowData.append(QString::fromStdString(cell));
    }
    data.append(rowData);
  }
  
  showResults(data, headers);
  showStatusMessage(tr("Query executed: %1 rows returned").arg(data.size()), 3000);
}

void MainWindow::showResults(const QList<QStringList>& data, const QStringList& headers) {
  results_grid_->setData(data, headers);
  row_count_label_->setText(tr("%1 rows").arg(data.size()));
  
  // Show results dock if hidden
  if (!results_dock_->isVisible()) {
    results_dock_->show();
    action_results_->setChecked(true);
  }
}

void MainWindow::showError(const QString& message) {
  QMessageBox::critical(this, tr("Error"), message);
  showStatusMessage(tr("Error: %1").arg(message), 5000);
}

void MainWindow::showStatusMessage(const QString& message, int timeout) {
  status_label_->setText(message);
  if (timeout > 0) {
    statusBar()->showMessage(message, timeout);
  } else {
    statusBar()->showMessage(message);
  }
}

// Query menu slots
void MainWindow::onQueryExecute() {
  executeCurrentEditor();
}

void MainWindow::onQueryExecuteSelection() {
  if (auto* editor = currentEditor()) {
    QString sql = editor->selectedSql();
    if (!sql.isEmpty()) {
      executeSql(sql);
    }
  }
}

void MainWindow::onQueryExecuteScript() {
  if (auto* editor = currentEditor()) {
    QString sql = editor->toPlainText();
    if (sql.isEmpty()) {
      showStatusMessage(tr("No SQL to execute"), 2000);
      return;
    }
    
    // Split script into individual statements (basic splitting by semicolon)
    QStringList statements = sql.split(';', Qt::SkipEmptyParts);
    int executed = 0;
    int failed = 0;
    
    for (QString& stmt : statements) {
      stmt = stmt.trimmed();
      if (stmt.isEmpty()) continue;
      
      auto result = db_connection_->execute(stmt.toStdString() + ";");
      if (result.success) {
        executed++;
      } else {
        failed++;
        showError(tr("Statement failed: %1\nError: %2")
                  .arg(stmt.left(50))
                  .arg(QString::fromStdString(result.error_message)));
        break;  // Stop on first error
      }
    }
    
    showStatusMessage(tr("Script executed: %1 succeeded, %2 failed").arg(executed).arg(failed), 5000);
  }
}

void MainWindow::onQueryStop() {
  if (!current_query_task_id_.empty()) {
    if (async_executor_.CancelQuery(current_query_task_id_)) {
      showStatusMessage(tr("Query cancellation requested"), 2000);
    } else {
      showStatusMessage(tr("Could not cancel query (may already be complete)"), 2000);
    }
  } else {
    showStatusMessage(tr("No query running"), 2000);
  }
}

void MainWindow::onQueryExplain() {
  if (auto* editor = currentEditor()) {
    QString sql = editor->selectedSql();
    if (sql.isEmpty()) {
      sql = editor->toPlainText();
    }
    
    if (sql.trimmed().isEmpty()) {
      showStatusMessage(tr("No SQL to explain"), 2000);
      return;
    }
    
    // Prepend EXPLAIN to the query
    QString explainSql = "EXPLAIN " + sql;
    executeSql(explainSql);
  }
}

void MainWindow::onQueryExplainAnalyze() {
  if (auto* editor = currentEditor()) {
    QString sql = editor->selectedSql();
    if (sql.isEmpty()) {
      sql = editor->toPlainText();
    }
    
    if (sql.trimmed().isEmpty()) {
      showStatusMessage(tr("No SQL to explain"), 2000);
      return;
    }
    
    // Prepend EXPLAIN ANALYZE to the query
    QString explainSql = "EXPLAIN ANALYZE " + sql;
    executeSql(explainSql);
  }
}

void MainWindow::onQueryFormatSql() {
  if (auto* editor = currentEditor()) {
    QString sql = editor->textCursor().selectedText();
    bool hasSelection = !sql.isEmpty();
    if (!hasSelection) {
      sql = editor->toPlainText();
    }
    
    if (sql.trimmed().isEmpty()) {
      showStatusMessage(tr("No SQL to format"), 2000);
      return;
    }
    
    // Basic SQL formatting
    QString formatted = sql;
    
    // Capitalize keywords
    QStringList keywords = {
      "select", "from", "where", "and", "or", "not", "order", "by", "group",
      "having", "join", "inner", "outer", "left", "right", "full", "on",
      "insert", "into", "values", "update", "set", "delete", "create",
      "table", "alter", "drop", "index", "view", "procedure", "function",
      "begin", "end", "commit", "rollback", "transaction", "as", "case",
      "when", "then", "else", "null", "is", "in", "exists", "like",
      "between", "distinct", "all", "union", "intersect", "except", "limit",
      "offset", "top", "with", "recursive", "using", "natural", "cross"
    };
    
    QRegularExpression wordRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b");
    QRegularExpressionMatchIterator i = wordRegex.globalMatch(formatted);
    
    // Collect matches in reverse order to replace from end to start
    QList<QRegularExpressionMatch> matches;
    while (i.hasNext()) {
      matches.prepend(i.next());
    }
    
    for (const auto& match : matches) {
      QString word = match.captured(1);
      if (keywords.contains(word.toLower())) {
        formatted.replace(match.capturedStart(1), match.capturedLength(1), 
                         word.toUpper());
      }
    }
    
    // Add newlines after major keywords
    formatted.replace(QRegularExpression("\\s*\\b(SELECT|FROM|WHERE|GROUP BY|ORDER BY|HAVING|JOIN|LEFT JOIN|RIGHT JOIN|INNER JOIN|OUTER JOIN|UNION|INTERSECT|EXCEPT)\\s+", 
                                        QRegularExpression::CaseInsensitiveOption),
                     "\n\\1 ");
    
    // Clean up multiple newlines
    formatted.replace(QRegularExpression("\n{3,}"), "\n\n");
    
    // Replace text using cursor
    QTextCursor cursor = editor->textCursor();
    if (hasSelection) {
      cursor.insertText(formatted);
    } else {
      editor->setPlainText(formatted);
    }
    
    showStatusMessage(tr("SQL formatted"), 2000);
  }
}

void MainWindow::onQueryComment() {
  if (auto* editor = currentEditor()) {
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    if (text.isEmpty()) {
      // Comment current line
      cursor.select(QTextCursor::LineUnderCursor);
      text = cursor.selectedText();
      
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        if (!lines[i].trimmed().isEmpty()) {
          lines[i] = "-- " + lines[i];
        }
      }
      cursor.insertText(lines.join('\n'));
    } else {
      // Comment selection
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        lines[i] = "-- " + lines[i];
      }
      cursor.insertText(lines.join('\n'));
    }
  }
}

void MainWindow::onQueryUncomment() {
  if (auto* editor = currentEditor()) {
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    if (text.isEmpty()) {
      cursor.select(QTextCursor::LineUnderCursor);
      text = cursor.selectedText();
      
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].startsWith("-- ")) {
          lines[i] = lines[i].mid(3);
        } else if (lines[i].startsWith("--")) {
          lines[i] = lines[i].mid(2);
        }
      }
      cursor.insertText(lines.join('\n'));
    } else {
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].startsWith("-- ")) {
          lines[i] = lines[i].mid(3);
        } else if (lines[i].startsWith("--")) {
          lines[i] = lines[i].mid(2);
        }
      }
      cursor.insertText(lines.join('\n'));
    }
  }
}

void MainWindow::onQueryToggleComment() {
  if (auto* editor = currentEditor()) {
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    bool hasSelection = !text.isEmpty();
    
    if (!hasSelection) {
      cursor.select(QTextCursor::LineUnderCursor);
      text = cursor.selectedText();
    }
    
    // Check if already commented
    bool isCommented = text.trimmed().startsWith("--");
    
    if (isCommented) {
      // Uncomment
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        int pos = 0;
        while (pos < line.length() && line[pos].isSpace()) {
          ++pos;
        }
        if (line.mid(pos, 3) == "-- ") {
          lines[i] = line.left(pos) + line.mid(pos + 3);
        } else if (line.mid(pos, 2) == "--") {
          lines[i] = line.left(pos) + line.mid(pos + 2);
        }
      }
      text = lines.join('\n');
    } else {
      // Comment
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        lines[i] = "-- " + lines[i];
      }
      text = lines.join('\n');
    }
    
    if (hasSelection) {
      cursor.insertText(text);
    } else {
      QTextCursor lineCursor = editor->textCursor();
      lineCursor.select(QTextCursor::LineUnderCursor);
      lineCursor.insertText(text);
    }
  }
}

void MainWindow::onQueryUppercase() {
  if (auto* editor = currentEditor()) {
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
      cursor.insertText(text.toUpper());
    } else {
      // Transform entire document
      editor->setPlainText(editor->toPlainText().toUpper());
    }
    showStatusMessage(tr("Converted to uppercase"), 2000);
  }
}

void MainWindow::onQueryLowercase() {
  if (auto* editor = currentEditor()) {
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
      cursor.insertText(text.toLower());
    } else {
      // Transform entire document
      editor->setPlainText(editor->toPlainText().toLower());
    }
    showStatusMessage(tr("Converted to lowercase"), 2000);
  }
}

// Transaction menu slots
void MainWindow::onTransactionStart() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  if (db_connection_->beginTransaction()) {
    showStatusMessage(tr("Transaction started"), 3000);
  } else {
    showError(tr("Failed to start transaction: %1")
              .arg(QString::fromStdString(db_connection_->lastError())));
  }
}

void MainWindow::onTransactionCommit() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  if (db_connection_->commit()) {
    showStatusMessage(tr("Transaction committed"), 3000);
  } else {
    showError(tr("Failed to commit transaction: %1")
              .arg(QString::fromStdString(db_connection_->lastError())));
  }
}

void MainWindow::onTransactionRollback() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  if (db_connection_->rollback()) {
    showStatusMessage(tr("Transaction rolled back"), 3000);
  } else {
    showError(tr("Failed to rollback transaction: %1")
              .arg(QString::fromStdString(db_connection_->lastError())));
  }
}

void MainWindow::onTransactionSavepoint() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  bool ok;
  QString name = QInputDialog::getText(this, tr("Set Savepoint"),
                                       tr("Savepoint name:"),
                                       QLineEdit::Normal,
                                       tr("SAVEPOINT_1"), &ok);
  if (ok && !name.isEmpty()) {
    QString sql = QString("SAVEPOINT %1").arg(name);
    auto result = db_connection_->execute(sql.toStdString());
    if (result.success) {
      showStatusMessage(tr("Savepoint '%1' created").arg(name), 3000);
    } else {
      showError(tr("Failed to create savepoint: %1")
                .arg(QString::fromStdString(result.error_message)));
    }
  }
}

void MainWindow::onTransactionAutoCommit(bool enabled) {
  showStatusMessage(tr("Auto-commit: %1").arg(enabled ? tr("enabled") : tr("disabled")), 2000);
}

void MainWindow::onTransactionIsolationLevel(const QString& level) {
  showStatusMessage(tr("Isolation level: %1").arg(level), 2000);
}

void MainWindow::onTransactionReadOnly(bool enabled) {
  showStatusMessage(tr("Read-only: %1").arg(enabled ? tr("enabled") : tr("disabled")), 2000);
}

// Window menu slots
void MainWindow::onWindowNew() {
  createSqlEditorTab();
}

void MainWindow::onWindowClose() {
  int index = editor_tabs_->currentIndex();
  if (index >= 0) {
    editor_tabs_->removeTab(index);
  }
}

void MainWindow::onWindowCloseAll() {
  while (editor_tabs_->count() > 0) {
    editor_tabs_->removeTab(0);
  }
}

void MainWindow::onWindowNext() {
  int count = editor_tabs_->count();
  if (count > 1) {
    int current = editor_tabs_->currentIndex();
    int next = (current + 1) % count;
    editor_tabs_->setCurrentIndex(next);
  }
}

void MainWindow::onWindowPrevious() {
  int count = editor_tabs_->count();
  if (count > 1) {
    int current = editor_tabs_->currentIndex();
    int prev = (current - 1 + count) % count;
    editor_tabs_->setCurrentIndex(prev);
  }
}

void MainWindow::onWindowCascade() {
  // For tabbed interface, cascade means showing tabs in a particular way
  // We'll split the editor area to show multiple tabs side by side
  if (editor_tabs_->count() >= 2) {
    // Show first two tabs in a split view by using docks
    showStatusMessage(tr("Cascade: Use 'Window > Tile' for split views"), 3000);
  } else {
    showStatusMessage(tr("Need at least 2 tabs to cascade"), 2000);
  }
}

void MainWindow::onWindowTileHorizontal() {
  // Tile horizontally - split editor and results vertically
  resizeDocks({navigator_dock_, results_dock_}, {150, 300}, Qt::Vertical);
  showStatusMessage(tr("Tiled horizontally"), 2000);
}

void MainWindow::onWindowTileVertical() {
  // Tile vertically - side by side
  // Move navigator to right side
  removeDockWidget(navigator_dock_);
  addDockWidget(Qt::RightDockWidgetArea, navigator_dock_);
  showStatusMessage(tr("Tiled vertically"), 2000);
}

void MainWindow::onWindowActivated(int index) {
  if (index >= 0 && index < editor_tabs_->count()) {
    editor_tabs_->setCurrentIndex(index);
  }
}

// Tools menu slots
void MainWindow::onToolsGenerateDdl() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  // Open table designer to generate DDL
  TableDesignerDialog dialog(TableDesignerDialog::Mode::Create, this);
  dialog.exec();
}

void MainWindow::onToolsCompareSchemas() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  // Show schema compare panel
  auto* panel = new SchemaComparePanel(session_client_, this);
  panel->setAttribute(Qt::WA_DeleteOnClose);
  
  QDockWidget* dock = new QDockWidget(tr("Schema Compare"), this);
  dock->setWidget(panel);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::onToolsCompareData() {
  showStatusMessage(tr("Data comparison - select two tables in navigator"), 3000);
}

void MainWindow::onToolsImportSql() {
  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Import SQL Script"),
                                                  QString(),
                                                  tr("SQL Files (*.sql);;All Files (*.*)"));
  if (!fileName.isEmpty()) {
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString sql = QString::fromUtf8(file.readAll());
      file.close();
      
      // Create new editor tab with the SQL
      createSqlEditorTab(QFileInfo(fileName).fileName());
      if (auto* editor = currentEditor()) {
        editor->setPlainText(sql);
      }
      showStatusMessage(tr("Imported %1").arg(fileName), 3000);
    } else {
      showError(tr("Failed to open file: %1").arg(fileName));
    }
  }
}

void MainWindow::onToolsImportCsv() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }

  QString fileName = QFileDialog::getOpenFileName(this, tr("Import CSV"),
                                                  QString(), tr("CSV Files (*.csv);;All Files (*.*)"));
  if (fileName.isEmpty()) return;

  CsvImportDialog dialog(this);
  if (!dialog.loadCsv(fileName)) return;

  if (dialog.exec() == QDialog::Accepted) {
    auto options = dialog.options();
    QStringList headers = dialog.headers();
    QList<QStringList> dataRows = dialog.dataRows();

    if (headers.isEmpty() || dataRows.isEmpty()) {
      showError(tr("No data to import"));
      return;
    }

    // Build CREATE TABLE statement if requested
    if (options.create_table) {
      QString createSql = "CREATE TABLE IF NOT EXISTS \"" + options.table_name + "\" (";
      for (int i = 0; i < headers.size(); ++i) {
        if (i > 0) createSql += ", ";
        // Sanitize column name
        QString colName = headers[i].trimmed();
        colName.replace("\"", "\"\"");
        createSql += "\"" + colName + "\" TEXT";
      }
      createSql += ")";

      auto result = db_connection_->execute(createSql.toStdString());
      if (!result.success) {
        showError(tr("Failed to create table: %1").arg(QString::fromStdString(result.error_message)));
        return;
      }
    }

    // Insert data in batches
    int totalRows = dataRows.size();
    int imported = 0;
    int failed = 0;

    for (int i = 0; i < dataRows.size(); i += options.batch_size) {
      QString insertSql = "INSERT INTO \"" + options.table_name + "\" (";
      for (int h = 0; h < headers.size(); ++h) {
        if (h > 0) insertSql += ", ";
        QString colName = headers[h].trimmed();
        colName.replace("\"", "\"\"");
        insertSql += "\"" + colName + "\"";
      }
      insertSql += ") VALUES ";

      int batchEnd = qMin(i + options.batch_size, dataRows.size());
      bool firstValue = true;

      for (int row = i; row < batchEnd; ++row) {
        if (!firstValue) insertSql += ", ";
        firstValue = false;

        insertSql += "(";
        const QStringList& rowData = dataRows[row];
        for (int col = 0; col < headers.size(); ++col) {
          if (col > 0) insertSql += ", ";
          if (col < rowData.size()) {
            QString val = rowData[col];
            val.replace("'", "''");
            insertSql += "'" + val + "'";
          } else {
            insertSql += "NULL";
          }
        }
        insertSql += ")";
      }

      auto result = db_connection_->execute(insertSql.toStdString());
      if (result.success) {
        imported += (batchEnd - i);
      } else {
        failed += (batchEnd - i);
        showError(tr("Batch insert failed: %1").arg(QString::fromStdString(result.error_message)));
      }

      // Update progress
      showStatusMessage(tr("Importing: %1/%2 rows").arg(imported).arg(totalRows), 1000);
      QApplication::processEvents();
    }

    showStatusMessage(tr("CSV import complete: %1 imported, %2 failed").arg(imported).arg(failed), 5000);
  }
}

void MainWindow::onToolsImportJson() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }

  QString fileName = QFileDialog::getOpenFileName(this, tr("Import JSON"),
                                                  QString(), tr("JSON Files (*.json);;All Files (*.*)"));
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    showError(tr("Failed to open file: %1").arg(fileName));
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull()) {
    showError(tr("Invalid JSON file"));
    return;
  }

  // Get table name from filename
  QFileInfo fileInfo(fileName);
  QString tableName = fileInfo.baseName();
  bool ok;
  tableName = QInputDialog::getText(this, tr("Import JSON"),
                                    tr("Table name:"), QLineEdit::Normal,
                                    tableName, &ok);
  if (!ok || tableName.isEmpty()) return;

  // Sanitize table name
  tableName.replace("\"", "\"\"");

  QJsonArray array;
  if (doc.isArray()) {
    array = doc.array();
  } else if (doc.isObject()) {
    array.append(doc.object());
  }

  if (array.isEmpty()) {
    showError(tr("No data found in JSON file"));
    return;
  }

  // Extract column names from first object
  QJsonObject firstObj = array.first().toObject();
  QStringList columns;
  for (const QString& key : firstObj.keys()) {
    columns.append(key);
  }

  // Create table
  QString createSql = "CREATE TABLE IF NOT EXISTS \"" + tableName + "\" (";
  for (int i = 0; i < columns.size(); ++i) {
    if (i > 0) createSql += ", ";
    QString col = columns[i];
    col.replace("\"", "\"\"");
    createSql += "\"" + col + "\" TEXT";
  }
  createSql += ")";

  auto createResult = db_connection_->execute(createSql.toStdString());
  if (!createResult.success) {
    showError(tr("Failed to create table: %1").arg(QString::fromStdString(createResult.error_message)));
    return;
  }

  // Insert data
  int imported = 0;
  int batchSize = 100;
  
  for (int i = 0; i < array.size(); i += batchSize) {
    QString insertSql = "INSERT INTO \"" + tableName + "\" (";
    for (int c = 0; c < columns.size(); ++c) {
      if (c > 0) insertSql += ", ";
      QString col = columns[c];
      col.replace("\"", "\"\"");
      insertSql += "\"" + col + "\"";
    }
    insertSql += ") VALUES ";

    bool firstVal = true;
    int batchEnd = qMin(i + batchSize, array.size());
    
    for (int j = i; j < batchEnd; ++j) {
      if (!firstVal) insertSql += ", ";
      firstVal = false;

      QJsonObject obj = array[j].toObject();
      insertSql += "(";
      for (int c = 0; c < columns.size(); ++c) {
        if (c > 0) insertSql += ", ";
        
        QJsonValue val = obj.value(columns[c]);
        if (val.isNull() || val.isUndefined()) {
          insertSql += "NULL";
        } else if (val.isString()) {
          QString s = val.toString();
          s.replace("'", "''");
          insertSql += "'" + s + "'";
        } else if (val.isBool()) {
          insertSql += val.toBool() ? "TRUE" : "FALSE";
        } else if (val.isDouble()) {
          insertSql += QString::number(val.toDouble());
        } else {
          QString s = val.toVariant().toString();
          s.replace("'", "''");
          insertSql += "'" + s + "'";
        }
      }
      insertSql += ")";
    }

    auto result = db_connection_->execute(insertSql.toStdString());
    if (result.success) {
      imported += (batchEnd - i);
    }

    showStatusMessage(tr("Importing JSON: %1/%2").arg(imported).arg(array.size()), 1000);
    QApplication::processEvents();
  }

  showStatusMessage(tr("JSON import complete: %1 rows imported").arg(imported), 5000);
}

void MainWindow::onToolsImportExcel() {
#ifdef SCRATCHROBIN_WITH_QXLSX
  QString fileName = QFileDialog::getOpenFileName(this, tr("Import Excel"),
                                                  QString(),
                                                  tr("Excel Files (*.xlsx *.xls)"));
  if (fileName.isEmpty()) return;
  
  QXlsx::Document xlsx(fileName);
  if (xlsx.load() == false) {
    showError(tr("Failed to load Excel file: %1").arg(fileName));
    return;
  }
  
  // Get the current worksheet
  auto* sheet = xlsx.currentSheet();
  if (!sheet) {
    showError(tr("No worksheet found in Excel file"));
    return;
  }
  QString sheetName = sheet->sheetName();
  
  // Read dimensions
  QXlsx::CellRange range = xlsx.dimension();
  int firstRow = range.firstRow();
  int lastRow = range.lastRow();
  int firstCol = range.firstColumn();
  int lastCol = range.lastColumn();
  
  if (firstRow > lastRow || firstCol > lastCol) {
    showError(tr("Excel file appears to be empty"));
    return;
  }
  
  // Preview dialog
  QDialog previewDialog(this);
  previewDialog.setWindowTitle(tr("Excel Import Preview"));
  previewDialog.setMinimumSize(700, 500);
  
  auto* layout = new QVBoxLayout(&previewDialog);
  
  // Info label
  auto* infoLabel = new QLabel(tr("Sheet: %1 | Rows: %2 | Columns: %3")
                               .arg(sheetName)
                               .arg(lastRow - firstRow + 1)
                               .arg(lastCol - firstCol + 1), &previewDialog);
  layout->addWidget(infoLabel);
  
  // Preview table
  auto* table = new QTableWidget(&previewDialog);
  table->setColumnCount(lastCol - firstCol + 1);
  table->setRowCount(qMin(100, lastRow - firstRow + 1)); // Show first 100 rows
  
  // Set headers from first row
  QStringList headers;
  for (int col = firstCol; col <= lastCol; ++col) {
    auto cell = xlsx.cellAt(firstRow, col);
    QString header = cell ? cell->value().toString() : tr("Column %1").arg(col);
    headers << header;
  }
  table->setHorizontalHeaderLabels(headers);
  
  // Fill data
  for (int row = firstRow + 1; row <= qMin(lastRow, firstRow + 100); ++row) {
    int tableRow = row - firstRow - 1;
    for (int col = firstCol; col <= lastCol; ++col) {
      auto cell = xlsx.cellAt(row, col);
      QString value = cell ? cell->value().toString() : QString();
      table->setItem(tableRow, col - firstCol, new QTableWidgetItem(value));
    }
  }
  
  table->resizeColumnsToContents();
  layout->addWidget(table);
  
  // Target table input
  auto* formLayout = new QFormLayout();
  auto* targetTableEdit = new QLineEdit(&previewDialog);
  targetTableEdit->setPlaceholderText(tr("target_table"));
  formLayout->addRow(tr("Target Table:"), targetTableEdit);
  
  auto* firstRowHeaderCheck = new QCheckBox(tr("First row contains headers"), &previewDialog);
  firstRowHeaderCheck->setChecked(true);
  formLayout->addRow(firstRowHeaderCheck);
  
  layout->addLayout(formLayout);
  
  // Buttons
  auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                      &previewDialog);
  connect(btnBox, &QDialogButtonBox::accepted, &previewDialog, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, &previewDialog, &QDialog::reject);
  layout->addWidget(btnBox);
  
  if (previewDialog.exec() != QDialog::Accepted) return;
  
  QString targetTable = targetTableEdit->text().trimmed();
  if (targetTable.isEmpty()) {
    showError(tr("Target table name is required"));
    return;
  }
  
  bool firstRowIsHeader = firstRowHeaderCheck->isChecked();
  int startRow = firstRowIsHeader ? firstRow + 1 : firstRow;
  
  // Generate INSERT statements
  QStringList sqlStatements;
  int rowCount = 0;
  
  for (int row = startRow; row <= lastRow; ++row) {
    QStringList values;
    bool hasData = false;
    
    for (int col = firstCol; col <= lastCol; ++col) {
      auto cell = xlsx.cellAt(row, col);
      QString value = cell ? cell->value().toString() : QString();
      if (!value.isEmpty()) hasData = true;
      
      // Escape single quotes
      value.replace("'", "''");
      values << QString("'%1'").arg(value);
    }
    
    if (!hasData) continue; // Skip empty rows
    
    QString sql = QString("INSERT INTO %1 (%2) VALUES (%3);")
                  .arg(targetTable)
                  .arg(headers.join(", "))
                  .arg(values.join(", "));
    sqlStatements << sql;
    rowCount++;
    
    if (rowCount >= 1000) break; // Limit for safety
  }
  
  // Show SQL in current editor
  if (!sqlStatements.isEmpty()) {
    SqlEditor* editor = currentEditor();
    if (editor) {
      editor->setPlainText(sqlStatements.join("\n"));
      showStatusMessage(tr("Generated %1 INSERT statements for table '%2'")
                        .arg(rowCount).arg(targetTable), 3000);
    }
  }
  
#else
  showStatusMessage(tr("Excel import requires QXlsx library (not available)"), 3000);
  QMessageBox::information(this, tr("Excel Import"),
                           tr("Excel support is not enabled. "
                              "Rebuild with SCRATCHROBIN_ENABLE_EXCEL=ON"));
#endif
}

void MainWindow::onToolsExportSql() {
  if (!results_grid_->hasData()) {
    showStatusMessage(tr("No results to export"), 2000);
    return;
  }

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export SQL"), 
                                                  QString(), tr("SQL Files (*.sql)"));
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    showError(tr("Cannot open file for writing: %1").arg(fileName));
    return;
  }

  QTextStream stream(&file);
  QStringList headers = results_grid_->headers();
  QList<QStringList> data = results_grid_->allData();

  // Generate INSERT statements
  for (const QStringList& row : data) {
    stream << "INSERT INTO table_name (";
    for (int i = 0; i < headers.size(); ++i) {
      if (i > 0) stream << ", ";
      QString col = headers[i];
      col.replace("\"", "\"\"");
      stream << "\"" << col << "\"";
    }
    stream << ") VALUES (";
    for (int i = 0; i < row.size(); ++i) {
      if (i > 0) stream << ", ";
      QString val = row[i];
      if (val.isEmpty()) {
        stream << "NULL";
      } else {
        val.replace("'", "''");
        stream << "'" << val << "'";
      }
    }
    stream << ");\n";
  }

  file.close();
  showStatusMessage(tr("Exported %1 rows to SQL").arg(data.size()), 3000);
}

void MainWindow::onToolsExportCsv() {
  if (!results_grid_->hasData()) {
    showStatusMessage(tr("No results to export"), 2000);
    return;
  }

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export CSV"),
                                                  QString(), tr("CSV Files (*.csv)"));
  if (fileName.isEmpty()) return;

  results_grid_->exportToCsv(fileName);
  showStatusMessage(tr("Exported to CSV: %1").arg(fileName), 3000);
}

void MainWindow::onToolsExportJson() {
  if (!results_grid_->hasData()) {
    showStatusMessage(tr("No results to export"), 2000);
    return;
  }

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export JSON"),
                                                  QString(), tr("JSON Files (*.json)"));
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    showError(tr("Cannot open file for writing: %1").arg(fileName));
    return;
  }

  QStringList headers = results_grid_->headers();
  QList<QStringList> data = results_grid_->allData();

  QJsonArray array;
  for (const QStringList& row : data) {
    QJsonObject obj;
    for (int i = 0; i < qMin(row.size(), headers.size()); ++i) {
      obj[headers[i]] = row[i];
    }
    array.append(obj);
  }

  QJsonDocument doc(array);
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();

  showStatusMessage(tr("Exported %1 rows to JSON").arg(data.size()), 3000);
}

void MainWindow::onToolsExportExcel() {
#ifdef SCRATCHROBIN_WITH_QXLSX
  if (!results_grid_->hasData()) {
    showStatusMessage(tr("No results to export"), 2000);
    return;
  }
  
  QString fileName = QFileDialog::getSaveFileName(this, tr("Export Excel"),
                                                  QString(),
                                                  tr("Excel Files (*.xlsx)"));
  if (fileName.isEmpty()) return;
  
  // Ensure .xlsx extension
  if (!fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
    fileName += ".xlsx";
  }
  
  QXlsx::Document xlsx;
  QXlsx::Format headerFormat;
  headerFormat.setFontBold(true);
  headerFormat.setFillPattern(QXlsx::Format::PatternSolid);
  headerFormat.setPatternBackgroundColor(QColor(200, 200, 200));
  
  // Get data from grid
  QStringList headers = results_grid_->headers();
  QList<QStringList> data = results_grid_->allData();
  
  // Write headers
  for (int col = 0; col < headers.size(); ++col) {
    xlsx.write(1, col + 1, headers[col], headerFormat);
  }
  
  // Write data
  for (int row = 0; row < data.size(); ++row) {
    const QStringList& rowData = data[row];
    for (int col = 0; col < qMin(rowData.size(), headers.size()); ++col) {
      // Try to detect numeric values
      bool isNumber = false;
      double numValue = rowData[col].toDouble(&isNumber);
      
      if (isNumber) {
        xlsx.write(row + 2, col + 1, numValue);
      } else {
        xlsx.write(row + 2, col + 1, rowData[col]);
      }
    }
    
    // Progress update every 1000 rows
    if ((row + 1) % 1000 == 0) {
      showStatusMessage(tr("Exporting row %1 of %2...").arg(row + 1).arg(data.size()), 500);
      QApplication::processEvents();
    }
  }
  
  // Auto-fit column widths (approximate)
  for (int col = 0; col < headers.size(); ++col) {
    int maxWidth = headers[col].length();
    for (int row = 0; row < qMin(data.size(), 100); ++row) {
      if (col < data[row].size()) {
        maxWidth = qMax(maxWidth, data[row][col].length());
      }
    }
    xlsx.setColumnWidth(col + 1, qMin(maxWidth + 2, 50));
  }
  
  // Freeze header row
  xlsx.addSheet("Results");
  
  if (xlsx.saveAs(fileName)) {
    showStatusMessage(tr("Exported %1 rows to %2").arg(data.size()).arg(fileName), 3000);
  } else {
    showError(tr("Failed to save Excel file: %1").arg(fileName));
  }
  
#else
  showStatusMessage(tr("Excel export requires QXlsx library (not available)"), 3000);
  QMessageBox::information(this, tr("Excel Export"),
                           tr("Excel support is not enabled. "
                              "Rebuild with SCRATCHROBIN_ENABLE_EXCEL=ON"));
#endif
}

void MainWindow::onToolsExportXml() {
  if (!results_grid_->hasData()) {
    showStatusMessage(tr("No results to export"), 2000);
    return;
  }

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export XML"),
                                                  QString(), tr("XML Files (*.xml)"));
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    showError(tr("Cannot open file for writing: %1").arg(fileName));
    return;
  }

  QXmlStreamWriter writer(&file);
  writer.setAutoFormatting(true);
  writer.writeStartDocument();
  writer.writeStartElement("resultset");

  QStringList headers = results_grid_->headers();
  QList<QStringList> data = results_grid_->allData();

  for (const QStringList& row : data) {
    writer.writeStartElement("row");
    for (int i = 0; i < qMin(row.size(), headers.size()); ++i) {
      QString colName = headers[i];
      // Sanitize XML element name
      colName.replace(QRegularExpression("[^a-zA-Z0-9_]"), "_");
      if (colName.at(0).isDigit()) colName = "col" + colName;
      
      writer.writeTextElement(colName, row[i]);
    }
    writer.writeEndElement(); // row
  }

  writer.writeEndElement(); // resultset
  writer.writeEndDocument();
  file.close();

  showStatusMessage(tr("Exported %1 rows to XML").arg(data.size()), 3000);
}

void MainWindow::onToolsExportHtml() {
  if (!results_grid_->hasData()) {
    showStatusMessage(tr("No results to export"), 2000);
    return;
  }

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export HTML"),
                                                  QString(), tr("HTML Files (*.html)"));
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    showError(tr("Cannot open file for writing: %1").arg(fileName));
    return;
  }

  QTextStream stream(&file);
  QStringList headers = results_grid_->headers();
  QList<QStringList> data = results_grid_->allData();

  stream << "<!DOCTYPE html>\n";
  stream << "<html>\n<head>\n";
  stream << "<title>Query Results</title>\n";
  stream << "<style>\n";
  stream << "table { border-collapse: collapse; width: 100%; }\n";
  stream << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
  stream << "th { background-color: #4CAF50; color: white; }\n";
  stream << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
  stream << "</style>\n";
  stream << "</head>\n<body>\n";
  stream << "<h2>Query Results</h2>\n";
  stream << "<p>Total rows: " << data.size() << "</p>\n";
  stream << "<table>\n<thead>\n<tr>\n";

  // Headers
  for (const QString& header : headers) {
    stream << "<th>" << header.toHtmlEscaped() << "</th>\n";
  }
  stream << "</tr>\n</thead>\n<tbody>\n";

  // Data rows
  for (const QStringList& row : data) {
    stream << "<tr>\n";
    for (const QString& cell : row) {
      stream << "<td>" << cell.toHtmlEscaped() << "</td>\n";
    }
    stream << "</tr>\n";
  }

  stream << "</tbody>\n</table>\n";
  stream << "</body>\n</html>\n";

  file.close();
  showStatusMessage(tr("Exported %1 rows to HTML").arg(data.size()), 3000);
}

void MainWindow::onToolsMigration() {
  // Open schema migration tool panel
  auto* panel = new SchemaMigrationToolPanel(session_client_, this);
  panel->setAttribute(Qt::WA_DeleteOnClose);
  
  QDockWidget* dock = new QDockWidget(tr("Schema Migration"), this);
  dock->setWidget(panel);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::onToolsScheduler() {
  // Open scheduled jobs dock
  auto* jobs = new ScheduledJobsPanel(session_client_, this);
  jobs->setAttribute(Qt::WA_DeleteOnClose);
  
  QDockWidget* dock = new QDockWidget(tr("Scheduled Jobs"), this);
  dock->setWidget(jobs);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::onToolsMonitorConnections() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  // Query for connections (database-specific)
  auto result = db_connection_->query("SELECT * FROM pg_stat_activity");
  if (result.success) {
    showStatusMessage(tr("Found %1 connections").arg(result.rows.size()), 3000);
  } else {
    showStatusMessage(tr("Connection monitoring: %1").arg(QString::fromStdString(result.error_message)), 3000);
  }
}

void MainWindow::onToolsMonitorTransactions() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  showStatusMessage(tr("Transaction monitoring - check status bar for TX indicator"), 3000);
}

void MainWindow::onToolsMonitorStatements() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  // Create and show monitoring dashboard with Statements tab active
  auto* dashboard = new MonitoringDashboard(session_client_, this);
  dashboard->setAttribute(Qt::WA_DeleteOnClose);
  dashboard->setActiveTab(0); // Connection tab (Statements/Queries)
  dashboard->show();
  dashboard->resize(900, 600);
}

void MainWindow::onToolsMonitorLocks() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  // Create and show monitoring dashboard with Locks tab active
  auto* dashboard = new MonitoringDashboard(session_client_, this);
  dashboard->setAttribute(Qt::WA_DeleteOnClose);
  dashboard->setActiveTab(2); // Locks tab
  dashboard->show();
  dashboard->resize(900, 600);
}

void MainWindow::onToolsMonitorPerformance() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    showError(tr("Not connected to database"));
    return;
  }
  
  // Create and show monitoring dashboard with Performance tab active
  auto* dashboard = new MonitoringDashboard(session_client_, this);
  dashboard->setAttribute(Qt::WA_DeleteOnClose);
  dashboard->setActiveTab(3); // Performance tab
  dashboard->show();
  dashboard->resize(900, 600);
}

// ============================================================================
// New Feature Menu Actions
// ============================================================================

void MainWindow::onViewViewManager() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("view_manager");
    showStatusMessage(tr("View Manager toggled"), 2000);
  }
}

void MainWindow::onViewTriggerManager() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("trigger_manager");
    showStatusMessage(tr("Trigger Manager toggled"), 2000);
  }
}

void MainWindow::onViewGitIntegration() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("git_integration");
    showStatusMessage(tr("Git Integration toggled"), 2000);
  }
}

void MainWindow::onViewReportManager() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("report_manager");
    showStatusMessage(tr("Report Manager toggled"), 2000);
  }
}

void MainWindow::onViewDashboardManager() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("dashboard_manager");
    showStatusMessage(tr("Dashboard Manager toggled"), 2000);
  }
}

void MainWindow::onViewTableDesigner() {
  TableDesignerDialog dialog(TableDesignerDialog::Mode::Create, this);
  dialog.exec();
}

void MainWindow::onViewSqlHistory() {
  // Create storage and load dialog
  auto* storage = new QueryHistoryStorage(this);
  QueryHistoryDialog dialog(storage, this);
  dialog.exec();
}

void MainWindow::onViewQueryFavorites() {
  // Open query favorites dialog
  QMessageBox::information(this, tr("Query Favorites"), 
    tr("Query Favorites feature - select a saved query to load into editor"));
}

void MainWindow::onDashboardOpenRequested(const QString& dashboardId) {
  Q_UNUSED(dashboardId)
  // Open dashboard viewer
  QMessageBox::information(this, tr("Dashboard"), 
    tr("Opening dashboard viewer..."));
}

// ============================================================================
// Schema Compare, Migration, and Data Modeling Tools
// ============================================================================

void MainWindow::onToolsSchemaCompare() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("schema_compare");
    showStatusMessage(tr("Schema Compare opened"), 2000);
  }
}

void MainWindow::onToolsSchemaMigration() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("schema_migration");
    showStatusMessage(tr("Schema Migration Tool opened"), 2000);
  }
}

void MainWindow::onToolsDataModeler() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("data_modeler");
    showStatusMessage(tr("Data Modeler opened"), 2000);
  }
}

void MainWindow::onToolsERDiagramDesigner() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("er_diagram");
    showStatusMessage(tr("ER Diagram Designer opened"), 2000);
  }
}

void MainWindow::onToolsQueryBuilder() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("query_builder");
    showStatusMessage(tr("Query Builder opened"), 2000);
  }
}

void MainWindow::onToolsQueryPlanVisualizer() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("query_plan");
    showStatusMessage(tr("Query Plan Visualizer opened"), 2000);
  }
}

void MainWindow::onToolsSqlDebugger() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("sql_debugger");
    showStatusMessage(tr("SQL Debugger opened"), 2000);
  }
}

void MainWindow::onToolsProcedureDebugger() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("procedure_debugger");
    showStatusMessage(tr("Procedure Debugger opened"), 2000);
  }
}

void MainWindow::onToolsSqlProfiler() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("sql_profiler");
    showStatusMessage(tr("SQL Profiler opened"), 2000);
  }
}

void MainWindow::onToolsCodeFormatter() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("code_formatter");
    showStatusMessage(tr("Code Formatter opened"), 2000);
  }
}

void MainWindow::onToolsPerformanceDashboard() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("performance_dashboard");
    showStatusMessage(tr("Performance Dashboard opened"), 2000);
  }
}

void MainWindow::onToolsSlowQueryLog() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("slow_query_log");
    showStatusMessage(tr("Slow Query Log Viewer opened"), 2000);
  }
}

void MainWindow::onToolsHealthCheck() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("health_check");
    showStatusMessage(tr("Database Health Check opened"), 2000);
  }
}

void MainWindow::onToolsAlertSystem() {
  if (dock_workspace_) {
    dock_workspace_->togglePanel("alert_system");
    showStatusMessage(tr("Alert System opened"), 2000);
  }
}

// ============================================================================
// Security Management Actions
// ============================================================================

void MainWindow::onToolsUserManagement() {
  // Create panel if not exists
  auto* panel = new UserManagementPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "user_management";
  info.title = tr("User Management");
  info.category = "security";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("user_management");
  showStatusMessage(tr("User Management opened"), 2000);
}

void MainWindow::onToolsRoleManagement() {
  auto* panel = new RoleManagementPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "role_management";
  info.title = tr("Role Management");
  info.category = "security";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("role_management");
  showStatusMessage(tr("Role Management opened"), 2000);
}

void MainWindow::onToolsPermissionBrowser() {
  auto* panel = new PermissionBrowserPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "permission_browser";
  info.title = tr("Permission Browser");
  info.category = "security";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("permission_browser");
  showStatusMessage(tr("Permission Browser opened"), 2000);
}

void MainWindow::onToolsAuditLogViewer() {
  auto* panel = new AuditLogViewerPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "audit_log_viewer";
  info.title = tr("Audit Log Viewer");
  info.category = "security";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("audit_log_viewer");
  showStatusMessage(tr("Audit Log Viewer opened"), 2000);
}

void MainWindow::onToolsSSLCertificateManager() {
  auto* panel = new SSLCertificateManagerPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "ssl_certificate_manager";
  info.title = tr("SSL Certificate Manager");
  info.category = "security";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("ssl_certificate_manager");
  showStatusMessage(tr("SSL Certificate Manager opened"), 2000);
}

// ============================================================================
// Maintenance and Backup Actions
// ============================================================================

void MainWindow::onToolsBackupManager() {
  auto* panel = new BackupManagerPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "backup_manager";
  info.title = tr("Backup & Restore");
  info.category = "maintenance";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("backup_manager");
  showStatusMessage(tr("Backup Manager opened"), 2000);
}

void MainWindow::onToolsScheduledJobs() {
  auto* panel = new ScheduledJobsPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "scheduled_jobs";
  info.title = tr("Scheduled Jobs");
  info.category = "maintenance";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("scheduled_jobs");
  showStatusMessage(tr("Scheduled Jobs opened"), 2000);
}

void MainWindow::onToolsVacuumAnalyze() {
  auto* panel = new VacuumAnalyzePanel(session_client_, this);
  DockPanelInfo info;
  info.id = "vacuum_analyze";
  info.title = tr("Vacuum/Analyze");
  info.category = "maintenance";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("vacuum_analyze");
  showStatusMessage(tr("Vacuum/Analyze tool opened"), 2000);
}

// ============================================================================
// Data Management Actions
// ============================================================================

void MainWindow::onToolsDataGenerator() {
  auto* panel = new DataGeneratorPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "data_generator";
  info.title = tr("Data Generator");
  info.category = "tools";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("data_generator");
  showStatusMessage(tr("Data Generator opened"), 2000);
}

void MainWindow::onToolsDataMasking() {
  auto* panel = new DataMaskingPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "data_masking";
  info.title = tr("Data Masking");
  info.category = "security";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("data_masking");
  showStatusMessage(tr("Data Masking opened"), 2000);
}

void MainWindow::onToolsDataCleansing() {
  auto* panel = new DataCleansingPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "data_cleansing";
  info.title = tr("Data Cleansing");
  info.category = "tools";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("data_cleansing");
  showStatusMessage(tr("Data Cleansing opened"), 2000);
}

void MainWindow::onToolsDataLineage() {
  auto* panel = new DataLineagePanel(session_client_, this);
  DockPanelInfo info;
  info.id = "data_lineage";
  info.title = tr("Data Lineage");
  info.category = "analysis";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("data_lineage");
  showStatusMessage(tr("Data Lineage opened"), 2000);
}

void MainWindow::onToolsTeamSharing() {
  auto* panel = new TeamSharingPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "team_sharing";
  info.title = tr("Team Sharing");
  info.category = "collaboration";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("team_sharing");
  showStatusMessage(tr("Team Sharing opened"), 2000);
}

void MainWindow::onToolsQueryComments() {
  // For now, open as a dialog. Can be changed to dock panel if preferred.
  // Using a sample query ID since we don't have a specific query selected
  SqlEditor* editor = currentEditor();
  QString content = editor ? editor->toPlainText() : "";
  QueryCommentsDialog dialog("query_1", tr("Current Query"), content,
                             session_client_, this);
  dialog.exec();
  showStatusMessage(tr("Query Comments opened"), 2000);
}

void MainWindow::onToolsChangeTracking() {
  auto* panel = new ChangeTrackingPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "change_tracking";
  info.title = tr("Change Tracking");
  info.category = "audit";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("change_tracking");
  showStatusMessage(tr("Change Tracking opened"), 2000);
}

void MainWindow::onToolsPluginManager() {
  auto* panel = new PluginManagerPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "plugin_manager";
  info.title = tr("Plugin Manager");
  info.category = "system";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("plugin_manager");
  showStatusMessage(tr("Plugin Manager opened"), 2000);
}

void MainWindow::onToolsMacroRecording() {
  auto* panel = new MacroManagerPanel(session_client_, this);
  DockPanelInfo info;
  info.id = "macro_manager";
  info.title = tr("Macro Manager");
  info.category = "automation";
  info.defaultArea = DockAreaType::Right;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("macro_manager");
  showStatusMessage(tr("Macro Manager opened"), 2000);
}

void MainWindow::onToolsScriptingConsole() {
  auto* panel = new ScriptingConsolePanel(session_client_, this);
  DockPanelInfo info;
  info.id = "scripting_console";
  info.title = tr("Scripting Console");
  info.category = "development";
  info.defaultArea = DockAreaType::Bottom;
  info.allowFloating = true;
  info.allowClose = true;
  dock_workspace_->registerPanel(info, panel);
  dock_workspace_->showPanel("scripting_console");
  showStatusMessage(tr("Scripting Console opened"), 2000);
}

void MainWindow::updateWindowTitle(const QString& filename) {
  if (filename.isEmpty()) {
    setWindowTitle(tr("ScratchRobin - Database Administration Tool"));
  } else {
    QFileInfo info(filename);
    setWindowTitle(tr("%1 - ScratchRobin").arg(info.fileName()));
  }
}

}  // namespace scratchrobin::ui
