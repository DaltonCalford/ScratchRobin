#include "ui/about_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QtGlobal>

namespace scratchrobin::ui {

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("About ScratchRobin"));
    setMinimumSize(450, 350);
    resize(500, 400);
    
    setupUi();
}

AboutDialog::~AboutDialog() = default;

void AboutDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Logo placeholder
    logoLabel_ = new QLabel(this);
    logoLabel_->setAlignment(Qt::AlignCenter);
    logoLabel_->setText("🐦");  // Bird emoji as placeholder logo
    logoLabel_->setStyleSheet("QLabel { font-size: 64px; }");
    mainLayout->addWidget(logoLabel_);
    
    // Application name
    auto* nameLabel = new QLabel(tr("<h1>ScratchRobin</h1>"), this);
    nameLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(nameLabel);
    
    // Version
    versionLabel_ = new QLabel(this);
    versionLabel_->setAlignment(Qt::AlignCenter);
    versionLabel_->setText(getVersionInfo());
    mainLayout->addWidget(versionLabel_);
    
    // Description
    auto* descLabel = new QLabel(
        tr("<p>A native database administration client for ScratchBird.</p>"),
        this);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);
    
    // Separator line
    auto* line = new QLabel(this);
    line->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    mainLayout->addWidget(line);
    
    // Build info
    buildLabel_ = new QLabel(this);
    buildLabel_->setAlignment(Qt::AlignCenter);
    buildLabel_->setText(getBuildInfo());
    buildLabel_->setStyleSheet("QLabel { color: gray; font-size: 11px; }");
    mainLayout->addWidget(buildLabel_);
    
    // Copyright
    copyrightLabel_ = new QLabel(this);
    copyrightLabel_->setAlignment(Qt::AlignCenter);
    copyrightLabel_->setText(tr("© 2025-2026 Dalton Calford<br>Licensed under MIT License"));
    copyrightLabel_->setStyleSheet("QLabel { color: gray; font-size: 11px; }");
    mainLayout->addWidget(copyrightLabel_);
    
    mainLayout->addStretch();
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    websiteBtn_ = new QPushButton(tr("Visit Website"), this);
    buttonLayout->addWidget(websiteBtn_);
    
    issuesBtn_ = new QPushButton(tr("Report Issue"), this);
    buttonLayout->addWidget(issuesBtn_);
    
    licenseBtn_ = new QPushButton(tr("View License"), this);
    buttonLayout->addWidget(licenseBtn_);
    
    buttonLayout->addStretch();
    
    okBtn_ = new QPushButton(tr("OK"), this);
    okBtn_->setDefault(true);
    buttonLayout->addWidget(okBtn_);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(websiteBtn_, &QPushButton::clicked, this, &AboutDialog::onVisitWebsite);
    connect(issuesBtn_, &QPushButton::clicked, this, &AboutDialog::onReportIssue);
    connect(licenseBtn_, &QPushButton::clicked, this, &AboutDialog::onViewLicense);
    connect(okBtn_, &QPushButton::clicked, this, &QDialog::accept);
}

QString AboutDialog::getVersionInfo() const {
    return tr("<p><b>Version 0.1.0</b> (Alpha)</p>");
}

QString AboutDialog::getBuildInfo() const {
    QString buildInfo = tr("<p>Built with:<br>");
    buildInfo += tr("• Qt %1<br>").arg(QT_VERSION_STR);
    buildInfo += tr("• CMake 3.20+<br>");
    buildInfo += tr("• GCC 13.3.0<br>");
    buildInfo += tr("• C++20</p>");
    return buildInfo;
}

void AboutDialog::onVisitWebsite() {
    QDesktopServices::openUrl(QUrl("https://scratchbird.dev"));
}

void AboutDialog::onReportIssue() {
    QDesktopServices::openUrl(QUrl("https://github.com/DaltonCalford/ScratchRobin/issues"));
}

void AboutDialog::onViewLicense() {
    QDesktopServices::openUrl(QUrl("https://github.com/DaltonCalford/ScratchRobin/blob/main/LICENSE"));
}

} // namespace scratchrobin::ui
