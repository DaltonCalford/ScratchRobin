#include "code_formatter.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>

namespace scratchrobin::ui {

// ============================================================================
// SQL Formatter
// ============================================================================

SqlCodeFormatter::SqlCodeFormatter(const FormatOptions& options)
    : options_(options) {
}

QString SqlCodeFormatter::format(const QString& sql) {
    if (sql.isEmpty()) return sql;
    
    QString result = sql;
    
    // Simple formatting rules
    // 1. Normalize whitespace
    result = result.simplified();
    
    // 2. Apply keyword casing
    QStringList keywords = {"SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE",
                           "JOIN", "LEFT", "RIGHT", "INNER", "OUTER", "ON", "AND", "OR",
                           "GROUP", "BY", "ORDER", "HAVING", "LIMIT", "OFFSET", "UNION",
                           "ALL", "DISTINCT", "AS", "CREATE", "TABLE", "INDEX", "DROP",
                           "ALTER", "ADD", "COLUMN", "PRIMARY", "KEY", "FOREIGN", "REFERENCES"};
    
    for (const QString& kw : keywords) {
        QRegularExpression re("\\b" + kw + "\\b", QRegularExpression::CaseInsensitiveOption);
        QString replacement = options_.uppercaseKeywords ? kw : kw.toLower();
        result.replace(re, replacement);
    }
    
    // 3. Add newlines before major clauses
    if (options_.newlineBeforeFrom) {
        result.replace(QRegularExpression("\\s+FROM\\s+", QRegularExpression::CaseInsensitiveOption), "\nFROM ");
    }
    if (options_.newlineBeforeWhere) {
        result.replace(QRegularExpression("\\s+WHERE\\s+", QRegularExpression::CaseInsensitiveOption), "\nWHERE ");
    }
    if (options_.newlineBeforeGroupBy) {
        result.replace(QRegularExpression("\\s+GROUP\\s+BY\\s+", QRegularExpression::CaseInsensitiveOption), "\nGROUP BY ");
    }
    if (options_.newlineBeforeOrderBy) {
        result.replace(QRegularExpression("\\s+ORDER\\s+BY\\s+", QRegularExpression::CaseInsensitiveOption), "\nORDER BY ");
    }
    if (options_.newlineBeforeJoin) {
        result.replace(QRegularExpression("\\s+(LEFT|RIGHT|INNER|OUTER)?\\s*JOIN\\s+", QRegularExpression::CaseInsensitiveOption), "\n\\1 JOIN ");
    }
    
    // 4. Handle SELECT list items
    if (options_.newlineAfterSelectItem) {
        // Add newline after each comma in SELECT list
        // This is a simplified version
    }
    
    // 5. Apply indentation
    if (!options_.useTabs) {
        QString indent(options_.indentSize, ' ');
        QStringList lines = result.split('\n');
        for (int i = 1; i < lines.size(); ++i) {
            lines[i] = indent + lines[i];
        }
        result = lines.join('\n');
    }
    
    // 6. Space after comma
    if (options_.spaceAfterComma) {
        result.replace(",", ", ");
        result.replace(",  ", ", "); // Fix double spaces
    }
    
    // 7. Space around operators
    if (options_.spaceAroundOperators) {
        QStringList ops = {"=", "<>", "<", ">", "<=", ">=", "+", "-", "*", "/"};
        for (const QString& op : ops) {
            QString pattern = "\\s*" + QRegularExpression::escape(op) + "\\s*";
            result.replace(QRegularExpression(pattern), " " + op + " ");
        }
    }
    
    return result;
}

QString SqlCodeFormatter::formatFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QString content = file.readAll();
    file.close();
    
    return format(content);
}

QString SqlCodeFormatter::quickFormat(const QString& sql) {
    FormatOptions defaultOptions;
    SqlCodeFormatter formatter(defaultOptions);
    return formatter.format(sql);
}

