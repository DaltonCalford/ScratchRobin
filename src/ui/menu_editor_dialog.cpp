#include "ui/menu_editor_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QStyle>
#include <QInputDialog>

namespace scratchrobin::ui {

MenuEditorDialog::MenuEditorDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Customize Menus"));
    setMinimumSize(800, 600);
    resize(900, 700);
    
    setupUi();
}

MenuEditorDialog::~MenuEditorDialog() = default;

void MenuEditorDialog::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(12);
    
    // Left side - Menu tree
    auto* left_layout = new QVBoxLayout();
    
    auto* tree_group = new QGroupBox(tr("Menu Structure"), this);
    auto* tree_layout = new QVBoxLayout(tree_group);
    
    menu_tree_ = new QTreeWidget(this);
    menu_tree_->setHeaderLabel(tr("Menu Items"));
    menu_tree_->setDragEnabled(true);
    menu_tree_->setAcceptDrops(true);
    menu_tree_->setDragDropMode(QAbstractItemView::InternalMove);
    menu_tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_layout->addWidget(menu_tree_);
    
    // Tree buttons
    auto* tree_btn_layout = new QHBoxLayout();
    add_menu_btn_ = new QPushButton(tr("Add Menu"), this);
    add_action_btn_ = new QPushButton(tr("Add Action"), this);
    add_separator_btn_ = new QPushButton(tr("Add Sep"), this);
    remove_btn_ = new QPushButton(tr("Remove"), this);
    tree_btn_layout->addWidget(add_menu_btn_);
    tree_btn_layout->addWidget(add_action_btn_);
    tree_btn_layout->addWidget(add_separator_btn_);
    tree_btn_layout->addWidget(remove_btn_);
    tree_layout->addLayout(tree_btn_layout);
    
    // Move buttons
    auto* move_layout = new QHBoxLayout();
    move_up_btn_ = new QPushButton(tr("↑ Up"), this);
    move_down_btn_ = new QPushButton(tr("↓ Down"), this);
    move_left_btn_ = new QPushButton(tr("← Out"), this);
    move_right_btn_ = new QPushButton(tr("→ In"), this);
    move_layout->addWidget(move_up_btn_);
    move_layout->addWidget(move_down_btn_);
    move_layout->addWidget(move_left_btn_);
    move_layout->addWidget(move_right_btn_);
    tree_layout->addLayout(move_layout);
    
    left_layout->addWidget(tree_group);
    layout->addLayout(left_layout, 1);
    
    // Right side - Properties
    auto* right_layout = new QVBoxLayout();
    
    props_group_ = new QGroupBox(tr("Properties"), this);
    auto* props_layout = new QGridLayout(props_group_);
    
    int row = 0;
    props_layout->addWidget(new QLabel(tr("ID:")), row, 0);
    id_label_ = new QLabel(tr("(none)"), this);
    props_layout->addWidget(id_label_, row, 1);
    row++;
    
    props_layout->addWidget(new QLabel(tr("Text:")), row, 0);
    text_edit_ = new QLineEdit(this);
    props_layout->addWidget(text_edit_, row, 1);
    row++;
    
    props_layout->addWidget(new QLabel(tr("Shortcut:")), row, 0);
    shortcut_edit_ = new QKeySequenceEdit(this);
    props_layout->addWidget(shortcut_edit_, row, 1);
    row++;
    
    props_layout->addWidget(new QLabel(tr("Tooltip:")), row, 0);
    tooltip_edit_ = new QLineEdit(this);
    props_layout->addWidget(tooltip_edit_, row, 1);
    row++;
    
    props_layout->addWidget(new QLabel(tr("Icon:")), row, 0);
    icon_combo_ = new QComboBox(this);
    populateIconCombo();
    props_layout->addWidget(icon_combo_, row, 1);
    row++;
    
    checkable_check_ = new QCheckBox(tr("Checkable"), this);
    props_layout->addWidget(checkable_check_, row, 0, 1, 2);
    row++;
    
    enabled_check_ = new QCheckBox(tr("Enabled"), this);
    enabled_check_->setChecked(true);
    props_layout->addWidget(enabled_check_, row, 0);
    
    visible_check_ = new QCheckBox(tr("Visible"), this);
    visible_check_->setChecked(true);
    props_layout->addWidget(visible_check_, row, 1);
    row++;
    
    right_layout->addWidget(props_group_);
    
    // Global actions
    auto* global_group = new QGroupBox(tr("Global Actions"), this);
    auto* global_layout = new QHBoxLayout(global_group);
    
    reset_btn_ = new QPushButton(tr("Reset Current Menu"), this);
    reset_all_btn_ = new QPushButton(tr("Reset All Menus"), this);
    import_btn_ = new QPushButton(tr("Import..."), this);
    export_btn_ = new QPushButton(tr("Export..."), this);
    global_layout->addWidget(reset_btn_);
    global_layout->addWidget(reset_all_btn_);
    global_layout->addStretch();
    global_layout->addWidget(import_btn_);
    global_layout->addWidget(export_btn_);
    
    right_layout->addWidget(global_group);
    right_layout->addStretch();
    
    // Dialog buttons
    auto* dialog_layout = new QHBoxLayout();
    dialog_layout->addStretch();
    auto* ok_btn = new QPushButton(tr("OK"), this);
    auto* cancel_btn = new QPushButton(tr("Cancel"), this);
    dialog_layout->addWidget(ok_btn);
    dialog_layout->addWidget(cancel_btn);
    right_layout->addLayout(dialog_layout);
    
    layout->addLayout(right_layout, 1);
    
    // Connections
    connect(menu_tree_, &QTreeWidget::itemSelectionChanged, this, &MenuEditorDialog::onItemSelected);
    connect(menu_tree_, &QTreeWidget::itemChanged, this, &MenuEditorDialog::saveMenuState);
    connect(add_menu_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onAddMenu);
    connect(add_action_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onAddAction);
    connect(add_separator_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onAddSeparator);
    connect(remove_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onRemoveItem);
    connect(move_up_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onMoveUp);
    connect(move_down_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onMoveDown);
    connect(move_left_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onMoveLeft);
    connect(move_right_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onMoveRight);
    connect(text_edit_, &QLineEdit::textChanged, this, &MenuEditorDialog::onTextChanged);
    connect(shortcut_edit_, &QKeySequenceEdit::keySequenceChanged, this, &MenuEditorDialog::onShortcutChanged);
    connect(tooltip_edit_, &QLineEdit::textChanged, this, &MenuEditorDialog::onTooltipChanged);
    connect(checkable_check_, &QCheckBox::stateChanged, this, &MenuEditorDialog::onCheckableChanged);
    connect(enabled_check_, &QCheckBox::stateChanged, this, &MenuEditorDialog::onEnabledChanged);
    connect(visible_check_, &QCheckBox::stateChanged, this, &MenuEditorDialog::onVisibleChanged);
    connect(icon_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MenuEditorDialog::onIconChanged);
    connect(reset_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onResetMenu);
    connect(reset_all_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onResetAll);
    connect(import_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onImport);
    connect(export_btn_, &QPushButton::clicked, this, &MenuEditorDialog::onExport);
    connect(ok_btn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
}

