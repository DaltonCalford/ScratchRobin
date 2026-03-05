# Form and Dialog Specification

## Overview
Detailed specifications for all forms, dialogs, and wizards in robin-migrate.

---

## Connection Dialog

**Class**: `ConnectionDialog`  
**Type**: Modal Dialog  
**Size**: 500x400 (expandable)

### Layout
```
┌─────────────────────────────────────────────────────────────┐
│ Connection Settings                                    [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Connection Name:    [______________________________]   [*]  │
│                                                             │
│ Host:               [______________________________]   [*]  │
│ Port:               [____]                                │
│                      ^-- default: 4044                      │
│                                                             │
│ Database:           [______________________________]        │
│                                                             │
│ Authentication:     [Standard            ▼]                 │
│                      - Standard                             │
│                      - Windows Authentication               │
│                      - SSH Key                              │
│                                                             │
│ Username:           [______________________________]        │
│ Password:           [______________________________]   [👁]  │
│                                                             │
│ [✓] Save password                                           │
│                                                             │
│ [  Test Connection  ]                                       │
│ Status: Ready                                                 │
│                                                             │
│ [Advanced >>]                                               │
│                                                             │
│ ╔═══════════════════════════════════════════════════════╗   │
│ ║ Advanced Options                                      ║   │
│ ║                                                       ║   │
│ ║ Connection timeout:    [___] seconds                  ║   │
│ ║ Keep-alive interval:   [___] seconds (0=disabled)     ║   │
│ ║                                                       ║   │
│ ║ [SSL/TLS Settings...]  [SSH Tunnel Settings...]       ║   │
│ ╚═══════════════════════════════════════════════════════╝   │
│                                                             │
│         [Cancel]    [Test]    [Save]    [Connect]           │
└─────────────────────────────────────────────────────────────┘
```

### Field Specifications

| Field | Type | Required | Validation | Default |
|-------|------|----------|------------|---------|
| Connection Name | Text | Yes | Unique, non-empty | "" |
| Host | Text | Yes | Non-empty | "localhost" |
| Port | Integer | No | 1-65535 | 4044 |
| Database | Text | No | | "default" |
| Authentication | Dropdown | Yes | | "Standard" |
| Username | Text | Conditional | Required if Standard auth | "" |
| Password | Password | Conditional | | "" |
| Save Password | Checkbox | No | | true |
| Connection Timeout | Integer | No | 1-300 | 30 |
| Keep-alive Interval | Integer | No | 0-3600 | 0 |

### Buttons

| Button | Action | Validation |
|--------|--------|------------|
| Test | Test connection with current settings | All required fields valid |
| Save | Save without connecting | All required fields valid |
| Connect | Save and connect | All required fields valid |
| Cancel | Close dialog, discard changes | None |

### Behavior
- Port field shows "4044" placeholder
- Password visibility toggle (eye icon)
- Test Connection shows progress, then success/error
- Advanced section expandable/collapsible
- SSL/TLS and SSH Tunnel open sub-dialogs

---

## Preferences Dialog

**Class**: `PreferencesDialog`  
**Type**: Modal Dialog  
**Size**: 600x500

### Layout
```
┌─────────────────────────────────────────────────────────────┐
│ Preferences                                            [X]  │
├─────────────────────────────────────────────────────────────┤
│ ┌────────────────┐ ┌──────────────────────────────────────┐ │
│ │ General        │ │                                      │ │
│ │ Editor         │ │  [Content changes based on tab]      │ │
│ │ Results        │ │                                      │ │
│ │ Connection     │ │                                      │ │
│ │ Appearance     │ │                                      │ │
│ │ Shortcuts      │ │                                      │ │
│ │                │ │                                      │ │
│ └────────────────┘ └──────────────────────────────────────┘ │
│                                                             │
│         [Restore Defaults]    [Cancel]    [OK]              │
└─────────────────────────────────────────────────────────────┘
```

### Tab: General

```
Language:              [English ▼]
                       - English
                       - Spanish
                       - French
                       - German

Theme:                 [System Default ▼]
                       - System Default
                       - Light
                       - Dark
                       - High Contrast

Confirmations:
  [✓] Confirm object deletion
  [✓] Confirm exit with unsaved changes
  [✓] Confirm transaction rollback

Auto-save:
  [✓] Auto-save SQL scripts
       Interval: [__] minutes
```

### Tab: Editor

```
Font:
  Family:  [Monospace            ▼]
  Size:    [10] pt

Indentation:
  [ ] Use tabs
       Tab size: [4] spaces

Display:
  [✓] Show line numbers
  [✓] Highlight current line
  [ ] Word wrap
  [✓] Show whitespace characters

Code Completion:
  [✓] Enable code completion
       Auto-activate: [✓] after [3] characters
       Delay: [250] ms
```

