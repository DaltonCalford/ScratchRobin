#include "confirmation_dialog.h"
#include <QDialogButtonBox>
#include <QApplication>
#include <QStyle>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

namespace scratchrobin {

ConfirmationDialog::ConfirmationDialog(QWidget* parent)
    : QDialog(parent), timeoutTimer_(new QTimer(this)) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setWindowTitle("Confirmation");
    setMinimumSize(450, 300);
    resize(550, 400);

    connect(timeoutTimer_, &QTimer::timeout, this, &ConfirmationDialog::onTimeoutUpdate);
}

bool ConfirmationDialog::confirm(QWidget* parent, const ConfirmationOptions& options) {
    // Check if we should skip asking
    if (options.showDontAskAgain) {
        QString operationId = options.title.toLower().replace(" ", "_");
        if (!shouldAskAgain(operationId)) {
            // Default to the safest action or first non-destructive action
            for (const ConfirmationAction& action : options.actions) {
                if (!action.isDestructive) {
                    return action.label.toLower().contains("yes") ||
                           action.label.toLower().contains("ok") ||
                           action.label.toLower().contains("continue");
                }
            }
            return false; // Default to cancel for safety
        }
    }

    ConfirmationDialog dialog(parent);
    dialog.currentOptions_ = options;
    dialog.operationId_ = options.title.toLower().replace(" ", "_");
    dialog.setupUI();

    return dialog.exec() == QDialog::Accepted;
}

bool ConfirmationDialog::confirmDelete(QWidget* parent, const QString& itemType, const QString& itemName, int itemCount) {
    ConfirmationOptions options;
    options.title = QString("Delete %1").arg(itemCount > 1 ? QString("%1 %2s").arg(itemCount).arg(itemType) : itemType);
    options.message = QString("Are you sure you want to delete %1?").arg(
        itemCount > 1 ? QString("%1 %2s").arg(itemCount).arg(itemType) :
        QString("the %1 '%2'").arg(itemType).arg(itemName)
    );
    options.detailedMessage = "This action cannot be undone.";
    options.type = ConfirmationType::CHECKBOX;
    options.riskLevel = itemCount > 10 ? RiskLevel::HIGH : RiskLevel::MEDIUM;
    options.showDontAskAgain = itemCount <= 5;

    ConfirmationAction yesAction;
    yesAction.label = "Delete";
    yesAction.isDestructive = true;
    yesAction.iconName = "delete";

    ConfirmationAction noAction;
    noAction.label = "Cancel";
    noAction.isDefault = true;
    noAction.iconName = "cancel";

    options.actions = {yesAction, noAction};
    options.impactDetails = QStringList()
        << QString("Will permanently remove %1").arg(
            itemCount > 1 ? QString("%1 %2s").arg(itemCount).arg(itemType) :
            QString("the %1 '%2'").arg(itemType).arg(itemName)
        )
        << "Data will be lost permanently"
        << "Related records may be affected";

    return confirm(parent, options);
}

bool ConfirmationDialog::confirmDropTable(QWidget* parent, const QString& tableName, int rowCount) {
    ConfirmationOptions options;
    options.title = "Drop Table";
    options.message = QString("Drop table '%1'?").arg(tableName);
    options.detailedMessage = QString("This will permanently delete the table and all its data (%1 rows).").arg(rowCount);
    options.type = ConfirmationType::CRITICAL;
    options.riskLevel = RiskLevel::CRITICAL;
    options.showDontAskAgain = false;

    ConfirmationAction dropAction;
    dropAction.label = "Drop Table";
    dropAction.description = "Permanently delete the table and all data";
    dropAction.isDestructive = true;
    dropAction.iconName = "delete_table";

    ConfirmationAction backupAction;
    backupAction.label = "Backup & Drop";
    backupAction.description = "Create backup before dropping table";
    backupAction.iconName = "backup";

    ConfirmationAction cancelAction;
    cancelAction.label = "Cancel";
    cancelAction.isDefault = true;
    cancelAction.iconName = "cancel";

    options.actions = {dropAction, backupAction, cancelAction};
    options.impactDetails = QStringList()
        << QString("Table '%1' will be permanently deleted").arg(tableName)
        << QString("All %1 rows of data will be lost").arg(rowCount)
        << "Related views and constraints may be affected"
        << "Applications using this table may break";
    options.consequences = QStringList()
        << "Data loss is irreversible"
        << "May require application restart"
        << "Backup recommended before proceeding";
    options.alternatives = QStringList()
        << "Export data before dropping"
        << "Rename table instead of dropping"
        << "Archive table data";

    ConfirmationAction result = showWithActions(parent, options);
    return result.label == "Drop Table" || result.label == "Backup & Drop";
}