QString SqlCodeFormatter::minify(const QString& sql) {
    QString result = sql;
    // Remove all unnecessary whitespace
    result = result.simplified();
    // Remove spaces around operators while preserving functionality
    return result;
}

QString SqlCodeFormatter::addComments(const QString& sql) {
    // Add explanatory comments to SQL
    Q_UNUSED(sql)
    return sql;
}

QString SqlCodeFormatter::removeComments(const QString& sql) {
    // Remove all comments from SQL
    QString result = sql;
    // Remove single-line comments
    result.remove(QRegularExpression("--[^\\n]*"));
    // Remove multi-line comments
    result.remove(QRegularExpression("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption));
    return result;
}

// ============================================================================
// Code Formatter Panel
// ============================================================================

CodeFormatterPanel::CodeFormatterPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("code_formatter", parent)
    , client_(client) {
    setupUi();
    loadPresets();
}

void CodeFormatterPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    presetCombo_ = new QComboBox(this);
    presetCombo_->addItems({tr("Default"), tr("Compact"), tr("Extended"), tr("Custom")});
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CodeFormatterPanel::onLoadPreset);
    toolbarLayout->addWidget(new QLabel(tr("Preset:"), this));
    toolbarLayout->addWidget(presetCombo_);
    
    toolbarLayout->addSpacing(20);
    
    auto* formatBtn = new QPushButton(tr("Format"), this);
    connect(formatBtn, &QPushButton::clicked, this, &CodeFormatterPanel::onFormatSql);
    toolbarLayout->addWidget(formatBtn);
    
    auto* minifyBtn = new QPushButton(tr("Minify"), this);
    connect(minifyBtn, &QPushButton::clicked, this, &CodeFormatterPanel::onMinify);
    toolbarLayout->addWidget(minifyBtn);
    
    toolbarLayout->addStretch();
    
    auto* openBtn = new QPushButton(tr("Open File"), this);
    connect(openBtn, &QPushButton::clicked, this, &CodeFormatterPanel::onOpenFile);
    toolbarLayout->addWidget(openBtn);
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &CodeFormatterPanel::onSaveFile);
    toolbarLayout->addWidget(saveBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Options panel
    setupOptionsPanel();
    auto* optionsWidget = new QWidget(this);
    auto* optionsLayout = new QVBoxLayout(optionsWidget);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->addWidget(indentationGroup_);
    optionsLayout->addWidget(casingGroup_);
    optionsLayout->addWidget(spacingGroup_);
    optionsLayout->addWidget(lineBreakGroup_);
    optionsLayout->addStretch();
    
    auto* btnLayout = new QHBoxLayout();
    auto* resetBtn = new QPushButton(tr("Reset"), this);
    connect(resetBtn, &QPushButton::clicked, this, &CodeFormatterPanel::onResetDefaults);
    btnLayout->addWidget(resetBtn);
    optionsLayout->addLayout(btnLayout);
    
    splitter->addWidget(optionsWidget);
    
    // Right: Preview panel
    setupPreviewPanel();
    auto* previewWidget = new QWidget(this);
    auto* previewLayout = new QVBoxLayout(previewWidget);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    
    auto* previewTabs = new QTabWidget(this);
    previewTabs->addTab(inputEdit_, tr("Input"));
    previewTabs->addTab(outputEdit_, tr("Formatted"));
    previewLayout->addWidget(previewTabs);
    
    splitter->addWidget(previewWidget);
    splitter->setSizes({300, 500});
    
    mainLayout->addWidget(splitter, 1);
}

