#include "ui/reporting_system.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTreeView>
#include <QTableView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>

namespace scratchrobin::ui {

// ============================================================================
// ReportPreviewWidget
// ============================================================================

ReportPreviewWidget::ReportPreviewWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void ReportPreviewWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Toolbar
    auto* toolbar = new QHBoxLayout();
    
    auto* zoomInBtn = new QPushButton(tr("Zoom In"), this);
    auto* zoomOutBtn = new QPushButton(tr("Zoom Out"), this);
    auto* fitWidthBtn = new QPushButton(tr("Fit Width"), this);
    auto* fitPageBtn = new QPushButton(tr("Fit Page"), this);
    
    toolbar->addWidget(zoomInBtn);
    toolbar->addWidget(zoomOutBtn);
    toolbar->addWidget(fitWidthBtn);
    toolbar->addWidget(fitPageBtn);
    toolbar->addStretch();
    
    layout->addLayout(toolbar);
    
    // View
    view_ = new QGraphicsView(this);
    scene_ = new QGraphicsScene(this);
    view_->setScene(scene_);
    view_->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    
    layout->addWidget(view_);
    
    connect(zoomInBtn, &QPushButton::clicked, this, &ReportPreviewWidget::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &ReportPreviewWidget::zoomOut);
    connect(fitWidthBtn, &QPushButton::clicked, this, &ReportPreviewWidget::fitToWidth);
    connect(fitPageBtn, &QPushButton::clicked, this, &ReportPreviewWidget::fitToPage);
}

void ReportPreviewWidget::setReportData(const QList<QHash<QString, QVariant>>& data) {
    data_ = data;
    renderTable();
}

void ReportPreviewWidget::setLayoutType(const QString& type) {
    layoutType_ = type;
    if (type == "chart") {
        renderChart();
    } else if (type == "dashboard") {
        renderDashboard();
    } else {
        renderTable();
    }
}

void ReportPreviewWidget::setLayoutConfig(const QHash<QString, QVariant>& config) {
    layoutConfig_ = config;
}

void ReportPreviewWidget::renderTable() {
    scene_->clear();
    
    if (data_.isEmpty()) {
        auto* text = scene_->addText(tr("No data to display"));
        text->setPos(50, 50);
        return;
    }
    
    // Simple table rendering
    QStringList headers = data_.first().keys();
    int colWidth = 120;
    int rowHeight = 25;
    int margin = 10;
    
    // Header row
    for (int col = 0; col < headers.size(); ++col) {
        auto* rect = scene_->addRect(col * colWidth, 0, colWidth, rowHeight,
                                     QPen(Qt::gray), QBrush(QColor(240, 240, 240)));
        auto* text = scene_->addText(headers[col]);
        text->setPos(col * colWidth + margin, 2);
    }
    
    // Data rows
    for (int row = 0; row < data_.size(); ++row) {
        for (int col = 0; col < headers.size(); ++col) {
            auto* rect = scene_->addRect(col * colWidth, (row + 1) * rowHeight, 
                                         colWidth, rowHeight, QPen(Qt::gray));
            QString value = data_[row][headers[col]].toString();
            auto* text = scene_->addText(value.left(12));
            text->setPos(col * colWidth + margin, (row + 1) * rowHeight + 2);
        }
    }
    
    scene_->setSceneRect(0, 0, headers.size() * colWidth, (data_.size() + 1) * rowHeight);
}

