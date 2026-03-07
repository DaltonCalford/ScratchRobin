#pragma once
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QHash>
#include <QRegularExpression>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QPushButton;
class QTabWidget;
class QTextEdit;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Type Rendering System
 * 
 * Implements FORM_SPECIFICATION.md Type Rendering section:
 * - Date/Time decoder with multiple formats
 * - Decimal/Money decoder with locale support
 * - JSON/JSONB decoder with syntax highlighting
 * - Array/Composite decoder
 * - Format preferences
 */

// ============================================================================
// Type Render Preferences
// ============================================================================
struct TypeRenderPreferences {
    // Date/Time formats
    QString dateFormat = "yyyy-MM-dd";
    QString timeFormat = "hh:mm:ss";
    QString dateTimeFormat = "yyyy-MM-dd hh:mm:ss";
    QString timestampFormat = "yyyy-MM-dd hh:mm:ss.zzz";
    bool useSystemLocale = false;
    bool showTimeZone = false;
    
    // Number formats
    QString decimalSeparator = ".";
    QString thousandsSeparator = ",";
    int decimalPlaces = 2;
    bool useThousandsSeparator = true;
    
    // Money/Currency
    QString currencySymbol = "$";
    bool currencySymbolBefore = true;
    bool showCurrencySymbol = true;
    
    // JSON
    bool jsonPrettyPrint = true;
    bool jsonSyntaxHighlight = true;
    int jsonIndentSize = 2;
    
    // Arrays
    QString arrayStart = "[";
    QString arrayEnd = "]";
    QString arrayElementSeparator = ", ";
    int maxArrayElements = 100;
    
    // Binary/BLOB
    bool blobAsHex = true;
    int maxBlobDisplayLength = 100;
    
    // NULL display
    QString nullDisplay = "NULL";
    bool nullItalic = true;
    bool nullGrayed = true;
    
    // Boolean
    QString trueDisplay = "true";
    QString falseDisplay = "false";
    bool booleanUppercase = false;
};

// ============================================================================
// Type Renderer Base
// ============================================================================
class TypeRenderer {
public:
    explicit TypeRenderer(const TypeRenderPreferences& prefs = TypeRenderPreferences());
    virtual ~TypeRenderer() = default;
    
    void setPreferences(const TypeRenderPreferences& prefs);
    TypeRenderPreferences preferences() const { return prefs_; }
    
    // Main render function
    virtual QString render(const QVariant& value, const QString& typeName = QString()) const;
    
    // Type-specific renderers
    QString renderDate(const QDate& date) const;
    QString renderTime(const QTime& time) const;
    QString renderDateTime(const QDateTime& dateTime) const;
    QString renderDecimal(const QString& value) const;
    QString renderMoney(const QString& value) const;
    QString renderJson(const QString& json) const;
    QString renderArray(const QString& array) const;
    QString renderBlob(const QByteArray& data) const;
    QString renderBoolean(bool value) const;
    QString renderNull() const;
    
    // Type detection
    static QString detectType(const QVariant& value);
    static bool isDateTimeType(const QString& typeName);
    static bool isNumericType(const QString& typeName);
    static bool isJsonType(const QString& typeName);
    static bool isArrayType(const QString& typeName);
    static bool isBlobType(const QString& typeName);
    
    // Validation
    static bool isValidDate(const QString& value);
    static bool isValidDateTime(const QString& value);
    static bool isValidJson(const QString& value);
    
protected:
    TypeRenderPreferences prefs_;
    
    QString formatNumber(double value) const;
    QString formatNumber(const QString& value) const;
    QString prettifyJson(const QString& json) const;
    QString highlightJson(const QString& json) const;
    QString formatArray(const QStringList& elements) const;
    QString bytesToHex(const QByteArray& data) const;
};

// ============================================================================
// Date/Time Parser
// ============================================================================
class DateTimeParser {
public:
    static QDateTime parse(const QString& value);
    static QDate parseDate(const QString& value);
    static QTime parseTime(const QString& value);
    
    static QStringList supportedFormats();
    
private:
    static const QStringList DATE_FORMATS;
    static const QStringList TIME_FORMATS;
    static const QStringList DATETIME_FORMATS;
};

// ============================================================================
// JSON Formatter
// ============================================================================
class JsonFormatter {
public:
    struct FormatOptions {
        bool prettyPrint;
        bool sortKeys;
        int indentSize;
        bool escapeUnicode;
        
        FormatOptions() : prettyPrint(true), sortKeys(false), indentSize(2), escapeUnicode(false) {}
    };
    
    static QString format(const QString& json, const FormatOptions& options);
    static QString minify(const QString& json);
    static bool validate(const QString& json);
    static QStringList extractPaths(const QString& json);
    static QVariant extractValue(const QString& json, const QString& path);
    
