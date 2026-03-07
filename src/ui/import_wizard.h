#pragma once
#include <QWizard>
#include <QHash>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QComboBox;
class QTableView;
class QStandardItemModel;
class QCheckBox;
class QSpinBox;
class QTextEdit;
class QProgressBar;
class QLabel;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::ui {

class CsvParser;

/**
 * @brief Import Data Wizard (4 steps)
 * 
 * Implements FORM_SPECIFICATION.md Import Data Wizard section:
 * - Step 1: Source (file, format, encoding)
 * - Step 2: Preview & Column Mapping
 * - Step 3: Options (delimiter, null handling, target)
 * - Step 4: Execution with progress
 */
class ImportDataWizard : public QWizard {
    Q_OBJECT

public:
    explicit ImportDataWizard(QWidget* parent = nullptr);
    ~ImportDataWizard() override;

    // Data for wizard pages
    struct ImportConfig {
        QString sourceFile;
        QString format;
        QString encoding;
        QString delimiter;
        QString quoteChar;
        bool hasHeader;
        QString targetTable;
        bool createNewTable;
        bool truncateTable;
        int skipLines;
        QString nullRepresentation;
        bool trimWhitespace;
    };

    ImportConfig config() const;
    void setConfig(const ImportConfig& config);

signals:
    void importStarted(const QString& fileName);
    void importProgress(int current, int total);
    void importFinished(int rowsImported, int errors);

private:
    void setupPages();
};

// ============================================================================
// Step 1: Source Selection
// ============================================================================
class ImportSourcePage : public QWizardPage {
    Q_OBJECT

public:
    explicit ImportSourcePage(QWidget* parent = nullptr);

    bool isComplete() const override;
    int nextId() const override;

private slots:
    void onBrowseFile();
    void onFileSelected();
    void detectFileFormat();

private:
    void setupUi();

    QLineEdit* fileEdit_ = nullptr;
    QPushButton* browseBtn_ = nullptr;
    QComboBox* formatCombo_ = nullptr;
    QComboBox* encodingCombo_ = nullptr;
    QLabel* infoLabel_ = nullptr;
};

// ============================================================================
// Step 2: Preview & Column Mapping
// ============================================================================
class ImportPreviewPage : public QWizardPage {
    Q_OBJECT

public:
    explicit ImportPreviewPage(QWidget* parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;
    int nextId() const override;

    QStringList columnNames() const;
    QList<int> selectedColumnIndices() const;

private slots:
    void onAutoDetectTypes();
    void onColumnSelectionChanged();

private:
    void setupUi();
    void loadPreview();
    void detectColumnTypes();

    QTableView* previewTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QPushButton* autoDetectBtn_ = nullptr;
    QLabel* infoLabel_ = nullptr;
    
    // Column mapping data
    QStringList detectedTypes_;
    QHash<int, QString> columnMappings_;
};

// ============================================================================
// Step 3: Options
// ============================================================================
class ImportOptionsPage : public QWizardPage {
    Q_OBJECT

public:
    explicit ImportOptionsPage(QWidget* parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;
    int nextId() const override;

private slots:
    void onTargetTypeChanged();
    void updateOptionsState();

private:
    void setupUi();

    // CSV options
    QGroupBox* csvGroup_ = nullptr;
    QComboBox* delimiterCombo_ = nullptr;
    QComboBox* quoteCombo_ = nullptr;
    QCheckBox* headerCheck_ = nullptr;
    QSpinBox* skipLinesSpin_ = nullptr;
    QCheckBox* trimCheck_ = nullptr;
    
    // Target options
    QGroupBox* targetGroup_ = nullptr;
    QComboBox* targetTypeCombo_ = nullptr;
    QComboBox* existingTableCombo_ = nullptr;
    QLineEdit* newTableEdit_ = nullptr;
    QCheckBox* truncateCheck_ = nullptr;
    
    // Error handling
    QComboBox* errorHandlingCombo_ = nullptr;
};

// ============================================================================
// Step 4: Execution
// ============================================================================
class ImportExecutionPage : public QWizardPage {
    Q_OBJECT

public:
    explicit ImportExecutionPage(QWidget* parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;
    int nextId() const override { return -1; }

private slots:
    void startImport();
    void cancelImport();
    void onImportProgress(int current, int total);
    void onImportFinished(int rowsImported, int errors);

private:
    void setupUi();
    void updateSummary();

    QLabel* summaryLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    QLabel* resultLabel_ = nullptr;
    
    bool importRunning_ = false;
    bool importComplete_ = false;
};

// ============================================================================
// CSV Parser
// ============================================================================
class CsvParser : public QObject {
    Q_OBJECT

public:
    struct ParseOptions {
        QString delimiter = ",";
        QString quoteChar = "\"";
        QString escapeChar = "\"";
        bool hasHeader = true;
        int skipLines = 0;
        QString encoding = "UTF-8";
    };

    explicit CsvParser(QObject* parent = nullptr);

    bool loadFile(const QString& fileName, const ParseOptions& options);
    
    QStringList headers() const { return headers_; }
    QList<QStringList> previewRows(int maxRows = 10) const;
    QList<QStringList> allRows() const { return data_; }
    
    int rowCount() const { return data_.size(); }
    int columnCount() const { return headers_.size(); }

signals:
    void parseError(const QString& error);
    void parseWarning(const QString& warning);

private:
    QStringList parseLine(const QString& line);
    QStringList splitCsvLine(const QString& line);

    QStringList headers_;
    QList<QStringList> data_;
    ParseOptions options_;
};

} // namespace scratchrobin::ui
