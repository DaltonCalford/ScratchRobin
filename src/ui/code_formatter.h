#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QComboBox;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QGroupBox;
class QTabWidget;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QListWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief SQL Code Formatter - SQL beautifier and style tool
 * 
 * Format SQL code according to style preferences:
 * - Indentation control
 * - Keyword casing
 * - Line breaks and spacing
 * - Comma placement
 * - Parenthesis handling
 * - Custom style rules
 */

// ============================================================================
// Format Options
// ============================================================================
struct FormatOptions {
    // Indentation
    bool useTabs = false;
    int indentSize = 4;
    bool indentWithTabs = false;
    
    // Casing
    bool uppercaseKeywords = true;
    bool uppercaseFunctions = false;
    bool uppercaseDataTypes = false;
    bool lowercaseIdentifiers = true;
    
    // Spacing
    bool spaceAfterComma = true;
    bool spaceAroundOperators = true;
    bool spaceBeforeParenthesis = false;
    bool spaceInsideParenthesis = false;
    bool compactParenthesis = false;
    
    // Line breaks
    bool newlineBeforeSelectList = false;
    bool newlineBeforeFrom = true;
    bool newlineBeforeWhere = true;
    bool newlineBeforeGroupBy = true;
    bool newlineBeforeOrderBy = true;
    bool newlineBeforeJoin = true;
    bool newlineAfterSelectItem = true;
    int maxLineLength = 120;
    
    // Alignment
    bool alignSelectList = false;
    bool alignWhereConditions = false;
    bool alignJoinConditions = false;
    
    // Comma placement
    bool commaBeforeItem = false; // true = leading comma, false = trailing
    
    // Comments
    bool preserveComments = true;
    bool formatComments = false;
    
    // Special formatting
    bool compactInLists = false;
    bool expandInLists = false;
    bool compactCaseExpressions = false;
};

// ============================================================================
// SQL Code Formatter
// ============================================================================
class SqlCodeFormatter {
public:
    explicit SqlCodeFormatter(const FormatOptions& options);
    
    QString format(const QString& sql);
    QString formatFile(const QString& filePath);
    
    static QString quickFormat(const QString& sql);
    static QString minify(const QString& sql);
    static QString addComments(const QString& sql);
    static QString removeComments(const QString& sql);
    
private:
    QString formatSelect(const QString& sql);
    QString formatInsert(const QString& sql);
    QString formatUpdate(const QString& sql);
    QString formatDelete(const QString& sql);
    QString formatCreate(const QString& sql);
    QString formatAlter(const QString& sql);
    QString formatDrop(const QString& sql);
    
    QString applyCaseRules(const QString& token);
    QString applyIndentation(const QString& line, int level);
    
    FormatOptions options_;
};

// ============================================================================
// Code Formatter Panel
// ============================================================================
class CodeFormatterPanel : public DockPanel {
    Q_OBJECT

public:
    explicit CodeFormatterPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Code Formatter"); }
    QString panelCategory() const override { return "tools"; }

public slots:
    // Format actions
    void onFormatSql();
    void onFormatSelection();
    void onMinify();
    void onBeautify();
    
    // File operations
    void onOpenFile();
    void onSaveFile();
    void onSaveFormatted();
    
    // Options
    void onOptionsChanged();
    void onLoadPreset();
    void onSavePreset();
    void onResetDefaults();
    
    // Compare
    void onToggleCompare();
    void onShowDiff();
    
    // Batch processing
    void onFormatClipboard();
    void onFormatEditorContent();

signals:
    void sqlFormatted(const QString& formatted);
    void formattingError(const QString& error);

private:
    void setupUi();
    void setupOptionsPanel();
    void setupPreviewPanel();
    void loadPresets();
    void updatePreview();
    void applyFormatting();
    
    backend::SessionClient* client_;
    FormatOptions currentOptions_;
    QString originalSql_;
    QString formattedSql_;
    
    // UI components
    QTabWidget* tabs_ = nullptr;
    
    // Original input
    QTextEdit* inputEdit_ = nullptr;
    
    // Formatted output
    QTextEdit* outputEdit_ = nullptr;
    
    // Compare mode
    bool compareMode_ = false;
    QTextEdit* diffEdit_ = nullptr;
    
    // Options panel
    QGroupBox* indentationGroup_ = nullptr;
    QSpinBox* indentSizeSpin_ = nullptr;
    QCheckBox* useTabsCheck_ = nullptr;
    
