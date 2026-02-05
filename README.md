# ScratchRobin

[![License: IDPL](https://img.shields.io/badge/License-IDPL-blue.svg)](https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Version](https://img.shields.io/badge/version-0.2.0--alpha-orange.svg)]()

**The Database Design & Deployment Tool for Modern Development Workflows**

ScratchRobin brings software engineering best practices (version control, testing, CI/CD) to database development. Design your database schema in isolation, review changes with your team, test thoroughly, and deploy safely.

**Release Targets:** `docs/planning/RELEASE_TARGETS.md`

---
### **Note To New Users**

ScratchBird is in early alpha release. No binaries have been officially released at this time. The code is ready to be built and tested if you want to setup your own test environment.

If you are curious, clone the directories and have your friendly local AI analyse the code base (documentation is out of date except for specifications) - tell it to find out the capabilities of the project from the implemented source code, not the comments or documentation. This will give you a good understanding of what is done and what is going to be done.

The drivers and management interface (ScratchBird-drivers and ScratchRobin) are getting heavy testing and updating. They are getting multiple commits per day on average.

The initial preview will be a docker containing the database engine and an app-image or standalone executable so that you can test the project without any problems of getting rid of it afterward.

This project has become my answer to the constant "Damn I wish I had the ability to...." issues I have encountered over 35 years of database use.

I have been seeing multiple clones of my project(s) via the tracker but I have not received any feedback yet - don't be afraid, I need feedback and I don't bite.

I am sure there are things others have encountered over the years and wish they had a tool to cover it.

Thanks for your interest in the project.

---
## 🌟 Key Features

### AI-Powered Database Assistance
- **Multi-Provider Support** - OpenAI, Anthropic, Ollama, Google Gemini
- **SQL Assistant** - Natural language to SQL, query explanation, optimization suggestions
- **Schema Design Help** - AI-assisted table design and index recommendations
- **Secure API Key Storage** - Credentials stored in system keyring

### Issue Tracker Integration
- **Multi-Platform Support** - Jira, GitHub, GitLab integration
- **Object-Issue Linking** - Link database objects (tables, columns, procedures) to issues
- **Bi-Directional Sync** - Real-time updates via webhooks and scheduled sync
- **Issue Templates** - Auto-create issues for bugs, enhancements, or refactoring
- **Context Generation** - Automatic context extraction for issue creation

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

**Current Version:** 0.2.0-alpha  
**Last Updated:** 2026-02-03

### Recently Completed 🎉
- **AI Integration Framework** - Complete implementation with multi-provider support
- **Issue Tracker System** - Full Jira/GitHub/GitLab integration with sync scheduler
- **SQL Editor AI Assistant** - Context-aware query help and optimization
- **All Core Object Managers** - Tables, Indexes, Views, Triggers, Procedures, Sequences, Packages
- **Complete ERD System** - 5 notations, auto-layout, forward/reverse engineering

**Current Version:** 0.2.0-alpha

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

### Newly Implemented (2026-02) 🎉
- [x] **AI Integration** - Multi-provider AI assistance (OpenAI, Anthropic, Ollama, Google Gemini)
- [x] **Issue Tracker Integration** - Jira, GitHub, GitLab issue linking and sync
- [x] **SQL Assistant** - AI-powered query help, optimization, and explanation

### Planned 📋
- [ ] Deployment plans and management
- [ ] Data lineage tracking
- [ ] Data masking
- [ ] API generation
- [ ] CDC/Event streaming

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

### AI Integration
- [AI_INTEGRATION_SPEC.md](docs/AI_INTEGRATION_SPEC.md) - AI provider setup and usage

### Issue Tracking
- [ISSUE_TRACKER_SPECIFICATION.md](docs/ISSUE_TRACKER_SPECIFICATION.md) - Issue tracker configuration

### Git Integration
- [GIT_INTEGRATION.md](docs/GIT_INTEGRATION.md) - Dual-repo Git model
- [DATABASE_GIT_PROTOCOL.md](docs/DATABASE_GIT_PROTOCOL.md) - Database Git protocol
- [GIT_WORKFLOW_EXAMPLES.md](docs/GIT_WORKFLOW_EXAMPLES.md) - Practical examples

### Additional Features
- [ADDITIONAL_FEATURES.md](docs/ADDITIONAL_FEATURES.md) - Cube design, testing, lineage
- [planning/IMPLEMENTATION_ROADMAP.md](docs/planning/IMPLEMENTATION_ROADMAP.md) - Current execution roadmap

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
- libcurl (for AI and Issue Tracker features)
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

# With AI and Issue Tracker support (requires libcurl)
cmake -B build -DSCRATCHROBIN_ENABLE_AI=ON \
               -DSCRATCHROBIN_ENABLE_ISSUE_TRACKERS=ON
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
