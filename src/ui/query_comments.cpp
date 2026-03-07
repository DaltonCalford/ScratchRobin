#include "query_comments.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QGroupBox>
#include <QScrollArea>
#include <QHeaderView>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QTextBlock>

namespace scratchrobin::ui {

// ============================================================================
// Query Comments Dialog
// ============================================================================
QueryCommentsDialog::QueryCommentsDialog(const QString& queryId, const QString& queryName,
                                         const QString& queryContent,
                                         backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), queryId_(queryId), queryName_(queryName),
      queryContent_(queryContent), client_(client) {
    setupUi();
    loadComments();
}

void QueryCommentsDialog::setupUi() {
    setWindowTitle(tr("Comments: %1").arg(queryName_));
    setMinimumSize(800, 600);
    
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    newCommentBtn_ = new QPushButton(tr("New Comment"), this);
    connect(newCommentBtn_, &QPushButton::clicked, this, &QueryCommentsDialog::onNewComment);
    toolbarLayout->addWidget(newCommentBtn_);
    
    replyBtn_ = new QPushButton(tr("Reply"), this);
    replyBtn_->setEnabled(false);
    connect(replyBtn_, &QPushButton::clicked, this, &QueryCommentsDialog::onReplyToComment);
    toolbarLayout->addWidget(replyBtn_);
    
    editBtn_ = new QPushButton(tr("Edit"), this);
    editBtn_->setEnabled(false);
    connect(editBtn_, &QPushButton::clicked, this, &QueryCommentsDialog::onEditComment);
    toolbarLayout->addWidget(editBtn_);
    
    deleteBtn_ = new QPushButton(tr("Delete"), this);
    deleteBtn_->setEnabled(false);
    connect(deleteBtn_, &QPushButton::clicked, this, &QueryCommentsDialog::onDeleteComment);
    toolbarLayout->addWidget(deleteBtn_);
    
    resolveBtn_ = new QPushButton(tr("Resolve"), this);
    resolveBtn_->setEnabled(false);
    connect(resolveBtn_, &QPushButton::clicked, this, &QueryCommentsDialog::onResolveComment);
    toolbarLayout->addWidget(resolveBtn_);
    
    toolbarLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &QueryCommentsDialog::onRefreshComments);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);
    
    // Left panel - Comments list
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    typeFilterCombo_ = new QComboBox(this);
    typeFilterCombo_->addItem(tr("All Types"));
    typeFilterCombo_->addItem(tr("Line Comments"));
    typeFilterCombo_->addItem(tr("Block Comments"));
    typeFilterCombo_->addItem(tr("General"));
    typeFilterCombo_->addItem(tr("Review"));
    typeFilterCombo_->addItem(tr("Questions"));
    typeFilterCombo_->addItem(tr("Suggestions"));
    connect(typeFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QueryCommentsDialog::onFilterByType);
    filterLayout->addWidget(typeFilterCombo_);
    
    statusFilterCombo_ = new QComboBox(this);
    statusFilterCombo_->addItem(tr("All Status"));
    statusFilterCombo_->addItem(tr("Open"));
    statusFilterCombo_->addItem(tr("Resolved"));
    statusFilterCombo_->addItem(tr("Closed"));
    connect(statusFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QueryCommentsDialog::onFilterByStatus);
    filterLayout->addWidget(statusFilterCombo_);
    
    leftLayout->addLayout(filterLayout);
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search comments..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &QueryCommentsDialog::onSearchTextChanged);
    leftLayout->addWidget(searchEdit_);
    
    // Comments tree
    commentsTree_ = new QTreeWidget(this);
    commentsTree_->setHeaderLabels({tr("Comment"), tr("Author"), tr("Date"), tr("Status")});
    commentsTree_->setRootIsDecorated(true);
    commentsTree_->setAlternatingRowColors(true);
    connect(commentsTree_, &QTreeWidget::itemClicked,
            [this](QTreeWidgetItem* item, int) {
                if (item) {
                    QString commentId = item->data(0, Qt::UserRole).toString();
                    // Find and select comment
                    for (const auto& thread : threads_) {
                        if (thread.rootComment.id == commentId) {
                            currentComment_ = thread.rootComment;
                            updateCommentDetails(currentComment_);
                            break;
                        }
                    }
                }
            });
    leftLayout->addWidget(commentsTree_, 1);
    
    mainSplitter_->addWidget(leftWidget);
    
    // Right panel - Query and comment details
    auto* rightSplitter = new QSplitter(Qt::Vertical, this);
    
    // Query editor
    auto* editorGroup = new QGroupBox(tr("Query"), this);
    auto* editorLayout = new QVBoxLayout(editorGroup);
    queryEditor_ = new QPlainTextEdit(this);
    queryEditor_->setPlainText(queryContent_);
    queryEditor_->setReadOnly(true);
    editorLayout->addWidget(queryEditor_);
    rightSplitter->addWidget(editorGroup);
    
    // Comment details
    auto* detailsGroup = new QGroupBox(tr("Comment Details"), this);
    auto* detailsLayout = new QVBoxLayout(detailsGroup);
    
    auto* detailsHeaderLayout = new QHBoxLayout();
    commentAuthorLabel_ = new QLabel(this);
    detailsHeaderLayout->addWidget(commentAuthorLabel_);
    detailsHeaderLayout->addStretch();
    commentDateLabel_ = new QLabel(this);
    detailsHeaderLayout->addWidget(commentDateLabel_);
    detailsLayout->addLayout(detailsHeaderLayout);
    
    commentLocationLabel_ = new QLabel(this);
    detailsLayout->addWidget(commentLocationLabel_);
    
    commentContentBrowser_ = new QTextBrowser(this);
    commentContentBrowser_->setMaximumHeight(150);
    detailsLayout->addWidget(commentContentBrowser_);
    
    detailsLayout->addWidget(new QLabel(tr("Replies:"), this));
    repliesList_ = new QListWidget(this);
    repliesList_->setMaximumHeight(100);
    detailsLayout->addWidget(repliesList_);
    
    rightSplitter->addWidget(detailsGroup);
    rightSplitter->setSizes({300, 200});
    
    mainSplitter_->addWidget(rightSplitter);
    mainSplitter_->setSizes({300, 500});
    
    mainLayout->addWidget(mainSplitter_, 1);
    
    // Summary label
    auto* summaryLabel = new QLabel(tr("Loading comments..."), this);
    mainLayout->addWidget(summaryLabel);
}

