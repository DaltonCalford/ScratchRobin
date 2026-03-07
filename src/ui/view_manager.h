#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QTreeView;
class QTextEdit;
class QSplitter;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QCheckBox;
class QTabWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief View Manager - Database views management
 * 
 * Implements view creation, modification, and management
 * - List all views in database
 * - Create new views
 * - Edit view definitions
 * - Show view dependencies
 * - View data preview
 */

// ============================================================================
// View Info Structure
// ============================================================================
struct ViewInfo {
    QString name;
    QString schema;
    QString definition;
    QStringList columns;
    bool isSystem = false;
    bool isMaterialized = false;
    QDateTime created;
    QDateTime modified;
    QString owner;
    QString comment;
};

// ============================================================================
// View Manager Panel (Dockable)
// ============================================================================
class ViewManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ViewManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("View Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateView();
    void onEditView();
    void onDropView();
    void onRefreshView();
    void onShowViewData();
    void onShowViewDefinition();
    void onShowDependencies();
    void onDuplicateView();
    void onRenameView();
    void onExportView();
    void onImportView();
    void onFilterChanged(const QString& filter);

signals:
    void viewSelected(const QString& viewName);
    void viewOpened(const QString& viewName, const QString& definition);
    void dataPreviewRequested(const QString& viewName);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupToolbar();
    void setupModel();
    void loadViews();
    void updateViewDetails();
    
    backend::SessionClient* client_;
    
    QTreeView* viewTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QTextEdit* definitionPreview_ = nullptr;
    
    QPushButton* createBtn_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* dropBtn_ = nullptr;
    QPushButton* refreshBtn_ = nullptr;
    QPushButton* dataBtn_ = nullptr;
};

// ============================================================================
// View Designer Dialog
// ============================================================================
class ViewDesignerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ViewDesignerDialog(backend::SessionClient* client, 
                                const QString& viewName = QString(),
                                QWidget* parent = nullptr);

    QString generatedSql() const;

public slots:
    void onValidate();
    void onPreview();
    void onSave();
    void onFormatSql();
    void onCheckSyntax();
    void onToggleOptions();

private:
    void setupUi();
    void loadView(const QString& viewName);
    void buildQuery();
    void updatePreview();
    QTabWidget* tabWidget() const;
    
    backend::SessionClient* client_;
    QString originalViewName_;
    bool isEditing_ = false;
    
    // Definition tab
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* definitionEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    
    // Options tab
    QCheckBox* checkOptionCheck_ = nullptr;
    QCheckBox* readOnlyCheck_ = nullptr;
    QCheckBox* materializedCheck_ = nullptr;
    QLineEdit* commentEdit_ = nullptr;
    
    // Preview
    QTableView* previewGrid_ = nullptr;
    QTextEdit* sqlPreview_ = nullptr;
};

// ============================================================================
// View Data Preview Dialog
// ============================================================================
class ViewDataDialog : public QDialog {
    Q_OBJECT

public:
    explicit ViewDataDialog(backend::SessionClient* client,
                           const QString& viewName,
                           QWidget* parent = nullptr);

private slots:
    void onRefresh();
    void onFilter();
    void onSort();
    void onExport();
    void onPageChanged(int page);

private:
    void setupUi();
    void loadData();
    
    backend::SessionClient* client_;
    QString viewName_;
    
    QTableView* dataGrid_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QComboBox* sortCombo_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    
    int currentPage_ = 1;
    int pageSize_ = 100;
    int totalRows_ = 0;
};

// ============================================================================
// View Dependencies Dialog
// ============================================================================
class ViewDependenciesDialog : public QDialog {
    Q_OBJECT

public:
    explicit ViewDependenciesDialog(backend::SessionClient* client,
                                   const QString& viewName,
                                   QWidget* parent = nullptr);

private:
    void setupUi();
    void loadDependencies();
    void loadDependents();
    
    backend::SessionClient* client_;
    QString viewName_;
    
    QTreeView* dependenciesTree_ = nullptr;
    QTreeView* dependentsTree_ = nullptr;
};

} // namespace scratchrobin::ui
