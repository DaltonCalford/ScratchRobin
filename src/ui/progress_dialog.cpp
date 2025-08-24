#include "progress_dialog.h"
#include <QDialogButtonBox>
#include <QApplication>
#include <QStyle>
#include <QDesktopWidget>
#include <QTextCursor>
#include <QDateTime>
#include <QDebug>
#include <QThread>

namespace scratchrobin {

ProgressDialog::ProgressDialog(QWidget* parent)
    : QDialog(parent),
      updateTimer_(new QTimer(this)),
      autoCloseTimer_(new QTimer(this)) {

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setWindowTitle("Progress");

    setupUI();

    connect(updateTimer_, &QTimer::timeout, this, &ProgressDialog::onTimerUpdate);
    connect(autoCloseTimer_, &QTimer::timeout, this, &ProgressDialog::onAutoCloseTimer);

    updateTimer_->setInterval(500); // Update every 500ms
}

void ProgressDialog::setOperation(const QString& title, const QString& description) {
    setWindowTitle(title);
    titleLabel_->setText(title);
    if (!description.isEmpty()) {
        descriptionLabel_->setText(description);
        descriptionLabel_->setVisible(true);
    }
}

void ProgressDialog::setMode(ProgressDialogMode mode) {
    mode_ = mode;
    updateUI();
}

void ProgressDialog::setAllowCancel(bool allow) {
    allowCancel_ = allow;
    cancelButton_->setVisible(allow);
}

void ProgressDialog::setShowDetails(bool show) {
    showDetails_ = show;
    detailsGroup_->setVisible(show);
    showDetailsCheck_->setChecked(show);
}

void ProgressDialog::setAutoClose(bool autoClose) {
    autoClose_ = autoClose;
}

void ProgressDialog::setAutoCloseDelay(int seconds) {
    autoCloseDelay_ = seconds;
}

void ProgressDialog::addOperation(const QString& name, const QString& description, int totalSteps) {
    ProgressOperation operation;
    operation.name = name;
    operation.description = description;
    operation.totalSteps = totalSteps;
    operation.currentStep = 0;
    operation.progress = 0.0;
    operation.hasError = false;
    operation.timer.start();

    operations_.append(operation);

    if (mode_ == ProgressDialogMode::MULTI_STEP) {
        updateUI();
    }
}

void ProgressDialog::updateOperation(int operationIndex, double progress, const QString& status) {
    if (operationIndex < 0 || operationIndex >= operations_.size()) {
        return;
    }

    ProgressOperation& operation = operations_[operationIndex];
    operation.progress = qBound(0.0, progress, 1.0);

    if (!status.isEmpty()) {
        operation.description = status;
    }

    // Calculate overall progress
    if (!operations_.isEmpty()) {
        double totalProgress = 0.0;
        for (const ProgressOperation& op : operations_) {
            totalProgress += op.progress;
        }
        totalProgress /= operations_.size();
        setProgress(totalProgress);
    }

    if (mode_ == ProgressDialogMode::MULTI_STEP) {
        updateUI();
    }
}

void ProgressDialog::setOperationError(int operationIndex, const QString& errorMessage) {
    if (operationIndex < 0 || operationIndex >= operations_.size()) {
        return;
    }

    ProgressOperation& operation = operations_[operationIndex];
    operation.hasError = true;
    operation.errorMessage = errorMessage;

    emit operationError(operationIndex, errorMessage);

    if (mode_ == ProgressDialogMode::MULTI_STEP) {
        updateUI();
    }
}

void ProgressDialog::completeOperation(int operationIndex) {
    if (operationIndex < 0 || operationIndex >= operations_.size()) {
        return;
    }

    ProgressOperation& operation = operations_[operationIndex];
    operation.progress = 1.0;

    emit operationCompleted(operationIndex);

    if (mode_ == ProgressDialogMode::MULTI_STEP) {
        updateUI();
    }
}

void ProgressDialog::setProgress(int value) {
    currentProgress_ = qBound(0, value, 100);
    progressBar_->setValue(currentProgress_);
    emit progressChanged(currentProgress_);
}

void ProgressDialog::setProgress(double value) {
    setProgress(qRound(value * 100.0));
}

void ProgressDialog::setProgress(int current, int total) {
    if (total > 0) {
        setProgress((current * 100) / total);
    }
}

void ProgressDialog::setStatusText(const QString& text) {
    currentStatus_ = text;
    statusLabel_->setText(text);
    emit statusChanged(text);
}

void ProgressDialog::setDetailsText(const QString& text) {
    detailsTextContent_ = text;
    detailsText_->setPlainText(text);
    detailsText_->moveCursor(QTextCursor::End);
}

void ProgressDialog::appendDetailsText(const QString& text) {
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    detailsTextContent_ += timestamp + text + "\n";
    detailsText_->append(timestamp + text);
    detailsText_->moveCursor(QTextCursor::End);
}

void ProgressDialog::start() {
    status_ = ProgressStatus::RUNNING;
    elapsedTimer_.start();
    updateTimer_->start();
    cancelButton_->setEnabled(true);

    // Update initial UI
    updateUI();
    updateElapsedTime();

    // Show the dialog
    show();
    QApplication::processEvents();
}

void ProgressDialog::stop() {
    updateTimer_->stop();

    if (status_ == ProgressStatus::RUNNING) {
        if (currentProgress_ >= 100) {
            status_ = ProgressStatus::COMPLETED;
            setStatusText("Operation completed successfully");
            appendDetailsText("✓ Operation completed successfully");
        } else {
            status_ = ProgressStatus::ERROR;
            setStatusText("Operation failed");
            appendDetailsText("✗ Operation failed");
        }
    }

    cancelButton_->setEnabled(false);

    if (autoClose_ && status_ == ProgressStatus::COMPLETED) {
        autoCloseTimer_->start(autoCloseDelay_ * 1000);
    }

    emit finished(status_);
}

void ProgressDialog::cancel() {
    if (!allowCancel_ || status_ != ProgressStatus::RUNNING) {
        return;
    }

    status_ = ProgressStatus::CANCELLED;
    updateTimer_->stop();
    setStatusText("Operation cancelled");
    appendDetailsText("⚠ Operation cancelled by user");

    cancelButton_->setEnabled(false);
    emit cancelled();
    emit finished(status_);
}

bool ProgressDialog::isRunning() const {
    return status_ == ProgressStatus::RUNNING;
}

bool ProgressDialog::isCancelled() const {
    return status_ == ProgressStatus::CANCELLED;
}

ProgressStatus ProgressDialog::getStatus() const {
    return status_;
}

bool ProgressDialog::showProgress(QWidget* parent,
                                 const QString& title,
                                 const QString& description,
                                 std::function<bool(ProgressDialog*)> operation,
                                 bool allowCancel) {
    ProgressDialog dialog(parent);
    dialog.setOperation(title, description);
    dialog.setAllowCancel(allowCancel);
    dialog.setMode(ProgressDialogMode::DETERMINATE);

    dialog.start();

    // Run the operation in a separate thread or in the same thread
    bool success = false;
    try {
        success = operation(&dialog);
    } catch (const std::exception& e) {
        dialog.setStatusText("Error: " + QString::fromStdString(e.what()));
        dialog.appendDetailsText("Error: " + QString::fromStdString(e.what()));
        success = false;
    } catch (...) {
        dialog.setStatusText("Unknown error occurred");
        dialog.appendDetailsText("Unknown error occurred");
        success = false;
    }

    if (success && !dialog.isCancelled()) {
        dialog.setProgress(100);
    }

    dialog.stop();

    return success && !dialog.isCancelled();
}

bool ProgressDialog::showIndeterminateProgress(QWidget* parent,
                                             const QString& title,
                                             const QString& description,
                                             std::function<bool(ProgressDialog*)> operation,
                                             bool allowCancel) {
    ProgressDialog dialog(parent);
    dialog.setOperation(title, description);
    dialog.setAllowCancel(allowCancel);
    dialog.setMode(ProgressDialogMode::INDETERMINATE);

    dialog.start();

    bool success = false;
    try {
        success = operation(&dialog);
    } catch (const std::exception& e) {
        dialog.setStatusText("Error: " + QString::fromStdString(e.what()));
        dialog.appendDetailsText("Error: " + QString::fromStdString(e.what()));
        success = false;
    } catch (...) {
        dialog.setStatusText("Unknown error occurred");
        dialog.appendDetailsText("Unknown error occurred");
        success = false;
    }

    dialog.stop();

    return success && !dialog.isCancelled();
}

void ProgressDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(10);
    mainLayout_->setContentsMargins(20, 20, 20, 20);

