#include "error_details_dialog.h"
#include <QDialogButtonBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTextStream>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStyle>

namespace scratchrobin {

ErrorDetailsDialog::ErrorDetailsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setWindowTitle("Error Details");
    setMinimumSize(600, 500);
    resize(800, 600);

    setupUI();
}

void ErrorDetailsDialog::setError(const ErrorDetails& error) {
    clearErrors();
    addError(error);
    if (!errors_.isEmpty()) {
        onErrorSelected(0);
    }
}

void ErrorDetailsDialog::addError(const ErrorDetails& error) {
    errors_.append(error);

    if (errors_.size() > 1) {
        showMultipleErrors_ = true;
        updateUI();
    }

    // Update error list
    if (showMultipleErrors_ && errorList_) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(error.title);
        item->setIcon(QIcon(getSeverityIcon(error.severity)));
        errorList_->addItem(item);
    }
}

void ErrorDetailsDialog::clearErrors() {
    errors_.clear();
    currentErrorIndex_ = -1;
    showMultipleErrors_ = false;

    if (errorList_) {
        errorList_->clear();
    }

    updateUI();
}

void ErrorDetailsDialog::showError(QWidget* parent, const ErrorDetails& error) {
    ErrorDetailsDialog dialog(parent);
    dialog.setError(error);
    dialog.exec();
}

void ErrorDetailsDialog::showError(QWidget* parent,
                                  const QString& title,
                                  const QString& message,
                                  ErrorSeverity severity) {
    ErrorDetails error;
    error.title = title;
    error.summary = message;
    error.severity = severity;
    error.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    showError(parent, error);
}

void ErrorDetailsDialog::showDatabaseError(QWidget* parent,
                                          const QString& query,
                                          const QString& errorMessage,
                                          const QString& connectionString) {
    ErrorDetails error;
    error.title = "Database Error";
    error.summary = "A database operation failed";
    error.detailedDescription = errorMessage;
    error.category = ErrorCategory::DATABASE;
    error.severity = ErrorSeverity::ERROR;
    error.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    error.technicalDetails = QString("Query: %1\nConnection: %2")
                           .arg(query, connectionString);
    error.suggestedActions = QStringList()
        << "Check database connection"
        << "Verify query syntax"
        << "Check database permissions"
        << "Ensure database server is running";
    error.helpUrl = "help://database-errors";

    showError(parent, error);
}

void ErrorDetailsDialog::showFileError(QWidget* parent,
                                      const QString& filePath,
                                      const QString& operation,
                                      const QString& errorMessage) {
    ErrorDetails error;
    error.title = "File System Error";
    error.summary = QString("Failed to %1 file: %2").arg(operation, QFileInfo(filePath).fileName());
    error.detailedDescription = errorMessage;
    error.category = ErrorCategory::FILESYSTEM;
    error.severity = ErrorSeverity::ERROR;
    error.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    error.technicalDetails = QString("File: %1\nOperation: %2\nError: %3")
                           .arg(filePath, operation, errorMessage);
    error.suggestedActions = QStringList()
        << "Check file permissions"
        << "Verify file path exists"
        << "Ensure sufficient disk space"
        << "Close file if open in another application";
    error.helpUrl = "help://filesystem-errors";

    showError(parent, error);
}

void ErrorDetailsDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(10);
    mainLayout_->setContentsMargins(20, 20, 20, 20);

    setupHeader();
    setupContent();
    setupActions();

    updateUI();
}

void ErrorDetailsDialog::setupHeader() {
    headerWidget_ = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget_);

    // Severity icon
    severityIconLabel_ = new QLabel();
    severityIconLabel_->setFixedSize(32, 32);
    headerLayout->addWidget(severityIconLabel_);

    // Text information
    QVBoxLayout* textLayout = new QVBoxLayout();

    QHBoxLayout* titleRow = new QHBoxLayout();
    titleLabel_ = new QLabel();
    titleLabel_->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c5aa0;");
    titleRow->addWidget(titleLabel_);

    categoryLabel_ = new QLabel();
    categoryLabel_->setStyleSheet("font-size: 10px; color: #666; background-color: #f0f0f0; padding: 2px 6px; border-radius: 3px;");
    titleRow->addWidget(categoryLabel_);
    titleRow->addStretch();

    textLayout->addLayout(titleRow);

    summaryLabel_ = new QLabel();
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setStyleSheet("font-size: 12px; color: #333;");
    textLayout->addWidget(summaryLabel_);

    timestampLabel_ = new QLabel();
    timestampLabel_->setStyleSheet("font-size: 10px; color: #888;");
    textLayout->addWidget(timestampLabel_);

    headerLayout->addLayout(textLayout);
    headerLayout->addStretch();

    mainLayout_->addWidget(headerWidget_);
}

