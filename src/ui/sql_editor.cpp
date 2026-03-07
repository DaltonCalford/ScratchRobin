#include "ui/sql_editor.h"
#include "backend/session_client.h"

#include <QPainter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTextBlock>
#include <QKeyEvent>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>

namespace scratchrobin::ui {

// ============================================================================
// SQL Syntax Highlighter
// ============================================================================

SqlSyntaxHighlighter::SqlSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
  setupRules();
}

void SqlSyntaxHighlighter::setupRules() {
  // Keywords
  QStringList keywords;
  keywords << "SELECT" << "FROM" << "WHERE" << "INSERT" << "UPDATE" << "DELETE"
           << "CREATE" << "DROP" << "ALTER" << "TABLE" << "INDEX" << "VIEW"
           << "JOIN" << "LEFT" << "RIGHT" << "INNER" << "OUTER" << "ON"
           << "AND" << "OR" << "NOT" << "NULL" << "IS" << "IN" << "EXISTS"
           << "ORDER" << "BY" << "GROUP" << "HAVING" << "LIMIT" << "OFFSET"
           << "UNION" << "ALL" << "DISTINCT" << "AS" << "VALUES" << "SET"
           << "BEGIN" << "COMMIT" << "ROLLBACK" << "TRANSACTION" << "PRIMARY"
           << "KEY" << "FOREIGN" << "REFERENCES" << "UNIQUE" << "CHECK" << "DEFAULT"
           << "INT" << "VARCHAR" << "CHAR" << "TEXT" << "DATE" << "TIME"
           << "DATETIME" << "DECIMAL" << "NUMERIC" << "FLOAT" << "BOOLEAN";
  
  keywordFormat_.setForeground(QColor(0, 0, 255));
  keywordFormat_.setFontWeight(QFont::Bold);
  
  for (const QString& keyword : keywords) {
    HighlightingRule rule;
    rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(keyword), QRegularExpression::CaseInsensitiveOption);
    rule.format = keywordFormat_;
    rules_.append(rule);
  }
  
  // Functions
  QStringList functions;
  functions << "COUNT" << "SUM" << "AVG" << "MIN" << "MAX" << "CONCAT"
            << "SUBSTRING" << "LENGTH" << "UPPER" << "LOWER" << "COALESCE"
            << "CAST" << "NOW" << "CURRENT_DATE" << "CURRENT_TIMESTAMP";
  
  functionFormat_.setForeground(QColor(128, 0, 128));
  
  for (const QString& function : functions) {
    HighlightingRule rule;
    rule.pattern = QRegularExpression(QString("\\b%1(?=\\s*\\()").arg(function), QRegularExpression::CaseInsensitiveOption);
    rule.format = functionFormat_;
    rules_.append(rule);
  }
  
  // Strings
  stringFormat_.setForeground(QColor(0, 128, 0));
  HighlightingRule stringRule;
  stringRule.pattern = QRegularExpression("'[^']*'");
  stringRule.format = stringFormat_;
  rules_.append(stringRule);
  
  // Comments
  commentFormat_.setForeground(QColor(128, 128, 128));
  commentFormat_.setFontItalic(true);
  HighlightingRule commentRule;
  commentRule.pattern = QRegularExpression("--[^\\n]*");
  commentRule.format = commentFormat_;
  rules_.append(commentRule);
  
  // Multi-line comments
  HighlightingRule multiLineComment;
  multiLineComment.pattern = QRegularExpression("/\\*.*?\\*/");
  multiLineComment.format = commentFormat_;
  rules_.append(multiLineComment);
  
  // Numbers
  numberFormat_.setForeground(QColor(255, 0, 0));
  HighlightingRule numberRule;
  numberRule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?\\b");
  numberRule.format = numberFormat_;
  rules_.append(numberRule);
  
  // Operators
  operatorFormat_.setForeground(QColor(0, 0, 128));
  HighlightingRule operatorRule;
  operatorRule.pattern = QRegularExpression("[+\\-*/=<>!]+");
  operatorRule.format = operatorFormat_;
  rules_.append(operatorRule);
}

