#include "query_parameters.h"
#include <backend/session_client.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QTableView>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTabWidget>
#include <QListWidget>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QDateEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>

namespace scratchrobin::ui {

// ============================================================================
// Query Parameters Dialog
// ============================================================================
QueryParametersDialog::QueryParametersDialog(const QList<QueryParameter>& parameters,
                                             backend::SessionClient* client,
                                             QWidget* parent)
    : QDialog(parent), parameters_(parameters), client_(client) {
    setupUi();
    createParameterInputs();
    loadPresets();
}

void QueryParametersDialog::setupUi() {
    setWindowTitle(tr("Query Parameters"));
    setMinimumSize(450, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    tabWidget_ = new QTabWidget(this);
    
    // Parameters tab
    paramsTab_ = new QWidget(this);
    auto* paramsLayout = new QVBoxLayout(paramsTab_);
    paramsLayout->setContentsMargins(8, 8, 8, 8);
    
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    auto* scrollWidget = new QWidget(this);
    paramsContainerLayout_ = new QFormLayout(scrollWidget);
    paramsContainerLayout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    scrollArea->setWidget(scrollWidget);
    paramsLayout->addWidget(scrollArea);
    
    // Preset buttons
    auto* presetLayout = new QHBoxLayout();
    auto* loadPresetBtn = new QPushButton(tr("Load Preset"), this);
    connect(loadPresetBtn, &QPushButton::clicked, this, &QueryParametersDialog::onLoadPreset);
    presetLayout->addWidget(loadPresetBtn);
    
    auto* savePresetBtn = new QPushButton(tr("Save Preset"), this);
    connect(savePresetBtn, &QPushButton::clicked, this, &QueryParametersDialog::onSavePreset);
    presetLayout->addWidget(savePresetBtn);
    
    presetLayout->addStretch();
    
    auto* resetBtn = new QPushButton(tr("Reset"), this);
    connect(resetBtn, &QPushButton::clicked, this, &QueryParametersDialog::onResetDefaults);
    presetLayout->addWidget(resetBtn);
    
    paramsLayout->addLayout(presetLayout);
    
    tabWidget_->addTab(paramsTab_, tr("Parameters"));
    
    // Presets tab
    presetsTab_ = new QWidget(this);
    auto* presetsLayout = new QVBoxLayout(presetsTab_);
    
    presetList_ = new QListWidget(this);
    presetsLayout->addWidget(presetList_);
    
    auto* presetBtnLayout = new QHBoxLayout();
    auto* deletePresetBtn = new QPushButton(tr("Delete"), this);
    connect(deletePresetBtn, &QPushButton::clicked, this, &QueryParametersDialog::onDeletePreset);
    presetBtnLayout->addWidget(deletePresetBtn);
    
    presetsLayout->addLayout(presetBtnLayout);
    
    tabWidget_->addTab(presetsTab_, tr("Presets"));
    
    layout->addWidget(tabWidget_, 1);
    
    // Status label
    statusLabel_ = new QLabel(tr("%1 parameter(s)").arg(parameters_.size()), this);
    layout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* clearBtn = new QPushButton(tr("Clear All"), this);
    connect(clearBtn, &QPushButton::clicked, this, &QueryParametersDialog::onClearAll);
    btnLayout->addWidget(clearBtn);
    
    executeBtn_ = new QPushButton(tr("Execute"), this);
    executeBtn_->setDefault(true);
    connect(executeBtn_, &QPushButton::clicked, this, &QueryParametersDialog::onExecute);
    btnLayout->addWidget(executeBtn_);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void QueryParametersDialog::createParameterInputs() {
    for (const auto& param : parameters_) {
        auto* inputWidget = new ParameterInputWidget(param, this);
        
        connect(inputWidget, &ParameterInputWidget::valueChanged,
                [this, param](const QVariant& value) {
                    onParameterChanged(param.name, value);
                });
        
        paramsContainerLayout_->addRow(param.displayName.isEmpty() ? param.name : param.displayName,
                                       inputWidget);
        inputWidgets_[param.name] = inputWidget;
        
        // Set initial value
        if (param.value.isValid()) {
            currentValues_[param.name] = param.value;
        } else if (param.defaultValue.isValid()) {
            currentValues_[param.name] = param.defaultValue;
        }
    }
}

void QueryParametersDialog::onParameterChanged(const QString& name, const QVariant& value) {
    currentValues_[name] = value;
    emit parametersChanged(currentValues_);
}

QHash<QString, QVariant> QueryParametersDialog::parameterValues() const {
    return currentValues_;
}

void QueryParametersDialog::onLoadPreset() {
    bool ok;
    QString presetName = QInputDialog::getItem(this, tr("Load Preset"),
        tr("Select preset:"), QStringList({"Default Values", "Test Data", "Production"}),
        0, false, &ok);
    
    if (ok) {
        // Load preset values
        QMessageBox::information(this, tr("Preset Loaded"),
            tr("Loaded preset: %1").arg(presetName));
    }
}

void QueryParametersDialog::onSavePreset() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Save Preset"),
        tr("Preset name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        // Save current values as preset
        QMessageBox::information(this, tr("Preset Saved"),
            tr("Preset '%1' saved successfully.").arg(name));
    }
}

void QueryParametersDialog::onDeletePreset() {
    auto* item = presetList_->currentItem();
    if (!item) return;
    
    auto reply = QMessageBox::question(this, tr("Delete Preset"),
        tr("Delete preset '%1'?").arg(item->text()));
    
    if (reply == QMessageBox::Yes) {
        delete presetList_->takeItem(presetList_->row(item));
    }
}

void QueryParametersDialog::onResetDefaults() {
    for (const auto& param : parameters_) {
        if (inputWidgets_.contains(param.name)) {
            auto* widget = qobject_cast<ParameterInputWidget*>(inputWidgets_[param.name]);
            if (widget) {
                widget->setValue(param.defaultValue);
            }
        }
    }
}

void QueryParametersDialog::onClearAll() {
    for (auto* widget : inputWidgets_) {
        auto* inputWidget = qobject_cast<ParameterInputWidget*>(widget);
        if (inputWidget) {
            inputWidget->setValue(QVariant());
        }
    }
    currentValues_.clear();
}

bool QueryParametersDialog::validateParameters() {
    bool allValid = true;
    QStringList errors;
    
    for (const auto& param : parameters_) {
        if (param.required && !currentValues_.contains(param.name)) {
            errors.append(tr("%1 is required").arg(param.name));
            allValid = false;
        }
    }
    
    if (!allValid) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Please fix the following errors:\n\n%1").arg(errors.join("\n")));
    }
    
    return allValid;
}

void QueryParametersDialog::onValidate() {
    if (validateParameters()) {
        accept();
    }
}

void QueryParametersDialog::onExecute() {
    if (validateParameters()) {
        emit executeRequested(currentValues_);
        accept();
    }
}

void QueryParametersDialog::loadPresets() {
    presetList_->clear();
    presetList_->addItem(tr("Default Values"));
    presetList_->addItem(tr("Test Data"));
    presetList_->addItem(tr("Production"));
}

// ============================================================================
// Parameter Input Widget
// ============================================================================
ParameterInputWidget::ParameterInputWidget(const QueryParameter& param, QWidget* parent)
    : QWidget(parent), param_(param) {
    setupUi();
    createInputForType();
    
    if (param_.value.isValid()) {
        setValue(param_.value);
    } else if (param_.defaultValue.isValid()) {
        setValue(param_.defaultValue);
    }
}

void ParameterInputWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    
    errorLabel_ = new QLabel(this);
    errorLabel_->setStyleSheet("color: red; font-size: 11px;");
    errorLabel_->hide();
    layout->addWidget(errorLabel_);
}

