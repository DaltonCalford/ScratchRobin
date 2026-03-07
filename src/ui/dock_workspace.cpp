#include "ui/dock_workspace.h"

#include <QMainWindow>
#include <QDockWidget>
#include <QTabWidget>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QMouseEvent>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>

namespace scratchrobin::ui {

// ============================================================================
// DockWorkspace
// ============================================================================

DockWorkspace::DockWorkspace(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent), mainWindow_(mainWindow), layoutSettings_("ScratchRobin", "Layouts") {
    
    if (mainWindow_) {
        mainWindow_->setDockNestingEnabled(true);
        mainWindow_->setDockOptions(QMainWindow::AnimatedDocks 
                                    | QMainWindow::AllowNestedDocks
                                    | QMainWindow::AllowTabbedDocks);
    }
}

DockWorkspace::~DockWorkspace() = default;

void DockWorkspace::registerPanel(const DockPanelInfo& info, QWidget* widget) {
    if (panels_.contains(info.id)) {
        qWarning() << "Panel already registered:" << info.id;
        return;
    }
    
    QDockWidget* dock = createDockWidget(info, widget);
    
    PanelData data;
    data.info = info;
    data.dock = dock;
    data.widget = widget;
    
    panels_[info.id] = data;
    
    // Add to main window in default area
    Qt::DockWidgetArea area = Qt::LeftDockWidgetArea;
    switch (info.defaultArea) {
        case DockAreaType::Left: area = Qt::LeftDockWidgetArea; break;
        case DockAreaType::Right: area = Qt::RightDockWidgetArea; break;
        case DockAreaType::Top: area = Qt::TopDockWidgetArea; break;
        case DockAreaType::Bottom: area = Qt::BottomDockWidgetArea; break;
        case DockAreaType::Center: 
            // Center area uses central widget or tabified docks
            area = Qt::NoDockWidgetArea;
            break;
        default: break;
    }
    
    if (area != Qt::NoDockWidgetArea && mainWindow_) {
        mainWindow_->addDockWidget(area, dock);
        
        // Set default size
        if (info.defaultWidth > 0 || info.defaultHeight > 0) {
            dock->resize(info.defaultWidth, info.defaultHeight);
        }
    }
    
    connectDockSignals(dock, info.id);
    
    // Set initial visibility state
    data.isVisible = (info.category == "core");
    
    // Hide by default unless it's a core panel
    if (info.category != "core") {
        dock->hide();
    } else {
        dock->show();
    }
}

void DockWorkspace::unregisterPanel(const QString& id) {
    if (!panels_.contains(id)) return;
    
    PanelData& data = panels_[id];
    if (data.dock) {
        if (mainWindow_) {
            mainWindow_->removeDockWidget(data.dock);
        }
        delete data.dock;
    }
    
    panels_.remove(id);
}

QWidget* DockWorkspace::panel(const QString& id) const {
    if (!panels_.contains(id)) return nullptr;
    return panels_[id].widget;
}

bool DockWorkspace::isPanelVisible(const QString& id) const {
    if (!panels_.contains(id)) return false;
    return panels_[id].isVisible;
}

DockAreaType DockWorkspace::panelArea(const QString& id) const {
    if (!panels_.contains(id)) return DockAreaType::Left;
    return currentArea(panels_[id].dock);
}

void DockWorkspace::showPanel(const QString& id, bool show) {
    if (!panels_.contains(id)) return;
    
    PanelData& data = panels_[id];
    QDockWidget* dock = data.dock;
    if (!dock) return;
    
    if (show) {
        dock->show();
        dock->raise();
        data.isVisible = true;
        emit panelShown(id);
    } else {
        dock->hide();
        data.isVisible = false;
        emit panelHidden(id);
    }
}

void DockWorkspace::hidePanel(const QString& id) {
    showPanel(id, false);
}

void DockWorkspace::togglePanel(const QString& id) {
    if (isPanelVisible(id)) {
        hidePanel(id);
    } else {
        showPanel(id);
    }
}

void DockWorkspace::floatPanel(const QString& id, bool floating) {
    if (!panels_.contains(id)) return;
    
    QDockWidget* dock = panels_[id].dock;
    if (!dock) return;
    
    dock->setFloating(floating);
    
    if (floating) {
        emit panelFloated(id);
    } else {
        emit panelDocked(id, currentArea(dock));
    }
}

void DockWorkspace::dockPanel(const QString& id, DockAreaType area) {
    if (!panels_.contains(id) || !mainWindow_) return;
    
    QDockWidget* dock = panels_[id].dock;
    if (!dock) return;
    
    dock->setFloating(false);
    
    Qt::DockWidgetArea qtArea = Qt::LeftDockWidgetArea;
    switch (area) {
        case DockAreaType::Left: qtArea = Qt::LeftDockWidgetArea; break;
        case DockAreaType::Right: qtArea = Qt::RightDockWidgetArea; break;
        case DockAreaType::Top: qtArea = Qt::TopDockWidgetArea; break;
        case DockAreaType::Bottom: qtArea = Qt::BottomDockWidgetArea; break;
        default: qtArea = Qt::LeftDockWidgetArea; break;
    }
    
    mainWindow_->addDockWidget(qtArea, dock);
    emit panelDocked(id, area);
}

void DockWorkspace::closePanel(const QString& id) {
    if (!panels_.contains(id)) return;
    
    const auto& info = panels_[id].info;
    if (!info.allowClose) return;
    
    hidePanel(id);
}

void DockWorkspace::setActivePanel(const QString& id) {
    if (!panels_.contains(id)) return;
    
    QDockWidget* dock = panels_[id].dock;
    if (!dock) return;
    
    dock->raise();
    dock->setFocus();
    
    if (auto* panel = qobject_cast<DockPanel*>(panels_[id].widget)) {
        panel->panelActivated();
    }
    
    activePanel_ = id;
    emit panelActivated(id);
}

QString DockWorkspace::activePanel() const {
    return activePanel_;
}

void DockWorkspace::tabPanels(const QString& panel1, const QString& panel2) {
    if (!panels_.contains(panel1) || !panels_.contains(panel2)) return;
    if (!mainWindow_) return;
    
    QDockWidget* dock1 = panels_[panel1].dock;
    QDockWidget* dock2 = panels_[panel2].dock;
    
    if (dock1 && dock2) {
        mainWindow_->tabifyDockWidget(dock1, dock2);
        panels_[panel1].isTabbed = true;
        panels_[panel2].isTabbed = true;
    }
}

void DockWorkspace::untabPanel(const QString& id) {
    if (!panels_.contains(id)) return;
    
    // Qt doesn't have direct "untab" - we need to float and re-dock
    QDockWidget* dock = panels_[id].dock;
    if (dock && dock->isVisible()) {
        dock->setFloating(true);
        dock->setFloating(false);
    }
    
    panels_[id].isTabbed = false;
}

void DockWorkspace::saveLayout(const QString& name) {
    if (!mainWindow_) return;
    
    QByteArray state = mainWindow_->saveState();
    layoutSettings_.setValue(QString("layouts/%1/state").arg(name), state);
    layoutSettings_.setValue(QString("layouts/%1/timestamp").arg(name), 
                            QDateTime::currentDateTime());
    
    emit layoutChanged(name);
}

void DockWorkspace::loadLayout(const QString& name) {
    QByteArray state = layoutSettings_.value(QString("layouts/%1/state").arg(name)).toByteArray();
    if (!state.isEmpty() && mainWindow_) {
        mainWindow_->restoreState(state);
        emit stateRestored();
    }
}

void DockWorkspace::deleteLayout(const QString& name) {
    layoutSettings_.remove(QString("layouts/%1").arg(name));
}

QStringList DockWorkspace::availableLayouts() {
    layoutSettings_.beginGroup("layouts");
    QStringList names = layoutSettings_.childGroups();
    layoutSettings_.endGroup();
    return names;
}

void DockWorkspace::resetToDefaultLayout() {
    // Hide all non-core panels
    for (auto it = panels_.begin(); it != panels_.end(); ++it) {
        if (it.value().info.category != "core") {
            hidePanel(it.key());
        }
    }
    
    // Show core panels
    for (auto it = panels_.begin(); it != panels_.end(); ++it) {
        if (it.value().info.category == "core") {
            showPanel(it.key());
            dockPanel(it.key(), it.value().info.defaultArea);
        }
    }
}

QByteArray DockWorkspace::saveState() const {
    if (!mainWindow_) return QByteArray();
    return mainWindow_->saveState();
}

bool DockWorkspace::restoreState(const QByteArray& state) {
    if (!mainWindow_) return false;
    bool result = mainWindow_->restoreState(state);
    if (result) emit stateRestored();
    return result;
}

QMenu* DockWorkspace::createViewMenu(QWidget* parent) {
    QMenu* menu = new QMenu(tr("&View"), parent);
    
    // Group panels by category
    QHash<QString, QList<QAction*>> categoryActions;
    
    for (auto it = panels_.begin(); it != panels_.end(); ++it) {
        const QString& id = it.key();
        const PanelData& data = it.value();
        
        QAction* action = new QAction(data.info.title, menu);
        action->setCheckable(true);
        action->setChecked(isPanelVisible(id));
        action->setShortcut(QKeySequence(data.info.shortcut));
        
        connect(action, &QAction::triggered, [this, id](bool checked) {
            showPanel(id, checked);
        });
        
        categoryActions[data.info.category].append(action);
    }
    
    // Add actions grouped by category
    for (const QString& category : categoryActions.keys()) {
        menu->addSection(category.at(0).toUpper() + category.mid(1));
        for (QAction* action : categoryActions[category]) {
            menu->addAction(action);
        }
    }
    
    return menu;
}

QList<QAction*> DockWorkspace::panelActions() {
    QList<QAction*> actions;
    
    for (auto it = panels_.begin(); it != panels_.end(); ++it) {
        const QString& id = it.key();
        const PanelData& data = it.value();
        
        QAction* action = new QAction(data.info.title);
        action->setCheckable(true);
        action->setChecked(isPanelVisible(id));
        
        connect(action, &QAction::triggered, [this, id](bool checked) {
            showPanel(id, checked);
        });
        
        actions.append(action);
    }
    
    return actions;
}

void DockWorkspace::setAutoHide(const QString& id, bool autoHide) {
    Q_UNUSED(id)
    Q_UNUSED(autoHide)
    // Implementation would use DockAutoHidePanel
}

bool DockWorkspace::isAutoHide(const QString& id) const {
    Q_UNUSED(id)
    return false;
}

void DockWorkspace::lockLayout(bool locked) {
    layoutLocked_ = locked;
    
    for (auto& data : panels_) {
        if (data.dock) {
            data.dock->setFeatures(locked ? QDockWidget::NoDockWidgetFeatures 
                                          : (QDockWidget::DockWidgetClosable 
                                             | QDockWidget::DockWidgetMovable 
                                             | QDockWidget::DockWidgetFloatable));
        }
    }
}

bool DockWorkspace::isLayoutLocked() const {
    return layoutLocked_;
}

QDockWidget* DockWorkspace::createDockWidget(const DockPanelInfo& info, QWidget* widget) {
    QDockWidget* dock = new QDockWidget(info.title, mainWindow_);
    dock->setWidget(widget);
    dock->setObjectName(info.id);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setFeatures(info.allowFloating ? QDockWidget::DockWidgetFloatable 
                                         : QDockWidget::NoDockWidgetFeatures);
    
    if (!info.allowClose) {
        dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetClosable);
    }
    
