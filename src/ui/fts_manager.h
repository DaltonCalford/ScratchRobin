#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QListWidget;
class QTabWidget;
class QLabel;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Full-Text Search (FTS) Configuration Manager
 * 
 * Manages FTS configurations, dictionaries, and parsers.
 * - Create/edit FTS configurations
 * - Manage text search dictionaries
 * - Configure text search parsers
 * - Test FTS configurations
 */

// ============================================================================
// FTS Info Structures
// ============================================================================
struct FtsConfigurationInfo {
    QString name;
    QString schema;
    QString parser;
    QMap<QString, QString> tokenMapping;
    bool isDefault = false;
};

struct FtsDictionaryInfo {
    QString name;
    QString schema;
    QString templateName;
    QStringList options;
    QString comment;
};

// ============================================================================
// FTS Manager Panel
// ============================================================================
class FtsManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit FtsManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Full-Text Search Configuration"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateConfiguration();
    void onCreateDictionary();
    void onAlterConfiguration();
    void onDropObject();
    void onTestConfiguration();
    void onFilterChanged(const QString& filter);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModels();
    void loadConfigurations();
    void loadDictionaries();
    
    backend::SessionClient* client_;
    
    QTabWidget* tabWidget_ = nullptr;
    
    QTreeView* configTree_ = nullptr;
    QStandardItemModel* configModel_ = nullptr;
    
    QTreeView* dictionaryTree_ = nullptr;
    QStandardItemModel* dictionaryModel_ = nullptr;
    
    QLineEdit* filterEdit_ = nullptr;
};

// ============================================================================
// Create FTS Configuration Dialog
// ============================================================================
class CreateFtsConfigurationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateFtsConfigurationDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();

private:
    void setupUi();
    void loadParsers();
    void loadCopyFromConfigs();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* parserCombo_ = nullptr;
    QComboBox* copyFromCombo_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Create FTS Dictionary Dialog
// ============================================================================
class CreateFtsDictionaryDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateFtsDictionaryDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();

private:
    void setupUi();
    void loadTemplates();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* templateCombo_ = nullptr;
    QTextEdit* optionsEdit_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Alter FTS Configuration Dialog
// ============================================================================
class AlterFtsConfigurationDialog : public QDialog {
    Q_OBJECT

public:
    explicit AlterFtsConfigurationDialog(backend::SessionClient* client,
                                        const QString& configName,
                                        QWidget* parent = nullptr);

public slots:
    void onAddMapping();
    void onAlterMapping();
    void onDropMapping();
    void onPreview();
    void onApply();

private:
    void setupUi();
    void loadTokenTypes();
    void loadDictionaries();
    QString generateDdl();
    
    backend::SessionClient* client_;
    QString configName_;
    
    QComboBox* tokenTypeCombo_ = nullptr;
    QListWidget* dictionaryList_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Test FTS Configuration Dialog
// ============================================================================
class TestFtsConfigurationDialog : public QDialog {
    Q_OBJECT

public:
    explicit TestFtsConfigurationDialog(backend::SessionClient* client,
                                       const QString& configName = QString(),
                                       QWidget* parent = nullptr);

public slots:
    void onTest();
    void onTestToTsvector();
    void onTestToTsquery();

private:
    void setupUi();
    void loadConfigurations();
    
    backend::SessionClient* client_;
    
    QComboBox* configCombo_ = nullptr;
    QTextEdit* inputEdit_ = nullptr;
    QTextEdit* resultEdit_ = nullptr;
};

} // namespace scratchrobin::ui
