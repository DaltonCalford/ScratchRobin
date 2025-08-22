#include "editor/text_editor.h"
#include "metadata/metadata_manager.h"
#include "search/search_engine.h"
#include "core/connection_manager.h"
#include <QApplication>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QCompleter>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QFontMetrics>
#include <QTextBlockUserData>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QInputDialog>
#include <QClipboard>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace scratchrobin {

// Line number area widget
class LineNumberArea : public QWidget {
public:
    LineNumberArea(QPlainTextEdit* editor) : QWidget(editor), textEdit_(editor) {}

    QSize sizeHint() const override {
        return QSize(calculateWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.fillRect(event->rect(), Qt::lightGray);

        QTextBlock block = textEdit_->firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = (int)textEdit_->blockBoundingGeometry(block).translated(textEdit_->contentOffset()).top();
        int bottom = top + (int)textEdit_->blockBoundingRect(block).height();

        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(Qt::black);
                painter.drawText(0, top, width(), fontMetrics().height(), Qt::AlignRight, number);
            }

            block = block.next();
            top = bottom;
            bottom = top + (int)textEdit_->blockBoundingRect(block).height();
            ++blockNumber;
        }
    }

private:
    QPlainTextEdit* textEdit_;

    int calculateWidth() const {
        int digits = 1;
        int max = qMax(1, textEdit_->blockCount());
        while (max >= 10) {
            max /= 10;
            ++digits;
        }
        return 8 + fontMetrics().width(QLatin1Char('9')) * digits;
    }
};

// Text editor line highlight data
class LineHighlightData : public QTextBlockUserData {
public:
    LineHighlightData() = default;
    ~LineHighlightData() override = default;
};

class TextEditor::Impl {
public:
    Impl(TextEditor* parent)
        : parent_(parent), lineNumberArea_(nullptr), completer_(nullptr) {}

    void setupUI() {
        textEdit_ = new QPlainTextEdit();
        textEdit_->setParent(parent_);

        // Set default font
        QFont font("Monaco", 12);
        textEdit_->setFont(font);

        // Configure text edit
        textEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);
        textEdit_->setTabStopDistance(QFontMetrics(font).width(' ') * 4);
        textEdit_->setContextMenuPolicy(Qt::CustomContextMenu);

        // Line numbers
        lineNumberArea_ = new LineNumberArea(textEdit_);
        updateLineNumberAreaWidth(0);
        updateLineNumberArea(textEdit_->rect());

        // Connections
        connect(textEdit_->document(), &QTextDocument::contentsChanged,
                [this]() { updateLineNumberAreaWidth(0); });
        connect(textEdit_, &QPlainTextEdit::blockCountChanged,
                [this](int newBlockCount) { updateLineNumberAreaWidth(0); });
        connect(textEdit_, &QPlainTextEdit::updateRequest,
                [this](const QRect& rect, int dy) { updateLineNumberArea(rect, dy); });
        connect(textEdit_, &QPlainTextEdit::cursorPositionChanged,
                [this]() { highlightCurrentLine(); });
        connect(textEdit_, &QPlainTextEdit::customContextMenuRequested,
                parent_, &TextEditor::onContextMenuRequested);

