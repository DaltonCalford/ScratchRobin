# ScratchRobin Tests

This directory contains the test suite for ScratchRobin, organized into unit tests and integration tests.

## Directory Structure

```
tests/
├── README.md                    # This file
├── test_main.cpp               # Google Test entry point
├── unit/                       # Unit tests
│   ├── test_metadata_model.cpp
│   ├── test_statement_splitter.cpp
│   ├── test_value_formatter.cpp
│   ├── test_result_exporter.cpp
│   ├── test_config.cpp
│   ├── test_credentials.cpp
│   ├── test_simple_json.cpp
│   ├── test_error_handler.cpp
│   ├── test_capability_detector.cpp
│   ├── test_job_queue.cpp
│   ├── test_session_state.cpp
│   ├── test_diagram_model.cpp
│   ├── test_layout_engine.cpp
│   └── test_forward_engineer.cpp
├── integration/                # Integration tests
│   ├── test_postgres_backend.cpp
│   ├── test_mysql_backend.cpp
│   └── test_firebird_backend.cpp
└── fixtures/                   # Test fixtures and data
```

## Running Tests

### Build Tests

```bash
cd /path/to/ScratchRobin
mkdir build && cd build
cmake ..
make scratchrobin_tests
```

### Run All Tests

```bash
./scratchrobin_tests
```

### Run Specific Test Suite

```bash
./scratchrobin_tests --gtest_filter="MetadataModelTest.*"
```

### Run with Verbose Output

```bash
./scratchrobin_tests --gtest_verbose
```

## Integration Tests

Integration tests require running database servers. They are automatically skipped if not configured.

### PostgreSQL Tests

Set environment variable:
```bash
export SCRATCHROBIN_TEST_PG_DSN="host=localhost port=5432 dbname=test user=postgres password=secret"
```

### MySQL Tests

Set environment variable:
```bash
export SCRATCHROBIN_TEST_MYSQL_DSN="host=localhost port=3306 dbname=test user=root password=secret"
```

### Firebird Tests

Set environment variable:
```bash
export SCRATCHROBIN_TEST_FB_DSN="host=localhost port=3050 dbname=/path/to/test.fdb user=sysdba password=masterkey"
```

## Test Coverage

Generate coverage report (requires gcovr):

```bash
cmake -DSCRATCHROBIN_ENABLE_COVERAGE=ON ..
make
make test
gcovr -r .. --html -o coverage.html
```

## Writing New Tests

### Unit Test Template

```cpp
#include <gtest/gtest.h>
#include "path/to/component.h"

using namespace scratchrobin;

class ComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test fixtures
    }
    
    void TearDown() override {
        // Clean up
    }
};

TEST_F(ComponentTest, TestName) {
    // Arrange
    
    // Act
    
    // Assert
    EXPECT_EQ(expected, actual);
}
```

### Test Naming Conventions

- Test files: `test_<component>.cpp`
- Test fixtures: `<Component>Test`
- Test cases: `<Action><Condition>`
  - Examples: `ParseValidJson`, `RejectInvalidInput`, `HandleEmptyData`

## Continuous Integration

Tests are run automatically on:
- Every commit
- Pull requests
- Nightly builds

## Test Statistics

- Unit Tests: 16 test suites
- Integration Tests: 3 backend adapters
- Total Test Cases: 200+
- Target Coverage: >80%
