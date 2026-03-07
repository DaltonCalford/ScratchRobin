#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Tip of the Day dialog
 * 
 * Shows daily tips to help users learn features
 */
class TipOfDayDialog : public QDialog {
    Q_OBJECT

public:
    explicit TipOfDayDialog(QWidget* parent = nullptr);
    ~TipOfDayDialog() override;

    static bool shouldShowOnStartup();

private slots:
    void onNextTip();
    void onPreviousTip();
    void onShowOnStartupChanged(int state);

private:
    void setupUi();
    void loadTip(int index);
    void loadTips();

    struct Tip {
        QString title;
        QString content;
    };

    QVector<Tip> tips_;
    int currentTipIndex_ = 0;

    QLabel* iconLabel_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* contentLabel_ = nullptr;
    QCheckBox* showOnStartupCheck_ = nullptr;
    QPushButton* previousBtn_ = nullptr;
    QPushButton* nextBtn_ = nullptr;
    QPushButton* closeBtn_ = nullptr;
};

} // namespace scratchrobin::ui
