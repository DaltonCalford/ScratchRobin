#pragma once
#include <QString>
#include <QHash>
#include <QDateTime>
#include <QElapsedTimer>
#include <QObject>
#include <QWidget>
#include <memory>

QT_BEGIN_NAMESPACE
class QFile;
class QTextStream;
class QTableWidget;
class QProgressBar;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace scratchrobin::core {

/**
 * @brief Performance Profiling System
 * 
 * Implements FORM_SPECIFICATION.md Performance section:
 * - Editor performance (10K lines)
 * - Grid performance (10K rows)
 * - Export performance (100K rows in < 30s)
 * - Performance testing framework
 */

// ============================================================================
// Performance Metrics
// ============================================================================
struct PerformanceMetrics {
    QString operation;
    qint64 durationMs = 0;
    qint64 memoryUsedBytes = 0;
    int itemsProcessed = 0;
    double throughput = 0.0;  // items per second
    QDateTime timestamp;
    QString context;
    
    double itemsPerSecond() const {
        if (durationMs > 0) {
            return (itemsProcessed * 1000.0) / durationMs;
        }
        return 0.0;
    }
};

// ============================================================================
// Performance Benchmark
// ============================================================================
struct PerformanceBenchmark {
    QString name;
    QString category;  // editor, grid, export, etc.
    int targetItems = 0;
    qint64 targetDurationMs = 0;
    double minThroughput = 0.0;
    qint64 maxMemoryBytes = 0;
    bool strict = false;  // Fail if target not met?
    
    bool evaluate(const PerformanceMetrics& metrics) const {
        if (metrics.durationMs > targetDurationMs) return false;
        if (minThroughput > 0 && metrics.itemsPerSecond() < minThroughput) return false;
        if (maxMemoryBytes > 0 && metrics.memoryUsedBytes > maxMemoryBytes) return false;
        return true;
    }
};

// ============================================================================
// Performance Profiler
// ============================================================================
class PerformanceProfiler : public QObject {
    Q_OBJECT

public:
    static PerformanceProfiler* instance();
    
    // Profiling session
    void startSession(const QString& name);
    void endSession();
    bool isSessionActive() const { return sessionActive_; }
    
    // Operation timing
    void startOperation(const QString& name, const QString& context = QString());
    void endOperation(const QString& name);
    
    // Scoped timing helper
    class ScopedTimer {
    public:
        ScopedTimer(const QString& name, PerformanceProfiler* profiler = nullptr);
        ~ScopedTimer();
        
    private:
        QString name_;
        PerformanceProfiler* profiler_;
        QElapsedTimer timer_;
    };
    
    // Metrics collection
    void recordMetric(const PerformanceMetrics& metric);
    void recordMemoryUsage(const QString& context);
    
    // Benchmarks
    void addBenchmark(const PerformanceBenchmark& benchmark);
    bool runBenchmark(const QString& name);
    bool runAllBenchmarks();
    
    // Results
    QList<PerformanceMetrics> getMetrics(const QString& operation = QString()) const;
    PerformanceMetrics getLastMetric(const QString& operation) const;
    PerformanceMetrics getAggregate(const QString& operation) const;
    
    // Reporting
    QString generateReport() const;
    void exportToCsv(const QString& fileName) const;
    void exportToJson(const QString& fileName) const;
    
    // Targets verification
    bool verifyTarget(const QString& operation, int items, qint64 maxDurationMs) const;
    bool verifyAllTargets() const;
    
signals:
    void metricRecorded(const PerformanceMetrics& metric);
    void benchmarkStarted(const QString& name);
    void benchmarkFinished(const QString& name, bool passed);
    void performanceWarning(const QString& operation, const QString& message);

private:
    explicit PerformanceProfiler(QObject* parent = nullptr);
    
    qint64 getCurrentMemoryUsage() const;
    
    bool sessionActive_ = false;
    QString sessionName_;
    QDateTime sessionStart_;
    
    QHash<QString, PerformanceBenchmark> benchmarks_;
    QList<PerformanceMetrics> metrics_;
    