    return dock;
}

DockAreaType DockWorkspace::currentArea(QDockWidget* dock) const {
    if (!dock || !mainWindow_) return DockAreaType::Left;
    
    Qt::DockWidgetArea area = mainWindow_->dockWidgetArea(dock);
    
    switch (area) {
        case Qt::LeftDockWidgetArea: return DockAreaType::Left;
        case Qt::RightDockWidgetArea: return DockAreaType::Right;
        case Qt::TopDockWidgetArea: return DockAreaType::Top;
        case Qt::BottomDockWidgetArea: return DockAreaType::Bottom;
        default: return dock->isFloating() ? DockAreaType::Floating : DockAreaType::Center;
    }
}

void DockWorkspace::connectDockSignals(QDockWidget* dock, const QString& id) {
    connect(dock, &QDockWidget::visibilityChanged, [this, id](bool visible) {
        if (panels_.contains(id)) {
            panels_[id].isVisible = visible;
        }
        if (visible) {
            emit panelShown(id);
        } else {
            emit panelHidden(id);
        }
    });
    
    connect(dock, &QDockWidget::topLevelChanged, [this, id](bool floating) {
        if (floating) {
            emit panelFloated(id);
        } else {
            emit panelDocked(id, panelArea(id));
        }
    });
}

// ============================================================================
// DockPanel
// ============================================================================

