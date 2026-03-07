#include "plugin_system.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QStackedWidget>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace scratchrobin::ui {

// ============================================================================
// Plugin Manager Panel
// ============================================================================
PluginManagerPanel::PluginManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("plugin_manager", parent), client_(client) {
    setupUi();
    loadPlugins();
}

void PluginManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* marketplaceBtn = new QPushButton(tr("Browse Marketplace"), this);
    connect(marketplaceBtn, &QPushButton::clicked, this, &PluginManagerPanel::onBrowseMarketplace);
    toolbarLayout->addWidget(marketplaceBtn);
    
    auto* installFileBtn = new QPushButton(tr("Install from File"), this);
    connect(installFileBtn, &QPushButton::clicked, this, &PluginManagerPanel::onInstallFromFile);
    toolbarLayout->addWidget(installFileBtn);
    
    toolbarLayout->addStretch();
    
    auto* checkUpdatesBtn = new QPushButton(tr("Check Updates"), this);
    connect(checkUpdatesBtn, &QPushButton::clicked, this, &PluginManagerPanel::onCheckUpdates);
    toolbarLayout->addWidget(checkUpdatesBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    typeFilterCombo_ = new QComboBox(this);
    typeFilterCombo_->addItem(tr("All Types"));
    typeFilterCombo_->addItem(tr("UI Extensions"));
    typeFilterCombo_->addItem(tr("Database Drivers"));
    typeFilterCombo_->addItem(tr("Tools"));
    typeFilterCombo_->addItem(tr("Themes"));
    typeFilterCombo_->addItem(tr("Languages"));
    connect(typeFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PluginManagerPanel::onFilterByType);
    filterLayout->addWidget(typeFilterCombo_);
    
    statusFilterCombo_ = new QComboBox(this);
    statusFilterCombo_->addItem(tr("All Status"));
    statusFilterCombo_->addItem(tr("Enabled"));
    statusFilterCombo_->addItem(tr("Disabled"));
    statusFilterCombo_->addItem(tr("Has Updates"));
    connect(statusFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PluginManagerPanel::onFilterByStatus);
    filterLayout->addWidget(statusFilterCombo_);
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search plugins..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &PluginManagerPanel::onSearchTextChanged);
    filterLayout->addWidget(searchEdit_, 1);
    
    mainLayout->addLayout(filterLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Plugins table
    pluginsTable_ = new QTableView(this);
    pluginsModel_ = new QStandardItemModel(this);
    pluginsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Version"), tr("Type"), tr("Status")});
    pluginsTable_->setModel(pluginsModel_);
    pluginsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginsTable_->setAlternatingRowColors(true);
    pluginsTable_->horizontalHeader()->setStretchLastSection(true);
    
    connect(pluginsTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PluginManagerPanel::onPluginSelected);
    
    splitter->addWidget(pluginsTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    
    pluginNameLabel_ = new QLabel(tr("Select a plugin"), this);
    QFont titleFont = pluginNameLabel_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    pluginNameLabel_->setFont(titleFont);
    detailsLayout->addWidget(pluginNameLabel_);
    
    pluginVersionLabel_ = new QLabel(this);
    detailsLayout->addWidget(pluginVersionLabel_);
    
    pluginAuthorLabel_ = new QLabel(this);
    detailsLayout->addWidget(pluginAuthorLabel_);
    
    pluginStatusLabel_ = new QLabel(this);
    detailsLayout->addWidget(pluginStatusLabel_);
    
    pluginDescriptionBrowser_ = new QTextBrowser(this);
    pluginDescriptionBrowser_->setMaximumHeight(100);
    detailsLayout->addWidget(pluginDescriptionBrowser_);
    
    detailsLayout->addWidget(new QLabel(tr("Permissions:"), this));
    permissionsList_ = new QListWidget(this);
    permissionsList_->setMaximumHeight(80);
    detailsLayout->addWidget(permissionsList_);
    
    detailsLayout->addWidget(new QLabel(tr("Dependencies:"), this));
    dependenciesList_ = new QListWidget(this);
    dependenciesList_->setMaximumHeight(60);
    detailsLayout->addWidget(dependenciesList_);
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    
    enableBtn_ = new QPushButton(tr("Enable"), this);
    enableBtn_->setEnabled(false);
    connect(enableBtn_, &QPushButton::clicked, this, &PluginManagerPanel::onEnablePlugin);
    btnLayout->addWidget(enableBtn_);
    
    disableBtn_ = new QPushButton(tr("Disable"), this);
    disableBtn_->setEnabled(false);
    connect(disableBtn_, &QPushButton::clicked, this, &PluginManagerPanel::onDisablePlugin);
    btnLayout->addWidget(disableBtn_);
    
    configureBtn_ = new QPushButton(tr("Configure"), this);
    configureBtn_->setEnabled(false);
    connect(configureBtn_, &QPushButton::clicked, this, &PluginManagerPanel::onConfigurePlugin);
    btnLayout->addWidget(configureBtn_);
    
    updateBtn_ = new QPushButton(tr("Update"), this);
    updateBtn_->setEnabled(false);
    connect(updateBtn_, &QPushButton::clicked, this, &PluginManagerPanel::onUpdatePlugin);
    btnLayout->addWidget(updateBtn_);
    
    uninstallBtn_ = new QPushButton(tr("Uninstall"), this);
    uninstallBtn_->setEnabled(false);
    connect(uninstallBtn_, &QPushButton::clicked, this, &PluginManagerPanel::onUninstallPlugin);
    btnLayout->addWidget(uninstallBtn_);
    
    detailsLayout->addLayout(btnLayout);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({400, 400});
    
    mainLayout->addWidget(splitter, 1);
}

void PluginManagerPanel::loadPlugins() {
    plugins_.clear();
    
    // Sample plugins
    PluginInfo p1;
    p1.id = "plugin.csv_importer";
    p1.name = tr("CSV Import/Export");
    p1.description = tr("Import and export data in CSV format with advanced options");
    p1.version = "1.2.0";
    p1.author = tr("ScratchRobin Team");
    p1.type = PluginType::ImportExport;
    p1.status = PluginStatus::Enabled;
    p1.isOfficial = true;
    p1.installedAt = QDateTime::currentDateTime().addDays(-30);
    plugins_.append(p1);
    
    PluginInfo p2;
    p2.id = "plugin.dark_theme";
    p2.name = tr("Midnight Theme");
    p2.description = tr("A dark theme optimized for low-light environments");
    p2.version = "2.0.1";
    p2.author = tr("Theme Studio");
    p2.type = PluginType::Theme;
    p2.status = PluginStatus::Enabled;
    p2.installedAt = QDateTime::currentDateTime().addDays(-60);
    plugins_.append(p2);
    
    PluginInfo p3;
    p3.id = "plugin.postgres_tools";
    p3.name = tr("PostgreSQL Tools");
    p3.description = tr("Advanced PostgreSQL-specific tools and features");
    p3.version = "1.5.2";
    p3.author = tr("DB Experts");
    p3.type = PluginType::DatabaseDriver;
    p3.status = PluginStatus::Disabled;
    p3.installedAt = QDateTime::currentDateTime().addDays(-15);
    plugins_.append(p3);
    
    PluginInfo p4;
    p4.id = "plugin.git_sync";
    p4.name = tr("Git Synchronization");
    p4.description = tr("Sync queries and scripts with Git repositories");
    p4.version = "1.0.0";
    p4.author = tr("DevTools Inc");
    p4.type = PluginType::Integration;
    p4.status = PluginStatus::UpdateAvailable;
    p4.installedAt = QDateTime::currentDateTime().addDays(-90);
    plugins_.append(p4);
    
    updatePluginList();
}

void PluginManagerPanel::updatePluginList() {
    pluginsModel_->clear();
    pluginsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Version"), tr("Type"), tr("Status")});
    
    for (const auto& plugin : plugins_) {
        QString typeStr;
        switch (plugin.type) {
            case PluginType::UIExtension: typeStr = tr("UI"); break;
            case PluginType::DatabaseDriver: typeStr = tr("Driver"); break;
            case PluginType::Tool: typeStr = tr("Tool"); break;
            case PluginType::Theme: typeStr = tr("Theme"); break;
            case PluginType::LanguageSupport: typeStr = tr("Language"); break;
            case PluginType::ImportExport: typeStr = tr("I/O"); break;
            case PluginType::Integration: typeStr = tr("Integration"); break;
        }
        
        QString statusStr;
        switch (plugin.status) {
            case PluginStatus::Enabled: statusStr = tr("● Enabled"); break;
            case PluginStatus::Disabled: statusStr = tr("○ Disabled"); break;
            case PluginStatus::UpdateAvailable: statusStr = tr("↑ Update"); break;
            case PluginStatus::Error: statusStr = tr("✗ Error"); break;
            default: statusStr = tr("-");
        }
        
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(plugin.name)
             << new QStandardItem(plugin.version)
             << new QStandardItem(typeStr)
             << new QStandardItem(statusStr);
        
        (*row)[0]->setData(plugin.id, Qt::UserRole);
        
        // Color by status
        if (plugin.status == PluginStatus::Enabled) {
            (*row)[3]->setForeground(QBrush(QColor(0, 150, 0)));
        } else if (plugin.status == PluginStatus::UpdateAvailable) {
            (*row)[3]->setForeground(QBrush(QColor(0, 100, 200)));
        }
        
        pluginsModel_->appendRow(*row);
    }
}

void PluginManagerPanel::onPluginSelected(const QModelIndex& index) {
    if (!index.isValid()) {
        enableBtn_->setEnabled(false);
        disableBtn_->setEnabled(false);
        configureBtn_->setEnabled(false);
        uninstallBtn_->setEnabled(false);
        updateBtn_->setEnabled(false);
        return;
    }
    
    QString pluginId = pluginsModel_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    for (const auto& plugin : plugins_) {
        if (plugin.id == pluginId) {
            currentPlugin_ = plugin;
            updatePluginDetails(plugin);
            
            bool isEnabled = plugin.status == PluginStatus::Enabled;
            bool isDisabled = plugin.status == PluginStatus::Disabled;
            bool hasUpdate = plugin.status == PluginStatus::UpdateAvailable;
            
            enableBtn_->setEnabled(isDisabled);
            disableBtn_->setEnabled(isEnabled);
            configureBtn_->setEnabled(true);
            uninstallBtn_->setEnabled(!plugin.isBuiltin);
            updateBtn_->setEnabled(hasUpdate);
            break;
        }
    }
}

void PluginManagerPanel::updatePluginDetails(const PluginInfo& info) {
    pluginNameLabel_->setText(info.name);
    pluginVersionLabel_->setText(tr("Version: %1").arg(info.version));
    pluginAuthorLabel_->setText(tr("By: %1").arg(info.author));
    
    QString statusText;
    switch (info.status) {
        case PluginStatus::Enabled: statusText = tr("Status: <span style='color: green;'>Enabled</span>"); break;
        case PluginStatus::Disabled: statusText = tr("Status: <span style='color: gray;'>Disabled</span>"); break;
        case PluginStatus::UpdateAvailable: statusText = tr("Status: <span style='color: blue;'>Update Available</span>"); break;
        default: statusText = tr("Status: %1").arg(static_cast<int>(info.status));
    }
    pluginStatusLabel_->setText(statusText);
    
    pluginDescriptionBrowser_->setPlainText(info.description);
    
    permissionsList_->clear();
    for (const auto& perm : info.permissions) {
        permissionsList_->addItem(perm);
    }
    if (info.permissions.isEmpty()) {
        permissionsList_->addItem(tr("No special permissions required"));
    }
    
    dependenciesList_->clear();
    for (const auto& dep : info.dependencies) {
        dependenciesList_->addItem(dep);
    }
    if (info.dependencies.isEmpty()) {
        dependenciesList_->addItem(tr("No dependencies"));
    }
}

void PluginManagerPanel::onEnablePlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    // Update status
    for (auto& plugin : plugins_) {
        if (plugin.id == currentPlugin_.id) {
            plugin.status = PluginStatus::Enabled;
            break;
        }
    }
    
    updatePluginList();
    emit pluginEnabled(currentPlugin_.id);
    QMessageBox::information(this, tr("Plugin Enabled"), 
        tr("%1 has been enabled.").arg(currentPlugin_.name));
}

