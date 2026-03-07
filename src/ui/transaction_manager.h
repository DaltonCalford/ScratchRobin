#pragma once
#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QWidget;
class QLabel;
class QPushButton;
class QComboBox;
class QAction;
class QLineEdit;
class QTableWidget;
class QListWidget;
class QTextEdit;
class QHeaderView;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Transaction Management System
 * 
 * Implements FORM_SPECIFICATION.md Transaction Support section:
 * - Transaction state tracking
 * - Auto-commit toggle
 * - Isolation level selection
 * - Visual transaction indicator
 * - Commit/Rollback with confirmation
 */

// ============================================================================
// Transaction State
// ============================================================================
struct TransactionState {
    bool inTransaction = false;
    bool autoCommit = true;
    QString isolationLevel = "READ COMMITTED";
    QDateTime startTime;
    int statementCount = 0;
    int savepointCount = 0;
    QStringList executedStatements;
    
    bool isActive() const { return inTransaction && !autoCommit; }
};

// ============================================================================
// Transaction Manager
// ============================================================================
class TransactionManager : public QObject {
    Q_OBJECT

public:
    explicit TransactionManager(backend::SessionClient* client, QObject* parent = nullptr);

    // State queries
    bool inTransaction() const { return state_.inTransaction; }
    bool autoCommit() const { return state_.autoCommit; }
    QString isolationLevel() const { return state_.isolationLevel; }
    const TransactionState& state() const { return state_; }
    
    // Available isolation levels
    QStringList supportedIsolationLevels() const;
    
    // Transaction info
    int statementCount() const { return state_.statementCount; }
    qint64 transactionDurationMs() const;
    QStringList pendingStatements() const { return state_.executedStatements; }

public slots:
    // Auto-commit control
    void setAutoCommit(bool enabled);
    void toggleAutoCommit();
    
    // Isolation level
    void setIsolationLevel(const QString& level);
    
    // Transaction control
    bool beginTransaction();
    bool commit();
    bool rollback();
    bool rollbackToSavepoint(const QString& name);
    bool createSavepoint(const QString& name);
    bool releaseSavepoint(const QString& name);
    
    // Statement tracking
    void recordStatement(const QString& sql);
    void clearTransaction();

signals:
    void transactionStarted();
    void transactionCommitted();
    void transactionRolledBack();
    void savepointCreated(const QString& name);
    void savepointReleased(const QString& name);
    void savepointRolledBack(const QString& name);
    void autoCommitChanged(bool enabled);
    void isolationLevelChanged(const QString& level);
    void stateChanged(const TransactionState& state);
    void transactionError(const QString& error);

private:
    bool executeSql(const QString& sql);
    void updateState();
    
    backend::SessionClient* client_ = nullptr;
    TransactionState state_;
    
    static const QStringList ISOLATION_LEVELS;
};

// ============================================================================
// Transaction Indicator Widget (status bar)
// ============================================================================
class TransactionIndicator : public QWidget {
    Q_OBJECT

public:
    explicit TransactionIndicator(QWidget* parent = nullptr);

    void setState(const TransactionState& state);
    void setInTransaction(bool inTransaction);
    void setAutoCommit(bool autoCommit);
    void setStatementCount(int count);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void setupUi();
    void updateAppearance();
    
    bool inTransaction_ = false;
    bool autoCommit_ = true;
    int statementCount_ = 0;
    QLabel* statusLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;
};

// ============================================================================
// Transaction Toolbar
// ============================================================================
class TransactionToolbar : public QWidget {
    Q_OBJECT

public:
    explicit TransactionToolbar(TransactionManager* manager, QWidget* parent = nullptr);

    QAction* commitAction() const { return commitAction_; }
    QAction* rollbackAction() const { return rollbackAction_; }
    QAction* autoCommitAction() const { return autoCommitAction_; }

private slots:
    void onCommit();
    void onRollback();
    void onToggleAutoCommit();
    void onIsolationLevelChanged(int index);
    void updateButtonStates();

private:
    void setupUi();

    TransactionManager* manager_ = nullptr;
    
    QPushButton* commitBtn_ = nullptr;
    QPushButton* rollbackBtn_ = nullptr;
    QPushButton* savepointBtn_ = nullptr;
    QComboBox* isolationCombo_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    
    QAction* commitAction_ = nullptr;
    QAction* rollbackAction_ = nullptr;
    QAction* autoCommitAction_ = nullptr;
};

// ============================================================================
// Transaction Dialog (detailed view)
// ============================================================================
class TransactionDialog : public QDialog {
    Q_OBJECT

public:
    explicit TransactionDialog(TransactionManager* manager, QWidget* parent = nullptr);

private slots:
    void refresh();
    void onCommit();
    void onRollback();
    void onCreateSavepoint();
    void onRollbackToSavepoint();
    void onReleaseSavepoint();
    void updateButtons();

private:
    void setupUi();

    TransactionManager* manager_ = nullptr;
    
    class QTextEdit* logEdit_ = nullptr;
    class QListWidget* savepointList_ = nullptr;
    class QLabel* durationLabel_ = nullptr;
    class QLabel* statementCountLabel_ = nullptr;
    class QPushButton* commitBtn_ = nullptr;
    class QPushButton* rollbackBtn_ = nullptr;
};

// ============================================================================
// Savepoint Manager Dialog
// ============================================================================
class SavepointManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit SavepointManagerDialog(TransactionManager* manager, QWidget* parent = nullptr);

private slots:
    void refresh();
    void onCreateSavepoint();
    void onRollbackToSavepoint();
    void onReleaseSavepoint();
    void onReleaseAll();

private:
    void setupUi();

    TransactionManager* manager_ = nullptr;
    class QTableWidget* savepointTable_ = nullptr;
    class QLineEdit* nameEdit_ = nullptr;
};

} // namespace scratchrobin::ui
