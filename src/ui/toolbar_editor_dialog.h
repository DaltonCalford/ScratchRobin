#pragma once
#include <QDialog>
#include <QToolBar>
#include <QAction>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QComboBox;
class QLabel;
class QLineEdit;
class QCheckBox;
class QListWidgetItem;
QT_END_NAMESPACE

namespace scratchrobin::ui {

struct ToolbarAction {
    QString id;
    QString text;
    QString iconName;
    QString shortcut;
    QString category;
    bool separator = false;
};

class ToolbarEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ToolbarEditorDialog(QWidget* parent = nullptr);
    ~ToolbarEditorDialog() override;

    void setCurrentToolbar(QToolBar* toolbar);
    void loadToolbars(const QList<QToolBar*>& toolbars);

signals:
    void toolbarModified(QToolBar* toolbar);
    void toolbarsReset();

private slots:
    void onToolbarChanged(int index);
    void onAddAction();
    void onRemoveAction();
    void onMoveUp();
    void onMoveDown();
    void onAddSeparator();
    void onResetToolbar();
    void onResetAll();
    void onIconSizeChanged(int index);
    void onShowTextChanged(int state);
    void onActionSelected();
    void onCurrentActionSelected();
    void onNewToolbar();
    void onDeleteToolbar();
    void onImport();
    void onExport();

private:
    void setupUi();
    void populateAvailableActions();
    void populateCurrentToolbar();
    void saveCurrentToolbarState();
    
    QList<ToolbarAction> getAvailableActions() const;
    
    QComboBox* toolbar_combo_;
    QListWidget* available_list_;
    QListWidget* current_list_;
    
    QPushButton* add_btn_;
    QPushButton* remove_btn_;
    QPushButton* move_up_btn_;
    QPushButton* move_down_btn_;
    QPushButton* separator_btn_;
    QPushButton* new_toolbar_btn_;
    QPushButton* delete_toolbar_btn_;
    QPushButton* reset_btn_;
    QPushButton* reset_all_btn_;
    QPushButton* import_btn_;
    QPushButton* export_btn_;
    
    QComboBox* icon_size_combo_;
    QCheckBox* show_text_check_;
    QLineEdit* toolbar_name_edit_;
    
    QToolBar* current_toolbar_ = nullptr;
    QList<QToolBar*> all_toolbars_;
    
    // Default actions database
    QList<ToolbarAction> action_database_;
};

} // namespace scratchrobin::ui
