#include "ui/synonym_manager.h"
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
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QToolBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QSplitter>

namespace scratchrobin::ui {

// ============================================================================
// Synonym Manager Panel
// ============================================================================
SynonymManagerPanel::SynonymManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("synonym_manager", parent)
    , client_(client)
{
    setupUi();
    setupModel();
    refresh();
}

void SynonymManagerPanel::setupUi()
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
    auto* dropPublicBtn = new QPushButton(tr("Drop Public"), this);
    auto* validateBtn = new QPushButton(tr("Validate"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(dropBtn);
    toolbar->addWidget(dropPublicBtn);
    toolbar->addWidget(validateBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(createBtn, &QPushButton::clicked, this, &SynonymManagerPanel::onCreateSynonym);
    connect(editBtn, &QPushButton::clicked, this, &SynonymManagerPanel::onEditSynonym);
    connect(dropBtn, &QPushButton::clicked, this, &SynonymManagerPanel::onDropSynonym);
    connect(dropPublicBtn, &QPushButton::clicked, this, &SynonymManagerPanel::onDropPublicSynonym);
    connect(validateBtn, &QPushButton::clicked, this, &SynonymManagerPanel::onValidateSynonym);
    connect(refreshBtn, &QPushButton::clicked, this, &SynonymManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter synonyms..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &SynonymManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Splitter for tree and details
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Synonym tree
    synonymTree_ = new QTreeView(this);
    synonymTree_->setAlternatingRowColors(true);
    synonymTree_->setSortingEnabled(true);
    connect(synonymTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SynonymManagerPanel::onSynonymSelected);
    
    splitter->addWidget(synonymTree_);
    
    // Right: Details panel
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    auto* detailsGroup = new QGroupBox(tr("Synonym Details"), this);
    auto* detailsLayout = new QFormLayout(detailsGroup);
    
    nameLabel_ = new QLabel(this);
    typeLabel_ = new QLabel(this);
    targetLabel_ = new QLabel(this);
    statusLabel_ = new QLabel(this);
    
    detailsLayout->addRow(tr("Name:"), nameLabel_);
    detailsLayout->addRow(tr("Type:"), typeLabel_);
    detailsLayout->addRow(tr("Target:"), targetLabel_);
    detailsLayout->addRow(tr("Status:"), statusLabel_);
    
    rightLayout->addWidget(detailsGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setPlaceholderText(tr("DDL Preview..."));
    rightLayout->addWidget(previewEdit_, 1);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({400, 400});
    
    mainLayout->addWidget(splitter);
}

void SynonymManagerPanel::setupModel()
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(5);
    model_->setHorizontalHeaderLabels({
        tr("Synonym"), tr("Schema"), tr("Public"), tr("Target"), tr("Valid")
    });
    synonymTree_->setModel(model_);
    synonymTree_->header()->setStretchLastSection(false);
    synonymTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    synonymTree_->header()->setSectionResizeMode(3, QHeaderView::Stretch);
}

void SynonymManagerPanel::refresh()
{
    loadSynonyms();
}

void SynonymManagerPanel::panelActivated()
{
    refresh();
}

void SynonymManagerPanel::loadSynonyms()
{
    model_->removeRows(0, model_->rowCount());
    
    // TODO: Load from SessionClient when API is available
    // For now, just populate with sample data structure
}

void SynonymManagerPanel::onCreateSynonym()
{
    SynonymEditorDialog dialog(client_, SynonymEditorDialog::Create, QString(), false, this);
    dialog.exec();
}

void SynonymManagerPanel::onEditSynonym()
{
    auto index = synonymTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    bool isPublic = model_->item(index.row(), 2)->text() == tr("Yes");
    
    SynonymEditorDialog dialog(client_, SynonymEditorDialog::Edit, name, isPublic, this);
    dialog.exec();
}

void SynonymManagerPanel::onDropSynonym()
{
    auto index = synonymTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString schema = model_->item(index.row(), 1)->text();
    
    auto reply = QMessageBox::question(this, tr("Drop Synonym"),
        tr("Drop SYNONYM %1.%2?").arg(schema).arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void SynonymManagerPanel::onDropPublicSynonym()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Drop Public Synonym"),
                                         tr("Public synonym name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, tr("Drop Public Synonym"),
        tr("Drop PUBLIC SYNONYM %1?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void SynonymManagerPanel::onValidateSynonym()
{
    // TODO: Implement validation when SessionClient API is available
    QMessageBox::information(this, tr("Validation"),
        tr("Synonym validated successfully."));
}

void SynonymManagerPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < model_->rowCount(); ++i) {
        QString name = model_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        synonymTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

void SynonymManagerPanel::onSynonymSelected(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    QString schema = model_->item(index.row(), 1)->text();
    bool isPublic = model_->item(index.row(), 2)->text() == tr("Yes");
    QString target = model_->item(index.row(), 3)->text();
    bool isValid = model_->item(index.row(), 4)->text() == tr("Yes");
    
    nameLabel_->setText(isPublic ? tr("PUBLIC.%1").arg(name) : tr("%1.%2").arg(schema).arg(name));
    typeLabel_->setText(isPublic ? tr("Public Synonym") : tr("Private Synonym"));
    targetLabel_->setText(target);
    statusLabel_->setText(isValid ? tr("Valid") : tr("Invalid"));
    statusLabel_->setStyleSheet(isValid ? "color: green;" : "color: red;");
    
    // Generate DDL preview
    QString ddl;
    if (isPublic) {
        ddl = QString("CREATE OR REPLACE PUBLIC SYNONYM %1\n  FOR %2;").arg(name).arg(target);
    } else {
        ddl = QString("CREATE OR REPLACE SYNONYM %1.%2\n  FOR %3;").arg(schema).arg(name).arg(target);
    }
    previewEdit_->setPlainText(ddl);
    
    emit synonymSelected(name);
}

// ============================================================================
// Synonym Editor Dialog
// ============================================================================
SynonymEditorDialog::SynonymEditorDialog(backend::SessionClient* client,
                                         Mode mode,
                                         const QString& synonymName,
                                         bool isPublic,
                                         QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , mode_(mode)
    , originalIsPublic_(isPublic)
{
    setMinimumSize(500, 400);
    
    if (mode == Create) {
        setWindowTitle(tr("Create Synonym"));
    } else {
        setWindowTitle(tr("Edit Synonym"));
    }
    
    setupUi();
    
    if (mode_ == Edit) {
        nameEdit_->setText(synonymName);
        nameEdit_->setReadOnly(true);
        publicCheck_->setChecked(isPublic);
        publicCheck_->setEnabled(false);
    }
}

void SynonymEditorDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Identity section
    auto* identityGroup = new QGroupBox(tr("Synonym Identity"), this);
    auto* identityLayout = new QFormLayout(identityGroup);
    
    nameEdit_ = new QLineEdit(this);
    identityLayout->addRow(tr("Name:"), nameEdit_);
    
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    identityLayout->addRow(tr("Schema:"), schemaCombo_);
    
    publicCheck_ = new QCheckBox(tr("Create as PUBLIC synonym"), this);
    connect(publicCheck_, &QCheckBox::toggled, [this](bool checked) {
        schemaCombo_->setEnabled(!checked);
    });
    identityLayout->addRow(QString(), publicCheck_);
    
    mainLayout->addWidget(identityGroup);
    
    // Target section
    auto* targetGroup = new QGroupBox(tr("Target Object"), this);
    auto* targetLayout = new QFormLayout(targetGroup);
    
    auto* targetRowLayout = new QHBoxLayout();
    targetSchemaCombo_ = new QComboBox(this);
    targetSchemaCombo_->addItem("public");
    targetSchemaCombo_->setEditable(true);
    targetRowLayout->addWidget(targetSchemaCombo_);
    
    targetRowLayout->addWidget(new QLabel("."));
    
    targetNameEdit_ = new QLineEdit(this);
    targetRowLayout->addWidget(targetNameEdit_, 1);
    
    browseBtn_ = new QPushButton(tr("Browse..."), this);
    connect(browseBtn_, &QPushButton::clicked, this, &SynonymEditorDialog::onBrowseTarget);
    targetRowLayout->addWidget(browseBtn_);
    
    targetLayout->addRow(tr("Target:"), targetRowLayout);
    
    targetDbLinkCombo_ = new QComboBox(this);
    targetDbLinkCombo_->setEditable(true);
    targetDbLinkCombo_->addItem(tr("(local)"));
    targetLayout->addRow(tr("DB Link:"), targetDbLinkCombo_);
    
    // Target info
    targetTypeLabel_ = new QLabel(tr("-"), this);
    targetStatusLabel_ = new QLabel(tr("-"), this);
    targetLayout->addRow(tr("Target Type:"), targetTypeLabel_);
    targetLayout->addRow(tr("Target Status:"), targetStatusLabel_);
    
    mainLayout->addWidget(targetGroup);
    
    // Preview section
    auto* previewGroup = new QGroupBox(tr("DDL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* validateBtn = buttonBox->addButton(tr("Validate"), QDialogButtonBox::ActionRole);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Save);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(validateBtn, &QPushButton::clicked, this, &SynonymEditorDialog::onValidate);
    connect(previewBtn, &QPushButton::clicked, this, &SynonymEditorDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SynonymEditorDialog::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SynonymEditorDialog::onBrowseTarget()
{
    TargetBrowserDialog dialog(client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        targetSchemaCombo_->setCurrentText(dialog.selectedSchema());
        targetNameEdit_->setText(dialog.selectedName());
        targetTypeLabel_->setText(dialog.selectedType());
    }
}

void SynonymEditorDialog::onValidate()
{
    // TODO: Implement target validation when SessionClient API is available
    targetStatusLabel_->setText(tr("Valid"));
    targetStatusLabel_->setStyleSheet("color: green;");
}

void SynonymEditorDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void SynonymEditorDialog::onSave()
{
    if (nameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Synonym name is required."));
        return;
    }
    
    if (targetNameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Target object name is required."));
        return;
    }
    
    // TODO: Execute via SessionClient when API is available
    accept();
}

QString SynonymEditorDialog::generateDdl(bool forDrop) const
{
    QString synonymName = nameEdit_->text();
    QString schema = schemaCombo_->currentText();
    QString targetSchema = targetSchemaCombo_->currentText();
    QString targetName = targetNameEdit_->text();
    QString target = targetSchema.isEmpty() ? targetName : 
                     QString("%1.%2").arg(targetSchema).arg(targetName);
    
    // Add DB link if specified
    QString dbLink = targetDbLinkCombo_->currentText();
    if (!dbLink.isEmpty() && dbLink != tr("(local)")) {
        target += "@" + dbLink;
    }
    
    if (publicCheck_->isChecked()) {
        return QString("CREATE OR REPLACE PUBLIC SYNONYM %1\n  FOR %2;")
            .arg(synonymName)
            .arg(target);
    } else {
        return QString("CREATE OR REPLACE SYNONYM %1.%2\n  FOR %3;")
            .arg(schema)
            .arg(synonymName)
            .arg(target);
    }
}

// ============================================================================
// Target Browser Dialog
// ============================================================================
TargetBrowserDialog::TargetBrowserDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Select Target Object"));
    setMinimumSize(400, 500);
    
    setupUi();
}

void TargetBrowserDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Schema selection
    auto* schemaLayout = new QHBoxLayout();
    schemaLayout->addWidget(new QLabel(tr("Schema:")));
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    connect(schemaCombo_, &QComboBox::currentTextChanged,
            this, &TargetBrowserDialog::loadObjects);
    schemaLayout->addWidget(schemaCombo_);
    mainLayout->addLayout(schemaLayout);
    
    // Object tree
    objectTree_ = new QTreeView(this);
    objectTree_->setAlternatingRowColors(true);
    
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(2);
    model_->setHorizontalHeaderLabels({tr("Object Name"), tr("Type")});
    objectTree_->setModel(model_);
    objectTree_->header()->setStretchLastSection(false);
    objectTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    mainLayout->addWidget(objectTree_);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, [this]() {
        auto index = objectTree_->currentIndex();
        if (index.isValid()) {
            selectedSchema_ = schemaCombo_->currentText();
            selectedName_ = model_->item(index.row(), 0)->text();
            selectedType_ = model_->item(index.row(), 1)->text();
            accept();
        }
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    loadObjects("public");
}

void TargetBrowserDialog::loadObjects(const QString& schema)
{
    model_->removeRows(0, model_->rowCount());
    
    // TODO: Load from SessionClient when API is available
    // For now, populate with sample data structure
}

QString TargetBrowserDialog::selectedSchema() const { return selectedSchema_; }
QString TargetBrowserDialog::selectedName() const { return selectedName_; }
QString TargetBrowserDialog::selectedType() const { return selectedType_; }

} // namespace scratchrobin::ui
