#include "ui/type_renderer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QTextStream>
#include <QSettings>

namespace scratchrobin::ui {

// ============================================================================
// TypeRenderer
// ============================================================================

TypeRenderer::TypeRenderer(const TypeRenderPreferences& prefs) : prefs_(prefs) {
}

void TypeRenderer::setPreferences(const TypeRenderPreferences& prefs) {
    prefs_ = prefs;
}

QString TypeRenderer::render(const QVariant& value, const QString& typeName) const {
    if (value.isNull()) {
        return renderNull();
    }
    
    // Detect type if not specified
    QString actualType = typeName.isEmpty() ? detectType(value) : typeName;
    actualType = actualType.toLower();
    
    // Route to appropriate renderer
    if (isDateTimeType(actualType)) {
        if (actualType.contains("timestamp")) {
            return renderDateTime(value.toDateTime());
        } else if (actualType.contains("date")) {
            return renderDate(value.toDate());
        } else if (actualType.contains("time")) {
            return renderTime(value.toTime());
        }
    }
    
    if (isNumericType(actualType)) {
        if (actualType.contains("money") || actualType.contains("currency")) {
            return renderMoney(value.toString());
        }
        return renderDecimal(value.toString());
    }
    
    if (isJsonType(actualType)) {
        return renderJson(value.toString());
    }
    
    if (isArrayType(actualType)) {
        return renderArray(value.toString());
    }
    
    if (isBlobType(actualType)) {
        return renderBlob(value.toByteArray());
    }
    
    if (actualType == "bool" || actualType == "boolean") {
        return renderBoolean(value.toBool());
    }
    
    // Default: return as string
    return value.toString();
}

QString TypeRenderer::renderDate(const QDate& date) const {
    if (!date.isValid()) return QString();
    return date.toString(prefs_.dateFormat);
}

QString TypeRenderer::renderTime(const QTime& time) const {
    if (!time.isValid()) return QString();
    return time.toString(prefs_.timeFormat);
}

QString TypeRenderer::renderDateTime(const QDateTime& dateTime) const {
    if (!dateTime.isValid()) return QString();
    QString result = dateTime.toString(prefs_.dateTimeFormat);
    if (prefs_.showTimeZone) {
        result += " " + dateTime.timeZoneAbbreviation();
    }
    return result;
}

QString TypeRenderer::renderDecimal(const QString& value) const {
    bool ok;
    double num = value.toDouble(&ok);
    if (!ok) return value;
    return formatNumber(num);
}

QString TypeRenderer::renderMoney(const QString& value) const {
    bool ok;
    double num = value.toDouble(&ok);
    if (!ok) return value;
    
    QString formatted = formatNumber(num);
    if (prefs_.showCurrencySymbol) {
        if (prefs_.currencySymbolBefore) {
            formatted = prefs_.currencySymbol + formatted;
        } else {
            formatted = formatted + prefs_.currencySymbol;
        }
    }
    return formatted;
}

QString TypeRenderer::renderJson(const QString& json) const {
    if (json.isEmpty()) return QString();
    
    QString result = json;
    if (prefs_.jsonPrettyPrint) {
        result = prettifyJson(json);
    }
    if (prefs_.jsonSyntaxHighlight) {
        result = highlightJson(result);
    }
    return result;
}

QString TypeRenderer::renderArray(const QString& array) const {
    QStringList elements = ArrayFormatter::parse(array);
    return formatArray(elements);
}

QString TypeRenderer::renderBlob(const QByteArray& data) const {
    if (data.isEmpty()) return QString();
    
    if (prefs_.blobAsHex) {
        return bytesToHex(data.left(prefs_.maxBlobDisplayLength));
    }
    
    // Try to interpret as text
    QString text = QString::fromUtf8(data);
    if (text.isEmpty()) {
        return bytesToHex(data.left(prefs_.maxBlobDisplayLength));
    }
    return text.left(prefs_.maxBlobDisplayLength);
}

QString TypeRenderer::renderBoolean(bool value) const {
    QString result = value ? prefs_.trueDisplay : prefs_.falseDisplay;
    if (prefs_.booleanUppercase) {
        result = result.toUpper();
    }
    return result;
}

QString TypeRenderer::renderNull() const {
    QString result = prefs_.nullDisplay;
    // Note: styling (italic/gray) is applied by the display widget
    return result;
}

QString TypeRenderer::formatNumber(double value) const {
    QString result = QString::number(value, 'f', prefs_.decimalPlaces);
    
    if (prefs_.useThousandsSeparator) {
        // Split into integer and decimal parts
        int decimalPos = result.indexOf('.');
        QString integerPart = decimalPos >= 0 ? result.left(decimalPos) : result;
        QString decimalPart = decimalPos >= 0 ? result.mid(decimalPos) : QString();
        
        // Add thousands separators
        QString withSeparators;
        int count = 0;
        for (int i = integerPart.length() - 1; i >= 0; --i) {
            if (count > 0 && count % 3 == 0) {
                withSeparators = prefs_.thousandsSeparator + withSeparators;
            }
            withSeparators = integerPart[i] + withSeparators;
            count++;
        }
        
        result = withSeparators + decimalPart;
    }
    
    // Replace decimal point if needed
    if (prefs_.decimalSeparator != ".") {
        result.replace(".", prefs_.decimalSeparator);
    }
    
    return result;
}

QString TypeRenderer::formatNumber(const QString& value) const {
    bool ok;
    double num = value.toDouble(&ok);
    if (ok) {
        return formatNumber(num);
    }
    return value;
}

QString TypeRenderer::prettifyJson(const QString& json) const {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull()) return json;
    return doc.toJson(QJsonDocument::Indented);
}