        // Set up completer
        setupCompleter();
    }

    void setupCompleter() {
        completer_ = new QCompleter();
        completer_->setWidget(textEdit_);
        completer_->setCompletionMode(QCompleter::PopupCompletion);
        completer_->setCaseSensitivity(Qt::CaseInsensitive);
        completer_->setWrapAround(false);

        connect(completer_, QOverload<const QModelIndex&>::of(&QCompleter::activated),
                parent_, &TextEditor::onCompletionActivated);
    }

    void updateLineNumberAreaWidth(int /* newBlockCount */) {
        textEdit_->setViewportMargins(calculateLineNumberAreaWidth(), 0, 0, 0);
    }

    void updateLineNumberArea(const QRect& rect, int dy = 0) {
        if (dy) {
            lineNumberArea_->scroll(0, dy);
        } else {
            lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
        }

        if (rect.contains(textEdit_->viewport()->rect())) {
            updateLineNumberAreaWidth(0);
        }
    }

    int calculateLineNumberAreaWidth() const {
        int digits = 1;
        int max = qMax(1, textEdit_->blockCount());
        while (max >= 10) {
            max /= 10;
            ++digits;
        }
        return 8 + textEdit_->fontMetrics().width(QLatin1Char('9')) * digits;
    }

    void highlightCurrentLine() {
        QList<QTextEdit::ExtraSelection> extraSelections;

        if (!textEdit_->isReadOnly()) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(QColor(245, 245, 245));
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textEdit_->textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }

        textEdit_->setExtraSelections(extraSelections);
    }

    EditorPosition getCursorPosition() const {
        QTextCursor cursor = textEdit_->textCursor();
        return {
            cursor.blockNumber() + 1,  // Line (1-based)
            cursor.positionInBlock() + 1,  // Column (1-based)
            cursor.position()  // Absolute position
        };
    }

    void setCursorPosition(const EditorPosition& position) {
        QTextCursor cursor = textEdit_->textCursor();
        QTextBlock block = textEdit_->document()->findBlockByNumber(position.line - 1);
        if (block.isValid()) {
            cursor.setPosition(block.position() + qMin(position.column - 1, block.length() - 1));
            textEdit_->setTextCursor(cursor);
            textEdit_->ensureCursorVisible();
        }
    }

    TextSelection getSelection() const {
        QTextCursor cursor = textEdit_->textCursor();
        if (!cursor.hasSelection()) {
            return {{0, 0, 0}, {0, 0, 0}, "", false};
        }

        auto start = convertCursorToPosition(cursor.selectionStart());
        auto end = convertCursorToPosition(cursor.selectionEnd());

        return {start, end, cursor.selectedText(), true};
    }

    void setSelection(const TextSelection& selection) {
        if (!selection.hasSelection) {
            textEdit_->textCursor().clearSelection();
            return;
        }

        QTextCursor cursor = textEdit_->textCursor();
        cursor.setPosition(selection.start.absolutePosition);
        cursor.setPosition(selection.end.absolutePosition, QTextCursor::KeepAnchor);
        textEdit_->setTextCursor(cursor);
        textEdit_->ensureCursorVisible();
    }

    EditorPosition convertCursorToPosition(int position) const {
        QTextCursor cursor(textEdit_->document());
        cursor.setPosition(position);

        return {
            cursor.blockNumber() + 1,
            cursor.positionInBlock() + 1,
            position
        };
    }

    int convertPositionToCursor(const EditorPosition& position) const {
        QTextBlock block = textEdit_->document()->findBlockByNumber(position.line - 1);
        if (block.isValid()) {
            return block.position() + qMin(position.column - 1, block.length() - 1);
        }
        return 0;
    }

    void performAutoIndentation() {
        QTextCursor cursor = textEdit_->textCursor();
        QTextBlock currentBlock = cursor.block();
        QTextBlock previousBlock = currentBlock.previous();

        if (previousBlock.isValid()) {
            QString previousLine = previousBlock.text();
            QString indentation;

            // Find leading whitespace
            for (QChar ch : previousLine) {
                if (ch.isSpace()) {
                    indentation += ch;
                } else {
                    break;
                }
            }

            // Add extra indentation for certain constructs
            if (previousLine.trimmed().endsWith("BEGIN") ||
                previousLine.trimmed().endsWith("(") ||
                previousLine.trimmed().endsWith("\\")) {
                if (indentation.isEmpty()) {
                    indentation = QString(4, ' '); // Default 4 spaces
                } else {
                    indentation += QString(4, ' '); // Add one level
                }
            }

            cursor.insertText(indentation);
        }
    }

    void handleBracketMatching() {
        QTextCursor cursor = textEdit_->textCursor();
        QTextDocument* doc = textEdit_->document();

        // Find matching bracket
        QTextCursor matchCursor = doc->find("(", cursor.selectionStart() - 1);
        if (!matchCursor.isNull()) {
            // Found opening bracket, check if we have a closing one
            int depth = 1;
            int pos = matchCursor.position() + 1;

            while (pos < doc->characterCount()) {
                QChar ch = doc->characterAt(pos);
                if (ch == '(') {
                    depth++;
                } else if (ch == ')') {
                    depth--;
                    if (depth == 0) {
                        // Found matching closing bracket
                        return;
                    }
                }
                pos++;
            }

            // No matching closing bracket, insert it
            cursor.setPosition(cursor.selectionEnd());
            cursor.insertText(")");
            cursor.setPosition(cursor.position() - 1);
            textEdit_->setTextCursor(cursor);
        }
    }

    bool loadFile(const std::string& filePath) {
        QFile file(QString::fromStdString(filePath));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&file);
        QString text = in.readAll();
        file.close();

        textEdit_->setPlainText(text);

        documentInfo_.filePath = filePath;
        documentInfo_.isModified = false;
        documentInfo_.isNew = false;
        documentInfo_.lastSavedAt = QFileInfo(file).lastModified().toUTC();

        return true;
    }

    bool saveFile(const std::string& filePath) {
        QFile file(QString::fromStdString(filePath));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream out(&file);
        out << textEdit_->toPlainText();
        file.close();

        documentInfo_.filePath = filePath;
        documentInfo_.isModified = false;
        documentInfo_.isNew = false;
        documentInfo_.lastSavedAt = QDateTime::currentDateTimeUtc();

        return true;
    }

    void applyConfiguration() {
        // Apply font
        QFont font(config_.fontFamily, config_.fontSize);
        textEdit_->setFont(font);

        // Apply colors
        QPalette palette = textEdit_->palette();
        palette.setColor(QPalette::Base, config_.backgroundColor);
        palette.setColor(QPalette::Text, config_.foregroundColor);
        palette.setColor(QPalette::Highlight, config_.selectionColor);
        textEdit_->setPalette(palette);

        // Apply other settings
        textEdit_->setLineWrapMode(config_.enableWordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
        textEdit_->setTabStopDistance(QFontMetrics(font).width(' ') * config_.defaultTabWidth);

        highlightCurrentLine();
    }

private:
    TextEditor* parent_;
    QPlainTextEdit* textEdit_;
    LineNumberArea* lineNumberArea_;
    QCompleter* completer_;
    EditorConfiguration config_;
    DocumentInfo documentInfo_;
};