### Tab: Results

```
Default View:          [Grid ▼]
                       - Grid
                       - Record
                       - Text

Data Fetching:
  Fetch size:          [1000] rows
  Maximum rows:        [10000] rows (0=unlimited)

Display:
  Null value display:  [NULL] (text)
  Date/Time format:    [YYYY-MM-DD HH:MM:SS]
  Decimal separator:   [.]

Grid Options:
  [✓] Show row numbers
  [✓] Alternating row colors
  [ ] Show grid lines
```

### Tab: Connection

```
Defaults:
  Connection timeout:  [30] seconds
  Keep-alive interval: [0] seconds (0=disabled)

Auto-connect:
  [ ] Connect on startup
  [ ] Reconnect on connection loss

Security:
  [✓] Save passwords (encrypted)
  [✓] Confirm before executing dangerous operations
       (DELETE without WHERE, DROP, etc.)
```

### Tab: Shortcuts

```
┌────────────────────────────────────────────────────┐
│ Action                    │ Shortcut               │
├───────────────────────────┼────────────────────────┤
│ Execute SQL               │ F9                     │
│ Execute Selection         │ Ctrl+F9                │
│ New Connection            │ Ctrl+N                 │
│ ...                       │ ...                    │
└────────────────────────────────────────────────────┘

[Reset to Defaults]  [Change Shortcut...]
```

---

## Export Results Dialog

**Class**: `ExportResultsDialog`  
**Type**: Modal Dialog  
**Size**: 500x400

### Layout
```
┌─────────────────────────────────────────────────────────────┐
│ Export Results                                         [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Format:               [CSV (Comma Separated)    ▼]          │
│                       - CSV (Comma Separated)               │
│                       - SQL INSERT statements               │
│                       - JSON                              │
│                       - Excel (.xlsx)                     │
│                       - XML                               │
│                       - HTML                              │
│                                                             │
│ Destination:          [Browse...]                           │
│                       [/home/user/export.csv]               │
│                                                             │
│ ╔═══════════════════════════════════════════════════════╗   │
│ ║ CSV Options                                           ║   │
│ ║                                                       ║   │
│ ║ Delimiter:            [Comma          ▼]              ║   │
│ ║ Quote character:      ["]                             ║   │
│ ║ Escape character:     [\]                             ║   │
│ ║                                                       ║   │
│ ║ [✓] Include header row                                ║   │
│ ║ Header position:      [Top            ▼]              ║   │
│ ║                                                       ║   │
│ ║ Null value:           [NULL]                          ║   │
│ ║ Encoding:             [UTF-8          ▼]              ║   │
│ ╚═══════════════════════════════════════════════════════╝   │
│                                                             │
│ Scope:                (•) All rows  ( ) Selected rows only  │
│                                                             │
│         [Cancel]    [Export]                                │
└─────────────────────────────────────────────────────────────┘
```

### Format-Specific Options

**CSV Options**:
- Delimiter: Comma, Tab, Semicolon, Pipe, Custom
- Quote: ", ', None
- Escape: \, None
- Header: Top, Bottom, None, Both
- Null string: text
- Encoding: UTF-8, ASCII, etc.
- BOM: Yes/No

**SQL Options**:
- Target table name
- Include column names: Yes/No
- Rows per statement: 1, 100, 1000
- Statement type: INSERT, UPSERT, REPLACE
- Omit schema: Yes/No

**JSON Options**:
- Format: Array of objects, Object of arrays
- Pretty print: Yes/No
- Include metadata: Yes/No

---

## Import Data Wizard

**Class**: `ImportDataWizard`  
**Type**: Wizard (multi-step)  
**Steps**: 4

### Step 1: Source

```
┌─────────────────────────────────────────────────────────────┐
│ Import Data - Step 1 of 4                              [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Source File:          [Browse...]                           │
│                       [/home/user/data.csv]                 │
│                                                             │
│ Format:               [CSV                  ▼]              │
│                       Detected: CSV, 15 columns             │
│                                                             │
│ Encoding:             [UTF-8                ▼]              │
│                                                             │
│ [Preview first 10 rows...]                                  │
│                                                             │
│                   [Cancel]    [Next >]                      │
└─────────────────────────────────────────────────────────────┘
```

**Validation**:
- File exists and is readable
- Format detected or selected

### Step 2: Preview & Mapping