QString TypeRenderer::highlightJson(const QString& json) const {
    // Simple syntax highlighting using HTML
    QString highlighted = json;
    highlighted.replace("&", "&amp;");
    highlighted.replace("<", "&lt;");
    highlighted.replace(">", "&gt;");
    
    // Note: Full JSON highlighting would require a proper parser
    // This is a simplified version that escapes HTML
    
    return highlighted;
}

QString TypeRenderer::formatArray(const QStringList& elements) const {
    QString result = prefs_.arrayStart;
    
    int count = 0;
    for (const QString& elem : elements) {
        if (count >= prefs_.maxArrayElements) {
            result += "...";
            break;
        }
        if (count > 0) {
            result += prefs_.arrayElementSeparator;
        }
        result += elem;
        count++;
    }
    
    result += prefs_.arrayEnd;
    return result;
}

QString TypeRenderer::bytesToHex(const QByteArray& data) const {
    QString result = "0x";
    for (uchar c : data) {
        result += QString("%1").arg(c, 2, 16, QChar('0'));
    }
    return result;
}

QString TypeRenderer::detectType(const QVariant& value) {
    switch (value.typeId()) {
        case QMetaType::QDate:
            return "date";
        case QMetaType::QTime:
            return "time";
        case QMetaType::QDateTime:
            return "timestamp";
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Double:
        case QMetaType::Float:
            return "numeric";
        case QMetaType::Bool:
            return "boolean";
        case QMetaType::QByteArray:
            return "blob";
        default:
            return "text";
    }
}

bool TypeRenderer::isDateTimeType(const QString& typeName) {
    QString lower = typeName.toLower();
    return lower.contains("date") || lower.contains("time") || lower.contains("timestamp");
}

bool TypeRenderer::isNumericType(const QString& typeName) {
    QString lower = typeName.toLower();
    static const QStringList numericTypes = {
        "int", "integer", "bigint", "smallint", "tinyint",
        "decimal", "numeric", "float", "double", "real",
        "money", "currency"
    };
    return numericTypes.contains(lower);
}