bool ConfirmationDialog::confirmDropDatabase(QWidget* parent, const QString& databaseName) {
    ConfirmationOptions options;
    options.title = "Drop Database";
    options.message = QString("Drop database '%1'?").arg(databaseName);
    options.detailedMessage = "This will permanently delete the entire database, all tables, and all data.";
    options.type = ConfirmationType::CRITICAL;
    options.riskLevel = RiskLevel::CRITICAL;
    options.showDontAskAgain = false;
    options.timeoutSeconds = 30; // Extra time to think about this

    ConfirmationAction dropAction;
    dropAction.label = "Drop Database";
    dropAction.description = "Permanently delete the entire database";
    dropAction.isDestructive = true;
    dropAction.iconName = "delete_database";

    ConfirmationAction cancelAction;
    cancelAction.label = "Cancel";
    cancelAction.isDefault = true;
    cancelAction.iconName = "cancel";

    options.actions = {dropAction, cancelAction};
    options.impactDetails = QStringList()
        << QString("Database '%1' will be completely removed").arg(databaseName)
        << "All tables, views, and data will be lost"
        << "All users and permissions will be removed"
        << "Connected applications will lose access";
    options.consequences = QStringList()
        << "This action CANNOT be undone"
        << "Complete data loss"
        << "Service disruption for connected applications"
        << "May require system administrator intervention";

    ConfirmationAction result = showWithActions(parent, options);
    return result.label == "Drop Database";
}

bool ConfirmationDialog::confirmOverwrite(QWidget* parent, const QString& fileName, const QString& operation) {
    ConfirmationOptions options;
    options.title = "Confirm Overwrite";
    options.message = QString("File '%1' already exists").arg(fileName);
    options.detailedMessage = QString("Do you want to overwrite the existing file with your %1?").arg(operation);
    options.type = ConfirmationType::CHECKBOX;
    options.riskLevel = RiskLevel::MEDIUM;
    options.showDontAskAgain = true;

    ConfirmationAction overwriteAction;
    overwriteAction.label = "Overwrite";
    overwriteAction.isDestructive = true;
    overwriteAction.iconName = "overwrite";

    ConfirmationAction renameAction;
    renameAction.label = "Rename";
    renameAction.description = "Choose a different name";
    renameAction.iconName = "rename";

    ConfirmationAction cancelAction;
    cancelAction.label = "Cancel";
    cancelAction.isDefault = true;
    cancelAction.iconName = "cancel";

    options.actions = {overwriteAction, renameAction, cancelAction};
    options.impactDetails = QStringList()
        << QString("Existing file '%1' will be replaced").arg(fileName)
        << "Original file content will be lost"
        << "File modification date will be updated";

    ConfirmationAction result = showWithActions(parent, options);
    return result.label == "Overwrite";
}

