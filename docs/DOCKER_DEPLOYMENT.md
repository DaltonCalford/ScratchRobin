# Docker Deployment Guide

## Overview

ScratchBird can be deployed as a Docker container with configurable database services (Native, PostgreSQL, MySQL, Firebird). ScratchRobin provides integrated Docker management to start, stop, configure, and monitor containers.

## Quick Start

### Using ScratchRobin UI

```
1. Open Docker Manager (Tools → Docker → Manage Containers)
2. Select a template (Development, Production, Minimal, etc.)
3. Configure services (enable/disable PostgreSQL, MySQL, etc.)
4. Set resource limits (memory, CPU)
5. Choose directory mappings
6. Click "Start Container"
```

### Using Docker CLI

```bash
# Pull the image
docker pull scratchbird:latest

# Run with default settings
docker run -d \
  --name scratchbird-server \
  -p 3092:3092 \\      # ScratchBird Native (SBWP v1.1)
  -p 5432:5432 \\      # PostgreSQL
  -p 3306:3306 \\      # MySQL
  -p 3050:3050 \\      # Firebird
  -v $(pwd)/data:/var/lib/scratchbird/data \
  scratchbird:latest

# Run with custom configuration
docker run -d \
  --name scratchbird-server \
  --memory=4G \
  --cpus=2 \
  -e SB_ENABLE_POSTGRES=true \
  -e SB_ENABLE_MYSQL=false \
  -e SB_MEMORY_LIMIT=4G \
  -e SB_MAX_CONNECTIONS=200 \
  -p 3050:3050 \
  -p 5432:5432 \
  -v /host/data:/var/lib/scratchbird/data \
  -v /host/config:/etc/scratchbird \
  scratchbird:latest
```

### Using Docker Compose

```yaml
version: '3.8'

services:
  scratchbird:
    image: scratchbird:latest
    container_name: scratchbird-server
    restart: unless-stopped
    
    ports:
      - "3050:3050"    # Native ScratchBird
      - "5432:5432"    # PostgreSQL
      - "3306:3306"    # MySQL
      - "3051:3051"    # Firebird
      - "8080:8080"    # Web Admin
      - "9090:9090"    # Metrics
    
    environment:
      # Service enablement
      SB_ENABLE_NATIVE: "true"
      SB_ENABLE_POSTGRES: "true"
      SB_ENABLE_MYSQL: "true"
      SB_ENABLE_FIREBIRD: "true"
      
      # Resource limits
      SB_MEMORY_LIMIT: "4G"
      SB_MAX_CONNECTIONS: 200
      
      # Performance
      SB_SHARED_BUFFERS: "512MB"
      SB_WORK_MEM: "128MB"
      
      # Security
      SB_SSL_MODE: "require"
      SB_REQUIRE_AUTH: "true"
      
      # Logging
      SB_LOG_LEVEL: "INFO"
      
      # Backup
      SB_BACKUP_ENABLED: "true"
      SB_BACKUP_SCHEDULE: "0 2 * * *"
    
    volumes:
      - ./data:/var/lib/scratchbird/data
      - ./config:/etc/scratchbird
      - ./logs:/var/log/scratchbird
      - ./backups:/var/lib/scratchbird/backups
    
    networks:
      - scratchbird-network
    
    healthcheck:
      test: ["CMD", "/usr/local/bin/healthcheck.sh"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 60s

networks:
  scratchbird-network:
    driver: bridge
```

