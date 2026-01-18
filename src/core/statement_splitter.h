#ifndef SCRATCHROBIN_STATEMENT_SPLITTER_H
#define SCRATCHROBIN_STATEMENT_SPLITTER_H

#include <string>
#include <vector>

namespace scratchrobin {

class StatementSplitter {
public:
    struct SplitResult {
        std::vector<std::string> statements;
        std::string delimiter = ";";
    };

    SplitResult Split(const std::string& input) const;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_STATEMENT_SPLITTER_H
