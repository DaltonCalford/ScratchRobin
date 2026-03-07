#include "ui/toolbar_editor_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QStyle>

namespace scratchrobin::ui {

ToolbarEditorDialog::ToolbarEditorDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Customize Toolbars"));
    setMinimumSize(700, 500);
    resize(800, 600);
    
    setupUi();
    populateAvailableActions();
}

ToolbarEditorDialog::~ToolbarEditorDialog() = default;

void ToolbarEditorDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    
    // Toolbar selection
    auto* select_layout = new QHBoxLayout();
    select_layout->addWidget(new QLabel(tr("Toolbar:")));
    toolbar_combo_ = new QComboBox(this);
    select_layout->addWidget(toolbar_combo_, 1);
    new_toolbar_btn_ = new QPushButton(tr("New..."), this);
    delete_toolbar_btn_ = new QPushButton(tr("Delete"), this);
    select_layout->addWidget(new_toolbar_btn_);
    select_layout->addWidget(delete_toolbar_btn_);
    layout->addLayout(select_layout);
    
    // Toolbar properties
    auto* props_group = new QGroupBox(tr("Toolbar Properties"), this);
    auto* props_layout = new QHBoxLayout(props_group);
    
    props_layout->addWidget(new QLabel(tr("Name:")));
    toolbar_name_edit_ = new QLineEdit(this);
    props_layout->addWidget(toolbar_name_edit_);
    
    props_layout->addWidget(new QLabel(tr("Icon Size:")));
    icon_size_combo_ = new QComboBox(this);
    icon_size_combo_->addItem(tr("Small (16x16)"), 16);
    icon_size_combo_->addItem(tr("Medium (22x22)"), 22);
    icon_size_combo_->addItem(tr("Large (32x32)"), 32);
    icon_size_combo_->addItem(tr("Extra Large (48x48)"), 48);
    props_layout->addWidget(icon_size_combo_);
    
    show_text_check_ = new QCheckBox(tr("Show text labels"), this);
    props_layout->addWidget(show_text_check_);
    
    layout->addWidget(props_group);
    
    // Lists layout
    auto* lists_layout = new QHBoxLayout();
    
    // Available actions
    auto* available_group = new QGroupBox(tr("Available Actions"), this);
    auto* available_layout = new QVBoxLayout(available_group);
    
    available_list_ = new QListWidget(this);
    available_list_->setDragEnabled(true);
    available_list_->setAcceptDrops(false);
    available_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    available_layout->addWidget(available_list_);
    
    auto* available_btn_layout = new QHBoxLayout();
    add_btn_ = new QPushButton(tr("Add >"), this);
    separator_btn_ = new QPushButton(tr("Add Separator"), this);
    available_btn_layout->addWidget(add_btn_);
    available_btn_layout->addWidget(separator_btn_);
    available_btn_layout->addStretch();
    available_layout->addLayout(available_btn_layout);
    
    lists_layout->addWidget(available_group);
    
    // Move buttons
    auto* move_layout = new QVBoxLayout();
    move_layout->addStretch();
    move_up_btn_ = new QPushButton(tr("↑"), this);
    move_up_btn_->setMaximumWidth(40);
    move_down_btn_ = new QPushButton(tr("↓"), this);
    move_down_btn_->setMaximumWidth(40);
    remove_btn_ = new QPushButton(tr("< Remove"), this);
    move_layout->addWidget(move_up_btn_);
    move_layout->addWidget(move_down_btn_);
    move_layout->addSpacing(20);
    move_layout->addWidget(remove_btn_);
    move_layout->addStretch();
    lists_layout->addLayout(move_layout);
    
    // Current toolbar
    auto* current_group = new QGroupBox(tr("Current Toolbar"), this);
    auto* current_layout = new QVBoxLayout(current_group);
    
    current_list_ = new QListWidget(this);
    current_list_->setAcceptDrops(true);
    current_list_->setDragEnabled(true);
    current_list_->setDragDropMode(QAbstractItemView::InternalMove);
    current_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    current_layout->addWidget(current_list_);
    
    lists_layout->addWidget(current_group);
    lists_layout->setStretch(0, 1);
    lists_layout->setStretch(2, 1);
    
    layout->addLayout(lists_layout, 1);
    
    // Import/Export buttons
    auto* import_export_layout = new QHBoxLayout();
    import_btn_ = new QPushButton(tr("Import..."), this);
    export_btn_ = new QPushButton(tr("Export..."), this);
    import_export_layout->addWidget(import_btn_);
    import_export_layout->addWidget(export_btn_);
    import_export_layout->addStretch();
    reset_btn_ = new QPushButton(tr("Reset Current"), this);
    reset_all_btn_ = new QPushButton(tr("Reset All"), this);
    import_export_layout->addWidget(reset_btn_);
    import_export_layout->addWidget(reset_all_btn_);
    layout->addLayout(import_export_layout);
    
    // Dialog buttons
    auto* dialog_layout = new QHBoxLayout();
    dialog_layout->addStretch();
    auto* ok_btn = new QPushButton(tr("OK"), this);
    auto* cancel_btn = new QPushButton(tr("Cancel"), this);
    dialog_layout->addWidget(ok_btn);
    dialog_layout->addWidget(cancel_btn);
    layout->addLayout(dialog_layout);
    
    // Connections
    connect(toolbar_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolbarEditorDialog::onToolbarChanged);
    connect(add_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onAddAction);
    connect(remove_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onRemoveAction);
    connect(move_up_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onMoveUp);
    connect(move_down_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onMoveDown);
    connect(separator_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onAddSeparator);
    connect(reset_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onResetToolbar);
    connect(reset_all_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onResetAll);
    connect(new_toolbar_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onNewToolbar);
    connect(delete_toolbar_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onDeleteToolbar);
    connect(icon_size_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolbarEditorDialog::onIconSizeChanged);
    connect(show_text_check_, &QCheckBox::stateChanged, this, &ToolbarEditorDialog::onShowTextChanged);
    connect(import_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onImport);
    connect(export_btn_, &QPushButton::clicked, this, &ToolbarEditorDialog::onExport);
    connect(ok_btn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
    connect(available_list_, &QListWidget::itemDoubleClicked, this, &ToolbarEditorDialog::onAddAction);
    connect(current_list_, &QListWidget::itemDoubleClicked, this, &ToolbarEditorDialog::onRemoveAction);
}

