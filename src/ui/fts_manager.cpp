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
    
    if (!client_) return;
    
    // Query pg_ts_config for text search configurations
    std::string sql = 
        "SELECT c.cfgname, p.prsname as parser, "
        "COALESCE(obj_description(c.oid, 'pg_ts_config'), '') as description "
        "FROM pg_ts_config c "
        "JOIN pg_ts_parser p ON c.cfgparser = p.oid "
        "ORDER BY c.cfgname";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() < 3) continue;
            
            QList<QStandardItem*> items;
            items << new QStandardItem(QString::fromStdString(row[0])); // Configuration
            items << new QStandardItem(QString::fromStdString(row[1])); // Parser
            items << new QStandardItem(QString::fromStdString(row[2])); // Description
            
            configModel_->appendRow(items);
        }
    }
}

void FtsManagerPanel::loadDictionaries()
{
    dictionaryModel_->removeRows(0, dictionaryModel_->rowCount());
    
    if (!client_) return;
    
    // Query pg_ts_dict for text search dictionaries
    std::string sql = 
        "SELECT d.dictname, t.tmplname as template, "
        "COALESCE(obj_description(d.oid, 'pg_ts_dict'), '') as description "
        "FROM pg_ts_dict d "
        "JOIN pg_ts_template t ON d.dicttemplate = t.oid "
        "ORDER BY d.dictname";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() < 3) continue;
            
            QList<QStandardItem*> items;
            items << new QStandardItem(QString::fromStdString(row[0])); // Dictionary
            items << new QStandardItem(QString::fromStdString(row[1])); // Template
            items << new QStandardItem(QString::fromStdString(row[2])); // Description
            
            dictionaryModel_->appendRow(items);
        }
    }
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
    auto index = configTree_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, tr("Alter Configuration"), tr("Please select a configuration."));
        return;
    }
    
    QString configName = configModel_->item(index.row(), 0)->text();
    AlterFtsConfigurationDialog dialog(client_, configName, this);
    dialog.exec();
}

void FtsManagerPanel::onDropObject()
{
    int tab = tabWidget_->currentIndex();
    
    if (tab == 0) { // Configurations
        auto index = configTree_->currentIndex();
        if (!index.isValid()) {
            QMessageBox::information(this, tr("Drop"), tr("Please select a configuration."));
            return;
        }
        
        QString configName = configModel_->item(index.row(), 0)->text();
        
        auto reply = QMessageBox::question(this, tr("Drop Configuration"),
            tr("Drop text search configuration '%1'?").arg(configName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
        if (reply == QMessageBox::Yes && client_) {
            std::string sql = QString("DROP TEXT SEARCH CONFIGURATION IF EXISTS %1 CASCADE").arg(configName).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
            refresh();
        }
    } else if (tab == 1) { // Dictionaries
        auto index = dictionaryTree_->currentIndex();
        if (!index.isValid()) {
            QMessageBox::information(this, tr("Drop"), tr("Please select a dictionary."));
            return;
        }
        
        QString dictName = dictionaryModel_->item(index.row(), 0)->text();
        
        auto reply = QMessageBox::question(this, tr("Drop Dictionary"),
            tr("Drop text search dictionary '%1'?").arg(dictName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
        if (reply == QMessageBox::Yes && client_) {
            std::string sql = QString("DROP TEXT SEARCH DICTIONARY IF EXISTS %1 CASCADE").arg(dictName).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
            refresh();
        }
    }
}

void FtsManagerPanel::onTestConfiguration()
{
    TestFtsConfigurationDialog dialog(client_, QString(), this);
    dialog.exec();
}

void FtsManagerPanel::onFilterChanged(const QString& filter)
{
    int tab = tabWidget_->currentIndex();
    
    if (tab == 0) { // Configurations
        for (int i = 0; i < configModel_->rowCount(); ++i) {
            bool match = filter.isEmpty();
            if (!match) {
                for (int j = 0; j < configModel_->columnCount(); ++j) {
                    auto* item = configModel_->item(i, j);
                    if (item && item->text().contains(filter, Qt::CaseInsensitive)) {
                        match = true;
                        break;
                    }
                }
            }
            configTree_->setRowHidden(i, QModelIndex(), !match);
        }
    } else if (tab == 1) { // Dictionaries
        for (int i = 0; i < dictionaryModel_->rowCount(); ++i) {
            bool match = filter.isEmpty();
            if (!match) {
                for (int j = 0; j < dictionaryModel_->columnCount(); ++j) {
                    auto* item = dictionaryModel_->item(i, j);
                    if (item && item->text().contains(filter, Qt::CaseInsensitive)) {
                        match = true;
                        break;
                    }
                }
            }
            dictionaryTree_->setRowHidden(i, QModelIndex(), !match);
        }
    }
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
    parserCombo_->clear();
    
    if (!client_) {
        parserCombo_->addItem("default");
        return;
    }
    
    std::string sql = "SELECT prsname FROM pg_ts_parser ORDER BY prsname";
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (!row.empty()) {
                parserCombo_->addItem(QString::fromStdString(row[0]));
            }
        }
    }
    
    if (parserCombo_->count() == 0) {
        parserCombo_->addItem("default");
    }
}

void CreateFtsConfigurationDialog::loadCopyFromConfigs()
{
    copyFromCombo_->clear();
    copyFromCombo_->addItem(tr("(none)"));
    
    if (!client_) return;
    
    std::string sql = "SELECT cfgname FROM pg_ts_config ORDER BY cfgname";
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (!row.empty()) {
                copyFromCombo_->addItem(QString::fromStdString(row[0]));
            }
        }
    }
}

void CreateFtsConfigurationDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateFtsConfigurationDialog::onCreate()
{
    if (!client_) {
        accept();
        return;
    }
    
    QString sql = generateDdl();
    auto response = client_->ExecuteSql(4044, "scratchbird", sql.toStdString());
    
    if (response.status.ok) {
        accept();
    } else {
        QMessageBox::warning(this, tr("Error"),
            tr("Failed to create configuration: %1")
            .arg(QString::fromStdString(response.status.message)));
    }
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
    templateCombo_->clear();
    
    if (!client_) {
        templateCombo_->addItem("simple");
        templateCombo_->addItem("synonym");
        templateCombo_->addItem("ispell");
        templateCombo_->addItem("snowball");
        return;
    }
    
    std::string sql = "SELECT tmplname FROM pg_ts_template ORDER BY tmplname";
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (!row.empty()) {
                templateCombo_->addItem(QString::fromStdString(row[0]));
            }
        }
    }
    
    if (templateCombo_->count() == 0) {
        templateCombo_->addItem("simple");
        templateCombo_->addItem("synonym");
        templateCombo_->addItem("ispell");
        templateCombo_->addItem("snowball");
    }
}

void CreateFtsDictionaryDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateFtsDictionaryDialog::onCreate()
{
    if (!client_) {
        accept();
        return;
    }
    
    QString sql = generateDdl();
    auto response = client_->ExecuteSql(4044, "scratchbird", sql.toStdString());
    
    if (response.status.ok) {
        accept();
    } else {
        QMessageBox::warning(this, tr("Error"),
            tr("Failed to create dictionary: %1")
            .arg(QString::fromStdString(response.status.message)));
    }
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
    tokenTypeCombo_->clear();
    
    if (!client_) return;
    
    // Get token types from the parser associated with this config
    std::string sql = QString(
        "SELECT t.alias "
        "FROM pg_ts_config c "
        "JOIN pg_ts_parser p ON c.cfgparser = p.oid "
        "JOIN pg_ts_config_map m ON m.mapcfg = c.oid "
        "JOIN pg_ts_token_type t ON t.tokid = m.maptokentype AND t.tokid != 0 "
        "WHERE c.cfgname = '%1' "
        "GROUP BY t.alias "
        "ORDER BY t.alias"
    ).arg(configName_).toStdString();
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (!row.empty()) {
                tokenTypeCombo_->addItem(QString::fromStdString(row[0]));
            }
        }
    }
}

void AlterFtsConfigurationDialog::loadDictionaries()
{
    dictionaryList_->clear();
    
    if (!client_) return;
    
    std::string sql = 
        "SELECT dictname FROM pg_ts_dict ORDER BY dictname";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (!row.empty()) {
                dictionaryList_->addItem(QString::fromStdString(row[0]));
            }
        }
    }
}

void AlterFtsConfigurationDialog::onAddMapping()
{
    QString tokenType = tokenTypeCombo_->currentText();
    QString dictName = dictionaryList_->currentItem() ? dictionaryList_->currentItem()->text() : QString();
    
    if (tokenType.isEmpty() || dictName.isEmpty()) {
        QMessageBox::warning(this, tr("Add Mapping"), tr("Please select a token type and dictionary."));
        return;
    }
    
    if (client_) {
        std::string sql = QString("ALTER TEXT SEARCH CONFIGURATION %1 ADD MAPPING FOR '%2' WITH %3")
            .arg(configName_).arg(tokenType).arg(dictName).toStdString();
        client_->ExecuteSql(4044, "scratchbird", sql);
    }
    
    previewEdit_->setPlainText(generateDdl());
}

