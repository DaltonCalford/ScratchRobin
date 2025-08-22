# Phase 1.3: Dependency Management Implementation

## Overview
This phase focuses on setting up comprehensive dependency management for ScratchRobin, including external libraries, build tools, and security scanning.

## Prerequisites
- ✅ Phase 1.1 (Project Setup) completed
- ✅ Phase 1.2 (Architecture Design) completed
- Git repository initialized
- CMake build system configured
- Basic project structure created

## Implementation Tasks

### Task 1.3.1: Core Dependencies Setup

#### 1.3.1.1: C++ Dependencies Management
**Objective**: Set up vcpkg for C++ dependency management

**Implementation Steps:**
1. Install vcpkg package manager
2. Configure vcpkg integration with CMake
3. Set up dependency manifest file

**Code Changes:**
```bash
# Install vcpkg
cd /opt
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
export VCPKG_ROOT=/opt/vcpkg
```

**File: vcpkg.json**
```json
{
  "name": "scratchrobin",
  "version": "0.1.0",
  "description": "Database management interface for ScratchBird",
  "dependencies": [
    "boost",
    "openssl",
    "zlib",
    "libpqxx",
    "nlohmann-json",
    "spdlog",
    "gtest",
    "qt5-base",
    "qt5-widgets",
    "qt5-network"
  ],
  "overrides": [
    {
      "name": "boost",
      "version": "1.82.0"
    },
    {
      "name": "openssl",
      "version": "3.1.1"
    }
  ]
}
```

**File: CMakeLists.txt (updates)**
```cmake
# Enable vcpkg
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Find packages
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)
```

#### 1.3.1.2: JavaScript/Node.js Dependencies
**Objective**: Set up npm/yarn for frontend dependencies

**Implementation Steps:**
1. Initialize package.json with core dependencies
2. Configure webpack/babel for React application
3. Set up TypeScript configuration

**Code Changes:**

**File: web/package.json**
```json
{
  "name": "scratchrobin-web",
  "version": "0.1.0",
  "private": true,
  "dependencies": {
    "@emotion/react": "^11.10.6",
    "@emotion/styled": "^11.10.6",
    "@mui/material": "^5.12.2",
    "@mui/icons-material": "^5.11.16",
    "@mui/x-data-grid": "^6.3.0",
    "react": "^18.2.0",
    "react-dom": "^18.2.0",
    "react-router-dom": "^6.8.1",
    "axios": "^1.3.4",
    "socket.io-client": "^4.7.0",
    "react-syntax-highlighter": "^15.5.0",
    "react-split-pane": "^0.1.92",
    "react-window": "^1.8.8"
  },
  "devDependencies": {
    "@types/react": "^18.0.28",
    "@types/react-dom": "^18.0.11",
    "typescript": "^4.9.5",
    "webpack": "^5.76.0",
    "webpack-cli": "^5.0.1",
    "webpack-dev-server": "^4.12.0",
    "@babel/core": "^7.21.3",
    "@babel/preset-env": "^7.20.2",
    "@babel/preset-react": "^7.18.6",
    "@babel/preset-typescript": "^7.21.0",
    "eslint": "^8.37.0",
    "prettier": "^2.8.7",
    "jest": "^29.5.0",
    "@testing-library/react": "^14.0.0",
    "@testing-library/jest-dom": "^5.16.5"
  },
  "scripts": {
    "start": "webpack serve --mode development",
    "build": "webpack --mode production",
    "test": "jest",
    "lint": "eslint src --ext .ts,.tsx",
    "format": "prettier --write src/**/*.{ts,tsx,css}"
  }
}
```

**File: web/tsconfig.json**
```json
{
  "compilerOptions": {
    "target": "es2020",
    "lib": ["dom", "dom.iterable", "es2020"],
    "allowJs": true,
    "skipLibCheck": true,
    "esModuleInterop": true,
    "allowSyntheticDefaultImports": true,
    "strict": true,
    "forceConsistentCasingInFileNames": true,
    "noFallthroughCasesInSwitch": true,
    "module": "esnext",
    "moduleResolution": "node",
    "resolveJsonModule": true,
    "isolatedModules": true,
    "noEmit": true,
    "jsx": "react-jsx"
  },
  "include": [
    "src"
  ]
}
```

#### 1.3.1.3: Go Dependencies
**Objective**: Set up Go module dependencies for CLI

**Implementation Steps:**
1. Initialize go.mod with core dependencies
2. Configure module proxy settings
3. Set up dependency injection

**Code Changes:**