void ReportPreviewWidget::renderChart() {
    scene_->clear();
    
    // Simple bar chart
    int barWidth = 40;
    int spacing = 20;
    int maxHeight = 200;
    int baseline = 250;
    
    QList<int> values = {150, 200, 100, 250, 180, 220};
    QStringList labels = {"Jan", "Feb", "Mar", "Apr", "May", "Jun"};
    
    // Draw axes
    scene_->addLine(50, baseline, 350, baseline, QPen(Qt::black, 2));  // X axis
    scene_->addLine(50, baseline, 50, 50, QPen(Qt::black, 2));  // Y axis
    
    // Draw bars
    for (int i = 0; i < values.size(); ++i) {
        int x = 60 + i * (barWidth + spacing);
        int height = values[i];
        
        QLinearGradient gradient(x, baseline - height, x, baseline);
        gradient.setColorAt(0, QColor(66, 133, 244));
        gradient.setColorAt(1, QColor(25, 103, 210));
        
        auto* bar = scene_->addRect(x, baseline - height, barWidth, height,
                                    QPen(Qt::NoPen), QBrush(gradient));
        
        auto* label = scene_->addText(labels[i]);
        label->setPos(x + 10, baseline + 5);
        
        auto* value = scene_->addText(QString::number(values[i]));
        value->setPos(x + 5, baseline - height - 20);
    }
    
    scene_->setSceneRect(0, 0, 400, 300);
}

void ReportPreviewWidget::renderDashboard() {
    scene_->clear();
    
    // Mock dashboard with multiple widgets
    auto* title = scene_->addText(tr("Sales Dashboard"));
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setPos(20, 10);
    
    // KPI boxes
    QList<QPair<QString, QString>> kpis = {
        {tr("Total Sales"), "$125,000"},
        {tr("Orders"), "1,234"},
        {tr("Customers"), "567"},
        {tr("Avg Order"), "$101"}
    };
    
    int kpiX = 20;
    for (const auto& kpi : kpis) {
        auto* box = scene_->addRect(kpiX, 50, 110, 60, 
                                    QPen(QColor(200, 200, 200)), 
                                    QBrush(QColor(250, 250, 250)));
        
        auto* label = scene_->addText(kpi.first);
        label->setPos(kpiX + 10, 55);
        
        auto* value = scene_->addText(kpi.second);
        QFont valueFont = value->font();
        valueFont.setPointSize(12);
        valueFont.setBold(true);
        value->setFont(valueFont);
        value->setPos(kpiX + 10, 80);
        
        kpiX += 130;
    }
    
    scene_->setSceneRect(0, 0, 550, 300);
}

void ReportPreviewWidget::zoomIn() {
    zoom_ *= 1.2;
    view_->resetTransform();
    view_->scale(zoom_, zoom_);
}

void ReportPreviewWidget::zoomOut() {
    zoom_ /= 1.2;
    view_->resetTransform();
    view_->scale(zoom_, zoom_);
}

void ReportPreviewWidget::fitToWidth() {
    view_->fitInView(scene_->itemsBoundingRect(), Qt::KeepAspectRatioByExpanding);
}