bool TypeRenderer::isJsonType(const QString& typeName) {
    QString lower = typeName.toLower();
    return lower.contains("json");
}

bool TypeRenderer::isArrayType(const QString& typeName) {
    QString lower = typeName.toLower();
    return lower.contains("array") || lower.contains("[]");
}

bool TypeRenderer::isBlobType(const QString& typeName) {
    QString lower = typeName.toLower();
    return lower.contains("blob") || lower.contains("binary") || lower.contains("bytea");
}

bool TypeRenderer::isValidDate(const QString& value) {
    return DateTimeParser::parseDate(value).isValid();
}

bool TypeRenderer::isValidDateTime(const QString& value) {
    return DateTimeParser::parse(value).isValid();
}

bool TypeRenderer::isValidJson(const QString& value) {
    QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());
    return !doc.isNull();
}

// ============================================================================
// DateTimeParser
// ============================================================================

const QStringList DateTimeParser::DATE_FORMATS = {
    "yyyy-MM-dd",
    "dd/MM/yyyy",
    "MM/dd/yyyy",
    "dd.MM.yyyy",
    "yyyy/MM/dd"
};

const QStringList DateTimeParser::TIME_FORMATS = {
    "hh:mm:ss",
    "h:mm:ss AP",
    "hh:mm:ss.zzz",
    "HH:mm:ss"
};

const QStringList DateTimeParser::DATETIME_FORMATS = {
    "yyyy-MM-dd hh:mm:ss",
    "yyyy-MM-dd hh:mm:ss.zzz",
    "dd/MM/yyyy hh:mm:ss",
    "MM/dd/yyyy hh:mm:ss",
    "yyyy-MM-ddThh:mm:ss"
};

QDateTime DateTimeParser::parse(const QString& value) {
    for (const QString& format : DATETIME_FORMATS) {
        QDateTime dt = QDateTime::fromString(value, format);
        if (dt.isValid()) return dt;
    }
    return QDateTime();
}

QDate DateTimeParser::parseDate(const QString& value) {
    for (const QString& format : DATE_FORMATS) {
        QDate date = QDate::fromString(value, format);
        if (date.isValid()) return date;
    }
    return QDate();
}

QTime DateTimeParser::parseTime(const QString& value) {
    for (const QString& format : TIME_FORMATS) {
        QTime time = QTime::fromString(value, format);
        if (time.isValid()) return time;
    }
    return QTime();
}

QStringList DateTimeParser::supportedFormats() {
    QStringList all;
    all.append(DATE_FORMATS);
    all.append(TIME_FORMATS);
    all.append(DATETIME_FORMATS);
    return all;
}

// ============================================================================
// JsonFormatter
// ============================================================================

QString JsonFormatter::format(const QString& json, const FormatOptions& options) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull()) return json;
    
    if (options.prettyPrint) {
        return doc.toJson(QJsonDocument::Indented);
    } else {
        return doc.toJson(QJsonDocument::Compact);
    }
}

QString JsonFormatter::minify(const QString& json) {
    FormatOptions opts;
    opts.prettyPrint = false;
    opts.sortKeys = false;
    opts.indentSize = 0;
    opts.escapeUnicode = false;
    return format(json, opts);
}

bool JsonFormatter::validate(const QString& json) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return !doc.isNull();
}

QStringList JsonFormatter::extractPaths(const QString& json) {
    QStringList paths;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull()) return paths;
    
    // Would implement recursive path extraction
    return paths;
}

QVariant JsonFormatter::extractValue(const QString& json, const QString& path) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull()) return QVariant();
    
    // Simple path parsing (e.g., "user.name")
    QStringList parts = path.split('.');
    QJsonValue value = doc.object();
    
    for (const QString& part : parts) {
        if (value.isObject()) {
            value = value.toObject().value(part);
        } else if (value.isArray()) {
            bool ok;
            int index = part.toInt(&ok);
            if (ok && index >= 0 && index < value.toArray().size()) {
                value = value.toArray()[index];
            } else {
                return QVariant();
            }
        } else {
            return QVariant();
        }
    }
    
    return value.toVariant();
}

