#include "update_dialog.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QDesktopServices>
#include <QSettings>
#include <QDateTime>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QUrlQuery>

namespace scratchrobin {

UpdateDialog::UpdateDialog(QWidget* parent)
    : QDialog(parent),
      updateAvailable_(false),
      downloadInProgress_(false) {
    setWindowTitle("Check for Updates");
    setModal(true);
    setFixedSize(550, 500);
    resize(550, 500);

    setWindowIcon(QIcon(":/logos/Artwork/ScratchRobin.png"));

    setupUI();

    // Setup network manager
    networkManager_ = new QNetworkAccessManager(this);

    // Setup progress timer for fake progress during checking
    progressTimer_ = new QTimer(this);

    // Load settings
    loadSettings();
}

UpdateDialog::~UpdateDialog() {
    if (currentReply_) {
        currentReply_->abort();
        currentReply_->deleteLater();
    }
}

void UpdateDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QLabel* headerLabel = new QLabel("Software Updates", this);
    headerLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(headerLabel);

    // Tab widget
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    setupUpdateInfoTab();
    setupDownloadTab();
    setupSettingsTab();

    // Button box
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox_);
}

void UpdateDialog::setupUpdateInfoTab() {
    QWidget* updateTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(updateTab);

    // Current version info
    QGroupBox* currentGroup = new QGroupBox("Current Version");
    QFormLayout* currentForm = new QFormLayout(currentGroup);

    currentVersionLabel_ = new QLabel("0.1.0", currentGroup);
    currentVersionLabel_->setStyleSheet("font-weight: bold; color: #2E7D32;");
    currentForm->addRow("Installed:", currentVersionLabel_);

    layout->addWidget(currentGroup);

    // Latest version info
    QGroupBox* latestGroup = new QGroupBox("Latest Version");
    QFormLayout* latestForm = new QFormLayout(latestGroup);

    latestVersionLabel_ = new QLabel("Checking...", latestGroup);
    latestForm->addRow("Available:", latestVersionLabel_);

    layout->addWidget(latestGroup);

    // Status and actions
    QWidget* actionWidget = new QWidget();
    QVBoxLayout* actionLayout = new QVBoxLayout(actionWidget);

    statusLabel_ = new QLabel("Ready to check for updates", actionWidget);
    statusLabel_->setWordWrap(true);
    statusLabel_->setStyleSheet("padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #f9f9f9;");
    actionLayout->addWidget(statusLabel_);

    QWidget* buttonWidget = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);

    checkUpdateButton_ = new QPushButton("Check for Updates", buttonWidget);
    checkUpdateButton_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px 16px; border: none; border-radius: 4px; } QPushButton:hover { background-color: #1976D2; }");
    connect(checkUpdateButton_, &QPushButton::clicked, this, &UpdateDialog::checkForUpdates);
    buttonLayout->addWidget(checkUpdateButton_);

    downloadButton_ = new QPushButton("Download Update", buttonWidget);
    downloadButton_->setEnabled(false);
    downloadButton_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px 16px; border: none; border-radius: 4px; } QPushButton:hover { background-color: #45a049; }");
    connect(downloadButton_, &QPushButton::clicked, this, &UpdateDialog::downloadUpdate);
    buttonLayout->addWidget(downloadButton_);

    viewReleaseNotesButton_ = new QPushButton("View Release Notes", buttonWidget);
    viewReleaseNotesButton_->setEnabled(false);
    connect(viewReleaseNotesButton_, &QPushButton::clicked, this, &UpdateDialog::showReleaseNotes);
    buttonLayout->addWidget(viewReleaseNotesButton_);

    buttonLayout->addStretch();
    actionLayout->addWidget(buttonWidget);

    layout->addWidget(actionWidget);

    // Release notes
    QGroupBox* notesGroup = new QGroupBox("Release Notes");
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);

    releaseNotesText_ = new QTextEdit(notesGroup);
    releaseNotesText_->setReadOnly(true);
    releaseNotesText_->setMaximumHeight(150);
    releaseNotesText_->setPlaceholderText("Release notes will appear here after checking for updates...");
    notesLayout->addWidget(releaseNotesText_);

    layout->addWidget(notesGroup);

    tabWidget_->addTab(updateTab, "Updates");
}

