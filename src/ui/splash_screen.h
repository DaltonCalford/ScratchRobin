#pragma once
#include <QSplashScreen>
#include <QPixmap>

QT_BEGIN_NAMESPACE
class QLabel;
class QProgressBar;
QT_END_NAMESPACE

namespace scratchrobin::ui {

class SplashScreen : public QSplashScreen {
  Q_OBJECT

 public:
  explicit SplashScreen(QWidget* parent = nullptr);
  ~SplashScreen() override;

  void showSplash();
  void setProgress(int percentage);
  void setStatusMessage(const QString& message);
  void beginLoading();
  void setStepConfigLoad();
  void setStepDecryptPasswords();
  void setStepInitUI();
  void setStepComplete();
  void closeSplash();

 private:
  void setupUi();
  QLabel* status_label_;
  QProgressBar* progress_bar_;
};

}  // namespace scratchrobin::ui
