# fc-core Instructions

These rules apply to the deterministic Fight Caves simulator.

## Ownership

`fc-core` is the authoritative source for gameplay state and state transitions. Combat, movement, pathfinding, collision, NPC behavior, waves, prayer, observations, action legality, reward events, and render snapshots must be derived here rather than reimplemented in training or viewer code.

Public contracts live in `include/`. Implementation details live in `src/`. Keep the public surface as small as practical.

## Determinism and state

- Put mutable simulation state in `FcState`, an explicitly owned runtime object, or a caller-provided buffer. Do not add mutable global or function-static state.
- Use only the repository RNG path (`fc_rng_*`) for stochastic simulation behavior. Do not use `rand`, wall-clock time, process IDs, address values, iteration-order accidents, or viewer frame timing.
- Preserve deterministic ordering and explicit tie-breakers for NPC slots, target selection, pathfinding candidates, hit resolution, and entity iteration.
- Never hash raw struct padding or depend on uninitialized bytes.
- Reset every episode-level and per-tick field at the correct lifecycle boundary. New state must be initialized, reset, and included in deterministic diagnostics where relevant.

## Hot-path constraints

- Do not allocate or free heap memory in `fc_step`, `fc_tick`, NPC ticks, pathfinding, combat resolution, observation writing, mask writing, or reward calculation.
- Prefer fixed-capacity arrays and caller-provided scratch buffers consistent with existing limits.
- Avoid hidden quadratic scans in per-NPC/per-tick loops. Reuse a computed ordering, occupancy map, or derived value when it is authoritative for the same tick.
- Do not cache derived state unless the cache has a clear invalidation rule and measurably removes repeated work.

## Tick and action semantics

The phase ordering in `fc_tick.c` is a behavioral contract. A change to prayer timing, attack initiation, movement, timers, NPC processing, pending-hit resolution, terminal checks, or per-tick flag clearing requires focused tests for the ordering itself.

- Resolve actions against the observation and identity semantics under which they were selected.
- Do not let later movement silently rebind a visible NPC slot or retroactively make a pre-movement attack legal.
- Keep explicit movement, stale routes, combat approach, target persistence, and projectile lifetime rules coherent. Do not patch one interaction with a special case that contradicts another path.
- Record combat and movement events at the authoritative moment they occur, not later by inferring them from final state.

## Contracts

`include/fc_contracts.h` is the single source of truth for observation layouts, action heads, reward-feature layouts, and action-mask dimensions.

- Do not reproduce contract offsets, dimensions, slot order, or normalization divisors as independent magic numbers.
- Keep observation slot ordering deterministic and shared by observations, masks, target resolution, and diagnostics.
- A policy-visible contract change must update every producer and consumer, config/manifest versions, and validation tests in one change.
- Keep raw gameplay events distinct from configurable reward weights. The simulator should expose facts; the training adapter supplies the active experiment parameters.

## Movement, collision, and pathfinding

- Use full entity footprints for static and dynamic legality checks.
- Apply dynamic occupancy throughout route search and step validation, not only at the final destination.
- Preserve start-of-tick reservation behavior where required to prevent entity swap-through.
- Centralize distance, line-of-sight, attack-position, and footprint legality calculations. Do not create slightly different versions for player, NPC, viewer, or tests.
- Prefer a general invariant-preserving pathfinding fix over NPC-type or wave-specific exceptions.

## Combat and NPC behavior

- Keep attack selection, legality, launch, travel/pending state, resolution, prayer lock timing, damage, healing, death, split/respawn, and wave progression as distinct phases with explicit data flow.
- NPC-specific mechanics belong behind NPC-type data or focused behavior helpers, not scattered conditionals across unrelated files.
- When adding an event used by observations, rewards, or analytics, create one authoritative event field and reset it correctly rather than reconstructing it downstream.

## Refactoring rules

- Before adding a helper, search `fc_combat`, `fc_pathfinding`, `fc_npc`, `fc_wave`, `fc_prayer`, and `fc_state` for the existing concept.
- Remove superseded helpers and call paths in the same patch.
- Do not move implementation into headers merely for convenience. Header-only logic is appropriate only for small, stable, performance-relevant helpers with no duplicate definition risk.
- If adding a new core source file, update both the normal CMake build and the training amalgamation/build path. Do not create two implementations to satisfy the two build modes.
- Keep `fc_capi.c` a thin ABI boundary; do not place simulator rules there.

## Tests

Add or update focused tests in `fc-validation` for any behavior change. Include the relevant boundary and ordering cases, not only the happy path.

Useful commands from the repository root:

```bash
cmake -S runescape-rl -B runescape-rl/build -DCMAKE_BUILD_TYPE=Release
cmake --build runescape-rl/build -j --target fc_core phase2_guardrails_core test_headless
ctest --test-dir runescape-rl/build -L guardrail --output-on-failure
```

For determinism-sensitive work, verify identical hashes or state outputs for identical seed/action sequences and divergence only where intended.
