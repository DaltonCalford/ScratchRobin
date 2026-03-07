#include "ui/doc_generator_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QTreeWidget>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTableFormat>
#include <QTextFrameFormat>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QDir>
#include <QApplication>

namespace scratchrobin::ui {

DocGeneratorDialog::DocGeneratorDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Generate Database Documentation"));
    setMinimumSize(700, 550);
    resize(800, 600);
    
    setupUi();
    updatePreview();
}

DocGeneratorDialog::~DocGeneratorDialog() = default;

void DocGeneratorDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    
    tab_widget_ = new QTabWidget(this);
    layout->addWidget(tab_widget_);
    
    createOptionsTab();
    createPreviewTab();
    createOutputTab();
    
    // Button box
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    preview_btn_ = new QPushButton(tr("Preview"), this);
    button_layout->addWidget(preview_btn_);
    
    cancel_btn_ = new QPushButton(tr("Cancel"), this);
    button_layout->addWidget(cancel_btn_);
    
    generate_btn_ = new QPushButton(tr("Generate"), this);
    generate_btn_->setDefault(true);
    button_layout->addWidget(generate_btn_);
    
    layout->addLayout(button_layout);
    
    // Connections
    connect(preview_btn_, &QPushButton::clicked, this, &DocGeneratorDialog::onPreview);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
    connect(generate_btn_, &QPushButton::clicked, this, &DocGeneratorDialog::onGenerate);
    connect(format_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DocGeneratorDialog::onFormatChanged);
    connect(tables_check_, &QCheckBox::toggled, this, &DocGeneratorDialog::updatePreview);
    connect(views_check_, &QCheckBox::toggled, this, &DocGeneratorDialog::updatePreview);
    connect(title_edit_, &QLineEdit::textChanged, this, &DocGeneratorDialog::updatePreview);
}