QString JsonFormatter::highlight(const QString& json) {
    // Simple syntax highlighting using HTML
    QString highlighted = json;
    highlighted.replace("&", "&amp;");
    highlighted.replace("<", "&lt;");
    highlighted.replace(">", "&gt;");
    
    // Highlight keys (strings followed by colon)
    highlighted.replace(QRegularExpression("(\"[^\"]+\")\\s*:"), 
                       "<span style=\"color: #2196F3;\">\\1</span>:");
    
    // Highlight string values
    highlighted.replace(QRegularExpression("(\"[^\"]+\")"), 
                       "<span style=\"color: #4CAF50;\">\\1</span>");
    
    // Highlight numbers
    highlighted.replace(QRegularExpression("\\b(\\d+\\.?\\d*)\\b"), 
                       "<span style=\"color: #FF9800;\">\\1</span>");
    
    // Highlight booleans and null
    highlighted.replace(QRegularExpression("\\b(true|false|null)\\b"), 
                       "<span style=\"color: #9C27B0;\">\\1</span>");
    
    return highlighted;
}

// ============================================================================
// NumberFormatter
// ============================================================================

QString NumberFormatter::format(double value, const FormatOptions& options) {
    QString result = QString::number(value, 'f', options.decimalPlaces);
    
    if (options.useThousandsSeparator) {
        int decimalPos = result.indexOf('.');
        QString intPart = decimalPos >= 0 ? result.left(decimalPos) : result;
        QString decPart = decimalPos >= 0 ? result.mid(decimalPos + 1) : QString();
        
        QString withSep;
        int count = 0;
        for (int i = intPart.length() - 1; i >= 0; --i) {
            if (count > 0 && count % 3 == 0) {
                withSep = options.thousandsSeparator + withSep;
            }
            withSep = intPart[i] + withSep;
            count++;
        }
        
        result = withSep;
        if (!decPart.isEmpty()) {
            result += options.decimalSeparator + decPart;
        }
    } else if (options.decimalSeparator != '.') {
        result.replace(".", options.decimalSeparator);
    }
    
    return result;
}

QString NumberFormatter::format(const QString& value, const FormatOptions& options) {
    bool ok;
    double num = value.toDouble(&ok);
    if (ok) {
        return format(num, options);
    }
    return value;
}

QString NumberFormatter::formatCurrency(double value, const QString& symbol, 
                                        bool symbolBefore, int decimalPlaces) {
    FormatOptions opts;
    opts.decimalPlaces = decimalPlaces;
    opts.useThousandsSeparator = true;
    QString formatted = format(value, opts);
    
    if (symbolBefore) {
        return symbol + formatted;
    } else {
        return formatted + symbol;
    }
}

QString NumberFormatter::formatPercent(double value, int decimalPlaces) {
    FormatOptions opts;
    opts.decimalPlaces = decimalPlaces;
    return format(value, opts) + "%";
}

QString NumberFormatter::formatScientific(double value, int decimalPlaces) {
    return QString::number(value, 'e', decimalPlaces);
}

double NumberFormatter::parse(const QString& value) {
    QString cleaned = value;
    cleaned.remove(',');
    cleaned.remove('$');
    cleaned.remove('%');
    cleaned.remove(' ');
    return cleaned.toDouble();
}

bool NumberFormatter::isValidNumber(const QString& value) {
    bool ok;
    parse(value);
    return ok;
}

// ============================================================================
// ArrayFormatter
// ============================================================================

QStringList ArrayFormatter::parse(const QString& array, const QString& delimiter) {
    if (array.startsWith('[') && array.endsWith(']')) {
        QString content = array.mid(1, array.length() - 2);
        return content.split(delimiter, Qt::SkipEmptyParts);
    }
    return array.split(delimiter, Qt::SkipEmptyParts);
}

