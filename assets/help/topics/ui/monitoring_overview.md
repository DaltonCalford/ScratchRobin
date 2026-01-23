---
id: ui.monitoring.overview
title: Monitoring Overview
category: ui
window: Monitoring
commands: ["command.window.monitoring", "command.monitor.refresh"]
backends: ["scratchbird", "postgresql", "mysql", "firebird", "mock"]
---

# Monitoring Overview

The Monitoring window provides a read-only view of live activity. Use the
connection selector to choose a target, then pick a view:

- Sessions
- Locks
- Performance
- Statements
- Long-running

Use Refresh to reload the data. Columns vary by backend and are aligned to the
capabilities of the connected engine.