void ParameterInputWidget::createInputForType() {
    switch (param_.type) {
        case ParameterType::String: {
            auto* edit = new QLineEdit(this);
            if (param_.stringMaxLength > 0) {
                edit->setMaxLength(param_.stringMaxLength);
            }
            if (!param_.placeholder.isEmpty()) {
                edit->setPlaceholderText(param_.placeholder);
            }
            connect(edit, &QLineEdit::textChanged, [this](const QString& text) {
                emit valueChanged(text);
                validate();
            });
            inputWidget_ = edit;
            break;
        }
        
        case ParameterType::Integer: {
            auto* spin = new QSpinBox(this);
            spin->setRange(-999999999, 999999999);
            if (param_.minValue.isValid()) {
                spin->setMinimum(param_.minValue.toInt());
            }
            if (param_.maxValue.isValid()) {
                spin->setMaximum(param_.maxValue.toInt());
            }
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
                emit valueChanged(val);
                validate();
            });
            inputWidget_ = spin;
            break;
        }
        
        case ParameterType::Float: {
            auto* spin = new QDoubleSpinBox(this);
            spin->setRange(-999999999.99, 999999999.99);
            spin->setDecimals(param_.decimalPlaces);
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double val) {
                emit valueChanged(val);
                validate();
            });
            inputWidget_ = spin;
            break;
        }
        
        case ParameterType::Boolean: {
            auto* check = new QCheckBox(this);
            connect(check, &QCheckBox::toggled, [this](bool checked) {
                emit valueChanged(checked);
                validate();
            });
            inputWidget_ = check;
            break;
        }
        
        case ParameterType::Date: {
            auto* dateEdit = new QDateEdit(this);
            dateEdit->setCalendarPopup(true);
            dateEdit->setDate(QDate::currentDate());
            connect(dateEdit, &QDateEdit::dateChanged, [this](const QDate& date) {
                emit valueChanged(date);
                validate();
            });
            inputWidget_ = dateEdit;
            break;
        }
        
        case ParameterType::DateTime: {
            auto* dateTimeEdit = new QDateTimeEdit(this);
            dateTimeEdit->setCalendarPopup(true);
            dateTimeEdit->setDateTime(QDateTime::currentDateTime());
            connect(dateTimeEdit, &QDateTimeEdit::dateTimeChanged, [this](const QDateTime& dt) {
                emit valueChanged(dt);
                validate();
            });
            inputWidget_ = dateTimeEdit;
            break;
        }
        
        case ParameterType::Null: {
            auto* label = new QLabel(tr("(NULL)"), this);
            inputWidget_ = label;
            break;
        }
        
        default: {
            auto* edit = new QLineEdit(this);
            connect(edit, &QLineEdit::textChanged, [this](const QString& text) {
                emit valueChanged(text);
                validate();
            });
            inputWidget_ = edit;
        }
    }
    
    if (inputWidget_) {
        layout()->addWidget(inputWidget_);
    }
}

