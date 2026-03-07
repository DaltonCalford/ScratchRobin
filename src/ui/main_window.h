#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <memory>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QToolBar;
class QStatusBar;
class QAction;
class QMenu;
class QMenuBar;
class QTextEdit;
class QDockWidget;
class QLabel;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
class ScratchbirdConnection;
class ConnectionInfo;
}

namespace scratchrobin::ui {

class ProjectNavigator;
class DataGrid;
class SqlEditor;
class QueryMenu;
class TransactionMenu;
class WindowMenu;
class ToolsMenu;
class DockWorkspace;
class ViewManagerPanel;
class TriggerManagerPanel;
class GitIntegrationPanel;
class ReportManagerPanel;
class DashboardManagerPanel;
class SchemaComparePanel;
class SchemaMigrationToolPanel;
class DataModelerPanel;
class ERDiagramWidget;
class QueryBuilderPanel;
class QueryPlanVisualizerPanel;
class SqlDebuggerPanel;
class ProcedureDebuggerPanel;
class SqlProfilerPanel;
class CodeFormatterPanel;
class PerformanceDashboardPanel;
class SlowQueryLogViewerPanel;
class DatabaseHealthCheckPanel;
class AlertNotificationSystemPanel;
class UserManagementPanel;
class RoleManagementPanel;
class PermissionBrowserPanel;
class AuditLogViewerPanel;
class SSLCertificateManagerPanel;
class BackupManagerPanel;
class ScheduledJobsPanel;
class VacuumAnalyzePanel;
class DataGeneratorPanel;
class DataMaskingPanel;
class DataCleansingPanel;
class DataLineagePanel;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(backend::SessionClient* session_client, QWidget* parent = nullptr);
  ~MainWindow() override;

  void executeSql(const QString& sql);
  void showResults(const QList<QStringList>& data, const QStringList& headers);
  void showError(const QString& message);
  void showStatusMessage(const QString& message, int timeout = 3000);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private slots:
  void onFileNewConnection();
  void onFileOpenSql();
  void onFileSave();
  void onFileSaveAs();
  void onFileExit();
  void onEditUndo();
  void onEditRedo();
  void onEditCut();
  void onEditCopy();
  void onEditPaste();
  void onEditFind();
  void onEditFindReplace();
  void onEditFindNext();
  void onEditFindPrevious();
  void onEditPreferences();
  void onViewRefresh();
  void onDbConnect();
  void onDbDisconnect();
  void onDbBackup();
  void onDbRestore();
  void onToolsMigrationWizard();
  void onToolsERDiagram();
  void onToolsDocGenerator();
  void onToolsToolbarEditor();
  void onToolsMenuEditor();
  void onToolsThemeSettings();
  void onHelpShortcuts();
  void onHelpTipOfDay();
  void onHelpAbout();
  
  // Query menu slots
  void onQueryExecute();
  void onQueryExecuteSelection();
  void onQueryExecuteScript();
  void onQueryStop();
  void onQueryExplain();
  void onQueryExplainAnalyze();
  void onQueryFormatSql();
  void onQueryComment();
  void onQueryUncomment();
  void onQueryToggleComment();
  void onQueryUppercase();
  void onQueryLowercase();
  
  // Transaction menu slots
  void onTransactionStart();
  void onTransactionCommit();
  void onTransactionRollback();
  void onTransactionSavepoint();
  void onTransactionAutoCommit(bool enabled);
  void onTransactionIsolationLevel(const QString& level);
  void onTransactionReadOnly(bool enabled);
  
  // Window menu slots
  void onWindowNew();
  void onWindowClose();
  void onWindowCloseAll();
  void onWindowNext();
  void onWindowPrevious();
  void onWindowCascade();
  void onWindowTileHorizontal();
  void onWindowTileVertical();
  void onWindowActivated(int index);
  