**File: cli/go.mod**
```go
module github.com/scratchbird/scratchrobin-cli

go 1.20

require (
    github.com/spf13/cobra v1.7.0
    github.com/spf13/viper v1.15.0
    github.com/spf13/pflag v1.0.5
    github.com/olekukonko/tablewriter v0.0.5
    github.com/fatih/color v1.15.0
    github.com/mattn/go-sqlite3 v1.14.17
    github.com/lib/pq v1.10.9
    github.com/go-sql-driver/mysql v1.7.1
    github.com/gorilla/websocket v1.5.0
    gopkg.in/yaml.v3 v3.0.1
)

// Development dependencies
require (
    github.com/cucumber/godog v0.13.0
    github.com/onsi/ginkgo/v2 v2.11.0
    github.com/onsi/gomega v1.27.8
    github.com/stretchr/testify v1.8.4
)

// Security scanning dependencies
require (
    github.com/sonatypecommunity/nancy v1.0.39
    github.com/securecodewarrior/github-action-gosec v1.0.0
)
```

**File: cli/go.sum**
```go
github.com/spf13/cobra v1.7.0 h1:hyqWnYt1ZQShIddO5kBpj3vu05/++x6tJ6dg8EC572I=
github.com/spf13/cobra v1.7.0/go.mod h1:uLxZILRyS/50WlhOIKD7W6V5bgeIt+4sICxh6uRMrb0=
github.com/spf13/viper v1.15.0 h1:js3yy885G8xwJa6iOISGFwd+qlUo5AvyXb7CiihdtiU=
github.com/spf13/viper v1.15.0/go.mod h1:fFcTBJxvhhzSJiZy8n+PeW6t8l+KeT/uTARa0jwwgGQ=
// ... additional checksums
```

### Task 1.3.2: Build System Dependencies

#### 1.3.2.1: CMake Configuration
**Objective**: Configure CMake with all necessary modules and find scripts

**Implementation Steps:**
1. Set up CMake module path
2. Configure external project dependencies
3. Set up cross-platform build configurations

**Code Changes:**

**File: cmake/FindScratchBird.cmake**
```cmake
# FindScratchBird.cmake - Find ScratchBird database engine

find_path(SCRATCHBIRD_INCLUDE_DIR
    NAMES scratchbird/engine.h
    PATHS
        ${SCRATCHBIRD_ROOT}/include
        $ENV{SCRATCHBIRD_ROOT}/include
        /usr/local/include
        /usr/include
)

find_library(SCRATCHBIRD_LIBRARY
    NAMES scratchbird libscratchbird
    PATHS
        ${SCRATCHBIRD_ROOT}/lib
        $ENV{SCRATCHBIRD_ROOT}/lib
        /usr/local/lib
        /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ScratchBird
    REQUIRED_VARS SCRATCHBIRD_INCLUDE_DIR SCRATCHBIRD_LIBRARY
)

if(SCRATCHBIRD_FOUND)
    set(SCRATCHBIRD_LIBRARIES ${SCRATCHBIRD_LIBRARY})
    set(SCRATCHBIRD_INCLUDE_DIRS ${SCRATCHBIRD_INCLUDE_DIR})
endif()

mark_as_advanced(SCRATCHBIRD_INCLUDE_DIR SCRATCHBIRD_LIBRARY)
```

**File: cmake/CompilerFlags.cmake**
```cmake
# CompilerFlags.cmake - Common compiler flags

function(set_common_flags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4          # High warning level
            /permissive- # Standards conformance
            /Zc:__cplusplus # Enable correct __cplusplus macro value
        )

        # Release optimizations
        target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/O2>)
        target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:/Od>)

    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wunused
            -Wuninitialized
            -Wundef
        )

        # Release optimizations
        target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-O3 -DNDEBUG>)
        target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-O0 -g3>)

        # Sanitizers for debug builds
        if(ENABLE_SANITIZERS)
            target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
            target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
        endif()
    endif()
endfunction()
```

#### 1.3.2.2: Docker Configuration
**Objective**: Set up Docker for consistent development and deployment

**Implementation Steps:**
1. Create Dockerfile for each component
2. Set up docker-compose for development environment
3. Configure multi-stage builds

**Code Changes:**

