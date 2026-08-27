# Central Project TODO

This is the authoritative working list for remaining FC-RL work. Completed
parity, refactor, and code-analysis work is consolidated in
`runescape-rl/docs/archive/fc_cleanup_and_parity_history.md`; active and
deferred work is tracked only here.

## Current Engineering Priorities

### 1. Policy-Visible Combat Information — decision complete

- [x] Reviewed the NPC observations that explicitly expose committed attack
  style, exact remaining hit ticks, whether prayer can still affect the hit,
  and prayer-deadline urgency.
- [x] Decision: retain the current explicit combat timing information for the
  intended abstract Fight Caves MDP. The observed attack has already been
  committed, so this is a task-definition choice rather than reward leakage.

### 2. Native Action Masks and Policy Input — decision complete

- [x] Reviewed the intended use of native Puffer action masks for preventing
  impossible movement and nonexistent NPC targets.
- [x] Confirmed that the model receives the full 319-value environment observation
  (`285` policy features plus `34` mask bits) even though comments describe the
  policy input as only the 285 policy features.
- [x] Decision: leave the current native-mask enforcement and policy-visible
  legality bits unchanged. The additional collision and target-availability
  information is accepted as part of the current task contract.

### 3. Run Energy and Movement Parity

- [ ] Audit run-energy initialization, movement validity, gameplay drain, and
  regeneration. The current behavior appears to allow effectively unlimited
  running because normal drain/regeneration is not meaningfully simulated.
- [ ] Review the resulting OSRS parity and movement/kiting difficulty impact.
- [ ] Review remaining NPC movement and pathfinding approximations, especially
  obstacle and body-blocking behavior, while preserving the improvements from
  the recent LOS and collision work.

### 4. Recurrent-State and Evaluation Hygiene

- [ ] Review MinGRU hidden-state lifecycle. It currently appears to reset at
  rollout boundaries rather than when an individual Fight Caves episode
  auto-resets, allowing a new episode to temporarily inherit recurrent state
  from the previous episode.
- [ ] Review post-training evaluation isolation. It currently appears to
  continue the same environment and RNG streams rather than starting a fully
  isolated held-out evaluation set.
- [ ] Treat both findings as correctness and evaluation-quality concerns, not
  evidence of intentional leakage.

### 5. Codebase Refactor and Maintainability Pass — completed

- [x] Completed the approved core, training, and viewer refactor and dead-code
  cleanup program with deterministic tests and fixed 100M training comparisons.
- [x] Archived the implementation history, rationale, removals, contract
  migrations, and validation in
  `runescape-rl/docs/archive/fc_cleanup_and_parity_history.md`.

## Current Baseline: v4.5 `txqsiahp`

- The authoritative live config is `runescape-rl/config/fight_caves.ini`.
- Its architecture, trainer hyperparameters, seed, and 750M-step budget remain
  pinned to sweep winner `1nvvx5qu` and deterministic retrain `8oivozuq`.
- Its live environment includes the approved OSRS movement/LOS parity backend
  and the zero-danger-Prayer-reward setting evaluated by W&B run `txqsiahp`.
- W&B run `i215ulj4` is the otherwise comparable parity-backend run with the
  previous danger-Prayer reward. `txqsiahp` improved Jad kill rate from 88.02%
  to 92.74% and wave-63 reach to 94.12%.
- W&B runs `88l9p7ie`, `z5vbs56z`, `hevp6ehc`, and `l9o32hhz` remain earlier
  comparison evidence rather than the live baseline.
- The source and synchronized Puffer INIs are byte-identical with SHA-256
  `33bca87ccde19636c3b742b7e292732978307f648a8e80f6849f1d8d1612c9b6`.
- The original `v4_simple_reward` derivation, exact config, trajectory, and
  `l9o32hhz` metrics are recorded at the top of
  `runescape-rl/docs/run_history.md`. The completed top-eight Stage 2 sweep
  analysis is recorded in `sweep_top8.md`; newer run-history documentation is
  tracked below.

## Completed Stage 2 Experiment

- [x] Completed the corrected 130-run, 750M-step value/optimizer,
  architecture, and agent-count sweep without changing the environment,
  rewards, observations, actions, loadout, or seed within the sweep.
- [x] Ranked every run using Jad completion, wave-63 reach, conditional Jad
  conversion, consistency, progression, and value-learning behavior.