void PluginManagerPanel::onDisablePlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, tr("Disable Plugin"),
        tr("Are you sure you want to disable %1?").arg(currentPlugin_.name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        for (auto& plugin : plugins_) {
            if (plugin.id == currentPlugin_.id) {
                plugin.status = PluginStatus::Disabled;
                break;
            }
        }
        
        updatePluginList();
        emit pluginDisabled(currentPlugin_.id);
    }
}

void PluginManagerPanel::onConfigurePlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    PluginConfigDialog dialog(currentPlugin_, client_, this);
    dialog.exec();
}

void PluginManagerPanel::onUninstallPlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, tr("Uninstall Plugin"),
        tr("Are you sure you want to uninstall %1?\n\nThis action cannot be undone.")
        .arg(currentPlugin_.name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        plugins_.removeAll(currentPlugin_);
        currentPlugin_ = PluginInfo();
        updatePluginList();
        emit pluginUninstalled(currentPlugin_.id);
    }
}

void PluginManagerPanel::onUpdatePlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    QMessageBox::information(this, tr("Update Plugin"),
        tr("Updating %1 to the latest version...").arg(currentPlugin_.name));
    
    // Simulate update
    for (auto& plugin : plugins_) {
        if (plugin.id == currentPlugin_.id) {
            plugin.status = PluginStatus::Enabled;
            plugin.version = "1.1.0"; // New version
            break;
        }
    }
    
    updatePluginList();
}