bool ConfirmationDialog::confirmCloseUnsaved(QWidget* parent, int unsavedCount) {
    ConfirmationOptions options;
    options.title = "Unsaved Changes";
    options.message = QString("You have %1 unsaved %2")
        .arg(unsavedCount)
        .arg(unsavedCount == 1 ? "change" : "changes");
    options.detailedMessage = "Do you want to save your changes before closing?";
    options.type = ConfirmationType::MULTI_OPTION;
    options.riskLevel = RiskLevel::MEDIUM;
    options.showDontAskAgain = unsavedCount <= 3;

    ConfirmationAction saveAction;
    saveAction.label = "Save Changes";
    saveAction.description = "Save all changes before closing";
    saveAction.iconName = "save";

    ConfirmationAction discardAction;
    discardAction.label = "Discard Changes";
    discardAction.description = "Close without saving";
    discardAction.isDestructive = true;
    discardAction.iconName = "discard";

    ConfirmationAction cancelAction;
    cancelAction.label = "Cancel";
    cancelAction.isDefault = true;
    cancelAction.iconName = "cancel";

    options.actions = {saveAction, discardAction, cancelAction};
    options.impactDetails = QStringList()
        << QString("%1 %2 will be lost if not saved").arg(unsavedCount).arg(
            unsavedCount == 1 ? "change" : "changes"
        )
        << "Work in progress may be lost";

    ConfirmationAction result = showWithActions(parent, options);
    return result.label == "Save Changes";
}

bool ConfirmationDialog::confirmBulkOperation(QWidget* parent, const QString& operation, int affectedCount) {
    ConfirmationOptions options;
    options.title = "Confirm Bulk Operation";
    options.message = QString("Apply '%1' to %2 items?").arg(operation).arg(affectedCount);
    options.detailedMessage = "This operation will affect multiple items simultaneously.";
    options.type = ConfirmationType::CHECKBOX;
    options.riskLevel = affectedCount > 100 ? RiskLevel::HIGH : RiskLevel::MEDIUM;
    options.showDontAskAgain = affectedCount <= 50;

    ConfirmationAction applyAction;
    applyAction.label = "Apply";
    applyAction.isDestructive = operation.contains("delete", Qt::CaseInsensitive);
    applyAction.iconName = "apply";

    ConfirmationAction cancelAction;
    cancelAction.label = "Cancel";
    cancelAction.isDefault = true;
    cancelAction.iconName = "cancel";

    options.actions = {applyAction, cancelAction};
    options.impactDetails = QStringList()
        << QString("Operation will be applied to %1 items").arg(affectedCount)
        << "Changes may take time to complete"
        << "Some operations cannot be undone";

    return confirm(parent, options);
}

ConfirmationAction ConfirmationDialog::showWithActions(QWidget* parent, const ConfirmationOptions& options) {
    ConfirmationDialog dialog(parent);
    dialog.currentOptions_ = options;
    dialog.operationId_ = options.title.toLower().replace(" ", "_");

    // Check if we should skip asking
    if (options.showDontAskAgain && !shouldAskAgain(dialog.operationId_)) {
        // Return the safest non-destructive action
        for (const ConfirmationAction& action : options.actions) {
            if (!action.isDestructive) {
                return action;
            }
        }
        // If no safe action, return first action
        return options.actions.isEmpty() ? ConfirmationAction() : options.actions.first();
    }

    dialog.setupUI();

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.selectedAction_;
    }

    // User cancelled, return cancel action if available
    for (const ConfirmationAction& action : options.actions) {
        if (action.label.toLower().contains("cancel")) {
            return action;
        }
    }

    return ConfirmationAction(); // Empty action means cancelled
}

bool ConfirmationDialog::shouldAskAgain(const QString& operationId) {
    QSettings settings("ScratchRobin", "Confirmations");
    return settings.value(QString("ask_again_%1").arg(operationId), true).toBool();
}

void ConfirmationDialog::setDontAskAgain(const QString& operationId, bool dontAsk) {
    QSettings settings("ScratchRobin", "Confirmations");
    settings.setValue(QString("ask_again_%1").arg(operationId), !dontAsk);
}

void ConfirmationDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(15);
    mainLayout_->setContentsMargins(20, 20, 20, 20);

    setupHeader();
    setupContent();
    setupActions();

    updateRiskStyling();
}

void ConfirmationDialog::setupHeader() {
    headerWidget_ = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget_);

    // Icon
    iconLabel_ = new QLabel();
    iconLabel_->setFixedSize(48, 48);
    iconLabel_->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(iconLabel_);

    // Text
    QVBoxLayout* textLayout = new QVBoxLayout();

    titleLabel_ = new QLabel();
    titleLabel_->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c5aa0;");
    textLayout->addWidget(titleLabel_);

    messageLabel_ = new QLabel();
    messageLabel_->setWordWrap(true);
    messageLabel_->setStyleSheet("font-size: 12px; color: #333; margin-top: 5px;");
    textLayout->addWidget(messageLabel_);

    headerLayout->addLayout(textLayout);
    headerLayout->addStretch();

    mainLayout_->addWidget(headerWidget_);
}