void ReportPreviewWidget::fitToPage() {
    view_->fitInView(scene_->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void ReportPreviewWidget::exportToPdf(const QString& fileName) {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    
    QPainter painter(&printer);
    scene_->render(&painter);
}

void ReportPreviewWidget::exportToExcel(const QString& fileName) {
    // Export to Excel format
    Q_UNUSED(fileName)
}

void ReportPreviewWidget::exportToHtml(const QString& fileName) {
    // Export to HTML
    Q_UNUSED(fileName)
}

void ReportPreviewWidget::exportToCsv(const QString& fileName) {
    // Export to CSV
    Q_UNUSED(fileName)
}

// ============================================================================
// ReportDesignerDialog
// ============================================================================

ReportDesignerDialog::ReportDesignerDialog(backend::SessionClient* client,
                                          const QString& reportId,
                                          QWidget* parent)
    : QDialog(parent), client_(client), reportId_(reportId) {
    
    setWindowTitle(reportId.isEmpty() ? tr("New Report") : tr("Edit Report"));
    setMinimumSize(900, 700);
    
    setupUi();
    
    if (!reportId.isEmpty()) {
        loadReport(reportId);
    }
}

void ReportDesignerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* tabWidget = new QTabWidget(this);
    
    setupGeneralTab();
    setupQueryTab();
    setupParametersTab();
    setupLayoutTab();
    setupPreviewTab();
    
    tabWidget->addTab(findChild<QWidget*>("generalTab"), tr("&General"));
    tabWidget->addTab(findChild<QWidget*>("queryTab"), tr("&Query"));
    tabWidget->addTab(findChild<QWidget*>("parametersTab"), tr("&Parameters"));
    tabWidget->addTab(findChild<QWidget*>("layoutTab"), tr("&Layout"));
    tabWidget->addTab(findChild<QWidget*>("previewTab"), tr("Pre&view"));
    
    mainLayout->addWidget(tabWidget);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* runBtn = new QPushButton(tr("&Run"), this);
    auto* saveBtn = new QPushButton(tr("&Save"), this);
    saveBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(tr("&Cancel"), this);
    
    btnLayout->addWidget(runBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(runBtn, &QPushButton::clicked, this, &ReportDesignerDialog::onRun);
    connect(saveBtn, &QPushButton::clicked, this, &ReportDesignerDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ReportDesignerDialog::setupGeneralTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("generalTab");
    auto* layout = new QGridLayout(tab);
    
    layout->addWidget(new QLabel(tr("Report Name:"), this), 0, 0);
    nameEdit_ = new QLineEdit(this);
    layout->addWidget(nameEdit_, 0, 1);
    
    layout->addWidget(new QLabel(tr("Category:"), this), 1, 0);
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->setEditable(true);
    categoryCombo_->addItems({"Sales", "Inventory", "Customers", "System", "Custom"});
    layout->addWidget(categoryCombo_, 1, 1);
    
    layout->addWidget(new QLabel(tr("Description:"), this), 2, 0, Qt::AlignTop);
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setMaximumHeight(100);
    layout->addWidget(descriptionEdit_, 2, 1);
    
    layout->setRowStretch(3, 1);
}

void ReportDesignerDialog::setupQueryTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("queryTab");
    auto* layout = new QVBoxLayout(tab);
    
    layout->addWidget(new QLabel(tr("SQL Query:"), this));
    
    queryEdit_ = new QTextEdit(this);
    queryEdit_->setFont(QFont("Consolas", 10));
    queryEdit_->setPlaceholderText(tr("Enter SQL query here..."));
    layout->addWidget(queryEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    validateBtn_ = new QPushButton(tr("&Validate"), this);
    auto* formatBtn = new QPushButton(tr("&Format"), this);
    
    btnLayout->addWidget(validateBtn_);
    btnLayout->addWidget(formatBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    connect(validateBtn_, &QPushButton::clicked, this, &ReportDesignerDialog::onQueryChanged);
}

void ReportDesignerDialog::setupParametersTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("parametersTab");
    auto* layout = new QVBoxLayout(tab);
    
    layout->addWidget(new QLabel(tr("Report Parameters:"), this));
    
    paramList_ = new QListWidget(this);
    layout->addWidget(paramList_);
    
    auto* btnLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("&Add"), this);
    auto* editBtn = new QPushButton(tr("&Edit"), this);
    auto* removeBtn = new QPushButton(tr("&Remove"), this);
    
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    connect(addBtn, &QPushButton::clicked, this, &ReportDesignerDialog::onAddParameter);
    connect(editBtn, &QPushButton::clicked, this, &ReportDesignerDialog::onEditParameter);
    connect(removeBtn, &QPushButton::clicked, this, &ReportDesignerDialog::onRemoveParameter);
}

void ReportDesignerDialog::setupLayoutTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("layoutTab");
    auto* layout = new QVBoxLayout(tab);
    
    auto* formLayout = new QHBoxLayout();
    formLayout->addWidget(new QLabel(tr("Layout Type:"), this));
    layoutTypeCombo_ = new QComboBox(this);
    layoutTypeCombo_->addItems({tr("Table"), tr("Chart"), tr("Dashboard")});
    formLayout->addWidget(layoutTypeCombo_);
    formLayout->addStretch();
    layout->addLayout(formLayout);
    
    layoutConfigWidget_ = new QGroupBox(tr("Layout Configuration"), this);
    auto* configLayout = new QVBoxLayout(layoutConfigWidget_);
    configLayout->addWidget(new QLabel(tr("Configure layout options here..."), this));
    layout->addWidget(layoutConfigWidget_);
    
    layout->addStretch();
    
    connect(layoutTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportDesignerDialog::onLayoutTypeChanged);
}

void ReportDesignerDialog::setupPreviewTab() {
    auto* tab = new QWidget(this);
    tab->setObjectName("previewTab");
    auto* layout = new QVBoxLayout(tab);
    
    previewWidget_ = new ReportPreviewWidget(this);
    layout->addWidget(previewWidget_);
    
    auto* btnLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(tr("&Refresh"), this);
    btnLayout->addWidget(refreshBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    connect(refreshBtn, &QPushButton::clicked, this, &ReportDesignerDialog::onPreview);
}

void ReportDesignerDialog::loadReport(const QString& reportId) {
    Q_UNUSED(reportId)
    // Mock load
    nameEdit_->setText("Customer Sales Report");
    categoryCombo_->setCurrentText("Sales");
    descriptionEdit_->setPlainText(tr("Monthly sales by customer"));
    queryEdit_->setPlainText("SELECT * FROM sales WHERE date >= :start_date AND date <= :end_date");
}

void ReportDesignerDialog::updateParameterList() {
    paramList_->clear();
    for (const auto& param : parameters_) {
        paramList_->addItem(QString("%1 (%2)%3")
            .arg(param.name)
            .arg(param.type)
            .arg(param.required ? " *" : ""));
    }
}

void ReportDesignerDialog::onSave() {
    if (nameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Report name is required."));
        return;
    }
    accept();
}

void ReportDesignerDialog::onRun() {
    ReportDefinition report;
    report.name = nameEdit_->text();
    report.sqlQuery = queryEdit_->toPlainText();
    
    ReportViewerDialog dlg(client_, report, this);
    dlg.exec();
}

void ReportDesignerDialog::onPreview() {
    // Generate mock data
    QList<QHash<QString, QVariant>> data;
    for (int i = 1; i <= 10; ++i) {
        QHash<QString, QVariant> row;
        row["ID"] = i;
        row["Name"] = QString("Customer %1").arg(i);
        row["Sales"] = 1000 + i * 100;
        row["Orders"] = i * 5;
        data.append(row);
    }
    
    previewWidget_->setReportData(data);
    previewWidget_->setLayoutType(layoutTypeCombo_->currentText().toLower());
}

void ReportDesignerDialog::onAddParameter() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Parameter"),
        tr("Parameter name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        ReportParameter param;
        param.name = name;
        param.type = "string";
        parameters_.append(param);
        updateParameterList();
    }
}

void ReportDesignerDialog::onEditParameter() {
    // Edit selected parameter
}

void ReportDesignerDialog::onRemoveParameter() {
    int row = paramList_->currentRow();
    if (row >= 0) {
        parameters_.removeAt(row);
        updateParameterList();
    }
}

void ReportDesignerDialog::onQueryChanged() {
    QMessageBox::information(this, tr("Validate"), tr("Query is valid."));
}

void ReportDesignerDialog::onLayoutTypeChanged(int index) {
    Q_UNUSED(index)
    onPreview();
}

// ============================================================================
// ReportViewerDialog
// ============================================================================

ReportViewerDialog::ReportViewerDialog(backend::SessionClient* client,
                                      const ReportDefinition& report,
                                      QWidget* parent)
    : QDialog(parent), client_(client), report_(report) {
    
    setWindowTitle(report.name);
    setMinimumSize(900, 600);
    
    setupUi();
    createParameterWidgets();
}

void ReportViewerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Parameters area
    auto* paramGroup = new QGroupBox(tr("Parameters"), this);
    paramGroup->setObjectName("paramGroup");
    auto* paramLayout = new QHBoxLayout(paramGroup);
    paramLayout->addStretch();
    mainLayout->addWidget(paramGroup);
    
    // Preview
    preview_ = new ReportPreviewWidget(this);
    mainLayout->addWidget(preview_);
    
    // Status
    statusLabel_ = new QLabel(this);
    mainLayout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* runBtn = new QPushButton(tr("&Run"), this);
    auto* exportBtn = new QPushButton(tr("&Export"), this);
    auto* printBtn = new QPushButton(tr("&Print"), this);
    auto* scheduleBtn = new QPushButton(tr("&Schedule"), this);
    auto* closeBtn = new QPushButton(tr("&Close"), this);
    
    btnLayout->addWidget(runBtn);
    btnLayout->addWidget(exportBtn);
    btnLayout->addWidget(printBtn);
    btnLayout->addWidget(scheduleBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(runBtn, &QPushButton::clicked, this, &ReportViewerDialog::onRun);
    connect(exportBtn, &QPushButton::clicked, this, &ReportViewerDialog::onExport);
    connect(printBtn, &QPushButton::clicked, this, &ReportViewerDialog::onPrint);
    connect(scheduleBtn, &QPushButton::clicked, this, &ReportViewerDialog::onSchedule);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void ReportViewerDialog::createParameterWidgets() {
    auto* paramGroup = findChild<QGroupBox*>("paramGroup");
    if (!paramGroup) return;
    
    auto* layout = paramGroup->layout();
    
    // Clear existing widgets except stretch
    while (layout->count() > 1) {
        auto* item = layout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    paramWidgets_.clear();
    
    // Create widgets for parameters
    for (const auto& param : report_.parameters) {
        auto* label = new QLabel(param.name + ":", this);
        layout->addWidget(label);
        
        QWidget* widget = nullptr;
        if (param.type == "date") {
            widget = new QDateEdit(this);
        } else if (param.type == "number") {
            widget = new QSpinBox(this);
        } else if (param.type == "boolean") {
            widget = new QCheckBox(this);
        } else if (param.type == "list" && !param.allowedValues.isEmpty()) {
            auto* combo = new QComboBox(this);
            combo->addItems(param.allowedValues);
            widget = combo;
        } else {
            widget = new QLineEdit(this);
        }
        
        layout->addWidget(widget);
        paramWidgets_[param.name] = widget;
    }
}

void ReportViewerDialog::runReport() {
    // Generate mock data
    QList<QHash<QString, QVariant>> data;
    for (int i = 1; i <= 20; ++i) {
        QHash<QString, QVariant> row;
        row["ID"] = i;
        row["Customer"] = QString("Customer %1").arg(i);
        row["Revenue"] = QString("$%1").arg(5000 + i * 500);
        row["Orders"] = 10 + i;
        data.append(row);
    }
    
    preview_->setReportData(data);
    preview_->setLayoutType(report_.layoutType);
    
    statusLabel_->setText(tr("Generated %1 rows in 0.5 seconds").arg(data.size()));
}

void ReportViewerDialog::onRun() {
    runReport();
}

void ReportViewerDialog::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Report"),
        QString(), tr("PDF Files (*.pdf);;Excel Files (*.xlsx);;HTML Files (*.html);;CSV Files (*.csv)"));
    
    if (fileName.isEmpty()) return;
    
    if (fileName.endsWith(".pdf")) {
        preview_->exportToPdf(fileName);
    } else if (fileName.endsWith(".html")) {
        preview_->exportToHtml(fileName);
    } else if (fileName.endsWith(".csv")) {
        preview_->exportToCsv(fileName);
    }
    
    QMessageBox::information(this, tr("Export"), 
        tr("Report exported to %1").arg(fileName));
}

void ReportViewerDialog::onPrint() {
    QPrinter printer;
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        // Print
    }
}

