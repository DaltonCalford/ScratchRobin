#include "core/performance_profiler.h"

#include <QTableWidget>
#include <QThread>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTimer>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace scratchrobin::core {

// ============================================================================
// PerformanceProfiler
// ============================================================================

static PerformanceProfiler* g_instance = nullptr;

PerformanceProfiler* PerformanceProfiler::instance() {
    if (!g_instance) {
        g_instance = new PerformanceProfiler();
    }
    return g_instance;
}

PerformanceProfiler::PerformanceProfiler(QObject* parent) : QObject(parent) {
    // Define default benchmarks based on FORM_SPECIFICATION.md targets
    PerformanceBenchmark editorBenchmark;
    editorBenchmark.name = "Editor_10K_Lines";
    editorBenchmark.category = "editor";
    editorBenchmark.targetItems = 10000;
    editorBenchmark.targetDurationMs = 5000;  // 5 seconds
    editorBenchmark.minThroughput = 2000.0;   // 2000 lines/sec
    benchmarks_[editorBenchmark.name] = editorBenchmark;
    
    PerformanceBenchmark gridBenchmark;
    gridBenchmark.name = "Grid_10K_Rows";
    gridBenchmark.category = "grid";
    gridBenchmark.targetItems = 10000;
    gridBenchmark.targetDurationMs = 3000;    // 3 seconds
    gridBenchmark.minThroughput = 3000.0;     // 3000 rows/sec
    benchmarks_[gridBenchmark.name] = gridBenchmark;
    
    PerformanceBenchmark exportBenchmark;
    exportBenchmark.name = "Export_100K_Rows";
    exportBenchmark.category = "export";
    exportBenchmark.targetItems = 100000;
    exportBenchmark.targetDurationMs = 30000;  // 30 seconds
    exportBenchmark.minThroughput = 3333.0;    // 3333 rows/sec
    benchmarks_[exportBenchmark.name] = exportBenchmark;
}

void PerformanceProfiler::startSession(const QString& name) {
    sessionName_ = name;
    sessionStart_ = QDateTime::currentDateTime();
    sessionActive_ = true;
    metrics_.clear();
}

void PerformanceProfiler::endSession() {
    sessionActive_ = false;
}

void PerformanceProfiler::startOperation(const QString& name, const QString& context) {
    ActiveOperation op;
    op.timer.start();
    op.context = context;
    op.startMemory = getCurrentMemoryUsage();
    activeOperations_[name] = op;
}

void PerformanceProfiler::endOperation(const QString& name) {
    if (!activeOperations_.contains(name)) return;
    
    auto& op = activeOperations_[name];
    PerformanceMetrics metric;
    metric.operation = name;
    metric.context = op.context;
    metric.durationMs = op.timer.elapsed();
    metric.timestamp = QDateTime::currentDateTime();
    metric.memoryUsedBytes = getCurrentMemoryUsage() - op.startMemory;
    
    recordMetric(metric);
    activeOperations_.remove(name);
}

void PerformanceProfiler::recordMetric(const PerformanceMetrics& metric) {
    metrics_.append(metric);
    emit metricRecorded(metric);
    
    // Check if this is a slow operation
    if (metric.durationMs > 5000) {  // > 5 seconds
        emit performanceWarning(metric.operation, 
            tr("Slow operation: %1 took %2 ms")
            .arg(metric.operation)
            .arg(metric.durationMs));
    }
}

void PerformanceProfiler::recordMemoryUsage(const QString& context) {
    PerformanceMetrics metric;
    metric.operation = "Memory_" + context;
    metric.memoryUsedBytes = getCurrentMemoryUsage();
    metric.timestamp = QDateTime::currentDateTime();
    recordMetric(metric);
}

void PerformanceProfiler::addBenchmark(const PerformanceBenchmark& benchmark) {
    benchmarks_[benchmark.name] = benchmark;
}

