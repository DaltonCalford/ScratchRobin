#include "diagram/diagram_services.h"
#include "tests/test_harness.h"

#include <chrono>
#include <string>
#include <vector>

using namespace scratchrobin;
using namespace scratchrobin::diagram;

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"perf/diagram_quarantine_smoke", [] {
                        DiagramService service;

                        beta1b::DiagramDocument doc;
                        doc.diagram_id = "perf-doc";
                        doc.notation = "crowsfoot";
                        constexpr int kNodeCount = 1000;
                        for (int i = 0; i < kNodeCount; ++i) {
                            beta1b::DiagramNode node;
                            node.node_id = "n" + std::to_string(i);
                            node.object_type = "table";
                            node.parent_node_id = "root";
                            node.logical_datatype = "int";
                            node.x = i * 3;
                            node.y = i * 2;
                            node.width = 100;
                            node.height = 40;
                            doc.nodes.push_back(std::move(node));
                            if (i > 0) {
                                beta1b::DiagramEdge edge;
                                edge.edge_id = "e" + std::to_string(i);
                                edge.from_node_id = "n" + std::to_string(i - 1);
                                edge.to_node_id = "n" + std::to_string(i);
                                edge.relation_type = "fk";
                                doc.edges.push_back(std::move(edge));
                            }
                        }

                        const auto start = std::chrono::steady_clock::now();
                        const auto payload = service.ExportDiagram(doc, "svg", "full");
                        const auto end = std::chrono::steady_clock::now();
                        const auto elapsed_ms =
                            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                        scratchrobin::tests::AssertTrue(payload.find("diagram-export:svg") == 0U,
                                                        "unexpected export payload");
                        // Quarantine-style threshold; do not fail for minor variance.
                        scratchrobin::tests::AssertTrue(elapsed_ms < 2000,
                                                        "diagram export perf threshold exceeded");
                    }});

    return scratchrobin::tests::RunTests(tests);
}
