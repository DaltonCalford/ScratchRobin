#ifndef SCRATCHROBIN_TEXT_EDITOR_H
#define SCRATCHROBIN_TEXT_EDITOR_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <QWidget>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QSyntaxHighlighter>
#include <QCompleter>
#include <QTimer>
#include <QDateTime>
#include <QMenu>
#include <QAction>

namespace scratchrobin {

class IMetadataManager;
class ISearchEngine;
class IConnectionManager;

enum class EditorMode {
    SQL,
    TEXT,
    JSON,
    XML,
    CSV,
    LOG,
    CUSTOM
};

enum class TextEncoding {
    UTF8,
    UTF16,
    UTF32,
    LATIN1,
    ASCII,
    SYSTEM
};

enum class LineEnding {
    UNIX,    // \n
    WINDOWS, // \r\n
    MAC      // \r
};

enum class IndentationMode {
    SPACES,
    TABS,
    SMART
};

struct EditorPosition {
    int line = 0;
    int column = 0;
    int absolutePosition = 0;
};

struct TextSelection {
    EditorPosition start;
    EditorPosition end;
    QString selectedText;
    bool hasSelection = false;
};

struct DocumentInfo {
    std::string filePath;
    std::string title;
    bool isModified = false;
    bool isNew = true;
    EditorMode mode = EditorMode::SQL;
    TextEncoding encoding = TextEncoding::UTF8;
    LineEnding lineEnding = LineEnding::UNIX;
    IndentationMode indentationMode = IndentationMode::SPACES;
    int tabWidth = 4;
    int indentWidth = 4;
    QDateTime createdAt;
    QDateTime modifiedAt;
    QDateTime lastSavedAt;
    std::unordered_map<std::string, std::string> metadata;
};

struct EditorConfiguration {
    EditorMode defaultMode = EditorMode::SQL;
    TextEncoding defaultEncoding = TextEncoding::UTF8;
    LineEnding defaultLineEnding = LineEnding::UNIX;
    IndentationMode defaultIndentation = IndentationMode::SPACES;
    int defaultTabWidth = 4;
    int defaultIndentWidth = 4;
    bool enableSyntaxHighlighting = true;
    bool enableCodeCompletion = true;
    bool enableAutoIndentation = true;
    bool enableLineNumbers = true;
    bool enableWordWrap = false;
    bool enableAutoSave = false;
    int autoSaveIntervalSeconds = 300;
    bool enableBracketMatching = true;
    bool enableCurrentLineHighlighting = true;
    bool enableWhitespaceVisualization = false;
    int maxUndoSteps = 100;
    QString fontFamily = "Monaco";
    int fontSize = 12;
    QColor backgroundColor = QColor(255, 255, 255);
    QColor foregroundColor = QColor(0, 0, 0);
    QColor selectionColor = QColor(173, 214, 255);
    QColor currentLineColor = QColor(245, 245, 245);
    QColor lineNumberColor = QColor(128, 128, 128);
    QColor bracketMatchColor = QColor(255, 255, 0);
    std::unordered_map<std::string, QColor> syntaxColors;
};

class ITextEditor {
public:
    virtual ~ITextEditor() = default;

    virtual void initialize(const EditorConfiguration& config) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;

    virtual bool loadFile(const std::string& filePath) = 0;
    virtual bool saveFile(const std::string& filePath = "") = 0;
    virtual bool saveAs(const std::string& filePath) = 0;
    virtual bool newDocument() = 0;
    virtual bool closeDocument() = 0;

    virtual QString getText() const = 0;
    virtual void setText(const QString& text) = 0;
    virtual QString getSelectedText() const = 0;
    virtual void setSelectedText(const QString& text) = 0;

    virtual EditorPosition getCursorPosition() const = 0;
    virtual void setCursorPosition(const EditorPosition& position) = 0;
    virtual void setCursorPosition(int line, int column) = 0;
    virtual TextSelection getSelection() const = 0;
    virtual void setSelection(const TextSelection& selection) = 0;

    virtual void insertText(const QString& text) = 0;
    virtual void insertTextAt(const QString& text, const EditorPosition& position) = 0;
    virtual void replaceText(const QString& oldText, const QString& newText) = 0;
    virtual void replaceSelection(const QString& text) = 0;
    virtual void deleteText(const EditorPosition& start, const EditorPosition& end) = 0;

    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;
    virtual void clearUndoRedoHistory() = 0;

    virtual void cut() = 0;
    virtual void copy() = 0;
    virtual void paste() = 0;
    virtual void selectAll() = 0;

    virtual void findText(const QString& text, bool caseSensitive = false, bool wholeWords = false, bool regex = false) = 0;
    virtual void replaceText(const QString& findText, const QString& replaceText, bool caseSensitive = false, bool wholeWords = false, bool regex = false) = 0;
    virtual void findNext() = 0;
    virtual void findPrevious() = 0;

    virtual void gotoLine(int lineNumber) = 0;
    virtual void gotoPosition(int position) = 0;

    virtual void indent() = 0;
    virtual void unindent() = 0;
    virtual void commentLine() = 0;
    virtual void uncommentLine() = 0;
    virtual void duplicateLine() = 0;
    virtual void deleteLine() = 0;

    virtual void toUpperCase() = 0;
    virtual void toLowerCase() = 0;
    virtual void toTitleCase() = 0;

    virtual DocumentInfo getDocumentInfo() const = 0;
    virtual void setDocumentInfo(const DocumentInfo& info) = 0;
    virtual EditorConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const EditorConfiguration& config) = 0;