void ReportViewerDialog::onSchedule() {
    ReportScheduleDialog dlg(client_, report_.id, this);
    dlg.exec();
}

// ============================================================================
// ReportManagerPanel
// ============================================================================

ReportManagerPanel::ReportManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("report_manager", parent), client_(client) {
    setupUi();
}

void ReportManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QHBoxLayout();
    
    auto* newBtn = new QPushButton(tr("New"), this);
    auto* editBtn = new QPushButton(tr("Edit"), this);
    auto* runBtn = new QPushButton(tr("Run"), this);
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(newBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(runBtn);
    toolbar->addWidget(deleteBtn);
    toolbar->addStretch();
    toolbar->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbar);
    
    // Tree
    reportTree_ = new QTreeView(this);
    reportTree_->setHeaderHidden(false);
    reportTree_->setAlternatingRowColors(true);
    
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Report"), tr("Category"), tr("Modified")});
    reportTree_->setModel(model_);
    
    mainLayout->addWidget(reportTree_);
    
    loadReports();
    
    connect(newBtn, &QPushButton::clicked, this, &ReportManagerPanel::onNewReport);
    connect(editBtn, &QPushButton::clicked, this, &ReportManagerPanel::onEditReport);
    connect(runBtn, &QPushButton::clicked, this, &ReportManagerPanel::onRunReport);
    connect(deleteBtn, &QPushButton::clicked, this, &ReportManagerPanel::onDeleteReport);
    connect(refreshBtn, &QPushButton::clicked, this, &ReportManagerPanel::refresh);
}

