#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QTimer>

namespace scratchrobin {

struct VersionInfo {
    QString version;
    QString downloadUrl;
    QString releaseNotes;
    QString releaseDate;
    QString size;
    bool isNewer;
};

class UpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget* parent = nullptr);
    ~UpdateDialog();

    void checkForUpdates();
    void setCurrentVersion(const QString& version);

signals:
    void updateAvailable(const VersionInfo& info);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& error);

public slots:
    void downloadUpdate();
    void showReleaseNotes();

private slots:
    void onUpdateCheckFinished(QNetworkReply* reply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    void updateProgress();

private:
    void setupUI();
    void setupUpdateInfoTab();
    void setupDownloadTab();
    void setupSettingsTab();
    void loadSettings();
    void parseVersionInfo(const QJsonDocument& doc);
    void showUpdateAvailable(const VersionInfo& info);
    void showNoUpdateAvailable();
    void showUpdateCheckError(const QString& error);
    void startDownload(const QString& url);

    QTabWidget* tabWidget_;
    QDialogButtonBox* buttonBox_;

    // Update info tab
    QLabel* currentVersionLabel_;
    QLabel* latestVersionLabel_;
    QLabel* statusLabel_;
    QTextEdit* releaseNotesText_;
    QPushButton* checkUpdateButton_;
    QPushButton* downloadButton_;
    QPushButton* viewReleaseNotesButton_;

    // Download tab
    QProgressBar* downloadProgress_;
    QLabel* downloadStatusLabel_;
    QPushButton* cancelDownloadButton_;
    QPushButton* openDownloadButton_;

    // Settings tab
    QLineEdit* updateUrlEdit_;
    QCheckBox* autoCheckUpdates_;
    QSpinBox* checkIntervalSpin_;
    QComboBox* updateChannelCombo_;

    // Network
    QNetworkAccessManager* networkManager_;
    QNetworkReply* currentReply_;
    QString currentDownloadPath_;

    // State
    QString currentVersion_;
    VersionInfo latestVersionInfo_;
    bool updateAvailable_;
    bool downloadInProgress_;
    QTimer* progressTimer_;
};

} // namespace scratchrobin
