---
id: ui.catalog_browser.overview
title: Catalog Browser Overview
category: ui
window: CatalogBrowser
commands: ["command.window.monitoring", "command.window.users_roles"]
backends: ["scratchbird", "postgresql", "mysql", "firebird", "mock"]
---

# Catalog Browser Overview

The catalog browser is the landing surface for ScratchRobin. It shows the
current connection list and the object tree for the selected catalog.

Use the filter box to narrow the tree, then select an object to see:
- Overview metadata
- DDL text (where available)
- Dependency notes

Right-click a tree node for quick actions like copying names, opening a SQL
editor, or refreshing the node.