void CodeFormatterPanel::setupOptionsPanel() {
    // Indentation
    indentationGroup_ = new QGroupBox(tr("Indentation"), this);
    auto* indentLayout = new QFormLayout(indentationGroup_);
    
    useTabsCheck_ = new QCheckBox(tr("Use tabs instead of spaces"), this);
    indentSizeSpin_ = new QSpinBox(this);
    indentSizeSpin_->setRange(2, 8);
    indentSizeSpin_->setValue(4);
    indentSizeSpin_->setSuffix(tr(" spaces"));
    
    indentLayout->addRow(useTabsCheck_);
    indentLayout->addRow(tr("Indent size:"), indentSizeSpin_);
    
    // Casing
    casingGroup_ = new QGroupBox(tr("Casing"), this);
    auto* casingLayout = new QVBoxLayout(casingGroup_);
    
    uppercaseKeywordsCheck_ = new QCheckBox(tr("Uppercase keywords"), this);
    uppercaseKeywordsCheck_->setChecked(true);
    uppercaseFunctionsCheck_ = new QCheckBox(tr("Uppercase functions"), this);
    uppercaseDataTypesCheck_ = new QCheckBox(tr("Uppercase data types"), this);
    lowercaseIdentifiersCheck_ = new QCheckBox(tr("Lowercase identifiers"), this);
    lowercaseIdentifiersCheck_->setChecked(true);
    
    casingLayout->addWidget(uppercaseKeywordsCheck_);
    casingLayout->addWidget(uppercaseFunctionsCheck_);
    casingLayout->addWidget(uppercaseDataTypesCheck_);
    casingLayout->addWidget(lowercaseIdentifiersCheck_);
    
    // Spacing
    spacingGroup_ = new QGroupBox(tr("Spacing"), this);
    auto* spacingLayout = new QVBoxLayout(spacingGroup_);
    
    spaceAfterCommaCheck_ = new QCheckBox(tr("Space after comma"), this);
    spaceAfterCommaCheck_->setChecked(true);
    spaceAroundOperatorsCheck_ = new QCheckBox(tr("Space around operators"), this);
    spaceAroundOperatorsCheck_->setChecked(true);
    spaceBeforeParenCheck_ = new QCheckBox(tr("Space before parenthesis"), this);
    spaceInsideParenCheck_ = new QCheckBox(tr("Space inside parenthesis"), this);
    
    spacingLayout->addWidget(spaceAfterCommaCheck_);
    spacingLayout->addWidget(spaceAroundOperatorsCheck_);
    spacingLayout->addWidget(spaceBeforeParenCheck_);
    spacingLayout->addWidget(spaceInsideParenCheck_);
    
    // Line breaks
    lineBreakGroup_ = new QGroupBox(tr("Line Breaks"), this);
    auto* lineBreakLayout = new QVBoxLayout(lineBreakGroup_);
    
    newlineBeforeFromCheck_ = new QCheckBox(tr("Newline before FROM"), this);
    newlineBeforeFromCheck_->setChecked(true);
    newlineBeforeWhereCheck_ = new QCheckBox(tr("Newline before WHERE"), this);
    newlineBeforeWhereCheck_->setChecked(true);
    newlineBeforeGroupByCheck_ = new QCheckBox(tr("Newline before GROUP BY"), this);
    newlineBeforeGroupByCheck_->setChecked(true);
    newlineBeforeOrderByCheck_ = new QCheckBox(tr("Newline before ORDER BY"), this);
    newlineBeforeOrderByCheck_->setChecked(true);
    newlineBeforeJoinCheck_ = new QCheckBox(tr("Newline before JOIN"), this);
    newlineBeforeJoinCheck_->setChecked(true);
    newlineAfterSelectItemCheck_ = new QCheckBox(tr("Newline after each SELECT item"), this);
    
    maxLineLengthSpin_ = new QSpinBox(this);
    maxLineLengthSpin_->setRange(60, 200);
    maxLineLengthSpin_->setValue(120);
    maxLineLengthSpin_->setSuffix(tr(" chars"));
    
    lineBreakLayout->addWidget(newlineBeforeFromCheck_);
    lineBreakLayout->addWidget(newlineBeforeWhereCheck_);
    lineBreakLayout->addWidget(newlineBeforeGroupByCheck_);
    lineBreakLayout->addWidget(newlineBeforeOrderByCheck_);
    lineBreakLayout->addWidget(newlineBeforeJoinCheck_);
    lineBreakLayout->addWidget(newlineAfterSelectItemCheck_);
    
    auto* maxLineLayout = new QHBoxLayout();
    maxLineLayout->addWidget(new QLabel(tr("Max line length:"), this));
    maxLineLayout->addWidget(maxLineLengthSpin_);
    maxLineLayout->addStretch();
    lineBreakLayout->addLayout(maxLineLayout);
    
    // Connect all options to update signal
    QList<QCheckBox*> checks = {
        useTabsCheck_, uppercaseKeywordsCheck_, uppercaseFunctionsCheck_,
        uppercaseDataTypesCheck_, lowercaseIdentifiersCheck_, spaceAfterCommaCheck_,
        spaceAroundOperatorsCheck_, spaceBeforeParenCheck_, spaceInsideParenCheck_,
        newlineBeforeFromCheck_, newlineBeforeWhereCheck_, newlineBeforeGroupByCheck_,
        newlineBeforeOrderByCheck_, newlineBeforeJoinCheck_, newlineAfterSelectItemCheck_
    };
    for (auto* check : checks) {
        connect(check, &QCheckBox::toggled, this, &CodeFormatterPanel::onOptionsChanged);
    }
    connect(indentSizeSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CodeFormatterPanel::onOptionsChanged);
    connect(maxLineLengthSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CodeFormatterPanel::onOptionsChanged);
}

