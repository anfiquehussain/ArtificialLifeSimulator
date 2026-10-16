## Description

Please include a summary of the changes and the relevant motivation or issue being resolved.

Fixes # (issue)

## Type of Change

- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing tests/snapshots to fail)
- [ ] Documentation update
- [ ] Performance improvement / refactor

## Architectural Checklist

- [ ] Decoupled: No SFML/rendering dependencies added to `include/alife/simulation/`, `biology/`, `ai/`, or `core/`
- [ ] Deterministic: Identical seeds produce bit-identical simulation results
- [ ] Zero-Allocation: No dynamic allocations inside hot simulation loops
- [ ] Tests added/updated: Automated unit tests cover new behavior in `tests/unit/`
- [ ] All 11 automated test suites pass locally via `ctest`
