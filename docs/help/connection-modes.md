# Connection Modes

ScratchRobin supports three connection modes to ScratchBird databases, each optimized for different use cases.

## Embedded Mode

**Best for**: Single-user development, edge deployments, testing

Embedded mode runs the database engine directly within the ScratchRobin process. This provides the fastest performance with zero network overhead.

### Advantages
- **Fastest performance** - No IPC or network overhead
- **Zero-copy** - Direct memory access to data
- **No server required** - Database is self-contained
- **Ideal for development** - Quick iteration cycles

### Limitations
- Single-user only
- Database file must be local
- No remote access

### Configuration
```
Connection Mode: Embedded
Database Path: /path/to/database.sbd
Username: (optional)
Password: (optional)
```

## IPC Mode

**Best for**: Local multi-user, production on same host

IPC (Inter-Process Communication) mode uses Unix domain sockets (Linux/macOS) or named pipes (Windows) to communicate with a local ScratchBird server.

### Advantages
- **Low latency** - Faster than TCP for local connections
- **Multi-user** - Supports concurrent connections
- **File permissions** - Uses filesystem for access control
- **No network stack** - Bypasses TCP/IP overhead

### Configuration
```
Connection Mode: IPC
Socket Path: /var/run/scratchbird/mydb.sock
Database: mydb
Username: myuser
Password: mypassword
```

### Default Socket Paths

**Linux/macOS**:
- Default: `/var/run/scratchbird/scratchbird.sock`
- Per-database: `/var/run/scratchbird/{database}.sock`

**Windows**:
- Default: `\\.\pipe\scratchbird`
- Per-database: `\\.\pipe\scratchbird-{database}`

## Network Mode

**Best for**: Remote access, production servers, distributed systems

Network mode uses TCP/IP to connect to a ScratchBird server over the network.

### Advantages
- **Remote access** - Connect from anywhere
- **Scalable** - Supports many concurrent clients
- **Flexible** - Works across subnets and VPNs
- **Production-ready** - Standard client-server architecture

### Configuration
```
Connection Mode: Network
Host: db.example.com
Port: 3092
Database: production
Username: appuser
Password: ********
SSL Mode: require
```

### SSL/TLS Options

- **disable** - No encryption (not recommended)
- **allow** - Encrypt if server supports it
- **prefer** - Prefer encryption but allow unencrypted
- **require** - Require encryption (default)
- **verify-ca** - Require encryption and verify CA
- **verify-full** - Require encryption, verify CA and hostname

## Choosing a Connection Mode

| Scenario | Recommended Mode |
|----------|-----------------|
| Local development | Embedded |
| Testing/CI | Embedded |
| Single-user production | Embedded or IPC |
| Multi-user on same server | IPC |
| Remote access required | Network |
| High security requirements | Network with SSL |
| Maximum performance | Embedded |

## Switching Connection Modes

To change the connection mode for an existing profile:

1. Open **Window → Connections**
2. Select the connection profile
3. Click **Edit**
4. Choose the desired **Connection Mode**
5. Update mode-specific settings
6. Click **Test Connection** to verify
7. Click **Save**

## Troubleshooting

### Embedded Mode
- **"Failed to open database"** - Check file permissions and path
- **"Database is locked"** - Another process may be using the database

### IPC Mode
- **"No such file or directory"** - Server not running or socket path incorrect
- **"Permission denied"** - Check socket file permissions

### Network Mode
- **"Connection refused"** - Server not running or firewall blocking port
- **"SSL connection failed"** - Check SSL certificates and mode
- **"Timeout"** - Network latency or server overload
