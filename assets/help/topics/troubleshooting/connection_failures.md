---
id: troubleshooting.connection.failures
title: Connection Failures
category: troubleshooting
backends: ["scratchbird", "postgresql", "mysql", "firebird", "mock"]
---

# Connection Failures

If a connection fails:

- Confirm host, port, and database path
- Verify credentials exist in the OS credential store
- Ensure any required client library (libpq/mysqlclient/fbclient) is installed
- Check firewall rules for remote connections

Use the Messages pane in the SQL Editor for full error text.
