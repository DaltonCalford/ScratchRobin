#pragma once
#include <QDialog>
#include <QDateTime>
#include <QColor>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPlainTextEdit;
class QComboBox;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;
class QCheckBox;
class QTabWidget;
class QSplitter;
class QStackedWidget;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;
class QDialogButtonBox;
class QVBoxLayout;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Query Comments - Annotations on queries
 * 
 * Add comments and annotations to SQL queries:
 * - Line-specific comments
 * - General query documentation
 * - Review threads and discussions
 * - Comment categories/tags
 * - @mentions for team members
 * - Comment status (open/resolved)
 */

// ============================================================================
// Comment Types and Structures
// ============================================================================
enum class CommentType {
    LineComment,        // Comment on specific line
    BlockComment,       // Comment on a range of lines
    GeneralComment,     // General query comment
    ReviewComment,      // Code review style comment
    Question,           // Question about the query
    Suggestion          // Improvement suggestion
};

enum class CommentStatus {
    Open,
    Resolved,
    Closed
};

// ============================================================================
// Comment Structure
// ============================================================================
struct QueryComment {
    QString id;
    QString queryId;
    QString queryName;
    CommentType type;
    CommentStatus status;
    QString author;
    QString authorId;
    QDateTime createdAt;
    QDateTime updatedAt;
    QString content;
    QStringList mentions;
    QStringList tags;
    
    // Line/block specific
    int startLine = -1;
    int endLine = -1;
    int startColumn = -1;
    int endColumn = -1;
    QString selectedText;
    
    // Thread support
    QString parentId;  // For replies
    QStringList replyIds;
    int replyCount = 0;
};

struct CommentThread {
    QString id;
    QueryComment rootComment;
    QList<QueryComment> replies;
    bool isResolved = false;
    QString resolvedBy;
    QDateTime resolvedAt;
};

// ============================================================================
// Query Comments Dialog
// ============================================================================
class QueryCommentsDialog : public QDialog {
    Q_OBJECT

public:
    explicit QueryCommentsDialog(const QString& queryId, const QString& queryName,
                                 const QString& queryContent,
                                 backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    // Navigation
    void onCommentSelected(const QModelIndex& index);
    void onFilterByType(int type);
    void onFilterByStatus(int status);
    void onSearchTextChanged(const QString& text);
    
    // Comment actions
    void onNewComment();
    void onReplyToComment();
    void onEditComment();
    void onDeleteComment();
    void onResolveComment();
    void onReopenComment();
    
    // Review actions
    void onStartReview();
    void onSubmitReview();
    void onCancelReview();
    
    // General
    void onRefreshComments();
    void onExportComments();

signals:
    void commentAdded(const QueryComment& comment);
    void commentUpdated(const QueryComment& comment);
    void commentDeleted(const QString& commentId);

private:
    void setupUi();
    void setupCommentsList();
    void setupCommentDetails();
    void setupEditor();
    void loadComments();
    void updateCommentsList();
    void updateCommentDetails(const QueryComment& comment);
    void highlightCommentInEditor(const QueryComment& comment);
    
    QString queryId_;
    QString queryName_;
    QString queryContent_;
    backend::SessionClient* client_;
    
    QList<CommentThread> threads_;
    QueryComment currentComment_;
    bool inReviewMode_ = false;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QSplitter* mainSplitter_ = nullptr;
    
    // Comments list
    QTreeWidget* commentsTree_ = nullptr;
    QComboBox* typeFilterCombo_ = nullptr;
    QComboBox* statusFilterCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    // Comment details
    QStackedWidget* detailsStack_ = nullptr;
    QLabel* commentAuthorLabel_ = nullptr;
    QLabel* commentDateLabel_ = nullptr;
    QLabel* commentLocationLabel_ = nullptr;
    QTextBrowser* commentContentBrowser_ = nullptr;
    QListWidget* repliesList_ = nullptr;
    
    // Editor
    QPlainTextEdit* queryEditor_ = nullptr;
    
    // Actions
    QPushButton* newCommentBtn_ = nullptr;
    QPushButton* replyBtn_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* deleteBtn_ = nullptr;
    QPushButton* resolveBtn_ = nullptr;
};

// ============================================================================
// Add Comment Dialog
// ============================================================================
class AddCommentDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddCommentDialog(const QString& queryId, const QString& queryContent,
                              int startLine, int endLine,
                              const QString& selectedText,
                              backend::SessionClient* client, QWidget* parent = nullptr);
    