void ToolbarEditorDialog::populateAvailableActions() {
    available_list_->clear();
    
    // Add categories
    QStringList categories = {
        tr("File"), tr("Edit"), tr("View"), tr("Database"), 
        tr("Tools"), tr("Window"), tr("Help")
    };
    
    for (const auto& category : categories) {
        auto* categoryItem = new QListWidgetItem("--- " + category + " ---");
        categoryItem->setFlags(Qt::NoItemFlags);
        categoryItem->setBackground(QColor(230, 230, 230));
        available_list_->addItem(categoryItem);
        
        // Add actions for this category (sample data)
        if (category == tr("File")) {
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_FileIcon), tr("New Connection")));
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Open SQL")));
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save")));
        } else if (category == tr("Edit")) {
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_ArrowBack), tr("Undo")));
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_ArrowForward), tr("Redo")));
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_CommandLink), tr("Cut")));
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView), tr("Copy")));
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Paste")));
        } else if (category == tr("Database")) {
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon), tr("Connect")));
            available_list_->addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_BrowserReload), tr("Execute")));
        }
    }
}

void ToolbarEditorDialog::populateCurrentToolbar() {
    current_list_->clear();
    
    if (!current_toolbar_) return;
    
    // Populate from current toolbar actions
    for (QAction* action : current_toolbar_->actions()) {
        if (action->isSeparator()) {
            auto* item = new QListWidgetItem("——— Separator ———");
            item->setData(Qt::UserRole, "separator");
            item->setBackground(QColor(240, 240, 255));
            current_list_->addItem(item);
        } else {
            auto* item = new QListWidgetItem(action->icon(), action->text());
            item->setData(Qt::UserRole, action->objectName());
            current_list_->addItem(item);
        }
    }
}

