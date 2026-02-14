#!/usr/bin/env python3
import argparse
import csv
import datetime as dt
import os
from pathlib import Path
from typing import Dict, List, Tuple


VALID_STATUS = {"NOT_STARTED", "IN_PROGRESS", "BLOCKED", "DONE"}
VALID_TEST_RESULT = {"PASS", "FAIL", "SKIPPED", "N/A"}


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def read_csv(path: Path) -> Tuple[List[str], List[Dict[str, str]]]:
    with path.open("r", newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise RuntimeError(f"CSV header missing: {path}")
        rows = list(reader)
        return reader.fieldnames, rows


def write_csv(path: Path, fieldnames: List[str], rows: List[Dict[str, str]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def next_entry_id(log_rows: List[Dict[str, str]]) -> str:
    highest = 0
    for row in log_rows:
        value = row.get("entry_id", "")
        if value.startswith("E") and value[1:].isdigit():
            highest = max(highest, int(value[1:]))
    return f"E{highest + 1:04d}"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Update beta1b implementation progress tracker and append step log row.")
    parser.add_argument("--queue-id", required=True, help="Queue ID, e.g. P0-03")
    parser.add_argument("--status", required=True, choices=sorted(VALID_STATUS), help="Status after update")
    parser.add_argument("--percent", required=True, type=int, help="Percent complete after update (0..100)")
    parser.add_argument("--step-title", required=True, help="Short step label")
    parser.add_argument("--step-detail", default="", help="Longer detail text")
    parser.add_argument("--next-step", default="", help="Next action summary")
    parser.add_argument("--blocked-by", default="", help="Blocking reason (required if status=BLOCKED)")
    parser.add_argument("--files-touched", default="", help="Pipe-delimited files changed")
    parser.add_argument("--tests-run", default="N/A", help="Tests executed")
    parser.add_argument("--test-result", default="N/A", choices=sorted(VALID_TEST_RESULT), help="Test result status")
    parser.add_argument("--evidence-ref", default="", help="Evidence reference path")
    parser.add_argument("--updated-by", default=os.getenv("USER", "unknown"), help="Actor/user id")
    parser.add_argument("--update-type", default="PROGRESS", help="Step log update type")
    parser.add_argument("--tracker-file", default="docs/conformance/BETA1B_IMPLEMENTATION_PROGRESS_TRACKER.csv")
    parser.add_argument("--step-log-file", default="docs/conformance/BETA1B_IMPLEMENTATION_STEP_LOG.csv")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.percent < 0 or args.percent > 100:
        raise RuntimeError("--percent must be in range 0..100")
    if args.status == "DONE" and args.percent != 100:
        raise RuntimeError("--status DONE requires --percent 100")
    if args.status == "BLOCKED" and not args.blocked_by:
        raise RuntimeError("--status BLOCKED requires --blocked-by")

    tracker_path = Path(args.tracker_file)
    step_log_path = Path(args.step_log_file)
    if not tracker_path.exists():
        raise RuntimeError(f"Tracker file not found: {tracker_path}")
    if not step_log_path.exists():
        raise RuntimeError(f"Step log file not found: {step_log_path}")

    tracker_fields, tracker_rows = read_csv(tracker_path)
    log_fields, log_rows = read_csv(step_log_path)

    row = None
    for item in tracker_rows:
        if item.get("queue_id") == args.queue_id:
            row = item
            break
    if row is None:
        known = ", ".join(sorted({r.get("queue_id", "") for r in tracker_rows if r.get("queue_id")}))
        raise RuntimeError(f"Unknown queue_id '{args.queue_id}'. Known: {known}")

    previous_percent = int(row.get("percent_complete", "0") or "0")
    if args.percent < previous_percent and args.status != "BLOCKED":
        raise RuntimeError(
            f"percent regression not allowed for non-blocked updates: previous={previous_percent}, new={args.percent}"
        )

    timestamp = utc_now()
    row["status"] = args.status
    row["percent_complete"] = str(args.percent)
    row["current_step"] = args.step_title
    if args.next_step:
        row["next_step"] = args.next_step
    row["blocked_by"] = args.blocked_by
    row["last_update_utc"] = timestamp
    row["last_update_by"] = args.updated_by
    row["last_update_note"] = args.step_detail if args.step_detail else args.step_title
    if args.evidence_ref:
        row["evidence_ref"] = args.evidence_ref

    write_csv(tracker_path, tracker_fields, tracker_rows)

    entry = {
        "entry_id": next_entry_id(log_rows),
        "timestamp_utc": timestamp,
        "queue_id": args.queue_id,
        "update_type": args.update_type,
        "status_after": args.status,
        "percent_after": str(args.percent),
        "step_title": args.step_title,
        "step_detail": args.step_detail,
        "files_touched": args.files_touched,
        "tests_run": args.tests_run,
        "test_result": args.test_result,
        "evidence_ref": args.evidence_ref,
        "updated_by": args.updated_by,
    }
    log_rows.append(entry)
    write_csv(step_log_path, log_fields, log_rows)

    print(
        "Updated tracker:",
        f"queue_id={args.queue_id}",
        f"status={args.status}",
        f"percent={args.percent}",
        f"log_entry={entry['entry_id']}",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
