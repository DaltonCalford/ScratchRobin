#ifndef SCRATCHROBIN_DIAGRAM_MODEL_H
#define SCRATCHROBIN_DIAGRAM_MODEL_H

#include <string>
#include <vector>

namespace scratchrobin {

enum class DiagramType {
    Erd,
    Silverston
};

enum class Cardinality {
    One,
    ZeroOrOne,
    OneOrMany,
    ZeroOrMany
};

struct DiagramAttribute {
    std::string name;
    std::string data_type;
    bool is_primary = false;
    bool is_foreign = false;
};

struct DiagramNode {
    std::string id;
    std::string name;
    std::string type;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    int stack_count = 1;
    bool ghosted = false;
    std::vector<DiagramAttribute> attributes;
};

struct DiagramEdge {
    std::string id;
    std::string source_id;
    std::string target_id;
    std::string label;
    bool directed = true;
    bool identifying = false;
    Cardinality source_cardinality = Cardinality::One;
    Cardinality target_cardinality = Cardinality::OneOrMany;
};

class DiagramModel {
public:
    explicit DiagramModel(DiagramType type);

    DiagramType type() const;
    void set_type(DiagramType type);

    DiagramNode& AddNode(const DiagramNode& node);
    DiagramEdge& AddEdge(const DiagramEdge& edge);

    std::vector<DiagramNode>& nodes();
    const std::vector<DiagramNode>& nodes() const;

    std::vector<DiagramEdge>& edges();
    const std::vector<DiagramEdge>& edges() const;

    int NextNodeIndex();
    int NextEdgeIndex();

private:
    DiagramType type_;
    int next_node_index_ = 1;
    int next_edge_index_ = 1;
    std::vector<DiagramNode> nodes_;
    std::vector<DiagramEdge> edges_;
};

std::string DiagramTypeLabel(DiagramType type);
std::string DiagramTypeKey(DiagramType type);
std::string CardinalityLabel(Cardinality value);

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_MODEL_H
