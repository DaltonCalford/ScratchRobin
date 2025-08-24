#include "main_window.h"
#include "core/application.h"
#include "utils/logger.h"
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include "connection_dialog.h"
#include "about_dialog.h"
#include "preferences_dialog.h"
#include "update_dialog.h"
#include "import_export_dialog.h"
#include "query_history_dialog.h"
#include "backup_restore_dialog.h"
#include "find_replace_dialog.h"
#include "column_editor_dialog.h"
#include "data_editor_dialog.h"
#include "progress_dialog.h"
#include "error_details_dialog.h"
#include "confirmation_dialog.h"
#include "keyboard_shortcuts_dialog.h"
#include "favorites_manager_dialog.h"
#include "dynamic_connection_dialog.h"
#include "app_icons.h"
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>
#include <QLineEdit>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QThread>

namespace scratchrobin {

class MainWindow::Impl : public QMainWindow {
public:
    Impl(Application* app) : application_(app), databaseDriverManager(&DatabaseDriverManager::instance()) {
        setupUI();
    }

    // Database connectivity
    DatabaseDriverManager* databaseDriverManager;

    // No component references needed for placeholders

    void setupUI() {
        // Set window properties
        setWindowTitle("ScratchRobin Database GUI");
        setWindowIcon(AppIcons::instance().getAppIcon());
        setMinimumSize(800, 600);
        resize(1024, 768);

        // Create central widget
        QWidget* centralWidget = new QWidget();
        setCentralWidget(centralWidget);

        // Create main layout
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

        // Create tab widget for different views
        QTabWidget* tabWidget = new QTabWidget();
        mainLayout->addWidget(tabWidget);

        // Create Query Tab - Text Editor + Result Viewer
        QWidget* queryTab = new QWidget();
        QVBoxLayout* queryLayout = new QVBoxLayout(queryTab);

        // Create splitter for query editor and results
        QSplitter* querySplitter = new QSplitter(Qt::Vertical);

        // Text editor for SQL queries
        QTextEdit* sqlEditor = new QTextEdit();
        sqlEditor->setPlaceholderText("Enter your SQL query here...\n\nExample:\nSELECT * FROM users;\n\nOr:\nCREATE TABLE example (\n  id INTEGER PRIMARY KEY,\n  name TEXT\n);");
        sqlEditor->setFont(QFont("Monaco", 12));
        sqlEditor->setStyleSheet("QTextEdit { background-color: #f8f8f8; border: 1px solid #ccc; }");
        querySplitter->addWidget(sqlEditor);

        // Control buttons
        QWidget* buttonWidget = new QWidget();
        QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);

        QPushButton* executeButton = new QPushButton("Execute Query");
        executeButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px 16px; border: none; border-radius: 4px; } QPushButton:hover { background-color: #45a049; }");
        buttonLayout->addWidget(executeButton);

        QPushButton* clearButton = new QPushButton("Clear");
        clearButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px 16px; border: none; border-radius: 4px; } QPushButton:hover { background-color: #d32f2f; }");
        buttonLayout->addWidget(clearButton);

        buttonLayout->addStretch();
        queryLayout->addWidget(buttonWidget);

        // Result viewer for query results
        QTableWidget* resultTable = new QTableWidget();
        resultTable->setAlternatingRowColors(true);
        resultTable->setStyleSheet("QTableWidget { border: 1px solid #ccc; } QHeaderView::section { background-color: #e0e0e0; padding: 4px; border: 1px solid #ccc; }");

        // Add some sample data
        resultTable->setColumnCount(3);
        resultTable->setHorizontalHeaderLabels({"ID", "Name", "Email"});
        resultTable->setRowCount(3);

        resultTable->setItem(0, 0, new QTableWidgetItem("1"));
        resultTable->setItem(0, 1, new QTableWidgetItem("John Doe"));
        resultTable->setItem(0, 2, new QTableWidgetItem("john@example.com"));

        resultTable->setItem(1, 0, new QTableWidgetItem("2"));
        resultTable->setItem(1, 1, new QTableWidgetItem("Jane Smith"));
        resultTable->setItem(1, 2, new QTableWidgetItem("jane@example.com"));

        resultTable->setItem(2, 0, new QTableWidgetItem("3"));
        resultTable->setItem(2, 1, new QTableWidgetItem("Bob Wilson"));
        resultTable->setItem(2, 2, new QTableWidgetItem("bob@example.com"));

        querySplitter->addWidget(resultTable);

        // Set initial splitter sizes
        querySplitter->setSizes({400, 200});
        queryLayout->addWidget(querySplitter);

        tabWidget->addTab(queryTab, "Query");

        // Create Objects Tab - Object Browser + Property Viewer
        QWidget* objectTab = new QWidget();
        QVBoxLayout* objectLayout = new QVBoxLayout(objectTab);

        // Create splitter for object browser and property viewer
        QSplitter* objectSplitter = new QSplitter(Qt::Horizontal);

        // Object browser tree
        QTreeWidget* objectTree = new QTreeWidget();
        objectTree->setHeaderLabel("Database Objects");
        objectTree->setAlternatingRowColors(true);
        objectTree->setStyleSheet("QTreeWidget { border: 1px solid #ccc; } QHeaderView::section { background-color: #e0e0e0; padding: 4px; border: 1px solid #ccc; }");

        // Add sample database objects
        QTreeWidgetItem* rootItem = new QTreeWidgetItem(objectTree);
        rootItem->setText(0, "ScratchRobin Database");

        QTreeWidgetItem* schemasItem = new QTreeWidgetItem(rootItem);
        schemasItem->setText(0, "Schemas");

        QTreeWidgetItem* publicSchema = new QTreeWidgetItem(schemasItem);
        publicSchema->setText(0, "public");

        QTreeWidgetItem* tablesItem = new QTreeWidgetItem(publicSchema);
        tablesItem->setText(0, "Tables");

        QTreeWidgetItem* usersTable = new QTreeWidgetItem(tablesItem);
        usersTable->setText(0, "users");

        QTreeWidgetItem* productsTable = new QTreeWidgetItem(tablesItem);
        productsTable->setText(0, "products");

        QTreeWidgetItem* viewsItem = new QTreeWidgetItem(publicSchema);
        viewsItem->setText(0, "Views");

        QTreeWidgetItem* userView = new QTreeWidgetItem(viewsItem);
        userView->setText(0, "active_users");

        objectTree->expandAll();
        objectSplitter->addWidget(objectTree);

        // Property viewer
        QTableWidget* propertyTable = new QTableWidget();
        propertyTable->setColumnCount(2);
        propertyTable->setHorizontalHeaderLabels({"Property", "Value"});
        propertyTable->setAlternatingRowColors(true);
        propertyTable->setStyleSheet("QTableWidget { border: 1px solid #ccc; } QHeaderView::section { background-color: #e0e0e0; padding: 4px; border: 1px solid #ccc; }");

        // Add sample properties
        propertyTable->setRowCount(8);
        propertyTable->setItem(0, 0, new QTableWidgetItem("Name"));
        propertyTable->setItem(0, 1, new QTableWidgetItem("users"));

        propertyTable->setItem(1, 0, new QTableWidgetItem("Type"));
        propertyTable->setItem(1, 1, new QTableWidgetItem("Table"));

        propertyTable->setItem(2, 0, new QTableWidgetItem("Schema"));
        propertyTable->setItem(2, 1, new QTableWidgetItem("public"));

        propertyTable->setItem(3, 0, new QTableWidgetItem("Owner"));
        propertyTable->setItem(3, 1, new QTableWidgetItem("postgres"));

        propertyTable->setItem(4, 0, new QTableWidgetItem("Row Count"));
        propertyTable->setItem(4, 1, new QTableWidgetItem("1,234"));

        propertyTable->setItem(5, 0, new QTableWidgetItem("Size"));
        propertyTable->setItem(5, 1, new QTableWidgetItem("64 KB"));

        propertyTable->setItem(6, 0, new QTableWidgetItem("Created"));
        propertyTable->setItem(6, 1, new QTableWidgetItem("2025-01-15"));

        propertyTable->setItem(7, 0, new QTableWidgetItem("Modified"));
        propertyTable->setItem(7, 1, new QTableWidgetItem("2025-08-23"));

        objectSplitter->addWidget(propertyTable);

        objectLayout->addWidget(objectSplitter);
        tabWidget->addTab(objectTab, "Objects");

        // Create Tables Tab - Table Designer
        QWidget* tableTab = new QWidget();
        QVBoxLayout* tableLayout = new QVBoxLayout(tableTab);

        // Table properties section
        QGroupBox* tableGroup = new QGroupBox("Table Properties");
        QFormLayout* tableForm = new QFormLayout(tableGroup);

        QLineEdit* tableNameEdit = new QLineEdit();
        tableNameEdit->setPlaceholderText("Enter table name...");
        tableForm->addRow(QString("Table Name:"), tableNameEdit);

        QComboBox* schemaCombo = new QComboBox();
        schemaCombo->addItems({"public", "private", "admin"});
        tableForm->addRow(QString("Schema:"), schemaCombo);

        QLineEdit* descriptionEdit = new QLineEdit();
        descriptionEdit->setPlaceholderText("Optional table description...");
        tableForm->addRow(QString("Description:"), descriptionEdit);

        tableLayout->addWidget(tableGroup);

        // Columns section
        QGroupBox* columnsGroup = new QGroupBox("Columns");
        QVBoxLayout* columnsLayout = new QVBoxLayout(columnsGroup);

        // Column list
        QTableWidget* columnsTable = new QTableWidget();
        columnsTable->setColumnCount(5);
        columnsTable->setHorizontalHeaderLabels({"Name", "Type", "Nullable", "Default", "Primary Key"});
        columnsTable->setAlternatingRowColors(true);
        columnsTable->setStyleSheet("QTableWidget { border: 1px solid #ccc; }");

        // Add some sample columns
        columnsTable->setRowCount(3);

        columnsTable->setItem(0, 0, new QTableWidgetItem("id"));
        columnsTable->setItem(0, 1, new QTableWidgetItem("INTEGER"));
        columnsTable->setItem(0, 2, new QTableWidgetItem("No"));
        columnsTable->setItem(0, 3, new QTableWidgetItem("AUTO"));
        columnsTable->setItem(0, 4, new QTableWidgetItem("Yes"));

        columnsTable->setItem(1, 0, new QTableWidgetItem("name"));
        columnsTable->setItem(1, 1, new QTableWidgetItem("VARCHAR(100)"));
        columnsTable->setItem(1, 2, new QTableWidgetItem("No"));
        columnsTable->setItem(1, 3, new QTableWidgetItem(""));
        columnsTable->setItem(1, 4, new QTableWidgetItem("No"));

        columnsTable->setItem(2, 0, new QTableWidgetItem("email"));
        columnsTable->setItem(2, 1, new QTableWidgetItem("VARCHAR(255)"));
        columnsTable->setItem(2, 2, new QTableWidgetItem("Yes"));
        columnsTable->setItem(2, 3, new QTableWidgetItem(""));
        columnsTable->setItem(2, 4, new QTableWidgetItem("No"));

        columnsLayout->addWidget(columnsTable);

        // Column buttons
        QWidget* columnButtons = new QWidget();
        QHBoxLayout* columnButtonsLayout = new QHBoxLayout(columnButtons);

        QPushButton* addColumnBtn = new QPushButton("Add Column");
        addColumnBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 6px 12px; border: none; border-radius: 3px; }");