- [x] Selected the robust `512 / 3 / 4,096` region represented by `1nvvx5qu`,
  reproduced it outside sweep mode as `8oivozuq`, and recorded the top-eight
  analysis in `sweep_top8.md`.

## Planned Trainer Experiment Sequence

### 5. Stage 3: Learning-Rate Schedule

- [ ] Starting from the selected Stage 2 configuration, compare:
  - constant learning rate;
  - decay to zero;
  - decay to a `0.05` final learning-rate ratio;
  - decay to a `0.10` final learning-rate ratio;
  - decay to a `0.25` final learning-rate ratio.
- [ ] Keep total timesteps fixed because Puffer schedules annealing against the
  configured run budget.
- [ ] Evaluate whether scheduling preserves strong late policies and reduces
  peak-to-final degradation.

### 6. Stage 4: Architecture and Batch Sweep (Combined into Stage 2)

- [x] Include these dimensions in the combined Stage 2 search:
  - `policy.hidden_size`: `128`, `256`, `512`;
  - `policy.num_layers`: `2`, `3`, `4`;
  - `vec.total_agents`: `2048`, `4096`, `8192`.
- [x] Compare Jad completion, consistency, parameter count, VRAM, SPS, and
  wall-clock cost. Do not select a larger model from cave progress alone.

### 7. Final Multi-Seed Confirmation

- [ ] Rerun the strongest configurations at the same development budget with
  trainer seeds `73`, `101`, and `202`.
- [ ] Select finalists using aggregate Jad completion and consistency rather
  than one lucky run.
- [ ] Run the top two or three configurations at fixed 1.5B and/or 2.5B-step
  budgets.
- [ ] Account for Puffer's priority-beta schedule when comparing different
  configured timestep budgets.
- [ ] Rerun sweep-selected configurations outside sweep mode when checkpoints
  are required.

## Remaining Encounter-Parity Work

- [ ] Implement real run-energy drain and regeneration, add focused parity
  tests, and retrain/compare against the current baseline.
- [ ] Decide and then test/implement the remaining confirmed encounter
  differences individually:
  - Ket-Zek and Jad Magic attack bonuses;
  - spawn-region tile selection;
  - Jad healer activation threshold;
  - healer aggression, return, and healing behavior;
  - inter-wave delay;
  - large Tz-Kek melee recoil.
- [ ] Establish stronger evidence before changing mechanics where the exact
  modern OSRS behavior remains unresolved:
  - Yt-MejKot healing probability;
  - Tok-Xil adjacent melee/ranged selection probability;
  - Jad Prayer-lock server-cycle boundary;
  - generic NPC natural regeneration rate.

## Remaining Cleanup and Refactor Work

The legacy `MINA`/animation-v1 asset reader is intentionally retained for
possible future asset files and export pipelines. It is not a cleanup target.

- [x] Convert the implementation-heavy viewer headers for animation loading,
  models, NPC models, objects, spot animations, terrain, and the debug overlay
  into conventional `.c/.h` modules with one compiled implementation each.
  Tiny inline I/O helpers and data-only headers remain headers intentionally.
- [ ] Add the strict compiler, first-party static-analysis, and linker
  reachability checks as an explicitly invoked audit test under
  `fc-validation`. Do not put it in the normal per-change test path or run it
  automatically for every implementation.
- [x] Determine why `fc-training/build.sh` globally passes
  `-Wno-unused-but-set-variable` before changing it. History shows that it was
  introduced when the host compiler changed from Clang to GCC, but does not
  document the warning it was meant to silence. It currently applies to the C
  core/adapter/standalone compilations, not NVCC or the C++ binding build.
  Unsuppressed local, CPU, and CUDA builds now pass without this warning.
- [ ] Remove `-Wno-unused-but-set-variable` from the production build after
  approval. No narrower placement is currently warranted because none of the
  three supported build modes needs the suppression.
- [x] Investigate further decomposition of `viewer.c` and `ui.c`. The strongest
  viewer ownership boundaries are combat presentation/projectile lifecycle,
  actor animation lifecycle, and viewer-console controls. `ui.c` remains a
  cohesive UI subsystem; split its input handling into private per-region
  helpers before considering separate input/draw translation units.
- [ ] Extract viewer combat-presentation and actor-animation lifecycle modules
  using owned sub-state, then consider extracting viewer-console controls. Do
  not pass the entire `ViewerState` into new modules merely to reduce file size.