**File: Dockerfile**
```dockerfile
# Multi-stage build for ScratchRobin
FROM ubuntu:22.04 AS base

# Install common dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libboost-all-dev \
    qt5-default \
    && rm -rf /var/lib/apt/lists/*

# Development stage
FROM base AS development

# Install development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    clang-format \
    cppcheck \
    && rm -rf /var/lib/apt/lists/*

# Set up vcpkg
RUN git clone https://github.com/Microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh && \
    ln -s /opt/vcpkg/vcpkg /usr/local/bin/vcpkg

# Set environment variables
ENV VCPKG_ROOT=/opt/vcpkg
ENV CC=clang
ENV CXX=clang++

WORKDIR /workspace

# Production stage
FROM base AS production

COPY --from=development /opt/vcpkg /opt/vcpkg
COPY . /workspace

WORKDIR /workspace/build

RUN cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && make -j$(nproc) \
    && make install

# Runtime stage
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    libssl1.1 \
    libboost-system1.74.0 \
    libqt5core5a \
    libqt5widgets5 \
    libqt5network5 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=production /usr/local/bin/scratchrobin /usr/local/bin/
COPY --from=production /usr/local/lib/libscratchrobin* /usr/local/lib/

# Create application user
RUN useradd -r -s /bin/false scratchrobin
USER scratchrobin

EXPOSE 3000 8080

CMD ["scratchrobin"]
```

**File: docker-compose.yml**
```yaml
version: '3.8'

services:
  scratchbird:
    image: scratchbird:latest
    container_name: scratchbird-db
    ports:
      - "5432:5432"
    environment:
      - SCRATCHBIRD_CONFIG=/etc/scratchbird/config.yaml
    volumes:
      - ./config/scratchbird.yaml:/etc/scratchbird/config.yaml:ro
      - scratchbird_data:/var/lib/scratchbird
    networks:
      - scratchrobin-network
    healthcheck:
      test: ["CMD", "pg_isready", "-U", "scratchbird"]
      interval: 30s
      timeout: 10s
      retries: 3

  scratchrobin:
    build:
      context: .
      dockerfile: Dockerfile
      target: development
    container_name: scratchrobin-app
    ports:
      - "3000:3000"
      - "8080:8080"
    environment:
      - DATABASE_URL=postgresql://scratchbird:password@scratchbird:5432/scratchbird
      - NODE_ENV=development
    volumes:
      - .:/workspace
      - /workspace/node_modules
    depends_on:
      scratchbird:
        condition: service_healthy
    networks:
      - scratchrobin-network
    command: npm run dev

  scratchrobin-cli:
    build:
      context: ./cli
      dockerfile: Dockerfile
    container_name: scratchrobin-cli
    volumes:
      - .:/workspace
    networks:
      - scratchrobin-network
    profiles:
      - tools

volumes:
  scratchbird_data:

networks:
  scratchrobin-network:
    driver: bridge
```

### Task 1.3.3: Security Dependencies

#### 1.3.3.1: Security Scanning Setup
**Objective**: Set up security scanning for dependencies and code

**Implementation Steps:**
1. Configure dependency vulnerability scanning
2. Set up static analysis tools
3. Implement security testing framework

**Code Changes:**

**File: .github/workflows/security.yml**
```yaml
name: Security Scanning

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]
  schedule:
    - cron: '0 6 * * *'  # Daily at 6 AM UTC

jobs:
  dependency-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Run npm audit
        run: |
          cd web
          npm audit --audit-level high

      - name: Run Go security scan
        run: |
          cd cli
          go mod download
          nancy --version
          nancy --output json .

      - name: Run C++ security scan
        uses: github/super-linter/slim@v4
        env:
          DEFAULT_BRANCH: main
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
          VALIDATE_CPP: true

  codeql-analysis:
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix:
        language: [ 'cpp', 'javascript', 'go' ]

    steps:
      - name: Checkout repository
        uses: actions/checkout@v3

      - name: Initialize CodeQL
        uses: github/codeql-action/init@v2
        with:
          languages: ${{ matrix.language }}

      - name: Autobuild
        uses: github/codeql-action/autobuild@v2

      - name: Perform CodeQL Analysis
        uses: github/codeql-action/analyze@v2
        with:
          category: "/language:${{matrix.language}}"
```

