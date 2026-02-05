# Context-Sensitive Help System

Status: Draft
Last Updated: 2026-01-09

This specification defines the help resources and indexing strategy used to
provide context-sensitive help for ScratchRobin windows, commands, and
language references.

## Goals

- Provide F1/context help for every window, menu, and command.
- Surface the correct language guide for the active backend.
- Support offline help with packaged Markdown content.
- Allow external links when online, while keeping a local fallback.

## Scope

Help content includes:
- UI usage guidance (per window)
- Reference guides for tools and admin workflows
- Language guides (ScratchBird, PostgreSQL, MySQL, Firebird)
- Troubleshooting and common tasks

## Content Sources

### Local (Packaged)
- `assets/help/` (Markdown topics)
- `assets/help/images/` (screenshots, diagrams)
- `assets/help/help_index.json` (topic registry)

### External (Optional)
- ScratchBird project documentation and language guides
- ScratchRobin project documentation

Local content is the default. External URLs are supplemental.

## Directory Layout

```
assets/
  help/
    help_index.json
    topics/
      ui/
      reference/
      language/
      troubleshooting/
    images/
```

## Help Topic Format

Topics are Markdown with a lightweight front matter header.

Example:

```
---
id: ui.sql_editor.overview
title: SQL Editor Overview
category: ui
window: SqlEditor
commands: ["command.sql.run", "command.sql.cancel"]
backends: ["scratchbird", "postgresql", "mysql", "firebird"]
---

# SQL Editor
...
```

Required fields:
- `id` (unique)
- `title`
- `category` (ui/reference/language/troubleshooting)

Optional fields:
- `window` (window type)
- `commands` (command IDs)
- `backends` (backend filters)
- `tags` (search tags)

## Help Index Manifest

`assets/help/help_index.json` provides fast lookups and cross-links.

Example:

```json
{
  "version": "1.0",
  "topics": [
    {
      "id": "ui.sql_editor.overview",
      "path": "topics/ui/sql_editor_overview.md",
      "title": "SQL Editor Overview",
      "window": "SqlEditor",
      "commands": ["command.sql.run", "command.sql.cancel"],
      "backends": ["scratchbird", "postgresql", "mysql", "firebird"],
      "category": "ui"
    }
  ]
}
```

## Context Mapping

### Window Types
- `CatalogBrowser`
- `SqlEditor`
- `Monitoring`
- `UsersRoles`
- `Diagram`
- `JobScheduler`
- `ProjectWorkspace`
- `ServerManager`
- `ClusterManager`
- `DatabaseManager`
- `GitIntegration`
- `BackupRestore`
- `Preferences`
- `ActivityLog`
- `StartupBranding`

### Command IDs
Commands use a stable naming scheme:
- `command.connection.server.create`
- `command.connection.database.connect`
- `command.sql.run`
- `command.sql.cancel`
- `command.diagram.create.erd`
- `command.git.status`

### Language Guides
Language help is filtered by the active connection backend. If no backend is
connected, the default is ScratchBird.

- ScratchBird: `language.scratchbird.*`
- PostgreSQL: `language.postgresql.*`
- MySQL: `language.mysql.*`
- Firebird: `language.firebird.*`

## UI Behavior

- F1 opens the highest-ranked topic for the current window and selection.
- Help menu includes:
  - Help for this window
  - Help for selected command
  - Language guide (active backend)
  - Search help
- Help panel includes search, category filters, and recent topics.

## Ranking Rules

When multiple topics match:
1. Exact command match
2. Window match
3. Backend match
4. Category priority: ui -> reference -> language -> troubleshooting

## Offline vs Online

- Offline: load local Markdown only.
- Online: allow external links in a secondary "Related" section.
- All external URLs must have a local fallback topic.

## Versioning

- `help_index.json` includes a schema version.
- Help topics are versioned with the ScratchRobin release.

## Required Topic Sets (Minimum)

UI Topics:
- Catalog Browser
- SQL Editor
- Monitoring
- Users & Roles
- Diagram (ERD)
- Project Workspace

Reference Topics:
- Connections menu
- Configuration files
- Credential storage

Language Topics:
- ScratchBird SQL
- PostgreSQL SQL (subset)
- MySQL SQL (subset)
- Firebird SQL (subset)

Troubleshooting:
- Connection failures
- TLS/SSL setup
- Backend capability limits