void ErrorDetailsDialog::setupContent() {
    // Create error list for multiple errors
    errorList_ = new QListWidget();
    errorList_->setMaximumWidth(200);
    errorList_->setVisible(false);
    connect(errorList_, &QListWidget::currentRowChanged, this, &ErrorDetailsDialog::onErrorSelected);

    // Create content tabs
    contentTabs_ = new QTabWidget();
    setupOverviewTab();
    setupDetailsTab();
    setupContextTab();
    setupRelatedTab();

    // Layout for content area
    QWidget* contentWidget = new QWidget();
    QHBoxLayout* contentLayout = new QHBoxLayout(contentWidget);

    contentLayout->addWidget(errorList_);
    contentLayout->addWidget(contentTabs_, 1);

    mainLayout_->addWidget(contentWidget);
}

void ErrorDetailsDialog::setupOverviewTab() {
    overviewTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(overviewTab_);

    // Description
    QGroupBox* descriptionGroup = new QGroupBox("Description");
    QVBoxLayout* descLayout = new QVBoxLayout(descriptionGroup);

    descriptionLabel_ = new QLabel();
    descriptionLabel_->setWordWrap(true);
    descriptionLabel_->setStyleSheet("color: #333; line-height: 1.4;");
    descLayout->addWidget(descriptionLabel_);

    layout->addWidget(descriptionGroup);

    // Suggestions
    suggestionsGroup_ = new QGroupBox("Suggested Actions");
    QVBoxLayout* suggestionsLayout = new QVBoxLayout(suggestionsGroup_);

    suggestionsList_ = new QListWidget();
    suggestionsList_->setStyleSheet("QListWidget { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 4px; }"
                                   "QListWidget::item { padding: 8px; border-bottom: 1px solid #e9ecef; }"
                                   "QListWidget::item:selected { background-color: #e3f2fd; color: #1976d2; }");
    suggestionsLayout->addWidget(suggestionsList_);

    layout->addWidget(suggestionsGroup_);

    // Error count label
    errorCountLabel_ = new QLabel();
    errorCountLabel_->setVisible(false);
    errorCountLabel_->setStyleSheet("font-size: 11px; color: #666; font-style: italic;");
    layout->addWidget(errorCountLabel_);

    layout->addStretch();

    contentTabs_->addTab(overviewTab_, "Overview");
}

void ErrorDetailsDialog::setupDetailsTab() {
    detailsTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(detailsTab_);

    // Technical details
    QGroupBox* techGroup = new QGroupBox("Technical Details");
    QVBoxLayout* techLayout = new QVBoxLayout(techGroup);

    technicalDetailsText_ = new QTextEdit();
    technicalDetailsText_->setReadOnly(true);
    technicalDetailsText_->setMaximumHeight(150);
    technicalDetailsText_->setFontFamily("monospace");
    technicalDetailsText_->setStyleSheet("QTextEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; }");
    techLayout->addWidget(technicalDetailsText_);

    layout->addWidget(techGroup);

    // Stack trace
    QGroupBox* stackGroup = new QGroupBox("Stack Trace");
    QVBoxLayout* stackLayout = new QVBoxLayout(stackGroup);

    stackTraceText_ = new QTextEdit();
    stackTraceText_->setReadOnly(true);
    stackTraceText_->setMaximumHeight(200);
    stackTraceText_->setFontFamily("monospace");
    stackTraceText_->setStyleSheet("QTextEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; }");
    stackLayout->addWidget(stackTraceText_);

    layout->addWidget(stackGroup);

    contentTabs_->addTab(detailsTab_, "Technical Details");
}

void ErrorDetailsDialog::setupContextTab() {
    contextTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(contextTab_);

    QGroupBox* contextGroup = new QGroupBox("Context Information");
    QVBoxLayout* contextLayout = new QVBoxLayout(contextGroup);

    contextDataText_ = new QTextEdit();
    contextDataText_->setReadOnly(true);
    contextDataText_->setFontFamily("monospace");
    contextDataText_->setStyleSheet("QTextEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; }");
    contextLayout->addWidget(contextDataText_);

    layout->addWidget(contextGroup);
    layout->addStretch();

    contentTabs_->addTab(contextTab_, "Context");
}

