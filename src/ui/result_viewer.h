#ifndef SCRATCHROBIN_RESULT_VIEWER_H
#define SCRATCHROBIN_RESULT_VIEWER_H

#include <memory>
#include <string>
#include "types/database_types.h"

namespace scratchrobin {

class ResultViewer {
public:
    ResultViewer();
    ~ResultViewer();

    void setResults(const QueryResult& results);
    void clearResults();
    void exportResults(const std::string& format, const std::string& filename);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_RESULT_VIEWER_H