void QueryCommentsDialog::loadComments() {
    threads_.clear();
    
    // Sample data
    CommentThread thread1;
    thread1.id = "thread1";
    thread1.rootComment.id = "c1";
    thread1.rootComment.queryId = queryId_;
    thread1.rootComment.type = CommentType::ReviewComment;
    thread1.rootComment.status = CommentStatus::Open;
    thread1.rootComment.author = tr("John Doe");
    thread1.rootComment.createdAt = QDateTime::currentDateTime().addDays(-2);
    thread1.rootComment.content = tr("Consider adding an index on the customer_id column for better performance.");
    thread1.rootComment.startLine = 5;
    thread1.rootComment.endLine = 5;
    thread1.rootComment.replyCount = 1;
    
    QueryComment reply1;
    reply1.id = "c1r1";
    reply1.parentId = "c1";
    reply1.author = tr("Jane Smith");
    reply1.createdAt = QDateTime::currentDateTime().addDays(-1);
    reply1.content = tr("Good point! I'll add that.");
    thread1.replies.append(reply1);
    
    threads_.append(thread1);
    
    CommentThread thread2;
    thread2.id = "thread2";
    thread2.rootComment.id = "c2";
    thread2.rootComment.queryId = queryId_;
    thread2.rootComment.type = CommentType::Question;
    thread2.rootComment.status = CommentStatus::Resolved;
    thread2.rootComment.author = tr("Bob Wilson");
    thread2.rootComment.createdAt = QDateTime::currentDateTime().addDays(-5);
    thread2.rootComment.content = tr("Why are we using LEFT JOIN here instead of INNER JOIN?");
    thread2.rootComment.startLine = 10;
    thread2.rootComment.endLine = 12;
    thread2.isResolved = true;
    thread2.resolvedBy = tr("John Doe");
    thread2.resolvedAt = QDateTime::currentDateTime().addDays(-3);
    threads_.append(thread2);
    
    CommentThread thread3;
    thread3.id = "thread3";
    thread3.rootComment.id = "c3";
    thread3.rootComment.queryId = queryId_;
    thread3.rootComment.type = CommentType::Suggestion;
    thread3.rootComment.status = CommentStatus::Open;
    thread3.rootComment.author = tr("Alice Brown");
    thread3.rootComment.createdAt = QDateTime::currentDateTime().addDays(-1);
    thread3.rootComment.content = tr("We could simplify this using a CTE.");
    thread3.rootComment.tags = QStringList({"refactoring"});
    threads_.append(thread3);
    
    updateCommentsList();
}