// TextEditor implementation

TextEditor::TextEditor(QWidget* parent)
    : QWidget(parent), impl_(std::make_unique<Impl>(this)) {

    impl_->setupUI();
    setupConnections();

    // Set default configuration
    config_.fontFamily = "Monaco";
    config_.fontSize = 12;
    config_.backgroundColor = QColor(255, 255, 255);
    config_.foregroundColor = QColor(0, 0, 0);
    config_.selectionColor = QColor(173, 214, 255);
    config_.currentLineColor = QColor(245, 245, 245);
    config_.enableSyntaxHighlighting = true;
    config_.enableCodeCompletion = true;
    config_.enableAutoIndentation = true;
    config_.enableLineNumbers = true;
    config_.enableWordWrap = false;
    config_.enableAutoSave = false;
    config_.autoSaveIntervalSeconds = 300;
    config_.enableBracketMatching = true;
    config_.enableCurrentLineHighlighting = true;
    config_.enableWhitespaceVisualization = false;
    config_.maxUndoSteps = 100;
    config_.defaultTabWidth = 4;
    config_.defaultIndentWidth = 4;

    impl_->applyConfiguration();

    // Initialize document info
    documentInfo_.isNew = true;
    documentInfo_.isModified = false;
    documentInfo_.mode = EditorMode::SQL;
    documentInfo_.encoding = TextEncoding::UTF8;
    documentInfo_.lineEnding = LineEnding::UNIX;
    documentInfo_.indentationMode = IndentationMode::SPACES;
    documentInfo_.tabWidth = 4;
    documentInfo_.indentWidth = 4;
    documentInfo_.createdAt = QDateTime::currentDateTimeUtc();
    documentInfo_.modifiedAt = QDateTime::currentDateTimeUtc();

    // Set up auto-save timer
    autoSaveTimer_ = new QTimer(this);
    connect(autoSaveTimer_, &QTimer::timeout, this, &TextEditor::onAutoSaveTimer);
}

