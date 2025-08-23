#include "result_viewer.h"
#include "execution/sql_executor.h"

namespace scratchrobin {

class ResultViewer::Impl {};

ResultViewer::ResultViewer()
    : impl_(std::make_unique<Impl>()) {
}

ResultViewer::~ResultViewer() = default;

void ResultViewer::setResults(const QueryResult& results) {
    // TODO: Implement result setting
}

void ResultViewer::clearResults() {
    // TODO: Implement result clearing
}

void ResultViewer::exportResults(const std::string& format, const std::string& filename) {
    // TODO: Implement result export
}

} // namespace scratchrobin