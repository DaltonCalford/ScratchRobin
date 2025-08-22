# ScratchRobin

A comprehensive database management interface for ScratchBird, designed to provide powerful tools for database administration, query development, and system management.

## Overview

ScratchRobin is a modern, cross-platform database management tool that provides:

- **Multi-Interface Support**: Web UI, Desktop Application, and Command Line Interface
- **Advanced Query Tools**: SQL Editor with syntax highlighting, auto-completion, and query analysis
- **Database Browser**: Intuitive schema navigation and object management
- **Performance Monitoring**: Real-time metrics and query performance analysis
- **Security Management**: User management, role-based access control, and audit logging
- **Backup & Restore**: Comprehensive backup and point-in-time recovery
- **Client Libraries**: Native support for multiple programming languages

## Features

### Core Functionality
- ✅ Database connection management with SSL/TLS support
- ✅ SQL query execution with result visualization
- ✅ Schema and object browsing
- ✅ Table data editing and management
- ✅ Index creation and management
- ✅ Constraint management (primary keys, foreign keys, check constraints)
- ✅ User and role management
- ✅ Backup and restore operations
- ✅ Query history and favorites

### Advanced Features
- 🔄 Real-time query monitoring and performance analysis
- 🔄 Visual query plan analysis
- 🔄 Database schema comparison and synchronization
- 🔄 Advanced security and audit logging
- 🔄 Multi-database support
- 🔄 Plugin architecture for extensibility
- 🔄 REST API for automation and integration

## Installation

### Prerequisites

- **C++ Compiler**: GCC 8+ or Clang 8+ or MSVC 2019+
- **CMake**: Version 3.20 or higher
- **Git**: For version control
- **Node.js**: Version 18+ (for web interface)
- **Go**: Version 1.19+ (for CLI tools)
- **Google Test**: For unit testing

### Quick Start

1. **Clone the repository**:
   ```bash
   git clone https://github.com/scratchbird/scratchrobin.git
   cd scratchrobin
   ```

2. **Build the application**:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

3. **Install dependencies**:
   ```bash
   # For web interface
   cd ../web
   npm install

   # For CLI tools
   cd ../cli
   go mod download
   ```

4. **Run the application**:
   ```bash
   cd ../build
   ./bin/scratchrobin
   ```

## Architecture

ScratchRobin follows a modular architecture with clear separation of concerns:

```
src/
├── core/           # Core application logic
│   ├── application.h/cpp     # Main application class
│   ├── connection_manager.h/cpp  # Database connection management
│   └── metadata_manager.h/cpp    # Schema and metadata handling
├── ui/             # User interface components
│   ├── main_window.h/cpp      # Main application window
│   ├── connection_dialog.h/cpp    # Connection setup dialog
│   ├── query_editor.h/cpp     # SQL editor component
│   └── object_browser.h/cpp   # Schema browser
├── utils/          # Utility functions and classes
│   ├── logger.h/cpp          # Logging system
│   ├── string_utils.h/cpp    # String manipulation utilities
│   └── file_utils.h/cpp      # File system utilities
└── components/     # Specialized components
    ├── sql_executor.h/cpp    # SQL execution engine
    ├── query_parser.h/cpp    # SQL parsing and analysis
    └── schema_loader.h/cpp   # Schema loading and caching
```

## Project Structure

```
ScratchRobin/
├── src/                    # Source code
├── web/                    # Web interface (React)
├── desktop/               # Desktop application (Electron)
├── cli/                   # Command line interface (Go)
├── client-libraries/      # Client libraries
│   ├── python/           # Python client
│   ├── java/            # Java client
│   ├── nodejs/          # Node.js client
│   └── cpp/             # C++ client
├── tests/                # Test suites
│   ├── unit/            # Unit tests
│   ├── integration/     # Integration tests
│   └── e2e/             # End-to-end tests
├── docs/                # Documentation
├── scripts/             # Build and utility scripts
├── ProjectPlan/         # Implementation planning
└── CMakeLists.txt       # Build configuration
```

## Development

### Setting Up Development Environment

1. **Install development dependencies**:
   ```bash
   # Install Google Test
   sudo apt-get install libgtest-dev

   # Install additional development tools
   sudo apt-get install valgrind cppcheck clang-format
   ```

2. **Configure the build for development**:
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
   make -j$(nproc)
   ```

3. **Run tests**:
   ```bash
   make test
   # Or run specific test suites
   ./bin/unit_tests
   ./bin/integration_tests
   ```

### Code Style

This project follows specific coding standards:

- **C++**: C++17 standard, Google C++ Style Guide
- **JavaScript/TypeScript**: ESLint with Airbnb configuration
- **Go**: Standard Go formatting (`go fmt`)
- **Python**: PEP 8 with type hints

### Running Linters

```bash
# C++ linting
make cppcheck

# JavaScript/TypeScript linting
cd web && npm run lint

# Go linting
cd cli && golangci-lint run

# Python linting
cd client-libraries/python && flake8 .
```

## Testing

ScratchRobin includes comprehensive test suites:

### Unit Tests
```bash
cd build
./bin/unit_tests
```

### Integration Tests
```bash
cd build
./bin/integration_tests
```

### End-to-End Tests
```bash
cd build
./bin/e2e_tests
```

### Web Interface Tests
```bash
cd web
npm run test
```

## Documentation

- **API Documentation**: Available in `docs/api/`
- **User Guide**: Available in `docs/user/`
- **Developer Guide**: Available in `docs/developer/`
- **Architecture**: See `ProjectPlan/Architecture/`

## Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details on:

- Code of conduct
- Development process
- Submitting pull requests
- Reporting issues

### Development Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run tests (`make test`)
5. Commit your changes (`git commit -m 'Add amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [docs.scratchrobin.org](https://docs.scratchrobin.org)
- **Issues**: [GitHub Issues](https://github.com/scratchbird/scratchrobin/issues)
- **Discussions**: [GitHub Discussions](https://github.com/scratchbird/scratchrobin/discussions)
- **Email**: team@scratchrobin.org

## Roadmap

### Phase 1 (Current): Foundation ✅
- Project setup and architecture
- Core connection management
- Basic UI components
- SQL editor foundation

### Phase 2: Core Features 🔄
- Advanced query tools
- Schema management
- User management
- Performance monitoring

### Phase 3: Enterprise Features 📋
- High availability
- Advanced security
- Backup and recovery
- Multi-database support

### Phase 4: Advanced Tools 📋
- Visual query builder
- Data import/export
- Schema comparison
- Plugin system

### Phase 5: Ecosystem 📋
- Client libraries
- REST API
- Cloud integration
- Mobile applications

## Acknowledgments

- **ScratchBird**: The underlying database engine
- **Contributors**: Thank you to all our contributors
- **Community**: Thanks to our amazing user community

---

**ScratchRobin** - Making database management powerful and accessible.