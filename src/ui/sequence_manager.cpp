#include "ui/sequence_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTableView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QToolBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QSplitter>

namespace scratchrobin::ui {

// ============================================================================
// Sequence Manager Panel
// ============================================================================
SequenceManagerPanel::SequenceManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("sequence_manager", parent)
    , client_(client)
{
    setupUi();
    setupModel();
    refresh();
}

void SequenceManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* createBtn = new QPushButton(tr("Create"), this);
    auto* editBtn = new QPushButton(tr("Edit"), this);
    auto* dropBtn = new QPushButton(tr("Drop"), this);
    auto* nextValuesBtn = new QPushButton(tr("Next Values"), this);
    auto* resetBtn = new QPushButton(tr("Reset"), this);
    auto* setValueBtn = new QPushButton(tr("Set Value"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(dropBtn);
    toolbar->addSeparator();
    toolbar->addWidget(nextValuesBtn);
    toolbar->addWidget(setValueBtn);
    toolbar->addWidget(resetBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(createBtn, &QPushButton::clicked, this, &SequenceManagerPanel::onCreateSequence);
    connect(editBtn, &QPushButton::clicked, this, &SequenceManagerPanel::onEditSequence);
    connect(dropBtn, &QPushButton::clicked, this, &SequenceManagerPanel::onDropSequence);
    connect(nextValuesBtn, &QPushButton::clicked, this, &SequenceManagerPanel::onShowNextValues);
    connect(resetBtn, &QPushButton::clicked, this, &SequenceManagerPanel::onResetSequence);
    connect(setValueBtn, &QPushButton::clicked, this, &SequenceManagerPanel::onSetValue);
    connect(refreshBtn, &QPushButton::clicked, this, &SequenceManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter sequences..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &SequenceManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Splitter for tree and details
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Sequence tree
    sequenceTree_ = new QTreeView(this);
    sequenceTree_->setAlternatingRowColors(true);
    sequenceTree_->setSortingEnabled(true);
    connect(sequenceTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SequenceManagerPanel::onSequenceSelected);
    
    splitter->addWidget(sequenceTree_);
    
    // Right: Details panel
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    auto* detailsGroup = new QGroupBox(tr("Sequence Details"), this);
    auto* detailsLayout = new QFormLayout(detailsGroup);
    
    currentValueLabel_ = new QLabel("-", this);
    minValueLabel_ = new QLabel("-", this);
    maxValueLabel_ = new QLabel("-", this);
    incrementLabel_ = new QLabel("-", this);
    cycleLabel_ = new QLabel("-", this);
    ownedByLabel_ = new QLabel("-", this);
    
    detailsLayout->addRow(tr("Current Value:"), currentValueLabel_);
    detailsLayout->addRow(tr("Min Value:"), minValueLabel_);
    detailsLayout->addRow(tr("Max Value:"), maxValueLabel_);
    detailsLayout->addRow(tr("Increment:"), incrementLabel_);
    detailsLayout->addRow(tr("Cycle:"), cycleLabel_);
    detailsLayout->addRow(tr("Owned By:"), ownedByLabel_);
    
    rightLayout->addWidget(detailsGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setPlaceholderText(tr("DDL Preview..."));
    rightLayout->addWidget(previewEdit_, 1);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({400, 400});
    
    mainLayout->addWidget(splitter);
}

void SequenceManagerPanel::setupModel()
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(5);
    model_->setHorizontalHeaderLabels({
        tr("Sequence"), tr("Schema"), tr("Current"), tr("Increment"), tr("Cycle")
    });
    sequenceTree_->setModel(model_);
    sequenceTree_->header()->setStretchLastSection(false);
    sequenceTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void SequenceManagerPanel::refresh()
{
    loadSequences();
}

void SequenceManagerPanel::panelActivated()
{
    refresh();
}

void SequenceManagerPanel::loadSequences()
{
    model_->removeRows(0, model_->rowCount());
    
    // TODO: Load from SessionClient when API is available
}

void SequenceManagerPanel::onCreateSequence()
{
    SequenceEditorDialog dialog(client_, SequenceEditorDialog::Create, QString(), this);
    dialog.exec();
}

void SequenceManagerPanel::onEditSequence()
{
    auto index = sequenceTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    
    SequenceEditorDialog dialog(client_, SequenceEditorDialog::Edit, name, this);
    dialog.exec();
}

void SequenceManagerPanel::onDropSequence()
{
    auto index = sequenceTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString schema = model_->item(index.row(), 1)->text();
    
    auto reply = QMessageBox::question(this, tr("Drop Sequence"),
        tr("Drop sequence '%1.%2'?").arg(schema).arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void SequenceManagerPanel::onShowNextValues()
{
    auto index = sequenceTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    
    ShowNextValuesDialog dialog(client_, name, this);
    dialog.exec();
}

void SequenceManagerPanel::onResetSequence()
{
    auto index = sequenceTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString schema = model_->item(index.row(), 1)->text();
    
    auto reply = QMessageBox::question(this, tr("Reset Sequence"),
        tr("Reset sequence '%1.%2' to its start value?").arg(schema).arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void SequenceManagerPanel::onSetValue()
{
    auto index = sequenceTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString currentStr = model_->item(index.row(), 2)->text();
    qint64 currentValue = currentStr.toLongLong();
    
    SetSequenceValueDialog dialog(client_, name, currentValue, this);
    dialog.exec();
}

void SequenceManagerPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < model_->rowCount(); ++i) {
        QString name = model_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        sequenceTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

void SequenceManagerPanel::onSequenceSelected(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString schema = model_->item(index.row(), 1)->text();
    QString current = model_->item(index.row(), 2)->text();
    QString increment = model_->item(index.row(), 3)->text();
    QString cycle = model_->item(index.row(), 4)->text();
    
    currentValueLabel_->setText(current);
    incrementLabel_->setText(increment);
    cycleLabel_->setText(cycle);
    
    // TODO: Load full details from SessionClient when API is available
    
    // Generate DDL preview
    QString ddl = QString("CREATE SEQUENCE %1.%2;").arg(schema).arg(name);
    previewEdit_->setPlainText(ddl);
    
    emit sequenceSelected(name);
}

// ============================================================================
// Sequence Editor Dialog
// ============================================================================
SequenceEditorDialog::SequenceEditorDialog(backend::SessionClient* client,
                                           Mode mode,
                                           const QString& sequenceName,
                                           QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , mode_(mode)
    , originalName_(sequenceName)
{
    setMinimumSize(500, 500);
    
    if (mode_ == Create) {
        setWindowTitle(tr("Create Sequence"));
    } else {
        setWindowTitle(tr("Edit Sequence"));
    }
    
    setupUi();
}

void SequenceEditorDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Identity
    auto* identityGroup = new QGroupBox(tr("Identity"), this);
    auto* identityLayout = new QFormLayout(identityGroup);
    
    nameEdit_ = new QLineEdit(this);
    identityLayout->addRow(tr("Name:"), nameEdit_);
    
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    identityLayout->addRow(tr("Schema:"), schemaCombo_);
    
    mainLayout->addWidget(identityGroup);
    
    // Parameters
    auto* paramGroup = new QGroupBox(tr("Parameters"), this);
    auto* paramLayout = new QFormLayout(paramGroup);
    
    startSpin_ = new QSpinBox(this);
    startSpin_->setRange(-2147483647, 2147483647);
    startSpin_->setValue(1);
    paramLayout->addRow(tr("Start Value:"), startSpin_);
    
    incrementSpin_ = new QSpinBox(this);
    incrementSpin_->setRange(-2147483647, 2147483647);
    incrementSpin_->setValue(1);
    paramLayout->addRow(tr("Increment By:"), incrementSpin_);
    
    minValueSpin_ = new QSpinBox(this);
    minValueSpin_->setRange(-2147483647, 2147483647);
    minValueSpin_->setValue(1);
    paramLayout->addRow(tr("Min Value:"), minValueSpin_);
    
    maxValueSpin_ = new QSpinBox(this);
    maxValueSpin_->setRange(-2147483647, 2147483647);
    maxValueSpin_->setValue(2147483647);
    paramLayout->addRow(tr("Max Value:"), maxValueSpin_);
    
    cacheSpin_ = new QSpinBox(this);
    cacheSpin_->setRange(1, 1000000);
    cacheSpin_->setValue(1);
    paramLayout->addRow(tr("Cache:"), cacheSpin_);
    
    cycleCheck_ = new QCheckBox(tr("Cycle when limit reached"), this);
    paramLayout->addRow(QString(), cycleCheck_);
    
    orderedCheck_ = new QCheckBox(tr("Ordered (guarantee no gaps)"), this);
    paramLayout->addRow(QString(), orderedCheck_);
    
    mainLayout->addWidget(paramGroup);
    
    // Ownership
    auto* ownerGroup = new QGroupBox(tr("Ownership"), this);
    auto* ownerLayout = new QFormLayout(ownerGroup);
    
    ownedByTableCombo_ = new QComboBox(this);
    ownedByTableCombo_->setEditable(true);
    ownedByTableCombo_->addItem(tr("(none)"));
    ownerLayout->addRow(tr("Owned By Table:"), ownedByTableCombo_);
    
    ownedByColumnCombo_ = new QComboBox(this);
    ownedByColumnCombo_->setEditable(true);
    ownedByColumnCombo_->setEnabled(false);
    ownerLayout->addRow(tr("Column:"), ownedByColumnCombo_);
    
    connect(ownedByTableCombo_, &QComboBox::currentTextChanged, [this](const QString& text) {
        ownedByColumnCombo_->setEnabled(text != tr("(none)") && !text.isEmpty());
    });
    
    mainLayout->addWidget(ownerGroup);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("DDL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Save);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(previewBtn, &QPushButton::clicked, this, &SequenceEditorDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SequenceEditorDialog::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    onPreview();
}

void SequenceEditorDialog::onSave()
{
    if (nameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Sequence name is required."));
        return;
    }
    
    // TODO: Save via SessionClient when API is available
    accept();
}

void SequenceEditorDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

QString SequenceEditorDialog::generateDdl()
{
    QString sql;
    
    if (mode_ == Edit) {
        sql = QString("ALTER SEQUENCE %1.%2\n")
            .arg(schemaCombo_->currentText())
            .arg(nameEdit_->text());
        sql += QString("    RESTART WITH %1\n").arg(startSpin_->value());
        sql += QString("    INCREMENT BY %1\n").arg(incrementSpin_->value());
        sql += QString("    MINVALUE %1\n").arg(minValueSpin_->value());
        sql += QString("    MAXVALUE %1\n").arg(maxValueSpin_->value());
        sql += QString("    %1\n").arg(cycleCheck_->isChecked() ? "CYCLE" : "NO CYCLE");
        sql += QString("    CACHE %1").arg(cacheSpin_->value());
        sql += ";";
    } else {
        sql = QString("CREATE SEQUENCE %1.%2\n")
            .arg(schemaCombo_->currentText())
            .arg(nameEdit_->text());
        sql += QString("    START WITH %1\n").arg(startSpin_->value());
        sql += QString("    INCREMENT BY %1\n").arg(incrementSpin_->value());
        sql += QString("    MINVALUE %1\n").arg(minValueSpin_->value());
        sql += QString("    MAXVALUE %1\n").arg(maxValueSpin_->value());
        sql += QString("    %1\n").arg(cycleCheck_->isChecked() ? "CYCLE" : "NO CYCLE");
        sql += QString("    CACHE %1").arg(cacheSpin_->value());
        if (orderedCheck_->isChecked()) {
            sql += "\n    ORDERED";
        }
        sql += ";";
    }
    
    return sql;
}

// ============================================================================
// Set Sequence Value Dialog
// ============================================================================
SetSequenceValueDialog::SetSequenceValueDialog(backend::SessionClient* client,
                                               const QString& sequenceName,
                                               qint64 currentValue,
                                               QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , sequenceName_(sequenceName)
    , currentValue_(currentValue)
{
    setWindowTitle(tr("Set Sequence Value - %1").arg(sequenceName));
    setupUi();
}

void SetSequenceValueDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    currentLabel_ = new QLabel(QString::number(currentValue_), this);
    formLayout->addRow(tr("Current Value:"), currentLabel_);
    
    newValueSpin_ = new QSpinBox(this);
    newValueSpin_->setRange(-2147483647, 2147483647);
    newValueSpin_->setValue(currentValue_);
    formLayout->addRow(tr("New Value:"), newValueSpin_);
    
    auto* advanceSpin = new QSpinBox(this);
    advanceSpin->setRange(-1000000, 1000000);
    advanceSpin->setValue(0);
    formLayout->addRow(tr("Advance By:"), advanceSpin);
    
    connect(advanceSpin, &QSpinBox::valueChanged, [this](int value) {
        newValueSpin_->setValue(currentValue_ + value);
    });
    
    isCalledCheck_ = new QCheckBox(tr("Mark as called (next call will increment)"), this);
    isCalledCheck_->setChecked(true);
    formLayout->addRow(QString(), isCalledCheck_);
    
    mainLayout->addLayout(formLayout);
    
    auto* buttonBox = new QDialogButtonBox(this);
    auto* setBtn = buttonBox->addButton(tr("Set Value"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(setBtn, &QPushButton::clicked, [this]() {
        // TODO: Execute via SessionClient when API is available
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// ============================================================================
// Show Next Values Dialog
// ============================================================================
ShowNextValuesDialog::ShowNextValuesDialog(backend::SessionClient* client,
                                           const QString& sequenceName,
                                           QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , sequenceName_(sequenceName)
{
    setWindowTitle(tr("Next Values - %1").arg(sequenceName));
    setupUi();
}

void ShowNextValuesDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* countLayout = new QHBoxLayout();
    countLayout->addWidget(new QLabel(tr("Show count:")));
    auto* countSpin = new QSpinBox(this);
    countSpin->setRange(1, 100);
    countSpin->setValue(10);
    countLayout->addWidget(countSpin);
    countLayout->addStretch();
    mainLayout->addLayout(countLayout);
    
    auto* valuesTable = new QTableView(this);
    auto* model = new QStandardItemModel(this);
    model->setColumnCount(2);
    model->setHorizontalHeaderLabels({tr("Call #"), tr("Value")});
    valuesTable->setModel(model);
    valuesTable->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(valuesTable);
    
    auto* buttonBox = new QDialogButtonBox(this);
    auto* refreshBtn = buttonBox->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Close);
    
    mainLayout->addWidget(buttonBox);
    
    connect(refreshBtn, &QPushButton::clicked, [this, model, countSpin]() {
        model->removeRows(0, model->rowCount());
        int count = countSpin->value();
        for (int i = 1; i <= count; ++i) {
            auto* numItem = new QStandardItem(QString::number(i));
            auto* valItem = new QStandardItem(tr("(preview)"));
            QList<QStandardItem*> row;
            row << numItem << valItem;
            model->appendRow(row);
        }
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
}

} // namespace scratchrobin::ui
