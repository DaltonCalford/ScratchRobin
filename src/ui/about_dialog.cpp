#include "about_dialog.h"
#include <QApplication>
#include <QIcon>
#include <QDate>
#include <QSysInfo>
#include <QStandardPaths>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QTextEdit>
#include <QPushButton>

namespace scratchrobin {

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("About ScratchRobin");
    setModal(true);
    setFixedSize(500, 400);
    resize(500, 400);

    // Set window icon
    setWindowIcon(QIcon(":/logos/Artwork/ScratchRobin.png"));

    setupUI();
}

AboutDialog::~AboutDialog() {
}

void AboutDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Tab widget
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    setupAboutTab();
    setupCreditsTab();
    setupLicenseTab();
    setupSystemTab();

    // Button box
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox_);
}

void AboutDialog::setupAboutTab() {
    QWidget* aboutTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(aboutTab);

    // Logo and title section
    QWidget* headerWidget = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);

    logoLabel_ = new QLabel();
    QPixmap logo(":/logos/Artwork/ScratchRobin.png");
    if (!logo.isNull()) {
        logoLabel_->setPixmap(logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    headerLayout->addWidget(logoLabel_);

    QWidget* titleWidget = new QWidget();
    QVBoxLayout* titleLayout = new QVBoxLayout(titleWidget);

    titleLabel_ = new QLabel("ScratchRobin");
    titleLabel_->setStyleSheet("font-size: 24px; font-weight: bold; color: #2E7D32;");
    titleLayout->addWidget(titleLabel_);

    versionLabel_ = new QLabel("Version 0.1.0");
    versionLabel_->setStyleSheet("font-size: 14px; color: #666;");
    titleLayout->addWidget(versionLabel_);

    headerLayout->addWidget(titleWidget);
    headerLayout->addStretch();

    layout->addWidget(headerWidget);

    // Description
    descriptionLabel_ = new QLabel(
        "ScratchRobin is a modern, professional database management interface designed for developers and database administrators. "
        "It provides an intuitive graphical interface for working with various database systems including PostgreSQL, MySQL, SQLite, and more."
    );
    descriptionLabel_->setWordWrap(true);
    descriptionLabel_->setStyleSheet("margin-top: 15px; padding: 10px; background-color: #f5f5f5; border-radius: 5px;");
    layout->addWidget(descriptionLabel_);

    // Features section
    QGroupBox* featuresGroup = new QGroupBox("Key Features");
    QVBoxLayout* featuresLayout = new QVBoxLayout(featuresGroup);

    QStringList features = {
        "• Multi-database support (PostgreSQL, MySQL, SQLite, Oracle, SQL Server)",
        "• Visual query builder with syntax highlighting",
        "• Database schema browser and object explorer",
        "• Table designer with DDL generation",
        "• Connection management with profiles",
        "• Query history and favorites",
        "• Import/export functionality",
        "• Backup and restore capabilities"
    };

    for (const QString& feature : features) {
        QLabel* featureLabel = new QLabel(feature);
        featuresLayout->addWidget(featureLabel);
    }

    layout->addWidget(featuresGroup);
    layout->addStretch();

    tabWidget_->addTab(aboutTab, "About");
}

void AboutDialog::setupCreditsTab() {
    QWidget* creditsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(creditsTab);

    QLabel* creditsTitle = new QLabel("Credits & Acknowledgments");
    creditsTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(creditsTitle);

    // Development team
    QGroupBox* teamGroup = new QGroupBox("Development Team");
    QVBoxLayout* teamLayout = new QVBoxLayout(teamGroup);

    QLabel* leadLabel = new QLabel("• Lead Developer: Dalton Calford");
    teamLayout->addWidget(leadLabel);

    QLabel* contributorsLabel = new QLabel("• Contributors: Open source community");
    teamLayout->addWidget(contributorsLabel);

    layout->addWidget(teamGroup);

    // Third-party libraries
    QGroupBox* librariesGroup = new QGroupBox("Third-Party Libraries");
    QVBoxLayout* librariesLayout = new QVBoxLayout(librariesGroup);

    QLabel* qtLabel = new QLabel("• Qt Framework - Cross-platform GUI toolkit");
    librariesLayout->addWidget(qtLabel);

    QLabel* sqliteLabel = new QLabel("• SQLite - Embedded database engine");
    librariesLayout->addWidget(sqliteLabel);

    QLabel* postgresLabel = new QLabel("• PostgreSQL Drivers - Database connectivity");
    librariesLayout->addWidget(postgresLabel);

    QLabel* mysqlLabel = new QLabel("• MySQL Drivers - Database connectivity");
    librariesLayout->addWidget(mysqlLabel);

    layout->addWidget(librariesGroup);

    // Special thanks
    QGroupBox* thanksGroup = new QGroupBox("Special Thanks");
    QVBoxLayout* thanksLayout = new QVBoxLayout(thanksGroup);

    QLabel* communityLabel = new QLabel("• Open source community for valuable contributions");
    thanksLayout->addWidget(communityLabel);

    QLabel* usersLabel = new QLabel("• Early adopters and beta testers");
    thanksLayout->addWidget(usersLabel);

    QLabel* inspirationLabel = new QLabel("• Inspired by industry-leading database tools");
    thanksLayout->addWidget(inspirationLabel);

    layout->addWidget(thanksGroup);
    layout->addStretch();

    tabWidget_->addTab(creditsTab, "Credits");
}

void AboutDialog::setupLicenseTab() {
    QWidget* licenseTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(licenseTab);

    QLabel* licenseTitle = new QLabel("License Information");
    licenseTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(licenseTitle);

    // License information
    QLabel* licenseInfo = new QLabel(
        "ScratchRobin is released under the MIT License.\n\n"
        "Copyright (c) 2025 Dalton Calford\n\n"
        "Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files (the \"Software\"), to deal "
        "in the Software without restriction, including without limitation the rights "
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
        "copies of the Software, and to permit persons to whom the Software is "
        "furnished to do so, subject to the following conditions:"
    );
    licenseInfo->setWordWrap(true);
    licenseInfo->setStyleSheet("padding: 10px; background-color: #f9f9f9; border-radius: 5px;");
    layout->addWidget(licenseInfo);

    // Full license text
    QTextEdit* licenseText = new QTextEdit();
    licenseText->setPlainText(
        "The above copyright notice and this permission notice shall be included in all "
        "copies or substantial portions of the Software.\n\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
        "SOFTWARE."
    );
    licenseText->setReadOnly(true);
    licenseText->setMaximumHeight(150);
    layout->addWidget(licenseText);

    tabWidget_->addTab(licenseTab, "License");
}

void AboutDialog::setupSystemTab() {
    QWidget* systemTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(systemTab);

    QLabel* systemTitle = new QLabel("System Information");
    systemTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(systemTitle);

    // System information
    QGroupBox* systemGroup = new QGroupBox("System Details");
    QFormLayout* systemForm = new QFormLayout(systemGroup);

    qtVersionLabel_ = new QLabel(qVersion());
    systemForm->addRow("Qt Version:", qtVersionLabel_);

    buildDateLabel_ = new QLabel(__DATE__ " " __TIME__);
    systemForm->addRow("Build Date:", buildDateLabel_);

    osLabel_ = new QLabel(QSysInfo::prettyProductName());
    systemForm->addRow("Operating System:", osLabel_);

    QString arch = QSysInfo::currentCpuArchitecture();
    if (QSysInfo::WordSize == 64) {
        arch += " (64-bit)";
    } else {
        arch += " (32-bit)";
    }
    architectureLabel_ = new QLabel(arch);
    systemForm->addRow("Architecture:", architectureLabel_);

    layout->addWidget(systemGroup);

    // Application paths
    QGroupBox* pathsGroup = new QGroupBox("Application Paths");
    QFormLayout* pathsForm = new QFormLayout(pathsGroup);

    QLabel* appDirLabel = new QLabel(QCoreApplication::applicationDirPath());
    pathsForm->addRow("Application Directory:", appDirLabel);

    QLabel* configDirLabel = new QLabel(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    pathsForm->addRow("Configuration Directory:", configDirLabel);

    QLabel* dataDirLabel = new QLabel(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    pathsForm->addRow("Data Directory:", dataDirLabel);

    layout->addWidget(pathsGroup);
    layout->addStretch();

    tabWidget_->addTab(systemTab, "System");
}

void AboutDialog::showLicense() {
    tabWidget_->setCurrentIndex(2); // Switch to license tab
}

} // namespace scratchrobin