- [x] The latest strict-warning, Cppcheck, and exhaustive redundant-condition
  audit found no additional high-confidence dead production logic or
  redundant branches requiring removal.

## Deferred Environment and Control Debt

These items are known but are not immediate blockers for the current baseline
or the planned trainer experiments.

- [ ] Revisit the intermittent suboptimal click-to-tile route issue. The last
  routing correction improved collision behavior but did not eliminate every
  observed non-shortest route. Require a reproducible scenario and focused
  failing guardrail before another correction.
- [ ] Revisit target persistence and automatic approach/pathing control only if
  checkpoint replays show clear targeting, disengagement, retargeting, or
  movement-control failures.
- [ ] If that work is justified, first add focused diagnostics for target
  acquisition/drop/switching, retarget latency, stale targets, route starts,
  completions, cancellations, stale routes, policy movement overrides, and
  approach success/timeouts.
- [ ] Keep static terrain, walls, diagonal clipping, arena boundaries, and NPC
  occupancy rules under regression coverage while changing player routing.

## Trainer Validation Debt

These are reference-level learner checks from the earlier reward/observation
training plan. Implement them as external validation in `fc-validation`; do
not modify vendored PufferLib for FC-RL-specific tests.

- [ ] Verify rollout and PPO masked log-probabilities against a reference
  implementation.
- [ ] Verify masked entropy against a reference implementation.
- [ ] Prove every environment reward is consumed exactly once by GAE.
- [ ] Prove horizon-boundary rewards are retained.
- [ ] Prove terminal rewards at every possible rollout index are retained.
- [ ] Verify scalar and vector advantage implementations agree.
- [ ] Verify recurrent state resets correctly at episode boundaries, or record
  and test the intentionally selected behavior.

## Later Research Backlog

These are not the current next step. Review them against the current parity
contracts and baseline before reviving them.

- [ ] Test a late-wave/Jad-focused curriculum with a separate fixed full-cave
  evaluation distribution.
- [ ] Compare minimal terminal/progress reward designs without mixing reward,
  observation, and trainer changes in one experiment.
- [ ] Run grouped observation ablations, including coherent incoming-hit, NPC
  identity/type, slot-layout, rotation, and encoder comparisons.
- [ ] Establish a vanilla PPO control, then reintroduce replay, prioritized
  sampling, and V-trace one at a time if each proves beneficial.
- [ ] Revisit further reward simplification only as a controlled experiment;
  do not treat the older rejected Phase 7 variants as active baselines.

## Repo / PufferLib PR Readiness

- [ ] Clean up the entire repo so it is ready to PR into PufferLib on GitHub.
  - Align file layout, build scripts, config style, docs, tests, and asset handling
    with PufferLib conventions.
  - Organize assets, UI, and viewer frontend data into clean installable/downloadable
    packs instead of relying on loose local clutter.
  - Provide a script or setup path that installs required Fight Caves assets when a
    user runs the environment.
  - Keep raw cache inputs local/external unless there is a deliberate reason to ship
    them.
  - Ensure the environment can build, train, evaluate, and render from a clean clone
    using documented commands.
- [ ] Do not modify vendored PufferLib for FC-RL-specific behavior; keep the
  integration in FC-RL modules and adapters.

## Stale Documentation and Repository Tracking

- [x] Consolidate the completed root `refactor.md`, `code_analysis.md`, and
  `osrs_parity.md` documents into the archival history at
  `runescape-rl/docs/archive/fc_cleanup_and_parity_history.md`. Keep all
  remaining work in this file.
- [ ] Update the root README's outdated SOTA and project-status sections.
- [ ] Add the README architecture diagram currently marked TODO.
- [ ] Record `hevp6ehc`, `z5vbs56z`, `88l9p7ie`, `1nvvx5qu`, `8oivozuq`,
  `i215ulj4`, and `txqsiahp`, together with their relevant comparisons, in
  `runescape-rl/docs/run_history.md`.
- [ ] Remove or replace links from tracked documentation to ignored local
  files, including `sweep_history/v3_simple_reward_sweep.md`, so a clean clone
  has no broken documentation references.
- [ ] Decide whether the following currently ignored local documents should be
  force-added/tracked or deliberately remain local-only:
  - `obs.md`
  - `parity_fix.md`
  - `parity_fix_tests.md`
  - `parity_fix_config.md`
  - `hparam_sweep_07-16.md`
  - `SOTA_DIFF_AUDIT.md`