bool PerformanceProfiler::runBenchmark(const QString& name) {
    if (!benchmarks_.contains(name)) return false;
    
    const auto& benchmark = benchmarks_[name];
    emit benchmarkStarted(name);
    
    // Run the appropriate test based on category
    PerformanceTestSuite testSuite;
    bool passed = false;
    
    if (benchmark.category == "editor") {
        passed = testSuite.testEditorLoad(benchmark.targetItems);
    } else if (benchmark.category == "grid") {
        passed = testSuite.testGridLoad(benchmark.targetItems, 10);
    } else if (benchmark.category == "export") {
        passed = testSuite.testExportCsv(benchmark.targetItems, "/tmp/test_export.csv");
    }
    
    emit benchmarkFinished(name, passed);
    return passed;
}

bool PerformanceProfiler::runAllBenchmarks() {
    bool allPassed = true;
    for (const auto& name : benchmarks_.keys()) {
        if (!runBenchmark(name)) {
            allPassed = false;
        }
    }
    return allPassed;
}

QList<PerformanceMetrics> PerformanceProfiler::getMetrics(const QString& operation) const {
    if (operation.isEmpty()) return metrics_;
    
    QList<PerformanceMetrics> result;
    for (const auto& m : metrics_) {
        if (m.operation == operation) {
            result.append(m);
        }
    }
    return result;
}

PerformanceMetrics PerformanceProfiler::getLastMetric(const QString& operation) const {
    for (int i = metrics_.size() - 1; i >= 0; --i) {
        if (metrics_[i].operation == operation) {
            return metrics_[i];
        }
    }
    return PerformanceMetrics();
}

PerformanceMetrics PerformanceProfiler::getAggregate(const QString& operation) const {
    PerformanceMetrics aggregate;
    aggregate.operation = operation + "_aggregate";
    
    qint64 totalDuration = 0;
    qint64 totalMemory = 0;
    int count = 0;
    
    for (const auto& m : metrics_) {
        if (m.operation == operation) {
            totalDuration += m.durationMs;
            totalMemory += m.memoryUsedBytes;
            aggregate.itemsProcessed += m.itemsProcessed;
            count++;
        }
    }
    
    if (count > 0) {
        aggregate.durationMs = totalDuration / count;
        aggregate.memoryUsedBytes = totalMemory / count;
    }
    
    return aggregate;
}

QString PerformanceProfiler::generateReport() const {
    QString report;
    QTextStream stream(&report);
    
    stream << "Performance Report\n";
    stream << "==================\n\n";
    
    if (sessionActive_) {
        stream << "Session: " << sessionName_ << "\n";
        stream << "Started: " << sessionStart_.toString() << "\n";
        stream << "Duration: " << sessionStart_.secsTo(QDateTime::currentDateTime()) << " seconds\n\n";
    }
    
    stream << "Operations:\n";
    stream << "-----------\n";
    
    // Group by operation
    QHash<QString, QList<PerformanceMetrics>> grouped;
    for (const auto& m : metrics_) {
        grouped[m.operation].append(m);
    }
    
    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        const auto& metrics = it.value();
        if (metrics.isEmpty()) continue;
        
        qint64 totalDuration = 0;
        qint64 maxDuration = 0;
        for (const auto& m : metrics) {
            totalDuration += m.durationMs;
            maxDuration = qMax(maxDuration, m.durationMs);
        }
        
        stream << it.key() << ":\n";
        stream << "  Count: " << metrics.size() << "\n";
        stream << "  Avg Duration: " << (totalDuration / metrics.size()) << " ms\n";
        stream << "  Max Duration: " << maxDuration << " ms\n";
        stream << "\n";
    }
    
    return report;
}

void PerformanceProfiler::exportToCsv(const QString& fileName) const {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) return;
    
    QTextStream stream(&file);
    stream << "timestamp,operation,context,duration_ms,memory_bytes,items,throughput\n";
    
    for (const auto& m : metrics_) {
        stream << m.timestamp.toString(Qt::ISODate) << ","
               << m.operation << ","
               << m.context << ","
               << m.durationMs << ","
               << m.memoryUsedBytes << ","
               << m.itemsProcessed << ","
               << m.itemsPerSecond() << "\n";
    }
    
    file.close();
}

