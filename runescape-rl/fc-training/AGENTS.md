# fc-training Instructions

These rules apply to the PufferLib adapter and training integration.

## Ownership

`fc-training` translates between PufferLib and `fc-core`. It may own:

- adapter structs and lifecycle hooks;
- action-head translation and native mask publication;
- configuration parsing and parameter transfer;
- reward-parameter transfer, reward publication, clipping diagnostics, and episode analytics;
- trainer-facing logs;
- local CPU/CUDA/build integration.

It must not own a second implementation of gameplay, observation semantics, action legality, NPC behavior, pathfinding, combat, or wave progression.

## Adapter discipline

- Call the canonical `fc-core` API and consume `fc_contracts.h` constants. Do not mirror dimensions, offsets, NPC ordering, or action meanings with independent literals.
- Keep the Puffer-facing three-head no-supplies policy mapping explicit and separate from the seven canonical core action heads.
- Keep the float compatibility mask and PufferLib native mask derived from the same core mask in the same write path. Never allow them to drift.
- Reset every adapter runtime, reward runtime, diagnostic accumulator, seed counter dependency, and episode log at the correct boundary.
- Analytics must observe behavior, not change behavior. Logging, counters, W&B fields, or diagnostic classifications must not influence simulation transitions or rewards.

## Reward and config plumbing

When adding, changing, or deleting a reward/config parameter, update the whole path atomically:

1. canonical config key;
2. adapter field/default/parser;
3. `FcRewardParams` transfer if applicable;
4. reward breakdown/channel logging;
5. episode reset and terminal drain;
6. run manifest and policy/reward version metadata when semantics change;
7. viewer diagnostics if displayed;
8. validation tests.

Do not leave zero-effect compatibility keys. If a term is retired, delete its field, parser, config entry, logs, docs, and tests unless a documented checkpoint or external-interface requirement prevents removal.

Reward clipping belongs to the trainer contract. Keep pre-clip and post-clip diagnostics explicit and do not silently clip in multiple layers.

## Maintainability and performance

- Do not grow `fight_caves.h` by default. For substantial cohesive adapter-only logic, first consider a focused local helper module/header that can be consumed by all training build modes. Do not split out tiny one-use wrappers.
- Keep per-step code allocation-free and free of Python calls, filesystem access, network calls, formatting, or unbounded logging.
- Prefer episode-end aggregation over per-tick string/log emission.
- Do not add a local workaround for a core bug. Fix the authoritative core behavior and keep the adapter thin.
- Remove superseded mappings, counters, parser branches, and compatibility paths when replacing them.

## PufferLib boundary

Do not edit `pufferlib_4` to implement an FC-RL-specific feature when the behavior can live in this adapter, config, or tooling layer. An upstream modification requires an explicit task and evidence that no clean extension point exists.

## Build and validation

The shared core C API shim and training adapter guardrails should continue to build:

```bash
cmake -S runescape-rl -B runescape-rl/build -DCMAKE_BUILD_TYPE=Release
cmake --build runescape-rl/build -j --target fc_capi phase2_guardrails_training
ctest --test-dir runescape-rl/build \
  -R 'rng_seed_diversity|no_supplies_policy_contract|native_action_mask_adapter' \
  --output-on-failure
ctest --test-dir runescape-rl/build -L action_mask --output-on-failure
```

When the local toolchain is available, use the appropriate integration build:

```bash
bash runescape-rl/fc-training/build.sh --local
bash runescape-rl/fc-training/build.sh --cpu
```

Run the CUDA build and architecture guardrails only for CUDA-relevant changes or when explicitly requested. Do not use a long RL run as the first validation step for adapter correctness.