void MenuEditorDialog::populateIconCombo() {
    icon_combo_->addItem(tr("(None)"), "");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_FileIcon), tr("File"), "file");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Open"), "open");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save"), "save");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon), tr("Computer"), "computer");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon), tr("Database"), "database");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_BrowserReload), tr("Refresh"), "refresh");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_ArrowBack), tr("Back"), "back");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_ArrowForward), tr("Forward"), "forward");
    icon_combo_->addItem(QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion), tr("Help"), "help");
}

void MenuEditorDialog::setMenuBar(QMenuBar* menuBar) {
    menu_bar_ = menuBar;
    populateMenuTree();
}

void MenuEditorDialog::populateMenuTree() {
    menu_tree_->clear();
    
    if (!menu_bar_) return;
    
    for (QMenu* menu : menu_bar_->findChildren<QMenu*>()) {
        auto* menuItem = new QTreeWidgetItem(menu_tree_, QStringList(menu->title()));
        menuItem->setData(0, Qt::UserRole, "menu");
        populateMenuTreeRecursive(menuItem, menu);
    }
}

void MenuEditorDialog::populateMenuTreeRecursive(QTreeWidgetItem* parent, QMenu* menu) {
    for (QAction* action : menu->actions()) {
        if (action->isSeparator()) {
            auto* sepItem = new QTreeWidgetItem(parent, QStringList("——— Separator ———"));
            sepItem->setData(0, Qt::UserRole, "separator");
            sepItem->setBackground(0, QColor(240, 240, 255));
        } else if (action->menu()) {
            auto* subMenuItem = new QTreeWidgetItem(parent, QStringList(action->text()));
            subMenuItem->setData(0, Qt::UserRole, "menu");
            populateMenuTreeRecursive(subMenuItem, action->menu());
        } else {
            auto* actionItem = new QTreeWidgetItem(parent, QStringList(action->text()));
            actionItem->setData(0, Qt::UserRole, "action");
            if (!action->shortcut().isEmpty()) {
                actionItem->setText(0, action->text() + "\t" + action->shortcut().toString());
            }
        }
    }
}