void SqlSyntaxHighlighter::setColorScheme(const QString& scheme) {
  Q_UNUSED(scheme)
  // TODO: Implement different color schemes (light, dark)
}

void SqlSyntaxHighlighter::highlightBlock(const QString& text) {
  for (const HighlightingRule& rule : rules_) {
    QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }
  
  // Handle multi-line comments
  setCurrentBlockState(0);
  
  int startIndex = 0;
  if (previousBlockState() != 1) {
    startIndex = text.indexOf("/*");
  }
  
  while (startIndex >= 0) {
    int endIndex = text.indexOf("*/", startIndex);
    int commentLength;
    if (endIndex == -1) {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    } else {
      commentLength = endIndex - startIndex + 2;
    }
    setFormat(startIndex, commentLength, commentFormat_);
    startIndex = text.indexOf("/*", startIndex + commentLength);
  }
}

// ============================================================================
// Line Number Area
// ============================================================================

LineNumberArea::LineNumberArea(SqlEditor* editor) : QWidget(editor), editor_(editor) {}

QSize LineNumberArea::sizeHint() const {
  return QSize(editor_->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
  editor_->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent* event) {
  // Handle line number click (select entire line)
  int lineHeight = editor_->fontMetrics().height();
  int lineNumber = event->y() / lineHeight;
  
  QTextBlock block = editor_->document()->findBlockByNumber(lineNumber);
  if (block.isValid()) {
    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    editor_->setTextCursor(cursor);
  }
}

// ============================================================================
// Code Folding Area
// ============================================================================

CodeFoldingArea::CodeFoldingArea(SqlEditor* editor)
    : QWidget(editor), editor_(editor) {
}

QSize CodeFoldingArea::sizeHint() const {
  return QSize(15, 0);
}

void CodeFoldingArea::paintEvent(QPaintEvent* event) {
  editor_->drawFoldingMarkers(event);
}

void CodeFoldingArea::mousePressEvent(QMouseEvent* event) {
  int lineHeight = editor_->fontMetrics().height();
  int blockNumber = event->y() / lineHeight;
  
  QTextBlock block = editor_->document()->findBlockByNumber(blockNumber);
  if (block.isValid() && editor_->isFoldable(block)) {
    emit foldClicked(blockNumber);
  }
}

// ============================================================================
// SQL Editor
// ============================================================================

SqlEditor::SqlEditor(backend::SessionClient* client, QWidget* parent)
    : QPlainTextEdit(parent)
    , session_client_(client)
    , highlighter_(nullptr)
    , completer_(nullptr)
    , completionModel_(nullptr)
    , codeFoldingArea_(nullptr) {
  setupEditor();
  setupCompleter();
  setupFolding();
}

SqlEditor::~SqlEditor() = default;

void SqlEditor::setupEditor() {
  QFont font("Consolas", 10);
  font.setFixedPitch(true);
  setFont(font);
  setLineWrapMode(QPlainTextEdit::NoWrap);
  
  highlighter_ = new SqlSyntaxHighlighter(document());
  
  lineNumberArea_ = new LineNumberArea(this);
  
  connect(this, &QPlainTextEdit::blockCountChanged,
          this, &SqlEditor::updateLineNumberAreaWidth);
  connect(this, &QPlainTextEdit::updateRequest,
          this, &SqlEditor::updateLineNumberArea);
  connect(this, &QPlainTextEdit::cursorPositionChanged,
          this, &SqlEditor::highlightCurrentLine);
  
  updateLineNumberAreaWidth(0);
  highlightCurrentLine();
}

void SqlEditor::setupFolding() {
  codeFoldingArea_ = new CodeFoldingArea(this);
  connect(codeFoldingArea_, &CodeFoldingArea::foldClicked,
          this, &SqlEditor::onFoldClicked);
  
  updateLineNumberAreaWidth(0);
}

void SqlEditor::setupCompleter() {
  QStringList keywords;
  keywords << "SELECT" << "FROM" << "WHERE" << "AND" << "OR" << "INSERT"
           << "UPDATE" << "DELETE" << "CREATE" << "TABLE" << "INDEX"
           << "JOIN" << "LEFT" << "RIGHT" << "INNER" << "ON";
  
  completionModel_ = new QStringListModel(keywords, this);
  
  completer_ = new QCompleter(this);
  completer_->setModel(completionModel_);
  completer_->setCompletionMode(QCompleter::PopupCompletion);
  completer_->setCaseSensitivity(Qt::CaseInsensitive);
  completer_->setWrapAround(false);
  
  connect(completer_, QOverload<const QString&>::of(&QCompleter::activated),
          this, &SqlEditor::insertCompletion);
}

void SqlEditor::setKeywords(const QStringList& keywords) {
  completionModel_->setStringList(keywords);
}

// ============================================================================
// Line Numbers
// ============================================================================

int SqlEditor::lineNumberAreaWidth() {
  int digits = 1;
  int max = qMax(1, blockCount());
  while (max >= 10) {
    max /= 10;
    ++digits;
  }
  int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
  
  // Add space for folding area
  if (codeFoldingEnabled_ && codeFoldingArea_) {
    space += 15;
  }
  
  return space;
}

void SqlEditor::updateLineNumberAreaWidth(int) {
  setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void SqlEditor::updateLineNumberArea(const QRect& rect, int dy) {
  if (dy) {
    lineNumberArea_->scroll(0, dy);
    if (codeFoldingArea_) codeFoldingArea_->scroll(0, dy);
  } else {
    lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    if (codeFoldingArea_) {
      codeFoldingArea_->update(0, rect.y(), codeFoldingArea_->width(), rect.height());
    }
  }
  if (rect.contains(viewport()->rect())) {
    updateLineNumberAreaWidth(0);
  }
}

void SqlEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
  QPainter painter(lineNumberArea_);
  painter.fillRect(event->rect(), QColor(240, 240, 240));
  
  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());
  
  int foldingWidth = codeFoldingEnabled_ ? 15 : 0;
  
  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      QString number = QString::number(blockNumber + 1);
      painter.setPen(QColor(120, 120, 120));
      painter.drawText(foldingWidth, top, lineNumberArea_->width() - foldingWidth - 5, 
                     fontMetrics().height(),
                     Qt::AlignRight, number);
    }
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++blockNumber;
  }
}

