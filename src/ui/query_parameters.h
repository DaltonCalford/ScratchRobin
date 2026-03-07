#pragma once
#include <QDialog>
#include <QVariant>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QStandardItemModel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QTextEdit;
class QTabWidget;
class QListWidget;
class QGroupBox;
class QSpinBox;
class QDoubleSpinBox;
class QDateTimeEdit;
class QCheckBox;
class QFormLayout;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Query Parameters - Parameter binding UI
 * 
 * Manage SQL query parameters:
 * - Named parameter detection (:name, @name, ?)
 * - Type-aware input fields
 * - Parameter validation
 * - Default values and presets
 * - Batch parameter sets
 */

// ============================================================================
// Parameter Types
// ============================================================================
enum class ParameterType {
    String,
    Integer,
    Float,
    Boolean,
    Date,
    DateTime,
    Time,
    Blob,
    Null,
    Auto
};

// ============================================================================
// Query Parameter Structure
// ============================================================================
struct QueryParameter {
    QString name;
    QString displayName;
    ParameterType type;
    QVariant value;
    QVariant defaultValue;
    QString description;
    bool required = true;
    bool isArray = false;  // For IN clauses
    QVariantList arrayValues;
    
    // Validation
    QVariant minValue;
    QVariant maxValue;
    QString validationRegex;
    QStringList allowedValues;  // For enum/combo
    
    // UI hints
    int stringMaxLength = -1;
    int decimalPlaces = 2;
    QString placeholder;
};

struct ParameterPreset {
    QString id;
    QString name;
    QString description;
    QHash<QString, QVariant> values;
    QDateTime createdAt;
};

// ============================================================================
// Query Parameters Dialog
// ============================================================================
class QueryParametersDialog : public QDialog {
    Q_OBJECT

public:
    explicit QueryParametersDialog(const QList<QueryParameter>& parameters,
                                   backend::SessionClient* client,
                                   QWidget* parent = nullptr);
    
    QHash<QString, QVariant> parameterValues() const;

public slots:
    void onParameterChanged(const QString& name, const QVariant& value);
    void onLoadPreset();
    void onSavePreset();
    void onDeletePreset();
    void onResetDefaults();
    void onClearAll();
    void onValidate();
    void onExecute();

signals:
    void parametersChanged(const QHash<QString, QVariant>& values);
    void executeRequested(const QHash<QString, QVariant>& values);

private:
    void setupUi();
    void createParameterInputs();
    void updateParameterValue(const QString& name, const QVariant& value);
    bool validateParameters();
    void loadPresets();
    
    QList<QueryParameter> parameters_;
    backend::SessionClient* client_;
    QHash<QString, QWidget*> inputWidgets_;
    QHash<QString, QVariant> currentValues_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QWidget* paramsTab_ = nullptr;
    QWidget* presetsTab_ = nullptr;
    QFormLayout* paramsContainerLayout_ = nullptr;
    QListWidget* presetList_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* executeBtn_ = nullptr;
};

// ============================================================================
// Parameter Input Widget
// ============================================================================
class ParameterInputWidget : public QWidget {
    Q_OBJECT

public:
    explicit ParameterInputWidget(const QueryParameter& param, QWidget* parent = nullptr);
    
    QVariant value() const;
    void setValue(const QVariant& value);
    bool isValid() const;
    QString errorMessage() const;

signals:
    void valueChanged(const QVariant& value);
    void validationChanged(bool isValid);

private:
    void setupUi();
    void createInputForType();
    void validate();
    
    QueryParameter param_;
    QWidget* inputWidget_ = nullptr;
    QLabel* errorLabel_ = nullptr;
    bool isValid_ = true;
};

// ============================================================================
// Parameter Detection Utility
// ============================================================================
class ParameterDetector {
public:
    static QList<QueryParameter> detectParameters(const QString& sql);
    static QList<QueryParameter> detectNamedParameters(const QString& sql);
    static QList<QueryParameter> detectPositionalParameters(const QString& sql);
    static QString replaceParameters(const QString& sql, const QHash<QString, QVariant>& values);
    
private:
    static ParameterType inferType(const QString& context);
};

// ============================================================================
// Parameter Preset Manager
// ============================================================================
class ParameterPresetManager : public QObject {
    Q_OBJECT

public:
    explicit ParameterPresetManager(QObject* parent = nullptr);
    
    QList<ParameterPreset> getPresetsForQuery(const QString& queryHash) const;
    void savePreset(const QString& queryHash, const ParameterPreset& preset);
    void deletePreset(const QString& queryHash, const QString& presetId);
    ParameterPreset loadPreset(const QString& queryHash, const QString& presetId) const;

signals:
    void presetSaved(const QString& queryHash, const ParameterPreset& preset);
    void presetDeleted(const QString& queryHash, const QString& presetId);

private:
    QString getStoragePath() const;
};

// ============================================================================
// Batch Parameters Dialog
// ============================================================================
class BatchParametersDialog : public QDialog {
    Q_OBJECT

public:
    explicit BatchParametersDialog(const QList<QueryParameter>& parameters,
                                   QWidget* parent = nullptr);
    
    QList<QHash<QString, QVariant>> getParameterSets() const;

public slots:
    void onAddRow();
    void onRemoveRow();
    void onDuplicateRow();
    void onImportCSV();
    void onExportCSV();
    void onLoadTemplate();
    void onSaveTemplate();

private:
    void setupUi();
    void addParameterSetRow(const QHash<QString, QVariant>& values = {});
    
    QList<QueryParameter> parameters_;
    QTableView* tableView_ = nullptr;
    QStandardItemModel* tableModel_ = nullptr;
};

// ============================================================================
// Quick Parameter Toolbar
// ============================================================================
class QuickParameterToolbar : public QWidget {
    Q_OBJECT

public:
    explicit QuickParameterToolbar(QWidget* parent = nullptr);
    
    void setParameters(const QList<QueryParameter>& parameters);

public slots:
    void onParameterEdited();
    void onShowFullDialog();
    void onResetDefaults();

signals:
    void parametersChanged(const QHash<QString, QVariant>& values);
    void dialogRequested();

private:
    void setupUi();
    void rebuildInputs();
    
    QList<QueryParameter> parameters_;
    QHash<QString, QLineEdit*> quickInputs_;
};

} // namespace scratchrobin::ui