  // Tools menu slots
  void onToolsGenerateDdl();
  void onToolsCompareSchemas();
  void onToolsCompareData();
  void onToolsSchemaCompare();
  void onToolsSchemaMigration();
  void onToolsDataModeler();
  void onToolsERDiagramDesigner();
  void onToolsQueryBuilder();
  void onToolsQueryPlanVisualizer();
  void onToolsSqlDebugger();
  void onToolsProcedureDebugger();
  void onToolsSqlProfiler();
  void onToolsCodeFormatter();
  void onToolsPerformanceDashboard();
  void onToolsSlowQueryLog();
  void onToolsHealthCheck();
  void onToolsAlertSystem();
  void onToolsImportSql();
  void onToolsImportCsv();
  void onToolsImportJson();
  void onToolsImportExcel();
  void onToolsExportSql();
  void onToolsExportCsv();
  void onToolsExportJson();
  void onToolsExportExcel();
  void onToolsExportXml();
  void onToolsExportHtml();
  void onToolsMigration();
  void onToolsScheduler();
  void onToolsMonitorConnections();
  void onToolsMonitorTransactions();
  void onToolsMonitorStatements();
  void onToolsMonitorLocks();
  void onToolsMonitorPerformance();
  void createSqlEditorTab(const QString& title = QString());
  
  // Dock window actions
  void onViewNavigator();
  void onViewResults();
  void onViewEditor();
  
  // Window layout actions
  void onWindowSaveLayout();
  void onWindowLoadLayout();
  void onWindowResetLayout();
  void onWindowManageLayouts();
  
  // Security management actions
  void onToolsUserManagement();
  void onToolsRoleManagement();
  void onToolsPermissionBrowser();
  void onToolsAuditLogViewer();
  void onToolsSSLCertificateManager();
  
  // Maintenance and backup actions
  void onToolsBackupManager();
  void onToolsScheduledJobs();
  void onToolsVacuumAnalyze();
  
  // Data management actions
  void onToolsDataGenerator();
  void onToolsDataMasking();
  void onToolsDataCleansing();
  void onToolsDataLineage();
  void onToolsTeamSharing();
  void onToolsQueryComments();
  void onToolsChangeTracking();
  void onToolsPluginManager();
  void onToolsMacroRecording();
  void onToolsScriptingConsole();
  
  // New feature menu actions
  void onViewViewManager();
  void onViewTriggerManager();
  void onViewGitIntegration();
  void onViewReportManager();
  void onViewDashboardManager();
  void onViewTableDesigner();
  void onViewSqlHistory();
  void onViewQueryFavorites();
  
  // Dashboard open handler
  void onDashboardOpenRequested(const QString& dashboardId);

 private:
  void setupUi();
  void createMenus();
  void createToolbars();
  void createStatusBar();
  void createDockWindows();
  void setupDockWorkspace();
  void setupConnections();
  
  SqlEditor* currentEditor() const;
  void executeCurrentEditor();

  backend::SessionClient* session_client_;
  std::unique_ptr<backend::ScratchbirdConnection> db_connection_;
  
  // Dock Workspace (new)
  DockWorkspace* dock_workspace_;
  
  // New manager panels (integrated via DockWorkspace)
  ViewManagerPanel* view_manager_panel_;
  TriggerManagerPanel* trigger_manager_panel_;
  GitIntegrationPanel* git_integration_panel_;
  ReportManagerPanel* report_manager_panel_;
  DashboardManagerPanel* dashboard_manager_panel_;
  
  // Legacy dock widgets (migrating to DockWorkspace)
  QDockWidget* navigator_dock_;
  QDockWidget* results_dock_;
  
  // Central widget components
  QTabWidget* editor_tabs_;
  
  // Dock contents
  ProjectNavigator* navigator_;
  DataGrid* results_grid_;
  
  // Status bar widgets
  QLabel* status_label_;
  QLabel* connection_label_;
  QLabel* row_count_label_;
  
  QMenu *file_menu_, *edit_menu_, *view_menu_, *db_menu_, *tools_menu_, *help_menu_, *window_menu_;
  QToolBar *main_toolbar_, *db_toolbar_, *view_toolbar_;
  
  QAction *action_new_connection_, *action_open_sql_, *action_save_, *action_exit_;
  QAction *action_undo_, *action_redo_, *action_cut_, *action_copy_, *action_paste_;
  QAction *action_refresh_, *action_connect_, *action_disconnect_, *action_backup_;
  QAction *action_navigator_, *action_results_, *action_editor_;
  QAction *action_toolbar_editor_, *action_menu_editor_;
  
  // New menu objects
  QueryMenu* query_menu_obj_;
  TransactionMenu* transaction_menu_obj_;
  WindowMenu* window_menu_obj_;
  ToolsMenu* tools_menu_ext_;
};

}  // namespace scratchrobin::ui
