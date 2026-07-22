# Local TODO

This file is intentionally local-only and ignored by git. Use it as a working list
for project follow-ups that should not become public repo documentation yet.

## Immediate Priority: Clean Baselines vs NPC-Heal Penalty

This section is the next work to execute. It supersedes older "immediate next"
wording farther down this file without deleting that historical context.

Goal: establish short- and long-budget baselines for the three strongest clean
hyperparameter recipes, then repeat the exact matrix with one reward change:
`shape_npc_heal_penalty = -0.01`. This separates ordinary training variance,
long-budget behavior, and the effect of the healing-loop correction.

Authoritative sweep analysis, exact hyperparameters, baseline config, run IDs,
and exploit classifications:
`sweep_history/v1.0_hparam_sweep.md`.

### Primary Recipes

Use the top three clean recipes rather than the literal raw top three:

1. `ge0h4chm` - raw rank 1, clean winner, cave progress `0.937295`.
2. `luiq0j2s` - raw rank 3, clean runner-up, cave progress `0.915094`.
3. `5mpum7x7` - raw rank 4, strongest clean alternate parameter region,
   cave progress `0.892173`.

Do not use raw rank-2 `fi6th21p` as a clean baseline. It had excellent cave
progress (`0.920874`) but a serious Yt-MejKot healing loop. Preserve it for the
later exploit-focused stress tests.

### Controls for Every Primary Run

- Use the exact six trainer hyperparameters recorded for that source W&B run.
- Keep top-level trainer seed `73` for direct paired comparisons.
- Keep backend/core commit, v6 observation contract, action contract, reward
  settings, 256x3 MinGRU policy, vector settings, SOTA TBow loadout, no-food,
  and no-prayer-potion setup fixed.
- Before the heal-fix phase, keep `shape_npc_heal_penalty = 0.0`.
- During the heal-fix phase, change only
  `shape_npc_heal_penalty: 0.0 -> -0.01`.
- Do not simultaneously change correct-prayer reward, progress timers, wave
  stall penalties, action masks, or any other hyperparameter/reward.
- Enable normal checkpoint saving for these direct runs so interesting policies
  can be replayed. Sweep trials did not preserve normal checkpoint series.
- Give every run a unique W&B group/tag identifying recipe, budget, and whether
  the heal penalty is enabled. Record every resulting W&B ID in
  `sweep_history/v1.0_hparam_sweep.md` before moving to the next phase.
- Record the exact git commit, config hash, backend build stamp, seed, and
  manifest for reproducibility.

Using seed 73 makes this a controlled A/B comparison and should reproduce the
750M sweep policies if the effective binary and config are unchanged. It does
not provide independent-seed confidence. Seeds 101 and 202 can be added after
the paired matrix identifies which recipe/reward combination is worth deeper
validation.

### Phase A: Pre-Fix Baselines (`shape_npc_heal_penalty = 0.0`)

Run and analyze all six before changing the reward configuration.

750M reproduction runs:

- [ ] `ge0h4chm` recipe at 750M steps, seed 73.
- [ ] `luiq0j2s` recipe at 750M steps, seed 73.
- [ ] `5mpum7x7` recipe at 750M steps, seed 73.

2.5B long-budget runs, changing only `total_timesteps`:

- [ ] `ge0h4chm` recipe at 2.5B steps, seed 73.
- [ ] `luiq0j2s` recipe at 2.5B steps, seed 73.
- [ ] `5mpum7x7` recipe at 2.5B steps, seed 73.

Phase A exit gate:

- [ ] Verify each 750M reproduction against its source sweep run.
- [ ] Analyze whether each recipe improves, plateaus, regresses, or develops a
  healing/stall failure between 750M and 2.5B.
- [ ] Record all six W&B IDs, configs, checkpoint locations, and full metric
  comparisons in the sweep-history document.
- [ ] Do not enable the heal penalty until all six baselines are complete and
  documented.

### Phase B: Paired Heal-Penalty Runs (`shape_npc_heal_penalty = -0.01`)

After Phase A, enable only the existing per-successful-heal penalty and repeat
the same six jobs.

750M paired runs:

- [ ] `ge0h4chm` recipe at 750M steps, seed 73, heal penalty `-0.01`.
- [ ] `luiq0j2s` recipe at 750M steps, seed 73, heal penalty `-0.01`.
- [ ] `5mpum7x7` recipe at 750M steps, seed 73, heal penalty `-0.01`.

