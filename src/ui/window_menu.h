#pragma once
#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
class QWidget;
class QMainWindow;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Window menu implementation for MDI window management
 * 
 * Implements MENU_SPECIFICATION.md §Window section:
 * - New Window
 * - Close Window / Close All Windows
 * - Next Window / Previous Window
 * - Cascade / Tile Horizontal / Tile Vertical
 */
class WindowMenu : public QObject {
    Q_OBJECT

public:
    explicit WindowMenu(QMainWindow* mainWindow);
    ~WindowMenu() override;

    QMenu* menu() const { return menu_; }

    // State management
    void setHasWindows(bool hasWindows);
    void setHasMultipleWindows(bool hasMultiple);
    void updateWindowList(const QStringList& windowNames, int activeIndex);

signals:
    void newWindowRequested();
    void closeWindowRequested();
    void closeAllWindowsRequested();
    void nextWindowRequested();
    void previousWindowRequested();
    void cascadeRequested();
    void tileHorizontalRequested();
    void tileVerticalRequested();
    void windowActivated(int index);

private slots:
    void onNewWindow();
    void onCloseWindow();
    void onCloseAllWindows();
    void onNextWindow();
    void onPreviousWindow();
    void onCascade();
    void onTileHorizontal();
    void onTileVertical();
    void onWindowActionTriggered();

private:
    void setupActions();
    void updateActionStates();
    void rebuildWindowList();

    QMainWindow* mainWindow_;
    QMenu* menu_ = nullptr;

    // State
    bool hasWindows_ = false;
    bool hasMultipleWindows_ = false;
    QStringList windowNames_;
    int activeWindowIndex_ = -1;

    // Actions
    QAction* actionNew_ = nullptr;
    QAction* actionClose_ = nullptr;
    QAction* actionCloseAll_ = nullptr;
    QAction* actionNext_ = nullptr;
    QAction* actionPrevious_ = nullptr;
    QAction* actionCascade_ = nullptr;
    QAction* actionTileH_ = nullptr;
    QAction* actionTileV_ = nullptr;
    
    // Window list actions (dynamic)
    QList<QAction*> windowActions_;
};

} // namespace scratchrobin::ui
