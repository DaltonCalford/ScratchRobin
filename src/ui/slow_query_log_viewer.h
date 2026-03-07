#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QLineEdit;
class QDateTimeEdit;
class QSplitter;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class QProgressBar;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Slow Query Log Viewer - Analyze slow query logs
 * 
 * View and analyze database slow query logs:
 * - Import slow query logs
 * - Filter by time, duration, user, database
 * - Pattern analysis
 * - Index recommendations
 * - Export reports
 */

// ============================================================================
// Slow Query Entry
// ============================================================================
struct SlowQueryEntry {
    int id = 0;
    QDateTime timestamp;
    QString database;
    QString user;
    QString clientHost;
    double duration = 0; // milliseconds
    double lockTime = 0;
    int rowsSent = 0;
    int rowsExamined = 0;
    QString sql;
    QString normalizedSql;
    QString callStack;
    bool hasIndexSuggestion = false;
};

// ============================================================================
// Slow Query Log Viewer Panel
// ============================================================================
class SlowQueryLogViewerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SlowQueryLogViewerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Slow Query Log"); }
    QString panelCategory() const override { return "performance"; }

public slots:
    // File operations
    void onImportLog();
    void onImportFromServer();
    void onReloadLog();
    void onClearLog();
    void onExportResults();
    
    // Filtering
    void onApplyFilters();
    void onClearFilters();
    void onDurationFilterChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onTimeRangeChanged();
    
    // Analysis
    void onAnalyzeSelected();
    void onAnalyzeAll();
    void onShowPatterns();
    void onShowIndexSuggestions();
    void onShowQueryPlan();
    
    // Selection
    void onQuerySelected(const QModelIndex& index);
    void onQueryDoubleClicked(const QModelIndex& index);
    
    // Summary
    void onRefreshSummary();
    void onGenerateReport();

signals:
    void querySelectedForAnalysis(const SlowQueryEntry& entry);
    void indexSuggestionAvailable(const QString& sql, const QString& suggestion);

private:
    void setupUi();
    void setupToolbar();
    void setupQueryTable();
    void setupDetailsPanel();
    void setupSummaryPanel();
    void loadSlowQueries();
    void applyFilters();
    void analyzeQuery(const SlowQueryEntry& entry);
    QStringList generateIndexSuggestions(const SlowQueryEntry& entry);
    void updateSummary();
    
    backend::SessionClient* client_;
    QList<SlowQueryEntry> allQueries_;
    QList<SlowQueryEntry> filteredQueries_;
    
    // Filters
    double minDuration_ = 100; // ms
    QString searchText_;
    QDateTime fromTime_;
    QDateTime toTime_;
    
    // UI Components
    QTableView* queryTable_ = nullptr;
    QStandardItemModel* queryModel_ = nullptr;
    
    // Details
    QTextEdit* sqlPreview_ = nullptr;
    QTextEdit* analysisResults_ = nullptr;
    QTableView* indexSuggestionsTable_ = nullptr;
    QStandardItemModel* indexSuggestionsModel_ = nullptr;
    
    // Filters UI
    QComboBox* durationFilterCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QDateTimeEdit* fromTimeEdit_ = nullptr;
    QDateTimeEdit* toTimeEdit_ = nullptr;
    QCheckBox* showWithSuggestionsCheck_ = nullptr;
    
    // Summary
    QLabel* totalQueriesLabel_ = nullptr;
    QLabel* slowQueriesLabel_ = nullptr;
    QLabel* avgDurationLabel_ = nullptr;
    QLabel* maxDurationLabel_ = nullptr;
    QLabel* uniquePatternsLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
};

// ============================================================================
// Import Log Dialog
// ============================================================================
class ImportLogDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportLogDialog(QWidget* parent = nullptr);

public slots:
    void onBrowseFile();
    void onImport();
    void onPreview();

private:
    void setupUi();
    void detectLogFormat();
    
    QLineEdit* filePathEdit_ = nullptr;
    QComboBox* logFormatCombo_ = nullptr;
    QSpinBox* minDurationSpin_ = nullptr;
    QCheckBox* parseCommentsCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
};

// ============================================================================
// Query Analysis Dialog
// ============================================================================
class QueryAnalysisDialog : public QDialog {
    Q_OBJECT

public:
    explicit QueryAnalysisDialog(const SlowQueryEntry& entry,
                                backend::SessionClient* client,
                                QWidget* parent = nullptr);

public slots:
    void onExplainQuery();
    void onCreateIndex();
    void onOptimizeQuery();
    void onExportAnalysis();

private:
    void setupUi();
    void analyze();
    
    SlowQueryEntry entry_;
    backend::SessionClient* client_;
    
    QTextEdit* sqlEdit_ = nullptr;
    QTextEdit* analysisEdit_ = nullptr;
    QTableView* suggestionsTable_ = nullptr;
    QStandardItemModel* suggestionsModel_ = nullptr;
    QTextEdit* optimizedSqlEdit_ = nullptr;
};

// ============================================================================
// Pattern Analysis Dialog
// ============================================================================
class PatternAnalysisDialog : public QDialog {
    Q_OBJECT

public:
    explicit PatternAnalysisDialog(const QList<SlowQueryEntry>& entries,
                                  QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExportPatterns();
    void onViewPatternDetails();

private:
    void setupUi();
    void analyzePatterns();
    
    QList<SlowQueryEntry> entries_;
    
    QTableView* patternsTable_ = nullptr;
    QStandardItemModel* patternsModel_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
};

} // namespace scratchrobin::ui
