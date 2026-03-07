#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QCheckBox;
class QListWidget;
class QSpinBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Routine Manager - Functions and Procedures management
 * 
 * - List all functions and procedures
 * - Create/edit/drop routines
 * - View routine source code
 * - Execute routines with parameters
 * - Manage routine privileges
 * - Debug support (when available)
 */

// ============================================================================
// Routine Parameter Info
// ============================================================================
struct RoutineParameter {
    QString name;
    QString dataType;
    QString mode;  // IN, OUT, INOUT
    QVariant defaultValue;
    bool hasDefault = false;
};

// ============================================================================
// Routine Info
// ============================================================================
struct RoutineInfo {
    QString name;
    QString schema;
    QString type;  // FUNCTION, PROCEDURE
    QString language;  // SQL, PLPGSQL, C, etc.
    QString returnType;  // For functions
    QList<RoutineParameter> parameters;
    QString definition;  // Source code
    QString owner;
    bool isSecurityDefiner = false;
    bool isLeakproof = false;
    QString volatility;  // IMMUTABLE, STABLE, VOLATILE
    int cost = 100;
    int rows = 1000;  // For functions returning sets
    QDateTime created;
    QString comment;
};

// ============================================================================
// Routine Manager Panel
// ============================================================================
class RoutineManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit RoutineManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Routine Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateFunction();
    void onCreateProcedure();
    void onEditRoutine();
    void onDropRoutine();
    void onExecuteRoutine();
    void onShowSource();
    void onShowDependencies();
    void onFilterChanged(const QString& filter);
    void onTypeFilterChanged(int type);

signals:
    void routineSelected(const QString& name, const QString& type);
    void routineExecuteRequested(const QString& name, const QList<QVariant>& args);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadRoutines();
    void updateSourcePreview();
    
    backend::SessionClient* client_;
    
    QComboBox* typeFilterCombo_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QTreeView* routineTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QTextEdit* sourcePreview_ = nullptr;
    
    QPushButton* execBtn_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* dropBtn_ = nullptr;
};

// ============================================================================
// Routine Editor Dialog
// ============================================================================
class RoutineEditorDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { CreateFunction, CreateProcedure, Edit };
    
    explicit RoutineEditorDialog(backend::SessionClient* client,
                                Mode mode,
                                const QString& routineName = QString(),
                                QWidget* parent = nullptr);

public slots:
    void onAddParameter();
    void onRemoveParameter();
    void onMoveParameterUp();
    void onMoveParameterDown();
    void onValidate();
    void onSave();
    void onFormatCode();

private:
    void setupUi();
    void setupParametersTab();
    void setupOptionsTab();
    void setupSourceTab();
    void loadRoutine(const QString& name);
    void updateParameterList();
    void updatePreview();
    QString generateDdl();
    
    backend::SessionClient* client_;
    Mode mode_;
    QString originalName_;
    QList<RoutineParameter> parameters_;
    
    // Identity
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* languageCombo_ = nullptr;
    
    // Parameters
    QTableView* paramTable_ = nullptr;
    QStandardItemModel* paramModel_ = nullptr;
    
    // Return type (functions only)
    QComboBox* returnTypeCombo_ = nullptr;
    QCheckBox* returnsSetCheck_ = nullptr;
    
    // Options
    QComboBox* volatilityCombo_ = nullptr;
    QCheckBox* securityDefinerCheck_ = nullptr;
    QCheckBox* leakproofCheck_ = nullptr;
    QSpinBox* costSpin_ = nullptr;
    QSpinBox* rowsSpin_ = nullptr;
    
    // Source
    QTextEdit* sourceEdit_ = nullptr;
    QTextEdit* sqlPreview_ = nullptr;
};

// ============================================================================
// Routine Execution Dialog
// ============================================================================
class RoutineExecutionDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutineExecutionDialog(backend::SessionClient* client,
                                   const RoutineInfo& routine,
                                   QWidget* parent = nullptr);

public slots:
    void onExecute();
    void onSavePreset();
    void onLoadPreset();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    RoutineInfo routine_;
    
    QTableView* paramTable_ = nullptr;
    QStandardItemModel* paramModel_ = nullptr;
    QTextEdit* resultEdit_ = nullptr;
};

} // namespace scratchrobin::ui