void PluginManagerPanel::onReloadPlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    QMessageBox::information(this, tr("Reload Plugin"),
        tr("%1 has been reloaded.").arg(currentPlugin_.name));
}

void PluginManagerPanel::onBrowseMarketplace() {
    PluginMarketplaceDialog dialog(client_, this);
    dialog.exec();
}

void PluginManagerPanel::onCheckUpdates() {
    QMessageBox::information(this, tr("Check Updates"),
        tr("Checking for plugin updates...\n\n1 update available."));
}

void PluginManagerPanel::onInstallFromFile() {
    InstallPluginDialog dialog(client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        QMessageBox::information(this, tr("Plugin Installed"),
            tr("Plugin has been installed successfully."));
        loadPlugins();
    }
}

void PluginManagerPanel::onExportPlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Plugin"),
        currentPlugin_.id + ".srplugin", tr("Plugin Files (*.srplugin)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"),
            tr("Plugin exported to %1").arg(fileName));
    }
}

void PluginManagerPanel::onFilterByType(int type) {
    Q_UNUSED(type)
    filterPlugins();
}

void PluginManagerPanel::onFilterByStatus(int status) {
    Q_UNUSED(status)
    filterPlugins();
}

void PluginManagerPanel::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    filterPlugins();
}

void PluginManagerPanel::onCategoryChanged(int index) {
    Q_UNUSED(index)
    filterPlugins();
}

