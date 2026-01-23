---
id: ui.sql_editor.overview
title: SQL Editor Overview
category: ui
window: SqlEditor
commands: ["command.sql.run", "command.sql.cancel", "command.sql.export_csv", "command.sql.export_json"]
backends: ["scratchbird", "postgresql", "mysql", "firebird", "mock"]
---

# SQL Editor Overview

The SQL Editor lets you connect, run statements, and inspect results.

Key elements:
- Connection selector and session controls (connect, auto-commit, begin/commit/rollback)
- Run and Cancel controls for query execution
- Row limit control to cap displayed rows in non-paged results
- Results tabs for data grid, plan, SBLR, and messages
- Export actions for CSV/JSON

Use the statement history selector to reload previous queries. The status
bar reports elapsed time, rows returned, and whether a row limit was hit.
