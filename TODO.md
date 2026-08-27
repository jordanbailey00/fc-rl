# Central Project TODO

This file is the authoritative list of unfinished FC-RL work. Completed
decisions, parity changes, refactors, dead-code removal, documentation cleanup,
and validation results are preserved in
[`fc_cleanup_and_parity_history.md`](runescape-rl/docs/archive/fc_cleanup_and_parity_history.md).
Current configuration and result context lives in [`README.md`](README.md) and
[`run_history.md`](runescape-rl/docs/run_history.md).

## Mechanics and OSRS parity

- [ ] Implement real run-energy initialization, movement drain, and
  regeneration; add focused parity tests and retrain against the current
  baseline to measure the movement and kiting impact.
- [ ] Review the remaining NPC movement/pathfinding approximations, especially
  obstacle and body-blocking behavior, while preserving the current LOS,
  collision, footprint, and route-ordering guarantees.
- [ ] Verify and implement each remaining confirmed encounter difference as an
  isolated change with focused tests:
  - Ket-Zek and Jad's missing `+60` Magic attack bonus;
  - random tile selection within spawn regions instead of fixed center tiles;
  - Jad healer activation threshold;
  - Jad healer aggression, return, and healing behavior;
  - inter-wave delay;
  - large Tz-Kek melee recoil.
- [ ] Establish stronger primary-source evidence before changing mechanics
  whose exact modern OSRS behavior remains unresolved:
  - Yt-MejKot healing probability;
  - Tok-Xil adjacent melee/ranged selection probability;
  - exact Jad Prayer-lock server-cycle boundary;
  - generic NPC natural-regeneration rate.

## Training and evaluation correctness

- [ ] Verify MinGRU hidden-state lifecycle at individual episode boundaries.
  It currently appears possible for state to reset at rollout boundaries
  rather than each auto-reset; prove the behavior with an evaluator/trainer
  test before changing it.
- [ ] Decide whether post-training evaluation should use isolated held-out
  environment and RNG streams instead of continuing training streams, then
  implement and document the selected evaluation contract.
- [ ] Verify rollout and PPO masked log-probabilities against a reference
  implementation.
- [ ] Verify masked entropy against a reference implementation.
- [ ] Prove that every environment reward is consumed exactly once by GAE.
- [ ] Prove that horizon-boundary rewards are retained.
- [ ] Prove that terminal rewards at every rollout index are retained.
- [ ] Verify that scalar and vector advantage implementations agree.

Keep FC-RL-specific trainer validation in `fc-validation`; do not modify
vendored PufferLib for environment-specific behavior.

## Next trainer experiments

- [ ] Run the Stage 3 learning-rate schedule comparison from the selected Stage
  2 configuration:
  - constant learning rate;
  - decay to zero;
  - decay to a `0.05` final learning-rate ratio;
  - decay to a `0.10` final learning-rate ratio;
  - decay to a `0.25` final learning-rate ratio.
- [ ] Keep total timesteps fixed across the schedule comparison because Puffer
  schedules annealing against the configured run budget.
- [ ] Determine whether scheduling preserves strong late policies and reduces
  peak-to-final degradation.
- [ ] Rerun the strongest configurations at the same development budget with
  trainer seeds `73`, `101`, and `202`.
- [ ] Select finalists using aggregate Jad completion and consistency rather
  than one run, then train the top two or three at fixed 1.5B and/or 2.5B-step
  budgets.
- [ ] Account for Puffer's priority-beta schedule when comparing different
  configured timestep budgets.
- [ ] Rerun sweep-selected configurations outside sweep mode whenever a
  replayable checkpoint is required.

## Deferred environment and control debt

- [ ] Revisit the intermittent suboptimal click-to-tile route only after
  capturing a reproducible scenario and focused failing guardrail.
- [ ] Revisit target persistence and automatic approach/pathing control only if
  checkpoint replays show clear targeting, disengagement, retargeting, or
  movement-control failures.
- [ ] If target/approach work is justified, first add focused diagnostics for
  acquisition, drops, switches, retarget latency, stale targets, route starts,
  completions, cancellations, policy overrides, and approach timeouts.
- [ ] Keep static terrain, walls, diagonal clipping, arena boundaries, and NPC
  occupancy rules under regression coverage during routing changes.

## Validation tooling

- [ ] Add strict compiler warnings, first-party static analysis, and linker
  reachability checks as an explicitly invoked audit under `fc-validation`.
  Keep this out of the normal per-change test path.

The legacy `MINA`/animation-v1 asset reader is intentionally retained for
future asset files and export pipelines; it is not a cleanup target.

## Later research backlog

- [ ] Test a late-wave/Jad-focused curriculum with a separate fixed full-cave
  evaluation distribution.
- [ ] Compare minimal terminal/progress reward designs without mixing reward,
  observation, and trainer changes in one experiment.
- [ ] Run grouped observation ablations, including coherent incoming-hit, NPC
  identity/type, slot-layout, rotation, and encoder comparisons.
- [ ] Establish a vanilla PPO control, then reintroduce replay, prioritized
  sampling, and V-trace one at a time if each proves beneficial.
- [ ] Revisit further reward simplification only as a controlled experiment;
  do not revive rejected historical reward variants as active baselines.

## PufferLib PR readiness

- [ ] Align the environment's file layout, build scripts, config style, docs,
  tests, and asset handling with PufferLib conventions.
- [ ] Package the required core and viewer assets into deliberate installable
  or downloadable bundles instead of relying on loose development files.
- [ ] Provide a documented setup path that installs required Fight Caves
  assets while keeping raw cache inputs external unless deliberately shipped.
- [ ] Verify build, training, evaluation, and rendering from a clean clone.
- [ ] Keep all FC-RL integration in FC-RL modules and adapters; do not modify
  vendored PufferLib for environment-specific behavior.
