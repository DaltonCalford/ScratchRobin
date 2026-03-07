#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Find/Replace dialog for SQL editor
 * 
 * Implements FORM_SPECIFICATION.md Find/Replace section:
 * - Find text with options
 * - Replace text
 * - Replace All
 * - Case sensitive, whole word, regex options
 */
class FindReplaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit FindReplaceDialog(QWidget* parent = nullptr);
    ~FindReplaceDialog() override;

    void setTextEdit(QPlainTextEdit* textEdit);
    void setFindText(const QString& text);

signals:
    void findNext(const QString& text, bool caseSensitive, bool wholeWord, bool regex);
    void findPrevious(const QString& text, bool caseSensitive, bool wholeWord, bool regex);
    void replace(const QString& findText, const QString& replaceText);
    void replaceAll(const QString& findText, const QString& replaceText);

private slots:
    void onFindNext();
    void onFindPrevious();
    void onReplace();
    void onReplaceAll();
    void onClose();
    void onFindTextChanged();
    void onOptionsChanged();

private:
    void setupUi();
    void updateButtonStates();
    bool findText(bool forward);

    QPlainTextEdit* textEdit_ = nullptr;

    // UI elements
    QLineEdit* findEdit_ = nullptr;
    QLineEdit* replaceEdit_ = nullptr;
    QPushButton* findNextBtn_ = nullptr;
    QPushButton* findPrevBtn_ = nullptr;
    QPushButton* replaceBtn_ = nullptr;
    QPushButton* replaceAllBtn_ = nullptr;
    QPushButton* closeBtn_ = nullptr;
    
    // Options
    QCheckBox* caseSensitiveCheck_ = nullptr;
    QCheckBox* wholeWordCheck_ = nullptr;
    QCheckBox* regexCheck_ = nullptr;
    
    // Status
    QLabel* statusLabel_ = nullptr;
};

} // namespace scratchrobin::ui
