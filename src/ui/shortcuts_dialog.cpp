#include "ui/shortcuts_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace scratchrobin::ui {

ShortcutsDialog::ShortcutsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Keyboard Shortcuts"));
    setMinimumSize(600, 500);
    resize(700, 550);
    
    setupUi();
    populateShortcuts();
}

ShortcutsDialog::~ShortcutsDialog() = default;

void ShortcutsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Search
    auto* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(tr("Search:"), this));
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Filter shortcuts..."));
    searchLayout->addWidget(searchEdit_);
    mainLayout->addLayout(searchLayout);
    
    // Shortcuts tree
    shortcutsTree_ = new QTreeWidget(this);
    shortcutsTree_->setHeaderLabels(QStringList() << tr("Action") << tr("Shortcut") << tr("Description"));
    shortcutsTree_->setAlternatingRowColors(true);
    shortcutsTree_->setRootIsDecorated(true);
    shortcutsTree_->setColumnWidth(0, 200);
    shortcutsTree_->setColumnWidth(1, 150);
    shortcutsTree_->header()->setStretchLastSection(true);
    mainLayout->addWidget(shortcutsTree_);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    resetBtn_ = new QPushButton(tr("Reset to Defaults"), this);
    buttonLayout->addWidget(resetBtn_);
    
    exportBtn_ = new QPushButton(tr("Export..."), this);
    buttonLayout->addWidget(exportBtn_);
    
    importBtn_ = new QPushButton(tr("Import..."), this);
    buttonLayout->addWidget(importBtn_);
    
    buttonLayout->addStretch();
    
    closeBtn_ = new QPushButton(tr("Close"), this);
    closeBtn_->setDefault(true);
    buttonLayout->addWidget(closeBtn_);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(searchEdit_, &QLineEdit::textChanged, this, &ShortcutsDialog::onSearchTextChanged);
    connect(shortcutsTree_, &QTreeWidget::itemClicked, this, &ShortcutsDialog::onShortcutSelected);
    connect(resetBtn_, &QPushButton::clicked, this, &ShortcutsDialog::onResetToDefaults);
    connect(exportBtn_, &QPushButton::clicked, this, &ShortcutsDialog::onExportShortcuts);
    connect(importBtn_, &QPushButton::clicked, this, &ShortcutsDialog::onImportShortcuts);
    connect(closeBtn_, &QPushButton::clicked, this, &QDialog::accept);
}

void ShortcutsDialog::populateShortcuts() {
    shortcutsTree_->clear();
    
    // File menu
    auto* fileCat = addShortcutCategory(tr("File"));
    addShortcutItem(fileCat, tr("New Connection"), "Ctrl+N", tr("Create a new database connection"));
    addShortcutItem(fileCat, tr("Open SQL Script"), "Ctrl+O", tr("Open a SQL script file"));
    addShortcutItem(fileCat, tr("Save"), "Ctrl+S", tr("Save current script"));
    addShortcutItem(fileCat, tr("Save As"), "Ctrl+Shift+S", tr("Save with a new name"));
    addShortcutItem(fileCat, tr("Exit"), "Ctrl+Q", tr("Exit the application"));
    
    // Edit menu
    auto* editCat = addShortcutCategory(tr("Edit"));
    addShortcutItem(editCat, tr("Undo"), "Ctrl+Z", tr("Undo last action"));
    addShortcutItem(editCat, tr("Redo"), "Ctrl+Shift+Z", tr("Redo last undone action"));
    addShortcutItem(editCat, tr("Cut"), "Ctrl+X", tr("Cut selection to clipboard"));
    addShortcutItem(editCat, tr("Copy"), "Ctrl+C", tr("Copy selection to clipboard"));
    addShortcutItem(editCat, tr("Paste"), "Ctrl+V", tr("Paste from clipboard"));
    addShortcutItem(editCat, tr("Select All"), "Ctrl+A", tr("Select all content"));
    addShortcutItem(editCat, tr("Find"), "Ctrl+F", tr("Find text"));
    addShortcutItem(editCat, tr("Find and Replace"), "Ctrl+H", tr("Find and replace text"));
    addShortcutItem(editCat, tr("Find Next"), "F3", tr("Find next occurrence"));
    addShortcutItem(editCat, tr("Find Previous"), "Shift+F3", tr("Find previous occurrence"));
    addShortcutItem(editCat, tr("Preferences"), "Ctrl+,", tr("Edit application preferences"));
    
    // View menu
    auto* viewCat = addShortcutCategory(tr("View"));
    addShortcutItem(viewCat, tr("Refresh"), "F5", tr("Refresh current view"));
    addShortcutItem(viewCat, tr("Zoom In"), "Ctrl++", tr("Increase zoom level"));
    addShortcutItem(viewCat, tr("Zoom Out"), "Ctrl+-", tr("Decrease zoom level"));
    addShortcutItem(viewCat, tr("Reset Zoom"), "Ctrl+0", tr("Reset zoom to default"));
    addShortcutItem(viewCat, tr("Fullscreen"), "F11", tr("Toggle fullscreen mode"));
    
    // Query menu
    auto* queryCat = addShortcutCategory(tr("Query"));
    addShortcutItem(queryCat, tr("Execute"), "F9", tr("Execute current SQL statement"));
    addShortcutItem(queryCat, tr("Execute Selection"), "Ctrl+F9", tr("Execute selected SQL text"));
    addShortcutItem(queryCat, tr("Toggle Comment"), "Ctrl+/", tr("Toggle comment on selection"));
    
    // Window menu
    auto* windowCat = addShortcutCategory(tr("Window"));
    addShortcutItem(windowCat, tr("Close Window"), "Ctrl+W", tr("Close current window"));
    addShortcutItem(windowCat, tr("Next Window"), "Ctrl+Tab", tr("Switch to next window"));
    addShortcutItem(windowCat, tr("Previous Window"), "Ctrl+Shift+Tab", tr("Switch to previous window"));
    
    // Help menu
    auto* helpCat = addShortcutCategory(tr("Help"));
    addShortcutItem(helpCat, tr("Documentation"), "F1", tr("Open documentation"));
    addShortcutItem(helpCat, tr("Context Help"), "Shift+F1", tr("Open context help"));
    
    shortcutsTree_->expandAll();
}

