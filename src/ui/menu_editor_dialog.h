#pragma once
#include <QDialog>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLineEdit;
class QComboBox;
class QLabel;
class QKeySequenceEdit;
class QCheckBox;
class QGroupBox;
class QIcon;
QT_END_NAMESPACE

namespace scratchrobin::ui {

struct MenuItem {
    QString id;
    QString text;
    QString shortcut;
    QString iconName;
    QString tooltip;
    bool separator = false;
    bool checkable = false;
    bool checked = false;
    bool enabled = true;
    bool visible = true;
    QList<MenuItem> children;
};

class MenuEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit MenuEditorDialog(QWidget* parent = nullptr);
    ~MenuEditorDialog() override;

    void setMenuBar(QMenuBar* menuBar);

signals:
    void menuModified();
    void menusReset();

private slots:
    void onItemSelected();
    void onAddMenu();
    void onAddAction();
    void onAddSeparator();
    void onRemoveItem();
    void onMoveUp();
    void onMoveDown();
    void onMoveLeft();
    void onMoveRight();
    void onTextChanged();
    void onShortcutChanged();
    void onTooltipChanged();
    void onCheckableChanged(int state);
    void onEnabledChanged(int state);
    void onVisibleChanged(int state);
    void onIconChanged(int index);
    void onResetMenu();
    void onResetAll();
    void onImport();
    void onExport();

private:
    void setupUi();
    void populateMenuTree();
    void populateMenuTreeRecursive(QTreeWidgetItem* parent, QMenu* menu);
    void updatePropertiesPanel();
    void saveMenuState();
    void populateIconCombo();
    
    QMenuBar* menu_bar_ = nullptr;
    
    // Tree view
    QTreeWidget* menu_tree_;
    
    // Buttons
    QPushButton* add_menu_btn_;
    QPushButton* add_action_btn_;
    QPushButton* add_separator_btn_;
    QPushButton* remove_btn_;
    QPushButton* move_up_btn_;
    QPushButton* move_down_btn_;
    QPushButton* move_left_btn_;
    QPushButton* move_right_btn_;
    
    // Properties panel
    QGroupBox* props_group_;
    QLineEdit* text_edit_;
    QKeySequenceEdit* shortcut_edit_;
    QLineEdit* tooltip_edit_;
    QComboBox* icon_combo_;
    QCheckBox* checkable_check_;
    QCheckBox* enabled_check_;
    QCheckBox* visible_check_;
    QLabel* id_label_;
    
    // Bottom buttons
    QPushButton* reset_btn_;
    QPushButton* reset_all_btn_;
    QPushButton* import_btn_;
    QPushButton* export_btn_;
    
    QTreeWidgetItem* current_item_ = nullptr;
};

} // namespace scratchrobin::ui