QString ArrayFormatter::format(const QStringList& elements, const QString& start,
                               const QString& end, const QString& separator) {
    return start + elements.join(separator) + end;
}

QString ArrayFormatter::format(const QVariantList& elements, const TypeRenderer& renderer) {
    QStringList rendered;
    for (const QVariant& elem : elements) {
        rendered.append(renderer.render(elem));
    }
    return format(rendered);
}

QStringList ArrayFormatter::parsePostgresArray(const QString& array) {
    // PostgreSQL array format: {a,b,c} or {"a","b","c"}
    QStringList result;
    if (!array.startsWith('{') || !array.endsWith('}')) {
        return result;
    }
    
    QString content = array.mid(1, array.length() - 2);
    // Handle quoted elements
    bool inQuote = false;
    QString current;
    for (int i = 0; i < content.length(); ++i) {
        QChar c = content[i];
        if (c == '"' && (i == 0 || content[i-1] != '\\')) {
            inQuote = !inQuote;
        } else if (c == ',' && !inQuote) {
            result.append(current.trimmed());
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.isEmpty()) {
        result.append(current.trimmed());
    }
    
    return result;
}

QString ArrayFormatter::formatPostgresArray(const QStringList& elements) {
    QStringList escaped;
    for (const QString& elem : elements) {
        if (elem.contains(',') || elem.contains('"') || elem.contains('\\')) {
            QString copy = elem;
            escaped.append("\"" + copy.replace("\\", "\\\\").replace("\"", "\\\"") + "\"");
        } else {
            escaped.append(elem);
        }
    }
    return "{" + escaped.join(",") + "}";
}

// ============================================================================
// TypeRenderPreferencesDialog
// ============================================================================

TypeRenderPreferencesDialog::TypeRenderPreferencesDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Type Rendering Preferences"));
    setMinimumSize(600, 500);
    
    setupUi();
    
    // Load defaults
    QSettings settings;
    prefs_.dateFormat = settings.value("TypeRender/DateFormat", prefs_.dateFormat).toString();
    prefs_.decimalPlaces = settings.value("TypeRender/DecimalPlaces", prefs_.decimalPlaces).toInt();
}

void TypeRenderPreferencesDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Tab widget
    auto* tabWidget = new QTabWidget(this);
    
    // Date/Time tab
    setupDateTimeTab();
    tabWidget->addTab(dateFormatEdit_->parentWidget(), tr("Date/Time"));
    
    // Number tab
    setupNumberTab();
    tabWidget->addTab(decimalPlacesSpin_->parentWidget(), tr("Numbers"));
    
    // JSON tab
    setupJsonTab();
    tabWidget->addTab(jsonPrettyCheck_->parentWidget(), tr("JSON"));
    
    // Array tab
    setupArrayTab();
    tabWidget->addTab(arrayStartEdit_->parentWidget(), tr("Arrays"));
    
    // NULL tab
    setupNullTab();
    tabWidget->addTab(nullDisplayEdit_->parentWidget(), tr("NULL"));
    
    mainLayout->addWidget(tabWidget);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    auto* previewLabel = new QLabel(tr("Sample output will appear here"), this);
    previewLayout->addWidget(previewLabel);
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset,
        this);
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked,
            this, &TypeRenderPreferencesDialog::onReset);
}