// ============================================================================
// Code Folding
// ============================================================================

void SqlEditor::setCodeFoldingEnabled(bool enabled) {
  codeFoldingEnabled_ = enabled;
  if (codeFoldingArea_) {
    codeFoldingArea_->setVisible(enabled);
  }
  updateLineNumberAreaWidth(0);
}

void SqlEditor::toggleFold(int blockNumber) {
  QTextBlock block = document()->findBlockByNumber(blockNumber);
  if (!block.isValid() || !isFoldable(block)) return;
  
  foldedBlocks_[blockNumber] = !foldedBlocks_.value(blockNumber, false);
  
  // Hide/show blocks
  int endBlock = findMatchingEnd(block);
  if (endBlock > blockNumber) {
    bool folded = foldedBlocks_[blockNumber];
    QTextBlock current = block.next();
    while (current.isValid() && current.blockNumber() < endBlock) {
      current.setVisible(!folded);
      current = current.next();
    }
  }
  
  viewport()->update();
  emit foldToggled(blockNumber, foldedBlocks_[blockNumber]);
}

void SqlEditor::onFoldClicked(int line) {
  toggleFold(line);
}

bool SqlEditor::isFolded(int blockNumber) const {
  return foldedBlocks_.value(blockNumber, false);
}

bool SqlEditor::isFoldable(const QTextBlock& block) const {
  QString text = block.text().trimmed().toUpper();
  return text.startsWith("SELECT") || text.startsWith("INSERT") ||
         text.startsWith("UPDATE") || text.startsWith("DELETE") ||
         text.startsWith("CREATE") || text.startsWith("BEGIN");
}

