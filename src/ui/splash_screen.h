#pragma once

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>

namespace scratchrobin {

class SplashScreen : public QDialog {
  Q_OBJECT

public:
  explicit SplashScreen(QWidget* parent = nullptr);
  ~SplashScreen();

  void setProgress(int value);
  void setMessage(const QString& message);
  void finish();

protected:
  void paintEvent(QPaintEvent* event) override;

private slots:
  void updateProgress();

private:
  void setupUI();

  QLabel* logoLabel_;
  QLabel* messageLabel_;
  QProgressBar* progressBar_;
  QTimer* timer_;
  int progressValue_;
  QStringList loadingMessages_;
  int messageIndex_;
};

} // namespace scratchrobin
