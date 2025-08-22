#ifndef SCRATCHROBIN_CONNECTION_DIALOG_H
#define SCRATCHROBIN_CONNECTION_DIALOG_H

#include <memory>

namespace scratchrobin {

class ConnectionDialog {
public:
    ConnectionDialog();
    ~ConnectionDialog();

    void show();
    void hide();
    bool isVisible() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_DIALOG_H
