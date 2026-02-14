# ScratchRobin Backup & Restore UI Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/ui/backup_dialog.cpp`, `src/ui/restore_dialog.cpp`

---

## Overview

The Backup & Restore system provides a user interface for creating database backups, restoring from backups, and managing backup history. Integration with ScratchBird's `sb_backup` and `sb_restore` CLI tools.

---

## Scope

### In Scope
- Full database backup creation
- Database restore from backup files
- Backup history tracking
- Backup scheduling configuration
- Progress monitoring

### Out of Scope
- Incremental/differential backup UI (Future)
- Point-in-time recovery (Future)
- Cross-database backup migration

---

## Backend Support

| Backend | Backup Support | Tool | Format |
|---------|---------------|------|--------|
| ScratchBird | ✅ Native | sb_backup, sb_restore | .sbbak |
| PostgreSQL | ✅ Via pg_dump | pg_dump, pg_restore | .sql, .dump |
| MySQL | ✅ Via mysqldump | mysqldump, mysql | .sql |
| Firebird | ✅ Via gbak | gbak | .fbk |

---

## Backup Format Specification

### ScratchBird Backup (.sbbak)

ZIP archive structure:
```
backup_20260203_143022.sbbak
├── manifest.json
├── schema/
│   ├── public/
│   │   ├── tables.sql
│   │   ├── views.sql
│   │   └── indexes.sql
│   └── other_schema/
├── data/
│   ├── public/
│   │   ├── table_name.csv
│   │   └── ...
├── blobs/
│   └── blob_data/
└── checksums.sha256
```

**manifest.json:**
```json
{
  "version": "1.0",
  "created_at": "2026-02-03T14:30:22Z",
  "source_server": "localhost:3092",
  "source_database": "mydb",
  "scratchrobin_version": "0.1.0",
  "schemas_included": ["public", "app_data"],
  "tables_included": 47,
  "backup_options": {
    "format": "portable",
    "compression": "gzip",
    "include_blobs": true
  },
  "size_bytes": 15472931
}
```

---

## SQL Surface

### Backup Commands

**ScratchBird (via CLI):**
```bash
sb_backup --host=localhost --port=3092 --database=mydb \
  --output=backup_$(date +%Y%m%d_%H%M%S).sbbak \
  --format=portable \
  --compression=gzip
```

**PostgreSQL:**
```bash
pg_dump -h localhost -p 5432 -U postgres -F c -b -v -f backup.dump mydb
```

**MySQL:**
```bash
mysqldump -h localhost -u root -p --databases mydb > backup.sql
```

**Firebird:**
```bash
gbak -b -v localhost:mydb.fdb backup.fbk -user SYSDBA -pass masterkey
```

### Restore Commands

**ScratchBird:**
```bash
sb_restore --input=backup.sbbak --host=localhost --database=mydb_restored
```

**PostgreSQL:**
```bash
pg_restore -h localhost -p 5432 -U postgres -d mydb -v backup.dump
```

**MySQL:**
```bash
mysql -h localhost -u root -p mydb < backup.sql
```

**Firebird:**
```bash
gbak -c -v backup.fbk localhost:mydb_restored.fdb -user SYSDBA
```

---

## UI Requirements

### Menu Placement
- Main menu: `Admin -> Backup & Restore`
- Submenu: `Backup...`, `Restore...`, `Backup History...`, `Backup Schedule...`

### Backup Dialog