void PluginManagerPanel::filterPlugins() {
    // TODO: Implement filtering
    updatePluginList();
}

// ============================================================================
// Plugin Marketplace Dialog
// ============================================================================
PluginMarketplaceDialog::PluginMarketplaceDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadMarketplace();
}

void PluginMarketplaceDialog::setupUi() {
    setWindowTitle(tr("Plugin Marketplace"));
    setMinimumSize(800, 600);
    
    auto* layout = new QVBoxLayout(this);
    
    // Search and filter bar
    auto* filterLayout = new QHBoxLayout();
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search plugins..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &PluginMarketplaceDialog::onSearchTextChanged);
    filterLayout->addWidget(searchEdit_, 1);
    
    sortCombo_ = new QComboBox(this);
    sortCombo_->addItem(tr("Most Popular"));
    sortCombo_->addItem(tr("Highest Rated"));
    sortCombo_->addItem(tr("Newest"));
    sortCombo_->addItem(tr("Name A-Z"));
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PluginMarketplaceDialog::onSortChanged);
    filterLayout->addWidget(sortCombo_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &PluginMarketplaceDialog::onRefreshList);
    filterLayout->addWidget(refreshBtn);
    
    layout->addLayout(filterLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Categories
    categoryTree_ = new QTreeWidget(this);
    categoryTree_->setHeaderLabel(tr("Categories"));
    categoryTree_->addTopLevelItem(new QTreeWidgetItem(QStringList(tr("All Plugins"))));
    categoryTree_->addTopLevelItem(new QTreeWidgetItem(QStringList(tr("Database Drivers"))));
    categoryTree_->addTopLevelItem(new QTreeWidgetItem(QStringList(tr("Import/Export"))));
    categoryTree_->addTopLevelItem(new QTreeWidgetItem(QStringList(tr("Themes"))));
    categoryTree_->addTopLevelItem(new QTreeWidgetItem(QStringList(tr("Developer Tools"))));
    categoryTree_->addTopLevelItem(new QTreeWidgetItem(QStringList(tr("Productivity"))));
    connect(categoryTree_, &QTreeWidget::itemClicked,
            [this](QTreeWidgetItem* item, int) {
                if (item) onCategorySelected(item->text(0));
            });
    splitter->addWidget(categoryTree_);
    
    // Plugins table
    pluginsTable_ = new QTableView(this);
    pluginsModel_ = new QStandardItemModel(this);
    pluginsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Author"), tr("Rating"), tr("Downloads")});
    pluginsTable_->setModel(pluginsModel_);
    pluginsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginsTable_->setAlternatingRowColors(true);
    
    connect(pluginsTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PluginMarketplaceDialog::onPluginSelected);
    
    splitter->addWidget(pluginsTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    
    nameLabel_ = new QLabel(tr("Select a plugin"), this);
    QFont titleFont = nameLabel_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    nameLabel_->setFont(titleFont);
    detailsLayout->addWidget(nameLabel_);
    
    authorLabel_ = new QLabel(this);
    detailsLayout->addWidget(authorLabel_);
    
    auto* statsLayout = new QHBoxLayout();
    ratingLabel_ = new QLabel(this);
    downloadsLabel_ = new QLabel(this);
    statsLayout->addWidget(ratingLabel_);
    statsLayout->addWidget(downloadsLabel_);
    statsLayout->addStretch();
    detailsLayout->addLayout(statsLayout);
    
    descriptionBrowser_ = new QTextBrowser(this);
    detailsLayout->addWidget(descriptionBrowser_, 1);
    
    installBtn_ = new QPushButton(tr("Install"), this);
    installBtn_->setEnabled(false);
    connect(installBtn_, &QPushButton::clicked, this, &PluginMarketplaceDialog::onInstallPlugin);
    detailsLayout->addWidget(installBtn_);
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({150, 400, 250});
    
    layout->addWidget(splitter, 1);
}

void PluginMarketplaceDialog::loadMarketplace() {
    marketplacePlugins_.clear();
    
    // Sample marketplace plugins
    PluginInfo p1;
    p1.id = "marketplace.json_viewer";
    p1.name = tr("JSON Viewer");
    p1.description = tr("View and edit JSON data with syntax highlighting and formatting");
    p1.version = "1.0.0";
    p1.author = tr("Data Tools Inc");
    p1.type = PluginType::UIExtension;
    p1.rating = 4.5;
    p1.reviewCount = 128;
    p1.downloadCount = 5420;
    marketplacePlugins_.append(p1);
    
    PluginInfo p2;
    p2.id = "marketplace.redis_driver";
    p2.name = tr("Redis Driver");
    p2.description = tr("Connect to Redis databases");
    p2.version = "2.1.0";
    p2.author = tr("Cache Experts");
    p2.type = PluginType::DatabaseDriver;
    p2.rating = 4.8;
    p2.reviewCount = 85;
    p2.downloadCount = 3200;
    marketplacePlugins_.append(p2);
    
    PluginInfo p3;
    p3.id = "marketplace.excel_export";
    p3.name = tr("Excel Export Pro");
    p3.description = tr("Export query results to Excel with formatting");
    p3.version = "1.5.0";
    p3.author = tr("Office Tools");
    p3.type = PluginType::ImportExport;
    p3.rating = 4.2;
    p3.reviewCount = 256;
    p3.downloadCount = 8900;
    marketplacePlugins_.append(p3);
    
    updatePluginList();
}

void PluginMarketplaceDialog::updatePluginList() {
    pluginsModel_->clear();
    pluginsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Author"), tr("Rating"), tr("Downloads")});
    
    for (const auto& plugin : marketplacePlugins_) {
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(plugin.name)
             << new QStandardItem(plugin.author)
             << new QStandardItem(QString("★ %1").arg(plugin.rating))
             << new QStandardItem(QString::number(plugin.downloadCount));
        
        (*row)[0]->setData(plugin.id, Qt::UserRole);
        pluginsModel_->appendRow(*row);
    }
}

void PluginMarketplaceDialog::updatePluginDetails(const PluginInfo& info) {
    currentPlugin_ = info;
    
    nameLabel_->setText(info.name);
    authorLabel_->setText(tr("By %1").arg(info.author));
    ratingLabel_->setText(tr("★ %1 (%2 reviews)").arg(info.rating).arg(info.reviewCount));
    downloadsLabel_->setText(tr("%1 downloads").arg(info.downloadCount));
    descriptionBrowser_->setPlainText(info.description);
    
    installBtn_->setEnabled(true);
}

void PluginMarketplaceDialog::onCategorySelected(const QString& category) {
    Q_UNUSED(category)
    updatePluginList();
}

void PluginMarketplaceDialog::onPluginSelected(const QModelIndex& index) {
    if (!index.isValid()) {
        installBtn_->setEnabled(false);
        return;
    }
    
    QString pluginId = pluginsModel_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    for (const auto& plugin : marketplacePlugins_) {
        if (plugin.id == pluginId) {
            updatePluginDetails(plugin);
            break;
        }
    }
}

void PluginMarketplaceDialog::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    // TODO: Filter
}

