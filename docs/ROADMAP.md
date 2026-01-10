# Roadmap (Rewrite)

## Phase 0: Skeleton (current)
- App bootstraps with SDI main window and SQL editor window.
- Build system and docs in place.

## Phase 1: ScratchBird Connectivity
- Implement `ConnectionManager` using ScratchBird client API.
- Connection profiles and credential storage.
- Basic query execution with cancel.

## Phase 2: Metadata + Tree
- Lazy metadata loading (schemas, tables, views, procedures).
- Observer notifications to keep tree + inspector in sync.
- Basic context menus (connect, disconnect, properties).

## Phase 3: SQL Editor
- Statement splitting (ScratchBird dialect, custom terminators).
- Result grid with paging.
- History and favorites.

## Phase 4: Object Inspector
- HTML templates + hyperlink navigation.
- DDL extraction and dependency lists.

## Phase 5: Admin Tools
- Backup/restore UI.
- User/role management.
- Monitoring (sessions, locks, perf counters).