void ConfirmationDialog::setupContent() {
    contentWidget_ = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget_);

    // Detailed message
    if (!currentOptions_.detailedMessage.isEmpty()) {
        detailedText_ = new QTextEdit();
        detailedText_->setPlainText(currentOptions_.detailedMessage);
        detailedText_->setReadOnly(true);
        detailedText_->setMaximumHeight(80);
        detailedText_->setStyleSheet("QTextEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 4px; padding: 8px; }");
        contentLayout->addWidget(detailedText_);
    }

    // Impact details
    if (!currentOptions_.impactDetails.isEmpty()) {
        impactGroup_ = new QGroupBox("Impact");
        QVBoxLayout* impactLayout = new QVBoxLayout(impactGroup_);

        impactList_ = new QListWidget();
        for (const QString& impact : currentOptions_.impactDetails) {
            QListWidgetItem* item = new QListWidgetItem(impact);
            item->setIcon(QIcon(":/icons/warning.png"));
            impactList_->addItem(item);
        }
        impactList_->setStyleSheet("QListWidget { background-color: #fff3cd; border: 1px solid #ffeaa7; border-radius: 4px; }");
        impactLayout->addWidget(impactList_);

        contentLayout->addWidget(impactGroup_);
    }

    // Consequences
    if (!currentOptions_.consequences.isEmpty()) {
        consequencesGroup_ = new QGroupBox("Consequences");
        QVBoxLayout* consLayout = new QVBoxLayout(consequencesGroup_);

        consequencesList_ = new QListWidget();
        for (const QString& consequence : currentOptions_.consequences) {
            QListWidgetItem* item = new QListWidgetItem(consequence);
            item->setIcon(QIcon(":/icons/error.png"));
            consequencesList_->addItem(item);
        }
        consequencesList_->setStyleSheet("QListWidget { background-color: #f8d7da; border: 1px solid #f5c6cb; border-radius: 4px; }");
        consLayout->addWidget(consequencesList_);

        contentLayout->addWidget(consequencesGroup_);
    }

    // Alternatives
    if (!currentOptions_.alternatives.isEmpty()) {
        alternativesGroup_ = new QGroupBox("Alternatives");
        QVBoxLayout* altLayout = new QVBoxLayout(alternativesGroup_);

        alternativesList_ = new QListWidget();
        for (const QString& alternative : currentOptions_.alternatives) {
            QListWidgetItem* item = new QListWidgetItem(alternative);
            item->setIcon(QIcon(":/icons/info.png"));
            alternativesList_->addItem(item);
        }
        alternativesList_->setStyleSheet("QListWidget { background-color: #d1ecf1; border: 1px solid #bee5eb; border-radius: 4px; }");
        altLayout->addWidget(alternativesList_);

        contentLayout->addWidget(alternativesGroup_);
    }

    mainLayout_->addWidget(contentWidget_);
}

void ConfirmationDialog::setupActions() {
    // Setup specific UI based on confirmation type
    switch (currentOptions_.type) {
        case ConfirmationType::SIMPLE:
            setupSimpleConfirmation();
            break;
        case ConfirmationType::MULTI_OPTION:
            setupMultiOptionConfirmation();
            break;
        case ConfirmationType::CHECKBOX:
            setupCheckboxConfirmation();
            break;
        case ConfirmationType::TIMED:
            setupTimedConfirmation();
            break;
        case ConfirmationType::CRITICAL:
            setupCriticalConfirmation();
            break;
    }

    // Setup bottom section
    setupBottomSection();
}

