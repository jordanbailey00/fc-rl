# Central Project TODO

This file is the authoritative list of unfinished FC-RL work. Items under
**Investigate** require evidence before we decide whether implementation is
needed. Items under **Deferred** are accepted future work but are not current
priorities. Completed work and historical decisions are preserved in
[`fc_cleanup_and_parity_history.md`](runescape-rl/docs/archive/fc_cleanup_and_parity_history.md).

## Investigate

### Movement, routing, and collision parity

- [x] Audit the remaining movement, pathfinding, and routing discrepancies
  before proposing any implementation:
  - compare NPC obstacle navigation and body-blocking behavior with the OSRS
    references and determine whether any remaining approximations create
    invalid safespots or escape routes;
  - capture a deterministic reproduction of the intermittent non-shortest
    player click route and identify whether it originates in core routing,
    action translation, or viewer presentation;
  - document what is actually wrong, how often it occurs, and its gameplay
    impact;
  - preserve regression coverage for terrain, walls, diagonal clipping, arena
    bounds, actor footprints, occupancy, and the already-correct routing
    behavior while testing any eventual change.

  **Audit finding (2026-08-27): no reproducible defect found.** The core's NPC
  pursuit uses the same footprint-aware naive destination and validated
  diagonal/horizontal/vertical step order as the RSMod and Void references.
  Player click movement uses breadth-first search and the existing Fight Caves
  regression proves the audited wall case is both directionally legal and the
  shortest 18-step route. The viewer previews that same core route rather than
  calculating a separate path. No deterministic reproduction or code path was
  found for the previously reported intermittent non-shortest click route, so
  there is no evidenced implementation change to make. Relevant FC-RL code:
  `runescape-rl/fc-core/src/fc_pathfinding.c`,
  `runescape-rl/fc-core/src/fc_npc.c`,
  `runescape-rl/fc-viewer/src/fc_click_feedback.c`, and
  `runescape-rl/fc-validation/tests/phase2_guardrails_core.c`. Reference code:
  `rsmod/api/route/src/main/kotlin/org/rsmod/api/route/StepFactory.kt`,
  `rsmod/api/game-process/src/main/kotlin/org/rsmod/api/game/process/npc/NpcMovementProcessor.kt`,
  and
  `void_rsps/engine/src/main/kotlin/world/gregs/voidps/engine/entity/character/mode/move/Movement.kt`.

### Targeting behavior

- [x] Investigate whether target acquisition, persistence, disengagement,
  retargeting, or automatic approach currently behaves incorrectly. Use
  focused diagnostics for target acquisition/drop/switches, retarget latency,
  stale targets, route cancellation, policy overrides, approach success, and
  timeouts to distinguish a policy decision from an action-translation or core
  control problem. Do not change targeting behavior without a reproducible
  failure and demonstrated gameplay impact.

  **Audit finding (2026-08-27): no reproducible defect found.** Target actions
  resolve through the same visible-NPC ordering used by the observation,
  remain bound to the selected NPC's core array identity, persist while that
  NPC is alive, rebuild an approach route when the target footprint moves,
  switch immediately on a new valid target command, and clear on target death
  or an explicit movement-only command. Focused existing checks for slot
  identity, stale-target firing, target/movement conflicts, approach routing,
  route cancellation, and next-tick movement all pass. These behaviors are
  owned by `runescape-rl/fc-core/src/fc_tick.c` and
  `runescape-rl/fc-core/src/fc_observation.c`; playable input is translated in
  `runescape-rl/fc-viewer/src/viewer.c`. No FC-RL or vendored-code issue was
  identified, so there is no evidenced targeting change to make.

## Deferred

These mechanics are accepted future parity work, but they are intentionally
deprioritized until the current investigations and higher-priority encounter
work are complete.

- [ ] Select valid spawn tiles within configured spawn regions instead of
  always resolving each region to its fixed center tile, while preserving the
  authoritative Fight Caves rotations.
- [ ] Implement the large Tz-Kek melee-contact recoil behavior for future
  melee-capable loadouts and policies.
- [ ] Establish stronger primary-source evidence for Yt-MejKot's exact healing
  probability before changing its healing behavior.
- [ ] Rerun the Fight Caves hyperparameter sweep on the corrected Ket-Zek and
  Jad Magic-accuracy mechanics. The previous sweep remains useful historical
  evidence, but its optimum was selected on the easier pre-correction combat
  model; optimize both Jad completion and late-run consistency before
  promoting a new trainer recipe.

## PufferLib PR readiness

This section remains last and should be undertaken after the planned
environment, investigation, and validation work is finished.

- [ ] Align the environment's file layout, build scripts, config style, docs,
  tests, and asset handling with PufferLib conventions.
- [ ] Package the required core and viewer assets into deliberate installable
  or downloadable bundles instead of relying on loose development files.
- [ ] Provide a documented setup path that installs required Fight Caves
  assets while keeping raw cache inputs external unless deliberately shipped.
- [ ] Verify build, training, evaluation, and rendering from a clean clone.
- [ ] Keep all FC-RL integration in FC-RL modules and adapters; do not modify
  vendored PufferLib for environment-specific behavior.