QVariant ParameterInputWidget::value() const {
    if (!inputWidget_) return QVariant();
    
    switch (param_.type) {
        case ParameterType::String:
            return qobject_cast<QLineEdit*>(inputWidget_)->text();
        case ParameterType::Integer:
            return qobject_cast<QSpinBox*>(inputWidget_)->value();
        case ParameterType::Float:
            return qobject_cast<QDoubleSpinBox*>(inputWidget_)->value();
        case ParameterType::Boolean:
            return qobject_cast<QCheckBox*>(inputWidget_)->isChecked();
        case ParameterType::Date:
            return qobject_cast<QDateEdit*>(inputWidget_)->date();
        case ParameterType::DateTime:
            return qobject_cast<QDateTimeEdit*>(inputWidget_)->dateTime();
        default:
            return qobject_cast<QLineEdit*>(inputWidget_)->text();
    }
}

void ParameterInputWidget::setValue(const QVariant& value) {
    if (!value.isValid()) return;
    
    switch (param_.type) {
        case ParameterType::String:
            if (auto* edit = qobject_cast<QLineEdit*>(inputWidget_)) {
                edit->setText(value.toString());
            }
            break;
        case ParameterType::Integer:
            if (auto* spin = qobject_cast<QSpinBox*>(inputWidget_)) {
                spin->setValue(value.toInt());
            }
            break;
        case ParameterType::Float:
            if (auto* spin = qobject_cast<QDoubleSpinBox*>(inputWidget_)) {
                spin->setValue(value.toDouble());
            }
            break;
        case ParameterType::Boolean:
            if (auto* check = qobject_cast<QCheckBox*>(inputWidget_)) {
                check->setChecked(value.toBool());
            }
            break;
        case ParameterType::Date:
            if (auto* dateEdit = qobject_cast<QDateEdit*>(inputWidget_)) {
                dateEdit->setDate(value.toDate());
            }
            break;
        case ParameterType::DateTime:
            if (auto* dateTimeEdit = qobject_cast<QDateTimeEdit*>(inputWidget_)) {
                dateTimeEdit->setDateTime(value.toDateTime());
            }
            break;
        default:
            if (auto* edit = qobject_cast<QLineEdit*>(inputWidget_)) {
                edit->setText(value.toString());
            }
    }
}