    // Syntax highlighting for display
    static QString highlight(const QString& json);
    
private:
    static QString doFormat(const QJsonValue& value, int indent, const FormatOptions& options);
    static QString escapeString(const QString& str);
};

// ============================================================================
// Number Formatter
// ============================================================================
class NumberFormatter {
public:
    struct FormatOptions {
        int decimalPlaces;
        bool useThousandsSeparator;
        QChar decimalSeparator;
        QChar thousandsSeparator;
        bool padWithZeros;
        
        FormatOptions() : decimalPlaces(2), useThousandsSeparator(true),
                          decimalSeparator('.'), thousandsSeparator(','),
                          padWithZeros(false) {}
    };
    
    static QString format(double value, const FormatOptions& options);
    static QString format(const QString& value, const FormatOptions& options);
    static QString formatCurrency(double value, const QString& symbol = "$", 
                                  bool symbolBefore = true, int decimalPlaces = 2);
    static QString formatPercent(double value, int decimalPlaces = 2);
    static QString formatScientific(double value, int decimalPlaces = 6);
    
    static double parse(const QString& value);
    static bool isValidNumber(const QString& value);
    
private:
    static QString insertThousandsSeparators(const QString& value, QChar separator);
};

// ============================================================================
// Array Parser/Formatter
// ============================================================================
class ArrayFormatter {
public:
    static QStringList parse(const QString& array, const QString& delimiter = ",");
    static QString format(const QStringList& elements, const QString& start = "[",
                          const QString& end = "]", const QString& separator = ", ");
    static QString format(const QVariantList& elements, const TypeRenderer& renderer);
    
    // Multi-dimensional arrays
    static QString parseNested(const QString& array, int& depth);
    static int detectDimensions(const QString& array);
    
    // PostgreSQL array specific
    static QStringList parsePostgresArray(const QString& array);
    static QString formatPostgresArray(const QStringList& elements);
};

// ============================================================================
// Type Render Preferences Dialog
// ============================================================================
class TypeRenderPreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit TypeRenderPreferencesDialog(QWidget* parent = nullptr);

    void setPreferences(const TypeRenderPreferences& prefs);
    TypeRenderPreferences preferences() const;

private slots:
    void onApply();
    void onReset();
    void onTabChanged(int index);
    void updatePreview();

private:
    void setupUi();
    void setupDateTimeTab();
    void setupNumberTab();
    void setupJsonTab();
    void setupArrayTab();
    void setupNullTab();
    
    TypeRenderPreferences prefs_;
    
    // Date/Time tab
    QLineEdit* dateFormatEdit_ = nullptr;
    QLineEdit* timeFormatEdit_ = nullptr;
    QLineEdit* dateTimeFormatEdit_ = nullptr;
    QCheckBox* useLocaleCheck_ = nullptr;
    QCheckBox* showTimezoneCheck_ = nullptr;
    QLabel* datePreview_ = nullptr;
    
    // Number tab
    QSpinBox* decimalPlacesSpin_ = nullptr;
    QLineEdit* decimalSepEdit_ = nullptr;
    QLineEdit* thousandsSepEdit_ = nullptr;
    QCheckBox* useThousandsCheck_ = nullptr;
    QLineEdit* currencySymbolEdit_ = nullptr;
    QCheckBox* currencyBeforeCheck_ = nullptr;
    QLabel* numberPreview_ = nullptr;
    
    // JSON tab
    QCheckBox* jsonPrettyCheck_ = nullptr;
    QCheckBox* jsonHighlightCheck_ = nullptr;
    QSpinBox* jsonIndentSpin_ = nullptr;
    QTextEdit* jsonPreview_ = nullptr;
    
    // Array tab
    QLineEdit* arrayStartEdit_ = nullptr;
    QLineEdit* arrayEndEdit_ = nullptr;
    QLineEdit* arraySepEdit_ = nullptr;
    QLabel* arrayPreview_ = nullptr;
    
    // NULL tab
    QLineEdit* nullDisplayEdit_ = nullptr;
    QCheckBox* nullItalicCheck_ = nullptr;
    QCheckBox* nullGrayedCheck_ = nullptr;
};

// ============================================================================
// Data Grid Cell Renderer (for DataGrid integration)
// ============================================================================
class DataGridCellRenderer : public TypeRenderer {
public:
    explicit DataGridCellRenderer(const TypeRenderPreferences& prefs = TypeRenderPreferences());
    
    // Extended rendering with cell context
    QString renderCell(const QVariant& value, const QString& typeName, 
                       int row, int column, bool isSelected, bool isNull) const;
    
    // Alignment hints
    static Qt::Alignment alignmentForType(const QString& typeName);
    
    // Color coding
    static QColor colorForValue(const QVariant& value, const QString& typeName);
    
    // Tooltip generation
    QString tooltipForValue(const QVariant& value, const QString& typeName) const;
};

} // namespace scratchrobin::ui