void UpdateDialog::setupDownloadTab() {
    QWidget* downloadTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(downloadTab);

    QLabel* infoLabel = new QLabel("Download Progress", downloadTab);
    infoLabel->setStyleSheet("font-size: 14px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(infoLabel);

    // Download progress
    QGroupBox* progressGroup = new QGroupBox("Download Progress");
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);

    downloadProgress_ = new QProgressBar(progressGroup);
    downloadProgress_->setRange(0, 100);
    downloadProgress_->setValue(0);
    downloadProgress_->setVisible(false);
    progressLayout->addWidget(downloadProgress_);

    downloadStatusLabel_ = new QLabel("No download in progress", progressGroup);
    downloadStatusLabel_->setWordWrap(true);
    progressLayout->addWidget(downloadStatusLabel_);

    QWidget* downloadButtonWidget = new QWidget();
    QHBoxLayout* downloadButtonLayout = new QHBoxLayout(downloadButtonWidget);

    cancelDownloadButton_ = new QPushButton("Cancel Download", downloadButtonWidget);
    cancelDownloadButton_->setEnabled(false);
    cancelDownloadButton_->setStyleSheet("QPushButton { background-color: #F44336; color: white; padding: 6px 12px; border: none; border-radius: 3px; }");
    connect(cancelDownloadButton_, &QPushButton::clicked, [this]() {
        if (currentReply_) {
            currentReply_->abort();
        }
    });
    downloadButtonLayout->addWidget(cancelDownloadButton_);

    openDownloadButton_ = new QPushButton("Open Download Folder", downloadButtonWidget);
    openDownloadButton_->setEnabled(false);
    openDownloadButton_->setStyleSheet("QPushButton { background-color: #607D8B; color: white; padding: 6px 12px; border: none; border-radius: 3px; }");
    connect(openDownloadButton_, &QPushButton::clicked, [this]() {
        QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QDesktopServices::openUrl(QUrl::fromLocalFile(downloadDir));
    });
    downloadButtonLayout->addWidget(openDownloadButton_);

    downloadButtonLayout->addStretch();
    progressLayout->addWidget(downloadButtonWidget);

    layout->addWidget(progressGroup);

    // Instructions
    QGroupBox* instructionsGroup = new QGroupBox("Installation Instructions");
    QVBoxLayout* instructionsLayout = new QVBoxLayout(instructionsGroup);

    QLabel* instructionsText = new QLabel(
        "1. Download the update package\n"
        "2. Close ScratchRobin application\n"
        "3. Run the installer or extract the update\n"
        "4. Follow the installation instructions\n"
        "5. Restart ScratchRobin with the new version",
        instructionsGroup
    );
    instructionsText->setWordWrap(true);
    instructionsLayout->addWidget(instructionsText);

    layout->addWidget(instructionsGroup);
    layout->addStretch();

    tabWidget_->addTab(downloadTab, "Download");
}

void UpdateDialog::setupSettingsTab() {
    QWidget* settingsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(settingsTab);

    // Update source settings
    QGroupBox* sourceGroup = new QGroupBox("Update Source");
    QFormLayout* sourceForm = new QFormLayout(sourceGroup);

    updateUrlEdit_ = new QLineEdit("https://api.github.com/repos/DaltonCalford/ScratchRobin/releases/latest", sourceGroup);
    sourceForm->addRow("Update URL:", updateUrlEdit_);

    updateChannelCombo_ = new QComboBox(sourceGroup);
    updateChannelCombo_->addItems({"Stable", "Beta", "Development"});
    sourceForm->addRow("Channel:", updateChannelCombo_);

    layout->addWidget(sourceGroup);

    // Auto-check settings
    QGroupBox* autoGroup = new QGroupBox("Automatic Updates");
    QVBoxLayout* autoLayout = new QVBoxLayout(autoGroup);

    autoCheckUpdates_ = new QCheckBox("Automatically check for updates");
    autoCheckUpdates_->setChecked(true);
    autoLayout->addWidget(autoCheckUpdates_);

    QWidget* intervalWidget = new QWidget();
    QHBoxLayout* intervalLayout = new QHBoxLayout(intervalWidget);
    intervalLayout->addWidget(new QLabel("Check interval:"));
    checkIntervalSpin_ = new QSpinBox();
    checkIntervalSpin_->setRange(1, 30);
    checkIntervalSpin_->setValue(7);
    checkIntervalSpin_->setSuffix(" days");
    intervalLayout->addWidget(checkIntervalSpin_);
    intervalLayout->addStretch();
    autoLayout->addWidget(intervalWidget);

    layout->addWidget(autoGroup);
    layout->addStretch();

    tabWidget_->addTab(settingsTab, "Settings");
}