void AlterFtsConfigurationDialog::onAlterMapping()
{
    QString tokenType = tokenTypeCombo_->currentText();
    QString dictName = dictionaryList_->currentItem() ? dictionaryList_->currentItem()->text() : QString();
    
    if (tokenType.isEmpty() || dictName.isEmpty()) {
        QMessageBox::warning(this, tr("Alter Mapping"), tr("Please select a token type and dictionary."));
        return;
    }
    
    if (client_) {
        std::string sql = QString("ALTER TEXT SEARCH CONFIGURATION %1 ALTER MAPPING FOR '%2' WITH %3")
            .arg(configName_).arg(tokenType).arg(dictName).toStdString();
        client_->ExecuteSql(4044, "scratchbird", sql);
    }
    
    previewEdit_->setPlainText(generateDdl());
}

void AlterFtsConfigurationDialog::onDropMapping()
{
    QString tokenType = tokenTypeCombo_->currentText();
    
    if (tokenType.isEmpty()) {
        QMessageBox::warning(this, tr("Drop Mapping"), tr("Please select a token type."));
        return;
    }
    
    if (client_) {
        std::string sql = QString("ALTER TEXT SEARCH CONFIGURATION %1 DROP MAPPING FOR '%2'")
            .arg(configName_).arg(tokenType).toStdString();
        client_->ExecuteSql(4044, "scratchbird", sql);
    }
    
    previewEdit_->setPlainText(generateDdl());
}

void AlterFtsConfigurationDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void AlterFtsConfigurationDialog::onApply()
{
    if (client_) {
        QString sql = generateDdl();
        auto response = client_->ExecuteSql(4044, "scratchbird", sql.toStdString());
        
        if (!response.status.ok) {
            QMessageBox::warning(this, tr("Error"),
                tr("Failed to alter configuration: %1")
                .arg(QString::fromStdString(response.status.message)));
            return;
        }
    }
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
    configCombo_->clear();
    
    if (!client_) {
        configCombo_->addItem("english");
        configCombo_->addItem("simple");
        return;
    }
    
    std::string sql = "SELECT cfgname FROM pg_ts_config ORDER BY cfgname";
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (!row.empty()) {
                configCombo_->addItem(QString::fromStdString(row[0]));
            }
        }
    }
    
    if (configCombo_->count() == 0) {
        configCombo_->addItem("english");
        configCombo_->addItem("simple");
    }
}

void TestFtsConfigurationDialog::onTest()
{
    onTestToTsvector();
}

void TestFtsConfigurationDialog::onTestToTsvector()
{
    if (!client_) {
        QString sql = QString("SELECT to_tsvector('%1', '%2');")
            .arg(configCombo_->currentText())
            .arg(inputEdit_->toPlainText());
        resultEdit_->setPlainText(tr("Would execute:\n%1\n\n(Offline mode)").arg(sql));
        return;
    }
    
    QString sql = QString("SELECT to_tsvector('%1', '%2')::text")
        .arg(configCombo_->currentText())
        .arg(inputEdit_->toPlainText());
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql.toStdString());
    
    if (response.status.ok && !response.result_set.rows.empty()) {
        resultEdit_->setPlainText(QString::fromStdString(response.result_set.rows[0][0]));
    } else {
        resultEdit_->setPlainText(tr("Error: %1")
            .arg(QString::fromStdString(response.status.message)));
    }
}

void TestFtsConfigurationDialog::onTestToTsquery()
{
    if (!client_) {
        QString sql = QString("SELECT to_tsquery('%1', '%2');")
            .arg(configCombo_->currentText())
            .arg(inputEdit_->toPlainText());
        resultEdit_->setPlainText(tr("Would execute:\n%1\n\n(Offline mode)").arg(sql));
        return;
    }
    
    QString sql = QString("SELECT to_tsquery('%1', '%2')::text")
        .arg(configCombo_->currentText())
        .arg(inputEdit_->toPlainText());
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql.toStdString());
    
    if (response.status.ok && !response.result_set.rows.empty()) {
        resultEdit_->setPlainText(QString::fromStdString(response.result_set.rows[0][0]));
    } else {
        resultEdit_->setPlainText(tr("Error: %1")
            .arg(QString::fromStdString(response.status.message)));
    }
}

} // namespace scratchrobin::ui