## Configuration Options

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SB_IP` | 0.0.0.0 | IP address to bind |
| `SB_DATA_DIR` | /var/lib/scratchbird/data | Data directory |
| `SB_CONFIG_DIR` | /etc/scratchbird | Configuration directory |
| `SB_LOG_DIR` | /var/log/scratchbird | Log directory |
| `SB_TEMP_DIR` | /var/tmp/scratchbird | Temporary directory |
| `SB_MEMORY_LIMIT` | 2G | Memory limit |
| `SB_DISK_LIMIT` | 100G | Disk usage limit |

### Service Ports

| Variable | Default | Service | Protocol |
|----------|---------|---------|----------|
| `SB_NATIVE_PORT` | 3092 | ScratchBird Native | SBWP v1.1 |
| `SB_POSTGRES_PORT` | 5432 | PostgreSQL | PostgreSQL wire |
| `SB_MYSQL_PORT` | 3306 | MySQL | MySQL wire |
| `SB_FIREBIRD_PORT` | 3050 | Firebird | Firebird wire |

### Service Enablement

| Variable | Default | Description |
|----------|---------|-------------|
| `SB_ENABLE_NATIVE` | true | Enable ScratchBird native |
| `SB_ENABLE_POSTGRES` | true | Enable PostgreSQL |
| `SB_ENABLE_MYSQL` | true | Enable MySQL |
| `SB_ENABLE_FIREBIRD` | true | Enable Firebird |

### Performance Settings

| Variable | Default | Description |
|----------|---------|-------------|
| `SB_MAX_CONNECTIONS` | 100 | Maximum connections |
| `SB_SHARED_BUFFERS` | 256MB | Shared memory buffers |
| `SB_WORK_MEM` | 64MB | Work memory per query |
| `SB_MAINTENANCE_WORK_MEM` | 256MB | Maintenance operation memory |
| `SB_EFFECTIVE_CACHE_SIZE` | 1GB | Expected cache size |

### Security Settings

| Variable | Default | Description |
|----------|---------|-------------|
| `SB_SSL_MODE` | prefer | SSL mode (disable, allow, prefer, require) |
| `SB_SSL_CERT_PATH` | /etc/scratchbird/ssl/server.crt | SSL certificate |
| `SB_SSL_KEY_PATH` | /etc/scratchbird/ssl/server.key | SSL private key |
| `SB_REQUIRE_AUTH` | true | Require authentication |

### Backup Settings

| Variable | Default | Description |
|----------|---------|-------------|
| `SB_BACKUP_ENABLED` | true | Enable automatic backups |
| `SB_BACKUP_SCHEDULE` | 0 2 * * * | Cron schedule for backups |
| `SB_BACKUP_RETENTION_DAYS` | 7 | Days to retain backups |
| `SB_BACKUP_DIR` | /var/lib/scratchbird/backups | Backup directory |

## ScratchRobin Docker Manager

### Starting a Container

```cpp
// Using the C++ API
DockerContainerConfig config;
config.container_name = "my-scratchbird";
config.EnableService("postgres");
config.EnableService("mysql");
config.memory_limit = "4G";
config.data_directory = "/home/user/scratchbird-data";

DockerContainerManager manager;
manager.StartContainer(config);
```

### Monitoring Status

```cpp
// Get container status
DockerContainerStatus status = manager.GetContainerStatus("my-scratchbird");

if (status.IsRunning()) {
    std::cout << "CPU: " << status.cpu_percent << "%" << std::endl;
    std::cout << "Memory: " << status.memory_percent << "%" << std::endl;
    std::cout << "Health: " << status.health << std::endl;
}

// Start monitoring with callback
manager.StartStatusMonitoring("my-scratchbird", [](const DockerContainerStatus& status) {
    UpdateUI(status);
}, 5);  // Update every 5 seconds
```

### Streaming Logs

```cpp
manager.StreamContainerLogs("my-scratchbird", [](const std::string& log_line) {
    AppendToLogWindow(log_line);
});
```

## Templates

### Development Template
```cpp
auto config = DockerTemplates::DevelopmentTemplate();
// - All services enabled
// - 2GB memory
// - Debug logging
// - Auto-restart on failure
```

### Production Template
```cpp
auto config = DockerTemplates::ProductionTemplate();
// - SSL required
// - 8GB memory
// - Resource limits enforced
// - Backups enabled
// - Health checks
```

### Minimal Template
```cpp
auto config = DockerTemplates::MinimalTemplate();
// - Only ScratchBird native
// - 1GB memory
// - No optional services
```

## Directory Structure

```
scratchbird-deployment/
├── docker-compose.yml          # Docker Compose configuration
├── Dockerfile                  # Custom Dockerfile (optional)
├── data/                       # Persistent data
│   ├── native/
│   ├── postgres/
│   ├── mysql/
│   └── firebird/
├── config/                     # Configuration files
│   ├── scratchbird.conf
│   ├── postgres.conf
│   └── ssl/
│       ├── server.crt
│       └── server.key
├── logs/                       # Log files
│   ├── scratchbird/
│   ├── postgres/
│   └── mysql/
├── backups/                    # Automatic backups
└── scripts/                    # Custom scripts
    └── init.sql               # Initialization script