void ErrorDetailsDialog::setupRelatedTab() {
    relatedTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(relatedTab_);

    QGroupBox* relatedGroup = new QGroupBox("Related Errors");
    QVBoxLayout* relatedLayout = new QVBoxLayout(relatedGroup);

    relatedErrorsList_ = new QListWidget();
    relatedErrorsList_->setStyleSheet("QListWidget { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 4px; }"
                                     "QListWidget::item { padding: 8px; border-bottom: 1px solid #e9ecef; }"
                                     "QListWidget::item:hover { background-color: #e9ecef; }");
    relatedLayout->addWidget(relatedErrorsList_);

    layout->addWidget(relatedGroup);
    layout->addStretch();

    contentTabs_->addTab(relatedTab_, "Related");
}

void ErrorDetailsDialog::setupActions() {
    buttonLayout_ = new QHBoxLayout();

    // Left side buttons
    copyButton_ = new QPushButton("Copy Details");
    copyButton_->setIcon(QIcon(":/icons/copy.png"));
    connect(copyButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onCopyToClipboard);
    buttonLayout_->addWidget(copyButton_);

    saveButton_ = new QPushButton("Save Log");
    saveButton_->setIcon(QIcon(":/icons/save.png"));
    connect(saveButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onSaveErrorLog);
    buttonLayout_->addWidget(saveButton_);

    reportButton_ = new QPushButton("Report Error");
    reportButton_->setIcon(QIcon(":/icons/report.png"));
    connect(reportButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onReportError);
    buttonLayout_->addWidget(reportButton_);

    helpButton_ = new QPushButton("Help");
    helpButton_->setIcon(QIcon(":/icons/help.png"));
    connect(helpButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onShowHelp);
    buttonLayout_->addWidget(helpButton_);

    buttonLayout_->addSpacing(20);

    // Navigation buttons (for multiple errors)
    previousButton_ = new QPushButton("Previous");
    previousButton_->setVisible(false);
    connect(previousButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onPreviousError);
    buttonLayout_->addWidget(previousButton_);

    nextButton_ = new QPushButton("Next");
    nextButton_->setVisible(false);
    connect(nextButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onNextError);
    buttonLayout_->addWidget(nextButton_);

    buttonLayout_->addStretch();

    // Right side buttons
    retryButton_ = new QPushButton("Retry");
    retryButton_->setVisible(false);
    connect(retryButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onRetryOperation);
    buttonLayout_->addWidget(retryButton_);

    ignoreButton_ = new QPushButton("Ignore");
    ignoreButton_->setVisible(false);
    connect(ignoreButton_, &QPushButton::clicked, this, &ErrorDetailsDialog::onIgnoreError);
    buttonLayout_->addWidget(ignoreButton_);

    closeButton_ = new QPushButton("Close");
    closeButton_->setDefault(true);
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout_->addWidget(closeButton_);

    mainLayout_->addLayout(buttonLayout_);
}