void DocGeneratorDialog::createOptionsTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);
    layout->setSpacing(12);
    
    // Document title
    auto* title_layout = new QHBoxLayout();
    title_layout->addWidget(new QLabel(tr("Document Title:")));
    title_edit_ = new QLineEdit(tr("ScratchBird Database Documentation"), this);
    title_layout->addWidget(title_edit_);
    layout->addLayout(title_layout);
    
    // Objects to include
    auto* objects_group = new QGroupBox(tr("Objects to Include"), tab);
    auto* objects_layout = new QGridLayout(objects_group);
    
    tables_check_ = new QCheckBox(tr("Tables"), this);
    tables_check_->setChecked(true);
    objects_layout->addWidget(tables_check_, 0, 0);
    
    views_check_ = new QCheckBox(tr("Views"), this);
    views_check_->setChecked(true);
    objects_layout->addWidget(views_check_, 0, 1);
    
    procs_check_ = new QCheckBox(tr("Stored Procedures"), this);
    procs_check_->setChecked(true);
    objects_layout->addWidget(procs_check_, 1, 0);
    
    funcs_check_ = new QCheckBox(tr("Functions"), this);
    funcs_check_->setChecked(true);
    objects_layout->addWidget(funcs_check_, 1, 1);
    
    triggers_check_ = new QCheckBox(tr("Triggers"), this);
    triggers_check_->setChecked(true);
    objects_layout->addWidget(triggers_check_, 2, 0);
    
    indexes_check_ = new QCheckBox(tr("Indexes"), this);
    indexes_check_->setChecked(true);
    objects_layout->addWidget(indexes_check_, 2, 1);
    
    relationships_check_ = new QCheckBox(tr("Relationships / Foreign Keys"), this);
    relationships_check_->setChecked(true);
    objects_layout->addWidget(relationships_check_, 3, 0);
    
    diagrams_check_ = new QCheckBox(tr("Include ER Diagrams"), this);
    diagrams_check_->setChecked(true);
    objects_layout->addWidget(diagrams_check_, 3, 1);
    
    sample_data_check_ = new QCheckBox(tr("Include Sample Data"), this);
    sample_data_check_->setChecked(false);
    objects_layout->addWidget(sample_data_check_, 4, 0);
    
    layout->addWidget(objects_group);
    
    // Object selection tree
    auto* tree_group = new QGroupBox(tr("Select Specific Objects"), tab);
    auto* tree_layout = new QVBoxLayout(tree_group);
    
    objects_tree_ = new QTreeWidget(this);
    objects_tree_->setHeaderLabel(tr("Database Objects"));
    objects_tree_->setSelectionMode(QAbstractItemView::MultiSelection);
    
    // Add sample items
    auto* tablesItem = new QTreeWidgetItem(objects_tree_, QStringList(tr("Tables")));
    tablesItem->setCheckState(0, Qt::Checked);
    new QTreeWidgetItem(tablesItem, QStringList("users"));
    new QTreeWidgetItem(tablesItem, QStringList("orders"));
    new QTreeWidgetItem(tablesItem, QStringList("products"));
    new QTreeWidgetItem(tablesItem, QStringList("order_items"));
    new QTreeWidgetItem(tablesItem, QStringList("categories"));
    tablesItem->setExpanded(true);
    
    auto* viewsItem = new QTreeWidgetItem(objects_tree_, QStringList(tr("Views")));
    viewsItem->setCheckState(0, Qt::Checked);
    new QTreeWidgetItem(viewsItem, QStringList("v_active_orders"));
    new QTreeWidgetItem(viewsItem, QStringList("v_user_summary"));
    
    auto* procsItem = new QTreeWidgetItem(objects_tree_, QStringList(tr("Procedures")));
    procsItem->setCheckState(0, Qt::Checked);
    new QTreeWidgetItem(procsItem, QStringList("sp_get_user_orders"));
    new QTreeWidgetItem(procsItem, QStringList("sp_update_inventory"));
    
    objects_tree_->expandAll();
    tree_layout->addWidget(objects_tree_);
    
    auto* tree_btn_layout = new QHBoxLayout();
    auto* select_all_btn = new QPushButton(tr("Select All"), this);
    auto* select_none_btn = new QPushButton(tr("Select None"), this);
    tree_btn_layout->addWidget(select_all_btn);
    tree_btn_layout->addWidget(select_none_btn);
    tree_btn_layout->addStretch();
    tree_layout->addLayout(tree_btn_layout);
    
    connect(select_all_btn, &QPushButton::clicked, this, &DocGeneratorDialog::onSelectAll);
    connect(select_none_btn, &QPushButton::clicked, this, &DocGeneratorDialog::onSelectNone);
    
    layout->addWidget(tree_group);
    layout->addStretch();
    
    tab_widget_->addTab(tab, tr("Options"));
}

void DocGeneratorDialog::createPreviewTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);
    
    preview_edit_ = new QTextEdit(this);
    preview_edit_->setReadOnly(true);
    layout->addWidget(preview_edit_);
    
    tab_widget_->addTab(tab, tr("Preview"));
}

void DocGeneratorDialog::createOutputTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);
    layout->setSpacing(12);
    
    // Output format
    auto* format_layout = new QHBoxLayout();
    format_layout->addWidget(new QLabel(tr("Output Format:")));
    format_combo_ = new QComboBox(this);
    format_combo_->addItem(tr("HTML Document"), static_cast<int>(DocFormat::HTML));
    format_combo_->addItem(tr("Markdown"), static_cast<int>(DocFormat::Markdown));
    format_combo_->addItem(tr("PDF Document"), static_cast<int>(DocFormat::PDF));
    format_combo_->addItem(tr("Word Document"), static_cast<int>(DocFormat::Word));
    format_layout->addWidget(format_combo_);
    format_layout->addStretch();
    layout->addLayout(format_layout);
    
    // Output path
    auto* path_layout = new QHBoxLayout();
    path_layout->addWidget(new QLabel(tr("Output Path:")));
    output_path_edit_ = new QLineEdit(this);
    output_path_edit_->setText(QDir::homePath() + "/database_doc.html");
    path_layout->addWidget(output_path_edit_);
    browse_btn_ = new QPushButton(tr("Browse..."), this);
    path_layout->addWidget(browse_btn_);
    layout->addLayout(path_layout);
    
    // Progress
    progress_bar_ = new QProgressBar(this);
    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    layout->addWidget(progress_bar_);
    
    layout->addStretch();
    
    connect(browse_btn_, &QPushButton::clicked, this, &DocGeneratorDialog::onBrowseOutput);
    
    tab_widget_->addTab(tab, tr("Output"));
}

