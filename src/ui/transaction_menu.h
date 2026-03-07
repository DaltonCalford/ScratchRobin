#pragma once
#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
class QWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Transaction menu implementation for database transaction control
 * 
 * Implements MENU_SPECIFICATION.md §Transaction section:
 * - Start Transaction
 * - Commit / Rollback
 * - Set Savepoint
 * - Auto Commit toggle
 * - Isolation Level submenu
 * - Read Only toggle
 */
class TransactionMenu : public QObject {
    Q_OBJECT

public:
    explicit TransactionMenu(QWidget* parent = nullptr);
    ~TransactionMenu() override;

    QMenu* menu() const { return menu_; }

    // Transaction state
    enum class TxState {
        NoTransaction,    // Not in a transaction
        InTransaction,    // Active transaction
        AutoCommit        // Auto-commit mode
    };

    // State management
    void setConnected(bool connected);
    void setTransactionState(TxState state);
    void setAutoCommit(bool autoCommit);
    void setIsolationLevel(const QString& level);
    void setReadOnly(bool readOnly);

signals:
    void startTransactionRequested();
    void commitRequested();
    void rollbackRequested();
    void setSavepointRequested();
    void autoCommitToggled(bool enabled);
    void isolationLevelChanged(const QString& level);
    void readOnlyToggled(bool enabled);

private slots:
    void onStartTransaction();
    void onCommit();
    void onRollback();
    void onSetSavepoint();
    void onAutoCommitToggled(bool checked);
    void onIsolationLevelTriggered();
    void onReadOnlyToggled(bool checked);

private:
    void setupActions();
    void updateActionStates();
    void createIsolationSubmenu();

    QWidget* parent_;
    QMenu* menu_ = nullptr;
    QMenu* isolationMenu_ = nullptr;

    // State
    bool connected_ = false;
    TxState txState_ = TxState::NoTransaction;
    bool autoCommit_ = true;
    QString isolationLevel_ = "Read Committed";
    bool readOnly_ = false;

    // Actions
    QAction* actionStart_ = nullptr;
    QAction* actionCommit_ = nullptr;
    QAction* actionRollback_ = nullptr;
    QAction* actionSavepoint_ = nullptr;
    QAction* actionAutoCommit_ = nullptr;
    QAction* actionReadOnly_ = nullptr;
    
    // Isolation level actions
    QAction* actionReadUncommitted_ = nullptr;
    QAction* actionReadCommitted_ = nullptr;
    QAction* actionRepeatableRead_ = nullptr;
    QAction* actionSerializable_ = nullptr;
};

} // namespace scratchrobin::ui