void UpdateDialog::setCurrentVersion(const QString& version) {
    currentVersion_ = version;
    currentVersionLabel_->setText(version);
}

void UpdateDialog::checkForUpdates() {
    checkUpdateButton_->setEnabled(false);
    statusLabel_->setText("Checking for updates...");
    statusLabel_->setStyleSheet("padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #fff3cd; color: #856404;");

    // For demonstration, we'll simulate a network check
    // In a real application, you would make an actual network request
    progressTimer_->start(100);

    QTimer::singleShot(2000, [this]() {
        // Simulate finding a newer version
        VersionInfo simulatedUpdate;
        simulatedUpdate.version = "0.2.0";
        simulatedUpdate.downloadUrl = "https://github.com/DaltonCalford/ScratchRobin/releases/download/v0.2.0/ScratchRobin-0.2.0-setup.exe";
        simulatedUpdate.releaseNotes = "New Features:\n• Enhanced connection management\n• Improved query editor\n• Better error handling\n\nBug Fixes:\n• Fixed connection timeout issues\n• Resolved memory leaks\n• Improved stability";
        simulatedUpdate.releaseDate = QDate::currentDate().toString("yyyy-MM-dd");
        simulatedUpdate.size = "45.2 MB";
        simulatedUpdate.isNewer = true;

        showUpdateAvailable(simulatedUpdate);
    });
}

void UpdateDialog::downloadUpdate() {
    if (!updateAvailable_ || latestVersionInfo_.downloadUrl.isEmpty()) {
        QMessageBox::warning(this, "Download Error", "No update available for download.");
        return;
    }

    tabWidget_->setCurrentIndex(1); // Switch to download tab
    startDownload(latestVersionInfo_.downloadUrl);
}

void UpdateDialog::showReleaseNotes() {
    if (releaseNotesText_->toPlainText().isEmpty()) {
        QMessageBox::information(this, "Release Notes", "No release notes available.");
        return;
    }

    QMessageBox::information(this, "Release Notes", releaseNotesText_->toPlainText());
}

void UpdateDialog::startDownload(const QString& url) {
    downloadInProgress_ = true;
    downloadProgress_->setVisible(true);
    downloadProgress_->setValue(0);
    downloadStatusLabel_->setText("Starting download...");
    cancelDownloadButton_->setEnabled(true);

    // For demonstration, we'll simulate a download
    progressTimer_->start(200);

    QTimer::singleShot(5000, [this]() {
        downloadInProgress_ = false;
        downloadProgress_->setValue(100);
        downloadStatusLabel_->setText("Download completed successfully!");
        cancelDownloadButton_->setEnabled(false);
        openDownloadButton_->setEnabled(true);

        QMessageBox::information(this, "Download Complete",
                               "Update downloaded successfully!\n\n"
                               "Please close ScratchRobin and run the installer to complete the update.");
    });
}

void UpdateDialog::onUpdateCheckFinished(QNetworkReply* reply) {
    checkUpdateButton_->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        showUpdateCheckError(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull()) {
        showUpdateCheckError("Invalid response format");
    } else {
        parseVersionInfo(doc);
    }

    reply->deleteLater();
}