TextEditor::~TextEditor() = default;

void TextEditor::setupConnections() {
    connect(impl_->textEdit_, &QPlainTextEdit::textChanged, this, &TextEditor::onTextChanged);
    connect(impl_->textEdit_, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::onCursorPositionChanged);
    connect(impl_->textEdit_, &QPlainTextEdit::selectionChanged, this, &TextEditor::onSelectionChanged);
    connect(impl_->textEdit_, &QPlainTextEdit::modificationChanged, this, &TextEditor::onDocumentModified);
}

void TextEditor::initialize(const EditorConfiguration& config) {
    config_ = config;
    impl_->applyConfiguration();
}

void TextEditor::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    metadataManager_ = metadataManager;
}

void TextEditor::setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) {
    searchEngine_ = searchEngine;
}

void TextEditor::setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) {
    connectionManager_ = connectionManager;
}

bool TextEditor::loadFile(const std::string& filePath) {
    if (impl_->loadFile(filePath)) {
        documentInfo_.title = QFileInfo(QString::fromStdString(filePath)).fileName().toStdString();
        setDocumentInfo(documentInfo_);
        return true;
    }
    return false;
}

bool TextEditor::saveFile(const std::string& filePath) {
    std::string path = filePath.empty() ? documentInfo_.filePath : filePath;
    if (path.empty()) {
        return false;
    }

    if (impl_->saveFile(path)) {
        documentInfo_.title = QFileInfo(QString::fromStdString(path)).fileName().toStdString();
        setDocumentInfo(documentInfo_);
        return true;
    }
    return false;
}

bool TextEditor::saveAs(const std::string& filePath) {
    return saveFile(filePath);
}

bool TextEditor::newDocument() {
    documentInfo_ = DocumentInfo();
    documentInfo_.isNew = true;
    documentInfo_.isModified = false;
    documentInfo_.mode = EditorMode::SQL;
    documentInfo_.encoding = TextEncoding::UTF8;
    documentInfo_.lineEnding = LineEnding::UNIX;
    documentInfo_.indentationMode = IndentationMode::SPACES;
    documentInfo_.tabWidth = 4;
    documentInfo_.indentWidth = 4;
    documentInfo_.createdAt = QDateTime::currentDateTimeUtc();
    documentInfo_.modifiedAt = QDateTime::currentDateTimeUtc();
    documentInfo_.title = "Untitled";

    impl_->textEdit_->clear();
    setDocumentInfo(documentInfo_);

    return true;
}

bool TextEditor::closeDocument() {
    if (documentInfo_.isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes",
            "The document has been modified. Do you want to save changes?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save) {
            if (!saveFile()) {
                return false;
            }
        } else if (reply == QMessageBox::Cancel) {
            return false;
        }
    }

    return true;
}

QString TextEditor::getText() const {
    return impl_->textEdit_->toPlainText();
}

void TextEditor::setText(const QString& text) {
    impl_->textEdit_->setPlainText(text);
    documentInfo_.isModified = false;
    documentInfo_.modifiedAt = QDateTime::currentDateTimeUtc();
}

QString TextEditor::getSelectedText() const {
    return impl_->textEdit_->textCursor().selectedText();
}

void TextEditor::setSelectedText(const QString& text) {
    impl_->textEdit_->textCursor().insertText(text);
}

EditorPosition TextEditor::getCursorPosition() const {
    return impl_->getCursorPosition();
}

void TextEditor::setCursorPosition(const EditorPosition& position) {
    impl_->setCursorPosition(position);
}

void TextEditor::setCursorPosition(int line, int column) {
    setCursorPosition({line, column, 0});
}

TextSelection TextEditor::getSelection() const {
    return impl_->getSelection();
}

void TextEditor::setSelection(const TextSelection& selection) {
    impl_->setSelection(selection);
}

