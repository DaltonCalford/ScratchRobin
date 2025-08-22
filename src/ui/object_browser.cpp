#include "object_browser.h"

namespace scratchrobin {

class ObjectBrowser::Impl {};

ObjectBrowser::ObjectBrowser()
    : impl_(std::make_unique<Impl>()) {
}

ObjectBrowser::~ObjectBrowser() = default;

void ObjectBrowser::refresh() {
    // TODO: Implement refresh
}

void ObjectBrowser::expandNode(const std::string& path) {
    // TODO: Implement node expansion
}

void ObjectBrowser::selectObject(const std::string& objectPath) {
    // TODO: Implement object selection
}

} // namespace scratchrobin