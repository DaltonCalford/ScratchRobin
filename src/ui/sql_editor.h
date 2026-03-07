#pragma once
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QCompleter>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

class SqlEditor;
class SqlFormatter;

// ============================================================================
// SQL Syntax Highlighter
// ============================================================================
class SqlSyntaxHighlighter : public QSyntaxHighlighter {
  Q_OBJECT

 public:
  explicit SqlSyntaxHighlighter(QTextDocument* parent = nullptr);
  void setColorScheme(const QString& scheme);

 protected:
  void highlightBlock(const QString& text) override;

 private:
  void setupRules();
  
  struct HighlightingRule {
    QRegularExpression pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> rules_;
  
  QTextCharFormat keywordFormat_;
  QTextCharFormat stringFormat_;
  QTextCharFormat commentFormat_;
  QTextCharFormat numberFormat_;
  QTextCharFormat functionFormat_;
  QTextCharFormat operatorFormat_;
};

// ============================================================================
// Line Number Area
// ============================================================================
class LineNumberArea : public QWidget {
 public:
  explicit LineNumberArea(SqlEditor* editor);
  QSize sizeHint() const override;
 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
 private:
  SqlEditor* editor_;
};

// ============================================================================
// Code Folding Area
// ============================================================================
class CodeFoldingArea : public QWidget {
  Q_OBJECT
 public:
  explicit CodeFoldingArea(SqlEditor* editor);
  QSize sizeHint() const override;
 signals:
  void foldClicked(int line);
 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
 private:
  SqlEditor* editor_;
};

// ============================================================================
// SQL Editor (Enhanced with zoom, folding, formatting)
// ============================================================================
class SqlEditor : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit SqlEditor(backend::SessionClient* client, QWidget* parent = nullptr);
  ~SqlEditor() override;

  // Line numbers
  int lineNumberAreaWidth();
  void lineNumberAreaPaintEvent(QPaintEvent* event);
  
  // Code folding
  void setCodeFoldingEnabled(bool enabled);
  bool codeFoldingEnabled() const { return codeFoldingEnabled_; }
  void toggleFold(int blockNumber);
  bool isFolded(int blockNumber) const;
  void drawFoldingMarkers(QPaintEvent* event);
  bool isFoldable(const QTextBlock& block) const;
  bool isFoldStart(const QTextBlock& block) const;
  int findMatchingEnd(const QTextBlock& startBlock) const;

  // Zoom
  void zoomIn(int range = 1);
  void zoomOut(int range = 1);
  void resetZoom();
  int zoomLevel() const { return zoomLevel_; }

  // SQL Formatting
  void formatSql();
  void minifySql();
  
  // Editor utilities
  void toggleComment();
  void commentSelection();
  void uncommentSelection();
  void selectCurrentStatement();
  QString selectedSql() const;
  QString currentStatement() const;
  
  // Word wrap
  void setWordWrap(bool wrap);
  bool wordWrap() const;

  // Convenience methods for text access
  void setText(const QString& text) { setPlainText(text); }
  QString text() const { return toPlainText(); }
  
  void setKeywords(const QStringList& keywords);

 signals:
  void executeRequested();
  void selectionChanged();
  void zoomChanged(int level);
  void foldToggled(int blockNumber, bool folded);

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;

 private slots:
  void updateLineNumberAreaWidth(int newBlockCount);
  void highlightCurrentLine();
  void updateLineNumberArea(const QRect& rect, int dy);
  void insertCompletion(const QString& completion);
  void onFoldClicked(int line);

 private:
  void setupEditor();
  void setupCompleter();
  void setupFolding();
  QString textUnderCursor() const;
  void autoIndent();

  backend::SessionClient* session_client_;
  QWidget* lineNumberArea_;
  CodeFoldingArea* codeFoldingArea_;
  SqlSyntaxHighlighter* highlighter_;
  QCompleter* completer_;
  QStringListModel* completionModel_;
  
  // Folding state
  bool codeFoldingEnabled_ = true;
  QHash<int, bool> foldedBlocks_;
  
  // Zoom state
  int zoomLevel_ = 0;
  static constexpr int ZOOM_MIN = -10;
  static constexpr int ZOOM_MAX = 20;
  
  friend class LineNumberArea;
  friend class CodeFoldingArea;
};

// ============================================================================
// SQL Formatter Utility
// ============================================================================
class SqlFormatter {
 public:
  struct FormatOptions {
    bool uppercaseKeywords;
    bool compactLists;
    int indentSize;
    bool useTabs;
    int maxLineLength;
    bool alignColumns;
    
    FormatOptions() : uppercaseKeywords(true), compactLists(false), indentSize(4),
                      useTabs(false), maxLineLength(120), alignColumns(true) {}
  };

  static QString format(const QString& sql, const FormatOptions& options);
  static QString minify(const QString& sql);
  
 private:
  static QString formatSelect(const QString& sql, const FormatOptions& options);
  static QString formatInsert(const QString& sql, const FormatOptions& options);
  static QString formatUpdate(const QString& sql, const FormatOptions& options);
  static QString formatDelete(const QString& sql, const FormatOptions& options);
  static QString formatCreate(const QString& sql, const FormatOptions& options);
  
  static QString indentStr(int level, const FormatOptions& options);
};

}  // namespace scratchrobin::ui
