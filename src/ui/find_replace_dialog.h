#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QListWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>

namespace scratchrobin {

struct FindOptions {
    QString searchText;
    QString replaceText;
    bool caseSensitive = false;
    bool wholeWords = false;
    bool regularExpression = false;
    bool wrapAround = true;
    bool searchBackwards = false;
    QString searchScope = "Current Document"; // Current Document, All Open Documents, Selection
    bool highlightAll = true;
    bool incrementalSearch = true;
    int maxResults = 1000;
};

struct FindReplaceResult {
    QString fileName;
    int lineNumber;
    int columnNumber;
    QString context;
    QString fullText;
    int matchStart;
    int matchLength;
};

class FindReplaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit FindReplaceDialog(QWidget* parent = nullptr);
    ~FindReplaceDialog() override = default;

    void setSearchText(const QString& text);
    void setReplaceText(const QString& text);
    void setCurrentDocumentText(const QString& text, const QString& fileName = "Current Query");
    void addSearchHistory(const QString& searchText);
    void addReplaceHistory(const QString& replaceText);

signals:
    void findRequested(const FindOptions& options);
    void replaceRequested(const FindOptions& options);
    void replaceAllRequested(const FindOptions& options);
    void findAllRequested(const FindOptions& options);
    void searchResultsReady(const QList<FindReplaceResult>& results);

private slots:
    void onFindClicked();
    void onReplaceClicked();
    void onReplaceAllClicked();
    void onFindAllClicked();
    void onSearchTextChanged(const QString& text);
    void onReplaceTextChanged(const QString& text);
    void onOptionsChanged();
    void onScopeChanged();
    void onHistoryItemClicked(QListWidgetItem* item);
    void onTabChanged(int index);

private:
    void setupUI();
    void setupFindTab();
    void setupReplaceTab();
    void setupAdvancedTab();
    void setupResultsTab();
    void setupOptionsGroup();

    void loadSettings();
    void saveSettings();
    void updateButtonStates();
    void validateRegex();

    // UI Components
    QTabWidget* tabWidget_;

    // Find tab
    QWidget* findTab_;
    QLineEdit* findTextEdit_;
    QComboBox* findScopeCombo_;
    QPushButton* findButton_;
    QPushButton* findAllButton_;
    QPushButton* closeButton_;

    // Replace tab
    QWidget* replaceTab_;
    QLineEdit* replaceTextEdit_;
    QPushButton* replaceButton_;
    QPushButton* replaceAllButton_;

    // Options
    QGroupBox* optionsGroup_;
    QCheckBox* caseSensitiveCheck_;
    QCheckBox* wholeWordsCheck_;
    QCheckBox* regexCheck_;
    QCheckBox* wrapAroundCheck_;
    QCheckBox* highlightAllCheck_;
    QCheckBox* incrementalCheck_;

    // Advanced options
    QGroupBox* advancedGroup_;
    QSpinBox* maxResultsSpin_;
    QComboBox* searchDirectionCombo_;

    // History
    QListWidget* findHistoryList_;
    QListWidget* replaceHistoryList_;

    // Results tab
    QWidget* resultsTab_;
    QTableWidget* resultsTable_;
    QLabel* resultsCountLabel_;
    QPushButton* clearResultsButton_;

public:
    // Public access methods
    QLineEdit* getFindTextEdit() const { return findTextEdit_; }
    QLineEdit* getReplaceTextEdit() const { return replaceTextEdit_; }

    // Current state
    QString currentDocumentText_;
    QString currentDocumentName_;
    QList<FindReplaceResult> currentResults_;
    FindOptions currentOptions_;

    // Settings
    QStringList findHistory_;
    QStringList replaceHistory_;
    QStringList recentSearches_;
    QStringList recentReplacements_;
};

} // namespace scratchrobin