```
┌─────────────────────────────────────────────────────────────┐
│ Import Data - Step 2 of 4                              [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Preview (first 10 rows):                                    │
│ ┌─────────────┬─────────────┬─────────────┬─────────────┐   │
│ │ id          │ name        │ email       │ created     │   │
│ ├─────────────┼─────────────┼─────────────┼─────────────┤   │
│ │ 1           │ Alice       │ a@mail.com  │ 2026-01-15  │   │
│ │ 2           │ Bob         │ b@mail.com  │ 2026-01-16  │   │
│ └─────────────┴─────────────┴─────────────┴─────────────┘   │
│                                                             │
│ Column Mapping:                                             │
│ Source Column      -> Target Column        Data Type        │
│ ┌──────────────────────────────────────────────────────┐    │
│ │ [✓] id           -> [id            ▼]  [INTEGER   ▼] │    │
│ │ [✓] name         -> [name          ▼]  [VARCHAR   ▼] │    │
│ │ [✓] email        -> [email         ▼]  [VARCHAR   ▼] │    │
│ │ [✓] created      -> [created_at    ▼]  [TIMESTAMP ▼] │    │
│ └──────────────────────────────────────────────────────┘    │
│                                                             │
│ [Auto-detect types]  [Reset mapping]                        │
│                                                             │
│              [< Back]    [Cancel]    [Next >]               │
└─────────────────────────────────────────────────────────────┘
```

### Step 3: Options

```
┌─────────────────────────────────────────────────────────────┐
│ Import Data - Step 3 of 4                              [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ CSV Options:                                                │
│ Header row:           (•) First row is header               │
│                       ( ) No header                         │
│                                                             │
│ Delimiter:            [Comma          ▼]                    │
│ Quote character:      ["]                                   │
│                                                             │
│ Null handling:        [Empty string is NULL]                │
│                       - Empty string is NULL                │
│                       - 'NULL' text is NULL                 │
│                       - 'null' text is NULL                 │
│                                                             │
│ Trim whitespace:      [✓]                                   │
│                                                             │
│ Target:               (•) Import into existing table        │
│                       Table: [users              ▼]         │
│                       ( ) Create new table                  │
│                       Name: [______________]                │
│                                                             │
│ Before import:        [✓] Truncate table                    │
│                                                             │
│ On error:             [Stop and report]                     │
│                       - Stop and report                     │
│                       - Skip row and continue               │
│                       - Import to error log                 │
│                                                             │
│              [< Back]    [Cancel]    [Next >]               │
└─────────────────────────────────────────────────────────────┘
```

### Step 4: Execution

```
┌─────────────────────────────────────────────────────────────┐
│ Import Data - Step 4 of 4                              [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Summary:                                                    │
│   Source: /home/user/data.csv                               │
│   Target: users (existing)                                  │
│   Rows to import: ~500 (estimated)                          │
│                                                             │
│ Progress:                                                   │
│   [████████████████████░░░░░░░░░░]  67%                     │
│                                                             │
│   Rows imported: 334                                        │
│   Errors: 0                                                 │
│   Time elapsed: 00:00:03                                    │
│   Estimated remaining: 00:00:02                             │
│                                                             │
│ Log:                                                        │
│   [Processing row 335...                                    │
│    Processing row 336...                                    │
│    ...]                                                     │
│                                                             │
│              [Cancel]    [  < Finish  ]                     │
└─────────────────────────────────────────────────────────────┘
```

---

## Table Designer Dialog

**Class**: `TableDesignerDialog`  
**Type**: Modal Dialog  
**Size**: 700x500  
**Tabs**: Columns | Constraints | Indexes | Preview

### Columns Tab

```
┌─────────────────────────────────────────────────────────────┐
│ Design Table: users                                    [X]  │
├─────────────────────────────────────────────────────────────┤
│ [Columns] [Constraints] [Indexes] [Preview SQL]             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Columns:                                                    │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ [*] │ Name    │ Type      │ Size │ Null │ Default     │  │
│ ├─────┼─────────┼───────────┼──────┼──────┼─────────────┤  │
│ │ PK  │ id      │ INTEGER   │      │ [ ]  │             │  │
│ │     │ name    │ VARCHAR   │ 255  │ [✓]  │             │  │
│ │     │ email   │ VARCHAR   │ 255  │ [✓]  │             │  │
│ │     │ active  │ BOOLEAN   │      │ [✓]  │ true        │  │
│ └─────┴─────────┴───────────┴──────┴──────┴─────────────┘  │
│                                                             │
│ [Add] [Remove] [Move Up] [Move Down]                        │
│                                                             │
│ Column Details:                                             │
│ Name:           [id                    ]                    │
│ Data Type:      [INTEGER               ▼]                   │
│ Size/Precision: [                      ]                    │
│ Nullable:       [ ]                                         │
│ Default:        [                      ]                    │
│ Comment:        [                      ]                    │
│                                                             │
│         [Cancel]    [Generate SQL]    [Save]                │
└─────────────────────────────────────────────────────────────┘
```

