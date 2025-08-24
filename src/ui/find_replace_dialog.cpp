#include "find_replace_dialog.h"
#include <QDialogButtonBox>
#include <QSplitter>
#include <QHeaderView>
#include <QScrollArea>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <QStandardPaths>

namespace scratchrobin {

FindReplaceDialog::FindReplaceDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    setWindowTitle("Find & Replace");
    setMinimumSize(600, 500);
    resize(700, 600);

    loadSettings();
    updateButtonStates();
}

void FindReplaceDialog::setSearchText(const QString& text) {
    findTextEdit_->setText(text);
    addSearchHistory(text);
}

void FindReplaceDialog::setReplaceText(const QString& text) {
    replaceTextEdit_->setText(text);
    addReplaceHistory(text);
}

void FindReplaceDialog::setCurrentDocumentText(const QString& text, const QString& fileName) {
    currentDocumentText_ = text;
    currentDocumentName_ = fileName;
}

void FindReplaceDialog::addSearchHistory(const QString& searchText) {
    if (searchText.isEmpty() || findHistory_.contains(searchText)) {
        return;
    }

    findHistory_.prepend(searchText);
    while (findHistory_.size() > 20) { // Keep last 20 searches
        findHistory_.removeLast();
    }

    // Update history list
    findHistoryList_->clear();
    for (const QString& item : findHistory_) {
        findHistoryList_->addItem(item);
    }
}

void FindReplaceDialog::addReplaceHistory(const QString& replaceText) {
    if (replaceText.isEmpty() || replaceHistory_.contains(replaceText)) {
        return;
    }

    replaceHistory_.prepend(replaceText);
    while (replaceHistory_.size() > 20) { // Keep last 20 replacements
        replaceHistory_.removeLast();
    }

    // Update history list
    replaceHistoryList_->clear();
    for (const QString& item : replaceHistory_) {
        replaceHistoryList_->addItem(item);
    }
}

void FindReplaceDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Find & Replace");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c5aa0;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Tab widget
    tabWidget_ = new QTabWidget();
    setupFindTab();
    setupReplaceTab();
    setupAdvancedTab();
    setupResultsTab();

    connect(tabWidget_, &QTabWidget::currentChanged, this, &FindReplaceDialog::onTabChanged);
    mainLayout->addWidget(tabWidget_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* helpButton = new QPushButton("Help");
    helpButton->setIcon(QIcon(":/icons/help.png"));
    buttonLayout->addWidget(helpButton);

    closeButton_ = new QPushButton("Close");
    closeButton_->setDefault(true);
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton_);

    mainLayout->addLayout(buttonLayout);
}

void FindReplaceDialog::setupFindTab() {
    findTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(findTab_);

    // Search controls
    QGroupBox* searchGroup = new QGroupBox("Find");
    QFormLayout* searchLayout = new QFormLayout(searchGroup);

    // Find text input
    QHBoxLayout* findLayout = new QHBoxLayout();
    findTextEdit_ = new QLineEdit();
    findTextEdit_->setPlaceholderText("Enter text to find...");
    connect(findTextEdit_, &QLineEdit::textChanged, this, &FindReplaceDialog::onSearchTextChanged);
    findLayout->addWidget(findTextEdit_);

    QPushButton* findHistoryButton = new QPushButton("▼");
    findHistoryButton->setMaximumWidth(30);
    findHistoryButton->setToolTip("Search history");
    connect(findHistoryButton, &QPushButton::clicked, [this]() {
        if (findHistoryList_->isVisible()) {
            findHistoryList_->hide();
        } else {
            findHistoryList_->show();
            findHistoryList_->raise();
        }
    });
    findLayout->addWidget(findHistoryButton);

    searchLayout->addRow("Find:", findLayout);

    // Search scope
    findScopeCombo_ = new QComboBox();
    findScopeCombo_->addItems({"Current Document", "All Open Documents", "Selection", "Project Files"});
    connect(findScopeCombo_, &QComboBox::currentTextChanged, this, &FindReplaceDialog::onScopeChanged);
    searchLayout->addRow("Search in:", findScopeCombo_);

    layout->addWidget(searchGroup);

    // Options
    setupOptionsGroup();
    layout->addWidget(optionsGroup_);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    findButton_ = new QPushButton("Find Next");
    findButton_->setIcon(QIcon(":/icons/find.png"));
    findButton_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #1976D2; }");
    connect(findButton_, &QPushButton::clicked, this, &FindReplaceDialog::onFindClicked);
    buttonLayout->addWidget(findButton_);

    findAllButton_ = new QPushButton("Find All");
    findAllButton_->setIcon(QIcon(":/icons/find_all.png"));
    connect(findAllButton_, &QPushButton::clicked, this, &FindReplaceDialog::onFindAllClicked);
    buttonLayout->addWidget(findAllButton_);

    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    // History list (initially hidden)
    findHistoryList_ = new QListWidget();
    findHistoryList_->setMaximumHeight(100);
    findHistoryList_->hide();
    connect(findHistoryList_, &QListWidget::itemClicked, this, &FindReplaceDialog::onHistoryItemClicked);
    layout->addWidget(findHistoryList_);

    tabWidget_->addTab(findTab_, "Find");
}

