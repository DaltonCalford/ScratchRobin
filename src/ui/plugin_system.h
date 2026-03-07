#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QPluginLoader>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QSplitter;
class QTreeView;
class QTreeWidget;
class QListWidget;
class QStackedWidget;
class QTextBrowser;
class QPlainTextEdit;
class QDialogButtonBox;
class QProgressBar;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Plugin System - Extension architecture
 * 
 * Extend application functionality through plugins:
 * - Plugin manager with install/uninstall
 * - Plugin marketplace/browser
 * - Plugin settings and permissions
 * - Plugin API for developers
 * - Hot-reload during development
 */

// ============================================================================
// Plugin Types and Structures
// ============================================================================
enum class PluginType {
    UIExtension,        // Adds UI components
    DatabaseDriver,     // Database connectivity
    Tool,               // Standalone tool
    Theme,              // Visual theme
    LanguageSupport,    // SQL dialect support
    ImportExport,       // Data format support
    Integration         // External tool integration
};

enum class PluginStatus {
    NotInstalled,
    Installed,
    Enabled,
    Disabled,
    Error,
    UpdateAvailable
};

// ============================================================================
// Plugin Metadata
// ============================================================================
struct PluginInfo {
    QString id;
    QString name;
    QString description;
    QString version;
    QString author;
    QString authorEmail;
    QString website;
    QString icon;
    PluginType type;
    PluginStatus status;
    QStringList dependencies;
    QStringList permissions;
    QStringList supportedFormats;
    QDateTime installedAt;
    QDateTime updatedAt;
    qint64 downloadCount = 0;
    double rating = 0.0;
    int reviewCount = 0;
    QString localPath;
    QString repository;
    bool isBuiltin = false;
    bool isOfficial = false;
    QString minimumAppVersion;
    QString maximumAppVersion;
    
    bool operator==(const PluginInfo& other) const {
        return id == other.id;
    }
};

struct PluginPermission {
    QString id;
    QString name;
    QString description;
    bool granted = false;
    bool required = false;
};

struct PluginHook {
    QString name;
    QString description;
    QStringList arguments;
    QString returnType;
};

// ============================================================================
// Plugin Manager Panel
// ============================================================================
class PluginManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit PluginManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Plugin Manager"); }
    QString panelCategory() const override { return "system"; }

public slots:
    // Plugin actions
    void onPluginSelected(const QModelIndex& index);
    void onInstallPlugin();
    void onUninstallPlugin();
    void onEnablePlugin();
    void onDisablePlugin();
    void onConfigurePlugin();
    void onUpdatePlugin();
    void onReloadPlugin();
    
    // Navigation
    void onFilterByType(int type);
    void onFilterByStatus(int status);
    void onSearchTextChanged(const QString& text);
    void onCategoryChanged(int index);
    
    // Marketplace
    void onBrowseMarketplace();
    void onCheckUpdates();
    void onInstallFromFile();
    void onExportPlugin();

signals:
    void pluginEnabled(const QString& pluginId);
    void pluginDisabled(const QString& pluginId);
    void pluginInstalled(const PluginInfo& info);
    void pluginUninstalled(const QString& pluginId);

private:
    void setupUi();
    void setupPluginList();
    void setupDetailsPanel();
    void setupDeveloperPanel();
    void loadPlugins();
    void updatePluginList();
    void updatePluginDetails(const PluginInfo& info);
    void filterPlugins();
    
    backend::SessionClient* client_;
    QList<PluginInfo> plugins_;
    PluginInfo currentPlugin_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    
    // Installed plugins tab
    QTableView* pluginsTable_ = nullptr;
    QStandardItemModel* pluginsModel_ = nullptr;
    QComboBox* typeFilterCombo_ = nullptr;
    QComboBox* statusFilterCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    // Plugin details
    QStackedWidget* detailsStack_ = nullptr;
    QLabel* pluginNameLabel_ = nullptr;
    QLabel* pluginVersionLabel_ = nullptr;
    QLabel* pluginAuthorLabel_ = nullptr;
    QTextBrowser* pluginDescriptionBrowser_ = nullptr;
    QListWidget* permissionsList_ = nullptr;
    QListWidget* dependenciesList_ = nullptr;
    QLabel* pluginStatusLabel_ = nullptr;
    
    // Actions
    QPushButton* enableBtn_ = nullptr;
    QPushButton* disableBtn_ = nullptr;
    QPushButton* configureBtn_ = nullptr;
    QPushButton* uninstallBtn_ = nullptr;
    QPushButton* updateBtn_ = nullptr;
};

// ============================================================================
// Plugin Marketplace Dialog
// ============================================================================
class PluginMarketplaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginMarketplaceDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onCategorySelected(const QString& category);
    void onPluginSelected(const QModelIndex& index);
    void onSearchTextChanged(const QString& text);
    void onSortChanged(int index);
    void onFilterChanged();
    void onInstallPlugin();
    void onPreviewPlugin();
    void onViewReviews();
    void onCheckForUpdates();
    void onRefreshList();

