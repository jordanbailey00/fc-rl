# Sweep History

This is a chronological evidence archive. Terms such as "current backend" and
"SOTA" inside older sections describe the repository at the time of that
sweep. The live baseline is documented in the root README and the top of
`run_history.md`.

## v4.5 value/architecture/agent-count sweep (2026-08-20, completed — 130 runs)

The corrected Protein sweep ran 130 trials for 750 million agent steps,
maximizing final `jad_kill_rate`. It held the Fight Caves environment, loadout,
seed, observations, actions, rewards, and fixed trainer settings constant and
swept only:

- `train.vf_coef`
- `train.vf_clip_coef`
- `train.max_grad_norm`
- `train.beta1`
- `policy.hidden_size`
- `policy.num_layers`
- `vec.total_agents`

The finalists converged strongly on three recurrent layers, 512 hidden units,
and 4,096 agents. `1nvvx5qu` was selected as the balanced recipe because it
finished at 94.79% Jad completion with the strongest final-quarter consistency
floor. Standalone run `8oivozuq` reproduced its complete final environment
summary exactly and retained a replayable checkpoint.

The full eight-run configuration, behavior, learning-curve, and W&B-link
analysis is preserved in [`../../sweep_top8.md`](../../sweep_top8.md).

## v38 Loadout Sweep (2026-06-22, completed - 9 runs, current backend)

### Overview

Controlled one-variable sweep across every compiled Fight Caves loadout. The
goal was to answer whether the current backend plus the restored v35.1/SOTA
config could train each equipment/stat profile, and whether any loadout other
than the original SOTA TBow profile should become the next training target.

This was not a Protein hparam sweep. It was a sequential fixed-config sweep:
the config, PPO recipe, reward weights, policy, vectorization, supplies, seed,
and backend were held constant. Only `FC_ACTIVE_LOADOUT` changed between runs.

### Sweep Command Shape

The command used this structure:

```bash
cd /home/joe/projects/runescape-rl/fight-caves-rl

GROUP="loadout-sweep-1.5b-current-backend"

for entry in \
  "FC_LOADOUT_BLACK_DHIDE_RCB:black_dhide_rcb" \
  "FC_LOADOUT_SOTA_TBOW:sota_tbow" \
  "FC_LOADOUT_LOW_DEF_RCB:low_def_rcb" \
  "FC_LOADOUT_RCB_PURE:rcb_pure" \
  "FC_LOADOUT_MSBI_PURE:msbi_pure" \
  "FC_LOADOUT_BLOWPIPE_PURE:blowpipe_pure" \
  "FC_LOADOUT_ACB_ARMADYL:acb_armadyl" \
  "FC_LOADOUT_BOWFA_CRYSTAL:bowfa_crystal" \
  "FC_LOADOUT_TBOW_MASORI:tbow_masori"
do
  IFS=: read -r LOADOUT LABEL <<< "$entry"

  PYTHON_BIN=/usr/bin/python3 \
  CONFIG_PATH=/home/joe/projects/runescape-rl/fight-caves-rl/runescape-rl/config/fight_caves.ini \
  FC_ACTIVE_LOADOUT="$LOADOUT" \
  FORCE_BACKEND_REBUILD=1 \
  WANDB_PROJECT="fight caves rl" \
  WANDB_TAG="loadout-sweep-1.5b-$LABEL" \
  WANDB_NAME="loadout-sweep-1.5b-$LABEL" \
  ./runescape-rl/train.sh \
    --wandb-group "$GROUP" \
    --train.total-timesteps 1500000000
done
```

### Backend Used

The sweep used the current `testing` backend, not the original v35.1/SOTA
checkout or binary.

Important backend characteristics:

- Parent code state before committing this entry: `3fe5908fc` (`Restructure
  Fight Caves runtime layout`)
- Active branch: `testing`
- Current backend contains all 9 loadouts in `fc_player_init.h`
- `FC_ACTIVE_LOADOUT` is selected at compile time
- Each run set `FORCE_BACKEND_REBUILD=1`, so every loadout got a fresh
  `pufferlib_4/pufferlib/_C*.so`
- Jad healer behavior is the corrected/current implementation:
  - healer threshold is fixed at `150 HP`
  - healer respawn re-arms only after Jad returns to full HP
- Full supplies were used. The config leaves `initial_sharks` and
  `initial_prayer_doses` unset, so the backend defaults to max sharks and max
  prayer doses.
- Bowfa/no-ammo support is active via `weapon_uses_ammo = 0` for
  `FC_LOADOUT_BOWFA_CRYSTAL`

Supporting sweep-readiness changes were made before this history entry:

- make `FC_ACTIVE_LOADOUT` overridable instead of hard-coded
- add CMake and training-build loadout override plumbing
- add `train.sh` backend stamp checks so loadout changes trigger rebuilds
- improve Python selection so broken `.venv` environments are skipped
- add CTest coverage for the headless Fight Caves test target
- add `build-*/` ignore coverage for local validation builds

### Fixed Config

All runs used `runescape-rl/config/fight_caves.ini`, the restored v35.1/SOTA
recipe, with `train.total_timesteps` overridden to `1_500_000_000`.

Key fixed settings:

| Category | Values |
|---|---|
| W&B group | `loadout-sweep-1.5b-current-backend` |
| project | `fight caves rl` |
| cold start | `load_model_path = null`, `load_id = null` |
| env rewards | `w_damage_dealt=0.9`, `w_damage_taken=-1.9`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_kill=2000.0`, `w_player_death=-11.0` |
| prayer shaping | `w_correct_jad_prayer=1.5`, `w_correct_danger_prayer=0.25`, `shape_wrong_prayer_penalty=-0.3`, `shape_unnecessary_prayer_penalty=-0.2` |
| positioning | `shape_kiting_reward=2.2`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=1.5`, `shape_npc_melee_penalty=-0.8` |
| resources | `shape_food_waste_scale=-1.2`, `shape_pot_waste_scale=-1.2`, full default supplies |
| Jad/healer shaping | `shape_jad_heal_penalty=-0.3`, `shape_wave_stall_start=1400`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_cap=-2.0` |
| obs ablations | `obs_ablate_npc_distance=0`, `obs_ablate_incoming_aggregates=1`, `obs_ablate_npc_valid=0` |
| vec | `total_agents=4096`, `num_buffers=2` |
| PPO | `lr=9e-4`, `gamma=0.996327`, `gae_lambda=0.964053`, `ent_coef=0.024234`, `clip_coef=0.178306`, `vf_coef=1`, `max_grad_norm=0.25` |
| replay/VTrace | `replay_ratio=1.567733`, `vtrace_rho_clip=0.5`, `vtrace_c_clip=0.503727`, `prio_alpha=0.968236`, `prio_beta0=0` |
| policy | MinGRU, `hidden_size=256`, `num_layers=3` |
| seeds | `train.seed=42`, top-level `seed=73` |

### Loadout Inventory

| Label | Symbol | Training profile |
|---|---|---|
| `black_dhide_rcb` | `FC_LOADOUT_BLACK_DHIDE_RCB` | Mid-level 70 HP / 43 prayer / 70 defence / 70 ranged, rune crossbow, black d'hide, adamant bolts. |
| `sota_tbow` | `FC_LOADOUT_SOTA_TBOW` | Original SOTA profile: 99 HP / 99 prayer / 99 defence / 99 ranged, fortified Masori, Twisted bow, dragon arrows, Venator ring. |
| `low_def_rcb` | `FC_LOADOUT_LOW_DEF_RCB` | 1-def Robin/Ranger-tunic RCB profile: 55 HP / 43 prayer / 61 ranged, high RCB offense but fragile. |
| `rcb_pure` | `FC_LOADOUT_RCB_PURE` | 1-def RCB pure with the same offensive numbers as `low_def_rcb` and lower defensive bonuses. |
| `msbi_pure` | `FC_LOADOUT_MSBI_PURE` | 1-def magic shortbow (i) pure: 60 HP / 43 prayer / 70 ranged, fast speed 3 but weak ranged strength. |
| `blowpipe_pure` | `FC_LOADOUT_BLOWPIPE_PURE` | 1-def blowpipe profile: 75 HP / 43 prayer / 75 ranged, speed 2, range 5, low per-hit strength. |
| `acb_armadyl` | `FC_LOADOUT_ACB_ARMADYL` | 80 HP / 70 prayer / 75 defence / 80 ranged, Armadyl crossbow, Armadyl armour, diamond dragon bolts (e). |
| `bowfa_crystal` | `FC_LOADOUT_BOWFA_CRYSTAL` | 85 HP / 70 prayer / 75 defence / 85 ranged, Bowfa and crystal armour, no ammo use. |
| `tbow_masori` | `FC_LOADOUT_TBOW_MASORI` | Max-ish Twisted bow profile, but weaker than `sota_tbow`: 77 prayer, 80 defence, lower ranged attack, lower ranged strength. |

### Results

All runs completed to `1,499,463,680` agent steps. Local JSON logs were written
under `pufferlib_4/logs/fight_caves/`. W&B run directories were written under
`pufferlib_4/wandb/wandb/run-20260622_*`.

| Rank | Loadout | W&B run | Final JKR | Peak JKR | Final reach63 | Peak reach63 | Final wave reached | Final SPS |
|---:|---|---|---:|---:|---:|---:|---:|---:|
| 1 | `sota_tbow` | `ov5qfn36` | 0.910 | 0.910 | 0.970 | 0.970 | 62.82 | 1.140M |
| 2 | `tbow_masori` | `ymj1j1mi` | 0.758 | 0.808 | 0.895 | 0.895 | 62.41 | 1.138M |
| 3 | `bowfa_crystal` | `5dkbvzep` | 0.050 | 0.151 | 0.497 | 0.973 | 57.53 | 1.148M |
| 4 | `acb_armadyl` | `bczf8mij` | 0.011 | 0.117 | 0.972 | 0.972 | 62.69 | 1.150M |
| 5 | `black_dhide_rcb` | `7q6slwsq` | 0.000 | 0.021 | 0.323 | 0.389 | 54.93 | 1.150M |
| 6 | `blowpipe_pure` | `5o15d7a3` | 0.005 | 0.013 | 0.120 | 0.513 | 52.39 | 1.145M |
| 7 | `low_def_rcb` | `6by4xslb` | 0.000 | 0.000 | 0.000 | 0.000 | 53.31 | 1.161M |
| 8 | `rcb_pure` | `gjbpdm8g` | 0.000 | 0.000 | 0.000 | 0.071 | 54.29 | 1.148M |
| 9 | `msbi_pure` | `je3gsp5c` | 0.000 | 0.000 | 0.000 | 0.000 | 41.75 | 1.108M |

### Trajectory Summary

Logged points are approximately 0.19B / 0.56B / 0.94B / 1.31B / 1.50B.

| Loadout | JKR trajectory | reach63 trajectory |
|---|---|---|
| `sota_tbow` | 0.000 / 0.279 / 0.559 / 0.799 / 0.910 | 0.000 / 0.402 / 0.814 / 0.899 / 0.970 |
| `tbow_masori` | 0.000 / 0.109 / 0.582 / 0.808 / 0.758 | 0.000 / 0.204 / 0.750 / 0.852 / 0.895 |
| `bowfa_crystal` | 0.000 / 0.151 / 0.028 / 0.046 / 0.050 | 0.088 / 0.900 / 0.973 / 0.803 / 0.497 |
| `acb_armadyl` | 0.000 / 0.117 / 0.095 / 0.013 / 0.011 | 0.000 / 0.398 / 0.652 / 0.720 / 0.972 |
| `black_dhide_rcb` | 0.000 / 0.000 / 0.003 / 0.021 / 0.000 | 0.000 / 0.005 / 0.294 / 0.389 / 0.323 |
| `blowpipe_pure` | 0.000 / 0.002 / 0.008 / 0.013 / 0.005 | 0.001 / 0.370 / 0.449 / 0.513 / 0.120 |
| `low_def_rcb` | 0.000 / 0.000 / 0.000 / 0.000 / 0.000 | 0.000 / 0.000 / 0.000 / 0.000 / 0.000 |
| `rcb_pure` | 0.000 / 0.000 / 0.000 / 0.000 / 0.000 | 0.000 / 0.000 / 0.000 / 0.071 / 0.000 |
| `msbi_pure` | 0.000 / 0.000 / 0.000 / 0.000 / 0.000 | 0.000 / 0.000 / 0.000 / 0.000 / 0.000 |

### Analysis

The sweep strongly validates the original `sota_tbow` loadout as the loadout
that this v35.1/SOTA PPO recipe actually fits. It was not just best by a small
margin: it was the only run to finish near a 90% Jad kill rate.

`tbow_masori` looked similar by name but is not equivalent in code. It has
lower max prayer, lower defence level, lower ranged attack, and lower ranged
strength than `sota_tbow`. Its lower final result is therefore plausible and
should not be interpreted as evidence that Masori/TBow is generically worse;
it is a weaker simulated loadout definition.

`ACB Armadyl` and `Bowfa Crystal` are the most useful non-SOTA diagnostics.
Both can learn wave progression. `ACB Armadyl` finished with `reached_wave_63 =
0.972` but only `jad_kill_rate = 0.011`, so its failure is Jad/healer closure,
not access to Jad. `Bowfa Crystal` peaked at `reached_wave_63 = 0.973` and
`jad_kill_rate = 0.151`, then regressed by the final bucket. Both likely need
loadout-specific hparams or reward tuning rather than simply more of the same
training recipe.

The lower-level and pure profiles are not solved by the current SOTA recipe at
1.5B. They are substantially more fragile, often lower prayer, lower range,
or lower damage, so they likely need curriculum work, longer budgets, and/or
separate hyperparameter sweeps.

Most importantly for reproducibility: this sweep makes stale compiled backend
state a first-class suspect. Prior current-backend reruns had poor or confusing
results despite matching high-level config metadata. This sweep force-rebuilt
for every loadout and got a strong `sota_tbow` result. Future training commands
should keep the backend stamp/rebuild guardrails and should log
`FC_ACTIVE_LOADOUT` directly to W&B.

### Recommendations

- Treat `ov5qfn36` as the current-backend SOTA-loadout reference run for this
  sweep.
- For SOTA reproduction, use `FC_LOADOUT_SOTA_TBOW`, full supplies, the v35.1
  config, and force or verify backend rebuild.
- Do not train all loadouts with the same recipe and expect equivalent results.
  For `ACB Armadyl` and `Bowfa Crystal`, run targeted Jad/healer-focused
  sweeps or reward adjustments.
- Add explicit W&B config logging for `FC_ACTIVE_LOADOUT`, compiled backend
  stamp, source commit, and whether `FORCE_BACKEND_REBUILD` was active.
- For future loadout sweeps, consider a small repeated-seed sweep for
  `sota_tbow`, `tbow_masori`, `bowfa_crystal`, and `acb_armadyl` before making
  policy decisions from one seed per loadout.

### Artifacts

| Loadout | W&B run | Local JSON log | Checkpoint directory |
|---|---|---|---|
| `sota_tbow` | `ov5qfn36` | `pufferlib_4/logs/fight_caves/ov5qfn36.json` | `pufferlib_4/checkpoints/fight_caves/ov5qfn36/` |
| `tbow_masori` | `ymj1j1mi` | `pufferlib_4/logs/fight_caves/ymj1j1mi.json` | `pufferlib_4/checkpoints/fight_caves/ymj1j1mi/` |
| `bowfa_crystal` | `5dkbvzep` | `pufferlib_4/logs/fight_caves/5dkbvzep.json` | `pufferlib_4/checkpoints/fight_caves/5dkbvzep/` |
| `acb_armadyl` | `bczf8mij` | `pufferlib_4/logs/fight_caves/bczf8mij.json` | `pufferlib_4/checkpoints/fight_caves/bczf8mij/` |
| `black_dhide_rcb` | `7q6slwsq` | `pufferlib_4/logs/fight_caves/7q6slwsq.json` | `pufferlib_4/checkpoints/fight_caves/7q6slwsq/` |
| `blowpipe_pure` | `5o15d7a3` | `pufferlib_4/logs/fight_caves/5o15d7a3.json` | `pufferlib_4/checkpoints/fight_caves/5o15d7a3/` |
| `low_def_rcb` | `6by4xslb` | `pufferlib_4/logs/fight_caves/6by4xslb.json` | `pufferlib_4/checkpoints/fight_caves/6by4xslb/` |
| `rcb_pure` | `gjbpdm8g` | `pufferlib_4/logs/fight_caves/gjbpdm8g.json` | `pufferlib_4/checkpoints/fight_caves/gjbpdm8g/` |
| `msbi_pure` | `je3gsp5c` | `pufferlib_4/logs/fight_caves/je3gsp5c.json` | `pufferlib_4/checkpoints/fight_caves/je3gsp5c/` |

## v25.9.SWEEP — Phase 1 (2026-04-15, completed)

### Overview

Bayesian hyperparameter sweep over 10 reward/training params using v25.9 as the
base config. Goal: find optimal reward shaping for cold-start late-game
progression and Jad conversion.

### Sweep Config

- method: PufferLib Protein (GP-based Bayesian optimization)
- metric: `wave_reached` (maximize)
- 500M steps per run (~4 min each)
- 200 runs completed
- early_stop_quantile: 0.3
- max_suggestion_cost: 300s
- downsample: 5
- first 2 runs: v25.9 defaults (baseline)
- runs 3-10: Sobol sequence exploration
- runs 11+: GP-guided suggestions

### W&B

- entity: jbailey8531-oakton-college
- project: fight caves rl
- tag: v25.9-sweep-phase1

### Base Config (fixed, not swept)

- cold start (no warm-start)
- combat model: Masori (f) + TBow (range 10), `policy_obs=106`, `puffer_obs=142`
- `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_npc_kill=3.5`,
  `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_correct_jad_prayer=2.0`,
  `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`,
  `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`,
  `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`,
  `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`,
  `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`,
  `shape_not_attacking_penalty=-0.01`, `shape_kiting_min_dist=2`,
  `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`,
  `shape_jad_heal_penalty=-0.1`
- `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`,
  `vf_coef=0.5`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`
- `hidden_size=256`, `num_layers=2`, `total_agents=4096`, `num_buffers=2`

### Swept Parameters

| Param | v25.9 value | Sweep min | Sweep max | Distribution |
|-------|-------------|-----------|-----------|--------------|
| `w_damage_taken` | -0.6 | -1.5 | -0.2 | uniform |
| `w_wave_clear` | 15.0 | 5.0 | 30.0 | uniform |
| `w_player_death` | -20.0 | -50.0 | -5.0 | uniform |
| `w_correct_danger_prayer` | 0.25 | 0.05 | 1.5 | uniform |
| `shape_wrong_prayer_penalty` | -1.25 | -4.0 | -0.3 | uniform |
| `shape_npc_specific_prayer_bonus` | 1.5 | 0.3 | 4.0 | uniform |
| `shape_npc_melee_penalty` | -1.0 | -3.0 | -0.3 | uniform |
| `shape_kiting_reward` | 1.0 | 0.2 | 3.0 | uniform |
| `shape_safespot_attack_reward` | 1.5 | 0.3 | 4.0 | uniform |
| `ent_coef` | 0.01 | 0.003 | 0.05 | log_normal |

### Results

- wave_reached range: 23.27 to 55.75
- no runs produced `jad_kill_rate > 0` at 500M steps (expected — v25.9 didn't
  reach wave 60 until 1.275B)
- 1 run (`dark-planet-90`) reached `reached_wave_63 = 0.004`

### Top 10 Runs

#### #1: dry-snowflake-141 (`ojj2ywz4`) — wave_reached = 55.75

| Metric | Value |
|--------|-------|
| max_wave | 63 |
| episode_length | 6798 |
| correct_prayer | 1055.9 |
| wrong_prayer_hits | 409.6 |
| no_prayer_hits | 77.2 |
| prayer_switches | 465.1 |
| pray_magic | 0.478 |
| pray_range | 0.218 |
| damage_blocked | 117468 |
| dmg_taken_avg | 8140 |
| attack_ready_rate | 0.906 |
| food_eaten | 19.8 |
| food_wasted | 3.4 |
| pots_wasted | 8.0 |
| tokxil_melee | 0.9 |
| ketzek_melee | 4.3 |

