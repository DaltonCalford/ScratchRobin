# Configuration

ScratchRobin uses modern, human-readable TOML for configuration. The defaults follow XDG conventions.

## Locations
- Linux/macOS: `~/.config/scratchrobin/`
- Windows: `%APPDATA%\\ScratchRobin\\`

## Files
- `scratchrobin.toml`: application settings
- `connections.toml`: connection profiles (no plaintext passwords)

Passwords and tokens are stored in the OS credential store (Keychain/DPAPI/Secret Service). The
`connections.toml` file only references credential IDs. For development, `credential_id` can also
reference an environment variable with the `env:` prefix.

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
row_limit = 200
enable_suggestions = true

[startup]
enabled = true
show_progress = true

[ui.window.main]
show_menu = true
show_iconbar = true
replicate_menu = true
replicate_iconbar = true

[ui.window.sql_editor]
show_menu = true
show_iconbar = true
replicate_menu = true
replicate_iconbar = false

[ui.window.monitoring]
show_menu = false
show_iconbar = true
replicate_menu = true
replicate_iconbar = false

[ui.window.users_roles]
show_menu = false
show_iconbar = true
replicate_menu = true
replicate_iconbar = false

[ui.window.diagram]
show_menu = true
show_iconbar = true
replicate_menu = true
replicate_iconbar = true

[network]
connect_timeout_ms = 5000
query_timeout_ms = 0
```

## connections.toml (example)
```toml
[[connection]]
name = "Local ScratchBird"
host = "127.0.0.1"
port = 3092
database = "/data/scratchbird/demo.sdb"
username = "sysdba"
credential_id = "scratchrobin:local-sysdba"
application_name = "scratchrobin"
role = "app_readonly"
ssl_mode = "prefer"
ssl_root_cert = "/etc/ssl/certs/ca-certificates.crt"
ssl_cert = "/etc/ssl/certs/scratchrobin-client.crt"
ssl_key = "/etc/ssl/private/scratchrobin-client.key"
ssl_password = "optional-passphrase"
options = "-c search_path=users.public"
# backend = "mock"
# fixture_path = "config/fixtures/default.json"
```

### External backends (examples)
PostgreSQL:
```toml
[[connection]]
name = "Postgres"
backend = "postgresql"
host = "127.0.0.1"
port = 5432
database = "postgres"
username = "postgres"
credential_id = "env:PGPASSWORD"
ssl_mode = "prefer"
application_name = "scratchrobin"
role = "reporting"
ssl_root_cert = "/etc/ssl/certs/ca-certificates.crt"
ssl_cert = "/etc/ssl/certs/pg-client.crt"
ssl_key = "/etc/ssl/private/pg-client.key"
ssl_password = "optional-passphrase"
options = "-c search_path=public"
```

MySQL/MariaDB:
```toml
[[connection]]
name = "MySQL"
backend = "mysql"
host = "127.0.0.1"
port = 3306
database = "mysql"
username = "root"
credential_id = "env:MYSQL_PWD"
ssl_mode = "prefer"
ssl_root_cert = "/etc/ssl/certs/ca-certificates.crt"
ssl_cert = "/etc/ssl/certs/mysql-client.crt"
ssl_key = "/etc/ssl/private/mysql-client.key"
```

Firebird:
```toml
[[connection]]
name = "Firebird"
backend = "firebird"
host = "127.0.0.1"
port = 3050
database = "/data/firebird/demo.fdb"
username = "sysdba"
credential_id = "env:ISC_PASSWORD"
role = "readwrite"
```

### Mock backend
Use `backend = "mock"` and `fixture_path` to load canned responses from JSON fixtures.
Fixture schema is documented in `docs/fixtures/README.md`.

### Advanced connection fields
- `application_name`: Client label for audit/monitoring (native + PostgreSQL).
- `role`: Default role name (PostgreSQL via libpq options; Firebird via SQL role).
- `options`: Extra driver options (PostgreSQL `options` parameter).
- `ssl_root_cert`, `ssl_cert`, `ssl_key`, `ssl_password`: TLS files (backend support varies).

### Environment-backed credentials
```toml
credential_id = "env:SCRATCHROBIN_PASSWORD"
```

## PostgreSQL Test DSN
The optional PostgreSQL cancel integration test reads `SCRATCHROBIN_TEST_PG_DSN` as a libpq-style
key/value string. Example:
```bash
export SCRATCHROBIN_TEST_PG_DSN="host=127.0.0.1 port=5432 dbname=postgres user=postgres password=secret sslmode=disable"
```
To avoid embedding a password, you may supply `password_env=ENV_VAR_NAME` instead of `password=`.

## MySQL Test DSN
The optional MySQL integration test reads `SCRATCHROBIN_TEST_MYSQL_DSN` as a key/value string.
Example:
```bash
export SCRATCHROBIN_TEST_MYSQL_DSN="host=127.0.0.1 port=3306 database=mysql user=root password_env=MYSQL_PWD"
```

## Firebird Test DSN
The optional Firebird integration test reads `SCRATCHROBIN_TEST_FB_DSN` as a key/value string.
Example:
```bash
export SCRATCHROBIN_TEST_FB_DSN="host=127.0.0.1 port=3050 database=/data/firebird/demo.fdb user=sysdba password_env=ISC_PASSWORD"
```

## Testing
Env-gated integration tests:
- `SCRATCHROBIN_TEST_PG_DSN`: enables PostgreSQL cancel integration (`SELECT pg_sleep(30)` + cancel).
- `SCRATCHROBIN_TEST_MYSQL_DSN`: enables MySQL connectivity test (`SELECT 1`).
- `SCRATCHROBIN_TEST_FB_DSN`: enables Firebird connectivity test (`SELECT 1 FROM RDB$DATABASE`).
