#pragma once

#include <QString>
#include <QByteArray>
#include <QMap>
#include <QRect>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QDockWidget;
class QByteArray;
QT_END_NAMESPACE

namespace scratchrobin::core {

struct WindowState {
    QString name;
    QRect geometry;
    bool maximized = false;
    bool visible = true;
    bool floating = false;
    QByteArray dockState;
    
    QVariantMap toMap() const;
    static WindowState fromMap(const QVariantMap& map);
};

struct DockState {
    QString name;
    QString area; // "left", "right", "top", "bottom", "floating"
    bool visible = true;
    bool floating = false;
    QRect geometry;
    bool closed = false;
};

class WindowStateManager {
public:
    static WindowStateManager& instance();
    
    // Save/Load main window state
    void saveMainWindow(QMainWindow* window, const QString& name = "main");
    bool loadMainWindow(QMainWindow* window, const QString& name = "main");
    
    // Save/Load dock widget state
    void saveDockWidget(QDockWidget* dock, const QString& name);
    bool loadDockWidget(QDockWidget* dock, const QString& name);
    
    // Save/Load all dock widgets for a main window
    void saveAllDockWidgets(QMainWindow* window);
    void loadAllDockWidgets(QMainWindow* window);
    
    // Complete save/load
    void saveWindowLayout(QMainWindow* window, const QString& layoutName = "default");
    bool loadWindowLayout(QMainWindow* window, const QString& layoutName = "default");
    
    // Reset to defaults
    void resetLayout(const QString& layoutName = "default");
    
    // Available layouts
    QStringList availableLayouts() const;
    void deleteLayout(const QString& layoutName);
    
private:
    WindowStateManager() = default;
    ~WindowStateManager() = default;
    WindowStateManager(const WindowStateManager&) = delete;
    WindowStateManager& operator=(const WindowStateManager&) = delete;
    
    QString stateFilePath() const;
    void ensureStateDirExists() const;
    
    QVariantMap loadStateFile() const;
    void saveStateFile(const QVariantMap& state);
    
    static constexpr const char* STATE_FILENAME = "window_layouts.json";
};

} // namespace scratchrobin::core