bool SqlEditor::isFoldStart(const QTextBlock& block) const {
  return isFoldable(block);
}

int SqlEditor::findMatchingEnd(const QTextBlock& startBlock) const {
  int startIndent = startBlock.text().indexOf(QRegularExpression("\\S"));
  
  QTextBlock current = startBlock.next();
  while (current.isValid()) {
    QString text = current.text().trimmed();
    int indent = current.text().indexOf(QRegularExpression("\\S"));
    
    if (!text.isEmpty() && indent <= startIndent && 
        (text.toUpper().startsWith("SELECT") ||
         text.toUpper().startsWith("INSERT") ||
         text.toUpper().startsWith("UPDATE") ||
         text.toUpper().startsWith("DELETE") ||
         text.toUpper().startsWith("CREATE") ||
         text.toUpper().startsWith("END"))) {
      return current.blockNumber();
    }
    current = current.next();
  }
  
  return document()->blockCount() - 1;
}

void SqlEditor::drawFoldingMarkers(QPaintEvent* event) {
  QPainter painter(codeFoldingArea_);
  painter.fillRect(event->rect(), QColor(230, 230, 230));
  
  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());
  
  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      if (isFoldable(block)) {
        bool folded = foldedBlocks_.value(blockNumber, false);
        
        QRect markerRect(2, top + 3, 11, 11);
        painter.setPen(QColor(100, 100, 100));
        painter.drawRect(markerRect);
        
        painter.setPen(Qt::black);
        if (folded) {
          painter.drawLine(5, top + 9, 10, top + 9);
          painter.drawLine(8, top + 6, 8, top + 12);
        } else {
          painter.drawLine(5, top + 9, 10, top + 9);
        }
      }
    }
    
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++blockNumber;
  }
}

// ============================================================================
// Zoom
// ============================================================================

void SqlEditor::zoomIn(int range) {
  zoomLevel_ = qMin(ZOOM_MAX, zoomLevel_ + range);
  QFont f = font();
  f.setPointSizeF(10.0 + zoomLevel_ * 0.5);
  setFont(f);
  emit zoomChanged(zoomLevel_);
}

void SqlEditor::zoomOut(int range) {
  zoomLevel_ = qMax(ZOOM_MIN, zoomLevel_ - range);
  QFont f = font();
  f.setPointSizeF(10.0 + zoomLevel_ * 0.5);
  setFont(f);
  emit zoomChanged(zoomLevel_);
}

void SqlEditor::resetZoom() {
  zoomLevel_ = 0;
  QFont f = font();
  f.setPointSizeF(10.0);
  setFont(f);
  emit zoomChanged(zoomLevel_);
}

// ============================================================================
// SQL Formatting
// ============================================================================

void SqlEditor::formatSql() {
  QString sql = toPlainText();
  if (sql.isEmpty()) return;
  
  SqlFormatter::FormatOptions options;
  options.uppercaseKeywords = true;
  options.indentSize = 4;
  options.useTabs = false;
  
  QString formatted = SqlFormatter::format(sql, options);
  setPlainText(formatted);
}

void SqlEditor::minifySql() {
  QString sql = toPlainText();
  if (sql.isEmpty()) return;
  
  QString minified = SqlFormatter::minify(sql);
  setPlainText(minified);
}

void SqlEditor::toggleComment() {
  QTextCursor cursor = textCursor();
  if (!cursor.hasSelection()) {
    cursor.select(QTextCursor::LineUnderCursor);
  }
  
  int start = cursor.selectionStart();
  cursor.setPosition(start);
  cursor.movePosition(QTextCursor::StartOfBlock);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  QString line = cursor.selectedText();
  
  if (line.trimmed().startsWith("--")) {
    uncommentSelection();
  } else {
    commentSelection();
  }
}

