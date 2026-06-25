# Fight Caves RL Improvement Plan

This tracks the implementation plan from the reward/observation/training review.
Validation and reproducibility tooling should live outside `fc-core`; the core
simulator should remain normal game logic used by training and the viewer.

## Phase Status

| Phase | Status | Purpose |
| --- | --- | --- |
| Phase 1: Reproducibility | Complete | Write run manifests, make live supplies explicit, archive old no-consumables config, and fix docs drift. |
| Phase 2: Guardrail Tests | Complete | Add focused tests for high-impact correctness issues before changing behavior. |
| Phase 3: Environment Fixes | Complete | Fix target identity, RNG seeding, metrics aggregation, and healer spawn validation. |
| Phase 4: Reward Bug Cleanup | Complete | Remove fake reward incentives such as zero-damage hit reward and direct shallow safespot reward. |
| Phase 5: Invalid Action Diagnostics | Complete | Log invalid action classes clearly. |
| Phase 6: Corrected Baseline | Complete | Rerun the current best config after correctness fixes. |
| Phase 7: Reward Simplification | In Progress | Prune reward to core progress/survival signals. |
| Phase 8: Curriculum | Pending | Add late-wave/Jad-focused curriculum experiments. |
| Phase 9: Ablations | Pending | Test one change at a time after correctness is fixed. |

## Current Stop Point

As of 2026-06-25, Phase 7 reward simplification is in progress.

Completed so far:

- Phase 1 reproducibility is complete.
- Phase 2 guardrail tests are complete.
- Phase 3 environment fixes are complete.
- Phase 4 reward bug cleanup is complete.
- Phase 5 invalid action diagnostics are complete.
- Phase 6 corrected baseline training is complete.
- Phase 7 trial 1 completed, but the no-kiting-reward variant should not be
  adopted.
- The validation/test tooling is kept outside `fc-core`.
- The full fresh CTest run from `runescape-rl/build-phase2` passes.
- No expected-failing guardrails remain.

Current work: continue Phase 7 reward simplification with one-change reward
trials.

Recommended Phase 7 order:

1. Do not tune against the old historical SOTA numbers directly; Phase 6 shows
   the corrected environment performs differently.
2. Use the 1B-step development config for reward/testing iteration:
   `runescape-rl/config/experiments/fight_caves_phase7_dev_1b.ini`.
   This keeps each test run short enough for development while still long
   enough to show Jad-reach/Jad-kill signal.
3. Reserve the live 3B config (`runescape-rl/config/fight_caves.ini`) for
   final confirmation after a shorter run looks promising.
4. Start by pruning obviously noisy or redundant reward terms while preserving
   core progress/survival outcomes: damage dealt, NPC kills, wave clear, Jad
   kill, player death, and basic resource penalties.
5. Use the Phase 6 invalid-action diagnostics to decide whether invalid-action
   penalties are helping or just measuring unsupported/masked behavior.
6. Run one reward simplification at a time against the Phase 6 corrected
   baseline, not multiple reward changes in one run.

Phase 7 trial 1:

- Config: `runescape-rl/config/experiments/fight_caves_phase7_no_kiting_1b.ini`
- Change: set `shape_kiting_reward = 0.0`.
- Purpose: remove the largest direct non-core behavior-shaping reward from the
  Phase 6 baseline while leaving progress, damage/survival, prayer/resource,
  invalid-action, and tick-cost signals unchanged.
- Comparison target: Phase 6 corrected baseline plus the 1B dev baseline config,
  not historical pre-fix SOTA.

Default Phase 7 dev-run command:

```bash
CONFIG_PATH=runescape-rl/config/experiments/fight_caves_phase7_dev_1b.ini \
WANDB_TAG=phase7_dev_<change_name> \
./runescape-rl/train.sh train --wandb-group phase7_reward_dev
```

Phase 7 trial 1 command:

```bash
CONFIG_PATH=runescape-rl/config/experiments/fight_caves_phase7_no_kiting_1b.ini \
WANDB_TAG=phase7_no_kiting_1b \
./runescape-rl/train.sh train --wandb-group phase7_reward_dev
```

