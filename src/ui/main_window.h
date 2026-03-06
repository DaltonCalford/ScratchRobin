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
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

class ProjectNavigator;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(backend::SessionClient* session_client, QWidget* parent = nullptr);
  ~MainWindow() override;

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
  void onEditPreferences();
  void onViewRefresh();
  void onDbConnect();
  void onDbDisconnect();
  void onDbBackup();
  void onDbRestore();
  void onToolsQueryBuilder();
  void onToolsMigrationWizard();
  void onHelpAbout();

 private:
  void setupUi();
  void createMenus();
  void createToolbars();
  void createStatusBar();
  void setupConnections();

  backend::SessionClient* session_client_;
  QSplitter* central_splitter_;
  ProjectNavigator* navigator_;
  QTabWidget* editor_tabs_;
  
  QMenu *file_menu_, *edit_menu_, *view_menu_, *db_menu_, *tools_menu_, *help_menu_;
  QToolBar *main_toolbar_, *db_toolbar_;
  
  QAction *action_new_connection_, *action_open_sql_, *action_save_, *action_exit_;
  QAction *action_undo_, *action_redo_, *action_cut_, *action_copy_, *action_paste_;
  QAction *action_refresh_, *action_connect_, *action_disconnect_, *action_backup_;
};

}  // namespace scratchrobin::ui