void QueryCommentsDialog::updateCommentsList() {
    commentsTree_->clear();
    
    for (const auto& thread : threads_) {
        const auto& comment = thread.rootComment;
        
        QString typeIcon;
        switch (comment.type) {
            case CommentType::LineComment: typeIcon = tr("📍"); break;
            case CommentType::BlockComment: typeIcon = tr("📌"); break;
            case CommentType::GeneralComment: typeIcon = tr("💬"); break;
            case CommentType::ReviewComment: typeIcon = tr("👁"); break;
            case CommentType::Question: typeIcon = tr("❓"); break;
            case CommentType::Suggestion: typeIcon = tr("💡"); break;
        }
        
        QString statusStr;
        switch (comment.status) {
            case CommentStatus::Open: statusStr = tr("Open"); break;
            case CommentStatus::Resolved: statusStr = tr("✓ Resolved"); break;
            case CommentStatus::Closed: statusStr = tr("Closed"); break;
        }
        
        QString text = comment.content;
        if (text.length() > 50) {
            text = text.left(50) + "...";
        }
        
        auto* item = new QTreeWidgetItem(commentsTree_);
        item->setText(0, typeIcon + " " + text);
        item->setText(1, comment.author);
        item->setText(2, comment.createdAt.toString("MM-dd"));
        item->setText(3, statusStr);
        item->setData(0, Qt::UserRole, comment.id);
        
        if (comment.status == CommentStatus::Resolved) {
            item->setForeground(0, QBrush(Qt::gray));
        }
        
        // Add replies as children
        for (const auto& reply : thread.replies) {
            auto* replyItem = new QTreeWidgetItem(item);
            replyItem->setText(0, tr("↳ %1").arg(reply.content.left(40)));
            replyItem->setText(1, reply.author);
            replyItem->setText(2, reply.createdAt.toString("MM-dd"));
            replyItem->setData(0, Qt::UserRole, reply.id);
        }
        
        item->setExpanded(true);
    }
    
    commentsTree_->resizeColumnToContents(0);
    commentsTree_->resizeColumnToContents(1);
    commentsTree_->resizeColumnToContents(2);
}

void QueryCommentsDialog::updateCommentDetails(const QueryComment& comment) {
    currentComment_ = comment;
    
    commentAuthorLabel_->setText(tr("<b>%1</b> commented").arg(comment.author));
    commentDateLabel_->setText(comment.createdAt.toString("yyyy-MM-dd hh:mm"));
    
    if (comment.startLine >= 0) {
        if (comment.startLine == comment.endLine) {
            commentLocationLabel_->setText(tr("Line %1").arg(comment.startLine));
        } else {
            commentLocationLabel_->setText(tr("Lines %1-%2").arg(comment.startLine).arg(comment.endLine));
        }
    } else {
        commentLocationLabel_->setText(tr("General comment"));
    }
    
    QString content = comment.content;
    content.replace("\n", "<br>");
    commentContentBrowser_->setHtml(content);
    
    // Update buttons
    replyBtn_->setEnabled(true);
    editBtn_->setEnabled(true);
    deleteBtn_->setEnabled(true);
    resolveBtn_->setEnabled(comment.status == CommentStatus::Open);
    resolveBtn_->setText(comment.status == CommentStatus::Resolved ? tr("Reopen") : tr("Resolve"));
    
    // Highlight in editor
    highlightCommentInEditor(comment);
    
    // Load replies
    repliesList_->clear();
    for (const auto& thread : threads_) {
        if (thread.rootComment.id == comment.id || thread.rootComment.id == comment.parentId) {
            for (const auto& reply : thread.replies) {
                repliesList_->addItem(tr("%1: %2").arg(reply.author).arg(reply.content.left(50)));
            }
            break;
        }
    }
}