void SqlEditor::commentSelection() {
  QTextCursor cursor = textCursor();
  if (!cursor.hasSelection()) {
    cursor.select(QTextCursor::LineUnderCursor);
  }
  
  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();
  
  cursor.setPosition(start);
  int startLine = cursor.blockNumber();
  cursor.setPosition(end);
  int endLine = cursor.blockNumber();
  
  cursor.beginEditBlock();
  for (int i = startLine; i <= endLine; ++i) {
    cursor.movePosition(QTextCursor::Start);
    for (int j = 0; j < i; ++j) {
      cursor.movePosition(QTextCursor::NextBlock);
    }
    cursor.insertText("-- ");
  }
  cursor.endEditBlock();
}

void SqlEditor::uncommentSelection() {
  QTextCursor cursor = textCursor();
  if (!cursor.hasSelection()) {
    cursor.select(QTextCursor::LineUnderCursor);
  }
  
  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();
  
  cursor.setPosition(start);
  int startLine = cursor.blockNumber();
  cursor.setPosition(end);
  int endLine = cursor.blockNumber();
  
  cursor.beginEditBlock();
  for (int i = startLine; i <= endLine; ++i) {
    cursor.movePosition(QTextCursor::Start);
    for (int j = 0; j < i; ++j) {
      cursor.movePosition(QTextCursor::NextBlock);
    }
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    QString line = cursor.selectedText();
    if (line.trimmed().startsWith("--")) {
      int pos = line.indexOf("--");
      cursor.setPosition(cursor.block().position() + pos);
      cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);
      if (line.mid(pos + 2, 1) == " ") {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
      }
      cursor.removeSelectedText();
    }
  }
  cursor.endEditBlock();
}

void SqlEditor::selectCurrentStatement() {
  QTextCursor cursor = textCursor();
  QTextBlock block = cursor.block();
  
  // Find start of statement
  while (block.isValid() && !isFoldStart(block)) {
    block = block.previous();
  }
  
  if (!block.isValid()) {
    block = cursor.block();
  }
  
  int startPos = block.position();
  int endPos = findMatchingEnd(block);
  
  QTextBlock endBlock = document()->findBlockByNumber(endPos);
  int endPosition = endBlock.position() + endBlock.length();
  
  cursor.setPosition(startPos);
  cursor.setPosition(endPosition, QTextCursor::KeepAnchor);
  setTextCursor(cursor);
}

// ============================================================================
// Other Methods
// ============================================================================

void SqlEditor::setWordWrap(bool wrap) {
  setLineWrapMode(wrap ? WidgetWidth : NoWrap);
}

bool SqlEditor::wordWrap() const {
  return lineWrapMode() == WidgetWidth;
}

void SqlEditor::autoIndent() {
  QTextCursor cursor = textCursor();
  QTextBlock block = cursor.block().previous();
  if (!block.isValid()) return;
  
  QString prevLine = block.text();
  int indent = 0;
  
  for (QChar c : prevLine) {
    if (c == ' ') indent++;
    else if (c == '\t') indent += 4;
    else break;
  }
  
  QString trimmed = prevLine.trimmed().toUpper();
  if (trimmed.endsWith("(") || 
      trimmed.startsWith("SELECT") ||
      trimmed.startsWith("FROM") ||
      trimmed.startsWith("WHERE") ||
      trimmed.startsWith("JOIN") ||
      trimmed.startsWith("BEGIN")) {
    indent += 4;
  }
  
  cursor.insertText(QString(indent, ' '));
}

QString SqlEditor::selectedSql() const {
  QString selected = textCursor().selectedText();
  if (!selected.isEmpty()) {
    return selected;
  }
  return currentStatement();
}

QString SqlEditor::currentStatement() const {
  QTextCursor cursor = textCursor();
  cursor.movePosition(QTextCursor::StartOfBlock);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  return cursor.selectedText();
}

// ============================================================================
// Event Handlers
// ============================================================================

