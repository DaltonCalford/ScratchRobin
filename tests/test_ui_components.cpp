/**
 * @file test_ui_components.cpp
 * @brief Unit tests for UI components
 * 
 * Tests the DockWorkspace and panel management functionality.
 */

#include <QtTest/QtTest>
#include <QObject>
#include <QMainWindow>
#include <QWidget>

#include "ui/dock_workspace.h"

using namespace scratchrobin::ui;

class TestUiComponents : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // DockWorkspace tests
    void testDockWorkspaceCreation();
    void testPanelRegistration();
    void testPanelShowHide();
    void testLayoutSaveRestore();
    
    // DockPanel tests
    void testDockPanelLifecycle();
    void testDockPanelPersistence();

private:
    QMainWindow* mainWindow_ = nullptr;
    DockWorkspace* dockWorkspace_ = nullptr;
};

void TestUiComponents::initTestCase() {
    mainWindow_ = new QMainWindow();
    dockWorkspace_ = new DockWorkspace(mainWindow_);
}

void TestUiComponents::cleanupTestCase() {
    delete dockWorkspace_;
    delete mainWindow_;
}

void TestUiComponents::testDockWorkspaceCreation() {
    QVERIFY(dockWorkspace_ != nullptr);
}

void TestUiComponents::testPanelRegistration() {
    QWidget* testWidget = new QWidget();
    
    DockPanelInfo info;
    info.id = "test_panel";
    info.title = "Test Panel";
    info.category = "test";
    info.defaultArea = DockAreaType::Left;
    
    dockWorkspace_->registerPanel(info, testWidget);
    
    QWidget* retrieved = dockWorkspace_->panel("test_panel");
    QVERIFY(retrieved == testWidget);
}

void TestUiComponents::testPanelShowHide() {
    QWidget* testWidget = new QWidget();
    
    DockPanelInfo info;
    info.id = "visibility_test";
    info.title = "Visibility Test";
    info.category = "test";
    info.defaultArea = DockAreaType::Right;
    
    dockWorkspace_->registerPanel(info, testWidget);
    
    // Initially not visible until shown
    QVERIFY(!dockWorkspace_->isPanelVisible("visibility_test"));
    
    // Show panel
    dockWorkspace_->showPanel("visibility_test");
    QVERIFY(dockWorkspace_->isPanelVisible("visibility_test"));
    
    // Hide panel
    dockWorkspace_->hidePanel("visibility_test");
    QVERIFY(!dockWorkspace_->isPanelVisible("visibility_test"));
    
    // Toggle panel
    dockWorkspace_->togglePanel("visibility_test");
    QVERIFY(dockWorkspace_->isPanelVisible("visibility_test"));
}

void TestUiComponents::testLayoutSaveRestore() {
    // Save current state
    QByteArray savedState = dockWorkspace_->saveState();
    QVERIFY(!savedState.isEmpty());
    
    // Restore state
    bool restored = dockWorkspace_->restoreState(savedState);
    QVERIFY(restored);
}

void TestUiComponents::testDockPanelLifecycle() {
    // Test panel lifecycle events
    class TestPanel : public DockPanel {
    public:
        TestPanel() : DockPanel("test_lifecycle") {}
        bool shown = false;
        bool hidden = false;
        bool activated = false;
        
        // Expose protected methods for testing
        void callPanelShown() { panelShown(); }
        void callPanelHidden() { panelHidden(); }
        void callPanelActivated() { panelActivated(); }
        
    protected:
        void panelShown() override { shown = true; }
        void panelHidden() override { hidden = true; }
        void panelActivated() override { activated = true; }
    };
    
    TestPanel* panel = new TestPanel();
    
    // Simulate lifecycle
    panel->callPanelShown();
    QVERIFY(panel->shown);
    
    panel->callPanelActivated();
    QVERIFY(panel->activated);
    
    panel->callPanelHidden();
    QVERIFY(panel->hidden);
    
    delete panel;
}

void TestUiComponents::testDockPanelPersistence() {
    // Test settings persistence
    class TestPanel : public DockPanel {
    public:
        TestPanel() : DockPanel("test_persistence") {}
        QString savedData;
        
        void saveState(QSettings& settings) const override {
            settings.setValue("test_data", "saved_value");
        }
        
        void restoreState(QSettings& settings) override {
            savedData = settings.value("test_data").toString();
        }
    };
    
    TestPanel* panel = new TestPanel();
    
    QSettings testSettings("Test", "Persistence");
    panel->saveState(testSettings);
    panel->restoreState(testSettings);
    
    QCOMPARE(panel->savedData, QString("saved_value"));
    
    delete panel;
}

QTEST_MAIN(TestUiComponents)
#include "test_ui_components.moc"
