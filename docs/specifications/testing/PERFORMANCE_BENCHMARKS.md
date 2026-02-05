# ScratchRobin Performance Benchmarks

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Scope**: Performance metrics and testing framework

---

## Overview

This document defines performance benchmarks for ScratchRobin to ensure consistent user experience across releases and platforms.

## Benchmark Categories

### 1. Startup Performance

| Metric | Target | Critical Threshold |
|--------|--------|-------------------|
| Cold start time | < 2 seconds | > 5 seconds |
| Warm start time | < 500ms | > 2 seconds |
| Memory at startup | < 100 MB | > 200 MB |

**Measurement**: Time from process launch to main window visible and interactive.

### 2. Connection Performance

| Metric | Target | Critical Threshold |
|--------|--------|-------------------|
| Connect to local database | < 100ms | > 500ms |
| Connect to remote database | < 1 second | > 5 seconds |
| SSL handshake overhead | < 200ms | > 1 second |

### 3. Query Execution Performance

| Metric | Target | Critical Threshold |
|--------|--------|-------------------|
| Simple query (< 100 rows) | < 100ms | > 500ms |
| Medium query (1K-10K rows) | < 1 second | > 5 seconds |
| Large query (100K+ rows) | < 10 seconds | > 60 seconds |
| Result grid display (10K rows) | < 500ms | > 2 seconds |

### 4. ERD/Diagram Performance

| Metric | Target | Critical Threshold |
|--------|--------|-------------------|
| Load 50 entities | < 1 second | > 3 seconds |
| Load 200 entities | < 3 seconds | > 10 seconds |
| Load 500 entities | < 10 seconds | > 30 seconds |
| Auto-layout 50 entities | < 2 seconds | > 5 seconds |
| Auto-layout 200 entities | < 5 seconds | > 15 seconds |
| Pan/zoom frame rate | > 30 FPS | < 15 FPS |

### 5. Memory Usage

| Metric | Target | Critical Threshold |
|--------|--------|-------------------|
| Base memory | < 100 MB | > 200 MB |
| Per SQL editor | < 10 MB | > 50 MB |
| Per diagram (100 entities) | < 50 MB | > 200 MB |
| Large result set (1M rows) | < 500 MB | > 2 GB |
| Memory growth (8 hours) | < 50 MB | > 200 MB |

## Benchmark Implementation

### Test Harness

```cpp
class PerformanceBenchmark {
public:
    void StartTimer();
    void StopTimer();
    double GetElapsedMs() const;
    size_t GetMemoryUsage() const;
    
    void Report(const std::string& test_name);
};
```

### Automated Benchmarks

Run with: `./scratchrobin_benchmarks`

Results output to: `benchmarks/results_YYYYMMDD_HHMMSS.json`

## CI/CD Integration

Benchmarks run on every commit to `main` branch. Regression detection:
- Warning: > 10% slower than baseline
- Critical: > 25% slower than baseline

Baseline stored in: `benchmarks/baseline.json`