**File: scripts/security-scan.sh**
```bash
#!/bin/bash

# Security scanning script for ScratchRobin

set -euo pipefail

echo "🔒 Running ScratchRobin Security Scan"
echo "====================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
echo "Checking for required security tools..."

if ! command_exists "npm"; then
    echo -e "${YELLOW}Warning: npm not found, skipping Node.js security check${NC}"
else
    echo -e "${GREEN}✓ npm found${NC}"
fi

if ! command_exists "nancy"; then
    echo -e "${YELLOW}Warning: nancy not found, skipping Go security check${NC}"
else
    echo -e "${GREEN}✓ nancy found${NC}"
fi

if ! command_exists "cppcheck"; then
    echo -e "${YELLOW}Warning: cppcheck not found, skipping C++ security check${NC}"
else
    echo -e "${GREEN}✓ cppcheck found${NC}"
fi

# Node.js security check
if command_exists "npm"; then
    echo -e "\n📦 Checking Node.js dependencies..."
    cd web
    if npm audit --audit-level moderate; then
        echo -e "${GREEN}✓ Node.js dependency scan passed${NC}"
    else
        echo -e "${RED}✗ Node.js dependency vulnerabilities found${NC}"
        exit 1
    fi
    cd ..
fi

# Go security check
if command_exists "nancy"; then
    echo -e "\n🐹 Checking Go dependencies..."
    cd cli
    if nancy --output json . > nancy-report.json; then
        echo -e "${GREEN}✓ Go dependency scan passed${NC}"
    else
        echo -e "${RED}✗ Go dependency vulnerabilities found${NC}"
        cat nancy-report.json
        exit 1
    fi
    cd ..
fi

# C++ security check
if command_exists "cppcheck"; then
    echo -e "\n🔧 Running C++ static analysis..."
    if cppcheck --enable=all --std=c++17 --language=c++ \
                --suppress=missingIncludeSystem \
                --suppress=unusedFunction \
                --suppress=unmatchedSuppression \
                --inline-suppr \
                --xml --xml-version=2 \
                src/ 2> cppcheck-report.xml; then
        echo -e "${GREEN}✓ C++ static analysis completed${NC}"
    else
        echo -e "${YELLOW}⚠ C++ static analysis found issues${NC}"
    fi
fi

# File permission check
echo -e "\n🔐 Checking file permissions..."
if find . -type f \( -name "*.pem" -o -name "*.key" -o -name "*.crt" \) -perm 0644 2>/dev/null | grep -q .; then
    echo -e "${RED}✗ Found sensitive files with incorrect permissions${NC}"
    exit 1
else
    echo -e "${GREEN}✓ File permissions are secure${NC}"
fi

# Dependency version check
echo -e "\n📋 Checking dependency versions..."
if [ -f "web/package.json" ]; then
    echo "Node.js dependencies:"
    cd web
    npm outdated || true
    cd ..
fi

if [ -f "cli/go.mod" ]; then
    echo "Go dependencies:"
    cd cli
    go list -u -m all | grep -v "(indirect)" || true
    cd ..
fi

echo -e "\n${GREEN}✅ Security scan completed successfully${NC}"
```

#### 1.3.3.2: Security Configuration
**Objective**: Set up security configuration and policies

**Implementation Steps:**
1. Create security configuration templates
2. Set up SSL/TLS certificates management
3. Configure authentication and authorization

**Code Changes:**

**File: config/security.yaml**
```yaml
# Security configuration for ScratchRobin

# SSL/TLS Configuration
ssl:
  enabled: true
  certificate_file: "/etc/scratchrobin/ssl/server.crt"
  private_key_file: "/etc/scratchrobin/ssl/server.key"
  ca_certificate_file: "/etc/scratchrobin/ssl/ca.crt"
  cipher_suites:
    - "ECDHE-RSA-AES256-GCM-SHA384"
    - "ECDHE-RSA-AES128-GCM-SHA256"
  min_tls_version: "1.2"

# Authentication Configuration
authentication:
  method: "database"  # database, ldap, oauth2, jwt
  database:
    table: "users"
    username_column: "username"
    password_column: "password_hash"
    salt_column: "salt"
    iterations: 10000

  ldap:
    enabled: false
    server: "ldap.example.com"
    port: 389
    base_dn: "dc=example,dc=com"
    bind_user: "cn=admin,dc=example,dc=com"
    bind_password: "secret"

  oauth2:
    enabled: false
    provider: "google"
    client_id: "your-client-id"
    client_secret: "your-client-secret"
    redirect_uri: "http://localhost:3000/auth/callback"

# Authorization Configuration
authorization:
  enabled: true
  model: "rbac"  # rbac, abac
  cache:
    enabled: true
    ttl: 300  # seconds

  roles:
    - name: "admin"
      permissions:
        - "system.*"
        - "database.*"
        - "user.*"

    - name: "developer"
      permissions:
        - "database.read"
        - "database.write"
        - "query.execute"

    - name: "viewer"
      permissions:
        - "database.read"
        - "query.execute"

# Session Management
session:
  timeout: 3600  # seconds
  max_concurrent_sessions: 5
  session_storage: "redis"  # memory, redis, database

# Audit Configuration
audit:
  enabled: true
  log_file: "/var/log/scratchrobin/audit.log"
  log_level: "info"
  events:
    - "login"
    - "logout"
    - "query_execution"
    - "schema_change"
    - "permission_change"

# Encryption Configuration
encryption:
  algorithm: "aes-256-gcm"
  key_derivation: "pbkdf2"
  key_length: 32
  iterations: 100000

# Security Headers (for web interface)
security_headers:
  enabled: true
  headers:
    - "X-Content-Type-Options: nosniff"
    - "X-Frame-Options: DENY"
    - "X-XSS-Protection: 1; mode=block"
    - "Strict-Transport-Security: max-age=31536000; includeSubDomains"
    - "Content-Security-Policy: default-src 'self'"

# Rate Limiting
rate_limiting:
  enabled: true
  requests_per_minute: 60
  burst_limit: 10
  block_duration: 300  # seconds

# Monitoring and Alerting
monitoring:
  enabled: true
  metrics:
    - "authentication_failures"
    - "unauthorized_access"
    - "suspicious_activity"
    - "connection_attempts"

  alerts:
    - name: "high_auth_failure_rate"
      condition: "rate(authentication_failures[5m]) > 10"
      severity: "warning"
      message: "High authentication failure rate detected"

    - name: "unauthorized_access"
      condition: "rate(unauthorized_access[5m]) > 5"
      severity: "error"
      message: "Multiple unauthorized access attempts"
```

