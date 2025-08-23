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
#include <QLineEdit>

namespace scratchrobin {

class MainWindow::Impl : public QMainWindow {
public:
    Impl(Application* app) : application_(app) {
        setupUI();
    }

    // No component references needed for placeholders

    void setupUI() {
        // Set window properties
        setWindowTitle("ScratchRobin Database GUI");
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

        QAction* newAction = new QAction("&New", this);
        newAction->setShortcut(QKeySequence::New);
        fileMenu->addAction(newAction);

        QAction* openAction = new QAction("&Open", this);
        openAction->setShortcut(QKeySequence::Open);
        fileMenu->addAction(openAction);

        fileMenu->addSeparator();

        QAction* exitAction = new QAction("E&xit", this);
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
        fileMenu->addAction(exitAction);

        // Edit menu
        QMenu* editMenu = menuBar()->addMenu("&Edit");

        QAction* undoAction = new QAction("&Undo", this);
        undoAction->setShortcut(QKeySequence::Undo);
        editMenu->addAction(undoAction);

        QAction* redoAction = new QAction("&Redo", this);
        redoAction->setShortcut(QKeySequence::Redo);
        editMenu->addAction(redoAction);

        // Help menu
        QMenu* helpMenu = menuBar()->addMenu("&Help");

        QAction* aboutAction = new QAction("&About ScratchRobin", this);
        helpMenu->addAction(aboutAction);
    }

    void setupStatusBar() {
        statusBar()->showMessage("Ready - ScratchRobin Database GUI v0.1.0");
    }

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
