# Adapter Port Rules

## Policy

1. One parser dialect per configured port.
2. Parser logic must not implement protocol auto-detect fallback.
3. Each listener binds to exactly one configured parser dialect.

## Rationale

- Keeps protocol behavior deterministic.
- Avoids ambiguous parser selection under mixed clients.
- Matches ScratchBird operational constraints for parser/driver work.

## Implementation Notes

- Use explicit `port -> dialect` registry.
- Validate mapping before compile/forward work begins.
- Fail closed if no mapping exists.