void CodeFormatterPanel::setupPreviewPanel() {
    inputEdit_ = new QTextEdit(this);
    inputEdit_->setFont(QFont("Consolas", 10));
    inputEdit_->setPlaceholderText(tr("Enter SQL to format..."));
    
    outputEdit_ = new QTextEdit(this);
    outputEdit_->setFont(QFont("Consolas", 10));
    outputEdit_->setReadOnly(true);
    outputEdit_->setPlaceholderText(tr("Formatted SQL will appear here..."));
}

void CodeFormatterPanel::loadPresets() {
    // Load presets from settings
}

void CodeFormatterPanel::updatePreview() {
    // Auto-format on option change if enabled
}

void CodeFormatterPanel::applyFormatting() {
    SqlCodeFormatter formatter(currentOptions_);
    formattedSql_ = formatter.format(originalSql_);
    outputEdit_->setPlainText(formattedSql_);
}

void CodeFormatterPanel::onFormatSql() {
    originalSql_ = inputEdit_->toPlainText();
    
    // Update options from UI
    currentOptions_.useTabs = useTabsCheck_->isChecked();
    currentOptions_.indentSize = indentSizeSpin_->value();
    currentOptions_.uppercaseKeywords = uppercaseKeywordsCheck_->isChecked();
    currentOptions_.uppercaseFunctions = uppercaseFunctionsCheck_->isChecked();
    currentOptions_.uppercaseDataTypes = uppercaseDataTypesCheck_->isChecked();
    currentOptions_.lowercaseIdentifiers = lowercaseIdentifiersCheck_->isChecked();
    currentOptions_.spaceAfterComma = spaceAfterCommaCheck_->isChecked();
    currentOptions_.spaceAroundOperators = spaceAroundOperatorsCheck_->isChecked();
    currentOptions_.spaceBeforeParenthesis = spaceBeforeParenCheck_->isChecked();
    currentOptions_.spaceInsideParenthesis = spaceInsideParenCheck_->isChecked();
    currentOptions_.newlineBeforeFrom = newlineBeforeFromCheck_->isChecked();
    currentOptions_.newlineBeforeWhere = newlineBeforeWhereCheck_->isChecked();
    currentOptions_.newlineBeforeGroupBy = newlineBeforeGroupByCheck_->isChecked();
    currentOptions_.newlineBeforeOrderBy = newlineBeforeOrderByCheck_->isChecked();
    currentOptions_.newlineBeforeJoin = newlineBeforeJoinCheck_->isChecked();
    currentOptions_.newlineAfterSelectItem = newlineAfterSelectItemCheck_->isChecked();
    currentOptions_.maxLineLength = maxLineLengthSpin_->value();
    
    applyFormatting();
    emit sqlFormatted(formattedSql_);
}