Phase 7 trial 1 result:

- Completed on 2026-06-25.
- W&B run: `d4uu9i58` / `amber-pyramid-615`
- W&B group: `phase7_reward_dev`
- W&B tag: `phase7_no_kiting_1b`
- W&B URL:
  `https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/d4uu9i58`
- Manifest:
  `pufferlib_4/logs/fight_caves/manifests/20260625T034655Z-train-459333.json`
- Local metrics JSON: `pufferlib_4/logs/fight_caves/d4uu9i58.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/d4uu9i58/`
- Final training-step checkpoint:
  `pufferlib_4/checkpoints/fight_caves/d4uu9i58/0000000999292928.bin`
- Final/eval checkpoint artifact path:
  `pufferlib_4/checkpoints/fight_caves/d4uu9i58/0000001049624576.bin`

Config/run contract:

- Experiment config:
  `runescape-rl/config/experiments/fight_caves_phase7_no_kiting_1b.ini`
- Change tested: `shape_kiting_reward = 0.0`
- Total timesteps: `1,000,000,000`
- Effective final logged steps: `999,292,928`
- Reward version: `fight_caves_v38_phase7_no_kiting_reward`

Final eval summary from W&B/local JSON:

- `jad_kill_rate`: `0.0`
- `reached_wave_63`: `0.0`
- `wave_reached`: `54.796986`
- `npcs_slayed`: `228.318420`
- `attack_when_ready_rate`: `0.932380`
- `correct_prayer`: `1228.262695`
- `wrong_prayer_hits`: `26.028284`
- `no_prayer_hits`: `48.420322`
- `food_eaten`: `13.300385`
- `pots_used`: `31.999113`
- `pots_wasted`: `31.997734`
- `rwd_kiting_total`: `0.0`
- `rwd_wave_clear_total`: `22126.009766`
- `rwd_jad_kill_total`: `0.0`

Interpretation:

- Do not adopt the no-kiting-reward variant. Removing `shape_kiting_reward`
  outright is too aggressive at the 1B development-run budget.
- The run learned into the mid/high waves, but did not reach wave 63 or kill
  Jad.
- A comparable Phase 6 checkpoint around `1.126B` steps had
  `reached_wave_63=0.697452`, `jad_kill_rate=0.408166`, and
  `wave_reached=60.667226`, so Phase 7 trial 1 is materially worse.
- Keep the current direct kiting reward in the live 3B config for now.

Next Phase 7 trial:

- Test a smaller one-change reward edit. Recommended order:
  reduce `shape_kiting_reward` from `2.2` to `1.0` instead of zeroing it, or
  leave kiting intact and prune one smaller shaping term such as
  `shape_wasted_attack_penalty`, `shape_wave_stall_base_penalty`, or a
  prayer-correctness reward in a separate 1B run.
- Continue using the 1B experiment config pattern and compare every trial
  against the Phase 6 corrected baseline.

## Phase 1 Completion Notes

Completed on 2026-06-24.

- Added `runescape-rl/tools/validation/run_manifest.py`.
- Wired `runescape-rl/train.sh` to emit a manifest before training starts.
- Manifest records git state, config hashes, effective PufferLib config,
  loadout, backend stamp, resolved supplies, seeds, trainer contract, reward
  clipping, and action-mask behavior.
- Made live supplies explicit in `runescape-rl/config/fight_caves.ini`:
  `initial_sharks = 20` and `initial_prayer_doses = 32`.
- Archived the v36 no-consumables diagnostic config at
  `runescape-rl/config/archive/fight_caves_v36_no_consumables.ini`.
- Updated README config docs to match the live full-supplies baseline.

## Phase 2 Guardrail Scope

Completed on 2026-06-24.

Phase 2 added tests under `fc-validation` and `tools/validation`, not inside
`fc-core`.

Guardrails added:

- Stable attack target identity from observed slot to acted target.
- Distinct per-env reset seeds in the Puffer training adapter.
- Race-free analytics aggregation shape.
- Jad healer spawn validity.
- Zero-damage player hits should not produce damage reward.
- Italy-rock-style LOS safespot behavior remains valid.