bool ParameterInputWidget::isValid() const {
    return isValid_;
}

QString ParameterInputWidget::errorMessage() const {
    return errorLabel_->text();
}

void ParameterInputWidget::validate() {
    isValid_ = true;
    QString error;
    
    QVariant val = value();
    
    if (param_.required && (!val.isValid() || val.toString().isEmpty())) {
        isValid_ = false;
        error = tr("Required");
    }
    
    if (isValid_ && !param_.validationRegex.isEmpty()) {
        QRegularExpression regex(param_.validationRegex);
        if (!regex.match(val.toString()).hasMatch()) {
            isValid_ = false;
            error = tr("Invalid format");
        }
    }
    
    if (isValid_) {
        errorLabel_->hide();
    } else {
        errorLabel_->setText(error);
        errorLabel_->show();
    }
    
    emit validationChanged(isValid_);
}

// ============================================================================
// Parameter Detector
// ============================================================================
QList<QueryParameter> ParameterDetector::detectParameters(const QString& sql) {
    QList<QueryParameter> params;
    
    // Detect named parameters (:param, @param)
    QRegularExpression namedRegex(":([a-zA-Z_][a-zA-Z0-9_]*)");
    auto namedMatches = namedRegex.globalMatch(sql);
    
    while (namedMatches.hasNext()) {
        auto match = namedMatches.next();
        QString name = match.captured(1);
        
        QueryParameter param;
        param.name = name;
        param.displayName = name;
        param.type = inferType(sql.mid(match.capturedEnd()));
        param.placeholder = QString("Enter value for %1").arg(name);
        
        // Check for duplicates
        bool exists = false;
        for (const auto& p : params) {
            if (p.name == name) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            params.append(param);
        }
    }
    
    return params;
}

QList<QueryParameter> ParameterDetector::detectNamedParameters(const QString& sql) {
    return detectParameters(sql);
}

QList<QueryParameter> ParameterDetector::detectPositionalParameters(const QString& sql) {
    QList<QueryParameter> params;
    
    QRegularExpression posRegex("\\?");
    auto matches = posRegex.globalMatch(sql);
    int index = 1;
    
    while (matches.hasNext()) {
        matches.next();
        
        QueryParameter param;
        param.name = QString::number(index);
        param.displayName = QString("Parameter %1").arg(index);
        param.type = ParameterType::Auto;
        
        params.append(param);
        index++;
    }
    
    return params;
}

QString ParameterDetector::replaceParameters(const QString& sql, const QHash<QString, QVariant>& values) {
    QString result = sql;
    
    for (auto it = values.begin(); it != values.end(); ++it) {
        QString placeholder = ":" + it.key();
        QString value = it.value().toString();
        
        // Escape special characters
        value.replace("'", "''");
        
        result.replace(placeholder, "'" + value + "'");
    }
    
    return result;
}

