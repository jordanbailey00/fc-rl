# Local TODO

This file is intentionally local-only and ignored by git. Use it as a working list
for project follow-ups that should not become public repo documentation yet.

## Current Focus

- Completed: 48-run core-hparam Protein sweep and six-run trainer-seed
  confirmation. Full configs, run IDs, metrics, and analysis are in
  `runescape-rl/docs/run_history.md`.
- Selected baseline: `v1.0`, derived from W&B run `b5m07qqr`. The canonical
  live INI contains its exact trainer values with the validated no-supplies
  v38 backend/task contract.
- Immediate next experiment: run v1.0 for 1.5B steps with native trainer seeds
  73, 101, and 202 before treating its Jad conversion as robust.
- Then sweep policy size: hidden sizes `128, 256, 384, 512` crossed with layer
  counts `2, 3, 4`, ideally two seeds each. Compare progress, Jad reach and
  completion, stability, SPS, runtime, and parameter count.
- After architecture selection, retune learning rate and entropy for the new
  network. Separately extend ranges for core hparams that favored a prior sweep
  boundary, especially GAE lambda.
- Behavior still needing refinement: prayer conservation, Yt-MejKot healing
  loops, and Jad/healer conversion. Hard action masks and target persistence /
  auto-path control remain backlog because invalid actions and target handling
  are not current bottlenecks.
- Historical implementation plans remain in `fc_revamp.md` and
  `runescape-rl/docs/fight_caves_improvement_plan.md`.

## Backend / Training Correctness

- Compare the current backend core against the SOTA run baseline and identify every
  training-impacting diff.
  - Scope only `fc-core`, `fc-training`, config/loadout/stat initialization, action
    masking, reward/obs generation, reset logic, wave/NPC/combat mechanics, and any
    PufferLib integration that changes training behavior.
  - Exclude viewer-only diffs, UI diffs, render assets, screenshots, and frontend-only
    code.
  - For each backend diff, decide whether it should be kept, validated against OSRS,
    fixed, or rolled back.

- Using the simplified no-supplies config, run controlled training for each
  loadout/equipment/stat setup and analyze performance by setup.
  - Next planned sweep: after the simplified Tbow no-supplies retrain, run each
    current backend loadout with the same no-food/no-prayer-potion contract.
  - Keep all other training settings fixed so differences are attributable to loadout,
    stats, and resource availability.
  - Compare Jad reach rate, Jad kill rate, final/peak stability, resource usage,
    prayer accuracy, death causes, and checkpoint quality.

- Clean up bugs, defects, or simplifications that do not map to OSRS functionality.
  - Prioritize mechanics that affect training behavior: combat timing, pathing,
    collision, line of sight, prayer resolution, healer behavior, NPC targeting,
    inventory/resource handling, and action masks.
  - If behavior is intentionally simplified, document the reason and make sure the
    simplification is not accidental or misleading.

## Reward / Observation Iteration

- Follow `fc_revamp.md` as the immediate reward/observation implementation plan.
  - NPC type observations and prayer-decision deadline observations are already
    implemented.
  - Current reward trial is raw net required-work progress with
    `w_progress=0.001`, stronger prayer conservation, and wave-scaled
    no-attack pressure. Latest run: `mzqf7iml`.
  - Next decision: inspect/replay `mzqf7iml` prayer behavior and add
    prayer-action diagnostics if needed.
  - Backlog, not current priority: hard-apply action masks, then review target
    persistence and auto-pathing control.

- Longer-running cleanup: reduce and prune the reward config, then run another
  sweep.
  - Remove reward terms that are redundant, dead, unreachable, or mostly acting as
    optimizer noise.
  - Prefer simpler reward terms that line up with OSRS-relevant outcomes and avoid
    over-shaping behavior that should emerge from environment dynamics.
  - After pruning, run a fresh sweep using the simplified reward surface and compare
    against the current SOTA config.

## Repo / PufferLib PR Readiness

- Clean up the entire repo so it is ready to PR into PufferLib on GitHub.
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