Known broken guardrails are tracked as expected-failing CTest entries until the
corresponding Phase 3 or Phase 4 fix lands.

Current known-issue outputs:

- None. The Phase 2 known-issue guardrails have been fixed or converted to
  passing preservation/Phase 3/Phase 4 guardrails.

Passing preservation guardrail:

- `phase2_safespot_los_preserved`: blocked LOS prevents both player and Jad
  attacks through the safespot.

## Phase 3 Completion Notes

Completed on 2026-06-25.

- Added one shared visible-NPC slot ordering helper for observation, action
  masks, and attack target resolution.
- Bound attack-slot actions to the NPC identity from the pre-action observation,
  so movement in the same tick cannot retarget the action to a different NPC.
- Changed Puffer reset seeding to derive each episode seed from the env RNG id
  plus the per-env episode counter.
- Moved training analytics out of shared global counters and into the per-env
  PufferLib `Log` aggregation path.
- Replaced the unsafe global `most_npcs_slayed` export with race-free
  per-episode average `npcs_slayed`. True all-time max metrics need a separate
  Puffer aggregation design if we want them back without globals.
- Validated Jad healer spawns against walkable footprints and active
  player/NPC overlap before spawning.
- Converted the fixed Phase 3 guardrails to normal passing CTest entries:
  `phase3_target_identity_stable`, `phase3_rng_seed_diversity`,
  `phase3_analytics_no_globals`, and `phase3_healer_spawn_validity`.

Verification:

- `cmake -S runescape-rl -B runescape-rl/build-phase2 -DCMAKE_BUILD_TYPE=Release`
- `cmake --build runescape-rl/build-phase2 --target phase2_guardrails_core phase2_guardrails_training -j2`
- `ctest --test-dir runescape-rl/build-phase2 -L phase3 --output-on-failure`
- `cmake --build runescape-rl/build-phase2 -j2`
- `ctest --test-dir runescape-rl/build-phase2 --output-on-failure`
- Direct Puffer binding compile of `fc-training/binding.c`
- `git diff --check`

## Phase 4 Completion Notes

Completed on 2026-06-25.

- Changed zero-damage player hits so they still resolve mechanically, including
  healer distraction, but do not increment the `damage_dealt` reward hit count.
- Disabled direct shallow safespot reward in reward computation. LOS
  safespotting remains valid through combat/pathing mechanics; it just no
  longer receives a separate bonus.
- Set live `shape_safespot_attack_reward = 0.0` in both live configs.
- Updated live `reward_version` to
  `fight_caves_v38_phase4_reward_cleanup`.
- Converted the zero-damage known issue to passing guardrail
  `phase4_zero_damage_reward`.
- Added `phase4_safespot_reward_disabled` and
  `phase4_live_safespot_reward_zero`.

Verification:

- `cmake -S runescape-rl -B runescape-rl/build-phase2 -DCMAKE_BUILD_TYPE=Release`
- `cmake --build runescape-rl/build-phase2 --target phase2_guardrails_core phase2_guardrails_training -j2`
- `ctest --test-dir runescape-rl/build-phase2 -L phase4 --output-on-failure`
- `cmake --build runescape-rl/build-phase2 -j2`
- `ctest --test-dir runescape-rl/build-phase2 --output-on-failure`
- Direct Puffer binding compile of `fc-training/binding.c`
- Direct run-manifest smoke confirmed reward version and safespot value.
- `bash -n runescape-rl/train.sh`
- `git diff --check`

## Phase 5 Completion Notes

Completed on 2026-06-25.

- Added explicit invalid-action classes for RL-facing heads 0-4:
  movement, attack, prayer, eat, and drink.
- Added `fc_action_invalid_classes(...)` so validation/logging can classify
  invalid actions without duplicating mask logic.
- Kept heads 5-6 path-target actions excluded from invalid-action diagnostics,
  matching the current policy mask and reward behavior.
- Preserved `invalid_action_this_tick` as the aggregate reward feature; Phase 5
  only adds diagnostics and does not change action semantics, masking, or reward
  weighting.
