#include "ui/splash_screen.h"
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QThread>

namespace scratchrobin::ui {

SplashScreen::SplashScreen(QWidget* parent) : QSplashScreen(QPixmap()) {
    Q_UNUSED(parent)
  setupUi();
}

SplashScreen::~SplashScreen() = default;

void SplashScreen::setupUi() {
  QPixmap pixmap(500, 320);
  pixmap.fill(QColor(30, 50, 80));
  setPixmap(pixmap);

  auto* widget = new QWidget(this);
  widget->setGeometry(0, 0, 500, 320);
  
  auto* main_layout = new QVBoxLayout(widget);
  main_layout->setContentsMargins(40, 40, 40, 30);
  main_layout->setSpacing(10);

  auto* title_label = new QLabel(tr("ScratchRobin"), widget);
  QFont title_font = title_label->font();
  title_font.setPointSize(28);
  title_font.setBold(true);
  title_label->setFont(title_font);
  title_label->setStyleSheet("color: white;");
  title_label->setAlignment(Qt::AlignCenter);
  main_layout->addWidget(title_label);

  auto* version_label = new QLabel(tr("Version 0.1.0 - ScratchBird Native Client"), widget);
  version_label->setStyleSheet("color: rgb(180, 200, 230);");
  version_label->setAlignment(Qt::AlignCenter);
  main_layout->addWidget(version_label);
  main_layout->addStretch();

  status_label_ = new QLabel(tr("Initializing..."), widget);
  status_label_->setStyleSheet("color: white;");
  main_layout->addWidget(status_label_);

  progress_bar_ = new QProgressBar(widget);
  progress_bar_->setRange(0, 100);
  progress_bar_->setValue(0);
  progress_bar_->setTextVisible(false);
  progress_bar_->setStyleSheet(
      "QProgressBar { border: 1px solid grey; border-radius: 5px; background-color: #1e3a5f; height: 20px; }"
      "QProgressBar::chunk { background-color: #4a90d9; border-radius: 5px; }");
  main_layout->addWidget(progress_bar_);
}

void SplashScreen::showSplash() { show(); raise(); QApplication::processEvents(); }

void SplashScreen::setProgress(int percentage) {
  percentage = qBound(0, percentage, 100);
  if (progress_bar_) progress_bar_->setValue(percentage);
  QApplication::processEvents();
}

void SplashScreen::setStatusMessage(const QString& message) {
  if (status_label_) status_label_->setText(message);
  QApplication::processEvents();
}

void SplashScreen::beginLoading() { setProgress(0); setStatusMessage(tr("Starting application...")); }
void SplashScreen::setStepConfigLoad() { setProgress(20); setStatusMessage(tr("Loading configuration...")); }
void SplashScreen::setStepDecryptPasswords() { setProgress(50); setStatusMessage(tr("Waiting for password decryption key...")); }
void SplashScreen::setStepInitUI() { setProgress(75); setStatusMessage(tr("Initializing user interface...")); }
void SplashScreen::setStepComplete() { setProgress(100); setStatusMessage(tr("Ready")); QThread::msleep(200); QApplication::processEvents(); }
void SplashScreen::closeSplash() { close(); }

}  // namespace scratchrobin::ui