void UpdateDialog::parseVersionInfo(const QJsonDocument& doc) {
    if (!doc.isObject()) {
        showUpdateCheckError("Invalid JSON response");
        return;
    }

    QJsonObject obj = doc.object();

    VersionInfo info;
    info.version = obj.value("tag_name").toString().remove("v");
    info.downloadUrl = obj.value("html_url").toString();
    info.releaseNotes = obj.value("body").toString();
    info.releaseDate = obj.value("published_at").toString().left(10);

    // Check if this version is newer
    QStringList currentParts = currentVersion_.split('.');
    QStringList latestParts = info.version.split('.');

    info.isNewer = false;
    for (int i = 0; i < qMin(currentParts.size(), latestParts.size()); ++i) {
        int currentNum = currentParts[i].toInt();
        int latestNum = latestParts[i].toInt();

        if (latestNum > currentNum) {
            info.isNewer = true;
            break;
        } else if (latestNum < currentNum) {
            break;
        }
    }

    if (info.isNewer) {
        showUpdateAvailable(info);
    } else {
        showNoUpdateAvailable();
    }
}

void UpdateDialog::showUpdateAvailable(const VersionInfo& info) {
    updateAvailable_ = true;
    latestVersionInfo_ = info;

    latestVersionLabel_->setText(info.version + " (Newer)");
    latestVersionLabel_->setStyleSheet("font-weight: bold; color: #4CAF50;");

    statusLabel_->setText("Update available! Click 'Download Update' to get the latest version.");
    statusLabel_->setStyleSheet("padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #d4edda; color: #155724;");

    releaseNotesText_->setPlainText(info.releaseNotes);
    downloadButton_->setEnabled(true);
    viewReleaseNotesButton_->setEnabled(true);

    checkUpdateButton_->setEnabled(true);
}

void UpdateDialog::showNoUpdateAvailable() {
    updateAvailable_ = false;

    latestVersionLabel_->setText(currentVersion_ + " (Current)");
    latestVersionLabel_->setStyleSheet("color: #666;");

    statusLabel_->setText("You are running the latest version.");
    statusLabel_->setStyleSheet("padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #f9f9f9; color: #666;");

    releaseNotesText_->setPlainText("No updates available. You are running the latest version.");
    downloadButton_->setEnabled(false);
    viewReleaseNotesButton_->setEnabled(false);

    checkUpdateButton_->setEnabled(true);
}

void UpdateDialog::showUpdateCheckError(const QString& error) {
    latestVersionLabel_->setText("Error");
    latestVersionLabel_->setStyleSheet("color: #F44336;");

    statusLabel_->setText("Failed to check for updates: " + error);
    statusLabel_->setStyleSheet("padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #f8d7da; color: #721c24;");

    downloadButton_->setEnabled(false);
    viewReleaseNotesButton_->setEnabled(false);
    checkUpdateButton_->setEnabled(true);
}

void UpdateDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
        downloadProgress_->setValue(progress);
        downloadStatusLabel_->setText(QString("Downloaded %1 of %2 bytes")
                                    .arg(bytesReceived)
                                    .arg(bytesTotal));
    }
}

void UpdateDialog::onDownloadFinished() {
    downloadInProgress_ = false;
    downloadProgress_->setValue(100);
    downloadStatusLabel_->setText("Download completed successfully!");
    cancelDownloadButton_->setEnabled(false);
    openDownloadButton_->setEnabled(true);

    QMessageBox::information(this, "Download Complete",
                           "Update downloaded successfully!\n\n"
                           "Please close ScratchRobin and run the installer to complete the update.");
}

void UpdateDialog::onDownloadError(QNetworkReply::NetworkError error) {
    downloadInProgress_ = false;
    downloadStatusLabel_->setText("Download failed: " + currentReply_->errorString());
    cancelDownloadButton_->setEnabled(false);
    openDownloadButton_->setEnabled(false);
}

void UpdateDialog::updateProgress() {
    static int progress = 0;
    if (downloadInProgress_) {
        progress = (progress + 5) % 100;
        downloadProgress_->setValue(progress);
    }
}

void UpdateDialog::loadSettings() {
    QSettings settings("ScratchRobin", "Updates");

    updateUrlEdit_->setText(settings.value("updateUrl", "https://api.github.com/repos/DaltonCalford/ScratchRobin/releases/latest").toString());
    autoCheckUpdates_->setChecked(settings.value("autoCheckUpdates", true).toBool());
    checkIntervalSpin_->setValue(settings.value("checkInterval", 7).toInt());
    updateChannelCombo_->setCurrentText(settings.value("updateChannel", "Stable").toString());
}

} // namespace scratchrobin