void CodeFormatterPanel::onFormatSelection() {
    // Format selected text in editor
}

void CodeFormatterPanel::onMinify() {
    originalSql_ = inputEdit_->toPlainText();
    formattedSql_ = SqlCodeFormatter::minify(originalSql_);
    outputEdit_->setPlainText(formattedSql_);
}

void CodeFormatterPanel::onBeautify() {
    // Apply beautification with extra spacing
    onFormatSql();
}

void CodeFormatterPanel::onOpenFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open SQL File"),
        QString(),
        tr("SQL Files (*.sql);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            inputEdit_->setPlainText(file.readAll());
            file.close();
        }
    }
}

void CodeFormatterPanel::onSaveFile() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Formatted SQL"),
        QString(),
        tr("SQL Files (*.sql);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(outputEdit_->toPlainText().toUtf8());
            file.close();
            
            QMessageBox::information(this, tr("Save"),
                tr("File saved successfully."));
        }
    }
}

void CodeFormatterPanel::onSaveFormatted() {
    onSaveFile();
}

void CodeFormatterPanel::onOptionsChanged() {
    // Auto-format if enabled
}

void CodeFormatterPanel::onLoadPreset() {
    int preset = presetCombo_->currentIndex();
    
    switch (preset) {
    case 0: // Default
        useTabsCheck_->setChecked(false);
        indentSizeSpin_->setValue(4);
        uppercaseKeywordsCheck_->setChecked(true);
        spaceAfterCommaCheck_->setChecked(true);
        spaceAroundOperatorsCheck_->setChecked(true);
        newlineBeforeFromCheck_->setChecked(true);
        newlineBeforeWhereCheck_->setChecked(true);
        break;
    case 1: // Compact
        indentSizeSpin_->setValue(2);
        spaceAfterCommaCheck_->setChecked(false);
        newlineBeforeFromCheck_->setChecked(false);
        newlineBeforeWhereCheck_->setChecked(false);
        break;
    case 2: // Extended
        indentSizeSpin_->setValue(4);
        newlineBeforeFromCheck_->setChecked(true);
        newlineBeforeWhereCheck_->setChecked(true);
        newlineBeforeGroupByCheck_->setChecked(true);
        newlineBeforeOrderByCheck_->setChecked(true);
        newlineBeforeJoinCheck_->setChecked(true);
        newlineAfterSelectItemCheck_->setChecked(true);
        break;
    case 3: // Custom - keep current settings
        break;
    }
}

void CodeFormatterPanel::onSavePreset() {
    QMessageBox::information(this, tr("Save Preset"),
        tr("Save current settings as preset - not yet implemented."));
}

void CodeFormatterPanel::onResetDefaults() {
    presetCombo_->setCurrentIndex(0);
    onLoadPreset();
}

void CodeFormatterPanel::onToggleCompare() {
    compareMode_ = !compareMode_;
    // Toggle compare view
}

void CodeFormatterPanel::onShowDiff() {
    // Show diff between input and output
}

void CodeFormatterPanel::onFormatClipboard() {
    QClipboard* clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    if (!text.isEmpty()) {
        inputEdit_->setPlainText(text);
        onFormatSql();
        clipboard->setText(outputEdit_->toPlainText());
        QMessageBox::information(this, tr("Clipboard"),
            tr("Formatted SQL copied to clipboard."));
    }
}