### Task 1.3.4: Testing Dependencies

#### 1.3.4.1: Unit Testing Framework
**Objective**: Set up comprehensive unit testing infrastructure

**Implementation Steps:**
1. Configure Google Test for C++
2. Set up Jest for JavaScript/TypeScript
3. Configure Go testing framework
4. Set up test coverage reporting

**Code Changes:**

**File: tests/unit/test_main.cpp**
```cpp
#include <gtest/gtest.h>

// Test main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Set up test environment
    // - Initialize mock objects
    // - Set up test databases
    // - Configure logging

    return RUN_ALL_TESTS();
}
```

**File: web/jest.config.js**
```javascript
module.exports = {
  testEnvironment: 'jsdom',
  setupFilesAfterEnv: ['<rootDir>/src/setupTests.ts'],
  moduleNameMapping: {
    '\\.(css|less|scss|sass)$': 'identity-obj-proxy',
    '\\.(jpg|jpeg|png|gif|svg)$': '<rootDir>/src/__mocks__/fileMock.js',
  },
  collectCoverageFrom: [
    'src/**/*.{ts,tsx}',
    '!src/**/*.d.ts',
    '!src/index.tsx',
    '!src/serviceWorker.ts',
  ],
  coverageDirectory: 'coverage',
  coverageReporters: [
    'text',
    'lcov',
    'html',
    'json-summary',
  ],
  coverageThreshold: {
    global: {
      branches: 80,
      functions: 80,
      lines: 80,
      statements: 80,
    },
  },
  transform: {
    '^.+\\.(ts|tsx)$': 'ts-jest',
    '^.+\\.(js|jsx)$': 'babel-jest',
  },
  moduleFileExtensions: ['ts', 'tsx', 'js', 'jsx', 'json', 'node'],
};
```

**File: cli/Makefile**
```makefile
# Go testing configuration
.PHONY: test
test:
	go test -v -race -coverprofile=coverage.out ./...

.PHONY: test-coverage
test-coverage:
	go test -v -race -coverprofile=coverage.out ./...
	go tool cover -html=coverage.out -o coverage.html

.PHONY: test-integration
test-integration:
	go test -v -tags=integration ./...

.PHONY: bench
bench:
	go test -bench=. -benchmem ./...

.PHONY: lint
lint:
	golangci-lint run
```

#### 1.3.4.2: Integration Testing Setup
**Objective**: Set up integration testing environment

**Implementation Steps:**
1. Create test database setup scripts
2. Configure test data fixtures
3. Set up CI/CD testing pipeline

**Code Changes:**

