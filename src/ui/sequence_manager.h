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
class QSpinBox;
class QLabel;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Sequence Manager - Database sequence management
 * 
 * - List all sequences
 * - Create/edit/drop sequences
 * - View sequence current value and stats
 * - Alter sequence parameters
 * - Set/advance sequence values (with safeguards)
 */

// ============================================================================
// Sequence Info
// ============================================================================
struct SequenceInfo {
    QString name;
    QString schema;
    QString dataType;  // INTEGER, BIGINT, etc.
    qint64 startValue = 1;
    qint64 increment = 1;
    qint64 minValue = 1;
    qint64 maxValue = 9223372036854775807LL;  // MAX BIGINT
    qint64 currentValue = 0;
    qint64 cacheSize = 1;
    bool cycle = false;
    bool isCalled = false;
    QString owner;
    QString ownedBy;  // Table.column or empty
    QDateTime created;
    QString comment;
};

// ============================================================================
// Sequence Manager Panel
// ============================================================================
class SequenceManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SequenceManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Sequence Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateSequence();
    void onEditSequence();
    void onDropSequence();
    void onShowNextValues();
    void onResetSequence();
    void onSetValue();
    void onFilterChanged(const QString& filter);
    void onSequenceSelected(const QModelIndex& index);

signals:
    void sequenceSelected(const QString& name);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadSequences();
    void updateStats();
    
    backend::SessionClient* client_;
    
    QTreeView* sequenceTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    
    QLabel* currentValueLabel_ = nullptr;
    QLabel* minValueLabel_ = nullptr;
    QLabel* maxValueLabel_ = nullptr;
    QLabel* incrementLabel_ = nullptr;
    QLabel* cycleLabel_ = nullptr;
    QLabel* ownedByLabel_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Sequence Editor Dialog
// ============================================================================
class SequenceEditorDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Create, Edit };
    
    explicit SequenceEditorDialog(backend::SessionClient* client,
                                 Mode mode,
                                 const QString& sequenceName = QString(),
                                 QWidget* parent = nullptr);

public slots:
    void onSave();
    void onPreview();

private:
    void setupUi();
    void loadSequence(const QString& name);
    QString generateDdl();
    
    backend::SessionClient* client_;
    Mode mode_;
    QString originalName_;
    
    // Identity
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    
    // Parameters
    QSpinBox* startSpin_ = nullptr;
    QSpinBox* incrementSpin_ = nullptr;
    QSpinBox* minValueSpin_ = nullptr;
    QSpinBox* maxValueSpin_ = nullptr;
    QSpinBox* cacheSpin_ = nullptr;
    QCheckBox* cycleCheck_ = nullptr;
    QCheckBox* orderedCheck_ = nullptr;
    
    // Ownership
    QComboBox* ownedByTableCombo_ = nullptr;
    QComboBox* ownedByColumnCombo_ = nullptr;
    
    // Preview
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Set Sequence Value Dialog
// ============================================================================
class SetSequenceValueDialog : public QDialog {
    Q_OBJECT

public:
    explicit SetSequenceValueDialog(backend::SessionClient* client,
                                   const QString& sequenceName,
                                   qint64 currentValue,
                                   QWidget* parent = nullptr);

private:
    void setupUi();
    
    backend::SessionClient* client_;
    QString sequenceName_;
    qint64 currentValue_;
    
    QLabel* currentLabel_ = nullptr;
    QSpinBox* newValueSpin_ = nullptr;
    QSpinBox* advanceSpin_ = nullptr;
    QCheckBox* isCalledCheck_ = nullptr;
};

// ============================================================================
// Show Next Values Dialog
// ============================================================================
class ShowNextValuesDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShowNextValuesDialog(backend::SessionClient* client,
                                 const QString& sequenceName,
                                 QWidget* parent = nullptr);



private:
    void setupUi();
    void loadNextValues();
    
    backend::SessionClient* client_;
    QString sequenceName_;
    
    QTableView* valuesTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QSpinBox* countSpin_ = nullptr;
};

} // namespace scratchrobin::ui
