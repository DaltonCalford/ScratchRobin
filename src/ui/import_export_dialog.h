#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QStandardPaths>

namespace scratchrobin {

struct ExportOptions {
    QString format = "SQL";
    QString filePath;
    QStringList selectedTables;
    QStringList selectedSchemas;
    bool includeData = false;
    bool includeDropStatements = false;
    bool includeCreateStatements = true;
    bool includeIndexes = true;
    bool includeConstraints = true;
    bool includeTriggers = true;
    bool includeViews = true;
    bool includeSequences = true;
    QString encoding = "UTF-8";
    bool compressOutput = false;
};

struct ImportOptions {
    QString format = "SQL";
    QString filePath;
    bool ignoreErrors = false;
    bool continueOnError = false;
    bool dropExistingObjects = false;
    bool createSchemas = true;
    bool createTables = true;
    bool createIndexes = true;
    bool createConstraints = true;
    bool createTriggers = true;
    bool createViews = true;
    QString encoding = "UTF-8";
    bool previewOnly = false;
};

class ImportExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportExportDialog(QWidget* parent = nullptr);
    ~ImportExportDialog() override = default;

    void setCurrentDatabase(const QString& databaseName);
    void setAvailableTables(const QStringList& tables);
    void setAvailableSchemas(const QStringList& schemas);

signals:
    void exportRequested(const ExportOptions& options);
    void importRequested(const ImportOptions& options);
    void previewRequested(const QString& filePath);

private slots:
    void onExportFormatChanged(const QString& format);
    void onImportFormatChanged(const QString& format);
    void onBrowseExportFile();
    void onBrowseImportFile();
    void onSelectAllTables();
    void onSelectAllSchemas();
    void onClearSelection();
    void onExportClicked();
    void onImportClicked();
    void onPreviewClicked();
    void onTabChanged(int index);

private:
    void setupUI();
    void setupExportTab();
    void setupImportTab();
    void setupSettingsTab();
    void updateExportFileExtension();
    void updateImportFileExtension();
    void loadSettings();
    void saveSettings();

    // UI Components
    QTabWidget* tabWidget_;

    // Export tab
    QWidget* exportTab_;
    QComboBox* exportFormatCombo_;
    QLineEdit* exportFilePathEdit_;
    QPushButton* exportBrowseButton_;
    QListWidget* exportTablesList_;
    QListWidget* exportSchemasList_;
    QCheckBox* exportIncludeDataCheck_;
    QCheckBox* exportIncludeDropCheck_;
    QCheckBox* exportIncludeCreateCheck_;
    QCheckBox* exportIncludeIndexesCheck_;
    QCheckBox* exportIncludeConstraintsCheck_;
    QCheckBox* exportIncludeTriggersCheck_;
    QCheckBox* exportIncludeViewsCheck_;
    QCheckBox* exportIncludeSequencesCheck_;
    QComboBox* exportEncodingCombo_;
    QCheckBox* exportCompressCheck_;
    QPushButton* exportSelectAllTablesButton_;
    QPushButton* exportClearSelectionButton_;
    QPushButton* exportButton_;

    // Import tab
    QWidget* importTab_;
    QComboBox* importFormatCombo_;
    QLineEdit* importFilePathEdit_;
    QPushButton* importBrowseButton_;
    QCheckBox* importIgnoreErrorsCheck_;
    QCheckBox* importContinueOnErrorCheck_;
    QCheckBox* importDropExistingCheck_;
    QCheckBox* importCreateSchemasCheck_;
    QCheckBox* importCreateTablesCheck_;
    QCheckBox* importCreateIndexesCheck_;
    QCheckBox* importCreateConstraintsCheck_;
    QCheckBox* importCreateTriggersCheck_;
    QCheckBox* importCreateViewsCheck_;
    QComboBox* importEncodingCombo_;
    QCheckBox* importPreviewOnlyCheck_;
    QTextEdit* importPreviewText_;
    QPushButton* importPreviewButton_;
    QPushButton* importButton_;

    // Settings tab
    QWidget* settingsTab_;
    QLineEdit* defaultExportDirEdit_;
    QLineEdit* defaultImportDirEdit_;
    QPushButton* defaultExportBrowseButton_;
    QPushButton* defaultImportBrowseButton_;
    QCheckBox* autoCompressCheck_;
    QCheckBox* showProgressCheck_;
    QSpinBox* previewLineLimitSpin_;

    // Common components
    QLabel* databaseLabel_;
    QProgressBar* progressBar_;
    QPushButton* closeButton_;

    // State
    QString currentDatabase_;
    QStringList availableTables_;
    QStringList availableSchemas_;

    QStringList exportFormats_ = {"SQL", "CSV", "JSON", "XML", "YAML"};
    QStringList importFormats_ = {"SQL", "CSV", "JSON", "XML", "YAML"};
    QStringList encodings_ = {"UTF-8", "UTF-16", "ISO-8859-1", "CP1252", "ASCII"};
};

} // namespace scratchrobin