**File: scripts/setup-test-db.sh**
```bash
#!/bin/bash

# Setup test database for integration tests

set -euo pipefail

# Configuration
TEST_DB_HOST=${TEST_DB_HOST:-localhost}
TEST_DB_PORT=${TEST_DB_PORT:-5432}
TEST_DB_NAME=${TEST_DB_NAME:-scratchrobin_test}
TEST_DB_USER=${TEST_DB_USER:-scratchrobin}
TEST_DB_PASSWORD=${TEST_DB_PASSWORD:-test_password}

echo "Setting up test database: $TEST_DB_NAME"

# Wait for database to be ready
echo "Waiting for database to be ready..."
until pg_isready -h "$TEST_DB_HOST" -p "$TEST_DB_PORT" -U "$TEST_DB_USER"; do
    sleep 1
done

# Create test database
echo "Creating test database..."
createdb -h "$TEST_DB_HOST" -p "$TEST_DB_PORT" -U "$TEST_DB_USER" "$TEST_DB_NAME"

# Run schema setup
echo "Setting up test schema..."
psql -h "$TEST_DB_HOST" -p "$TEST_DB_PORT" -U "$TEST_DB_USER" -d "$TEST_DB_NAME" << 'EOF'
-- Test schema setup
CREATE TABLE test_users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE test_products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert test data
INSERT INTO test_users (username, email) VALUES
    ('testuser1', 'test1@example.com'),
    ('testuser2', 'test2@example.com');

INSERT INTO test_products (name, price, description) VALUES
    ('Test Product 1', 19.99, 'A test product'),
    ('Test Product 2', 29.99, 'Another test product');
EOF

echo "Test database setup completed successfully"
```

**File: tests/integration/test_database_connection.cpp**
```cpp
#include <gtest/gtest.h>
#include <memory>
#include "core/connection_manager.h"
#include "core/metadata_manager.h"

class DatabaseConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test database connection
        connectionManager_ = std::make_unique<ConnectionManager>();
        metadataManager_ = std::make_unique<MetadataManager>(connectionManager_.get());

        // Load test configuration
        // Connect to test database
    }

    void TearDown() override {
        // Clean up test data
        // Disconnect from test database
    }

    std::unique_ptr<ConnectionManager> connectionManager_;
    std::unique_ptr<MetadataManager> metadataManager_;
};

TEST_F(DatabaseConnectionTest, CanEstablishConnection) {
    ConnectionInfo info;
    info.name = "test_connection";
    info.host = "localhost";
    info.port = 5432;
    info.database = "scratchrobin_test";
    info.username = "scratchrobin";
    info.password = "test_password";

    auto connectionId = connectionManager_->connect(info);
    ASSERT_NE(connectionId, "");
    EXPECT_TRUE(connectionManager_->isConnected(connectionId));

    // Clean up
    connectionManager_->disconnect(connectionId);
}

TEST_F(DatabaseConnectionTest, CanExecuteSimpleQuery) {
    // Similar to above test but execute a simple SELECT query
    // Verify results
}

TEST_F(DatabaseConnectionTest, HandlesConnectionFailure) {
    ConnectionInfo info;
    info.name = "bad_connection";
    info.host = "nonexistent";
    info.port = 5432;
    info.database = "scratchrobin_test";
    info.username = "baduser";
    info.password = "badpass";

    auto connectionId = connectionManager_->connect(info);
    EXPECT_EQ(connectionId, "");
}
```

### Task 1.3.5: Documentation Dependencies

#### 1.3.5.1: Documentation Build System
**Objective**: Set up documentation generation and hosting

**Implementation Steps:**
1. Configure Doxygen for C++ documentation
2. Set up TypeDoc for TypeScript documentation
3. Configure MkDocs for general documentation
4. Set up automated documentation builds

**Code Changes:**

**File: docs/Doxyfile**
```makefile
# Doxyfile configuration for ScratchRobin C++ documentation

PROJECT_NAME           = "ScratchRobin"
PROJECT_NUMBER         = 0.1.0
PROJECT_BRIEF          = "Database management interface for ScratchBird"

INPUT                  = ../src
RECURSIVE              = YES
EXCLUDE_PATTERNS       = */tests/* */build/* */cmake-build-*

OUTPUT_DIRECTORY       = doxygen
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
GENERATE_MAN           = NO
GENERATE_RTF           = NO
GENERATE_XML           = YES
GENERATE_DOCSET        = NO

HTML_OUTPUT            = html
HTML_HEADER            = header.html
HTML_FOOTER            = footer.html
HTML_STYLESHEET        = custom.css

EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = YES
EXTRACT_PACKAGE        = YES
EXTRACT_STATIC         = YES
EXTRACT_LOCAL_CLASSES  = YES
EXTRACT_LOCAL_METHODS  = YES

HIDE_UNDOC_MEMBERS     = NO
HIDE_UNDOC_CLASSES     = NO
HIDE_FRIEND_COMPOUNDS  = NO
HIDE_IN_BODY_DOCS      = NO

SHOW_INCLUDE_FILES     = YES
SHOW_GROUPED_MEMS_INC = YES

# UML diagrams
HAVE_DOT               = YES
DOT_GRAPH_MAX_NODES    = 100
DOT_IMAGE_FORMAT       = svg
DOT_TRANSPARENT        = YES

# Call graphs
CALL_GRAPH             = YES
CALLER_GRAPH           = YES
GRAPHICAL_HIERARCHY    = YES

# Search functionality
SEARCHENGINE           = YES
SERVER_BASED_SEARCH    = NO
```

