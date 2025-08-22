#ifndef SCRATCHROBIN_QUERY_EDITOR_H
#define SCRATCHROBIN_QUERY_EDITOR_H

#include <memory>
#include <string>

namespace scratchrobin {

class QueryEditor {
public:
    QueryEditor();
    ~QueryEditor();

    void setText(const std::string& text);
    std::string getText() const;
    void executeQuery();
    void formatSQL();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_QUERY_EDITOR_H