void ConfirmationDialog::setupSimpleConfirmation() {
    buttonLayout_ = new QHBoxLayout();

    for (const ConfirmationAction& action : currentOptions_.actions) {
        QPushButton* button = new QPushButton(action.label);
        if (!action.iconName.isEmpty()) {
            button->setIcon(QIcon(QString(":/icons/%1.png").arg(action.iconName)));
        }
        if (action.isDefault) {
            button->setDefault(true);
        }
        if (action.isDestructive) {
            button->setStyleSheet("QPushButton { background-color: #dc3545; color: white; border: none; padding: 8px 16px; border-radius: 4px; } QPushButton:hover { background-color: #c82333; }");
        }
        connect(button, &QPushButton::clicked, [this, action]() {
            selectedAction_ = action;
            accept();
        });
        buttonLayout_->addWidget(button);
        actionButtonsList_.append(button);
    }

    mainLayout_->addLayout(buttonLayout_);
}

void ConfirmationDialog::setupMultiOptionConfirmation() {
    actionsGroup_ = new QGroupBox("Choose Action");
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup_);

    actionButtonGroup_ = new QButtonGroup(this);

    for (int i = 0; i < currentOptions_.actions.size(); ++i) {
        const ConfirmationAction& action = currentOptions_.actions[i];
        QRadioButton* radioButton = new QRadioButton(action.label);
        if (!action.description.isEmpty()) {
            radioButton->setToolTip(action.description);
        }
        if (action.isDefault) {
            radioButton->setChecked(true);
        }
        actionButtonGroup_->addButton(radioButton, i);
        actionsLayout->addWidget(radioButton);
        actionButtons_.append(radioButton);
    }

    mainLayout_->addWidget(actionsGroup_);

    // Buttons
    buttonLayout_ = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK");
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &ConfirmationDialog::onActionClicked);
    buttonLayout_->addWidget(okButton);

    QPushButton* cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout_->addWidget(cancelButton);

    mainLayout_->addLayout(buttonLayout_);
}

void ConfirmationDialog::setupCheckboxConfirmation() {
    setupSimpleConfirmation();

    // Add checkbox
    if (currentOptions_.showDontAskAgain) {
        dontAskCheck_ = new QCheckBox(currentOptions_.checkboxText);
        connect(dontAskCheck_, &QCheckBox::toggled, this, &ConfirmationDialog::onDontAskAgainChanged);

        // Insert before buttons
        mainLayout_->insertWidget(mainLayout_->count() - 1, dontAskCheck_);
    }
}

void ConfirmationDialog::setupTimedConfirmation() {
    setupSimpleConfirmation();

    if (currentOptions_.timeoutSeconds > 0) {
        remainingSeconds_ = currentOptions_.timeoutSeconds;

        QHBoxLayout* timeoutLayout = new QHBoxLayout();
        timeoutLabel_ = new QLabel();
        timeoutProgress_ = new QProgressBar();
        timeoutProgress_->setRange(0, currentOptions_.timeoutSeconds);
        timeoutProgress_->setValue(remainingSeconds_);

        timeoutLayout->addWidget(new QLabel("Auto-cancel in:"));
        timeoutLayout->addWidget(timeoutLabel_);
        timeoutLayout->addWidget(timeoutProgress_);

        mainLayout_->insertLayout(mainLayout_->count() - 1, timeoutLayout);

        timeoutTimer_->start(1000); // Update every second
        updateTimeoutDisplay();
    }
}

void ConfirmationDialog::setupCriticalConfirmation() {
    setupMultiOptionConfirmation();

    // Make it more prominent with additional warnings
    titleLabel_->setStyleSheet("font-size: 18px; font-weight: bold; color: #dc3545;");
    messageLabel_->setStyleSheet("font-size: 14px; color: #721c24; font-weight: bold;");

    // Add critical warning icon
    iconLabel_->setPixmap(QPixmap(":/icons/critical.png").scaled(48, 48));
}

