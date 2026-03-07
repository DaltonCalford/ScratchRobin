#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QListWidget;
class QSplitter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Package Manager - PL/SQL-style package administration
 * 
 * Supports Oracle-style packages with separate specification and body.
 * Each package has:
 * - Specification: Public constants, types, variables, functions, procedures
 * - Body: Implementation of spec + private declarations
 * 
 * ScratchBird packages follow Oracle PL/SQL syntax:
 *   CREATE [OR REPLACE] PACKAGE [schema.]name [AUTHID {CURRENT_USER|DEFINER}] IS ... END;
 *   CREATE [OR REPLACE] PACKAGE BODY [schema.]name IS ... END;
 */

// ============================================================================
// Package Info
// ============================================================================
struct PackageInfo {
    QString name;
    QString schema;
    QString status;  // VALID, INVALID
    bool hasBody = false;
    bool isInvokerRights = false;  // AUTHID CURRENT_USER
    QDateTime created;
    QDateTime lastModified;
};

struct PackageMember {
    QString name;
    QString memberType;  // CONSTANT, VARIABLE, TYPE, CURSOR, FUNCTION, PROCEDURE
    QString dataType;    // For constants, variables, return types
    bool isPublic = true;  // From spec vs body-only
};

// ============================================================================
// Package Manager Panel
// ============================================================================
class PackageManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit PackageManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Package Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreatePackage();
    void onEditPackage();
    void onDropPackage();
    void onCompilePackage();
    void onShowErrors();
    void onFilterChanged(const QString& filter);
    void onPackageSelected(const QModelIndex& index);

signals:
    void packageSelected(const QString& name);
    void editPackageSpec(const QString& name);
    void editPackageBody(const QString& name);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadPackages();
    void loadPackageMembers(const QString& packageName);
    void showPackageSource(const QString& packageName, bool showBody);
    
    backend::SessionClient* client_;
    
    QTreeView* packageTree_ = nullptr;
    QTreeView* memberTree_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
    QStandardItemModel* packageModel_ = nullptr;
    QStandardItemModel* memberModel_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    
    QPushButton* createBtn_ = nullptr;
    QPushButton* editSpecBtn_ = nullptr;
    QPushButton* editBodyBtn_ = nullptr;
    QPushButton* dropBtn_ = nullptr;
    QPushButton* compileBtn_ = nullptr;
};

// ============================================================================
// Package Editor Dialog
// ============================================================================
class PackageEditorDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Create, EditSpec, EditBody };
    
    explicit PackageEditorDialog(backend::SessionClient* client,
                                 Mode mode,
                                 const QString& packageName = QString(),
                                 QWidget* parent = nullptr);

public slots:
    void onValidate();
    void onCompile();
    void onSave();

private:
    void setupUi();
    void loadPackage(const QString& name);
    void loadPackageSource(bool loadBody);
    QString generateSpecTemplate();
    QString generateBodyTemplate();
    bool compilePackage(const QString& specSql, const QString& bodySql);
    
    backend::SessionClient* client_;
    Mode mode_;
    QString originalName_;
    bool hasBody_ = false;
    
    // Identity
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* authIdCombo_ = nullptr;
    
    // Editors
    QTabWidget* editorTabs_ = nullptr;
    QTextEdit* specEdit_ = nullptr;
    QTextEdit* bodyEdit_ = nullptr;
    
    // Status
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Package Dependencies Dialog
// ============================================================================
class PackageDependenciesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PackageDependenciesDialog(backend::SessionClient* client,
                                      const QString& packageName,
                                      QWidget* parent = nullptr);

private:
    void setupUi();
    void loadDependencies();
    void loadDependents();
    
    backend::SessionClient* client_;
    QString packageName_;
    
    QTreeView* dependsOnTree_ = nullptr;
    QTreeView* dependedOnByTree_ = nullptr;
};

} // namespace scratchrobin::ui