### Constraints Tab

```
Primary Key:
  Name: [pk_users          ]
  Columns: [id, name       ] [Select...]

Foreign Keys:
  ┌──────────────────────────────────────────────┐
  │ Name      │ Columns │ References │ On Delete │
  ├───────────┼─────────┼────────────┼───────────┤
  │ fk_role   │ role_id │ roles(id)  │ CASCADE   │
  └──────────────────────────────────────────────┘
  [Add] [Remove] [Edit]

Unique Constraints:
  ┌──────────────────────────────────────────────┐
  │ Name          │ Columns                      │
  ├───────────────┼──────────────────────────────┤
  │ uk_email      │ email                        │
  └──────────────────────────────────────────────┘
  [Add] [Remove]
```

### Preview SQL Tab

```
Generated SQL:
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│ CREATE TABLE users (                                        │
│     id INTEGER PRIMARY KEY,                                 │
│     name VARCHAR(255),                                      │
│     email VARCHAR(255),                                     │
│     active BOOLEAN DEFAULT true                             │
│ );                                                          │
│                                                             │
│ CREATE INDEX idx_users_email ON users(email);               │
│                                                             │
└─────────────────────────────────────────────────────────────┘

[Copy to Clipboard]  [Save to File]
```

---

## Generate DDL Dialog

**Class**: `GenerateDDLDialog`  
**Type**: Modal Dialog  
**Size**: 600x500

```
┌─────────────────────────────────────────────────────────────┐
│ Generate DDL                                           [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Objects:                                                    │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ [✓] Users                                              │  │
│ │ [✓] Orders                                             │  │
│ │ [✓] Order_Items                                        │  │
│ │ [ ] Products                                           │  │
│ └────────────────────────────────────────────────────────┘  │
│ [Select All] [Deselect All] [Invert]                        │
│                                                             │
│ Options:                                                    │
│   [✓] Include DROP statements                               │
│   [✓] Include CREATE statements                             │
│   [ ] Include INSERT data                                   │
│   [✓] Include indexes                                       │
│   [✓] Include foreign keys                                  │
│                                                             │
│ Format:                                                     │
│   Keyword case: [Uppercase          ▼]                      │
│   Identifier case: [As-is           ▼]                      │
│                                                             │
│ Generated SQL:                                              │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ DROP TABLE IF EXISTS users;                            │  │
│ │ CREATE TABLE users (...);                              │  │
│ │ ...                                                    │  │
│ └────────────────────────────────────────────────────────┘  │
│                                                             │
│ [Copy]  [Save to File...]  [Execute]                       │
└─────────────────────────────────────────────────────────────┘
```

---

## About Dialog

**Class**: `AboutDialog`  
**Type**: Modal Dialog  
**Size**: 450x350

```
┌─────────────────────────────────────────────────────────────┐
│ About Robin-Migrate                                    [X]  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│                    [Logo Image]                             │
│                                                             │
│              Robin-Migrate                                  │
│              Version 1.0.0                                  │
│                                                             │
│    Database Administration Tool                             │
│    for ScratchBird Databases                                │
│                                                             │
│    ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━                         │
│                                                             │
│    Built with:                                              │
│    • wxWidgets 3.2.5                                        │
│    • Boost 1.83                                             │
│    • CMake 3.28                                             │
│                                                             │
│    © 2026 Robin-Migrate Contributors                        │
│    Licensed under MIT License                               │
│                                                             │
│    [Visit Website]  [Report Issue]  [View License]          │
│                                                             │
│                         [OK]                                │
└─────────────────────────────────────────────────────────────┘
```

---

## Dialog State Management

### Validation States

| State | Visual | Action |
|-------|--------|--------|
| Valid | Normal | Enable OK/Save |
| Invalid | Red border on field | Disable OK, show error tooltip |
| Required | Field label with * | Show "Required" if empty |

### Common Button Patterns

| Pattern | Buttons | Use Case |
|---------|---------|----------|
| OK/Cancel | [Cancel] [OK] | Settings, Preferences |
| Save/Cancel | [Cancel] [Save] | Connection dialog |
| Action/Cancel | [Cancel] [Execute] | Export, Import |
| Wizard | [< Back] [Cancel] [Next >] | Multi-step wizards |

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Enter | Default button (OK/Save/Next) |
| Escape | Cancel |
| Tab | Next field |
| Shift+Tab | Previous field |
| Ctrl+Enter | Submit (in multi-line text) |