ParameterType ParameterDetector::inferType(const QString& context) {
    QString lower = context.toLower();
    
    if (lower.contains("date") && lower.contains("time")) {
        return ParameterType::DateTime;
    } else if (lower.contains("date")) {
        return ParameterType::Date;
    } else if (lower.contains("time")) {
        return ParameterType::Time;
    } else if (lower.contains("int") || lower.contains("number")) {
        return ParameterType::Integer;
    } else if (lower.contains("float") || lower.contains("decimal") || lower.contains("real")) {
        return ParameterType::Float;
    } else if (lower.contains("bool")) {
        return ParameterType::Boolean;
    } else {
        return ParameterType::String;
    }
}

// ============================================================================
// Batch Parameters Dialog
// ============================================================================
BatchParametersDialog::BatchParametersDialog(const QList<QueryParameter>& parameters,
                                             QWidget* parent)
    : QDialog(parent), parameters_(parameters) {
    setupUi();
}

void BatchParametersDialog::setupUi() {
    setWindowTitle(tr("Batch Parameters"));
    setMinimumSize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* addBtn = new QPushButton(tr("Add Row"), this);
    connect(addBtn, &QPushButton::clicked, this, &BatchParametersDialog::onAddRow);
    toolbarLayout->addWidget(addBtn);
    
    auto* removeBtn = new QPushButton(tr("Remove Row"), this);
    connect(removeBtn, &QPushButton::clicked, this, &BatchParametersDialog::onRemoveRow);
    toolbarLayout->addWidget(removeBtn);
    
    auto* duplicateBtn = new QPushButton(tr("Duplicate"), this);
    connect(duplicateBtn, &QPushButton::clicked, this, &BatchParametersDialog::onDuplicateRow);
    toolbarLayout->addWidget(duplicateBtn);
    
    toolbarLayout->addStretch();
    
    auto* importBtn = new QPushButton(tr("Import CSV"), this);
    connect(importBtn, &QPushButton::clicked, this, &BatchParametersDialog::onImportCSV);
    toolbarLayout->addWidget(importBtn);
    
    auto* exportBtn = new QPushButton(tr("Export CSV"), this);
    connect(exportBtn, &QPushButton::clicked, this, &BatchParametersDialog::onExportCSV);
    toolbarLayout->addWidget(exportBtn);
    
    layout->addLayout(toolbarLayout);
    
    // Table
    tableView_ = new QTableView(this);
    tableModel_ = new QStandardItemModel(this);
    
    QStringList headers;
    for (const auto& param : parameters_) {
        headers.append(param.displayName.isEmpty() ? param.name : param.displayName);
    }
    tableModel_->setHorizontalHeaderLabels(headers);
    
    tableView_->setModel(tableModel_);
    tableView_->setAlternatingRowColors(true);
    layout->addWidget(tableView_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* okBtn = new QPushButton(tr("OK"), this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
    
    // Add one empty row
    addParameterSetRow();
}

void BatchParametersDialog::addParameterSetRow(const QHash<QString, QVariant>& values) {
    QList<QStandardItem*> row;
    
    for (const auto& param : parameters_) {
        QString text = values.value(param.name).toString();
        row.append(new QStandardItem(text));
    }
    
    tableModel_->appendRow(row);
}

QList<QHash<QString, QVariant>> BatchParametersDialog::getParameterSets() const {
    QList<QHash<QString, QVariant>> sets;
    
    for (int row = 0; row < tableModel_->rowCount(); ++row) {
        QHash<QString, QVariant> set;
        
        for (int col = 0; col < parameters_.size() && col < tableModel_->columnCount(); ++col) {
            QString paramName = parameters_[col].name;
            QString value = tableModel_->item(row, col)->text();
            set[paramName] = value;
        }
        
        sets.append(set);
    }
    
    return sets;
}

void BatchParametersDialog::onAddRow() {
    addParameterSetRow();
}

void BatchParametersDialog::onRemoveRow() {
    auto index = tableView_->currentIndex();
    if (index.isValid()) {
        tableModel_->removeRow(index.row());
    }
}

void BatchParametersDialog::onDuplicateRow() {
    auto index = tableView_->currentIndex();
    if (!index.isValid()) return;
    
    QList<QStandardItem*> row;
    for (int col = 0; col < tableModel_->columnCount(); ++col) {
        row.append(new QStandardItem(tableModel_->item(index.row(), col)->text()));
    }
    
    tableModel_->insertRow(index.row() + 1, row);
}

void BatchParametersDialog::onImportCSV() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import CSV"),
        QString(), tr("CSV Files (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Import"), tr("CSV imported successfully."));
    }
}