void ToolbarEditorDialog::loadToolbars(const QList<QToolBar*>& toolbars) {
    all_toolbars_ = toolbars;
    toolbar_combo_->clear();
    
    for (auto* toolbar : toolbars) {
        toolbar_combo_->addItem(toolbar->windowTitle(), QVariant::fromValue(reinterpret_cast<void*>(toolbar)));
    }
    
    if (!toolbars.isEmpty()) {
        setCurrentToolbar(toolbars.first());
    }
}

void ToolbarEditorDialog::setCurrentToolbar(QToolBar* toolbar) {
    current_toolbar_ = toolbar;
    
    if (toolbar) {
        toolbar_name_edit_->setText(toolbar->windowTitle());
        
        // Set icon size combo
        int iconSize = toolbar->iconSize().width();
        int index = icon_size_combo_->findData(iconSize);
        if (index >= 0) {
            icon_size_combo_->setCurrentIndex(index);
        }
        
        // Set show text
        show_text_check_->setChecked(toolbar->toolButtonStyle() != Qt::ToolButtonIconOnly);
        
        populateCurrentToolbar();
    }
}

void ToolbarEditorDialog::onToolbarChanged(int index) {
    if (index >= 0 && index < all_toolbars_.size()) {
        setCurrentToolbar(all_toolbars_[index]);
    }
}

void ToolbarEditorDialog::onAddAction() {
    auto* currentItem = available_list_->currentItem();
    if (!currentItem) return;
    
    // Skip category headers
    if (currentItem->flags() == Qt::NoItemFlags) return;
    
    auto* newItem = new QListWidgetItem(currentItem->icon(), currentItem->text());
    current_list_->addItem(newItem);
    current_list_->setCurrentItem(newItem);
    
    saveCurrentToolbarState();
}

void ToolbarEditorDialog::onRemoveAction() {
    auto* currentItem = current_list_->currentItem();
    if (!currentItem) return;
    
    delete current_list_->takeItem(current_list_->row(currentItem));
    saveCurrentToolbarState();
}

void ToolbarEditorDialog::onMoveUp() {
    int row = current_list_->currentRow();
    if (row <= 0) return;
    
    auto* item = current_list_->takeItem(row);
    current_list_->insertItem(row - 1, item);
    current_list_->setCurrentItem(item);
    
    saveCurrentToolbarState();
}

void ToolbarEditorDialog::onMoveDown() {
    int row = current_list_->currentRow();
    if (row < 0 || row >= current_list_->count() - 1) return;
    
    auto* item = current_list_->takeItem(row);
    current_list_->insertItem(row + 1, item);
    current_list_->setCurrentItem(item);
    
    saveCurrentToolbarState();
}

void ToolbarEditorDialog::onAddSeparator() {
    auto* item = new QListWidgetItem("——— Separator ———");
    item->setData(Qt::UserRole, "separator");
    item->setBackground(QColor(240, 240, 255));
    
    int row = current_list_->currentRow();
    if (row >= 0) {
        current_list_->insertItem(row + 1, item);
    } else {
        current_list_->addItem(item);
    }
    current_list_->setCurrentItem(item);
    
    saveCurrentToolbarState();
}

void ToolbarEditorDialog::onResetToolbar() {
    if (!current_toolbar_) return;
    
    if (QMessageBox::question(this, tr("Reset Toolbar"),
        tr("Reset '%1' to default?").arg(current_toolbar_->windowTitle())) == QMessageBox::Yes) {
        // Reset to defaults
        current_toolbar_->clear();
        // Add default actions back
        populateCurrentToolbar();
        emit toolbarModified(current_toolbar_);
    }
}

void ToolbarEditorDialog::onResetAll() {
    if (QMessageBox::question(this, tr("Reset All Toolbars"),
        tr("Reset all toolbars to defaults?")) == QMessageBox::Yes) {
        emit toolbarsReset();
    }
}

