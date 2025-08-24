#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QElapsedTimer>
#include <QGroupBox>
#include <QCheckBox>
#include <QFrame>

namespace scratchrobin {

enum class ProgressDialogMode {
    DETERMINATE,    // Shows percentage and progress bar
    INDETERMINATE,  // Shows animated progress bar (unknown duration)
    MULTI_STEP      // Shows multiple operations with individual progress
};

enum class ProgressStatus {
    RUNNING,
    COMPLETED,
    CANCELLED,
    ERROR
};

struct ProgressOperation {
    QString name;
    QString description;
    int currentStep = 0;
    int totalSteps = 0;
    double progress = 0.0; // 0.0 to 1.0
    bool hasError = false;
    QString errorMessage;
    QElapsedTimer timer;
};

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget* parent = nullptr);
    ~ProgressDialog() override = default;

    // Setup methods
    void setOperation(const QString& title, const QString& description = QString());
    void setMode(ProgressDialogMode mode);
    void setAllowCancel(bool allow);
    void setShowDetails(bool show);
    void setAutoClose(bool autoClose);
    void setAutoCloseDelay(int seconds);

    // Multi-step operations
    void addOperation(const QString& name, const QString& description, int totalSteps = 0);
    void updateOperation(int operationIndex, double progress, const QString& status = QString());
    void setOperationError(int operationIndex, const QString& errorMessage);
    void completeOperation(int operationIndex);

    // Progress updates
    void setProgress(int value); // 0-100
    void setProgress(double value); // 0.0-1.0
    void setProgress(int current, int total);
    void setStatusText(const QString& text);
    void setDetailsText(const QString& text);
    void appendDetailsText(const QString& text);

    // Control methods
    void start();
    void stop();
    void cancel();
    bool isRunning() const;
    bool isCancelled() const;
    ProgressStatus getStatus() const;

    // Static convenience methods
    static bool showProgress(QWidget* parent,
                           const QString& title,
                           const QString& description,
                           std::function<bool(ProgressDialog*)> operation,
                           bool allowCancel = true);

    static bool showIndeterminateProgress(QWidget* parent,
                                        const QString& title,
                                        const QString& description,
                                        std::function<bool(ProgressDialog*)> operation,
                                        bool allowCancel = true);

signals:
    void progressChanged(int value);
    void statusChanged(const QString& status);
    void operationCompleted(int operationIndex);
    void operationError(int operationIndex, const QString& error);
    void cancelled();
    void finished(ProgressStatus status);

private slots:
    void onCancelClicked();
    void onShowDetailsToggled(bool checked);
    void onTimerUpdate();
    void onAutoCloseTimer();

private:
    void setupUI();
    void setupBasicProgress();
    void setupMultiStepProgress();
    void setupHeader();
    void setupDetails();
    void setupButtons();
    void updateUI();
    void updateElapsedTime();
    void closeDialog();

    // UI Components
    QVBoxLayout* mainLayout_;

    // Header
    QLabel* titleLabel_;
    QLabel* descriptionLabel_;

    // Progress area
    QWidget* progressWidget_;
    QProgressBar* progressBar_;
    QLabel* progressLabel_;
    QLabel* statusLabel_;
    QLabel* timeLabel_;

    // Multi-step progress
    QWidget* multiStepWidget_;
    QVBoxLayout* operationsLayout_;
    QList<QWidget*> operationWidgets_;

    // Details area
    QGroupBox* detailsGroup_;
    QTextEdit* detailsText_;
    QCheckBox* showDetailsCheck_;

    // Buttons
    QHBoxLayout* buttonLayout_;
    QPushButton* cancelButton_;
    QPushButton* closeButton_;
    QPushButton* detailsButton_;

    // Current state
    ProgressDialogMode mode_ = ProgressDialogMode::DETERMINATE;
    ProgressStatus status_ = ProgressStatus::RUNNING;
    bool allowCancel_ = true;
    bool showDetails_ = false;
    bool autoClose_ = false;
    int autoCloseDelay_ = 3; // seconds

    // Progress tracking
    int currentProgress_ = 0;
    int totalProgress_ = 100;
    QString currentStatus_;
    QString detailsTextContent_;
    QElapsedTimer elapsedTimer_;
    QTimer* updateTimer_;
    QTimer* autoCloseTimer_;

    // Multi-step operations
    QList<ProgressOperation> operations_;
    int currentOperation_ = 0;
};

} // namespace scratchrobin