void QueryCommentsDialog::highlightCommentInEditor(const QueryComment& comment) {
    // TODO: Highlight the commented lines in the editor
    Q_UNUSED(comment)
}

void QueryCommentsDialog::onNewComment() {
    // Get selected text or current line
    QTextCursor cursor = queryEditor_->textCursor();
    QString selectedText = cursor.selectedText();
    int startLine = queryEditor_->document()->findBlock(cursor.selectionStart()).blockNumber() + 1;
    int endLine = queryEditor_->document()->findBlock(cursor.selectionEnd()).blockNumber() + 1;
    
    AddCommentDialog dialog(queryId_, queryContent_, startLine, endLine, selectedText, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        QueryComment newComment = dialog.comment();
        newComment.id = "c" + QString::number(threads_.size() + 1);
        newComment.createdAt = QDateTime::currentDateTime();
        
        CommentThread newThread;
        newThread.id = "thread" + QString::number(threads_.size() + 1);
        newThread.rootComment = newComment;
        threads_.append(newThread);
        
        updateCommentsList();
        emit commentAdded(newComment);
    }
}

void QueryCommentsDialog::onReplyToComment() {
    if (currentComment_.id.isEmpty()) return;
    
    bool ok;
    QString reply = QInputDialog::getMultiLineText(this, tr("Reply"),
        tr("Enter your reply:"), QString(), &ok);
    
    if (ok && !reply.isEmpty()) {
        QueryComment replyComment;
        replyComment.id = currentComment_.id + "r" + QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
        replyComment.parentId = currentComment_.id;
        replyComment.author = tr("Me");
        replyComment.createdAt = QDateTime::currentDateTime();
        replyComment.content = reply;
        
        for (auto& thread : threads_) {
            if (thread.rootComment.id == currentComment_.id ||
                thread.rootComment.id == currentComment_.parentId) {
                thread.replies.append(replyComment);
                thread.rootComment.replyCount++;
                break;
            }
        }
        
        updateCommentsList();
    }
}

void QueryCommentsDialog::onEditComment() {
    if (currentComment_.id.isEmpty()) return;
    
    EditCommentDialog dialog(currentComment_, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        QueryComment updated = dialog.comment();
        
        for (auto& thread : threads_) {
            if (thread.rootComment.id == currentComment_.id) {
                thread.rootComment.content = updated.content;
                thread.rootComment.status = updated.status;
                thread.rootComment.updatedAt = QDateTime::currentDateTime();
                updateCommentDetails(thread.rootComment);
                break;
            }
        }
        
        updateCommentsList();
        emit commentUpdated(updated);
    }
}

void QueryCommentsDialog::onDeleteComment() {
    if (currentComment_.id.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, tr("Delete Comment"),
        tr("Are you sure you want to delete this comment?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        for (int i = 0; i < threads_.size(); ++i) {
            if (threads_[i].rootComment.id == currentComment_.id) {
                threads_.removeAt(i);
                break;
            }
        }
        
        currentComment_ = QueryComment();
        updateCommentsList();
        emit commentDeleted(currentComment_.id);
    }
}

void QueryCommentsDialog::onResolveComment() {
    if (currentComment_.id.isEmpty()) return;
    
    for (auto& thread : threads_) {
        if (thread.rootComment.id == currentComment_.id) {
            if (thread.rootComment.status == CommentStatus::Open) {
                thread.rootComment.status = CommentStatus::Resolved;
                thread.isResolved = true;
                thread.resolvedBy = tr("Me");
                thread.resolvedAt = QDateTime::currentDateTime();
            } else {
                thread.rootComment.status = CommentStatus::Open;
                thread.isResolved = false;
            }
            updateCommentDetails(thread.rootComment);
            break;
        }
    }
    
    updateCommentsList();
}

void QueryCommentsDialog::onReopenComment() {
    onResolveComment(); // Toggle
}

void QueryCommentsDialog::onStartReview() {
    inReviewMode_ = true;
    newCommentBtn_->setText(tr("Add Review Comment"));
}

void QueryCommentsDialog::onSubmitReview() {
    inReviewMode_ = false;
    newCommentBtn_->setText(tr("New Comment"));
    QMessageBox::information(this, tr("Review Submitted"), tr("Your review has been submitted."));
}

