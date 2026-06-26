# Local TODO

This file is intentionally local-only and ignored by git. Use it as a working list
for project follow-ups that should not become public repo documentation yet.

## Current Focus

- Immediate next dev tracker: `fc_revamp.md`
- Current status on 2026-06-26: backend/config phases are complete enough to
  pause the old phase plan. The next work is the Fight Caves revamp experiments
  in `fc_revamp.md`, implemented one by one so each training result is
  interpretable.
- First planned revamp target: add explicit NPC type one-hot observations per
  visible NPC slot. After that, test reward cleanup by replacing per-hit damage
  reward with NPC kill/progress rewards and rescaling around the `-1, 1` clamp.
- Later revamp targets: expose prayer decision deadline, hard-apply native
  Puffer action masks, then review target persistence and auto-pathing control.
- Previous work tracker: `runescape-rl/docs/fight_caves_improvement_plan.md`
- Current status on 2026-06-25: Phase 7 simplified no-supplies retrain
  completed as W&B run `rw70szhc`; it is mechanically clean but should not be
  adopted as the new reward baseline.
- Done: run manifest/config clarity, full-supplies config cleanup, archived v36
  no-consumables config, Phase 2 guardrail tests outside `fc-core`, and Phase 3
  environment fixes for target identity, reset seed diversity, analytics
  aggregation, Jad healer spawn validity, and Phase 4 reward cleanup for
  zero-damage hit reward plus direct safespot reward. Phase 5 added invalid
  action class diagnostics and Puffer log keys. Phase 6 reran the corrected
  baseline as W&B run `sqho1beq`; it reliably reaches Jad but finished below
  old SOTA stability (`jad_kill_rate=0.421850`, `reached_wave_63=0.976376`).
- Phase 7 trial 1 removed direct kiting reward for a 1B run
  (`d4uu9i58` / `amber-pyramid-615`) and was negative:
  `reached_wave_63=0`, `jad_kill_rate=0`, `wave_reached=54.797`.
- Phase 7 simplified no-supplies candidate is implemented:
  `runescape-rl/config/experiments/fight_caves_phase7_simplified_no_supplies_1b.ini`.
  It removes consumable waste, wrong-prayer, melee-pressure, wasted-attack,
  kiting, safespot, and resource-threat-window shaping; sets unnecessary-prayer
  and wave-stall shaping to zero; keeps `shape_jad_heal_penalty=-0.3`; and
  exposes only move/attack/prayer to the Puffer policy.
- Result: `rw70szhc` finished at `wave_reached=27.592`,
  `reached_wave_63=0`, `jad_kill_rate=0`, versus benchmark `2386cesn` at
  `wave_reached=61.332`, `reached_wave_63=0.839`,
  `jad_kill_rate=0.126`. Food/pots stayed zero and the Puffer policy used the
  intended 3-head move/attack/prayer action space.
- Immediate next: use `fc_revamp.md` instead of continuing ad hoc Phase 7
  tweaks. Keep the no-supplies action contract, but first test structural fixes:
  NPC identity observations, kill/progress rewards, and cleaner reward scaling.
  The full-supplies benchmark `27x0wmy4` is historical because future runs are
  intended to be foodless and prayer-potionless.

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
  - Start with explicit NPC type one-hot observations.
  - Then replace per-hit damage reward with NPC kill/progress rewards.
  - Then restructure reward weights around the `-1, 1` clamp.
  - Then add prayer decision deadline observations.
  - Then hard-apply action masks and remove invalid-action reward clutter once
    validation proves masks are correct.
  - Finally, review target persistence and auto-pathing control.

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
