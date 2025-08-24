#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QGroupBox>
#include <QListWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QProgressBar>
#include <QDateTime>

namespace scratchrobin {

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
    FATAL
};

enum class ErrorCategory {
    DATABASE,
    NETWORK,
    FILESYSTEM,
    VALIDATION,
    PERMISSION,
    SYSTEM,
    APPLICATION,
    USER,
    CRITICAL,
    FATAL
};

struct ErrorDetails {
    QString errorId;
    QString title;
    QString summary;
    QString detailedDescription;
    QString technicalDetails;
    QString stackTrace;
    QString sourceLocation;
    QString timestamp;
    ErrorSeverity severity = ErrorSeverity::ERROR;
    ErrorCategory category = ErrorCategory::APPLICATION;
    QString errorCode;
    QString helpUrl;
    QStringList suggestedActions;
    QStringList relatedErrors;
    QVariantMap contextData;
    bool isRecoverable = true;
    QString recoverySuggestion;
};

class ErrorDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ErrorDetailsDialog(QWidget* parent = nullptr);
    ~ErrorDetailsDialog() override = default;

    void setError(const ErrorDetails& error);
    void addError(const ErrorDetails& error);
    void clearErrors();

    // Static convenience methods
    static void showError(QWidget* parent, const ErrorDetails& error);
    static void showError(QWidget* parent,
                         const QString& title,
                         const QString& message,
                         ErrorSeverity severity = ErrorSeverity::ERROR);
    static void showDatabaseError(QWidget* parent,
                                 const QString& query,
                                 const QString& errorMessage,
                                 const QString& connectionString = QString());
    static void showFileError(QWidget* parent,
                             const QString& filePath,
                             const QString& operation,
                             const QString& errorMessage);

signals:
    void helpRequested(const QString& helpUrl);
    void errorReported(const ErrorDetails& error);
    void retryRequested(const ErrorDetails& error);
    void ignoreRequested(const ErrorDetails& error);

private slots:
    void onCopyToClipboard();
    void onSaveErrorLog();
    void onReportError();
    void onShowHelp();
    void onRetryOperation();
    void onIgnoreError();
    void onPreviousError();
    void onNextError();
    void onErrorSelected(int index);
    void onTabChanged(int index);

private:
    void setupUI();
    void setupHeader();
    void setupContent();
    void setupActions();
    void setupOverviewTab();
    void setupDetailsTab();
    void setupContextTab();
    void setupRelatedTab();
    void updateUI();
    void updateSeverityIcon();
    void updateCategoryIcon();
    QString getSeverityIcon(ErrorSeverity severity) const;
    QString getSeverityColor(ErrorSeverity severity) const;
    QString getCategoryIcon(ErrorCategory category) const;
    QString formatTimestamp(const QString& timestamp) const;
    QString generateErrorReport() const;

    // UI Components
    QVBoxLayout* mainLayout_;

    // Header section
    QWidget* headerWidget_;
    QLabel* severityIconLabel_;
    QLabel* titleLabel_;
    QLabel* summaryLabel_;
    QLabel* timestampLabel_;
    QLabel* categoryLabel_;

    // Content tabs
    QTabWidget* contentTabs_;

    // Overview tab
    QWidget* overviewTab_;
    QLabel* descriptionLabel_;
    QGroupBox* suggestionsGroup_;
    QListWidget* suggestionsList_;

    // Details tab
    QWidget* detailsTab_;
    QTextEdit* technicalDetailsText_;
    QTextEdit* stackTraceText_;

    // Context tab
    QWidget* contextTab_;
    QTextEdit* contextDataText_;

    // Related tab
    QWidget* relatedTab_;
    QListWidget* relatedErrorsList_;

    // Error list (for multiple errors)
    QListWidget* errorList_;
    QLabel* errorCountLabel_;

    // Action buttons
    QHBoxLayout* buttonLayout_;
    QPushButton* copyButton_;
    QPushButton* saveButton_;
    QPushButton* reportButton_;
    QPushButton* helpButton_;
    QPushButton* retryButton_;
    QPushButton* ignoreButton_;
    QPushButton* previousButton_;
    QPushButton* nextButton_;
    QPushButton* closeButton_;

    // Current state
    QList<ErrorDetails> errors_;
    int currentErrorIndex_ = -1;
    bool showMultipleErrors_ = false;
};

} // namespace scratchrobin