DockPanel::DockPanel(const QString& panelId, QWidget* parent)
    : QWidget(parent), panelId_(panelId) {
}

void DockPanel::panelShown() {
}

void DockPanel::panelHidden() {
}

void DockPanel::panelActivated() {
}

void DockPanel::panelDeactivated() {
}

void DockPanel::saveState(QSettings& settings) const {
    Q_UNUSED(settings)
}

void DockPanel::restoreState(QSettings& settings) {
    Q_UNUSED(settings)
}

void DockPanel::setPanelTitle(const QString& title) {
    title_ = title;
    emit titleChanged(title);
}

// ============================================================================
// DockLayoutManager
// ============================================================================

DockLayoutManager::DockLayoutManager(QObject* parent)
    : QObject(parent), settings_("ScratchRobin", "DockLayouts") {
}

void DockLayoutManager::saveLayout(const QString& name, const QByteArray& state,
                                   const QString& description) {
    settings_.beginGroup("layouts");
    settings_.beginGroup(name);
    settings_.setValue("state", state);
    settings_.setValue("description", description);
    settings_.setValue("timestamp", QDateTime::currentDateTime());
    settings_.endGroup();
    settings_.endGroup();
}

QByteArray DockLayoutManager::loadLayout(const QString& name) const {
    return settings_.value(QString("layouts/%1/state").arg(name)).toByteArray();
}

