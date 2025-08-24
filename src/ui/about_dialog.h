#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include <QDialogButtonBox>

namespace scratchrobin {

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog();

private slots:
    void showLicense();

private:
    void setupUI();
    void setupAboutTab();
    void setupCreditsTab();
    void setupLicenseTab();
    void setupSystemTab();

    QTabWidget* tabWidget_;
    QDialogButtonBox* buttonBox_;

    // About tab widgets
    QLabel* logoLabel_;
    QLabel* titleLabel_;
    QLabel* versionLabel_;
    QLabel* descriptionLabel_;

    // System info widgets
    QLabel* qtVersionLabel_;
    QLabel* buildDateLabel_;
    QLabel* osLabel_;
    QLabel* architectureLabel_;
};

} // namespace scratchrobin