    struct ActiveOperation {
        QElapsedTimer timer;
        QString context;
        qint64 startMemory;
    };
    QHash<QString, ActiveOperation> activeOperations_;
};

// ============================================================================
// Performance Test Suite
// ============================================================================
class PerformanceTestSuite : public QObject {
    Q_OBJECT

public:
    explicit PerformanceTestSuite(QObject* parent = nullptr);
    
    // Editor performance tests
    bool testEditorLoad(int lineCount);
    bool testEditorTyping(int characters);
    bool testEditorFormatting(int lineCount);
    bool testEditorSearch(int lineCount, int searchCount);
    bool testEditorFolding(int foldCount);
    
    // Grid performance tests
    bool testGridLoad(int rowCount, int columnCount);
    bool testGridScroll(int rowCount);
    bool testGridSort(int rowCount);
    bool testGridFilter(int rowCount);
    bool testGridSelection(int rowCount);
    
    // Export performance tests
    bool testExportCsv(int rowCount, const QString& fileName);
    bool testExportJson(int rowCount, const QString& fileName);
    bool testExportSql(int rowCount, const QString& fileName);
    
    // Import performance tests
    bool testImportCsv(int rowCount, const QString& fileName);
    
    // Memory tests
    bool testMemoryUsage(const QString& operation, qint64 maxMemoryBytes);
    
    // Run all tests
    struct TestResult {
        QString name;
        bool passed;
        PerformanceMetrics metrics;
        QString errorMessage;
    };
    
    QList<TestResult> runAllTests();
    QString generateReport(const QList<TestResult>& results) const;

private:
    PerformanceProfiler* profiler_;
    
    // Test data generators
    QString generateLargeSqlScript(int lineCount);
    QStringList generateLargeResultSet(int rowCount, int columnCount);
    QByteArray generateLargeCsv(int rowCount, int columnCount);
};

// ============================================================================
// Memory Tracker
// ============================================================================
class MemoryTracker : public QObject {
    Q_OBJECT

public:
    static MemoryTracker* instance();
    
    void startTracking(const QString& context);
    void endTracking();
    
    qint64 currentUsage() const;
    qint64 peakUsage() const;
    qint64 delta() const;
    
    void reset();
    void setPeakThreshold(qint64 bytes);

signals:
    void memoryWarning(qint64 currentBytes, qint64 threshold);
    void peakExceeded(qint64 peakBytes, qint64 threshold);

private:
    explicit MemoryTracker(QObject* parent = nullptr);
    
    qint64 getCurrentUsage() const;
    
    bool tracking_ = false;
    QString context_;
    qint64 startUsage_ = 0;
    qint64 peakUsage_ = 0;
    qint64 threshold_ = 0;
};

// ============================================================================
// Performance Monitor Widget
// ============================================================================
class PerformanceMonitorWidget : public QWidget {
    Q_OBJECT

public:
    explicit PerformanceMonitorWidget(QWidget* parent = nullptr);

    void setProfiler(PerformanceProfiler* profiler);
    void refresh();

private slots:
    void onStartProfiling();
    void onStopProfiling();
    void onExportResults();
    void onClearResults();
    void onRunBenchmarks();

private:
    void setupUi();
    void updateDisplay();
    
    PerformanceProfiler* profiler_ = nullptr;
    
    QTableWidget* metricsTable_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
};

// ============================================================================
// Performance Budget
// ============================================================================
struct PerformanceBudget {
    QString operation;
    qint64 maxDurationMs = 0;
    qint64 maxMemoryBytes = 0;
    double minFps = 0.0;
};

class PerformanceBudgetManager : public QObject {
    Q_OBJECT

public:
    static PerformanceBudgetManager* instance();
    
    void setBudget(const QString& operation, const PerformanceBudget& budget);
    PerformanceBudget getBudget(const QString& operation) const;
    
    bool checkBudget(const QString& operation, const PerformanceMetrics& metrics) const;
    void enforceBudget(const QString& operation);

signals:
    void budgetExceeded(const QString& operation, const PerformanceMetrics& metrics);
    void budgetWarning(const QString& operation, double usagePercent);

private:
    explicit PerformanceBudgetManager(QObject* parent = nullptr);
    
    QHash<QString, PerformanceBudget> budgets_;
};

} // namespace scratchrobin::core