void FindReplaceDialog::setupReplaceTab() {
    replaceTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(replaceTab_);

    // Replace controls
    QGroupBox* replaceGroup = new QGroupBox("Replace");
    QFormLayout* replaceLayout = new QFormLayout(replaceGroup);

    // Replace text input
    QHBoxLayout* replaceLayout_ = new QHBoxLayout();
    replaceTextEdit_ = new QLineEdit();
    replaceTextEdit_->setPlaceholderText("Enter replacement text...");
    connect(replaceTextEdit_, &QLineEdit::textChanged, this, &FindReplaceDialog::onReplaceTextChanged);
    replaceLayout_->addWidget(replaceTextEdit_);

    QPushButton* replaceHistoryButton = new QPushButton("▼");
    replaceHistoryButton->setMaximumWidth(30);
    replaceHistoryButton->setToolTip("Replace history");
    connect(replaceHistoryButton, &QPushButton::clicked, [this]() {
        if (replaceHistoryList_->isVisible()) {
            replaceHistoryList_->hide();
        } else {
            replaceHistoryList_->show();
            replaceHistoryList_->raise();
        }
    });
    replaceLayout_->addWidget(replaceHistoryButton);

    replaceLayout->addRow("Replace with:", replaceLayout_);

    layout->addWidget(replaceGroup);

    // Options (reuse from find tab)
    if (optionsGroup_) {
        layout->addWidget(optionsGroup_);
    }

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    replaceButton_ = new QPushButton("Replace");
    replaceButton_->setIcon(QIcon(":/icons/replace.png"));
    replaceButton_->setStyleSheet("QPushButton { background-color: #FF9800; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #F57C00; }");
    connect(replaceButton_, &QPushButton::clicked, this, &FindReplaceDialog::onReplaceClicked);
    buttonLayout->addWidget(replaceButton_);

    replaceAllButton_ = new QPushButton("Replace All");
    replaceAllButton_->setIcon(QIcon(":/icons/replace_all.png"));
    connect(replaceAllButton_, &QPushButton::clicked, this, &FindReplaceDialog::onReplaceAllClicked);
    buttonLayout->addWidget(replaceAllButton_);

    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    // Replace history list (initially hidden)
    replaceHistoryList_ = new QListWidget();
    replaceHistoryList_->setMaximumHeight(100);
    replaceHistoryList_->hide();
    connect(replaceHistoryList_, &QListWidget::itemClicked, this, &FindReplaceDialog::onHistoryItemClicked);
    layout->addWidget(replaceHistoryList_);

    tabWidget_->addTab(replaceTab_, "Replace");
}