void SqlEditor::keyPressEvent(QKeyEvent* event) {
  // Zoom shortcuts
  if (event->modifiers() & Qt::ControlModifier) {
    switch (event->key()) {
      case Qt::Key_Plus:
      case Qt::Key_Equal:
        zoomIn();
        return;
      case Qt::Key_Minus:
        zoomOut();
        return;
      case Qt::Key_0:
        resetZoom();
        return;
      case Qt::Key_Slash:
        toggleComment();
        return;
    }
  }
  
  // Handle completion
  if (completer_->popup()->isVisible()) {
    switch (event->key()) {
      case Qt::Key_Enter:
      case Qt::Key_Return:
      case Qt::Key_Escape:
      case Qt::Key_Tab:
      case Qt::Key_Backtab:
        event->ignore();
        return;
      default:
        break;
    }
  }
  
  // F5 to execute
  if (event->key() == Qt::Key_F5) {
    emit executeRequested();
    return;
  }
  
  // Ctrl+Enter to execute
  if (event->key() == Qt::Key_Return && event->modifiers() & Qt::ControlModifier) {
    emit executeRequested();
    return;
  }
  
  // Auto-indent on Enter
  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    QPlainTextEdit::keyPressEvent(event);
    autoIndent();
    return;
  }
  
  QPlainTextEdit::keyPressEvent(event);
  
  // Show completion
  QString completionPrefix = textUnderCursor();
  if (completionPrefix.length() >= 2) {
    completer_->setCompletionPrefix(completionPrefix);
    completer_->popup()->setCurrentIndex(completer_->completionModel()->index(0, 0));
    QRect cr = cursorRect();
    cr.setWidth(completer_->popup()->sizeHintForColumn(0) + 
                completer_->popup()->verticalScrollBar()->sizeHint().width());
    completer_->complete(cr);
  }
}

void SqlEditor::wheelEvent(QWheelEvent* event) {
  if (event->modifiers() & Qt::ControlModifier) {
    if (event->angleDelta().y() > 0) {
      zoomIn();
    } else {
      zoomOut();
    }
    event->accept();
    return;
  }
  QPlainTextEdit::wheelEvent(event);
}

void SqlEditor::contextMenuEvent(QContextMenuEvent* event) {
  QMenu* menu = createStandardContextMenu();
  
  menu->addSeparator();
  
  // Zoom actions
  QMenu* zoomMenu = menu->addMenu(tr("Zoom"));
  QAction* zoomInAction = zoomMenu->addAction(tr("Zoom In"));
  QAction* zoomOutAction = zoomMenu->addAction(tr("Zoom Out"));
  QAction* zoomResetAction = zoomMenu->addAction(tr("Reset Zoom"));
  zoomMenu->addSeparator();
  zoomMenu->addAction(tr("Current: %1%").arg(100 + zoomLevel_ * 10))->setEnabled(false);
  
  connect(zoomInAction, &QAction::triggered, this, &SqlEditor::zoomIn);
  connect(zoomOutAction, &QAction::triggered, this, &SqlEditor::zoomOut);
  connect(zoomResetAction, &QAction::triggered, this, &SqlEditor::resetZoom);
  
  // Format actions
  menu->addSeparator();
  QAction* formatAction = menu->addAction(tr("Format SQL"));
  QAction* minifyAction = menu->addAction(tr("Minify SQL"));
  QAction* commentAction = menu->addAction(tr("Toggle Comment"));
  
  connect(formatAction, &QAction::triggered, this, &SqlEditor::formatSql);
  connect(minifyAction, &QAction::triggered, this, &SqlEditor::minifySql);
  connect(commentAction, &QAction::triggered, this, &SqlEditor::toggleComment);
  
  menu->exec(event->globalPos());
  delete menu;
}