void ReportManagerPanel::loadReports() {
    model_->removeRows(0, model_->rowCount());
    
    // Sales
    auto* salesCat = new QStandardItem(tr("Sales"));
    salesCat->appendRow({new QStandardItem(tr("Monthly Sales")),
                         new QStandardItem(tr("Sales")),
                         new QStandardItem(tr("2026-03-01"))});
    salesCat->appendRow({new QStandardItem(tr("Top Customers")),
                         new QStandardItem(tr("Sales")),
                         new QStandardItem(tr("2026-02-28"))});
    model_->appendRow(salesCat);
    
    // Inventory
    auto* invCat = new QStandardItem(tr("Inventory"));
    invCat->appendRow({new QStandardItem(tr("Stock Levels")),
                       new QStandardItem(tr("Inventory")),
                       new QStandardItem(tr("2026-03-05"))});
    invCat->appendRow({new QStandardItem(tr("Reorder Report")),
                       new QStandardItem(tr("Inventory")),
                       new QStandardItem(tr("2026-03-03"))});
    model_->appendRow(invCat);
    
    // System
    auto* sysCat = new QStandardItem(tr("System"));
    sysCat->appendRow({new QStandardItem(tr("Query Performance")),
                       new QStandardItem(tr("System")),
                       new QStandardItem(tr("2026-03-06"))});
    model_->appendRow(sysCat);
    
    reportTree_->expandAll();
    reportTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void ReportManagerPanel::refresh() {
    loadReports();
}

void ReportManagerPanel::panelActivated() {
    refresh();
}

void ReportManagerPanel::onNewReport() {
    ReportDesignerDialog dlg(client_, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void ReportManagerPanel::onEditReport() {
    auto index = reportTree_->currentIndex();
    if (!index.isValid()) return;
    
    ReportDesignerDialog dlg(client_, "report_id", this);
    dlg.exec();
}

void ReportManagerPanel::onDeleteReport() {
    auto reply = QMessageBox::question(this, tr("Delete Report"),
        tr("Are you sure you want to delete this report?"));
    
    if (reply == QMessageBox::Yes) {
        refresh();
    }
}

void ReportManagerPanel::onRunReport() {
    auto index = reportTree_->currentIndex();
    if (!index.isValid()) return;
    
    ReportDefinition report;
    report.name = index.data().toString();
    report.layoutType = "table";
    
    ReportViewerDialog dlg(client_, report, this);
    dlg.exec();
}

void ReportManagerPanel::onDuplicateReport() {
    // Duplicate selected report
}

void ReportManagerPanel::onExportDefinition() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Report"),
        QString(), tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Report exported."));
    }
}

void ReportManagerPanel::onImportDefinition() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Report"),
        QString(), tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Import"), tr("Report imported."));
        refresh();
    }
}