void FindReplaceDialog::setupOptionsGroup() {
    optionsGroup_ = new QGroupBox("Search Options");

    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup_);

    // Basic options
    QHBoxLayout* basicRow = new QHBoxLayout();

    caseSensitiveCheck_ = new QCheckBox("Case sensitive");
    connect(caseSensitiveCheck_, &QCheckBox::toggled, this, &FindReplaceDialog::onOptionsChanged);
    basicRow->addWidget(caseSensitiveCheck_);

    wholeWordsCheck_ = new QCheckBox("Whole words only");
    connect(wholeWordsCheck_, &QCheckBox::toggled, this, &FindReplaceDialog::onOptionsChanged);
    basicRow->addWidget(wholeWordsCheck_);

    regexCheck_ = new QCheckBox("Regular expression");
    connect(regexCheck_, &QCheckBox::toggled, this, &FindReplaceDialog::onOptionsChanged);
    basicRow->addWidget(regexCheck_);

    optionsLayout->addLayout(basicRow);

    // Advanced options
    QHBoxLayout* advancedRow = new QHBoxLayout();

    wrapAroundCheck_ = new QCheckBox("Wrap around");
    wrapAroundCheck_->setChecked(true);
    connect(wrapAroundCheck_, &QCheckBox::toggled, this, &FindReplaceDialog::onOptionsChanged);
    advancedRow->addWidget(wrapAroundCheck_);

    highlightAllCheck_ = new QCheckBox("Highlight all matches");
    highlightAllCheck_->setChecked(true);
    connect(highlightAllCheck_, &QCheckBox::toggled, this, &FindReplaceDialog::onOptionsChanged);
    advancedRow->addWidget(highlightAllCheck_);

    incrementalCheck_ = new QCheckBox("Incremental search");
    incrementalCheck_->setChecked(true);
    connect(incrementalCheck_, &QCheckBox::toggled, this, &FindReplaceDialog::onOptionsChanged);
    advancedRow->addWidget(incrementalCheck_);

    optionsLayout->addLayout(advancedRow);

    // Search direction
    QHBoxLayout* directionRow = new QHBoxLayout();
    directionRow->addWidget(new QLabel("Direction:"));

    searchDirectionCombo_ = new QComboBox();
    searchDirectionCombo_->addItems({"Forward", "Backward"});
    connect(searchDirectionCombo_, &QComboBox::currentTextChanged, this, &FindReplaceDialog::onOptionsChanged);
    directionRow->addWidget(searchDirectionCombo_);

    directionRow->addStretch();
    optionsLayout->addLayout(directionRow);
}

void FindReplaceDialog::setupAdvancedTab() {
    QWidget* advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(advancedTab);

    QGroupBox* performanceGroup = new QGroupBox("Performance");
    QFormLayout* perfLayout = new QFormLayout(performanceGroup);

    maxResultsSpin_ = new QSpinBox();
    maxResultsSpin_->setRange(1, 10000);
    maxResultsSpin_->setValue(1000);
    perfLayout->addRow("Maximum results:", maxResultsSpin_);

    layout->addWidget(performanceGroup);

    QGroupBox* shortcutsGroup = new QGroupBox("Keyboard Shortcuts");
    QVBoxLayout* shortcutsLayout = new QVBoxLayout(shortcutsGroup);

    QLabel* shortcutsLabel = new QLabel(
        "<b>Find:</b> Ctrl+F<br>"
        "<b>Replace:</b> Ctrl+H<br>"
        "<b>Find Next:</b> F3<br>"
        "<b>Find Previous:</b> Shift+F3<br>"
        "<b>Find All:</b> Ctrl+Shift+F"
    );
    shortcutsLabel->setTextFormat(Qt::RichText);
    shortcutsLayout->addWidget(shortcutsLabel);

    layout->addWidget(shortcutsGroup);
    layout->addStretch();

    tabWidget_->addTab(advancedTab, "Advanced");
}

