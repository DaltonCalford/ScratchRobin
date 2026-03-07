#include "ui/fts_manager.h"
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
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QToolBar>
#include <QLabel>
#include <QListWidget>
#include <QTabWidget>

namespace scratchrobin::ui {

// ============================================================================
// FTS Manager Panel
// ============================================================================
FtsManagerPanel::FtsManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("fts_manager", parent)
    , client_(client)
{
    setupUi();
    setupModels();
    refresh();
}

void FtsManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* createConfigBtn = new QPushButton(tr("New Configuration"), this);
    auto* createDictBtn = new QPushButton(tr("New Dictionary"), this);
    auto* alterBtn = new QPushButton(tr("Alter"), this);
    auto* dropBtn = new QPushButton(tr("Drop"), this);
    auto* testBtn = new QPushButton(tr("Test"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createConfigBtn);
    toolbar->addWidget(createDictBtn);
    toolbar->addWidget(alterBtn);
    toolbar->addWidget(dropBtn);
    toolbar->addWidget(testBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(createConfigBtn, &QPushButton::clicked, this, &FtsManagerPanel::onCreateConfiguration);
    connect(createDictBtn, &QPushButton::clicked, this, &FtsManagerPanel::onCreateDictionary);
    connect(testBtn, &QPushButton::clicked, this, &FtsManagerPanel::onTestConfiguration);
    connect(refreshBtn, &QPushButton::clicked, this, &FtsManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &FtsManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Tabs
    tabWidget_ = new QTabWidget(this);
    
    configTree_ = new QTreeView(this);
    configTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(configTree_, tr("Configurations"));
    
    dictionaryTree_ = new QTreeView(this);
    dictionaryTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(dictionaryTree_, tr("Dictionaries"));
    
    mainLayout->addWidget(tabWidget_);
}

void FtsManagerPanel::setupModels()
{
    configModel_ = new QStandardItemModel(this);
    configModel_->setColumnCount(3);
    configModel_->setHorizontalHeaderLabels({tr("Configuration"), tr("Parser"), tr("Description")});
    configTree_->setModel(configModel_);
    
    dictionaryModel_ = new QStandardItemModel(this);
    dictionaryModel_->setColumnCount(3);
    dictionaryModel_->setHorizontalHeaderLabels({tr("Dictionary"), tr("Template"), tr("Description")});
    dictionaryTree_->setModel(dictionaryModel_);
}

void FtsManagerPanel::refresh()
{
    loadConfigurations();
    loadDictionaries();
}

void FtsManagerPanel::panelActivated()
{
    refresh();
}

void FtsManagerPanel::loadConfigurations()
{
    configModel_->removeRows(0, configModel_->rowCount());
}

void FtsManagerPanel::loadDictionaries()
{
    dictionaryModel_->removeRows(0, dictionaryModel_->rowCount());
}

void FtsManagerPanel::onCreateConfiguration()
{
    CreateFtsConfigurationDialog dialog(client_, this);
    dialog.exec();
}

void FtsManagerPanel::onCreateDictionary()
{
    CreateFtsDictionaryDialog dialog(client_, this);
    dialog.exec();
}

void FtsManagerPanel::onAlterConfiguration()
{
    // TODO: Get selected config
    AlterFtsConfigurationDialog dialog(client_, "config", this);
    dialog.exec();
}

void FtsManagerPanel::onDropObject()
{
    // TODO: Drop selected object
}

void FtsManagerPanel::onTestConfiguration()
{
    TestFtsConfigurationDialog dialog(client_, QString(), this);
    dialog.exec();
}

void FtsManagerPanel::onFilterChanged(const QString& filter)
{
    Q_UNUSED(filter)
}

// ============================================================================
// Create FTS Configuration Dialog
// ============================================================================
CreateFtsConfigurationDialog::CreateFtsConfigurationDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create FTS Configuration"));
    setMinimumSize(400, 250);
    setupUi();
}

void CreateFtsConfigurationDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    nameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Name:"), nameEdit_);
    parserCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Parser:"), parserCombo_);
    copyFromCombo_ = new QComboBox(this);
    copyFromCombo_->addItem(tr("(none)"));
    formLayout->addRow(tr("Copy From:"), copyFromCombo_);
    mainLayout->addLayout(formLayout);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateFtsConfigurationDialog::loadParsers()
{
    parserCombo_->addItem("default");
}

void CreateFtsConfigurationDialog::loadCopyFromConfigs()
{
}

void CreateFtsConfigurationDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateFtsConfigurationDialog::onCreate()
{
    accept();
}

QString CreateFtsConfigurationDialog::generateDdl()
{
    if (copyFromCombo_->currentText() != tr("(none)")) {
        return QString("CREATE TEXT SEARCH CONFIGURATION %1 (COPY = %2);")
            .arg(nameEdit_->text())
            .arg(copyFromCombo_->currentText());
    }
    return QString("CREATE TEXT SEARCH CONFIGURATION %1 (PARSER = %2);")
        .arg(nameEdit_->text())
        .arg(parserCombo_->currentText());
}

// ============================================================================
// Create FTS Dictionary Dialog
// ============================================================================
CreateFtsDictionaryDialog::CreateFtsDictionaryDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create FTS Dictionary"));
    setMinimumSize(400, 300);
    setupUi();
}

void CreateFtsDictionaryDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    nameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Name:"), nameEdit_);
    templateCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Template:"), templateCombo_);
    optionsEdit_ = new QTextEdit(this);
    formLayout->addRow(tr("Options:"), optionsEdit_);
    mainLayout->addLayout(formLayout);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateFtsDictionaryDialog::loadTemplates()
{
    templateCombo_->addItem("simple");
    templateCombo_->addItem("synonym");
    templateCombo_->addItem("ispell");
    templateCombo_->addItem("snowball");
}

void CreateFtsDictionaryDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateFtsDictionaryDialog::onCreate()
{
    accept();
}

QString CreateFtsDictionaryDialog::generateDdl()
{
    return QString("CREATE TEXT SEARCH DICTIONARY %1 (TEMPLATE = %2);").arg(nameEdit_->text()).arg(templateCombo_->currentText());
}

// ============================================================================
// Alter FTS Configuration Dialog
// ============================================================================
AlterFtsConfigurationDialog::AlterFtsConfigurationDialog(backend::SessionClient* client,
                                                        const QString& configName,
                                                        QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , configName_(configName)
{
    setWindowTitle(tr("Alter FTS Configuration - %1").arg(configName));
    setMinimumSize(400, 350);
    setupUi();
}

void AlterFtsConfigurationDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    tokenTypeCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Token Type:"), tokenTypeCombo_);
    mainLayout->addLayout(formLayout);
    
    dictionaryList_ = new QListWidget(this);
    mainLayout->addWidget(new QLabel(tr("Dictionaries:")));
    mainLayout->addWidget(dictionaryList_);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void AlterFtsConfigurationDialog::loadTokenTypes()
{
}

void AlterFtsConfigurationDialog::loadDictionaries()
{
}

void AlterFtsConfigurationDialog::onAddMapping()
{
}

void AlterFtsConfigurationDialog::onAlterMapping()
{
}

void AlterFtsConfigurationDialog::onDropMapping()
{
}

void AlterFtsConfigurationDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void AlterFtsConfigurationDialog::onApply()
{
    accept();
}

QString AlterFtsConfigurationDialog::generateDdl()
{
    return QString("ALTER TEXT SEARCH CONFIGURATION %1 ...").arg(configName_);
}

// ============================================================================
// Test FTS Configuration Dialog
// ============================================================================
TestFtsConfigurationDialog::TestFtsConfigurationDialog(backend::SessionClient* client,
                                                      const QString& configName,
                                                      QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Test FTS Configuration"));
    setMinimumSize(500, 400);
    setupUi();
    
    if (!configName.isEmpty()) {
        configCombo_->setCurrentText(configName);
    }
}

void TestFtsConfigurationDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    configCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Configuration:"), configCombo_);
    mainLayout->addLayout(formLayout);
    
    inputEdit_ = new QTextEdit(this);
    inputEdit_->setPlaceholderText(tr("Enter text to analyze..."));
    inputEdit_->setMaximumHeight(100);
    mainLayout->addWidget(new QLabel(tr("Input Text:")));
    mainLayout->addWidget(inputEdit_);
    
    resultEdit_ = new QTextEdit(this);
    resultEdit_->setReadOnly(true);
    resultEdit_->setPlaceholderText(tr("Result will appear here..."));
    mainLayout->addWidget(new QLabel(tr("Result:")));
    mainLayout->addWidget(resultEdit_);
    
    auto* buttonLayout = new QHBoxLayout();
    auto* toTsvectorBtn = new QPushButton(tr("to_tsvector"), this);
    auto* toTsqueryBtn = new QPushButton(tr("to_tsquery"), this);
    auto* closeBtn = new QPushButton(tr("Close"), this);
    
    connect(toTsvectorBtn, &QPushButton::clicked, this, &TestFtsConfigurationDialog::onTestToTsvector);
    connect(toTsqueryBtn, &QPushButton::clicked, this, &TestFtsConfigurationDialog::onTestToTsquery);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addWidget(toTsvectorBtn);
    buttonLayout->addWidget(toTsqueryBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);
}

void TestFtsConfigurationDialog::loadConfigurations()
{
    configCombo_->addItem("english");
    configCombo_->addItem("simple");
}

void TestFtsConfigurationDialog::onTest()
{
    resultEdit_->setPlainText(tr("Test executed (not implemented)."));
}

void TestFtsConfigurationDialog::onTestToTsvector()
{
    QString sql = QString("SELECT to_tsvector('%1', '%2');")
        .arg(configCombo_->currentText())
        .arg(inputEdit_->toPlainText());
    resultEdit_->setPlainText(sql);
}

void TestFtsConfigurationDialog::onTestToTsquery()
{
    QString sql = QString("SELECT to_tsquery('%1', '%2');")
        .arg(configCombo_->currentText())
        .arg(inputEdit_->toPlainText());
    resultEdit_->setPlainText(sql);
}

} // namespace scratchrobin::ui