void ReportManagerPanel::onManageSchedule() {
    ReportScheduleDialog dlg(client_, QString(), this);
    dlg.exec();
}

// ============================================================================
// ReportScheduleDialog
// ============================================================================

ReportScheduleDialog::ReportScheduleDialog(backend::SessionClient* client,
                                          const QString& reportId,
                                          QWidget* parent)
    : QDialog(parent), client_(client), reportId_(reportId) {
    
    setWindowTitle(tr("Report Schedule"));
    setMinimumSize(700, 400);
    
    setupUi();
}

void ReportScheduleDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    layout()->addWidget(new QLabel(tr("Scheduled Reports:"), this));
    
    scheduleTable_ = new QTableView(this);
    scheduleTable_->setAlternatingRowColors(true);
    scheduleTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Report"), tr("Frequency"), 
                                        tr("Next Run"), tr("Status"), tr("Recipients")});
    scheduleTable_->setModel(model_);
    
    mainLayout->addWidget(scheduleTable_);
    
    // Toolbar
    auto* toolbar = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("&Add"), this);
    auto* editBtn = new QPushButton(tr("&Edit"), this);
    auto* deleteBtn = new QPushButton(tr("&Delete"), this);
    auto* runBtn = new QPushButton(tr("&Run Now"), this);
    auto* closeBtn = new QPushButton(tr("&Close"), this);
    
    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(deleteBtn);
    toolbar->addWidget(runBtn);
    toolbar->addStretch();
    toolbar->addWidget(closeBtn);
    mainLayout->addLayout(toolbar);
    
    connect(addBtn, &QPushButton::clicked, this, &ReportScheduleDialog::onAddSchedule);
    connect(editBtn, &QPushButton::clicked, this, &ReportScheduleDialog::onEditSchedule);
    connect(deleteBtn, &QPushButton::clicked, this, &ReportScheduleDialog::onDeleteSchedule);
    connect(runBtn, &QPushButton::clicked, this, &ReportScheduleDialog::onRunNow);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    loadSchedules();
}