**File: docs/mkdocs.yml**
```yaml
site_name: ScratchRobin Documentation
site_description: Database management interface for ScratchBird
site_author: ScratchRobin Team
site_url: https://docs.scratchrobin.org

repo_name: scratchbird/scratchrobin
repo_url: https://github.com/scratchbird/scratchrobin

nav:
  - Home: index.md
  - User Guide:
      - Getting Started: user/getting-started.md
      - Installation: user/installation.md
      - Configuration: user/configuration.md
      - Usage: user/usage.md
  - API Reference:
      - C++ API: api/cpp/index.md
      - Web API: api/web/index.md
      - CLI Reference: api/cli/index.md
  - Developer Guide:
      - Architecture: developer/architecture.md
      - Building: developer/building.md
      - Testing: developer/testing.md
      - Contributing: developer/contributing.md
  - Administration:
      - Security: admin/security.md
      - Monitoring: admin/monitoring.md
      - Backup: admin/backup.md
      - Troubleshooting: admin/troubleshooting.md

theme:
  name: material
  palette:
    primary: indigo
    accent: indigo
  font:
    text: Roboto
    code: Roboto Mono
  features:
    - navigation.tabs
    - navigation.tabs.sticky
    - navigation.sections
    - navigation.expand
    - search.suggest
    - search.highlight
    - content.tabs.link
    - content.code.annotation
    - content.code.copy

plugins:
  - search
  - mkdocstrings:
      default_handler: cpp
      handlers:
        cpp:
          rendering:
            show_root_heading: true
            show_source_links: true
          doxygen:
            ignore_doxyfile: false
  - git-revision-date-localized:
      enable_creation_date: true

markdown_extensions:
  - pymdownx.highlight:
      anchor_linenums: true
      line_spans: __span
      pygments_lang_class: true
  - pymdownx.inlinehilite
  - pymdownx.snippets
  - pymdownx.superfences:
      custom_fences:
        - name: mermaid
          class: mermaid
          format: !!python/name:pymdownx.superfences.fence_code_format
  - pymdownx.details
  - pymdownx.tabbed:
      alternate_style: true
  - pymdownx.arithmatex:
      generic: true
  - pymdownx.emoji:
      emoji_index: !!python/name:materialx.emoji.twemoji
      emoji_generator: !!python/name:materialx.emoji.to_svg
  - attr_list
  - md_in_html
  - footnotes
  - def_list
  - pymdownx.tasklist:
      custom_checkbox: true

extra:
  generator: false
  social:
    - icon: fontawesome/brands/github
      link: https://github.com/scratchbird/scratchrobin
    - icon: fontawesome/brands/docker
      link: https://hub.docker.com/r/scratchrobin
```