void CodeFormatterPanel::onFormatEditorContent() {
    // Format current editor content
}

// ============================================================================
// Format Options Dialog
// ============================================================================

FormatOptionsDialog::FormatOptionsDialog(FormatOptions* options, QWidget* parent)
    : QDialog(parent)
    , options_(options)
    , workingOptions_(*options) {
    setupUi();
    loadOptions();
}

void FormatOptionsDialog::setupUi() {
    setWindowTitle(tr("Format Options"));
    resize(500, 600);
    
    auto* layout = new QVBoxLayout(this);
    
    tabs_ = new QTabWidget(this);
    
    setupIndentationTab();
    setupCasingTab();
    setupSpacingTab();
    setupLineBreaksTab();
    setupAlignmentTab();
    
    layout->addWidget(tabs_, 1);
    
    // Preview
    layout->addWidget(new QLabel(tr("Preview:"), this));
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setFont(QFont("Consolas", 9));
    previewEdit_->setMaximumHeight(100);
    layout->addWidget(previewEdit_);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &FormatOptionsDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &FormatOptionsDialog::onPreview);
    btnBox->addButton(previewBtn, QDialogButtonBox::ActionRole);
    
    layout->addWidget(btnBox);
}

void FormatOptionsDialog::setupIndentationTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QFormLayout(widget);
    
    indentSizeSpin_ = new QSpinBox(this);
    indentSizeSpin_->setRange(2, 8);
    layout->addRow(tr("Indent size:"), indentSizeSpin_);
    
    useTabsCheck_ = new QCheckBox(tr("Use tabs"), this);
    layout->addRow(useTabsCheck_);
    
    tabs_->addTab(widget, tr("Indentation"));
}

void FormatOptionsDialog::setupCasingTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    uppercaseKeywordsCheck_ = new QCheckBox(tr("Uppercase keywords"), this);
    uppercaseFunctionsCheck_ = new QCheckBox(tr("Uppercase functions"), this);
    uppercaseDataTypesCheck_ = new QCheckBox(tr("Uppercase data types"), this);
    lowercaseIdentifiersCheck_ = new QCheckBox(tr("Lowercase identifiers"), this);
    
    layout->addWidget(uppercaseKeywordsCheck_);
    layout->addWidget(uppercaseFunctionsCheck_);
    layout->addWidget(uppercaseDataTypesCheck_);
    layout->addWidget(lowercaseIdentifiersCheck_);
    layout->addStretch();
    
    tabs_->addTab(widget, tr("Casing"));
}

void FormatOptionsDialog::setupSpacingTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    spaceAfterCommaCheck_ = new QCheckBox(tr("Space after comma"), this);
    spaceAroundOperatorsCheck_ = new QCheckBox(tr("Space around operators"), this);
    spaceBeforeParenCheck_ = new QCheckBox(tr("Space before parenthesis"), this);
    spaceInsideParenCheck_ = new QCheckBox(tr("Space inside parenthesis"), this);
    
    layout->addWidget(spaceAfterCommaCheck_);
    layout->addWidget(spaceAroundOperatorsCheck_);
    layout->addWidget(spaceBeforeParenCheck_);
    layout->addWidget(spaceInsideParenCheck_);
    layout->addStretch();
    
    tabs_->addTab(widget, tr("Spacing"));
}

