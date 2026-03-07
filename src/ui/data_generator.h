#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QTreeView;
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
class QSpinBox;
class QListWidget;
class QProgressBar;
class QStackedWidget;
class QTableWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Data Generator - Test data generation tool
 * 
 * Generate realistic test data for database tables:
 * - Multiple data types (names, addresses, emails, numbers, dates)
 * - Referential integrity support
 * - Batch generation with progress
 * - Custom formulas and expressions
 * - Export to various formats
 */

// ============================================================================
// Generator Types
// ============================================================================
enum class GeneratorType {
    FirstName,
    LastName,
    FullName,
    Email,
    Phone,
    Address,
    City,
    Country,
    PostalCode,
    Company,
    JobTitle,
    Integer,
    Float,
    Decimal,
    Boolean,
    Date,
    DateTime,
    Time,
    UUID,
    LoremIpsum,
    CustomList,
    Sequence,
    Reference, // Foreign key reference
    Formula,
    RandomPick,
    Regex
};

struct ColumnGenerator {
    QString columnName;
    QString dataType;
    GeneratorType generatorType;
    QVariant minValue;
    QVariant maxValue;
    int precision = 2;
    QString format;
    QStringList customValues;
    QString formula;
    QString regexPattern;
    int nullPercent = 0;
    bool unique = false;
    QString referenceTable;
    QString referenceColumn;
};

struct DataGenerationTask {
    QString id;
    QString tableName;
    QString schema;
    int rowCount = 1000;
    bool truncateFirst = false;
    bool respectForeignKeys = true;
    bool generateInTransaction = true;
    int batchSize = 1000;
    QList<ColumnGenerator> columns;
    QString outputFile; // For export instead of insert
    QString outputFormat; // SQL, CSV, JSON, etc.
};

// ============================================================================
// Data Generator Panel
// ============================================================================
class DataGeneratorPanel : public DockPanel {
    Q_OBJECT

public:
    explicit DataGeneratorPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Data Generator"); }
    QString panelCategory() const override { return "tools"; }

public slots:
    // Table selection
    void onSelectTable();
    void onTableSelected(const QModelIndex& index);
    void onRefreshTables();
    
    // Generation
    void onConfigureGenerators();
    void onPreviewData();
    void onGenerateData();
    void onStopGeneration();
    void onExportTemplate();
    
    // Presets
    void onSavePreset();
    void onLoadPreset();
    void onDeletePreset();
    void onPresetSelected(int index);
    
    // Batch operations
    void onGenerateForMultipleTables();
    void onClearGeneratedData();

signals:
    void generationStarted(const QString& taskId);
    void generationProgress(const QString& taskId, int percent);
    void generationCompleted(const QString& taskId, int rowsGenerated);

private:
    void setupUi();
    void setupTableList();
    void setupColumnConfig();
    void loadTables();
    void loadColumns(const QString& tableName);
    void updatePreview();
    
    backend::SessionClient* client_;
    DataGenerationTask currentTask_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QTreeView* tableTree_ = nullptr;
    QStandardItemModel* tableModel_ = nullptr;
    QTableWidget* columnTable_ = nullptr;
    
    // Configuration
    QSpinBox* rowCountSpin_ = nullptr;
    QCheckBox* truncateCheck_ = nullptr;
    QCheckBox* respectFKCheck_ = nullptr;
    QCheckBox* transactionCheck_ = nullptr;
    QSpinBox* batchSizeSpin_ = nullptr;
    
    // Preview
    QTableView* previewTable_ = nullptr;
    QStandardItemModel* previewModel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    
    // Presets
    QComboBox* presetCombo_ = nullptr;
};

// ============================================================================
// Column Generator Dialog
// ============================================================================
class ColumnGeneratorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColumnGeneratorDialog(ColumnGenerator* generator, const QStringList& availableTables, QWidget* parent = nullptr);

public slots:
    void onGeneratorTypeChanged(int index);
    void onPreview();
    void onSave();
    void onCancel();

private:
    void setupUi();
    void setupGeneratorSpecificOptions();
    void updatePreview();
    
    ColumnGenerator* generator_;
    QStringList availableTables_;
    
    QStackedWidget* optionsStack_ = nullptr;
    
    // Common options
    QComboBox* typeCombo_ = nullptr;
    QSpinBox* nullPercentSpin_ = nullptr;
    QCheckBox* uniqueCheck_ = nullptr;
    
    // Range options
    QLineEdit* minEdit_ = nullptr;
    QLineEdit* maxEdit_ = nullptr;
    QSpinBox* precisionSpin_ = nullptr;
    
    // List options
    QListWidget* valuesList_ = nullptr;
    QTextEdit* valuesEdit_ = nullptr;
    
    // Reference options
    QComboBox* refTableCombo_ = nullptr;
    QComboBox* refColumnCombo_ = nullptr;
    
    // Formula/Regex options
    QTextEdit* formulaEdit_ = nullptr;
    QLineEdit* regexEdit_ = nullptr;
    
    // Preview
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Preview Data Dialog
// ============================================================================
class PreviewDataDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreviewDataDialog(const DataGenerationTask& task, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExport();

private:
    void setupUi();
    void generatePreview();
    
    DataGenerationTask task_;
    
    QTableView* previewTable_ = nullptr;
    QStandardItemModel* previewModel_ = nullptr;
    QSpinBox* previewCountSpin_ = nullptr;
};

// ============================================================================
// Generation Progress Dialog
// ============================================================================
class GenerationProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit GenerationProgressDialog(const DataGenerationTask& task, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onStart();
    void onStop();
    void onPause();
    void onClose();
    void onProgressUpdate(int current, int total);

signals:
    void generationStopped();
    void generationPaused();

private:
    void setupUi();
    void executeGeneration();
    
    DataGenerationTask task_;
    backend::SessionClient* client_;
    bool running_ = false;
    bool paused_ = false;
    
    QProgressBar* progressBar_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* etaLabel_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    
    int generatedCount_ = 0;
    QDateTime startTime_;
};

// ============================================================================
// Multi-Table Generator Dialog
// ============================================================================
class MultiTableGeneratorDialog : public QDialog {
    Q_OBJECT

public:
    explicit MultiTableGeneratorDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onAddTable();
    void onRemoveTable();
    void onTableSelectionChanged();
    void onConfigureTable();
    void onGenerateAll();

private:
    void setupUi();
    void loadTables();
    
    backend::SessionClient* client_;
    QList<DataGenerationTask> tasks_;
    
    QListWidget* tableList_ = nullptr;
    QTableWidget* configTable_ = nullptr;
    QTextEdit* summaryEdit_ = nullptr;
};

} // namespace scratchrobin::ui