void TypeRenderPreferencesDialog::setupDateTimeTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QGridLayout(widget);
    
    layout->addWidget(new QLabel(tr("Date Format:"), this), 0, 0);
    dateFormatEdit_ = new QLineEdit(prefs_.dateFormat, this);
    layout->addWidget(dateFormatEdit_, 0, 1);
    
    layout->addWidget(new QLabel(tr("Time Format:"), this), 1, 0);
    timeFormatEdit_ = new QLineEdit(prefs_.timeFormat, this);
    layout->addWidget(timeFormatEdit_, 1, 1);
    
    layout->addWidget(new QLabel(tr("DateTime Format:"), this), 2, 0);
    dateTimeFormatEdit_ = new QLineEdit(prefs_.dateTimeFormat, this);
    layout->addWidget(dateTimeFormatEdit_, 2, 1);
    
    useLocaleCheck_ = new QCheckBox(tr("Use system locale"), this);
    useLocaleCheck_->setChecked(prefs_.useSystemLocale);
    layout->addWidget(useLocaleCheck_, 3, 0, 1, 2);
    
    showTimezoneCheck_ = new QCheckBox(tr("Show timezone"), this);
    showTimezoneCheck_->setChecked(prefs_.showTimeZone);
    layout->addWidget(showTimezoneCheck_, 4, 0, 1, 2);
    
    datePreview_ = new QLabel(this);
    datePreview_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    layout->addWidget(new QLabel(tr("Preview:"), this), 5, 0);
    layout->addWidget(datePreview_, 5, 1);
    
    layout->setRowStretch(6, 1);
}

void TypeRenderPreferencesDialog::setupNumberTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QGridLayout(widget);
    
    layout->addWidget(new QLabel(tr("Decimal Places:"), this), 0, 0);
    decimalPlacesSpin_ = new QSpinBox(this);
    decimalPlacesSpin_->setRange(0, 10);
    decimalPlacesSpin_->setValue(prefs_.decimalPlaces);
    layout->addWidget(decimalPlacesSpin_, 0, 1);
    
    layout->addWidget(new QLabel(tr("Decimal Separator:"), this), 1, 0);
    decimalSepEdit_ = new QLineEdit(prefs_.decimalSeparator, this);
    layout->addWidget(decimalSepEdit_, 1, 1);
    
    layout->addWidget(new QLabel(tr("Thousands Separator:"), this), 2, 0);
    thousandsSepEdit_ = new QLineEdit(prefs_.thousandsSeparator, this);
    layout->addWidget(thousandsSepEdit_, 2, 1);
    
    useThousandsCheck_ = new QCheckBox(tr("Use thousands separator"), this);
    useThousandsCheck_->setChecked(prefs_.useThousandsSeparator);
    layout->addWidget(useThousandsCheck_, 3, 0, 1, 2);
    
    layout->addWidget(new QLabel(tr("Currency Symbol:"), this), 4, 0);
    currencySymbolEdit_ = new QLineEdit(prefs_.currencySymbol, this);
    layout->addWidget(currencySymbolEdit_, 4, 1);
    
    currencyBeforeCheck_ = new QCheckBox(tr("Symbol before value"), this);
    currencyBeforeCheck_->setChecked(prefs_.currencySymbolBefore);
    layout->addWidget(currencyBeforeCheck_, 5, 0, 1, 2);
    
    numberPreview_ = new QLabel(this);
    numberPreview_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    layout->addWidget(new QLabel(tr("Preview:"), this), 6, 0);
    layout->addWidget(numberPreview_, 6, 1);
    
    layout->setRowStretch(7, 1);
}

void TypeRenderPreferencesDialog::setupJsonTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    jsonPrettyCheck_ = new QCheckBox(tr("Pretty print JSON"), this);
    jsonPrettyCheck_->setChecked(prefs_.jsonPrettyPrint);
    layout->addWidget(jsonPrettyCheck_);
    
    jsonHighlightCheck_ = new QCheckBox(tr("Syntax highlight JSON"), this);
    jsonHighlightCheck_->setChecked(prefs_.jsonSyntaxHighlight);
    layout->addWidget(jsonHighlightCheck_);
    
    auto* indentLayout = new QHBoxLayout();
    indentLayout->addWidget(new QLabel(tr("Indent Size:"), this));
    jsonIndentSpin_ = new QSpinBox(this);
    jsonIndentSpin_->setRange(1, 8);
    jsonIndentSpin_->setValue(prefs_.jsonIndentSize);
    indentLayout->addWidget(jsonIndentSpin_);
    indentLayout->addStretch();
    layout->addLayout(indentLayout);
    
    jsonPreview_ = new QTextEdit(this);
    jsonPreview_->setReadOnly(true);
    jsonPreview_->setPlainText("{\n  \"key\": \"value\"\n}");
    layout->addWidget(new QLabel(tr("Preview:"), this));
    layout->addWidget(jsonPreview_);
}