void ErrorDetailsDialog::updateUI() {
    if (currentErrorIndex_ < 0 || currentErrorIndex_ >= errors_.size()) {
        return;
    }

    const ErrorDetails& error = errors_[currentErrorIndex_];

    // Update header
    updateSeverityIcon();
    titleLabel_->setText(error.title);
    summaryLabel_->setText(error.summary);
    timestampLabel_->setText(formatTimestamp(error.timestamp));
    categoryLabel_->setText(getCategoryIcon(error.category));

    // Update overview tab
    descriptionLabel_->setText(error.detailedDescription.isEmpty() ?
                              error.summary : error.detailedDescription);

    suggestionsList_->clear();
    for (const QString& action : error.suggestedActions) {
        suggestionsList_->addItem("• " + action);
    }

    // Update details tab
    technicalDetailsText_->setPlainText(error.technicalDetails);
    stackTraceText_->setPlainText(error.stackTrace);

    // Update context tab
    QJsonObject contextObj;
    for (auto it = error.contextData.begin(); it != error.contextData.end(); ++it) {
        contextObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    contextDataText_->setPlainText(QJsonDocument(contextObj).toJson(QJsonDocument::Indented));

    // Update related tab
    relatedErrorsList_->clear();
    for (const QString& related : error.relatedErrors) {
        relatedErrorsList_->addItem(related);
    }

    // Update buttons
    retryButton_->setVisible(error.isRecoverable);
    ignoreButton_->setVisible(error.category != ErrorCategory::CRITICAL &&
                             error.category != ErrorCategory::FATAL);
    helpButton_->setVisible(!error.helpUrl.isEmpty());

    // Update navigation
    bool hasMultipleErrors = errors_.size() > 1;
    previousButton_->setVisible(hasMultipleErrors);
    nextButton_->setVisible(hasMultipleErrors);
    errorList_->setVisible(hasMultipleErrors);

    if (hasMultipleErrors) {
        previousButton_->setEnabled(currentErrorIndex_ > 0);
        nextButton_->setEnabled(currentErrorIndex_ < errors_.size() - 1);
        errorCountLabel_->setText(QString("Error %1 of %2").arg(currentErrorIndex_ + 1).arg(errors_.size()));
        errorCountLabel_->setVisible(true);
    } else {
        errorCountLabel_->setVisible(false);
    }
}

void ErrorDetailsDialog::updateSeverityIcon() {
    if (currentErrorIndex_ < 0 || currentErrorIndex_ >= errors_.size()) {
        return;
    }

    const ErrorDetails& error = errors_[currentErrorIndex_];
    severityIconLabel_->setPixmap(QPixmap(getSeverityIcon(error.severity)).scaled(32, 32, Qt::KeepAspectRatio));
    severityIconLabel_->setStyleSheet(QString("background-color: %1; border-radius: 4px; padding: 4px;")
                                     .arg(getSeverityColor(error.severity)));
}

QString ErrorDetailsDialog::getSeverityIcon(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::INFO:
            return ":/icons/info.png";
        case ErrorSeverity::WARNING:
            return ":/icons/warning.png";
        case ErrorSeverity::ERROR:
            return ":/icons/error.png";
        case ErrorSeverity::CRITICAL:
            return ":/icons/critical.png";
        case ErrorSeverity::FATAL:
            return ":/icons/fatal.png";
        default:
            return ":/icons/error.png";
    }
}

QString ErrorDetailsDialog::getSeverityColor(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::INFO:
            return "#e3f2fd"; // Light blue
        case ErrorSeverity::WARNING:
            return "#fff3e0"; // Light orange
        case ErrorSeverity::ERROR:
            return "#ffebee"; // Light red
        case ErrorSeverity::CRITICAL:
            return "#ffebee"; // Light red
        case ErrorSeverity::FATAL:
            return "#ffebee"; // Light red
        default:
            return "#f5f5f5"; // Light gray
    }
}

QString ErrorDetailsDialog::getCategoryIcon(ErrorCategory category) const {
    switch (category) {
        case ErrorCategory::DATABASE:
            return "DATABASE";
        case ErrorCategory::NETWORK:
            return "NETWORK";
        case ErrorCategory::FILESYSTEM:
            return "FILESYSTEM";
        case ErrorCategory::VALIDATION:
            return "VALIDATION";
        case ErrorCategory::PERMISSION:
            return "PERMISSION";
        case ErrorCategory::SYSTEM:
            return "SYSTEM";
        case ErrorCategory::APPLICATION:
            return "APPLICATION";
        case ErrorCategory::USER:
            return "USER";
        default:
            return "UNKNOWN";
    }
}

QString ErrorDetailsDialog::formatTimestamp(const QString& timestamp) const {
    QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
    if (dt.isValid()) {
        return dt.toString("yyyy-MM-dd hh:mm:ss");
    }
    return timestamp;
}

void ErrorDetailsDialog::onCopyToClipboard() {
    QString errorReport = generateErrorReport();
    QApplication::clipboard()->setText(errorReport);

    QMessageBox::information(this, "Copied",
                           "Error details have been copied to clipboard.");
}

void ErrorDetailsDialog::onSaveErrorLog() {
    if (currentErrorIndex_ < 0 || currentErrorIndex_ >= errors_.size()) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, "Save Error Log", "",
        "Text Files (*.txt);;Log Files (*.log);;All Files (*.*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << generateErrorReport();
        file.close();

        QMessageBox::information(this, "Saved",
                               QString("Error log saved to:\n\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, "Save Failed",
                           "Failed to save error log file.");
    }
}

