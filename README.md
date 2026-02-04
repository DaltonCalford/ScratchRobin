# ScratchRobin

[![License: IDPL](https://img.shields.io/badge/License-IDPL-blue.svg)](https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Version](https://img.shields.io/badge/version-0.1.0--alpha-orange.svg)]()

**The Database Design & Deployment Tool for Modern Development Workflows**

ScratchRobin brings software engineering best practices (version control, testing, CI/CD) to database development. Design your database schema in isolation, review changes with your team, test thoroughly, and deploy safely.

---

## 🌟 Key Features

### Project-Centric Design Workflow
- **Design in isolation** - Make changes safely away from production
- **State tracking** - Know which objects are extracted, new, modified, pending, approved, or deployed
- **Dual-repo Git** - Synchronize between design files and database schema
- **Team collaboration** - Real-time editing, comments, and reviews

### Visual Design Tools
- **ER Diagrams** - Auto-sync with catalog, multiple layout algorithms
- **Whiteboards** - Free-form brainstorming and sketching
- **Mind Maps** - Conceptual organization before implementation
- **OLAP Cube Designer** - Data warehouse dimension and measure design

### Comprehensive Testing
- **6 Test Types** - Unit, Integration, Performance, Data Quality, Security, Migration
- **Auto-generation** - Create tests from schema automatically
- **Multiple formats** - Text, JSON, HTML, JUnit XML, Markdown reports
- **CI/CD Ready** - Integrate with your build pipeline

### Safe Deployment
- **Migration scripts** - Auto-generated DDL with rollback
- **Deployment plans** - Step-by-step execution with validation
- **Multi-environment** - Dev → Test → Staging → Production
- **Atomic deployments** - All-or-nothing with automatic rollback

---

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/DaltonCalford/ScratchRobin.git
cd ScratchRobin

# Build
mkdir build && cd build
cmake ..
make -j4

# Run
./scratchrobin
```

### Your First Project

```bash
# 1. Create a new project
File → New Project

# 2. Connect to your database
Enter connection details for your source database

# 3. Extract baseline schema
Project → Extract from Database
All objects are marked "extracted" (read-only baseline)

# 4. Make design changes
- Create new tables in the designer
- Modify existing tables
- Changes are marked "new" or "modified"

# 5. Generate tests
Tests → Auto-Generate → Schema Tests

# 6. Review and approve
Mark objects as "pending" for review
Reviewer marks as "approved"

# 7. Deploy
Deploy → Create Plan → Execute
```

---

## 📊 Project Status

**Current Version:** 0.1.0-alpha

### Implemented ✅
- [x] Core project system with state management
- [x] Dual-repo Git integration (designs + database)
- [x] Design state tracking (9 states with visual indicators)
- [x] Comprehensive icon system (74 icons, 121 slots)
- [x] Testing framework core (6 test types)
- [x] ERD diagramming with auto-sync
- [x] Multi-database support (PostgreSQL, MySQL, Firebird, ScratchBird)

### In Progress 🚧
- [ ] Whiteboards and mind maps
- [ ] OLAP cube designer
- [ ] Test execution UI
- [ ] Migration generation

### Planned 📋
- [ ] Deployment plans and management
- [ ] Data lineage tracking
- [ ] Data masking
- [ ] API generation
- [ ] CDC/Event streaming
- [ ] AI-assisted design

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SCRATCHROBIN                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │
│  │   PROJECT REPO  │  │   DESIGN TOOLS  │  │  TESTING FRAME  │             │
│  │   (Git)         │  │                 │  │                 │             │
│  │                 │  │ • ERD Diagrams  │  │ • Unit Tests    │             │
│  │ • Designs       │  │ • Whiteboards   │  │ • Integration   │             │
│  │ • Tests         │  │ • Mind Maps     │  │ • Performance   │             │
│  │ • Docs          │  │ • Cubes         │  │ • Data Quality  │             │
│  │                 │  │                 │  │ • Security      │             │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘             │
│           │                    │                    │                       │
│           └────────────────────┼────────────────────┘                       │
│                                ▼                                            │
│                    ┌─────────────────────┐                                  │
│                    │   SYNC ENGINE       │                                  │
│                    │                     │                                  │
│                    │ • DDL Generation    │                                  │
│                    │ • Conflict Detect   │                                  │
│                    │ • State Management  │                                  │
│                    └──────────┬──────────┘                                  │
│                               ▼                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │
│  │   DATABASE REPO │  │   DEPLOYMENT    │  │   OPERATIONS    │             │
│  │   (ScratchBird) │  │                 │  │                 │             │
│  │                 │  │ • Migrations    │  │ • Monitoring    │             │
│  │ • Schema        │  │ • Plans         │  │ • Lineage       │             │
│  │ • Migrations    │  │ • Rollback      │  │ • CDC           │             │
│  │ • Snapshots     │  │ • Multi-Env     │  │ • API Gen       │             │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 📖 Documentation

### Getting Started
- [TARGET_FEATURES.md](docs/TARGET_FEATURES.md) - What ScratchRobin does and why
- [WORKFLOW_GUIDE.md](docs/WORKFLOW_GUIDE.md) - Step-by-step workflow guide
- [SPECIFICATIONS.md](docs/SPECIFICATIONS.md) - Complete technical specifications

### Git Integration
- [GIT_INTEGRATION.md](docs/GIT_INTEGRATION.md) - Dual-repo Git model
- [DATABASE_GIT_PROTOCOL.md](docs/DATABASE_GIT_PROTOCOL.md) - Database Git protocol
- [GIT_WORKFLOW_EXAMPLES.md](docs/GIT_WORKFLOW_EXAMPLES.md) - Practical examples

### Additional Features
- [ADDITIONAL_FEATURES.md](docs/ADDITIONAL_FEATURES.md) - Cube design, testing, lineage
- [IMPLEMENTATION_ROADMAP.md](docs/IMPLEMENTATION_ROADMAP.md) - 12-month roadmap

---

## 🎯 Why ScratchRobin?

### The Problem
Traditional database tools work directly on live databases:
- ❌ Changes are immediate and risky
- ❌ No version control integration
- ❌ Limited testing capabilities
- ❌ Poor collaboration features
- ❌ Difficult rollbacks

### The Solution
ScratchRobin brings modern development practices to databases:
- ✅ Design in isolation from production
- ✅ Full Git version control
- ✅ Comprehensive testing framework
- ✅ Real-time collaboration
- ✅ Safe deployments with rollback

---

## 🖥️ Screenshots

*Coming soon - UI screenshots*

---

## 🛠️ Building from Source

### Requirements
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.20+
- wxWidgets 3.2+
- (Optional) PostgreSQL, MySQL, Firebird client libraries

### Build Options

```bash
# Basic build
cmake -B build
cmake --build build

# With all database backends
cmake -B build -DSCRATCHROBIN_USE_LIBPQ=ON \
               -DSCRATCHROBIN_USE_MYSQL=ON \
               -DSCRATCHROBIN_USE_FIREBIRD=ON
cmake --build build

# Debug build with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DSCRATCHROBIN_ENABLE_ASAN=ON
cmake --build build
```

---

## 🤝 Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Ways to Contribute
- 🐛 Report bugs
- 💡 Suggest features
- 📝 Improve documentation
- 🔧 Submit PRs
- 🎨 Design icons

---

## 📜 License

ScratchRobin is licensed under the [Initial Developer's Public License Version 1.0](https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/).

---

## 🙏 Acknowledgments

- wxWidgets team for the excellent cross-platform UI framework
- Firebird project for the IDPL license model
- Contributors and testers

---

## 📧 Contact

- **Issues:** [GitHub Issues](https://github.com/DaltonCalford/ScratchRobin/issues)
- **Discussions:** [GitHub Discussions](https://github.com/DaltonCalford/ScratchRobin/discussions)

---

*Built with ❤️ for database developers who want modern tools.*
