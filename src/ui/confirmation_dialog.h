#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QProgressBar>
#include <QTimer>
#include <QDateTime>
#include <QGroupBox>
#include <QListWidget>

namespace scratchrobin {

enum class ConfirmationType {
    SIMPLE,         // Basic yes/no confirmation
    MULTI_OPTION,   // Multiple choice options
    CHECKBOX,       // With "Don't ask again" checkbox
    TIMED,          // With countdown timer
    CRITICAL        // High-risk operation with detailed warnings
};

enum class RiskLevel {
    LOW,        // Safe operations (e.g., closing dialog)
    MEDIUM,     // Moderate risk (e.g., deleting a row)
    HIGH,       // High risk (e.g., dropping table)
    CRITICAL    // Critical risk (e.g., dropping database)
};

struct ConfirmationAction {
    QString label;
    QString description;
    QString iconName;
    bool isDestructive = false;
    bool isDefault = false;
    QVariant userData;
};

struct ConfirmationOptions {
    QString title;
    QString message;
    QString detailedMessage;
    ConfirmationType type = ConfirmationType::SIMPLE;
    RiskLevel riskLevel = RiskLevel::MEDIUM;
    QList<ConfirmationAction> actions;
    QStringList impactDetails;
    QStringList consequences;
    QStringList alternatives;
    bool showDontAskAgain = false;
    int timeoutSeconds = 0; // 0 = no timeout
    QString helpUrl;
    QPixmap customIcon;
    QString checkboxText = "Don't ask me again";
};

class ConfirmationDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfirmationDialog(QWidget* parent = nullptr);
    ~ConfirmationDialog() override = default;

    // Main confirmation methods
    static bool confirm(QWidget* parent, const ConfirmationOptions& options);
    static bool confirmDelete(QWidget* parent, const QString& itemType, const QString& itemName, int itemCount = 1);
    static bool confirmDropTable(QWidget* parent, const QString& tableName, int rowCount = 0);
    static bool confirmDropDatabase(QWidget* parent, const QString& databaseName);
    static bool confirmOverwrite(QWidget* parent, const QString& fileName, const QString& operation = "save");
    static bool confirmCloseUnsaved(QWidget* parent, int unsavedCount = 1);
    static bool confirmBulkOperation(QWidget* parent, const QString& operation, int affectedCount);

    // Advanced confirmation
    static ConfirmationAction showWithActions(QWidget* parent, const ConfirmationOptions& options);

    // Settings integration
    static bool shouldAskAgain(const QString& operationId);
    static void setDontAskAgain(const QString& operationId, bool dontAsk);

signals:
    void actionSelected(const ConfirmationAction& action);
    void timeoutReached();
    void dontAskAgainChanged(const QString& operationId, bool dontAsk);

private slots:
    void onActionClicked();
    void onDontAskAgainChanged(bool checked);
    void onTimeoutUpdate();
    void onTimeoutReached();

private:
    void setupUI();
    void setupSimpleConfirmation();
    void setupMultiOptionConfirmation();
    void setupCheckboxConfirmation();
    void setupTimedConfirmation();
    void setupCriticalConfirmation();

    void updateRiskStyling();
    void updateTimeoutDisplay();
    QString getRiskIcon(RiskLevel risk) const;
    QColor getRiskColor(RiskLevel risk) const;
    QString formatImpactText() const;

    // UI setup methods
    void setupHeader();
    void setupContent();
    void setupActions();
    void setupBottomSection();

    // UI Components
    QVBoxLayout* mainLayout_;

    // Header section
    QWidget* headerWidget_;
    QLabel* iconLabel_;
    QLabel* titleLabel_;
    QLabel* messageLabel_;

    // Content sections
    QGroupBox* impactGroup_;
    QListWidget* impactList_;
    QGroupBox* consequencesGroup_;
    QListWidget* consequencesList_;
    QGroupBox* alternativesGroup_;
    QListWidget* alternativesList_;

    // Content area
    QWidget* contentWidget_;
    QTextEdit* detailedText_;

    // Action selection (for multi-option)
    QGroupBox* actionsGroup_;
    QButtonGroup* actionButtonGroup_;
    QList<QRadioButton*> actionButtons_;

    // Bottom section
    QHBoxLayout* bottomLayout_;

    // Checkbox
    QCheckBox* dontAskCheck_;

    // Timeout display
    QLabel* timeoutLabel_;
    QProgressBar* timeoutProgress_;

    // Buttons
    QHBoxLayout* buttonLayout_;
    QList<QPushButton*> actionButtonsList_;
    QPushButton* helpButton_;

    // Current state
    ConfirmationOptions currentOptions_;
    QTimer* timeoutTimer_;
    int remainingSeconds_;
    QString operationId_;
    ConfirmationAction selectedAction_;
};

} // namespace scratchrobin