2.5B paired runs, changing only `total_timesteps`:

- [ ] `ge0h4chm` recipe at 2.5B steps, seed 73, heal penalty `-0.01`.
- [ ] `luiq0j2s` recipe at 2.5B steps, seed 73, heal penalty `-0.01`.
- [ ] `5mpum7x7` recipe at 2.5B steps, seed 73, heal penalty `-0.01`.

Compare each post-fix run only to its same-recipe, same-budget pre-fix pair
first. Then compare winners across recipes. Required outcome metrics:

- Cave progress and average wave.
- Wave-63 reach count/rate and cave completion count/rate.
- Learning curve, peak, final value, and late-training stability.
- NPC healing total and healing/gross-damage ratio.
- Yt-MejKot target share, episode duration, longest-wave duration, and the wave
  containing the longest delay.
- Progress, correct-prayer, heal, no-progress, no-attack, tick, prayer-loss,
  damage-taken, death, and total reward channels.
- Prayer accuracy/conservation, attack activity, invalid actions, Jad damage,
  healer targeting, SPS, runtime, and optimization stability.

Phase B success criteria:

- [ ] Healing stays near normal clean-run levels, approximately below 5-10% of
  gross damage.
- [ ] Multi-thousand-tick Yt-MejKot loops do not appear.
- [ ] Cave progress, wave-63 reach, and completion do not materially regress
  versus the matching pre-fix run.
- [ ] Correct-prayer behavior remains useful, and progress remains the dominant
  positive reward channel.

### Phase C: Exploit-Focused Stress Tests

After the clean paired matrix is complete, rerun selected high-performing
healing-loop recipes with `shape_npc_heal_penalty = -0.01`. These runs answer a
different question: whether the fix removes an already demonstrated exploit,
not merely whether clean policies tolerate the new reward.

Initial priority candidates:

1. `fi6th21p` - raw rank 2, progress `0.920874`, healing `47.85%`, longest
   wave 12,532 ticks. This is the highest-performing compromised policy.
2. `vxpabmtb` - raw rank 7, progress `0.867107`, healing `19.16%`, longest
   wave 2,706 ticks. This is the other compromised top-10 policy.
3. `c5ygx7jk` - progress `0.782454`, healing `60.99%`, 24.54% wave-63 reach,
   and 0.3065% completion. This is the strongest clear severe-loop stress case
   that still beat the original `gp2dfafs` baseline.

Additional candidates if more coverage is useful:
`j4rm3ojl`, `bth7nkal`, `1lpyx352`, and `nc149won`. Each exceeded baseline
cave progress while recording at least 10% healing.

- [ ] Start each selected exploit recipe at 750M with its original seed-73
  hyperparameters and only the `-0.01` heal-penalty change.
- [ ] Compare directly against its original sweep metrics.
- [ ] Promote only informative, non-looping results to 2.5B confirmation.
- [ ] If the loop survives, do not stack fixes immediately. First determine
  whether the direct penalty fires at the expected count and magnitude; then
  test the next isolated option documented in the sweep analysis.

## Current Focus

- Completed: 48-run core-hparam Protein sweep and six-run trainer-seed
  confirmation. Full configs, run IDs, metrics, and analysis are in
  `runescape-rl/docs/run_history.md`.
- Selected baseline: `v1.0`, derived from W&B run `b5m07qqr`. The canonical
  live INI contains its exact trainer values with the validated no-supplies
  v38 backend/task contract.
- Immediate next experiment: run the 96-trial, 750M-step Protein sweep in
  `runescape-rl/config/experiments/fight_caves_v1_mechanics_hparam_sweep_750m.ini`.
  It holds the new backend, v6 observations, rewards, 256x3 policy, horizon,
  and minibatch fixed while retuning six PPO/optimizer parameters. W&B group:
  `fc_v1_mechanics_hparam_sweep_750m`.
- After the sweep, compare all runs against new-mechanics reference `ocunodgx`,
  then confirm the best few recipes with independent trainer seeds 101 and 202.
- Then sweep policy size: hidden sizes `128, 256, 384, 512` crossed with layer
  counts `2, 3, 4`, ideally two seeds each. Compare progress, Jad reach and
  completion, stability, SPS, runtime, and parameter count.
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