void PluginMarketplaceDialog::onSortChanged(int index) {
    Q_UNUSED(index)
    updatePluginList();
}

void PluginMarketplaceDialog::onFilterChanged() {
    updatePluginList();
}

void PluginMarketplaceDialog::onInstallPlugin() {
    if (currentPlugin_.id.isEmpty()) return;
    
    QMessageBox::information(this, tr("Installing"),
        tr("Installing %1...\n\nThe plugin will be available after restart.").arg(currentPlugin_.name));
    
    accept();
}

void PluginMarketplaceDialog::onPreviewPlugin() {
    // TODO: Show preview
}

void PluginMarketplaceDialog::onViewReviews() {
    // TODO: Show reviews
}

void PluginMarketplaceDialog::onCheckForUpdates() {
    QMessageBox::information(this, tr("Updates"), tr("All plugins are up to date."));
}

void PluginMarketplaceDialog::onRefreshList() {
    loadMarketplace();
}

// ============================================================================
// Plugin Configuration Dialog
// ============================================================================
PluginConfigDialog::PluginConfigDialog(const PluginInfo& info, backend::SessionClient* client,
                                       QWidget* parent)
    : QDialog(parent), pluginInfo_(info), client_(client) {
    setupUi();
    loadSettings();
}

void PluginConfigDialog::setupUi() {
    setWindowTitle(tr("Configure: %1").arg(pluginInfo_.name));
    setMinimumSize(450, 350);
    
    auto* layout = new QVBoxLayout(this);
    
    tabWidget_ = new QTabWidget(this);
    
    // General tab
    generalTab_ = new QWidget(this);
    auto* generalLayout = new QFormLayout(generalTab_);
    generalLayout->addRow(tr("Plugin:"), new QLabel(pluginInfo_.name, this));
    generalLayout->addRow(tr("Version:"), new QLabel(pluginInfo_.version, this));
    generalLayout->addRow(tr("Author:"), new QLabel(pluginInfo_.author, this));
    tabWidget_->addTab(generalTab_, tr("General"));
    
    // Advanced tab
    advancedTab_ = new QWidget(this);
    auto* advancedLayout = new QVBoxLayout(advancedTab_);
    advancedLayout->addWidget(new QLabel(tr("Advanced settings will appear here."), this));
    advancedLayout->addStretch();
    tabWidget_->addTab(advancedTab_, tr("Advanced"));
    
    // Permissions tab
    permissionsTab_ = new QWidget(this);
    auto* permLayout = new QVBoxLayout(permissionsTab_);
    permLayout->addWidget(new QLabel(tr("Plugin permissions:"), this));
    auto* permList = new QListWidget(this);
    permList->addItem(tr("Access file system"));
    permList->addItem(tr("Connect to network"));
    permList->addItem(tr("Execute external commands"));
    permLayout->addWidget(permList);
    tabWidget_->addTab(permissionsTab_, tr("Permissions"));
    
    layout->addWidget(tabWidget_, 1);
    
    // Buttons
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                                      QDialogButtonBox::Reset, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox_);
}

