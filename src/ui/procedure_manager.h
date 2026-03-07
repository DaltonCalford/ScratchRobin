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
class QTableWidget;
class QCheckBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Procedure/Function Manager - Stored procedure and function administration
 * 
 * - List all stored procedures and functions
 * - Create/edit/drop procedures
 * - Manage parameters (add/remove/edit)
 * - View source code
 * - Compile/validate procedures
 * - Execute procedures with parameters
 */

// ============================================================================
// Procedure Info
// ============================================================================
struct ProcedureInfo {
    QString name;
    QString schema;
    QString type;  // PROCEDURE, FUNCTION, TRIGGER, PACKAGE
    QString returnType;  // For functions
    QString language;  // SQL, PLpgSQL, C, JAVA, etc.
    bool isDeterministic = false;
    int argCount = 0;
    QString security;  // INVOKER, DEFINER
    QDateTime created;
    QDateTime lastModified;
    bool isValid = true;
};

struct ProcedureParameter {
    QString name;
    QString mode;  // IN, OUT, INOUT
    QString dataType;
    QString defaultValue;
    bool hasDefault = false;
};

// ============================================================================
// Procedure Manager Panel
// ============================================================================
class ProcedureManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ProcedureManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Procedure Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateProcedure();
    void onEditProcedure();
    void onDropProcedure();
    void onCompileProcedure();
    void onExecuteProcedure();
    void onFilterChanged(const QString& filter);
    void onProcedureSelected(const QModelIndex& index);

signals:
    void procedureSelected(const QString& name);
    void editProcedure(const QString& name);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadProcedures();
    void updateDetails(const ProcedureInfo& info);
    void showSource(const QString& name);
    
    backend::SessionClient* client_;
    
    QTreeView* procedureTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Procedure Editor Dialog
// ============================================================================
class ProcedureEditorDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { CreateProcedure, CreateFunction, Edit };
    
    explicit ProcedureEditorDialog(backend::SessionClient* client,
                                   Mode mode,
                                   const QString& procedureName = QString(),
                                   QWidget* parent = nullptr);

public slots:
    void onAddParameter();
    void onRemoveParameter();
    void onMoveParameterUp();
    void onMoveParameterDown();
    void onValidate();
    void onCompile();
    void onSave();
    void onPreview();

private:
    void setupUi();
    void loadProcedure(const QString& name);
    QString generateDdl();
    void updateParameterTable();
    
    backend::SessionClient* client_;
    Mode mode_;
    QString originalName_;
    QList<ProcedureParameter> parameters_;
    
    // Identity
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;  // PROCEDURE or FUNCTION
    
    // Parameters
    QTableWidget* paramTable_ = nullptr;
    QPushButton* addParamBtn_ = nullptr;
    QPushButton* removeParamBtn_ = nullptr;
    QPushButton* moveUpBtn_ = nullptr;
    QPushButton* moveDownBtn_ = nullptr;
    
    // Options
    QComboBox* languageCombo_ = nullptr;
    QComboBox* returnTypeCombo_ = nullptr;
    QCheckBox* deterministicCheck_ = nullptr;
    QComboBox* securityCombo_ = nullptr;
    
    // Code
    QTextEdit* bodyEdit_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
    
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Parameter Edit Dialog
// ============================================================================
class ParameterEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit ParameterEditDialog(ProcedureParameter* param, QWidget* parent = nullptr);

private:
    void setupUi();
    
    ProcedureParameter* param_;
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* modeCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QCheckBox* hasDefaultCheck_ = nullptr;
    QLineEdit* defaultEdit_ = nullptr;
};

// ============================================================================
// Execute Procedure Dialog
// ============================================================================
class ExecuteProcedureDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExecuteProcedureDialog(backend::SessionClient* client,
                                   const QString& procedureName,
                                   const QList<ProcedureParameter>& params,
                                   QWidget* parent = nullptr);



private:
    void setupUi();
    
    backend::SessionClient* client_;
    QString procedureName_;
    QList<ProcedureParameter> params_;
    
    QTableWidget* paramTable_ = nullptr;
    QTextEdit* resultEdit_ = nullptr;
};

} // namespace scratchrobin::ui