QTreeWidgetItem* ShortcutsDialog::addShortcutCategory(const QString& category) {
    auto* item = new QTreeWidgetItem(shortcutsTree_);
    item->setText(0, category);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    item->setExpanded(true);
    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);
    return item;
}

void ShortcutsDialog::addShortcutItem(QTreeWidgetItem* parent, const QString& action, 
                                       const QString& shortcut, const QString& description) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, action);
    item->setText(1, shortcut);
    item->setText(2, description);
    item->setData(0, Qt::UserRole, shortcut);
}

void ShortcutsDialog::onSearchTextChanged() {
    QString searchText = searchEdit_->text().toLower();
    
    for (int i = 0; i < shortcutsTree_->topLevelItemCount(); ++i) {
        auto* category = shortcutsTree_->topLevelItem(i);
        bool categoryVisible = false;
        
        for (int j = 0; j < category->childCount(); ++j) {
            auto* item = category->child(j);
            QString action = item->text(0).toLower();
            QString shortcut = item->text(1).toLower();
            QString description = item->text(2).toLower();
            
            bool match = action.contains(searchText) || 
                        shortcut.contains(searchText) || 
                        description.contains(searchText);
            
            item->setHidden(!match);
            if (match) categoryVisible = true;
        }
        
        category->setHidden(!categoryVisible);
    }
}

void ShortcutsDialog::onShortcutSelected() {
    // Could be used for shortcut customization in the future
}

void ShortcutsDialog::onResetToDefaults() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        tr("Reset Shortcuts"),
        tr("Are you sure you want to reset all shortcuts to default values?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        populateShortcuts();
    }
}

void ShortcutsDialog::onExportShortcuts() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Shortcuts"),
        QString(),
        tr("JSON Files (*.json);;XML Files (*.xml)"));
    
    if (fileName.isEmpty()) return;
    
    QJsonObject root;
    QJsonArray shortcuts;
    
    // Export current shortcuts from tree widget
    for (int i = 0; i < shortcutsTree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = shortcutsTree_->topLevelItem(i);
        QJsonObject shortcut;
        shortcut["action"] = item->text(0);
        shortcut["context"] = item->text(1);
        shortcut["shortcut"] = item->text(2);
        shortcuts.append(shortcut);
    }
    
    root["shortcuts"] = shortcuts;
    root["version"] = "1.0";
    root["exported"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(root);
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        
        QMessageBox::information(this, tr("Export Complete"),
            tr("Shortcuts exported to %1").arg(fileName));
    }
}

void ShortcutsDialog::onImportShortcuts() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import Shortcuts"),
        QString(),
        tr("JSON Files (*.json);;XML Files (*.xml);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Cannot open file for reading."));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, tr("Error"),
            tr("Invalid JSON format."));
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray shortcuts = root["shortcuts"].toArray();
    
    // Confirm import
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Confirm Import"),
        tr("Import %1 shortcuts? This will update existing shortcuts.").arg(shortcuts.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Import shortcuts
        for (const auto& s : shortcuts) {
            QJsonObject shortcut = s.toObject();
            QString action = shortcut["action"].toString();
            QString key = shortcut["shortcut"].toString();
            
            // Update in tree
            for (int i = 0; i < shortcutsTree_->topLevelItemCount(); ++i) {
                QTreeWidgetItem* item = shortcutsTree_->topLevelItem(i);
                if (item->text(0) == action) {
                    item->setText(2, key);
                    break;
                }
            }
        }
        
        QMessageBox::information(this, tr("Import Complete"),
            tr("Imported %1 shortcuts.").arg(shortcuts.size()));
    }
}

} // namespace scratchrobin::ui