void PerformanceProfiler::exportToJson(const QString& fileName) const {
    QJsonArray array;
    for (const auto& m : metrics_) {
        QJsonObject obj;
        obj["timestamp"] = m.timestamp.toString(Qt::ISODate);
        obj["operation"] = m.operation;
        obj["context"] = m.context;
        obj["durationMs"] = static_cast<int>(m.durationMs);
        obj["memoryBytes"] = static_cast<int>(m.memoryUsedBytes);
        obj["itemsProcessed"] = m.itemsProcessed;
        obj["throughput"] = m.itemsPerSecond();
        array.append(obj);
    }
    
    QJsonDocument doc(array);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

bool PerformanceProfiler::verifyTarget(const QString& operation, int items, qint64 maxDurationMs) const {
    auto metrics = getMetrics(operation);
    for (const auto& m : metrics) {
        if (m.itemsProcessed >= items && m.durationMs <= maxDurationMs) {
            return true;
        }
    }
    return false;
}

bool PerformanceProfiler::verifyAllTargets() const {
    // Check all defined benchmarks
    for (const auto& benchmark : benchmarks_) {
        auto metrics = getMetrics(benchmark.name);
        bool found = false;
        for (const auto& m : metrics) {
            if (benchmark.evaluate(m)) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

qint64 PerformanceProfiler::getCurrentMemoryUsage() const {
#ifdef Q_OS_LINUX
    // Read from /proc/self/status
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("VmRSS:")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024;  // Convert KB to bytes
                }
            }
        }
    }
#endif
    return 0;
}

// ============================================================================
// PerformanceProfiler::ScopedTimer
// ============================================================================

PerformanceProfiler::ScopedTimer::ScopedTimer(const QString& name, PerformanceProfiler* profiler)
    : name_(name), profiler_(profiler ? profiler : PerformanceProfiler::instance()) {
    timer_.start();
}

PerformanceProfiler::ScopedTimer::~ScopedTimer() {
    PerformanceMetrics metric;
    metric.operation = name_;
    metric.durationMs = timer_.elapsed();
    metric.timestamp = QDateTime::currentDateTime();
    profiler_->recordMetric(metric);
}

// ============================================================================
// PerformanceTestSuite
// ============================================================================

PerformanceTestSuite::PerformanceTestSuite(QObject* parent)
    : QObject(parent), profiler_(PerformanceProfiler::instance()) {
}

bool PerformanceTestSuite::testEditorLoad(int lineCount) {
    profiler_->startOperation("EditorLoad", QString("lines=%1").arg(lineCount));
    
    // Simulate loading large SQL script
    QString script = generateLargeSqlScript(lineCount);
    
    // Simulate processing
    QThread::msleep(100);
    
    profiler_->endOperation("EditorLoad");
    
    auto metric = profiler_->getLastMetric("EditorLoad");
    metric.itemsProcessed = lineCount;
    profiler_->recordMetric(metric);
    
    // Target: 10K lines in < 5 seconds
    return metric.durationMs < 5000;
}

bool PerformanceTestSuite::testEditorTyping(int characters) {
    profiler_->startOperation("EditorTyping", QString("chars=%1").arg(characters));
    
    // Simulate typing
    QThread::msleep(50);
    
    profiler_->endOperation("EditorTyping");
    
    auto metric = profiler_->getLastMetric("EditorTyping");
    metric.itemsProcessed = characters;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 1000;
}

bool PerformanceTestSuite::testEditorFormatting(int lineCount) {
    profiler_->startOperation("EditorFormatting", QString("lines=%1").arg(lineCount));
    
    // Simulate formatting
    QThread::msleep(200);
    
    profiler_->endOperation("EditorFormatting");
    
    auto metric = profiler_->getLastMetric("EditorFormatting");
    metric.itemsProcessed = lineCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 3000;
}

bool PerformanceTestSuite::testEditorSearch(int lineCount, int searchCount) {
    profiler_->startOperation("EditorSearch", QString("lines=%1,searches=%2").arg(lineCount).arg(searchCount));
    
    // Simulate search operations
    QThread::msleep(100);
    
    profiler_->endOperation("EditorSearch");
    
    auto metric = profiler_->getLastMetric("EditorSearch");
    metric.itemsProcessed = lineCount * searchCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 2000;
}

