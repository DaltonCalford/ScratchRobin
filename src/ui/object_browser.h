#ifndef SCRATCHROBIN_OBJECT_BROWSER_H
#define SCRATCHROBIN_OBJECT_BROWSER_H

#include <memory>
#include <string>

namespace scratchrobin {

class ObjectBrowser {
public:
    ObjectBrowser();
    ~ObjectBrowser();

    void refresh();
    void expandNode(const std::string& path);
    void selectObject(const std::string& objectPath);

    QWidget* getWidget();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_OBJECT_BROWSER_H
