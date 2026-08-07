# Root Agent Instructions

These instructions apply to the entire repository. A nested `AGENTS.md` adds or narrows rules for its directory.

## Primary objective

Produce the simplest correct change that preserves FC-RL's architectural boundaries, deterministic behavior, and experiment reproducibility.

Do not optimize for the amount of code produced. Prefer a smaller conceptual surface: fewer code paths, fewer sources of truth, less duplicated state, and fewer abstractions that a maintainer must understand.

## Repository boundaries

- `runescape-rl/fc-core/` owns deterministic simulation state, mechanics, tick ordering, pathfinding, combat, NPC behavior, waves, observations, masks, reward events, and public simulation contracts.
- `runescape-rl/fc-training/` adapts `fc-core` to PufferLib and owns trainer-facing configuration plumbing, reward-parameter parsing and transfer, reward publication, clipping diagnostics, action translation, episode analytics, and training build integration.
- `runescape-rl/fc-viewer/` owns rendering, assets, UI, human input translation, replay presentation, and diagnostics. It must not become a second simulator.
- `runescape-rl/fc-validation/` owns executable behavioral guardrails and regression tests that consume the simulator as the system under test.
- `runescape-rl/config/` owns canonical live experiment configuration.
- `runescape-rl/tools/` owns reproducibility, validation, analysis, and operational tooling. Tools must not contain gameplay behavior.
- `pufferlib_4/` is a vendored upstream dependency. Do not modify it for an FC-RL-specific change unless the task explicitly requires a PufferLib change and the adapter cannot solve the problem cleanly.

Respect these boundaries even when placing logic elsewhere would be faster in the moment.

## Required workflow before editing

For any nontrivial change:

1. Read the nearest applicable `AGENTS.md` files.
2. Inspect the current implementation, its callers, relevant tests, and related contracts before choosing a design.
3. Search for existing helpers, parallel implementations, stale compatibility paths, and configuration keys that already address part of the task.
4. Identify the authoritative source of truth for every behavior or value being changed.
5. Form a brief change plan covering:
   - existing code to reuse or modify;
   - obsolete code to remove;
   - contracts and downstream consumers affected;
   - tests that will prove the behavior.

Do not start by adding a new helper, file, flag, or code path. First determine whether the existing design should be simplified, consolidated, or replaced.

## Implementation policy

- Prefer modifying, consolidating, or deleting existing code over adding a parallel implementation.
- When new behavior supersedes old behavior, remove the obsolete implementation, callers, tests, configuration, and documentation in the same change.
- Keep one source of truth for each mechanic, contract, derived value, configuration value, and piece of mutable state.
- Do not preserve an interface solely because it already exists. Preserve it when it has a known caller, serialized contract, checkpoint compatibility requirement, or other documented compatibility need.
- Do not add speculative abstractions, generic frameworks, feature flags, fallback paths, compatibility layers, or configuration knobs for hypothetical future work.
- Add an abstraction only when it removes meaningful duplication, expresses a real invariant, or enforces an architectural boundary.
- Keep changes cohesive. Perform cleanup needed to make the implementation internally consistent, but do not refactor unrelated subsystems.
- Comments should explain non-obvious invariants, ordering constraints, parity decisions, or why a simpler-looking alternative is wrong. Do not narrate obvious code.
- Match existing naming and data-flow conventions unless the task is explicitly correcting those conventions.
- Treat warnings, ignored return values, silent fallback behavior, and partial initialization as correctness issues rather than cosmetic issues.

Net line count is diagnostic, not a goal. A larger patch may be correct when it replaces a worse design; a smaller patch may be worse when it hides complexity.

## Cross-module change rules

A change is incomplete when it updates a producer but leaves consumers, versions, diagnostics, or tests stale.

### Mechanics or state-transition changes

Check all of the following as applicable:

- `fc-core` implementation and public headers;
- tick-order and deterministic-state implications;
- observations, masks, reward events, and render snapshots derived from the changed state;
- viewer diagnostics or controls that expose the behavior;
- training analytics that interpret the behavior;
- focused `fc-validation` regression tests.

### Observation, action, mask, or reward-contract changes

Treat these as schema migrations. Update atomically:

- `fc-core/include/fc_contracts.h`, the single source of truth;
- core writers/readers and any normalization or ordering rules;
- Puffer-facing adapter dimensions and mappings;
- native and compatibility mask paths;
- viewer diagnostics, replay/input mapping, and labels;
- config/manifest version strings when policy-visible semantics change;
- static and behavioral guardrails.

Do not duplicate numeric offsets or dimensions in another module when a contract constant can be consumed.

### Reward changes

Keep these responsibilities separate:

- `fc-core` records authoritative events and computes the parameterized reward breakdown and scalar total;
- `fc-training` supplies configured parameters, publishes the reward to PufferLib, tracks pre/post-clip diagnostics, and records analytics;
- `config` defines the canonical live experiment values;
- `fc-viewer` only displays the same breakdown;
- `fc-validation` verifies firing conditions, signs, scaling, clipping assumptions, and reset behavior.

Do not add a reward term without deciding what obsolete or overlapping shaping can be removed.

### Configuration changes

Every live key must have a real reader, a documented effect, a reproducible manifest value, and validation where practical. Delete dead keys across the entire path instead of leaving no-op compatibility entries.

### Documentation and history

Keep the root README and current documentation aligned with live behavior. Treat completed run results, sweep history, hashes, and historical configuration records as evidence; correct or supersede them explicitly rather than silently rewriting them.

## Simplification pass

After the implementation works and targeted tests pass, review the complete diff separately and:

- remove dead, superseded, unreachable, or duplicate code;
- remove temporary adapters and migration scaffolding that are no longer required;
- remove unused fields, configuration keys, includes, comments, tests, and logging;
- combine duplicated calculations and state transitions under one authoritative helper;
- simplify control flow and naming where doing so makes invariants clearer;
- verify that old and new implementations do not coexist unnecessarily;
- verify that no module now owns behavior assigned to another module;
- verify that the patch did not introduce per-tick allocation, hidden nondeterminism, or avoidable hot-loop work.

Do not skip this pass merely because tests are green.

## Validation expectations

From the repository root, the standard C build and test path is:

```bash
cmake -S runescape-rl -B runescape-rl/build -DCMAKE_BUILD_TYPE=Release
cmake --build runescape-rl/build -j
ctest --test-dir runescape-rl/build --output-on-failure
```

Run the narrowest relevant tests while iterating, then the broader applicable suite before completion. Additional module-specific commands appear in nested files.

Do not run a long training job solely to compensate for missing deterministic tests. Add focused tests first. Run expensive training, sweeps, or large evaluations only when requested or when the task is specifically experimental.

Never claim a test, build, benchmark, or training result that was not actually run. State environment limitations explicitly.

## Completion report

Conclude nontrivial implementation work with:

- what existing code was reused;
- what was refactored, consolidated, or deleted;
- what new code or abstraction was added and why it was necessary;
- which contracts and downstream consumers were checked;
- exact tests and checks run, including failures or limitations;
- any relevant remaining duplication or technical debt.