void TypeRenderPreferencesDialog::setupArrayTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QGridLayout(widget);
    
    layout->addWidget(new QLabel(tr("Array Start:"), this), 0, 0);
    arrayStartEdit_ = new QLineEdit(prefs_.arrayStart, this);
    layout->addWidget(arrayStartEdit_, 0, 1);
    
    layout->addWidget(new QLabel(tr("Array End:"), this), 1, 0);
    arrayEndEdit_ = new QLineEdit(prefs_.arrayEnd, this);
    layout->addWidget(arrayEndEdit_, 1, 1);
    
    layout->addWidget(new QLabel(tr("Element Separator:"), this), 2, 0);
    arraySepEdit_ = new QLineEdit(prefs_.arrayElementSeparator, this);
    layout->addWidget(arraySepEdit_, 2, 1);
    
    arrayPreview_ = new QLabel("[a, b, c]", this);
    arrayPreview_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    layout->addWidget(new QLabel(tr("Preview:"), this), 3, 0);
    layout->addWidget(arrayPreview_, 3, 1);
    
    layout->setRowStretch(4, 1);
}

void TypeRenderPreferencesDialog::setupNullTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QGridLayout(widget);
    
    layout->addWidget(new QLabel(tr("NULL Display:"), this), 0, 0);
    nullDisplayEdit_ = new QLineEdit(prefs_.nullDisplay, this);
    layout->addWidget(nullDisplayEdit_, 0, 1);
    
    nullItalicCheck_ = new QCheckBox(tr("Italic"), this);
    nullItalicCheck_->setChecked(prefs_.nullItalic);
    layout->addWidget(nullItalicCheck_, 1, 0, 1, 2);
    
    nullGrayedCheck_ = new QCheckBox(tr("Grayed out"), this);
    nullGrayedCheck_->setChecked(prefs_.nullGrayed);
    layout->addWidget(nullGrayedCheck_, 2, 0, 1, 2);
    
    auto* preview = new QLabel(prefs_.nullDisplay, this);
    preview->setStyleSheet("font-style: italic; color: gray;");
    layout->addWidget(new QLabel(tr("Preview:"), this), 3, 0);
    layout->addWidget(preview, 3, 1);
    
    layout->setRowStretch(4, 1);
}

void TypeRenderPreferencesDialog::setPreferences(const TypeRenderPreferences& prefs) {
    prefs_ = prefs;
}

TypeRenderPreferences TypeRenderPreferencesDialog::preferences() const {
    TypeRenderPreferences result;
    
    result.dateFormat = dateFormatEdit_->text();
    result.timeFormat = timeFormatEdit_->text();
    result.dateTimeFormat = dateTimeFormatEdit_->text();
    result.useSystemLocale = useLocaleCheck_->isChecked();
    result.showTimeZone = showTimezoneCheck_->isChecked();
    
    result.decimalPlaces = decimalPlacesSpin_->value();
    result.decimalSeparator = decimalSepEdit_->text();
    result.thousandsSeparator = thousandsSepEdit_->text();
    result.useThousandsSeparator = useThousandsCheck_->isChecked();
    result.currencySymbol = currencySymbolEdit_->text();
    result.currencySymbolBefore = currencyBeforeCheck_->isChecked();
    
    result.jsonPrettyPrint = jsonPrettyCheck_->isChecked();
    result.jsonSyntaxHighlight = jsonHighlightCheck_->isChecked();
    result.jsonIndentSize = jsonIndentSpin_->value();
    
    result.arrayStart = arrayStartEdit_->text();
    result.arrayEnd = arrayEndEdit_->text();
    result.arrayElementSeparator = arraySepEdit_->text();
    
    result.nullDisplay = nullDisplayEdit_->text();
    result.nullItalic = nullItalicCheck_->isChecked();
    result.nullGrayed = nullGrayedCheck_->isChecked();
    
    return result;
}