DocOptions DocGeneratorDialog::options() const {
    DocOptions opts;
    opts.includeTables = tables_check_->isChecked();
    opts.includeViews = views_check_->isChecked();
    opts.includeStoredProcs = procs_check_->isChecked();
    opts.includeFunctions = funcs_check_->isChecked();
    opts.includeTriggers = triggers_check_->isChecked();
    opts.includeIndexes = indexes_check_->isChecked();
    opts.includeRelationships = relationships_check_->isChecked();
    opts.includeSampleData = sample_data_check_->isChecked();
    opts.includeDiagrams = diagrams_check_->isChecked();
    opts.format = static_cast<DocFormat>(format_combo_->currentData().toInt());
    opts.outputPath = output_path_edit_->text();
    opts.title = title_edit_->text();
    return opts;
}

void DocGeneratorDialog::onGenerate() {
    DocOptions opts = options();
    
    // Simulate generation progress
    progress_bar_->setValue(0);
    for (int i = 0; i <= 100; i += 10) {
        progress_bar_->setValue(i);
        QApplication::processEvents();
        QThread::msleep(50);
    }
    
    QString content = generateDocumentation();
    
    QFile file(opts.outputPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << content;
        file.close();
        
        QMessageBox::information(this, tr("Success"),
            tr("Documentation generated successfully:\n%1").arg(opts.outputPath));
        accept();
    } else {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to write file:\n%1").arg(opts.outputPath));
    }
}

void DocGeneratorDialog::onBrowseOutput() {
    DocFormat fmt = static_cast<DocFormat>(format_combo_->currentData().toInt());
    QString filter;
    QString defaultExt;
    
    switch (fmt) {
        case DocFormat::HTML:
            filter = tr("HTML Files (*.html *.htm)");
            defaultExt = ".html";
            break;
        case DocFormat::Markdown:
            filter = tr("Markdown Files (*.md)");
            defaultExt = ".md";
            break;
        case DocFormat::PDF:
            filter = tr("PDF Files (*.pdf)");
            defaultExt = ".pdf";
            break;
        case DocFormat::Word:
            filter = tr("Word Documents (*.docx)");
            defaultExt = ".docx";
            break;
    }
    
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Documentation"),
                                                    output_path_edit_->text(),
                                                    filter);
    if (!filename.isEmpty()) {
        if (!filename.endsWith(defaultExt, Qt::CaseInsensitive)) {
            filename += defaultExt;
        }
        output_path_edit_->setText(filename);
    }
}

void DocGeneratorDialog::onFormatChanged(int index) {
    Q_UNUSED(index)
    // Update default extension in path
    QString path = output_path_edit_->text();
    DocFormat fmt = static_cast<DocFormat>(format_combo_->currentData().toInt());
    
    // Remove old extension and add new one
    int dotPos = path.lastIndexOf('.');
    if (dotPos > 0) {
        path = path.left(dotPos);
    }
    
    switch (fmt) {
        case DocFormat::HTML: path += ".html"; break;
        case DocFormat::Markdown: path += ".md"; break;
        case DocFormat::PDF: path += ".pdf"; break;
        case DocFormat::Word: path += ".docx"; break;
    }
    
    output_path_edit_->setText(path);
}

void DocGeneratorDialog::onSelectAll() {
    for (int i = 0; i < objects_tree_->topLevelItemCount(); ++i) {
        objects_tree_->topLevelItem(i)->setCheckState(0, Qt::Checked);
    }
}

void DocGeneratorDialog::onSelectNone() {
    for (int i = 0; i < objects_tree_->topLevelItemCount(); ++i) {
        objects_tree_->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    }
}

void DocGeneratorDialog::onPreview() {
    tab_widget_->setCurrentIndex(1); // Switch to preview tab
    updatePreview();
}