Config:
- `w_damage_taken = -1.5000`
- `w_wave_clear = 15.1569`
- `w_player_death = -11.0760`
- `w_correct_danger_prayer = 0.0534`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -0.7750`
- `shape_kiting_reward = 2.1879`
- `shape_safespot_attack_reward = 2.2753`
- `ent_coef = 0.0100`

Notes: Best balanced run. Lowest melee exposure (tokxil=0.9), lowest food waste
(3.4), highest damage_blocked (117K), highest correct_prayer (1056). Moderate
config — did NOT push w_wave_clear or safespot to max unlike most other top runs.

#### #2: exalted-music-50 (`fx5v4jlx`) — wave_reached = 55.32

Config:
- `w_damage_taken = -1.5000`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.3753`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.5353`
- `shape_npc_melee_penalty = -2.0004`
- `shape_kiting_reward = 0.6893`
- `shape_safespot_attack_reward = 3.3302`
- `ent_coef = 0.0045`

Notes: Very low ent_coef (0.0045). Worst melee exposure in top 5 (tokxil=14.7,
ketzek=10.3). Low prayer_switches (87) — camps prayer. food_wasted=5.0.

#### #3: lilac-river-106 (`6rx5t0ay`) — wave_reached = 54.47

Config:
- `w_damage_taken = -1.4786`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.0500`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -2.4544`
- `shape_kiting_reward = 1.7317`
- `shape_safespot_attack_reward = 4.0000`
- `ent_coef = 0.0070`

Notes: Max safespot (4.0). Strong melee penalty (-2.45). Good melee avoidance
(tokxil=3.5, ketzek=3.2). food_wasted=6.4.

#### #4: summer-brook-66 (`rtzmrnmc`) — wave_reached = 53.26

Config:
- `w_damage_taken = -1.2019`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.3349`
- `w_correct_danger_prayer = 0.0500`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.7634`
- `shape_npc_melee_penalty = -2.1295`
- `shape_kiting_reward = 1.8303`
- `shape_safespot_attack_reward = 4.0000`
- `ent_coef = 0.0071`

Notes: Only top-5 run that never reached wave 63 (max_wave=60). Highest pot
waste in top 5 (13.2).

#### #5: whole-donkey-155 (`9bgbaa3s`) — wave_reached = 52.85

Config:
- `w_damage_taken = -1.5000`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.0500`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -2.0590`
- `shape_kiting_reward = 2.9137`
- `shape_safespot_attack_reward = 4.0000`
- `ent_coef = 0.0111`

Notes: Max kiting (2.91) and safespot (4.0). Worst pot waste (18.7).
Highest ent_coef in top 5 (0.011).

#### #6: golden-eon-130 (`eeutolls`) — wave_reached = 52.60

Config:
- `w_damage_taken = -1.4658`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.1087`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -1.7052`
- `shape_kiting_reward = 1.8085`
- `shape_safespot_attack_reward = 4.0000`
- `ent_coef = 0.0094`

Notes: max_wave=61. food_wasted=6.4. Moderate melee avoidance.

#### #7: bumbling-gorge-113 (`tak401lk`) — wave_reached = 51.57

Config:
- `w_damage_taken = -1.5000`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.0500`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -2.6348`
- `shape_kiting_reward = 2.1238`
- `shape_safespot_attack_reward = 4.0000`
- `ent_coef = 0.0054`

Notes: max_wave=60. Strongest melee penalty in top 10 (-2.63). Low ent_coef.

#### #8: fresh-resonance-137 (`482ehze8`) — wave_reached = 50.40

Config:
- `w_damage_taken = -1.5000`
- `w_wave_clear = 17.5657`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.2417`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -0.9251`
- `shape_kiting_reward = 1.8607`
- `shape_safespot_attack_reward = 2.3341`
- `ent_coef = 0.0089`

Notes: max_wave=60. Moderate w_wave_clear (17.6). Worst food_wasted (11.2).

#### #9: smart-leaf-200 (`ip6zzpx5`) — wave_reached = 50.27

Config:
- `w_damage_taken = -1.5000`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.1154`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -3.0000`
- `shape_kiting_reward = 2.5137`
- `shape_safespot_attack_reward = 4.0000`
- `ent_coef = 0.0077`

Notes: max_wave=59. Max melee penalty (-3.0) — strongest in sweep. Lowest
tokxil_melee (1.6) and ketzek_melee (2.5) in top 10 after #1.

#### #10: dark-planet-90 (`pm0rw9xx`) — wave_reached = 49.84

Config:
- `w_damage_taken = -1.5000`
- `w_wave_clear = 30.0000`
- `w_player_death = -5.0000`
- `w_correct_danger_prayer = 0.0500`
- `shape_wrong_prayer_penalty = -0.3000`
- `shape_npc_specific_prayer_bonus = 0.3000`
- `shape_npc_melee_penalty = -2.1960`
- `shape_kiting_reward = 1.9065`
- `shape_safespot_attack_reward = 4.0000`
- `ent_coef = 0.0070`

Notes: Only run in top 10 with `reached_wave_63 > 0` (0.004). Lowest
correct_prayer (574.8) and damage_blocked (52067). Lowest prayer uptime.

### Parameter Trends

| Param | v25.9 | Top 10 avg | Top 10 std | Direction |
|-------|-------|-----------|-----------|-----------|
| `w_damage_taken` | -0.60 | -1.47 | 0.09 | much more negative |
| `w_wave_clear` | 15.0 | 27.3 | 5.5 | much higher |
| `w_player_death` | -20.0 | -5.6 | 1.8 | much less negative |
| `w_correct_danger_prayer` | 0.25 | 0.11 | 0.10 | lower |
| `shape_wrong_prayer_penalty` | -1.25 | -0.30 | 0.00 | much less negative |
| `shape_npc_specific_prayer_bonus` | 1.50 | 0.37 | 0.15 | much lower |
| `shape_npc_melee_penalty` | -1.00 | -1.99 | 0.66 | more negative |
| `shape_kiting_reward` | 1.00 | 1.96 | 0.55 | higher |
| `shape_safespot_attack_reward` | 1.50 | 3.59 | 0.68 | much higher |
| `ent_coef` | 0.010 | 0.008 | 0.002 | slightly lower |

### Correlations with wave_reached

| Param | r | Strength |
|-------|---|----------|
| `shape_npc_specific_prayer_bonus` | -0.313 | STRONG |
| `shape_wrong_prayer_penalty` | 0.291 | moderate |
| `w_player_death` | 0.282 | moderate |
| `w_damage_taken` | -0.265 | moderate |
| `w_correct_danger_prayer` | -0.220 | moderate |
| `shape_safespot_attack_reward` | 0.184 | weak |
| `shape_kiting_reward` | 0.182 | weak |
| `ent_coef` | -0.163 | weak |
| `shape_npc_melee_penalty` | -0.156 | weak |
| `w_wave_clear` | 0.145 | weak |

### Key Findings

1. **Positioning > prayer for cold start.** Top configs maximize positioning
   rewards (safespot, kiting, melee penalty) and minimize prayer signals
   (wrong_prayer, npc_specific_prayer, correct_danger_prayer).
2. **`w_damage_taken = -1.5` is nearly universal.** 8 of top 10 hit the cap.
   Heavily punishing damage forces the agent to learn positioning early.
3. **`w_player_death = -5.0` dominates.** 8 of top 10 used minimum death
   penalty. Lower death fear = more risk-taking = faster wave progression.
4. **`shape_wrong_prayer_penalty = -0.3` is universal.** All top 10 used the
   minimum. The agent learns better without heavy prayer punishment.
5. **#1 (dry-snowflake-141) is the best balanced run** despite not pushing
   params to extremes. Lower w_wave_clear (15 vs 30), moderate melee penalty
   (-0.78 vs -2.0), moderate safespot (2.28 vs 4.0), but best melee avoidance,
   food discipline, prayer accuracy, and attack rate.

---

## v25.9.SWEEP — Phase 2 (2026-04-16, in progress)

### Overview

Full 5B step runs on the top configs from phase 1 to see how they scale at
full training length.

### Runs

#### v26 — Top sweep config (source: dry-snowflake-141, `ojj2ywz4`) — COMPLETED

Best balanced run from phase 1. Moderate config that produced the best overall
stats at 500M steps.

Config:
- `w_damage_taken = -1.5`
- `w_wave_clear = 15.0`
- `w_player_death = -11.0`
- `w_correct_danger_prayer = 0.05`
- `shape_wrong_prayer_penalty = -0.3`
- `shape_npc_specific_prayer_bonus = 0.3`
- `shape_npc_melee_penalty = -0.8`
- `shape_kiting_reward = 2.2`
- `shape_safespot_attack_reward = 2.3`
- `ent_coef = 0.01`
- all other params: same as v25.9

Results (`zhqujlo6`):
- **Best run in project history.**
- `wave_reached = 59.79` (final)
- `reached_wave_63 = 0.2444` (24.4% of episodes reaching Jad at final)
- `jad_kill_rate` peak = **0.05603** (5.6%) at 3.440B steps
- 907 of 2384 sampled windows (38%) had `jad_kill_rate > 0`
- peak `reached_wave_63 = 0.9261` (93% reaching Jad) at 3.337B
- peak `wave_reached = 62.80` at 3.337B

Comparison to prior best:
- jad_kill_rate peak: **5.6%** (v26) vs 0.42% (v25.9) vs 0.13% (v25.4) — **13x** improvement over v25.9
- reached_wave_63 final: **24.4%** vs 2.3% (v25.9) — **10x** improvement
- Jad windows: **907** vs 17 (v25.9) — sustained Jad activity, not rare spikes
- food_wasted: 4.93, pots_wasted: 1.13 — excellent resource discipline

#### v26.1 — Consensus max config (source: top 5 average with caps) — COMPLETED

Averages the top 5 configs, pushing shared-cap values. Tests the "extreme
positioning" approach at full scale.

Config:
- `w_damage_taken = -1.5`
- `w_wave_clear = 30.0`
- `w_player_death = -5.0`
- `w_correct_danger_prayer = 0.05`
- `shape_wrong_prayer_penalty = -0.3`
- `shape_npc_specific_prayer_bonus = 0.3`
- `shape_npc_melee_penalty = -2.0`
- `shape_kiting_reward = 2.0`
- `shape_safespot_attack_reward = 4.0`
- `ent_coef = 0.007`
- all other params: same as v25.9

Results (`bhwunrye`):
- **Highest peak jad_kill_rate in project history: 12.5%** at 3.311B
- But **catastrophic collapse after ~4B** — wave_reached crashed from 62 to 25
- Peak reached_wave_63 = 93% at 3.173B
- 750 Jad windows (31% of samples)
- Final: wave_reached=25.49, reached_wave_63=0.0, jad_kill_rate=0.0

The aggressive config produced 2.2x better peak Jad performance than v26 but
was completely unstable. Root causes: low `ent_coef` (0.007) prevented
recovery from PPO drift, minimal `w_player_death` (-5.0) provided no
backstop against death spiral, and extreme positioning signals made the
policy too specialized to recover.

### Phase 2 Conclusions

| | v26 (moderate) | v26.1 (aggressive) |
|---|---|---|
| Peak jad_kill_rate | 5.6% | **12.5%** |
| Final wave_reached | **59.79** | 25.49 (collapsed) |
| Stability | **Stable** | Catastrophic collapse |
| Best for phase 3 base | **Yes** | No (but peak checkpoint valuable) |

**Decision: v26 is the base config for phase 3.** Stable, no collapse,
strong sustained Jad activity. v26.1's 3.3B checkpoint is preserved as a
potential warm-start candidate.

---

## Phase 3 — top runs (by peak jad_kill_rate)

### Phase 3 Swept Parameters

| Param | v26 value | Sweep min | Sweep max | Distribution |
|-------|-----------|-----------|-----------|--------------|
| `w_damage_taken` | -1.5 | -2.5 | -1.5 | uniform |
| `shape_safespot_attack_reward` | 2.3 | 2.5 | 5.5 | uniform |
| `w_jad_damage` | 2.0 | 1.0 | 7.0 | uniform |
| `w_jad_kill` | 150.0 | 700.0 | 2500.0 | uniform |
| `w_correct_jad_prayer` | 2.0 | 1.0 | 5.0 | uniform |
| `shape_jad_heal_penalty` | -0.1 | -5.0 | -0.1 | uniform |
| `w_correct_danger_prayer` | 0.05 | 0.075 | 0.75 | uniform |
| `w_player_death` | -11.0 | -15.0 | -5.0 | uniform |
| `ent_coef` | 0.01 | 0.008 | 0.05 | log_normal |

Each value is annotated with its position within the sweep range (% from min) or `AT MIN` / `AT MAX` if within 2% of a bound.

### 5lt2u1im (peak 33.7%)
```
w_damage_taken               = -1.940    (56% from min)
shape_safespot_attack_reward =  4.235    (58% from min)
w_jad_damage                 =  2.361    (23% from min)
w_jad_kill                   =  2123.753 (79% from min)
w_correct_jad_prayer         =  2.379    (34% from min)
shape_jad_heal_penalty       = -1.154    (78% from min)
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -10.391   (46% from min)
ent_coef                     =  0.00979  (4% from min, near min)
```

### zkmy30mi (peak 25.4%)
```
w_damage_taken               = -1.694    (81% from min)
shape_safespot_attack_reward =  3.604    (37% from min)
w_jad_damage                 =  4.837    (64% from min)
w_jad_kill                   =  2311.811 (90% from min, near max)
w_correct_jad_prayer         =  3.967    (74% from min)
shape_jad_heal_penalty       = -0.745    (87% from min)
w_correct_danger_prayer      =  0.081    AT MIN
w_player_death               = -7.977    (70% from min)
ent_coef                     =  0.00830  AT MIN
```

### vzzwgqcn (peak 25.1%)
```
w_damage_taken               = -2.163    (34% from min)
shape_safespot_attack_reward =  2.500    AT MIN
w_jad_damage                 =  1.000    AT MIN
w_jad_kill                   =  965.145  (15% from min)
w_correct_jad_prayer         =  4.193    (80% from min)
shape_jad_heal_penalty       = -0.100    AT MAX
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -15.000   AT MIN
ent_coef                     =  0.00800  AT MIN
```

### 4jd8ivr8 (peak 20.6%)
```
w_damage_taken               = -1.676    (82% from min)
shape_safespot_attack_reward =  2.500    AT MIN
w_jad_damage                 =  2.753    (29% from min)
w_jad_kill                   =  700.000  AT MIN
w_correct_jad_prayer         =  2.744    (44% from min)
shape_jad_heal_penalty       = -0.100    AT MAX
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -11.030   (40% from min)
ent_coef                     =  0.01390  (14% from min)
```

### u0wwg7lp (peak 18.2%)
```
w_damage_taken               = -1.808    (69% from min)
shape_safespot_attack_reward =  3.401    (30% from min)
w_jad_damage                 =  4.807    (63% from min)
w_jad_kill                   =  2222.769 (85% from min)
w_correct_jad_prayer         =  3.092    (52% from min)
shape_jad_heal_penalty       = -1.385    (74% from min)
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -9.890    (51% from min)
ent_coef                     =  0.01301  (12% from min)
```

### fmwgl0y6 (peak 18.0%)
```
w_damage_taken               = -1.500    AT MAX
shape_safespot_attack_reward =  2.500    AT MIN
w_jad_damage                 =  2.836    (31% from min)
w_jad_kill                   =  700.000  AT MIN
w_correct_jad_prayer         =  3.713    (68% from min)
shape_jad_heal_penalty       = -0.100    AT MAX
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -9.053    (59% from min)
ent_coef                     =  0.01517  (17% from min)
```

### dlowimne (peak 16.8%)
```
w_damage_taken               = -1.753    (75% from min)
shape_safespot_attack_reward =  2.500    AT MIN
w_jad_damage                 =  2.137    (19% from min)
w_jad_kill                   =  700.000  AT MIN
w_correct_jad_prayer         =  3.185    (55% from min)
shape_jad_heal_penalty       = -0.100    AT MAX
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -13.314   (17% from min)
ent_coef                     =  0.00981  (4% from min, near min)
```

### n89g743y (peak 15.6%)
```
w_damage_taken               = -1.791    (71% from min)
shape_safespot_attack_reward =  2.500    AT MIN
w_jad_damage                 =  3.230    (37% from min)
w_jad_kill                   =  747.166  (3% from min, near min)
w_correct_jad_prayer         =  2.195    (30% from min)
shape_jad_heal_penalty       = -0.100    AT MAX
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -14.829   AT MIN
ent_coef                     =  0.01362  (13% from min)
```

### 8mi9v41r (peak 15.2%)
```
w_damage_taken               = -2.186    (31% from min)
shape_safespot_attack_reward =  2.784    (9% from min, near min)
w_jad_damage                 =  1.460    (8% from min, near min)
w_jad_kill                   =  700.000  AT MIN
w_correct_jad_prayer         =  4.605    (90% from min, near max)
shape_jad_heal_penalty       = -0.159    AT MAX
w_correct_danger_prayer      =  0.075    AT MIN
w_player_death               = -13.733   (13% from min)
ent_coef                     =  0.00986  (4% from min, near min)
```

---

## Phase 4 sweep using v28.4 base (2026-04-23/24, completed — 5 runs)

First systematic ablation of **non-env** hyperparameters in the
v28 era. Prior sweeps all modified reward shaping or training
budget; this sweep holds reward config fixed (v28.4 values,
unchanged since v27.2) and varies one PPO/PER/architecture knob
per run, anchored on **v28.8 as the baseline** (deployment SOTA:
peak 0.591, best-250M 0.316, stability 0.29, reach63 0.973 at
3.0B steps, seed 42).

### Sweep-level findings (read first)

1. **`seed` is a no-op.** v29.0 (seed=7) produced byte-identical
   results to v28.8 through the first 2.1B steps and converged
   to matching final values. `grep -n seed pufferlib/*.py` finds
   one hit — inside a comment in `sweep.py`. The config key is
   logged to wandb but never consumed. Implication: v28.7 and
   v28.10's "unlucky seed" narrative was wrong — those runs ran
   the same effective trajectory setup as v28.8/v28.9 and
   collapsed due to budget-dependent PER/CUDA interactions.
2. **v29.1 (disable PER) is a new deployment SOTA.** Single-line
   change `prio_alpha = 0` beats v28.8 on peak, best-250M, final
   jad, and conditional kill rate. Peak arrives 240M steps earlier
   (1.56B vs 1.80B) and best-250M arrives 800M steps earlier
   (1.62B vs 2.42B). Net: same peak quality, 33% less compute.
3. **Three of five ablations catastrophically collapsed.** Lower
   `ent_coef`, aggressive `anneal_lr`, and smaller `hidden_size`
   all failed completely (final jad = 0 for all three). The
   "standard-default" values in PPO (ent=0.01, hidden=256, flat LR)
   are load-bearing for Fight Caves. Only the PER default was wrong.
4. **Noise floor at seed=42 is effectively zero.** v29.0 vs v28.8
   match to 4 decimal places across most metrics. Any observed
   delta > 0.01 on headline metrics is real signal, not variance.

### Cross-sweep summary table

| run | tag | knob change vs v28.8 | peak | peak step | best 250M | final jad | final ent | stability | verdict |
|---|---|---|---|---|---|---|---|---|---|
| v28.8 | v28.8 | (baseline) | 0.591 | 1.80B | 0.316 | 0.223 | 1.63 | 0.29 | reference |
| v29.0 | v29.0 | seed 42 → 7 | 0.591 | 1.80B | 0.316 | 0.223 | 1.63 | 0.29 | **byte-identical to v28.8** |
| **v29.1** | **v29.1** | **prio_alpha 0.8 → 0** | **0.598** | **1.56B** | **0.336** | **0.281** | **1.03** | 0.08 | **✅ NEW SOTA** |
| v29.2 | v29.2 | ent_coef 0.01 → 0.005 | 0.000 | — | 0.000 | 0.000 | 0.14 | 0.00 | ❌ degenerate collapse |
| v29.3 | v29.3 | anneal_lr 0 → 1, min_lr_ratio 0 → 0.1 | 0.024 | 2.36B | 0.007 | 0.000 | 2.66 | 0.00 | ❌ gradient starvation |
| v29.4 | v29.4 | hidden_size 256 → 192 | 0.103 | 1.68B | 0.048 | 0.000 | 1.69 | 0.00 | ❌ capacity-limited |

---

### v29.0 — seed-variance noise-floor probe (`78xa2j6u` / leafy-breeze-329)

Single-variable change from v28.8: `seed = 42 → 7` (added
explicitly under `[train]`, was default 42).

**Bottom line — `seed` is silently ignored.** v29.0 reproduced
v28.8 exactly. The first 998 of 1430 sampled epochs (through step
~2.1B) are byte-identical in `jad_kill_rate`; the remainder
diverge slightly due to CUDA-level kernel scheduling but converge
to matching final values. This is a **training-stack finding**,
not a behavioral finding — the config key is wired to wandb
logging but has no path to any RNG initialization.

#### Results

| metric | v28.8 | v29.0 | Δ |
|---|---|---|---|
| peak `jad_kill_rate` | 0.5911 | 0.5911 | 0.000 |
| peak step | 1,803,550,720 | 1,803,550,720 | 0 |
| best 250M window | 0.3155 | 0.3162 | +0.001 |
| best-250M step | 2,423,259,136 | 2,423,259,136 | 0 |
| final `jad_kill_rate` | 0.2232 | 0.2232 | 0.000 |
| final `reached_wave_63` | 0.9732 | 0.9732 | 0.000 |
| final `wave_reached` | 62.86 | 62.86 | 0.00 |
| final entropy | 1.63 | 1.63 | 0.00 |
| stability (L10/peak) | 0.295 | 0.295 | 0.000 |
| identical samples (count) | — | **998 / 1430 (69.8%)** | — |

#### Config (single-knob diff)

| key | v28.8 | v29.0 |
|---|---|---|
| `[train] seed` | 42 (default) | **7** |

All other keys unchanged: v28.4 reward config, 3.0B budget,
`prio_alpha=0.8`, `ent_coef=0.01`, `anneal_lr=0`, `hidden_size=256`.

#### Observations & diagnosis

- Code audit of pufferl/torch_pufferl for `seed`: single reference
  inside a comment in `sweep.py:763`. No `torch.manual_seed`,
  `np.random.seed`, or env-level seeding reads this key.
- Since v29.0 = v28.8, **every v29.x ablation's delta vs v29.0 is
  a clean single-knob ablation delta vs v28.8** with no confounding
  seed variance.
- Implication for v28.7/v28.10 retrospective: the "seed variance
  vs budget effect" ambiguity we posed in v28.10's recommendations
  was unfalsifiable at the time because we couldn't actually
  change seeds. The two collapses were caused by budget-linked
  deterministic effects (β annealing, CUDA kernel scheduling at
  different training durations), not stochastic bad draws.

#### What worked

Establishing the noise floor — every other run in the sweep can
now be interpreted with confidence. Baseline is reproducible.

#### What didn't work

The sweep's nominal purpose (testing seed variance) was
unanswerable because the knob doesn't exist in code. Fixing this
is low-priority — we're now more confident in single-sample runs
than we were before the sweep.

#### Artifacts

- wandb run: `78xa2j6u`, tag `v29.0`, state `finished`
- Local log: `pufferlib_4/logs/fight_caves/78xa2j6u.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/78xa2j6u/*.bin`
- Deployable: **same as v28.8** — no new artifact value

---

### v29.1 — disable prioritized experience replay (`ml71cg6v` / autumn-bush-330) — 🏆 NEW SOTA

Single-variable change from v28.8: `prio_alpha = 0` (was default
`0.8`), which collapses the entire PER subsystem into uniform
sampling + no IS correction (`adv**0 = 1` everywhere, `anneal_beta`
becomes constant `b0`, `mb_prio = 1`). Effectively: vanilla PPO
with replay buffer.

**Bottom line — genuine improvement across every deployment
metric.** Peak jad 0.598 (+1.2% vs v28.8), best-250M 0.336
(+6.3%), final jad 0.281 (+26%), final conditional kill 30.2%
(vs 22.9%). Peak arrives **240M steps earlier** (1.56B vs 1.80B)
in a tight 40M-step basin, and best-250M arrives **800M steps
earlier** (1.62B vs 2.42B) — effectively a 33% compute reduction
for same-or-better deployment quality. The only "loss" is
late-training stability: a U-shaped trajectory where mid-last-10%
drifts to jad ~0.05 before rebounding to 0.28 in the final 60M
steps. The stability=0.08 single-number metric is misleading
because the endpoint is strong.

**New deployment artifact:** `ml71cg6v` @ 1.62B checkpoint
supersedes `3d7yk2hd` @ 2.42B.

#### Results — full metric sweep

**Core performance:**

| metric | v28.8 | **v29.1** | Δ | % |
|---|---|---|---|---|
| peak `jad_kill_rate` | 0.591 | **0.598** | +0.007 | **+1.2%** |
| peak step | 1.80B | **1.56B** | −240M | **−13% compute** |
| best 250M window | 0.316 | **0.336** | +0.020 | **+6.3%** |
| best-250M step | 2.42B | **1.62B** | −800M | **−33% compute** |
| final `jad_kill_rate` | 0.223 | **0.281** | +0.058 | **+26%** |
| final `reached_wave_63` | 0.973 | 0.930 | −0.043 | −4.4% |
| final conditional kill | 22.9% | **30.2%** | +7.3pp | **+32%** |
| peak-window cond. kill ([1.5–1.75B)) | 24.1% | **49.6%** | +25.5pp | **+106%** |
| last-10% mean jad | 0.174 | 0.049 | −0.125 | — |
| stability (L10/peak) | 0.295 | 0.082 | −0.213 | (misleading; see trajectory) |

The `stability` metric doesn't capture v29.1's U-shape recovery.
Last-10% mean is dragged down by a mid-last-10% dip (step 2.70B
→ 2.95B averaged ~0.05), but the final 60M steps climbed back
to 0.28. **Final-point deployment comparison favors v29.1 strongly.**

**Behavior metrics:**

| metric | v28.8 | **v29.1** | Δ |
|---|---|---|---|
| `episode_length` | 9,039 | 8,875 | −164 |
| `max_wave_ticks` (cleared) | 314 | 358 | +44 |
| `prayer_switches` | 564 | 381 | −183 (−32%) |
| `correct_prayer` | 1,544 | 1,431 | −113 |
| `wrong_prayer_hits` | 280 | 372 | +92 (+33%) |
| `no_prayer_hits` | 67 | 110 | +43 |
| `prayer_uptime_magic` | 0.38 | 0.43 | +0.05 |
| `prayer_uptime_range` | 0.31 | 0.24 | −0.07 |
| `damage_blocked` | 184,802 | **192,734** | **+7,932 (+4.3%)** |
| `dmg_taken_avg` | 8,183 | 9,214 | +1,031 (+13%) |
| `attack_when_ready_rate` | 0.895 | 0.904 | +0.009 |
| `food_eaten` | 19.1 | **15.6** | −3.5 (−18%) |
| `food_wasted` | 4.40 | **1.51** | **−2.89 (−66%)** |
| `pots_used` | 31.7 | 31.5 | −0.2 |
| `pots_wasted` | 3.67 | 10.68 | +7.01 |
| `avg_hp_on_food` | 0.64 | 0.55 | −0.09 |
| `tokxil_melee_ticks` | 2.43 | **1.95** | **−0.48 (−20%)** |
| `ketzek_melee_ticks` | 2.88 | 5.10 | +2.22 |
| `most_npcs_slayed` | 278 | 277 | −1 |
| correct-per-switch | 2.74 | 3.76 | +1.02 |
| final `loss/entropy` | 1.63 | **1.03** | −0.60 |

Notable patterns:

- **v29.1 converged to a different policy, not just "more of the same."**
  Policy entropy is much lower (1.03 vs 1.63), more committed.
  Tokxil positioning improved; ketzek positioning regressed;
  food efficiency improved dramatically (−66% waste); prayer
  accuracy declined slightly; damage blocking improved.
- **Fewer prayer switches but higher correct-per-switch (2.74 →
  3.76).** Policy learned to switch less often but commit to the
  switch decision.
- **Food efficiency is the biggest behavior delta.** `food_wasted`
  dropped by 66% and `avg_hp_on_food` dropped by 0.09 — the
  agent eats later (lower HP when eating) and wastes less.

**Reward-channel breakdown (final state):**

| channel | v28.8 fires / total | **v29.1 fires / total** | Δtotal |
|---|---|---|---|
| `damage_dealt` | 1,774 / +1,739 | 1,753 / +1,719 | −20 |
| `npc_kill` | 271 / +949 | 270 / +945 | −4 |
| `wave_clear` | 62.2 / +29,437 | 61.8 / +29,180 | −257 |
| **`jad_kill`** | **0.22 / +446** | **0.28 / +561** | **+115 (+26%)** |
| `correct_jad_prayer` | 85.6 / +128 | 89.1 / +134 | +6 |
| `correct_danger_prayer` | 1,420 / +355 | 1,308 / +327 | −28 |
| `wrong_danger_prayer` | 317 / −95 | 446 / −134 | −39 |
| `unnecessary_prayer` | 171 / −34 | 203 / −41 | −7 |
| `melee_pressure` | 398 / −384 | 338 / −333 | +51 |
| `wasted_attack` | 1,939 / −194 | 1,937 / −194 | 0 |
| `kiting` | 985 / +2,167 | 995 / +2,189 | +22 |
| `safespot_attack` | 1,700 / +2,551 | 1,718 / +2,577 | +26 |
| `wave_stall` | 0 / 0 | 15.1 / −10.7 | −10.7 |
| **`invalid_action`** | **2,421 / −242** | **1,868 / −187** | **+55 (−23% firing)** |
| `tick_penalty` | 9,084 / −45 | 8,949 / −45 | 0 |
| `food_waste` | 4.6 / −2.7 | 1.5 / −0.8 | +1.9 |
| `pot_waste` | 0.2 / −0.1 | 0.7 / −0.2 | −0.1 |
| `player_death` | 0.78 / −8.5 | 0.71 / −7.9 | +0.6 |
| **total positive** | +37,773 | +37,631 | −142 |
| **total negative** | −1,305 | −1,210 | +95 |
| **net** | +36,467 | +36,420 | −47 (−0.1%) |

Net reward is essentially identical, but `jad_kill` total is +26%
higher. Policy traded marginal reductions in every other channel
for a concentrated improvement in the headline metric. The
`invalid_action` drop (−23% firing) is a secondary signal that
the policy is cleaner / less exploratory.

#### Trajectory — early, sharp peak then U-shape

| bucket | v28.8 jad / ent | **v29.1 jad / ent** | v28.8 r63 | **v29.1 r63** | v28.8 cond-kill | **v29.1 cond-kill** |
|---|---|---|---|---|---|---|
| [0.00, 0.25B) | 0.000 / 4.93 | 0.000 / 5.02 | 0.000 | 0.000 | — | — |
| [0.25, 0.50B) | 0.000 / 3.35 | 0.000 / 2.66 | 0.000 | 0.000 | — | — |
| [0.50, 0.75B) | 0.000 / 3.40 | 0.001 / 2.44 | 0.000 | 0.041 | — | 2.5% |
| [0.75, 1.00B) | 0.000 / 3.57 | 0.007 / 2.11 | 0.000 | 0.279 | — | 2.6% |
| [1.00, 1.25B) | 0.000 / 3.30 | 0.070 / 1.87 | 0.000 | 0.431 | — | **16.2%** |
| [1.25, 1.50B) | 0.026 / 2.56 | **0.201** / 1.65 | 0.152 | 0.540 | 17.0% | **37.3%** |
| [1.50, 1.75B) | **0.142** / 2.20 | **0.279** / 1.56 | 0.590 | 0.562 | 24.1% | **49.6%** |
| [1.75, 2.00B) | **0.259** / 2.14 | 0.238 / 1.38 | 0.577 | 0.717 | 44.9% | 33.2% |
| [2.00, 2.25B) | 0.157 / 2.17 | 0.101 / 1.22 | 0.687 | 0.523 | 22.9% | 19.3% |
| [2.25, 2.50B) | **0.279** / 2.11 | 0.090 / 1.19 | 0.806 | 0.375 | 34.7% | 24.1% |
| [2.50, 2.75B) | 0.169 / 1.98 | 0.010 / 1.15 | 0.841 | 0.087 | 20.1% | 12.0% |
| [2.75, 3.00B) | 0.158 / 1.76 | **0.058** / 1.03 | 0.906 | 0.550 | 17.4% | 10.5% |

**v29.1 vs v28.8 divergence profile:**

- **Earlier learning:** non-zero `jad_kill` starts at [0.5, 0.75B)
  for v29.1 vs [1.25, 1.5B) for v28.8. **~750M steps of head start.**
- **Faster entropy commitment:** v29.1 drops to 1.56 entropy by
  [1.5, 1.75B); v28.8 reaches 2.20 in the same window.
- **Peak window arrives 240M earlier and is 2× more effective
  at killing:** v29.1 [1.5, 1.75B) has 49.6% conditional kill
  rate, vs v28.8's peak window [1.75, 2.0B) at 44.9%.
- **Post-peak decay is faster:** by [2.5, 2.75B) v29.1 is at
  jad=0.010 while v28.8 holds 0.169. This is the "stability
  cost" of removing PER — without importance-weighted sampling
  keeping the policy near high-value basins, it drifted.
- **Late-training U-shape:** the final 60M steps rebound
  (samples showing 0.08 → 0.14 → 0.18 → 0.28 jad_kill_rate
  within the last ~3% of training). Mechanism unclear — may be
  that the policy re-found the Jad-kill basin as late training
  noise pushed it back toward high-advantage states.

v29.1 top-10 peak samples (40M-step tight basin at 1.54–1.58B):

| step | jad_kill_rate |
|---|---|
| 1,564,475,392 | 0.5979 |
| 1,566,572,544 | 0.5637 |
| 1,553,989,632 | 0.5617 |
| 1,556,086,784 | 0.5602 |
| 1,549,795,328 | 0.5488 |
| 1,562,378,240 | 0.5484 |
| 1,543,503,872 | 0.5409 |
| 1,547,698,176 | 0.5388 |
| 1,577,058,304 | 0.5370 |
| 1,568,669,696 | 0.5362 |

All 10 above jad=0.53 in a 40M-step window — a dense peak
cluster, not a single lucky sample.

#### What worked

**1. PER was genuinely not helping.** All the "budget-dependent
behavior" we debugged through v28.7 → v28.10 collapsed was
PER-mediated. Removing it:
- Eliminates β annealing as a training-time variable
- Eliminates importance-weighted sampling as a nondeterministic
  schedule
- Removes mb_prio IS correction from advantage scaling
- Delivers a better peak on a better schedule

**2. Sample efficiency improved dramatically.** Best-250M at
1.62B vs 2.42B is a 33% compute reduction for better quality.

**3. Cleaner final policy.** `invalid_action` firing dropped 23%,
entropy dropped 0.6 nats, correct-per-switch improved.

**4. The 1.54–1.58B window produced 10 consecutive samples at
jad ≥ 0.53** — this is a deployment-grade basin, not a
stochastic spike.

#### What didn't work

**1. Late-training stability regressed.** Mid-last-10% drift to
jad ~0.05 is a real regression vs v28.8's 0.17 last-10%.
Fortunately the final endpoint recovers.

**2. Ketzek positioning regressed** (2.88 → 5.10 ticks). Some
mid-cave skill loss. Not catastrophic (wave_reached = 62.48 vs
v28.8's 62.86).

**3. Food-vs-pot tradeoff flipped in the wrong direction.**
Food efficiency dramatically improved (waste −66%) but pot
efficiency collapsed (waste +191%). Net resource management
cost is small (~1% of gross reward) but suggests the
no-PER policy has a different resource-allocation prior.

**4. Stability metric is broken.** stability=0.08 looks alarming
but the U-shape recovery means the 3.0B endpoint is fine. For
v30.x we should adopt a better late-training quality measure —
e.g., "final-3 samples mean" or "min jad in last 100M steps."

#### Expected-outcome audit (against the v29.1 .ini staged predictions)

| prediction | target | actual | status |
|---|---|---|---|
| peak within ±0.05 of v29.0 | [0.54, 0.64] | 0.598 | ✅ |
| if peak ~v29.0 → delete PER | qualitative | confirmed | ✅ **delete PER** |
| peak drop by 0.10+ → keep PER | — | no drop | N/A |
| final entropy lower | qualitative | 1.03 vs 1.63 | ✅ |

Landed in the strongest outcome tier we projected. **The PER
subsystem is not needed.**

#### Exact active config (v29.1)

```
[env]   (v28.4 reward config, unchanged)
w_damage_dealt = 0.9           w_damage_taken = -1.9
w_npc_kill = 3.5               w_wave_clear = 15.0
w_jad_kill = 2000.0            w_player_death = -11.0
w_correct_jad_prayer = 1.5     w_correct_danger_prayer = 0.25
w_invalid_action = -0.1        w_tick_penalty = -0.005
shape_food_full_waste_penalty = -6.5
shape_food_waste_scale = -1.2
shape_pot_full_waste_penalty = -6.5
shape_pot_waste_scale = -1.2
shape_wrong_prayer_penalty = -0.3
shape_npc_melee_penalty = -0.8
shape_wasted_attack_penalty = -0.1
shape_kiting_reward = 2.2
shape_kiting_min_dist = 2      shape_kiting_max_dist = 10
shape_safespot_attack_reward = 1.5
shape_unnecessary_prayer_penalty = -0.2
shape_wave_stall_start = 1400
shape_wave_stall_ramp_interval = 150
shape_wave_stall_base_penalty = -0.5
shape_wave_stall_cap = -2.0
shape_resource_threat_window = 2

[vec]
total_agents = 4096            num_buffers = 2

[train]
total_timesteps = 3_000_000_000
learning_rate = 0.0003         anneal_lr = 0
gamma = 0.999                  gae_lambda = 0.95
clip_coef = 0.2                vf_coef = 0.5
ent_coef = 0.01                max_grad_norm = 0.5
horizon = 256                  minibatch_size = 4096
prio_alpha = 0                 # ← single diff from v28.8

[policy]
hidden_size = 256              num_layers = 2
```

#### Artifacts

- Wandb run: `ml71cg6v`, tag `v29.1`, state `finished`,
  display `autumn-bush-330`
- Local log: `pufferlib_4/logs/fight_caves/ml71cg6v.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/ml71cg6v/*.bin`
- **Deployable artifact: the 1.62B checkpoint** (best-250M = 0.336,
  peak cond-kill 49.6%). Supersedes v28.8's 2.42B checkpoint as
  deployment SOTA.
- Secondary: the 1.56B checkpoint (single-step peak = 0.598) for
  peak-jad benchmarking.

---

### v29.2 — lower entropy bonus (`rxuybk5m` / morning-snowflake-331) — ❌ COLLAPSE

Single-variable change from v28.8: `ent_coef = 0.01 → 0.005`.
PPO's loss includes `-ent_coef * entropy`, so halving `ent_coef`
halves the exploration pressure keeping the policy broad.

**Bottom line — degenerate premature commitment, catastrophic
collapse.** Entropy cratered to 0.14 by the end of training,
agent locked into a policy that doesn't progress past early
waves. Final `jad_kill_rate = 0.000`, final `reached_wave_63 =
0.000`, final `wave_reached = 15.10` (vs v28.8's 62.86). 97%
net reward loss vs baseline.

#### Results

| metric | v28.8 | v29.2 | Δ |
|---|---|---|---|
| peak `jad_kill_rate` | 0.591 | **0.000** | −0.591 |
| final `jad_kill_rate` | 0.223 | 0.000 | −0.223 |
| final `reached_wave_63` | 0.973 | 0.000 | −0.973 |
| final `wave_reached` | 62.86 | **15.10** | −47.76 |
| final `max_wave` | 63 | 35 | −28 |
| final `most_npcs_slayed` | 278 | 132 | −146 |
| final entropy | 1.63 | **0.14** | −1.49 |
| final `episode_length` | 9,039 | **3,279** | −5,760 |
| final value loss | 7.06 | **581** | +574 (≈82×) |
| final `loss/kl` | 0.01 | 0.04 | +0.03 |
| net reward | +36,467 | **+1,216** | **−96.7%** |

#### Config (single-knob diff)

| key | v28.8 | v29.2 |
|---|---|---|
| `[train] ent_coef` | 0.01 | **0.005** |

#### Observations & diagnosis

- Entropy collapsed by [0.5, 0.75B) — dropped from 5.0 (random)
  to 0.73 in 750M steps (vs v28.8 reaching 3.40 at the same point).
- `prayer_switches = 13` (vs v28.8's 564). Agent effectively
  stopped switching prayers.
- `correct_prayer = 112` (vs 1,544). Prayer behavior
  effectively disabled.
- `attack_when_ready_rate = 0.34` (vs 0.89). Not even attacking
  most of the time.
- `value loss = 581` — the value function cannot fit the
  non-progressing state distribution.
- `avg_hp_on_food = 0.90` (vs 0.64). Eating at near-full HP
  defensively — classic symptom of degenerate-safe behavior.
- The policy committed to a **"do almost nothing" attractor**
  very early. Once committed, no gradient existed to pull it out
  because the reward landscape was effectively flat around that
  fixed point.

**Diagnostic insight:** `ent_coef = 0.01` is not conservatism —
it's load-bearing. The v28 training regime discovers Jad play
slowly (no Jad kills until step ~1.0B), and needs sustained
exploration pressure through that entire window. Halving it is
enough to let the policy collapse onto a local minimum before
the reward signal has time to pull it toward Jad.

#### What worked

Establishing the **lower bound** for `ent_coef` — 0.005 is below it.

#### What didn't work

Everything else.

#### Artifacts

- Wandb run: `rxuybk5m`, tag `v29.2`, state `finished`,
  display `morning-snowflake-331`
- Local log: `pufferlib_4/logs/fight_caves/rxuybk5m.json`
- **No deployable artifact.** Do not use v29.2 checkpoints.

---

### v29.3 — enable LR annealing (`zsbxnx5f` / charmed-tree-332) — ❌ COLLAPSE

Two-variable change from v28.8: `anneal_lr = 0 → 1` AND
`min_lr_ratio = 0 → 0.1`. Activates cosine decay in
`torch_pufferl.py:262-266`: LR decays from 3e-4 → 3e-5 over
the full run on a cosine schedule.

**Bottom line — LR decay kills the gradient too early.** Partial
learning through ~1.5B (reached some mid-waves), then stalled
entirely. Final `jad_kill_rate = 0`, final `reached_wave_63 = 0`,
final entropy 2.66 (never committed). 71% net reward loss.

#### Results

| metric | v28.8 | v29.3 | Δ |
|---|---|---|---|
| peak `jad_kill_rate` | 0.591 | **0.024** | −0.567 |
| peak step | 1.80B | 2.36B | — |
| final `jad_kill_rate` | 0.223 | 0.000 | −0.223 |
| final `reached_wave_63` | 0.973 | 0.000 | −0.973 |
| final `wave_reached` | 62.86 | **34.99** | −27.87 |
| final `max_wave` | 63 | 63 (reached once) | — |
| final entropy | 1.63 | **2.66** | +1.03 |
| final value loss | 7.06 | 33.5 | +26.4 |
| final `episode_length` | 9,039 | 4,059 | −4,980 |
| net reward | +36,467 | **+10,605** | **−70.9%** |

#### Config (single-knob diff)

| key | v28.8 | v29.3 |
|---|---|---|
| `[train] anneal_lr` | 0 | **1** |
| `[train] min_lr_ratio` | 0 | **0.1** |

#### Observations & diagnosis

- Entropy stays in the 2.2–2.6 range throughout training — agent
  never commits.
- `prayer_switches = 100` (low but not zero). Partial prayer
  learning.
- `correct_prayer = 489` — a third of v28.8's 1,544. Some
  prayer skill formed but never fully committed.
- `food_wasted = 15.60` (vs v28.8's 4.40). High waste is a
  secondary symptom of undeveloped resource-timing skill.
- `tokxil_melee_ticks = 15.54` — positioning never learned
  on Tok-Xil waves.
- Late training (buckets [2.0B+]) shows episode length
  **decreasing** (7600 → 4734). Agent getting WORSE at
  survival as LR decays — gradient too small to hold onto
  previously-learned behaviors.
- LR at terminal step is 3e-5 (cosine-decayed from 3e-4).
  At that learning rate, the PPO update step is too small
  to refine a policy that hasn't yet converged.

**Diagnostic insight:** anneal_lr is meant as a **stability
aid for converged policies** in late training. Applied to a
policy that hasn't yet converged at training start, it starves
the gradient before the policy can find the good basin. The
`min_lr_ratio=0.1` (10% of initial) is too aggressive for our
regime. If we try LR anneal again in v30.x, start with
`min_lr_ratio=0.5` (half-strength decay).

#### What worked

Establishing that **aggressive LR decay is harmful at our scale
of budget**. Informs v30.x design.

#### What didn't work

Same failure mode as v29.2 but via a different mechanism — both
end with undeveloped Jad-skill and collapsed headline metrics.

#### Artifacts

- Wandb run: `zsbxnx5f`, tag `v29.3`, state `finished`,
  display `charmed-tree-332`
- Local log: `pufferlib_4/logs/fight_caves/zsbxnx5f.json`
- **No deployable artifact.**

---

### v29.4 — smaller policy net (`fba68bxk` / ethereal-sky-333) — ❌ CAPACITY COLLAPSE

Single-variable change from v28.8: `hidden_size = 256 → 192`
(−25% hidden, ~−41% total parameters on a 2-layer MinGRU).

**Bottom line — smaller net learns survival but not Jad-specific
skills.** Final `wave_reached = 60.11` (near v28.8's 62.86 —
the agent reaches Jad) but final `jad_kill_rate = 0` and
`correct_jad_prayer_fires = 3.6` (vs v28.8's 85.6). The
policy never developed the high-frequency action sequencing
required for Jad prayer switching. 13% net reward loss —
better than v29.2/v29.3 but still a clear failure on the
headline metric.

#### Results

| metric | v28.8 | v29.4 | Δ |
|---|---|---|---|
| peak `jad_kill_rate` | 0.591 | **0.103** | −0.488 |
| peak step | 1.80B | 1.68B | −120M |
| best 250M window | 0.316 | 0.048 | −0.268 |
| final `jad_kill_rate` | 0.223 | 0.000 | −0.223 |
| final `reached_wave_63` | 0.973 | 0.103 | −0.870 |
| final `wave_reached` | 62.86 | **60.11** | −2.75 (close!) |
| final `most_npcs_slayed` | 278 | 276 | −2 (near match) |
| final `correct_jad_prayer_fires` | 85.6 | **3.6** | **−95.8%** |
| final entropy | 1.63 | 1.69 | +0.06 |
| final value loss | 7.06 | 8.45 | +1.4 |
| net reward | +36,467 | +31,793 | −12.8% |

#### Config (single-knob diff)

| key | v28.8 | v29.4 |
|---|---|---|
| `[policy] hidden_size` | 256 | **192** |

#### Observations & diagnosis

- **Wave survival learned fine:** `wave_reached = 60.11`,
  `most_npcs_slayed = 276`, `correct_prayer = 1,230` (near
  v28.8's 1,544).
- **Jad-specific skills never formed:** `correct_jad_prayer_fires
  = 3.6` — effectively zero. Compare v28.8: 85.6, v29.1: 89.1.
- **Positioning regressed:** `ketzek_melee_ticks = 16.54`,
  `tokxil_melee_ticks = 12.94` — 5–8× v28.8's. Agent survives
  but takes more damage doing so.
- **Food waste high:** 8.46 (vs v28.8's 4.40). Undeveloped
  resource timing.
- **Trajectory pattern:** peaked briefly at 0.10 jad in
  [1.5, 1.75B), then drifted. Best 250M was only 0.048.

**Diagnostic insight:** 192-hidden MinGRU has enough capacity
for bulk wave survival (positioning-adjacent tasks, prayer
against known wave schedules) but NOT for the
precise-timing prayer switching that Jad attacks require. Jad
has a variable 3-tick attack rotation between magic and ranged;
killing Jad requires predicting the attack type before each
throw and switching prayer within a 2-tick window. This needs
enough net capacity to carry the attack-history context across
timesteps AND produce a sharp decision boundary. At 192 hidden,
the network can carry one but not both.

**Bound established:** don't go below `hidden_size = 256` for
this task. 256 is load-bearing.

#### What worked

Establishing the **lower bound for policy capacity**. If we ever
want to reduce params (e.g., for SPS), the answer is **not via
hidden_size**. Try `num_layers = 1` or `expansion_factor = 0.75`
next time, not further hidden reduction.

#### What didn't work

Straight hidden_size reduction. ~41% parameter cut loses Jad-
specific skill entirely despite preserving bulk wave-clear.

#### Artifacts

- Wandb run: `fba68bxk`, tag `v29.4`, state `finished`,
  display `ethereal-sky-333`
- Local log: `pufferlib_4/logs/fight_caves/fba68bxk.json`
- **No deployable artifact** for Jad. Could be used as a
  "wave-clear-only" model if that's ever useful.

---

### Cross-cutting conclusions

**Adopt immediately:**
1. **`prio_alpha = 0` is the new default.** Delete `prio_alpha` and
   `prio_beta0` from future configs OR set to `prio_alpha = 0`
   explicitly. v29.1 deploys better than v28.8 on every headline
   metric at 33% less compute.
2. **v29.1 @ 1.62B checkpoint is the new deployment SOTA.**
   Best-250M = 0.336, peak cond-kill 49.6%. Supersedes v28.8 @ 2.42B.

**Do NOT change from v28.8 baseline:**
1. `ent_coef` must stay at 0.01 (0.005 produced degenerate
   collapse).
2. `hidden_size` must stay at 256 (192 lacks Jad-skill capacity).
3. `anneal_lr` must stay at 0 **OR**, if enabled, use
   `min_lr_ratio ≥ 0.5` (0.1 is too aggressive for our budget).

**Low-priority cleanup:**
1. `seed` config key is a silent no-op. Either wire it up
   (`torch.manual_seed` + `np.random.seed` at trainer init) or
   remove it. Low priority because we now know noise floor
   is effectively zero at the effective seed.

**Follow-up experiments worth running (v30.x):**
1. **v30.0 (strongest candidate): `prio_alpha=0` + `total_timesteps
   = 2_000_000_000`.** v29.1 peaked at 1.56B; 2.0B captures the
   peak window plus a bit of post-peak without the long drift.
   Projected best-250M: ≥ 0.336 at 67% of v29.1's compute.
2. **v30.1: `prio_alpha=0` + `anneal_lr=1` + `min_lr_ratio=0.5`.**
   Test whether a gentle (half-strength) LR decay catches v29.1's
   peak and settles the policy through the late-training U-shape.
3. **v30.2 (diagnostic, optional): wire up `seed` properly, re-run
   v29.1 at seed=7.** Gets us a real seed-variance estimate for
   future ablation comparisons.

### Summary — what the sweep taught us about our training stack

1. **We had one wrong default out of ~22 non-env knobs.** Every
   other PPO default (gamma, gae_lambda, clip_coef, vf_coef,
   ent_coef, max_grad_norm, horizon, minibatch_size, hidden_size,
   num_layers, flat LR) was correct. The "standard defaults" are
   good defaults for Fight Caves.
2. **PER was actively hurting peak performance.** Not just
   neutral — it was slowing convergence and adding nondeterminism.
   v28.7 and v28.10's mysterious collapses were downstream of
   PER's β-annealing-vs-budget interaction.
3. **Our noise floor is ~0** — we can trust single-seed ablation
   deltas above 0.01 on headline metrics.
4. **Deployment SOTA is now v29.1 @ 1.62B**, superseding v28.8.
   Five hours of GPU time produced a ~26% final-jad improvement
   and 33% compute reduction. First net-positive run of the v28
   era since v28.8 set the prior SOTA.

---

## v28.8 Sweep 1: reward leave-one-out ablation (2026-04-24/25, complete — 10 runs)

### Purpose

Before running the larger hparam sweeps (Sweep 2 and Sweep 3), first
determine which of the currently-used env reward-shaping signals are
actually contributing. Any reward that zero-outs cleanly (all headline
metrics within v28.8 ± noise floor) can be dropped, shrinking the search
space and compute cost for the subsequent non-reward and reward-tuning
sweeps.

### Baseline

**v28.8** (3B steps, peak 0.591, best-250M 0.316, stability 0.29, final 0.223).

Chose v28.8 rather than v29.1 (current deployment SOTA) to keep
comparability with all prior v28.x runs already in run_history. The PER
fix from v29.1 can be composed with whatever reward changes fall out of
this sweep in a later round.

### Method

Each run is a single-variable leave-one-out change from v28.8: exactly
one reward weight is set to 0, all other env + train + policy hparams
remain identical. Full v28.8 budget (3B steps) per run so we get
deployment-quality signal and apples-to-apples comparability with prior
v28.x runs.

### Runs

| Run | Config | Zeroed reward | Prior value |
|-----|--------|---------------|-------------|
| s1.01 | fight_caves_s1_01.ini | w_npc_kill | 3.5 |
| s1.02 | fight_caves_s1_02.ini | w_invalid_action | -0.1 |
| s1.03 | fight_caves_s1_03.ini | w_tick_penalty | -0.005 |
| s1.04 | fight_caves_s1_04.ini | shape_wasted_attack_penalty | -0.1 |
| s1.05 | fight_caves_s1_05.ini | shape_wrong_prayer_penalty | -0.3 |
| s1.06 | fight_caves_s1_06.ini | shape_unnecessary_prayer_penalty | -0.2 |
| s1.07 | fight_caves_s1_07.ini | shape_food_full_waste_penalty | -6.5 |
| s1.08 | fight_caves_s1_08.ini | shape_food_waste_scale | -1.2 |
| s1.09 | fight_caves_s1_09.ini | shape_pot_full_waste_penalty | -6.5 |
| s1.10 | fight_caves_s1_10.ini | shape_pot_waste_scale | -1.2 |

Launched via `sweep_s1.sh` (sequential; tags `v28.8-s1.01` → `v28.8-s1.10`).

### Interpretation criteria

For each run, compare to v28.8 on four headline metrics:
peak `jad_kill_rate`, best-250M rolling mean, stability (last-10% / peak),
and final `jad_kill_rate`.

- **All 4 metrics within v28.8 ± 0.01 (confirmed noise floor from v29.0):**
  reward is inert. Drop it from the config for Sweep 2/3 and deployment.
- **Peak or best-250M drops ≥ 0.03:** reward is load-bearing. Keep it.
- **Peak improves ≥ 0.03:** the reward was over-penalizing. Strong signal
  to drop or soften it.
- **Metrics move in opposite directions** (e.g., peak up, stability down):
  mixed effect. Flag for a follow-up ramp sweep (5× → 3× → 1× → 0) before
  deciding.

### Rewards deliberately NOT in this sweep

Out of scope for Sweep 1 (prior evidence already establishes they are
load-bearing):

- `w_damage_dealt`, `w_damage_taken` — primary combat signal. Zeroing
  would gut the learning signal.
- `w_wave_clear`, `w_jad_kill`, `w_player_death` — terminal rewards.
  Zeroing destroys the task objective.
- `w_correct_jad_prayer`, `w_correct_danger_prayer` — prior runs showed
  these are needed to learn prayer-switching.
- `shape_npc_melee_penalty`, `shape_safespot_attack_reward`,
  `shape_kiting_reward` — positioning signals that the agent cannot
  infer from raw damage alone.
- `shape_wave_stall_*` — validated as sweet-spot via the Phase 3 ramp
  sweep (1000 → 1200 → 1400 → 1600 tested).

### Reference config (v28.8 baseline, shared across all 10 runs)

```ini
[env]
w_damage_dealt = 0.9
w_damage_taken = -1.9
w_npc_kill = 3.5
w_wave_clear = 15.0
w_jad_kill = 2000.0
w_player_death = -11.0
w_correct_jad_prayer = 1.5
w_correct_danger_prayer = 0.25
w_invalid_action = -0.1
w_tick_penalty = -0.005
shape_food_full_waste_penalty = -6.5
shape_food_waste_scale = -1.2
shape_pot_full_waste_penalty = -6.5
shape_pot_waste_scale = -1.2
shape_wrong_prayer_penalty = -0.3
shape_npc_melee_penalty = -0.8
shape_wasted_attack_penalty = -0.1
shape_kiting_reward = 2.2
shape_kiting_min_dist = 2
shape_kiting_max_dist = 10
shape_safespot_attack_reward = 1.5
shape_unnecessary_prayer_penalty = -0.2
shape_wave_stall_start = 1400
shape_wave_stall_ramp_interval = 150
shape_wave_stall_base_penalty = -0.5
shape_wave_stall_cap = -2.0
shape_resource_threat_window = 2

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 3_000_000_000
learning_rate = 0.0003
anneal_lr = 0
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.01
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Each per-run config (`fight_caves_s1_NN.ini`) is byte-identical except
for the single ablation target set to 0.

### Results

All 10 runs completed full 3B-step budget (2861 epochs each, ~26 min wall
each on the local box). Final metrics pulled from wandb runs
`fjc8ogzl, 4fmjnm3m, ockcnahb, tvsebomn, fwvi03kw, ekcl8wuw, 9vp07i6p,
vvr0f1qm, zer9a3k5, hx63kihe`. Baseline reference is the byte-identical
v29.0 run (`78xa2j6u`) per the v29.0 noise-floor finding (seed key is
silently ignored, so v29.0 == v28.8).

#### Headline metrics

| run | reward zeroed (prior value) | peak | peak step | best 250M | final jad | L10 mean | stab | final ent |
|---|---|---|---|---|---|---|---|---|
| **v28.8** | (baseline) | 0.591 | 1.80B | 0.316 | 0.223 | 0.174 | 0.29 | 1.63 |
| s1.01 | `w_npc_kill` (3.5) | **0.800** | 1.19B | **0.552** | 0.161 | 0.137 | 0.17 | 2.03 |
| s1.02 | `w_invalid_action` (-0.1) | 0.278 | 1.91B | 0.136 | 0.086 | 0.066 | 0.24 | 3.31 |
| s1.03 | `w_tick_penalty` (-0.005) | 0.364 | 2.52B | 0.224 | 0.110 | 0.208 | 0.57 | 1.75 |
| s1.04 | `shape_wasted_attack_penalty` (-0.1) | 0.626 | 2.45B | 0.428 | 0.109 | 0.209 | 0.33 | 0.96 |
| s1.05 | `shape_wrong_prayer_penalty` (-0.3) | 0.515 | 1.53B | 0.237 | 0.206 | 0.151 | 0.29 | 1.72 |
| s1.06 | `shape_unnecessary_prayer_penalty` (-0.2) | 0.014 | 1.73B | 0.001 | 0.000 | 0.001 | 0.04 | 1.11 |
| s1.07 | `shape_food_full_waste_penalty` (-6.5) | 0.591 | 1.80B | 0.316 | 0.223 | 0.174 | 0.29 | 1.63 |
| s1.08 | `shape_food_waste_scale` (-1.2) | 0.413 | 1.64B | 0.226 | **0.297** | 0.212 | 0.51 | 3.10 |
| s1.09 | `shape_pot_full_waste_penalty` (-6.5) | 0.597 | 1.81B | 0.316 | 0.223 | 0.174 | 0.29 | 1.63 |
| s1.10 | `shape_pot_waste_scale` (-1.2) | 0.278 | 2.82B | 0.143 | 0.027 | 0.129 | 0.46 | 2.91 |

#### Deltas vs v28.8 + classification

| run | reward zeroed | Δ peak | Δ b250M | Δ final | Δ L10 | Δ stab | classification |
|---|---|---|---|---|---|---|---|
| s1.01 | `w_npc_kill` | **+0.209** | **+0.236** | -0.062 | -0.037 | -0.12 | **OVER-PENALIZING** |
| s1.02 | `w_invalid_action` | -0.313 | -0.180 | -0.137 | -0.108 | -0.06 | load-bearing |
| s1.03 | `w_tick_penalty` | -0.227 | -0.092 | -0.113 | +0.034 | +0.28 | load-bearing (mixed: stability up) |
| s1.04 | `shape_wasted_attack_penalty` | +0.035 | **+0.111** | -0.114 | +0.035 | +0.04 | **OVER-PENALIZING (mixed: final down)** |
| s1.05 | `shape_wrong_prayer_penalty` | -0.076 | -0.079 | -0.017 | -0.024 | -0.00 | load-bearing (mild) |
| s1.06 | `shape_unnecessary_prayer_penalty` | -0.577 | -0.315 | -0.223 | -0.174 | -0.26 | load-bearing (catastrophic) |
| **s1.07** | **`shape_food_full_waste_penalty`** | **+0.000** | **-0.000** | **+0.000** | **+0.000** | **+0.00** | **✅ INERT — drop** |
| s1.08 | `shape_food_waste_scale` | -0.179 | -0.090 | **+0.073** | +0.038 | +0.22 | load-bearing (mixed: best final + best stability) |
| **s1.09** | **`shape_pot_full_waste_penalty`** | **+0.006** | **-0.000** | **+0.000** | **-0.000** | **-0.00** | **✅ INERT — drop** |
| s1.10 | `shape_pot_waste_scale` | -0.313 | -0.174 | -0.196 | -0.045 | +0.17 | load-bearing |

#### Behavioral snapshot (final-epoch values)

| run | corr_pr | pr_sw | food | pots | food_wst | pots_wst | inv_fires | wat_atk_fires | unnp_fires | no_pray_h | wrong_pray_h | ep_len |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| v28.8 | 1544 | 564 | 19.1 | 31.7 | 4.4 | 3.7 | 2421 | 1939 | 171 | 67 | 280 | 9039 |
| s1.01 (no npc_kill) | 1264 | 604 | 16.7 | 30.8 | 9.0 | 8.6 | 2838 | 1807 | 444 | 69 | 321 | 8550 |
| s1.02 (no invalid) | 1084 | 294 | 20.0 | 25.4 | 17.7 | 9.6 | 0✱ | 1550 | 259 | 67 | 180 | 7203 |
| s1.03 (no tick) | 1414 | 392 | 19.2 | 31.3 | 4.8 | 3.2 | 1794 | 1835 | 297 | 56 | 262 | 8771 |
| s1.04 (no wasted_atk) | 1361 | 359 | 17.4 | 31.0 | 8.2 | 4.8 | 2003 | 0✱ | 211 | 56 | 284 | 8705 |
| s1.05 (no wrong_pray) | 1325 | 414 | 19.0 | 31.4 | 6.9 | 10.5 | 2070 | 1732 | 345 | 93 | 342 | 8206 |
| s1.06 (no unnp_pray) | 1292 | **1401** | 19.8 | 32.0 | 2.8 | 0.9 | 6587 | 1701 | 0✱ | 59 | 433 | 7918 |
| s1.07 (no food_full) | 1544 | 564 | 19.1 | 31.7 | 4.4 | 3.7 | 2421 | 1939 | 171 | 67 | 280 | 9039 |
| s1.08 (no food_scale) | 1277 | 640 | 20.0 | 29.9 | 17.5 | 12.3 | 4121 | 1802 | 292 | 78 | 250 | 8333 |
| s1.09 (no pot_full) | 1544 | 564 | 19.1 | 31.7 | 4.4 | 3.7 | 2421 | 1939 | 171 | 67 | 280 | 9039 |
| s1.10 (no pot_scale) | 1192 | 244 | 20.0 | 29.6 | 16.1 | 6.1 | 4307 | 1634 | 315 | 85 | 241 | 7539 |

✱ The `*_fires` counters are reward-gated (only incremented when the
   reward parameter is non-zero), so 0 here means "the metric is
   uninformative for this ablation," not "the underlying event never
   happened." Use the underlying counts (`food_wasted`, `prayer_switches`,
   etc.) for behavioral comparisons in those rows.

### Observations

#### Headline takeaways

1. **`shape_pot_full_waste_penalty` is byte-identical to baseline.**
   s1.09 produced literally the same numbers as v28.8 to floating-point
   precision on every metric — `jad_kill_rate`, `loss/entropy`,
   `food_wasted`, `pots_wasted`, `episode_length`, `reached_wave_63`.
   Trajectory diverges only after step 1.32B (sample 630 / 1430), and the
   divergence is tiny (~1e-4 in jkr). **The penalty literally never
   fires once during training.** Drop unconditionally — it has been
   carrying zero signal since at least v28.4.

2. **`shape_food_full_waste_penalty` is also effectively inert.** s1.07
   matches v28.8 to ≥4 decimals on every headline metric. `loss/entropy`
   diverges by 0.001, indicating the full-waste case fires very rarely
   (a handful of times across 3B steps) but never enough to perturb the
   policy meaningfully. Drop.

3. **`w_npc_kill` is over-penalizing — the largest signal in this
   sweep.** Removing it improved peak +0.21 (0.591 → 0.800, +35%
   relative) and best-250M +0.24 (0.316 → 0.552, +75%). This is not
   noise — it dwarfs the v29.0 noise-floor of ±0.001 by two orders of
   magnitude. Caveat: stability got *worse* (0.29 → 0.17), and final
   `jad_kill_rate` dropped 0.06. Profile is "climbs higher, falls
   harder" — the same shape we saw with v29.1 (PER off). Strongly
   suggests `w_npc_kill = 3.5` is distracting the policy from the
   terminal jad reward during the late-stage convergence phase. Worth
   a follow-up ramp sweep (3.5 → 1.75 → 0.875 → 0) before dropping
   outright, OR a long-budget run at 0 to see if the late-stage
   collapse reflects under-trained late-game rather than reward
   misspec.

4. **`shape_wasted_attack_penalty` is mildly over-penalizing.** Peak
   +0.04 and b250M +0.11 are both above the 0.03 OVER threshold, but
   final dropped 0.11. Same "climbs higher, falls harder" shape as
   s1.01 but smaller magnitude. Recommended: include in Sweep 3
   reward magnitude search (range [0, -0.1]) rather than dropping.

5. **`shape_unnecessary_prayer_penalty` is critical.** s1.06 collapsed
   to peak 0.014 with `prayer_switches = 1401` (2.5× baseline of 564).
   Without this penalty the agent flicks prayers rapidly,
   draining prayer points and breaking the prayer-uptime discipline
   that wave 63 / Jad require. Keep at -0.2 as-is.

6. **Proportional-waste penalties (`shape_food_waste_scale`,
   `shape_pot_waste_scale`) are load-bearing but interacting.**
   - s1.08 (no food_scale): peak -0.18 BUT final +0.07 (best final
     across the entire sweep at 0.297) and stability +0.22 (0.29 →
     0.51). Same "wastes more food" behavior (food_wasted 4.4 → 17.5)
     but somehow the stability profile is *better*.
   - s1.10 (no pot_scale): peak -0.31, final -0.20 (catastrophic),
     food_wasted 4.4 → 16.1 (cross-resource leak — agent gives up on
     food discipline too).

   The asymmetry is informative: removing the food penalty doesn't
   destabilize prayer discipline, but removing the pot penalty does
   (because pots gate prayer uptime, which gates surviving Jad).
   Keep both, but `shape_food_waste_scale = -1.2` may be over-tuned —
   include in Sweep 3 with a softer floor.

7. **Prayer-correctness penalty `shape_wrong_prayer_penalty` is the
   weakest of the load-bearing rewards.** s1.05 dropped peak by only
   -0.08 (vs -0.23 to -0.58 for the strong load-bearers). Final
   dropped only -0.02. The behavior signature (`no_prayer_hits` 67 → 93,
   `wrong_prayer_hits` 280 → 342) shows the agent *does* get sloppier
   without the explicit penalty, but the damage-taken signal partially
   compensates. Keep at -0.3 but flag for Sweep 3 magnitude search —
   it may be reducible.

8. **`w_invalid_action` and `w_tick_penalty` are hard-load-bearing.**
   s1.02 (no invalid): peak -0.31, final -0.14. The agent learns to
   spam invalid actions (food=20.0 max, food_wasted 4.4 → 17.7 since
   eat-when-full now incurs no procedural penalty). s1.03 (no tick):
   peak -0.23, but interestingly stability +0.28. The tick penalty
   acts as an episode-length pressure; without it the agent stalls
   more (peak step pushed to 2.52B vs 1.80B baseline).

#### Recommended action for Sweep 2/3 config

**Drop unconditionally (confirmed inert):**
- `shape_food_full_waste_penalty` (was -6.5 → 0)
- `shape_pot_full_waste_penalty` (was -6.5 → 0)

**Drop with confirmation run (high-leverage but mixed):**
- `w_npc_kill` (was 3.5) — schedule a confirmation run at 0 with
  longer late-stage tail to see if stability recovers, OR include in
  Sweep 3 with a ramp [0, 1.0, 2.0, 3.5].

**Sweep magnitude in Sweep 3 (over-tuned candidate):**
- `shape_wasted_attack_penalty` — currently -0.1, peak/b250M improved
  with 0; sweep [-0.1, 0] linearly.
- `shape_food_waste_scale` — currently -1.2, mixed signal (peak down,
  final + stability up); sweep [-1.5, -0.3] log.
- `shape_wrong_prayer_penalty` — currently -0.3, mild load-bearing;
  sweep [-0.5, -0.05] log.

**Keep at current values (load-bearing, not over-tuned):**
- `w_invalid_action = -0.1`
- `w_tick_penalty = -0.005`
- `shape_unnecessary_prayer_penalty = -0.2`
- `shape_pot_waste_scale = -1.2`

#### What this changes for Sweep 2

Sweep 2 (non-reward hparam sweep) should run with a **reduced reward
config**: drop the two confirmed-inert keys before starting. This
also slightly shrinks the env's reward-component variance, which
should make the optimizer's job easier.

The `w_npc_kill` finding is the more actionable one for Sweep 3 —
the "climbs higher, falls harder" signature mirrors v29.1 (PER off)
exactly, suggesting both knobs are in the same regime: the v28.x
baseline is over-regularized at the cost of peak. We may want to
revisit Sweep 2 ordering — try the v28.8-minus-w_npc_kill config as
the new baseline for Sweep 2 instead of pure v28.8.

---

## v30.0 — confirm full-waste penalties are inert (`k27sckzi` / summer-dawn-345) — ✅ CONFIRMED

Two-variable change from v28.8:

- `shape_food_full_waste_penalty`: -6.5 → 0
- `shape_pot_full_waste_penalty`:  -6.5 → 0

All other env / train / policy hparams identical to v28.8 baseline
(`prio_alpha = 0.8` default kept — this is a v28.8-pedigree run, not a
v29.1-pedigree run, so deltas remain comparable to the Sweep 1 table).

### Reason for the diffs

v28.8 Sweep 1 (above) tested each full-waste penalty in isolation:

- **s1.07** (zero `shape_food_full_waste_penalty`): matched v28.8 to ≥4
  decimals on every headline metric. `loss/entropy` differed by ~0.001,
  indicating the full-waste case fires only a handful of times across
  3B steps and never enough to perturb the policy meaningfully.
- **s1.09** (zero `shape_pot_full_waste_penalty`): **byte-identical** to
  v28.8 — even `loss/entropy` matched to 6 decimals. The penalty
  literally never fires once during training.

Both penalties gate on a niche edge case (consume at full HP / full
prayer). The agent learns to avoid it via the proportional waste
shaping (`shape_food_waste_scale`, `shape_pot_waste_scale`) plus
opportunity cost. The full-waste trip-wires were dead code.

This run zeros both at once to confirm there's no interaction effect
that holds either penalty up. If v30.0 also matches v28.8, both keys
are safe to drop unconditionally before Sweep 2.

### Expected outcome

Predicted result: within `loss/entropy` noise of v28.8 on every
headline metric:

- peak `jad_kill_rate` ≈ 0.591 (±0.001)
- peak step ≈ 1.80B
- best 250M ≈ 0.316 (±0.001)
- final `jad_kill_rate` ≈ 0.223 (±0.001)
- L10 mean ≈ 0.174
- stability ≈ 0.29
- final `loss/entropy` ≈ 1.63 (may differ by 0.001–0.005 due to rare
  food-full-waste fires, same as s1.07's deviation)

Any headline-metric divergence > 0.01 falsifies the "drop both"
recommendation and means there's an interaction effect we missed —
in which case we keep them both and re-investigate.

### Config diff (vs v28.8)

```diff
 [env]
 ...
-shape_food_full_waste_penalty = -6.5
+shape_food_full_waste_penalty = 0
 shape_food_waste_scale = -1.2
-shape_pot_full_waste_penalty = -6.5
+shape_pot_full_waste_penalty = 0
 shape_pot_waste_scale = -1.2
 ...
```

Everything else (all 24 other env keys, all 11 train keys, both
policy keys, both vec keys) is byte-identical to v28.8 / s1.09.

### Reference config

Stored at `runescape-rl/config/fight_caves_v30_0.ini`. Diff against
`fight_caves_s1_09.ini` is exactly the `shape_food_full_waste_penalty`
flip (-6.5 → 0) plus the comment block. Verified.

### Launch

```bash
WANDB_TAG=v30.0 CONFIG_PATH=/home/joe/projects/runescape-rl/claude/runescape-rl/config/fight_caves_v30_0.ini bash /home/joe/projects/runescape-rl/claude/runescape-rl/train.sh
```

`train.sh` syncs `CONFIG_PATH` into `$PUFFER_DIR/config/fight_caves.ini`
on every run (line 30), so the override propagates to pufferlib's
config loader without any further changes. The wandb tag is forwarded
via `--tag $WANDB_TAG` (line 94-96).

3B steps, ~26 min wall on the local box (matching the s1.NN per-run
budget).

### Results

Wandb run `k27sckzi` / `summer-dawn-345`, state `finished`, runtime
26.3 min, 2861 epochs (full 3B-step budget).

**Bottom line — prediction held exactly. v30.0 is functionally identical
to v28.8 on every measured metric.** Final-state values match to 10
decimals (the limit of wandb's float serialization). Both full-waste
penalties confirmed safe to drop unconditionally.

#### Headline metrics (full precision)

| metric | v28.8 | v30.0 | Δ |
|---|---|---|---|
| peak `jad_kill_rate` | 0.591078 | 0.591078 | **+0.000000** |
| peak step | 1.804B | 1.804B | 0 |
| best 250M window | 0.316194 | 0.316398 | +0.000204 |
| final `jad_kill_rate` | 0.2232142836 | 0.2232142836 | **0** (10 decimals) |
| final `reached_wave_63` | 0.9732142687 | 0.9732142687 | **0** (10 decimals) |
| final `wave_reached` | 62.8620 | 62.8620 | 0 |
| final `loss/entropy` | 1.6289957762 | 1.6289957762 | **0** (10 decimals) |
| L10 mean jad | 0.174097 | 0.174076 | -0.000021 |
| stability (L10/peak) | 0.295 | 0.295 | -0.000 |

#### Full-precision summary comparison (every metric byte-identical)

| metric | v28.8 | v30.0 |
|---|---|---|
| `env/jad_kill_rate` | 0.2232142836 | **0.2232142836** |
| `loss/entropy` | 1.6289957762 | **1.6289957762** |
| `env/food_wasted` | 4.4017858505 | **4.4017858505** |
| `env/pots_wasted` | 3.6696429253 | **3.6696429253** |
| `env/episode_length` | 9039.0898 | **9039.0898** |
| `env/correct_prayer` | 1543.8036 | **1543.8036** |
| `env/prayer_switches` | 563.6250 | **563.6250** |
| `env/food_eaten` | 19.1071434021 | **19.1071434021** |
| `env/pots_used` | 31.6964282990 | **31.6964282990** |
| `env/dmg_taken_avg` | 8183.0712890625 | **8183.0712890625** |
| `env/damage_blocked` | 184802.234375 | **184802.234375** |

**Every single behavioral metric matches to floating-point precision.**

#### Trajectory divergence

v30.0 first diverges from v28.8 at sample 643/1430 (step 1.35B) — the
same divergence point as s1.07 (food_full_waste alone). The divergence
is in `loss/entropy` only, magnitude ~1e-3, and **the policy
re-converges to v28.8's exact endpoint**. Notably, even though s1.07
alone ended at `loss/entropy = 1.630184` (vs v28.8's 1.628996), v30.0
ended at v28.8's exact value 1.628996. The food-full-waste fires that
caused s1.07's tiny drift happen to compose with the (zero) pot-full-waste
gradient in a way that lands the policy on the same convergence point.

#### Cross-comparison with Sweep 1 single ablations

| run | reward(s) zeroed | final jkr | final ent | final food_wst | final pots_wst | trajectory divergence |
|---|---|---|---|---|---|---|
| v28.8 | (none) | 0.2232142836 | 1.6289957762 | 4.4018 | 3.6696 | — |
| s1.07 | food_full only | 0.2232142836 | 1.6301844120 | 4.4018 | 3.6696 | step 1.35B |
| s1.09 | pot_full only | 0.2232142836 | 1.6289957762 | 4.4018 | 3.6696 | step 1.32B |
| **v30.0** | **both** | **0.2232142836** | **1.6289957762** | **4.4018** | **3.6696** | step 1.35B |

### Observations

#### What this proves

1. **No interaction effect.** Removing both full-waste penalties together
   produces the same outcome as removing each individually — and that
   outcome is byte-identical to v28.8. The two penalties are
   independently inert and remain inert in combination.
2. **Both keys are confirmed dead code in the trained policy.**
   `shape_food_full_waste_penalty` fires occasionally during late-stage
   exploration (perturbing entropy by ~1e-3) but contributes no
   gradient signal that changes any deployment metric.
   `shape_pot_full_waste_penalty` literally never fires.
3. **The proportional-waste shaping (`shape_food_waste_scale`,
   `shape_pot_waste_scale`) is sufficient to teach consumption
   discipline without the trip-wire penalty.** The agent never needs
   to be told "don't eat at full HP" because the proportional penalty
   plus opportunity cost already fully shapes the behavior.

#### Action items

- ✅ **Drop `shape_food_full_waste_penalty` from the canonical config.**
- ✅ **Drop `shape_pot_full_waste_penalty` from the canonical config.**
- The reward struct in `fc-core/include/fc_reward.h` (lines 24, 26,
  160, 162, 265, 280) can either keep the keys defaulted at 0 or
  remove the conditional branches entirely. Suggest: keep the struct
  fields (so `fight_caves.ini` files in the wild still parse cleanly)
  but drop the `*_full_waste_penalty` lines from all current configs
  to reduce search-space confusion in Sweep 2/3.

#### Promote v30.0 to baseline?

v30.0 is *functionally identical* to v28.8, not "better than." There's
no reason to prefer it as a deployment artifact — it's literally the
same policy. But for **future sweep baselines** (Sweep 2, Sweep 3,
v30.x diagnostic runs), v30.0's config is preferred because:

- Two fewer hparams in the search space.
- Two fewer reward channels in the wandb summary (cleaner dashboards).
- One fewer interaction-effect risk to worry about.

Recommendation: **adopt v30.0's config as the canonical Sweep 2 baseline.**

### Backend audit + cleanup (2026-04-25)

The v30.0 byte-identical result raised a red flag: if zeroing both
penalties produces the *exact same* final entropy as v28.8, maybe the
keys were never actually wired into the reward function. Code audit
traced the full pipeline:

| Layer | File:line | Status |
|---|---|---|
| INI parse | `pufferlib_4` ConfigParser reads `[env]` | ✓ |
| kwargs → env struct | `fc-training/binding.c:49-50, 53-54` | ✓ both keys read |
| env → params | `fc-training/fight_caves.h:197,199` | ✓ both copied |
| params → reward function | `fc-core/include/fc_reward.h:265,280` | ✓ both branches reference params |

**Plumbing was correct end-to-end.** The values DID reach the reward
function. They just guarded a precondition (`pre_hp >= max_hp` /
`pre_prayer >= max_prayer`) that the rest of the system **architecturally
prevents from ever being true**.

The "consume at full HP/prayer" precondition is filtered at three
independent layers before reward computation:

1. **Action mask** (`fc_state.c:518-528` calling `eat_action_valid`/
   `drink_action_valid`): the eat/drink slot is masked to 0 in the
   action mask whenever HP/prayer is at max. The policy literally
   cannot sample these actions when at full state.
2. **Invalid-action detection** (`fc_state.c:481-490` →
   `fc_tick.c:125`): even if the policy somehow sampled a masked
   slot, `fc_action_attempt_is_invalid` returns TRUE, triggering
   `w_invalid_action = -0.1`, and the eat/drink handler is skipped.
3. **Eat/drink handler short-circuit** (`fc_tick.c:137-138`,
   `172-173`): the handler itself has a redundant `current_hp <
   max_hp` / `current_prayer < max_prayer` guard. `state->pre_eat_hp`
   and `state->food_used_this_tick` are only assigned inside this
   guarded block.

The reward branch is then dead in two compounding ways:
- The outer `if (FOOD_USED > 0.0f)` is FALSE if the handler short-
  circuited (FOOD_USED was never set).
- Even if FOOD_USED were somehow set, `pre_eat_hp` would still be 0
  (the per-tick reset value at `fc_tick.c:52`), making `pre_hp >=
  max_hp` false anyway.

Both penalty branches have been **architecturally unreachable since
the action mask was first wired up**. They were defensive code
guarding an impossibility.

#### Cleanup applied

Removed all references to `shape_food_full_waste_penalty` and
`shape_pot_full_waste_penalty` from live code/configs:

- `fc-core/include/fc_reward.h` — removed 2 struct fields, 2 default
  values, 2 unreachable `if (pre_hp >= max_hp)` branches; reward
  function collapsed to just the proportional case.
- `fc-training/fight_caves.h` — removed 2 env-struct fields, 2
  env→params copies.
- `fc-training/fight_caves.c` — removed 2 default-init lines.
- `fc-training/binding.c` — removed 2 kwargs reads.
- `fc-viewer/src/viewer.c` — removed 2 INI parser entries.
- `runescape-rl/config/fight_caves.ini` — removed 2 keys.
- `runescape-rl/config/fight_caves_v30_0.ini` — removed 2 keys
  (now identical to v28.8 baseline) + updated comment block.
- Historical configs (`fight_caves_s1_*.ini`, `fight_caves_v29_*.ini`,
  snapshots) left intact — the dead keys are now harmlessly ignored
  by `binding.c`.

#### Cleanup verification rerun (`bqrdh8hc` / quiet-mountain-346)

Re-ran v30.0 with the exact same launch command after the cleanup
to verify nothing changed:

```bash
WANDB_TAG=v30.0 CONFIG_PATH=…/fight_caves_v30_0.ini bash …/train.sh
```

State `finished`, runtime 25.2 min, 2861 epochs.

**Every deployment metric matches v28.8 and the original v30.0 byte-
identically:**

| metric | v28.8 | v30.0 orig (`k27sckzi`) | **v30.0 rerun** (`bqrdh8hc`) |
|---|---|---|---|
| peak `jad_kill_rate` | 0.591078 | 0.591078 | **0.591078** |
| peak step | 1.804B | 1.804B | **1.804B** |
| final `jad_kill_rate` | 0.2232142836 | 0.2232142836 | **0.2232142836** |
| final `wave_reached` | 62.8619613647 | 62.8619613647 | **62.8619613647** |
| `reached_wave_63` | 0.9732142687 | 0.9732142687 | **0.9732142687** |
| `food_eaten` | 19.1071434021 | 19.1071434021 | **19.1071434021** |
| `food_wasted` | 4.4017858505 | 4.4017858505 | **4.4017858505** |
| `pots_used` | 31.6964282990 | 31.6964282990 | **31.6964282990** |
| `pots_wasted` | 3.6696429253 | 3.6696429253 | **3.6696429253** |
| `correct_prayer` | 1543.8036 | 1543.8036 | **1543.8036** |
| `prayer_switches` | 563.6250 | 563.6250 | **563.6250** |
| `dmg_taken_avg` | 8183.0713 | 8183.0713 | **8183.0713** |
| `episode_length` | 9039.0898 | 9039.0898 | **9039.0898** |

Two metrics drift within the documented CUDA scheduling noise floor:

| metric | v28.8 | v30.0 orig | v30.0 rerun |
|---|---|---|---|
| `loss/entropy` | 1.6289957762 | 1.6289957762 | 1.6301844120 (Δ +0.0012) |
| `damage_blocked` | 184802.234 | 184802.234 | 184802.266 (Δ +0.03 of 185k) |

The 0.0012 entropy drift is the *exact same* value s1.07 ended at —
this is the documented late-trajectory CUDA-scheduling noise floor
(see v29.0 finding: "first 998 of 1430 epochs byte-identical, remainder
diverge slightly due to CUDA-level kernel scheduling but converge to
matching final values"). Trajectory diverges at sample 640 (step
1.35B) — same neighborhood as s1.07 (sample 643).

**Conclusion:** the cleanup is verified safe. Removing the dead struct
fields, defaults, kwargs reads, and reward branches produced zero
behavioral effect. Every metric measuring agent behavior matches
byte-identically; the only differences are within the floating-point
noise floor that exists between byte-identical reruns of the same
config.

---

## OBS Sweep / Ablation (2026-04-26/27 — 3 runs, complete)

### Purpose

Test whether features that are *theoretically* derivable from the rest of the
obs are actually doing work for the policy, or whether the recurrent net
(MinGRU, 2 layers × 256 hidden) is silently re-deriving them. This is a
leave-one-out ablation on the **observation side** — analogous to the v28.8
Sweep 1 reward-LOO ablation, but applied to the policy input vector instead
of the reward weights.

If a candidate feature is cut and final `jad_kill_rate` lands within noise
of the v30.0 / v28.8 baseline, the feature is a safe permanent removal:
- smaller policy input → faster forward pass → modest SPS bump
- cleaner obs contract (fewer fields to maintain in
  `fc_contracts.h` and `fc_state.c::fc_write_obs`)
- no asymptotic perf cost (the agent didn't need the feature)

If a candidate feature is cut and `jad_kill_rate` regresses meaningfully,
the feature **was** load-bearing — the GRU was not re-deriving it during
training, and we keep it.

### Baseline

**v30.0** (`bqrdh8hc`) — same reward config as v28.8 (the deployment SOTA
reward set), `prio_alpha=1.0` PER on, full obs, 3B steps.

| Baseline metric | v30.0 = v28.8 |
|---|---|
| peak `jad_kill_rate` | 0.591 |
| best 250M window | 0.316 |
| final `jad_kill_rate` | 0.223 |
| final `reached_wave_63` | 0.973 |

No new baseline run is needed — `bqrdh8hc` was just verified byte-identical
to v28.8 in the v30.0 cleanup-verification rerun.

### Method

3 runs, each diffs from the v30.0 baseline by exactly one observation
ablation flag set to 1. All other config (reward weights, PPO hparams,
policy size, vec settings, total_timesteps=3B) held identical to v30.0.

Each run launches via `sweep_obs.sh` with a self-describing `WANDB_TAG` so
the ablation is identifiable in W&B without cross-referencing docs:

| Variant | wandb tag |
|---|---|
| obs.0 | `obs.0_no_distance` |
| obs.1 | `obs.1_no_aggregates` |
| obs.2 | `obs.2_no_valid` |

Tag flow: `WANDB_TAG` env var → `train.sh --tag` → `pufferl.py:246`
`wandb.init(tags=[args['tag']])`. Single tag per run. Sequential,
~26 min per run, ~80 min total.

### The 3 runs

| Run | What it ablates | # floats zeroed | Reason it's a candidate |
|---|---|---|---|
| **obs.0** | `NPC_DISTANCE` (offset 4 within each NPC stride, 8 slots) | 8 | Pure mathematical redundancy: `chebyshev(player_xy, npc_xy)` is computable from `PLAYER_X/Y` + `NPC_X/Y`, all of which the policy already gets. The MinGRU should be able to re-derive distance trivially. |
| **obs.1** | The 9 incoming-hit aggregate counts: `PLAYER_IN_{MEL,RNG,MAG}_1T`, `PLAYER_IN_{MEL,RNG,MAG}_2T`, `META_IN_{MEL,RNG,MAG}_3T` | 9 | These are arena-wide style/timing summaries derived from pending-hit timelines. The per-NPC `NPC_PENDING_STYLE` + `NPC_PENDING_TICKS` already give the same info for the 8 visible NPCs. The aggregates only add new signal for overflow NPCs (slots 9-16, rare in practice — usually only late waves) and for NPCs with multiple simultaneous pending hits (also rare — `fc_state.c:424` `break;` only reports the first per NPC). |
| **obs.2** | `NPC_VALID` (offset 0 within each NPC stride, 8 slots) | 8 | Empty NPC slots already have all other features zeroed by `memset` (`fc_state.c:346`) plus the gather/sort step only places live NPCs. The agent can detect "empty slot" from `NPC_HP == 0` or `NPC_DISTANCE == 0`. The flag is just an explicit gate that's already implicit. |

Total cuttable obs across all three (if all flat): **25 of 122 floats (~20%)**
without losing real signal. Of the 25, the 9 incoming-aggregates is the
biggest single block.

### Backend wiring (single source of truth)

The zeroing logic lives in fc-core and is shared by the trainer and the
viewer (so eval-replay sees the same obs distribution training did):

```
fc-core/src/fc_state.c:
    void fc_apply_obs_ablation(float* out,
                               int ablate_npc_distance,
                               int ablate_incoming_aggregates,
                               int ablate_npc_valid);
    — zeroes the indices listed above; no-op when all flags are 0.

fc-training/fight_caves.h::fc_puffer_write_obs:
    fc_write_obs(...);
    fc_apply_obs_ablation(obs, env->obs_ablate_*);   ← applied AFTER fc_write_obs

fc-training/binding.c:
    reads kwargs `obs_ablate_npc_distance / _incoming_aggregates / _npc_valid`
    (default 0) into FightCaves.obs_ablate_*.

fc-viewer/src/viewer.c:
    parses the same 3 keys into ViewerState.obs_ablate_*; calls
    fc_apply_obs_ablation in write_obs_to_pipe so eval_viewer policy replay
    sees the same obs distribution the policy was trained on.
```

The zeroing is applied **after** the full obs is written, leaving reward
features (offsets 122-140) and the action mask (offsets 158-193) untouched.

### Variant configs (vs v30.0)

```
config/fight_caves_obs0.ini   diff: obs_ablate_npc_distance = 1
config/fight_caves_obs1.ini   diff: obs_ablate_incoming_aggregates = 1
config/fight_caves_obs2.ini   diff: obs_ablate_npc_valid = 1
```

Everything else byte-identical to `fight_caves_v30_0.ini`.

### Launch

```bash
bash sweep_obs.sh           # all 3 in order
bash sweep_obs.sh 1         # only obs.1
bash sweep_obs.sh 0 2       # obs.0 then obs.2
```

### Expected outcome

Based on theory:
- **obs.0** (NPC_DISTANCE): likely flat (within ±3-5%) — trivially derivable.
- **obs.1** (aggregates): likely flat or small regression (-1 to -3% peak) —
  the per-NPC pending fields already cover the visible 8 NPCs.
- **obs.2** (NPC_VALID): likely flat — empty slots are already zero in every
  channel.

Caveat: single-seed variance. v29.x sweep runs swung 5-10% on `jad_kill_rate`
across single-knob changes. Use **best-250M-window** (more stable than peak)
as the primary judgment metric. Anything within ±5% of v30.0's 0.316 is
"no signal." Treat clear regressions (>10%) as "feature was load-bearing,
keep it."

### Results / Observations

All 3 runs completed back-to-back over ~80 min wall time (2026-04-27 01:45 → 02:39 UTC). Backend recompiled cleanly per variant; no crashes; tags applied correctly. SPS held at ~1.82-1.86M (vs baseline 1.93M — small overhead from the in-place zeroing pass, ~3-5%).

#### Run inventory

| Variant | wandb id | wandb name | tag | runtime | SPS |
|---|---|---|---|---|---|
| baseline (v30.0) | `bqrdh8hc` | quiet-mountain-346 | `v30.0` | 25.5m | 1.93M |
| obs.0 — no NPC_DISTANCE | `5uyuib9g` | dauntless-brook-347 | `obs.0_no_distance` | 27.2m | 1.86M |
| obs.1 — no incoming aggregates | `6g2i2gmy` | hearty-field-348 | `obs.1_no_aggregates` | 26.3m | 1.87M |
| obs.2 — no NPC_VALID | `2hvj8kkm` | solar-plasma-349 | `obs.2_no_valid` | 25.9m | 1.82M |

#### Headline cross-run table (vs v30.0 baseline)

| Metric | baseline `bqrdh8hc` | obs.0 `5uyuib9g` | obs.1 `6g2i2gmy` | obs.2 `2hvj8kkm` |
|---|---|---|---|---|
| peak `jad_kill_rate` | 0.591 | 0.354 (**-40.1%**) | 0.702 (**+18.8%**) | 0.081 (**-86.3%**) |
| peak step | 1.80B | 1.76B | 1.22B (-580M, -32%) | 3.00B (never peaked) |
| best 250M window | 0.316 | 0.153 (-51.7%) | 0.348 (**+10.0%**) | 0.014 (-95.5%) |
| last-10% mean (stability proxy) | 0.174 | 0.095 (-45%) | 0.244 (**+40%**) | 0.012 (-93%) |
| last-10% / peak (stability ratio) | 0.295 | 0.267 | 0.347 | 0.151 |
| final `jad_kill_rate` | 0.223 | 0.137 (-39%) | 0.174 (-22%) | 0.081 (-64%) |
| final `reached_wave_63` | 0.973 | 0.832 (-14.5%) | 0.833 (-14.4%) | 0.559 (-43%) |
| final `correct_jad_prayer_fires` | 85.6 | 11.7 (-86%) | 73.5 (-14%) | 2.7 (-97%) |
| final `wrong_prayer_hits` | 280 | 334 (+19%) | 293 (+5%) | 368 (+31%) |

**Verdict matrix** (judged primarily on best-250M and last-10% mean, with stability ratio as tiebreaker; single-final-point treated as noise):

| Run | Outcome | Verdict |
|---|---|---|
| obs.0 | major regression across every metric | **NPC_DISTANCE is load-bearing — keep** |
| obs.1 | better peak, better best-250M, better last-10% mean, better stability ratio | **incoming aggregates likely net-noise — strong candidate for permanent removal** |
| obs.2 | catastrophic — agent failed to reach wave 63 until ~2.5B and never killed Jad above 0.081 | **NPC_VALID is critical — keep** |

#### Per-run deep dive

##### obs.0 — ablate NPC_DISTANCE (`5uyuib9g`) — ❌ MAJOR REGRESSION

Counterintuitive: `NPC_DISTANCE` is mathematically derivable from `PLAYER_X/Y` + `NPC_X/Y`, all of which the policy still has. We hypothesised the GRU would re-derive `chebyshev(player_xy, npc_xy)` trivially. **The training data says it does not.** Removing the explicit distance feature regressed every single metric.

Trajectory:
| Step | reach63 | jad_kill_rate | wave_reached | episode_length |
|---|---|---|---|---|
| 0.5B | 0.000 | 0.000 | 54.6 (vs base 47.5 — early **lead**) | 6683 (vs base 5406) |
| 1.0B | 0.459 (vs base 0.000 — **early reach63 lead**) | 0.017 | 61.6 | 8259 |
| 1.5B | 0.533 | 0.044 | 60.3 | 8046 |
| 2.0B | 0.429 | 0.028 | 60.9 | 8127 |
| 2.5B | 0.441 | 0.066 | 61.7 | 8260 |
| 3.0B | 0.832 | 0.137 | 62.2 | 8281 |

Pattern: **early-game advantage, late-game collapse on Jad-specific behavior**.
- The agent without distance learned wave-reach faster than baseline (reach63=0.459 at 1.0B vs baseline 0.000) — so basic positioning relative to safespot didn't need explicit distance.
- But `correct_jad_prayer_fires` final = 11.7 (vs baseline 85.6, **-86%**). The agent doesn't pray correctly during Jad — Jad's tactical loop (prayer flick + range/LOS check + safespot positioning) all depend on tight distance reasoning that the GRU couldn't reconstruct.
- `dmg_taken_avg` worse: 9030 (vs baseline 8183, +10%).
- Lesson: theoretical derivability ≠ practical redundancy. The GRU learns features, not arithmetic. NPC_DISTANCE is doing real work.

##### obs.1 — ablate the 9 incoming-hit aggregates (`6g2i2gmy`) — ✅ NET IMPROVEMENT (likely)

The standout result. Removing `PLAYER_IN_{MEL,RNG,MAG}_{1T,2T}` + `META_IN_{MEL,RNG,MAG}_3T` (9 floats) **improved** training across multiple primary metrics:

| Metric | Baseline | obs.1 | Δ |
|---|---|---|---|
| peak `jad_kill_rate` | 0.591 | **0.702** | **+18.8%** |
| peak step | 1.80B | **1.22B** | -580M (-32% compute) |
| best 250M window | 0.316 | **0.348** | +10.0% |
| last-10% mean | 0.174 | **0.244** | +40% |
| stability ratio | 0.295 | **0.347** | +18% |
| final point (3B) | 0.223 | 0.174 | -22% (single-point noise) |

Trajectory: peaks earlier and higher than any v28.x/v29.x run we have on record. Hits 0.702 jad_kill_rate at **1.22B**, then crashes to ~0.008 at 1.78B (similar crash pattern to baseline at 2.0B), then recovers steadily through 3B.

Why this might be real signal, not just noise:
- Multiple primary metrics moved in the same direction (peak, best-250M, last-10% mean, stability)
- The peak is **+18.8%** — well outside our v29.x ±5% noise floor
- The peak comes 580M steps earlier (32% compute reduction at the new peak)
- Theory matches: the per-NPC `NPC_PENDING_STYLE` + `NPC_PENDING_TICKS` (16 floats across 8 slots) already cover the visible NPCs, so the 9 arena-wide aggregate counts were largely duplicative. With aggregates removed, the policy may have stopped spending GRU capacity on a redundant signal.
- One alternative explanation worth flagging: the aggregates included **3T-window** info that the per-NPC fields don't (per-NPC has full ticks-remaining timer, but in normalized form / 10). The 3T aggregate is a coarser, longer-horizon summary. It's possible the agent was mildly **misled** by the 3T look-ahead at the cost of 1T/2T precision.

Caveat: single-seed result. Before declaring victory, this should ideally be reproduced with a 2nd seed (cheap — 26 min) to rule out lucky variance.

##### obs.2 — ablate NPC_VALID (`2hvj8kkm`) — ❌ CATASTROPHIC COLLAPSE

Worst outcome of the sweep. Agent essentially fails to learn the task — `jad_kill_rate` is **0.000 through 2.5B steps**, only beginning a slow climb to 0.081 by 3B. `reached_wave_63` is 0.000 through 2.0B and barely reaches 0.559 at 3B (vs baseline 0.973).

| Step | reach63 | jad_kill_rate | wave_reached |
|---|---|---|---|
| 0.5B-2.0B | 0.000 | 0.000 | 45-51 (stuck) |
| 2.5B | 0.003 | 0.000 | 49.6 |
| 3.0B | 0.559 | 0.081 | 61.95 |

Why this was wrong-headed: I argued NPC_VALID was redundant with HP/distance/etc. being zero in empty slots. **It is not.** The actual issue:
- An empty NPC slot has `(VALID, X, Y, HP, DIST, ...) = (0, 0, 0, 0, 0, ...)` after `memset`
- An NPC genuinely standing at tile (0, 0) at the start of the arena would also have `(X, Y) = (0, 0)` (after normalization, near-zero). With a still-occupied slot dimension, the agent could conflate "empty slot" with "real NPC near origin" — only `NPC_VALID = 1` definitively says "this is real."
- `correct_jad_prayer_fires` = 2.7 (vs baseline 85.6, -97%) — the agent never even learns to disambiguate target identity well enough to pray correctly during Jad.
- `pots_wasted` = 6.2 (vs baseline 3.7, +69%) — agent over-pots, suggesting confused state interpretation.

Lesson: NPC_VALID is doing critical disambiguation work that's invisible from the contract docs alone. The "implicit gate via zero values" theory is wrong because zero is a valid coordinate / HP-fraction value. **Keep NPC_VALID.**

#### Cross-cutting conclusions

1. **Theoretical derivability ≠ practical redundancy.** NPC_DISTANCE is mathematically computable from positions, but the GRU does not actually re-derive it during training. We should not assume "obviously redundant" features are safe to cut — we have to test.

2. **NPC_VALID is doing more work than it looks.** The "empty slots are zero in every channel" argument breaks because zero is a valid value for several other features. The explicit gate is necessary.

3. **The 9 incoming-hit aggregates may be net-noise** for the policy. Single-seed evidence, but multiple metrics moved in the same direction with effect sizes well outside the noise floor. Worth a confirmation run on a different seed before permanently removing.

4. **Removing 9 of 122 obs is the most defensible cut so far.** If obs.1 reproduces on a 2nd seed, we can permanently delete the 9 aggregate features — potentially **the cleanest reward-config-frozen improvement we've found post-v28.8** (+18.8% peak, -32% compute to peak).

5. **Stability metric matters.** All four runs have similar `wave_reached` final (~62), but jad_kill_rate diverges 7x. Top-level metrics like wave_reached can hide the actual signal. This is the same lesson as v29.x — Jad is the hardest task and the only one that distinguishes good policies from mediocre ones.

#### Recommended next steps

- **Confirmation seed for obs.1** (cheapest path to a real win). One additional run with `obs_ablate_incoming_aggregates=1` and a different seed_counter starting point. If peak ≥ 0.65 and best-250M ≥ 0.33, declare obs.1 the new SOTA and permanently remove the 9 aggregate fields from `fc_contracts.h` / `fc_state.c`.
- **Do NOT remove NPC_DISTANCE or NPC_VALID** — both regressed sharply.
- **Consider a partial-aggregate ablation** as a follow-up: keep 1T/2T (panic-window panic gauge), drop only 3T aggregates (5 floats kept, 3 cut). The 3T window is the most-likely-confusing feature given that per-NPC PENDING_TICKS already provides a tighter signal.
- **Update obs contract docs** in `fc_contracts.h` to flag these findings near the relevant feature definitions, so future ablation candidates can avoid the same wrong-headed assumptions.

---

## v34_sweep_long — non-reward Protein hparam sweep (2026-04-30 → 2026-05-03, complete — 200 runs)

### Purpose

Broad Protein (GP-based, Pareto-pruning) sweep over **22 non-reward
hparams** with the reward config frozen at the v32.0 SOTA baseline.
This is the realization of the long-planned "Sweep 2" (non-reward
hparams) — superseding the v28.8 Sweep 2 plan which was never run
because the project pivoted to the v32 reward stack first.

The v32.0 baseline (`k9gsilze`) was the prior SOTA at peak `jad_kill_rate`
0.370 / final 0.112 (`+15.4%` vs the obs.1 prior SOTA). The goal of
v34_sweep_long was to discover whether the optimizer / policy / vec
defaults were miscalibrated for Fight Caves — analogous to the
`prio_alpha=0` finding in v29.1 but covering all 22 swept knobs at once.

**Result preview**: yes, badly miscalibrated. New SOTA is **`a3mi6u2g`
peak 0.886 / final 0.886** — a +139% improvement over v32.0 final and
+9.4% over the prior absolute peak. Two trials broke 0.8, ten broke
0.6, and 51/200 trials are stable+strong (peak ≥0.3, final ≥85% of peak).

### Sweep Config

```ini
[sweep]
method = Protein
metric = jad_kill_rate
metric_distribution = linear
goal = maximize
max_runs = 200
max_suggestion_cost = 3600     # 60-minute Pareto suggestion cap
gpus = 1
downsample = 5                  # 5 (cost, score) buckets per trial
prune_pareto = true             # Pareto front early-stop active
early_stop_quantile = 0.3
use_gpu = true
```

22 sweep dimensions across `[train]`, `[policy]`, `[vec]`. First two
trials replay the `[train]` defaults (baseline-replay seed). Subsequent
trials are GP-suggested from the observed (cost, score) buckets.

### W&B

- Project: `jbailey8531-oakton-college/fight caves rl`
- Group: `v34_sweep_long`
- Total trials: **200** (sweep cap reached)
- Wall time: **68.2 hrs** total / **20.5 min/trial average** / 45.3 min max / 0.2 min min
- Throughput: **median 1.40 M SPS** at full bucket (p25 1.14M, p75 1.59M)

### Baseline-replay integrity check

Trials `09z5sl7u` and `w8543fbr` use the v32.0 hparams verbatim. Both
produced jkr trace `[0.000, 0.014, 0.290, 0.370, 0.112]` — identical
to v32.0's `k9gsilze` byte-for-byte. **Environment, seed, and reward
stack are confirmed frozen across the sweep.** No drift.

### Headline results

| Rank | Run | Peak jkr | Final jkr | Trajectory | Steps | Time |
|------|-----|---------:|----------:|-----------|------:|-----:|
| 🏆 1 | `a3mi6u2g` | **0.886** | **0.886** | `0.00 / 0.32 / 0.53 / 0.78 / 0.89` | 2.03 B | 32 m |
| 🥈 2 | `fpdao4ac` | **0.826** | **0.826** | `0.00 / 0.35 / 0.64 / 0.75 / 0.83` | 2.34 B | 35 m |
| 3 | `fvikxb56` | 0.728 | 0.316 (crashed) | `0.00 / 0.04 / 0.73 / 0.66 / 0.32` | 1.61 B | 24 m |
| 4 | `txt7fvpp` | 0.709 | 0.338 (crashed) | `0.00 / 0.00 / 0.50 / 0.71 / 0.34` | 1.96 B | 24 m |
| 5 | `a7wp0gy2` | 0.684 | 0.684 | `0.00 / 0.21 / 0.39 / 0.53 / 0.68` | 1.99 B | 25 m |
| 6 | `r2v1cmfg` | 0.680 | 0.474 | — | 1.72 B | 18 m |
| 7 | `3ftyvwwy` | 0.667 | 0.667 | — | 2.29 B | 35 m |
| 8 | `bogro0f3` | 0.650 | 0.441 | — | 1.55 B | 17 m |
| 9 | `ua0in6tk` | 0.645 | 0.517 | — | 2.04 B | 22 m |
| 10 | `s9ycbdpp` | 0.644 | 0.644 | — | 1.67 B | 28 m |
| (...) | | | | | | |
| baseline | `k9gsilze` (v32.0) | 0.370 | 0.112 | `0.00 / 0.01 / 0.29 / 0.37 / 0.11` | 3.00 B | 26 m |

### Outcome distribution

```
peak jad_kill_rate                       final jad_kill_rate
[0.00, 0.05):  25                        [0.00, 0.05):  33
[0.05, 0.10):   2                        [0.05, 0.10):  12
[0.10, 0.20):  16                        [0.10, 0.20):  35
[0.20, 0.30):  23                        [0.20, 0.30):  44
[0.30, 0.40):  35                        [0.30, 0.40):  28
[0.40, 0.50):  41                        [0.40, 0.50):  23
[0.50, 0.60):  35                        [0.50, 0.60):  14
[0.60, 0.70):  19                        [0.60, 0.70):   9
[0.70, 0.80):   2                        [0.70, 0.80):   0
[0.80, 1.01):   2                        [0.80, 1.01):   2
```

Median peak across 200 trials: 0.397. Median final: 0.258. Half the
sweep beat v32.0's peak (0.370). 99/200 trials beat v32.0's final
(0.112). The sweep clearly identified a much better hparam basin than
the v32.0 defaults.

### Stability classification

| Class | Definition | Count |
|-------|-----------|------:|
| Strong + stable | peak ≥ 0.3 AND final ≥ 0.85·peak | **51** |
| Crashed | peak ≥ 0.3 AND final < 0.50·peak | 33 |
| Mid-tier | peak ≥ 0.1 (otherwise) | 89 |
| Weak | peak < 0.1 | 27 |

The crash pattern is meaningful: ~40% of high-peak trials regressed
sharply by the final eval. Crashed trials have **higher median gamma
(0.9947 vs 0.9925)** and **higher median vf_coef (0.967 vs 0.945)**
than stable ones — over-aggressive critic + super-long horizon ≈
late-training instability.

### GP convergence over time

Best-so-far progression (by trial index, by file mtime):

```
Trial 1-2:    0.370 (baseline replays establish prior)
Trial 22:     0.465  ← first beat-baseline (13ekvy2i)
Trial 25:     0.547  ← Protein finds gradient (5nn27uh3)
Trial 43:     0.636  ← strong basin (do3vrern)
Trial 86:     0.709  ← txt7fvpp
Trial 101:    0.826  ← fpdao4ac
Trial 128:    0.886  ← a3mi6u2g (NEW SOTA)
Trials 129-200: 0.886 (no further improvement — sweep saturated)
```

Sweep-progression chunks (median peak by chunk of 28 trials):

```
trials   1- 28: median 0.002 (random exploration, mostly 0.00)
trials  29- 56: median 0.251
trials  57- 84: median 0.360
trials  85-112: median 0.496  ← steady-state high-quality region
trials 113-140: median 0.495
trials 141-168: median 0.493
trials 169-200: median 0.416  (slight regression as GP explored periphery)
```

GP narrowing of the lr range (most impactful knob):

```
Chunk 1-25:    [1.1e-04, 8.1e-04], median 3.1e-04
Chunk 26-50:   [1.4e-04, 5.7e-04], median 3.5e-04
Chunk 51-75:   [2.7e-04, 9.0e-04], median 6.1e-04
Chunk 76-100:  [5.0e-04, 9.0e-04], median 8.3e-04
Chunk 101-125: [5.8e-04, 9.0e-04], median 9.0e-04  ← pinned at sweep max
Chunk 126-200: [5.8-6.9e-04, 9.0e-04], median 9.0e-04
```

Protein clearly learned that lr at the sweep max (9e-4) was best, but
the search ceiling = 1e-1 with `log_normal(min=1e-5, max=1e-1, scale=0.5)`,
so the GP only suggested as high as 9e-4. **The actual lr optimum may
lie above 9e-4** — the next sweep should test 5e-4 ≤ lr ≤ 3e-3.

### TOP SOTA #1 — `a3mi6u2g` (peak/final = 0.886)

**Hyperparameters:**

```
[train]
learning_rate    = 9.00e-04   (sweep boundary - max possible)
ent_coef         = 2.42e-02   (high)
gamma            = 0.9963     (in IQR for elite)
gae_lambda       = 0.964      (high but normal)
clip_coef        = 0.178      (lower than default 0.2)
vf_coef          = 1.000      (~2x default 0.5)
vf_clip_coef     = 0.151      (lower than default 0.2)
max_grad_norm    = 0.250      (sweep boundary - min possible, half default)
ent_coef         = 2.42e-02   (~2.4x default 0.01)
beta1            = 0.9500     (default)
beta2            = 0.9996     (very high; default 0.999)
eps              = 1.00e-10   (extremely small; default 1e-12 is even smaller)
prio_alpha       = 0.968      (much higher than default 0.8)
prio_beta0       = 0.000      (sweep boundary - min possible, vs default 0.2)
horizon          = 256        (=default, GP converged)
replay_ratio     = 1.568      (vs default 1.0)
minibatch_size   = 4096       (=default, GP converged)
vtrace_rho_clip  = 0.500      (sweep boundary - min possible, vs default 1.0)
vtrace_c_clip    = 0.504      (sweep boundary - min possible, vs default 1.0)

[policy]
hidden_size      = 256        (=default)
num_layers       = 3          (vs default 2)

[vec]
total_agents     = 4096       (=default, GP converged)
num_buffers      = 2          (=default)
```

**Behavioral metrics (final bucket = 10k-episode eval):**

```
wave_reached         62.87 / 63   ← almost always full clear
most_npcs_slayed     424          ← all NPCs in cave killed
correct_prayer       2212         ← high prayer accuracy
wrong_prayer_hits     26.7        ← was 105 mid-training (massive improvement)
no_prayer_hits        15.4        ← was 56 mid-training
damage_blocked      241,955      ← prayer pressure handled
dmg_taken_avg         1947       ← was 5602 mid-training (3x reduction)
attack_when_ready     0.98       ← attacks every available tick
food_eaten             9.5       ← efficient consumable use
food_wasted            5.07
pots_used             32         ← drinks all pots
pots_wasted           29         ← ⚠️ but most are wasted (timing issue)
avg_hp_on_food         0.79      ← eating at high HP (waste-prone)
```

The policy **kills Jad ~89% of the time** but still wastes consumables.
Pot waste = 29/32 is a real shaping defect, not a measurement artifact.

### TOP SOTA #2 — `fpdao4ac` (peak/final = 0.826)

**Hyperparameters:**

```
lr=9.0e-4  ent=0.0193  gamma=0.9953  gae=0.947  clip=0.174
vf_coef=1.000  vf_clip=0.177  max_grad=0.309
horizon=256  minibatch=4096  replay_ratio=1.460
prio_alpha=0.925  prio_beta0=0.121
vtrace_rho_clip=0.500  vtrace_c_clip=0.500
beta1=0.95  beta2=0.9997  eps=1e-10
hidden=256  layers=3  agents=4096  num_buffers=1
```

Almost identical recipe to `a3mi6u2g`. Differences: slightly lower
ent_coef (0.019 vs 0.024), slightly lower gamma (0.9953 vs 0.9963),
and num_buffers=1 (vs 2). **This is convergent evidence** — Protein
found two near-identical points in hparam space.

### Spearman correlation: hparam vs peak_jkr (across all 200 trials)

```
minibatch_size         -0.95   ← strong signal (caveat below)
horizon                -0.94
total_agents           -0.94
hidden_size            -0.91
vtrace_rho_clip        -0.76
vtrace_c_clip          -0.60
num_buffers            -0.60
eps                    -0.54
prio_beta0             -0.44
vf_coef                +0.39
clip_coef              -0.37
num_layers             -0.34
max_grad_norm          -0.34
vf_clip_coef           -0.33
learning_rate          +0.33
replay_ratio           +0.32
gamma                  -0.25
prio_alpha             -0.08
ent_coef               +0.03
gae_lambda             -0.03
beta2                  -0.01
beta1                  -0.01
```

**Caveat:** the size-related correlations (`minibatch_size`, `horizon`,
`total_agents`, `hidden_size`) are partly inflated by Protein's
GP-driven sampling bias. After trial ~50, the GP almost exclusively
sampled the smallest pow2 in each range (256/4096/4096/256), so larger
values appear in mostly-failing trials. The correlation is real (these
sizes are not optimal here) but the magnitude is overstated.

The actionable signals after correcting for that:
- **vtrace_rho_clip** strongly negative (0.5 dominates 1.0+)
- **vf_coef** strongly positive (1.0 better than default 0.5)
- **prio_beta0** strongly negative (0.0 better than default 0.2)
- **lr** positive (higher is better — 9e-4 > 3e-4)

### Constellation TSNE clustering (sklearn TSNE on 22-D normalized hparam vectors)

KMeans (k=6) on raw normalized hparam space identified 5 functional
basins + 1 launch-failure cluster:

| Cluster | n | peak max | peak mean | Profile |
|---------|---:|---------:|----------:|---------|
| 4 (TOP) | 60 | 0.886 | 0.472 | lr~9e-4, ent~0.022, gamma~0.99-0.997, vf~0.95, vtrace=0.5, prio_alpha=0.97 |
| 0 | 49 | 0.684 | 0.443 | Same as 4 but lower gamma (~0.99 minimum) |
| 2 | 46 | 0.709 | 0.367 | Mid-tier: lower lr, higher ent, more aggressive clip |
| 1 | 21 | 0.547 | 0.197 | Low-lr region (~3e-4), num_layers=4-5 |
| 3 | 19 | 0.465 | 0.131 | Includes baseline-replays, broader mid-range |
| 5 | 5 | 0.000 | 0.000 | Launch failures (5 runs crashed at t=0) |

**Cluster 4 is the elite basin** — 60/200 trials live here and produce
a median peak of 0.487 (vs 0.397 sweep-wide). The basin centroid is the
v35 starting recipe (see below).

### Elite basin centroid (peak ≥ 0.5, n=58 trials) — RECOMMENDED v35 BASE

```
[train]
learning_rate    = 9.00e-04         IQR [7.75e-04, 9.00e-04]   ⚠ pinned at sweep max
ent_coef         = 0.0229           IQR [0.0195, 0.0279]
gamma            = 0.9937           IQR [0.9900, 0.9955]       ⚠ at sweep min
gae_lambda       = 0.9651           IQR [0.9537, 0.9733]
clip_coef        = 0.1658           IQR [0.1538, 0.1821]
vf_coef          = 0.9744           IQR [0.8744, 1.0000]       ⚠ near sweep max
vf_clip_coef     = 0.1770           IQR [0.1444, 0.2080]
max_grad_norm    = 0.3511           IQR [0.2500, 0.5114]       ⚠ near sweep min (0.1)
beta1            = 0.9500           IQR [0.9471, 0.9500]
beta2            = 0.9997           IQR [0.9996, 0.9997]
eps              = 1.00e-10         IQR [1.00e-10, 1.10e-10]
prio_alpha       = 1.000            IQR [0.9449, 1.0000]       ⚠ pinned at sweep max
prio_beta0       = 0.000            IQR [0.000, 0.108]         ⚠ pinned at sweep min
horizon          = 256              GP converged
replay_ratio     = 1.4514           IQR [1.34, 1.58]
minibatch_size   = 4096             GP converged
vtrace_rho_clip  = 0.5000           ⚠ pinned at sweep min
vtrace_c_clip    = 0.5000           IQR [0.5000, 0.5352]       ⚠ pinned at sweep min

[policy]
hidden_size      = 256              GP converged
num_layers       = 2-3              IQR [2, 3]

[vec]
total_agents     = 4096             GP converged
num_buffers      = 1-2              IQR [1, 2]
```

**6 hparams are pinned at sweep boundaries** — Protein wanted to push
further in those directions but couldn't. These are the v35 widening
candidates:

| Knob | Current bound | Hit at | Suggested v35 bound |
|------|---------------|--------|---------------------|
| `learning_rate` | log [1e-5, 1e-1], scale 0.5 → effective max ~9e-4 | max | log [1e-4, 5e-3] |
| `vtrace_rho_clip` | uniform [0.1, 5.0] | 0.5 (effective min) | uniform [0.1, 1.0] |
| `vtrace_c_clip` | uniform [0.1, 5.0] | 0.5 (effective min) | uniform [0.1, 1.0] |
| `max_grad_norm` | uniform [0.1, 5.0] | 0.25 (effective min) | uniform [0.1, 1.5] |
| `prio_alpha` | uniform [0.0, 1.0] | 1.0 (max) | fix at 1.0 (or [0.85, 1.0]) |
| `prio_beta0` | uniform [0.0, 1.0] | 0.0 (min) | fix at 0.0 (or [0.0, 0.2]) |
| `vf_coef` | uniform [0.1, 5.0] | 0.97 | uniform [0.5, 2.0] |
| `gamma` | logit_normal [0.8, 0.9999] | 0.99 (lower IQR) | logit_normal [0.99, 0.999] |

Protein's effective lr ceiling is determined by `scale=0.5` on the
log_normal distribution: even though the spec says `[1e-5, 1e-1]`, the
GP very rarely sampled above ~1e-3 because the prior is concentrated
around the geometric mean. v35 should either narrow the bounds tighter
around 9e-4 or use `scale=1.0` to widen.

### v35 sweep recommendations

1. **Hardcode the GP-converged knobs** (don't waste sweep budget on them):
   - `horizon=256`, `minibatch_size=4096`, `total_agents=4096`,
     `hidden_size=256` (or sweep over `[2, 4]` for num_layers only).
   - Also fix `beta1=0.95`, `beta2=0.9997`, `eps=1e-10` — all three
     converged tightly.

2. **Widen the boundary-pinned knobs** per the table above. Especially:
   - lr to `[1e-4, 5e-3]` to test if 9e-4 is really the optimum.
   - vtrace_rho/c_clip lower bound to 0.1 so GP can test below 0.5.
   - max_grad_norm to allow even 0.1 (the v34 minimum was 0.25 effective).

3. **Sweep tighter on the truly-uncertain knobs**:
   - `clip_coef` ∈ uniform[0.05, 0.30]
   - `gae_lambda` ∈ logit_normal[0.92, 0.99]
   - `replay_ratio` ∈ uniform[1.0, 2.0]
   - `ent_coef` ∈ log_normal[0.01, 0.05]
   - `vf_clip_coef` ∈ uniform[0.10, 0.30]

4. **Confirm a3mi6u2g and fpdao4ac at full 3B steps**. Both top runs
   only reached 2.0–2.3 B steps before Pareto pruning truncated them.
   The 5-bucket trajectories show jkr still rising in the final bucket
   (`0.78 → 0.89` in the last 250M-step segment for a3mi6u2g) — the
   peak almost certainly continues climbing past 0.89 with more steps.
   Re-run both configs at full 3B for confirmation before promoting to
   the v35 baseline.

5. **Drop `total_timesteps` from the sweep**. It's currently
   `log_normal[3e7, 1e11]` which produced trial budgets ranging from
   100M to 7B+ steps. The Pareto pruner works better when budget is
   fixed at a known-useful value (e.g., 3B steps). Free up the GP's
   capacity for hparams that actually matter.

6. **Add `prio_alpha=1.0` and `prio_beta0=0.0` to the `[train]`
   defaults** — both pinned at sweep boundary in 60% of elite runs.
   Useful even outside the sweep.

### Cross-cutting findings

#### What v34 confirmed about the v32 baseline

The v32 baseline (`learning_rate=3e-4`, `vf_coef=0.5`, `prio_alpha=0.8`,
`prio_beta0=0.2`, `vtrace_rho_clip=1.0`, `vtrace_c_clip=1.0`) is
**substantially miscalibrated for Fight Caves**:

- lr too low by 3× (optimum ≥ 9e-4)
- vf_coef too low by 2× (optimum ~1.0)
- prio_alpha too low (optimum ≥ 0.97)
- prio_beta0 too high (optimum 0.0)
- vtrace clips too high (optimum at 0.5 lower bound)

These five corrections together explain the 0.886 vs 0.370 jump.
Individually each contributes ~5-15% to the lift; together they
compound multiplicatively.

#### Why crashes happen in this region

The 33 crashed trials (peak ≥ 0.3, final < 50% of peak) tend to use
**higher gamma (0.995+) AND higher vf_coef (0.97+) simultaneously**.
This suggests a runaway value-target-amplification pathology: very-high
gamma compounds tiny errors over the 8000-tick episode, and a large
vf_coef forces the policy network to over-fit those amplified errors.

Mitigation in v35: cap gamma at 0.997 (the elite IQR lid), and don't
let vf_coef and gamma both push to their respective maxima.

#### Behavioral observations from the SOTA runs

- **Pot waste is still high** (29/32 pots wasted in `a3mi6u2g`'s final
  eval). The reward stack's `shape_pot_waste_scale=-1.2` is not strong
  enough to fix it — or Jad's prayer-flick demand makes pot timing
  inherently lossy. Worth a small ablation in v35.
- **avg_hp_on_food = 0.79** for `a3mi6u2g`. The policy still eats
  proactively at high HP, mostly because consumable timing is locked
  to prayer/attack rotations rather than HP need. Same shaping question.
- **wrong_prayer_hits fall from 105 to 26 over training** — that's the
  classic "Jad mastery" signature. The policy is genuinely learning the
  attack-style read, not just gaming reward shaping.

### Cross-sweep summary table

| Sweep | Date | Runs | Best peak jkr | Best final jkr | SOTA delta vs prior |
|-------|------|-----:|-------:|-------:|---------------------|
| OBS sweep | 2026-04-26 | 3 | ~0.39 | ~0.27 | +18.8% peak vs v30.0 |
| v32.0 | 2026-04-29 | 1 | 0.370 | 0.112 | +15.4% vs OBS.1 |
| **v34_sweep_long** | **2026-04-30→05-03** | **200** | **0.886** | **0.886** | **+139% final / +9.4% peak** |

### Consistent high-jkr runs deep-dive (the 33 stable performers)

Beyond raw peak, we want runs that **reached AND HELD** a high jad_kill_rate.
Defining "consistent" as `last3_mean ≥ 0.4` AND `last3_std ≤ 0.10` across
the 5 downsample buckets (last bucket = 10k-episode eval, very low noise):

**33 of 200 runs qualify. 16 are elite tier (`last3_mean ≥ 0.5`).**

The two absolute SOTAs (`a3mi6u2g` peak/final 0.886 and `a7wp0gy2` peak/final
0.684) fell just outside the strict std≤0.10 filter because their trajectories
ramp very steeply — they're perfectly monotonic, just not flat. Treat them
as a separate "fast climber" archetype on top of the consistent list below.

| # | ID | Run name | L3mean | L3std | Final | Peak | Stab | Mono | Trajectory |
|---|------|--------|------:|-----:|-----:|-----:|---:|---:|---|
| 1 | `fpdao4ac` | feasible-fog-479 | **0.739** | 0.075 | 0.826 | 0.826 | 1.00 | 1.00 | 0.00 / 0.35 / 0.64 / 0.75 / 0.83 |
| 2 | `3ftyvwwy` | smooth-shape-499 | 0.574 | 0.071 | 0.667 | 0.667 | 1.00 | 0.75 | 0.00 / 0.42 / 0.56 / 0.50 / 0.67 |
| 3 | `r2v1cmfg` | dazzling-glade-515 | 0.558 | 0.088 | 0.474 | 0.680 | 0.70 | 0.75 | 0.00 / 0.14 / 0.52 / 0.68 / 0.47 |
| 4 | `xc02snv5` | sleek-durian-458 | 0.557 | 0.053 | 0.523 | 0.632 | 0.83 | 0.75 | 0.00 / 0.21 / 0.52 / 0.63 / 0.52 |
| 5 | `n6kxxu6e` | fresh-frost-537 | 0.557 | 0.042 | 0.615 | 0.615 | 1.00 | 1.00 | 0.00 / 0.39 / 0.52 / 0.54 / 0.61 |
| 6 | `m1176z7o` | rosy-shape-453 | 0.556 | 0.098 | 0.626 | 0.626 | 1.00 | 1.00 | 0.00 / 0.10 / 0.42 / 0.62 / 0.63 |
| 7 | `596c0g4e` | wandering-hill-442 | 0.541 | 0.061 | 0.605 | 0.605 | 1.00 | 0.75 | 0.00 / 0.06 / 0.56 / 0.46 / 0.61 |
| 8 | `gpljjloi` | flowing-bush-569 | 0.538 | 0.050 | 0.534 | 0.601 | 0.89 | 0.75 | 0.00 / 0.10 / 0.48 / 0.60 / 0.53 |
| 9 | `30l9geu9` | glad-sponge-518 | 0.530 | 0.063 | 0.467 | 0.616 | 0.76 | 0.75 | 0.00 / 0.10 / 0.51 / 0.62 / 0.47 |
| 10 | `upr5m838` | generous-thunder-491 | 0.521 | 0.059 | 0.600 | 0.600 | 1.00 | 1.00 | 0.00 / 0.31 / 0.46 / 0.50 / 0.60 |
| 11 | `9jhfsqhf` | devout-rain-571 | 0.521 | 0.068 | 0.615 | 0.615 | 1.00 | 1.00 | 0.00 / 0.27 / 0.46 / 0.49 / 0.62 |
| 12 | `0i7q95ue` | vibrant-donkey-508 | 0.512 | 0.067 | 0.527 | 0.585 | 0.90 | 0.75 | 0.00 / 0.33 / 0.59 / 0.42 / 0.53 |
| 13 | `61dfl4h3` | twilight-universe-553 | 0.509 | 0.035 | 0.558 | 0.558 | 1.00 | 1.00 | 0.01 / 0.39 / 0.47 / 0.49 / 0.56 |
| 14 | `b72qfux9` | misunderstood-paper-497 | 0.506 | 0.028 | 0.470 | 0.538 | 0.87 | 0.50 | 0.00 / 0.48 / 0.54 / 0.51 / 0.47 |
| 15 | `ixtp7vds` | confused-rain-495 | 0.504 | 0.065 | 0.413 | 0.560 | 0.74 | 0.75 | 0.00 / 0.09 / 0.54 / 0.56 / 0.41 |
| 16 | `g2obmcq3` | fallen-river-448 | 0.501 | 0.056 | 0.452 | 0.579 | 0.78 | 0.75 | 0.00 / 0.18 / 0.47 / 0.58 / 0.45 |
| 17 | `z6m1ubub` | wise-frost-489 | 0.494 | 0.031 | 0.470 | 0.537 | 0.87 | 0.75 | 0.00 / 0.42 / 0.48 / 0.54 / 0.47 |
| 18 | `ed2hp3xc` | neat-water-466 | 0.490 | 0.048 | 0.556 | 0.556 | 1.00 | 0.75 | 0.00 / 0.08 / 0.47 / 0.44 / 0.56 |
| 19 | `i3jj9vzj` | amber-cloud-476 | 0.482 | 0.093 | 0.587 | 0.587 | 1.00 | 1.00 | 0.00 / 0.06 / 0.36 / 0.50 / 0.59 |
| 20 | `wpdc8u6t` | quiet-aardvark-540 | 0.460 | 0.030 | 0.423 | 0.497 | 0.85 | 0.75 | 0.00 / 0.01 / 0.46 / 0.50 / 0.42 |
| 21 | `9xtci15w` | sunny-dust-486 | 0.457 | 0.024 | 0.456 | 0.487 | 0.94 | 0.75 | 0.08 / 0.37 / 0.49 / 0.43 / 0.46 |
| 22 | `mu1djfhy` | eternal-sunset-473 | 0.453 | 0.071 | 0.354 | 0.517 | 0.68 | 0.75 | 0.00 / 0.28 / 0.49 / 0.52 / 0.35 |
| 23 | `3hc1e782` | royal-morning-547 | 0.452 | 0.028 | 0.487 | 0.487 | 1.00 | 1.00 | 0.00 / 0.09 / 0.42 / 0.45 / 0.49 |
| 24 | `dt40kh73` | glorious-morning-519 | 0.439 | 0.068 | 0.517 | 0.517 | 1.00 | 0.75 | 0.00 / 0.04 / 0.45 / 0.35 / 0.52 |
| 25 | `vgc3sio1` | stoic-microwave-450 | 0.438 | 0.052 | 0.508 | 0.508 | 1.00 | 1.00 | 0.00 / 0.34 / 0.38 / 0.42 / 0.51 |
| 26 | `j0g0ggf9` | worldly-water-575 | 0.434 | 0.066 | 0.500 | 0.500 | 1.00 | 1.00 | 0.00 / 0.00 / 0.34 / 0.46 / 0.50 |
| 27 | `qj3dzjya` | faithful-snow-521 | 0.426 | 0.058 | 0.465 | 0.470 | 0.99 | 1.00 | 0.00 / 0.00 / 0.34 / 0.47 / 0.46 |
| 28 | `jo0ltz8y` | divine-violet-517 | 0.424 | 0.044 | 0.485 | 0.485 | 1.00 | 0.75 | 0.00 / 0.04 / 0.40 / 0.38 / 0.49 |
| 29 | `sf7o8qnh` | solar-grass-577 | 0.417 | 0.068 | 0.378 | 0.513 | 0.74 | 0.75 | 0.00 / 0.00 / 0.36 / 0.51 / 0.38 |
| 30 | `ub52jrge` | unique-firefly-465 | 0.415 | 0.073 | 0.424 | 0.500 | 0.85 | 0.75 | 0.00 / 0.00 / 0.32 / 0.50 / 0.42 |
| 31 | `cd2c11ki` | amber-resonance-536 | 0.415 | 0.085 | 0.297 | 0.493 | 0.60 | 0.75 | 0.00 / 0.06 / 0.45 / 0.49 / 0.30 |
| 32 | `wecckv3s` | pleasant-snowball-531 | 0.403 | 0.064 | 0.493 | 0.493 | 1.00 | 1.00 | 0.00 / 0.26 / 0.36 / 0.35 / 0.49 |
| 33 | `oa2d19mp` | volcanic-capybara-504 | 0.402 | 0.078 | 0.305 | 0.495 | 0.62 | 0.75 | 0.00 / 0.01 / 0.41 / 0.49 / 0.30 |

**Legend:** L3mean/L3std = mean/std of last-3 buckets, Stab = final/peak ratio,
Mono = fraction of consecutive bucket pairs that were non-decreasing (≥ −0.02
tolerance to allow tiny noise wiggles).

#### Hparam centroid of the 33 consistent runs

```
lr               9.00e-04   IQR [8.43e-04, 9.00e-04]    pinned at sweep max
ent_coef         0.0227     IQR [0.0197, 0.0274]
gamma            0.9925     IQR [0.9900, 0.9948]        lower IQR pinned at sweep min
gae_lambda       0.9623     IQR [0.9470, 0.9712]
clip_coef        0.1671     IQR [0.1547, 0.1837]
vf_coef          0.9746     IQR [0.8756, 1.0000]
vf_clip_coef     0.1824     IQR [0.1556, 0.2077]
max_grad_norm    0.3036     IQR [0.2500, 0.4777]
replay_ratio     1.4356     IQR [1.3927, 1.5888]
prio_alpha       0.9972     IQR [0.9404, 1.0000]
prio_beta0       0.0337     IQR [0.0000, 0.1212]
vtrace_rho_clip  0.5000     [all 0.5000]                pinned at sweep min
vtrace_c_clip    0.5000     IQR [0.5000, 0.5358]
beta1            0.9500     IQR [0.9471, 0.9500]
beta2            0.9997     IQR [0.9996, 0.9997]
eps              1e-10      IQR [1e-10, 1.1e-10]
horizon          256        [all 256]
minibatch_size   4096       [all 4096]
num_layers       2          IQR [2, 3]
hidden_size      256        [all 256]
total_agents     4096       [all 4096]
num_buffers      1          IQR [1, 2]
```

#### Consistent vs Crashed: what discriminates them

The 23 "crashed" runs (peak ≥ 0.4 then collapsed to <50% of peak)
share most knobs with the consistent group but differ in a few telling
ways. Median deltas:

```
hparam          consistent    crashed     delta
gamma             0.9925      0.9947     -0.0022   crashes prefer higher gamma
gae_lambda        0.9623      0.9694     -0.0071   crashes prefer higher GAE
replay_ratio      1.4356      1.5208     -0.0852   crashes prefer more replay
prio_beta0        0.0337      0.0000     +0.0337   stable runs use small >0 prio_beta0
num_buffers       1           2          -1        stable prefer 1 buffer
ent_coef          0.0227      0.0205     +0.002    slight: more entropy = more stable
```

**Stable recipe** = `gamma ≤ 0.995, replay_ratio ≤ 1.45, prio_beta0
small-but-nonzero, num_buffers=1`. **Crash recipe** = same recipe but
with `gamma 0.995+ AND replay_ratio 1.52+ AND num_buffers=2` →
late-training value-target amplification → policy collapse.

Add this to the v35 boundary table: cap `gamma ≤ 0.997`, prefer
`replay_ratio ∈ [1.0, 1.5]`, lock `num_buffers = 1`.

### Post-sweep tooling — what PufferLib gives us

Beyond the analysis scripts, PufferLib has several built-in features
useful after a sweep. Documented here so future sweeps can reuse:

#### `puffer eval <env> --load-id <run_id> --wandb`
Replay a trained policy interactively. Pulls the W&B model artifact for
the given run, loads the latest checkpoint, runs the env in render mode.
Implementation: `pufferlib/torch_pufferl.py:492-504`.

#### `puffer train <env> --load-id <run_id> --wandb` (kickstart)
Same artifact-download mechanism, but in training mode. Use to **continue
a Pareto-truncated trial at full budget** — both v34 SOTAs only ran
2.0–2.3 B steps before the cost cap; jkr was still climbing.

#### `_replay_prior_observations` — sweep warm-start
`pufferl.py:395-433`. At sweep start, reads `logs/<env>/*.json` and feeds
every prior trial's `(score, cost)` buckets back into Protein's GP via
`sweep_obj.observe()`. **For v35 this is huge**: drop v34's 200×5 = 1000
buckets into the GP and v35 converges in ~30 trials instead of 100+.

#### Constellation (`./build.sh constellation` → `seethestars` binary)
Interactive Raylib-based 3D visualizer for sweep results.
- `constellation/cache_data.py` preprocesses logs → TSNE projection +
  experiments.json
- `constellation.c` renders 4 panels (3D scatter, 2D scatter, TSNE,
  hparam box plots) with live filtering and tooltip-on-right-click
- Mostly redundant with our numerical clustering analysis; keep in
  reserve for presentation visuals or cross-env exploration.

#### `pufferlib.sweep.pareto_points()` / `prune_pareto_front()`
Public API at `sweep.py:272` and `:294`. Call directly on lists of
`{output, cost}` dicts to extract a Pareto frontier offline. Useful
for plotting the score-vs-time tradeoff curve from any sweep's logs.

#### `nsys profile` via `profile.sh`
Kernel-level CUDA profiling, groups by `matmul / ppo_loss / mingru /
muon / copy / sample / advantage / prio_replay / decoder_grad`. Useful
when investigating SPS regressions — v34's slow tail (min 0.54M SPS vs
median 1.40M) is likely priority-replay or GRU-forward bound, but
nsys would tell us which.

#### Built-in metrics dump
`pufferl.py:357` writes the full `metrics` dict to JSON at run end.
Each v34 log has 600+ keys we haven't analyzed yet — `loss/policy`,
`loss/value`, `loss/kl`, `loss/clipfrac`, `perf/rollout`, `perf/eval_env`
are all there. Worth pulling for v35 to diagnose crashes earlier.

### Artifacts

- W&B group: `v34_sweep_long`
- Top SOTA log: `pufferlib_4/logs/fight_caves/a3mi6u2g.json`
- Second SOTA log: `pufferlib_4/logs/fight_caves/fpdao4ac.json`
- Consistent run IDs (33): `/tmp/v34_consistent.json`
- Constellation projection (TSNE + KMeans): `/tmp/v34_constellation.json`
- Sweep config: `runescape-rl/config/fight_caves.ini` (current branch state matches what ran)

### Pending / next steps

1. **Confirm SOTAs at full 3B steps**: re-run `a3mi6u2g` and `fpdao4ac`
   configs without Pareto pruning. Expectation: peak climbs past 0.90.
2. **Author v35 sweep config** using the elite-basin centroid as the
   prior and the boundary-widening table above for new bounds. Drop
   converged knobs from the search space.
3. **Resume the paused `w_attack_attempt` reward refactor** — pre-v34
   we had identified that `w_damage_dealt` is RNG-tainted (Jad's high
   variance damage rolls leak into the reward). Replace with
   `w_attack_attempt=0.5` once v35 starts. Note: the v34 SOTA was
   achieved WITH the RNG-tainted reward, so the fix should only help.
4. **Investigate pot-waste shaping** — the SOTA policy still wastes
   nearly all pots. Either increase `shape_pot_waste_scale` magnitude
   or add a pot-no-threat penalty similar to food's.

---

## v28.8 Sweep 2: non-reward hparam sweep (planned, runs after Sweep 1) — SUPERSEDED BY v34_sweep_long

### Purpose

Broad Bayesian/population-based sweep over training + policy + vec
hparams with the reward config frozen at whatever Sweep 1 validates
(v28.8 minus any rewards that zeroed out cleanly). Goal: discover
whether pufferlib's default non-reward hparams are miscalibrated for
Fight Caves the same way `prio_alpha` was in v29.1.

### Likely search space

Candidate dims (finalized after Sweep 1):

- **[train]** — `learning_rate`, `ent_coef`, `clip_coef`, `vf_coef`,
  `vf_clip_coef`, `max_grad_norm`, `gamma`, `gae_lambda`, `prio_alpha`,
  `prio_beta0`, `horizon`, `minibatch_size`, `replay_ratio`
- **[policy]** — `hidden_size`, `num_layers`
- Fixed: `total_timesteps`, `total_agents`, reward config (from Sweep 1
  outcome)

Budget per run will likely drop from 3B to 500M–1B steps to keep the
sweep tractable (~100–200 runs). Top candidates from the sweep will
be re-run at full 3B budget as a confirmation round before adopting.

### Config

TBD after Sweep 1 completes.

### Results / Observations

(pending)

---

## v28.8 Sweep 3: reward hparam sweep (planned, runs after Sweep 2)

### Purpose

With optimizer + policy hparams locked from Sweep 2, sweep reward
magnitudes on the surviving shaping/penalty rewards. Goal: check
whether any of the currently-tuned values are off-peak after all other
hparams are optimized.

### Likely search space

The Sweep 1 reward keys that were NOT zeroed out (i.e., retained as
load-bearing), swept over ranges around their current values. Likely
log_normal or uniform around the current value (e.g., [0.25×, 4×]).

Candidate groupings (finalized after Sweeps 1 and 2):

- Primary-signal rewards: `w_npc_kill`, `w_invalid_action`,
  `w_tick_penalty`
- Consumable shaping: `shape_food_full_waste_penalty`,
  `shape_food_waste_scale`, `shape_pot_full_waste_penalty`,
  `shape_pot_waste_scale`
- Prayer/attack shaping: `shape_wrong_prayer_penalty`,
  `shape_unnecessary_prayer_penalty`, `shape_wasted_attack_penalty`

50–75 runs at full 3B budget (reward changes shift the loss landscape
more than optimizer changes, so benefit from full convergence).

### Config

TBD after Sweeps 1 and 2 complete.

### Results / Observations

(pending)

---

## hparams

All hparams in the current v26 config. Values from `runescape-rl/config/fight_caves.ini` (explicit) or inherited from `pufferlib_4/config/default.ini` (default).

### [base]

| Param | Value | Source |
|-------|-------|--------|
| env_name | fight_caves | ini |
| checkpoint_interval | 50 | ini |
| rank | 0 | default |
| world_size | 1 | default |
| gpu_id | 0 | default |
| nccl_id | 'None' | default |
| profile | False | default |
| checkpoint_dir | checkpoints | default |
| log_dir | logs | default |
| eval_episodes | 10000 | default |
| cudagraphs | 10 | default |
| seed | 73 | default |
| reset_state | True | default |

### [env] — reward weights and shaping

| Param | Value |
|-------|-------|
| w_damage_dealt | 0.7 |
| w_attack_attempt | 0.2 |
| w_damage_taken | -1.5 |
| w_npc_kill | 3.5 |
| w_wave_clear | 15.0 |
| w_jad_damage | 2.0 |
| w_jad_kill | 150.0 |
| w_player_death | -11.0 |
| w_correct_jad_prayer | 2.0 |
| w_correct_danger_prayer | 0.05 |
| w_invalid_action | -0.1 |
| w_tick_penalty | -0.005 |
| shape_food_full_waste_penalty | -6.5 |
| shape_food_waste_scale | -1.2 |
| shape_food_no_threat_penalty | 0.0 |
| shape_pot_full_waste_penalty | -6.5 |
| shape_pot_waste_scale | -1.2 |
| shape_pot_no_threat_penalty | 0.0 |
| shape_wrong_prayer_penalty | -0.3 |
| shape_npc_specific_prayer_bonus | 0.3 |
| shape_npc_melee_penalty | -0.8 |
| shape_wasted_attack_penalty | -0.1 |
| shape_wave_stall_start | 500 |
| shape_wave_stall_base_penalty | -0.5 |
| shape_wave_stall_ramp_interval | 50 |
| shape_wave_stall_cap | -2.0 |
| shape_not_attacking_grace_ticks | 2 |
| shape_not_attacking_penalty | -0.01 |
| shape_kiting_reward | 2.2 |
| shape_kiting_min_dist | 2 |
| shape_kiting_max_dist | 10 |
| shape_safespot_attack_reward | 2.3 |
| shape_unnecessary_prayer_penalty | -0.2 |
| shape_jad_heal_penalty | -0.1 |
| shape_reach_wave_60_bonus | 0.0 |
| shape_reach_wave_61_bonus | 0.0 |
| shape_reach_wave_62_bonus | 0.0 |
| shape_reach_wave_63_bonus | 0.0 |
| shape_jad_kill_bonus | 0.0 |
| shape_resource_threat_window | 2 |

### [vec] — vectorized env

| Param | Value | Source |
|-------|-------|--------|
| total_agents | 4096 | ini |
| num_buffers | 2 | ini |
| num_threads | 16 | default |

### [policy] — model architecture

| Param | Value | Source |
|-------|-------|--------|
| hidden_size | 256 | ini |
| num_layers | 2 | ini |
| expansion_factor | 1 | default |

### [torch] — model backend

| Param | Value | Source |
|-------|-------|--------|
| network | MinGRU | default |
| encoder | DefaultEncoder | default |
| decoder | DefaultDecoder | default |

### [train] — PPO / optimizer

| Param | Value | Source |
|-------|-------|--------|
| total_timesteps | 3_500_000_000 | ini |
| learning_rate | 0.0003 | ini |
| anneal_lr | 0 | ini |
| gamma | 0.999 | ini |
| gae_lambda | 0.95 | ini |
| clip_coef | 0.2 | ini |
| vf_coef | 0.5 | ini |
| ent_coef | 0.01 | ini |
| max_grad_norm | 0.5 | ini |
| horizon | 256 | ini |
| minibatch_size | 4096 | ini |
| gpus | 1 | default |
| seed | 42 | default |
| min_lr_ratio | 0.0 | default |
| replay_ratio | 1.0 | default |
| vf_clip_coef | 0.2 | default |
| beta1 | 0.95 | default |
| beta2 | 0.999 | default |
| eps | 1e-12 | default |
| vtrace_rho_clip | 1.0 | default |
| vtrace_c_clip | 1.0 | default |
| prio_alpha | 0.8 | default |
| prio_beta0 | 0.2 | default |

### [sweep] — sweep control

| Param | Value |
|-------|-------|
| method | Protein |
| metric | jad_kill_rate |
| metric_distribution | linear |
| goal | maximize |
| max_runs | 100 |
| max_suggestion_cost | 2100 |
| gpus | 1 |
| early_stop_quantile | 0.3 |
| prune_pareto | true |
| use_gpu | true |
| downsample | 5 |
| sweep_only | env/w_damage_taken, env/shape_safespot_attack_reward, env/w_jad_damage, env/w_jad_kill, env/w_correct_jad_prayer, env/shape_jad_heal_penalty, env/w_correct_danger_prayer, env/w_player_death, train/ent_coef |

### [sweep.*] — per-param bounds (Phase 3 active)

| Subsection | distribution | min | max |
|------------|--------------|-----|-----|
| sweep.env.w_damage_taken | uniform | -2.5 | -1.5 |
| sweep.env.shape_safespot_attack_reward | uniform | 2.5 | 5.5 |
| sweep.env.w_jad_damage | uniform | 1.0 | 7.0 |
| sweep.env.w_jad_kill | uniform | 700.0 | 2500.0 |
| sweep.env.w_correct_jad_prayer | uniform | 1.0 | 5.0 |
| sweep.env.shape_jad_heal_penalty | uniform | -5.0 | -0.1 |
| sweep.env.w_correct_danger_prayer | uniform | 0.075 | 0.75 |
| sweep.env.w_player_death | uniform | -15.0 | -5.0 |
| sweep.train.ent_coef | log_normal | 0.008 | 0.05 |

### CLI-only (passed via `train.sh` args, not in ini)

| Flag | Default |
|------|---------|
| --load-model-path | None |
| --load-id | None |
| --render-mode | auto |
| --wandb | false (opt-in) |
| --wandb-project | "fight caves rl" |
| --wandb-group | debug |
| --tag | None |
| --slowly | false |
| --save-frames | 0 |
| --gif-path | eval.gif |
| --fps | 15 |