private:
    void setupUi();
    void loadMarketplace();
    void updatePluginList();
    void updatePluginDetails(const PluginInfo& info);
    
    backend::SessionClient* client_;
    QList<PluginInfo> marketplacePlugins_;
    PluginInfo currentPlugin_;
    
    QTreeWidget* categoryTree_ = nullptr;
    QTableView* pluginsTable_ = nullptr;
    QStandardItemModel* pluginsModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* sortCombo_ = nullptr;
    QComboBox* filterCombo_ = nullptr;
    
    // Details panel
    QLabel* nameLabel_ = nullptr;
    QLabel* authorLabel_ = nullptr;
    QLabel* ratingLabel_ = nullptr;
    QLabel* downloadsLabel_ = nullptr;
    QTextBrowser* descriptionBrowser_ = nullptr;
    QPushButton* installBtn_ = nullptr;
};

// ============================================================================
// Plugin Configuration Dialog
// ============================================================================
class PluginConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginConfigDialog(const PluginInfo& info, backend::SessionClient* client, 
                                QWidget* parent = nullptr);

public slots:
    void onSettingChanged(const QString& key, const QVariant& value);
    void onResetDefaults();
    void onImportSettings();
    void onExportSettings();
    void onValidate();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    
    PluginInfo pluginInfo_;
    backend::SessionClient* client_;
    QHash<QString, QVariant> settings_;
    
    QTabWidget* tabWidget_ = nullptr;
    QWidget* generalTab_ = nullptr;
    QWidget* advancedTab_ = nullptr;
    QWidget* permissionsTab_ = nullptr;
    QDialogButtonBox* buttonBox_ = nullptr;
};

// ============================================================================
// Install Plugin Dialog
// ============================================================================
class InstallPluginDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstallPluginDialog(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString selectedPluginPath() const;

public slots:
    void onBrowseFile();
    void onInstallFromUrl();
    void onValidatePlugin();
    void onAcceptPermissions();
    void onInstall();

private:
    void setupUi();
    void validatePluginFile(const QString& path);
    void showPermissions(const QList<PluginPermission>& permissions);
    
    backend::SessionClient* client_;
    QString pluginPath_;
    PluginInfo pluginInfo_;
    
    QStackedWidget* stack_ = nullptr;
    QLineEdit* pathEdit_ = nullptr;
    QLineEdit* urlEdit_ = nullptr;
    QTextBrowser* infoBrowser_ = nullptr;
    QListWidget* permissionsList_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPushButton* installBtn_ = nullptr;
};

// ============================================================================
// Plugin Developer Tools Dialog
// ============================================================================
class PluginDevToolsDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginDevToolsDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onNewPlugin();
    void onLoadPlugin();
    void onReloadPlugin();
    void onPackagePlugin();
    void onValidatePlugin();
    void onTestPlugin();
    void onDebugPlugin();
    void onPublishPlugin();
    
    // Template selection
    void onTemplateSelected(int index);
    void onGenerateTemplate();

private:
    void setupUi();
    void loadTemplates();
    void updateApiReference();
    
    backend::SessionClient* client_;
    
    QTabWidget* tabWidget_ = nullptr;
    QPlainTextEdit* codeEdit_ = nullptr;
    QTextBrowser* consoleBrowser_ = nullptr;
    QTreeWidget* apiTree_ = nullptr;
    QTextBrowser* helpBrowser_ = nullptr;
    QComboBox* templateCombo_ = nullptr;
    QLineEdit* pluginNameEdit_ = nullptr;
};

// ============================================================================
// API Reference Panel
// ============================================================================
class ApiReferencePanel : public QWidget {
    Q_OBJECT

public:
    explicit ApiReferencePanel(QWidget* parent = nullptr);

public slots:
    void onSearchTextChanged(const QString& text);
    void onCategorySelected(const QString& category);
    void onItemSelected(const QModelIndex& index);

private:
    void setupUi();
    void loadApiDocs();
    
    QTreeWidget* apiTree_ = nullptr;
    QTextBrowser* docBrowser_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
};

// ============================================================================
// Plugin Hook Manager
// ============================================================================
class PluginHookManager : public QObject {
    Q_OBJECT

public:
    static PluginHookManager* instance();
    
    void registerHook(const QString& name, const PluginHook& hook);
    void unregisterHook(const QString& name);
    
    QVariant executeHook(const QString& name, const QVariantList& args);
    bool hasHook(const QString& name) const;
    
    QList<PluginHook> getHooks() const;

signals:
    void hookExecuted(const QString& name, const QVariant& result);
    void hookRegistered(const QString& name);
    void hookUnregistered(const QString& name);

private:
    PluginHookManager(QObject* parent = nullptr);
    ~PluginHookManager();
    
    QHash<QString, PluginHook> hooks_;
};

} // namespace scratchrobin::ui
