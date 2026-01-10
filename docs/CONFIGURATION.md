# Configuration

ScratchRobin uses modern, human-readable TOML for configuration. The defaults follow XDG conventions.

## Locations
- Linux/macOS: `~/.config/scratchrobin/`
- Windows: `%APPDATA%\\ScratchRobin\\`

## Files
- `scratchrobin.toml`: application settings
- `connections.toml`: connection profiles (no plaintext passwords)

Passwords and tokens are stored in the OS credential store (Keychain/DPAPI/Secret Service). The
`connections.toml` file only references credential IDs.

## scratchrobin.toml (example)
```toml
[app]
name = "ScratchRobin"
telemetry = false

[ui]
theme = "system"
font_family = "default"
font_size = 11

[editor]
autocomplete = true
history_max_items = 2000

[network]
connect_timeout_ms = 5000
query_timeout_ms = 0
```

## connections.toml (example)
```toml
[[connection]]
name = "Local ScratchBird"
host = "127.0.0.1"
port = 3050
database = "/data/scratchbird/demo.sdb"
username = "sysdba"
credential_id = "scratchrobin:local-sysdba"
ssl_mode = "prefer"
```