    QGroupBox* casingGroup_ = nullptr;
    QCheckBox* uppercaseKeywordsCheck_ = nullptr;
    QCheckBox* uppercaseFunctionsCheck_ = nullptr;
    QCheckBox* uppercaseDataTypesCheck_ = nullptr;
    QCheckBox* lowercaseIdentifiersCheck_ = nullptr;
    
    QGroupBox* spacingGroup_ = nullptr;
    QCheckBox* spaceAfterCommaCheck_ = nullptr;
    QCheckBox* spaceAroundOperatorsCheck_ = nullptr;
    QCheckBox* spaceBeforeParenCheck_ = nullptr;
    QCheckBox* spaceInsideParenCheck_ = nullptr;
    
    QGroupBox* lineBreakGroup_ = nullptr;
    QCheckBox* newlineBeforeFromCheck_ = nullptr;
    QCheckBox* newlineBeforeWhereCheck_ = nullptr;
    QCheckBox* newlineBeforeGroupByCheck_ = nullptr;
    QCheckBox* newlineBeforeOrderByCheck_ = nullptr;
    QCheckBox* newlineBeforeJoinCheck_ = nullptr;
    QCheckBox* newlineAfterSelectItemCheck_ = nullptr;
    QSpinBox* maxLineLengthSpin_ = nullptr;
    
    QGroupBox* alignmentGroup_ = nullptr;
    QCheckBox* alignSelectListCheck_ = nullptr;
    QCheckBox* alignWhereConditionsCheck_ = nullptr;
    
    QGroupBox* commaGroup_ = nullptr;
    QComboBox* commaPlacementCombo_ = nullptr;
    
    QComboBox* presetCombo_ = nullptr;
};

// ============================================================================
// Format Options Dialog
// ============================================================================
class FormatOptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit FormatOptionsDialog(FormatOptions* options, QWidget* parent = nullptr);

public slots:
    void onAccept();
    void onReset();
    void onPreview();

private:
    void setupUi();
    void setupIndentationTab();
    void setupCasingTab();
    void setupSpacingTab();
    void setupLineBreaksTab();
    void setupAlignmentTab();
    void loadOptions();
    
    FormatOptions* options_;
    FormatOptions workingOptions_;
    
    QTabWidget* tabs_ = nullptr;
    
    // Indentation
    QSpinBox* indentSizeSpin_ = nullptr;
    QCheckBox* useTabsCheck_ = nullptr;
    
    // Casing
    QCheckBox* uppercaseKeywordsCheck_ = nullptr;
    QCheckBox* uppercaseFunctionsCheck_ = nullptr;
    QCheckBox* uppercaseDataTypesCheck_ = nullptr;
    QCheckBox* lowercaseIdentifiersCheck_ = nullptr;
    
    // Spacing
    QCheckBox* spaceAfterCommaCheck_ = nullptr;
    QCheckBox* spaceAroundOperatorsCheck_ = nullptr;
    QCheckBox* spaceBeforeParenCheck_ = nullptr;
    QCheckBox* spaceInsideParenCheck_ = nullptr;
    
    // Line breaks
    QCheckBox* newlineBeforeFromCheck_ = nullptr;
    QCheckBox* newlineBeforeWhereCheck_ = nullptr;
    QCheckBox* newlineBeforeGroupByCheck_ = nullptr;
    QCheckBox* newlineBeforeOrderByCheck_ = nullptr;
    QCheckBox* newlineAfterSelectItemCheck_ = nullptr;
    QSpinBox* maxLineLengthSpin_ = nullptr;
    
    // Alignment
    QCheckBox* alignSelectListCheck_ = nullptr;
    QCheckBox* alignWhereConditionsCheck_ = nullptr;
    
    // Preview
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Batch Format Dialog
// ============================================================================
class BatchFormatDialog : public QDialog {
    Q_OBJECT

public:
    explicit BatchFormatDialog(QWidget* parent = nullptr);

public slots:
    void onAddFiles();
    void onRemoveFiles();
    void onClearFiles();
    void onSelectOutputDir();
    void onFormat();
    void onCancel();

private:
    void setupUi();
    void processFiles();
    
    QStringList inputFiles_;
    QString outputDir_;
    
    QListWidget* filesList_ = nullptr;
    QLineEdit* outputDirEdit_ = nullptr;
    QComboBox* formatPresetCombo_ = nullptr;
    QCheckBox* backupCheck_ = nullptr;
    QCheckBox* recursiveCheck_ = nullptr;
};

} // namespace scratchrobin::ui
