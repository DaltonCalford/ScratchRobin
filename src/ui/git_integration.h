#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QHash>
#include <QVariant>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QListWidget;
class QLabel;
class QSplitter;
class QCheckBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Git Integration - Version control for SQL scripts and objects
 * 
 * - Git repository browser
 * - SQL script versioning
 * - Object change tracking
 * - Branch management
 * - Commit/merge/push/pull operations
 */

// ============================================================================
// Git File Info
// ============================================================================
struct GitFileInfo {
    QString path;
    QString status;  // modified, added, deleted, untracked, ignored
    QString diff;
    int additions = 0;
    int deletions = 0;
};

// ============================================================================
// Git Commit Info
// ============================================================================
struct GitCommitInfo {
    QString hash;
    QString shortHash;
    QString message;
    QString author;
    QDateTime date;
    QStringList parents;
    QStringList changedFiles;
};

// ============================================================================
// Git Integration Panel
// ============================================================================
class GitIntegrationPanel : public DockPanel {
    Q_OBJECT

public:
    explicit GitIntegrationPanel(QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Git Integration"); }
    QString panelCategory() const override { return "version"; }

    void setRepositoryPath(const QString& path);
    void refresh();

public slots:
    void onInitRepository();
    void onCloneRepository();
    void onCommit();
    void onStash();
    void onBranch();
    void onMerge();
    void onPull();
    void onPush();
    void onFetch();
    void onShowHistory();
    void onShowDiff();
    void onBlame();

signals:
    void fileSelected(const QString& path);
    void commitSelected(const QString& hash);

protected:
    void panelActivated() override;

private slots:
    void onFileDoubleClicked(const QModelIndex& index);
    void onCommitDoubleClicked(const QModelIndex& index);

private:
    void setupUi();
    void setupFileTree();
    void setupCommitList();
    void setupToolbar();
    void updateStatus();
    
    QString repoPath_;
    QString currentBranch_;
    
    // UI
    QSplitter* splitter_ = nullptr;
    QTreeView* fileTree_ = nullptr;
    QTableView* commitTable_ = nullptr;
    QTextEdit* diffView_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QStandardItemModel* fileModel_ = nullptr;
    QStandardItemModel* commitModel_ = nullptr;
    
    // Actions
    QPushButton* initBtn_ = nullptr;
    QPushButton* commitBtn_ = nullptr;
    QPushButton* pushBtn_ = nullptr;
    QPushButton* pullBtn_ = nullptr;
};

// ============================================================================
// Git Commit Dialog
// ============================================================================
class GitCommitDialog : public QDialog {
    Q_OBJECT

public:
    explicit GitCommitDialog(QWidget* parent = nullptr);
    
    void setStagedFiles(const QStringList& files);
    void setUnstagedFiles(const QStringList& files);

public slots:
    void onStageAll();
    void onUnstageAll();
    void onStageSelected();
    void onUnstageSelected();
    void onCommit();

private:
    void setupUi();
    void updatePreview();
    
    QListWidget* stagedList_ = nullptr;
    QListWidget* unstagedList_ = nullptr;
    QTextEdit* messageEdit_ = nullptr;
    QCheckBox* amendCheck_ = nullptr;
    QCheckBox* signOffCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Git Branch Dialog
// ============================================================================
class GitBranchDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { BranchList, Create, Delete, Merge, Checkout };
    
    explicit GitBranchDialog(Mode mode, QWidget* parent = nullptr);

public slots:
    void onCreateBranch();
    void onDeleteBranch();
    void onMergeBranch();
    void onCheckoutBranch();
    void onPushBranch();

private:
    void setupUi();
    void loadBranches();
    
    Mode mode_;
    QListWidget* branchList_ = nullptr;
    QLineEdit* newNameEdit_ = nullptr;
    QComboBox* baseCombo_ = nullptr;
    QCheckBox* checkoutCheck_ = nullptr;
};

// ============================================================================
// Git History Dialog
// ============================================================================
class GitHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit GitHistoryDialog(QWidget* parent = nullptr);
    
    void setFilePath(const QString& path);

public slots:
    void onFilterChanged(const QString& text);
    void onCheckoutCommit();
    void onCreateBranchFromCommit();
    void onCherryPickCommit();
    void onRevertCommit();
    void onCompareCommits();

private:
    void setupUi();
    void loadHistory();
    void updateDiff();
    
    QString filePath_;
    
    QTableView* historyTable_ = nullptr;
    QTextEdit* commitDetails_ = nullptr;
    QTextEdit* diffView_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QStandardItemModel* model_ = nullptr;
};

// ============================================================================
// Git Diff Dialog
// ============================================================================
class GitDiffDialog : public QDialog {
    Q_OBJECT

public:
    explicit GitDiffDialog(QWidget* parent = nullptr);
    
    void setFilePath(const QString& path);
    void setCommits(const QString& fromCommit, const QString& toCommit);

public slots:
    void onPreviousChange();
    void onNextChange();
    void onStageHunk();
    void onUnstageHunk();

private:
    void setupUi();
    void loadDiff();
    
    QString filePath_;
    QString fromCommit_;
    QString toCommit_;
    
    QTextEdit* diffEdit_ = nullptr;
    QLabel* statsLabel_ = nullptr;
};

} // namespace scratchrobin::ui