void PluginConfigDialog::loadSettings() {
    // TODO: Load plugin settings
}

void PluginConfigDialog::saveSettings() {
    // TODO: Save plugin settings
}

void PluginConfigDialog::onSettingChanged(const QString& key, const QVariant& value) {
    Q_UNUSED(key)
    Q_UNUSED(value)
}

void PluginConfigDialog::onResetDefaults() {
    QMessageBox::information(this, tr("Reset"), tr("Settings reset to defaults."));
}

void PluginConfigDialog::onImportSettings() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Settings"),
        QString(), tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Imported"), tr("Settings imported."));
    }
}

void PluginConfigDialog::onExportSettings() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Settings"),
        tr("settings.json"), tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("Settings exported."));
    }
}

void PluginConfigDialog::onValidate() {
    accept();
}

// ============================================================================
// Install Plugin Dialog
// ============================================================================
InstallPluginDialog::InstallPluginDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
}

void InstallPluginDialog::setupUi() {
    setWindowTitle(tr("Install Plugin"));
    setMinimumSize(400, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    stack_ = new QStackedWidget(this);
    
    // Page 1: Select source
    auto* sourcePage = new QWidget(this);
    auto* sourceLayout = new QVBoxLayout(sourcePage);
    
    sourceLayout->addWidget(new QLabel(tr("Select plugin file:"), this));
    
    auto* fileLayout = new QHBoxLayout();
    pathEdit_ = new QLineEdit(this);
    pathEdit_->setPlaceholderText(tr("Path to .srplugin file..."));
    fileLayout->addWidget(pathEdit_);
    
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &InstallPluginDialog::onBrowseFile);
    fileLayout->addWidget(browseBtn);
    
    sourceLayout->addLayout(fileLayout);
    
    sourceLayout->addWidget(new QLabel(tr("Or enter URL:"), this));
    urlEdit_ = new QLineEdit(this);
    urlEdit_->setPlaceholderText(tr("https://..."));
    sourceLayout->addWidget(urlEdit_);
    
    auto* urlBtn = new QPushButton(tr("Install from URL"), this);
    connect(urlBtn, &QPushButton::clicked, this, &InstallPluginDialog::onInstallFromUrl);
    sourceLayout->addWidget(urlBtn);
    
    sourceLayout->addStretch();
    
    stack_->addWidget(sourcePage);
    
    // Page 2: Permissions
    auto* permPage = new QWidget(this);
    auto* permLayout = new QVBoxLayout(permPage);
    
    permLayout->addWidget(new QLabel(tr("This plugin requires the following permissions:"), this));
    
    permissionsList_ = new QListWidget(this);
    permLayout->addWidget(permissionsList_);
    
    auto* permBtnLayout = new QHBoxLayout();
    permBtnLayout->addStretch();
    
    auto* acceptPermBtn = new QPushButton(tr("Accept"), this);
    connect(acceptPermBtn, &QPushButton::clicked, this, &InstallPluginDialog::onAcceptPermissions);
    permBtnLayout->addWidget(acceptPermBtn);
    
    auto* cancelPermBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelPermBtn, &QPushButton::clicked, this, &QDialog::reject);
    permBtnLayout->addWidget(cancelPermBtn);
    
    permLayout->addLayout(permBtnLayout);
    
    stack_->addWidget(permPage);
    
    // Page 3: Progress
    auto* progressPage = new QWidget(this);
    auto* progressLayout = new QVBoxLayout(progressPage);
    
    progressLayout->addWidget(new QLabel(tr("Installing plugin..."), this));
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(50);
    progressLayout->addWidget(progressBar_);
    progressLayout->addStretch();
    
    stack_->addWidget(progressPage);
    
    layout->addWidget(stack_);
}