    QueryComment comment() const;

public slots:
    void onTypeChanged(int index);
    void onAddMention();
    void onAddTag();
    void onValidate();

private:
    void setupUi();
    
    QString queryId_;
    QString queryContent_;
    int startLine_;
    int endLine_;
    QString selectedText_;
    backend::SessionClient* client_;
    
    QComboBox* typeCombo_ = nullptr;
    QTextEdit* contentEdit_ = nullptr;
    QLabel* locationLabel_ = nullptr;
    QListWidget* mentionsList_ = nullptr;
    QListWidget* tagsList_ = nullptr;
    QLineEdit* mentionEdit_ = nullptr;
    QLineEdit* tagEdit_ = nullptr;
    QDialogButtonBox* buttonBox_ = nullptr;
};

// ============================================================================
// Edit Comment Dialog
// ============================================================================
class EditCommentDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditCommentDialog(const QueryComment& comment,
                               backend::SessionClient* client, QWidget* parent = nullptr);
    
    QueryComment comment() const;

public slots:
    void onValidate();

private:
    void setupUi();
    
    QueryComment originalComment_;
    backend::SessionClient* client_;
    
    QTextEdit* contentEdit_ = nullptr;
    QComboBox* statusCombo_ = nullptr;
    QDialogButtonBox* buttonBox_ = nullptr;
};

// ============================================================================
// Comment Thread Widget
// ============================================================================
class CommentThreadWidget : public QWidget {
    Q_OBJECT

public:
    explicit CommentThreadWidget(const CommentThread& thread, QWidget* parent = nullptr);

public slots:
    void onReplyClicked();
    void onResolveClicked();
    void onEditClicked();
    void onDeleteClicked();

signals:
    void replyRequested(const QString& commentId);
    void resolveRequested(const QString& threadId);
    void editRequested(const QString& commentId);
    void deleteRequested(const QString& commentId);

private:
    void setupUi();
    void addCommentWidget(const QueryComment& comment, int indent = 0);
    
    CommentThread thread_;
    QVBoxLayout* commentsLayout_ = nullptr;
};

// ============================================================================
// Inline Comment Widget
// ============================================================================
class InlineCommentWidget : public QWidget {
    Q_OBJECT

public:
    explicit InlineCommentWidget(const QueryComment& comment, QWidget* parent = nullptr);

signals:
    void clicked();
    void resolveRequested(const QString& commentId);

private:
    void setupUi();
    void updateAppearance();
    
    QueryComment comment_;
    QLabel* indicatorLabel_ = nullptr;
    QColor indicatorColor_;
};

// ============================================================================
// Comment Overview Panel
// ============================================================================
class CommentOverviewPanel : public QWidget {
    Q_OBJECT

public:
    explicit CommentOverviewPanel(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onQueryChanged(const QString& queryId, const QString& queryName);
    void onRefresh();
    void onFilterChanged(const QString& filter);

signals:
    void commentSelected(const QString& commentId);
    void navigateToLine(int line);

private:
    void setupUi();
    void loadComments();
    void updateOverview();
    
    backend::SessionClient* client_;
    QString currentQueryId_;
    QList<CommentThread> threads_;
    
    QLabel* summaryLabel_ = nullptr;
    QTreeWidget* commentsTree_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
};

} // namespace scratchrobin::ui
