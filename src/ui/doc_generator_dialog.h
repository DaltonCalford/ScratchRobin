#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QTextEdit;
class QComboBox;
class QPushButton;
class QCheckBox;
class QLineEdit;
class QProgressBar;
class QTreeWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

enum class DocFormat {
    HTML,
    Markdown,
    PDF,
    Word
};

struct DocOptions {
    bool includeTables = true;
    bool includeViews = true;
    bool includeStoredProcs = true;
    bool includeFunctions = true;
    bool includeTriggers = true;
    bool includeIndexes = true;
    bool includeRelationships = true;
    bool includeSampleData = false;
    bool includeDiagrams = true;
    int sampleDataRows = 5;
    DocFormat format = DocFormat::HTML;
    QString outputPath;
    QString title;
};

class DocGeneratorDialog : public QDialog {
    Q_OBJECT

public:
    explicit DocGeneratorDialog(QWidget* parent = nullptr);
    ~DocGeneratorDialog() override;

    DocOptions options() const;

public slots:
    void onGenerate();
    void onBrowseOutput();
    void onFormatChanged(int index);
    void onSelectAll();
    void onSelectNone();
    void onPreview();

private slots:
    void updatePreview();

private:
    void setupUi();
    void createOptionsTab();
    void createPreviewTab();
    void createOutputTab();
    
    QString generateDocumentation();
    QString generateTableDoc(const QString& tableName);
    QString generateDiagramHtml();
    
    QTabWidget* tab_widget_;
    
    // Options tab
    QTreeWidget* objects_tree_;
    QCheckBox* tables_check_;
    QCheckBox* views_check_;
    QCheckBox* procs_check_;
    QCheckBox* funcs_check_;
    QCheckBox* triggers_check_;
    QCheckBox* indexes_check_;
    QCheckBox* relationships_check_;
    QCheckBox* sample_data_check_;
    QCheckBox* diagrams_check_;
    QComboBox* format_combo_;
    QLineEdit* title_edit_;
    
    // Output tab
    QLineEdit* output_path_edit_;
    QPushButton* browse_btn_;
    QProgressBar* progress_bar_;
    QPushButton* generate_btn_;
    
    // Preview tab
    QTextEdit* preview_edit_;
    
    // Buttons
    QPushButton* preview_btn_;
    QPushButton* cancel_btn_;
};

} // namespace scratchrobin::ui
