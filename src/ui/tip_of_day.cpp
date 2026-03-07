#include "ui/tip_of_day.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSettings>
#include <QRandomGenerator>

namespace scratchrobin::ui {

TipOfDayDialog::TipOfDayDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Tip of the Day"));
    setMinimumSize(450, 300);
    resize(500, 350);
    
    loadTips();
    setupUi();
    
    // Show random tip
    if (!tips_.isEmpty()) {
        currentTipIndex_ = QRandomGenerator::global()->bounded(tips_.size());
        loadTip(currentTipIndex_);
    }
}

TipOfDayDialog::~TipOfDayDialog() = default;

void TipOfDayDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header with icon and title
    auto* headerLayout = new QHBoxLayout();
    
    iconLabel_ = new QLabel(this);
    iconLabel_->setText("💡");
    iconLabel_->setStyleSheet("QLabel { font-size: 48px; }");
    headerLayout->addWidget(iconLabel_);
    
    titleLabel_ = new QLabel(this);
    titleLabel_->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; }");
    headerLayout->addWidget(titleLabel_, 1);
    headerLayout->addStretch();
    
    mainLayout->addLayout(headerLayout);
    
    // Tip content
    contentLabel_ = new QLabel(this);
    contentLabel_->setWordWrap(true);
    contentLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    contentLabel_->setStyleSheet("QLabel { padding: 10px; background: palette(alternate-base); border-radius: 5px; }");
    contentLabel_->setMinimumHeight(150);
    mainLayout->addWidget(contentLabel_, 1);
    
    // Show on startup checkbox
    showOnStartupCheck_ = new QCheckBox(tr("Show tips on startup"), this);
    showOnStartupCheck_->setChecked(shouldShowOnStartup());
    mainLayout->addWidget(showOnStartupCheck_);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    previousBtn_ = new QPushButton(tr("← Previous Tip"), this);
    buttonLayout->addWidget(previousBtn_);
    
    nextBtn_ = new QPushButton(tr("Next Tip →"), this);
    buttonLayout->addWidget(nextBtn_);
    
    buttonLayout->addStretch();
    
    closeBtn_ = new QPushButton(tr("Close"), this);
    closeBtn_->setDefault(true);
    buttonLayout->addWidget(closeBtn_);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(previousBtn_, &QPushButton::clicked, this, &TipOfDayDialog::onPreviousTip);
    connect(nextBtn_, &QPushButton::clicked, this, &TipOfDayDialog::onNextTip);
    connect(closeBtn_, &QPushButton::clicked, this, &QDialog::accept);
    connect(showOnStartupCheck_, &QCheckBox::stateChanged, this, &TipOfDayDialog::onShowOnStartupChanged);
}

void TipOfDayDialog::loadTips() {
    tips_.append({
        tr("Quick Execute"),
        tr("Press <b>F9</b> to execute the current SQL statement. Use <b>Ctrl+F9</b> to execute only the selected text.")
    });
    
    tips_.append({
        tr("Navigate Windows"),
        tr("Use <b>Ctrl+Tab</b> to switch to the next window and <b>Ctrl+Shift+Tab</b> to go to the previous window.")
    });
    
    tips_.append({
        tr("Comment Shortcuts"),
        tr("Press <b>Ctrl+/</b> to toggle comments on the current line or selection. You can also use Query → Comment to comment multiple lines.")
    });
    
    tips_.append({
        tr("Connection Management"),
        tr("Use File → New Connection to create connection profiles. Your connections are saved and automatically available on restart.")
    });
    
    tips_.append({
        tr("Transaction Control"),
        tr("Use the Transaction menu to manually control transactions. You can set isolation levels and enable/disable auto-commit mode.")
    });
    
    tips_.append({
        tr("Find and Replace"),
        tr("Press <b>Ctrl+F</b> to find text and <b>Ctrl+H</b> for find and replace. You can use regular expressions for advanced searching.")
    });
    
    tips_.append({
        tr("Zoom Controls"),
        tr("Use <b>Ctrl++</b> to zoom in, <b>Ctrl+-</b> to zoom out, and <b>Ctrl+0</b> to reset zoom in the SQL editor.")
    });
    
    tips_.append({
        tr("Export Results"),
        tr("You can export query results to multiple formats including CSV, JSON, Excel, and SQL INSERT statements using the Tools → Export Data menu.")
    });
    
    tips_.append({
        tr("Import Data"),
        tr("Use Tools → Import Data to import data from CSV, JSON, Excel, or SQL files into your database.")
    });
    
    tips_.append({
        tr("Monitor Database"),
        tr("Use Tools → Monitor to view active connections, transactions, statements, locks, and performance metrics.")
    });
    
    tips_.append({
        tr("Customize Appearance"),
        tr("Go to Tools → Theme Settings to change between Light, Dark, and Night themes, or customize colors and fonts.")
    });
    
    tips_.append({
        tr("Keyboard Shortcuts"),
        tr("Press F1 to view the full list of keyboard shortcuts. You can also access it from Help → Keyboard Shortcuts.")
    });
    
    tips_.append({
        tr("ER Diagram"),
        tr("Generate visual entity-relationship diagrams using Tools → ER Diagram to understand your database schema.")
    });
    
    tips_.append({
        tr("Query History"),
        tr("Your executed queries are saved in the history. Access them to quickly rerun previous statements.")
    });
    
    tips_.append({
        tr("Drag and Drop"),
        tr("Drag tables from the Project Navigator to the SQL editor to quickly generate SELECT statements.")
    });
}

void TipOfDayDialog::loadTip(int index) {
    if (index < 0 || index >= tips_.size()) return;
    
    const Tip& tip = tips_[index];
    titleLabel_->setText(tip.title);
    contentLabel_->setText(tip.content);
    
    // Update button states
    previousBtn_->setEnabled(index > 0);
    nextBtn_->setEnabled(index < tips_.size() - 1);
}

void TipOfDayDialog::onNextTip() {
    if (currentTipIndex_ < tips_.size() - 1) {
        currentTipIndex_++;
        loadTip(currentTipIndex_);
    }
}

void TipOfDayDialog::onPreviousTip() {
    if (currentTipIndex_ > 0) {
        currentTipIndex_--;
        loadTip(currentTipIndex_);
    }
}

void TipOfDayDialog::onShowOnStartupChanged(int state) {
    QSettings settings;
    settings.setValue("showTipsOnStartup", state == Qt::Checked);
}

bool TipOfDayDialog::shouldShowOnStartup() {
    QSettings settings;
    return settings.value("showTipsOnStartup", true).toBool();
}

} // namespace scratchrobin::ui