void FormatOptionsDialog::setupLineBreaksTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    newlineBeforeFromCheck_ = new QCheckBox(tr("Newline before FROM"), this);
    newlineBeforeWhereCheck_ = new QCheckBox(tr("Newline before WHERE"), this);
    newlineBeforeGroupByCheck_ = new QCheckBox(tr("Newline before GROUP BY"), this);
    newlineBeforeOrderByCheck_ = new QCheckBox(tr("Newline before ORDER BY"), this);
    newlineAfterSelectItemCheck_ = new QCheckBox(tr("Newline after each SELECT item"), this);
    
    maxLineLengthSpin_ = new QSpinBox(this);
    maxLineLengthSpin_->setRange(60, 200);
    maxLineLengthSpin_->setSuffix(tr(" chars"));
    
    layout->addWidget(newlineBeforeFromCheck_);
    layout->addWidget(newlineBeforeWhereCheck_);
    layout->addWidget(newlineBeforeGroupByCheck_);
    layout->addWidget(newlineBeforeOrderByCheck_);
    layout->addWidget(newlineAfterSelectItemCheck_);
    
    auto* hLayout = new QHBoxLayout();
    hLayout->addWidget(new QLabel(tr("Max line length:"), this));
    hLayout->addWidget(maxLineLengthSpin_);
    hLayout->addStretch();
    layout->addLayout(hLayout);
    layout->addStretch();
    
    tabs_->addTab(widget, tr("Line Breaks"));
}

void FormatOptionsDialog::setupAlignmentTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    alignSelectListCheck_ = new QCheckBox(tr("Align SELECT list"), this);
    alignWhereConditionsCheck_ = new QCheckBox(tr("Align WHERE conditions"), this);
    
    layout->addWidget(alignSelectListCheck_);
    layout->addWidget(alignWhereConditionsCheck_);
    layout->addStretch();
    
    tabs_->addTab(widget, tr("Alignment"));
}

void FormatOptionsDialog::loadOptions() {
    useTabsCheck_->setChecked(workingOptions_.useTabs);
    indentSizeSpin_->setValue(workingOptions_.indentSize);
    uppercaseKeywordsCheck_->setChecked(workingOptions_.uppercaseKeywords);
    uppercaseFunctionsCheck_->setChecked(workingOptions_.uppercaseFunctions);
    uppercaseDataTypesCheck_->setChecked(workingOptions_.uppercaseDataTypes);
    lowercaseIdentifiersCheck_->setChecked(workingOptions_.lowercaseIdentifiers);
    spaceAfterCommaCheck_->setChecked(workingOptions_.spaceAfterComma);
    spaceAroundOperatorsCheck_->setChecked(workingOptions_.spaceAroundOperators);
    spaceBeforeParenCheck_->setChecked(workingOptions_.spaceBeforeParenthesis);
    spaceInsideParenCheck_->setChecked(workingOptions_.spaceInsideParenthesis);
    newlineBeforeFromCheck_->setChecked(workingOptions_.newlineBeforeFrom);
    newlineBeforeWhereCheck_->setChecked(workingOptions_.newlineBeforeWhere);
    newlineBeforeGroupByCheck_->setChecked(workingOptions_.newlineBeforeGroupBy);
    newlineBeforeOrderByCheck_->setChecked(workingOptions_.newlineBeforeOrderBy);
    newlineAfterSelectItemCheck_->setChecked(workingOptions_.newlineAfterSelectItem);
    maxLineLengthSpin_->setValue(workingOptions_.maxLineLength);
    alignSelectListCheck_->setChecked(workingOptions_.alignSelectList);
    alignWhereConditionsCheck_->setChecked(workingOptions_.alignWhereConditions);
}

void FormatOptionsDialog::onAccept() {
    *options_ = workingOptions_;
    accept();
}

void FormatOptionsDialog::onReset() {
    workingOptions_ = FormatOptions();
    loadOptions();
}

void FormatOptionsDialog::onPreview() {
    // Show preview with current options
    previewEdit_->setPlainText("SELECT id, name\nFROM customers\nWHERE status = 'active';");
}

// ============================================================================
// Batch Format Dialog
// ============================================================================

