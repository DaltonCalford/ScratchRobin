---
id: reference.backup_restore
title: Backup & Restore
category: reference
window: BackupRestore
backends: ["scratchbird", "firebird", "postgresql", "mysql", "mock"]
---

# Backup & Restore

ScratchRobin will provide a guided backup/restore workflow once the UI is
wired. For now, use the command-line tools for your engine:

- ScratchBird: sb_backup / sb_restore
- Firebird: gbak
- PostgreSQL: pg_dump / pg_restore
- MySQL: mysqldump

UI integration is planned for a future phase.