    virtual void setEditorMode(EditorMode mode) = 0;
    virtual EditorMode getEditorMode() const = 0;
    virtual void setTextEncoding(TextEncoding encoding) = 0;
    virtual TextEncoding getTextEncoding() const = 0;

    virtual int getLineCount() const = 0;
    virtual int getWordCount() const = 0;
    virtual int getCharacterCount() const = 0;
    virtual int getSelectedCharacterCount() const = 0;

    virtual bool isModified() const = 0;
    virtual void setModified(bool modified) = 0;

    using TextChangedCallback = std::function<void()>;
    using CursorPositionChangedCallback = std::function<void(const EditorPosition&)>;
    using SelectionChangedCallback = std::function<void(const TextSelection&)>;
    using DocumentModifiedCallback = std::function<void(bool)>;

    virtual void setTextChangedCallback(TextChangedCallback callback) = 0;
    virtual void setCursorPositionChangedCallback(CursorPositionChangedCallback callback) = 0;
    virtual void setSelectionChangedCallback(SelectionChangedCallback callback) = 0;
    virtual void setDocumentModifiedCallback(DocumentModifiedCallback callback) = 0;

    virtual QWidget* getWidget() = 0;
    virtual QTextDocument* getDocument() = 0;
    virtual QPlainTextEdit* getTextEdit() = 0;
};

class TextEditor : public QWidget, public ITextEditor {
    Q_OBJECT

public:
    TextEditor(QWidget* parent = nullptr);
    ~TextEditor() override;

    // ITextEditor implementation
    void initialize(const EditorConfiguration& config) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;

    bool loadFile(const std::string& filePath) override;
    bool saveFile(const std::string& filePath = "") override;
    bool saveAs(const std::string& filePath) override;
    bool newDocument() override;
    bool closeDocument() override;

    QString getText() const override;
    void setText(const QString& text) override;
    QString getSelectedText() const override;
    void setSelectedText(const QString& text) override;

    EditorPosition getCursorPosition() const override;
    void setCursorPosition(const EditorPosition& position) override;
    void setCursorPosition(int line, int column) override;
    TextSelection getSelection() const override;
    void setSelection(const TextSelection& selection) override;

    void insertText(const QString& text) override;
    void insertTextAt(const QString& text, const EditorPosition& position) override;
    void replaceText(const QString& oldText, const QString& newText) override;
    void replaceSelection(const QString& text) override;
    void deleteText(const EditorPosition& start, const EditorPosition& end) override;

    void undo() override;
    void redo() override;
    bool canUndo() const override;
    bool canRedo() const override;
    void clearUndoRedoHistory() override;

    void cut() override;
    void copy() override;
    void paste() override;
    void selectAll() override;

    void findText(const QString& text, bool caseSensitive = false, bool wholeWords = false, bool regex = false) override;
    void replaceText(const QString& findText, const QString& replaceText, bool caseSensitive = false, bool wholeWords = false, bool regex = false) override;
    void findNext() override;
    void findPrevious() override;

    void gotoLine(int lineNumber) override;
    void gotoPosition(int position) override;

    void indent() override;
    void unindent() override;
    void commentLine() override;
    void uncommentLine() override;
    void duplicateLine() override;
    void deleteLine() override;

    void toUpperCase() override;
    void toLowerCase() override;
    void toTitleCase() override;

    DocumentInfo getDocumentInfo() const override;
    void setDocumentInfo(const DocumentInfo& info) override;
    EditorConfiguration getConfiguration() const override;
    void updateConfiguration(const EditorConfiguration& config) override;

    void setEditorMode(EditorMode mode) override;
    EditorMode getEditorMode() const override;
    void setTextEncoding(TextEncoding encoding) override;
    TextEncoding getTextEncoding() const override;

    int getLineCount() const override;
    int getWordCount() const override;
    int getCharacterCount() const override;
    int getSelectedCharacterCount() const override;

    bool isModified() const override;
    void setModified(bool modified) override;

    void setTextChangedCallback(TextChangedCallback callback) override;
    void setCursorPositionChangedCallback(CursorPositionChangedCallback callback) override;
    void setSelectionChangedCallback(SelectionChangedCallback callback) override;
    void setDocumentModifiedCallback(DocumentModifiedCallback callback) override;

    QWidget* getWidget() override;
    QTextDocument* getDocument() override;
    QPlainTextEdit* getTextEdit() override;

signals:
    void documentModified(bool modified);
    void textChanged();
    void cursorPositionChanged(int line, int column);
    void selectionChanged();
    void findResultFound(bool found);
    void replaceResultFound(int count);

private slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void onSelectionChanged();
    void onDocumentModified(bool modified);
    void onAutoSaveTimer();
    void onContextMenuRequested(const QPoint& pos);
    void onCompletionActivated(const QModelIndex& index);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // UI Components
    QPlainTextEdit* textEdit_;
    QCompleter* completer_;
    QSyntaxHighlighter* syntaxHighlighter_;

    // Configuration and state
    EditorConfiguration config_;
    DocumentInfo documentInfo_;
    EditorPosition currentPosition_;
    TextSelection currentSelection_;

    // Timers
    QTimer* autoSaveTimer_;

    // Callbacks
    TextChangedCallback textChangedCallback_;
    CursorPositionChangedCallback cursorPositionChangedCallback_;
    SelectionChangedCallback selectionChangedCallback_;
    DocumentModifiedCallback documentModifiedCallback_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISearchEngine> searchEngine_;
    std::shared_ptr<IConnectionManager> connectionManager_;

    // Internal methods
    void setupUI();
    void setupCompleter();
    void setupConnections();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TEXT_EDITOR_H