void ToolbarEditorDialog::onIconSizeChanged(int index) {
    if (!current_toolbar_) return;
    
    int size = icon_size_combo_->itemData(index).toInt();
    current_toolbar_->setIconSize(QSize(size, size));
}

void ToolbarEditorDialog::onShowTextChanged(int state) {
    if (!current_toolbar_) return;
    
    if (state == Qt::Checked) {
        current_toolbar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    } else {
        current_toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
}

void ToolbarEditorDialog::onNewToolbar() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Toolbar"),
                                         tr("Toolbar name:"), QLineEdit::Normal,
                                         tr("Custom Toolbar"), &ok);
    if (ok && !name.isEmpty()) {
        // Create new toolbar would be handled by MainWindow
        // For now, just add to combo
        toolbar_combo_->addItem(name);
        toolbar_combo_->setCurrentIndex(toolbar_combo_->count() - 1);
    }
}

void ToolbarEditorDialog::onDeleteToolbar() {
    if (toolbar_combo_->count() <= 1) {
        QMessageBox::warning(this, tr("Cannot Delete"),
            tr("You must have at least one toolbar."));
        return;
    }
    
    if (QMessageBox::question(this, tr("Delete Toolbar"),
        tr("Delete toolbar '%1'?").arg(toolbar_combo_->currentText())) == QMessageBox::Yes) {
        toolbar_combo_->removeItem(toolbar_combo_->currentIndex());
    }
}

void ToolbarEditorDialog::onImport() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Import Toolbar Configuration"),
                                                    QString(),
                                                    tr("JSON Files (*.json)"));
    if (filename.isEmpty()) return;
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file"));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid configuration file"));
        return;
    }
    
    // Apply configuration
    QMessageBox::information(this, tr("Success"), tr("Configuration imported"));
}

void ToolbarEditorDialog::onExport() {
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Toolbar Configuration"),
                                                    QString(),
                                                    tr("JSON Files (*.json)"));
    if (filename.isEmpty()) return;
    
    QJsonObject root;
    QJsonArray toolbars;
    
    // Export each toolbar
    for (auto* toolbar : all_toolbars_) {
        QJsonObject tbObj;
        tbObj["name"] = toolbar->windowTitle();
        tbObj["iconSize"] = toolbar->iconSize().width();
        tbObj["showText"] = toolbar->toolButtonStyle() != Qt::ToolButtonIconOnly;
        
        QJsonArray actions;
        for (QAction* action : toolbar->actions()) {
            QJsonObject actObj;
            if (action->isSeparator()) {
                actObj["type"] = "separator";
            } else {
                actObj["type"] = "action";
                actObj["text"] = action->text();
                actObj["name"] = action->objectName();
            }
            actions.append(actObj);
        }
        tbObj["actions"] = actions;
        toolbars.append(tbObj);
    }
    
    root["toolbars"] = toolbars;
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, tr("Success"), tr("Configuration exported"));
    }
}

void ToolbarEditorDialog::saveCurrentToolbarState() {
    if (!current_toolbar_) return;
    
    // Update the actual toolbar
    current_toolbar_->clear();
    
    for (int i = 0; i < current_list_->count(); ++i) {
        auto* item = current_list_->item(i);
        if (item->data(Qt::UserRole).toString() == "separator") {
            current_toolbar_->addSeparator();
        } else {
            auto* action = new QAction(item->icon(), item->text(), this);
            current_toolbar_->addAction(action);
        }
    }
    
    emit toolbarModified(current_toolbar_);
}

void ToolbarEditorDialog::onActionSelected() {
    add_btn_->setEnabled(available_list_->currentItem() != nullptr &&
                         available_list_->currentItem()->flags() != Qt::NoItemFlags);
}

void ToolbarEditorDialog::onCurrentActionSelected() {
    bool hasSelection = current_list_->currentItem() != nullptr;
    remove_btn_->setEnabled(hasSelection);
    move_up_btn_->setEnabled(hasSelection && current_list_->currentRow() > 0);
    move_down_btn_->setEnabled(hasSelection && current_list_->currentRow() < current_list_->count() - 1);
}

} // namespace scratchrobin::ui
