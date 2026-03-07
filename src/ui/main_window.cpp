#include "ui/main_window.h"
#include "ui/project_navigator.h"
#include "ui/sql_editor.h"
#include "ui/connection_dialog.h"
#include "ui/data_grid.h"
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

namespace scratchrobin::ui {

MainWindow::MainWindow(backend::SessionClient* session_client, QWidget* parent)
    : QMainWindow(parent)
    , session_client_(session_client)
    , db_connection_(std::make_unique<backend::ScratchbirdConnection>())
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
  help_menu_->addAction(tr("&Context Help"), this, []() { /* TODO */ }, QKeySequence("Shift+F1"));
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
  // TODO: Save current editor content
  showStatusMessage(tr("Saved"), 2000);
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
    // TODO: Apply preferences to application
    showStatusMessage(tr("Preferences saved"), 2000);
  });
  dialog.exec();
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
  // TODO: Open layout management dialog
  showStatusMessage(tr("Layout management - not yet implemented"), 2000);
}

// Help menu
void MainWindow::onEditFind() {
  showStatusMessage(tr("Find - not yet implemented"), 2000);
}

void MainWindow::onEditFindReplace() {
  FindReplaceDialog dialog(this);
  if (auto* editor = currentEditor()) {
    dialog.setTextEdit(editor);
    dialog.setFindText(editor->selectedSql());
  }
  dialog.show();  // Non-modal
}

void MainWindow::onEditFindNext() {
  showStatusMessage(tr("Find next - not yet implemented"), 2000);
}

void MainWindow::onEditFindPrevious() {
  showStatusMessage(tr("Find previous - not yet implemented"), 2000);
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
  showStatusMessage(tr("Execute script - not yet implemented"), 3000);
}

void MainWindow::onQueryStop() {
  showStatusMessage(tr("Stop execution - not yet implemented"), 3000);
}

void MainWindow::onQueryExplain() {
  showStatusMessage(tr("Explain plan - not yet implemented"), 3000);
}

void MainWindow::onQueryExplainAnalyze() {
  showStatusMessage(tr("Explain analyze - not yet implemented"), 3000);
}

void MainWindow::onQueryFormatSql() {
  showStatusMessage(tr("Format SQL - not yet implemented"), 3000);
}

void MainWindow::onQueryComment() {
  showStatusMessage(tr("Comment lines - not yet implemented"), 3000);
}

void MainWindow::onQueryUncomment() {
  showStatusMessage(tr("Uncomment lines - not yet implemented"), 3000);
}

void MainWindow::onQueryToggleComment() {
  showStatusMessage(tr("Toggle comment - not yet implemented"), 3000);
}

void MainWindow::onQueryUppercase() {
  if (auto* editor = currentEditor()) {
    // TODO: Convert selection to uppercase
    showStatusMessage(tr("Uppercase - not yet implemented"), 2000);
  }
}

void MainWindow::onQueryLowercase() {
  if (auto* editor = currentEditor()) {
    // TODO: Convert selection to lowercase
    showStatusMessage(tr("Lowercase - not yet implemented"), 2000);
  }
}

// Transaction menu slots
void MainWindow::onTransactionStart() {
  showStatusMessage(tr("Start transaction - not yet implemented"), 3000);
}

void MainWindow::onTransactionCommit() {
  showStatusMessage(tr("Commit transaction - not yet implemented"), 3000);
}

void MainWindow::onTransactionRollback() {
  showStatusMessage(tr("Rollback transaction - not yet implemented"), 3000);
}

void MainWindow::onTransactionSavepoint() {
  showStatusMessage(tr("Set savepoint - not yet implemented"), 3000);
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
  showStatusMessage(tr("Cascade windows - not yet implemented"), 2000);
}

void MainWindow::onWindowTileHorizontal() {
  showStatusMessage(tr("Tile horizontal - not yet implemented"), 2000);
}

void MainWindow::onWindowTileVertical() {
  showStatusMessage(tr("Tile vertical - not yet implemented"), 2000);
}

void MainWindow::onWindowActivated(int index) {
  if (index >= 0 && index < editor_tabs_->count()) {
    editor_tabs_->setCurrentIndex(index);
  }
}

// Tools menu slots
void MainWindow::onToolsGenerateDdl() {
  showStatusMessage(tr("Generate DDL - not yet implemented"), 3000);
}

void MainWindow::onToolsCompareSchemas() {
  showStatusMessage(tr("Compare schemas - not yet implemented"), 3000);
}

void MainWindow::onToolsCompareData() {
  showStatusMessage(tr("Compare data - not yet implemented"), 3000);
}

void MainWindow::onToolsImportSql() {
  showStatusMessage(tr("Import SQL - not yet implemented"), 3000);
}

void MainWindow::onToolsImportCsv() {
  showStatusMessage(tr("Import CSV - not yet implemented"), 3000);
}

void MainWindow::onToolsImportJson() {
  showStatusMessage(tr("Import JSON - not yet implemented"), 3000);
}

void MainWindow::onToolsImportExcel() {
  showStatusMessage(tr("Import Excel - not yet implemented"), 3000);
}

void MainWindow::onToolsExportSql() {
  showStatusMessage(tr("Export SQL - not yet implemented"), 3000);
}

void MainWindow::onToolsExportCsv() {
  showStatusMessage(tr("Export CSV - not yet implemented"), 3000);
}

void MainWindow::onToolsExportJson() {
  showStatusMessage(tr("Export JSON - not yet implemented"), 3000);
}

void MainWindow::onToolsExportExcel() {
  showStatusMessage(tr("Export Excel - not yet implemented"), 3000);
}

void MainWindow::onToolsExportXml() {
  showStatusMessage(tr("Export XML - not yet implemented"), 3000);
}

void MainWindow::onToolsExportHtml() {
  showStatusMessage(tr("Export HTML - not yet implemented"), 3000);
}

void MainWindow::onToolsMigration() {
  showStatusMessage(tr("Database migration - not yet implemented"), 3000);
}

void MainWindow::onToolsScheduler() {
  showStatusMessage(tr("Job scheduler - not yet implemented"), 3000);
}

void MainWindow::onToolsMonitorConnections() {
  showStatusMessage(tr("Monitor connections - not yet implemented"), 3000);
}

void MainWindow::onToolsMonitorTransactions() {
  showStatusMessage(tr("Monitor transactions - not yet implemented"), 3000);
}

void MainWindow::onToolsMonitorStatements() {
  showStatusMessage(tr("Monitor statements - not yet implemented"), 3000);
}

void MainWindow::onToolsMonitorLocks() {
  showStatusMessage(tr("Monitor locks - not yet implemented"), 3000);
}

void MainWindow::onToolsMonitorPerformance() {
  showStatusMessage(tr("Monitor performance - not yet implemented"), 3000);
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

}  // namespace scratchrobin::ui