void TypeRenderPreferencesDialog::onApply() {
    prefs_ = preferences();
    
    // Save to settings
    QSettings settings;
    settings.setValue("TypeRender/DateFormat", prefs_.dateFormat);
    settings.setValue("TypeRender/DecimalPlaces", prefs_.decimalPlaces);
    settings.setValue("TypeRender/NullDisplay", prefs_.nullDisplay);
}

void TypeRenderPreferencesDialog::onReset() {
    prefs_ = TypeRenderPreferences();
    setPreferences(prefs_);
}

void TypeRenderPreferencesDialog::onTabChanged(int index) {
    Q_UNUSED(index)
}

void TypeRenderPreferencesDialog::updatePreview() {
    // Update all preview labels
    TypeRenderPreferences temp = preferences();
    TypeRenderer renderer(temp);
    
    datePreview_->setText(renderer.renderDate(QDate::currentDate()));
    numberPreview_->setText(renderer.renderDecimal("1234567.89"));
    arrayPreview_->setText(renderer.renderArray("a,b,c,d"));
}

// ============================================================================
// DataGridCellRenderer
// ============================================================================

DataGridCellRenderer::DataGridCellRenderer(const TypeRenderPreferences& prefs)
    : TypeRenderer(prefs) {
}

QString DataGridCellRenderer::renderCell(const QVariant& value, const QString& typeName,
                                         int row, int column, bool isSelected, bool isNull) const {
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(isSelected)
    
    if (isNull || value.isNull()) {
        return renderNull();
    }
    
    return render(value, typeName);
}

static bool isBooleanType(const QString& typeName) {
    return typeName == "bool" || typeName == "boolean";
}

Qt::Alignment DataGridCellRenderer::alignmentForType(const QString& typeName) {
    QString lower = typeName.toLower();
    
    if (TypeRenderer::isNumericType(lower)) {
        return Qt::AlignRight | Qt::AlignVCenter;
    }
    
    if (TypeRenderer::isDateTimeType(lower)) {
        return Qt::AlignCenter | Qt::AlignVCenter;
    }
    
    if (isBooleanType(lower)) {
        return Qt::AlignCenter | Qt::AlignVCenter;
    }
    
    return Qt::AlignLeft | Qt::AlignVCenter;
}

QColor DataGridCellRenderer::colorForValue(const QVariant& value, const QString& typeName) {
    if (value.isNull()) {
        return QColor(128, 128, 128);  // Gray for NULL
    }
    
    QString lower = typeName.toLower();
    
    if (lower.contains("error") || lower.contains("fail")) {
        return QColor(255, 200, 200);  // Light red
    }
    
    if (lower.contains("success") || lower.contains("ok")) {
        return QColor(200, 255, 200);  // Light green
    }
    
    if (lower.contains("warning")) {
        return QColor(255, 255, 200);  // Light yellow
    }
    
    return QColor();
}

QString DataGridCellRenderer::tooltipForValue(const QVariant& value, const QString& typeName) const {
    if (value.isNull()) {
        return QStringLiteral("NULL value");
    }
    
    // Show original value in tooltip if different from rendered
    QString rendered = render(value, typeName);
    QString original = value.toString();
    
    if (rendered != original) {
        return QStringLiteral("Original: %1\nRendered: %2").arg(original, rendered);
    }
    
    return original;
}

} // namespace scratchrobin::ui