void ErrorDetailsDialog::onReportError() {
    if (currentErrorIndex_ < 0 || currentErrorIndex_ >= errors_.size()) {
        return;
    }

    QMessageBox::information(this, "Report Error",
                           "Error reporting functionality would send the error details to the development team.\n\n"
                           "This feature can be configured to integrate with bug tracking systems like Jira, GitHub Issues, etc.");

    emit errorReported(errors_[currentErrorIndex_]);
}

void ErrorDetailsDialog::onShowHelp() {
    if (currentErrorIndex_ < 0 || currentErrorIndex_ >= errors_.size()) {
        return;
    }

    const ErrorDetails& error = errors_[currentErrorIndex_];
    if (!error.helpUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(error.helpUrl));
    } else {
        QMessageBox::information(this, "Help",
                               "Help system integration would show context-sensitive help for this error type.");
    }

    emit helpRequested(error.helpUrl);
}

void ErrorDetailsDialog::onRetryOperation() {
    if (currentErrorIndex_ < 0 || currentErrorIndex_ >= errors_.size()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Retry Operation",
        "Do you want to retry the operation that failed?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        emit retryRequested(errors_[currentErrorIndex_]);
        accept();
    }
}

void ErrorDetailsDialog::onIgnoreError() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Ignore Error",
        "Are you sure you want to ignore this error?\n\nThis may cause unexpected behavior.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        emit ignoreRequested(errors_[currentErrorIndex_]);
        accept();
    }
}

void ErrorDetailsDialog::onPreviousError() {
    if (currentErrorIndex_ > 0) {
        onErrorSelected(currentErrorIndex_ - 1);
    }
}

void ErrorDetailsDialog::onNextError() {
    if (currentErrorIndex_ < errors_.size() - 1) {
        onErrorSelected(currentErrorIndex_ + 1);
    }
}

void ErrorDetailsDialog::onErrorSelected(int index) {
    if (index >= 0 && index < errors_.size()) {
        currentErrorIndex_ = index;
        errorList_->setCurrentRow(index);
        updateUI();
    }
}

void ErrorDetailsDialog::onTabChanged(int index) {
    // Handle tab changes if needed
    Q_UNUSED(index)
}

QString ErrorDetailsDialog::generateErrorReport() const {
    if (currentErrorIndex_ < 0 || currentErrorIndex_ >= errors_.size()) {
        return QString();
    }

    const ErrorDetails& error = errors_[currentErrorIndex_];

    QString report;
    QTextStream stream(&report);

    stream << "ERROR REPORT\n";
    stream << "============\n\n";

    stream << "Error ID: " << error.errorId << "\n";
    stream << "Title: " << error.title << "\n";
    stream << "Timestamp: " << formatTimestamp(error.timestamp) << "\n";
    stream << "Severity: " << static_cast<int>(error.severity) << "\n";
    stream << "Category: " << static_cast<int>(error.category) << "\n";
    stream << "Error Code: " << error.errorCode << "\n\n";

    stream << "Summary:\n" << error.summary << "\n\n";

    if (!error.detailedDescription.isEmpty()) {
        stream << "Detailed Description:\n" << error.detailedDescription << "\n\n";
    }

    if (!error.technicalDetails.isEmpty()) {
        stream << "Technical Details:\n" << error.technicalDetails << "\n\n";
    }

    if (!error.stackTrace.isEmpty()) {
        stream << "Stack Trace:\n" << error.stackTrace << "\n\n";
    }

    if (!error.suggestedActions.isEmpty()) {
        stream << "Suggested Actions:\n";
        for (const QString& action : error.suggestedActions) {
            stream << "• " << action << "\n";
        }
        stream << "\n";
    }

    if (!error.contextData.isEmpty()) {
        stream << "Context Data:\n";
        QJsonObject contextObj;
        for (auto it = error.contextData.begin(); it != error.contextData.end(); ++it) {
            contextObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        stream << QJsonDocument(contextObj).toJson(QJsonDocument::Indented);
        stream << "\n";
    }

    stream << "System Information:\n";
    stream << "Application: ScratchRobin\n";
    stream << "Version: 0.1.0\n";
    stream << "Qt Version: " << qVersion() << "\n";
    stream << "OS: " << QSysInfo::prettyProductName() << "\n";

    return report;
}

} // namespace scratchrobin