void TextEditor::insertText(const QString& text) {
    impl_->textEdit_->textCursor().insertText(text);
}

void TextEditor::insertTextAt(const QString& text, const EditorPosition& position) {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    cursor.setPosition(impl_->convertPositionToCursor(position));
    cursor.insertText(text);
}

void TextEditor::replaceText(const QString& oldText, const QString& newText) {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    cursor.insertText(newText);
    cursor.endEditBlock();
}

void TextEditor::replaceSelection(const QString& text) {
    impl_->textEdit_->textCursor().insertText(text);
}

void TextEditor::deleteText(const EditorPosition& start, const EditorPosition& end) {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    cursor.setPosition(impl_->convertPositionToCursor(start));
    cursor.setPosition(impl_->convertPositionToCursor(end), QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
}

void TextEditor::undo() {
    if (impl_->textEdit_->document()->isUndoAvailable()) {
        impl_->textEdit_->undo();
    }
}

void TextEditor::redo() {
    if (impl_->textEdit_->document()->isRedoAvailable()) {
        impl_->textEdit_->redo();
    }
}

bool TextEditor::canUndo() const {
    return impl_->textEdit_->document()->isUndoAvailable();
}

bool TextEditor::canRedo() const {
    return impl_->textEdit_->document()->isRedoAvailable();
}

void TextEditor::clearUndoRedoHistory() {
    impl_->textEdit_->document()->clearUndoRedoStacks();
}

void TextEditor::cut() {
    impl_->textEdit_->cut();
}

void TextEditor::copy() {
    impl_->textEdit_->copy();
}

void TextEditor::paste() {
    impl_->textEdit_->paste();
}

void TextEditor::selectAll() {
    impl_->textEdit_->selectAll();
}

void TextEditor::findText(const QString& text, bool caseSensitive, bool wholeWords, bool regex) {
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWords) flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor = impl_->textEdit_->textCursor();
    if (regex) {
        // Handle regex search
        QRegularExpression regexPattern(text, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        cursor = impl_->textEdit_->document()->find(regexPattern, cursor, flags);
    } else {
        cursor = impl_->textEdit_->document()->find(text, cursor, flags);
    }

    if (!cursor.isNull()) {
        impl_->textEdit_->setTextCursor(cursor);
        impl_->textEdit_->ensureCursorVisible();
    }
}

void TextEditor::replaceText(const QString& findText, const QString& replaceText, bool caseSensitive, bool wholeWords, bool regex) {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == findText) {
        cursor.insertText(replaceText);
    } else {
        findText(findText, caseSensitive, wholeWords, regex);
        if (impl_->textEdit_->textCursor().hasSelection()) {
            impl_->textEdit_->textCursor().insertText(replaceText);
        }
    }
}

void TextEditor::findNext() {
    // Implementation would need to store last search parameters
}

void TextEditor::findPrevious() {
    // Implementation would need to store last search parameters
}

void TextEditor::gotoLine(int lineNumber) {
    QTextBlock block = impl_->textEdit_->document()->findBlockByNumber(lineNumber - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        impl_->textEdit_->setTextCursor(cursor);
        impl_->textEdit_->ensureCursorVisible();
    }
}

void TextEditor::gotoPosition(int position) {
    QTextCursor cursor(impl_->textEdit_->document());
    cursor.setPosition(position);
    impl_->textEdit_->setTextCursor(cursor);
    impl_->textEdit_->ensureCursorVisible();
}

void TextEditor::indent() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    if (cursor.hasSelection()) {
        // Indent selected lines
        QTextBlock startBlock = impl_->textEdit_->document()->findBlock(cursor.selectionStart());
        QTextBlock endBlock = impl_->textEdit_->document()->findBlock(cursor.selectionEnd());

        cursor.beginEditBlock();
        for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next()) {
            cursor.setPosition(block.position());
            cursor.insertText(QString(config_.defaultTabWidth, ' '));
        }
        cursor.endEditBlock();
    } else {
        // Indent current line
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.insertText(QString(config_.defaultTabWidth, ' '));
        cursor.endEditBlock();
    }
}