void QueryCommentsDialog::onCancelReview() {
    inReviewMode_ = false;
    newCommentBtn_->setText(tr("New Comment"));
}

void QueryCommentsDialog::onRefreshComments() {
    loadComments();
}

void QueryCommentsDialog::onExportComments() {
    // TODO: Export to file
}

void QueryCommentsDialog::onFilterByType(int type) {
    Q_UNUSED(type)
    updateCommentsList();
}

void QueryCommentsDialog::onFilterByStatus(int status) {
    Q_UNUSED(status)
    updateCommentsList();
}

void QueryCommentsDialog::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    // TODO: Filter comments by search text
}

void QueryCommentsDialog::onCommentSelected(const QModelIndex& index) {
    Q_UNUSED(index)
    // Handled in itemClicked signal
}

// ============================================================================
// Add Comment Dialog
// ============================================================================
AddCommentDialog::AddCommentDialog(const QString& queryId, const QString& queryContent,
                                   int startLine, int endLine,
                                   const QString& selectedText,
                                   backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), queryId_(queryId), queryContent_(queryContent),
      startLine_(startLine), endLine_(endLine), selectedText_(selectedText),
      client_(client) {
    setupUi();
}

void AddCommentDialog::setupUi() {
    setWindowTitle(tr("Add Comment"));
    setMinimumSize(400, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    // Location info
    locationLabel_ = new QLabel(this);
    if (startLine_ == endLine_) {
        locationLabel_->setText(tr("Line %1").arg(startLine_));
    } else {
        locationLabel_->setText(tr("Lines %1-%2").arg(startLine_).arg(endLine_));
    }
    layout->addWidget(locationLabel_);
    
    // Selected text preview
    if (!selectedText_.isEmpty()) {
        auto* previewLabel = new QLabel(tr("Selected text:"), this);
        layout->addWidget(previewLabel);
        
        auto* previewEdit = new QTextEdit(this);
        previewEdit->setPlainText(selectedText_);
        previewEdit->setReadOnly(true);
        previewEdit->setMaximumHeight(60);
        layout->addWidget(previewEdit);
    }
    
    // Type
    auto* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel(tr("Type:"), this));
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem(tr("💬 General Comment"), static_cast<int>(CommentType::GeneralComment));
    typeCombo_->addItem(tr("👁 Review Comment"), static_cast<int>(CommentType::ReviewComment));
    typeCombo_->addItem(tr("❓ Question"), static_cast<int>(CommentType::Question));
    typeCombo_->addItem(tr("💡 Suggestion"), static_cast<int>(CommentType::Suggestion));
    typeLayout->addWidget(typeCombo_);
    typeLayout->addStretch();
    layout->addLayout(typeLayout);
    
    // Content
    layout->addWidget(new QLabel(tr("Comment:"), this));
    contentEdit_ = new QTextEdit(this);
    contentEdit_->setPlaceholderText(tr("Enter your comment here..."));
    layout->addWidget(contentEdit_, 1);
    
    // Tags
    auto* tagsLayout = new QHBoxLayout();
    tagEdit_ = new QLineEdit(this);
    tagEdit_->setPlaceholderText(tr("Add tag..."));
    tagsLayout->addWidget(tagEdit_);
    auto* addTagBtn = new QPushButton(tr("Add"), this);
    connect(addTagBtn, &QPushButton::clicked, this, &AddCommentDialog::onAddTag);
    tagsLayout->addWidget(addTagBtn);
    layout->addLayout(tagsLayout);
    
    tagsList_ = new QListWidget(this);
    tagsList_->setMaximumHeight(50);
    tagsList_->setFlow(QListView::LeftToRight);
    layout->addWidget(tagsList_);
    
    // Buttons
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &AddCommentDialog::onValidate);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox_);
}

void AddCommentDialog::onTypeChanged(int index) {
    Q_UNUSED(index)
}

void AddCommentDialog::onAddMention() {
    // TODO: Add mention
}

void AddCommentDialog::onAddTag() {
    QString tag = tagEdit_->text().trimmed();
    if (!tag.isEmpty()) {
        auto* item = new QListWidgetItem(tag);
        item->setBackground(QBrush(QColor(200, 220, 255)));
        tagsList_->addItem(item);
        tagEdit_->clear();
    }
}

void AddCommentDialog::onValidate() {
    if (contentEdit_->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter a comment."));
        return;
    }
    
    accept();
}