        QPushButton* removeColumnBtn = new QPushButton("Remove Column");
        removeColumnBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 6px 12px; border: none; border-radius: 3px; }");

        columnButtonsLayout->addWidget(addColumnBtn);
        columnButtonsLayout->addWidget(removeColumnBtn);
        columnButtonsLayout->addStretch();

        columnsLayout->addWidget(columnButtons);
        tableLayout->addWidget(columnsGroup);

        // DDL preview section
        QGroupBox* ddlGroup = new QGroupBox("Generated DDL");
        QVBoxLayout* ddlLayout = new QVBoxLayout(ddlGroup);

        QTextEdit* ddlPreview = new QTextEdit();
        ddlPreview->setPlainText("-- Generated SQL DDL\nCREATE TABLE users (\n  id INTEGER PRIMARY KEY AUTO_INCREMENT,\n  name VARCHAR(100) NOT NULL,\n  email VARCHAR(255)\n);");
        ddlPreview->setFont(QFont("Monaco", 10));
        ddlPreview->setMaximumHeight(150);
        ddlPreview->setStyleSheet("QTextEdit { background-color: #f5f5f5; border: 1px solid #ccc; }");
        ddlLayout->addWidget(ddlPreview);

        tableLayout->addWidget(ddlGroup);

        // Action buttons
        QWidget* actionButtons = new QWidget();
        QHBoxLayout* actionButtonsLayout = new QHBoxLayout(actionButtons);

        QPushButton* generateBtn = new QPushButton("Generate DDL");
        generateBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px 16px; border: none; border-radius: 4px; font-weight: bold; }");

        QPushButton* createBtn = new QPushButton("Create Table");
        createBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; padding: 8px 16px; border: none; border-radius: 4px; font-weight: bold; }");

        actionButtonsLayout->addStretch();
        actionButtonsLayout->addWidget(generateBtn);
        actionButtonsLayout->addWidget(createBtn);

        tableLayout->addWidget(actionButtons);

        tabWidget->addTab(tableTab, "Tables");

        // Setup menu bar
        setupMenuBar();

        // Setup status bar
        setupStatusBar();

        Logger::info("MainWindow UI setup completed");
    }

    void setupMenuBar() {
        // File menu
        QMenu* fileMenu = menuBar()->addMenu("&File");

        QAction* newAction = new QAction(AppIcons::instance().getNewIcon(), "&New", this);
        newAction->setShortcut(QKeySequence::New);
        newAction->setStatusTip("Create a new database connection");
        fileMenu->addAction(newAction);

        QAction* openAction = new QAction(AppIcons::instance().getOpenIcon(), "&Open", this);
        openAction->setShortcut(QKeySequence::Open);
        openAction->setStatusTip("Open existing file or project");
        fileMenu->addAction(openAction);

        QAction* connectAction = new QAction(AppIcons::instance().getConnectIcon(), "&Connect to Database", this);
        connectAction->setShortcut(QKeySequence("Ctrl+D"));
        connectAction->setStatusTip("Connect to a database");
        connect(connectAction, &QAction::triggered, [this]() {
            showConnectionDialog();
        });
        fileMenu->addAction(connectAction);

        // Recent connections submenu
        recentConnectionsMenu_ = new QMenu("&Recent Connections", this);
        recentConnectionsMenu_->setIcon(AppIcons::instance().getConnectIcon());
        updateRecentConnectionsMenu();
        fileMenu->addMenu(recentConnectionsMenu_);

        fileMenu->addSeparator();

        // Import/Export actions
        QAction* importAction = new QAction(AppIcons::instance().getOpenIcon(), "&Import...", this);
        importAction->setShortcut(QKeySequence("Ctrl+I"));
        importAction->setStatusTip("Import data from file");
        connect(importAction, &QAction::triggered, [this]() {
            showImportExportDialog(true);
        });
        fileMenu->addAction(importAction);

        QAction* exportAction = new QAction(AppIcons::instance().getSaveIcon(), "&Export...", this);
        exportAction->setShortcut(QKeySequence("Ctrl+E"));
        exportAction->setStatusTip("Export data to file");
        connect(exportAction, &QAction::triggered, [this]() {
            showImportExportDialog(false);
        });
        fileMenu->addAction(exportAction);

        QAction* preferencesAction = new QAction(AppIcons::instance().getSettingsIcon(), "&Preferences...", this);
        preferencesAction->setShortcut(QKeySequence("Ctrl+,"));
        preferencesAction->setStatusTip("Open application preferences");
        connect(preferencesAction, &QAction::triggered, [this]() {
            showPreferencesDialog();
        });
        fileMenu->addAction(preferencesAction);

        fileMenu->addSeparator();

        QAction* exitAction = new QAction("E&xit", this);
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
        fileMenu->addAction(exitAction);

        // View menu
        QMenu* viewMenu = menuBar()->addMenu("&View");

        QAction* queryHistoryAction = new QAction(AppIcons::instance().getOpenIcon(), "&Query History", this);
        queryHistoryAction->setShortcut(QKeySequence("Ctrl+H"));
        queryHistoryAction->setStatusTip("View query execution history");
        connect(queryHistoryAction, &QAction::triggered, [this]() {
            showQueryHistoryDialog();
        });
        viewMenu->addAction(queryHistoryAction);

        viewMenu->addSeparator();

        QAction* refreshAction = new QAction(AppIcons::instance().getRefreshIcon(), "&Refresh", this);
        refreshAction->setShortcut(QKeySequence("F5"));
        refreshAction->setStatusTip("Refresh current view");
        viewMenu->addAction(refreshAction);

        // Tools menu
        QMenu* toolsMenu = menuBar()->addMenu("&Tools");

        QAction* backupRestoreAction = new QAction(AppIcons::instance().getSaveIcon(), "&Backup & Restore", this);
        backupRestoreAction->setShortcut(QKeySequence("Ctrl+B"));
        backupRestoreAction->setStatusTip("Create and restore database backups");
        connect(backupRestoreAction, &QAction::triggered, [this]() {
            showBackupRestoreDialog();
        });
        toolsMenu->addAction(backupRestoreAction);

        toolsMenu->addSeparator();

        QAction* columnEditorAction = new QAction(AppIcons::instance().getTableIcon(), "&Column Editor...", this);
        columnEditorAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
        columnEditorAction->setStatusTip("Create or edit database table columns");
        connect(columnEditorAction, &QAction::triggered, [this]() {
            showColumnEditorDialog();
        });
        toolsMenu->addAction(columnEditorAction);

        QAction* dataEditorAction = new QAction(AppIcons::instance().getTableIcon(), "&Data Editor...", this);
        dataEditorAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
        dataEditorAction->setStatusTip("View and edit table data");
        connect(dataEditorAction, &QAction::triggered, [this]() {
            showDataEditorDialog();
        });
        toolsMenu->addAction(dataEditorAction);

        toolsMenu->addSeparator();

        QAction* progressDemoAction = new QAction(AppIcons::instance().getRefreshIcon(), "&Progress Demo...", this);
        progressDemoAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
        progressDemoAction->setStatusTip("Demonstrate progress dialog functionality");
        connect(progressDemoAction, &QAction::triggered, [this]() {
            showProgressDialogDemo();
        });
        toolsMenu->addAction(progressDemoAction);

        QAction* errorDemoAction = new QAction(AppIcons::instance().getErrorIcon(), "&Error Details Demo...", this);
        errorDemoAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
        errorDemoAction->setStatusTip("Demonstrate error details dialog functionality");
        connect(errorDemoAction, &QAction::triggered, [this]() {
            showErrorDetailsDialogDemo();
        });
        toolsMenu->addAction(errorDemoAction);

        QAction* confirmationDemoAction = new QAction(AppIcons::instance().getWarningIcon(), "&Confirmation Demo...", this);
        confirmationDemoAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
        confirmationDemoAction->setStatusTip("Demonstrate confirmation dialog functionality");
        connect(confirmationDemoAction, &QAction::triggered, [this]() {
            showConfirmationDialogDemo();
        });
        toolsMenu->addAction(confirmationDemoAction);

        QAction* keyboardShortcutsAction = new QAction(AppIcons::instance().getSettingsIcon(), "&Keyboard Shortcuts...", this);
        keyboardShortcutsAction->setShortcut(QKeySequence("Ctrl+K"));
        keyboardShortcutsAction->setStatusTip("Configure keyboard shortcuts");
        connect(keyboardShortcutsAction, &QAction::triggered, [this]() {
            showKeyboardShortcutsDialog();
        });
        toolsMenu->addAction(keyboardShortcutsAction);

        QAction* favoritesManagerAction = new QAction(AppIcons::instance().getSettingsIcon(), "&Query Favorites...", this);
        favoritesManagerAction->setShortcut(QKeySequence("Ctrl+F"));
        favoritesManagerAction->setStatusTip("Manage query favorites and bookmarks");
        connect(favoritesManagerAction, &QAction::triggered, [this]() {
            showFavoritesManagerDialog();
        });
        toolsMenu->addAction(favoritesManagerAction);

        // Edit menu
        QMenu* editMenu = menuBar()->addMenu("&Edit");

        QAction* findAction = new QAction(AppIcons::instance().getFindIcon(), "&Find...", this);
        findAction->setShortcut(QKeySequence::Find);
        findAction->setStatusTip("Find text in the current query");
        connect(findAction, &QAction::triggered, [this]() {
            showFindReplaceDialog(false);
        });
        editMenu->addAction(findAction);

        QAction* replaceAction = new QAction(AppIcons::instance().getReplaceIcon(), "&Replace...", this);
        replaceAction->setShortcut(QKeySequence("Ctrl+H"));
        replaceAction->setStatusTip("Find and replace text in the current query");
        connect(replaceAction, &QAction::triggered, [this]() {
            showFindReplaceDialog(true);
        });
        editMenu->addAction(replaceAction);

        editMenu->addSeparator();

        QAction* undoAction = new QAction("&Undo", this);
        undoAction->setShortcut(QKeySequence::Undo);
        editMenu->addAction(undoAction);

        QAction* redoAction = new QAction("&Redo", this);
        redoAction->setShortcut(QKeySequence::Redo);
        editMenu->addAction(redoAction);

        // Help menu
        QMenu* helpMenu = menuBar()->addMenu("&Help");

        QAction* updateAction = new QAction(AppIcons::instance().getRefreshIcon(), "Check for &Updates", this);
        updateAction->setShortcut(QKeySequence("Ctrl+U"));
        updateAction->setStatusTip("Check for software updates");
        connect(updateAction, &QAction::triggered, [this]() {
            showUpdateDialog();
        });
        helpMenu->addAction(updateAction);

        helpMenu->addSeparator();

        QAction* aboutAction = new QAction(AppIcons::instance().getAboutIcon(), "&About ScratchRobin", this);
        aboutAction->setShortcut(QKeySequence("F1"));
        aboutAction->setStatusTip("Show information about ScratchRobin");
        connect(aboutAction, &QAction::triggered, [this]() {
            showAboutDialog();
        });
        helpMenu->addAction(aboutAction);
    }

    void setupStatusBar() {
        // Clear any existing widgets
        statusBar()->clearMessage();

        // Create connection status widget
        connectionStatusWidget_ = new QWidget();
        QHBoxLayout* statusLayout = new QHBoxLayout(connectionStatusWidget_);
        statusLayout->setContentsMargins(0, 0, 0, 0);
        statusLayout->setSpacing(5);

        // Connection indicator icon
        connectionIconLabel_ = new QLabel();
        connectionIconLabel_->setFixedSize(16, 16);
        connectionIconLabel_->setPixmap(AppIcons::instance().getDisconnectIcon().pixmap(16, 16));
        statusLayout->addWidget(connectionIconLabel_);

        // Connection status text
        connectionStatusLabel_ = new QLabel("Not connected");
        connectionStatusLabel_->setStyleSheet("color: #666; font-size: 11px;");
        statusLayout->addWidget(connectionStatusLabel_);

        statusLayout->addStretch();

        // Add to status bar
        statusBar()->addWidget(connectionStatusWidget_, 1);

        // Progress bar (initially hidden)
        progressBar_ = new QProgressBar();
        progressBar_->setVisible(false);
        progressBar_->setRange(0, 100);
        progressBar_->setFixedWidth(200);
        progressBar_->setStyleSheet("QProgressBar { border: 1px solid #ccc; border-radius: 3px; text-align: center; } QProgressBar::chunk { background-color: #4CAF50; }");
        statusBar()->addPermanentWidget(progressBar_);

        // Right side - version info
        versionLabel_ = new QLabel("ScratchRobin v0.1.0");
        versionLabel_->setStyleSheet("color: #888; font-size: 10px; margin-right: 10px;");
        statusBar()->addPermanentWidget(versionLabel_);

        updateConnectionStatus(false, "Not connected");
    }

    void showConnectionDialog() {
        DynamicConnectionDialog dialog(this);

        // Connect to the connection tested signal to provide real-time feedback
        connect(&dialog, &DynamicConnectionDialog::connectionTested, [this](bool success, const QString& message) {
            if (success) {
                statusBar()->showMessage("Connection test successful: " + message, 3000);
            } else {
                statusBar()->showMessage("Connection test failed: " + message, 3000);
            }
        });

        if (dialog.exec() == QDialog::Accepted) {
            DatabaseConnectionConfig config = dialog.getConnectionConfig();

            // Update connection status with detailed information
            QString connectionString = QString("%1@%2:%3/%4 (%5)")
                                     .arg(config.username)
                                     .arg(config.host)
                                     .arg(config.port)
                                     .arg(config.database)
                                     .arg(this->databaseDriverManager->databaseTypeToString(config.databaseType));

            updateConnectionStatus(true, connectionString);

            // Add to recent connections
            QString recentString = QString("%1@%2:%3/%4")
                                 .arg(config.username)
                                 .arg(config.host)
                                 .arg(config.port)
                                 .arg(config.database);

            addToRecentConnections(recentString);

            // Also show temporary message
            statusBar()->showMessage("Successfully connected to database", 3000);
        }
    }

    void showAboutDialog() {
        AboutDialog dialog(this);
        dialog.exec();
    }

    void showPreferencesDialog() {
        PreferencesDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            // Settings have been saved, could trigger application restart or refresh
            statusBar()->showMessage("Preferences saved successfully", 3000);
        }
    }

    void showUpdateDialog() {
        UpdateDialog dialog(this);
        dialog.setCurrentVersion("0.1.0");
        dialog.exec();
    }

    void showImportExportDialog(bool importMode) {
        ImportExportDialog dialog(this);

        // Set current database if connected
        if (!currentDatabase_.isEmpty()) {
            dialog.setCurrentDatabase(currentDatabase_);
        }

        // Set available tables and schemas (in a real implementation, this would come from the metadata manager)
        QStringList sampleTables = QStringList() << "users" << "products" << "orders" << "categories" << "customers";
        QStringList sampleSchemas = QStringList() << "public" << "inventory" << "sales";

        dialog.setAvailableTables(sampleTables);
        dialog.setAvailableSchemas(sampleSchemas);

        // Connect signals
        connect(&dialog, &ImportExportDialog::exportRequested, this, &MainWindow::Impl::handleExportRequest);
        connect(&dialog, &ImportExportDialog::importRequested, this, &MainWindow::Impl::handleImportRequest);
        connect(&dialog, &ImportExportDialog::previewRequested, this, &MainWindow::Impl::handlePreviewRequest);

        // If importMode is true, switch to import tab
        if (importMode) {
            // The dialog will open on the import tab by default when importMode is true
        }

        dialog.exec();
    }

    void showQueryHistoryDialog() {
        QueryHistoryDialog dialog(this);

        // In a real implementation, you would load query history from the database
        // For now, the dialog will use its sample data

        // Connect signals
        connect(&dialog, &QueryHistoryDialog::queryReRun, this, &MainWindow::Impl::handleQueryReRun);
        connect(&dialog, &QueryHistoryDialog::queryDeleted, this, &MainWindow::Impl::handleQueryDeleted);

        dialog.exec();
    }

    void handleQueryReRun(const std::string& query) {
        // In a real implementation, you would send this query to the query editor
        // For now, just show a message
        QMessageBox::information(this, "Query Re-run",
                               QString("Query sent to editor:\n\n%1").arg(QString::fromStdString(query)));
    }

    void handleQueryDeleted(const std::string& id) {
        // In a real implementation, you would delete the query from the database
        QMessageBox::information(this, "Query Deleted",
                               QString("Query %1 has been deleted from history.").arg(QString::fromStdString(id)));
    }

    void showBackupRestoreDialog() {
        BackupRestoreDialog dialog(this);

        // Set current database if connected
        if (!currentDatabase_.isEmpty()) {
            dialog.setCurrentDatabase(currentDatabase_);
        }

        // Set available tables and schemas (in a real implementation, this would come from the metadata manager)
        QStringList sampleTables = QStringList() << "users" << "products" << "orders" << "categories" << "customers";
        QStringList sampleSchemas = QStringList() << "public" << "inventory" << "sales";

        dialog.setAvailableTables(sampleTables);
        dialog.setAvailableSchemas(sampleSchemas);

        // Connect signals
        connect(&dialog, &BackupRestoreDialog::backupRequested, this, &MainWindow::Impl::handleBackupRequest);
        connect(&dialog, &BackupRestoreDialog::restoreRequested, this, &MainWindow::Impl::handleRestoreRequest);
        connect(&dialog, &BackupRestoreDialog::backupVerified, this, &MainWindow::Impl::handleBackupVerified);
        connect(&dialog, &BackupRestoreDialog::backupDeleted, this, &MainWindow::Impl::handleBackupDeleted);

        dialog.exec();
    }

    void handleBackupRequest(const BackupOptions& options) {
        // Show progress bar
        progressBar_->setVisible(true);
        progressBar_->setValue(0);

        // Simulate backup process
        QTimer::singleShot(100, [this, options]() {
            this->progressBar_->setValue(25);

            QTimer::singleShot(500, [this, options]() {
                this->progressBar_->setValue(75);

                QTimer::singleShot(300, [this, options]() {
                    this->progressBar_->setValue(100);

                    // Simulate completion
                    QMessageBox::information(this, "Backup Complete",
                                           QString("Database backup completed successfully!\n\nFile: %1\nType: %2\nFormat: %3\nSize: ~2.3 MB")
                                           .arg(options.filePath)
                                           .arg(options.backupType)
                                           .arg(options.format));

                    this->progressBar_->setVisible(false);
                });
            });
        });
    }

    void handleRestoreRequest(const RestoreOptions& options) {
        // Show progress bar
        progressBar_->setVisible(true);
        progressBar_->setValue(0);

        // Check if preview only
        if (options.previewOnly) {
            QMessageBox::information(this, "Restore Preview",
                                   "Restore would be performed in preview mode.\nNo changes will be made to the database.");
            progressBar_->setVisible(false);
            return;
        }

        // Simulate restore process
        QTimer::singleShot(100, [this, options]() {
            this->progressBar_->setValue(25);

            QTimer::singleShot(500, [this, options]() {
                this->progressBar_->setValue(75);

                QTimer::singleShot(300, [this, options]() {
                    this->progressBar_->setValue(100);

                    // Simulate completion
                    QMessageBox::information(this, "Restore Complete",
                                           QString("Database restore completed successfully!\n\nFile: %1\nMode: %2\nObjects Restored: 15 tables, 8 views, 12 indexes")
                                           .arg(options.filePath)
                                           .arg(options.restoreMode));

                    this->progressBar_->setVisible(false);
                });
            });
        });
    }

    void handleBackupVerified(const QString& filePath) {
        QMessageBox::information(this, "Backup Verification",
                               QString("Backup file verified successfully!\n\nFile: %1\nStatus: All data intact ✅")
                               .arg(filePath));
    }

    void handleBackupDeleted(const QString& filePath) {
        QMessageBox::information(this, "Backup Deleted",
                               QString("Backup file has been deleted.\n\nFile: %1")
                               .arg(filePath));
    }

    void showFindReplaceDialog(bool replaceMode) {
        FindReplaceDialog dialog(this);

        // Set current document text if available
        // In a real implementation, you would get this from the active query editor
        QString currentText = "SELECT * FROM users WHERE active = true ORDER BY created_at DESC LIMIT 100";
        dialog.setCurrentDocumentText(currentText, "Current Query.sql");

        // Connect signals
        connect(&dialog, &FindReplaceDialog::findRequested, this, &MainWindow::Impl::handleFindRequest);
        connect(&dialog, &FindReplaceDialog::replaceRequested, this, &MainWindow::Impl::handleReplaceRequest);
        connect(&dialog, &FindReplaceDialog::replaceAllRequested, this, &MainWindow::Impl::handleReplaceAllRequest);
        connect(&dialog, &FindReplaceDialog::findAllRequested, this, &MainWindow::Impl::handleFindAllRequest);
        connect(&dialog, &FindReplaceDialog::searchResultsReady, this, &MainWindow::Impl::handleSearchResults);

        // If replaceMode is true, switch to replace tab
        if (replaceMode) {
            dialog.getReplaceTextEdit()->setFocus();
        } else {
            dialog.getFindTextEdit()->setFocus();
        }

        dialog.exec();
    }

    void handleFindRequest(const FindOptions& options) {
        // In a real implementation, you would search in the actual query editor
        QMessageBox::information(this, "Find",
                               QString("Searching for: %1\nOptions: Case sensitive=%2, Regex=%3")
                               .arg(options.searchText)
                               .arg(options.caseSensitive ? "Yes" : "No")
                               .arg(options.regularExpression ? "Yes" : "No"));
    }

    void handleReplaceRequest(const FindOptions& options) {
        QMessageBox::information(this, "Replace",
                               QString("Replacing: %1\nWith: %2\nOptions: Case sensitive=%3")
                               .arg(options.searchText)
                               .arg(options.replaceText)
                               .arg(options.caseSensitive ? "Yes" : "No"));
    }

    void handleReplaceAllRequest(const FindOptions& options) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Replace All",
            QString("Replace all occurrences of '%1' with '%2'?\n\nThis action cannot be undone.")
            .arg(options.searchText)
            .arg(options.replaceText),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            QMessageBox::information(this, "Replace All Complete",
                                   QString("Replaced all occurrences of '%1' with '%2'.\n\nReplaced: 5 occurrences")
                                   .arg(options.searchText)
                                   .arg(options.replaceText));
        }
    }

    void handleFindAllRequest(const FindOptions& options) {
        // Simulate search results
        QList<FindReplaceResult> results;

        FindReplaceResult result1;
        result1.fileName = "Current Query.sql";
        result1.lineNumber = 1;
        result1.columnNumber = 8;
        result1.context = "SELECT * FROM users WHERE active = true";
        result1.fullText = "SELECT * FROM users WHERE active = true ORDER BY created_at DESC LIMIT 100";
        result1.matchStart = 7;
        result1.matchLength = 5;
        results.append(result1);

        FindReplaceResult result2;
        result2.fileName = "Current Query.sql";
        result2.lineNumber = 2;
        result2.columnNumber = 15;
        result2.context = "ORDER BY created_at DESC LIMIT 100";
        result2.fullText = "SELECT * FROM users WHERE active = true ORDER BY created_at DESC LIMIT 100";
        result2.matchStart = 14;
        result2.matchLength = 5;
        results.append(result2);

        QMessageBox::information(this, "Find All Results",
                               QString("Found %1 occurrences of '%2'")
                               .arg(results.size())
                               .arg(options.searchText));
    }

    void handleSearchResults(const QList<FindReplaceResult>& results) {
        // In a real implementation, you would display these results in the UI
        QMessageBox::information(this, "Search Results",
                               QString("Found %1 results").arg(results.size()));
    }

    void showColumnEditorDialog() {
        ColumnEditorDialog dialog(this);

        // Set up default column definition for demonstration
        ColumnEditorDefinition defaultDef;
        defaultDef.name = "new_column";
        defaultDef.dataType = ColumnDataType::VARCHAR;
        defaultDef.length = 255;
        defaultDef.nullable = true;
        defaultDef.comment = "New column description";

        dialog.setColumnDefinition(defaultDef, true);

        if (dialog.exec() == QDialog::Accepted) {
            ColumnEditorDefinition result = dialog.getColumnDefinition();

            // In a real implementation, you would apply this to the actual table
            QMessageBox::information(this, "Column Created",
                                   QString("Column '%1' has been created with type %2")
                                   .arg(result.name)
                                   .arg(dialog.getDataTypeString(result.dataType)));
        }
    }

    void showDataEditorDialog() {
        // Create sample table info for demonstration
        TableInfo tableInfo;
        tableInfo.name = "users";
        tableInfo.schema = "public";
        tableInfo.columnNames = QStringList() << "id" << "username" << "email" << "created_at" << "active" << "last_login";
        tableInfo.columnTypes = QStringList() << "INTEGER" << "VARCHAR(100)" << "VARCHAR(255)" << "TIMESTAMP" << "BOOLEAN" << "TIMESTAMP";
        tableInfo.nullableColumns = QList<bool>() << false << false << false << false << true << true;
        tableInfo.primaryKeyColumns = QStringList() << "id";

        // Create sample data
        QList<QList<QVariant>> sampleData;
        for (int i = 1; i <= 50; ++i) {
            QList<QVariant> row;
            row << i;                                    // id
            row << QString("user%1").arg(i);           // username
            row << QString("user%1@example.com").arg(i); // email
            row << QDateTime::currentDateTime();       // created_at
            row << (i % 3 != 0);                       // active (every 3rd user inactive)
            row << (i % 5 == 0 ? QDateTime::currentDateTime() : QVariant()); // last_login
            sampleData.append(row);
        }

        bool accepted = DataEditorDialog::showDataEditor(this, tableInfo, sampleData, "ScratchRobinDB");

        if (accepted) {
            QMessageBox::information(this, "Data Editor Closed",
                                   "Data editor session completed.\n\n"
                                   "In a real implementation, any changes would be saved to the database.");
        }
    }

    void showProgressDialogDemo() {
        // Demonstrate different types of progress dialogs

        // 1. Simple determinate progress
        bool success1 = ProgressDialog::showProgress(this,
            "Processing Data",
            "Processing large dataset...",
            [](ProgressDialog* dialog) -> bool {
                for (int i = 0; i <= 100; ++i) {
                    if (dialog->isCancelled()) return false;

                    dialog->setProgress(i);
                    dialog->setStatusText(QString("Processing item %1 of 100").arg(i));

                    // Simulate work
                    QThread::msleep(50);

                    // Process events to keep UI responsive
                    QApplication::processEvents();
                }
                return true;
            },
            true);

        if (!success1) {
            QMessageBox::information(this, "Demo Cancelled", "Determinate progress demo was cancelled.");
            return;
        }

        // 2. Indeterminate progress
        bool success2 = ProgressDialog::showIndeterminateProgress(this,
            "Connecting to Database",
            "Establishing connection to remote server...",
            [](ProgressDialog* dialog) -> bool {
                dialog->setStatusText("Authenticating...");
                QThread::sleep(1);

                dialog->setStatusText("Connecting...");
                QThread::sleep(1);

                dialog->setStatusText("Verifying connection...");
                QThread::sleep(1);

                return true;
            },
            true);

        if (!success2) {
            QMessageBox::information(this, "Demo Cancelled", "Indeterminate progress demo was cancelled.");
            return;
        }

        // 3. Multi-step progress
        ProgressDialog* multiStepDialog = new ProgressDialog(this);
        multiStepDialog->setOperation("Database Maintenance", "Performing comprehensive database maintenance");
        multiStepDialog->setMode(ProgressDialogMode::MULTI_STEP);
        multiStepDialog->setAllowCancel(true);
        multiStepDialog->setShowDetails(true);
        multiStepDialog->setAutoClose(true);
        multiStepDialog->setAutoCloseDelay(3);

        // Add operations
        multiStepDialog->addOperation("analyze", "Analyzing table statistics", 3);
        multiStepDialog->addOperation("vacuum", "Cleaning up database files", 5);
        multiStepDialog->addOperation("reindex", "Rebuilding indexes", 2);
        multiStepDialog->addOperation("backup", "Creating backup", 1);

        multiStepDialog->start();

        // Simulate multi-step operations
        QStringList operations = QStringList() << "analyze" << "vacuum" << "reindex" << "backup";
        QStringList descriptions = QStringList()
            << "Analyzing table statistics..."
            << "Cleaning up database files..."
            << "Rebuilding indexes..."
            << "Creating backup...";

        for (int opIndex = 0; opIndex < operations.size(); ++opIndex) {
            if (multiStepDialog->isCancelled()) break;

            multiStepDialog->setStatusText(descriptions[opIndex]);
            multiStepDialog->appendDetailsText("Starting: " + operations[opIndex]);

            // Simulate steps for each operation
            int steps = (opIndex == 0) ? 3 : (opIndex == 1) ? 5 : (opIndex == 2) ? 2 : 1;

            for (int step = 1; step <= steps; ++step) {
                if (multiStepDialog->isCancelled()) break;

                multiStepDialog->updateOperation(opIndex, static_cast<double>(step) / steps,
                    QString("Step %1 of %2").arg(step).arg(steps));
                multiStepDialog->appendDetailsText(QString("  Completed step %1").arg(step));

                QThread::msleep(300);
                QApplication::processEvents();
            }

            if (!multiStepDialog->isCancelled()) {
                multiStepDialog->completeOperation(opIndex);
                multiStepDialog->appendDetailsText("✓ Completed: " + operations[opIndex]);
            }
        }

        multiStepDialog->stop();

        if (multiStepDialog->isCancelled()) {
            QMessageBox::information(this, "Demo Cancelled", "Multi-step progress demo was cancelled.");
        } else {
            QMessageBox::information(this, "Demo Completed",
                                   "All progress dialog demonstrations completed successfully!\n\n"
                                   "The Progress Dialog supports:\n"
                                   "• Determinate progress with percentage\n"
                                   "• Indeterminate progress for unknown duration\n"
                                   "• Multi-step operations with individual tracking\n"
                                   "• Detailed logging and status updates\n"
                                   "• Cancellation support\n"
                                   "• Auto-close functionality\n"
                                   "• Integration with long-running operations");
        }

        multiStepDialog->deleteLater();
    }

    void showErrorDetailsDialogDemo() {
        // Demonstrate different types of error dialogs

        // 1. Simple error message
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Demo Selection",
            "Choose an error type to demonstrate:\n\n"
            "• Database Error - SQL query failure\n"
            "• File System Error - File operation failure\n"
            "• Multiple Errors - Several related errors\n"
            "• Custom Error - Application-specific error",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
        );

        if (reply == QMessageBox::Cancel) {
            return;
        }

        if (reply == QMessageBox::Yes) {
            // Database error demo
            ErrorDetailsDialog::showDatabaseError(
                this,
                "SELECT * FROM users WHERE active = true",
                "ERROR: relation \"users\" does not exist\nLINE 1: SELECT * FROM users WHERE active = true\n                 ^",
                "postgresql://user@localhost:5432/mydb"
            );
        } else {
            // File system error demo
            ErrorDetailsDialog::showFileError(
                this,
                "/home/user/data/scratchrobin/export.csv",
                "write",
                "Permission denied (errno 13)\nThe file or directory is read-only."
            );
        }

        // 2. Custom error with full details
        ErrorDetails customError;
        customError.errorId = "ERR-2024-001";
        customError.title = "Query Execution Timeout";
        customError.summary = "Database query took longer than the configured timeout period";
        customError.detailedDescription =
            "The database query execution exceeded the maximum allowed time of 30 seconds.\n\n"
            "This can happen when:\n"
            "• The query is too complex for the available resources\n"
            "• Database server is under heavy load\n"
            "• Network latency is affecting query performance\n"
            "• Large result sets are being processed without proper indexing";
        customError.technicalDetails =
            "Query: SELECT * FROM large_table WHERE complex_condition\n"
            "Timeout: 30000ms\n"
            "Connection Pool: 10/10 connections in use\n"
            "Server Load: CPU 85%, Memory 92%";
        customError.severity = ErrorSeverity::WARNING;
        customError.category = ErrorCategory::DATABASE;
        customError.errorCode = "DB_TIMEOUT_001";
        customError.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        customError.suggestedActions = QStringList()
            << "Optimize query by adding proper indexes"
            << "Increase timeout limit for this operation"
            << "Break query into smaller chunks"
            << "Check database server performance"
            << "Consider using EXPLAIN ANALYZE to optimize the query";
        customError.relatedErrors = QStringList()
            << "Similar timeout issues in the last 24 hours: 5"
            << "Related slow query patterns detected";
        customError.contextData = {
            {"query_complexity", "HIGH"},
            {"table_size", "2.5M rows"},
            {"result_size", "150K rows"},
            {"server_memory", "16GB"},
            {"server_cpu", "Intel Xeon 8-core"}
        };
        customError.helpUrl = "help://query-optimization";
        customError.isRecoverable = true;
        customError.recoverySuggestion = "Try running the query during off-peak hours or optimize it first";

        ErrorDetailsDialog* errorDialog = new ErrorDetailsDialog(this);
        errorDialog->setError(customError);

        // Connect signals
        connect(errorDialog, &ErrorDetailsDialog::helpRequested,
                [this](const QString& helpUrl) {
            QMessageBox::information(this, "Help Requested",
                                   QString("Help system would open: %1").arg(helpUrl));
        });

        connect(errorDialog, &ErrorDetailsDialog::retryRequested,
                [this](const ErrorDetails& error) {
            QMessageBox::information(this, "Retry Requested",
                                   QString("Would retry operation: %1").arg(error.title));
        });

        connect(errorDialog, &ErrorDetailsDialog::errorReported,
                [this](const ErrorDetails& error) {
            QMessageBox::information(this, "Error Reported",
                                   QString("Error would be reported: %1").arg(error.errorId));
        });

        errorDialog->exec();
        errorDialog->deleteLater();

        // 3. Multiple errors demonstration
        QMessageBox::StandardButton multiReply = QMessageBox::question(
            this, "Multiple Errors Demo",
            "Would you like to see a demonstration of multiple related errors?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (multiReply == QMessageBox::Yes) {
            ErrorDetailsDialog* multiErrorDialog = new ErrorDetailsDialog(this);

            // Add several related errors
            ErrorDetails error1;
            error1.title = "Connection Lost";
            error1.summary = "Lost connection to database server";
            error1.severity = ErrorSeverity::ERROR;
            error1.category = ErrorCategory::NETWORK;
            error1.timestamp = QDateTime::currentDateTime().addSecs(-30).toString(Qt::ISODate);
            error1.suggestedActions = QStringList() << "Check network connectivity" << "Restart connection";

            ErrorDetails error2;
            error2.title = "Query Failed";
            error2.summary = "Previous query failed due to connection loss";
            error2.severity = ErrorSeverity::WARNING;
            error2.category = ErrorCategory::DATABASE;
            error2.timestamp = QDateTime::currentDateTime().addSecs(-25).toString(Qt::ISODate);
            error2.suggestedActions = QStringList() << "Retry query" << "Check connection status";

            ErrorDetails error3;
            error3.title = "Data Sync Issue";
            error3.summary = "Failed to synchronize data due to previous errors";
            error3.severity = ErrorSeverity::CRITICAL;
            error3.category = ErrorCategory::APPLICATION;
            error3.timestamp = QDateTime::currentDateTime().addSecs(-10).toString(Qt::ISODate);
            error3.suggestedActions = QStringList() << "Manual data synchronization required" << "Contact administrator";

            multiErrorDialog->setError(error1);
            multiErrorDialog->addError(error2);
            multiErrorDialog->addError(error3);

            multiErrorDialog->exec();
            multiErrorDialog->deleteLater();
        }

        QMessageBox::information(this, "Demo Completed",
                               "Error Details Dialog demonstrations completed successfully!\n\n"
                               "The Error Details Dialog supports:\n"
                               "• Multiple error categories (Database, Network, Filesystem, etc.)\n"
                               "• Severity levels (Info, Warning, Error, Critical, Fatal)\n"
                               "• Detailed technical information and stack traces\n"
                               "• Suggested recovery actions\n"
                               "• Context data and related errors\n"
                               "• Copy to clipboard and save to file\n"
                               "• Error reporting integration\n"
                               "• Help system integration\n"
                               "• Retry and ignore operations\n"
                               "• Multiple error navigation");
    }

    void showConfirmationDialogDemo() {
        // Demonstrate different types of confirmation dialogs

        // 1. Simple delete confirmation
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Demo Selection",
            "Choose a confirmation type to demonstrate:\n\n"
            "• Simple Delete - Basic delete confirmation\n"
            "• Table Drop - Critical database operation\n"
            "• Bulk Operation - Multiple item operation\n"
            "• File Overwrite - File replacement warning\n"
            "• Unsaved Changes - Close without saving",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
        );

        if (reply == QMessageBox::Cancel) {
            return;
        }

        bool confirmed = false;

        if (reply == QMessageBox::Yes) {
            // Simple delete confirmation
            confirmed = ConfirmationDialog::confirmDelete(
                this,
                "user",
                "john.doe@example.com",
                1
            );
        } else {
            // Bulk operation confirmation
            confirmed = ConfirmationDialog::confirmBulkOperation(
                this,
                "Delete selected records",
                25
            );
        }

        if (confirmed) {
            QMessageBox::information(this, "Confirmed",
                                   "The operation was confirmed and would proceed.");
        } else {
            QMessageBox::information(this, "Cancelled",
                                   "The operation was cancelled by the user.");
        }

        // 2. Table drop confirmation (high risk)
        QMessageBox::StandardButton tableReply = QMessageBox::question(
            this, "Table Drop Demo",
            "Would you like to see a table drop confirmation?\n\n"
            "This demonstrates a high-risk database operation.",
            QMessageBox::Yes | QMessageBox::No
        );

        if (tableReply == QMessageBox::Yes) {
            confirmed = ConfirmationDialog::confirmDropTable(
                this,
                "user_sessions",
                15432
            );

            if (confirmed) {
                QMessageBox::information(this, "Table Dropped",
                                       "The table would be dropped (demo only).");
            } else {
                QMessageBox::information(this, "Operation Cancelled",
                                       "Table drop was cancelled.");
            }
        }

        // 3. File overwrite confirmation
        QMessageBox::StandardButton fileReply = QMessageBox::question(
            this, "File Overwrite Demo",
            "Would you like to see a file overwrite confirmation?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (fileReply == QMessageBox::Yes) {
            confirmed = ConfirmationDialog::confirmOverwrite(
                this,
                "/home/user/exports/data_backup.csv",
                "export"
            );

            if (confirmed) {
                QMessageBox::information(this, "File Overwritten",
                                       "The file would be overwritten (demo only).");
            } else {
                QMessageBox::information(this, "Operation Cancelled",
                                       "File overwrite was cancelled.");
            }
        }

        // 4. Unsaved changes confirmation
        QMessageBox::StandardButton unsavedReply = QMessageBox::question(
            this, "Unsaved Changes Demo",
            "Would you like to see an unsaved changes confirmation?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (unsavedReply == QMessageBox::Yes) {
            bool shouldSave = ConfirmationDialog::confirmCloseUnsaved(this, 3);

            if (shouldSave) {
                QMessageBox::information(this, "Save & Close",
                                       "Changes would be saved before closing.");
            } else {
                QMessageBox::information(this, "Close Without Saving",
                                       "Window would close without saving changes.");
            }
        }

        // 5. Advanced confirmation with custom actions
        QMessageBox::StandardButton advancedReply = QMessageBox::question(
            this, "Advanced Confirmation Demo",
            "Would you like to see an advanced confirmation with custom actions?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (advancedReply == QMessageBox::Yes) {
            ConfirmationOptions options;
            options.title = "Database Maintenance";
            options.message = "Perform database maintenance operations?";
            options.detailedMessage = "This will optimize database performance and reclaim disk space.";
            options.type = ConfirmationType::MULTI_OPTION;
            options.riskLevel = RiskLevel::LOW;
            options.showDontAskAgain = true;

            ConfirmationAction quickAction;
            quickAction.label = "Quick Maintenance";
            quickAction.description = "Fast optimization (5-10 minutes)";
            quickAction.iconName = "maintenance_quick";

            ConfirmationAction fullAction;
            fullAction.label = "Full Maintenance";
            fullAction.description = "Complete optimization (30+ minutes)";
            fullAction.iconName = "maintenance_full";

            ConfirmationAction customAction;
            customAction.label = "Custom Settings";
            customAction.description = "Configure specific maintenance options";
            customAction.iconName = "maintenance_custom";

            ConfirmationAction cancelAction;
            cancelAction.label = "Cancel";
            cancelAction.isDefault = true;
            cancelAction.iconName = "cancel";

            options.actions = {quickAction, fullAction, customAction, cancelAction};
            options.impactDetails = QStringList()
                << "Database may be temporarily unavailable"
                << "Operations will be faster after completion"
                << "Disk space will be optimized";

            ConfirmationAction selectedAction = ConfirmationDialog::showWithActions(this, options);

            QMessageBox::information(this, "Selected Action",
                                   QString("Selected: %1\nDescription: %2")
                                   .arg(selectedAction.label, selectedAction.description));
        }

        QMessageBox::information(this, "Demo Completed",
                               "Confirmation Dialog demonstrations completed successfully!\n\n"
                               "The Confirmation Dialog supports:\n"
                               "• Risk-based visual indicators (Low, Medium, High, Critical)\n"
                               "• Multiple confirmation types (Simple, Multi-option, Checkbox)\n"
                               "• Detailed impact assessment and consequences\n"
                               "• Alternative action suggestions\n"
                               "• 'Don't ask again' functionality\n"
                               "• Timeout-based auto-cancellation\n"
                               "• Custom action buttons and workflows\n"
                               "• Integration with application settings\n"
                               "• Safety mechanisms for destructive operations");
    }

    void showKeyboardShortcutsDialog() {
        // Show the keyboard shortcuts configuration dialog
        KeyboardShortcutsDialog::showKeyboardShortcuts(this);
    }

    void showFavoritesManagerDialog() {
        // Show the query favorites manager dialog
        FavoritesManagerDialog::showFavoritesManager(this);
    }

    void handleExportRequest(const ExportOptions& options) {
        // Show progress bar
        progressBar_->setVisible(true);
        progressBar_->setValue(0);

        // Simulate export process
        QTimer::singleShot(100, [this, options]() {
            this->progressBar_->setValue(25);

            QTimer::singleShot(500, [this, options]() {
                this->progressBar_->setValue(75);

                QTimer::singleShot(300, [this, options]() {
                    this->progressBar_->setValue(100);

                    // Simulate completion
                    QMessageBox::information(this, "Export Complete",
                                           QString("Database exported successfully to:\n%1\n\nFormat: %2\nTables: %3")
                                           .arg(options.filePath)
                                           .arg(options.format)
                                           .arg(options.selectedTables.size()));

                    this->progressBar_->setVisible(false);
                });
            });
        });
    }

    void handleImportRequest(const ImportOptions& options) {
        // Show progress bar
        progressBar_->setVisible(true);
        progressBar_->setValue(0);

        // Check if preview only
        if (options.previewOnly) {
            QMessageBox::information(this, "Preview Mode",
                                   "Import would be performed in preview mode.\nNo changes will be made to the database.");
            progressBar_->setVisible(false);
            return;
        }

        // Simulate import process
        QTimer::singleShot(100, [this, options]() {
            this->progressBar_->setValue(25);

            QTimer::singleShot(500, [this, options]() {
                this->progressBar_->setValue(75);

                QTimer::singleShot(300, [this, options]() {
                    this->progressBar_->setValue(100);

                    // Simulate completion
                    QMessageBox::information(this, "Import Complete",
                                           QString("Data imported successfully from:\n%1\n\nFormat: %2\nOptions: %3")
                                           .arg(options.filePath)
                                           .arg(options.format)
                                           .arg(options.createTables ? "Create tables" : "Skip tables"));

                    this->progressBar_->setVisible(false);
                });
            });
        });
    }

    void handlePreviewRequest(const QString& filePath) {
        // Simulate file preview
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Preview Error", "Could not open file for preview.");
            return;
        }

        QTextStream in(&file);
        QString content;

        // Read first 1000 lines or until end of file
        int maxLines = 1000;
        int lineCount = 0;

        while (!in.atEnd() && lineCount < maxLines) {
            content += in.readLine() + "\n";
            lineCount++;
        }

        if (!in.atEnd()) {
            content += "\n... (truncated, showing first " + QString::number(maxLines) + " lines) ...";
        }

        file.close();

        // In a real implementation, you would emit this back to the dialog
        // For now, just show it in a message box
        QMessageBox previewBox;
        previewBox.setWindowTitle("File Preview");
        previewBox.setText("File preview (first " + QString::number(lineCount) + " lines):");

        QTextEdit* previewText = new QTextEdit();
        previewText->setPlainText(content);
        previewText->setFontFamily("monospace");
        previewText->setMaximumHeight(400);
        previewText->setMaximumWidth(600);

        QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(previewBox.layout());
        if (layout) {
            layout->insertWidget(0, previewText);
        }

        previewBox.exec();
    }

    void updateRecentConnectionsMenu() {
        recentConnectionsMenu_->clear();

        // Load recent connections from settings
        QSettings settings("ScratchRobin", "RecentConnections");
        QStringList recentConnections = settings.value("connections").toStringList();

        if (recentConnections.isEmpty()) {
            QAction* noRecentAction = new QAction("No recent connections", this);
            noRecentAction->setEnabled(false);
            recentConnectionsMenu_->addAction(noRecentAction);
        } else {
            for (int i = 0; i < recentConnections.size() && i < 10; ++i) {
                QString connectionString = recentConnections.at(i);
                QAction* connectionAction = new QAction(connectionString, this);
                connectionAction->setStatusTip("Connect to " + connectionString);

                connect(connectionAction, &QAction::triggered, [this, connectionString]() {
                    connectToRecentConnection(connectionString);
                });

                recentConnectionsMenu_->addAction(connectionAction);
            }

            recentConnectionsMenu_->addSeparator();

            QAction* clearAction = new QAction("Clear Recent Connections", this);
            connect(clearAction, &QAction::triggered, [this]() {
                clearRecentConnections();
            });
            recentConnectionsMenu_->addAction(clearAction);
        }
    }

    void addToRecentConnections(const QString& connectionString) {
        QSettings settings("ScratchRobin", "RecentConnections");
        QStringList recentConnections = settings.value("connections").toStringList();

        // Remove if already exists
        recentConnections.removeAll(connectionString);

        // Add to beginning
        recentConnections.prepend(connectionString);

        // Keep only the last 10 connections
        while (recentConnections.size() > 10) {
            recentConnections.removeLast();
        }

        settings.setValue("connections", recentConnections);
        updateRecentConnectionsMenu();
    }

    void connectToRecentConnection(const QString& connectionString) {
        // Parse the connection string and show connection dialog with pre-filled values
        QStringList parts = connectionString.split("@");
        if (parts.size() == 2) {
            QString user = parts.at(0);
            QString connectionPart = parts.at(1);

            // Try to parse host:port/database format
            QStringList connectionParts = connectionPart.split("/");
            if (connectionParts.size() == 2) {
                QString hostPort = connectionParts.at(0);
                QString database = connectionParts.at(1);

                QStringList hostPortParts = hostPort.split(":");
                QString host = hostPortParts.at(0);
                int port = hostPortParts.size() > 1 ? hostPortParts.at(1).toInt() : 5432;

                // Show connection dialog with pre-filled values
                showConnectionDialogWithValues(user, host, port, database);
            }
        }
    }

    void clearRecentConnections() {
        QSettings settings("ScratchRobin", "RecentConnections");
        settings.remove("connections");
        updateRecentConnectionsMenu();
    }

    void showConnectionDialogWithValues(const QString& username, const QString& host, int port, const QString& database) {
        DynamicConnectionDialog dialog(this);

        // Create a config with the pre-filled values
        DatabaseConnectionConfig config;
        config.databaseType = DatabaseType::POSTGRESQL; // Default assumption
        config.host = host;
        config.port = port;
        config.database = database;
        config.username = username;
        config.connectionName = QString("Connection to %1").arg(database);

        dialog.setConnectionConfig(config);

        if (dialog.exec() == QDialog::Accepted) {
            DatabaseConnectionConfig finalConfig = dialog.getConnectionConfig();

            // Update connection status
            QString connectionString = QString("%1@%2:%3/%4 (%5)")
                                     .arg(finalConfig.username)
                                     .arg(finalConfig.host)
                                     .arg(finalConfig.port)
                                     .arg(finalConfig.database)
                                     .arg(this->databaseDriverManager->databaseTypeToString(finalConfig.databaseType));

            updateConnectionStatus(true, connectionString);

            // Add to recent connections
            QString recentString = QString("%1@%2:%3/%4")
                                 .arg(finalConfig.username)
                                 .arg(finalConfig.host)
                                 .arg(finalConfig.port)
                                 .arg(finalConfig.database);

            addToRecentConnections(recentString);

            // Show success message
            statusBar()->showMessage("Successfully connected to database", 3000);
        }
    }

    void updateConnectionStatus(bool connected, const QString& message) {
        if (connected) {
            connectionIconLabel_->setPixmap(AppIcons::instance().getConnectIcon().pixmap(16, 16));
            connectionStatusLabel_->setText(message);
            connectionStatusLabel_->setStyleSheet("color: #4CAF50; font-size: 11px; font-weight: bold;");
        } else {
            connectionIconLabel_->setPixmap(AppIcons::instance().getDisconnectIcon().pixmap(16, 16));
            connectionStatusLabel_->setText(message);
            connectionStatusLabel_->setStyleSheet("color: #666; font-size: 11px;");
        }
    }

    // Status bar components
    QWidget* connectionStatusWidget_;
    QLabel* connectionIconLabel_;
    QLabel* connectionStatusLabel_;
    QLabel* versionLabel_;
    QProgressBar* progressBar_;

    // Recent connections
    QMenu* recentConnectionsMenu_;

    // Import/Export state
    QString currentDatabase_;

    Application* application_;
};

MainWindow::MainWindow(Application* application)
    : impl_(std::make_unique<Impl>(application)) {
    Logger::info("MainWindow created");
}

MainWindow::~MainWindow() {
    Logger::info("MainWindow destroyed");
}

void MainWindow::show() {
    impl_->show();
    Logger::info("MainWindow shown");
}

void MainWindow::hide() {
    impl_->hide();
    Logger::info("MainWindow hidden");
}

bool MainWindow::isVisible() const {
    return impl_->isVisible();
}

void MainWindow::close() {
    impl_->close();
    Logger::info("MainWindow closed");
}

void MainWindow::setTitle(const std::string& title) {
    impl_->setWindowTitle(QString::fromStdString(title));
    Logger::info("MainWindow title set to: " + title);
}

std::string MainWindow::getTitle() const {
    return impl_->windowTitle().toStdString();
}

void MainWindow::setSize(int width, int height) {
    impl_->resize(width, height);
    Logger::info("MainWindow size set to: " + std::to_string(width) + "x" + std::to_string(height));
}

void MainWindow::getSize(int& width, int& height) const {
    QSize size = impl_->size();
    width = size.width();
    height = size.height();
}

} // namespace scratchrobin