void InstallPluginDialog::onBrowseFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Plugin"),
        QString(), tr("Plugin Files (*.srplugin);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        pathEdit_->setText(fileName);
        validatePluginFile(fileName);
    }
}

void InstallPluginDialog::onInstallFromUrl() {
    QString url = urlEdit_->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a valid URL."));
        return;
    }
    
    QMessageBox::information(this, tr("Download"), tr("Downloading from %1...").arg(url));
    stack_->setCurrentIndex(2);
}

void InstallPluginDialog::validatePluginFile(const QString& path) {
    QFile file(path);
    if (!file.exists()) {
        QMessageBox::warning(this, tr("Error"), tr("File not found."));
        return;
    }
    
    // Show permissions page
    permissionsList_->clear();
    permissionsList_->addItem(tr("✓ Read/Write files"));
    permissionsList_->addItem(tr("✓ Access network"));
    permissionsList_->addItem(tr("✓ Execute SQL commands"));
    
    stack_->setCurrentIndex(1);
}

void InstallPluginDialog::onAcceptPermissions() {
    stack_->setCurrentIndex(2);
    
    // Simulate installation
    QTimer::singleShot(1500, this, [this]() {
        accept();
    });
}

void InstallPluginDialog::onValidatePlugin() {
    // Validate plugin structure
}

void InstallPluginDialog::onInstall() {
    accept();
}

QString InstallPluginDialog::selectedPluginPath() const {
    return pluginPath_;
}

// ============================================================================
// Plugin Developer Tools Dialog
// ============================================================================
PluginDevToolsDialog::PluginDevToolsDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadTemplates();
}

void PluginDevToolsDialog::setupUi() {
    setWindowTitle(tr("Plugin Developer Tools"));
    setMinimumSize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    tabWidget_ = new QTabWidget(this);
    
    // Editor tab
    auto* editorWidget = new QWidget(this);
    auto* editorLayout = new QVBoxLayout(editorWidget);
    
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* newBtn = new QPushButton(tr("New"), this);
    connect(newBtn, &QPushButton::clicked, this, &PluginDevToolsDialog::onNewPlugin);
    toolbarLayout->addWidget(newBtn);
    
    auto* loadBtn = new QPushButton(tr("Load"), this);
    connect(loadBtn, &QPushButton::clicked, this, &PluginDevToolsDialog::onLoadPlugin);
    toolbarLayout->addWidget(loadBtn);
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    toolbarLayout->addWidget(saveBtn);
    
    toolbarLayout->addStretch();
    
    auto* testBtn = new QPushButton(tr("Test"), this);
    connect(testBtn, &QPushButton::clicked, this, &PluginDevToolsDialog::onTestPlugin);
    toolbarLayout->addWidget(testBtn);
    
    auto* packageBtn = new QPushButton(tr("Package"), this);
    connect(packageBtn, &QPushButton::clicked, this, &PluginDevToolsDialog::onPackagePlugin);
    toolbarLayout->addWidget(packageBtn);
    
    editorLayout->addLayout(toolbarLayout);
    
    codeEdit_ = new QPlainTextEdit(this);
    codeEdit_->setPlaceholderText(tr("// Plugin code here..."));
    editorLayout->addWidget(codeEdit_, 1);
    
    tabWidget_->addTab(editorWidget, tr("Editor"));
    
    // Console tab
    consoleBrowser_ = new QTextBrowser(this);
    consoleBrowser_->setPlainText(tr("Plugin console ready.\n"));
    tabWidget_->addTab(consoleBrowser_, tr("Console"));
    
    // API Reference tab
    apiTree_ = new QTreeWidget(this);
    apiTree_->setHeaderLabel(tr("API Reference"));
    
    auto* root = new QTreeWidgetItem(QStringList(tr("Core API")));
    new QTreeWidgetItem(root, QStringList(tr("registerHook()")));
    new QTreeWidgetItem(root, QStringList(tr("executeHook()")));
    new QTreeWidgetItem(root, QStringList(tr("getConfig()")));
    apiTree_->addTopLevelItem(root);
    
    auto* ui = new QTreeWidgetItem(QStringList(tr("UI API")));
    new QTreeWidgetItem(ui, QStringList(tr("addMenuItem()")));
    new QTreeWidgetItem(ui, QStringList(tr("addToolbarButton()")));
    new QTreeWidgetItem(ui, QStringList(tr("showDialog()")));
    apiTree_->addTopLevelItem(ui);
    
    tabWidget_->addTab(apiTree_, tr("API Reference"));
    
    // Help tab
    helpBrowser_ = new QTextBrowser(this);
    helpBrowser_->setHtml(tr("<h2>Plugin Development Guide</h2>"
        "<p>Plugins extend ScratchRobin functionality.</p>"
        "<h3>Getting Started</h3>"
        "<p>1. Create a new plugin file</p>"
        "<p>2. Define plugin metadata</p>"
        "<p>3. Implement required hooks</p>"
        "<p>4. Test and package</p>"));
    tabWidget_->addTab(helpBrowser_, tr("Help"));
    
    layout->addWidget(tabWidget_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void PluginDevToolsDialog::loadTemplates() {
    // Load plugin templates
}

void PluginDevToolsDialog::onNewPlugin() {
    codeEdit_->setPlainText(tr("// New Plugin\n"
        "{\n"
        "  \"name\": \"My Plugin\",\n"
        "  \"version\": \"1.0.0\",\n"
        "  \"author\": \"Your Name\",\n"
        "  \"main\": \"plugin.js\"\n"
        "}\n"));
}

void PluginDevToolsDialog::onLoadPlugin() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Plugin"),
        QString(), tr("Plugin Files (*.js *.json)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            codeEdit_->setPlainText(file.readAll());
        }
    }
}

