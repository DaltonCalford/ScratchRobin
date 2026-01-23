---
id: reference.configuration.files
title: Configuration Files
category: reference
window: Preferences
backends: ["scratchbird", "postgresql", "mysql", "firebird", "mock"]
---

# Configuration Files

ScratchRobin reads TOML configuration from the user config directory.

Files:
- scratchrobin.toml: application settings (UI, editor, startup)
- connections.toml: connection profiles (no plaintext passwords)

Passwords are stored in the OS credential store. To use environment variables
for credentials, set credential_id to env:NAME.