QueryComment AddCommentDialog::comment() const {
    QueryComment comment;
    comment.queryId = queryId_;
    comment.type = static_cast<CommentType>(typeCombo_->currentData().toInt());
    comment.content = contentEdit_->toPlainText().trimmed();
    comment.status = CommentStatus::Open;
    comment.author = tr("Me");
    comment.startLine = startLine_;
    comment.endLine = endLine_;
    comment.selectedText = selectedText_;
    
    for (int i = 0; i < tagsList_->count(); ++i) {
        comment.tags.append(tagsList_->item(i)->text());
    }
    
    return comment;
}

// ============================================================================
// Edit Comment Dialog
// ============================================================================
EditCommentDialog::EditCommentDialog(const QueryComment& comment,
                                     backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), originalComment_(comment), client_(client) {
    setupUi();
}

void EditCommentDialog::setupUi() {
    setWindowTitle(tr("Edit Comment"));
    setMinimumSize(400, 250);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    statusCombo_ = new QComboBox(this);
    statusCombo_->addItem(tr("Open"), static_cast<int>(CommentStatus::Open));
    statusCombo_->addItem(tr("Resolved"), static_cast<int>(CommentStatus::Resolved));
    statusCombo_->addItem(tr("Closed"), static_cast<int>(CommentStatus::Closed));
    statusCombo_->setCurrentIndex(static_cast<int>(originalComment_.status));
    formLayout->addRow(tr("Status:"), statusCombo_);
    
    layout->addLayout(formLayout);
    
    layout->addWidget(new QLabel(tr("Content:"), this));
    contentEdit_ = new QTextEdit(this);
    contentEdit_->setText(originalComment_.content);
    layout->addWidget(contentEdit_, 1);
    
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &EditCommentDialog::onValidate);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox_);
}

void EditCommentDialog::onValidate() {
    if (contentEdit_->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Comment cannot be empty."));
        return;
    }
    
    accept();
}

QueryComment EditCommentDialog::comment() const {
    QueryComment comment = originalComment_;
    comment.content = contentEdit_->toPlainText().trimmed();
    comment.status = static_cast<CommentStatus>(statusCombo_->currentData().toInt());
    comment.updatedAt = QDateTime::currentDateTime();
    return comment;
}

// ============================================================================
// Comment Thread Widget
// ============================================================================
CommentThreadWidget::CommentThreadWidget(const CommentThread& thread, QWidget* parent)
    : QWidget(parent), thread_(thread) {
    setupUi();
}

void CommentThreadWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Root comment
    addCommentWidget(thread_.rootComment, 0);
    
    // Replies
    for (const auto& reply : thread_.replies) {
        addCommentWidget(reply, 1);
    }
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* replyBtn = new QPushButton(tr("Reply"), this);
    connect(replyBtn, &QPushButton::clicked, this, &CommentThreadWidget::onReplyClicked);
    btnLayout->addWidget(replyBtn);
    
    if (!thread_.isResolved) {
        auto* resolveBtn = new QPushButton(tr("Resolve"), this);
        connect(resolveBtn, &QPushButton::clicked, this, &CommentThreadWidget::onResolveClicked);
        btnLayout->addWidget(resolveBtn);
    }
    
    mainLayout->addLayout(btnLayout);
    
    mainLayout->addStretch();
}

void CommentThreadWidget::addCommentWidget(const QueryComment& comment, int indent) {
    Q_UNUSED(indent)
    
    auto* commentWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(commentWidget);
    layout->setContentsMargins(8, 8, 8, 8);
    
    auto* headerLayout = new QHBoxLayout();
    auto* authorLabel = new QLabel(tr("<b>%1</b>").arg(comment.author), this);
    headerLayout->addWidget(authorLabel);
    headerLayout->addStretch();
    auto* dateLabel = new QLabel(comment.createdAt.toString("yyyy-MM-dd hh:mm"), this);
    dateLabel->setStyleSheet("color: gray;");
    headerLayout->addWidget(dateLabel);
    layout->addLayout(headerLayout);
    
    auto* contentLabel = new QLabel(comment.content, this);
    contentLabel->setWordWrap(true);
    layout->addWidget(contentLabel);
    
    commentWidget->setStyleSheet("background-color: #f5f5f5; border-radius: 4px;");
    
    commentsLayout_->addWidget(commentWidget);
}