**File: scripts/build-docs.sh**
```bash
#!/bin/bash

# Build documentation for all components

set -euo pipefail

echo "📚 Building ScratchRobin Documentation"
echo "======================================"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Build C++ documentation
echo -e "\n🔧 Building C++ documentation..."
if command -v doxygen >/dev/null 2>&1; then
    cd docs
    doxygen Doxyfile
    echo -e "${GREEN}✓ C++ documentation built${NC}"
    cd ..
else
    echo -e "${YELLOW}⚠ Doxygen not found, skipping C++ docs${NC}"
fi

# Build general documentation
echo -e "\n📖 Building general documentation..."
if command -v mkdocs >/dev/null 2>&1; then
    cd docs
    mkdocs build --clean --strict
    echo -e "${GREEN}✓ General documentation built${NC}"
    cd ..
else
    echo -e "${YELLOW}⚠ MkDocs not found, skipping general docs${NC}"
fi

# Build TypeScript documentation
echo -e "\n🌐 Building TypeScript documentation..."
if command -v typedoc >/dev/null 2>&1; then
    cd web
    typedoc --out ../docs/api/web --exclude "**/node_modules/**" src/
    echo -e "${GREEN}✓ TypeScript documentation built${NC}"
    cd ..
else
    echo -e "${YELLOW}⚠ TypeDoc not found, skipping TypeScript docs${NC}"
fi

# Build Go documentation
echo -e "\n🐹 Building Go documentation..."
if command -v godoc >/dev/null 2>&1; then
    cd cli
    mkdir -p ../docs/api/cli
    godoc -html . > ../docs/api/cli/index.html
    echo -e "${GREEN}✓ Go documentation built${NC}"
    cd ..
else
    echo -e "${YELLOW}⚠ GoDoc not found, skipping Go docs${NC}"
fi

# Generate API documentation
echo -e "\n📋 Generating API documentation..."
if [ -f "scripts/generate-api-docs.py" ]; then
    python3 scripts/generate-api-docs.py
    echo -e "${GREEN}✓ API documentation generated${NC}"
else
    echo -e "${YELLOW}⚠ API documentation generator not found${NC}"
fi

# Create documentation index
echo -e "\n📑 Creating documentation index..."
cat > docs/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>ScratchRobin Documentation</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        h1 { color: #333; }
        .doc-section { margin: 20px 0; padding: 20px; border: 1px solid #ddd; border-radius: 5px; }
        a { color: #0066cc; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1>ScratchRobin Documentation</h1>

    <div class="doc-section">
        <h2>📖 User Documentation</h2>
        <ul>
            <li><a href="user/getting-started.html">Getting Started</a></li>
            <li><a href="user/installation.html">Installation Guide</a></li>
            <li><a href="user/usage.html">User Guide</a></li>
        </ul>
    </div>

    <div class="doc-section">
        <h2>🔧 Developer Documentation</h2>
        <ul>
            <li><a href="developer/architecture.html">Architecture Overview</a></li>
            <li><a href="developer/building.html">Building from Source</a></li>
            <li><a href="developer/contributing.html">Contributing Guide</a></li>
        </ul>
    </div>

    <div class="doc-section">
        <h2>📚 API Documentation</h2>
        <ul>
            <li><a href="api/cpp/index.html">C++ API Reference</a></li>
            <li><a href="api/web/index.html">Web API Reference</a></li>
            <li><a href="api/cli/index.html">CLI Reference</a></li>
        </ul>
    </div>

    <div class="doc-section">
        <h2>⚙️ Administration</h2>
        <ul>
            <li><a href="admin/security.html">Security Guide</a></li>
            <li><a href="admin/monitoring.html">Monitoring Setup</a></li>
            <li><a href="admin/backup.html">Backup and Recovery</a></li>
        </ul>
    </div>
</body>
</html>
EOF

echo -e "${GREEN}✓ Documentation index created${NC}"
echo -e "\n${GREEN}✅ Documentation build completed successfully${NC}"
echo -e "\n📂 Documentation available at:"
echo -e "  - HTML: docs/index.html"
echo -e "  - MkDocs: docs/site/index.html"
echo -e "  - Doxygen: docs/doxygen/html/index.html"
```

## Testing and Validation

### Task 1.3.6: Dependency Testing

#### 1.3.6.1: Dependency Compatibility Testing
**Objective**: Ensure all dependencies work together correctly

**Implementation Steps:**
1. Create dependency compatibility matrix
2. Set up automated compatibility testing
3. Implement version conflict resolution

**Test Commands:**
```bash
# Test C++ dependencies
cd build
ctest --output-on-failure -R "*dependency*"

# Test JavaScript dependencies
cd web
npm run test:dependencies

# Test Go dependencies
cd cli
go mod verify
go test ./...
```

#### 1.3.6.2: Security Vulnerability Testing
**Objective**: Regularly scan for security vulnerabilities

**Implementation Steps:**
1. Set up automated vulnerability scanning
2. Create vulnerability reporting system
3. Implement dependency update automation

**Security Commands:**
```bash
# Run security scans
./scripts/security-scan.sh

# Update dependencies safely
./scripts/update-dependencies.sh

# Check for vulnerabilities
npm audit
nancy --output json .
```

## Summary

This phase 1.3 implementation provides comprehensive dependency management for ScratchRobin:

✅ **External Dependencies**: Configured vcpkg, npm, and Go modules with all required libraries
✅ **Build System**: Enhanced CMake configuration with Docker support and cross-platform builds
✅ **Security**: Implemented vulnerability scanning, SSL/TLS configuration, and security policies
✅ **Testing**: Set up unit testing frameworks for all languages with coverage reporting
✅ **Documentation**: Configured automated documentation generation for all components
✅ **CI/CD Ready**: Created configuration files for automated builds and security scanning

The dependency management system is now ready to support the development of ScratchRobin across all interfaces and components. The system ensures security, compatibility, and maintainability throughout the development lifecycle.

**Next Phase**: Phase 2.1 - Connection Pooling Implementation
