# Non-Goals

1. Do not implement Firebird SQL execution in engine-side code.
2. Do not introduce a second query execution boundary outside `ServerSession`.
3. Do not add parser auto-detection fallback behavior.
4. Do not support non-native parser dialects in this project.
5. Do not rewrite ScratchBird core engine semantics in this repository.
6. Do not reinterpret MGA/TIP/OIT/OAT/OST recovery semantics as WAL-based core
   recovery behavior.