BatchFormatDialog::BatchFormatDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void BatchFormatDialog::setupUi() {
    setWindowTitle(tr("Batch Format"));
    resize(600, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Files list
    layout->addWidget(new QLabel(tr("Files to format:"), this));
    filesList_ = new QListWidget(this);
    layout->addWidget(filesList_, 1);
    
    // File buttons
    auto* fileBtnLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("Add Files"), this);
    connect(addBtn, &QPushButton::clicked, this, &BatchFormatDialog::onAddFiles);
    fileBtnLayout->addWidget(addBtn);
    
    auto* removeBtn = new QPushButton(tr("Remove"), this);
    connect(removeBtn, &QPushButton::clicked, this, &BatchFormatDialog::onRemoveFiles);
    fileBtnLayout->addWidget(removeBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &BatchFormatDialog::onClearFiles);
    fileBtnLayout->addWidget(clearBtn);
    
    fileBtnLayout->addStretch();
    layout->addLayout(fileBtnLayout);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    
    outputDirEdit_ = new QLineEdit(this);
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &BatchFormatDialog::onSelectOutputDir);
    auto* dirLayout = new QHBoxLayout();
    dirLayout->addWidget(outputDirEdit_, 1);
    dirLayout->addWidget(browseBtn);
    optionsLayout->addRow(tr("Output directory:"), dirLayout);
    
    formatPresetCombo_ = new QComboBox(this);
    formatPresetCombo_->addItems({tr("Default"), tr("Compact"), tr("Extended")});
    optionsLayout->addRow(tr("Format preset:"), formatPresetCombo_);
    
    backupCheck_ = new QCheckBox(tr("Create backup files"), this);
    backupCheck_->setChecked(true);
    optionsLayout->addRow(backupCheck_);
    
    recursiveCheck_ = new QCheckBox(tr("Process subdirectories"), this);
    optionsLayout->addRow(recursiveCheck_);
    
    layout->addWidget(optionsGroup);
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* formatBtn = new QPushButton(tr("Format"), this);
    connect(formatBtn, &QPushButton::clicked, this, &BatchFormatDialog::onFormat);
    btnLayout->addWidget(formatBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void BatchFormatDialog::onAddFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this,
        tr("Select SQL Files"),
        QString(),
        tr("SQL Files (*.sql);;All Files (*.*)"));
    
    for (const QString& file : files) {
        filesList_->addItem(file);
        inputFiles_.append(file);
    }
}

void BatchFormatDialog::onRemoveFiles() {
    auto item = filesList_->currentItem();
    if (item) {
        int row = filesList_->row(item);
        inputFiles_.removeAt(row);
        delete item;
    }
}

void BatchFormatDialog::onClearFiles() {
    filesList_->clear();
    inputFiles_.clear();
}

void BatchFormatDialog::onSelectOutputDir() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));
    if (!dir.isEmpty()) {
        outputDir_ = dir;
        outputDirEdit_->setText(dir);
    }
}

void BatchFormatDialog::onFormat() {
    if (inputFiles_.isEmpty()) {
        QMessageBox::warning(this, tr("No Files"),
            tr("Please add files to format."));
        return;
    }
    
    processFiles();
    accept();
}

void BatchFormatDialog::processFiles() {
    int successCount = 0;
    int failCount = 0;
    
    FormatOptions options;
    SqlCodeFormatter formatter(options);
    
    for (const QString& file : inputFiles_) {
        QFileInfo fi(file);
        QString outputFile = outputDir_ + "/" + fi.fileName();
        
        QString formatted = formatter.formatFile(file);
        if (!formatted.isEmpty()) {
            if (backupCheck_->isChecked()) {
                QFile::rename(file, file + ".bak");
            }
            
            QFile out(outputFile);
            if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
                out.write(formatted.toUtf8());
                out.close();
                successCount++;
            } else {
                failCount++;
            }
        } else {
            failCount++;
        }
    }
    
    QMessageBox::information(this, tr("Batch Format Complete"),
        tr("Successfully formatted %1 files.\nFailed: %2")
        .arg(successCount).arg(failCount));
}

void BatchFormatDialog::onCancel() {
    reject();
}

} // namespace scratchrobin::ui
