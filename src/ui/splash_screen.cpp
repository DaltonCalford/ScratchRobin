#include "splash_screen.h"
#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QApplication>
#include <QDesktopWidget>
#include <QDir>

namespace scratchrobin {

SplashScreen::SplashScreen(QWidget* parent)
    : QDialog(parent),
      progressValue_(0),
      messageIndex_(0) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setFixedSize(600, 400);

  // Setup loading messages
  loadingMessages_ << "Initializing ScratchRobin Database GUI..."
                   << "Loading core components..."
                   << "Setting up connection manager..."
                   << "Initializing metadata manager..."
                   << "Preparing user interface..."
                   << "Loading database drivers..."
                   << "Configuring application settings..."
                   << "Ready to connect to database!";

  setupUI();

  // Setup progress timer
  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &SplashScreen::updateProgress);
  timer_->start(200); // Update every 200ms

  // Center the splash screen
  QDesktopWidget* desktop = QApplication::desktop();
  QRect screenGeometry = desktop->availableGeometry();
  move((screenGeometry.width() - width()) / 2,
       (screenGeometry.height() - height()) / 2);
}

SplashScreen::~SplashScreen() {
  // Clean up
}

void SplashScreen::setupUI() {
  // Main layout
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(15);

  // Logo section
  logoLabel_ = new QLabel(this);
  logoLabel_->setAlignment(Qt::AlignCenter);

  // Load the logo
  QString logoPath = ":/logos/Artwork/ScratchRobinLogoHeader.png";
  QPixmap logo(logoPath);
  if (!logo.isNull()) {
    // Scale logo to fit splash screen
    logo = logo.scaledToWidth(400, Qt::SmoothTransformation);
    logoLabel_->setPixmap(logo);
  } else {
    // Fallback if logo not found
    logoLabel_->setText("ScratchRobin Database GUI");
    logoLabel_->setFont(QFont("Arial", 24, QFont::Bold));
    logoLabel_->setStyleSheet("color: #2E7D32; padding: 20px;");
  }

  mainLayout->addWidget(logoLabel_);

  // Version info
  QLabel* versionLabel = new QLabel("Version 0.1.0", this);
  versionLabel->setAlignment(Qt::AlignCenter);
  versionLabel->setFont(QFont("Arial", 12));
  versionLabel->setStyleSheet("color: #666; margin-bottom: 10px;");
  mainLayout->addWidget(versionLabel);

  // Message label
  messageLabel_ = new QLabel("Starting ScratchRobin...", this);
  messageLabel_->setAlignment(Qt::AlignCenter);
  messageLabel_->setFont(QFont("Arial", 11));
  messageLabel_->setStyleSheet("color: #333; padding: 5px;");
  messageLabel_->setWordWrap(true);
  mainLayout->addWidget(messageLabel_);

  // Progress bar
  progressBar_ = new QProgressBar(this);
  progressBar_->setRange(0, 100);
  progressBar_->setValue(0);
  progressBar_->setTextVisible(true);
  progressBar_->setStyleSheet(
    "QProgressBar {"
    "    border: 2px solid #ddd;"
    "    border-radius: 5px;"
    "    text-align: center;"
    "    background-color: #f5f5f5;"
    "}"
    "QProgressBar::chunk {"
    "    background-color: #4CAF50;"
    "    border-radius: 3px;"
    "}"
  );
  mainLayout->addWidget(progressBar_);

  // Copyright notice
  QLabel* copyrightLabel = new QLabel("© 2025 ScratchRobin. All rights reserved.", this);
  copyrightLabel->setAlignment(Qt::AlignCenter);
  copyrightLabel->setFont(QFont("Arial", 9));
  copyrightLabel->setStyleSheet("color: #999; margin-top: 15px;");
  mainLayout->addWidget(copyrightLabel);
}

void SplashScreen::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Draw rounded rectangle background
  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(255, 255, 255, 240));
  painter.drawRoundedRect(rect(), 15, 15);

  // Draw border
  painter.setPen(QPen(QColor(200, 200, 200), 1));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 14, 14);

  QDialog::paintEvent(event);
}

void SplashScreen::setProgress(int value) {
  progressValue_ = value;
  progressBar_->setValue(value);
}

void SplashScreen::setMessage(const QString& message) {
  messageLabel_->setText(message);
}

void SplashScreen::updateProgress() {
  if (progressValue_ < 100) {
    progressValue_ += 2; // Increment progress
    progressBar_->setValue(progressValue_);

    // Update message based on progress
    int messageStep = (progressValue_ / 100.0) * loadingMessages_.size();
    if (messageStep < loadingMessages_.size()) {
      messageLabel_->setText(loadingMessages_[messageStep]);
    }

    // Check if we're done
    if (progressValue_ >= 100) {
      timer_->stop();
      QTimer::singleShot(500, this, &SplashScreen::accept);
    }
  }
}

void SplashScreen::finish() {
  progressValue_ = 100;
  progressBar_->setValue(100);
  messageLabel_->setText("Ready!");
  timer_->stop();
  QTimer::singleShot(300, this, &SplashScreen::accept);
}

} // namespace scratchrobin
