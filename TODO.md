# Central Project TODO

This is the authoritative working list for remaining FC-RL work. Historical
plans and experiment reports remain in their original files as evidence, but
active and deferred work should be tracked here so there is one current source
of truth.

## Tomorrow's Priority: Environment/Training Review and Refactor

Review all four areas below together before making changes. More detailed
review criteria and implementation direction will be supplied during the
review.

### 1. Policy-Visible Combat Information

- [ ] Review the NPC observations that explicitly expose committed attack
  style, exact remaining hit ticks, whether prayer can still affect the hit,
  and prayer-deadline urgency.
- [ ] Decide whether this explicit information is appropriate for the intended
  abstract Fight Caves MDP or makes the environment materially easier than
  actual OSRS, where players infer attacks from animations and timing.
- [ ] Treat this as a task-definition and parity decision, not reward leakage:
  the observed attack has already been committed.

### 2. Native Action Masks and Policy Input

- [ ] Review the intended use of native Puffer action masks for preventing
  impossible movement and nonexistent NPC targets.
- [ ] Review why the model receives the full 319-value environment observation
  (`285` policy features plus `34` mask bits) even though comments describe the
  policy input as only the 285 policy features.
- [ ] Decide whether policy-visible legality bits are intentional and should be
  documented, or whether they should be removed from the neural-network input
  while remaining available for native action enforcement.
- [ ] Account for the extra collision and target-availability information those
  mask bits provide; classify it as an unintended convenience rather than
  catastrophic leakage.

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

### 5. Codebase Refactor and Maintainability Pass

- [ ] After reviewing the four areas above, refactor existing code and logic to
  make the codebase cleaner and more maintainable.
- [ ] Define the refactor scope during the review before editing, favoring
  consolidation, clearer ownership, removal of duplication and dead paths, and
  simpler authoritative data flow over new abstractions.
- [ ] Preserve behavior unless a reviewed and approved correction from the four
  areas above intentionally changes it; validate each affected contract and
  module boundary.

## Current Baseline: v4.5 `1nvvx5qu`

- The authoritative live config is `runescape-rl/config/fight_caves.ini`.
- It promotes the exact 750M-step configuration selected by sweep run
  `1nvvx5qu` with the current backend and 60-second HP regeneration mechanics.
- W&B run `8oivozuq` is the standalone seed-73 reproduction. Its full trainer,
  environment, architecture, and contract configuration matches `1nvvx5qu`,
  and all 124 environment metric series reproduced exactly. It retained the
  replayable checkpoint that sweep mode did not save.
- W&B run `88l9p7ie` (`v4.5_correct_movement`) is the preceding seed-73
  empirical comparison baseline using the prior trainer recipe.
- W&B run `z5vbs56z` is the immediately preceding seed-73 baseline before the
  stationary-attack-tick correction.
- W&B run `hevp6ehc` is the immediately preceding parity-backend comparison
  before the LOS/collision/pathing corrections. W&B run `l9o32hhz` remains the
  original `v4_simple_reward` reference.
- The source and synchronized Puffer INIs are byte-identical with SHA-256
  `31291c7ec281bda1e4a5eee911a8c47298662654c6670525c9f215f2dbfb54d6`.
- The original `v4_simple_reward` derivation, exact config, trajectory, and
  `l9o32hhz` metrics are recorded at the top of
  `runescape-rl/docs/run_history.md`. The later `hevp6ehc`, `z5vbs56z`, and
  `88l9p7ie`, `1nvvx5qu`, and `8oivozuq` comparisons have not yet been added
  there. The completed top-eight sweep analysis is recorded in
  `sweep_top8.md`.

## Immediate Next Steps: Validate v4 Before Stage 2

This section is the authoritative next work.

Planning document for all proposed sweep stages:
`sweep_history/v3_simple_reward_sweep.md`. Stage 1 is complete. Stage 2 combines
the planned value/optimizer and architecture/batch searches around the
post-fix `88l9p7ie` baseline. It must not start until the validation gate below
passes.

### 1. Close the New-Backend Baseline Gate

- [ ] Formally accept `88l9p7ie` as the frozen backend baseline for future
  trainer experiments.
- [ ] Select useful checkpoints from W&B baseline run `88l9p7ie`, including an
  early/transition policy, a strong late policy, and the final 1.499B policy.
- [ ] Replay each selected checkpoint in the eval viewer, preferably across
  multiple episodes rather than drawing conclusions from one cave.
- [ ] Validate learned behavior: movement and combat timing, targeting and
  target priority, prayer switching and conservation, attack activity,
  safespot/LOS use, wave progression, Yt-MejKot handling, Jad prayer, healer
  aggro, healer targeting, and healer/Jad interactions.
- [ ] Specifically inspect the behavior differences highlighted by the
  `88l9p7ie` versus `z5vbs56z` and `z5vbs56z` versus `hevp6ehc` comparisons:
  safespot positioning, movement/attack timing, running, idling, wrong-prayer
  hits, damage taken, and where deaths occur.
