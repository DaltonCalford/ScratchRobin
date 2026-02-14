#include "diagram/diagram_services.h"
#include "tests/test_harness.h"

#include <filesystem>

using namespace scratchrobin;
using namespace scratchrobin::diagram;

namespace {

void ExpectReject(const std::string& code, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RejectError& ex) {
        scratchrobin::tests::AssertEq(ex.payload().code, code, "unexpected reject");
        return;
    }
    throw std::runtime_error("expected reject not thrown");
}

beta1b::DiagramDocument BuildDocument() {
    beta1b::DiagramDocument doc;
    doc.diagram_id = "d1";
    doc.notation = "crowsfoot";
    doc.nodes.push_back({"n1", "table", "root", 0, 0, 120, 60, "int"});
    doc.nodes.push_back({"n2", "table", "root", 140, 0, 120, 60, "varchar"});
    doc.edges.push_back({"e1", "n1", "n2", "fk"});
    return doc;
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/diagram_save_load", [] {
                        namespace fs = std::filesystem;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_diagram";
                        fs::remove_all(temp);

                        DiagramService svc;
                        auto doc = BuildDocument();
                        auto save = svc.SaveModel((temp / "diagram.json").string(), DiagramType::Erd, doc);
                        scratchrobin::tests::AssertTrue(save.bytes_written > 0U, "diagram save bytes missing");
                        auto loaded = svc.LoadModel((temp / "diagram.json").string(), DiagramType::Erd);
                        scratchrobin::tests::AssertEq(loaded.diagram_id, "d1", "diagram load mismatch");

                        fs::remove_all(temp);
                    }});

    tests.push_back({"integration/diagram_canvas_trace", [] {
                        DiagramService svc;
                        auto doc = BuildDocument();
                        svc.ApplyCanvasCommand(doc, "drag", "n1", "root");
                        ExpectReject("SRB1-R-6201", [&] { svc.ApplyCanvasCommand(doc, "drag", "missing", "root"); });

                        svc.ValidateTraceRefs({{"n1", {"spec:a", "spec:b"}}}, {"spec:a", "spec:b"});
                        ExpectReject("SRB1-R-6101", [&] {
                            svc.ValidateTraceRefs({{"n1", {"spec:a", "missing"}}}, {"spec:a"});
                        });
                    }});

    tests.push_back({"integration/diagram_forward_migration_export", [] {
                        DiagramService svc;
                        auto ddl = svc.GenerateForwardSql("public.customer",
                                                          {"int", "varchar"},
                                                          {{"int", "INTEGER"}, {"varchar", "VARCHAR(50)"}});
                        scratchrobin::tests::AssertTrue(ddl.size() == 2U, "forward ddl count mismatch");
                        ExpectReject("SRB1-R-6301", [&] {
                            (void)svc.GenerateForwardSql("public.customer", {"unknown"}, {{"int", "INTEGER"}});
                        });

                        auto plan = svc.GenerateMigrationDiffPlan(
                            {{"1", "table", "public.a", "add", "CREATE TABLE public.a (id INT)"}},
                            false);
                        scratchrobin::tests::AssertTrue(plan.size() == 1U, "migration plan mismatch");
                        ExpectReject("SRB1-R-6302", [&] {
                            (void)svc.GenerateMigrationDiffPlan(
                                {{"2", "table", "public.a", "alter", "ALTER TABLE public.a ADD COLUMN x INT"}},
                                false);
                        });

                        auto doc = BuildDocument();
                        auto exp = svc.ExportDiagram(doc, "svg", "full");
                        scratchrobin::tests::AssertTrue(exp.find("diagram-export:svg") == 0U, "diagram export mismatch");
                        ExpectReject("SRB1-R-6303", [&] { (void)svc.ExportDiagram(doc, "pdf", "minimal_ui"); });
                    }});

    return scratchrobin::tests::RunTests(tests);
}

