# ScratchRobin Integration Guide

## Overview

This guide provides comprehensive information for integrating ScratchRobin with existing systems, tools, and workflows. Whether you're integrating with CI/CD pipelines, monitoring systems, or custom applications, this guide will help you achieve seamless integration.

## API Integration

### REST API Integration

#### Authentication
```bash
# Get API token
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "username": "admin",
    "password": "password"
  }'

# Use token in subsequent requests
curl -X GET http://localhost:8080/api/v1/connections \
  -H "Authorization: Bearer YOUR_TOKEN_HERE"
```

#### Database Operations
```python
import requests

class ScratchRobinClient:
    def __init__(self, base_url, api_key):
        self.base_url = base_url
        self.api_key = api_key
        self.session = requests.Session()
        self.session.headers.update({
            'X-API-Key': api_key,
            'Content-Type': 'application/json'
        })

    def create_connection(self, connection_info):
        """Create a new database connection"""
        response = self.session.post(
            f"{self.base_url}/api/v1/connections",
            json=connection_info
        )
        return response.json()

    def execute_query(self, connection_id, sql):
        """Execute SQL query"""
        response = self.session.post(
            f"{self.base_url}/api/v1/queries/execute",
            json={
                'connectionId': connection_id,
                'sql': sql,
                'timeout': 30000
            }
        )
        return response.json()

    def get_schema_info(self, connection_id, schema_name):
        """Get schema information"""
        response = self.session.get(
            f"{self.base_url}/api/v1/schemas/{schema_name}",
            params={'connectionId': connection_id}
        )
        return response.json()
```

### Webhook Integration

#### Configuring Webhooks
```json
{
  "webhooks": {
    "enabled": true,
    "endpoints": [
      {
        "name": "query_completion",
        "url": "https://your-app.com/webhooks/query-completed",
        "events": ["query.completed", "query.failed"],
        "headers": {
          "Authorization": "Bearer webhook_secret_token"
        },
        "timeout": 5000,
        "retries": 3
      },
      {
        "name": "connection_status",
        "url": "https://monitoring.your-app.com/webhooks/connection-status",
        "events": ["connection.connected", "connection.disconnected", "connection.error"],
        "timeout": 10000,
        "retries": 5
      }
    ]
  }
}
```

#### Webhook Payload Examples

Query Completion Event:
```json
{
  "event": "query.completed",
  "timestamp": "2024-01-15T10:30:00Z",
  "data": {
    "queryId": "query_12345",
    "connectionId": "conn_67890",
    "sql": "SELECT * FROM users WHERE active = true",
    "executionTime": 45,
    "rowCount": 1000,
    "status": "success"
  }
}
```

Connection Error Event:
```json
{
  "event": "connection.error",
  "timestamp": "2024-01-15T10:35:00Z",
  "data": {
    "connectionId": "conn_67890",
    "error": "Connection timeout",
    "errorCode": "CONNECTION_TIMEOUT",
    "retryCount": 2
  }
}
```

## CI/CD Integration

### GitHub Actions Integration

#### Database Schema Testing
```yaml
name: Database Schema Tests

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  schema-test:
    runs-on: ubuntu-latest

    services:
      postgres:
        image: postgres:13
        env:
          POSTGRES_PASSWORD: postgres
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5
        ports:
          - 5432:5432

    steps:
    - uses: actions/checkout@v3

    - name: Setup ScratchRobin
      run: |
        wget https://github.com/DaltonCalford/ScratchRobin/releases/latest/download/scratchrobin-cli-linux-x64.tar.gz
        tar -xzf scratchrobin-cli-linux-x64.tar.gz
        sudo mv scratchrobin-cli /usr/local/bin/

    - name: Test Database Connection
      run: |
        scratchrobin-cli connection test \
          --host localhost \
          --port 5432 \
          --database postgres \
          --username postgres \
          --password postgres

    - name: Validate Schema
      run: |
        scratchrobin-cli schema validate \
          --connection postgres \
          --schema public \
          --config .scratchrobin/schema-rules.json

    - name: Generate Schema Report
      run: |
        scratchrobin-cli schema report \
          --connection postgres \
          --output schema-report.json \
          --format json
```

