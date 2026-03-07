#pragma once
#include <QDialog>
#include <QKeySequence>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QPushButton;
class QKeySequenceEdit;
QT_END_NAMESPACE

#include <QTreeWidgetItem>

namespace scratchrobin::ui {

/**
 * @brief Keyboard shortcuts reference and customization dialog
 * 
 * Displays all keyboard shortcuts organized by category
 * Allows viewing and potentially customizing shortcuts
 */
class ShortcutsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutsDialog(QWidget* parent = nullptr);
    ~ShortcutsDialog() override;

private slots:
    void onSearchTextChanged();
    void onShortcutSelected();
    void onResetToDefaults();
    void onExportShortcuts();
    void onImportShortcuts();

private:
    void setupUi();
    void populateShortcuts();
    QTreeWidgetItem* addShortcutCategory(const QString& category);
    void addShortcutItem(QTreeWidgetItem* parent, const QString& action, 
                         const QString& shortcut, const QString& description);

    QTreeWidget* shortcutsTree_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QPushButton* resetBtn_ = nullptr;
    QPushButton* exportBtn_ = nullptr;
    QPushButton* importBtn_ = nullptr;
    QPushButton* closeBtn_ = nullptr;
};

} // namespace scratchrobin::ui
