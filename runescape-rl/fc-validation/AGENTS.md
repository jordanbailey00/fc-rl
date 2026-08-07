# fc-validation Instructions

These rules apply to executable parity, architecture, contract, and regression guardrails.

## Purpose

Validation code consumes `fc-core` and the training adapter as systems under test. It must not become a hidden implementation layer or require test-only behavior in production code.

## Test design

- Every bug fix or semantic change should add the smallest deterministic regression test that fails for the old behavior and passes for the corrected behavior.
- Test externally meaningful state transitions, events, outputs, ordering, and invariants rather than internal function shape or incidental line structure.
- Use fixed seeds, explicit action sequences, and bounded tick counts. Do not use wall-clock sleeps, uncontrolled randomness, network state, or machine-speed assumptions.
- Cover the relevant positive, negative, boundary, reset, and cross-tick cases. For ordering bugs, assert both the allowed and forbidden same-tick sequences.
- For observation/action/reward changes, test dimensions, offsets/order, normalization, firing conditions, masks, version expectations, and reset behavior as applicable.
- For pathfinding/collision changes, include footprints, diagonals, occupied destinations, intermediate steps, start reservations, and self-ignore behavior as applicable.
- For determinism changes, compare complete authoritative outputs or state hashes across repeated runs.

## Keep tests maintainable

- Name tests for the invariant or behavior, not only the implementation phase that introduced them. Use CTest labels for project phase, subsystem, or workstream metadata.
- Reuse small setup/assertion helpers for repeated scenario construction, but do not create a test framework more complex than the simulator behavior being tested.
- Do not keep appending unrelated logic to one giant guardrail function. When a coherent domain becomes substantial, split it into focused source files or targets while preserving discoverable CTest registration.
- Remove or rewrite obsolete tests when behavior is intentionally replaced. Do not keep tests that validate a superseded implementation.
- Static source-text guardrails are a last resort for architecture constraints that cannot be tested behaviorally. Keep patterns narrow and explain why source inspection is required.

## Production-code boundary

- Never add a production branch, exported function, state field, or behavior solely to make a test easy.
- Prefer exercising the public core API. Use intentionally exposed test hooks only for state that cannot be reached reliably through public actions.
- Test fixtures may construct precise state, but expected outcomes should follow public contracts rather than copy-pasted production algorithms.

## Commands

```bash
cmake -S runescape-rl -B runescape-rl/build -DCMAKE_BUILD_TYPE=Release
cmake --build runescape-rl/build -j --target phase2_guardrails_core phase2_guardrails_training
ctest --test-dir runescape-rl/build -L guardrail --output-on-failure
```

Use subsystem labels or `-R <test-name>` while iterating, then run all applicable guardrails before completion.