void DocGeneratorDialog::updatePreview() {
    QString html = generateDocumentation();
    preview_edit_->setHtml(html);
}

QString DocGeneratorDialog::generateDocumentation() {
    QString title = title_edit_->text();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    
    QString html;
    html += "<!DOCTYPE html>\n";
    html += "<html>\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<title>" + title + "</title>\n";
    html += R"(
<style>
body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
.container { max-width: 1200px; margin: 0 auto; background: white; padding: 40px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
h1 { color: #2c3e50; border-bottom: 3px solid #3498db; padding-bottom: 10px; }
h2 { color: #34495e; border-bottom: 2px solid #95a5a6; padding-bottom: 8px; margin-top: 30px; }
h3 { color: #7f8c8d; }
table { border-collapse: collapse; width: 100%; margin: 15px 0; }
th, td { border: 1px solid #ddd; padding: 10px; text-align: left; }
th { background: #34495e; color: white; }
tr:nth-child(even) { background: #f9f9f9; }
.code { background: #f4f4f4; padding: 15px; border-left: 4px solid #3498db; font-family: Consolas, monospace; overflow-x: auto; }
.metadata { color: #7f8c8d; font-size: 0.9em; margin-bottom: 30px; }
.section { margin-bottom: 40px; }
</style>
)";
    html += "</head>\n<body>\n";
    html += "<div class=\"container\">\n";
    
    // Title
    html += "<h1>" + title + "</h1>\n";
    html += "<p class=\"metadata\">Generated: " + timestamp + "</p>\n";
    
    // Tables Section
    if (tables_check_->isChecked()) {
        html += "<div class=\"section\">\n";
        html += "<h2>Tables</h2>\n";
        
        html += generateTableDoc("users");
        html += generateTableDoc("orders");
        html += generateTableDoc("products");
        
        html += "</div>\n";
    }
    
    // Views Section
    if (views_check_->isChecked()) {
        html += "<div class=\"section\">\n";
        html += "<h2>Views</h2>\n";
        html += "<h3>v_active_orders</h3>\n";
        html += "<p>Shows all active orders with user information.</p>\n";
        html += "</div>\n";
    }
    
    // Stored Procedures
    if (procs_check_->isChecked()) {
        html += "<div class=\"section\">\n";
        html += "<h2>Stored Procedures</h2>\n";
        html += "<h3>sp_get_user_orders</h3>\n";
        html += "<div class=\"code\">CREATE PROCEDURE sp_get_user_orders(IN user_id INT)</div>\n";
        html += "</div>\n";
    }
    
    // Relationships / ER Diagram
    if (relationships_check_->isChecked() && diagrams_check_->isChecked()) {
        html += "<div class=\"section\">\n";
        html += "<h2>Entity Relationship Diagram</h2>\n";
        html += generateDiagramHtml();
        html += "</div>\n";
    }
    
    html += "</div>\n";
    html += "</body>\n</html>";
    
    return html;
}

QString DocGeneratorDialog::generateTableDoc(const QString& tableName) {
    QString html;
    html += "<h3>" + tableName + "</h3>\n";
    
    if (tableName == "users") {
        html += "<p>Stores user account information.</p>\n";
        html += "<table>\n";
        html += "<tr><th>Column</th><th>Type</th><th>Nullable</th><th>Default</th><th>Description</th></tr>\n";
        html += "<tr><td>id</td><td>INTEGER</td><td>No</td><td>AUTO_INCREMENT</td><td>Primary key</td></tr>\n";
        html += "<tr><td>username</td><td>VARCHAR(50)</td><td>No</td><td></td><td>Unique username</td></tr>\n";
        html += "<tr><td>email</td><td>VARCHAR(100)</td><td>No</td><td></td><td>User email address</td></tr>\n";
        html += "<tr><td>created_at</td><td>TIMESTAMP</td><td>No</td><td>CURRENT_TIMESTAMP</td><td>Account creation time</td></tr>\n";
        html += "</table>\n";
    } else if (tableName == "orders") {
        html += "<p>Stores customer orders.</p>\n";
        html += "<table>\n";
        html += "<tr><th>Column</th><th>Type</th><th>Nullable</th><th>Default</th><th>Description</th></tr>\n";
        html += "<tr><td>id</td><td>INTEGER</td><td>No</td><td>AUTO_INCREMENT</td><td>Primary key</td></tr>\n";
        html += "<tr><td>user_id</td><td>INTEGER</td><td>No</td><td></td><td>Foreign key to users</td></tr>\n";
        html += "<tr><td>total</td><td>DECIMAL(10,2)</td><td>No</td><td>0.00</td><td>Order total</td></tr>\n";
        html += "<tr><td>status</td><td>VARCHAR(20)</td><td>No</td><td>'pending'</td><td>Order status</td></tr>\n";
        html += "</table>\n";
    } else {
        html += "<p>Table documentation for " + tableName + ".</p>\n";
        html += "<table>\n";
        html += "<tr><th>Column</th><th>Type</th><th>Description</th></tr>\n";
        html += "<tr><td>id</td><td>INTEGER</td><td>Primary key</td></tr>\n";
        html += "<tr><td>name</td><td>VARCHAR</td><td>Name field</td></tr>\n";
        html += "</table>\n";
    }
    
    return html;
}

QString DocGeneratorDialog::generateDiagramHtml() {
    // Return a placeholder SVG representation
    return R"(
<div style="text-align: center; padding: 20px; background: #f9f9f9; border: 1px solid #ddd;">
    <svg width="600" height="400" xmlns="http://www.w3.org/2000/svg">
        <!-- users table -->
        <rect x="50" y="50" width="150" height="100" fill="#fff" stroke="#333" stroke-width="2"/>
        <rect x="50" y="50" width="150" height="25" fill="#4682B4"/>
        <text x="125" y="67" text-anchor="middle" fill="white" font-weight="bold">users</text>
        <text x="60" y="95" font-family="monospace" font-size="12">id PK</text>
        <text x="60" y="112" font-family="monospace" font-size="12">username</text>
        <text x="60" y="129" font-family="monospace" font-size="12">email</text>
        
        <!-- orders table -->
        <rect x="300" y="50" width="150" height="100" fill="#fff" stroke="#333" stroke-width="2"/>
        <rect x="300" y="50" width="150" height="25" fill="#4682B4"/>
        <text x="375" y="67" text-anchor="middle" fill="white" font-weight="bold">orders</text>
        <text x="310" y="95" font-family="monospace" font-size="12">id PK</text>
        <text x="310" y="112" font-family="monospace" font-size="12">user_id FK</text>
        <text x="310" y="129" font-family="monospace" font-size="12">total</text>
        
        <!-- Relationship line -->
        <line x1="200" y1="95" x2="300" y2="95" stroke="#666" stroke-width="2"/>
        <circle cx="250" cy="95" r="4" fill="#666"/>
        <text x="250" y="85" text-anchor="middle" font-size="10" fill="#666">1:N</text>
        
        <!-- products table -->
        <rect x="300" y="200" width="150" height="80" fill="#fff" stroke="#333" stroke-width="2"/>
        <rect x="300" y="200" width="150" height="25" fill="#4682B4"/>
        <text x="375" y="217" text-anchor="middle" fill="white" font-weight="bold">products</text>
        <text x="310" y="245" font-family="monospace" font-size="12">id PK</text>
        <text x="310" y="262" font-family="monospace" font-size="12">name</text>
        
        <!-- Legend -->
        <text x="50" y="350" font-weight="bold">Legend:</text>
        <line x1="50" y1="365" x2="100" y2="365" stroke="#666" stroke-width="2"/>
        <text x="110" y="369" font-size="12">Foreign Key Relationship</text>
        <rect x="50" y="375" width="20" height="15" fill="#4682B4"/>
        <text x="80" y="387" font-size="12">Table Header</text>
    </svg>
    <p><em>Interactive ER Diagram - Export to PDF for high-quality vector graphics</em></p>
</div>
)";
}

} // namespace scratchrobin::ui