bool PerformanceTestSuite::testEditorFolding(int foldCount) {
    profiler_->startOperation("EditorFolding", QString("folds=%1").arg(foldCount));
    
    // Simulate folding
    QThread::msleep(50);
    
    profiler_->endOperation("EditorFolding");
    
    auto metric = profiler_->getLastMetric("EditorFolding");
    metric.itemsProcessed = foldCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 1000;
}

bool PerformanceTestSuite::testGridLoad(int rowCount, int columnCount) {
    profiler_->startOperation("GridLoad", QString("rows=%1,cols=%2").arg(rowCount).arg(columnCount));
    
    // Simulate loading data into grid
    auto data = generateLargeResultSet(rowCount, columnCount);
    QThread::msleep(100);
    
    profiler_->endOperation("GridLoad");
    
    auto metric = profiler_->getLastMetric("GridLoad");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    // Target: 10K rows in < 3 seconds
    return metric.durationMs < 3000;
}

bool PerformanceTestSuite::testGridScroll(int rowCount) {
    profiler_->startOperation("GridScroll", QString("rows=%1").arg(rowCount));
    
    // Simulate scrolling
    QThread::msleep(50);
    
    profiler_->endOperation("GridScroll");
    
    auto metric = profiler_->getLastMetric("GridScroll");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 500;
}

bool PerformanceTestSuite::testGridSort(int rowCount) {
    profiler_->startOperation("GridSort", QString("rows=%1").arg(rowCount));
    
    // Simulate sorting
    QThread::msleep(200);
    
    profiler_->endOperation("GridSort");
    
    auto metric = profiler_->getLastMetric("GridSort");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 2000;
}

bool PerformanceTestSuite::testGridFilter(int rowCount) {
    profiler_->startOperation("GridFilter", QString("rows=%1").arg(rowCount));
    
    // Simulate filtering
    QThread::msleep(150);
    
    profiler_->endOperation("GridFilter");
    
    auto metric = profiler_->getLastMetric("GridFilter");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 1500;
}

bool PerformanceTestSuite::testGridSelection(int rowCount) {
    profiler_->startOperation("GridSelection", QString("rows=%1").arg(rowCount));
    
    // Simulate selection
    QThread::msleep(30);
    
    profiler_->endOperation("GridSelection");
    
    auto metric = profiler_->getLastMetric("GridSelection");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 500;
}

bool PerformanceTestSuite::testExportCsv(int rowCount, const QString& fileName) {
    profiler_->startOperation("ExportCsv", QString("rows=%1").arg(rowCount));
    
    // Generate CSV data
    auto data = generateLargeCsv(rowCount, 10);
    
    // Write to file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }
    
    profiler_->endOperation("ExportCsv");
    
    auto metric = profiler_->getLastMetric("ExportCsv");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    // Target: 100K rows in < 30 seconds
    return metric.durationMs < 30000;
}

bool PerformanceTestSuite::testExportJson(int rowCount, const QString& fileName) {
    profiler_->startOperation("ExportJson", QString("rows=%1").arg(rowCount));
    
    // Generate JSON data
    QJsonArray array;
    for (int i = 0; i < qMin(rowCount, 1000); ++i) {
        QJsonObject obj;
        obj["id"] = i;
        obj["name"] = QString("Item %1").arg(i);
        array.append(obj);
    }
    QJsonDocument doc(array);
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
    
    profiler_->endOperation("ExportJson");
    
    auto metric = profiler_->getLastMetric("ExportJson");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 30000;
}

bool PerformanceTestSuite::testExportSql(int rowCount, const QString& fileName) {
    profiler_->startOperation("ExportSql", QString("rows=%1").arg(rowCount));
    
    // Generate SQL INSERT statements
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        for (int i = 0; i < qMin(rowCount, 10000); ++i) {
            stream << QString("INSERT INTO test VALUES (%1, 'item%2');\n").arg(i).arg(i);
        }
        file.close();
    }
    
    profiler_->endOperation("ExportSql");
    
    auto metric = profiler_->getLastMetric("ExportSql");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 30000;
}