void FindReplaceDialog::setupResultsTab() {
    resultsTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(resultsTab_);

    // Results count
    QHBoxLayout* countLayout = new QHBoxLayout();
    resultsCountLabel_ = new QLabel("No results found");
    resultsCountLabel_->setStyleSheet("font-weight: bold; color: #666;");
    countLayout->addWidget(resultsCountLabel_);

    clearResultsButton_ = new QPushButton("Clear Results");
    connect(clearResultsButton_, &QPushButton::clicked, [this]() {
        resultsTable_->setRowCount(0);
        resultsCountLabel_->setText("No results found");
        currentResults_.clear();
    });
    countLayout->addWidget(clearResultsButton_);

    countLayout->addStretch();
    layout->addLayout(countLayout);

    // Results table
    resultsTable_ = new QTableWidget();
    resultsTable_->setColumnCount(4);
    resultsTable_->setHorizontalHeaderLabels({"File", "Line", "Column", "Context"});
    resultsTable_->horizontalHeader()->setStretchLastSection(true);
    resultsTable_->verticalHeader()->setVisible(false);
    resultsTable_->setAlternatingRowColors(true);
    resultsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(resultsTable_);

    tabWidget_->addTab(resultsTab_, "Results");
}

void FindReplaceDialog::onFindClicked() {
    if (findTextEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Find Error", "Please enter text to find.");
        return;
    }

    currentOptions_.searchText = findTextEdit_->text();
    currentOptions_.caseSensitive = caseSensitiveCheck_->isChecked();
    currentOptions_.wholeWords = wholeWordsCheck_->isChecked();
    currentOptions_.regularExpression = regexCheck_->isChecked();
    currentOptions_.wrapAround = wrapAroundCheck_->isChecked();
    currentOptions_.searchBackwards = searchDirectionCombo_->currentText() == "Backward";
    currentOptions_.searchScope = findScopeCombo_->currentText();
    currentOptions_.highlightAll = highlightAllCheck_->isChecked();
    currentOptions_.incrementalSearch = incrementalCheck_->isChecked();
    currentOptions_.maxResults = maxResultsSpin_->value();

    emit findRequested(currentOptions_);
}

void FindReplaceDialog::onReplaceClicked() {
    if (findTextEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Replace Error", "Please enter text to find.");
        return;
    }

    currentOptions_.searchText = findTextEdit_->text();
    currentOptions_.replaceText = replaceTextEdit_->text();
    currentOptions_.caseSensitive = caseSensitiveCheck_->isChecked();
    currentOptions_.wholeWords = wholeWordsCheck_->isChecked();
    currentOptions_.regularExpression = regexCheck_->isChecked();
    currentOptions_.wrapAround = wrapAroundCheck_->isChecked();
    currentOptions_.searchBackwards = searchDirectionCombo_->currentText() == "Backward";
    currentOptions_.searchScope = findScopeCombo_->currentText();

    emit replaceRequested(currentOptions_);
}

void FindReplaceDialog::onReplaceAllClicked() {
    if (findTextEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Replace All Error", "Please enter text to find.");
        return;
    }

    currentOptions_.searchText = findTextEdit_->text();
    currentOptions_.replaceText = replaceTextEdit_->text();
    currentOptions_.caseSensitive = caseSensitiveCheck_->isChecked();
    currentOptions_.wholeWords = wholeWordsCheck_->isChecked();
    currentOptions_.regularExpression = regexCheck_->isChecked();
    currentOptions_.searchScope = findScopeCombo_->currentText();

    emit replaceAllRequested(currentOptions_);
}

void FindReplaceDialog::onFindAllClicked() {
    if (findTextEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Find All Error", "Please enter text to find.");
        return;
    }

    currentOptions_.searchText = findTextEdit_->text();
    currentOptions_.caseSensitive = caseSensitiveCheck_->isChecked();
    currentOptions_.wholeWords = wholeWordsCheck_->isChecked();
    currentOptions_.regularExpression = regexCheck_->isChecked();
    currentOptions_.searchScope = findScopeCombo_->currentText();
    currentOptions_.maxResults = maxResultsSpin_->value();

    emit findAllRequested(currentOptions_);
}

void FindReplaceDialog::onSearchTextChanged(const QString& text) {
    addSearchHistory(text);
    updateButtonStates();

    if (incrementalCheck_->isChecked() && !text.isEmpty()) {
        // Perform incremental search
        QTimer::singleShot(300, [this, text]() {
            if (findTextEdit_->text() == text) {
                currentOptions_.searchText = text;
                currentOptions_.incrementalSearch = true;
                emit findRequested(currentOptions_);
            }
        });
    }
}