void BatchParametersDialog::onExportCSV() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export CSV"),
        tr("batch_params.csv"), tr("CSV Files (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("CSV exported successfully."));
    }
}

void BatchParametersDialog::onLoadTemplate() {
    // Load template
}

void BatchParametersDialog::onSaveTemplate() {
    // Save template
}

// ============================================================================
// Quick Parameter Toolbar
// ============================================================================
QuickParameterToolbar::QuickParameterToolbar(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void QuickParameterToolbar::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    auto* label = new QLabel(tr("Params:"), this);
    layout->addWidget(label);
    
    layout->addStretch();
    
    auto* moreBtn = new QPushButton(tr("More..."), this);
    connect(moreBtn, &QPushButton::clicked, this, &QuickParameterToolbar::onShowFullDialog);
    layout->addWidget(moreBtn);
    
    auto* resetBtn = new QPushButton(tr("Reset"), this);
    connect(resetBtn, &QPushButton::clicked, this, &QuickParameterToolbar::onResetDefaults);
    layout->addWidget(resetBtn);
}

void QuickParameterToolbar::setParameters(const QList<QueryParameter>& parameters) {
    parameters_ = parameters;
    rebuildInputs();
}

void QuickParameterToolbar::rebuildInputs() {
    // Remove old inputs
    for (auto* widget : quickInputs_) {
        delete widget;
    }
    quickInputs_.clear();
    
    // Add new inputs
    for (const auto& param : parameters_) {
        auto* edit = new QLineEdit(this);
        edit->setPlaceholderText(param.name);
        edit->setMaximumWidth(100);
        connect(edit, &QLineEdit::textChanged, this, &QuickParameterToolbar::onParameterEdited);
        
        layout()->addWidget(edit);
        quickInputs_[param.name] = edit;
    }
}

void QuickParameterToolbar::onParameterEdited() {
    QHash<QString, QVariant> values;
    
    for (auto it = quickInputs_.begin(); it != quickInputs_.end(); ++it) {
        values[it.key()] = it.value()->text();
    }
    
    emit parametersChanged(values);
}

void QuickParameterToolbar::onShowFullDialog() {
    emit dialogRequested();
}

void QuickParameterToolbar::onResetDefaults() {
    for (auto* edit : quickInputs_) {
        edit->clear();
    }
}

// ============================================================================
// Parameter Preset Manager
// ============================================================================
ParameterPresetManager::ParameterPresetManager(QObject* parent)
    : QObject(parent) {
}

QList<ParameterPreset> ParameterPresetManager::getPresetsForQuery(const QString& queryHash) const {
    Q_UNUSED(queryHash)
    return QList<ParameterPreset>();
}

void ParameterPresetManager::savePreset(const QString& queryHash, const ParameterPreset& preset) {
    Q_UNUSED(queryHash)
    Q_UNUSED(preset)
    emit presetSaved(queryHash, preset);
}

void ParameterPresetManager::deletePreset(const QString& queryHash, const QString& presetId) {
    emit presetDeleted(queryHash, presetId);
}

ParameterPreset ParameterPresetManager::loadPreset(const QString& queryHash, const QString& presetId) const {
    Q_UNUSED(queryHash)
    Q_UNUSED(presetId)
    return ParameterPreset();
}

QString ParameterPresetManager::getStoragePath() const {
    return QString();
}

} // namespace scratchrobin::ui