void DockLayoutManager::deleteLayout(const QString& name) {
    settings_.remove(QString("layouts/%1").arg(name));
}

QStringList DockLayoutManager::layoutNames() {
    settings_.beginGroup("layouts");
    QStringList names = settings_.childGroups();
    settings_.endGroup();
    return names;
}

QList<DockLayout> DockLayoutManager::availableLayouts() {
    QList<DockLayout> layouts;
    
    settings_.beginGroup("layouts");
    for (const QString& name : settings_.childGroups()) {
        settings_.beginGroup(name);
        
        DockLayout layout;
        layout.name = name;
        layout.description = settings_.value("description").toString();
        layout.state = settings_.value("state").toByteArray();
        layout.created = settings_.value("timestamp").toDateTime();
        layout.isDefault = (name == settings_.value("default").toString());
        
        layouts.append(layout);
        settings_.endGroup();
    }
    settings_.endGroup();
    
    return layouts;
}

void DockLayoutManager::setDefaultLayout(const QString& name) {
    settings_.setValue("default", name);
}

QString DockLayoutManager::defaultLayout() const {
    return settings_.value("default").toString();
}

void DockLayoutManager::exportLayout(const QString& name, const QString& fileName) {
    QByteArray state = loadLayout(name);
    if (state.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(state);
        file.close();
    }
}

bool DockLayoutManager::importLayout(const QString& fileName, QString* name) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray state = file.readAll();
    file.close();
    
    if (state.isEmpty()) return false;
    
    QString layoutName = name ? *name : QFileInfo(fileName).baseName();
    saveLayout(layoutName, state, QString("Imported from %1").arg(fileName));
    
    if (name) *name = layoutName;
    return true;
}

// ============================================================================
// DockAutoHidePanel
// ============================================================================

DockAutoHidePanel::DockAutoHidePanel(QWidget* content, Qt::Edge edge, QWidget* parent)
    : QWidget(parent), content_(content), edge_(edge) {
    setupUi();
}

void DockAutoHidePanel::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Tab button
    tabLabel_ = new QLabel(this);
    tabLabel_->setAlignment(Qt::AlignCenter);
    tabLabel_->setFixedWidth(collapsedSize_);
    
    // Content container
    auto* contentContainer = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(content_);
    
    // Pin button
    pinButton_ = new QToolButton(this);
    pinButton_->setText("📌");
    pinButton_->setCheckable(true);
    connect(pinButton_, &QToolButton::clicked, this, &DockAutoHidePanel::toggle);
    
    contentLayout->addWidget(pinButton_, 0, Qt::AlignRight);
    
    // Arrange based on edge
    if (edge_ == Qt::LeftEdge) {
        layout->addWidget(tabLabel_);
        layout->addWidget(contentContainer);
    } else if (edge_ == Qt::RightEdge) {
        layout->addWidget(contentContainer);
        layout->addWidget(tabLabel_);
    }
    
    contentContainer->hide();
    
    // Collapse timer
    collapseTimer_ = new QTimer(this);
    collapseTimer_->setSingleShot(true);
    collapseTimer_->setInterval(500);
    connect(collapseTimer_, &QTimer::timeout, this, &DockAutoHidePanel::collapse);
}

void DockAutoHidePanel::setTabTitle(const QString& title) {
    tabLabel_->setText(title);
}

void DockAutoHidePanel::setTabIcon(const QIcon& icon) {
    Q_UNUSED(icon)
    // Implementation would set icon
}

bool DockAutoHidePanel::isExpanded() const {
    return expanded_;
}

void DockAutoHidePanel::expand() {
    if (expanded_) return;
    
    expanded_ = true;
    // Show content, resize
    findChild<QWidget*>()->show();
    emit expanded();
    
    collapseTimer_->stop();
}

void DockAutoHidePanel::collapse() {
    if (!expanded_) return;
    if (pinButton_->isChecked()) return;  // Don't collapse if pinned
    
    expanded_ = false;
    // Hide content, resize back
    findChild<QWidget*>()->hide();
    emit collapsed();
}

void DockAutoHidePanel::toggle() {
    if (expanded_) {
        collapse();
    } else {
        expand();
    }
}

void DockAutoHidePanel::enterEvent(QEnterEvent* event) {
    QWidget::enterEvent(event);
    expand();
    collapseTimer_->stop();
}

void DockAutoHidePanel::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    collapseTimer_->start();
}

} // namespace scratchrobin::ui