    // Header
    setupHeader();

    // Progress area
    setupBasicProgress();
    setupMultiStepProgress();

    // Details area
    setupDetails();

    // Buttons
    setupButtons();

    // Set initial state
    updateUI();
}

void ProgressDialog::setupHeader() {
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QVBoxLayout* textLayout = new QVBoxLayout();
    titleLabel_ = new QLabel("Progress");
    titleLabel_->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c5aa0;");
    textLayout->addWidget(titleLabel_);

    descriptionLabel_ = new QLabel();
    descriptionLabel_->setWordWrap(true);
    descriptionLabel_->setVisible(false);
    textLayout->addWidget(descriptionLabel_);

    headerLayout->addLayout(textLayout);
    headerLayout->addStretch();

    mainLayout_->addLayout(headerLayout);
}

void ProgressDialog::setupBasicProgress() {
    progressWidget_ = new QWidget();
    QVBoxLayout* progressLayout = new QVBoxLayout(progressWidget_);

    // Progress bar
    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressLayout->addWidget(progressBar_);

    // Status and time info
    QHBoxLayout* infoLayout = new QHBoxLayout();

    statusLabel_ = new QLabel("Ready");
    infoLayout->addWidget(statusLabel_);

    infoLayout->addStretch();

    timeLabel_ = new QLabel("00:00:00");
    infoLayout->addWidget(timeLabel_);

    progressLayout->addLayout(infoLayout);

    mainLayout_->addWidget(progressWidget_);
}