#### Backup Automation
```yaml
name: Database Backup

on:
  schedule:
    - cron: '0 2 * * *'  # Daily at 2 AM
  workflow_dispatch:

jobs:
  backup:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Setup ScratchRobin CLI
      run: |
        wget https://github.com/DaltonCalford/ScratchRobin/releases/latest/download/scratchrobin-cli-linux-x64.tar.gz
        tar -xzf scratchrobin-cli-linux-x64.tar.gz
        sudo mv scratchrobin-cli /usr/local/bin/

    - name: Create Backup
      run: |
        scratchrobin-cli backup create \
          --connection production_db \
          --output backup_$(date +%Y%m%d_%H%M%S).sql \
          --compress \
          --encrypt

    - name: Upload Backup to S3
      uses: aws-actions/configure-aws-credentials@v2
      with:
        aws-access-key-id: ${{ secrets.AWS_ACCESS_KEY_ID }}
        aws-secret-access-key: ${{ secrets.AWS_SECRET_ACCESS_KEY }}
        aws-region: us-west-2

    - run: |
        aws s3 cp backup_*.sql s3://scratchrobin-backups/
        aws s3 cp backup_*.sql s3://scratchrobin-backups/latest.sql

    - name: Cleanup
      run: rm backup_*.sql
```

### Jenkins Integration

#### Jenkins Pipeline
```groovy
pipeline {
    agent any

    environment {
        SCRATCHROBIN_URL = 'http://scratchrobin.company.com:8080'
        SCRATCHROBIN_API_KEY = credentials('scratchrobin-api-key')
    }

    stages {
        stage('Database Migration') {
            steps {
                sh '''
                    # Apply database migrations
                    curl -X POST ${SCRATCHROBIN_URL}/api/v1/migrations/apply \
                         -H "X-API-Key: ${SCRATCHROBIN_API_KEY}" \
                         -H "Content-Type: application/json" \
                         -d @migrations.json
                '''
            }
        }

        stage('Schema Validation') {
            steps {
                sh '''
                    # Validate schema changes
                    curl -X POST ${SCRATCHROBIN_URL}/api/v1/schema/validate \
                         -H "X-API-Key: ${SCRATCHROBIN_API_KEY}" \
                         -H "Content-Type: application/json" \
                         -d '{
                           "connectionId": "dev_database",
                           "validationRules": ["naming_conventions", "data_types", "constraints"]
                         }'
                '''
            }
        }

        stage('Performance Test') {
            steps {
                sh '''
                    # Run performance tests
                    curl -X POST ${SCRATCHROBIN_URL}/api/v1/performance/test \
                         -H "X-API-Key: ${SCRATCHROBIN_API_KEY}" \
                         -H "Content-Type: application/json" \
                         -d @performance-test-config.json
                '''
            }
        }
    }

    post {
        always {
            sh '''
                # Generate test report
                curl -X GET ${SCRATCHROBIN_URL}/api/v1/reports/latest \
                     -H "X-API-Key: ${SCRATCHROBIN_API_KEY}" \
                     -o test-report.json
            '''
            archiveArtifacts artifacts: 'test-report.json', fingerprint: true
        }
    }
}
```

## Monitoring Integration

### Prometheus Integration

#### Prometheus Configuration
```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'scratchrobin'
    static_configs:
      - targets: ['scratchrobin.company.com:8080']
    metrics_path: '/metrics'
    scrape_interval: 30s

  - job_name: 'scratchrobin_database'
    static_configs:
      - targets: ['database.company.com:5432']
    metrics_path: '/metrics'
    scrape_interval: 60s
```

