#include "core/window_state_manager.h"

#include <QMainWindow>
#include <QDockWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

namespace scratchrobin::core {

QVariantMap WindowState::toMap() const {
    QVariantMap map;
    map["name"] = name;
    map["x"] = geometry.x();
    map["y"] = geometry.y();
    map["width"] = geometry.width();
    map["height"] = geometry.height();
    map["maximized"] = maximized;
    map["visible"] = visible;
    map["floating"] = floating;
    map["dockState"] = QString::fromLatin1(dockState.toBase64());
    return map;
}

WindowState WindowState::fromMap(const QVariantMap& map) {
    WindowState state;
    state.name = map["name"].toString();
    state.geometry = QRect(
        map["x"].toInt(),
        map["y"].toInt(),
        map["width"].toInt(),
        map["height"].toInt()
    );
    state.maximized = map["maximized"].toBool();
    state.visible = map["visible"].toBool();
    state.floating = map["floating"].toBool();
    state.dockState = QByteArray::fromBase64(map["dockState"].toString().toLatin1());
    return state;
}

WindowStateManager& WindowStateManager::instance() {
    static WindowStateManager instance;
    return instance;
}

void WindowStateManager::saveMainWindow(QMainWindow* window, const QString& name) {
    if (!window) return;
    
    WindowState state;
    state.name = name;
    state.geometry = window->geometry();
    state.maximized = window->isMaximized();
    
    // Save dock state if it's a main window with docks
    state.dockState = window->saveState();
    
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    auto currentLayout = layouts["current"].toString();
    if (currentLayout.isEmpty()) currentLayout = "default";
    
    auto layoutData = layouts[currentLayout].toMap();
    auto windows = layoutData["windows"].toMap();
    windows[name] = state.toMap();
    layoutData["windows"] = windows;
    layouts[currentLayout] = layoutData;
    fileState["layouts"] = layouts;
    
    saveStateFile(fileState);
}

bool WindowStateManager::loadMainWindow(QMainWindow* window, const QString& name) {
    if (!window) return false;
    
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    auto currentLayout = layouts["current"].toString();
    if (currentLayout.isEmpty()) currentLayout = "default";
    
    auto layoutData = layouts[currentLayout].toMap();
    auto windows = layoutData["windows"].toMap();
    
    if (!windows.contains(name)) return false;
    
    auto state = WindowState::fromMap(windows[name].toMap());
    
    if (state.maximized) {
        window->showMaximized();
    } else {
        window->setGeometry(state.geometry);
    }
    
    if (!state.dockState.isEmpty()) {
        window->restoreState(state.dockState);
    }
    
    return true;
}

void WindowStateManager::saveDockWidget(QDockWidget* dock, const QString& name) {
    if (!dock) return;
    
    QVariantMap dockMap;
    dockMap["name"] = name;
    dockMap["visible"] = dock->isVisible();
    dockMap["floating"] = dock->isFloating();
    dockMap["x"] = dock->geometry().x();
    dockMap["y"] = dock->geometry().y();
    dockMap["width"] = dock->geometry().width();
    dockMap["height"] = dock->geometry().height();
    
    // Determine dock area
    QString area = "floating";
    if (!dock->isFloating()) {
        if (auto* mainWin = qobject_cast<QMainWindow*>(dock->parentWidget())) {
            auto dockArea = mainWin->dockWidgetArea(dock);
            switch (dockArea) {
                case Qt::LeftDockWidgetArea: area = "left"; break;
                case Qt::RightDockWidgetArea: area = "right"; break;
                case Qt::TopDockWidgetArea: area = "top"; break;
                case Qt::BottomDockWidgetArea: area = "bottom"; break;
                default: area = "unknown"; break;
            }
        }
    }
    dockMap["area"] = area;
    
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    auto currentLayout = layouts["current"].toString();
    if (currentLayout.isEmpty()) currentLayout = "default";
    
    auto layoutData = layouts[currentLayout].toMap();
    auto docks = layoutData["docks"].toMap();
    docks[name] = dockMap;
    layoutData["docks"] = docks;
    layouts[currentLayout] = layoutData;
    fileState["layouts"] = layouts;
    
    saveStateFile(fileState);
}

bool WindowStateManager::loadDockWidget(QDockWidget* dock, const QString& name) {
    if (!dock) return false;
    
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    auto currentLayout = layouts["current"].toString();
    if (currentLayout.isEmpty()) currentLayout = "default";
    
    auto layoutData = layouts[currentLayout].toMap();
    auto docks = layoutData["docks"].toMap();
    
    if (!docks.contains(name)) return false;
    
    auto dockMap = docks[name].toMap();
    
    dock->setVisible(dockMap["visible"].toBool());
    
    if (dockMap["floating"].toBool()) {
        dock->setFloating(true);
        QRect geom(dockMap["x"].toInt(), dockMap["y"].toInt(),
                   dockMap["width"].toInt(), dockMap["height"].toInt());
        dock->setGeometry(geom);
    } else {
        // Restore to dock area
        QString area = dockMap["area"].toString();
        Qt::DockWidgetArea dockArea = Qt::LeftDockWidgetArea;
        if (area == "right") dockArea = Qt::RightDockWidgetArea;
        else if (area == "top") dockArea = Qt::TopDockWidgetArea;
        else if (area == "bottom") dockArea = Qt::BottomDockWidgetArea;
        
        if (auto* mainWin = qobject_cast<QMainWindow*>(dock->parentWidget())) {
            mainWin->addDockWidget(dockArea, dock);
        }
    }
    
    return true;
}

void WindowStateManager::saveAllDockWidgets(QMainWindow* window) {
    if (!window) return;
    
    for (auto* dock : window->findChildren<QDockWidget*>()) {
        if (!dock->objectName().isEmpty()) {
            saveDockWidget(dock, dock->objectName());
        }
    }
}

void WindowStateManager::loadAllDockWidgets(QMainWindow* window) {
    if (!window) return;
    
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    auto currentLayout = layouts["current"].toString();
    if (currentLayout.isEmpty()) currentLayout = "default";
    
    auto layoutData = layouts[currentLayout].toMap();
    auto docks = layoutData["docks"].toMap();
    
    for (auto* dock : window->findChildren<QDockWidget*>()) {
        if (!dock->objectName().isEmpty() && docks.contains(dock->objectName())) {
            loadDockWidget(dock, dock->objectName());
        }
    }
}

void WindowStateManager::saveWindowLayout(QMainWindow* window, const QString& layoutName) {
    if (!window) return;
    
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    
    QVariantMap layoutData;
    
    // Save main window
    WindowState mainState;
    mainState.name = "main";
    mainState.geometry = window->geometry();
    mainState.maximized = window->isMaximized();
    mainState.dockState = window->saveState();
    
    QVariantMap windows;
    windows["main"] = mainState.toMap();
    layoutData["windows"] = windows;
    
    // Save all dock widgets
    QVariantMap docks;
    for (auto* dock : window->findChildren<QDockWidget*>()) {
        if (!dock->objectName().isEmpty()) {
            QVariantMap dockMap;
            dockMap["name"] = dock->objectName();
            dockMap["visible"] = dock->isVisible();
            dockMap["floating"] = dock->isFloating();
            dockMap["x"] = dock->geometry().x();
            dockMap["y"] = dock->geometry().y();
            dockMap["width"] = dock->geometry().width();
            dockMap["height"] = dock->geometry().height();
            dockMap["closed"] = !dock->isVisible(); // Track if explicitly closed
            
            // Determine dock area
            QString area = "floating";
            if (!dock->isFloating()) {
                auto dockArea = window->dockWidgetArea(dock);
                switch (dockArea) {
                    case Qt::LeftDockWidgetArea: area = "left"; break;
                    case Qt::RightDockWidgetArea: area = "right"; break;
                    case Qt::TopDockWidgetArea: area = "top"; break;
                    case Qt::BottomDockWidgetArea: area = "bottom"; break;
                    default: area = "unknown"; break;
                }
            }
            dockMap["area"] = area;
            docks[dock->objectName()] = dockMap;
        }
    }
    layoutData["docks"] = docks;
    layoutData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    layouts[layoutName] = layoutData;
    fileState["layouts"] = layouts;
    fileState["current"] = layoutName;
    
    saveStateFile(fileState);
}

bool WindowStateManager::loadWindowLayout(QMainWindow* window, const QString& layoutName) {
    if (!window) return false;
    
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    
    if (!layouts.contains(layoutName)) return false;
    
    auto layoutData = layouts[layoutName].toMap();
    
    // Load main window state
    auto windows = layoutData["windows"].toMap();
    if (windows.contains("main")) {
        auto mainState = WindowState::fromMap(windows["main"].toMap());
        
        if (mainState.maximized) {
            window->showMaximized();
        } else {
            window->setGeometry(mainState.geometry);
        }
        
        if (!mainState.dockState.isEmpty()) {
            window->restoreState(mainState.dockState);
        }
    }
    
    // Load dock widget states
    auto docks = layoutData["docks"].toMap();
    for (auto* dock : window->findChildren<QDockWidget*>()) {
        if (!dock->objectName().isEmpty() && docks.contains(dock->objectName())) {
            auto dockMap = docks[dock->objectName()].toMap();
            
            bool wasClosed = dockMap["closed"].toBool();
            if (wasClosed) {
                dock->close();
            } else {
                dock->setVisible(dockMap["visible"].toBool());
                
                if (dockMap["floating"].toBool()) {
                    dock->setFloating(true);
                    QRect geom(dockMap["x"].toInt(), dockMap["y"].toInt(),
                               dockMap["width"].toInt(), dockMap["height"].toInt());
                    dock->setGeometry(geom);
                }
            }
        }
    }
    
    fileState["current"] = layoutName;
    saveStateFile(fileState);
    
    return true;
}

void WindowStateManager::resetLayout(const QString& layoutName) {
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    layouts.remove(layoutName);
    fileState["layouts"] = layouts;
    saveStateFile(fileState);
}

QStringList WindowStateManager::availableLayouts() const {
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    return layouts.keys();
}

void WindowStateManager::deleteLayout(const QString& layoutName) {
    auto fileState = loadStateFile();
    auto layouts = fileState["layouts"].toMap();
    layouts.remove(layoutName);
    fileState["layouts"] = layouts;
    saveStateFile(fileState);
}

QString WindowStateManager::stateFilePath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return configDir + "/" + STATE_FILENAME;
}

void WindowStateManager::ensureStateDirExists() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QVariantMap WindowStateManager::loadStateFile() const {
    ensureStateDirExists();
    QString path = stateFilePath();
    
    QFile file(path);
    if (!file.exists()) {
        return QVariantMap();
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open window state file:" << path;
        return QVariantMap();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return QVariantMap();
    }
    
    return doc.object().toVariantMap();
}

void WindowStateManager::saveStateFile(const QVariantMap& state) {
    ensureStateDirExists();
    QString path = stateFilePath();
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save window state file:" << path;
        return;
    }
    
    QJsonDocument doc(QJsonObject::fromVariantMap(state));
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

} // namespace scratchrobin::core