void SqlEditor::resizeEvent(QResizeEvent* event) {
  QPlainTextEdit::resizeEvent(event);
  QRect cr = contentsRect();
  
  int foldingWidth = codeFoldingEnabled_ ? 15 : 0;
  lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth() - foldingWidth, cr.height()));
  
  if (codeFoldingArea_) {
    codeFoldingArea_->setGeometry(QRect(cr.left(), cr.top(), foldingWidth, cr.height()));
  }
}

void SqlEditor::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasText()) {
    event->acceptProposedAction();
  }
}

void SqlEditor::dropEvent(QDropEvent* event) {
  if (event->mimeData()->hasText()) {
    QString text = event->mimeData()->text();
    QTextCursor cursor = textCursor();
    cursor.insertText(text);
    setTextCursor(cursor);
    event->acceptProposedAction();
  }
}

void SqlEditor::highlightCurrentLine() {
  QList<QTextEdit::ExtraSelection> extraSelections;
  
  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
  }
  
  setExtraSelections(extraSelections);
}

void SqlEditor::insertCompletion(const QString& completion) {
  QTextCursor cursor = textCursor();
  int extra = completion.length() - completer_->completionPrefix().length();
  cursor.movePosition(QTextCursor::Left);
  cursor.movePosition(QTextCursor::EndOfWord);
  cursor.insertText(completion.right(extra));
  setTextCursor(cursor);
}

QString SqlEditor::textUnderCursor() const {
  QTextCursor cursor = textCursor();
  cursor.select(QTextCursor::WordUnderCursor);
  return cursor.selectedText();
}

// ============================================================================
// SQL Formatter Implementation
// ============================================================================

QString SqlFormatter::format(const QString& sql, const FormatOptions& options) {
  if (sql.trimmed().isEmpty()) return sql;
  
  QString upperSql = sql.toUpper().trimmed();
  
  if (upperSql.startsWith("SELECT")) {
    return formatSelect(sql, options);
  } else if (upperSql.startsWith("INSERT")) {
    return formatInsert(sql, options);
  } else if (upperSql.startsWith("UPDATE")) {
    return formatUpdate(sql, options);
  } else if (upperSql.startsWith("DELETE")) {
    return formatDelete(sql, options);
  } else if (upperSql.startsWith("CREATE")) {
    return formatCreate(sql, options);
  }
  
  return sql;
}

QString SqlFormatter::minify(const QString& sql) {
  QString result = sql.simplified();
  result.replace(QRegularExpression("\\s*([,;()])\\s*"), "\\1");
  return result;
}

QString SqlFormatter::formatSelect(const QString& sql, const FormatOptions& options) {
  QStringList lines;
  QStringList tokens = sql.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  QString currentLine = indentStr(0, options) + (options.uppercaseKeywords ? "SELECT" : "select");
  
  bool afterSelect = true;
  for (int i = 1; i < tokens.size(); ++i) {
    QString token = tokens[i].toUpper();
    
    if (token == "FROM" || token == "WHERE" || token == "ORDER" ||
        token == "GROUP" || token == "HAVING" || token == "LIMIT" ||
        token == "UNION" || token == "JOIN" || token == "INNER" ||
        token == "LEFT" || token == "RIGHT" || token == "FULL" ||
        token == "OUTER") {
      lines.append(currentLine);
      
      if (token == "ORDER") {
        currentLine = indentStr(1, options) + (options.uppercaseKeywords ? "ORDER BY" : "order by");
        i++;
      } else if (token == "GROUP") {
        currentLine = indentStr(1, options) + (options.uppercaseKeywords ? "GROUP BY" : "group by");
        i++;
      } else {
        currentLine = indentStr(1, options) + (options.uppercaseKeywords ? token : token.toLower());
      }
      afterSelect = false;
    } else {
      if (afterSelect) {
        currentLine += ", ";
      } else {
        currentLine += " ";
      }
      currentLine += options.uppercaseKeywords ? tokens[i].toUpper() : tokens[i].toLower();
    }
  }
  
  if (!currentLine.isEmpty()) {
    lines.append(currentLine);
  }
  
  return lines.join("\n");
}