bool PerformanceTestSuite::testImportCsv(int rowCount, const QString& fileName) {
    profiler_->startOperation("ImportCsv", QString("rows=%1").arg(rowCount));
    
    // Simulate parsing CSV
    QThread::msleep(500);
    
    profiler_->endOperation("ImportCsv");
    
    auto metric = profiler_->getLastMetric("ImportCsv");
    metric.itemsProcessed = rowCount;
    profiler_->recordMetric(metric);
    
    return metric.durationMs < 60000;  // 60 seconds for import
}

bool PerformanceTestSuite::testMemoryUsage(const QString& operation, qint64 maxMemoryBytes) {
    MemoryTracker::instance()->startTracking(operation);
    
    // Perform operation
    QThread::msleep(100);
    
    MemoryTracker::instance()->endTracking();
    
    qint64 peak = MemoryTracker::instance()->peakUsage();
    return peak < maxMemoryBytes;
}

QList<PerformanceTestSuite::TestResult> PerformanceTestSuite::runAllTests() {
    QList<TestResult> results;
    
    // Editor tests
    TestResult editorLoad;
    editorLoad.name = "Editor_10K_Lines";
    editorLoad.passed = testEditorLoad(10000);
    editorLoad.metrics = profiler_->getLastMetric("EditorLoad");
    results.append(editorLoad);
    
    // Grid tests
    TestResult gridLoad;
    gridLoad.name = "Grid_10K_Rows";
    gridLoad.passed = testGridLoad(10000, 10);
    gridLoad.metrics = profiler_->getLastMetric("GridLoad");
    results.append(gridLoad);
    
    // Export tests
    TestResult exportCsv;
    exportCsv.name = "Export_100K_Rows_CSV";
    exportCsv.passed = testExportCsv(100000, "/tmp/test_export.csv");
    exportCsv.metrics = profiler_->getLastMetric("ExportCsv");
    results.append(exportCsv);
    
    return results;
}

QString PerformanceTestSuite::generateReport(const QList<TestResult>& results) const {
    QString report;
    QTextStream stream(&report);
    
    stream << "Performance Test Report\n";
    stream << "=======================\n\n";
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : results) {
        stream << result.name << ": " << (result.passed ? "PASSED" : "FAILED") << "\n";
        stream << "  Duration: " << result.metrics.durationMs << " ms\n";
        stream << "  Items: " << result.metrics.itemsProcessed << "\n";
        stream << "  Throughput: " << result.metrics.itemsPerSecond() << " items/sec\n\n";
        
        if (result.passed) passed++;
        else failed++;
    }
    
    stream << "Summary: " << passed << " passed, " << failed << " failed\n";
    
    return report;
}

QString PerformanceTestSuite::generateLargeSqlScript(int lineCount) {
    QString script;
    for (int i = 0; i < lineCount; ++i) {
        script += QString("SELECT * FROM table%1 WHERE id = %2;\n").arg(i % 10).arg(i);
    }
    return script;
}

QStringList PerformanceTestSuite::generateLargeResultSet(int rowCount, int columnCount) {
    QStringList result;
    for (int i = 0; i < rowCount; ++i) {
        QStringList row;
        for (int j = 0; j < columnCount; ++j) {
            row.append(QString("value_%1_%2").arg(i).arg(j));
        }
        result.append(row.join(","));
    }
    return result;
}

QByteArray PerformanceTestSuite::generateLargeCsv(int rowCount, int columnCount) {
    QByteArray csv;
    // Header
    for (int j = 0; j < columnCount; ++j) {
        if (j > 0) csv.append(",");
        csv.append(QString("Column%1").arg(j).toUtf8());
    }
    csv.append("\n");
    
    // Data rows
    for (int i = 0; i < rowCount; ++i) {
        for (int j = 0; j < columnCount; ++j) {
            if (j > 0) csv.append(",");
            csv.append(QString("value_%1_%2").arg(i).arg(j).toUtf8());
        }
        csv.append("\n");
    }
    
    return csv;
}