void FindReplaceDialog::onReplaceTextChanged(const QString& text) {
    addReplaceHistory(text);
    updateButtonStates();
}

void FindReplaceDialog::onOptionsChanged() {
    validateRegex();
}

void FindReplaceDialog::onScopeChanged() {
    // Update UI based on scope selection
    QString scope = findScopeCombo_->currentText();
    if (scope == "Selection") {
        wrapAroundCheck_->setChecked(false);
        wrapAroundCheck_->setEnabled(false);
    } else {
        wrapAroundCheck_->setEnabled(true);
    }
}

void FindReplaceDialog::onHistoryItemClicked(QListWidgetItem* item) {
    if (sender() == findHistoryList_) {
        findTextEdit_->setText(item->text());
    } else if (sender() == replaceHistoryList_) {
        replaceTextEdit_->setText(item->text());
    }

    // Hide the history list
    item->listWidget()->hide();
}

void FindReplaceDialog::onTabChanged(int index) {
    updateButtonStates();
}

void FindReplaceDialog::updateButtonStates() {
    bool hasFindText = !findTextEdit_->text().isEmpty();
    bool hasReplaceText = !replaceTextEdit_->text().isEmpty();

    findButton_->setEnabled(hasFindText);
    findAllButton_->setEnabled(hasFindText);

    replaceButton_->setEnabled(hasFindText);
    replaceAllButton_->setEnabled(hasFindText && hasReplaceText);
}

void FindReplaceDialog::validateRegex() {
    if (!regexCheck_->isChecked()) {
        return;
    }

    QString pattern = findTextEdit_->text();
    if (pattern.isEmpty()) {
        return;
    }

    QRegularExpression regex(pattern);
    if (!regex.isValid()) {
        QMessageBox::warning(this, "Invalid Regular Expression",
                           QString("The regular expression is invalid:\n%1")
                           .arg(regex.errorString()));
    }
}

void FindReplaceDialog::loadSettings() {
    QSettings settings("ScratchRobin", "FindReplace");

    // Load search history
    findHistory_ = settings.value("findHistory").toStringList();
    replaceHistory_ = settings.value("replaceHistory").toStringList();

    // Load options
    caseSensitiveCheck_->setChecked(settings.value("caseSensitive", false).toBool());
    wholeWordsCheck_->setChecked(settings.value("wholeWords", false).toBool());
    regexCheck_->setChecked(settings.value("regex", false).toBool());
    wrapAroundCheck_->setChecked(settings.value("wrapAround", true).toBool());
    highlightAllCheck_->setChecked(settings.value("highlightAll", true).toBool());
    incrementalCheck_->setChecked(settings.value("incremental", true).toBool());

    // Load max results
    maxResultsSpin_->setValue(settings.value("maxResults", 1000).toInt());

    // Update history lists
    findHistoryList_->clear();
    for (const QString& item : findHistory_) {
        findHistoryList_->addItem(item);
    }

    replaceHistoryList_->clear();
    for (const QString& item : replaceHistory_) {
        replaceHistoryList_->addItem(item);
    }
}

void FindReplaceDialog::saveSettings() {
    QSettings settings("ScratchRobin", "FindReplace");

    settings.setValue("findHistory", findHistory_);
    settings.setValue("replaceHistory", replaceHistory_);
    settings.setValue("caseSensitive", caseSensitiveCheck_->isChecked());
    settings.setValue("wholeWords", wholeWordsCheck_->isChecked());
    settings.setValue("regex", regexCheck_->isChecked());
    settings.setValue("wrapAround", wrapAroundCheck_->isChecked());
    settings.setValue("highlightAll", highlightAllCheck_->isChecked());
    settings.setValue("incremental", incrementalCheck_->isChecked());
    settings.setValue("maxResults", maxResultsSpin_->value());
}

} // namespace scratchrobin
