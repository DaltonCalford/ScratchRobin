#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief About dialog for ScratchRobin
 * 
 * Implements FORM_SPECIFICATION.md About Dialog section:
 * - Logo
 * - Version information
 * - Build information
 * - Copyright and license
 * - Links (website, issues, license)
 */
class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override;

private slots:
    void onVisitWebsite();
    void onReportIssue();
    void onViewLicense();

private:
    void setupUi();
    QString getVersionInfo() const;
    QString getBuildInfo() const;

    QLabel* logoLabel_ = nullptr;
    QLabel* versionLabel_ = nullptr;
    QLabel* buildLabel_ = nullptr;
    QLabel* copyrightLabel_ = nullptr;
    QPushButton* websiteBtn_ = nullptr;
    QPushButton* issuesBtn_ = nullptr;
    QPushButton* licenseBtn_ = nullptr;
    QPushButton* okBtn_ = nullptr;
};

} // namespace scratchrobin::ui