void CommentThreadWidget::onReplyClicked() {
    emit replyRequested(thread_.rootComment.id);
}

void CommentThreadWidget::onResolveClicked() {
    emit resolveRequested(thread_.id);
}

void CommentThreadWidget::onEditClicked() {
    emit editRequested(thread_.rootComment.id);
}

void CommentThreadWidget::onDeleteClicked() {
    emit deleteRequested(thread_.rootComment.id);
}

// ============================================================================
// Inline Comment Widget
// ============================================================================
InlineCommentWidget::InlineCommentWidget(const QueryComment& comment, QWidget* parent)
    : QWidget(parent), comment_(comment) {
    setupUi();
    updateAppearance();
}

void InlineCommentWidget::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    
    indicatorLabel_ = new QLabel(this);
    indicatorLabel_->setFixedSize(16, 16);
    layout->addWidget(indicatorLabel_);
    
    setCursor(Qt::PointingHandCursor);
}

void InlineCommentWidget::updateAppearance() {
    switch (comment_.type) {
        case CommentType::ReviewComment:
            indicatorColor_ = QColor(255, 200, 0);
            break;
        case CommentType::Question:
            indicatorColor_ = QColor(0, 150, 255);
            break;
        case CommentType::Suggestion:
            indicatorColor_ = QColor(0, 200, 0);
            break;
        default:
            indicatorColor_ = QColor(150, 150, 150);
    }
    
    if (comment_.status == CommentStatus::Resolved) {
        indicatorColor_ = QColor(200, 200, 200);
    }
    
    indicatorLabel_->setStyleSheet(QString("background-color: %1; border-radius: 8px;")
        .arg(indicatorColor_.name()));
}

// ============================================================================
// Comment Overview Panel
// ============================================================================
CommentOverviewPanel::CommentOverviewPanel(backend::SessionClient* client, QWidget* parent)
    : QWidget(parent), client_(client) {
    setupUi();
}

void CommentOverviewPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // Summary
    summaryLabel_ = new QLabel(tr("No query selected"), this);
    layout->addWidget(summaryLabel_);
    
    // Filter
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter comments..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &CommentOverviewPanel::onFilterChanged);
    layout->addWidget(filterEdit_);
    
    // Comments tree
    commentsTree_ = new QTreeWidget(this);
    commentsTree_->setHeaderLabels({tr("Comment"), tr("Author"), tr("Status")});
    commentsTree_->setRootIsDecorated(false);
    layout->addWidget(commentsTree_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &CommentOverviewPanel::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    layout->addLayout(btnLayout);
}

void CommentOverviewPanel::onQueryChanged(const QString& queryId, const QString& queryName) {
    currentQueryId_ = queryId;
    loadComments();
    
    int openCount = 0;
    int resolvedCount = 0;
    for (const auto& thread : threads_) {
        if (thread.isResolved) {
            resolvedCount++;
        } else {
            openCount++;
        }
    }
    
    summaryLabel_->setText(tr("<b>%1</b><br>%2 open, %3 resolved")
        .arg(queryName)
        .arg(openCount)
        .arg(resolvedCount));
}

void CommentOverviewPanel::loadComments() {
    threads_.clear();
    // TODO: Load from backend
}

void CommentOverviewPanel::updateOverview() {
    commentsTree_->clear();
    
    for (const auto& thread : threads_) {
        const auto& comment = thread.rootComment;
        
        QString text = comment.content;
        if (text.length() > 40) {
            text = text.left(40) + "...";
        }
        
        auto* item = new QTreeWidgetItem(commentsTree_);
        item->setText(0, text);
        item->setText(1, comment.author);
        item->setText(2, thread.isResolved ? tr("Resolved") : tr("Open"));
        item->setData(0, Qt::UserRole, comment.id);
        item->setData(0, Qt::UserRole + 1, comment.startLine);
    }
    
    commentsTree_->resizeColumnToContents(0);
}

void CommentOverviewPanel::onRefresh() {
    loadComments();
    updateOverview();
}

void CommentOverviewPanel::onFilterChanged(const QString& filter) {
    Q_UNUSED(filter)
    // TODO: Filter
}

} // namespace scratchrobin::ui
