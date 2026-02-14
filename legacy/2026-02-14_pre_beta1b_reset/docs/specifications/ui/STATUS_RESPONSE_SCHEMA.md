# STATUS Response Schema (UI)

This document defines how ScratchRobin displays SBWP `STATUS_RESPONSE` payloads in the UI.
It focuses on the user-facing schema (fields, categories, and presentation rules), not the
wire encoding.

## 1) Request Types

Status requests are grouped into four categories. The UI exposes a selector that defaults
per-connection.

- `server` → **Server Info**
- `connection` → **Connection Info**
- `database` → **Database Info**
- `statistics` → **Statistics**

## 2) Response Structure

Each response is a set of key/value entries. The UI renders these into a two-column grid.

```
StatusSnapshot
  request_type: enum (server|connection|database|statistics)
  entries: [
    { key: string, value: string }
  ]
```

## 3) UI Rendering Rules

- The UI renders responses as **compact cards**, grouped by category.
- Each card shows a two-column grid of fields and values.
- The top card is always **Request** with:
  - Field = `Type`
  - Value = display name of `request_type`

### Category grouping

Entries may be grouped by prefix in the key:

- `category.key` → category = `category`, field = `key`
- `category:key` → category = `category`, field = `key`
- No prefix → category = `General`

## 4) Defaults and Per-Connection Settings

Per connection, the following defaults may be stored in `connections.toml`:

- `status_auto_poll` (bool)
- `status_poll_interval_ms` (int, min 250)
- `status_request_default` (`server|connection|database|statistics`)
- `status_category_order` (comma-separated list, optional)
- `status_category_filter` (string, optional)
- `status_diff_enabled` (bool)
- `status_diff_ignore_unchanged` (bool)
- `status_diff_ignore_empty` (bool)

The UI uses these defaults when a profile is selected or when a connection is established.

## 5) Category Ordering

If `status_category_order` is set, cards render in that order. Any categories not listed
are appended after the configured list. `Request` is always first.

## 6) Error and Empty States

- On error: display a short message near the grid (no modal dialogs).
- On clear: the grid is emptied and a “Status cleared” message is shown.
- On unsupported backends: display “Status not supported by backend”.

## 7) Export

- Users can copy or save the current snapshot as JSON.
- JSON output mirrors the `StatusSnapshot` structure.
- If a category filter is selected, only that category is exported.

## 8) Diff View

When diff mode is enabled, the UI shows a **Changes** card containing only entries that
changed between the previous and current snapshots:

- `old` → previous value
- `new` → current value

In export, diffs are written under a `diff` array instead of `entries`.

### Diff thresholds

Diff rendering can be tuned per connection:

- `status_diff_ignore_unchanged = true` → skip keys where `old == new`
- `status_diff_ignore_empty = true` → skip keys where both old and new are empty

## 9) History Panel

The UI maintains a per-connection history list of recent snapshots (timestamp + request type).

- Selecting a history entry displays that snapshot.
- When diff mode is enabled, the comparison is against the immediately prior history entry.
- Clearing status also clears the history list.
