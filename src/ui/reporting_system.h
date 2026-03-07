#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QHash>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QListWidget;
class QGraphicsView;
class QGraphicsScene;
class QLabel;
class QSplitter;
class QPrinter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Reporting System - Database reports and analytics
 * 
 * - Report designer
 * - Data visualization (charts, graphs)
 * - Report scheduling
 * - Export to multiple formats (PDF, Excel, HTML)
 */

// ============================================================================
// Report Parameter
// ============================================================================
struct ReportParameter {
    QString name;
    QString type;  // string, number, date, boolean, list
    QVariant defaultValue;
    QString description;
    bool required = false;
    QStringList allowedValues;
};

// ============================================================================
// Report Definition
// ============================================================================
struct ReportDefinition {
    QString id;
    QString name;
    QString description;
    QString category;
    QString sqlQuery;
    QList<ReportParameter> parameters;
    QString layoutType;  // table, chart, dashboard
    QHash<QString, QVariant> layoutConfig;
    QString createdBy;
    QDateTime createdAt;
    QDateTime modifiedAt;
};

// ============================================================================
// Report Preview Widget
// ============================================================================
class ReportPreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReportPreviewWidget(QWidget* parent = nullptr);

    void setReportData(const QList<QHash<QString, QVariant>>& data);
    void setLayoutType(const QString& type);
    void setLayoutConfig(const QHash<QString, QVariant>& config);

    void exportToPdf(const QString& fileName);
    void exportToExcel(const QString& fileName);
    void exportToHtml(const QString& fileName);
    void exportToCsv(const QString& fileName);

public slots:
    void zoomIn();
    void zoomOut();
    void fitToWidth();
    void fitToPage();

private:
    void setupUi();
    void renderTable();
    void renderChart();
    void renderDashboard();
    
    QGraphicsView* view_ = nullptr;
    QGraphicsScene* scene_ = nullptr;
    QTableView* tableView_ = nullptr;
    
    QList<QHash<QString, QVariant>> data_;
    QString layoutType_ = "table";
    QHash<QString, QVariant> layoutConfig_;
    double zoom_ = 1.0;
};

// ============================================================================
// Report Designer Dialog
// ============================================================================
class ReportDesignerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReportDesignerDialog(backend::SessionClient* client,
                                 const QString& reportId = QString(),
                                 QWidget* parent = nullptr);

public slots:
    void onSave();
    void onRun();
    void onPreview();
    void onAddParameter();
    void onEditParameter();
    void onRemoveParameter();
    void onQueryChanged();
    void onLayoutTypeChanged(int index);

private:
    void setupUi();
    void setupGeneralTab();
    void setupQueryTab();
    void setupParametersTab();
    void setupLayoutTab();
    void setupPreviewTab();
    
    void loadReport(const QString& reportId);
    void updateParameterList();
    void updatePreview();
    
    backend::SessionClient* client_;
    QString reportId_;
    
    // General
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    
    // Query
    QTextEdit* queryEdit_ = nullptr;
    QPushButton* validateBtn_ = nullptr;
    
    // Parameters
    QListWidget* paramList_ = nullptr;
    QList<ReportParameter> parameters_;
    
    // Layout
    QComboBox* layoutTypeCombo_ = nullptr;
    QWidget* layoutConfigWidget_ = nullptr;
    
    // Preview
    ReportPreviewWidget* previewWidget_ = nullptr;
};

// ============================================================================
// Report Viewer Dialog
// ============================================================================
class ReportViewerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReportViewerDialog(backend::SessionClient* client,
                               const ReportDefinition& report,
                               QWidget* parent = nullptr);

public slots:
    void onRun();
    void onExport();
    void onPrint();
    void onSchedule();

private:
    void setupUi();
    void createParameterWidgets();
    void runReport();
    
    backend::SessionClient* client_;
    ReportDefinition report_;
    
    QHash<QString, QWidget*> paramWidgets_;
    ReportPreviewWidget* preview_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Report Manager Panel
// ============================================================================
class ReportManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ReportManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Report Manager"); }
    QString panelCategory() const override { return "reporting"; }
    
    void refresh();

public slots:
    void onNewReport();
    void onEditReport();
    void onDeleteReport();
    void onRunReport();
    void onDuplicateReport();
    void onExportDefinition();
    void onImportDefinition();
    void onManageSchedule();

signals:
    void reportSelected(const QString& reportId);
    void runReportRequested(const QString& reportId);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void loadReports();
    
    backend::SessionClient* client_;
    
    QTreeView* reportTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
};

// ============================================================================
// Report Schedule Dialog
// ============================================================================
class ReportScheduleDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReportScheduleDialog(backend::SessionClient* client,
                                 const QString& reportId = QString(),
                                 QWidget* parent = nullptr);

public slots:
    void onAddSchedule();
    void onEditSchedule();
    void onDeleteSchedule();
    void onEnableSchedule();
    void onDisableSchedule();
    void onRunNow();

private:
    void setupUi();
    void loadSchedules();
    
    backend::SessionClient* client_;
    QString reportId_;
    
    QTableView* scheduleTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
};

// ============================================================================
// Chart Builder Dialog
// ============================================================================
class ChartBuilderDialog : public QDialog {
    Q_OBJECT

public:
    enum ChartType {
        BarChart,
        LineChart,
        PieChart,
        AreaChart,
        ScatterPlot,
        DonutChart,
        RadarChart,
        HeatMap
    };
    
    explicit ChartBuilderDialog(backend::SessionClient* client,
                               QWidget* parent = nullptr);

    QHash<QString, QVariant> chartConfig() const;

public slots:
    void onChartTypeChanged(int type);
    void onDataSourceChanged();
    void onRefreshPreview();
    void onConfigureSeries();

private:
    void setupUi();
    void setupDataTab();
    void setupChartTab();
    void setupStyleTab();
    void updatePreview();
    
    backend::SessionClient* client_;
    ChartType currentType_ = BarChart;
    
    QComboBox* chartTypeCombo_ = nullptr;
    QComboBox* dataSourceCombo_ = nullptr;
    QComboBox* xAxisCombo_ = nullptr;
    QComboBox* yAxisCombo_ = nullptr;
    QListWidget* seriesList_ = nullptr;
    ReportPreviewWidget* preview_ = nullptr;
};

} // namespace scratchrobin::ui
