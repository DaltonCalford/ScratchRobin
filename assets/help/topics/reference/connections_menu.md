---
id: reference.connections.menu
title: Connections Menu
category: reference
window: CatalogBrowser
commands: ["command.connection.server.create", "command.connection.database.connect"]
backends: ["scratchbird", "postgresql", "mysql", "firebird", "mock"]
---

# Connections Menu

The Connections menu is the primary entry point for creating and attaching to
servers, clusters, databases, projects, and diagrams. It mirrors a classic
file-open workflow but uses database terms:

- Create: new server, cluster, or database resources
- Connect: attach to an existing resource
- Disconnect: end a session without dropping the resource
- Drop: remove a resource from the backend
- Remove: remove UI-only entries from recent lists

Quick Connections lists recent profiles for faster access.