void MenuEditorDialog::onItemSelected() {
    auto* item = menu_tree_->currentItem();
    current_item_ = item;
    
    updatePropertiesPanel();
}

void MenuEditorDialog::updatePropertiesPanel() {
    bool hasSelection = current_item_ != nullptr;
    bool isSeparator = hasSelection && current_item_->data(0, Qt::UserRole).toString() == "separator";
    
    props_group_->setEnabled(hasSelection && !isSeparator);
    
    if (hasSelection && !isSeparator) {
        text_edit_->setText(current_item_->text(0));
        // Load other properties from item data
    }
}

void MenuEditorDialog::onAddMenu() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("New Menu"),
                                         tr("Menu name:"), QLineEdit::Normal,
                                         tr("New Menu"), &ok);
    if (ok && !text.isEmpty()) {
        auto* item = new QTreeWidgetItem(QStringList(text));
        item->setData(0, Qt::UserRole, "menu");
        
        auto* parent = menu_tree_->currentItem();
        if (parent) {
            parent->addChild(item);
            parent->setExpanded(true);
        } else {
            menu_tree_->addTopLevelItem(item);
        }
    }
}

void MenuEditorDialog::onAddAction() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("New Action"),
                                         tr("Action name:"), QLineEdit::Normal,
                                         tr("New Action"), &ok);
    if (ok && !text.isEmpty()) {
        auto* item = new QTreeWidgetItem(QStringList(text));
        item->setData(0, Qt::UserRole, "action");
        
        auto* parent = menu_tree_->currentItem();
        if (parent && parent->data(0, Qt::UserRole).toString() == "menu") {
            parent->addChild(item);
        } else if (parent && parent->parent()) {
            parent->parent()->addChild(item);
        } else {
            menu_tree_->addTopLevelItem(item);
        }
    }
}

void MenuEditorDialog::onAddSeparator() {
    auto* item = new QTreeWidgetItem(QStringList("——— Separator ———"));
    item->setData(0, Qt::UserRole, "separator");
    item->setBackground(0, QColor(240, 240, 255));
    
    auto* parent = menu_tree_->currentItem();
    if (parent) {
        parent->addChild(item);
    } else {
        menu_tree_->addTopLevelItem(item);
    }
}

void MenuEditorDialog::onRemoveItem() {
    if (!current_item_) return;
    
    delete menu_tree_->takeTopLevelItem(menu_tree_->indexOfTopLevelItem(current_item_));
    
    // Also handle child items
    if (current_item_->parent()) {
        current_item_->parent()->removeChild(current_item_);
        delete current_item_;
    }
    
    current_item_ = nullptr;
    updatePropertiesPanel();
}

