#pragma once
#include <QObject>
#include <QHash>
#include <QList>
#include <QByteArray>
#include <QSettings>
#include <QWidget>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QDockWidget;
class QAction;
class QMenu;
class QTimer;
class QEnterEvent;
class QEvent;
class QIcon;
class QLabel;
class QToolButton;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Dockable Workspace Architecture
 * 
 * Implements full docking model from specifications:
 * - Multiple dockable panels
 * - Tabbed docking areas
 * - Floating windows
 * - Layout save/restore
 * - Dock areas: Left, Right, Top, Bottom, Center (tabbed)
 */

class DockPanel;
class DockArea;

// ============================================================================
// Dock Area Types
// ============================================================================
enum class DockAreaType {
    Left,       // Left sidebar (navigator, file explorer)
    Right,      // Right sidebar (properties, help)
    Top,        // Top toolbar area
    Bottom,     // Bottom panel (results, messages)
    Center,     // Central tabbed area (editors, viewers)
    Floating    // Detached floating window
};

// ============================================================================
// Dock Panel Info
// ============================================================================
struct DockPanelInfo {
    QString id;
    QString title;
    QString category;  // navigator, editor, results, tools, etc.
    DockAreaType defaultArea;
    bool allowFloating = true;
    bool allowClose = true;
    bool allowTabbed = true;
    int defaultWidth = 250;
    int defaultHeight = 200;
    QString iconName;
    QString shortcut;
};

// ============================================================================
// Dock Workspace Manager
// ============================================================================
class DockWorkspace : public QObject {
    Q_OBJECT

public:
    explicit DockWorkspace(QMainWindow* mainWindow, QObject* parent = nullptr);
    ~DockWorkspace() override;

    // Panel registration
    void registerPanel(const DockPanelInfo& info, QWidget* widget);
    void unregisterPanel(const QString& id);
    
    // Panel access
    QWidget* panel(const QString& id) const;
    bool isPanelVisible(const QString& id) const;
    DockAreaType panelArea(const QString& id) const;
    
    // Panel control
    void showPanel(const QString& id, bool show = true);
    void hidePanel(const QString& id);
    void togglePanel(const QString& id);
    void floatPanel(const QString& id, bool floating = true);
    void dockPanel(const QString& id, DockAreaType area);
    void closePanel(const QString& id);
    
    // Focus management
    void setActivePanel(const QString& id);
    QString activePanel() const;
    
    // Tab management
    void tabPanels(const QString& panel1, const QString& panel2);
    void untabPanel(const QString& panel);
    
    // Layout management
    void saveLayout(const QString& name);
    void loadLayout(const QString& name);
    void deleteLayout(const QString& name);
    QStringList availableLayouts();
    void resetToDefaultLayout();
    
    // Global layout state
    QByteArray saveState() const;
    bool restoreState(const QByteArray& state);
    
    // Menu generation
    QMenu* createViewMenu(QWidget* parent = nullptr);
    QList<QAction*> panelActions();
    
    // Auto-hide panels
    void setAutoHide(const QString& id, bool autoHide);
    bool isAutoHide(const QString& id) const;
    
    // Panel state
    void lockLayout(bool locked);
    bool isLayoutLocked() const;

signals:
    void panelShown(const QString& id);
    void panelHidden(const QString& id);
    void panelFloated(const QString& id);
    void panelDocked(const QString& id, DockAreaType area);
    void panelActivated(const QString& id);
    void layoutChanged(const QString& layoutName);
    void stateRestored();

private:
    void setupDefaultAreas();
    QDockWidget* createDockWidget(const DockPanelInfo& info, QWidget* widget);
    DockAreaType currentArea(QDockWidget* dock) const;
    void connectDockSignals(QDockWidget* dock, const QString& id);
    
    QMainWindow* mainWindow_;
    
    struct PanelData {
        DockPanelInfo info;
        QDockWidget* dock = nullptr;
        QWidget* widget = nullptr;
        bool isTabbed = false;
        bool isVisible = false;  // Track visibility state explicitly
    };
    QHash<QString, PanelData> panels_;
    
    QString activePanel_;
    bool layoutLocked_ = false;
    
    // Layout storage
    QSettings layoutSettings_;
};

// ============================================================================
// Dock Panel Base Class
// ============================================================================
class DockPanel : public QWidget {
    Q_OBJECT

public:
    explicit DockPanel(const QString& panelId, QWidget* parent = nullptr);
    
    QString panelId() const { return panelId_; }
    virtual QString panelTitle() const { return panelId_; }
    virtual QString panelCategory() const { return "general"; }
    
    // Panel lifecycle
    virtual void panelShown();
    virtual void panelHidden();
    virtual void panelActivated();
    virtual void panelDeactivated();
    
    // Persistence
    virtual void saveState(QSettings& settings) const;
    virtual void restoreState(QSettings& settings);
    
    // Toolbar
    virtual QList<QAction*> panelActions() const { return {}; }
    
signals:
    void titleChanged(const QString& title);
    void statusMessage(const QString& message);
    void closeRequested();

protected:
    void setPanelTitle(const QString& title);
    
private:
    QString panelId_;
    QString title_;
};

// ============================================================================
// Dock Layout
// ============================================================================
struct DockLayout {
    QString name;
    QString description;
    QByteArray state;
    QDateTime created;
    bool isDefault = false;
};

class DockLayoutManager : public QObject {
    Q_OBJECT

public:
    explicit DockLayoutManager(QObject* parent = nullptr);
    
    void saveLayout(const QString& name, const QByteArray& state, 
                    const QString& description = QString());
    QByteArray loadLayout(const QString& name) const;
    void deleteLayout(const QString& name);
    QStringList layoutNames();
    QList<DockLayout> availableLayouts();
    void setDefaultLayout(const QString& name);
    QString defaultLayout() const;
    void exportLayout(const QString& name, const QString& fileName);
    bool importLayout(const QString& fileName, QString* name = nullptr);

private:
    QSettings settings_;
};

// ============================================================================
// Dock Auto-Hide Panel
// ============================================================================
class DockAutoHidePanel : public QWidget {
    Q_OBJECT

public:
    explicit DockAutoHidePanel(QWidget* content, Qt::Edge edge, QWidget* parent = nullptr);
    
    void setTabTitle(const QString& title);
    void setTabIcon(const QIcon& icon);
    
    bool isExpanded() const;

signals:
    void expanded();
    void collapsed();

public slots:
    void expand();
    void collapse();
    void toggle();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    void setupUi();
    void animateExpansion(bool expand);
    
    QWidget* content_;
    Qt::Edge edge_;
    bool expanded_ = false;
    int collapsedSize_ = 24;
    int expandedSize_ = 250;
    
    QLabel* tabLabel_ = nullptr;
    QToolButton* pinButton_ = nullptr;
    QTimer* collapseTimer_ = nullptr;
};

} // namespace scratchrobin::ui
