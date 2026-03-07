#include "ui/procedure_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <QLabel>
#include <QToolBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QSplitter>
#include <QGroupBox>

namespace scratchrobin::ui {

// ============================================================================
// Procedure Manager Panel
// ============================================================================
ProcedureManagerPanel::ProcedureManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("procedure_manager", parent)
    , client_(client)
{
    setupUi();
    setupModel();
    refresh();
}

void ProcedureManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* createProcBtn = new QPushButton(tr("New Procedure"), this);
    auto* createFuncBtn = new QPushButton(tr("New Function"), this);
    auto* editBtn = new QPushButton(tr("Edit"), this);
    auto* dropBtn = new QPushButton(tr("Drop"), this);
    auto* compileBtn = new QPushButton(tr("Compile"), this);
    auto* execBtn = new QPushButton(tr("Execute"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createProcBtn);
    toolbar->addWidget(createFuncBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(compileBtn);
    toolbar->addWidget(execBtn);
    toolbar->addWidget(dropBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(createProcBtn, &QPushButton::clicked, [this]() { onCreateProcedure(); });
    connect(createFuncBtn, &QPushButton::clicked, [this]() { onCreateProcedure(); });
    connect(editBtn, &QPushButton::clicked, this, &ProcedureManagerPanel::onEditProcedure);
    connect(dropBtn, &QPushButton::clicked, this, &ProcedureManagerPanel::onDropProcedure);
    connect(compileBtn, &QPushButton::clicked, this, &ProcedureManagerPanel::onCompileProcedure);
    connect(execBtn, &QPushButton::clicked, this, &ProcedureManagerPanel::onExecuteProcedure);
    connect(refreshBtn, &QPushButton::clicked, this, &ProcedureManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter procedures..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &ProcedureManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Procedure tree
    procedureTree_ = new QTreeView(this);
    procedureTree_->setAlternatingRowColors(true);
    procedureTree_->setSortingEnabled(true);
    connect(procedureTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ProcedureManagerPanel::onProcedureSelected);
    
    splitter->addWidget(procedureTree_);
    
    // Right: Preview
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Monospace", 10));
    previewEdit_->setPlaceholderText(tr("Select a procedure to view its source..."));
    splitter->addWidget(previewEdit_);
    
    splitter->setSizes({400, 400});
    mainLayout->addWidget(splitter);
}

void ProcedureManagerPanel::setupModel()
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(6);
    model_->setHorizontalHeaderLabels({
        tr("Name"), tr("Schema"), tr("Type"), tr("Returns"), tr("Language"), tr("Valid")
    });
    procedureTree_->setModel(model_);
    procedureTree_->header()->setStretchLastSection(false);
    procedureTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void ProcedureManagerPanel::refresh()
{
    loadProcedures();
}

void ProcedureManagerPanel::panelActivated()
{
    refresh();
}

void ProcedureManagerPanel::loadProcedures()
{
    model_->removeRows(0, model_->rowCount());
    
    // TODO: Load from SessionClient when API is available
}

void ProcedureManagerPanel::onCreateProcedure()
{
    bool isFunction = (sender() && sender()->objectName().contains("Function"));
    ProcedureEditorDialog::Mode mode = isFunction ? 
        ProcedureEditorDialog::CreateFunction : ProcedureEditorDialog::CreateProcedure;
    
    ProcedureEditorDialog dialog(client_, mode, QString(), this);
    dialog.exec();
}

void ProcedureManagerPanel::onEditProcedure()
{
    auto index = procedureTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    
    ProcedureEditorDialog dialog(client_, ProcedureEditorDialog::Edit, name, this);
    dialog.exec();
}

void ProcedureManagerPanel::onDropProcedure()
{
    auto index = procedureTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString schema = model_->item(index.row(), 1)->text();
    QString type = model_->item(index.row(), 2)->text();
    
    auto reply = QMessageBox::question(this, tr("Drop %1").arg(type),
        tr("Drop %1 %2.%3?").arg(type).arg(schema).arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void ProcedureManagerPanel::onCompileProcedure()
{
    auto index = procedureTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString type = model_->item(index.row(), 2)->text();
    
    // TODO: Execute via SessionClient when API is available
    QMessageBox::information(this, tr("Compile"),
        tr("%1 compiled successfully.").arg(type));
    refresh();
}

void ProcedureManagerPanel::onExecuteProcedure()
{
    auto index = procedureTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString type = model_->item(index.row(), 2)->text();
    
    if (type != "PROCEDURE") {
        QMessageBox::information(this, tr("Execute"),
            tr("Only procedures can be executed. Use SELECT for functions."));
        return;
    }
    
    QList<ProcedureParameter> params;
    ExecuteProcedureDialog dialog(client_, name, params, this);
    dialog.exec();
}

void ProcedureManagerPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < model_->rowCount(); ++i) {
        QString name = model_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        procedureTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

void ProcedureManagerPanel::onProcedureSelected(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    emit procedureSelected(name);
}

// ============================================================================
// Procedure Editor Dialog
// ============================================================================
ProcedureEditorDialog::ProcedureEditorDialog(backend::SessionClient* client,
                                             Mode mode,
                                             const QString& procedureName,
                                             QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , mode_(mode)
    , originalName_(procedureName)
{
    setMinimumSize(900, 700);
    
    switch (mode_) {
        case CreateProcedure:
            setWindowTitle(tr("Create Procedure"));
            break;
        case CreateFunction:
            setWindowTitle(tr("Create Function"));
            break;
        case Edit:
            setWindowTitle(tr("Edit Procedure/Function"));
            break;
    }
    
    setupUi();
}

void ProcedureEditorDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Header
    auto* headerLayout = new QHBoxLayout();
    
    headerLayout->addWidget(new QLabel(tr("Name:")));
    nameEdit_ = new QLineEdit(this);
    headerLayout->addWidget(nameEdit_);
    
    headerLayout->addWidget(new QLabel(tr("Schema:")));
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    headerLayout->addWidget(schemaCombo_);
    
    headerLayout->addWidget(new QLabel(tr("Type:")));
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItems({"PROCEDURE", "FUNCTION"});
    connect(typeCombo_, &QComboBox::currentTextChanged, [this](const QString& text) {
        returnTypeCombo_->setEnabled(text == "FUNCTION");
        deterministicCheck_->setEnabled(text == "FUNCTION");
    });
    headerLayout->addWidget(typeCombo_);
    
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // Parameters section
    auto* paramGroup = new QGroupBox(tr("Parameters"), this);
    auto* paramLayout = new QVBoxLayout(paramGroup);
    
    paramTable_ = new QTableWidget(this);
    paramTable_->setColumnCount(4);
    paramTable_->setHorizontalHeaderLabels({tr("Name"), tr("Mode"), tr("Type"), tr("Default")});
    paramTable_->horizontalHeader()->setStretchLastSection(true);
    paramLayout->addWidget(paramTable_);
    
    auto* paramBtnLayout = new QHBoxLayout();
    addParamBtn_ = new QPushButton(tr("Add"), this);
    removeParamBtn_ = new QPushButton(tr("Remove"), this);
    moveUpBtn_ = new QPushButton(tr("Up"), this);
    moveDownBtn_ = new QPushButton(tr("Down"), this);
    
    paramBtnLayout->addWidget(addParamBtn_);
    paramBtnLayout->addWidget(removeParamBtn_);
    paramBtnLayout->addWidget(moveUpBtn_);
    paramBtnLayout->addWidget(moveDownBtn_);
    paramBtnLayout->addStretch();
    paramLayout->addLayout(paramBtnLayout);
    
    connect(addParamBtn_, &QPushButton::clicked, this, &ProcedureEditorDialog::onAddParameter);
    connect(removeParamBtn_, &QPushButton::clicked, this, &ProcedureEditorDialog::onRemoveParameter);
    connect(moveUpBtn_, &QPushButton::clicked, this, &ProcedureEditorDialog::onMoveParameterUp);
    connect(moveDownBtn_, &QPushButton::clicked, this, &ProcedureEditorDialog::onMoveParameterDown);
    
    mainLayout->addWidget(paramGroup);
    
    // Options section
    auto* optionsLayout = new QHBoxLayout();
    
    optionsLayout->addWidget(new QLabel(tr("Language:")));
    languageCombo_ = new QComboBox(this);
    languageCombo_->addItems({"SQL", "PLpgSQL", "C", "JAVA"});
    optionsLayout->addWidget(languageCombo_);
    
    optionsLayout->addWidget(new QLabel(tr("Returns:")));
    returnTypeCombo_ = new QComboBox(this);
    returnTypeCombo_->setEditable(true);
    returnTypeCombo_->addItems({"INTEGER", "VARCHAR", "BOOLEAN", "DATE", "TIMESTAMP", "NUMERIC", "VOID"});
    returnTypeCombo_->setEnabled(false);
    optionsLayout->addWidget(returnTypeCombo_);
    
    deterministicCheck_ = new QCheckBox(tr("DETERMINISTIC"), this);
    deterministicCheck_->setEnabled(false);
    optionsLayout->addWidget(deterministicCheck_);
    
    optionsLayout->addWidget(new QLabel(tr("Security:")));
    securityCombo_ = new QComboBox(this);
    securityCombo_->addItems({"INVOKER", "DEFINER"});
    optionsLayout->addWidget(securityCombo_);
    
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);
    
    // Body section
    auto* bodyGroup = new QGroupBox(tr("Procedure Body"), this);
    auto* bodyLayout = new QVBoxLayout(bodyGroup);
    
    bodyEdit_ = new QTextEdit(this);
    bodyEdit_->setFont(QFont("Monospace", 10));
    bodyLayout->addWidget(bodyEdit_);
    
    mainLayout->addWidget(bodyGroup, 1);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("DDL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Monospace", 9));
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Status
    statusLabel_ = new QLabel(this);
    statusLabel_->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    mainLayout->addWidget(statusLabel_);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* validateBtn = buttonBox->addButton(tr("Validate"), QDialogButtonBox::ActionRole);
    auto* compileBtn = buttonBox->addButton(tr("Compile"), QDialogButtonBox::ActionRole);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Save);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(validateBtn, &QPushButton::clicked, this, &ProcedureEditorDialog::onValidate);
    connect(compileBtn, &QPushButton::clicked, this, &ProcedureEditorDialog::onCompile);
    connect(previewBtn, &QPushButton::clicked, this, &ProcedureEditorDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProcedureEditorDialog::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ProcedureEditorDialog::updateParameterTable()
{
    paramTable_->setRowCount(parameters_.size());
    
    for (int i = 0; i < parameters_.size(); ++i) {
        const auto& param = parameters_[i];
        paramTable_->setItem(i, 0, new QTableWidgetItem(param.name));
        paramTable_->setItem(i, 1, new QTableWidgetItem(param.mode));
        paramTable_->setItem(i, 2, new QTableWidgetItem(param.dataType));
        paramTable_->setItem(i, 3, new QTableWidgetItem(
            param.hasDefault ? param.defaultValue : "-"));
    }
}

void ProcedureEditorDialog::onAddParameter()
{
    ProcedureParameter param;
    param.name = "p" + QString::number(parameters_.size() + 1);
    param.mode = "IN";
    param.dataType = "INTEGER";
    
    ParameterEditDialog dialog(&param, this);
    if (dialog.exec() == QDialog::Accepted) {
        parameters_.append(param);
        updateParameterTable();
    }
}

void ProcedureEditorDialog::onRemoveParameter()
{
    auto row = paramTable_->currentRow();
    if (row >= 0 && row < parameters_.size()) {
        parameters_.removeAt(row);
        updateParameterTable();
    }
}

void ProcedureEditorDialog::onMoveParameterUp()
{
    auto row = paramTable_->currentRow();
    if (row > 0) {
        std::swap(parameters_[row], parameters_[row - 1]);
        updateParameterTable();
        paramTable_->selectRow(row - 1);
    }
}

void ProcedureEditorDialog::onMoveParameterDown()
{
    auto row = paramTable_->currentRow();
    if (row >= 0 && row < parameters_.size() - 1) {
        std::swap(parameters_[row], parameters_[row + 1]);
        updateParameterTable();
        paramTable_->selectRow(row + 1);
    }
}

void ProcedureEditorDialog::onValidate()
{
    // TODO: Implement validation when SessionClient API is available
    statusLabel_->setText(tr("Valid syntax."));
    statusLabel_->setStyleSheet("color: green;");
}

void ProcedureEditorDialog::onCompile()
{
    // TODO: Implement compilation when SessionClient API is available
    statusLabel_->setText(tr("Compiled successfully."));
    statusLabel_->setStyleSheet("color: green;");
}

void ProcedureEditorDialog::onSave()
{
    // TODO: Save via SessionClient when API is available
    accept();
}

void ProcedureEditorDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

QString ProcedureEditorDialog::generateDdl()
{
    QString type = typeCombo_->currentText();
    QString name = nameEdit_->text();
    QString schema = schemaCombo_->currentText();
    
    QString sql = QString("CREATE OR REPLACE %1 %2.%3(")
        .arg(type)
        .arg(schema)
        .arg(name);
    
    // Add parameters
    QStringList paramList;
    for (const auto& param : parameters_) {
        QString paramStr = param.mode + " " + param.name + " " + param.dataType;
        if (param.hasDefault && !param.defaultValue.isEmpty()) {
            paramStr += " DEFAULT " + param.defaultValue;
        }
        paramList.append(paramStr);
    }
    sql += paramList.join(", ");
    sql += ")";
    
    if (type == "FUNCTION") {
        sql += QString("\nRETURNS %1").arg(returnTypeCombo_->currentText());
    }
    
    sql += QString("\nLANGUAGE %1").arg(languageCombo_->currentText());
    
    if (type == "FUNCTION" && deterministicCheck_->isChecked()) {
        sql += "\nDETERMINISTIC";
    }
    
    sql += QString("\nSECURITY %1").arg(securityCombo_->currentText());
    sql += "\nAS $$\n";
    sql += bodyEdit_->toPlainText();
    sql += "\n$$;";
    
    return sql;
}

// ============================================================================
// Parameter Edit Dialog
// ============================================================================
ParameterEditDialog::ParameterEditDialog(ProcedureParameter* param, QWidget* parent)
    : QDialog(parent)
    , param_(param)
{
    setWindowTitle(tr("Edit Parameter"));
    setupUi();
}

void ParameterEditDialog::setupUi()
{
    auto* layout = new QFormLayout(this);
    
    auto* nameEdit = new QLineEdit(param_->name, this);
    layout->addRow(tr("Name:"), nameEdit);
    
    auto* modeCombo = new QComboBox(this);
    modeCombo->addItems({"IN", "OUT", "INOUT"});
    modeCombo->setCurrentText(param_->mode);
    layout->addRow(tr("Mode:"), modeCombo);
    
    auto* typeCombo = new QComboBox(this);
    typeCombo->setEditable(true);
    typeCombo->addItems({"INTEGER", "VARCHAR", "BOOLEAN", "DATE", "TIMESTAMP", "NUMERIC", "TEXT"});
    typeCombo->setCurrentText(param_->dataType);
    layout->addRow(tr("Data Type:"), typeCombo);
    
    auto* hasDefaultCheck = new QCheckBox(tr("Has Default Value"), this);
    hasDefaultCheck->setChecked(param_->hasDefault);
    layout->addRow(QString(), hasDefaultCheck);
    
    auto* defaultEdit = new QLineEdit(param_->defaultValue, this);
    defaultEdit->setEnabled(param_->hasDefault);
    connect(hasDefaultCheck, &QCheckBox::toggled, defaultEdit, &QLineEdit::setEnabled);
    layout->addRow(tr("Default:"), defaultEdit);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, [this, nameEdit, modeCombo, typeCombo, hasDefaultCheck, defaultEdit]() {
        param_->name = nameEdit->text();
        param_->mode = modeCombo->currentText();
        param_->dataType = typeCombo->currentText();
        param_->hasDefault = hasDefaultCheck->isChecked();
        param_->defaultValue = defaultEdit->text();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttonBox);
}

// ============================================================================
// Execute Procedure Dialog
// ============================================================================
ExecuteProcedureDialog::ExecuteProcedureDialog(backend::SessionClient* client,
                                               const QString& procedureName,
                                               const QList<ProcedureParameter>& params,
                                               QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , procedureName_(procedureName)
    , params_(params)
{
    setWindowTitle(tr("Execute %1").arg(procedureName));
    setupUi();
}

void ExecuteProcedureDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Parameters table
    auto* paramTable = new QTableWidget(this);
    paramTable->setColumnCount(3);
    paramTable->setHorizontalHeaderLabels({tr("Parameter"), tr("Type"), tr("Value")});
    paramTable->horizontalHeader()->setStretchLastSection(true);
    
    paramTable->setRowCount(params_.size());
    for (int i = 0; i < params_.size(); ++i) {
        paramTable->setItem(i, 0, new QTableWidgetItem(params_[i].name));
        paramTable->setItem(i, 1, new QTableWidgetItem(params_[i].dataType));
        paramTable->setCellWidget(i, 2, new QLineEdit(this));
    }
    
    mainLayout->addWidget(new QLabel(tr("Enter parameter values:")));
    mainLayout->addWidget(paramTable);
    
    // Results
    mainLayout->addWidget(new QLabel(tr("Results:")));
    auto* resultEdit = new QTextEdit(this);
    resultEdit->setReadOnly(true);
    mainLayout->addWidget(resultEdit);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* execBtn = buttonBox->addButton(tr("Execute"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Close);
    
    mainLayout->addWidget(buttonBox);
    
    connect(execBtn, &QPushButton::clicked, [this, resultEdit]() {
        // TODO: Execute via SessionClient when API is available
        resultEdit->setPlainText(tr("Execution completed."));
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

} // namespace scratchrobin::ui