void MenuEditorDialog::onMoveUp() {
    if (!current_item_) return;
    
    int index = menu_tree_->indexOfTopLevelItem(current_item_);
    if (index > 0) {
        auto* item = menu_tree_->takeTopLevelItem(index);
        menu_tree_->insertTopLevelItem(index - 1, item);
        menu_tree_->setCurrentItem(item);
    } else if (current_item_->parent()) {
        auto* parent = current_item_->parent();
        index = parent->indexOfChild(current_item_);
        if (index > 0) {
            auto* item = parent->takeChild(index);
            parent->insertChild(index - 1, item);
            menu_tree_->setCurrentItem(item);
        }
    }
}

void MenuEditorDialog::onMoveDown() {
    if (!current_item_) return;
    
    int index = menu_tree_->indexOfTopLevelItem(current_item_);
    if (index >= 0 && index < menu_tree_->topLevelItemCount() - 1) {
        auto* item = menu_tree_->takeTopLevelItem(index);
        menu_tree_->insertTopLevelItem(index + 1, item);
        menu_tree_->setCurrentItem(item);
    } else if (current_item_->parent()) {
        auto* parent = current_item_->parent();
        index = parent->indexOfChild(current_item_);
        if (index < parent->childCount() - 1) {
            auto* item = parent->takeChild(index);
            parent->insertChild(index + 1, item);
            menu_tree_->setCurrentItem(item);
        }
    }
}

void MenuEditorDialog::onMoveLeft() {
    // Move item out of submenu to parent level
    if (!current_item_ || !current_item_->parent()) return;
    
    auto* parent = current_item_->parent();
    int index = parent->indexOfChild(current_item_);
    auto* item = parent->takeChild(index);
    
    if (parent->parent()) {
        int parentIndex = parent->parent()->indexOfChild(parent);
        parent->parent()->insertChild(parentIndex + 1, item);
    } else {
        int parentIndex = menu_tree_->indexOfTopLevelItem(parent);
        menu_tree_->insertTopLevelItem(parentIndex + 1, item);
    }
    menu_tree_->setCurrentItem(item);
}

void MenuEditorDialog::onMoveRight() {
    // Make item a child of the item above
    if (!current_item_) return;
    
    // Implementation would move item into previous sibling
}

void MenuEditorDialog::onTextChanged() {
    if (!current_item_) return;
    current_item_->setText(0, text_edit_->text());
}

void MenuEditorDialog::onShortcutChanged() {
    // Update shortcut
}

void MenuEditorDialog::onTooltipChanged() {
    // Update tooltip
}

void MenuEditorDialog::onCheckableChanged(int state) {
    Q_UNUSED(state)
}

void MenuEditorDialog::onEnabledChanged(int state) {
    Q_UNUSED(state)
}

void MenuEditorDialog::onVisibleChanged(int state) {
    Q_UNUSED(state)
}

void MenuEditorDialog::onIconChanged(int index) {
    Q_UNUSED(index)
}

void MenuEditorDialog::onResetMenu() {
    if (QMessageBox::question(this, tr("Reset Menu"), 
        tr("Reset current menu to defaults?")) == QMessageBox::Yes) {
        populateMenuTree();
    }
}

void MenuEditorDialog::onResetAll() {
    if (QMessageBox::question(this, tr("Reset All Menus"), 
        tr("Reset all menus to defaults?")) == QMessageBox::Yes) {
        populateMenuTree();
        emit menusReset();
    }
}

void MenuEditorDialog::onImport() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Import Menu Configuration"),
                                                    QString(), tr("JSON Files (*.json)"));
    if (!filename.isEmpty()) {
        // Import logic
    }
}

void MenuEditorDialog::onExport() {
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Menu Configuration"),
                                                    QString(), tr("JSON Files (*.json)"));
    if (!filename.isEmpty()) {
        QJsonObject root;
        QJsonArray menus;
        // Export menu structure
        root["menus"] = menus;
        
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            file.close();
        }
    }
}

void MenuEditorDialog::saveMenuState() {
    emit menuModified();
}

} // namespace scratchrobin::ui