void TextEditor::unindent() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    if (cursor.hasSelection()) {
        // Unindent selected lines
        QTextBlock startBlock = impl_->textEdit_->document()->findBlock(cursor.selectionStart());
        QTextBlock endBlock = impl_->textEdit_->document()->findBlock(cursor.selectionEnd());

        cursor.beginEditBlock();
        for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next()) {
            cursor.setPosition(block.position());
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, qMin(block.text().size(), config_.defaultTabWidth));
            QString text = cursor.selectedText();
            if (text.trimmed().isEmpty() && text.size() <= config_.defaultTabWidth) {
                cursor.removeSelectedText();
            }
            cursor.movePosition(QTextCursor::EndOfLine);
        }
        cursor.endEditBlock();
    } else {
        // Unindent current line
        QTextBlock currentBlock = cursor.block();
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, qMin(currentBlock.text().size(), config_.defaultTabWidth));
        QString text = cursor.selectedText();
        if (text.trimmed().isEmpty() && text.size() <= config_.defaultTabWidth) {
            cursor.removeSelectedText();
        }
        cursor.endEditBlock();
    }
}

void TextEditor::commentLine() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    QTextBlock startBlock = impl_->textEdit_->document()->findBlock(cursor.selectionStart());
    QTextBlock endBlock = impl_->textEdit_->document()->findBlock(cursor.selectionEnd());

    cursor.beginEditBlock();
    for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next()) {
        QString text = block.text();
        if (!text.trimmed().startsWith("--")) {
            cursor.setPosition(block.position());
            cursor.insertText("-- ");
        }
    }
    cursor.endEditBlock();
}

void TextEditor::uncommentLine() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    QTextBlock startBlock = impl_->textEdit_->document()->findBlock(cursor.selectionStart());
    QTextBlock endBlock = impl_->textEdit_->document()->findBlock(cursor.selectionEnd());

    cursor.beginEditBlock();
    for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next()) {
        QString text = block.text();
        if (text.trimmed().startsWith("--")) {
            cursor.setPosition(block.position());
            if (text.startsWith("-- ")) {
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 3);
            } else {
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);
            }
            cursor.removeSelectedText();
        }
    }
    cursor.endEditBlock();
}

void TextEditor::duplicateLine() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    QTextBlock currentBlock = cursor.block();

    cursor.beginEditBlock();
    cursor.setPosition(currentBlock.position() + currentBlock.text().size());
    cursor.insertText("\n" + currentBlock.text());
    cursor.endEditBlock();
}

void TextEditor::deleteLine() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    QTextBlock currentBlock = cursor.block();

    cursor.beginEditBlock();
    cursor.setPosition(currentBlock.position());
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    if (currentBlock.next().isValid()) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

void TextEditor::toUpperCase() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    if (cursor.hasSelection()) {
        QString text = cursor.selectedText().toUpper();
        cursor.insertText(text);
    }
}

void TextEditor::toLowerCase() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    if (cursor.hasSelection()) {
        QString text = cursor.selectedText().toLower();
        cursor.insertText(text);
    }
}

void TextEditor::toTitleCase() {
    QTextCursor cursor = impl_->textEdit_->textCursor();
    if (cursor.hasSelection()) {
        QString text = cursor.selectedText();
        QStringList words = text.split(' ');
        for (QString& word : words) {
            if (!word.isEmpty()) {
                word = word.toLower();
                word[0] = word[0].toUpper();
            }
        }
        cursor.insertText(words.join(' '));
    }
}

DocumentInfo TextEditor::getDocumentInfo() const {
    return documentInfo_;
}

void TextEditor::setDocumentInfo(const DocumentInfo& info) {
    documentInfo_ = info;

    // Update window title or emit signal for parent to update
    emit documentModified(documentInfo_.isModified);
}

EditorConfiguration TextEditor::getConfiguration() const {
    return config_;
}

void TextEditor::updateConfiguration(const EditorConfiguration& config) {
    config_ = config;
    impl_->applyConfiguration();
}