void PluginDevToolsDialog::onReloadPlugin() {
    QMessageBox::information(this, tr("Reload"), tr("Plugin reloaded."));
}

void PluginDevToolsDialog::onPackagePlugin() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Package Plugin"),
        tr("myplugin.srplugin"), tr("Plugin Files (*.srplugin)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Packaged"), 
            tr("Plugin packaged successfully.\n%1").arg(fileName));
    }
}

void PluginDevToolsDialog::onValidatePlugin() {
    QMessageBox::information(this, tr("Validation"), tr("Plugin is valid."));
}

void PluginDevToolsDialog::onTestPlugin() {
    consoleBrowser_->append(tr("[Test] Running plugin tests...\n"));
    consoleBrowser_->append(tr("[Test] All tests passed!\n"));
}

void PluginDevToolsDialog::onDebugPlugin() {
    // Start debugging
}

void PluginDevToolsDialog::onPublishPlugin() {
    QMessageBox::information(this, tr("Publish"), 
        tr("Plugin would be submitted to marketplace."));
}

void PluginDevToolsDialog::onTemplateSelected(int index) {
    Q_UNUSED(index)
}

void PluginDevToolsDialog::onGenerateTemplate() {
    QMessageBox::information(this, tr("Template"), tr("Template generated."));
}

// ============================================================================
// Plugin Hook Manager
// ============================================================================
PluginHookManager* PluginHookManager::instance() {
    static PluginHookManager instance;
    return &instance;
}

PluginHookManager::PluginHookManager(QObject* parent)
    : QObject(parent) {
}

PluginHookManager::~PluginHookManager() {
}

void PluginHookManager::registerHook(const QString& name, const PluginHook& hook) {
    hooks_[name] = hook;
    emit hookRegistered(name);
}

void PluginHookManager::unregisterHook(const QString& name) {
    hooks_.remove(name);
    emit hookUnregistered(name);
}

QVariant PluginHookManager::executeHook(const QString& name, const QVariantList& args) {
    Q_UNUSED(args)
    
    if (!hooks_.contains(name)) {
        return QVariant();
    }
    
    // Execute hook logic here
    QVariant result;
    emit hookExecuted(name, result);
    return result;
}

bool PluginHookManager::hasHook(const QString& name) const {
    return hooks_.contains(name);
}

QList<PluginHook> PluginHookManager::getHooks() const {
    return hooks_.values();
}

// ============================================================================
// ApiReferencePanel - Stub implementations
// ============================================================================
ApiReferencePanel::ApiReferencePanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void ApiReferencePanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search API..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &ApiReferencePanel::onSearchTextChanged);
    layout->addWidget(searchEdit_);
    
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("All"));
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) { onCategorySelected(categoryCombo_->itemText(index)); });
    layout->addWidget(categoryCombo_);
    
    apiTree_ = new QTreeWidget(this);
    apiTree_->setHeaderLabel(tr("API"));
    layout->addWidget(apiTree_, 1);
    
    docBrowser_ = new QTextBrowser(this);
    layout->addWidget(docBrowser_, 2);
    
    loadApiDocs();
}

void ApiReferencePanel::loadApiDocs() {
    // Load API documentation
}

void ApiReferencePanel::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
}

void ApiReferencePanel::onCategorySelected(const QString& category) {
    Q_UNUSED(category)
}

void ApiReferencePanel::onItemSelected(const QModelIndex& index) {
    Q_UNUSED(index)
}

// ============================================================================
// PluginManagerPanel - Stub for onInstallPlugin
// ============================================================================
void PluginManagerPanel::onInstallPlugin() {
    // Install from marketplace
    if (currentPlugin_.id.isEmpty()) return;
    
    QMessageBox::information(this, tr("Installing"),
        tr("Installing %1...").arg(currentPlugin_.name));
}

} // namespace scratchrobin::ui
