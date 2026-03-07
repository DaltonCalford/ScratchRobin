#include "ui/window_menu.h"

#include <QMenu>
#include <QAction>
#include <QMainWindow>

namespace scratchrobin::ui {

WindowMenu::WindowMenu(QMainWindow* mainWindow)
    : QObject(mainWindow)
    , mainWindow_(mainWindow) {
    setupActions();
}

WindowMenu::~WindowMenu() = default;

void WindowMenu::setupActions() {
    menu_ = new QMenu(tr("&Window"), mainWindow_);

    // Window management group
    actionNew_ = menu_->addAction(tr("&New Window"), this, &WindowMenu::onNewWindow);
    actionNew_->setStatusTip(tr("Open a new query window"));

    actionClose_ = menu_->addAction(tr("&Close Window"), this, &WindowMenu::onCloseWindow);
    actionClose_->setShortcut(QKeySequence("Ctrl+W"));
    actionClose_->setStatusTip(tr("Close current window"));

    actionCloseAll_ = menu_->addAction(tr("Close &All Windows"), this, &WindowMenu::onCloseAllWindows);
    actionCloseAll_->setStatusTip(tr("Close all windows"));

    menu_->addSeparator();

    // Navigation group
    actionNext_ = menu_->addAction(tr("&Next Window"), this, &WindowMenu::onNextWindow);
    actionNext_->setShortcut(QKeySequence("Ctrl+Tab"));
    actionNext_->setStatusTip(tr("Switch to next window"));

    actionPrevious_ = menu_->addAction(tr("&Previous Window"), this, &WindowMenu::onPreviousWindow);
    actionPrevious_->setShortcut(QKeySequence("Ctrl+Shift+Tab"));
    actionPrevious_->setStatusTip(tr("Switch to previous window"));

    menu_->addSeparator();

    // Layout group
    actionCascade_ = menu_->addAction(tr("&Cascade"), this, &WindowMenu::onCascade);
    actionCascade_->setStatusTip(tr("Arrange windows cascaded"));

    actionTileH_ = menu_->addAction(tr("Tile &Horizontal"), this, &WindowMenu::onTileHorizontal);
    actionTileH_->setStatusTip(tr("Arrange windows horizontally tiled"));

    actionTileV_ = menu_->addAction(tr("Tile &Vertical"), this, &WindowMenu::onTileVertical);
    actionTileV_->setStatusTip(tr("Arrange windows vertically tiled"));

    updateActionStates();
}

void WindowMenu::updateActionStates() {
    actionClose_->setEnabled(hasWindows_);
    actionCloseAll_->setEnabled(hasWindows_);
    actionNext_->setEnabled(hasMultipleWindows_);
    actionPrevious_->setEnabled(hasMultipleWindows_);
    actionCascade_->setEnabled(hasMultipleWindows_);
    actionTileH_->setEnabled(hasMultipleWindows_);
    actionTileV_->setEnabled(hasMultipleWindows_);
}

void WindowMenu::setHasWindows(bool hasWindows) {
    hasWindows_ = hasWindows;
    updateActionStates();
}

void WindowMenu::setHasMultipleWindows(bool hasMultiple) {
    hasMultipleWindows_ = hasMultiple;
    updateActionStates();
}

void WindowMenu::updateWindowList(const QStringList& windowNames, int activeIndex) {
    windowNames_ = windowNames;
    activeWindowIndex_ = activeIndex;
    rebuildWindowList();
}

void WindowMenu::rebuildWindowList() {
    // Remove old window actions
    for (QAction* action : windowActions_) {
        menu_->removeAction(action);
        delete action;
    }
    windowActions_.clear();

    // Add separator before window list if we have windows
    if (!windowNames_.isEmpty()) {
        // Find the last separator position (after layout group)
        // We'll just append at the end
        menu_->addSeparator();

        for (int i = 0; i < windowNames_.size(); ++i) {
            QString text = QString("&%1 %2").arg(i + 1).arg(windowNames_[i]);
            QAction* action = menu_->addAction(text, this, &WindowMenu::onWindowActionTriggered);
            action->setCheckable(true);
            action->setChecked(i == activeWindowIndex_);
            action->setData(i);
            windowActions_.append(action);
        }
    }
}

// Slot implementations
void WindowMenu::onNewWindow() {
    emit newWindowRequested();
}

void WindowMenu::onCloseWindow() {
    emit closeWindowRequested();
}

void WindowMenu::onCloseAllWindows() {
    emit closeAllWindowsRequested();
}

void WindowMenu::onNextWindow() {
    emit nextWindowRequested();
}

void WindowMenu::onPreviousWindow() {
    emit previousWindowRequested();
}

void WindowMenu::onCascade() {
    emit cascadeRequested();
}

void WindowMenu::onTileHorizontal() {
    emit tileHorizontalRequested();
}

void WindowMenu::onTileVertical() {
    emit tileVerticalRequested();
}

void WindowMenu::onWindowActionTriggered() {
    auto* action = qobject_cast<QAction*>(sender());
    if (action) {
        int index = action->data().toInt();
        emit windowActivated(index);
    }
}

} // namespace scratchrobin::ui
