#include "ui/main_window.h"
#include "ui/project_navigator.h"
#include "backend/session_client.h"

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

namespace scratchrobin::ui {

MainWindow::MainWindow(backend::SessionClient* session_client, QWidget* parent)
    : QMainWindow(parent), session_client_(session_client) {
  setWindowTitle(tr("ScratchRobin - Database Administration Tool"));
  setMinimumSize(1024, 768);
  resize(1280, 800);
  setupUi();
  createMenus();
  createToolbars();
  createStatusBar();
  setupConnections();
  statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
  central_splitter_ = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(central_splitter_);

  navigator_ = new ProjectNavigator(this);
  navigator_->setMinimumWidth(250);
  navigator_->setMaximumWidth(400);
  central_splitter_->addWidget(navigator_);

  editor_tabs_ = new QTabWidget(this);
  editor_tabs_->setTabsClosable(true);
  editor_tabs_->setMovable(true);
  central_splitter_->addWidget(editor_tabs_);

  central_splitter_->setStretchFactor(0, 1);
  central_splitter_->setStretchFactor(1, 3);
}

void MainWindow::createMenus() {
  file_menu_ = menuBar()->addMenu(tr("&File"));
  action_new_connection_ = file_menu_->addAction(tr("&New Connection..."), this, &MainWindow::onFileNewConnection, QKeySequence::New);
  action_open_sql_ = file_menu_->addAction(tr("&Open SQL Script..."), this, &MainWindow::onFileOpenSql, QKeySequence::Open);
  file_menu_->addSeparator();
  action_save_ = file_menu_->addAction(tr("&Save"), this, &MainWindow::onFileSave, QKeySequence::Save);
  file_menu_->addAction(tr("Save &As..."), this, &MainWindow::onFileSaveAs, QKeySequence::SaveAs);
  file_menu_->addSeparator();
  action_exit_ = file_menu_->addAction(tr("E&xit"), this, &MainWindow::onFileExit, QKeySequence::Quit);

  edit_menu_ = menuBar()->addMenu(tr("&Edit"));
  action_undo_ = edit_menu_->addAction(tr("&Undo"), this, &MainWindow::onEditUndo, QKeySequence::Undo);
  action_redo_ = edit_menu_->addAction(tr("&Redo"), this, &MainWindow::onEditRedo, QKeySequence::Redo);
  edit_menu_->addSeparator();
  action_cut_ = edit_menu_->addAction(tr("Cu&t"), this, &MainWindow::onEditCut, QKeySequence::Cut);
  action_copy_ = edit_menu_->addAction(tr("&Copy"), this, &MainWindow::onEditCopy, QKeySequence::Copy);
  action_paste_ = edit_menu_->addAction(tr("&Paste"), this, &MainWindow::onEditPaste, QKeySequence::Paste);
  edit_menu_->addSeparator();
  edit_menu_->addAction(tr("&Preferences..."), this, &MainWindow::onEditPreferences);

  view_menu_ = menuBar()->addMenu(tr("&View"));
  action_refresh_ = view_menu_->addAction(tr("&Refresh"), this, &MainWindow::onViewRefresh, QKeySequence::Refresh);

  db_menu_ = menuBar()->addMenu(tr("&Database"));
  action_connect_ = db_menu_->addAction(tr("&Connect..."), this, &MainWindow::onDbConnect);
  action_disconnect_ = db_menu_->addAction(tr("&Disconnect"), this, &MainWindow::onDbDisconnect);
  db_menu_->addSeparator();
  action_backup_ = db_menu_->addAction(tr("&Backup..."), this, &MainWindow::onDbBackup);
  db_menu_->addAction(tr("&Restore..."), this, &MainWindow::onDbRestore);

  tools_menu_ = menuBar()->addMenu(tr("&Tools"));
  tools_menu_->addAction(tr("&Query Builder..."), this, &MainWindow::onToolsQueryBuilder);
  tools_menu_->addAction(tr("&Migration Wizard..."), this, &MainWindow::onToolsMigrationWizard);

  help_menu_ = menuBar()->addMenu(tr("&Help"));
  help_menu_->addAction(tr("&Documentation"), this, []() { QDesktopServices::openUrl(QUrl("https://scratchbird.dev/docs")); });
  help_menu_->addSeparator();
  help_menu_->addAction(tr("&About ScratchRobin"), this, &MainWindow::onHelpAbout);
}

void MainWindow::createToolbars() {
  main_toolbar_ = addToolBar(tr("Main"));
  main_toolbar_->addAction(action_new_connection_);
  main_toolbar_->addAction(action_open_sql_);
  main_toolbar_->addSeparator();
  main_toolbar_->addAction(action_save_);
  main_toolbar_->addSeparator();
  main_toolbar_->addAction(action_undo_);
  main_toolbar_->addAction(action_redo_);

  db_toolbar_ = addToolBar(tr("Database"));
  db_toolbar_->addAction(action_connect_);
  db_toolbar_->addAction(action_disconnect_);
  db_toolbar_->addSeparator();
  db_toolbar_->addAction(action_backup_);
  db_toolbar_->addSeparator();
  db_toolbar_->addAction(action_refresh_);
}

void MainWindow::createStatusBar() {
  statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupConnections() {
  connect(editor_tabs_, &QTabWidget::tabCloseRequested, [this](int index) { editor_tabs_->removeTab(index); });
}

void MainWindow::closeEvent(QCloseEvent* event) {
  event->accept();
}

void MainWindow::onFileNewConnection() { statusBar()->showMessage(tr("New Connection..."), 2000); }
void MainWindow::onFileOpenSql() {
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open SQL Script"), QString(), tr("SQL Files (*.sql);;All Files (*)"));
  if (!fileName.isEmpty()) {
    auto* editor = new QTextEdit(this);
    editor->setPlainText(tr("-- Opened: %1").arg(fileName));
    editor_tabs_->addTab(editor, QFileInfo(fileName).fileName());
    editor_tabs_->setCurrentWidget(editor);
  }
}
void MainWindow::onFileSave() { statusBar()->showMessage(tr("Save"), 2000); }
void MainWindow::onFileSaveAs() { statusBar()->showMessage(tr("Save As..."), 2000); }
void MainWindow::onFileExit() { close(); }

void MainWindow::onEditUndo() { if (auto* editor = qobject_cast<QTextEdit*>(editor_tabs_->currentWidget())) editor->undo(); }
void MainWindow::onEditRedo() { if (auto* editor = qobject_cast<QTextEdit*>(editor_tabs_->currentWidget())) editor->redo(); }
void MainWindow::onEditCut() { if (auto* editor = qobject_cast<QTextEdit*>(editor_tabs_->currentWidget())) editor->cut(); }
void MainWindow::onEditCopy() { if (auto* editor = qobject_cast<QTextEdit*>(editor_tabs_->currentWidget())) editor->copy(); }
void MainWindow::onEditPaste() { if (auto* editor = qobject_cast<QTextEdit*>(editor_tabs_->currentWidget())) editor->paste(); }
void MainWindow::onEditPreferences() { QMessageBox::information(this, tr("Preferences"), tr("Preferences dialog not yet implemented.")); }

void MainWindow::onViewRefresh() { navigator_->refresh(); statusBar()->showMessage(tr("Refreshed"), 2000); }

void MainWindow::onDbConnect() { statusBar()->showMessage(tr("Connect..."), 2000); }
void MainWindow::onDbDisconnect() { statusBar()->showMessage(tr("Disconnect"), 2000); }
void MainWindow::onDbBackup() { statusBar()->showMessage(tr("Backup..."), 2000); }
void MainWindow::onDbRestore() { statusBar()->showMessage(tr("Restore..."), 2000); }

void MainWindow::onToolsQueryBuilder() { statusBar()->showMessage(tr("Query Builder..."), 2000); }
void MainWindow::onToolsMigrationWizard() { statusBar()->showMessage(tr("Migration Wizard..."), 2000); }

void MainWindow::onHelpAbout() {
  QMessageBox::about(this, tr("About ScratchRobin"),
    tr("<h2>ScratchRobin v0.1.0</h2>"
       "<p>A native database administration client for ScratchBird.</p>"
       "<p>Built with Qt6</p>"
       "<p>Copyright 2025-2026 Dalton Calford</p>"));
}

}  // namespace scratchrobin::ui
