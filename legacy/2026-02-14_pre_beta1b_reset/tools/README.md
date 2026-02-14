# Build/Test Utilities

This folder contains helper scripts that keep build/test runs self-cleaning by
using a temporary directory that is removed after completion.

## Usage

Build (uses `build/tmp` for temporary files and cleans it on exit):

```bash
tools/build.sh
```

Run tests (uses `build/tmp` for temporary files and cleans it on exit):

```bash
tools/test.sh
```

## Environment Variables

- `BUILD_DIR` — override the build directory (default: `./build`)
- `BUILD_JOBS` — number of parallel build jobs (default: `4`)
- `TMPDIR` — override temp directory (default: `build/tmp`)
- `CTEST_ARGS` — extra arguments passed to `ctest`
- `SCRATCHROBIN_RUN_PERF_TESTS` — enable performance tests (set to `1` to run)
- `SCRATCHROBIN_PERF_MS_FACTOR` — scale performance time limits (e.g., `2.0` doubles thresholds)