void ProgressDialog::setupMultiStepProgress() {
    multiStepWidget_ = new QWidget();
    multiStepWidget_->setVisible(false);

    operationsLayout_ = new QVBoxLayout(multiStepWidget_);
    mainLayout_->addWidget(multiStepWidget_);
}

void ProgressDialog::setupDetails() {
    detailsGroup_ = new QGroupBox("Details");
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup_);

    detailsText_ = new QTextEdit();
    detailsText_->setMaximumHeight(200);
    detailsText_->setReadOnly(true);
    detailsText_->setFontFamily("monospace");
    detailsText_->setStyleSheet("QTextEdit { background-color: #f5f5f5; }");
    detailsLayout->addWidget(detailsText_);

    showDetailsCheck_ = new QCheckBox("Show details");
    connect(showDetailsCheck_, &QCheckBox::toggled, this, &ProgressDialog::onShowDetailsToggled);
    detailsLayout->addWidget(showDetailsCheck_);

    detailsGroup_->setVisible(false);
    mainLayout_->addWidget(detailsGroup_);
}

void ProgressDialog::setupButtons() {
    buttonLayout_ = new QHBoxLayout();

    detailsButton_ = new QPushButton("Show Details");
    connect(detailsButton_, &QPushButton::clicked, [this]() {
        setShowDetails(!showDetails_);
    });
    buttonLayout_->addWidget(detailsButton_);

    buttonLayout_->addStretch();

    cancelButton_ = new QPushButton("Cancel");
    cancelButton_->setIcon(QIcon(":/icons/cancel.png"));
    connect(cancelButton_, &QPushButton::clicked, this, &ProgressDialog::onCancelClicked);
    buttonLayout_->addWidget(cancelButton_);

    closeButton_ = new QPushButton("Close");
    closeButton_->setVisible(false);
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout_->addWidget(closeButton_);

    mainLayout_->addLayout(buttonLayout_);
}

void ProgressDialog::onCancelClicked() {
    cancel();
}

void ProgressDialog::onShowDetailsToggled(bool checked) {
    setShowDetails(checked);
}

void ProgressDialog::onTimerUpdate() {
    if (status_ == ProgressStatus::RUNNING) {
        updateElapsedTime();

        // Allow the UI to update
        QApplication::processEvents();

        // Check if operation was cancelled
        if (status_ == ProgressStatus::CANCELLED) {
            stop();
        }
    }
}

void ProgressDialog::onAutoCloseTimer() {
    closeDialog();
}

void ProgressDialog::updateUI() {
    // Update progress bar style based on mode
    if (mode_ == ProgressDialogMode::INDETERMINATE) {
        progressBar_->setRange(0, 0); // Indeterminate mode
    } else {
        progressBar_->setRange(0, 100);
    }

    // Show/hide appropriate widgets
    progressWidget_->setVisible(mode_ != ProgressDialogMode::MULTI_STEP);
    multiStepWidget_->setVisible(mode_ == ProgressDialogMode::MULTI_STEP);

    // Update multi-step operations if needed
    if (mode_ == ProgressDialogMode::MULTI_STEP) {
        // Clear existing operation widgets
        for (QWidget* widget : operationWidgets_) {
            operationsLayout_->removeWidget(widget);
            delete widget;
        }
        operationWidgets_.clear();

        // Create new operation widgets
        for (int i = 0; i < operations_.size(); ++i) {
            const ProgressOperation& operation = operations_[i];

            QWidget* opWidget = new QWidget();
            QHBoxLayout* opLayout = new QHBoxLayout(opWidget);

            QLabel* nameLabel = new QLabel(operation.name);
            nameLabel->setStyleSheet("font-weight: bold;");
            opLayout->addWidget(nameLabel);

            QProgressBar* opProgress = new QProgressBar();
            opProgress->setRange(0, 100);
            opProgress->setValue(qRound(operation.progress * 100));
            opLayout->addWidget(opProgress);

            if (operation.hasError) {
                opProgress->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");
            }

            QLabel* statusLabel = new QLabel(operation.description);
            statusLabel->setWordWrap(true);
            opLayout->addWidget(statusLabel);

            operationsLayout_->addWidget(opWidget);
            operationWidgets_.append(opWidget);
        }
    }
}

void ProgressDialog::updateElapsedTime() {
    if (elapsedTimer_.isValid()) {
        qint64 elapsed = elapsedTimer_.elapsed();
        int hours = elapsed / (1000 * 60 * 60);
        int minutes = (elapsed % (1000 * 60 * 60)) / (1000 * 60);
        int seconds = (elapsed % (1000 * 60)) / 1000;

        timeLabel_->setText(QString("%1:%2:%3")
                           .arg(hours, 2, 10, QChar('0'))
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0')));
    }
}

void ProgressDialog::closeDialog() {
    autoCloseTimer_->stop();
    accept();
}

} // namespace scratchrobin