#### Grafana Dashboard Integration
```json
{
  "dashboard": {
    "title": "ScratchRobin Monitoring",
    "tags": ["database", "monitoring"],
    "timezone": "UTC",
    "panels": [
      {
        "title": "Active Connections",
        "type": "stat",
        "targets": [
          {
            "expr": "scratchrobin_connections_active",
            "legendFormat": "Active Connections"
          }
        ]
      },
      {
        "title": "Query Performance",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(scratchrobin_queries_total[5m])",
            "legendFormat": "Queries/sec"
          },
          {
            "expr": "scratchrobin_query_duration_seconds{quantile=\"0.95\"}",
            "legendFormat": "95th Percentile"
          }
        ]
      },
      {
        "title": "Memory Usage",
        "type": "bargauge",
        "targets": [
          {
            "expr": "scratchrobin_memory_usage_bytes / scratchrobin_memory_limit_bytes * 100",
            "legendFormat": "Memory Usage %"
          }
        ]
      }
    ]
  }
}
```

### ELK Stack Integration

#### Logstash Configuration
```conf
# logstash.conf
input {
  http {
    host => "0.0.0.0"
    port => 8080
    codec => json
  }
}

filter {
  if [type] == "scratchrobin" {
    grok {
      match => { "message" => "%{TIMESTAMP_ISO8601:timestamp} %{LOGLEVEL:level} %{DATA:component} %{GREEDYDATA:message}" }
    }

    mutate {
      add_field => { "service" => "scratchrobin" }
    }
  }
}

output {
  elasticsearch {
    hosts => ["elasticsearch:9200"]
    index => "scratchrobin-%{+YYYY.MM.dd}"
  }

  stdout {
    codec => rubydebug
  }
}
```

#### Kibana Visualization
```json
{
  "visualization": {
    "title": "Query Performance Over Time",
    "type": "line",
    "aggs": [
      {
        "id": "1",
        "type": "date_histogram",
        "schema": "segment",
        "params": {
          "field": "@timestamp",
          "interval": "1h",
          "min_doc_count": 1
        }
      },
      {
        "id": "2",
        "type": "avg",
        "schema": "metric",
        "params": {
          "field": "execution_time"
        }
      }
    ]
  }
}
```

## Security Integration

### LDAP/Active Directory Integration

#### LDAP Configuration
```json
{
  "authentication": {
    "method": "ldap",
    "ldap": {
      "url": "ldap://ldap.company.com:389",
      "baseDN": "dc=company,dc=com",
      "bindDN": "cn=service,ou=services,dc=company,dc=com",
      "bindPassword": "encrypted_password",
      "userSearchFilter": "(sAMAccountName={username})",
      "groupSearchFilter": "(member={dn})",
      "adminGroups": ["DBA", "SystemAdmin"],
      "ssl": {
        "enabled": true,
        "verifyCertificate": true,
        "caFile": "/path/to/ca-cert.pem"
      }
    }
  }
}
```

#### SAML Integration
```json
{
  "authentication": {
    "method": "saml",
    "saml": {
      "idpMetadataUrl": "https://idp.company.com/metadata.xml",
      "spEntityId": "scratchrobin.company.com",
      "assertionConsumerServiceUrl": "https://scratchrobin.company.com/saml/acs",
      "singleLogoutServiceUrl": "https://scratchrobin.company.com/saml/slo",
      "nameIdFormat": "urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress",
      "signAuthnRequest": true,
      "wantAssertionsSigned": true,
      "wantResponsesSigned": true,
      "certificateFile": "/path/to/sp-cert.pem",
      "privateKeyFile": "/path/to/sp-key.pem",
      "idpCertificateFile": "/path/to/idp-cert.pem"
    }
  }
}
```

### OAuth2 Integration

#### Google OAuth2 Setup
```json
{
  "authentication": {
    "method": "oauth2",
    "oauth2": {
      "provider": "google",
      "clientId": "your_google_client_id",
      "clientSecret": "encrypted_secret",
      "redirectUri": "https://scratchrobin.company.com/oauth2/callback",
      "scopes": ["openid", "profile", "email"],
      "tokenUrl": "https://accounts.google.com/o/oauth2/token",
      "authUrl": "https://accounts.google.com/o/oauth2/auth",
      "userInfoUrl": "https://www.googleapis.com/oauth2/v2/userinfo",
      "groupMapping": {
        "admin": ["scratchrobin-admins@company.com"],
        "dba": ["database-team@company.com"],
        "user": ["default"]
      }
    }
  }
}
```

