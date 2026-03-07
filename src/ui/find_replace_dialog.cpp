#include "ui/find_replace_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <QRegularExpression>

namespace scratchrobin::ui {

FindReplaceDialog::FindReplaceDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Find and Replace"));
    setMinimumWidth(450);
    resize(500, 250);
    
    setupUi();
}

FindReplaceDialog::~FindReplaceDialog() = default;

void FindReplaceDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Find row
    auto* findLayout = new QHBoxLayout();
    findLayout->addWidget(new QLabel(tr("Find:"), this));
    findEdit_ = new QLineEdit(this);
    findEdit_->setPlaceholderText(tr("Text to find"));
    findLayout->addWidget(findEdit_);
    mainLayout->addLayout(findLayout);
    
    // Replace row
    auto* replaceLayout = new QHBoxLayout();
    replaceLayout->addWidget(new QLabel(tr("Replace:"), this));
    replaceEdit_ = new QLineEdit(this);
    replaceEdit_->setPlaceholderText(tr("Replacement text"));
    replaceLayout->addWidget(replaceEdit_);
    mainLayout->addLayout(replaceLayout);
    
    // Options
    auto* optionsGroup = new QHBoxLayout();
    caseSensitiveCheck_ = new QCheckBox(tr("Case sensitive"), this);
    wholeWordCheck_ = new QCheckBox(tr("Whole words only"), this);
    regexCheck_ = new QCheckBox(tr("Regular expression"), this);
    optionsGroup->addWidget(caseSensitiveCheck_);
    optionsGroup->addWidget(wholeWordCheck_);
    optionsGroup->addWidget(regexCheck_);
    optionsGroup->addStretch();
    mainLayout->addLayout(optionsGroup);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    findNextBtn_ = new QPushButton(tr("Find Next"), this);
    findNextBtn_->setDefault(true);
    buttonLayout->addWidget(findNextBtn_);
    
    findPrevBtn_ = new QPushButton(tr("Find Previous"), this);
    buttonLayout->addWidget(findPrevBtn_);
    
    replaceBtn_ = new QPushButton(tr("Replace"), this);
    buttonLayout->addWidget(replaceBtn_);
    
    replaceAllBtn_ = new QPushButton(tr("Replace All"), this);
    buttonLayout->addWidget(replaceAllBtn_);
    
    closeBtn_ = new QPushButton(tr("Close"), this);
    buttonLayout->addWidget(closeBtn_);
    
    mainLayout->addLayout(buttonLayout);
    
    // Status label
    statusLabel_ = new QLabel(this);
    statusLabel_->setStyleSheet("QLabel { color: gray; }");
    mainLayout->addWidget(statusLabel_);
    
    // Connections
    connect(findEdit_, &QLineEdit::textChanged, this, &FindReplaceDialog::onFindTextChanged);
    connect(findNextBtn_, &QPushButton::clicked, this, &FindReplaceDialog::onFindNext);
    connect(findPrevBtn_, &QPushButton::clicked, this, &FindReplaceDialog::onFindPrevious);
    connect(replaceBtn_, &QPushButton::clicked, this, &FindReplaceDialog::onReplace);
    connect(replaceAllBtn_, &QPushButton::clicked, this, &FindReplaceDialog::onReplaceAll);
    connect(closeBtn_, &QPushButton::clicked, this, &FindReplaceDialog::onClose);
    connect(caseSensitiveCheck_, &QCheckBox::stateChanged, this, &FindReplaceDialog::onOptionsChanged);
    connect(wholeWordCheck_, &QCheckBox::stateChanged, this, &FindReplaceDialog::onOptionsChanged);
    connect(regexCheck_, &QCheckBox::stateChanged, this, &FindReplaceDialog::onOptionsChanged);
    
    updateButtonStates();
}

void FindReplaceDialog::setTextEdit(QPlainTextEdit* textEdit) {
    textEdit_ = textEdit;
}

void FindReplaceDialog::setFindText(const QString& text) {
    findEdit_->setText(text);
    findEdit_->selectAll();
}

void FindReplaceDialog::updateButtonStates() {
    bool hasText = !findEdit_->text().isEmpty();
    findNextBtn_->setEnabled(hasText);
    findPrevBtn_->setEnabled(hasText);
    replaceBtn_->setEnabled(hasText);
    replaceAllBtn_->setEnabled(hasText);
}

void FindReplaceDialog::onFindTextChanged() {
    updateButtonStates();
    statusLabel_->clear();
}

void FindReplaceDialog::onOptionsChanged() {
    statusLabel_->clear();
}

bool FindReplaceDialog::findText(bool forward) {
    if (!textEdit_ || findEdit_->text().isEmpty()) {
        return false;
    }
    
    QString findText = findEdit_->text();
    QTextDocument::FindFlags flags;
    
    if (!forward) {
        flags |= QTextDocument::FindBackward;
    }
    
    if (caseSensitiveCheck_->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    
    if (wholeWordCheck_->isChecked()) {
        flags |= QTextDocument::FindWholeWords;
    }
    
    bool found;
    if (regexCheck_->isChecked()) {
        // Use regex search
        QRegularExpression regex(findText);
        if (!caseSensitiveCheck_->isChecked()) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
        QTextCursor cursor = textEdit_->textCursor();
        found = textEdit_->find(regex);
    } else {
        found = textEdit_->find(findText, flags);
    }
    
    if (found) {
        statusLabel_->clear();
    } else {
        statusLabel_->setText(tr("Text not found"));
        statusLabel_->setStyleSheet("QLabel { color: red; }");
    }
    
    return found;
}

void FindReplaceDialog::onFindNext() {
    emit findNext(findEdit_->text(), caseSensitiveCheck_->isChecked(), 
                  wholeWordCheck_->isChecked(), regexCheck_->isChecked());
    findText(true);
}

void FindReplaceDialog::onFindPrevious() {
    emit findPrevious(findEdit_->text(), caseSensitiveCheck_->isChecked(), 
                      wholeWordCheck_->isChecked(), regexCheck_->isChecked());
    findText(false);
}

void FindReplaceDialog::onReplace() {
    if (!textEdit_) return;
    
    QTextCursor cursor = textEdit_->textCursor();
    if (cursor.hasSelection()) {
        cursor.insertText(replaceEdit_->text());
        emit replace(findEdit_->text(), replaceEdit_->text());
    }
    
    // Find next occurrence
    findText(true);
}

void FindReplaceDialog::onReplaceAll() {
    if (!textEdit_) return;
    
    int count = 0;
    QTextCursor cursor = textEdit_->textCursor();
    cursor.movePosition(QTextCursor::Start);
    textEdit_->setTextCursor(cursor);
    
    while (findText(true)) {
        QTextCursor replaceCursor = textEdit_->textCursor();
        if (replaceCursor.hasSelection()) {
            replaceCursor.insertText(replaceEdit_->text());
            count++;
        }
    }
    
    statusLabel_->setText(tr("Replaced %1 occurrences").arg(count));
    statusLabel_->setStyleSheet("QLabel { color: green; }");
    
    emit replaceAll(findEdit_->text(), replaceEdit_->text());
}

void FindReplaceDialog::onClose() {
    close();
}

} // namespace scratchrobin::ui