- Added per-tick class flags and per-episode class counters on `FcState`.
- Exported Puffer log keys:
  `invalid_move`, `invalid_attack`, `invalid_prayer`, `invalid_eat`, and
  `invalid_drink`.
- Added passing guardrail `phase5_invalid_action_classes`, covering each class,
  a valid nonzero action bundle, and path-target exclusion.

Verification:

- `cmake --build runescape-rl/build-phase2 --target phase2_guardrails_core phase2_guardrails_training -j2`
- `ctest --test-dir runescape-rl/build-phase2 -L phase5 --output-on-failure`
- `ctest --test-dir runescape-rl/build-phase2 --output-on-failure`
- `cmake --build runescape-rl/build-phase2 -j2`
- Direct Puffer binding compile of `fc-training/binding.c`
- `git diff --check`

## Phase 6 Completion Notes

Completed on 2026-06-25.

Run:

- W&B run: `sqho1beq` / `young-yogurt-614`
- W&B group: `phase6_corrected_baseline_v38`
- W&B tag: `phase6_corrected_baseline_v38_20260625T015206Z`
- W&B URL:
  `https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/sqho1beq`
- Manifest:
  `pufferlib_4/logs/fight_caves/manifests/phase6_corrected_baseline_20260625T015206Z.json`
- Local run log:
  `pufferlib_4/logs/fight_caves/phase6/phase6_corrected_baseline_20260625T015206Z.log`
- Local metrics JSON:
  `pufferlib_4/logs/fight_caves/sqho1beq.json`
- Checkpoints:
  `pufferlib_4/checkpoints/fight_caves/sqho1beq/`
- Final/eval checkpoint artifact path:
  `pufferlib_4/checkpoints/fight_caves/sqho1beq/0000003041918976.bin`
- Final training-step checkpoint:
  `pufferlib_4/checkpoints/fight_caves/sqho1beq/0000002999975936.bin`

Config/run contract:

- Live config: `runescape-rl/config/fight_caves.ini`
- Synced config hash matched source hash in the manifest.
- Active loadout: `FC_LOADOUT_SOTA_TBOW`
- Total timesteps: `3,000,000,000`
- Effective final logged steps: `2,999,975,936`
- Runtime: about `47m 01s`
- Reward version: `fight_caves_v38_phase4_reward_cleanup`
- Observation/action contract unchanged:
  `fight_caves_puffer_policy_obs_v1_mask_heads_0_4` and
  `fight_caves_multidiscrete_5_head_v1`

Final eval summary from W&B/local JSON:

- `jad_kill_rate`: `0.421850`
- `reached_wave_63`: `0.976376`
- `wave_reached`: `62.843601`
- `npcs_slayed`: `281.480255`
- `attack_when_ready_rate`: `0.986408`
- `correct_prayer`: `2378.002686`
- `wrong_prayer_hits`: `18.215212`
- `no_prayer_hits`: `20.799242`
- `food_eaten`: `2.874202`
- `pots_used`: `32.0`
- `pots_wasted`: `32.0`

Invalid-action diagnostics at final eval:

- `invalid_move`: `15.771332`
- `invalid_attack`: `619.634399`
- `invalid_prayer`: `0.0`
- `invalid_eat`: `654.930298`
- `invalid_drink`: `284.901703`

Reward-channel sanity:

- `rwd_safespot_attack_total`: `0.0`
- `rwd_safespot_attack_fires`: `0.0`
- Direct safespot reward stayed disabled through the full run.

Interpretation:

- The corrected baseline learns to reach Jad reliably, but final Jad kill
  stability is materially below the historical live-config note
  (`peak/final 0.886` from the old top-pick comment).
- Downsampled local metrics peaked at `jad_kill_rate=0.557344` around
  `1.876B` steps and finished at `0.421850`.
- Phase 7 should treat this run as the corrected baseline, not as a recovered
  SOTA. Reward simplification should aim to reduce noisy behavior and improve
  final stability, especially around Jad execution and resource usage.

Verification:

- Short 1,048,576-step `--no-wandb` smoke completed before the full run.
- Full 3B W&B run completed and synced.
- Manifest, local metrics JSON, W&B summary, and checkpoints were written.