## Database Integration

### PostgreSQL Integration

#### Connection Pool Configuration
```json
{
  "database": {
    "type": "postgresql",
    "connectionPool": {
      "minConnections": 2,
      "maxConnections": 20,
      "connectionTimeout": 30000,
      "idleTimeout": 300000,
      "healthCheckInterval": 60000,
      "retryAttempts": 3,
      "retryDelay": 1000,
      "statementPooling": true,
      "preparedStatements": true
    },
    "ssl": {
      "mode": "require",
      "certFile": "/path/to/client-cert.pem",
      "keyFile": "/path/to/client-key.pem",
      "caFile": "/path/to/ca-cert.pem"
    }
  }
}
```

### MySQL Integration

#### MySQL-Specific Configuration
```json
{
  "database": {
    "type": "mysql",
    "charset": "utf8mb4",
    "collation": "utf8mb4_unicode_ci",
    "connectionPool": {
      "minConnections": 2,
      "maxConnections": 15,
      "connectionTimeout": 30000,
      "idleTimeout": 300000,
      "healthCheckInterval": 60000,
      "autoReconnect": true,
      "reconnectDelay": 5000
    },
    "ssl": {
      "enabled": true,
      "verifyServerCertificate": true,
      "caFile": "/path/to/ca-cert.pem"
    }
  }
}
```

## Custom Plugin Development

### Plugin Architecture

#### Plugin Interface
```cpp
// Plugin interface definition
class IScratchRobinPlugin {
public:
    virtual ~IScratchRobinPlugin() = default;

    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;

    virtual bool initialize(const PluginConfig& config) = 0;
    virtual bool shutdown() = 0;

    virtual void onDatabaseConnect(const ConnectionInfo& info) = 0;
    virtual void onQueryExecute(const QueryInfo& info) = 0;
    virtual void onQueryComplete(const QueryResult& result) = 0;

    virtual std::vector<std::string> getSupportedEvents() const = 0;
    virtual bool handleEvent(const std::string& event, const EventData& data) = 0;
};

// Example custom plugin
class AuditPlugin : public IScratchRobinPlugin {
public:
    std::string getName() const override { return "Audit Plugin"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Comprehensive audit logging plugin";
    }

    bool initialize(const PluginConfig& config) override {
        // Initialize audit log storage
        return true;
    }

    void onQueryExecute(const QueryInfo& info) override {
        // Log query execution
        auditLog_.logQuery(info);
    }

    void onQueryComplete(const QueryResult& result) override {
        // Log query completion with performance metrics
        auditLog_.logQueryCompletion(result);
    }

private:
    AuditLogger auditLog_;
};
```

#### Plugin Configuration
```json
{
  "plugins": {
    "enabled": true,
    "directory": "/opt/scratchrobin/plugins",
    "plugins": [
      {
        "name": "audit_plugin",
        "enabled": true,
        "config": {
          "logLevel": "info",
          "logFile": "/var/log/scratchrobin/audit.log",
          "maxFileSize": "100MB",
          "retention": "90d"
        }
      },
      {
        "name": "monitoring_plugin",
        "enabled": true,
        "config": {
          "metricsEndpoint": "http://monitoring.company.com:9091",
          "collectionInterval": 30,
          "enableDetailedMetrics": true
        }
      }
    ]
  }
}
```

## Docker Integration

### Docker Compose Setup