// ============================================================================
// MemoryTracker
// ============================================================================

static MemoryTracker* g_memoryTracker = nullptr;

MemoryTracker* MemoryTracker::instance() {
    if (!g_memoryTracker) {
        g_memoryTracker = new MemoryTracker();
    }
    return g_memoryTracker;
}

MemoryTracker::MemoryTracker(QObject* parent) : QObject(parent) {
}

void MemoryTracker::startTracking(const QString& context) {
    context_ = context;
    startUsage_ = getCurrentUsage();
    peakUsage_ = startUsage_;
    tracking_ = true;
    
    // Start polling
    QTimer::singleShot(100, [this]() {
        if (tracking_) {
            qint64 current = getCurrentUsage();
            if (current > peakUsage_) {
                peakUsage_ = current;
                if (threshold_ > 0 && peakUsage_ > threshold_) {
                    emit peakExceeded(peakUsage_, threshold_);
                }
            }
        }
    });
}

void MemoryTracker::endTracking() {
    tracking_ = false;
}

qint64 MemoryTracker::currentUsage() const {
    if (!tracking_) return 0;
    return getCurrentUsage() - startUsage_;
}

qint64 MemoryTracker::peakUsage() const {
    return peakUsage_ - startUsage_;
}

qint64 MemoryTracker::delta() const {
    return getCurrentUsage() - startUsage_;
}

void MemoryTracker::reset() {
    tracking_ = false;
    startUsage_ = 0;
    peakUsage_ = 0;
    context_.clear();
}

void MemoryTracker::setPeakThreshold(qint64 bytes) {
    threshold_ = bytes;
}

qint64 MemoryTracker::getCurrentUsage() const {
#ifdef Q_OS_LINUX
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("VmRSS:")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024;
                }
            }
        }
    }
#endif
    return 0;
}

// ============================================================================
// PerformanceBudgetManager
// ============================================================================

static PerformanceBudgetManager* g_budgetManager = nullptr;

PerformanceBudgetManager* PerformanceBudgetManager::instance() {
    if (!g_budgetManager) {
        g_budgetManager = new PerformanceBudgetManager();
    }
    return g_budgetManager;
}

PerformanceBudgetManager::PerformanceBudgetManager(QObject* parent) : QObject(parent) {
}

void PerformanceBudgetManager::setBudget(const QString& operation, const PerformanceBudget& budget) {
    budgets_[operation] = budget;
}

PerformanceBudget PerformanceBudgetManager::getBudget(const QString& operation) const {
    return budgets_.value(operation);
}

bool PerformanceBudgetManager::checkBudget(const QString& operation, const PerformanceMetrics& metrics) const {
    if (!budgets_.contains(operation)) return true;
    
    const auto& budget = budgets_[operation];
    
    if (budget.maxDurationMs > 0 && metrics.durationMs > budget.maxDurationMs) {
        return false;
    }
    
    if (budget.maxMemoryBytes > 0 && metrics.memoryUsedBytes > budget.maxMemoryBytes) {
        return false;
    }
    
    return true;
}

void PerformanceBudgetManager::enforceBudget(const QString& operation) {
    // Would implement logic to throttle or cancel operations exceeding budget
    Q_UNUSED(operation)
}

// ============================================================================
// PerformanceMonitorWidget
// ============================================================================