QString SqlFormatter::formatInsert(const QString& sql, const FormatOptions& options) {
  QString result = sql;
  result.replace(QRegularExpression("\\s*\\(\\s*"), " (");
  result.replace(QRegularExpression("\\s*\\)\\s*"), ") ");
  result.replace(QRegularExpression("\\s*,\\s*"), ", ");
  
  if (options.uppercaseKeywords) {
    result.replace(QRegularExpression("\\binsert\\b", QRegularExpression::CaseInsensitiveOption), "INSERT");
    result.replace(QRegularExpression("\\binto\\b", QRegularExpression::CaseInsensitiveOption), "INTO");
    result.replace(QRegularExpression("\\bvalues\\b", QRegularExpression::CaseInsensitiveOption), "VALUES");
  }
  
  return result;
}

QString SqlFormatter::formatUpdate(const QString& sql, const FormatOptions& options) {
  QStringList lines;
  QStringList tokens = sql.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  
  QString currentLine = options.uppercaseKeywords ? "UPDATE" : "update";
  bool afterSet = false;
  
  for (int i = 1; i < tokens.size(); ++i) {
    QString token = tokens[i].toUpper();
    
    if (token == "SET" || token == "WHERE") {
      lines.append(currentLine);
      currentLine = indentStr(1, options) + (options.uppercaseKeywords ? token : token.toLower());
      afterSet = (token == "SET");
    } else {
      if (afterSet && token == ",") {
        lines.append(currentLine);
        currentLine = indentStr(1, options);
      } else {
        currentLine += " ";
      }
      currentLine += options.uppercaseKeywords ? tokens[i].toUpper() : tokens[i].toLower();
    }
  }
  
  if (!currentLine.isEmpty()) {
    lines.append(currentLine);
  }
  
  return lines.join("\n");
}

QString SqlFormatter::formatDelete(const QString& sql, const FormatOptions& options) {
  QStringList lines;
  QStringList tokens = sql.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  
  QString currentLine = options.uppercaseKeywords ? "DELETE FROM" : "delete from";
  
  for (int i = 2; i < tokens.size(); ++i) {
    QString token = tokens[i].toUpper();
    
    if (token == "WHERE" || token == "LIMIT") {
      lines.append(currentLine);
      currentLine = indentStr(1, options) + (options.uppercaseKeywords ? token : token.toLower());
    } else {
      currentLine += " ";
      currentLine += options.uppercaseKeywords ? tokens[i].toUpper() : tokens[i].toLower();
    }
  }
  
  if (!currentLine.isEmpty()) {
    lines.append(currentLine);
  }
  
  return lines.join("\n");
}

QString SqlFormatter::formatCreate(const QString& sql, const FormatOptions& options) {
  QString result = sql;
  result.replace(QRegularExpression("\\s*\\(\\s*"), " (\n" + indentStr(1, options));
  result.replace(QRegularExpression("\\s*\\)\\s*"), "\n)");
  result.replace(QRegularExpression("\\s*,\\s*"), ",\n" + indentStr(1, options));
  
  if (options.uppercaseKeywords) {
    result.replace(QRegularExpression("\\bcreate\\b", QRegularExpression::CaseInsensitiveOption), "CREATE");
    result.replace(QRegularExpression("\\btable\\b", QRegularExpression::CaseInsensitiveOption), "TABLE");
    result.replace(QRegularExpression("\\bif\\b", QRegularExpression::CaseInsensitiveOption), "IF");
    result.replace(QRegularExpression("\\bnot\\b", QRegularExpression::CaseInsensitiveOption), "NOT");
    result.replace(QRegularExpression("\\bexists\\b", QRegularExpression::CaseInsensitiveOption), "EXISTS");
  }
  
  return result;
}

QString SqlFormatter::indentStr(int level, const FormatOptions& options) {
  int spaces = level * options.indentSize;
  return QString(spaces, options.useTabs ? '\t' : ' ');
}

}  // namespace scratchrobin::ui