**Layout:**
```
+--------------------------------------------------+
| Database Backup                                  |
+--------------------------------------------------+
|                                                  |
| Source: [Database: mydb @ localhost       [v]]   |
|                                                  |
| Destination: [/path/to/backups/          [...]]  |
|              [backup_20260203_143022.sbbak]      |
|                                                  |
| Format: ( ) Native (fastest)                    |
|         (*) Portable (cross-platform)           |
|         ( ) SQL only (text, largest)            |
|                                                  |
| Compression: [High (slow)           [v]]         |
|                                                  |
| Options: [x] Include BLOBs                       |
|          [x] Include indexes                     |
|          [ ] Verify after backup                 |
|          [x] Add timestamp to filename           |
|                                                  |
| Tables: (*) All tables                          |
|         ( ) Selected tables:                     |
|             [x] users                            |
|             [x] orders                           |
|             [ ] logs                             |
|                                                  |
| [Advanced Options...]                            |
|                                                  |
| +----------------------------------------------+ |
| | Progress: [==========>          ] 45%        | |
| | Status: Backing up table 'orders'...         | |
| | Time: 00:02:15 elapsed, 00:01:30 remaining   | |
| +----------------------------------------------+ |
|                                                  |
| [Cancel]              [Start Backup]             |
+--------------------------------------------------+
```

**Features:**
- Directory browser for destination
- Filename auto-generation with timestamp
- Format selection (Native/Portable/SQL)
- Compression level
- Table selection (all or subset)
- Real-time progress with ETA
- Cancel button (graceful cancellation)

### Restore Dialog

**Layout:**
```
+--------------------------------------------------+
| Database Restore                                 |
+--------------------------------------------------+
|                                                  |
| Backup File: [/path/to/backup.sbbak      [...]] |
|                                                  |
| Backup Info:                                     |
|   Created: 2026-02-03 14:30:22                  |
|   Database: mydb                                |
|   Size: 14.8 MB                                 |
|   Format: Portable                              |
|   Tables: 47                                    |
|                                                  |
| Target: [Create new database            [v]]    |
|         Database name: [mydb_restored    ]      |
|                                                  |
| Options: [x] Verify before restore              |
|          [ ] Overwrite if exists                |
|          [x] Restore to specific point          |
|            [2026-02-03 14:30:22       [v]]      |
|                                                  |
| [View Backup Contents...]                        |
|                                                  |
| +----------------------------------------------+ |
| | Progress: [==============>      ] 65%        | |
| | Status: Restoring table 'users'...           | |
| +----------------------------------------------+ |
|                                                  |
| [Cancel]              [Start Restore]            |
+--------------------------------------------------+
```

**Features:**
- Backup file validation
- Backup metadata display
- Target selection (existing or new database)
- Point-in-time restore (if supported)
- Conflict resolution options
- Progress monitoring

### Backup History Dialog

**Columns:**
- Date/Time
- Database
- Size
- Format
- Status (Success/Failed)
- Location
- Actions (Restore, Delete, Verify)

**Features:**
- Sortable/filterable list
- Quick restore from history
- Delete old backups
- Verify backup integrity

### Backup Schedule Dialog

**Fields:**
- Enable scheduled backups (checkbox)
- Frequency (Daily, Weekly, Monthly)
- Time of day
- Day of week (for weekly)
- Retention policy (keep N backups)
- Destination directory
- Email notifications (optional)

---

## Implementation Details

### Files
- `src/ui/backup_dialog.h` / `.cpp` - Backup creation
- `src/ui/restore_dialog.h` / `.cpp` - Restore from backup
- `src/ui/backup_history_dialog.h` / `.cpp` - History management
- `src/ui/backup_schedule_dialog.h` / `.cpp` - Scheduling

### Key Classes
```cpp
class BackupDialog : public wxDialog {
    // Backup configuration UI
    // Progress monitoring
    // CLI integration
};

class RestoreDialog : public wxDialog {
    // Backup file selection
    // Target configuration
    // Restore execution
};

class BackupHistoryDialog : public wxDialog {
    // History list management
    // Quick actions
};
```

### Backend Integration
- ScratchBird: Native `sb_backup`/`sb_restore` CLI
- PostgreSQL: `pg_dump`/`pg_restore`
- MySQL: `mysqldump`/`mysql`
- Firebird: `gbak`

### Progress Monitoring
- Subprocess output parsing
- Progress bar updates
- ETA calculation
- Cancellation support

---

## Future Enhancements

- Incremental backup support
- Cloud storage integration (S3, GCS, Azure)
- Backup encryption
- Automated backup testing
- Backup comparison/diff