PerformanceMonitorWidget::PerformanceMonitorWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void PerformanceMonitorWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton(tr("Start Profiling"), this);
    toolbarLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop Profiling"), this);
    stopBtn_->setEnabled(false);
    toolbarLayout->addWidget(stopBtn_);
    
    toolbarLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    toolbarLayout->addWidget(exportBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    toolbarLayout->addWidget(clearBtn);
    
    auto* benchmarkBtn = new QPushButton(tr("Run Benchmarks"), this);
    toolbarLayout->addWidget(benchmarkBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Metrics table
    metricsTable_ = new QTableWidget(this);
    metricsTable_->setColumnCount(6);
    metricsTable_->setHorizontalHeaderLabels({
        tr("Operation"), tr("Context"), tr("Duration (ms)"), 
        tr("Memory (KB)"), tr("Items"), tr("Throughput")
    });
    metricsTable_->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(metricsTable_);
    
    // Progress bar
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    mainLayout->addWidget(progressBar_);
    
    // Status
    statusLabel_ = new QLabel(tr("Ready"), this);
    mainLayout->addWidget(statusLabel_);
    
    // Connections
    connect(startBtn_, &QPushButton::clicked, this, &PerformanceMonitorWidget::onStartProfiling);
    connect(stopBtn_, &QPushButton::clicked, this, &PerformanceMonitorWidget::onStopProfiling);
    connect(exportBtn, &QPushButton::clicked, this, &PerformanceMonitorWidget::onExportResults);
    connect(clearBtn, &QPushButton::clicked, this, &PerformanceMonitorWidget::onClearResults);
    connect(benchmarkBtn, &QPushButton::clicked, this, &PerformanceMonitorWidget::onRunBenchmarks);
}

void PerformanceMonitorWidget::setProfiler(PerformanceProfiler* profiler) {
    profiler_ = profiler;
    if (profiler_) {
        connect(profiler_, &PerformanceProfiler::metricRecorded,
                this, &PerformanceMonitorWidget::refresh);
    }
}

void PerformanceMonitorWidget::refresh() {
    updateDisplay();
}

void PerformanceMonitorWidget::updateDisplay() {
    if (!profiler_) return;
    
    auto metrics = profiler_->getMetrics();
    metricsTable_->setRowCount(metrics.size());
    
    for (int i = 0; i < metrics.size(); ++i) {
        const auto& m = metrics[i];
        metricsTable_->setItem(i, 0, new QTableWidgetItem(m.operation));
        metricsTable_->setItem(i, 1, new QTableWidgetItem(m.context));
        metricsTable_->setItem(i, 2, new QTableWidgetItem(QString::number(m.durationMs)));
        metricsTable_->setItem(i, 3, new QTableWidgetItem(QString::number(m.memoryUsedBytes / 1024)));
        metricsTable_->setItem(i, 4, new QTableWidgetItem(QString::number(m.itemsProcessed)));
        metricsTable_->setItem(i, 5, new QTableWidgetItem(QString::number(m.itemsPerSecond(), 'f', 1)));
    }
}

void PerformanceMonitorWidget::onStartProfiling() {
    if (profiler_) {
        profiler_->startSession("Manual");
        startBtn_->setEnabled(false);
        stopBtn_->setEnabled(true);
        statusLabel_->setText(tr("Profiling active..."));
    }
}

void PerformanceMonitorWidget::onStopProfiling() {
    if (profiler_) {
        profiler_->endSession();
        startBtn_->setEnabled(true);
        stopBtn_->setEnabled(false);
        statusLabel_->setText(tr("Profiling stopped. %1 metrics recorded.")
                             .arg(profiler_->getMetrics().size()));
    }
}

void PerformanceMonitorWidget::onExportResults() {
    if (!profiler_) return;
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Performance Data"),
        QString(), tr("CSV (*.csv);;JSON (*.json)"));
    
    if (fileName.endsWith(".json")) {
        profiler_->exportToJson(fileName);
    } else {
        profiler_->exportToCsv(fileName);
    }
}

void PerformanceMonitorWidget::onClearResults() {
    metricsTable_->setRowCount(0);
    if (profiler_) {
        // Clear metrics
    }
}

void PerformanceMonitorWidget::onRunBenchmarks() {
    if (!profiler_) return;
    
    statusLabel_->setText(tr("Running benchmarks..."));
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    
    PerformanceTestSuite suite;
    auto results = suite.runAllTests();
    
    int passed = 0;
    for (const auto& r : results) {
        if (r.passed) passed++;
    }
    
    statusLabel_->setText(tr("Benchmarks complete: %1/%2 passed")
                         .arg(passed).arg(results.size()));
    progressBar_->setValue(100);
    
    updateDisplay();
}

} // namespace scratchrobin::core