void TextEditor::setEditorMode(EditorMode mode) {
    documentInfo_.mode = mode;
}

EditorMode TextEditor::getEditorMode() const {
    return documentInfo_.mode;
}

void TextEditor::setTextEncoding(TextEncoding encoding) {
    documentInfo_.encoding = encoding;
}

TextEncoding TextEditor::getTextEncoding() const {
    return documentInfo_.encoding;
}

int TextEditor::getLineCount() const {
    return impl_->textEdit_->blockCount();
}

int TextEditor::getWordCount() const {
    QString text = getText();
    return text.split(QRegExp("\\s+"), QString::SkipEmptyParts).size();
}

int TextEditor::getCharacterCount() const {
    return getText().length();
}

int TextEditor::getSelectedCharacterCount() const {
    return getSelectedText().length();
}

bool TextEditor::isModified() const {
    return documentInfo_.isModified;
}

void TextEditor::setModified(bool modified) {
    documentInfo_.isModified = modified;
    impl_->textEdit_->document()->setModified(modified);
}

void TextEditor::setTextChangedCallback(TextChangedCallback callback) {
    textChangedCallback_ = callback;
}

void TextEditor::setCursorPositionChangedCallback(CursorPositionChangedCallback callback) {
    cursorPositionChangedCallback_ = callback;
}

void TextEditor::setSelectionChangedCallback(SelectionChangedCallback callback) {
    selectionChangedCallback_ = callback;
}

void TextEditor::setDocumentModifiedCallback(DocumentModifiedCallback callback) {
    documentModifiedCallback_ = callback;
}

QWidget* TextEditor::getWidget() {
    return this;
}

QTextDocument* TextEditor::getDocument() {
    return impl_->textEdit_->document();
}

QPlainTextEdit* TextEditor::getTextEdit() {
    return impl_->textEdit_;
}

// Private slot implementations

void TextEditor::onTextChanged() {
    documentInfo_.isModified = true;
    documentInfo_.modifiedAt = QDateTime::currentDateTimeUtc();

    if (textChangedCallback_) {
        textChangedCallback_();
    }
}

void TextEditor::onCursorPositionChanged() {
    currentPosition_ = impl_->getCursorPosition();

    if (cursorPositionChangedCallback_) {
        cursorPositionChangedCallback_(currentPosition_);
    }
}

void TextEditor::onSelectionChanged() {
    currentSelection_ = impl_->getSelection();

    if (selectionChangedCallback_) {
        selectionChangedCallback_(currentSelection_);
    }
}

void TextEditor::onDocumentModified(bool modified) {
    documentInfo_.isModified = modified;

    if (documentModifiedCallback_) {
        documentModifiedCallback_(modified);
    }
}

void TextEditor::onAutoSaveTimer() {
    if (config_.enableAutoSave && documentInfo_.isModified && !documentInfo_.isNew) {
        saveFile();
    }
}

void TextEditor::onContextMenuRequested(const QPoint& pos) {
    // Create context menu
    QMenu menu(this);

    // Edit actions
    menu.addAction("Undo", this, &TextEditor::undo)->setEnabled(canUndo());
    menu.addAction("Redo", this, &TextEditor::redo)->setEnabled(canRedo());
    menu.addSeparator();
    menu.addAction("Cut", this, &TextEditor::cut)->setEnabled(!getSelection().selectedText.isEmpty());
    menu.addAction("Copy", this, &TextEditor::copy)->setEnabled(!getSelection().selectedText.isEmpty());
    menu.addAction("Paste", this, &TextEditor::paste)->setEnabled(!QApplication::clipboard()->text().isEmpty());
    menu.addAction("Select All", this, &TextEditor::selectAll);

    menu.exec(impl_->textEdit_->mapToGlobal(pos));
}

void TextEditor::onCompletionActivated(const QModelIndex& index) {
    // Handle completion activation
    if (impl_->completer_) {
        QTextCursor cursor = impl_->textEdit_->textCursor();
        cursor.insertText(impl_->completer_->completionModel()->data(index).toString());
    }
}

} // namespace scratchrobin
