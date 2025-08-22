#ifndef SCRATCHROBIN_MAIN_WINDOW_H
#define SCRATCHROBIN_MAIN_WINDOW_H

#include <memory>

namespace scratchrobin {

class Application;

class MainWindow {
public:
    explicit MainWindow(Application* application);
    ~MainWindow();

    void show();
    void hide();
    bool isVisible() const;
    void close();

    // Window management
    void setTitle(const std::string& title);
    std::string getTitle() const;

    void setSize(int width, int height);
    void getSize(int& width, int& height) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy and assignment
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MAIN_WINDOW_H
