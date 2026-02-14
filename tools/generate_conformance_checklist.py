#!/usr/bin/env python3
import csv
from pathlib import Path
from collections import Counter

repo = Path(__file__).resolve().parents[1]
source = repo.parent / "local_work/docs/specifications_beta1b/10_Execution_Tracks_and_Conformance/CONFORMANCE_VECTOR.csv"
out_dir = repo / "docs/conformance"
out_csv = out_dir / "CONFORMANCE_IMPLEMENTATION_CHECKLIST.csv"
out_md = out_dir / "CONFORMANCE_IMPLEMENTATION_CHECKLIST.md"

priority_order = {"Critical": 0, "High": 1, "Medium": 2, "Low": 3}
phase_order = {
    "00": 0,
    "01": 1,
    "02": 2,
    "03": 3,
    "04": 4,
    "05": 5,
    "06": 6,
    "07": 7,
    "08": 8,
    "09": 9,
    "10": 10,
}

phase_module = {
    "00": "src/phases/phase00_governance.cpp",
    "01": "src/phases/phase01_scope.cpp",
    "02": "src/phases/phase02_runtime.cpp",
    "03": "src/phases/phase03_project_data.cpp",
    "04": "src/phases/phase04_backend.cpp",
    "05": "src/phases/phase05_ui.cpp",
    "06": "src/phases/phase06_diagramming.cpp",
    "07": "src/phases/phase07_reporting.cpp",
    "08": "src/phases/phase08_advanced.cpp",
    "09": "src/phases/phase09_packaging.cpp",
    "10": "src/phases/phase10_conformance_release.cpp",
}

phase_name = {
    "00": "Governance and Scaffolding",
    "01": "Product Scope and Goals",
    "02": "Runtime Architecture",
    "03": "Project and Data Model",
    "04": "Backend Adapter and Connection",
    "05": "UI Surface and Workflows",
    "06": "Diagramming and Visual Modeling",
    "07": "Reporting and Governance Integration",
    "08": "Advanced Features and Integrations",
    "09": "Build/Test/Packaging",
    "10": "Conformance/Alpha/Release Gates",
}

def map_phase(area: str) -> str:
    a = area.strip().lower()
    if a == "governance":
        return "00"
    if a in {"connection", "copy", "prepared"}:
        return "04"
    if a == "project":
        return "03"
    if a == "ui":
        return "05"
    if a == "diagram":
        return "06"
    if a == "reporting":
        return "07"
    if a in {"advanced", "preview", "ops"}:
        return "08"
    if a in {"build", "packaging"}:
        return "09"
    if a in {"spec_support", "alpha_preservation"}:
        return "10"
    return "10"

rows = list(csv.DictReader(source.open(newline="", encoding="utf-8")))

checklist = []
for r in rows:
    ph = map_phase(r["area"])
    checklist.append({
        "execution_rank": "",
        "phase_id": ph,
        "phase_name": phase_name[ph],
        "phase_module_stub": phase_module[ph],
        "case_id": r["case_id"],
        "area": r["area"],
        "feature": r["feature"],
        "priority": r["priority"],
        "gate_type": r["gate_type"],
        "target_profile": r["target_profile"],
        "baseline": r["baseline"],
        "reject_code_on_fail": r["reject_code_on_fail"],
        "implementation_status": "NOT_STARTED",
        "implementation_notes": "",
        "evidence_ref": "",
    })

checklist.sort(key=lambda x: (
    priority_order.get(x["priority"], 9),
    phase_order.get(x["phase_id"], 99),
    x["case_id"],
))

for i, row in enumerate(checklist, start=1):
    row["execution_rank"] = str(i)

fieldnames = [
    "execution_rank",
    "phase_id",
    "phase_name",
    "phase_module_stub",
    "case_id",
    "area",
    "feature",
    "priority",
    "gate_type",
    "target_profile",
    "baseline",
    "reject_code_on_fail",
    "implementation_status",
    "implementation_notes",
    "evidence_ref",
]

with out_csv.open("w", newline="", encoding="utf-8") as f:
    w = csv.DictWriter(f, fieldnames=fieldnames)
    w.writeheader()
    w.writerows(checklist)

priority_counts = Counter(r["priority"] for r in checklist)
baseline_counts = Counter(r["baseline"] for r in checklist)
phase_counts = Counter(r["phase_id"] for r in checklist)

with out_md.open("w", encoding="utf-8") as f:
    f.write("# Conformance-First Implementation Checklist\n\n")
    f.write("Generated from beta1b `CONFORMANCE_VECTOR.csv` and mapped to local phase stubs (`00->10`).\n\n")
    f.write("## Summary\n\n")
    f.write(f"- Total cases: `{len(checklist)}`\n")
    f.write("- Priority counts:\n")
    for p in ["Critical", "High", "Medium", "Low"]:
        f.write(f"  - `{p}`: `{priority_counts.get(p, 0)}`\n")
    f.write("- Baseline counts:\n")
    for b in sorted(baseline_counts):
        f.write(f"  - `{b}`: `{baseline_counts[b]}`\n")
    f.write("- Phase distribution:\n")
    for ph in sorted(phase_counts, key=lambda k: phase_order[k]):
        f.write(f"  - `Phase {ph}` ({phase_name[ph]}): `{phase_counts[ph]}`\n")

    f.write("\n## Execution Order\n\n")
    f.write("Use `execution_rank` from the CSV for strict conformance-first ordering (priority-first, then phase-order, then case-id).\n\n")

    f.write("## First 25 Checklist Rows\n\n")
    f.write("| Rank | Case ID | Priority | Phase | Module Stub | Feature |\n")
    f.write("|---:|---|---|---|---|---|\n")
    for row in checklist[:25]:
        f.write(
            f"| {row['execution_rank']} | `{row['case_id']}` | `{row['priority']}` | `{row['phase_id']}` | "
            f"`{row['phase_module_stub']}` | {row['feature']} |\n"
        )

    f.write("\n## Full Data\n\n")
    f.write("Use `docs/conformance/CONFORMANCE_IMPLEMENTATION_CHECKLIST.csv` as the authoritative checklist table for implementation tracking.\n")

print(f"Wrote {out_csv}")
print(f"Wrote {out_md}")