void ReportScheduleDialog::loadSchedules() {
    model_->removeRows(0, model_->rowCount());
    
    QList<QList<QString>> schedules = {
        {tr("Monthly Sales"), tr("Monthly"), tr("2026-04-01 09:00"), tr("Enabled"), tr("admin@example.com")},
        {tr("Stock Levels"), tr("Daily"), tr("2026-03-07 08:00"), tr("Enabled"), tr("inventory@example.com")},
        {tr("Query Performance"), tr("Weekly"), tr("2026-03-10 07:00"), tr("Disabled"), tr("dba@example.com")},
    };
    
    for (const auto& row : schedules) {
        QList<QStandardItem*> items;
        for (const auto& cell : row) {
            items.append(new QStandardItem(cell));
        }
        model_->appendRow(items);
    }
    
    scheduleTable_->horizontalHeader()->setStretchLastSection(true);
}

void ReportScheduleDialog::onAddSchedule() {
    QMessageBox::information(this, tr("Add Schedule"), tr("Add new schedule..."));
    loadSchedules();
}

void ReportScheduleDialog::onEditSchedule() {
    QMessageBox::information(this, tr("Edit Schedule"), tr("Edit schedule..."));
}

void ReportScheduleDialog::onDeleteSchedule() {
    auto reply = QMessageBox::question(this, tr("Delete"),
        tr("Delete selected schedule?"));
    
    if (reply == QMessageBox::Yes) {
        loadSchedules();
    }
}

void ReportScheduleDialog::onEnableSchedule() {
    // Enable selected schedule
}

void ReportScheduleDialog::onDisableSchedule() {
    // Disable selected schedule
}

void ReportScheduleDialog::onRunNow() {
    QMessageBox::information(this, tr("Run Now"), tr("Running scheduled report now..."));
}

// ============================================================================
// ChartBuilderDialog
// ============================================================================