```

## Backup and Restore

### Automatic Backups

Backups are configured via environment variables:

```yaml
environment:
  SB_BACKUP_ENABLED: "true"
  SB_BACKUP_SCHEDULE: "0 2 * * *"  # Daily at 2 AM
  SB_BACKUP_RETENTION_DAYS: 7
```

### Manual Backup

```bash
# Via ScratchRobin
DockerContainerManager manager;
manager.BackupContainerData("scratchbird-server", "/backup/2024-02-04.tar.gz");

# Via Docker CLI
docker exec scratchbird-server /usr/local/bin/backup.sh
docker cp scratchbird-server:/var/lib/scratchbird/backups/backup.tar.gz ./
```

### Restore from Backup

```bash
# Via ScratchRobin
manager.RestoreContainerData("scratchbird-server", "/backup/2024-02-04.tar.gz");

# Via Docker CLI
docker cp ./backup.tar.gz scratchbird-server:/tmp/
docker exec scratchbird-server /usr/local/bin/restore.sh /tmp/backup.tar.gz
```

## Troubleshooting

### Container Won't Start

```bash
# Check logs
docker logs scratchbird-server

# Check configuration
DockerContainerConfig config = DockerContainerConfig::LoadFromFile("my-config.json");
auto validation = config.Validate();
for (const auto& error : validation.errors) {
    std::cerr << "Error: " << error << std::endl;
}
```

### Port Conflicts

```cpp
// Check if ports are available before starting
if (!config.IsPortAvailable(5432)) {
    // Find next available port
    config.SetServicePort("postgres", 5433);
}
```

### Out of Memory

```bash
# Increase memory limit
docker update --memory=8G scratchbird-server

# Or modify config and restart
config.memory_limit = "8G";
manager.RestartContainer("scratchbird-server");
```

### Health Check Failures

```bash
# Check health status
docker inspect --format='{{.State.Health.Status}}' scratchbird-server

# View health check logs
docker inspect --format='{{json .State.Health}}' scratchbird-server | jq
```

## Advanced Configuration

### Custom Network

```yaml
networks:
  scratchbird-internal:
    internal: true  # No external access
  scratchbird-external:
    driver: bridge
```

### Resource Constraints

```yaml
deploy:
  resources:
    limits:
      cpus: '2.0'
      memory: 4G
    reservations:
      cpus: '1.0'
      memory: 2G
```

### Volume Drivers

```yaml
volumes:
  scratchbird-data:
    driver: local
    driver_opts:
      type: nfs
      o: addr=nfs-server.example.com,rw
      device: ":/nfs/scratchbird"
```

## Security Best Practices

1. **Use secrets for sensitive data**
   ```yaml
   secrets:
     db_password:
       file: ./secrets/db_password.txt
   ```

2. **Enable SSL/TLS**
   ```yaml
   environment:
     SB_SSL_MODE: require
   volumes:
     - ./ssl:/etc/scratchbird/ssl:ro
   ```

3. **Run as non-root user**
   ```dockerfile
   USER scratchbird
   ```

4. **Use read-only filesystem where possible**
   ```yaml
   read_only: true
   tmpfs:
     - /tmp
     - /var/run
   ```

## Integration with ScratchRobin

The Docker Manager panel in ScratchRobin provides:

- **Visual Status**: CPU, memory, disk usage gauges
- **Log Viewer**: Real-time log streaming with filtering
- **Configuration Editor**: Point-and-click configuration
- **Template Gallery**: Pre-configured setups
- **One-Click Actions**: Start, stop, restart, remove
- **Health Monitoring**: Automatic health check display

Access via: `Tools → Docker → Manage Containers`

---

*For more information, see the main [README.md](../README.md)*