```yaml
# docker-compose.yml
version: '3.8'

services:
  scratchrobin:
    image: scratchrobin/scratchrobin:latest
    container_name: scratchrobin
    ports:
      - "8080:8080"    # REST API
      - "3000:3000"    # Web UI
      - "5433:5433"    # Additional services
    environment:
      - SCRATCHROBIN_SERVER_HOST=0.0.0.0
      - SCRATCHROBIN_DATABASE_HOST=postgres
      - SCRATCHROBIN_DATABASE_PORT=5432
      - SCRATCHROBIN_DATABASE_USERNAME=scratchrobin
      - SCRATCHROBIN_DATABASE_PASSWORD=${DB_PASSWORD}
    volumes:
      - scratchrobin_data:/app/data
      - scratchrobin_logs:/app/logs
      - ./config:/app/config
    depends_on:
      - postgres
    networks:
      - scratchrobin_network
    restart: unless-stopped

  postgres:
    image: postgres:15-alpine
    container_name: scratchrobin_db
    environment:
      - POSTGRES_DB=scratchrobin
      - POSTGRES_USER=scratchrobin
      - POSTGRES_PASSWORD=${DB_PASSWORD}
    volumes:
      - postgres_data:/var/lib/postgresql/data
    networks:
      - scratchrobin_network
    restart: unless-stopped

  pgadmin:
    image: dpage/pgadmin4:latest
    container_name: scratchrobin_admin
    environment:
      - PGADMIN_DEFAULT_EMAIL=admin@company.com
      - PGADMIN_DEFAULT_PASSWORD=${PGADMIN_PASSWORD}
    ports:
      - "5050:80"
    depends_on:
      - postgres
    networks:
      - scratchrobin_network
    restart: unless-stopped

volumes:
  scratchrobin_data:
  scratchrobin_logs:
  postgres_data:

networks:
  scratchrobin_network:
    driver: bridge
```

### Kubernetes Integration

#### Kubernetes Deployment
```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: scratchrobin
  namespace: database-tools
spec:
  replicas: 3
  selector:
    matchLabels:
      app: scratchrobin
  template:
    metadata:
      labels:
        app: scratchrobin
    spec:
      containers:
      - name: scratchrobin
        image: scratchrobin/scratchrobin:latest
        ports:
        - containerPort: 8080
          name: api
        - containerPort: 3000
          name: web
        env:
        - name: SCRATCHROBIN_SERVER_HOST
          value: "0.0.0.0"
        - name: SCRATCHROBIN_DATABASE_HOST
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: host
        resources:
          requests:
            memory: "512Mi"
            cpu: "250m"
          limits:
            memory: "2Gi"
            cpu: "1000m"
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
```

#### Service Configuration
```yaml
# service.yaml
apiVersion: v1
kind: Service
metadata:
  name: scratchrobin
  namespace: database-tools
spec:
  selector:
    app: scratchrobin
  ports:
  - name: api
    port: 8080
    targetPort: 8080
  - name: web
    port: 3000
    targetPort: 3000
  type: LoadBalancer
```

## Best Practices

### Integration Guidelines

1. **Use Connection Pooling**: Always configure appropriate connection pool settings
2. **Implement Retry Logic**: Handle transient failures with exponential backoff
3. **Monitor Performance**: Set up monitoring and alerting for integration points
4. **Security First**: Use HTTPS/TLS for all API communications
5. **Error Handling**: Implement comprehensive error handling and logging
6. **Rate Limiting**: Respect API rate limits and implement client-side throttling
7. **Version Management**: Use API versioning for compatibility
8. **Testing**: Implement comprehensive integration tests

### Security Considerations

1. **API Key Management**: Rotate API keys regularly and store securely
2. **Certificate Management**: Keep SSL certificates up to date
3. **Access Control**: Implement least privilege access for integrations
4. **Audit Logging**: Enable comprehensive audit logging for all operations
5. **Data Encryption**: Encrypt sensitive data in transit and at rest
6. **Network Security**: Use VPNs or private networks for internal integrations
7. **Monitoring**: Monitor integration health and security events

### Performance Optimization

1. **Connection Reuse**: Use connection pooling to avoid connection overhead
2. **Query Optimization**: Optimize queries before execution
3. **Caching**: Implement appropriate caching strategies
4. **Async Processing**: Use asynchronous processing for long-running operations
5. **Load Balancing**: Distribute load across multiple instances
6. **Resource Limits**: Set appropriate resource limits and quotas
7. **Monitoring**: Monitor performance metrics and optimize bottlenecks

This integration guide provides comprehensive information for successfully integrating ScratchRobin with your existing systems and workflows. Following these guidelines will ensure reliable, secure, and high-performance integrations.