ChartBuilderDialog::ChartBuilderDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    
    setWindowTitle(tr("Chart Builder"));
    setMinimumSize(900, 700);
    
    setupUi();
}

void ChartBuilderDialog::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    
    // Left panel - configuration
    auto* configPanel = new QWidget(this);
    auto* configLayout = new QVBoxLayout(configPanel);
    
    // Chart type
    configLayout->addWidget(new QLabel(tr("Chart Type:"), this));
    chartTypeCombo_ = new QComboBox(this);
    chartTypeCombo_->addItems({tr("Bar Chart"), tr("Line Chart"), tr("Pie Chart"),
                               tr("Area Chart"), tr("Scatter Plot"), tr("Donut Chart"),
                               tr("Radar Chart"), tr("Heat Map")});
    configLayout->addWidget(chartTypeCombo_);
    
    // Data source
    configLayout->addWidget(new QLabel(tr("Data Source:"), this));
    dataSourceCombo_ = new QComboBox(this);
    dataSourceCombo_->addItems({tr("sales_by_month"), tr("customer_by_region"), 
                                 tr("product_performance")});
    configLayout->addWidget(dataSourceCombo_);
    
    // Axes
    configLayout->addWidget(new QLabel(tr("X Axis:"), this));
    xAxisCombo_ = new QComboBox(this);
    configLayout->addWidget(xAxisCombo_);
    
    configLayout->addWidget(new QLabel(tr("Y Axis:"), this));
    yAxisCombo_ = new QComboBox(this);
    configLayout->addWidget(yAxisCombo_);
    
    // Series
    configLayout->addWidget(new QLabel(tr("Series:"), this));
    seriesList_ = new QListWidget(this);
    seriesList_->setMaximumHeight(100);
    configLayout->addWidget(seriesList_);
    
    auto* seriesBtnLayout = new QHBoxLayout();
    auto* addSeriesBtn = new QPushButton(tr("Add"), this);
    auto* configSeriesBtn = new QPushButton(tr("Config"), this);
    seriesBtnLayout->addWidget(addSeriesBtn);
    seriesBtnLayout->addWidget(configSeriesBtn);
    seriesBtnLayout->addStretch();
    configLayout->addLayout(seriesBtnLayout);
    
    configLayout->addStretch();
    
    // Preview button
    auto* refreshBtn = new QPushButton(tr("Refresh Preview"), this);
    configLayout->addWidget(refreshBtn);
    
    // Right panel - preview
    preview_ = new ReportPreviewWidget(this);
    
    mainLayout->addWidget(configPanel, 1);
    mainLayout->addWidget(preview_, 3);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* okBtn = new QPushButton(tr("&OK"), this);
    auto* cancelBtn = new QPushButton(tr("&Cancel"), this);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(chartTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartBuilderDialog::onChartTypeChanged);
    connect(refreshBtn, &QPushButton::clicked, this, &ChartBuilderDialog::onRefreshPreview);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    // Initial preview
    onRefreshPreview();
}

void ChartBuilderDialog::setupDataTab() {}
void ChartBuilderDialog::setupChartTab() {}
void ChartBuilderDialog::setupStyleTab() {}

void ChartBuilderDialog::onChartTypeChanged(int type) {
    currentType_ = static_cast<ChartType>(type);
    onRefreshPreview();
}

void ChartBuilderDialog::onDataSourceChanged() {
    onRefreshPreview();
}

void ChartBuilderDialog::onRefreshPreview() {
    preview_->setLayoutType("chart");
}

void ChartBuilderDialog::onConfigureSeries() {
    QMessageBox::information(this, tr("Configure Series"), tr("Configure series options..."));
}

QHash<QString, QVariant> ChartBuilderDialog::chartConfig() const {
    QHash<QString, QVariant> config;
    config["type"] = chartTypeCombo_->currentIndex();
    config["dataSource"] = dataSourceCombo_->currentText();
    return config;
}

} // namespace scratchrobin::ui