- [ ] Compare visible behavior against `88l9p7ie` metrics so apparent issues
  are supported or contradicted quantitatively.
- [ ] Record checkpoint paths, observed behavior, and any suspected mechanics
  defects before changing code or configuration.

### 2. Test the Environment for Defects

- [ ] Build a focused mechanics checklist covering natural HP regeneration,
  prayer drain and protection, player/NPC attack timing, damage calculation,
  movement speed, pathfinding, collision, LOS, safespots, NPC footprints,
  target/aggro persistence, wave composition, split NPCs, Yt-MejKot healing,
  Jad attacks, Yt-HurKot spawn/aggro/pathing/healing, deaths, resets, rewards,
  observations, and hard action masks.
- [ ] Exercise the checklist with targeted validation-module tests and focused
  playable-viewer scenarios. Keep diagnostic tooling outside `fc-core`.
- [ ] Compare uncertain mechanics against OSRS references and the local
  RuneScape reference repositories before labeling behavior defective.
- [ ] Verify the 100-tick natural HP regeneration correction remains active:
  no HP before tick 100 and exactly 1 HP on tick 100.
- [ ] Check for additional accidental simplifications or timing errors similar
  to the old 10-tick HP regeneration defect.
- [ ] Document each finding as confirmed correct, intentional simplification,
  confirmed defect, or unresolved. Do not fix behavior based only on visual
  suspicion.
- [ ] For every confirmed defect, add a failing guardrail first, implement the
  smallest correction, rerun the full validation suite, and assess whether the
  change invalidates `88l9p7ie` as the empirical baseline.

### 3. Stage 2 Go/No-Go Gate

- [ ] Confirm no known training-relevant mechanics defect remains unresolved.
- [ ] Confirm the viewer accurately represents backend actions well enough for
  behavioral evaluation.
- [ ] Confirm rewards, observations, action masks, loadout, seed, policy,
  backend build, live INI, and synchronized Puffer INI match the documented
  `v4_simple_reward` contract.
- [ ] If validation changes backend behavior, rerun `v4_simple_reward` before
  sweeping so Stage 2 has a valid post-fix baseline.
- [x] Configure the combined Stage 2 search around the post-fix v4.5 trainer
  recipe. Sweep only `vf_coef`, `vf_clip_coef`, `max_grad_norm`, `beta1`,
  `policy.hidden_size`, `policy.num_layers`, and `vec.total_agents`; keep the
  environment, rewards, observations, actions, loadout, seed, and all other
  selected Stage 1 trainer values fixed.
- [x] Set Stage 2 to 130 trials at 750M timesteps per trial. Do not mix
  environment fixes or reward changes into that sweep.

## Planned Trainer Experiment Sequence

### 4. Stage 2: Combined Value, Optimizer, Architecture, and Batch Sweep

- [x] Use `88l9p7ie` and its unchanged environment contract as the fixed
  center.
- [x] Use 130 trials with a fixed 750M-timestep budget.
- [x] Create
  `runescape-rl/config/experiments/fight_caves_v45_value_arch_batch_sweep_750m.ini`.
- [x] Add a fail-closed launcher preflight after the first attempted launch
  silently fell back to Puffer's generic 1,200-run sweep. Preserve but exclude
  the 125 runs in `v45_value_arch_batch_sweep_1p5b_130`; use a fresh
  `_corrected` group for the intended search.
- [x] Sweep only:
  - `vf_coef`
  - `vf_clip_coef`
  - `max_grad_norm`
  - `beta1`
  - `policy.hidden_size`
  - `policy.num_layers`
  - `vec.total_agents`
- [x] Keep the environment, rewards, observations, actions, loadout, seed,
  learning rate, entropy, discounting, rollout, minibatch, replay, priority,
  and V-trace values fixed.
- [ ] Rank results using Jad completion, wave-63 reach, conditional Jad
  conversion, stability, and value-learning behavior rather than shaped
  reward alone.

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
- [ ] Compare Jad completion, consistency, parameter count, VRAM, SPS, and
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

## Deferred Environment and Control Debt

These items are known but are not blockers for the Stage 2 trainer sweep.

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
- [ ] Update the root README's outdated SOTA and project-status sections.
- [ ] Add the README architecture diagram currently marked TODO.

## Documentation and Repository Tracking

- [ ] Keep this file updated as the single authoritative working list; leave
  historical experiment and implementation plans intact as evidence.
- [ ] Record the `hevp6ehc` parity-backend run, the `z5vbs56z`
  LOS/collision/pathing run, and their comparison in
  `runescape-rl/docs/run_history.md`.
- [ ] Decide whether the following currently ignored local documents should be
  force-added/tracked or deliberately remain local-only:
  - `obs.md`
  - `parity_fix.md`
  - `parity_fix_tests.md`
  - `parity_fix_config.md`
  - `hparam_sweep_07-16.md`
  - `SOTA_DIFF_AUDIT.md`