void ConfirmationDialog::setupBottomSection() {
    bottomLayout_ = new QHBoxLayout();

    if (!currentOptions_.helpUrl.isEmpty()) {
        helpButton_ = new QPushButton("Help");
        helpButton_->setIcon(QIcon(":/icons/help.png"));
        connect(helpButton_, &QPushButton::clicked, [this]() {
            QDesktopServices::openUrl(QUrl(currentOptions_.helpUrl));
        });
        bottomLayout_->addWidget(helpButton_);
    }

    bottomLayout_->addStretch();

    if (currentOptions_.type == ConfirmationType::MULTI_OPTION ||
        currentOptions_.type == ConfirmationType::CRITICAL) {
        QPushButton* cancelButton = new QPushButton("Cancel");
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        bottomLayout_->addWidget(cancelButton);
    }

    if (!bottomLayout_->isEmpty()) {
        mainLayout_->addLayout(bottomLayout_);
    }
}

void ConfirmationDialog::onActionClicked() {
    if (currentOptions_.type == ConfirmationType::MULTI_OPTION ||
        currentOptions_.type == ConfirmationType::CRITICAL) {
        int selectedId = actionButtonGroup_->checkedId();
        if (selectedId >= 0 && selectedId < currentOptions_.actions.size()) {
            selectedAction_ = currentOptions_.actions[selectedId];
        }
    }

    accept();
}

void ConfirmationDialog::onDontAskAgainChanged(bool checked) {
    if (checked) {
        setDontAskAgain(operationId_, true);
    }
}

void ConfirmationDialog::onTimeoutUpdate() {
    remainingSeconds_--;
    updateTimeoutDisplay();

    if (remainingSeconds_ <= 0) {
        onTimeoutReached();
    }
}

void ConfirmationDialog::onTimeoutReached() {
    timeoutTimer_->stop();

    // Find cancel action or first non-destructive action
    for (const ConfirmationAction& action : currentOptions_.actions) {
        if (action.label.toLower().contains("cancel") || !action.isDestructive) {
            selectedAction_ = action;
            break;
        }
    }

    emit timeoutReached();
    accept();
}

void ConfirmationDialog::updateRiskStyling() {
    QColor riskColor = getRiskColor(currentOptions_.riskLevel);
    QString riskIcon = getRiskIcon(currentOptions_.riskLevel);

    if (!currentOptions_.customIcon.isNull()) {
        iconLabel_->setPixmap(currentOptions_.customIcon.scaled(48, 48));
    } else if (!riskIcon.isEmpty()) {
        iconLabel_->setPixmap(QPixmap(riskIcon).scaled(48, 48));
    }

    // Apply color theming based on risk level
    QString bgColor = riskColor.name();
    headerWidget_->setStyleSheet(QString("QWidget { background-color: %1; border-radius: 8px; padding: 10px; } QLabel { color: white; }").arg(bgColor));
}

void ConfirmationDialog::updateTimeoutDisplay() {
    if (timeoutLabel_ && timeoutProgress_) {
        timeoutLabel_->setText(QString("%1 seconds").arg(remainingSeconds_));
        timeoutProgress_->setValue(remainingSeconds_);
    }
}

QString ConfirmationDialog::getRiskIcon(RiskLevel risk) const {
    switch (risk) {
        case RiskLevel::LOW:
            return ":/icons/info.png";
        case RiskLevel::MEDIUM:
            return ":/icons/warning.png";
        case RiskLevel::HIGH:
            return ":/icons/error.png";
        case RiskLevel::CRITICAL:
            return ":/icons/critical.png";
        default:
            return ":/icons/warning.png";
    }
}

QColor ConfirmationDialog::getRiskColor(RiskLevel risk) const {
    switch (risk) {
        case RiskLevel::LOW:
            return QColor("#17a2b8"); // Info blue
        case RiskLevel::MEDIUM:
            return QColor("#ffc107"); // Warning yellow
        case RiskLevel::HIGH:
            return QColor("#fd7e14"); // High orange
        case RiskLevel::CRITICAL:
            return QColor("#dc3545"); // Critical red
        default:
            return QColor("#6c757d"); // Gray
    }
}

QString ConfirmationDialog::formatImpactText() const {
    QStringList impactText;
    for (const QString& impact : currentOptions_.impactDetails) {
        impactText << QString("• %1").arg(impact);
    }
    return impactText.join("\n");
}

} // namespace scratchrobin
