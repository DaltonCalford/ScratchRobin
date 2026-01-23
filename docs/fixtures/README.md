# Mock Fixture Schema

ScratchRobin's mock backend reads JSON fixtures and returns protocol-shaped results.
The mock backend is intended for UI development before the ScratchBird listeners are ready.

## File Layout
Top-level object with a `queries` array and optional `metadata` section.

```json
{
  "queries": [
    { "match": "select 1", "result": { "columns": [], "rows": [] } }
  ],
  "metadata": {
    "nodes": [
      { "label": "demo", "kind": "table", "path": "native.public.demo" }
    ]
  }
}
```

## Query Entry
Each query entry must include:
- `match` (string): SQL match string.
- `match_type` (optional): `"exact"` (default) or `"regex"`.
- `result` (object) OR `error` (object).

### Result Object
```json
{
  "columns": [
    { "name": "id", "wire_type": "INT64" }
  ],
  "rows": [
    [1, "alpha"],
    [ { "text": "0x01", "data_hex": "0x01" } ]
  ],
  "rows_affected": 3,
  "command_tag": "SELECT 3"
}
```

Supported result fields:
- `columns` (array): each column uses `{ "name": string, "wire_type": string }`.
- `rows` (array): each row is an array of cell values.
- `rows_affected` (number): DML affected count.
- `command_tag` (string): server-style command tag.
- `messages` (array): optional notices/warnings with `{ "severity": string, "message": string, "detail": string }` or strings.
- `error_stack` (array): optional stack trace lines (strings).

### Error Object
```json
{
  "error": { "message": "Mock failure", "stack": ["Layer A", "Layer B"] }
}
```

## Cell Values
Rows accept one of:
- `null`
- `true` or `false`
- number
- string
- object with optional fields:
  - `is_null` (bool)
  - `text` (string)
  - `data_hex` (string, hex bytes, optional `0x` prefix)

If `data_hex` is provided, it is decoded into raw bytes and `text` is used for display.

## Matching Rules
- `exact`: normalized, case-insensitive match (whitespace collapsed, trailing semicolons ignored).
- `regex`: case-insensitive regex against the raw SQL input.

## Metadata Section
The optional `metadata` block drives the catalog tree in the UI.

```json
{
  "metadata": {
    "nodes": [
      {
        "label": "public",
        "kind": "schema",
        "catalog": "native",
        "path": "native.public"
      },
      {
        "label": "demo",
        "kind": "table",
        "catalog": "native",
        "path": "native.public.demo",
        "ddl": "CREATE TABLE public.demo (...);",
        "dependencies": ["index: demo_pkey"],
        "children": [
          { "label": "demo_id_seq", "kind": "sequence", "path": "native.public.demo_id_seq" }
        ]
      }
    ]
  }
}
```

Supported metadata fields:
- `label` (string, required): display name for the node.
- `kind` (string, optional): object type (schema, table, view, sequence, etc).
- `catalog` (string, optional): catalog/engine tag for grouping.
- `path` (string, optional): dotted or slash path used to build a hierarchical tree.
- `ddl` (string, optional): DDL extract to show in the inspector.
- `dependencies` (array, optional): list of dependency strings.
- `children` (array, optional): nested metadata nodes.

### Fixtures for Tests
- `tests/fixtures/metadata_complex.json`: exercises path expansion, child nodes, and label fallback.
- `tests/fixtures/metadata_invalid.json`: verifies error handling for invalid metadata arrays.
- `tests/fixtures/metadata_multicatalog.json`: exercises multiple catalog paths (native + emulated
  Firebird/PostgreSQL/MySQL).
