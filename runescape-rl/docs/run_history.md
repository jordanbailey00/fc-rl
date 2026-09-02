# RL Configuration History

Tracks training configuration changes with results and reasoning. The live
baseline is at the top; older sections preserve the terminology and conclusions
that were current when those experiments ran.

## Section template (followed by entries from v28 onward; older entries
may vary but cover the same content when data is available)

Each run's section should cover the following, ordered roughly as:

1. **Header** — `## vXX.Y (YYYY-MM-DD, <status> — `<run_id>`[/wandb display name])`
2. **Baseline** — one-line "single-variable change from vX.Y" / "warm-start
   from vX.Y checkpoint" / "cold-start, new config direction"
3. **Bottom line** — 2–4 sentence summary of what happened (peak, final,
   stability, headline metric movement)
4. **Config diff vs baseline** — table listing only the keys that changed
5. **(For major version jumps like v22→v23)** — rationale for the broader
   redesign: what problem we were solving, what observation/reward
   structure changed, what we hypothesized
6. **Backend diff** (if applicable) — any fc-core/fc-training code changes
   paired with this run, and why
7. **Rationale** — why these specific values; per-tick economics if
   relevant; break-even math for reward-shape changes
8. **Expected outcomes** (staged runs) or **Results — full metric sweep**
   (completed runs)
9. **Trajectory** — bucketed metrics over training steps, identifying
   divergence points from comparable runs
10. **Reward-channel breakdown** — total and fire count per channel at
    final state vs prior baseline
11. **Diagnosis / What worked / What didn't work**
12. **Expected-outcome audit** (prediction vs actual for staged targets)
13. **Recommendations for next run** (v28.7, etc.)
14. **Artifacts** — wandb run id, local log path, checkpoints, best
    deployable artifact

Entries prior to v28 may not have all of the above; they predate the
`rwd_*` per-channel analytics added at v27.3 and the structured
attribution-matrix style used from v28.5 onward.

## Formatting notes
- recent sections now include `Exact active config` snapshots
- these list all non-zero training-shaping values plus semantically important
  zero-valued keys when the zero itself materially changes behavior
  (for example `w_correct_jad_prayer = 2.0` or `load_model_path = null`)

---

## Current baseline — corrected Magic accuracy (`399wemq3`, 2026-09-02)

The live mechanics retain Ket-Zek and TzTok-Jad's OSRS `+60` Magic attack
bonus. This changes only Magic accuracy: Ket-Zek melee and Jad melee/Ranged
continue using equipment bonus zero, correct Protection Prayer still blocks
the matching attack completely, and the trainer configuration is unchanged.

The current cold-start baselines are `mvvexowt` at 100M steps and `399wemq3`
at 750M steps. They supersede the pre-correction behavior baselines while
preserving those runs as controlled historical evidence.

| Run | Role | Agent steps | Jad kill | Wave 63 | Damage taken |
| --- | --- | ---: | ---: | ---: | ---: |
| [`mvvexowt`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/mvvexowt) | current 100M mechanics baseline | 99,614,720 | 0.00% | 0.00% | 1,306.868 |
| [`399wemq3`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/399wemq3) | current 750M performance baseline | 749,731,840 | **91.49%** | **92.69%** | 1,024.873 |
| [`dqr4lxe9`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/dqr4lxe9) | 100M pre-correction control | 99,614,720 | 0.00% | 0.00% | 1,308.233 |
| [`r16nvaq7`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/r16nvaq7) | 750M pre-correction control | 749,731,840 | 92.74% | 94.12% | 978.003 |

The 750M corrected run averaged 91.97% Jad completion over its last 50 epochs
versus 94.16% for the pre-correction control. It nevertheless peaked at 98.05%
and reliably learned the complete cave. Because the selected `1nvvx5qu`
trainer recipe was swept on the easier pre-correction mechanic, a replacement
hyperparameter sweep is tracked as deferred work.

The exact implementation, roll values, deterministic tests, and controlled
100M/750M comparisons are archived in
`docs/archive/fc_cleanup_and_parity_history.md`.

---

## v4.5 former baseline — selected `1nvvx5qu`, promoted `txqsiahp` (2026-08-23)

This section records the baseline before Ket-Zek and Jad's Magic attack bonus
was corrected. It is preserved as historical experiment evidence.

The live configuration is `runescape-rl/config/fight_caves.ini`, SHA-256
`33bca87ccde19636c3b742b7e292732978307f648a8e80f6849f1d8d1612c9b6`.
It uses the approved OSRS-parity backend, no food or Prayer potions, the SOTA
Twisted bow/Masori loadout, observation contract v8, the three-head stationary
attack action contract v3, reward contract v4, and native trainer seed 73.

### Selection and reproduction

The corrected 130-run Stage 2 sweep held the environment, rewards,
observations, actions, loadout, seed, and fixed trainer settings constant while
sweeping value/optimizer settings, hidden size, recurrent layer count, and
agent count. `1nvvx5qu` was chosen over the highest terminal-score candidate
because its 512-hidden, three-layer, 4,096-agent policy had the strongest
final-quarter consistency floor. The complete top-eight analysis is in the
root `sweep_top8.md`.

Standalone run `8oivozuq` reproduced `1nvvx5qu` exactly on every final
environment metric. It therefore established that the selected sweep recipe
could be regenerated outside sweep mode with a replayable checkpoint.

### Current backend and reward promotion

`i215ulj4` applied the selected recipe to the approved OSRS movement,
collision, line-of-sight, pathfinding, combat-ordering, and encounter backend.
It retained the previous `w_correct_danger_prayer=0.005`. Run `txqsiahp`
changed only that configurable reward weight to zero; the Jad-specific correct
Prayer weight was already zero. The zero-Prayer-reward run improved final Jad
completion by 4.724 percentage points and wave-63 reach by 4.213 points without
rewarding correct Prayer or blocked hits.

### Exact active trainer settings

| Category | Values |
| --- | --- |
| Budget | 750,000,000 agent steps; no LR annealing |
| Vectorization | 4,096 agents, 2 buffers, 16 threads |
| Policy | MinGRU, 512 hidden, 3 layers, expansion 1 |
| PPO | `lr=0.00207567504650331`, `gamma=0.9991261141073255`, `gae_lambda=0.9`, `horizon=256`, `minibatch=32768` |
| Replay/value | `replay_ratio=2.055184291514704`, `vf_coef=0.9336215311545304`, `vf_clip=0.16791546282962394` |
| Optimizer | `max_grad_norm=0.1418276517190492`, `beta1=0.9832670364021693`, `beta2=0.9995810484472892`, `eps=1e-10` |
| V-trace/PER | `rho=2.0`, `c=0.9746667741536915`, `alpha=0.9110743956381228`, `beta0=0.2258134371255269` |
| Policy contract | 319 inputs = 285 policy features + 34 visible mask bits; action dimensions `[17, 9, 8]` |

The active reward weights are documented in the root README and defined in the
live INI. In particular, both correct-Prayer rewards are zero.

### Verified run sequence

These are final W&B summaries, not peak values:

| Run | Role | Agent steps | Jad kill | Wave 63 | Damage taken | SPS |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| [`hevp6ehc`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/hevp6ehc) | v4 simple-reward/new-backend comparison | 1,499,463,680 | 93.11% | 93.91% | 920.4 | 1,672,498 |
| [`z5vbs56z`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/z5vbs56z) | later v4 new-backend comparison | 1,499,463,680 | 90.16% | 91.17% | 986.8 | 1,567,444 |
| [`88l9p7ie`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/88l9p7ie) | corrected movement baseline | 1,499,463,680 | 90.08% | 91.34% | 967.4 | 1,538,435 |
| [`1nvvx5qu`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/1nvvx5qu) | selected Stage 2 sweep run | 749,731,840 | 94.79% | 95.94% | 877.7 | 948,405 |
| [`8oivozuq`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/8oivozuq) | exact standalone reproduction | 749,731,840 | 94.79% | 95.94% | 877.7 | 914,429 |
| [`i215ulj4`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/i215ulj4) | OSRS-parity backend, old Prayer reward | 749,731,840 | 88.02% | 89.91% | 973.5 | 871,112 |
| [`txqsiahp`](https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/txqsiahp) | live baseline, Prayer reward zero | 749,731,840 | **92.74%** | **94.12%** | 978.0 | 990,867 |

### Artifacts and interpretation

- `runescape-rl/config/experiments/fight_caves_v45_1nvvx5qu_retrain_750m.ini`
  preserves the sweep winner's standalone reproduction config.
- `runescape-rl/config/fight_caves.ini` is the current `txqsiahp` config.
- Checkpoints are stored under the compiled contract directory recorded by each
  run manifest; evaluator preflight rejects mismatched contracts or parameter
  counts.
- `1nvvx5qu` and `8oivozuq` are trainer-recipe evidence on the earlier backend.
  `txqsiahp` was the deployment baseline before the Magic-accuracy correction.
  Their raw rates should not be treated as a single-variable backend
  comparison.

---

## v4_simple_reward former baseline - source recipe `mmyxbyn4`, confirmation `l9o32hhz` (2026-07-27)

`v4_simple_reward` was the canonical Fight Caves baseline at this point in the
project and is now superseded by the v4.5 section above. It promoted the
trainer recipe from W&B run `mmyxbyn4`, the
highest-peak run in the 140-trial v3 Stage 1 Protein sweep, and uses a fixed
1.5B-step training budget with the current 60-second natural HP regeneration
mechanic.

The completed promotion run is W&B `l9o32hhz`. It used native trainer seed
`73`, the SOTA Twisted Bow loadout, no food or prayer potions, the v7
observation contract, native hard action masks, the three-head action contract,
and the unchanged v3 simple-reward matrix. The `v4_simple_reward` name denotes
the complete promoted baseline; it does not imply a fourth reward rewrite.

### Derivation

1. `8rg9wurg` established the v3 simple-reward environment and trainer center.
2. `v3_simple_reward_sweep1` ran 140 PufferLib Protein trials for 750M steps,
   optimizing `jad_kill_rate` while holding the environment, rewards, backend,
   policy architecture, loadout, observations, actions, and seed fixed.
3. `mmyxbyn4` ranked first by peak Jad kill rate at `98.0952%` and finished its
   750M sweep run at `95.5813%`.
4. Natural HP regeneration was corrected from 1 HP every 10 ticks (6 seconds)
   to 1 HP every 100 ticks (60 seconds).
5. The exact `mmyxbyn4` trainer recipe was rerun for 1.5B steps as `l9o32hhz`.
   That completed run is the empirical reference for this baseline.

Relative to the original `mmyxbyn4` sweep trial, `l9o32hhz` intentionally
changes two experimental conditions: the configured budget is 1.5B rather than
750M, and the backend uses 60-second rather than 6-second natural HP
regeneration. Puffer's priority correction schedule depends on the configured
total budget, so the two runs are historical relatives rather than a
single-variable A/B test.

### Exact canonical configuration

This snapshot is the source for both `runescape-rl/config/fight_caves.ini` and
the synchronized `pufferlib_4/config/fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50
eval_episodes = 10000
cudagraphs = 10
reset_state = True
seed = 73

[env]
initial_sharks = 0
initial_prayer_doses = 0
w_damage_dealt = 0.0
w_progress = 0.001
w_damage_taken = -0.25
w_npc_kill = 0.0
w_wave_clear = 0.0
w_jad_kill = 0.0
w_cave_complete = 1.0
w_player_death = -1.0
w_correct_jad_prayer = 0.0
w_correct_danger_prayer = 0.005
w_prayer_lost = -0.02
w_invalid_action = -0.1
w_tick_penalty = -0.0001
shape_unnecessary_prayer_penalty = 0.0
shape_wave_stall_start = 0
shape_wave_stall_ramp_interval = 0
shape_wave_stall_base_penalty = 0.0
shape_wave_stall_cap = 0.0
shape_jad_heal_penalty = 0.0
shape_npc_heal_penalty = -0.005
shape_no_progress_start_1 = 800
shape_no_progress_start_2 = 1600
shape_no_progress_start_3 = 2400
shape_no_progress_penalty_1 = -0.001
shape_no_progress_penalty_2 = -0.005
shape_no_progress_penalty_3 = -0.02
shape_no_attack_start = 50
shape_no_attack_base_penalty = -0.005
shape_no_attack_wave_scale = 0.05
obs_ablate_npc_distance = 0
obs_ablate_incoming_aggregates = 1
obs_ablate_npc_valid = 0

[vec]
total_agents = 4096
num_buffers = 2
num_threads = 16

[train]
gpus = 1
total_timesteps = 1_500_000_000
anneal_lr = 0
learning_rate = 0.00207567504650331
ent_coef = 0.000625460620549345
gamma = 0.9991261141073255
gae_lambda = 0.9
horizon = 256
minibatch_size = 32768
replay_ratio = 2.055184291514704
clip_coef = 0.05
vf_coef = 1
vf_clip_coef = 0.15124043205980495
max_grad_norm = 0.25
vtrace_rho_clip = 2.0
vtrace_c_clip = 0.9746667741536915
prio_alpha = 0.9110743956381228
prio_beta0 = 0.2258134371255269
beta1 = 0.95
beta2 = 0.9995810484472892
eps = 1e-10

[policy]
hidden_size = 256
num_layers = 3
expansion_factor = 1

[run]
manifest_path = ''
manifest_schema_version = 1
observation_version = 'fight_caves_puffer_policy_obs_v7_npc_prayer_drain_healing_aggro_kill_events_mask_heads_0_2_no_supplies'
action_version = 'fight_caves_multidiscrete_3_head_no_supplies_v1'
reward_version = 'fight_caves_v3_progress_npc_heal_penalty_m0005'
reward_clip_enabled = 1
reward_clip_min = -1.0
reward_clip_max = 1.0

[sweep]
metric = jad_kill_rate
```

### Backend and policy contract

- Natural HP regeneration: 1 HP every 100 ticks, or 60 seconds.
- Loadout: `FC_LOADOUT_SOTA_TBOW`.
- Supplies: no sharks and no prayer-potion doses.
- Policy: 256 hidden units, 3 MinGRU layers, expansion factor 1.
- Vectorization: 4,096 agents, two buffers, 16 environment threads, one GPU.
- Observation: v7 NPC identity, prayer-drain, healing, aggro, kill-event, and
  native action-mask contract.
- Actions: movement, target selection, and prayer; native masks are hard
  enforced.
- Reward clipping: per-tick scalar reward clamped to `[-1, 1]`.

### `l9o32hhz` final evaluation

The final evaluation contains `10,082` episodes at approximately 1.499B agent
steps:

| Metric | Result |
|---|---:|
| Jad kill / cave completion rate | 82.5134% |
| Reached wave 63 | 87.2942% |
| Jad kill conditional on reaching wave 63 | 94.5228% |
| Player death rate | 17.4866% |
| Mean cave progress | 0.980091 |
| Mean wave reached | 61.8601 |
| Mean NPCs slain | 266.433 |
| Mean episode length | 8,478.32 ticks |
| Attack-when-ready rate | 92.2003% |
| Correct-prayer share of danger hits | 86.3500% |
| Mean damage taken | 1,389.48 |
| Mean damage blocked | 138,986.59 |
| Mean no-progress ticks | 259.73 |
| Mean longest wave | 345.17 ticks, around wave 55.24 |
| Mean NPC healing | 6,861.30 |
| Mean Yt-MejKot healing | 2,574.26 |
| Mean Jad healing | 4,287.05 |
| Gross damage / permanent progress | 1.29087 |
| Final entropy | 1.66349 |
| Final KL | 0.006245 |
| Final clip fraction | 0.35972 |
| Mean SPS | approximately 1.69M |
| Runtime | approximately 14 minutes 49 seconds |

### Training trajectory

| Agent steps | Jad kill | Reached wave 63 | Death rate | Mean wave | NPC healing |
|---:|---:|---:|---:|---:|---:|
| 209M | 0.000% | 0.172% | 100.000% | 35.79 | 571.0 |
| 563M | 8.844% | 77.021% | 91.156% | 61.11 | 1,718.9 |
| 938M | 48.120% | 87.728% | 51.880% | 61.88 | 5,055.5 |
| 1.313B | 78.853% | 85.117% | 21.142% | 61.70 | 6,405.2 |
| 1.499B | 82.513% | 87.294% | 17.487% | 61.86 | 6,861.3 |

The sampled training history peaked at an 88.596% Jad kill rate near 1.254B
steps, then finished lower. This baseline is highly capable but not fully
stable: future comparisons should report peak, final evaluation, and
multi-seed behavior rather than only one of those values.

### Final reward accounting

| Reward channel | Mean episode contribution |
|---|---:|
| Progress | +114.0393 |
| Correct danger prayer | +8.1951 |
| Cave completion | +0.8251 |
| Damage taken | -0.3509 |
| Player death | -0.1749 |
| Prayer lost | -1.2594 |
| NPC healing | -0.5622 |
| Tick penalty | -0.8479 |
| No-progress penalty | -0.0164 |
| No-attack penalty | -0.0532 |
| Pre-clip total | +119.7977 |
| Post-clip total | +119.5761 |

Every intentionally disabled reward channel remained zero, and native action
masks kept invalid movement, attack, and prayer actions at zero. The main
remaining behavioral inefficiency is healing churn: the policy deals about
29.1% more gross damage than the permanent work it removes.

### Artifacts

- W&B source sweep run: `mmyxbyn4`.
- W&B promoted baseline run: `l9o32hhz`.
- Local run metadata: `pufferlib_4/logs/fight_caves/l9o32hhz.json`.
- Canonical source config: `runescape-rl/config/fight_caves.ini`.
- Synced Puffer config: `pufferlib_4/config/fight_caves.ini`.
- Dedicated mmy 1.5B reproduction config:
  `runescape-rl/config/experiments/fight_caves_mmyxbyn4_hp_regen_60s_1p5b.ini`.
- Dedicated il0 1.5B comparison config:
  `runescape-rl/config/experiments/fight_caves_il0xq0uf_hp_regen_60s_1p5b.ini`.
- Canonical INI SHA-256: `509af1168dca12f1c9f72ffc51d31bc2e7c7476536785915f20b034418127514`.

---

## v1.0 former baseline - source run `b5m07qqr` (2026-07-17, selected after multi-seed confirmation)

`v1.0` is the official name of the primary Fight Caves training configuration
from this point forward. Its source recipe is W&B run `b5m07qqr`, selected
because it combined the strongest cave-completion
result in the original 48-run Protein sweep with the best progress robustness
across the two additional trainer seeds. This is a trainer-hyperparameter
promotion only: the backend, core combat rules, SOTA TBow loadout, no-supplies
environment, rewards, observations, action heads, reward clamp, and policy
architecture remain unchanged from commit `856573792`.

Selection caveat: the original seed-73 policy completed 56 caves, but the new
seed-101 and seed-202 policies completed none at 750M steps. The configuration
is therefore the best current baseline, not a seed-robust solved policy. Jad
conversion and healer handling remain the main unresolved behavior.

### Exact canonical configuration

This snapshot is the source for both `runescape-rl/config/fight_caves.ini` and
the synchronized `pufferlib_4/config/fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50
eval_episodes = 10000
cudagraphs = 10
reset_state = True
seed = 73

[env]
initial_sharks = 0
initial_prayer_doses = 0
w_damage_dealt = 0.0
w_progress = 0.001
w_damage_taken = -0.25
w_npc_kill = 0.0
w_wave_clear = 0.0
w_jad_kill = 0.0
w_cave_complete = 1.0
w_player_death = -1.0
w_correct_jad_prayer = 0.0
w_correct_danger_prayer = 0.005
w_prayer_lost = -0.02
w_invalid_action = -0.1
w_tick_penalty = -0.0001
shape_unnecessary_prayer_penalty = 0.0
shape_wave_stall_start = 0
shape_wave_stall_ramp_interval = 0
shape_wave_stall_base_penalty = 0.0
shape_wave_stall_cap = 0.0
shape_jad_heal_penalty = 0.0
shape_npc_heal_penalty = 0.0
shape_no_progress_start_1 = 800
shape_no_progress_penalty_1 = -0.001
shape_no_progress_start_2 = 1600
shape_no_progress_penalty_2 = -0.005
shape_no_progress_start_3 = 2400
shape_no_progress_penalty_3 = -0.02
shape_no_attack_start = 50
shape_no_attack_base_penalty = -0.005
shape_no_attack_wave_scale = 0.05
obs_ablate_npc_distance = 0
obs_ablate_incoming_aggregates = 1
obs_ablate_npc_valid = 0

[vec]
total_agents = 4096
num_buffers = 2
num_threads = 16

[train]
gpus = 1
total_timesteps = 750_000_000
anneal_lr = 0
learning_rate = 0.0012501122189793278
ent_coef = 0.01261936845511762
gamma = 0.9977806603387215
gae_lambda = 0.9944084418817187
horizon = 256
minibatch_size = 4096
replay_ratio = 1.1570637170225382
clip_coef = 0.17830599245832296
vf_coef = 1
vf_clip_coef = 0.15124043205980495
max_grad_norm = 0.25
vtrace_rho_clip = 0.5
vtrace_c_clip = 0.5037274754757021
prio_alpha = 0.9682355928752012
prio_beta0 = 0
beta1 = 0.95
beta2 = 0.9995810484472892
eps = 1e-10

[policy]
hidden_size = 256
num_layers = 3
expansion_factor = 1

[run]
manifest_path = ''
observation_version = 'fight_caves_puffer_policy_obs_v5_npc_type_progress_prayer_deadline_mask_heads_0_2_no_supplies'
action_version = 'fight_caves_multidiscrete_3_head_no_supplies_v1'
reward_version = 'fight_caves_v38_fc_revamp_step2_raw_work_progress_prayer_conserve_no_attack'
reward_clip_enabled = 1
reward_clip_min = -1.0
reward_clip_max = 1.0

[sweep]
metric = cave_progress
```

The default budget remains the 750M budget at which the configuration was
selected. Longer confirmations should override only
`--train.total-timesteps`; they should not fork another INI.

### Alignment checks

- Default compile-time loadout: `FC_LOADOUT_SOTA_TBOW`.
- Policy: 256 hidden units, 3 MinGRU layers, about 654.6K parameters.
- Supplies: zero sharks and zero prayer-potion doses; only movement, attack,
  and prayer action heads are exposed.
- Observation contract includes NPC type one-hot fields, progress, prayer
  decision deadlines, and the three-head mask.
- Per-step scalar reward is clamped to `[-1, 1]`.
- Top-level `base.seed=73` controls native trainer/policy randomness. The old
  inherited `train.seed=42` does not control the native CUDA trainer.
- Fight Caves episode RNG is derived from environment slot and episode count,
  so changing trainer seed does not change the base episode sequence.

---

## v38 finalist trainer-seed confirmation (2026-07-17, completed - 6 runs)

This sequential confirmation reran the three Protein finalists at the same
750M budget with native trainer seeds 101 and 202. The original seed-73 sweep
run was reused analytically. All six new jobs finished normally, logged the
same 145 finite metrics, used source commit `856573792`, used the SOTA TBow
loadout, and saved 17 checkpoints each. Config comparison against the original
candidate W&B configs found no gameplay, vector, policy, optimizer, or runtime
contract drift other than the intended top-level seed and run metadata.

### Candidate definitions

Only these seven trainer values differed between candidates:

| Candidate | Learning rate | Entropy | Gamma | GAE lambda | Horizon | Minibatch | Replay ratio |
|---|---:|---:|---:|---:|---:|---:|---:|
| `b5m07qqr` | 0.001250112219 | 0.012619368455 | 0.997780660339 | 0.994408441882 | 256 | 4096 | 1.157063717023 |
| `8q93t1vq` | 0.001064972620 | 0.013845886142 | 0.997833378290 | 0.994966699651 | 256 | 4096 | 0.949680904271 |
| `hsfj3i5p` | 0.000910163720 | 0.013735875271 | 0.998837361320 | 0.995000000000 | 256 | 4096 | 0.644525331013 |

### Every new run

| Candidate | Seed | W&B run | Eval episodes | Progress | Avg wave | Reach wave 63 | Cave completions | Episode ticks | Longest wave | NPC healing |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `b5m07qqr` | 101 | `slj5jsvg` | 10,062 | 0.869819 | 55.456 | 7.553% | 0 | 6,988.5 | 301.9 | 822.1 |
| `b5m07qqr` | 202 | `a4itjqyq` | 10,085 | 0.874639 | 55.710 | 4.660% | 0 | 7,316.7 | 497.4 | 3,994.1 |
| `8q93t1vq` | 101 | `snjmo0qq` | 10,147 | 0.838814 | 53.496 | 7.066% | 1 | 6,575.0 | 300.1 | 432.7 |
| `8q93t1vq` | 202 | `esi8phbj` | 10,135 | 0.822102 | 52.466 | 0.000% | 0 | 6,504.6 | 329.5 | 648.6 |
| `hsfj3i5p` | 101 | `7b86gn3o` | 10,013 | 0.818069 | 52.147 | 0.270% | 0 | 6,558.0 | 494.0 | 576.0 |
| `hsfj3i5p` | 202 | `my8dpjy2` | 10,179 | 0.630504 | 40.328 | 0.000% | 0 | 4,335.0 | 246.6 | 303.0 |

### Two-new-seed aggregate

| Candidate | Episodes | Mean progress | Mean wave | Reached wave 63 | Completions | Completion rate | Conditional completion |
|---|---:|---:|---:|---:|---:|---:|---:|
| `b5m07qqr` | 20,147 | **0.872229** | **55.583** | **1,230 / 6.105%** | 0 | 0.00000% | 0.000% |
| `8q93t1vq` | 20,282 | 0.830458 | 52.981 | 717 / 3.535% | 1 | 0.00493% | 0.139% |
| `hsfj3i5p` | 20,192 | 0.724286 | 46.238 | 27 / 0.134% | 0 | 0.00000% | 0.000% |

`b5m07qqr` clearly generalized best on ordinary cave progress. Its two runs
were close to one another and both exceeded every new-seed run from the other
two candidates. `8q93t1vq` was weaker and seed 202 never reached Jad.
`hsfj3i5p`, which Protein ranked first on seed 73, was highly seed-sensitive;
seed 202 regressed to wave 40.3. This is direct evidence of winner's-curse
selection in the original single-seed sweep.

### Original seed-73 context

| Candidate | Progress | Avg wave | Reach wave 63 | Completions | Completion rate | Conditional completion |
|---|---:|---:|---:|---:|---:|---:|
| `b5m07qqr` | 0.878084 | 55.939 | 17.973% | 56 / 10,104 | **0.5542%** | **3.08%** |
| `8q93t1vq` | 0.898686 | 57.242 | 36.320% | 26 / 10,124 | 0.2568% | 0.71% |
| `hsfj3i5p` | **0.918610** | **58.485** | **43.230%** | 9 / 10,044 | 0.0896% | 0.21% |

Combining all three seeds is useful descriptively but should not be treated as
a balanced three-seed experiment because the candidates were selected for
being unusually good on seed 73:

| Candidate | Three-seed progress | Three-seed wave | Three-seed reach 63 | Three-seed completion rate |
|---|---:|---:|---:|---:|
| `b5m07qqr` | **0.87418** | **55.70** | 10.069% | **56 / 30,251 = 0.1851%** |
| `8q93t1vq` | 0.85320 | 54.40 | **14.451%** | 27 / 30,406 = 0.0888% |
| `hsfj3i5p` | 0.78906 | 50.32 | 14.450% | 9 / 30,236 = 0.0298% |

### Learning curves

The values below use 25M-step rolling cave progress:

| Candidate/seed | Peak progress @ step | Late mean | First 0.70 | First 0.80 | First 0.85 | First 0.90 |
|---|---:|---:|---:|---:|---:|---:|
| `b5` / 73 | 0.9055 @ 708M | 0.8996 | 388M | 496M | 640M | 704M |
| `b5` / 101 | 0.8660 @ 750M | 0.8543 | 340M | 459M | 484M | never |
| `b5` / 202 | 0.8808 @ 709M | 0.8721 | 309M | 514M | 619M | never |
| `8q` / 73 | 0.9039 @ 709M | 0.8999 | 324M | 368M | 481M | 703M |
| `8q` / 101 | 0.8789 @ 677M | 0.8572 | 362M | 453M | 576M | never |
| `8q` / 202 | 0.7945 @ 750M | 0.7692 | 366M | never | never | never |
| `hsf` / 73 | 0.9117 @ 750M | 0.8942 | 318M | 477M | 516M | 743M |
| `hsf` / 101 | 0.7942 @ 750M | 0.7684 | 611M | never | never | never |
| `hsf` / 202 | 0.6359 @ 730M | 0.6337 | never | never | never | never |

The b5 curves were the tightest across new seeds and were still improving or
stable at the end. The result supports a longer b5 confirmation, while showing
that a single seed-73 rank is not enough to choose a policy.

### Behavior and diagnostics

- **Jad conversion did not generalize.** The original b5 policy averaged 15.64
  Jad attack cycles per Jad reach and about 2,837 Jad damage per reach. Seeds
  101 and 202 averaged only 3.41/2.52 cycles and 596/465 damage per reach.
  They reached Jad but did not sustain the fight long enough to kill it.
- **Healing remains policy-dependent.** b5 seed 202 spent an average 497 ticks
  in its longest wave and allowed 3,994 NPC healing. The activity/no-attack
  metrics show this was an active Yt-MejKot healing loop near wave 43, not a
  motionless stall.
- **The clearest idle failure was hsf seed 101.** Its longest wave averaged 494
  ticks around wave 40.5, no-progress time reached 12.78%, and its no-attack
  reward was `-4.4888` per episode. It was genuinely idle/praying during
  no-progress periods.
- **Prayer conservation is still weak.** Across new runs, policies issued a
  prayer command on roughly 93% to 99.5% of ticks, maintained overhead prayer
  roughly 46% to 54% of ticks, and lost about 87 to 94 prayer points per
  episode. Correct-prayer accuracy ranged from about 68.5% to 78.7%.
- **Targeting remained mechanically valid.** b5 had target range/LOS on about
  94% to 95% of target ticks, no target on about 23% of ticks, and attack-ready
  state on about 89% to 93% of ticks. Invalid movement and attack actions were
  normally below one per episode; invalid prayer actions were zero.
- **Target choice still favors Yt-MejKot.** b5 spent about 41% to 44% of target
  time on MejKot, versus 34% to 37% for 8q and 32% to 36% for hsf. Healer and
  Jad target shares were near zero because Jad encounters remained rare.
- **Backend mechanics did not drift.** Damage per attack cycle and damaging-hit
  probability were effectively identical by NPC type across all candidates
  and seeds. The sweep changed learned behavior, not combat damage math.
- **Reward accounting was healthy.** Progress reward dominated at roughly 51
  to 94 reward per episode; correct-prayer reward was about 4 to 8; prayer
  loss was about `-1.7` to `-1.88`. Reward clamping changed episode return by
  only about 0.05 to 0.07. Every intentionally disabled reward stayed zero.
- **Optimization was numerically stable.** Entropy ranged from about 1.46 to
  2.13, KL from 0.016 to 0.021, and clip fraction from 0.179 to 0.233. No run
  showed NaNs, OOM, loss failure, or backend contract mismatch.
- **Compute matched replay ratio.** b5 ran at about 1.20M to 1.26M SPS, 8q at
  1.30M to 1.39M, and hsf at up to 1.78M. VRAM was about 2.04 GB.

### Decision

Promote the `b5m07qqr` recipe under the official `v1.0` name as the single
primary config. Do not carry all three candidate INIs forward. Next validate
v1.0 at 1.5B across seeds 73, 101, and 202, then use the confirmed v1.0 recipe
for the policy architecture sweep. The
architecture sweep should be followed by a small learning-rate/entropy retune
and, separately, a boundary-extension sweep for parameters that favored a
search limit.

---

## v38 core hyperparameter Protein sweep (2026-07-14, completed - 48 runs)

PufferLib 4.0 Protein varied seven trainer hyperparameters while holding the
entire Fight Caves task fixed. The sweep used W&B group/tag
`fc_hparam_sweep_core_750m`, optimized final `cave_progress`, ran every trial
for 750M steps, and used source commit `856573792` with
`FC_LOADOUT_SOTA_TBOW`. All 48 runs finished and synchronized successfully.

### Search space

| Hyperparameter | Distribution | Minimum | Maximum |
|---|---|---:|---:|
| `learning_rate` | log normal | 0.0002 | 0.003 |
| `ent_coef` | log normal | 0.003 | 0.08 |
| `gamma` | logit normal | 0.994 | 0.9995 |
| `gae_lambda` | logit normal | 0.90 | 0.995 |
| `horizon` | powers of two | 128 | 512 |
| `minibatch_size` | powers of two | 4096 | 16384 |
| `replay_ratio` | uniform | 0.5 | 2.5 |

Protein controls were `max_runs=48`, one GPU, `max_suggestion_cost=1500`,
`downsample=5`, GPU suggestions enabled, Pareto pruning enabled, and
`early_stop_quantile=0.3`. No trial was early-stopped. The first two trials
were identical baseline evaluations; Protein proposed the remaining 46.

The fixed baseline trainer values were learning rate `0.0009`, entropy
`0.0242337458`, gamma `0.9963272487`, GAE lambda `0.9640527403`, horizon 256,
minibatch 4096, and replay ratio `1.5677333364`. All environment, reward,
observation, action, model, vector, optimizer/loss values not listed in the
search table were identical to the current canonical config.

### Data validation and aggregate outcome

- 48/48 runs finished; all had the same 145 metrics with no missing,
  nonnumeric, NaN, or infinite values.
- Final evaluations covered 481,825 episodes, 32,158 wave-63 reaches, and 121
  cave completions.
- 34 runs reached wave 63 at least once; 14 completed at least one cave.
- 34 of 46 non-baseline trials beat baseline final cave progress; 11 beat the
  1.5B historical run `v1fo07p3` using half its steps; seven beat its cave
  completion rate.
- The two baseline runs, `9txyihmv` and `8n659rab`, were identical on all 133
  gameplay/training-result metrics. They were deterministic duplicates, not
  independent seed replications.

### Full final ranking

| Rank | Run | Cave progress | Avg wave | Reach wave 63 | Completion rate |
|---:|---|---:|---:|---:|---:|
| 1 | `hsfj3i5p` | 0.91861 | 58.48 | 43.23% | 0.090% |
| 2 | `8qy4q7xc` | 0.90459 | 57.64 | 23.65% | 0.030% |
| 3 | `5wcflv85` | 0.90057 | 57.36 | 19.93% | 0.010% |
| 4 | `8q93t1vq` | 0.89869 | 57.24 | 36.32% | 0.257% |
| 5 | `goyl90gv` | 0.89827 | 57.24 | 28.97% | 0.080% |
| 6 | `1to7kay8` | 0.89345 | 56.92 | 21.63% | 0.020% |
| 7 | `2lu2lj5o` | 0.89188 | 56.81 | 18.71% | 0.040% |
| 8 | `808doep9` | 0.88078 | 56.10 | 13.52% | 0.020% |
| 9 | `b5m07qqr` | 0.87808 | 55.94 | 17.97% | 0.554% |
| 10 | `5t5b3mhr` | 0.87797 | 55.90 | 10.90% | 0.000% |
| 11 | `kckj5cvf` | 0.86374 | 55.05 | 6.03% | 0.000% |
| 12 | `h05wfefp` | 0.85981 | 54.81 | 0.64% | 0.000% |
| 13 | `hgtwczd5` | 0.85628 | 54.56 | 5.45% | 0.010% |
| 14 | `zo8597g1` | 0.85046 | 54.21 | 3.72% | 0.059% |
| 15 | `qa8p3k5p` | 0.85024 | 54.16 | 1.96% | 0.000% |
| 16 | `j8838hfn` | 0.85020 | 54.21 | 12.17% | 0.000% |
| 17 | `cn119id1` | 0.84480 | 53.83 | 19.90% | 0.010% |
| 18 | `kizq4hq9` | 0.84197 | 53.67 | 2.23% | 0.000% |
| 19 | `6eb1kr5h` | 0.83623 | 53.33 | 9.61% | 0.000% |
| 20 | `0e61idka` | 0.82927 | 52.92 | 4.92% | 0.000% |
| 21 | `zngapnrg` | 0.82239 | 52.44 | 0.82% | 0.000% |
| 22 | `zruz1dlb` | 0.81957 | 52.26 | 10.76% | 0.000% |
| 23 | `qk4bwpo5` | 0.81768 | 52.19 | 0.42% | 0.010% |
| 24 | `fse6vteg` | 0.80454 | 51.34 | 2.07% | 0.000% |
| 25 | `wlawm7n0` | 0.79850 | 50.96 | 0.00% | 0.000% |
| 26 | `pab7415h` | 0.78662 | 50.20 | 0.21% | 0.000% |
| 27 | `bhar5sxe` | 0.78481 | 50.05 | 0.00% | 0.000% |
| 28 | `qjil1c0n` | 0.78135 | 49.83 | 0.13% | 0.000% |
| 29 | `y75ca32j` | 0.76995 | 49.14 | 0.01% | 0.000% |
| 30 | `i7dj6ju5` | 0.76016 | 48.52 | 0.09% | 0.000% |
| 31 | `26qo2xkw` | 0.75928 | 48.51 | 0.50% | 0.010% |
| 32 | `385svhmf` | 0.75443 | 48.15 | 1.16% | 0.000% |
| 33 | `p37ygb4l` | 0.74186 | 47.38 | 0.19% | 0.000% |
| 34 | `d8u86rzm` | 0.73079 | 46.64 | 0.00% | 0.000% |
| 35 | `9txyihmv` | 0.72126 | 46.07 | 0.63% | 0.000% |
| 36 | `8n659rab` | 0.72126 | 46.07 | 0.63% | 0.000% |
| 37 | `k2flb76s` | 0.71110 | 45.57 | 0.00% | 0.000% |
| 38 | `w08yd6bn` | 0.65647 | 42.00 | 0.10% | 0.000% |
| 39 | `s02hm3mb` | 0.64485 | 41.39 | 0.00% | 0.000% |
| 40 | `ct0g5ml0` | 0.63473 | 40.64 | 0.00% | 0.000% |
| 41 | `w53rjt4y` | 0.61601 | 39.42 | 0.00% | 0.000% |
| 42 | `gosbutpz` | 0.56430 | 36.15 | 0.00% | 0.000% |
| 43 | `hf2nuch2` | 0.56266 | 36.26 | 0.00% | 0.000% |
| 44 | `jj5bt5du` | 0.55402 | 35.72 | 0.00% | 0.000% |
| 45 | `s0esw9xm` | 0.54460 | 34.96 | 0.00% | 0.000% |
| 46 | `ny5drhm7` | 0.54449 | 35.13 | 0.00% | 0.000% |
| 47 | `o8qt1zux` | 0.44884 | 28.90 | 0.00% | 0.000% |
| 48 | `6tfuiyym` | 0.41209 | 26.58 | 0.00% | 0.000% |

### Main findings

- The useful learning-rate region was about `0.0009` to `0.0013`; very low
  rates learned too slowly and rates above about `0.0015` were inconsistent.
- Entropy around `0.010` to `0.016` was strongest. The three finalists were
  43% to 48% below baseline entropy; values above about `0.026` were usually
  poor.
- Gamma around `0.9978` to `0.9988` and GAE lambda near `0.995` greatly
  lengthened credit assignment. The rough advantage trace increased from
  about 25 ticks at baseline to 128-162 ticks for the finalists.
- Horizon 256/minibatch 4096 was the best-supported pair. Horizon 128 learned
  poorly; 512 cost more and did not beat the best 256 results.
- Replay ratio was almost perfectly inversely related to SPS in the common
  256/4096 group (Spearman approximately `-0.98`). Ratios around 0.65-1.2
  offered the best learning/throughput compromise.
- The leading runs improved correct-prayer accuracy, target range/LOS time,
  required work removed per tick, and learning speed without changing backend
  damage distributions.
- Cave progress alone was insufficient for selection. `goyl90gv` ranked fifth
  but had a 2,073-tick longest wave, 13.25% healing/gross-damage ratio, and 55%
  MejKot target share. `8qy4q7xc` ranked second but also showed elevated
  longest-wave duration and healing.
- b5 ranked only ninth on progress but had the best Jad conversion: 15.6 Jad
  attack cycles per reach and 3.08% completion after reaching Jad. That is why
  it entered confirmation as the primary candidate.
- Better policies still issued prayer commands on about 97% to 98% of ticks
  and did not solve healer management. Those are environment-policy issues for
  later refinement, not evidence of trainer instability.

### Statistical limitations

Every Protein trial used native trainer seed 73, Protein sampled adaptively
rather than as a balanced factorial design, 21 unique configurations hit the
GAE upper bound, and cross-validated models explained only about 18% of held-
out cave-progress variance. The exact independent effect of each parameter is
therefore uncertain. Original sweep trials also did not save normal policy
checkpoints; normal retrains are required for viewer replay.

---

## Retired Fight Caves INI registry (2026-07-17 cleanup)

The repository now keeps one Fight Caves source config:
`runescape-rl/config/fight_caves.ini`, plus its synchronized Puffer runtime
copy. The 37 files below were removed after their purpose, values, and results
were recorded. Tracked historical files are recoverable byte-for-byte from
commit `856573792` or its ancestors with:

```bash
git show 856573792:<path-from-table>
```

The two July 2026 sweep INIs were not committed before cleanup, so their exact
fixed contract, sweep ranges, and finalist values are preserved in the two
sections immediately above. Their SHA-256 hashes remain recorded for audit.

### Family index

| Family | Retired files | Historical definition/results |
|---|---|---|
| v27 prayer/progress | `snapshots/v27*.ini` | `v27` through `v27.3` sections in this file; variants changed wave/Jad rewards and correct-prayer weights. |
| v28.8 reward leave-one-out | `snapshots/fight_caves_s1_*.ini` | `v28.8 Sweep 1` in `docs/sweep_history.md`; each file zeroed exactly one listed reward. |
| v29 optimizer probes | `snapshots/fight_caves_v29_*.ini` | v29 sweep analysis in `docs/sweep_history.md`; tested seed 7, no PER, lower entropy, LR annealing, and a 192-hidden policy. |
| v30/observation ablations | `fight_caves_v30_0.ini`, `fight_caves_obs*.ini` | v30 and observation-sweep sections in `docs/sweep_history.md`; removed dead waste keys and separately ablated NPC distance, incoming aggregates, or NPC-valid flags. |
| v35 retrain finalists | `v35_retrains/v35.1.ini` through `v35.7.ini` | `v35.1-v35.7` section in this file contains every source run and the complete varying-hparam table. |
| v36 no supplies | `archive/fight_caves_v36_no_consumables.ini` | `v36` section in this file, run `jta3lkgx`; v35.1 recipe with zero food/potions. |
| v38 Phase 7/no-supplies | `experiments/fight_caves_compare_*`, `phase7_*` | `fight_caves_improvement_plan.md` and `fc_revamp.md`; includes benchmark `2386cesn`, simplified run `rw70szhc`, and the later raw-progress reward path. |
| July core hparam sweep | `fight_caves_hparam_sweep_core_750m.ini` | Full 48-run search space, ranking, and analysis above. |
| July finalist confirmation | `fight_caves_hparam_finalists_seed_confirmation_750m.ini` | Exact candidates and all six new runs above. |

### Exact retired-file hashes

| Retired path | SHA-256 |
|---|---|
| `runescape-rl/config/archive/fight_caves_v36_no_consumables.ini` | `f03909e5d1fb811bd0a05844764ab19d6ccf14e77895c5016e419531b0d61c06` |
| `runescape-rl/config/experiments/fight_caves_compare_current_no_supplies_1b.ini` | `88a07e36cfd63236130c3719139a69cbede4dfd3451d135e65fd2b0513c6f724` |
| `runescape-rl/config/experiments/fight_caves_hparam_finalists_seed_confirmation_750m.ini` | `25e24735a429e3e27edf987bc71ebf713c69e7c0fb8e1c6ddabfd51cff3d5227` |
| `runescape-rl/config/experiments/fight_caves_hparam_sweep_core_750m.ini` | `8c5f225f6927c891c42303319fa2f4f5d4d44f8dafdae4db1206c1cd9dcb7ab6` |
| `runescape-rl/config/experiments/fight_caves_phase7_dev_1b.ini` | `788c014ec3aaca3421b11adeed1f2c37fd32d7e5eb21fca476537ce79008ff9a` |
| `runescape-rl/config/experiments/fight_caves_phase7_no_kiting_1b.ini` | `f066373906305cc97ab558d32e7c07aa5046f4e6579335d081ea653fbc4e4b63` |
| `runescape-rl/config/experiments/fight_caves_phase7_simplified_no_supplies_1b.ini` | `c243d6ab2d382fed9c0d18adc26ac9f6d138a336b77111fe6dd7a6dfc1c8b658` |
| `runescape-rl/config/snapshots/fight_caves_obs0.ini` | `3f3c89e10cf3ed9023549377bbf3414f8efc5e477de80af9266c369d86e27afd` |
| `runescape-rl/config/snapshots/fight_caves_obs1.ini` | `060dd9c5c8d1ca218cbe9c4c5ad9c4001eb3456a4dee1f8b24b2d9f8ada0564b` |
| `runescape-rl/config/snapshots/fight_caves_obs2.ini` | `b9eeb51b4c2a2a65c5d5e1a2d1627def70ade4696c38415148cd92e2293b566f` |
| `runescape-rl/config/snapshots/fight_caves_s1_01.ini` | `7396b763842cf4a8604654d202f9b739a51d98cd5676c654589661ab78f47222` |
| `runescape-rl/config/snapshots/fight_caves_s1_02.ini` | `2bf0e32e08a3e64960ea516bb4c3ff87f898b980ace31708eeebfa4fba038158` |
| `runescape-rl/config/snapshots/fight_caves_s1_03.ini` | `465fa9b0e513f005517c0ff96e5086e70917e6bca3dc356843f8f74902c5c5a2` |
| `runescape-rl/config/snapshots/fight_caves_s1_04.ini` | `e52d75d066008c883ac931cf7494ccd2ca5f38b03ccad34c4e482fd07e3f86ef` |
| `runescape-rl/config/snapshots/fight_caves_s1_05.ini` | `76f83b5e84f8473e9826420efc4743e7933fed4c5b7607839476509cb6ca7d89` |
| `runescape-rl/config/snapshots/fight_caves_s1_06.ini` | `41546b71f1bd28cfedeb3c249175f1691e7fbf18dc2348e4d23a685983ee73da` |
| `runescape-rl/config/snapshots/fight_caves_s1_07.ini` | `a47a487940390c1f7a162dd45aeef4fc247fe7523514a814f1625641fb66e4f0` |
| `runescape-rl/config/snapshots/fight_caves_s1_08.ini` | `ffc4a61b8199ec4bf0a55ee0a9c4fee49d887afc69ed23338a324304c3d059cb` |
| `runescape-rl/config/snapshots/fight_caves_s1_09.ini` | `260b56e233b41d194c63272b1a553ad82c137e42b7616b241a78a6e92c0c1eab` |
| `runescape-rl/config/snapshots/fight_caves_s1_10.ini` | `515d9dfd45655aff2060a73e872d09801defafd3e96a09fc4f1e33c5268ba474` |
| `runescape-rl/config/snapshots/fight_caves_v29_0.ini` | `57eea278e804ff977da7eb0a5f76747332f6b4cb1b00ff254e26027e7acf7268` |
| `runescape-rl/config/snapshots/fight_caves_v29_1.ini` | `e97ead056f5f1b55a71c18df5671201472d70ad5b83b84e59e87d3e53b8fa9c5` |
| `runescape-rl/config/snapshots/fight_caves_v29_2.ini` | `19c1aae7f34620da9e4231aa7c7996b3dc7985a2c8966a1e929dbed2a3f71875` |
| `runescape-rl/config/snapshots/fight_caves_v29_3.ini` | `c99bb4fdad9aac77997d4858766e8da06a6cb96a079447179c6ee87a299c1155` |
| `runescape-rl/config/snapshots/fight_caves_v29_4.ini` | `68a2933e60469408a8908851ceefbdfff3c08dc767b5206d3ce80a502adaf03a` |
| `runescape-rl/config/snapshots/fight_caves_v30_0.ini` | `01eb0336da3f0aada1f2abc1154be860911d99c2db8971c80a7b202a2aca1787` |
| `runescape-rl/config/snapshots/v27.1.ini` | `b0dc338b18e3f02e5ec9349be49baba2e874f3c5d8b12baafb3aed11c66053e7` |
| `runescape-rl/config/snapshots/v27.2.ini` | `95fdae97d6a8ccffa7df65c7bf3591e98dbc8d52483c1edb3ae7825574e05578` |
| `runescape-rl/config/snapshots/v27.3.ini` | `5da5bd89a81352fd985becbb976b21cd23e0c745309bbc5ecf50aa6275ed1a37` |
| `runescape-rl/config/snapshots/v27.ini` | `91ab08666ed9499bf8e20553554b152450451224643b652ec48ae2dba36a4608` |
| `runescape-rl/config/v35_retrains/v35.1.ini` | `12a9f6d1149ff06827db40672a758a68c08e9cc7d3ef508c6a30926e5352d19c` |
| `runescape-rl/config/v35_retrains/v35.2.ini` | `1d4825e003fe65070b194778f70d75949fc2765f5dd0c16a624629d9248410d5` |
| `runescape-rl/config/v35_retrains/v35.3.ini` | `674886c556e92e88c6e2d88e58c0830f776e7cd06210628480d38fd1f469eb16` |
| `runescape-rl/config/v35_retrains/v35.4.ini` | `639a266db4cd8ca007db88a83ab95a66f6da22fbb042783e348273e1212e3c87` |
| `runescape-rl/config/v35_retrains/v35.5.ini` | `ba6bec8568d85a6c278da70e4d182db96e0a8fe795d819b0fa6a26fb5f62cb3f` |
| `runescape-rl/config/v35_retrains/v35.6.ini` | `bb1fb09cc5c54b8f01478e23adf3450576cc16e1e309bfd327344becc320d2c8` |
| `runescape-rl/config/v35_retrains/v35.7.ini` | `780a36d37fc7b920a9e8c6c51bd9a854f259d1a7c80af34f3a3deb59079ce591` |

---

## v38 loadout sweep (2026-06-22, completed - 9 current-backend loadout runs - W&B group `loadout-sweep-1.5b-current-backend`)

Sequential cold-start training sweep across every loadout compiled into the
current `testing` backend. Each run used the same v35.1/SOTA training config,
the same current backend, and a different compile-time `FC_ACTIVE_LOADOUT`.
The sweep was launched from `/home/joe/projects/runescape-rl/fight-caves-rl`
with `FORCE_BACKEND_REBUILD=1` for every run so each loadout got a fresh
training extension instead of reusing a stale `_C*.so`.

**Bottom line.** The sweep completed all 9 loadouts to `1,499,463,680` agent
steps. `FC_LOADOUT_SOTA_TBOW` was the clear winner: W&B run `ov5qfn36`
finished at `jad_kill_rate = 0.910` with `reached_wave_63 = 0.970`. The next
best was `FC_LOADOUT_TBOW_MASORI` (`ymj1j1mi`) with peak `0.808` and final
`0.758`. `ACB Armadyl` and `Bowfa Crystal` learned late-wave access but mostly
failed to convert Jad, while the lower-level/pure loadouts did not produce
useful Jad kill rates under this config.

The key historical conclusion is that the current backend/config can still
produce a strong SOTA-style TBow policy when the backend is rebuilt for the
intended loadout. This result conflicts with the earlier broad diagnosis that
the current backend cannot reproduce v35.1-like behavior at all. The strongest
remaining suspicion for those failed reruns is stale or mismatched compiled
backend state, command/environment drift, or a run that did not actually use
the intended active loadout/config despite matching high-level W&B metadata.

### Baseline

Fixed recipe for every run:

- Config: `runescape-rl/config/fight_caves.ini`
- Config family: restored v35.1/SOTA recipe from the v34 long Protein top pick
  (`a3mi6u2g`) and v35.1 run family (`gacjanj0`)
- Backend: current `testing` branch backend at parent `3fe5908fc`, plus the
  loadout-sweep readiness changes committed with this history update
- Cold start: `load_model_path = null`
- W&B group: `loadout-sweep-1.5b-current-backend`
- W&B project: `fight caves rl`
- Training horizon override: `--train.total-timesteps 1500000000`
- Train seed: `train.seed = 42`
- Env/top-level seed: `seed = 73`
- Vec: `total_agents = 4096`, `num_buffers = 2`
- Policy: MinGRU, `hidden_size = 256`, `num_layers = 3`
- Supplies: full default supplies; `initial_sharks` and
  `initial_prayer_doses` are unset in the config and default to max supplies
- Run cadence: sequential, one GPU, one loadout per run

### Backend / code state

This sweep used the current Fight Caves backend, not the original v35.1 binary:

- Current backend has all 9 loadouts in `fc_player_init.h`.
- `FC_ACTIVE_LOADOUT` is a compile-time selector. The sweep set it before each
  run and forced a backend rebuild.
- Jad healer logic is the current v35-style correction:
  - fixed healer threshold at `150 HP`
  - healer respawn re-arms only after Jad is healed back to full HP
- Training parser supports `initial_sharks` and `initial_prayer_doses`, but
  this sweep left both unset.
- Bowfa/no-ammo support is present via `weapon_uses_ammo`.
- The training wrapper now records a backend stamp so future loadout changes
  trigger a rebuild instead of silently reusing the previous compiled loadout.

Historical caveat: PufferLib/W&B did not record `FC_ACTIVE_LOADOUT` as a
first-class config value. Loadout identity is reconstructed from the launch
order, W&B tags, and per-run command labels. Future sweeps should add the
active loadout into W&B config explicitly.

### Exact active config

Every run used this active config, with only `train.total_timesteps` overridden
from 3B to 1.5B at the CLI:

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
shape_food_waste_scale = -1.2
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
shape_jad_heal_penalty = -0.3
obs_ablate_npc_distance = 0
obs_ablate_incoming_aggregates = 1
obs_ablate_npc_valid = 0

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 1_500_000_000
anneal_lr = 0
learning_rate = 0.0009000000000000007
ent_coef = 0.02423374579539897
gamma = 0.9963272487242703
gae_lambda = 0.96405274026941
clip_coef = 0.17830599245832296
vf_coef = 1
vf_clip_coef = 0.15124043205980495
max_grad_norm = 0.25
horizon = 256
minibatch_size = 4096
replay_ratio = 1.567733336432473
vtrace_rho_clip = 0.5
vtrace_c_clip = 0.5037274754757021
prio_alpha = 0.9682355928752012
prio_beta0 = 0
beta1 = 0.95
beta2 = 0.9995810484472892
eps = 1e-10

[policy]
hidden_size = 256
num_layers = 3

[sweep]
metric = jad_kill_rate
```

### Loadouts

| Label | Symbol | Description |
|---|---|---|
| `black_dhide_rcb` | `FC_LOADOUT_BLACK_DHIDE_RCB` | 70 HP, 43 prayer, 70 defence, 70 ranged. Rune crossbow, black d'hide, adamant bolts, range 7, speed 5, ranged attack 153, ranged strength 100. |
| `sota_tbow` | `FC_LOADOUT_SOTA_TBOW` | Original SOTA loadout. 99 HP, 99 prayer, 99 defence, 99 ranged. Twisted bow, fortified Masori, dragon arrows, Venator ring, range 10, speed 5, ranged attack 215, ranged strength 99. |
| `low_def_rcb` | `FC_LOADOUT_LOW_DEF_RCB` | 55 HP, 43 prayer, 1 defence, 61 ranged. Robin hood / ranger tunic / red d'hide crossbow setup, range 7, speed 5, ranged attack 166, ranged strength 100, prayer bonus 8. |
| `rcb_pure` | `FC_LOADOUT_RCB_PURE` | 55 HP, 43 prayer, 1 defence, 61 ranged. Rune crossbow pure profile with same offense as `low_def_rcb` but lower defensive bonuses. |
| `msbi_pure` | `FC_LOADOUT_MSBI_PURE` | 60 HP, 43 prayer, 1 defence, 70 ranged. Magic shortbow (i), rune arrows, range 7, speed 3, ranged attack 141, ranged strength 49. |
| `blowpipe_pure` | `FC_LOADOUT_BLOWPIPE_PURE` | 75 HP, 43 prayer, 1 defence, 75 ranged. Toxic blowpipe with adamant darts, range 5, speed 2, ranged attack 101, ranged strength 42. |
| `acb_armadyl` | `FC_LOADOUT_ACB_ARMADYL` | 80 HP, 70 prayer, 75 defence, 80 ranged. Armadyl crossbow, Armadyl armour, diamond dragon bolts (e), range 8, speed 5, ranged attack 220, ranged strength 129. |
| `bowfa_crystal` | `FC_LOADOUT_BOWFA_CRYSTAL` | 85 HP, 70 prayer, 75 defence, 85 ranged. Bow of faerdhinen (c), crystal armour, no ammo use, range 10, speed 4, ranged attack 233, ranged strength 113. |
| `tbow_masori` | `FC_LOADOUT_TBOW_MASORI` | 99 HP, 77 prayer, 80 defence, 99 ranged. Twisted bow and Masori profile, range 10, speed 5, ranged attack 205, ranged strength 97. This is weaker than `sota_tbow` in prayer, defence, ranged attack, and ranged strength. |

### Results

Values below come from local PufferLib JSON logs in
`pufferlib_4/logs/fight_caves/*.json`. These logs are downsampled to five
points per run (`sweep.downsample = 5`), so peak values are representative
bucket peaks rather than every raw W&B epoch.

| Rank | Loadout | W&B run | Peak JKR | Last-3 avg JKR | Final JKR | Peak reach63 | Final reach63 | Final wave | Final Jad heal reward | Final stall reward |
|---:|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | `sota_tbow` | `ov5qfn36` | 0.910 | 0.756 | 0.910 | 0.970 | 0.970 | 62.82 | -20.58 | -24.04 |
| 2 | `tbow_masori` | `ymj1j1mi` | 0.808 | 0.716 | 0.758 | 0.895 | 0.895 | 62.41 | -41.61 | -195.57 |
| 3 | `bowfa_crystal` | `5dkbvzep` | 0.151 | 0.041 | 0.050 | 0.973 | 0.497 | 57.53 | -55.63 | -261.60 |
| 4 | `acb_armadyl` | `bczf8mij` | 0.117 | 0.040 | 0.011 | 0.972 | 0.972 | 62.69 | -54.34 | -257.55 |
| 5 | `black_dhide_rcb` | `7q6slwsq` | 0.021 | 0.008 | 0.000 | 0.389 | 0.323 | 54.93 | -13.29 | -231.05 |
| 6 | `blowpipe_pure` | `5o15d7a3` | 0.013 | 0.009 | 0.005 | 0.513 | 0.120 | 52.39 | -34.57 | -908.40 |
| 7 | `low_def_rcb` | `6by4xslb` | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 53.31 | 0.00 | 0.00 |
| 8 | `rcb_pure` | `gjbpdm8g` | 0.000 | 0.000 | 0.000 | 0.071 | 0.000 | 54.29 | 0.00 | 0.00 |
| 9 | `msbi_pure` | `je3gsp5c` | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 41.75 | 0.00 | -0.36 |

### Trajectory

Five logged points per run: approximately 0.19B / 0.56B / 0.94B / 1.31B /
1.50B agent steps.

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

### Diagnosis

- `sota_tbow` is the only loadout that fully matched the v35.1/SOTA training
  recipe. It did not merely reach Jad; it converted Jad reliably at the end of
  the 1.5B run.
- `tbow_masori` is close but not equivalent to `sota_tbow`. It has lower max
  prayer, lower defence level, lower ranged attack, lower ranged strength, and
  no Venator ring in the loadout definition. Its lower final JKR is consistent
  with that weaker profile.
- `acb_armadyl` reached Jad almost as often as `sota_tbow` by the end
  (`reached_wave_63 = 0.972`) but had only `0.011` final JKR. This is a
  Jad/healer conversion failure, not a wave-progression failure.
- `bowfa_crystal` showed early wave-63 access (`0.973` peak reach63) and a
  small Jad-kill signal, then regressed badly by the final bucket. The v35.1
  hparams are not stable for this loadout as-is.
- The pure/low-level loadouts are too hard or mismatched for this reward and
  PPO recipe at 1.5B. They likely need loadout-specific shaping, curriculum,
  longer budgets, or a separate sweep.
- The clean `sota_tbow` outcome makes stale compiled backend state an important
  risk to control in future repro work. `FC_ACTIVE_LOADOUT` is compile-time,
  and the previous compiled `_C*.so` can silently represent the wrong loadout
  unless rebuild/stamp checks are enforced.

### Artifacts

- Winning run: `ov5qfn36`
- Best available winning checkpoint directory:
  `pufferlib_4/checkpoints/fight_caves/ov5qfn36/`
- Final winning training checkpoint:
  `pufferlib_4/checkpoints/fight_caves/ov5qfn36/0000001573912576.bin`
- Local logs:
  - `pufferlib_4/logs/fight_caves/ov5qfn36.json`
  - `pufferlib_4/logs/fight_caves/ymj1j1mi.json`
  - `pufferlib_4/logs/fight_caves/5dkbvzep.json`
  - `pufferlib_4/logs/fight_caves/bczf8mij.json`
  - `pufferlib_4/logs/fight_caves/7q6slwsq.json`
  - `pufferlib_4/logs/fight_caves/5o15d7a3.json`
  - `pufferlib_4/logs/fight_caves/6by4xslb.json`
  - `pufferlib_4/logs/fight_caves/gjbpdm8g.json`
  - `pufferlib_4/logs/fight_caves/je3gsp5c.json`

## v37 (2026-06-13, completed - SOTA v35.1 config on current testing backend - `vi78c39x` / clear-universe-591)

Cold-start control run using the restored v35.1 SOTA active config and SOTA
loadout, but on the current `testing` branch backend/training stack. The run
was launched with W&B tag `v37`.

**Bottom line.** v37 did **not** reproduce the v35.1 SOTA peak or sustained
Jad performance. Peak `jad_kill_rate = 0.867` @ 1.353B versus SOTA
`gacjanj0` peak 0.949 @ 1.215B. Best saved checkpoint fell from 0.884 to
0.816, best 250M-step mean fell from 0.729 to 0.604, and last-10% mean fell
from 0.610 to 0.320. The final checkpoint looks deceptively close on raw Jad
kills (`0.434` vs 0.460), but v37 was reaching Jad much more often at the
end (`reached_wave_63 = 0.926` vs 0.590) and failing to close. Final healer
and stall penalties were dramatically worse: `rwd_jad_heal_total = -286.9`
and `rwd_wave_stall_total = -455.2`.

The main conclusion is that restoring the SOTA config/loadout was not enough
to restore SOTA behavior on the current backend. The current branch learns
late-wave access, but its Jad/healer closure is materially worse. The first
suspect remains backend drift around Jad healer timing/re-arm semantics, with
secondary possible influence from post-SOTA fc-training/render/asset changes.

### Baseline

Baseline target is v35.1 SOTA: W&B run `gacjanj0` / `sweet-plant-580`,
with `peak_jad_kill_rate = 0.949`.

### Config diff vs v35.1 SOTA

No active env/train/policy/vec/torch config diff. The local run logs for
`gacjanj0` and `vi78c39x` match exactly for these blocks:

- `[env]`
- `[train]`
- `[policy]`
- `[vec]`
- torch network/encoder/decoder

The only expected W&B/log metadata differences are the v37 tag, repo path,
repo remote, git commit, and newer current-branch code.

The removed v36 no-consumables fields are not active here:

| key | v35.1 SOTA | **v37** |
|---|---:|---:|
| `initial_sharks` | unset, defaults to full sharks | unset, defaults to full sharks |
| `initial_prayer_doses` | unset, defaults to full prayer doses | unset, defaults to full prayer doses |

### Loadout diff vs v35.1 SOTA

No active loadout diff. v37 uses the restored SOTA loadout:

- `FC_ACTIVE_LOADOUT = 1`
- loadout B: `Masori (f) + TBow`
- 99 hitpoints, 99 prayer, 99 defence, 99 ranged
- Twisted bow, Masori (f), dragon arrows
- Twisted-bow target magic-level scaling is active in combat

### Backend/code diff vs v35.1 SOTA

These differences remain in the current `testing` branch and are the main
possible explanatory variables for the failed reproduction:

- Jad healer behavior differs:
  - v35.1 SOTA: healers spawn once Jad drops below 50% HP; healer respawn
    is rearmed if Jad heals back above 50% HP and drops below 50% again.
  - current `testing`: healers spawn once Jad drops below 150 HP; healer
    respawn is rearmed only after Jad heals all the way back to full HP and
    then drops below 150 HP again.
  - Plain effect: current `testing` changes the timing and distribution of
    healer pressure. Despite being "later" on paper, v37 incurred much worse
    final healer/stall penalties, so this did not behave like a simple
    difficulty reduction for learning.
- Collision loading differs:
  - current `testing` checks `runescape-rl/fc-core/assets/fightcaves.collision`
    as an additional asset path and prints a warning before falling back to
    an open arena if collision data is missing.
- Training-env resource knobs exist:
  - current `testing` can parse `initial_sharks` and
    `initial_prayer_doses`, but v37 leaves both unset, so defaults remain
    full `FC_MAX_SHARKS` and full `FC_MAX_PRAYER_DOSES`.
- Train/build portability differs:
  - current `testing` uses `PYTHON_BIN`, robust virtualenv lookup, and NVML
    versioned-link fallback logic.
- Render/viewer/asset stack differs:
  - current `testing` includes newer UI/assets/cache pipeline work and a
    shared asset resolver. This should not change core training dynamics
    directly, but it is still repo drift relative to the original SOTA run.
- Removed/non-runtime repo surface:
  - RunPod deployment files were removed.
  - Markdown docs are ignored except the root `README.md`.

### Exact active config

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
shape_food_waste_scale = -1.2
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
shape_jad_heal_penalty = -0.3
obs_ablate_npc_distance = 0
obs_ablate_incoming_aggregates = 1
obs_ablate_npc_valid = 0

[vec]
total_agents = 4096
num_buffers = 2
num_threads = 16

[train]
total_timesteps = 3_000_000_000
anneal_lr = 0
learning_rate = 0.0009000000000000007
ent_coef = 0.02423374579539897
gamma = 0.9963272487242703
gae_lambda = 0.96405274026941
clip_coef = 0.17830599245832296
vf_coef = 1
vf_clip_coef = 0.15124043205980495
max_grad_norm = 0.25
horizon = 256
minibatch_size = 4096
replay_ratio = 1.567733336432473
vtrace_rho_clip = 0.5
vtrace_c_clip = 0.5037274754757021
prio_alpha = 0.9682355928752012
prio_beta0 = 0
beta1 = 0.95
beta2 = 0.9995810484472892
eps = 1e-10
seed = 42

[policy]
hidden_size = 256
num_layers = 3
expansion_factor = 1

[sweep]
metric = jad_kill_rate
```

### Results - v37 vs v35.1 SOTA

| metric | v35.1 SOTA (`gacjanj0`) | **v37 (`vi78c39x`)** | Delta |
|---|---:|---:|---:|
| peak `jad_kill_rate` | **0.949 @ 1.215B** | 0.867 @ 1.353B | -0.082 |
| best 250M mean `jad_kill_rate` | **0.729** | 0.604 | -0.125 |
| last-10% mean `jad_kill_rate` | **0.610** | 0.320 | -0.290 |
| best saved checkpoint `jad_kill_rate` | **0.884 @ 1.626B** | 0.816 @ 1.417B | -0.069 |
| final `jad_kill_rate` | **0.460** | 0.434 | -0.027 |
| final `reached_wave_63` | 0.590 | **0.926** | +0.337 |
| final `wave_reached` | 57.48 | **62.45** | +4.97 |
| final `episode_length` | 7,740 | 9,314 | +1,575 |
| final `dmg_taken_avg` | **3,181** | 3,282 | +101 |
| final `correct_prayer` | 1,725 | **2,077** | +351 |
| final `wrong_prayer_hits` | **20.06** | 32.92 | +12.85 |
| final `no_prayer_hits` | **29.00** | 29.22 | +0.22 |
| final `prayer_switches` | 4,727 | 6,152 | +1,425 |
| final `pots_used` | 32.00 | 32.00 | 0.00 |
| final `food_eaten` | 15.46 | **14.15** | -1.31 |
| final `rwd_jad_heal_total` | **-12.61** | -286.87 | -274.26 |
| final `rwd_wave_stall_total` | **-103.34** | -455.15 | -351.81 |
| final `rwd_invalid_action_total` | **-282.17** | -446.73 | -164.55 |

### Reward-channel breakdown - final

| channel | v35.1 SOTA | **v37** | Delta |
|---|---:|---:|---:|
| `damage_dealt` | 1489.16 | 1855.13 | +365.97 |
| `damage_taken` | -145.86 | -184.02 | -38.16 |
| `npc_kill` | 875.71 | 945.69 | +69.99 |
| `wave_clear` | 25173.24 | 29432.65 | +4259.41 |
| `jad_kill` | 920.86 | 867.65 | -53.22 |
| `player_death` | -5.94 | -6.23 | -0.29 |
| `food_waste` | -4.46 | -3.18 | +1.28 |
| `pot_waste` | -25.49 | -26.15 | -0.66 |
| `correct_jad_prayer` | 90.31 | 253.35 | +163.03 |
| `correct_danger_prayer` | 389.66 | 440.45 | +50.78 |
| `wrong_danger_prayer` | -13.43 | -17.07 | -3.64 |
| `unnecessary_prayer` | -82.92 | -72.49 | +10.42 |
| `melee_pressure` | -25.61 | -31.96 | -6.35 |
| `wasted_attack` | -143.76 | -178.49 | -34.74 |
| `kiting` | 2055.75 | 2511.76 | +456.01 |
| `safespot_attack` | 2296.89 | 2817.87 | +520.98 |
| `wave_stall` | -103.34 | -455.15 | -351.81 |
| `jad_heal` | -12.61 | -286.87 | -274.26 |
| `invalid_action` | -282.17 | -446.73 | -164.55 |
| `tick_penalty` | -38.50 | -47.26 | -8.76 |

### Trajectory

Bucketed comparison:

| step | SOTA `jad_kill_rate` | **v37 `jad_kill_rate`** | SOTA `reached_wave_63` | **v37 `reached_wave_63`** | SOTA `wave_reached` | **v37 `wave_reached`** |
|---:|---:|---:|---:|---:|---:|---:|
| 0.500B | 0.000 | 0.000 | 0.847 | 0.000 | 62.51 | 45.14 |
| 1.000B | **0.663** | 0.605 | **0.949** | 0.814 | **62.38** | 61.14 |
| 1.500B | **0.527** | 0.129 | 0.747 | **0.822** | 60.60 | **62.35** |
| 2.000B | **0.460** | 0.261 | 0.675 | **0.775** | 58.97 | **61.42** |
| 2.500B | **0.694** | 0.376 | **0.865** | 0.723 | **61.54** | 61.05 |
| 3.000B | **0.460** | 0.434 | 0.590 | **0.926** | 57.48 | **62.45** |

Top raw v37 `jad_kill_rate` points:

| step | `jad_kill_rate` | `reached_wave_63` | `wave_reached` | heal total | stall total |
|---:|---:|---:|---:|---:|---:|
| 1.353B | **0.867** | 0.959 | 62.84 | -192.16 | -266.27 |
| 1.311B | 0.858 | 1.000 | 63.00 | -257.42 | -562.49 |
| 1.315B | 0.843 | 0.961 | 62.93 | -178.00 | -229.35 |
| 1.405B | 0.830 | 0.991 | 62.97 | -243.20 | -447.25 |
| 1.401B | 0.822 | 1.000 | 63.00 | -233.01 | -374.56 |
| 1.420B | 0.819 | 0.976 | 62.91 | -237.98 | -272.63 |
| 1.352B | 0.816 | 0.928 | 62.73 | -209.90 | -489.17 |
| 1.417B | 0.816 | 0.991 | 62.96 | -259.44 | -418.80 |

Best saved checkpoint:

```text
<repo-root>/pufferlib_4/checkpoints/fight_caves/vi78c39x/0000001416626176.bin
```

`jad_kill_rate = 0.816`, `reached_wave_63 = 0.991`,
`wave_reached = 62.96`.

### Diagnosis

- **The SOTA recipe did not reproduce on the current backend.** Config,
  hparams, policy size, vectorization, and active loadout match SOTA, but
  peak, best saved checkpoint, best 250M mean, and last-10% mean are all
  meaningfully worse.
- **The failure mode is Jad closure, not cave access.** Final v37 reached
  wave 63 in 92.6% of eval episodes, versus 59.0% for the SOTA final row,
  but final `jad_kill_rate` was still slightly lower. The gap between
  `reached_wave_63` and `jad_kill_rate` is about 49 points in v37, versus
  about 13 points in SOTA.
- **Healer/stall pressure is the main metric-level signature.** Final
  `rwd_jad_heal_total` worsened by 274 points and `rwd_wave_stall_total`
  worsened by 352 points relative to SOTA. The agent is surviving/reaching
  Jad, but spending too much of that time in bad healer/stall states.
- **The current backend made late-Jad learning less stable.** On paper,
  spawning healers at 150 HP and only rearming after full heal sounds less
  frequent than the old 50% threshold behavior. In practice, v37 appears to
  delay or reshape healer pressure in a way the old SOTA recipe does not
  handle cleanly.
- **Early learning was also slower.** At 0.5B steps, SOTA was already
  reaching wave 63 at 84.7%, while v37 was still around wave 45 with zero
  wave-63 reach. v37 eventually learned cave access, but the delayed
  acquisition likely reduced its total high-quality Jad practice.

### Recommendation

If the goal is to reproduce/compare against true v35.1 SOTA, run the same
config/loadout on the old SOTA healer backend: 50% HP healer threshold and
threshold-crossing re-arm. That isolates whether the v37 regression is
primarily the Jad healer backend change. If keeping the current backend, the
next work should target Jad closure directly: healer priority, anti-heal
shaping, and late-Jad stall/movement penalties rather than more general
early-wave or resource changes.

### Artifacts

- W&B run: `vi78c39x` / `clear-universe-591`
- W&B tag: `v37`
- W&B URL:
  `https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/vi78c39x`
- W&B run metadata commit:
  `03ae8a65c810c5d8fed536828607e0f0ff3e08ce`
- Local W&B dir:
  `pufferlib_4/wandb/wandb/run-20260613_005039-vi78c39x/`
- Local config/log:
  `pufferlib_4/logs/fight_caves/vi78c39x.json`
- Checkpoints:
  `pufferlib_4/checkpoints/fight_caves/vi78c39x/`
- Best saved checkpoint:
  `0000001416626176.bin`

## v36.2 (2026-06-12, completed - Bowfa loadout diagnostic - `t2rtfitt`)

Loadout/backend experiment from v36 (`jta3lkgx`): keep the same v36
no-consumables config, hparams, observation shape, action heads, reward
weights, and training length, but replace the loadout-B Twisted bow setup
with Bow of faerdhinen and no ammunition. The run was launched with tag
`v36-bowfa`.

**Bottom line.** Bowfa made the early and middle cave much easier, but it
was a clear regression on the objective that matters. Peak
`jad_kill_rate = 0.527` @ 1.598B, well below v36's 0.808 @ 2.600B. Best
saved checkpoint fell from 0.691 to 0.449. Final `jad_kill_rate` collapsed
to 0.000 even while final `reached_wave_63` improved from 0.685 to 0.964
and final `wave_reached` improved from 57.58 to 62.86. This is not an
early-wave failure; it is a Jad-closure failure after faster cave access.

The key lesson is that Bowfa is **not** a direct DPS buff over the previous
TBow backend against Ket-Zek/Jad. Bowfa has better visible flat ranged
attack/strength and a 4-tick cooldown, but the old TBow backend applied
magic-level scaling. That made TBow substantially stronger against the
high-magic late targets even though Bowfa is stronger against low/mid cave
NPCs.

### Config diff vs v36

No W&B config diff. The `[env]`, `[vec]`, `[train]`, `[policy]`, and
`[sweep]` values match v36. This was a code/loadout swap, not a reward or
hparam run.

### Backend/loadout diff vs v36

| key | v36 | **v36.2** |
|---|---:|---:|
| loadout-B weapon | Twisted bow + dragon arrows | **Bow of faerdhinen** |
| `weapon_kind` | `FC_WEAPON_TWISTED_BOW` | `FC_WEAPON_BOW_OF_FAERDHINEN` |
| `weapon_speed` | 5 | **4** |
| `weapon_range` | 10 | 10 |
| `ranged_atk` | 215 | **273** |
| `ranged_str` | 99 | **125** |
| `def_magic` | 150 | **152** |
| `ammo` | 50000 | **0** |

Backend changes paired with the run:

- `fc-core/include/fc_player_init.h`: loadout B now uses Bowfa stats and
  built-in/no-ammo semantics.
- `fc-core/src/fc_tick.c`: player attacks are allowed for Bowfa even when
  `ammo_count == 0`; ammo is decremented only for ammo-using weapons.
- `fc-core/src/fc_combat.c`: removed Twisted-bow-specific target magic
  scaling and uses the normal ranged attack/max-hit formula.
- Viewer/assets changes updated the displayed equipment and item sprites,
  but the training-relevant behavioral changes are the three core files
  above.

### Exact active config

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
shape_food_waste_scale = -1.2
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
shape_jad_heal_penalty = -0.3
initial_sharks = 0
initial_prayer_doses = 0
obs_ablate_npc_distance = 0
obs_ablate_incoming_aggregates = 1
obs_ablate_npc_valid = 0

[vec]
total_agents = 4096
num_buffers = 2
num_threads = 16

[train]
total_timesteps = 3_000_000_000
anneal_lr = 0
learning_rate = 0.0009000000000000007
ent_coef = 0.02423374579539897
gamma = 0.9963272487242703
gae_lambda = 0.96405274026941
clip_coef = 0.17830599245832296
vf_coef = 1
vf_clip_coef = 0.15124043205980495
max_grad_norm = 0.25
horizon = 256
minibatch_size = 4096
replay_ratio = 1.567733336432473
vtrace_rho_clip = 0.5
vtrace_c_clip = 0.5037274754757021
prio_alpha = 0.9682355928752012
prio_beta0 = 0
beta1 = 0.95
beta2 = 0.9995810484472892
eps = 1e-10
seed = 42

[policy]
hidden_size = 256
num_layers = 3
expansion_factor = 1

[sweep]
metric = jad_kill_rate
```

### Results - v36.2 vs v36

| metric | v36 TBow (`jta3lkgx`) | **v36.2 Bowfa (`t2rtfitt`)** | Delta |
|---|---:|---:|---:|
| peak `jad_kill_rate` | **0.808 @ 2.600B** | 0.527 @ 1.598B | -0.280 |
| best 250M mean `jad_kill_rate` | **0.611** | 0.320 | -0.292 |
| last-10% mean `jad_kill_rate` | **0.565** | 0.014 | -0.551 |
| best saved checkpoint `jad_kill_rate` | **0.691 @ 1.417B** | 0.449 @ 1.679B | -0.243 |
| final `jad_kill_rate` | **0.432** | 0.000 | -0.432 |
| final `reached_wave_63` | 0.685 | **0.964** | +0.279 |
| final `wave_reached` | 57.58 | **62.86** | +5.28 |
| final `episode_length` | 7,929 | **5,535** | -2,395 |
| final `dmg_taken_avg` | **1,656** | 2,743 | +1,087 |
| final `correct_prayer` | **1,801** | 1,051 | -750 |
| final `wrong_prayer_hits` | **17.65** | 40.15 | +22.49 |
| final `no_prayer_hits` | **26.19** | 31.91 | +5.71 |
| final `prayer_switches` | **4,905** | 2,231 | -2,675 |
| final `pots_used` | 0.00 | 0.00 | 0.00 |
| final `food_eaten` | 0.00 | 0.00 | 0.00 |
| final `rwd_jad_heal_total` | **-132.32** | -175.25 | -42.93 |
| final `rwd_wave_stall_total` | -343.99 | **-128.87** | +215.12 |
| final `rwd_invalid_action_total` | -321.64 | **-241.23** | +80.41 |

The better final wave-stall total is misleading: v36.2 dies much more
often after reaching Jad, so it spends less time in long stalled Jad states.
It is not solving the Jad phase better.

### Reward-channel breakdown - final

| channel | v36 | **v36.2** | Delta |
|---|---:|---:|---:|
| `damage_dealt` | 1538.37 | 1368.27 | -170.10 |
| `damage_taken` | -79.30 | -149.08 | -69.78 |
| `npc_kill` | 848.75 | **947.75** | +99.00 |
| `wave_clear` | 24990.74 | **29064.14** | +4073.40 |
| `jad_kill` | **864.20** | 0.00 | -864.20 |
| `player_death` | -6.25 | -11.00 | -4.75 |
| `food_waste` | 0.00 | 0.00 | 0.00 |
| `pot_waste` | 0.00 | 0.00 | 0.00 |
| `correct_jad_prayer` | 138.36 | **225.49** | +87.13 |
| `correct_danger_prayer` | **396.88** | 220.21 | -176.67 |
| `wrong_danger_prayer` | -11.96 | -19.88 | -7.92 |
| `unnecessary_prayer` | -29.56 | **-15.69** | +13.86 |
| `melee_pressure` | -19.34 | -24.39 | -5.05 |
| `wasted_attack` | -147.49 | **-121.31** | +26.18 |
| `kiting` | **2098.69** | 1735.96 | -362.73 |
| `safespot_attack` | **2356.81** | 2016.93 | -339.88 |
| `wave_stall` | -343.99 | **-128.87** | +215.12 |
| `jad_heal` | **-132.32** | -175.25 | -42.93 |
| `invalid_action` | -321.64 | **-241.23** | +80.41 |
| `tick_penalty` | -39.46 | **-27.22** | +12.25 |

### Trajectory

Selected trajectory points:

| step | `jad_kill_rate` | `reached_wave_63` | `wave_reached` | damage taken | heal total | stall total |
|---:|---:|---:|---:|---:|---:|---:|
| 0.500B | 0.015 | 0.824 | 62.00 | 4,082 | -189.2 | -682.8 |
| 0.750B | 0.382 | 0.782 | 61.09 | 3,504 | -161.8 | -1045.6 |
| 1.000B | 0.011 | 0.957 | 62.75 | 3,194 | -127.9 | -400.5 |
| 1.501B | 0.211 | 0.966 | 62.87 | 2,644 | -163.1 | -395.8 |
| 1.750B | 0.294 | 0.948 | 62.78 | 2,729 | -147.7 | -176.0 |
| 2.250B | 0.459 | 0.962 | 62.73 | 2,115 | -169.6 | -385.3 |
| 2.750B | 0.032 | 0.968 | 62.90 | 2,893 | -306.5 | -1444.6 |
| 3.000B | 0.000 | 0.964 | 62.86 | 2,743 | -175.3 | -128.9 |

Top raw `jad_kill_rate` points:

| step | `jad_kill_rate` | `reached_wave_63` | `wave_reached` | heal total | stall total |
|---:|---:|---:|---:|---:|---:|
| 1.598B | **0.527** | 0.964 | 62.76 | -142.91 | -220.62 |
| 2.221B | **0.527** | 0.989 | 62.93 | -178.10 | -542.19 |
| 2.414B | 0.517 | 0.966 | 62.83 | -139.40 | -102.51 |
| 1.601B | 0.510 | 0.967 | 62.73 | -152.08 | -256.45 |
| 1.607B | 0.505 | 0.974 | 62.87 | -179.90 | -525.03 |

Best saved checkpoint:

```text
<repo-root>/pufferlib_4/checkpoints/fight_caves/t2rtfitt/0000001678770176.bin
```

`jad_kill_rate = 0.449`, `reached_wave_63 = 0.976`, `wave_reached = 62.94`.

### Why this did worse than TBow

The run is worse because the swap changed the damage distribution, not just
the agent's raw power.

- **Bowfa buffs low/mid waves.** The final run clears more waves and reaches
  Jad far more often: `wave_clear_total` +4073 and `reached_wave_63`
  +0.279. This is exactly what we would expect from higher flat ranged
  attack/strength against low-magic NPCs.
- **TBow was still stronger against the late high-magic targets.** Under
  the old backend formulas, rough per-attack numbers against Jad were:
  TBow max hit about 58 HP at 62.6% hit chance on a 5-tick cycle; Bowfa max
  hit about 32 HP at 56.6% on a 4-tick cycle. That is roughly 3.63 HP/tick
  for TBow vs 2.26 HP/tick for Bowfa before accounting for movement and
  target switching. Ket-Zek has the same direction: TBow's scaling beats
  Bowfa's flat stats.
- **Longer/weaker Jad closure exposes prayer mistakes.** Final
  `wrong_prayer_hits` more than doubled (17.65 -> 40.15), `no_prayer_hits`
  increased, `correct_prayer` fell by ~750, and every final-eval episode
  ended in player death (`rwd_player_death_fires = 1.0` in the summary).
- **The policy learned fast cave access before stable Jad technique.** By
  0.5B it already reached wave 63 in 82% of episodes, long before v36 did,
  but Jad kill rate remained unstable and collapsed late.
- **The timing distribution changed.** Weapon cooldown moved from 5 to 4
  ticks. That is a legitimate buff in isolation, but it also changes attack
  timer observations, attack-ready cadence, and reward timing. This makes
  the run not a pure "same task but better weapon" comparison.

### Diagnosis

v36.2 is useful evidence that the no-supplies task is very sensitive to the
late-wave damage model. Bowfa improves access to Jad, but the previous TBow
implementation was carrying a lot of Jad/Ket-Zek closure through magic-level
scaling. Removing that scaling makes the final phase longer and less
forgiving, and the no-supplies policy does not survive the extra prayer
burden.

This also explains the apparently contradictory metrics: higher
`reached_wave_63` plus zero final `jad_kill_rate` means the agent gets to
the boss fight reliably and then dies there.

### Recommendation

Do not treat Bowfa as a drop-in upgrade over the existing TBow sim for SOTA
comparison. For an actual Bowfa track, evaluate the best checkpoint around
1.68B, then train with a Jad/healer curriculum or add more style-aware
prayer/Jad-closure shaping. If the intended experiment is "same task but
strictly stronger player DPS", keep TBow or explicitly model the intended
late-target DPS rather than assuming Bowfa's flat item stats dominate the
old TBow magic scaling.

### Artifacts

- W&B run: `t2rtfitt`
- W&B tag: `v36-bowfa`
- Local W&B dir:
  `pufferlib_4/wandb/wandb/run-20260612_193231-t2rtfitt/`
- Local log:
  `pufferlib_4/logs/fight_caves/t2rtfitt.json`
- Checkpoints:
  `pufferlib_4/checkpoints/fight_caves/t2rtfitt/`
- Best saved checkpoint:
  `0000001678770176.bin`

---

## v36 (2026-05-18, completed - no food/no pots diagnostic - `jta3lkgx`)

Single-variable experiment from v35 (`8z4lqldl`): keep the same backend,
reward config, hparams, policy shape, observation shape, and action heads,
but start every episode with `initial_sharks = 0` and
`initial_prayer_doses = 0`. The point was diagnostic: remove resource
crutches and see whether prayer/positioning technique improves when food
and prayer pots are unavailable.

**Bottom line.** No-supplies training did learn cleaner survival behavior,
but it did not beat v35 on peak or best-checkpoint quality. Peak
`jad_kill_rate = 0.808` @ 2.600B, below v35's 0.965. Final
`jad_kill_rate = 0.432`, better than v35's collapsed final 0.200, and
last-10% mean improved from 0.333 to 0.565. However, final
`reached_wave_63` dropped from 0.882 to 0.685 and final wave reached fell
from 62.46 to 57.58. The run is useful evidence that supplies were masking
some sloppy technique, but zero supplies makes the early/mid cave less
reliable and gives a lower best deployable checkpoint.

### Config diff vs v35

| key | v35 | **v36** |
|---|---:|---:|
| `initial_sharks` | implicit `20` (`FC_MAX_SHARKS`) | **0** |
| `initial_prayer_doses` | implicit `32` (`FC_MAX_PRAYER_DOSES`) | **0** |

All reward weights, hparams, policy architecture, vectorization, and obs
ablation flags were unchanged.

### Backend diff vs v35

Added config plumbing so the supply experiment is explicit and reproducible:

- `fc-training/fight_caves.h`: added `initial_sharks` and
  `initial_prayer_doses` to `FightCaves`; `c_reset` clamps and applies them
  after `fc_reset`.
- `fc-training/binding.c`: parses `initial_sharks` and
  `initial_prayer_doses` from `[env]`, defaulting to full supplies when the
  keys are absent.
- `fc-training/fight_caves.c`: standalone harness defaults to full supplies.
- `fc-viewer/src/viewer.c`: viewer/eval reset reads the same config keys, so
  replay and manual viewer state match training.
- `config/fight_caves.ini`: set both supply keys to 0.

Action space did **not** change. Training still uses five heads:
`move`, `attack`, `prayer`, `eat`, `drink`. With zero supplies, the eat and
drink masks make only `none` valid, so food/pots are not options while the
policy output shape remains comparable to previous checkpoints.

### Exact active config

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
shape_food_waste_scale = -1.2
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
shape_jad_heal_penalty = -0.3
initial_sharks = 0
initial_prayer_doses = 0
obs_ablate_npc_distance = 0
obs_ablate_incoming_aggregates = 1
obs_ablate_npc_valid = 0

[vec]
total_agents = 4096
num_buffers = 2
num_threads = 16

[train]
total_timesteps = 3_000_000_000
anneal_lr = 0
learning_rate = 0.0009000000000000007
ent_coef = 0.02423374579539897
gamma = 0.9963272487242703
gae_lambda = 0.96405274026941
clip_coef = 0.17830599245832296
vf_coef = 1
vf_clip_coef = 0.15124043205980495
max_grad_norm = 0.25
horizon = 256
minibatch_size = 4096
replay_ratio = 1.567733336432473
vtrace_rho_clip = 0.5
vtrace_c_clip = 0.5037274754757021
prio_alpha = 0.9682355928752012
prio_beta0 = 0
beta1 = 0.95
beta2 = 0.9995810484472892
eps = 1e-10
seed = 42

[policy]
hidden_size = 256
num_layers = 3
expansion_factor = 1

[sweep]
metric = jad_kill_rate
```

### Results - v36 vs v35

| metric | v35 (`8z4lqldl`) | **v36 (`jta3lkgx`)** | Delta |
|---|---:|---:|---:|
| peak `jad_kill_rate` | **0.965** @ 2.356B | 0.808 @ 2.600B | -0.158 |
| best 250M mean `jad_kill_rate` | **0.736** | 0.611 | -0.124 |
| last-10% mean `jad_kill_rate` | 0.333 | **0.565** | +0.231 |
| best saved checkpoint `jad_kill_rate` | **0.906** @ 2.360B | 0.691 @ 1.417B | -0.215 |
| final `jad_kill_rate` | 0.200 | **0.432** | +0.232 |
| final `reached_wave_63` | **0.882** | 0.685 | -0.197 |
| final `wave_reached` | **62.46** | 57.58 | -4.88 |
| final `episode_length` | 8,960 | 7,929 | -1,030 |
| final `dmg_taken_avg` | 2,930 | **1,656** | -1,274 |
| final `correct_prayer` | **2,104** | 1,801 | -303 |
| final `wrong_prayer_hits` | 21.98 | **17.65** | -4.33 |
| final `no_prayer_hits` | 26.64 | **26.19** | -0.45 |
| final `prayer_switches` | 5,748 | **4,905** | -842 |
| final `pots_used` | 32.00 | **0.00** | -32.00 |
| final `food_eaten` | 9.03 | **0.00** | -9.03 |
| final `rwd_jad_heal_total` | -212.90 | **-132.32** | +80.58 |
| final `rwd_wave_stall_total` | -358.89 | **-343.99** | +14.90 |
| final `rwd_invalid_action_total` | -361.47 | **-321.64** | +39.84 |

### Reward-channel breakdown - final

| channel | v35 | **v36** | Delta |
|---|---:|---:|---:|
| `damage_dealt` | 1763.22 | 1538.37 | -224.85 |
| `damage_taken` | -171.03 | -79.30 | +91.73 |
| `npc_kill` | 933.64 | 848.75 | -84.89 |
| `wave_clear` | 28685.32 | 24990.74 | -3694.58 |
| `jad_kill` | 400.00 | 864.20 | +464.20 |
| `player_death` | -8.80 | -6.25 | +2.55 |
| `food_waste` | -1.73 | 0.00 | +1.73 |
| `pot_waste` | -20.71 | 0.00 | +20.71 |
| `correct_jad_prayer` | 182.97 | 138.36 | -44.61 |
| `correct_danger_prayer` | 457.24 | 396.88 | -60.36 |
| `wrong_danger_prayer` | -12.79 | -11.96 | +0.84 |
| `unnecessary_prayer` | -84.06 | -29.56 | +54.50 |
| `melee_pressure` | -20.22 | -19.34 | +0.88 |
| `wasted_attack` | -168.71 | -147.49 | +21.22 |
| `kiting` | 2391.71 | 2098.69 | -293.03 |
| `safespot_attack` | 2688.85 | 2356.81 | -332.03 |
| `wave_stall` | -358.89 | -343.99 | +14.90 |
| `jad_heal` | -212.90 | -132.32 | +80.58 |
| `invalid_action` | -361.47 | -321.64 | +39.84 |
| `tick_penalty` | -45.04 | -39.46 | +5.57 |

### Trajectory

Top raw `jad_kill_rate` points:

| step | `jad_kill_rate` | `reached_wave_63` | `wave_reached` | heal total | stall total |
|---:|---:|---:|---:|---:|---:|
| 2.600B | **0.808** | 0.981 | 62.73 | -82.36 | -71.25 |
| 2.608B | 0.763 | 0.956 | 62.75 | -90.67 | -194.36 |
| 2.829B | 0.757 | 0.953 | 62.83 | -69.81 | -97.85 |
| 2.692B | 0.750 | 0.939 | 62.15 | -42.19 | -52.42 |
| 1.413B | 0.741 | 0.874 | 61.68 | -17.02 | -9.16 |

The best v36 checkpoint is not at the raw peak because checkpoints land
every 50 epochs. Best saved checkpoint:

```text
<repo-root>/pufferlib_4/checkpoints/fight_caves/jta3lkgx/0000001416626176.bin
```

`jad_kill_rate = 0.691`, `reached_wave_63 = 0.821`, `wave_reached = 61.20`.

### Diagnosis

- **Technique improved under resource pressure.** Damage taken fell by
  43% at final (2,930 -> 1,656), wrong-prayer hits fell, melee/range/magic
  prayer uptimes all fell, and the agent no longer burns resources because
  it has none.
- **Reliability fell.** Final `reached_wave_63` dropped from 0.882 to 0.685.
  The policy is cleaner when it survives, but no supplies means fewer
  episodes get enough late-wave/Jad reps.
- **Peak quality regressed.** Peak and best saved checkpoint both trail v35.
  The no-supply setting is useful as a diagnostic or curriculum component,
  not as the current deployment recipe.
- **Jad closure still needs separate work.** Final heal/stall pressure is
  better than v35 but still large: `rwd_jad_heal_total = -132` and
  `rwd_wave_stall_total = -344`. Removing supplies did not solve healer/Jad
  closure by itself.

### Recommendation

Keep v36 as a diagnostic result. For training quality, the better next move
is not zero supplies permanently; it is style-aware prayer shaping plus a
curriculum/ablation around supplies. A reasonable follow-up is:

- normal supplies for mainline SOTA attempts,
- a short no-food/no-pot curriculum or auxiliary run for technique pressure,
- add a style-aware unnecessary/wrong-overhead penalty so the agent learns
  not to flick melee/range when only magic is active.

### Artifacts

- W&B run: `jta3lkgx`
- Local W&B dir:
  `pufferlib_4/wandb/wandb/run-20260518_190333-jta3lkgx/`
- Local log:
  `pufferlib_4/logs/fight_caves/jta3lkgx.json`
- Checkpoints:
  `pufferlib_4/checkpoints/fight_caves/jta3lkgx/`
- Best saved checkpoint:
  `0000001416626176.bin`

---

## v35 (2026-05-18, completed - Jad healer respawn backend correction - `8z4lqldl`)

Backend-only change from the prior SOTA retrain (`gacjanj0`): keep the same
active config and training recipe, but correct Jad healer respawn behavior.
The previous backend re-armed healer spawning whenever Jad healed back above
the threshold. The corrected backend only re-arms the healer wave after Jad
has been healed all the way back to full HP. This was meant to remove the
viewer-observed immediate healer respawn loop after the player killed healers.

**Bottom line.** v35 set a new raw peak and best saved checkpoint, but it
ended worse than the prior SOTA run. Peak `jad_kill_rate = 0.965` @ 2.356B
versus prior SOTA `gacjanj0` peak 0.949. Best saved checkpoint improved from
0.884 to 0.906. Final behavior was worse: final `jad_kill_rate = 0.200`
versus 0.460, even though final `reached_wave_63` improved from 0.590 to
0.882. The policy reached Jad more often but stalled/fumbled closure:
final heal pressure jumped from -12.6 to -212.9 and wave-stall pressure from
-103.3 to -358.9.

### Config diff vs prior SOTA `gacjanj0`

No config diff. W&B `[env]`, `[vec]`, `[train]`, and `[policy]` values match
`gacjanj0`. v35 had full supplies through the backend constants:

```ini
initial_sharks = 20              # implicit FC_MAX_SHARKS; key absent in W&B config
initial_prayer_doses = 32        # implicit FC_MAX_PRAYER_DOSES; key absent in W&B config
```

### Backend diff vs prior SOTA

- `fc-core/include/fc_wave.h`: replaced 50% HP fraction threshold with
  `FC_JAD_HEALER_THRESHOLD_HP_TENTHS = 1500` (150 HP).
- `fc-core/src/fc_tick.c`: healer respawn latch now resets only when Jad
  reaches full HP. Crossing above the threshold is no longer enough.
- `fc-core/include/fc_types.h`: updated `jad_healers_spawned` comment to
  reflect full-HP re-arm semantics.
- `fc-viewer/tests/test_headless.c`: added regression coverage that healers
  do not respawn after Jad merely heals above threshold, and do respawn after
  full heal plus another threshold crossing.

### Exact active config

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
shape_food_waste_scale = -1.2
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
shape_jad_heal_penalty = -0.3
obs_ablate_npc_distance = 0
obs_ablate_incoming_aggregates = 1
obs_ablate_npc_valid = 0

[vec]
total_agents = 4096
num_buffers = 2
num_threads = 16

[train]
total_timesteps = 3_000_000_000
anneal_lr = 0
learning_rate = 0.0009000000000000007
ent_coef = 0.02423374579539897
gamma = 0.9963272487242703
gae_lambda = 0.96405274026941
clip_coef = 0.17830599245832296
vf_coef = 1
vf_clip_coef = 0.15124043205980495
max_grad_norm = 0.25
horizon = 256
minibatch_size = 4096
replay_ratio = 1.567733336432473
vtrace_rho_clip = 0.5
vtrace_c_clip = 0.5037274754757021
prio_alpha = 0.9682355928752012
prio_beta0 = 0
beta1 = 0.95
beta2 = 0.9995810484472892
eps = 1e-10
seed = 42

[policy]
hidden_size = 256
num_layers = 3
expansion_factor = 1

[sweep]
metric = jad_kill_rate
```

### Results - v35 vs prior SOTA `gacjanj0`

| metric | prior SOTA (`gacjanj0`) | **v35 (`8z4lqldl`)** | Delta |
|---|---:|---:|---:|
| peak `jad_kill_rate` | 0.949 @ 1.215B | **0.965 @ 2.356B** | +0.016 |
| best 250M mean `jad_kill_rate` | 0.729 | **0.736** | +0.006 |
| last-10% mean `jad_kill_rate` | **0.610** | 0.333 | -0.277 |
| best saved checkpoint `jad_kill_rate` | 0.884 @ 1.626B | **0.906 @ 2.360B** | +0.022 |
| final `jad_kill_rate` | **0.460** | 0.200 | -0.260 |
| final `reached_wave_63` | 0.590 | **0.882** | +0.292 |
| final `wave_reached` | 57.48 | **62.46** | +4.98 |
| final `episode_length` | 7,740 | 8,960 | +1,220 |
| final `dmg_taken_avg` | 3,181 | **2,930** | -251 |
| final `correct_prayer` | 1,725 | **2,104** | +379 |
| final `wrong_prayer_hits` | **20.06** | 21.98 | +1.92 |
| final `no_prayer_hits` | 29.00 | **26.64** | -2.36 |
| final `prayer_switches` | 4,727 | 5,748 | +1,021 |
| final `pots_used` | 32.00 | 32.00 | 0.00 |
| final `food_eaten` | 15.46 | **9.03** | -6.43 |
| final `rwd_jad_heal_total` | **-12.61** | -212.90 | -200.29 |
| final `rwd_wave_stall_total` | **-103.34** | -358.89 | -255.55 |
| final `rwd_invalid_action_total` | **-282.17** | -361.47 | -79.30 |

### Reward-channel breakdown - final

| channel | prior SOTA | **v35** | Delta |
|---|---:|---:|---:|
| `damage_dealt` | 1489.16 | 1763.22 | +274.05 |
| `damage_taken` | -145.86 | -171.03 | -25.17 |
| `npc_kill` | 875.71 | 933.64 | +57.94 |
| `wave_clear` | 25173.24 | 28685.32 | +3512.08 |
| `jad_kill` | 920.86 | 400.00 | -520.86 |
| `player_death` | -5.94 | -8.80 | -2.86 |
| `food_waste` | -4.46 | -1.73 | +2.74 |
| `pot_waste` | -25.49 | -20.71 | +4.78 |
| `correct_jad_prayer` | 90.31 | 182.97 | +92.66 |
| `correct_danger_prayer` | 389.66 | 457.24 | +67.58 |
| `wrong_danger_prayer` | -13.43 | -12.79 | +0.64 |
| `unnecessary_prayer` | -82.92 | -84.06 | -1.14 |
| `melee_pressure` | -25.61 | -20.22 | +5.39 |
| `wasted_attack` | -143.76 | -168.71 | -24.96 |
| `kiting` | 2055.75 | 2391.71 | +335.97 |
| `safespot_attack` | 2296.89 | 2688.85 | +391.96 |
| `wave_stall` | -103.34 | -358.89 | -255.54 |
| `jad_heal` | -12.61 | -212.90 | -200.29 |
| `invalid_action` | -282.17 | -361.47 | -79.30 |
| `tick_penalty` | -38.50 | -45.04 | -6.54 |

### Trajectory

Top raw `jad_kill_rate` points:

| step | `jad_kill_rate` | `reached_wave_63` | `wave_reached` | heal total | stall total |
|---:|---:|---:|---:|---:|---:|
| 2.356B | **0.965** | 1.000 | 63.00 | -125.67 | -161.79 |
| 2.357B | 0.956 | 0.993 | 62.88 | -132.42 | -187.18 |
| 2.353B | 0.955 | 0.993 | 62.96 | -125.77 | -107.63 |
| 0.718B | 0.942 | 0.968 | 62.66 | -10.24 | 0.00 |
| 2.358B | 0.941 | 0.992 | 62.97 | -139.74 | -223.93 |

Best saved checkpoint:

```text
<repo-root>/pufferlib_4/checkpoints/fight_caves/8z4lqldl/0000002360344576.bin
```

`jad_kill_rate = 0.906`, `reached_wave_63 = 0.961`, `wave_reached = 62.52`.

### Diagnosis

- **Backend correction improved peak capability but hurt final stability.**
  v35 produced the best raw peak and best saved checkpoint we have from this
  pair, but the late training state was worse.
- **The policy reaches Jad more often but closes worse.** Final
  `reached_wave_63` improved by 0.292 while final `jad_kill_rate` dropped by
  0.260. That means the regression is not early cave navigation; it is Jad
  closure.
- **Healer pressure explains the final collapse.** `rwd_jad_heal_total`
  worsened from -12.6 to -212.9 and `rwd_wave_stall_total` worsened from
  -103.3 to -358.9. The run spends much more time in bad Jad/healer states.
- **Food use dropped and damage fell.** v35 ate fewer sharks and took less
  damage, so the healer change did not simply make the whole fight harder.
  It changed the late-wave/Jad distribution.

### Recommendation

Use v35's best checkpoint for viewer inspection when we want the strongest
post-healer-correction policy, but do not treat the final checkpoint as
deployment quality. The next backend/reward work should target Jad closure:
style-aware prayer shaping, healer/Jad attack priority, and possibly a
light movement/stall cost around Jad rather than more generic resource
changes.

### Artifacts

- W&B run: `8z4lqldl`
- Local W&B dir:
  `pufferlib_4/wandb/wandb/run-20260518_164002-8z4lqldl/`
- Local log:
  `pufferlib_4/logs/fight_caves/8z4lqldl.json`
- Checkpoints:
  `pufferlib_4/checkpoints/fight_caves/8z4lqldl/`
- Best saved checkpoint:
  `0000002360344576.bin`

---

## v35.1 – v35.7 (2026-05-04, queued for tomorrow — retrains of v34_sweep_long top picks)

Reproduces the seven best v34_sweep_long trials so we have local checkpoints
to evaluate in the viewer. **Why retrain:** the v34 sweep ran with
`no_model_upload: true` (verified in each run's local config.yaml), so no
trial uploaded a model artifact to W&B. Combined with the sweep's
local-checkpoint Pareto-pruner, the SOTA `.bin` files exist neither locally
nor remotely. Retrains are the only way to produce eval checkpoints for
these recipes.

**One config per pick.** Each retrain replicates exactly one v34 trial's
hparams (lifted byte-for-byte from `pufferlib_4/wandb/wandb/run-*-<id>/files/config.yaml`).
The [env] block matches v32.0 baseline (held constant across the v34 sweep).
`total_timesteps` is fixed at 3e9 so retrains run to full convergence rather
than stopping wherever Pareto pruning happened to kill the original.

**Picks (ranked by v34 peak jad_kill_rate, stable runs only):**

| Tag | Source `run_id` | v34 peak | v34 final | last3-mean | last3-std | Steps | Picked because |
|---|---|---:|---:|---:|---:|---:|---|
| **v35.1** | `a3mi6u2g` | **0.886** | **0.886** | 0.733 | — | 2.03 B | absolute SOTA, stable peak/final |
| **v35.2** | `fpdao4ac` | 0.826 | 0.826 | **0.739** | **0.075** | 2.34 B | most-consistent SOTA (highest sustained, lowest std among top-2) |
| **v35.3** | `a7wp0gy2` | 0.684 | 0.684 | 0.533 | — | 1.99 B | rank-5 stable, only 2-layer policy in this set (recipe diversity) |
| **v35.4** | `3ftyvwwy` | 0.667 | 0.667 | 0.574 | 0.071 | 2.29 B | rank-7 stable, 2nd-most-consistent overall |
| **v35.5** | `n6kxxu6e` | 0.615 | 0.615 | 0.557 | **0.042** | ~2.10 B | lowest variance run in the sweep — most reproducible |
| **v35.6** | `m1176z7o` | 0.626 | 0.626 | 0.556 | 0.098 | ~2.10 B | top of the consistency-table ranking, distinct recipe (2-layer + low gamma) |
| **v35.7** | `s9ycbdpp` | 0.644 | 0.644 | 0.620 | — | 1.67 B | rank-10 stable peak, lower lr than the others (recipe diversity) |

Skipped from the headline top-10: `fvikxb56` (peak 0.728, final 0.316 —
crashed) and `txt7fvpp` (peak 0.709, final 0.338 — crashed). These recipes
produce instability so retraining them isn't useful for eval.

**Hparam patterns across the 7 picks** (these are the recipe lessons the
sweep found):

| Hparam | v32.0 baseline | Range across picks | Note |
|---|---|---|---|
| `learning_rate` | 3e-4 | **9e-4** (6 of 7) and 7.75e-4 (s9ycbdpp) | Pinned at sweep upper bound — Protein wanted higher lr than v32. |
| `gamma` | 0.999 | 0.9900 – 0.9963 | All picks use **lower** gamma than v32. |
| `replay_ratio` | 1.0 (default) | 1.44 – 1.60 | Higher than default; clusters tightly. |
| `ent_coef` | 0.01 | 0.0153 – 0.0300 | Mixed; no clear winner direction. |
| `vf_coef` | 0.5 | 0.5 – 1.0 | a3mi6u2g maxed at 1.0; others in middle. |
| `vf_clip_coef` | 0.2 | 0.10 – 0.51 | Wide spread. |
| `clip_coef` | 0.2 | 0.108 – 0.265 | Wide spread. |
| `hidden_size` | 256 | **256 (all 7)** | Sweep range was 128–512; all converged here. |
| `num_layers` | 2 | 2 (a7wp0gy2, m1176z7o) or 3 (the other 5) | 3-layer dominates the top picks. |
| `total_agents` | 4096 | **4096 (all 7)** | Pinned. |
| `num_buffers` | 2 | 1 (5 picks) or 2 (a3mi6u2g, 3ftyvwwy) | Mixed. |
| `horizon` | 256 | **256 (all 7)** | Pinned. |
| `minibatch_size` | 4096 | **4096 (all 7)** | Pinned. |

The picks are deliberately diverse on the policy/optimizer dimensions that
varied while sharing the convergence anchors that didn't (everyone agrees
on hidden=256, agents=4096, horizon=256, mb=4096). That tells us:
**fixed knobs → drop from v35 sweep**; **varied knobs are still where the
gain is**.

**Files generated (all under `runescape-rl/config/v35_retrains/`):**
- `v35.1.ini` … `v35.7.ini` — full configs ready to consume by `train.sh`.

**How to launch each retrain (tomorrow):**

```bash
cd <repo-root>/runescape-rl
CONFIG_PATH=config/v35_retrains/v35.1.ini ./train.sh   # a3mi6u2g recipe
CONFIG_PATH=config/v35_retrains/v35.2.ini ./train.sh   # fpdao4ac recipe
# ...etc through v35.7
```

Each will produce a new W&B run with its own ID and write `.bin`
checkpoints to `pufferlib_4/checkpoints/fight_caves/<new_run_id>/`.
**Expected wall time per retrain at ~1.9M SPS, 3B steps: ~26-35 min.**
Total wall time for all 7: ~3-4 hours sequential.

**To watch any retrain in the viewer once it has a checkpoint:**

```bash
cd <repo-root>/runescape-rl
python3 fc-viewer/eval_viewer.py --ckpt <repo-root>/pufferlib_4/checkpoints/fight_caves/<new_run_id>/<latest>.bin
```

**Pre-flight checklist:**
- [ ] Flip `no_model_upload` to false in pufferlib's checkpoint flow (or set ENV
  override) so v35 retrains DO upload artifacts — otherwise we re-create
  the same "no checkpoints to eval" problem in 24 hours.
- [ ] Optionally add `--tag v35.<N>` to each `train.sh` invocation so the
  retrains are easy to pull from W&B as a group.
- [ ] Consider dropping `seed = 42` (currently inherited from the v34 trial
  config) for variety, or keep it for exact reproducibility — explicit choice.

**Artifacts:** see `runescape-rl/config/v35_retrains/v35.{1..7}.ini`.

---

## v32.0 (2026-04-27, completed — `k9gsilze` / feasible-breeze-356)

Single-variable change from the obs.1 reproduction baseline (`5qfm1zbj`,
byte-identical to obs.1 `6g2i2gmy`): added `shape_jad_heal_penalty = -0.3`
(was 0.0/absent). Re-introduces a per-tick penalty multiplied by
`state.jad_heal_procs_this_tick` to make Yt-HurKot heal procs costly.
Original removal was in the v28-era graveyard cleanup (the old penalty
was deemed redundant when `w_damage_dealt` was assumed to indirectly
incentivize healer attacks); behavioral analysis of obs.1 showed the
agent never targets healers in practice, so the penalty was reinstated.

**Bottom line — NEW SOTA peak.** Peak `jad_kill_rate = 0.810` @ 1.98B
(vs obs.1's 0.702 @ 1.22B → **+15.4% absolute peak**). Best-250M-window
`0.489` vs obs.1's `0.261` (**+87.2%**). Last-10% mean `0.313` vs `0.244`
(**+28.6%**). Stability `0.387` vs `0.347` (+12%). The penalty was
intended to make the agent target healers; it did NOT. `npc_kill_fires`
stayed flat at 263 vs obs.1's 267 — no healer-targeting behavior change.
Instead the penalty acted as a **time pressure** on the Jad fight,
pushing the agent to commit damage faster once on Jad. The agent absorbs
~−122 reward/episode in heal penalties on average and still nets a
significant Jad-rate win.

### Config diff

**vs obs.1 repro** (immediate baseline):

| key | obs.1 repro | **v32.0** |
|---|---|---|
| `shape_jad_heal_penalty` | 0.0 (absent) | **−0.3** |

All other env/reward/policy/vec/train keys byte-identical. Obs ablation
flags (`obs_ablate_npc_distance=0`, `obs_ablate_incoming_aggregates=1`,
`obs_ablate_npc_valid=0`) preserved — same 122-float policy obs surface,
same encoder shape `nn.Linear(158, 256)`, same deterministic init flow.

### Backend diff

Three-file wiring restored alongside the config change (the channel /
parameter had been removed in the v28-era cleanup):

- `fc-core/include/fc_reward.h`: added `shape_jad_heal_penalty` to
  `FcRewardParams`, `jad_heal` field to `FcRewardBreakdown`, `FC_CH_JAD_HEAL`
  enum, `"jad_heal"` to `FC_CH_NAMES`, default 0 in `fc_reward_default_params`,
  multiplier `out.jad_heal = w × state->jad_heal_procs_this_tick` (fires
  only when both nonzero), included in `out.total`.
- `fc-training/fight_caves.h`: added `shape_jad_heal_penalty` field to
  `FightCaves` struct + wiring in `fc_reward_params_from_env`.
- `fc-training/binding.c`: kwarg parse with default 0 fallback.
- `fc-training/fight_caves.c`: standalone-harness default-init.
- `state.jad_heal_procs_this_tick` was already tracked
  (`fc_npc.c:290` increment, `fc_tick.c:54` reset) — no state changes.

### Rationale — per-tick economics

Healers heal Jad on a 4-tick interval; max 2 active healers ⇒ peak heal
rate ~0.5 procs/tick. At `w = -0.3`:

| heals/tick | per-tick cost | per-tick budget pressure |
|---|---|---|
| 0 (healers dead/distracted) | 0 | none |
| 1 | −0.3 | 60× tick_penalty |
| 2 (peak both healers fire) | −0.6 | 120× tick_penalty |
| avg over Jad fight | ~0.15 | comparable to `correct_jad_prayer ≈ 0.19/tick` peak gain |

Sized to be **modest** so the agent isn't drowned in penalty before it
figures out the source — the user explicitly flagged "may fire every
tick for a while until agent figures out what is causing it."

### Results — full metric sweep

**Core performance:**

| metric | obs.1 (`6g2i2gmy`) | **v32.0** (`k9gsilze`) | Δ |
|---|---|---|---|
| **peak `jad_kill_rate`** | 0.702 | **0.810** | **+0.108 (+15.4%)** |
| peak step | 1.225B | 1.982B | +0.76B (later) |
| **best 250M window** | 0.261 | **0.489** | **+0.228 (+87.2%)** |
| best-250M start | 1.25B | 2.50B | — |
| **last-10% mean** | 0.244 | **0.313** | **+0.070 (+28.6%)** |
| stability (L10/peak) | 0.347 | 0.387 | +0.040 (+11.5%) |
| peak `reached_wave_63` | 1.000 | 1.000 | tied |
| final `jad_kill_rate` | 0.174 | 0.112 | −0.062 (−35.6%) |
| final `reached_wave_63` | 0.833 | 0.879 | +0.046 |
| final `wave_reached` | 62.13 | 61.91 | −0.22 |
| final `episode_length` | 8,760 | 8,667 | −93 |

The "−35.6% on final" is the literal last-log point of two runs that
both collapse from peak. v32.0 peaks **later** (1.98B vs 1.22B), so its
collapse extends past the training budget; the proper comparisons are
peak / best-250M / last-10%, all big wins.

**Behavior metrics (final state):**

| metric | obs.1 | **v32.0** | Δ |
|---|---|---|---|
| `prayer_switches` | 386.0 | 393.9 | +7.8 (+2.0%) |
| `correct_prayer` | 1,388 | 1,435 | +47 (+3.4%) |
| `wrong_prayer_hits` | 293.0 | 292.3 | −0.6 |
| `prayer_uptime_melee` | 0.007 | 0.007 | +0% |
| `prayer_uptime_range` | 0.260 | 0.316 | +21.4% |
| `prayer_uptime_magic` | 0.432 | 0.389 | −10.1% |
| `damage_blocked` (channel) | — | — | (not in summary) |
| `dmg_taken_avg` | 8,017 | 8,037 | +20 (~flat) |
| `food_eaten` | 18.31 | 14.20 | **−22.5%** |
| `food_wasted` (fires) | 5.27 | 2.28 | **−56.7%** |
| `pots_used` | 30.26 | 30.53 | +0.27 |
| `most_npcs_slayed` | 277 | 280 | +3 |

Prayer-switching cadence essentially unchanged. Range prayer uptime up,
magic uptime down — slight rebalance, possibly because the agent now
finishes Jad waves faster (less time spent on Ket-Zek magic prayer
later). Food management improved (−57% food-waste fires) — agent eats
when actually low, suggesting better risk evaluation.

### Reward-channel breakdown (final state, per episode avg)

| channel | obs.1 total | v32.0 total | Δ | obs.1 fires | v32.0 fires | Δ fires |
|---|---:|---:|---:|---:|---:|---:|
| `damage_dealt` | +1,689 | +1,656 | −33 | 1,726 | 1,692 | −34 |
| `damage_taken` | −280 | −271 | +9 | 73.4 | 73.4 | 0 |
| `npc_kill` | +936 | +921 | −15 | 267.3 | **263.0** | **−4.3** |
| `wave_clear` | +28,639 | +28,144 | −495 | 61.2 | 60.4 | −0.8 |
| `jad_kill` | +348 | +224 | **−124 (−35.6%)** | 0.174 | 0.112 | −0.062 |
| `player_death` | −9.1 | −9.8 | −0.7 | 0.83 | 0.89 | +0.06 |
| `food_waste` | −3.4 | −1.4 | +2.0 | 5.3 | 2.3 | −3.0 |
| `pot_waste` | 0 | 0 | 0 | 0.02 | 0.06 | +0.04 |
| `correct_jad_prayer` | +110 | +109 | −1.6 | 73.5 | 72.4 | −1.1 |
| `correct_danger_prayer` | +321 | +331 | +10.1 | 1,284 | 1,325 | +40.6 |
| `wrong_danger_prayer` | −102 | −99 | +3.4 | 339.9 | 328.6 | −11.3 |
| `unnecessary_prayer` | −51 | −44 | +7.7 | 257.1 | 218.6 | −38.5 |
| `melee_pressure` | −276 | −300 | −24.8 | 275.6 | 306.5 | +31.0 |
| `wasted_attack` | −182 | −180 | +1.6 | 1,815 | 1,799 | −16 |
| `kiting` | +2,228 | +2,221 | −6.6 | 1,013 | 1,010 | −3 |
| `safespot_attack` | +2,531 | +2,486 | −45 | 1,687 | 1,657 | −30 |
| `wave_stall` | 0 | 0 | 0 | 0 | 0 | 0 |
| **`jad_heal`** | **0** | **−121.9** | **−121.9 (new)** | **0** | **108.3** | **+108.3** |
| `invalid_action` | −161 | −168 | −7 | 1,609 | 1,679 | +70 |
| `tick_penalty` | −44 | −43 | +0.9 | 8,756 | 8,569 | −186 |

Headline channel deltas:

- `jad_heal = −121.92` per episode (new). 108 ticks/episode where heals
  fire, costing avg −1.13/tick when fired (i.e. ~3.8 heals per fire-tick
  on average, consistent with multi-heal stacking when both healers
  proc same tick).
- `npc_kill_fires = −4.3` (267 → 263). **No healer-targeting behavior.**
  The penalty did not redirect the action policy toward Yt-HurKot.
- `correct_danger_prayer_fires = +40.6` (1,284 → 1,325). Slightly more
  correct prayer blocks per episode — incidental from spending less time
  cycling Jad waves.
- `unnecessary_prayer_fires = −38.5` (257 → 219). Less prayer-on with
  no threat. Cleaner prayer discipline.
- `wave_clear_total = −495` (28,639 → 28,144). Slightly less wave-clear
  reward — the agent dies earlier on Jad sometimes (final wave_reached
  61.9 vs 62.1).

### Trajectory by 250M buckets

| bucket | obs.1 jad / ent / pry_sw | **v32.0** jad / ent / pry_sw / heal_fires |
|---|---|---|
| [0.00, 0.25)B | 0.000 / 4.79 / 422 | 0.000 / 4.79 / 422 / 0 |
| [0.25, 0.50)B | 0.000 / 2.88 / 168 | 0.000 / 2.88 / 168 / 0 |
| [0.50, 0.75)B | 0.000 / 2.66 / 136 | 0.000 / 2.65 / 136 / 0 |
| [0.75, 1.00)B | 0.007 / 2.26 / 242 | 0.000 / 2.23 / 195 / 0 |
| [1.00, 1.25)B | **0.177** / 1.76 / 282 | 0.028 / 1.96 / 233 / 1.2 |
| [1.25, 1.50)B | 0.261 / 1.85 / 431 | 0.012 / 2.03 / 214 / 1.0 |
| [1.50, 1.75)B | 0.138 / 1.71 / 384 | **0.253** / 1.71 / 307 / 12.0 |
| [1.75, 2.00)B | 0.070 / 2.00 / 392 | 0.290 / 1.57 / 341 / 22.2 |
| [2.00, 2.25)B | 0.131 / 1.92 / 344 | 0.325 / 1.49 / 452 / 68.6 |
| [2.25, 2.50)B | 0.220 / 1.62 / 440 | 0.344 / 1.44 / 423 / 85.7 |
| [2.50, 2.75)B | 0.190 / 1.58 / 412 | **0.489** / 1.30 / 377 / 62.2 |
| [2.75, 3.00)B | 0.243 / 1.58 / 407 | 0.275 / 1.16 / 398 / 102.1 |

The two runs are **bit-identical through 0.75B** (same RNG flow until
sequences diverge). Divergence pattern:

- **Through 1.5B**: v32.0 LAGS obs.1 — by 1.0–1.5B, obs.1 is at 0.18
  jad while v32.0 is at 0.01–0.03. The penalty is a confusing signal
  early; the agent is still learning Jad mechanics and now also has to
  account for an unfamiliar negative pressure.
- **At 1.5–1.75B**: v32.0 catches up dramatically (0.253 vs 0.138) —
  this is when entropy drops to ~1.7 and the heal_fires count starts
  growing (12/ep), indicating the agent is consistently reaching Jad
  AND absorbing the penalty.
- **2.0–2.75B**: v32.0 climbs to 0.489 best-window mean while obs.1
  oscillates 0.13–0.22. Entropy continues dropping (1.30 vs obs.1's
  1.58).
- **2.75–3.00B**: v32.0 starts to collapse (0.275). Without the heal
  penalty's pressure off, the policy probably enters its own collapse
  mode — same shape as obs.1, just timeshifted later by ~0.75B.

### Diagnosis — heal penalty as time-pressure, not target-redirect

The hypothesis going in was **target-redirect**: penalty makes healers
costly → agent learns to attack them → fewer heals → cleaner Jad fight.

The actual mechanism is **time-pressure**:

- `npc_kill_fires` is essentially flat (267 → 263). Agent does NOT
  attack healers more.
- `heal_fires` GROWS over training (0 → 102 ticks/ep) because the agent
  reaches Jad MORE often, not less. You can't get healed if you never
  arrive.
- Per-tick cost of inaction once healers are alive ≈ 0.4 (avg), which
  is ~80× the `tick_penalty` of −0.005. The agent feels strong pressure
  to commit Jad damage faster.
- The improvement comes from **finishing Jad faster** (lower episode
  length: 8,667 vs 8,760) and **eating less defensively** (food_eaten
  −22.5%, food_waste fires −57%).

This is fine — we got a SOTA either way — but the design intuition for
v32.x should be updated: heal_penalty is a time-pressure lever, not a
healer-targeting lever. To actually get healer attacks, would need a
positive `w_healer_kill` bonus on top of (or in place of) the penalty.

### What worked

- **Single-variable change from obs.1 produced new SOTA.** +15% peak,
  +87% best-250M, +29% last-10%. Clear deterministic improvement.
- **Reward-channel logging caught the divergence cleanly.** The new
  `rwd_jad_heal_total/_fires` channels make it possible to reason about
  what the penalty actually costs the agent per episode.
- **Penalty sizing was right.** −0.3 is small enough that the agent
  didn't fail to converge, large enough to bend the trajectory upward
  meaningfully. Per-tick cost ~0.15 avg ≈ `correct_jad_prayer` peak
  gain — the two pressures balance.
- **Food management improved as a side-effect.** food_waste fires
  dropped 57%. Likely because the time pressure pushes the agent to
  use food more efficiently rather than panic-eat.

### What didn't work

- **Did not produce healer-targeting behavior.** `npc_kill_fires` flat.
  The agent absorbed −122 reward/episode rather than spend the action
  budget killing healers. If healer attacks specifically are the goal,
  −0.3 is insufficient pressure to redirect.
- **Slowed early Jad convergence.** v32.0 lagged obs.1 by 0.5B steps
  through the first frontier breakthrough. Cost ~0.25B of effective
  training time to "learn around" the penalty signal before the new
  trajectory emerged.
- **Single-seed result.** Per the obs.1 / v31 lessons, a single peak of
  0.81 isn't yet a confirmed real improvement — needs a confirmation
  seed before being treated as a robust SOTA. PufferLib determinism
  means the run reproduces exactly under the same code/config, but
  doesn't tell us about variance under torch RNG re-seeding.

### Recommendations for next runs

- **v32.0 confirmation:** re-run with a different torch seed (would
  need an explicit `torch.manual_seed` change in pufferl.py) to verify
  the +15% peak isn't a single-seed fluke.
- **v32.1 — stronger heal penalty (`-0.5` to `-0.7`):** test whether
  more pressure produces healer-targeting behavior, or just amplifies
  the time-pressure effect (likely the latter — penalties usually don't
  redirect action selection past a certain point).
- **v32.2 — add `w_healer_kill` positive bonus:** e.g. `+10` per
  Yt-HurKot kill on top of `w_npc_kill = 3.5`. Direct gradient toward
  healer attacks. May need to keep `shape_jad_heal_penalty` to maintain
  the time-pressure effect if it's confirmed valuable.
- **v32.3 — combo (penalty + healer-kill bonus):** belt and suspenders.

### Artifacts

- W&B run: `jbailey8531-oakton-college/fight caves rl/runs/k9gsilze`
- Active config at run time: `runescape-rl/config/fight_caves.ini` (v32.0)
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/k9gsilze/`
  (peak at `0000001982496256.bin`, ~1.98B steps)
- Compared against: obs.1 (`6g2i2gmy`, OBS Sweep run 1) and
  obs.1 reproduction (`5qfm1zbj`, byte-identical confirmation run)

---

## v32.2 (2026-04-27, planned — v32.1 baseline + longer training, not yet run)

Single-variable change from v32.1: `total_timesteps` raised from
`3_000_000_000` → `5_000_000_000` (+66%). Tests whether v32.1's
weak overall jad_kill (peak 0.483 vs v32.0's 0.810) was caused by
undertraining — the new obs distribution converges slower under
the same `ent_coef = 0.01` (entropy stuck at 3.22 final vs v32.0's
1.16), so the policy may simply need more wall-clock to commit on
a working strategy.

Hypothesis: v32.1's stuck entropy at 3B suggests the policy was
mid-convergence when training stopped. Conditional Jad-kill rate
was 31.5% (vs v32.0's 12.7%, +2.5×), so the obs are informative —
but reach-Jad rate dropped 87.9% → 23.2% because wave-clearing
hadn't consolidated. Extending to 5B gives the policy ~1.6× more
steps to converge while keeping the obs and exploration pressure
unchanged.

### Config diff

**vs v32.1** (immediate baseline):

| key | v32.1 | **v32.2** |
|---|---|---|
| `total_timesteps` | 3_000_000_000 | **5_000_000_000** |

All other env/reward/policy/vec/train/obs keys byte-identical to
v32.1. No backend changes required.

### Rationale

v32.1 (`qct9d64k`) data points at the 3B cutoff:
- `loss/entropy`: 3.22 final (v32.0: 1.16 final at same step count)
- `env/jad_kill_rate`: 0.483 peak @ 2.38B, 0.198 final
- `env/reached_wave_63`: 0.232 final (v32.0: 0.879)
- `env/rwd_invalid_action_fires`: 4,136 (v32.0: 1,679 — 2.5× more
  probing, signal of unconverged policy)

The "convergence rate is slower" hypothesis (this run, v32.2) and
the "obs are too narrow" hypothesis (v32.3) and the "exploration
pressure too high" hypothesis (v32.4) are three different reads on
the same v32.1 regression. v32.2 is the cheapest to test — same
obs, same hyperparams, just more wall-clock — so it goes first.

### Expected outcomes

- If v32.2 entropy drops to ~1.5 by 4B and peak ≥ v32.0 (0.810):
  v32.1's regression was undertraining, fixed by more steps. v32.2
  promotes to SOTA on the v32.1 obs distribution.
- If v32.2 ≈ v32.1 peak (~0.5): more training did not help; the
  obs distribution requires either narrower exploration (v32.4)
  or fewer obs (v32.3) to converge cleanly.
- If v32.2 entropy still stuck at 3+ at 5B: the new obs are
  creating optimization-landscape problems (gradient noise, signal
  sparsity). Would suggest v32.3 (split-obs) is the right path.

### Risk

- Wall-clock cost: +66% training time. If v32.2 fails, that time is
  partially recoverable as a longer-soak datapoint (no other v32.x
  has been trained beyond 3B).
- Late-collapse risk: v32.0 collapsed at 2.75B → 3.0B (0.489 → 0.275
  best-window). If v32.2 follows that pattern but on the v32.1 obs,
  we may see regression rather than convergence in the 4–5B window.

### What to monitor

- `loss/entropy` trajectory at 3.5B / 4B / 4.5B — does the slope
  resume the v32.0-shape decay, or stay flat at 3+?
- `env/jad_kill_rate` 250M-window mean past 3B — does it climb
  toward v32.0's peak (0.810) or plateau near v32.1's peak (0.483)?
- `env/reached_wave_63` past 3B — does the wave-clearing path
  recover toward v32.0's 0.879 final or stay stuck near 0.232?

### Artifacts (when run)

- W&B tag: `v32.2`
- Active config: `runescape-rl/config/fight_caves.ini` (write at run time)
- Run command:
  `cd ... && WANDB_TAG=v32.2 bash train.sh`

---

## v32.3 (2026-04-27, planned — split-obs ablation, not yet run)

Single-variable change from v32.1: ablate the 6 Jad-phase-specific
obs fields back to 0 at runtime, keep the 3 general-purpose ones live.
Tests whether v32.1's regression came from the Jad-specific obs
over-specializing the policy on the end-game at the expense of
wave-clearing skill.

Hypothesis (from v32.1 analysis): the 6 Jad-phase obs
(`LOCK_NOW_{MEL,RNG,MAG}`, `HEALERS_ACTIVE`, `JAD_HP`, `IS_JAD_WAVE`)
are mostly only meaningful at wave 63 (Jad fight), so the policy spent
training "learning when these signals matter" instead of consolidating
the longer journey. Conditional Jad-kill rate in v32.1 was 31.5% (vs
v32.0's 12.7%) but reach-Jad rate dropped from 87.9% → 23.2%. v32.3
isolates the 3 general-purpose obs (`FOOD_TIMER`, `POT_TIMER`,
`TICKS_IN_WAVE`) — these apply across all 63 waves, so they shouldn't
trigger the same Jad-specialization pathology.

### Config diff

**vs v32.1** (immediate baseline):

| key | v32.1 | **v32.3** |
|---|---|---|
| `obs_ablate_jad_phase` | (does not exist) | **1** |

All other env/reward/policy/vec/train keys byte-identical to v32.1.
Heal penalty (`-0.3`) carried over.

### Backend change required (NOT yet wired)

This run cannot start until a new ablation flag is added:

- **`obs_ablate_jad_phase`** — zeroes the 6 Jad-specific obs slots
  in-place after `fc_write_obs`:
  - `out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_LOCK_NOW_MEL] = 0`
  - `out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_LOCK_NOW_RNG] = 0`
  - `out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_LOCK_NOW_MAG] = 0`
  - `out[FC_OBS_META_START + FC_OBS_META_HEALERS_ACTIVE] = 0`
  - `out[FC_OBS_META_START + FC_OBS_META_JAD_HP] = 0`
  - `out[FC_OBS_META_START + FC_OBS_META_IS_JAD_WAVE] = 0`

Wiring touches the same surface the previous ablation flags went
through: `fc_api.h` (signature), `fc_state.c` (impl branch),
`fight_caves.h` (FightCaves struct field + call site), `binding.c`
(kwarg), `fight_caves.c` (default 0), `viewer.c` (parallel
ViewerState + parser).

`FC_POLICY_OBS_SIZE` stays 122 — encoder shape + RNG flow unchanged
from v32.1, so this is a deterministic comparison run.

### Rationale

The 3 general-purpose obs:
- `FOOD_TIMER` — agent currently can't eat-then-eat-again until cooldown clears; explicit timer enables 2-shark planning
- `POT_TIMER` — same shape for prayer-pot drink delay
- `TICKS_IN_WAVE` — anticipates `shape_wave_stall_*` penalty (kicks in at 1400 ticks)

These apply to every wave, including the 60-wave journey to Jad. The
6 Jad-phase obs are essentially zero through waves 1-62, then become
informative only at 63 — the policy gets a sudden injection of
new signal exactly at the hardest point. v32.1 showed the policy
struggled to integrate that timing.

### Expected outcomes

- If v32.3 ≥ v32.0 peak (0.810): the 3 general-purpose obs are net
  helpful, no Jad-specialization side effect. Promotes to v32.x line.
- If v32.3 ≈ v32.0 (within 5%): the 3 general-purpose obs are
  neutral-to-mild win. Marginal — would not promote.
- If v32.3 < v32.0: even the general-purpose obs cause regression.
  Suggests new obs add complexity the policy can't process at
  3B/ent_coef=0.01. Would point toward v32.4 (lower ent_coef) or
  v32.2 (longer training) as the right next step.

### What to monitor

- `loss/entropy` trajectory — does it drop below 2.0 by 1.5B steps?
  v32.0 hit 1.5 by 1.75B; v32.1 stuck at 3.6+. v32.3 should land
  somewhere in between.
- `env/reached_wave_63` — does the wave-clearing path recover to
  v32.0 levels (0.879 final / 1.0 peak)?
- `env/rwd_jad_heal_fires` — should rise back toward v32.0's 108
  (since `HEALERS_ACTIVE` is ablated, no signal to kill healers
  early). If it stays low like v32.1's 21, the heal penalty alone
  is sufficient incentive.

### Artifacts (when run)

- W&B tag: `v32.3`
- Active config: `runescape-rl/config/fight_caves.ini` (write at run time)
- Run command (after backend wiring lands):
  `cd ... && WANDB_TAG=v32.3 bash train.sh`

---

## v32.4 (2026-04-27, planned — lower ent_coef on v32.1 obs, not yet run)

Single-variable change from v32.1: `ent_coef` lowered from `0.01`
→ `0.005`. Tests whether v32.1's stuck-high entropy
(final 3.22 vs v32.0's 1.16) was caused by insufficient pressure
to commit, vs. genuinely needing the new obs distribution to be
explored more broadly.

Hypothesis: the new obs added 9 informative-but-narrow signals
(LOCK_NOW_* fires only on lock ticks, HEALERS_ACTIVE only nonzero
at wave 63, etc.). With the standard `ent_coef = 0.01`, the policy
keeps too much exploration over a richer signal space and never
collapses on a strategy. Halving `ent_coef` should let entropy
decay normally and the policy commit by ~2B steps as in v32.0.

### Config diff

**vs v32.1** (immediate baseline):

| key | v32.1 | **v32.4** |
|---|---|---|
| `ent_coef` | 0.01 | **0.005** |

All other env/reward/policy/vec/train/obs keys byte-identical to
v32.1. No backend changes required.

### Rationale

`ent_coef` is the entropy bonus weight in the PPO objective. Higher
values → more exploration / slower commitment; lower values →
faster commitment / risk of premature local-optimum lock-in.

v32.0 used `ent_coef = 0.01` and converged cleanly (entropy
1.16 final). v32.1 used the same `ent_coef = 0.01` but the new obs
distribution kept entropy at 3.22 — same exploration pressure,
richer signal space, slower convergence.

`0.005` is half-strength. Per the v29.2 sweep result (which used
the same `0.005` against the v28.x baseline), this typically
produces faster early commitment with limited downside on final
performance.

### Expected outcomes

- If v32.4 entropy drops to ~1.5 by 2B and peak ≥ v32.0 (0.810):
  v32.1's regression was a convergence-rate issue, fixed by tighter
  exploration. v32.4 promotes to SOTA.
- If v32.4 commits but to a worse strategy (peak < v32.0):
  the new obs may genuinely require either richer exploration
  (back to 0.01) or a smaller obs change (split, like v32.3).
- If v32.4 entropy still stuck at 3+: not an exploration issue —
  the new obs are creating optimization-landscape problems
  (gradient noise, signal sparsity). Would suggest v32.3 is the
  right path forward.

### Risk

Under-exploration risk: at `0.005`, if the policy commits to a
bad strategy in the first 0.5B steps before discovering Jad,
recovery is harder. Past data point: v29.2 (`ent_coef=0.005`)
landed peak jad 0.61 vs v28.8's 0.59 — within noise, no
catastrophic exploration failure observed at this level.

### What to monitor

- `loss/entropy` trajectory — should decay to ~2.0 by 1.5B,
  ~1.0 by 2.5B if the hypothesis holds.
- `env/jad_kill_rate` peak step — v32.1 peaked at 2.38B; v32.4
  should peak earlier (~1.5–2.0B) if commitment is the lever.
- `env/rwd_invalid_action_fires` — v32.1's 4,136 (vs v32.0's
  1,679) was a key signal of unconverged probing. Should drop
  toward v32.0 levels.

### Artifacts (when run)

- W&B tag: `v32.4`
- Run command:
  `cd ... && WANDB_TAG=v32.4 bash train.sh`
  (after writing `ent_coef = 0.005` into `fight_caves.ini`)

---

## v28.10 (2026-04-23, completed — `r4olxuy4` / peach-morning-327)

Single-variable change from v28.9: `total_timesteps` raised from
**2.5B → 2.75B** (+10%). Staged as an interpolation probe between
v28.8 (3.0B, deployment SOTA) and v28.9 (2.5B, peak SOTA) to find
the peak-vs-stability sweet spot in the β-compression curve.

**Bottom line — unexpected regression, tier D.** Peak
`jad_kill_rate = 0.1708` @ 2.15B (56% lower than v28.4, 71% lower
than v28.8, 73% lower than v28.9). Final `jad_kill_rate = 0.030`
(vs v28.8's 0.223, v28.9's 0.180) and stability = 0.156 (worst of
the v28 series). The training trajectory **never converged on Jad
play at all** — entropy stayed at 3.8+ through step 1.75B (vs
v28.9's 1.94 at the same step) and only dropped to ~3.0 by 2.15B
where the weak peak finally emerged. This is **not** a noisy
interpolation of v28.8/v28.9 — it's a completely different training
trajectory, closer to v28.7's 7.5B collapse than to any of its
neighbors on the budget axis. Combined with v28.7's prior collapse,
we now have two data points showing that the budget knob does
**not** produce a smooth landscape at seed=42 — the successful
runs (2.5B, 3.0B, 3.5B) sit on a narrow ridge that's surrounded by
failure modes on both sides.

Landed in tier **D — regression** from the staged outcome tiers.

### Config diff

**vs v28.9** (immediate baseline):

| key | v28.9 | v28.10 |
|---|---|---|
| `total_timesteps` | 2_500_000_000 | **2_750_000_000** |

All env/reward/policy/vec keys unchanged. The 2.75B bump is a
pure training-budget probe sharing the v28.4/v28.8/v28.9 reward
config. Verified via saved run configs — only `total_timesteps`
differs from v28.9 across the entire run config.

### Results — full metric sweep

**Core performance (the regression map):**

| metric | v28.4 (3.5B) | v28.8 (3.0B) | v28.9 (2.5B) | **v28.10 (2.75B)** | Δ(10−9) | Δ(10−8) |
|---|---|---|---|---|---|---|
| peak `jad_kill_rate` | 0.386 | 0.591 | 0.635 | **0.171** | **−0.464** | **−0.420** |
| peak step | 3.26B | 1.80B | 1.63B | **2.15B** | +0.52B | +0.35B |
| best 250M window | 0.240 | 0.316 | 0.296 | **0.097** | **−0.199** | **−0.219** |
| best-250M step | 3.27B | 2.42B | 1.76B | 2.31B | — | — |
| final `jad_kill_rate` | 0.214 | 0.223 | 0.180 | **0.030** | **−0.150** | **−0.193** |
| last-10% mean jad | 0.178 | 0.174 | 0.121 | **0.027** | −0.094 | −0.147 |
| stability (L10/peak) | 0.460 | 0.295 | 0.190 | **0.156** | −0.034 | −0.139 |
| final `reached_wave_63` | 0.466 | 0.973 | 0.707 | **0.462** | **−0.245** | **−0.511** |
| final `wave_reached` | 59.3 | 62.9 | 61.8 | 60.3 | −1.5 | −2.6 |
| final conditional kill | 45.9% | 22.9% | 25.5% | **6.6%** | −18.9pp | −16.3pp |
| peak conditional kill | 43.4% | 44.9% | 55.9% | **15.8%** | −40.1pp | −29.1pp |

Every headline metric is worse than v28.8 and v28.9. Peak jad is
~4× lower, best-250M is ~3× lower, final jad is ~6× lower. The
agent didn't "regress in the post-peak tail" — it never got to
a peak in the first place.

**Behavior metrics:**

| metric | v28.4 | v28.8 | v28.9 | **v28.10** | Δ(10−9) |
|---|---|---|---|---|---|
| `episode_length` | 7,987 | 9,039 | 8,268 | **8,113** | −155 |
| `max_wave_ticks` (cleared) | 369 | 314 | 287 | **280** | −7 |
| `prayer_switches` | 437 | 564 | 311 | **406** | +95 |
| `correct_prayer` | 1,301 | 1,544 | 1,442 | **1,223** | −219 |
| `wrong_prayer_hits` | 294 | 280 | 318 | 265 | −53 |
| `no_prayer_hits` | 79 | 67 | 86 | 82 | −4 |
| `prayer_up_range` | 0.313 | 0.311 | 0.286 | 0.277 | −0.009 |
| `prayer_up_magic` | 0.397 | 0.384 | 0.439 | 0.389 | −0.050 |
| `damage_blocked` | 146,083 | 184,802 | 164,853 | **134,106** | −30,747 |
| `dmg_taken_avg` | 7,125 | 8,183 | 8,270 | **5,690** | −2,580 |
| `attack_when_ready_rate` | 0.902 | 0.895 | 0.933 | **0.866** | −0.067 |
| `food_eaten` | 19.0 | 19.1 | 19.7 | 19.8 | +0.1 |
| `food_wasted` | 10.24 | 4.40 | 6.59 | **14.60** | **+8.01 (+122%)** |
| `pots_used` | 28.8 | 31.7 | 31.5 | 28.8 | −2.7 |
| `avg_hp_on_food` | 0.758 | 0.640 | 0.672 | **0.862** | +0.190 |
| `avg_prayer_on_pot` | 0.416 | 0.573 | 0.602 | 0.661 | +0.059 |
| `tokxil_melee_ticks` | 7.73 | 2.43 | 4.66 | **2.51** | −2.15 |
| `ketzek_melee_ticks` | 8.11 | 2.88 | 11.59 | **2.83** | **−8.76** |
| `most_npcs_slayed` | 277 | 278 | 277 | 278 | +1 |
| correct-per-switch | 2.98 | 2.74 | 4.64 | 3.01 | −1.63 |
| prayer accuracy | 81.6% | 84.4% | 81.2% | 82.2% | +1.0pp |

The mid-cave metrics are fine — `tokxil_melee=2.51` and
`ketzek_melee=2.83` are both at-or-better than v28.8 (2.43 / 2.88),
indicating positioning is learned. `prayer_accuracy = 82.2%` and
`correct_prayer = 1,223` show danger-wave prayer skill is mostly
intact. **The failure is Jad-specific** — the agent reaches Jad
only 46% of the time (matching v28.4's 47%) but converts only 6.6%
of those reaches into kills (vs v28.4's 46%).

Three behavior signals align with "never committed":

- **`food_wasted = 14.60`** (highest of any v28 run — 2× v28.9, 3×
  v28.8). Food-management skill regressed from v28.8 levels.
- **`avg_hp_on_food = 0.862`** (highest — agent eats at near-full
  HP more often, i.e. over-eats defensively). Consistent with
  high-entropy random play rather than converged risk evaluation.
- **`loss/entropy = 2.65`** (vs v28.8 1.63 / v28.9 1.78). Final-
  epoch entropy is still where v28.8/v28.9 were in their early
  training. The policy never committed.

**wave_stall activity — never triggered:**

| metric | v28.4 | v28.8 | v28.9 | **v28.10** |
|---|---|---|---|---|
| `rwd_wave_stall_fires` | 39 | 0 | 0 | **0** |
| `rwd_wave_stall_total` | −55 | 0 | 0 | **0** |

v28.10 is the third consecutive run (after v28.8 and v28.9) where
`wave_stall` never fires in the final window. At 2.75B, the agent
clears waves too quickly (or dies too early) to hit the 1400-tick
threshold. No diagnostic value for the wave_stall lever in this run.

### Reward-channel breakdown (final state)

| channel | v28.4 | v28.8 | v28.9 | **v28.10** | Δ(10−9) |
|---|---|---|---|---|---|
| `damage_dealt` | +1,515 | +1,739 | +1,592 | +1,505 | −87 |
| `damage_taken` | −191 | −299 | −223 | **−156** | +67 |
| `npc_kill` | +890 | +949 | +938 | +896 | −42 |
| `wave_clear` | +26,299 | +29,437 | +28,553 | +26,455 | −2,098 |
| **`jad_kill`** | **+428** | **+446** | **+361** | **+61** | **−300 (−83%)** |
| `player_death` | −9 | −9 | −9 | −11 | −2 |
| `food_waste` | −7.3 | −2.7 | −4.2 | **−10.6** | **−6.4 (+152%)** |
| `pot_waste` | −0.1 | −0.1 | −0.3 | −0.2 | +0.1 |
| `correct_jad_prayer` | +66 | +128 | +31 | **+31** | 0 |
| `correct_danger_prayer` | +306 | +355 | +348 | +291 | −57 |
| `wrong_danger_prayer` | −104 | −95 | −114 | −94 | +19 |
| `unnecessary_prayer` | −72 | −34 | −55 | −54 | +1 |
| `melee_pressure` | −295 | −384 | −203 | −363 | −160 |
| `wasted_attack` | −171 | −194 | −175 | −176 | −1 |
| `kiting` | +1,923 | +2,167 | +2,098 | +1,861 | −237 |
| `safespot_attack` | +2,275 | +2,551 | +2,417 | +2,249 | −168 |
| `wave_stall` | −55 | 0 | 0 | 0 | 0 |
| `invalid_action` | −322 | −242 | −204 | **−440** | **−236 (+116%)** |
| `tick_penalty` | −40 | −45 | −41 | −40 | +1 |
| **total positive** | +33,701 | +37,773 | +36,339 | +33,348 | −2,991 |
| **total negative** | −1,266 | −1,305 | −1,029 | −1,344 | −315 |
| **net** | +32,435 | +36,467 | +35,311 | +32,003 | −3,308 |

The reward landscape shifted in two diagnostic directions:

- `jad_kill` dropped **83%** (361 → 61). Every other positive
  channel dropped only 5–10%. The regression is Jad-specific, not
  a whole-policy collapse.
- `invalid_action` cost **doubled** (−204 → −440). Agent is still
  probing the action space late in training — consistent with
  unconverged policy.
- `food_waste` cost grew 152% (−4.2 → −10.6). Food-management
  gradient wasn't learned.
- Net reward (+32,003) is close to v28.4's (+32,435) — the agent
  is roughly as effective at non-Jad rewards as the weakest
  successful run in the series. It just can't kill Jad.

### Trajectory — divergence at ~1.25B, never recovers

| bucket | v28.4 jad / ent | v28.8 jad / ent | v28.9 jad / ent | **v28.10 jad / ent** |
|---|---|---|---|---|
| [0.00, 0.25B) | 0.000 / 5.06 | 0.000 / 4.93 | 0.000 / 4.93 | 0.000 / 5.01 |
| [0.25, 0.50B) | 0.000 / 3.72 | 0.000 / 3.35 | 0.000 / 3.05 | 0.000 / 3.79 |
| [0.50, 0.75B) | 0.000 / 3.72 | 0.000 / 3.40 | 0.000 / 2.58 | 0.000 / 3.89 |
| [0.75, 1.00B) | 0.000 / 3.62 | 0.000 / 3.57 | 0.001 / **2.24** | 0.000 / 3.81 |
| [1.00, 1.25B) | 0.001 / 3.55 | 0.000 / 3.30 | 0.010 / **2.15** | 0.001 / 3.77 |
| [1.25, 1.50B) | **0.142** / 3.27 | 0.026 / **2.56** | 0.013 / 2.22 | 0.003 / **3.92** |
| [1.50, 1.75B) | 0.118 / 2.91 | **0.142** / **2.20** | **0.295** / **1.94** | 0.004 / **3.81** |
| [1.75, 2.00B) | 0.113 / 2.74 | **0.259** / 2.14 | 0.144 / 1.89 | 0.018 / 3.31 |
| [2.00, 2.25B) | 0.135 / 2.14 | 0.157 / 2.17 | 0.090 / 1.86 | **0.094** / 3.00 |
| [2.25, 2.50B) | 0.221 / 2.23 | **0.279** / 2.11 | 0.121 / 1.74 | 0.060 / 3.06 |
| [2.50, 2.75B) | 0.087 / 2.32 | 0.169 / 1.98 | — | 0.025 / 2.92 |
| [2.75, 3.00B) | 0.171 / 2.43 | 0.158 / 1.76 | — | — |
| [3.00, 3.25B) | **0.234** / 2.54 | — | — | — |
| [3.25, 3.50B) | 0.173 / 2.55 | — | — | — |

**The entropy column tells the whole story.** By step 1.5B:

- v28.9 had committed (entropy 1.94).
- v28.8 had committed (entropy 2.20).
- v28.4 was committing (entropy 2.91, still declining).
- **v28.10 was still exploring at entropy 3.81** — above its own
  step-0.25B entropy of 3.79. The policy drifted sideways for the
  first 1.75B before any meaningful commitment.

Policy commitment in v28.10 finally starts between 1.75B → 2.15B
(entropy 3.31 → 3.00), and that's also exactly where the weak
0.17 peak emerges. With only 600M steps of actual "converged"
training left, and entropy still above 3.0 at the peak, there was
no runway for the Jad-specific skill to develop.

### Conditional kill rate per bucket

| bucket | v28.4 | v28.8 | v28.9 | **v28.10** |
|---|---|---|---|---|
| [1.00, 1.25B) | 3.9% | 0.0% | 7.4% | 1.6% |
| [1.25, 1.50B) | **43.4%** | 17.0% | 7.4% | 2.5% |
| [1.50, 1.75B) | 20.5% | 24.1% | **55.9%** | 1.6% |
| [1.75, 2.00B) | 15.6% | **44.9%** | 31.9% | 6.0% |
| [2.00, 2.25B) | 14.3% | 22.9% | 15.6% | **15.8%** |
| [2.25, 2.50B) | 30.2% | 34.7% | 23.8% | 12.6% |
| [2.50, 2.75B) | 15.7% | 20.1% | — | 6.5% |

v28.10's conditional kill rate **never exceeds 16%**, and that
only happens briefly in the [2.00, 2.25B) bucket that overlaps
the weak peak. Every comparable bucket of v28.4 / v28.8 / v28.9
sits at 20–56%. v28.10 is not a weaker-version-of-the-others —
it's playing a qualitatively different, pre-convergent policy
through the entire run.

### Diagnosis — exploration-trapped trajectory, not budget interpolation

The staged hypothesis was a smooth β-compression landscape:
lower `total_timesteps` → faster β ramp → earlier peak →
(prediction) mid-curve 2.75B lands between v28.8 and v28.9.

That hypothesis predicted **peak ≈ 0.60–0.64**. Actual peak =
**0.171**. The prediction failed by 0.43. This magnitude of
error means the interpolation model is wrong about what the
budget knob controls, at least at seed=42.

Three facts narrow the diagnosis:

1. **Entropy diverges from step ~0.75B onward.** v28.9 (2.5B)
   and v28.10 (2.75B) share the same reward config, same seed,
   and near-identical β at matched absolute steps (within 0.05
   of each other through step 1.5B). Yet v28.9 dropped entropy to
   2.24 by [0.75, 1.0B), while v28.10 stayed at 3.81. This is
   **not explained by β** — the β values are too close.
2. **v28.10 and v28.7 share a failure signature.** v28.7 at 7.5B
   also collapsed (peak 0.193, final 0.000, similar early high-
   entropy trajectory). We now have two points where the budget
   knob produced a divergent trajectory instead of a continuous
   variation of the successful runs.
3. **Every other v28 lever is held fixed.** No env, reward, obs,
   policy, or loss-function change between v28.9 and v28.10.
   `total_timesteps` is the only variable — and it controls the
   β schedule, the LR schedule (gated but still computed), **and**
   the epoch count feeding every epoch-indexed side-effect in
   `pufferl.py` / `torch_pufferl.py`.

Most-likely cause: **CUDA-nondeterminism-induced trajectory
divergence, amplified by epoch-count-dependent training side
effects.** Same-seed runs on CUDA are not deterministic across
different `total_timesteps` — batched kernels can reorder, and
any downstream calculation that reads the non-deterministic state
(replay buffer indexing, prio_probs sampling, initialization
ordering if any of those are budget-dependent) gets a different
realization. With tight budgets (2.5–3.5B) the random walk
happens to have landed in a favorable basin three times in a row.
At 2.75B and at 7.5B, it didn't.

Alternate hypothesis worth keeping alive: **2.75B intersects a
specific β/LR/replay-ratio interaction** that produces a
pathologically flat gradient in the 0.75B–1.5B window. We can't
distinguish this from nondeterminism without a repeat run —
seeded retry of v28.10 with a different CUDA seed would pin it
down.

### What worked

**1. Positioning skill transfers despite the collapse.**
`tokxil_melee = 2.51` and `ketzek_melee = 2.83` are at v28.8
quality, showing the mid-cave movement policy doesn't depend on
Jad-skill convergence.

**2. Prayer-switching skill is retained.** `correct_prayer = 1,223`
and `prayer_accuracy = 82.2%` are within a few percent of v28.4.
Danger-wave prayer is learned early and survives the Jad collapse.

**3. Run completed cleanly.** No CUDA OOM, no NaN, no crash —
2.75B steps ran in 23.1 min at ~1.93M SPS, matching prior v28
runs on the same hardware.

### What didn't work

**1. Peak jad dropped 73%** (0.635 → 0.171). Worst peak of any
non-v28.5 run in the v28 series.

**2. Entropy never collapsed into the converged regime.** Final
entropy 2.65 is above v28.8's final 1.63 by ~1.0 nats — the
policy is still effectively exploring at the end of training.

**3. Interpolation hypothesis falsified.** 2.75B does not sit
smoothly between v28.8 and v28.9 at seed=42. Budget knob is
non-monotonic at this granularity.

**4. `correct_jad_prayer_fires = 20.7`** — tied with v28.9 for
lowest, but v28.9 had a real peak of 0.635 while v28.10 only
has 0.171. Firing Jad prayer infrequently doesn't preclude
success; not firing it **and** not having a converged policy does.

**5. `invalid_action` cost doubled (−204 → −440).** Agent is
still probing bad actions late in training.

**6. Best deployable checkpoint is mediocre.** The 2.31B best-250M
window averages jad = 0.097 — far below v28.8's deployment-SOTA
0.316 and v28.4's 0.240.

### Expected-outcome audit

| prediction | target | actual | status |
|---|---|---|---|
| peak jad ≥ 0.60 (tier S/A) | ≥ 0.60 | 0.171 | ❌ |
| peak jad ≥ 0.55 (tier B) | ≥ 0.55 | 0.171 | ❌ |
| peak jad ≥ 0.55 (tier C floor) | ≥ 0.55 | 0.171 | ❌ |
| peak jad 0.55–0.65 (tier C band) | 0.55–0.65 | 0.171 | ❌ |
| stability ≥ 0.30 (tier S/B) | ≥ 0.30 | 0.156 | ❌ |
| stability ≥ 0.15 (tier A/C floor) | ≥ 0.15 | 0.156 | ✅ (just barely) |
| best 250M ≥ 0.31 | ≥ 0.31 | 0.097 | ❌ |
| `ketzek_melee` ≤ 5.0 late | ≤ 5.0 | 2.83 | ✅ |
| peak step 1.65–1.80B | 1.65–1.80B | 2.15B | ❌ (late by 350M) |

2 of 9 predictions hit (both stability-floor and positioning),
and both hit for the wrong reason — the run was stable-near-zero,
not stable-near-peak. Lands cleanly in tier **D — regression**.

### Recommendations for next run

Given the v28.7 + v28.10 collapse pattern, the budget sweep is
exhausted. Options for v28.11:

**Option A (RECOMMENDED — repeat v28.9 with different seed):**
Keep `total_timesteps = 2_500_000_000`, change `seed = 42 → 7`
(or any other value). Directly tests whether v28.9's 0.635 peak
is reproducible or a same-seed lucky-trajectory artifact. If the
repeat lands near 0.60 → v28.9 is robust. If it collapses like
v28.10 → same-seed success is noise and we need seed averaging
before trusting any budget conclusion. Cost: one 2.5B run (~25
min). Highest information per compute.

**Option B (robustness sweep — 3 seeds at v28.9 config):**
Same config as v28.9, seeds {7, 13, 42}. Three runs, characterizes
seed variance. Total cost ~75 min. Only worth it if Option A
shows disagreement with v28.9.

**Option C (pin the trajectory variable — log β/LR per step):**
Before any more budget runs, instrument `torch_pufferl.py` to log
β and any other epoch-indexed state at every sample. Then do a
short diagnostic run to confirm whether v28.10's training path
differs from v28.9's in a way that's explained by the schedule
or not. If not explained → nondeterminism. If explained → we've
found a real control knob.

**Option D (pivot — disable prio-beta annealing entirely):**
Set `prio_beta0 = 0.6` (fixed β) and remove the schedule. Removes
one source of budget-dependent behavior. Compare peak and
stability to v28.9. Higher upfront cost (code change + retrain)
but gives us a budget-invariant baseline.

**Option E (ship v28.8, move on):** Accept that v28.8's
0.591 peak / 0.316 best-250M / 0.29 stability is the deployment
artifact. v28.8 is beating every subsequent attempt. Pivot
compute to SPS improvements / refactor / observability work.

**Strongest recommendation: Option A (seed retry).** The
nondeterminism hypothesis is the simplest explanation for v28.10
vs v28.9, and a single retry disambiguates it cheaply. Spending
more compute on budget exploration before resolving the seed-
variance question is wasted search.

### Updated understanding — budget is not a smooth knob

Previous picture (v28.4 → v28.8 → v28.9): monotonic peak-vs-
stability tradeoff, compression lifts peak. After v28.10:

| budget | run | peak jad | best-250M | peak step | stability | trajectory |
|---|---|---|---|---|---|---|
| 7.5B | v28.7 | 0.193 | 0.116 | 1.88B | 0.00 | collapse |
| 3.5B | v28.4 | 0.386 | 0.240 | 3.26B | 0.46 | late-peak, stable |
| 3.0B | v28.8 | 0.591 | 0.316 | 1.80B | 0.29 | mid-peak, stable tail |
| **2.75B** | **v28.10** | **0.171** | **0.097** | **2.15B** | **0.16** | **collapse** |
| 2.5B | v28.9 | 0.635 | 0.296 | 1.63B | 0.19 | early-peak, tail drift |

Three-out-of-five of the budget points we've sampled land in the
"collapse" regime. The "monotonic peak lift" we thought we saw
between 3.5B → 3.0B → 2.5B was a three-point success run inside
a noisy landscape. Without seed-variance data, we can't tell if
the successes or the collapses are the "true" behavior.

### Artifacts

- Wandb run: `r4olxuy4`, display name `peach-morning-327`, state
  `finished`. Needs tag `v28.10` added.
- Local log: `pufferlib_4/logs/fight_caves/r4olxuy4.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/r4olxuy4/*.bin`
- Deployable artifact: **none from this run.** v28.8's 2.42B
  checkpoint (best-250M = 0.316) remains the deployment SOTA.
  v28.9's 1.63B checkpoint (peak = 0.635) remains the peak SOTA.

---

## v28.6 (2026-04-22, completed — `teg8ugmx`)

Single-variable change from v28.4: `shape_wave_stall_start` raised
from 1400 → **1600**. Equivalently: v28.5 with `shape_npc_melee_penalty`
reverted to v28.4's −0.8. This was the **isolation test** to determine
which of v28.5's two changes caused its catastrophic collapse.

**Bottom line — attribution answered cleanly.** v28.6 recovered from
v28.5's collapse (peak 0.072 → **0.296**, final jad 0.000 → **0.148**),
confirming the **melee_penalty=−1.0 bump was v28.5's primary
failure cause**, not the wave_stall softening. But v28.6 is also
**below v28.4's performance** (peak 0.296 vs 0.386, best-250M 0.184
vs 0.240), so wave_stall=1600 on its own is still a net regression
from v28.4's sweet-spot at 1400. The v28.6 → v28.5 delta attributes
−0.22 peak jad_kill and the full Jad-skill collapse to the
melee_penalty lever alone; the v28.4 → v28.6 delta attributes a
smaller −0.09 peak regression to wave_stall=1600 alone. Melee is
the nuclear lever; wave_stall=1600 is just "too soft".

Landed in tier **C — partial recovery** from the staged outcome tiers.

### Config diff

**vs v28.4** (baseline we're testing against):

| key | v28.4 | v28.6 |
|---|---|---|
| `shape_wave_stall_start` | 1400 | **1600** |
| `shape_npc_melee_penalty` | −0.8 | −0.8 (unchanged) |

**vs v28.5** (what we're isolating):

| key | v28.5 | v28.6 |
|---|---|---|
| `shape_npc_melee_penalty` | −1.0 | **−0.8** (reverted) |
| `shape_wave_stall_start` | 1600 | 1600 (kept) |

Verified via saved run configs — only these two fields differ between
v28.4, v28.5, and v28.6 across the entire env config.

### Results — full metric sweep

**Core performance (the attribution map):**

| metric | v28.4 (baseline) | v28.5 (both changes) | **v28.6 (wave_stall only)** | Δ(6−4) | Δ(6−5) | attribution |
|---|---|---|---|---|---|---|
| peak `jad_kill_rate` | 0.386 | 0.072 | **0.296** | −0.090 | +0.224 | melee bump caused 0.22 loss; wave_stall caused 0.09 loss |
| peak step | 3.26B | 2.97B | 1.52B | — | — | wave_stall=1600 shifted peak earlier |
| best 250M window | 0.240 | 0.028 | 0.184 | −0.056 | +0.156 | melee bump caused 0.16 loss; wave_stall 0.06 |
| best-250M step | 3.02B | 2.79B | 1.35B | — | — | — |
| final `jad_kill_rate` | 0.214 | 0.000 | 0.148 | −0.066 | +0.148 | melee bump caused 100% loss; wave_stall caused 31% |
| stability (final/peak) | 0.55 | 0.00 | 0.50 | −0.05 | +0.50 | wave_stall=1600 still fairly stable |
| final `reached_wave_63` | 0.466 | 0.000 | **0.626** | **+0.160** | +0.626 | wave_stall=1600 helps r63! |
| final `wave_reached` | 59.3 | 49.8 | **61.4** | +2.1 | +11.6 | wave_stall=1600 helps |
| last-10% conditional kill | 32.1% | 2.0% | 12.3% | −19.8pp | +10.3pp | — |
| peak conditional kill | 45.9% | 12.3% | 30.7% | −15pp | +18pp | — |

The attribution is clear: melee_penalty=−1.0 cost ~58% of v28.4's
peak; wave_stall=1600 on top cost ~23% of v28.4's peak. Additive
loss in v28.5 was ~81%. They weren't interacting badly — they were
each individually bad (one catastrophically, one mildly).

**Behavior metrics:**

| metric | v28.4 | v28.5 | **v28.6** | Δ(6−4) |
|---|---|---|---|---|
| `episode_length` | 7,987 | 6,072 | **9,184** | +1,197 (+15%) |
| `max_wave_ticks` (cleared) | 369 | 259 | **588** | +219 (+59%) |
| `stuck_wave_num` | 50.87 | 34.75 | 54.24 | +3.4 |
| `prayer_switches` | 437 | 105 | **864** | +426 (+98%) |
| `correct_prayer` | 1,301 | 936 | 1,409 | +108 |
| `wrong_prayer_hits` | 294 | 280 | 320 | +26 |
| `no_prayer_hits` | 79 | 150 | 78 | −1 |
| `prayer_up_range` | 0.313 | 0.345 | 0.266 | −0.05 |
| `prayer_up_magic` | 0.397 | 0.340 | 0.383 | −0.01 |
| `damage_blocked` | 146,083 | 97,626 | **192,170** | **+46k (+32%)** |
| `dmg_taken_avg` | 7,125 | 7,482 | 7,505 | +380 |
| `attack_when_ready_rate` | 0.902 | 0.847 | 0.858 | −0.04 |
| `food_eaten` | 19.0 | 17.8 | 19.8 | +0.8 |
| `food_wasted` | 10.24 | 4.73 | 9.86 | −0.4 |
| `pots_used` | 28.8 | 20.5 | 29.9 | +1.1 |
| `avg_hp_on_food` | 0.758 | 0.576 | 0.768 | +0.01 |
| `avg_prayer_on_pot` | 0.416 | 0.231 | 0.432 | +0.02 |
| `tokxil_melee_ticks` | 7.73 | 13.44 | 6.49 | **−1.24 (−16%)** |
| `ketzek_melee_ticks` | 8.12 | 15.54 | 9.19 | +1.07 |
| `most_npcs_slayed` | 277 | 274 | 280 | +3 |
| correct-per-switch | 2.97 | 8.93 | 1.63 | −1.34 |
| prayer accuracy | 81.6% | 77.0% | 81.5% | −0.1pp |

Notable patterns:
- **`prayer_switches` almost doubled** (437 → 864) — agent is
  switching prayers MUCH more than v28.4. Combined with lower
  correct-per-switch (2.97 → 1.63), this is a reverting-to-churning
  pattern, not v26-style tactical.
- **`damage_blocked` +32%** — more successful prayer blocks (1,409
  correct vs 1,301), more total prayer events.
- **`tokxil_melee` down 16%** — positioning on Tok-Xil waves is
  better than v28.4. Ket-Zek slightly worse.
- **`max_wave_ticks` +59%** (369 → 588) — longest cleared-wave
  durations jumped. Agent is spending more time on individual waves
  it eventually clears.

**wave_stall activity (big surprise — FIRES MORE, not less):**

| metric | v28.1 | v28.2 | v28.4 | v28.5 | **v28.6** |
|---|---|---|---|---|---|
| start threshold | 1000 | 1200 | 1400 | 1600 | **1600** |
| `rwd_wave_stall_fires` | 2.2 | 253 | 39 | 0 | **522** |
| `rwd_wave_stall_total` | −1.6 | −435 | −55 | 0 | **−943** |

**v28.6 fires wave_stall MORE than any prior run despite having the
highest threshold.** This is the opposite of what I predicted. The
mechanism: with +200 more grace (1600 vs 1400) AND melee penalty at
its gentler −0.8 value, the agent is confident about extending Jad
fights into the grace zone. Per-Jad-death tick calculation:

- v28.4: 39 fires / 0.252 dies-on-Jad rate = 155 avg penalty-ticks
  past 1400 → **Jad death at ~1555 ticks**
- v28.6: 522 fires / 0.478 dies-on-Jad = **1,092 avg penalty-ticks
  past 1600 → Jad death at ~2,692 ticks**

v28.6's agent spends ~73% longer per failing Jad fight than v28.4
(2,692 vs 1,555 ticks). The extra grace (and +50% longer fights) did
NOT produce proportionally more kills — the agent is just "failing
longer". Classic "grace-expands-into-waste" behavior.

### Reward-channel breakdown (final state)

| channel | v28.4 | v28.5 | **v28.6** | Δ(6−4) |
|---|---|---|---|---|
| `damage_dealt` | +1,515 | +1,087 | +1,732 | +217 (+14%) |
| `damage_taken` | −191 | −149 | −202 | −11 |
| `npc_kill` | +890 | +693 | +924 | +34 |
| `wave_clear` | +26,299 | +18,324 | +27,877 | +1,578 |
| **`jad_kill`** | **+428** | 0 | +296 | **−132 (−31%)** |
| `player_death` | −9 | −11 | −9 | 0 |
| `food_waste` | −7.3 | −3.3 | −6.7 | +0.6 |
| `pot_waste` | −0.1 | 0 | −0.4 | −0.3 |
| `correct_jad_prayer` | +66 | 0 | **+188** | **+122 (+184%)** |
| `correct_danger_prayer` | +306 | +225 | +308 | +2 |
| `wrong_danger_prayer` | −104 | −121 | −110 | −6 |
| `unnecessary_prayer` | −72 | −76 | −66 | +6 |
| `melee_pressure` | −295 | −543 | −367 | −72 |
| `wasted_attack` | −171 | −129 | −209 | −38 |
| `kiting` | +1,923 | +1,336 | +2,106 | +183 |
| `safespot_attack` | +2,275 | +1,588 | +2,572 | +297 |
| **`wave_stall`** | **−55** | 0 | **−943** | **−888** |
| `invalid_action` | −322 | −177 | −401 | −79 |
| `tick_penalty` | −40 | −29 | −46 | −6 |
| **total positive** | +33,701 | +23,253 | **+36,002** | +2,301 |
| **total negative** | −1,266 | −1,239 | −2,360 | −1,094 |
| **net** | +32,435 | +22,014 | **+33,642** | +1,207 (+4%) |

The reward landscape shifted meaningfully:
- `wave_stall` cost jumped from −55 (v28.4) to **−943 (v28.6)**, a
  17× increase. The agent is paying heavily for extended Jad fights.
- `correct_jad_prayer` almost TRIPLED vs v28.4 (+66 → +188 total,
  44 → 125 fires). Agent is praying more during Jad — but converting
  less into kills.
- `jad_kill` dropped 31% (+428 → +296). More Jad attempts, fewer
  successes.
- Other channels roughly scale with the longer episodes (wave_clear
  +6%, kiting +10%, safespot +13%) — proportional, not shifted.
- Net reward slightly higher (+33,642 vs +32,435) but from wave_clear
  not Jad — not a quality-adjusted improvement.

### Trajectory — same seed matches through 1.5B, then diverges

| step | v28.4 jad | **v28.6 jad** | v28.4 r63 | v28.6 r63 | v28.4 ep_len | v28.6 ep_len | v28.4 ws | v28.6 ws |
|---|---|---|---|---|---|---|---|---|
| [1.0, 1.5B) | 0.071 | 0.071 | 0.178 | 0.178 | 7,268 | 7,268 | 0 | 0 |
| [1.5, 2.0B) | 0.116 | 0.074 | 0.650 | 0.625 | 8,275 | 8,272 | 13 | 8 |
| [2.0, 2.5B) | **0.178** | 0.054 | 0.838 | 0.751 | 8,884 | 9,005 | 61 | **143** |
| [2.5, 2.75B) | 0.087 | 0.107 | 0.558 | 0.741 | 8,301 | 8,741 | 10 | 70 |
| [2.75, 3.0B) | 0.171 | 0.102 | 0.684 | 0.713 | 8,861 | 8,883 | 137 | 97 |
| [3.0, 3.25B) | **0.233** | 0.048 | 0.594 | 0.710 | 8,612 | 9,035 | 199 | 165 |
| [3.25, 3.5B) | 0.173 | 0.098 | 0.523 | 0.690 | 8,365 | 8,972 | 146 | 135 |

Identical through [1.0, 1.5B). Divergence patterns:
- **[1.5, 2.0B):** v28.6 slightly worse on jad (0.074 vs 0.116)
  but same r63. Wave_stall minimal.
- **[2.0, 2.5B):** v28.4's best-era starts; v28.6 stays flat. Wave_stall
  jumps in v28.6 (143 vs 61).
- **[2.5, 2.75B):** v28.6 briefly matches/exceeds v28.4 (0.107 vs 0.087).
- **[3.0, 3.25B):** v28.4's peak era (0.233); v28.6 drops to 0.048.
  Wave_stall still firing heavily in both.
- **[3.25, 3.5B):** v28.6 recovers slightly but both declining.

v28.4 showed a late-training peak-rising pattern; v28.6 shows an
early-plateau pattern with modest decline. The wave_stall=1600
softening either (a) gave the agent too much room to waste on
long-failing Jad fights, or (b) removed the gradient pressure that
was driving v28.4's late-training skill improvement. Both are
plausible from the data.

### Conditional kill rate per bucket

| step | v28.4 | **v28.6** |
|---|---|---|
| [1.0, 1.5B) | 40.1% | 40.1% (identical) |
| [1.5, 2.0B) | 17.8% | 11.8% |
| [2.0, 2.5B) | **21.3%** | 7.2% |
| [2.5, 2.75B) | 15.7% | 14.4% |
| [2.75, 3.0B) | **25.0%** | 14.3% |
| [3.0, 3.25B) | **39.3%** | 6.8% |
| [3.25, 3.5B) | **33.0%** | 14.2% |

v28.4 consistently beats v28.6 on conditional kill rate from 1.5B
onward. The deeper Jad-skill v28.4 developed at 2.75B+ never emerges
in v28.6.

### Diagnosis — the v28.5 attribution, resolved

**v28.5 failure attribution (confirmed):**

| variable | effect in v28.5 | effect isolated (v28.6 vs v28.4) |
|---|---|---|
| `melee_penalty` −0.8 → −1.0 | catastrophic: peak to 0.072 | [not directly tested] — but v28.6 recovery confirms this is the problem |
| `shape_wave_stall_start` 1400 → 1600 | [not directly tested] | mild: peak 0.386 → 0.296 |

Applying additive/subtractive logic to v28.5:
- v28.5 peak = 0.072. v28.4 peak = 0.386. Total loss = 0.314.
- v28.6 peak = 0.296. So wave_stall=1600 alone cost 0.386 − 0.296 = 0.090.
- If effects were additive: melee_penalty=−1.0 alone should cost ≈ 0.314 − 0.090 = 0.224.
- Predicted v28.5-equivalent-with-melee-only = 0.386 − 0.224 = 0.162.
- That is **far better than v28.5's 0.072**, which means the effects
  are **superadditive** — the two changes interacted badly.

Interpretation: wave_stall=1600 removes some training signal (longer
Jad fights without extra penalty → less pressure on efficient Jad kills).
melee_penalty=−1.0 breaks the mid-cave positioning policy. In
combination, the agent LOSES mid-cave efficiency AND can't compensate
with late-cave Jad kills — hitting both ends of the cave simultaneously.

### What worked

**1. Attribution achieved.** We now know melee_penalty is the
unstable lever; don't touch it in this reward framework.

**2. v28.6 is the best reach63 run with non-trivial Jad kills.**
Final r63 = 0.626 (highest since v28.2) with final jad_kill = 0.148
(similar to v28). Better balance on those two metrics than v28.4,
but not better jad_kill overall.

**3. prayer_switches doubled** — agent is more active with prayer
than v28.4. `correct_jad_prayer` fires tripled (44 → 125). The agent
is experimenting with Jad prayer more.

**4. `stuck_wave_num` recovered** (50.87 → 54.24). Agent dies at
later waves on average than v28.4.

### What didn't work

**1. Peak jad dropped 23%** (0.386 → 0.296). wave_stall=1600 is
worse than 1400.

**2. Peak shifted early** (3.26B → 1.52B). The late-training
skill-deepening that v28.4 showed is gone.

**3. `wave_stall_total` cost exploded to −943** — 17× v28.4's cost.
Longer failing Jad fights.

**4. Conditional kill rate at all mid-late training buckets is
worse than v28.4.** v28.4 [3.0, 3.25B) hit 39.3%; v28.6 hit 6.8%.

**5. `correct_jad_prayer_fires` tripled (44 → 125)** but `jad_kill`
rate dropped — agent is firing Jad prayer more but less effectively.

### Expected-outcome audit

| prediction | target | actual | status |
|---|---|---|---|
| `rwd_wave_stall_fires` | < 20/ep | **522/ep** | ❌ 26× target |
| peak jad ≥ 0.35 (tier A) | ≥ 0.35 | 0.296 | ❌ tier C |
| peak jad ≥ 0.42 (tier B) | ≥ 0.42 | 0.296 | ❌ |
| best-250M ≥ 0.20 | ≥ 0.20 | 0.184 | ❌ close |
| final r63 ≥ 0.40 | ≥ 0.40 | 0.626 | ✅ |
| conditional kill ≥ 15% mid-training | ≥ 15% | ~14% | ❌ close |
| `correct_jad_prayer_fires` > 20 | > 20 | 125 | ✅ |

3 of 7 hit, landed in tier **C (partial recovery, 0.15-0.30 peak)**.
Per the staged v28.7 branch: "revert to 1400, different lever".

### Recommendations for v28.7

Based on everything learned so far:

**Option A (RECOMMENDED — revert wave_stall, extend budget):**
`start = 1400` (v28.4 value), total_timesteps = 5B. Directly
tests whether v28.4's late-training peak-rising pattern continues
with more budget. v28.4's best-250M was at 3.02B and still rising —
~60% probability of breaking 0.45 peak with 5B budget.

**Option B (try a new lever — train budget with anneal_lr):**
`start = 1400`, `anneal_lr = 1`, total_timesteps = 3.5B. Forces
policy to settle rather than explore in the last third. Would
plausibly produce a "v28.4 but more stable" run without extra
compute.

**Option C (test if 1500 is a better threshold):**
`start = 1500` (halfway between v28.4's 1400 and v28.6's 1600).
Fine-tuning the threshold. Probably diminishing returns given v28.6
showed 1600 is already worse; 1500 might be marginal improvement
over 1400 but unlikely to unlock much.

**Option D (try different reward structure entirely):**
Reduce `w_correct_jad_prayer` from 1.5 → 1.0 or `shape_safespot_attack_reward`
from 1.5 → 1.0. These aren't driving improvement at current values;
lowering might refocus gradient on actual Jad kills.

**My strongest recommendation: Option A (revert to v28.4 + 5B budget).**
We've exhaustively tested softening wave_stall — 1000, 1200, 1400,
1600 all mapped. 1400 is the local optimum. More softening doesn't
help. The main untapped lever is training budget, and v28.4's trajectory
strongly suggests it has room to grow.

### Updated understanding of the wave_stall lever

| `start` value | run | peak jad | best-250M | peak step | interpretation |
|---|---|---|---|---|---|
| removed | v28 | 0.597 | 0.346 | 2.79B | exploit-driven |
| 1000 | v28.1 | 0.350 | 0.150 | 1.43B | over-corrective |
| 1200 | v28.2 | 0.277 | 0.165 | 2.75B | moderate |
| **1400** | **v28.4** | **0.386** | **0.240** | **3.26B** | **sweet spot** |
| 1600 | v28.6 | 0.296 | 0.184 | 1.52B | too soft |

The relationship is non-monotonic with a clear peak at 1400.
Further softening doesn't just have diminishing returns — it hurts.

### Artifacts

- Wandb run: `teg8ugmx`, tag `v28.6`, state `finished`
- Local log: `pufferlib_4/logs/fight_caves/teg8ugmx.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/teg8ugmx/*.bin`
- Deployable artifact: the 1.52B checkpoint range (peak era).
  But **v28.4's 3.26B checkpoint is still the best deployable agent**
  across the whole v28 series — v28.6 doesn't beat it.

---

## v28.5 (2026-04-22, completed — `ocd1nl6p`)

Two-variable change from v28.4:

1. `shape_wave_stall_start` **1400 → 1600** (+200 more grace)
2. `shape_npc_melee_penalty` **−0.8 → −1.0** (25% stronger)

**Bottom line — catastrophic regression.** Per the attribution matrix
this landed in the "peak ↓ AND reach63 ↓ → both changes hurt" cell.
Peak `jad_kill_rate = 0.072` (vs v28.4's 0.386, an 81% drop) and final
`jad_kill_rate = 0.000` — the agent ended training unable to kill Jad
at all. Final `reached_wave_63 = 0.000` and final `wave_reached = 49.8`
(down from v28.4's 59.3): the agent now dies ~10 waves before Jad on
average. The collapse is worse than v28.1's at equivalent points and
approaches v27.2-like territory. **Crucially, we cannot attribute the
failure to either variable alone** — both changed simultaneously and
ripped the policy out of the v28.4 operating point without replacement.
v28.6 should be a single-variable isolation run.

Paradox of the run: v28.5 developed **the most tactical prayer policy
of the entire v28 series** — 16.3 correct-per-switch at peak era,
77–80% accuracy, comparable to v26's 12.7 (the project's gold-standard
prayer behavior). But the policy failed at Jad specifically —
`correct_jad_prayer_fires = 0` at end of training, meaning the agent
reached Jad and **never prayed correctly against him**.

### Config diff vs v28.4

| key | v28.4 | v28.5 |
|---|---|---|
| `shape_wave_stall_start` | 1400 | **1600** |
| `shape_npc_melee_penalty` | −0.8 | **−1.0** |
| `shape_wave_stall_ramp_interval` | 150 | 150 (unchanged) |
| `shape_wave_stall_base_penalty` | −0.5 | −0.5 (unchanged) |
| `shape_wave_stall_cap` | −2.0 | −2.0 (unchanged) |

All other env/train/policy/vec/seed values identical. 3.5B steps.

### Results — full metric sweep

**Core performance (catastrophic regression):**

| metric | v28 | v28.1 | v28.2 | v28.4 | **v28.5** | Δ(5−4) |
|---|---|---|---|---|---|---|
| peak `jad_kill_rate` | 0.597 @ 2.79B | 0.350 @ 1.43B | 0.277 @ 2.75B | 0.386 @ 3.26B | **0.072 @ 2.97B** | **−0.314** |
| best 250M window | 0.346 | 0.150 | 0.165 | 0.240 | **0.028** | **−0.212** |
| final `jad_kill_rate` | 0.148 | 0.127 | 0.103 | 0.214 | **0.000** | **−0.214** |
| final `reached_wave_63` | 0.370 | 0.762 | 0.849 | 0.466 | **0.000** | **−0.466** |
| final `wave_reached` | 59.5 | 61.4 | 62.2 | 59.3 | **49.8** | **−9.5** |
| stability (final/peak) | 0.25 | 0.36 | 0.37 | 0.55 | **0.00** | **−0.55** |
| last-10% avg jad | 0.215 | 0.096 | 0.111 | 0.177 | **0.002** | −0.175 |
| last-10% conditional kill | 57.3% | 11.5% | 14.2% | 32.1% | **2.0%** | −30.1pp |
| peak conditional kill | 81.6% | 64.1% | 33.7% | 45.9% | **12.3%** | −33.6pp |

v28.5's peak row (step 2.97B): jad=0.072, r63=0.586, ep_len=8,090,
wave_reached=61.89 — the agent briefly reached Jad in most episodes
but never converted. The conditional kill rate NEVER exceeded ~6%
in any training bucket.

**Behavior metrics (the regression is across the board):**

| metric | v28.4 | **v28.5** | Δ | interpretation |
|---|---|---|---|---|
| `episode_length` | 7,987 | 6,072 | −1,915 (−24%) | dies faster |
| `wave_reached` | 59.3 | 49.8 | −9.5 | dies 10 waves earlier |
| `stuck_wave_num` | 50.87 | **34.75** | **−16** | stuck wave moved to wave ~35 |
| `max_wave_ticks` (cleared) | 369 | 259 | −110 | non-Jad waves cleared faster but less often |
| `most_npcs_slayed` | 277 | 274 | −3 | fewer total NPCs per episode |
| `pots_used` | 28.8 | **20.5** | **−8.3 (−29%)** | running out of prayer uses |
| `food_eaten` | 19.0 | 17.8 | −1.3 | similar eating |
| `food_wasted` | 10.2 | 4.7 | −5.5 | less panic-eating (but shorter episodes) |
| `attack_when_ready_rate` | 0.902 | 0.847 | −0.05 | less attacking |
| `damage_blocked` | 146,083 | **97,626** | **−33%** | much less damage blocked by prayer |

**Positioning (melee bump did NOT reduce melee exposure — it increased it):**

| metric | v28.4 | **v28.5** | Δ |
|---|---|---|---|
| `tokxil_melee_ticks` | 7.73 | **13.44** | **+74%** |
| `ketzek_melee_ticks` | 8.12 | **15.54** | **+91%** |
| `melee_pressure_fires` | 305 | **452** | **+48%** |
| `melee_pressure_total` | −295 | **−543** | −248 |
| `dmg_taken_avg` | 7,125 | 7,482 | +5% |

**This is the most striking paradox:** bumping `shape_npc_melee_penalty`
from −0.8 → −1.0 was supposed to discourage melee-range positioning.
Instead, the agent spent **48% more ticks in melee pressure states**,
74–91% more in Tok-Xil/Ket-Zek melee range specifically. The stronger
penalty didn't change WHERE the agent went — it changed the policy
gradient landscape early enough to push training into a different
local optimum where the agent is less cautious about NPC melee contact
but more cautious about prayer usage (see below).

**Prayer behavior — best-tactical-ever, but Jad prayer broken:**

| metric | v28 | v28.1 | v28.2 | v28.4 | **v28.5** |
|---|---|---|---|---|---|
| `prayer_switches` | 4,813 | 289 | 576 | 437 | **105** |
| `correct_prayer` | 2,674 | 1,403 | 1,426 | 1,301 | 936 |
| `wrong_prayer_hits` | 293 | 293 | 310 | 294 | 280 |
| `no_prayer_hits` | 76 | 58 | 80 | 79 | **150** |
| correct-per-switch | 0.56 | 4.86 | 2.48 | 2.97 | **8.93** |
| prayer accuracy (cor/cor+wrong) | 90.1% | 82.7% | 82.2% | 81.6% | **77.0%** |
| `prayer_up_range` | 0.253 | 0.306 | 0.284 | 0.313 | **0.345** |
| `prayer_up_magic` | 0.338 | 0.441 | 0.410 | 0.397 | 0.340 |
| `prayer_up_melee` | 0.006 | 0.001 | 0.000 | 0.000 | 0.000 |

v28.5's `correct-per-switch = 8.93` is the best of the v28 series and
second-best ever (v26 was 12.7). The agent developed **efficient
tactical prayer**: switch only when needed, usually correctly. But
the `no_prayer_hits = 150` (+91% vs v28.4) shows the agent is getting
styled-hit 150 times per episode with NO prayer active — the tactical
policy misses too many events.

### Reward-channel breakdown (final state)

| channel | v28.4 | **v28.5** | Δ | v28.4 fires | v28.5 fires |
|---|---|---|---|---|---|
| `damage_dealt` | +1,515 | +1,087 | −428 | 1,551 | 1,121 |
| `damage_taken` | −191 | −149 | +42 | 72 | 83 |
| `npc_kill` | +890 | +693 | −197 | 254 | 198 |
| `wave_clear` | +26,299 | **+18,324** | **−7,975** | 58 | **47** |
| **`jad_kill`** | +428 | **0** | **−428** | 0.2 | **0** |
| `player_death` | −9 | −11 | −2 | 0.8 | 1.0 |
| `food_waste` | −7 | −3 | +4 | 11 | 5 |
| `pot_waste` | −0.1 | 0.0 | +0.1 | 0.2 | 0.1 |
| **`correct_jad_prayer`** | +66 | **0** | **−66** | 44 | **0** |
| `correct_danger_prayer` | +306 | +225 | −81 | 1,223 | 900 |
| `wrong_danger_prayer` | −104 | −121 | −17 | 346 | 403 |
| `unnecessary_prayer` | −72 | −76 | −4 | 362 | 380 |
| **`melee_pressure`** | −295 | **−543** | **−248** | 305 | **452** |
| `wasted_attack` | −171 | −129 | +42 | 1,713 | 1,290 |
| `kiting` | +1,923 | +1,336 | −587 | 874 | 607 |
| `safespot_attack` | +2,275 | +1,588 | −687 | 1,517 | 1,059 |
| `wave_stall` | −55 | **0** | +55 | 39 | **0** |
| `invalid_action` | −322 | −177 | +145 | 3,220 | 1,774 |
| `tick_penalty` | −40 | −29 | +11 | 7,923 | 5,789 |
| **total positive** | +33,701 | **+23,253** | −10,448 | | |
| **total negative** | −1,266 | −1,239 | +27 | | |
| **net** | +32,435 | **+22,014** | **−10,421 (−32%)** | | |

Three key observations:

1. **`jad_kill = 0` and `correct_jad_prayer = 0`** (both total AND
   fires). The agent had ZERO Jad-specific events at end of training.
   This is a complete Jad-policy collapse — the agent might reach
   wave 63 occasionally but dies without praying or landing hits.

2. **`wave_stall = 0` fires entire run**. Even with start=1600, the
   agent never spent 1600+ ticks on a wave. This means the wave_stall
   softening had **NO direct behavioral effect** this run — the agent
   dies too fast to ever trigger it. Variable 1 was inert; Variable 2
   (melee penalty) is the likely culprit.

3. **`melee_pressure` 48% more fires** with 25% higher per-fire
   penalty. Stronger penalty didn't deter the behavior — it just
   charged more for it. The agent pays −543 (vs v28.4's −295) for
   melee-range events. This is strong evidence the melee bump
   CAUSED the behavior shift (not the expected behavior).

### Trajectory — v28.5 never stabilized

| step | jad | r63 | wave | ep_len | ws_fires | ketzek_mel | no_pry_hits |
|---|---|---|---|---|---|---|---|
| [0.25, 0.5B) | 0.000 | 0.000 | 46.8 | 5,313 | 0 | 3.3 | 94 |
| [0.5, 1.0B) | 0.000 | 0.000 | 52.1 | 6,209 | 0 | 5.1 | 62 |
| [1.0, 1.5B) | 0.010 | 0.185 | 58.4 | 7,446 | 0 | **14.9** | 57 |
| [1.5, 2.0B) | 0.016 | 0.399 | 61.4 | 8,002 | 0 | 12.5 | 37 |
| [2.0, 2.5B) | 0.010 | 0.269 | 61.0 | 7,971 | 0 | 12.2 | 43 |
| [2.5, 2.75B) | 0.017 | 0.453 | 61.3 | 8,025 | 0 | 11.7 | 48 |
| **[2.75, 3.0B)** | **0.025** | **0.620** | **62.1** | **8,147** | 0 | 12.2 | 48 |
| [3.0, 3.25B) | 0.008 | 0.126 | 59.0 | 7,548 | 0 | 10.2 | 65 |
| [3.25, 3.5B) | 0.003 | 0.133 | 54.7 | 6,868 | 2.2 | 11.8 | **91** |

v28.5's peak era was [2.75, 3.0B) — same region where v28 had its
exploit-driven 0.308 peak and v28.4 had 0.171. v28.5 reached 0.025
here (best conditional kill ~4%). Then:

**[3.0, 3.25B) is the collapse point:** r63 drops from 0.620 → 0.126
(−80%), wave_reached drops from 62.1 → 59.0. `no_prayer_hits` climbs
from 48 → 65 → 91 in the last 500M. The policy forgot whatever
progress it made and started dying to styled attacks without prayer.

**Ketzek_melee exposure was ELEVATED from [1.0B) onward**. The agent
rushed into Ket-Zek melee range as early as 1B steps and never
corrected — despite paying increased penalty. This is strong evidence
the melee_penalty bump pushed training into a different policy
regime where melee-range kiting was the baseline behavior.

### Conditional kill rate per bucket (the clearest signal of failure)

| step | v28 | v28.1 | v28.2 | v28.4 | **v28.5** |
|---|---|---|---|---|---|
| [1.0, 1.5B) | 40.1% | 37.0% | 35.7% | 40.1% | **5.4%** |
| [1.5, 2.0B) | 16.5% | 13.0% | 15.7% | 17.8% | **3.9%** |
| [2.0, 2.5B) | 14.0% | 5.3% | 8.9% | 21.3% | **3.6%** |
| [2.5, 2.75B) | 43.2% | 4.3% | 11.7% | 15.7% | **3.8%** |
| [2.75, 3.0B) | 74.6% | 6.4% | 17.4% | 25.0% | **4.1%** |
| [3.0, 3.25B) | 53.9% | 10.5% | 11.6% | 39.3% | **6.3%** |
| [3.25, 3.5B) | 60.7% | 12.1% | 16.1% | 33.0% | **2.0%** |

v28.5's conditional kill rate is **3–6% across all of training** —
the agent NEVER became competent at converting Jad reaches. This is
a pure Jad-skill failure, orthogonal to the positional issues.

### Diagnosis — why did v28.5 collapse?

Competing hypotheses, ranked by evidence:

**Hypothesis A (most likely): the melee_penalty bump created
unintended gradient pressure that broke Jad-skill learning.**

The mechanism: bumping penalty from −0.8 → −1.0 is a 25% amplification
of an already-firing signal (242 fires in v28.4). This doesn't change
the behavioral target (avoid melee range) but it makes EARLY training
harsher. When the cold-start policy is exploring, stronger penalties
on melee-range states make those gradients dominate. The policy
"locks in" a melee-avoidant-at-all-costs stance, but in doing so it
fails to learn the Jad-combat pattern (which requires close-ish
positioning during Jad's magic phases). The resulting Jad fights
(when the agent reaches Jad) are poorly-executed — agent tries to
avoid Jad melee range but doesn't know how to hit Jad properly
either.

Supporting evidence:
- ketzek_melee INCREASED despite stronger penalty (policy didn't
  converge on melee-avoidance; it found different bad local optimum)
- correct_jad_prayer = 0 (policy never learned Jad-specific prayer)
- prayer_switches collapsed to 105 (same precision as v26 but less
  total prayer activity, suggesting undertrained)

**Hypothesis B: the wave_stall=1600 softening made Jad attempts
essentially "free" in policy terms, undermining the pressure that
drove v28.4's Jad skill.**

The mechanism: in v28.4, threshold=1400 meant Jad fights had a
~100-tick "cost zone" before dying (avg Jad death at ~1523 ticks).
This provided gradient pressure to kill Jad faster. With start=1600,
Jad fights are essentially penalty-free for all realistic durations
— the agent has less reason to efficiently kill Jad.

Supporting evidence:
- wave_stall fires = 0 for the entire run (until last 250M)
- no reward-gradient pressure on Jad fight duration

**Hypothesis C: combined interaction pushed the policy into an
especially bad local optimum.**

The mechanism: both changes individually might be tolerable, but
together they create a reward landscape where the cheapest path
(locally) is "die quickly to avoid all penalties". Shorter episodes
(6,072 vs v28.4's 7,987) and dying at wave 49 vs 59 support this
— the agent minimizes cumulative penalty by ending episodes early.

**Most likely: hypothesis A is the dominant cause, with B and C as
contributing factors.** Melee penalty bumps are notoriously unstable
across reward-shaping literature because they don't bound the agent's
behavior (just make certain states costly).

### Attribution challenge — we need a follow-up

Two variables changed simultaneously, both went in the same
(catastrophic) direction, so we can't isolate. From the v28.5 staged
attribution matrix:

> `peak ↓, reach63 ↓ → both changes hurt — v28.6 revert wave_stall to
> 1400, re-test melee alone`

This prescription is sound. v28.6 should be **v28.4 + melee_penalty=
−1.0 only** — a single-variable isolation run. That would tell us
cleanly whether the melee bump alone causes the failure.

### What worked (the only silver lining)

**1. Tactical prayer policy emerged.** v28.5 achieved the best
correct-per-switch of the v28 series (8.93) and highest prayer
accuracy on cold-start non-Jad waves. This is a regime close to
v26's tactical prayer behavior. If we can preserve this prayer
policy while restoring Jad-skill, that would be very valuable.

**2. Early-wave efficiency held briefly.** At [1.5, 2.0B) the agent
cleared waves efficiently: ep_len 8,002, wave_reached 61.4, similar
to v28.4 at the same step. The collapse is a late-training
phenomenon.

### What didn't work (everything else)

**1. Peak jad_kill_rate −81%** (0.386 → 0.072). Biggest absolute
regression in the v28 series.

**2. Final reach63 → 0.000.** Agent unable to reach Jad at end of
training. Dies at wave 49-54 on average.

**3. Jad-specific skills zeroed out.** correct_jad_prayer = 0,
jad_kill = 0. The agent simply does not fight Jad.

**4. Melee exposure INCREASED despite stronger penalty.** The exact
opposite of the intended effect — ketzek_melee +91%.

**5. wave_clear dropped 30%** (+28,800 → +18,300). 12 fewer waves
cleared per episode (60 → 47). Agent dying earlier means less
wave-clearing reward, less training signal.

**6. Policy collapse at [3.0B+)**. No stability in last 500M of
training. Final metrics are all worse than mid-training.

### Expected-outcome audit (all predictions missed badly)

| prediction | target | actual | status |
|---|---|---|---|
| `rwd_wave_stall_fires` | < 25/ep | **0** | — (no firing at all) |
| `rwd_wave_stall_total` | ≈ −30 | **0** | — |
| peak jad_kill_rate | 0.35–0.45 | **0.072** | ❌ catastrophically low |
| sustained best-250M | ≥ 0.24 | **0.028** | ❌ 9× below target |
| `ketzek_melee_ticks` | ≤ 6 | **15.5** | ❌ opposite direction |
| `tokxil_melee_ticks` | ≤ 5 | **13.4** | ❌ opposite direction |
| `stuck_wave_num` | ≥ 54 | **34.75** | ❌ opposite direction |
| final reach63 | ≥ 0.70 | **0.000** | ❌ zero |
| final jad_kill_rate | ≥ 0.20 | **0.000** | ❌ zero |

**0 of 9 predictions hit.** The melee_penalty bump did the opposite
of what I predicted on every positioning metric — strong evidence it
drove the policy into a bad local optimum rather than toward the
intended target behavior.

### Recommendations for v28.6

**Primary recommendation: single-variable isolation run.**

**Option A (RECOMMENDED): v28.4 + melee_penalty=−1.0 only** (keep
wave_stall at 1400). Tests whether the melee bump alone causes the
failure observed in v28.5. Expected signal strength: if this also
collapses, we've confirmed melee_penalty=−1.0 is the problem and
v28.7 should revert melee_penalty to −0.8 AND test wave_stall=1600
alone. If v28.6 reaches v28.4-level performance, the problem was
the combination (not the individual changes).

**Option B: v28.4 + wave_stall=1600 only** (keep melee_penalty at
−0.8). Inverse isolation. Less likely to be the cause based on
analysis above, but would definitively answer the question.

**Option C: Revert fully to v28.4 and explore a different variable**
(e.g., extend training to 5B on v28.4's config). Given v28.4 was
still improving at 3.5B, the simplest path to more performance is
more compute.

**My strongest recommendation: Option A.** It's the cleanest test of
the hypothesis that melee_penalty bumps are unstable. If the
hypothesis is right, we know to avoid this lever entirely; if wrong,
we've learned something unexpected about this reward landscape. Either
way, the information value per training run is high.

**Option D (alternative): smaller melee_penalty bump** (e.g., −0.8
→ −0.9 instead of −1.0). Might be too small to matter but preserves
the intended direction. Less informative than Option A because
"collapse at −1.0 but not at −0.9" is a weak finding.

### Artifacts

- Wandb run: `ocd1nl6p`, tag `v28.5`, state `finished`
- Local log: `pufferlib_4/logs/fight_caves/ocd1nl6p.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/ocd1nl6p/*.bin`
- **Not deployable.** No checkpoint from v28.5 produces usable Jad-kill
  performance. The peak row (step 2.974B) has jad=0.072 and should
  not be used for production.
- **Use v28.4's 3.26B checkpoint as the current best deployable agent**
  until v28.6 or later produces something better.

---

## v28.4 (2026-04-22, completed — `onksd6md`)

Single-variable change from v28.2: raised `shape_wave_stall_start` from
**1200 → 1400** (+17% grace, matching the v28.1→v28.2 step size).
Everything else identical to v28.2. (v28.3 was skipped — user went
straight to v28.4.)

**Bottom line — this is a substantial win.** v28.4 is the best
post-v28 run across every meaningful quality metric except reach63.
Peak `jad_kill_rate` jumped to **0.386** (vs v28.2's 0.277, v28.1's
0.350) at step 3.26B. Best 250M rolling window **0.240** (+45% vs
v28.2's 0.165). Last-10% conditional kill rate **32.1%** (vs v28.2's
14.2%, v28.1's 11.5%) — more than double. Stability **0.55** (vs
v28.2's 0.37) — best of v28 series. Crucially, Ket-Zek melee
exposure stayed controlled at ~6 ticks throughout late training (no
regression like v28.1). The tradeoff: final `reached_wave_63` dropped
to 0.466 (vs v28.2's 0.849) — the agent now reaches Jad less often
but kills him much more efficiently when it gets there. Net Jad kills
per episode roughly doubled (0.103 → 0.214).

The v28.4 peak-250M window is **at step 3.02B, very late in training.**
The policy was still improving at the end of the 3.5B budget — this
run plausibly had more room to improve with a larger budget.

### Config diff vs v28.2

| key | v28.2 | v28.4 |
|---|---|---|
| `shape_wave_stall_start` | 1200 | **1400** |
| `shape_wave_stall_ramp_interval` | 150 | 150 (unchanged) |
| `shape_wave_stall_base_penalty` | −0.5 | −0.5 (unchanged) |
| `shape_wave_stall_cap` | −2.0 | −2.0 (unchanged) |

All other values (env, train, policy, vec, seeds) identical to v28.2.
3.5B steps, commit `38b447bd` + in-tree wave_stall re-add + Jad-runtime-
persistence fix from v28.1.

### Results — full metric sweep

**Core performance:**

| metric | v28 | v28.1 | v28.2 | **v28.4** | Δ(4−2) |
|---|---|---|---|---|---|
| peak `jad_kill_rate` | 0.597 @ 2.79B | 0.350 @ 1.43B | 0.277 @ 2.75B | **0.386 @ 3.26B** | **+0.109** |
| best 250M window | 0.346 | 0.150 | 0.165 | **0.240** | **+0.075 (+45%)** |
| final `jad_kill_rate` | 0.148 | 0.127 | 0.103 | **0.214** | **+0.111 (+107%)** |
| last-10% avg jad | 0.215 | 0.096 | 0.111 | **0.177** | +0.066 (+60%) |
| stability (final/peak) | 0.25 | 0.36 | 0.37 | **0.55** | +0.18 (+49%) |
| final `reached_wave_63` | 0.370 | 0.762 | 0.849 | 0.466 | **−0.383** |
| last-10% r63 | 0.376 | 0.830 | 0.781 | 0.552 | −0.229 |
| final `wave_reached` | 59.5 | 61.4 | 62.2 | 59.3 | −2.8 |
| final conditional kill rate | 40.0% | 16.7% | 12.1% | **45.9%** | +33.8pp |
| last-10% conditional kill | 57.3% | 11.5% | 14.2% | **32.1%** | +17.9pp |
| peak-250M step | 2.63B | 1.31B | 1.36B | **3.02B** | still rising |

The `peak-250M step = 3.02B` and `stability = 0.55` together say: **the
policy was still learning at the end of 3.5B** and didn't plateau like
the earlier v28.x runs. This is a qualitatively different trajectory
shape.

**wave_stall activity (counter-intuitive result):**

| metric | v28.1 | v28.2 | **v28.4** |
|---|---|---|---|
| `rwd_wave_stall_fires` | 2.2 | **253** | **39** |
| `rwd_wave_stall_total` | −1.6 | **−435** | **−55** |

Raising the threshold from 1200 → 1400 made wave_stall fire **6.5×
less often** (253 → 39 fires/ep) and cost **8× less** (−435 → −55).
The agent is NOT extending Jad fights further — it's doing roughly
the same length fights as v28.2 but the threshold no longer catches
most of them. Jad-death tick estimates:

- v28.2: 253 fires ÷ 0.728 dies-on-Jad rate = 347 avg penalty-ticks past 1200 → Jad death at ~tick 1547
- v28.4: 39 fires ÷ 0.316 dies-on-Jad rate = 123 avg penalty-ticks past 1400 → Jad death at ~tick 1523

Nearly identical Jad-fight durations (~1500 ticks), but the v28.4
policy pays 8× less penalty for those fights. **This is the
unexpected win** — the softer threshold made Jad attempts materially
cheaper without the agent extending fights further.

**Stall / episode structure:**

| metric | v28 | v28.1 | v28.2 | **v28.4** | Δ(4−2) |
|---|---|---|---|---|---|
| `episode_length` | 17,192 | 8,209 | 8,746 | **7,987** | −759 (−9%) |
| `max_wave_ticks` (cleared) | 7,894 | 302 | 308 | 369 | +61 (+20%) |
| `stuck_wave_num` | 49.1 | 54.9 | 55.9 | **50.9** | −5.0 |
| `prayer_switches` | 4,813 | 289 | 576 | **437** | −139 (−24%) |
| `food_wasted` | 11.65 | 2.60 | 2.19 | **10.24** | **+8.05** |
| `pots_wasted` | 7.44 | 0.86 | 1.41 | 1.70 | +0.29 |
| `avg_hp_on_food` | 0.805 | 0.572 | 0.604 | 0.758 | +0.15 |
| `attack_when_ready_rate` | 0.825 | 0.923 | 0.888 | 0.902 | +0.01 |

Episode_length dropped slightly (shorter episodes despite higher Jad
kill rate). `stuck_wave_num` moved from 56 → 51 — agent now dies at
earlier waves on average (Tok-Xil territory around wave 48-54).
**`food_wasted` jumped 5× (2.2 → 10.2)** — agent is back to
panic-eating at high HP (avg_hp_on_food 0.76 vs v28.2's 0.60). This
correlates with the reach63 regression: the agent is less cautious
mid-cave.

**Positioning (the v28.1 Achilles heel, mostly held):**

| metric | v28 | v28.1 | v28.2 | **v28.4** |
|---|---|---|---|---|
| `tokxil_melee_ticks` | 2.98 | 6.67 | 6.17 | 7.73 |
| `ketzek_melee_ticks` | 5.48 | **20.77** | 8.98 | **8.12** |
| `dmg_taken_avg` | 6,759 | 7,909 | 8,197 | **7,125** |
| `damage_blocked` | 672k | 162k | 177k | 146k |

Ket-Zek melee stayed controlled (8.12, similar to v28.2's 8.98,
massively better than v28.1's 20.77). Tok-Xil melee rose slightly
(6.17 → 7.73) — consistent with more deaths in the wave 48-54 range.
`dmg_taken_avg` dropped 13% vs v28.2 despite more Tok-Xil exposure —
the agent is better at not dying, just not as consistent at
progressing.

**Prayer:**

| metric | v28 | v28.1 | v28.2 | **v28.4** |
|---|---|---|---|---|
| `correct_prayer` | 2,674 | 1,403 | 1,426 | 1,301 |
| `wrong_prayer_hits` | 293 | 293 | 310 | 294 |
| `no_prayer_hits` | 76 | 58 | 80 | 79 |
| `prayer_up_range` | 0.253 | 0.306 | 0.284 | **0.313** |
| `prayer_up_magic` | 0.338 | 0.441 | 0.410 | 0.397 |
| correct per switch | 0.56 | 4.86 | 2.48 | **2.97** |

correct-per-switch recovered slightly (2.48 → 2.97), range prayer
uptime up (0.284 → 0.313) — agent is a bit more precise in prayer
selection than v28.2.

### Reward-channel breakdown (final state)

| channel | v28 | v28.1 | v28.2 | **v28.4** | Δ(4−2) |
|---|---|---|---|---|---|
| `damage_dealt` | +3,574 | +1,579 | +1,688 | +1,515 | −173 |
| `damage_taken` | −162 | −258 | −262 | **−191** | +71 |
| `npc_kill` | +902 | +925 | +941 | +890 | −51 |
| `wave_clear` | +26,621 | +28,124 | +28,801 | +26,299 | −2,502 |
| **`jad_kill`** | +296 | +254 | +206 | **+428** | **+221 (+107%)** |
| `player_death` | −9 | −10 | −10 | **−9** | +1 |
| `food_waste` | −8 | −1.6 | −1.2 | −7.3 | **−6.1** |
| `pot_waste` | −1.0 | −0.1 | −0.2 | −0.1 | +0.1 |
| `correct_jad_prayer` | +2,138 | +51 | +123 | +66 | −57 |
| `correct_danger_prayer` | +295 | +334 | +327 | +306 | −21 |
| `wrong_danger_prayer` | −103 | −99 | −110 | −104 | +6 |
| `unnecessary_prayer` | −63 | −60 | −85 | −72 | +12 |
| `melee_pressure` | −348 | −225 | −266 | −295 | −29 |
| `wasted_attack` | −548 | −174 | −195 | −171 | +23 |
| `kiting` | +2,034 | +2,071 | +2,064 | +1,923 | −141 |
| `safespot_attack` | +5,171 | +2,389 | +2,536 | +2,275 | −260 |
| `wave_stall` | — | −1.6 | −435 | **−55** | +380 |
| `invalid_action` | −1,384 | −95 | −194 | **−322** | **−128** |
| `tick_penalty` | −97 | −41 | −44 | −40 | +5 |
| **total positive** | +41,031 | +35,726 | +36,686 | **+33,701** | −2,985 |
| **total negative** | −2,724 | −965 | −1,601 | −1,266 | +335 |
| **net** | +38,307 | +34,761 | +35,085 | **+32,435** | −2,650 |

**Key shifts from v28.2:**
- `jad_kill` reward **doubled** (+206 → +428). Actual Jad kills per
  ep roughly doubled (0.10 → 0.21).
- `wave_stall` cost dropped 8× (−435 → −55). Softer threshold made
  Jad fights cheap.
- `wave_clear` dropped 9% (+28,801 → +26,299) — fewer waves cleared
  overall.
- `safespot_attack` dropped 10% (+2,536 → +2,275) — less
  safespotting.
- `correct_jad_prayer` fires dropped 46% (82 → 44) — agent prays
  LESS during Jad fights.
- `invalid_action` fires jumped 66% (1,937 → 3,220) — more masked-
  action picks.
- `food_waste` fires jumped 4× (2.5 → 10.6) — panic-eating returned.

The jad_kill channel went from 0.13 to 0.21 Jad kills per episode —
so in absolute terms, v28.4 converts **~62% more Jad encounters into
kills**. This is consistent with the conditional-kill-rate improvement.

### Trajectory — when behavior diverged

Same seed → byte-identical through ~1.5B. Key divergence points:

| step | v28 jad | v28.1 jad | v28.2 jad | **v28.4 jad** | v28 r63 | v28.1 r63 | v28.2 r63 | **v28.4 r63** |
|---|---|---|---|---|---|---|---|---|
| [0.5, 1.0B) | 0.000 | 0.000 | 0.000 | 0.000 | 0.001 | 0.001 | 0.001 | 0.001 |
| [1.0, 1.5B) | 0.071 | 0.066 | 0.063 | 0.071 | 0.178 | 0.177 | 0.178 | 0.178 |
| [1.5, 2.0B) | 0.110 | 0.098 | 0.131 | 0.116 | 0.663 | 0.754 | 0.831 | 0.650 |
| [2.0, 2.5B) | 0.091 | 0.047 | 0.077 | **0.178** | 0.652 | 0.887 | 0.870 | 0.838 |
| [2.5, 2.75B) | 0.236 | 0.042 | 0.096 | 0.087 | 0.545 | 0.977 | 0.816 | 0.558 |
| [2.75, 3.0B) | 0.308 | 0.062 | 0.146 | **0.171** | 0.413 | 0.970 | 0.839 | 0.684 |
| [3.0, 3.25B) | 0.194 | 0.089 | 0.099 | **0.233** | 0.361 | 0.854 | 0.850 | 0.594 |
| [3.25, 3.5B) | 0.222 | 0.098 | 0.121 | **0.173** | 0.366 | 0.815 | 0.747 | 0.523 |

v28.4 shows a distinctive trajectory: rapid mid-game improvement at
[2.0, 2.5B) (jad 0.178, 2.3× v28.2's 0.077), slight dip at [2.5, 2.75B)
(0.087 — exploration regression?), then sustained high at 0.17–0.23 from
2.75B onward. Peak cluster is in 3.0–3.3B range (top 10 rows all fall
2.31–3.27B with jad 0.313–0.386), matching the peak-250M window at 3.02B.

**The r63 story**: v28.4 follows v28.2's r63 trajectory up through
[2.0, 2.5B) (both ~0.84), then DIVERGES at [2.5, 2.75B) where v28.4
drops to 0.558 while v28.2 stays at 0.816. After that, v28.4's r63
stays in 0.5–0.7 range while v28.2 holds 0.75–0.85. The agent traded
reach63 consistency for Jad-kill efficiency around step 2.5B.

### Conditional kill rate per bucket

| step | v28 | v28.1 | v28.2 | **v28.4** |
|---|---|---|---|---|
| [1.0, 1.5B) | 40.1% | 37.0% | 35.7% | 40.1% |
| [1.5, 2.0B) | 16.5% | 13.0% | 15.7% | 17.8% |
| [2.0, 2.5B) | 14.0% | 5.3% | 8.9% | **21.3%** |
| [2.5, 2.75B) | 43.2% | 4.3% | 11.7% | 15.7% |
| [2.75, 3.0B) | **74.6%** | 6.4% | 17.4% | **25.0%** |
| [3.0, 3.25B) | 53.9% | 10.5% | 11.6% | **39.3%** |
| [3.25, 3.5B) | 60.7% | 12.1% | 16.1% | **33.0%** |

v28.4 consistently 2–4× v28.2's conditional kill rate in mid-to-late
training, and is the first non-v28-exploit run to reach **39% conditional
kill rate** at [3.0, 3.25B). This is a real Jad-skill improvement, not
a safespot-exploit like v28's 75% at [2.75, 3.0B).

### Diagnosis — why the threshold shift produced different behavior

v28.2 → v28.4 was a uniform +200 threshold shift (1200 → 1400), same
type of change as v28.1 → v28.2 (1000 → 1200). But the result was
qualitatively different:

- v28.1 → v28.2: peak DROPPED (0.350 → 0.277), wave_stall fires spiked
  (2.2 → 253)
- v28.2 → v28.4: peak ROSE (0.277 → 0.386), wave_stall fires dropped
  (253 → 39)

What's going on? Hypothesis: the 1200 threshold was **right at the
edge of Jad-fight duration** — the agent was trying to extend into
penalty territory to finish Jad. The 1400 threshold is **past the
typical Jad-death point (~1500 ticks)** — most Jad attempts finish
(win or die) before meaningful penalty accumulates. This flips the
incentive structure: instead of "pay penalty to try longer fights",
it becomes "fights are essentially free, focus on skill improvement".

The policy responded by reallocating training focus:
- Less wave-progression fidelity (reach63 down, panic-eating back)
- More Jad-kill skill (conditional kill rate up, actual kills doubled)
- Positioning stayed controlled (no Ket-Zek regression)

**This looks like a real regime shift, not a parameter-tuning nudge.**

### Peak-rising-at-end observation

v28.4's best 250M window is at step 3.02B (last third of training),
and last-10% jad_kill_rate (0.177) is close to peak-250M (0.240), with
stability ratio 0.55 (highest of any v28.x run). This contrasts with:

- v28: best-250M at 2.63B, stability 0.25 (peaked mid, collapsed after)
- v28.1: best-250M at 1.31B, stability 0.36 (peaked early, fell off)
- v28.2: best-250M at 1.36B, stability 0.37 (peaked early, fell off)
- **v28.4: best-250M at 3.02B, stability 0.55** (still improving)

This strongly suggests v28.4's policy would benefit from additional
training budget. Peak likely keeps rising with 4.5–5B steps.

### Expected-outcome audit

| prediction | target | actual | status |
|---|---|---|---|
| rwd_wave_stall_fires | 200–400/ep | **39/ep** | ❌ (way lower than predicted) |
| rwd_wave_stall_total | −400 to −700 | **−55** | ❌ (much less cost) |
| max_wave_ticks final | ~300 | 369 | ✅ close |
| peak jad_kill_rate | 0.30–0.40 | **0.386** | ✅ (upper end of range) |
| last-10% conditional kill | > 16% | **32.1%** | ✅ (double target) |
| sustained (best 250M) | ≥ 0.17 | **0.240** | ✅ (+41% over target) |
| ketzek_melee_ticks | ≤ 9.0 | **8.12** | ✅ |

5 of 7 predictions hit, 2 missed — the wave_stall activity predictions
were wrong because I expected the agent to extend Jad fights further;
instead it kept Jad duration similar and the threshold-shift made those
fights cheaper. Everything else was within or beat prediction.

### What worked

**1. Peak jad_kill_rate jumped 40%** (0.277 → 0.386). First non-exploit
run to exceed v28.1's 0.350. This is the project's best honest
Jad-kill skill demonstration.

**2. Sustained performance +45%** (0.165 → 0.240 best 250M window).
And the peak-250M is at step 3.02B — the policy was still improving.

**3. Conditional kill rate last-10% +126%** (14.2% → 32.1%). This is
a 3× improvement over v28.1 in the meaningful deployment metric
(conversion when at Jad).

**4. Stability best-ever** (0.55). The last-third of training held
most of peak performance, unlike every prior v28.x run.

**5. Ket-Zek regression stayed fixed.** 8.12 melee ticks — similar to
v28.2's 8.98 and much better than v28.1's 20.77. Mid-cave positioning
held.

**6. Actual Jad kills per episode doubled** (0.10 → 0.21). The
jad_kill reward channel doubled (+206 → +428).

### What didn't work

**1. Final reached_wave_63 dropped 45%** (0.849 → 0.466). The agent
reaches Jad far less consistently. `stuck_wave_num` shifted from 56
→ 51 — agent now dies at earlier mid-cave waves more often.

**2. Panic-eating returned.** food_wasted fires 2.5 → 10.6 (4×),
avg_hp_on_food 0.60 → 0.76 (agent eats at higher HP). Similar profile
to v28's wasteful eating, though at lower absolute levels.

**3. correct_jad_prayer activity dropped** (82 → 44 fires). The agent
is praying LESS during Jad fights despite fighting Jad more. This is
actually consistent with higher conditional kill rate (fewer prayer
events because Jad dies faster or the agent dies faster — in either
case less time to accumulate prayer events).

**4. `invalid_action` fires +66%** (1,937 → 3,220). Masked-action
rate up. Possibly noise from the new behavior regime the policy is
still converging on.

**5. Final wave_reached dropped** (62.2 → 59.3). Tied to reach63
regression — some of the lost Jad-reaches are because the agent dies
at earlier waves.

### Observations

**1. Softening past the Jad-fight natural duration unlocked a
qualitative regime change, not just incremental tuning.** The
v28.2 → v28.4 step crossed a boundary (from "threshold catches Jad
fights" to "threshold past Jad fights"). This is why the result isn't
linearly extrapolated from v28.1 → v28.2.

**2. There may be a Jad-reach vs Jad-kill tradeoff at fixed training
budget.** The policy can allocate gradient toward either skill; v28.2
was balanced toward reach, v28.4 is balanced toward kill. Given
3.5B isn't enough to fully train both on this skill hierarchy, we're
seeing the tradeoff surface.

**3. The v28 exploit peak (0.597) is looking approachable with
extended training.** v28.4's peak is 64% of v28's peak (0.386 vs
0.597) — and v28.4 is still rising at 3.5B. With a 5B budget v28.4's
peak might reach 0.45–0.50 honestly.

### Recommendations for v28.5

From the v28.4 staged go/no-go criteria:
- peak jad ≥ 0.32 → ✅ (0.386)
- conditional improvement persists → ✅ (32.1% vs v28.2's 14.2%)
- wave_stall_fires staying reasonable → ✅ (39, not excessive)

All softening-works criteria met. Natural next steps, ordered by
expected impact:

**Option A (continue softening):** `start = 1600`. Matches same step
size. If the regime shift continues, might unlock more.
**Risk:** diminishing returns likely — the regime shift at 1400 was
because threshold moved past Jad-fight duration; going to 1600 might
just be more of the same with less marginal effect.
**Probability of significant improvement: ~30%**.

**Option B (soften cap too):** `cap = −1.5` instead of −2.0. Reduces
worst-case penalty on unusual edge cases.
**Risk:** weakens the safespot-forever cap; at -1.5/tick + 0.28/tick
safespot positive = net -1.22/tick, still cappable but less dominant.
**Probability of improvement: ~25%**.

**Option C (extend training budget):** run v28.4 config for 5B steps
instead of 3.5B. v28.4 was still improving at end of training —
stability 0.55, peak-250M at 3.02B.
**Risk:** budget is 43% higher. If it plateaus at 4B we've wasted
1B steps.
**Probability of improvement: ~60%**. Best expected-value move IF
compute budget allows.

**Option D (address reach63 regression):** bump `shape_npc_melee_penalty`
from −0.8 → −1.5 to stabilize mid-cave progression. v28.1's standing
recommendation. Goal: recover the v28.2-level reach63 without
sacrificing v28.4's Jad-kill skill.
**Risk:** may nudge agent toward previous prayer-heavy patterns or
reduce Jad-kill focus.
**Probability of improvement: ~50%**.

**My recommendation: Option C (extend training budget to 5B) if
compute allows, otherwise Option D** (melee_penalty bump, single
variable). Option C directly exploits that v28.4's peak is still
rising; Option D addresses the observed side-effect (reach63 regression)
without reversing the win.

### Artifacts

- Wandb run: `onksd6md`, tag `v28.4`, state `finished`
- Local log: `pufferlib_4/logs/fight_caves/onksd6md.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/onksd6md/*.bin`
- **Deployable artifact: the 3.02B-3.26B checkpoint range** — best
  sustained performance window. The specific `0000003257395712.bin`
  (step 3.257B) has peak jad_kill_rate=0.386, r63=0.664, ep_len=8,567,
  and is representative of the peak-250M window.
- Comparison artifacts (prior deployables):
  - v28 2.73B: jad=0.519, r63=0.66 (exploit-driven)
  - v28.1 1.43B: jad=0.350, r63=0.545 (tight spike)
  - v28.2 2.75B: jad=0.277, r63=0.816 (stable plateau)
  - **v28.4 3.26B: jad=0.386, r63=0.664 (best honest agent)**

---

## v28.2 (2026-04-22, completed — `7qsyzbyo`)

Single-variable change from v28.1: raised `shape_wave_stall_start` from
**1000 → 1200** (20% more grace). Everything else identical to v28.1.

**Bottom line:** softening the threshold had the opposite effect I
predicted on peak but a real-improvement effect on sustained performance.
Peak `jad_kill_rate` dropped (0.350 → 0.277, −21%) and conditional kill
rate at peak dropped (64% → 34%). But mid-training conditional kill
rate **improved 2.7×** ([2.5, 3.0B): 5.4% → 14.6%), sustained 250M jad
window improved 10%, and the late-training Ket-Zek melee regression was
cut nearly in half (20.8 → 9.0 ticks). Net: v28.2 has a less-spiky
peak, more uniform mid-game performance, and fewer positioning regressions
than v28.1 — but doesn't close the gap to v28's exploit-driven 0.597
peak. The extra 200 ticks of grace were *used* — wave_stall fires jumped
115× (2.2 → 253 fires/ep, −1.6 → −435 reward/ep) — producing modestly
longer Jad fights that still mostly fail.

### Config diff vs v28.1

| key | v28.1 | v28.2 |
|---|---|---|
| `shape_wave_stall_start` | 1000 | **1200** |
| `shape_wave_stall_ramp_interval` | 150 | 150 (unchanged) |
| `shape_wave_stall_base_penalty` | −0.5 | −0.5 (unchanged) |
| `shape_wave_stall_cap` | −2.0 | −2.0 (unchanged) |

All other values (env, train, policy, vec) identical to v28.1. Seeds
identical (env 73, train 42), 3.5B steps, commit `38b447bd` + in-tree
v28.1 edit. Backend bit-identical to v28.1's `j5l97xrw`.

### Results — full metric sweep (final @ 3.5B)

**Core performance:**

| metric | v28 | v28.1 | **v28.2** | Δ(2-1) | % vs v28.1 |
|---|---|---|---|---|---|
| peak `jad_kill_rate` | 0.597 @ 2.79B | 0.350 @ 1.43B | **0.277 @ 2.75B** | −0.073 | **−21%** |
| final `jad_kill_rate` | 0.148 | 0.127 | **0.103** | −0.024 | −19% |
| stability (final/peak) | 0.25 | 0.36 | **0.37** | +0.01 | +3% |
| best 250M window jad | 0.346 | 0.150 | **0.165** | +0.015 | **+10%** |
| last 10% avg jad | 0.215 | 0.096 | **0.111** | +0.015 | **+16%** |
| peak `reached_wave_63` | 0.921 | 1.000 | 1.004 | +0.004 | — |
| final `reached_wave_63` | 0.370 | 0.762 | **0.849** | +0.087 | **+12%** |
| final `wave_reached` | 59.46 | 61.39 | **62.16** | +0.77 | +1.3% |
| final conditional kill rate | 40.0% | 16.7% | **12.1%** | −4.6pp | −28% |
| last 10% conditional kill | 57.3% | 11.5% | **14.2%** | +2.7pp | **+24%** |
| peak conditional kill | 81.6% | 64.1% | 33.7% | −30pp | −47% |

**wave_stall activity (the critical diagnostic channel):**

| metric | v28.1 | **v28.2** | Δ | observation |
|---|---|---|---|---|
| `rwd_wave_stall_fires` | 2.2/ep | **253/ep** | +115× | agent IS using the 1000–1200 zone |
| `rwd_wave_stall_total` | −1.6/ep | **−435/ep** | +272× | and paying heavily for going past 1200 |
| `max_wave_ticks` (cleared) | 302 | 308 | +2% | cleared-wave durations unchanged |

Critical mechanical insight: `max_wave_ticks` is the duration of the
longest *cleared* wave in an episode (updates only when `wave_clear`
fires). It stayed flat at ~308 because waves 1-62 clear in <400 ticks.
But `rwd_wave_stall_fires = 253/ep` means the agent spends ~253 ticks
per episode past the 1200-tick threshold on the Jad wave — where the
agent dies without clearing. The two metrics tell different stories:
v28.2's `max_wave_ticks = 308` says "non-Jad waves are fine"; the
`wave_stall_fires = 253` says "Jad fights are now 1450+ ticks long on
average, up from v28.1's effectively-none".

**Stall / stability metrics:**

| metric | v28 | v28.1 | **v28.2** | Δ(2-1) |
|---|---|---|---|---|
| `episode_length` | 17,192 | 8,209 | 8,746 | +537 (+6.5%) |
| `max_wave_ticks` | 7,894 | 302 | 308 | +6 (+2%) |
| `stuck_wave_num` | 49.1 | 54.9 | 55.9 | +1.0 |
| `prayer_switches` | 4,813 | 289 | **576** | +288 (**+100%**) |
| `food_wasted` | 11.65 | 2.60 | 2.19 | −0.41 (−16%) |
| `pots_wasted` | 7.44 | 0.86 | 1.41 | +0.55 (+64%) |
| `avg_hp_on_food` | 0.805 | 0.572 | 0.604 | +0.03 |
| `avg_prayer_on_pot` | 0.572 | 0.353 | 0.347 | −0.01 |
| `attack_when_ready_rate` | 0.825 | 0.923 | **0.888** | −0.03 |

**Positioning (v28.1's biggest regression — partially fixed):**

| metric | v28 | v28.1 | **v28.2** | v28.2 vs v28.1 |
|---|---|---|---|---|
| `tokxil_melee_ticks` | 2.98 | 6.67 | 6.17 | −7.5% ✓ |
| `ketzek_melee_ticks` | 5.48 | **20.77** | **8.98** | **−57%** ✓✓ |
| `dmg_taken_avg` | 6,759 | 7,909 | 8,197 | +3.6% |
| `damage_blocked` | 672k | 162k | 177k | +9% |

Ket-Zek melee exposure nearly halved vs v28.1 — the rush-to-melee
regression that showed up at [3.0, 3.5B) in v28.1 is largely gone.

**Prayer behavior:**

| metric | v28 | v28.1 | **v28.2** |
|---|---|---|---|
| `correct_prayer` | 2,674 | 1,403 | 1,426 |
| `wrong_prayer_hits` | 293 | 293 | 310 |
| `no_prayer_hits` | 76 | 58 | 80 |
| `prayer_up_range` | 0.253 | 0.306 | 0.284 |
| `prayer_up_magic` | 0.338 | 0.441 | 0.410 |
| correct per switch | 0.56 | **4.86** | 2.48 |

v28.2's correct-per-switch regressed to 2.48 (v28.1 was 4.86, v26-era
was 12.7). More switching, slightly lower precision per switch — agent
is experimenting more with prayer changes during the now-possible longer
Jad fights.

### Reward-channel breakdown (final state)

| channel | v28 total | v28.1 total | **v28.2 total** | v28.1 fires | **v28.2 fires** |
|---|---|---|---|---|---|
| `damage_dealt` | +3,574 | +1,579 | +1,688 | 1,617 | 1,724 |
| `damage_taken` | −162 | −258 | −262 | 72 | 79 |
| `npc_kill` | +902 | +925 | +941 | 264 | 269 |
| `wave_clear` | +26,621 | +28,124 | +28,801 | 61 | 61 |
| `jad_kill` | +296 | +254 | +206 | 0.13 | 0.10 |
| `player_death` | −9 | −10 | −10 | 0.9 | 0.9 |
| `food_waste` | −8 | −1.6 | −1.2 | 2.8 | 2.5 |
| `pot_waste` | −1.0 | −0.1 | −0.2 | 0.2 | 0.4 |
| `correct_jad_prayer` | +2,138 | +51 | **+123** | 34 | **82** |
| `correct_danger_prayer` | +295 | +334 | +327 | 1,335 | 1,308 |
| `wrong_danger_prayer` | −103 | −99 | −110 | 329 | 367 |
| `unnecessary_prayer` | −63 | −60 | −85 | 298 | 423 |
| `melee_pressure` | −348 | −225 | −266 | 242 | 284 |
| `wasted_attack` | −548 | −174 | −195 | 1,744 | 1,947 |
| `kiting` | +2,034 | +2,071 | +2,064 | 941 | 938 |
| `safespot_attack` | +5,171 | +2,389 | **+2,536** | 1,593 | **1,690** |
| `wave_stall` | — | **−1.6** | **−435** | 2.2 | **253** |
| `invalid_action` | −1,384 | −95 | −194 | 952 | 1,937 |
| `tick_penalty` | −97 | −41 | −44 | 8,222 | 8,846 |
| **total positive** | +41,031 | +35,726 | **+36,686** | | |
| **total negative** | −2,724 | −965 | **−1,601** | | |
| **net** | +38,307 | +34,761 | **+35,085** | | |

Notable shifts vs v28.1:
- `correct_jad_prayer`: **+140%** fires (34 → 82), +139% reward (51 → 123)
  — agent is praying during Jad more, confirming longer Jad fights
- `safespot_attack`: **+6%** fires, +6% reward — slight safespot increase
- `wave_stall`: 272× penalty cost — the cost of the extra Jad time
- `unnecessary_prayer`: +42% fires (298 → 423) — agent prays-during-idle
  more, possibly exploring Jad prayer patterns
- `invalid_action`: +104% fires (952 → 1,937), +104% penalty
- Net reward approximately unchanged (+35,085 vs +34,761, +0.9%)

### Trajectory — when did behaviors diverge

Same seed + identical first-1B code → **byte-identical** up to ~1.5B steps.

| step | v28.1 jad | v28.2 jad | v28.1 r63 | v28.2 r63 | v28.1 ep_len | v28.2 ep_len | v28.1 ws_fires | v28.2 ws_fires |
|---|---|---|---|---|---|---|---|---|
| [1.0, 1.5B) | 0.066 | 0.063 | 0.177 | 0.178 | 7,275 | 7,255 | 0.0 | 0.0 |
| [1.5, 2.0B) | 0.098 | **0.131** | 0.754 | **0.831** | 8,489 | 8,632 | 11 | 8 |
| [2.0, 2.5B) | 0.047 | **0.077** | 0.887 | 0.870 | 8,816 | 8,975 | 25 | **123** |
| [2.5, 2.75B) | 0.042 | **0.096** | 0.977 | 0.816 | 8,815 | 8,762 | 3 | **96** |
| [2.75, 3.0B) | 0.062 | **0.146** | 0.970 | 0.839 | 8,905 | 8,849 | 13 | **118** |
| [3.0, 3.25B) | 0.089 | 0.099 | 0.854 | 0.850 | 8,474 | 9,050 | 4 | **152** |
| [3.25, 3.5B) | 0.098 | **0.121** | 0.815 | 0.747 | 8,392 | 8,767 | 13 | **163** |

Key divergence points:
- **[1.5, 2.0B):** v28.2 first shows the benefit — +34% jad (0.131 vs
  0.098), +10% r63 (0.831 vs 0.754). Wave_stall still barely firing.
- **[2.0, 2.5B):** v28.2 jad +64% (0.077 vs 0.047); wave_stall starts
  firing meaningfully (123 vs 25). The agent has learned it can use the
  extra grace.
- **[2.75, 3.0B):** v28.2's best era — jad 0.146 (2.4× v28.1's 0.062).
  Wave_stall firing at 118/ep. Agent paying for longer Jad fights.
- **[3.25, 3.5B):** v28.2 ending stronger (jad 0.121 vs 0.098, +23%)
  despite wave_stall fires at 163 (highest bucket). Sustained modest
  improvement.

### Conditional kill rate per bucket (crucial diagnostic)

| step | v28 | v28.1 | **v28.2** |
|---|---|---|---|
| [1.0, 1.5B) | 40.1% | 37.0% | 35.7% |
| [1.5, 2.0B) | 16.5% | 13.0% | **15.7%** |
| [2.0, 2.5B) | 14.0% | 5.3% | **8.9%** |
| [2.5, 2.75B) | 43.2% | 4.3% | **11.7%** |
| [2.75, 3.0B) | **74.6%** | 6.4% | **17.4%** |
| [3.0, 3.25B) | 53.9% | 10.5% | 11.6% |
| [3.25, 3.5B) | 60.7% | 12.1% | **16.1%** |

v28.2 consistently 2–3× v28.1's conditional kill rate in mid-to-late
training. The wave_stall softening DID buy real improvement — the agent
is better at converting Jad encounters when given more time. But
conditional kill rate is still 3–5× below v28's safespot-era peak
(60–75% in the same buckets).

### Per-tick economics

Net per-tick reward during v28.2 training:

| step | safespot/tick | jad_pry/tick | ws/tick | net/tick | fires_ws |
|---|---|---|---|---|---|
| [1.5, 2.0B) | +0.286 | +0.009 | −0.001 | +0.290 | 7.8 |
| [2.0, 2.5B) | +0.287 | +0.015 | −0.018 | +0.279 | 122.7 |
| [2.5, 3.0B) | +0.287 | +0.012 | −0.018 | +0.277 | 107 avg |
| [3.0, 3.5B) | +0.286 | +0.015 | −0.024 | +0.273 | 157 avg |

The per-tick net is still very positive (+0.27–0.29) despite 150+
wave_stall fires. The agent's Jad safespot remains profitable even after
passing the 1200-tick threshold, because safespot_attack + correct_jad
still dwarf the wave_stall penalty in most of the ramp zone. This means
the extra grace didn't fully fix the incentive — it just added 200 ticks
of free operation and then compensated by letting the agent extend
further (paying ~0.02/tick from wave_stall during that extension).

### Peak-era distribution

Top-10 jad_kill_rate rows spread across training steps:

| run | top10 jad avg | range | step span |
|---|---|---|---|
| v28 | 0.513 | 0.480–0.597 | 2.74–2.82B (tight window) |
| v28.1 | 0.299 | 0.264–0.350 | 1.42–1.44B (tight spike) |
| **v28.2** | **0.251** | **0.236–0.277** | **1.40–3.43B (distributed)** |

v28.2's top performance is **spread across the entire training run**,
not concentrated in a single spike. This is a meaningful quality
difference — v28.1's 0.350 peak was one brief spike at 1.43B, while
v28.2's 0.277 peak is representative of mid-to-late training operating
levels. Sustainability matters for deployment.

### What worked

**1. Mid-late training conditional kill rate recovered partially.**
[2.75, 3.0B) went from 6.4% (v28.1) to 17.4% (v28.2), a 2.7× improvement.
The agent uses the 200 extra grace ticks to actually kill Jad sometimes.

**2. Ket-Zek melee regression largely fixed.** 20.77 → 8.98 ticks
(−57%). The rush-to-melee pathology that v28.1 developed at [3.0, 3.5B)
is substantially reduced; agent positions better when it has more
Jad-fight time available.

**3. Higher sustained / best-250M performance (+10%).** v28.2's peak is
lower but its sustained window is higher. Deployment-quality metric.

**4. More prayer-during-Jad activity.** `correct_jad_prayer_fires`
went from 34 → 82 (+141%). The telegraph observation gets more
training signal in v28.2 than v28.1.

**5. Final state marginally better across multiple metrics:**
wave_reached +1.3%, r63 +12%, conditional kill last 10% +24%.

### What didn't work

**1. Peak jad_kill_rate dropped further** (0.350 → 0.277, −21%).
I expected softening to push peak UP. Instead the agent reallocated the
extra grace into longer failing Jad fights (wave_stall fires 2 → 253)
rather than faster successful ones. The spike-peak pattern v28.1 had is
gone; what replaces it is flat mid-game at ~0.1–0.15.

**2. Peak conditional kill rate plummeted** (64% → 34%).
v28.1's 1.43B spike showed the agent could convert >60% of Jad reaches
when it first encountered them. v28.2 never reaches that conversion
density — too much training spent on failing longer fights.

**3. Prayer behavior got noisier.** Prayer_switches doubled (289 → 576),
correct-per-switch halved (4.86 → 2.48), unnecessary_prayer fires +42%.
The agent is experimenting more but not more precisely.

**4. `jad_kill` channel total dropped** (+254 → +206, −19%). The number
of actual Jad kills fell slightly despite the +140% increase in
`correct_jad_prayer_fires`. More prayer events, fewer kills — agent is
getting prayer signal but not translating to kills at the same rate.

**5. `attack_when_ready_rate` slightly worse** (0.923 → 0.888).
Consistent with the agent spending more ticks on prayer switching /
repositioning in longer Jad fights.

### Diagnosis

Raising `shape_wave_stall_start` from 1000 → 1200 unlocked a specific
behavior the agent was previously blocked from: **extending failing Jad
fights past the 1000-tick mark**. The agent now spends ~250 extra ticks
per episode in the 1200-1500 range, paying ~−435/ep for it, but doesn't
convert those extra ticks into proportionally more Jad kills. The
hypothesis "more time = more kills" was wrong — more time means more
*time spent failing*, not more successful kills.

The core problem is not the stall-cap height. It's that:
1. v28's 0.597 peak was driven by the safespot-forever exploit,
   which wave_stall of any flavor prevents.
2. The agent's "real" Jad-kill skill caps around 0.10–0.15 sustained
   jad_kill_rate — same for v28.1 and v28.2.
3. Further softening wave_stall may nudge that slightly but won't
   recover the v28 peak.

**v28.2 validates the direction (softer = slightly more sustained
performance) but not the magnitude of expected improvement.**

### Expected outcome audit

| prediction | target | actual | status |
|---|---|---|---|
| rwd_wave_stall_total | similar to v28.1 (−1 to −5/ep) | **−435/ep** | ❌ way off |
| max_wave_ticks final | < 500 | 308 | ✅ |
| peak jad_kill_rate | 0.40–0.55 | 0.277 | ❌ short |
| final reached_wave_63 | ≥ v28.1's 0.76 | 0.849 | ✅ |
| conditional kill rate | 25–35% | 12.1% | ❌ |
| ketzek_melee_ticks reduction | from 20.8 | 8.98 | ✅ |

3 of 6 predictions hit, 3 missed. The wave_stall cost prediction was
most wrong — I assumed the agent would continue avoiding threshold even
with more grace; instead it used the grace aggressively and paid the
penalty. The peak and conditional predictions missed because I
overestimated the ceiling unlocked by the softer threshold.

### Go/no-go decision for v28.3

From the v28.2 staged section, the go/no-go criteria were:
- peak ≥ 0.40 AND max_wave_ticks < 500 → keep softening
- peak < 0.40 OR no conditional improvement → stall cap isn't bottleneck
- max_wave_ticks > 1000 → revert

**Result: peak 0.277 (< 0.40), but conditional DID improve in mid-
training (5.3% → 8.9%, 6.4% → 17.4%). `max_wave_ticks` 308 well under
1000.**

This is a **mixed signal**. The peak criterion says "stop softening";
the conditional improvement says "it's helping but not enough".

Recommendations for v28.3 (three parallel options, user's call):

**Option A (continue softening):** `start=1500`, keep cap=−2.0.
Tests whether even more grace recovers peak. Risk: marginal returns
given we've already seen the extra 200 ticks mostly produces longer
failing fights. Probability of improvement: ~30%.

**Option B (address the positioning side):** keep v28.2's wave_stall
(start=1200), bump `shape_npc_melee_penalty` from −0.8 → −1.5 to fight
the ketzek-melee drift even harder. v28.1's recommendation (still open).
Addresses the "rushing into Ket-Zek range" behavior at its source.
Probability of improvement: ~50%.

**Option C (structural — change reward shape):** recognize that the
v28 peak was exploit-driven and cannot be recovered without either
(a) a much larger training budget, (b) a richer Jad-specific observation,
or (c) a different reward on Jad kills that doesn't depend on time-on-wave.
The cleanest next experiment would be to isolate Jad training — use the
v28.2 1.36B checkpoint as warm start and fine-tune for another 1–2B
steps on a Jad-only curriculum. Probability of significant improvement:
~70% but requires scaffolding we don't have yet.

**My recommendation: Option B** — single variable again (melee_penalty
bump), same run budget, uses the information we now have about why
v28.2 didn't fully deliver. If B fails to deliver > 0.35 peak, the
project's practical ceiling on cold-start 3.5B budget is probably
~0.35 for legitimate (non-exploit) Jad-kill policies, and we should
shift focus to warm-starts / curricula.

### Artifacts

- Wandb run: `7qsyzbyo`, tag `v28.2`, state `finished`
- Local log: `pufferlib_4/logs/fight_caves/7qsyzbyo.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/7qsyzbyo/*.bin`
  — the 2.75B checkpoint is the deployable artifact (jad_kill=0.277,
  reach63=0.839, ep_len=8,849)
- Compare to v28.1's deployable artifact: `j5l97xrw` 1.43B checkpoint
  (jad_kill=0.350, reach63=0.545, ep_len=7,923). v28.1's 1.43B has
  higher peak but lower reach63; v28.2's 2.75B has lower peak but much
  higher reach63 and more stable conditional kill rate going into the
  final stretch.

### v28-series consolidated summary

| | v28 | v28.1 | v28.2 | **v28.4** | v28.5 | v28.6 |
|---|---|---|---|---|---|---|
| run id | `h8o4lain` | `j5l97xrw` | `7qsyzbyo` | `onksd6md` | `ocd1nl6p` | `teg8ugmx` |
| `shape_wave_stall_start` | (removed) | 1000 | 1200 | **1400** | 1600 | 1600 |
| `shape_npc_melee_penalty` | −0.8 | −0.8 | −0.8 | −0.8 | **−1.0** | −0.8 |
| peak jad_kill | **0.597** | 0.350 | 0.277 | **0.386** | 0.072 | 0.296 |
| peak step | 2.79B | 1.43B | 2.75B | **3.26B** | 2.97B | 1.52B |
| best 250M window | 0.346 | 0.150 | 0.165 | **0.240** | 0.028 | 0.184 |
| best-250M step | 2.63B | 1.31B | 1.36B | **3.02B** | 2.79B | 1.35B |
| stability (final/peak) | 0.25 | 0.36 | 0.37 | **0.55** | 0.00 | 0.50 |
| final jad_kill | 0.148 | 0.127 | 0.103 | **0.214** | 0.000 | 0.148 |
| final reach63 | 0.370 | 0.762 | **0.849** | 0.466 | 0.000 | 0.626 |
| final wave_reached | 59.5 | 61.4 | **62.2** | 59.3 | 49.8 | 61.4 |
| last-10% condit. kill | 57.3% | 11.5% | 14.2% | **32.1%** | 2.0% | 12.3% |
| final ep_len | 17,192 | 8,209 | 8,746 | 7,987 | 6,072 | 9,184 |
| final max_wave_ticks | 7,894 | 302 | 308 | 369 | 259 | 588 |
| final ketzek_melee_ticks | 5.5 | **20.8** | 9.0 | 8.1 | 15.5 | 9.2 |
| final food_wasted | 11.6 | 2.6 | **2.2** | 10.2 | 4.7 | 9.9 |
| final correct-per-switch | 21.0 | 4.86 | 2.48 | 2.97 | **8.93** | 1.63 |
| final prayer_accuracy | 90.1% | 82.7% | 82.2% | 81.6% | 77.0% | 81.5% |
| rwd_wave_stall_fires | — | 2.2 | 253 | 39 | 0 | **522** |
| rwd_wave_stall_total | — | −1.6 | −435 | −55 | 0 | **−943** |
| rwd_correct_jad_prayer_fires | 1,426 | 34 | 82 | 44 | **0** | 125 |

**The practical production story:**
- v28's 2.73B peak (0.597 jad) was exploit-driven (safespot forever)
- v28.4's 3.02B-3.26B window (0.386 peak, 0.24 sustained) is the
  **best honest Jad-killer** the project has produced
- v28.2's 2.75B checkpoint is the best **reach-Jad-consistently**
  agent (r63=0.85 at end)
- v28.1's 1.43B checkpoint was a tight spike at 0.35 that stability
  didn't hold

**Three deployment options depending on use case:**
- Need the highest peak and don't mind spikiness: v28's 2.73B (exploit)
- Need consistent Jad reaches and moderate conditional conversion:
  v28.2's 2.75B (r63=0.85, cond=12%)
- Need the highest overall Jad-kill throughput: **v28.4's 3.26B
  (jad_kill=0.386, cond=58%, but lower reach63=0.66)**

---

## v28.1 (2026-04-22, completed — `j5l97xrw` / `warm-tree-318`)

Single-variable change from v28: re-added `shape_wave_stall_*` to counter
the safespot/prayer-spam attractor that drifted v28 from peak
`jad_kill_rate = 0.597` @ 2.79B down to 0.148 by 3.5B.

**Bottom line:** the wave_stall mechanism **worked exactly as designed for
what it was designed to prevent** — episode length stays flat at ~8.4k
ticks, `max_wave_ticks` holds at ~289 (vs v28's 7,894), prayer_switches
drops from 4,813 to 289 (94% less), and `reached_wave_63` doubles at
end-of-training (0.37 → 0.76). But it exposed a second-order pathology:
without the safespot-forever attractor, the agent now **rushes waves into
Ket-Zek melee range** (20.8 mean ticks vs 5.5 in v28, +279%) and loses
~40% of the peak jad_kill_rate (0.60 → 0.35), with conditional kill rate
dropping from 40% → 17% at end of training. v28.1 is strictly better for
stability but worse for peak extraction — a classic "fix one bug, expose
the adjacent one" trade.

### Config diff vs v28

| key | v28 | v28.1 |
|---|---|---|
| `shape_wave_stall_start` | — (removed) | **1000** |
| `shape_wave_stall_ramp_interval` | — (removed) | **150** |
| `shape_wave_stall_base_penalty` | — (removed) | **-0.5** |
| `shape_wave_stall_cap` | — (removed) | **-2.0** |

All other env/train/policy values **identical to v28**:
`w_wave_clear=15`, `w_jad_kill=2000`, `w_correct_jad_prayer=1.5`,
`w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`,
`shape_safespot_attack_reward=1.5`, `shape_unnecessary_prayer_penalty=-0.2`,
attack-style telegraph obs, logit-level mask, 3.5B steps, seed=73,
train.seed=42.

### Results — full metric sweep (final @ 3.5B)

**Core performance:**

| metric | v28 | v28.1 | Δ | % |
|---|---|---|---|---|
| peak `jad_kill_rate` | 0.597 @ 2.79B | **0.350 @ 1.43B** | −0.247 | **−41.4%** |
| final `jad_kill_rate` | 0.148 | 0.127 | −0.021 | −14.3% |
| stability (final/peak) | 0.25 | **0.36** | +0.11 | +46.0% |
| peak `reached_wave_63` | 0.921 | **1.000** | +0.079 | +8.6% |
| final `reached_wave_63` | 0.370 | **0.762** | +0.392 | **+105.7%** |
| final `wave_reached` | 59.46 | 61.39 | +1.93 | +3.2% |
| conditional kill rate (final) | 40.0% | 16.7% | −23.3pp | −58.3% |

**Stall / stability metrics (what wave_stall was meant to fix):**

| metric | v28 | v28.1 | Δ | % |
|---|---|---|---|---|
| `episode_length` | 17,192 | **8,209** | −8,982 | **−52.2%** |
| `max_wave_ticks` | 7,894 | **302** | −7,593 | **−96.2%** |
| `prayer_switches` | 4,813 | **289** | −4,524 | **−94.0%** |
| `food_wasted` | 11.65 | **2.60** | −9.05 | **−77.7%** |
| `pots_wasted` | 7.44 | **0.86** | −6.59 | **−88.5%** |
| `avg_hp_on_food` | 0.805 | 0.572 | −0.23 | −29% |
| `avg_prayer_on_pot` | 0.572 | 0.353 | −0.22 | −38% |
| `attack_when_ready_rate` | 0.825 | 0.923 | +0.10 | +12% |

**Positioning / combat exposure (where v28.1 regressed):**

| metric | v28 | v28.1 | Δ | % |
|---|---|---|---|---|
| `tokxil_melee_ticks` | 2.98 | 6.67 | +3.69 | **+124%** |
| `ketzek_melee_ticks` | 5.48 | **20.77** | +15.29 | **+279%** |
| `dmg_taken_avg` | 6,759 | 7,909 | +1,150 | +17% |
| `damage_blocked` | 672k | 162k | −510k | −76% |
| `prayer_up_melee` | 0.006 | 0.001 | −0.005 | −91% |

**Prayer (almost all channels moved, mostly improvements):**

| metric | v28 | v28.1 |
|---|---|---|
| `correct_prayer` | 2,674 | 1,403 |
| `wrong_prayer_hits` | 293 | 293 |
| `no_prayer_hits` | 76 | 58 |
| `prayer_up_range` | 0.253 | 0.306 |
| `prayer_up_magic` | 0.338 | 0.441 |
| correct per switch | 0.56 | **4.86** |

### Reward-channel breakdown (final state)

| channel | v28 total | v28.1 total | Δ | v28 fires | v28.1 fires |
|---|---|---|---|---|---|
| `damage_dealt` | +3,574 | +1,579 | −1,996 | 3,493 | 1,617 |
| `damage_taken` | −162 | −258 | −97 | 71 | 72 |
| `npc_kill` | +902 | +925 | +23 | 258 | 264 |
| `wave_clear` | +26,621 | +28,124 | +1,503 | 59 | 61 |
| `jad_kill` | +296 | +254 | −42 | 0.15 | 0.13 |
| `player_death` | −9.4 | −9.6 | −0.2 | 0.85 | 0.87 |
| `food_waste` | −8.0 | −1.6 | +6.4 | 12 | 3 |
| `pot_waste` | −1.0 | −0.1 | +0.9 | 2 | 0.2 |
| `correct_jad_prayer` | +2,138 | +51 | **−2,087** | 1,426 | **34** |
| `correct_danger_prayer` | +295 | +334 | +39 | 1,180 | 1,335 |
| `wrong_danger_prayer` | −103 | −99 | +4 | 343 | 329 |
| `unnecessary_prayer` | −63 | −60 | +4 | 317 | 298 |
| `melee_pressure` | −348 | −225 | +124 | 352 | 242 |
| `wasted_attack` | −548 | −174 | +374 | 5,477 | 1,744 |
| `kiting` | +2,034 | +2,071 | +37 | 925 | 941 |
| `safespot_attack` | +5,171 | +2,389 | **−2,782** | 3,447 | 1,593 |
| `wave_stall` | — | −1.6 | new | — | **2.2** |
| `invalid_action` | −1,384 | **−95** | +1,289 | 13,839 | **952** |
| `tick_penalty` | −97 | −41 | +56 | 19,475 | 8,222 |
| **total positive** | **+41,031** | **+35,726** | −5,305 | | |
| **total negative** | **−2,724** | **−965** | +1,759 | | |
| **net** | **+38,307** | **+34,761** | −3,546 | | |

The `wave_stall` channel fires only **2.2 times per episode** and costs
only **−1.6 reward**. Yet it drove a 94% reduction in prayer_switches, a
96% reduction in max_wave_ticks, and a 93% reduction in invalid_action
fires. This is a textbook example of a gradient-shaping reward — minimal
direct cost, large behavioral leverage.

### Trajectory — when did behavior diverge from v28

Same seed + identical code up to the wave_stall compute path means the
first ~1B steps are **byte-identical** between v28 and v28.1.

| step | v28 ep_len | v28.1 ep_len | v28 mwt | v28.1 mwt | v28 jad | v28.1 jad |
|---|---|---|---|---|---|---|
| [0.5, 1.0B) | 6,192 | 6,192 | 256 | 256 | 0.000 | 0.000 |
| [1.0, 1.5B) | 7,268 | 7,275 | 267 | 267 | 0.071 | 0.066 |
| [1.5, 2.0B) | 8,518 | 8,489 | 311 | 285 | 0.110 | 0.098 |
| [2.0, 2.5B) | 9,322 | 8,816 | 333 | 284 | 0.091 | 0.047 |
| [2.5, 3.0B) | **13,167** | 8,860 | **4,292** | 286 | 0.272 | 0.052 |
| [3.0, 3.5B) | **13,626** | 8,433 | **5,323** | 289 | 0.208 | 0.094 |

The runs are nearly identical through ~1.5B steps. Divergence becomes
significant at [2.0, 2.5B) when v28.1's max_wave_ticks stays flat at 284
while v28's has started to explode. By [2.5, 3.0B), v28 is in full
safespot-attractor mode (ep_len 13k+, mwt 4k+, pry_sw 2,345) while v28.1
is stable (ep_len 8.8k, mwt 286, pry_sw 381).

**Peak era in v28.1 is at 1.43B** (not the tail like v28's 2.79B):

| step | jad | reach63 | ep_len | ketzek_mel |
|---|---|---|---|---|
| 1.43B | 0.350 | 0.545 | 7,923 | 4.6 |
| 1.44B | 0.321 | 0.635 | 7,954 | 4.0 |
| 1.42B | 0.317 | 0.552 | 7,929 | 4.5 |

Clean profile — ~8k tick episodes, 55% reach Jad, 35% kill Jad,
conditional kill rate ~64%. This is a healthier policy than v28's peak
(which required 22k-tick episodes to achieve 60% kill rate).

### Late-training Ket-Zek regression (3.0B+)

`ketzek_melee_ticks` by bucket (v28.1):

| step | ketzek_mel |
|---|---|
| [0.25, 1.0B) | 3.72 |
| [1.0, 1.5B) | 3.77 |
| [1.5, 2.0B) | 4.11 |
| [2.0, 2.5B) | 5.22 |
| [2.5, 3.0B) | 3.93 |
| **[3.0, 3.5B)** | **14.23** |

Late-training jump is 3.6× the prior value. Something about the
[3.0, 3.5B) window pushed the policy into persistent Ket-Zek melee
exposure (possibly the agent found a sub-optimal "rush Ket-Zek +
tank-through-melee" strategy as a new local attractor).

### What worked

**1. wave_stall eliminated the safespot attractor entirely.**
Max wave ticks flat at 289 throughout training, never approaching the
1000-tick threshold. The agent learned the soft cap without ever paying
heavy penalties (total −1.6/ep).

**2. Stability improved without sacrificing wave progress.**
Final reach63 doubled (0.37 → 0.76). Final wave_reached up 3%. Agent
now consistently reaches Jad at end of training, which is the load-
bearing prerequisite for jad_kill_rate > 0.

**3. Resource economy cleaned up dramatically.**
Food_wasted −78%, pots_wasted −89%, avg_hp_on_food dropped 29% (eats at
lower HP), avg_prayer_on_pot dropped 38%. The agent no longer "panic-eats"
— consumable usage is tight and deliberate.

**4. Prayer precision per switch improved ~9× (0.56 → 4.86 correct per switch).**
Prayer_switches 4,813 → 289, correct_prayer 2,674 → 1,403. The agent is
making tactical prayer decisions, not farming correct-prayer reward by
spamming. This is closer to v26's 12.7-correct-per-switch policy than
anything since.

**5. `invalid_action_fires` collapsed from 13,839 → 952 (−93%).**
The invalid_action count was correlated with ep_len in v28 (longer
episodes = more invalid action picks). Short episodes + tighter policy
= far cleaner action selection.

### What didn't work

**1. Peak jad_kill_rate dropped 41%** (0.597 → 0.350).
v28's safespot exploit was actually the mechanism delivering high peak
jad_kill_rate — the agent could spend 7k+ ticks at Jad prayer-switching
until he died. Remove that, and the agent has to kill Jad the "proper"
way (LOS break + pray correctly + attack) in ~300-500 ticks. v28.1 is
undertrained on that tighter version because it spends less time at Jad
across training.

**2. Conditional kill rate at end plummeted** (40% → 17%).
v28.1 reaches Jad 2× more often but kills him 2.4× less often when it
gets there. The agent rushes waves to avoid wave_stall, arrives at Jad
with imperfect positioning / resources, fails.

**3. Ket-Zek melee exposure increased 279%** (5.5 → 20.8 ticks).
The rush-to-clear behavior pushes the agent into NPC melee range more
often. Particularly bad in [3.0, 3.5B) where it spiked to 14.2 (vs
~4 earlier in training).

**4. correct_jad_prayer channel collapsed** (1,426 fires → 34 fires).
Directly tied to (2): less time at Jad = fewer Jad prayer events. The
telegraph is still doing its job at the observation level but the policy
isn't getting many training signals there.

**5. Moderate dmg_taken increase** (6,759 → 7,909, +17%).
Shorter episodes should mean less cumulative damage, but per-tick damage
rose because the agent is in melee range more. Net bad outcome even with
damage_blocked dropping 76% (fewer prayer blocks).

### Conditional-on-reach analysis

| config | jad_kill | reach63 | conditional |
|---|---|---|---|
| v28 final | 0.148 | 0.370 | 40.0% |
| v28 peak (2.75B) | 0.597 | 0.657 | 90.9% |
| **v28.1 final** | **0.127** | **0.762** | **16.7%** |
| **v28.1 peak (1.43B)** | **0.350** | **0.545** | **64.2%** |

v28.1's peak conditional kill rate (64%) is strong but well below v28's
peak (91%). v28's peak required the safespot exploit; v28.1 can't access
that, so its peak caps lower.

### Diagnosis

The wave_stall penalty is too effective at eliminating safespot — it
removed the only reliable late-training Jad-kill strategy the agent had
discovered, without replacing it with a better one. In effect, the v28
agent had two things going at once: (a) a real Jad-killing skill that
worked on top of the (b) safespot farming exploit. v28.1 kept (a) the
skill but stripped out the (b) exploit, revealing how undertrained (a)
actually was.

Expected outcome audit:

| prediction | target | actual | status |
|---|---|---|---|
| max_wave_ticks | < 1,500 | 302 | ✅ beat |
| episode_length | 8k–11k | 8,209 | ✅ hit |
| peak jad_kill_rate | preserve 0.597 | 0.350 | ❌ −41% |
| stability | > 0.6 | 0.36 | ❌ short |
| prayer_switches | 2,000–3,000 | 289 | ⚠️ under |
| rwd_wave_stall_total | −50 to −300 | −1.6 | ⚠️ way under |

The "rwd_wave_stall_total −1.6" is informative: the policy learned to
avoid the threshold almost completely, never triggering more than the
first few ramp ticks per episode. Means the penalty is **over-calibrated**
— a softer penalty (e.g. base_penalty=-0.1 or start=1500) would still
shape the policy but might leave a bit more room for legitimate-but-slow
Jad fights without the peak regression.

### Observations & recommendations for v28.2+

**1. Soften wave_stall — APPLIED in v28.2 as single-variable test.**
v28.2 raises `shape_wave_stall_start` from 1000 → 1200 only (keeps ramp,
base_penalty, cap unchanged). If that isn't enough improvement, larger
softening candidates for v28.3:
   - `shape_wave_stall_start = 1500` (more grace for Jad kills)
   - `shape_wave_stall_base_penalty = -0.2` (gentler first ramp)
   - `shape_wave_stall_cap = -1.0` (softer ceiling, still dominant over
     +0.35/tick safespot reward but less aggressive)

**2. Consider `shape_npc_melee_penalty` bump — DEFERRED.** Left as a
future recommendation but NOT applied in v28.2 (to keep v28.2 as a clean
single-variable test). If the rush-to-melee regression persists after
softening wave_stall, bump `shape_npc_melee_penalty` from −0.8 → −1.5
to more strongly discourage the Ket-Zek rush pattern (current 242
fires/ep × −0.8 = −225 reward; −1.5 would double the signal).

**3. Stop at 2.0B** (or enable `anneal_lr=1`) if reproducing v28.1 exactly.
v28.1's peak is at 1.43B; the back 2B just drifted sideways with a
ketzek-melee regression in the tail. Stopping at 2B or LR-annealing would
capture the peak policy instead of letting it erode.

**4. Deploy the 1.43B checkpoint** as the production v28.1 artifact, not
the 3.5B final. At 1.43B: `jad_kill=0.35`, `reach63=0.55`, conditional
64%, ep_len 7,923, ketzek_mel 4.6 — a healthy, un-regressed policy.
Checkpoint path: `pufferlib_4/checkpoints/fight_caves/j5l97xrw/0000001419771904.bin`
(or nearest saved to 1.43B).

**5. Peak comparison against v28's peak is not apples-to-apples.** v28's
0.597 @ 2.79B depended on the exploit; v28.1's 0.350 @ 1.43B is the
first known "honest" Jad-kill policy at that rate. This is the better
scientific baseline going forward.

### Summary table — all v28-series runs

| | v26 | v27 | v27.3 | v28 | v28.1-broken | **v28.1** |
|---|---|---|---|---|---|---|
| peak jad_kill | 0.056 | 0.130 | 0.038 | **0.597** | 0.597 | 0.350 |
| peak @ | 3.44B | 1.77B | 3.50B | 2.79B | 2.79B | **1.43B** |
| final jad_kill | 0.000 | 0.000 | 0.038 | 0.148 | 0.148 | 0.127 |
| final reach63 | 0.258 | 0.228 | 0.916 | 0.370 | 0.370 | **0.762** |
| final ep_len | 7,650 | 7,755 | 8,181 | 17,192 | 17,192 | **8,209** |
| max_wave_ticks | 264 | 277 | 283 | 7,894 | 7,894 | **302** |
| prayer_switches | 110 | 1,852 | 2,293 | 4,813 | 4,813 | **289** |

v28.1 is the first run since v26 to combine:
- Stable short episodes (like v27.3 / v26)
- High reach63 at end (like v27.3)
- Respectable jad_kill_rate at end (better than v27.3, close to v28)
- Lowest prayer_switches since v26 / Phase 3

It's not the outright winner on peak jad_kill_rate, but it's the
**cleanest policy** the project has produced so far. Best candidate
for iterative refinement.

### Artifacts

- Wandb run: `j5l97xrw` / `warm-tree-318`, tag `v28.1`, state `finished`
- Local log: `pufferlib_4/logs/fight_caves/j5l97xrw.json`
- Checkpoints: `pufferlib_4/checkpoints/fight_caves/j5l97xrw/*.bin`
  — the 1.43B checkpoint is the deployable artifact
- Broken prior run: `b0n6xh74` / `jumping-haze-317`, tag `v28.1-broken`
  (see bug-fix note below)

### Backend bug-fix timeline

**2026-04-21 night:** ran v28.1 first attempt (`b0n6xh74`); trained
identically to v28 because `fc_puffer_compute_reward` was reconstructing
`FcRewardRuntime` fresh every tick from only `env->ticks_since_attack` —
I'd added `ticks_in_wave` to the runtime struct but missed the matching
`FightCaves.ticks_in_wave` persistence field. Result: timer
zero-initialized each tick, reached 1 in the compute, reset next tick,
never hit the 1000 threshold. Trajectory identical to v28 to 6 decimals.

**2026-04-22:** added `int ticks_in_wave` to `FightCaves`, loaded into
runtime at start of tick and saved back at end — matching the pattern
`ticks_since_attack` already used. Verified with direct C test
(threshold=5, ramp=3, base=-0.5, cap=-2.0): timer persists, ramps
correctly, clamps at cap, resets on wave_clear. Then re-ran as `j5l97xrw`
with full config. That's the data analyzed here.

The broken run has been re-tagged to `v28.1-broken` on wandb and in its
local JSON so it won't be confused with this result in future analysis.

---

## v28 (2026-04-21, completed — `h8o4lain` / `zany-jazz-316`)

Cold-start 3.5B run.

### Config diffs vs v26 and v27.x

| key | v26 | v27 | v27.1 | v27.2 | v27.3 | v28 |
|---|---|---|---|---|---|---|
| `w_damage_dealt` | 0.7 | 0.9 | 0.9 | 0.9 | 0.9 | 0.9 |
| `w_damage_taken` | -1.5 | -1.9 | -1.9 | -1.9 | -1.9 | -1.9 |
| `w_npc_kill` | 3.5 | 3.5 | 3.5 | 3.5 | 3.5 | 3.5 |
| `w_wave_clear` | 15 | 50 | 50 | 50 | 50 | 15 |
| `w_jad_kill` | 150 | 1500 | 3000 | 3000 | 3000 | 2000 |
| `w_player_death` | -11 | -11 | -11 | -11 | -11 | -11 |
| `w_correct_jad_prayer` | 2.0 | 1.5 | 1.5 | 1.5 | 1.5 | 1.5 |
| `w_correct_danger_prayer` | 0.05 | 0.035 | 0.035 | 0.035 | 0.15 | 0.25 |
| `w_invalid_action` | -0.1 | -0.1 | -0.1 | -0.1 | -0.1 | -0.1 |
| `w_tick_penalty` | -0.005 | -0.005 | -0.005 | -0.005 | -0.005 | -0.005 |
| `w_attack_attempt` | 0.2 | *removed* | *removed* | *removed* | *removed* | *removed* |
| `w_jad_damage` | 2.0 | *removed* | *removed* | *removed* | *removed* | *removed* |
| `shape_food_full_waste_penalty` | -6.5 | -6.5 | -6.5 | -6.5 | -6.5 | -6.5 |
| `shape_food_waste_scale` | -1.2 | -1.2 | -1.2 | -1.2 | -1.2 | -1.2 |
| `shape_pot_full_waste_penalty` | -6.5 | -6.5 | -6.5 | -6.5 | -6.5 | -6.5 |
| `shape_pot_waste_scale` | -1.2 | -1.2 | -1.2 | -1.2 | -1.2 | -1.2 |
| `shape_wrong_prayer_penalty` | -0.3 | -0.3 | -0.3 | -0.3 | -0.3 | -0.3 |
| `shape_npc_melee_penalty` | -0.8 | -0.8 | -0.8 | -0.8 | -0.8 | -0.8 |
| `shape_wasted_attack_penalty` | -0.1 | -0.1 | -0.1 | -0.1 | -0.1 | -0.1 |
| `shape_kiting_reward` | 2.2 | 2.2 | 2.2 | 2.2 | 2.2 | 2.2 |
| `shape_kiting_min_dist` | 2 | 2 | 2 | 2 | 2 | 2 |
| `shape_kiting_max_dist` | 10 | 10 | 10 | 10 | 10 | 10 |
| `shape_safespot_attack_reward` | 2.3 | 1.5 | 1.5 | 1.5 | 1.5 | 1.5 |
| `shape_unnecessary_prayer_penalty` | -0.2 | *removed* | *removed* | *removed* | *removed* | -0.2 |
| `shape_npc_specific_prayer_bonus` | 0.3 | *removed* | *removed* | *removed* | *removed* | *removed* |
| `shape_jad_heal_penalty` | -0.1 | *removed* | *removed* | *removed* | *removed* | *removed* |
| `shape_resource_threat_window` | 2 | 2 | 2 | 2 | 2 | 2 |
| `shape_wave_stall_start` | 500 | 500 | 500 | 500 | 500 | *removed* |
| `shape_wave_stall_base_penalty` | -0.5 | -0.5 | -0.5 | -0.5 | -0.5 | *removed* |
| `shape_wave_stall_ramp_interval` | 50 | 50 | 50 | 50 | 50 | *removed* |
| `shape_wave_stall_cap` | -2.0 | -2.0 | -2.0 | -2.0 | -2.0 | *removed* |
| `shape_not_attacking_grace_ticks` | 2 | 2 | 2 | 2 | 2 | *removed* |
| `shape_not_attacking_penalty` | -0.01 | -0.01 | -0.01 | -0.01 | -0.01 | *removed* |
| `total_timesteps` | 5B | 3.5B | 3.5B | 3.5B | 3.5B | 3.5B |
| `ent_coef` | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 |
| `hidden_size` | 256 | 256 | 256 | 256 | 256 | 256 |
| `num_layers` | 2 | 2 | 2 | 2 | 2 | 2 |
| action mask enforcement | obs | obs | obs | obs | obs | logit |
| attack-style telegraph | none | none | none | none | none | per-NPC one-hot + LOS |

### v28 active config

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
shape_resource_threat_window = 2

[train]
total_timesteps = 3_500_000_000
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

### Results

Behavioral metrics — peak and final.

| metric | v26 | v26.1 | v27 | v27.1 | v27.2 | v27.3 | v28 |
|---|---|---|---|---|---|---|---|
| peak `jad_kill_rate` | 0.040 | 0.094 | 0.114 | 0.114 | 0.000 | 0.038 | 0.597 |
| peak `jad_kill_rate` step | 3.46B | 3.31B | 1.77B | 1.77B | — | 3.50B | 2.75B |
| final `jad_kill_rate` | 0.004 | 0.000 | 0.000 | 0.000 | 0.000 | 0.038 | 0.148 |
| peak `reached_wave_63` | 0.926 | 0.916 | 0.953 | 0.953 | 0.000 | 0.916 | 0.921 |
| final `reached_wave_63` | 0.481 | 0.000 | 0.228 | 0.228 | 0.000 | 0.916 | 0.370 |
| peak `wave_reached` | 62.8 | 62.6 | 62.8 | 62.8 | 57.4 | 62.4 | 62.6 |
| final `wave_reached` | 61.07 | 25.34 | 60.03 | 60.03 | 49.56 | 62.00 | 59.46 |
| final `episode_length` | 7,935 | 2,517 | 7,755 | 7,755 | 5,629 | 8,181 | 17,192 |
| final `prayer_switches` | 127 | 53 | 1,852 | 1,852 | 2 | 2,293 | 4,813 |
| final `correct_prayer` | 1,334 | 134 | 1,369 | 1,369 | 767 | 1,130 | 2,674 |
| final `wrong_prayer_hits` | 391 | 13 | 467 | 467 | 262 | 451 | 293 |
| final `no_prayer_hits` | 44 | 154 | 40 | 40 | 15 | 75 | 76 |
| final `prayer_uptime_melee` | 0.000 | 0.000 | 0.050 | 0.050 | 0.000 | 0.000 | 0.006 |
| final `prayer_uptime_range` | 0.238 | 0.233 | 0.287 | 0.287 | 0.526 | 0.386 | 0.253 |
| final `prayer_uptime_magic` | 0.500 | 0.000 | 0.498 | 0.498 | 0.461 | 0.434 | 0.338 |
| final `damage_blocked` | 162,107 | 1,922 | 171,103 | 171,103 | 72,429 | 137,286 | 672,395 |
| final `dmg_taken_avg` | 7,606 | 2,318 | 8,425 | 8,425 | 5,885 | 10,125 | 6,759 |
| final `attack_when_ready_rate` | 0.948 | 0.846 | 0.922 | 0.922 | 0.943 | 0.926 | 0.825 |
| final `food_eaten` | 19.68 | 0.31 | 19.68 | 19.68 | 19.39 | 17.41 | 20.00 |
| final `food_wasted` | 4.93 | 0.27 | 1.58 | 1.58 | 6.87 | 0.15 | 11.65 |
| final `pots_used` | 28.67 | 0.00 | 29.57 | 29.57 | 30.65 | 29.34 | 29.74 |
| final `pots_wasted` | 1.13 | 0.00 | 0.85 | 0.85 | 8.78 | 1.16 | 7.44 |
| final `tokxil_melee_ticks` | 2.52 | 0.67 | 4.07 | 4.07 | 3.26 | 0.77 | 2.98 |
| final `ketzek_melee_ticks` | 11.41 | 0.00 | 18.82 | 18.82 | 11.37 | 2.44 | 5.48 |
| final `max_wave_ticks` | 264 | 205 | 277 | 277 | 238 | 283 | 7,894 |
| final `max_wave_ticks_wave` | 49.9 | 21.7 | 51.6 | 51.6 | 30.8 | 52.4 | 49.1 |

Reward-channel per-episode totals and fire counts (only v27.3+ have `rwd_*` analytics).

| channel | v27.3 total | v28 total | v27.3 fires | v28 fires |
|---|---|---|---|---|
| `damage_dealt` | +1,576 | +3,574 | 1,619 | 3,493 |
| `damage_taken` | -273 | -162 | 102 | 71 |
| `npc_kill` | +938 | +902 | 268 | 258 |
| `wave_clear` | +95,908 | +26,621 | 61.3 | 58.98 |
| `jad_kill` | +114 | +296 | 0.038 | 0.148 |
| `player_death` | -10.6 | -9.37 | 0.96 | 0.85 |
| `food_waste` | -0.10 | -7.95 | 0.24 | 12.17 |
| `pot_waste` | -0.06 | -0.97 | 0.14 | 1.91 |
| `correct_jad_prayer` | +4.32 | +2,138 | 2.88 | 1,426 |
| `correct_danger_prayer` | +167 | +295 | 1,116 | 1,180 |
| `wrong_danger_prayer` | -148 | -103 | 493 | 343 |
| `unnecessary_prayer` | — | -63 | — | 317 |
| `melee_pressure` | -354 | -348 | 366 | 352 |
| `wasted_attack` | -175 | -548 | 1,746 | 5,477 |
| `kiting` | +2,057 | +2,034 | 935 | 925 |
| `safespot_attack` | +2,379 | +5,170 | 1,586 | 3,447 |
| `invalid_action` | -262 | -1,384 | 2,620 | 13,839 |
| `tick_penalty` | -41.1 | -97.4 | 8,223 | 19,475 |
| `wave_stall` | 0.000 | — | 0 | — |
| `not_attacking` | -1.28 | — | 128 | — |

---

## v27 series — recommendations (as of 2026-04-21)

Based on the cross-run reward-channel analysis of v27/v27.1/v27.2/v27.3 reruns.
These are actionable changes derived from concrete data, not speculation.

### Tier A — confirmed-safe cleanup (zero behavioral risk)

The new `rwd_*` analytics definitively showed several hparams as dead weight
across all 4 configs:

**Remove `shape_wave_stall_*` family (4 hparams):**
- `rwd_wave_stall_fires = 0` in ALL 4 v27.x runs across ~40,000 episodes combined
- The 500-tick `shape_wave_stall_start` threshold is never reached
- Max wave duration observed: ~258 ticks (less than half the threshold)
- Params to remove: `shape_wave_stall_start`, `shape_wave_stall_base_penalty`,
  `shape_wave_stall_ramp_interval`, `shape_wave_stall_cap`

**Remove `shape_not_attacking_*` family (2 hparams):**
- `rwd_not_attacking_total` contributes 0.7-0.8% of `rwd_wasted_attack_total` in ALL 4 runs
- 129/fires per episode × -0.01 = -1.3 reward/ep — noise floor vs wasted_attack's -175/ep
- Firing conditions overlap: `wasted_attack` covers the behaviorally-meaningful cases
- Params to remove: `shape_not_attacking_grace_ticks`, `shape_not_attacking_penalty`

Expected impact: zero behavioral change, 6 fewer hparams in config/code surface.

### Tier B — address the action-mask gap (moderate risk)

`rwd_invalid_action_fires = 2500-2620 per episode (~32% of ticks)` across ALL 4
runs regardless of reward shaping. The agent picks masked actions on 1 in 3
ticks after 3.5B steps. PufferLib doesn't enforce the mask — it's an obs-only
feature, and the -0.1 reward signal isn't strong enough to teach the policy
to avoid masked actions.

**Option B1:** bump `w_invalid_action: -0.1 → -0.3` (3× strength)
- Current per-ep impact: -262 reward
- Bumped: ~-786 reward/ep — competitive with wave_clear's contribution per wave
- Risk: brief behavior regression while the policy re-optimizes mask avoidance

**Option B2:** enforce mask at logit level (larger code change)
- Zero out logits for masked actions before sampling
- Would reduce invalid_action_fires to ~0 immediately
- Requires modifying the policy forward pass in pufferlib/models.py
- Higher upside but more complex change

### Tier C — address wave_clear dominance (high reward, high risk)

The structural cap on Jad-kill learning across all 4 runs. wave_clear total
dominates the next-largest positive channel by 23-40× in every config, and
dominates Jad-kill reward by 840× (v27.3) or infinity (v27/v27.1/v27.2 where
Jad was never killed). Gradient follows density × magnitude, and wave_clear
is both dense-ish (60 fires/ep) and huge magnitude (~+1500/fire).

**Option C1:** reduce `w_wave_clear: 50 → 15` (Phase 3 / v26 baseline)
- Wave_clear total drops from ~96k to ~29k per ep
- Jad-kill's share of positive reward goes from 0.12% to 0.4% (still dominated, but 3× more visible)
- Risk: may reduce wave-reach rate if the policy needs that large a wave-clear signal to learn positioning

**Option C2:** add back `shape_reach_wave_63_bonus = +500` (one-shot milestone reward)
- Gives Jad-reach a step-function reward independent of accumulated wave clears
- Would fire ~90% of episodes in v27.3 = +450 per ep (~4× the Jad-kill reward)
- Low risk (just adds a positive signal on a channel already active)

**Option C3:** add back `shape_jad_kill_bonus = +500` or +1000
- Additional spike on top of w_jad_kill when Jad dies
- Makes Jad kills more impactful when they do fire
- Combined with C2, could shift the reward landscape meaningfully

**Recommendation order (if pursued):**
1. **Tier A first** — cheap cleanup, clean baseline for next experiments
2. **Tier C1** — address the structural dominance issue
3. **Tier B1 OR B2** — fix the mask-learning gap independently
4. (Optional) Tier C2 / C3 if Jad rate still hasn't emerged

Specific immediate recommendation: **Tier A + Tier C1 combined as v27.4** — one
config change set, clean-up + the highest-leverage single change. If Jad rate
improves meaningfully, iterate on prayer weights next. If not, try Tier B or C2.

### Context: why these specific changes over alternatives

Alternative ideas considered but NOT recommended based on v27.x evidence:

- **"Bump `w_jad_kill` further"** — v27→v27.1 doubled it with zero effect. Sparse
  rewards cannot overcome dense rewards via magnitude alone. Re-trying this
  direction would reproduce the null experiment.
- **"Bump `w_correct_jad_prayer`"** — v27.3 fired it only 2.9×/ep. The underlying
  problem is Jad reach rate, not Jad-prayer weight magnitude. Weight bumps don't
  help until reach rate improves.
- **"Lower `w_correct_danger_prayer` further"** — v27.2 proved 0.035 breaks
  prayer learning entirely (89.6% break-even unreachable). Stay in the
  0.05-0.2 range.
- **"Increase `ent_coef`"** — v26.1 collapse proved 0.007 is too low. Current
  0.01 is fine; bumping higher would add exploration noise without addressing
  the gradient-density issue.

---

## v27.3 (2026-04-20, completed — `tvrskte8`)

Status:
- completed — first v27.x run to HOLD peak through end of training
- peak `jad_kill_rate = 0.0382` at 3.50B steps, final = 0.0382 (held — no collapse)
- first run with new `rwd_*` per-channel reward analytics

**Outcome: traded peak for stability.** Peak is 3.4× lower than v27 (0.038 vs 0.130)
but stability is 130× better (0.253 vs 0.002). v27.3 didn't collapse — peaked at
step 3.50B (end of training, same profile as `4jd8ivr8`) and held through end.
Prayer learning was restored (1116 correct fires/ep vs v27.2's effective zero)
without the farming dominance of v27.

The new reward-channel analytics revealed the core bottleneck: **wave_clear
dominates total episode reward 40× over any other channel and 840× over Jad-kill.**
Jad-kill gradient is drowned out by wave-clearing gradient. This is structural
to the current reward landscape, not fixable by tuning Jad-specific weights.

Actual run:
- `fallen-deluge-311` (id `tvrskte8`), wandb group `v27.3`

Results summary:
- peak `jad_kill_rate`: 0.0382 at 3.50B
- final `jad_kill_rate`: 0.0382 (held peak)
- last 10% mean: 0.0097 (dropped slightly but nowhere near v27's full collapse)
- stability (last 10% / peak): 0.253
- peak `reached_wave_63`: 0.916
- final `reached_wave_63`: 0.916 (maintained)
- peak `wave_reached`: 62.41
- final `prayer_switches`: 2,293 (still high — not Phase-3-efficient)
- episode length final: 8,181 ticks
- runtime: ~30 min, cold start completed to 3.5B steps

TL;DR below.

### Reward channel breakdown (final state, per episode avg)

Sorted by absolute total contribution. All 19 channels logged via new
`rwd_*_total` and `rwd_*_fires` instrumentation.

| channel | fires/ep | total/ep | per-fire | % of ticks |
|---|---|---|---|---|
| **wave_clear** | 61.3 | **+95,908** | +1,563 | 0.75% |
| safespot_attack | 1,586 | +2,379 | +1.500 | 19.4% |
| kiting | 935 | +2,057 | +2.200 | 11.4% |
| damage_dealt | 1,619 | +1,576 | +0.973 | 19.8% |
| npc_kill | 268 | +938 | +3.500 | 3.3% |
| melee_pressure | 366 | -354 | -0.968 | 4.5% |
| damage_taken | 102 | -273 | -2.661 | 1.3% |
| **invalid_action** | **2,620** | **-262** | -0.100 | **32.0%** |
| wasted_attack | 1,746 | -175 | -0.100 | 21.3% |
| correct_danger_prayer | 1,116 | +167 | +0.150 | 13.6% |
| wrong_danger_prayer | 493 | -148 | -0.300 | 6.0% |
| jad_kill | 0.04 | +114 | +3,000 | 0.0005% |
| tick_penalty | 8,223 | -41 | -0.005 | 100% |
| player_death | 1.0 | -10.6 | -11.000 | 0.01% |
| correct_jad_prayer | 2.9 | +4.3 | +1.500 | 0.04% |
| not_attacking | 128 | -1.3 | -0.010 | 1.6% |
| food_waste | 0.24 | -0.1 | -0.427 | 0.003% |
| pot_waste | 0.14 | -0.06 | -0.443 | 0.002% |
| **wave_stall** | **0.0** | **0.0** | N/A | **0%** |

Net reward: +~102,000 per episode. `wave_clear` alone = 94% of all positive reward.

Sanity check: per-fire values all match configured weights (safespot=1.5,
kiting=2.2, wrong=-0.3, wasted=-0.1, tick=-0.005, jad_kill=3000, etc.) —
confirms instrumentation is correct.

### Key findings from analytics

**1. `wave_clear` dominance (40× over next channel):**
- wave_clear total: +95,908 / ep
- Next largest positive (safespot_attack): +2,379 / ep → 40× smaller
- jad_kill total: +114 / ep → 840× smaller than wave_clear

This is the structural reason Jad-kill learning is slow. PPO gradient flows
where the reward flows. Wave-clearing dominates, so the policy optimizes
wave-clearing; Jad is a side effect. Even v27.1's 2× bump to w_jad_kill
(1500→3000) couldn't change this (we already saw it was a null experiment).

**2. `wave_stall` is completely dead (0 fires):**
- The 500-tick `shape_wave_stall_start` threshold is never hit.
- Max wave duration in v27.3: 258 ticks (seen via max_wave_ticks metric).
- All 4 `shape_wave_stall_*` hparams are contributing zero. Candidates for removal.

**3. `invalid_action` fires on 32% of ticks:**
- 2,620 fires per episode × -0.1 = -262 reward per episode.
- PufferLib doesn't enforce the action mask (confirmed earlier); mask is an
  obs-only feature. Policy must learn to avoid masked actions purely via this
  penalty, and after 3.5B steps it still picks masked actions 1/3 of ticks.
- Option: bump `w_invalid_action` from -0.1 to -0.3 or -0.5 to strengthen
  the learning signal on action validity.

**4. `not_attacking` is nearly redundant with `wasted_attack`:**
- `wasted_attack`: 1,746 fires × -0.1 = -175 reward/ep
- `not_attacking`: 128 fires × -0.01 = -1.28 reward/ep (**0.7% of wasted_attack's impact**)
- Ratio 13.6× — they fire on different subsets but not_attacking's contribution
  is in the noise floor vs wasted_attack. User's earlier intuition confirmed.

**5. `food_waste` and `pot_waste` near-dead:**
- 0.24 and 0.14 fires/ep respectively. Full-waste flat penalty (-6.5 for eating
  at full HP) would fire <0.1× per episode. Scaled version almost never relevant.
- Agent rarely over-heals detectably. Candidates for simplification.

**6. Prayer balance hit the sweet spot:**
- correct_danger_prayer total: +167 (1,116 fires)
- wrong_danger_prayer total: -148 (493 fires)
- Net: +19 per episode — barely positive
- Actual prayer accuracy: 1116 / (1116+493) = **69.4%**, just above the 66.7%
  break-even math predicted
- Confirms v27.3's `w_correct_danger_prayer = 0.15` is in the bootstrap-able
  sweet spot (above v27.2's abandonment cliff, below v27's farming threshold)

**7. Prayer switches still high (2,293/ep):**
- Phase 3 tops: 242–778 (2.5–9× lower)
- Agent isn't Phase-3-efficient — churning through switches to catch blocks
  rather than pre-committing to expected styles. This is the remaining gap
  between v27.3 and Phase 3 top behavior.

### What improved / regressed / stayed the same (vs v27/v27.1/v27.2)

**Improved:**
- Stability: 0.002 → **0.253** (130× better — only run that held peak)
- Final `jad_kill_rate`: 0.000 → **0.038** (first post-v27 non-zero final)
- Final `reached_wave_63`: 0.228 → **0.916** (4× better — maintained Jad-reach capability)
- Peak step moved to 3.50B (end-of-training), matching `4jd8ivr8`'s stable pattern

**Regressed:**
- Peak `jad_kill_rate`: 0.130 → **0.038** (3.4× lower peak)
- Peak `reached_wave_63`: 0.969 → 0.916 (marginally lower)

**Stayed the same:**
- Wave-clearing rate (61+ clears/ep — ceiling for both)
- Attack tempo (attack_when_ready similar)
- `prayer_switches` still high (2,293 vs v27's 1,852 — same churny behavior regime)
- Food/pot usage patterns

### Recommendations for v27.4 (three tiers)

**Tier A — cleanup based on v27.3 evidence (safe, zero behavioral risk):**
- Remove `shape_wave_stall_*` entirely (4 hparams, 0 fires observed)
- Remove `shape_not_attacking_*` (2 hparams, -1.28/ep impact — noise vs wasted_attack's -175)
- Expected result: simpler config, no behavioral change

**Tier B — fix action-mask learning (moderate):**
- Bump `w_invalid_action`: -0.1 → -0.3 or -0.5
- Current penalty -262/ep would become -786 to -1,309/ep
- Forces policy to learn the mask more aggressively
- Risk: brief regression while policy adapts; upside: cleaner action selection

**Tier C — address wave_clear dominance (high reward, high risk):**
- Option 1: reduce `w_wave_clear`: 50 → 15 (Phase 3 baseline).
  Total drops from 95k to 28k/ep; Jad reward becomes more visible
  (still dominated but less so).
- Option 2: reintroduce `shape_reach_wave_63_bonus` (was removed in v27) as
  one-shot reward for reaching Jad (+500 or similar).
- Option 3: reintroduce `shape_jad_kill_bonus` (was 0.0 in v26, removed in v27).

Recommendation order: **Tier A first** (quick cleanup, makes future runs
easier to analyze), then **Tier C #1** (`w_wave_clear: 50 → 15`) to address
the root structural issue. Tier B (invalid_action) is a separate experiment.

Goal:
- recover from v27.2's catastrophic prayer-learning shutdown by bumping
  `w_correct_danger_prayer` from 0.035 back up to 0.15 — moderately above Phase 3's
  pinned 0.075 but well under v27's 0.3 farming-inducing level
- bet on a "middle ground" break-even (66.7% required accuracy) that is cleanly
  bootstrap-able from random, while avoiding the farming regime at ≤50% break-even
- keep `w_correct_jad_prayer = 1.5` from v27.2 so we finally get to test whether
  the Jad-specific signal at 1.5 is sufficient (v27.2 never reached Jad so we
  couldn't test this)

Rationale:
- v27 (w_correct_danger_prayer=0.3) → prayer farming (break-even 50%) → collapse.
- v27.2 (w_correct_danger_prayer=0.035) → prayer abandonment (break-even 89.6%) →
  never reached Jad. Required accuracy too high to bootstrap from random.
- Phase 3 tops (w_correct_danger_prayer=0.075) → break-even 80% → worked, peak
  Jad kills 15–34%.
- v27.3 picks **0.15** as a calibrated mid-point:
  - Break-even accuracy: `|−0.3|/(0.15+|−0.3|) = 66.7%` — easy to bootstrap, but
    high enough that farming isn't trivially optimal.
  - 2× higher than Phase 3's winning value → stronger gradient from correct blocks
    than Phase 3, so the agent should learn prayer *faster* than Phase 3 tops did.
  - 2× lower than v27 → still well above the farming threshold (since episode-avg
    correct_prayer = 1200, per-ep prayer reward is now 180, vs v27's 360).
- The bet: 0.15 sits in a Goldilocks zone where (a) gradient is positive enough for
  learning, (b) not so large that farming becomes optimal policy, and (c) the
  amplified w_jad_kill=3000 + w_correct_jad_prayer=1.5 can push the policy toward
  Jad-specific skill development once basic prayer learning is established.

Diff versus v27.2 (only):
- `w_correct_danger_prayer: 0.035 → 0.15` (4.3× increase — moves from no-learning zone to learning zone)

Everything else from v27.2 identical: `w_correct_jad_prayer=1.5`, `w_wave_clear=50`,
`w_jad_kill=3000`, damage_dealt formula, Jad/danger mutex, melee extension, all
hparam removals.

Diff versus v27 (cumulative):
- `w_correct_danger_prayer: 0.3 → 0.15` (halved)
- `w_correct_jad_prayer: 0.3 → 1.5` (5× up)
- `w_wave_clear: 30 → 50` (v27.1 carryover)
- `w_jad_kill: 1500 → 3000` (v27.1 carryover)

Diff versus Phase 3 tops (for reference):
- `w_correct_danger_prayer: 0.075 → 0.15` (2× higher than Phase 3 pin)
- `w_correct_jad_prayer: 2.2–4.6 (Phase 3 range) → 1.5` (lower than Phase 3)
- plus all the v27-series structural changes: damage_dealt + hits_landed, Jad/danger mutex, melee danger extension, hparam consolidation, etc.

Exact active config (changes from v27.2 in bold):
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model
- reward weights: `w_damage_dealt=0.9` (landed-hit formula), `w_damage_taken=-1.9`, `w_npc_kill=3.5`, `w_wave_clear=50.0`, `w_jad_kill=3000.0`, `w_player_death=-11.0`, `w_correct_jad_prayer=1.5` (Jad-only), **`w_correct_danger_prayer=0.15`** (non-Jad, any styled hit including melee), `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_wrong_prayer_penalty=-0.3`, `shape_npc_melee_penalty=-0.8`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=2.2`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=1.5`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=3_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Reward-scale context at v27.3 base:
- Break-even accuracy: 66.7% (Phase 3 tops: 80%, v26: 86%, v27.2: 89.6% broken, v27: 50% farmy)
- Per correct prayer block (non-Jad): 0.15
- Per wrong prayer block: -0.3 (unchanged, via shape_wrong_prayer_penalty)
- Per correct Jad prayer block: 1.5 (via Jad-only mutex path)
- Expected prayer-block reward/ep at 1200 blocks, 80% accuracy: `1200 × (0.8 × 0.15 - 0.2 × 0.3) = 1200 × 0.06 = 72` (well below v27's 360 farming level, well above v27.2's ~0 abandonment level)
- Expected Jad-kill reward/ep at 20% conversion: 3000 × 0.2 = 600
- Jad-kill/prayer ratio at 20% / 80%: 600 / 72 = **8.3×** (vs v27.2 projected 17.8×, v27 0.63×)

Expected vs actual behavior:
- `prayer_switches` at peak expected 500–1500; actual **2,293** — still high (churny, not Phase-3-efficient)
- `reached_wave_63` peak expected ≥ 0.85; actual **0.916** ✓
- `jad_kill_rate` peak target ≥ 0.15; actual **0.038** ✗ (much lower than target)
- Peak step target 2.0–3.2B; actual **3.50B** (end-of-training, like `4jd8ivr8`)
- `correct_prayer` > 1000 expected; actual **1,116** ✓
- `prayer_uptime_melee` > 0.02 expected; met ✓ (prayer learning restored)

Net: stability goals met, peak magnitude goal missed. The new rwd_* analytics
explain why — wave_clear dominates the gradient landscape, preventing Jad from
becoming a meaningful optimization target. Tier C recommendations address this.

---

## v27.2 (2026-04-20, completed — `yck7e9ni`)

Status:
- completed, **catastrophic failure** — never reached Jad in any episode
- peak `jad_kill_rate = 0.0000`, peak `reached_wave_63 = 0.0000`, peak `wave_reached = 57.48`
- policy regime change: agent abandoned prayer switching entirely (2 switches/ep at end)

Outcome vs plan:
- The plan: reduce the dense `w_correct_danger_prayer` signal from 0.3 to 0.035 to
  kill the farming behavior, while keeping `w_correct_jad_prayer=1.5` to preserve
  Jad-specific learning.
- The result: prayer learning broke entirely. Agent picked "range+magic alternating,
  never melee" and kept prayer on ~99% of the time to block whatever it happened
  to be correct for. Melee NPCs hit through wrong-prayer penalty, agent died at
  wave 49–57 before reaching Jad.
- The learning: `w_correct_danger_prayer` has a cliff below Phase 3's pinned value
  of 0.075. The wrong-prayer penalty at -0.3 dominates at low correct-rewards,
  making the expected value of prayer-switching attempts negative at cold-start
  accuracy levels (~25–33%).

Actual run:
- `earthy-feather-307` (id `yck7e9ni`), wandb group `v27.2`

Results summary (worst-in-series):
- peak `jad_kill_rate`: **0.0000** (zero kills across 3.5B steps)
- peak `reached_wave_63`: **0.0000** (never reached wave 63)
- peak `wave_reached`: **57.48**
- final `wave_reached`: 49.56
- final `prayer_switches`: **2** (vs v27 1852, vs Phase 3 range 68–795)
- final `prayer_uptime_melee`: **0.000** (agent abandoned melee prayer)
- final `damage_blocked`: 72,429 (58% reduction from v27)
- final `dmg_taken_avg`: 5,885 (lower, but only because episodes died sooner)
- runtime: ~30 min, cold start completed to 3.5B steps

Root-cause analysis — TL;DR:

**The break-even math I should have run before shipping v27.2**:

```
break_even_accuracy = |w_wrong_penalty| / (w_correct + |w_wrong_penalty|)
```

| config | `w_correct_danger` | `w_wrong_penalty` | ratio | break-even |
|---|---|---|---|---|
| v27, v27.1 | 0.300 | -0.300 | 1:1 | 50% (farmy) |
| Phase 3 tops | 0.075 | -0.300 | 1:4 | 80% (bootstrap-able) |
| v26 | 0.050 | -0.300 | 1:6 | 86% (marginal) |
| v27.2 | 0.035 | -0.300 | 1:8.6 | **89.6% (not reachable from random)** |

At v27.2's 89.6% break-even threshold, a cold-start policy (25–33% random prayer accuracy) faces a negative expected value for prayer attempts regardless of where it climbs — because even 85% accuracy gives expected value -0.015 per attempt. The gradient never goes positive until the policy is near-expert, and it can't reach near-expert without positive gradient pulling it there. Bootstrap impossible.

What improved (fake improvements from policy collapse):
- `attack_when_ready_rate`: 0.905 → 0.972 (no prayer-dithering, just attack)
- `dmg_taken_avg` final: 8425 → 5885 (episodes died sooner)

What regressed:
- Peak `jad_kill_rate`: 0.130 → **0.000**
- Peak `reached_wave_63`: 0.969 → **0.000**
- Peak `wave_reached`: 62.86 → 57.48
- `damage_blocked` final: 171k → 72k
- **Prayer learning entirely destroyed**: 1852 switches → 2
- `prayer_uptime_melee`: 0.050 → **0.000** (never used melee prayer)

What stayed the same:
- `pots_used`, `food_eaten` (full inventory)
- Jad/danger mutex mechanism (untested — never reached Jad)
- `w_damage_dealt` formula (fine in isolation)
- `w_correct_jad_prayer = 1.5` (untested — never reached Jad)

What v27.2 confirms:
- `w_correct_danger_prayer = 0.075` is a **floor**, not an upper bound to push past
- The "go past the bound" heuristic doesn't universally apply; when a sweep pins
  at a bound, it's signaling the optimum is at or near that bound, not further
- Below 0.075, prayer learning requires >85% break-even accuracy — not reachable
- `w_wave_clear = 50` and `w_jad_kill = 3000` are inert when policy is stuck
  (same null result as v27.1, now extended to a different stuck regime)

Diff versus v27.1:
- `w_correct_danger_prayer: 0.3 → 0.035` (8.6× down — broke prayer learning)
- `w_correct_jad_prayer: 0.3 → 1.5` (5× up — untested, never reached Jad)

Diff versus v27 (cumulative):
- `w_correct_danger_prayer: 0.3 → 0.035`
- `w_correct_jad_prayer: 0.3 → 1.5`
- `w_wave_clear: 30 → 50` (v27.1 carryover)
- `w_jad_kill: 1500 → 3000` (v27.1 carryover)

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model
- reward weights: `w_damage_dealt=0.9` (landed-hit formula), `w_damage_taken=-1.9`, `w_npc_kill=3.5`, `w_wave_clear=50.0`, `w_jad_kill=3000.0`, `w_player_death=-11.0`, `w_correct_jad_prayer=1.5` (Jad-only), `w_correct_danger_prayer=0.035` (non-Jad, any styled hit including melee), `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_wrong_prayer_penalty=-0.3`, `shape_npc_melee_penalty=-0.8`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=2.2`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=1.5`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=3_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

---

## v27.1 (2026-04-20, completed — `pbvgt23m`)

Status:
- completed, collapsed — outcome indistinguishable from v27 (byte-for-byte identical metrics)

Outcome vs plan:
- The plan: doubling `w_wave_clear` and `w_jad_kill` would amplify the "correct" rewards
  enough to overcome the prayer-farming signal that sank v27.
- The result: **training was effectively identical to v27.** Every milestone metric matched
  to 3+ decimal places. The doubled weights had no meaningful effect on policy learning.
- The learning: this is a falsification of the "amplify sparse rewards" hypothesis.
  PPO gradient is dominated by reward frequency, not reward magnitude. Doubling a
  sparse reward doesn't outcompete a dense reward.

Actual run:
- `proud-universe-306` (id `pbvgt23m`), wandb group `v27.1`

Results summary (all identical to v27 within numerical noise):
- peak `jad_kill_rate`: 0.1298 @ 1.77B (v27: 0.1298 @ 1.77B)
- peak `reached_wave_63`: 0.969 (v27: 0.969)
- peak `wave_reached`: 62.858 (v27: 62.858)
- peak `prayer_switches`: 6,187 (v27: 6,187)
- final `jad_kill_rate`: 0.000 (v27: 0.000)
- full series correlation with v27: essentially 1.0; max trajectory divergence on jad_kill_rate: 0.003
- runtime: ~30 min, cold start completed to 3.5B steps

Root-cause analysis — TL;DR:

1. **The policy was already ceiling'd on wave-clearing**. `reached_wave_63` peaked at 96.9%
   in both runs. Doubling `w_wave_clear` doesn't create gradient when there's no
   "clear more waves" alternative to differentiate against.
2. **Jad-kill reward is too sparse for magnitude to matter**. Expected per-ep reward went
   from 195 (v27) to 390 (v27.1). Prayer-block expected reward (360/ep) dominates either
   way until the Jad rate crosses ~25%, which neither v27 nor v27.1 reached.
3. **Same seed → near-deterministic trajectory**. Rare-event reward differences produce
   ~zero early gradient. By the time Jad kills start firing (1.0B+), the policy is already
   committed to the prayer-farming local optimum. The bigger reward arrives but doesn't
   redirect a fully-converged policy.
4. **Core PPO insight**: a dense reward firing every tick at 0.3 outweighs a sparse reward
   firing once per 1000 ticks at 3000. Frequency > magnitude for gradient influence.

What v27.1 confirms (the null result is informative):
- w_wave_clear in the 30-50 range is inert for our current policy. Not harmful; not
  helpful. Could be set anywhere.
- w_jad_kill in the 1500-3000 range is inert for our current policy. Same.
- The prayer-farming problem cannot be solved by amplifying competing rewards.

What v27.1 tells us to do next:
- Reduce `w_correct_danger_prayer` directly (v27.2 path) — the dense signal is the problem.

Diff versus v27:

**Reward weight changes (only):**
- `w_wave_clear: 30.0 → 50.0`
- `w_jad_kill: 1500.0 → 3000.0`

Everything else from v27 identical.

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model
- reward weights: `w_damage_dealt=0.9` (landed-hit formula), `w_damage_taken=-1.9`, `w_npc_kill=3.5`, `w_wave_clear=50.0`, `w_jad_kill=3000.0`, `w_player_death=-11.0`, `w_correct_jad_prayer=0.3` (Jad-only), `w_correct_danger_prayer=0.3` (non-Jad, any styled hit including melee), `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_wrong_prayer_penalty=-0.3`, `shape_npc_melee_penalty=-0.8`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=2.2`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=1.5`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=3_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

---

## v27 (2026-04-20, completed — `5osguwrs`)

Status:
- completed, collapsed (peak 0.130 at 1.77B, final 0.000)

Outcome vs plan:
- Peak `jad_kill_rate = 0.130` at 1.77B steps — earlier than any Phase 3 top run but lower
- Full collapse to 0.000 by end of training
- Underperformed all 9 Phase 3 top runs on both peak and stability

Actual run:
- `absurd-valley-305` (id `5osguwrs`), wandb group `v27`

Results summary:
- peak `jad_kill_rate`: 0.130 @ 1.77B (Phase 3 tops: 0.152–0.337 peak @ 2.3–3.5B)
- peak `reached_wave_63`: 0.969 (matches Phase 3 tops)
- peak `wave_reached`: 62.86 (matches Phase 3 tops)
- final `jad_kill_rate`: 0.000 (full collapse, like 5lt2u1im)
- runtime: ~30 min, cold start completed to 3.5B steps

Root-cause analysis — TL;DR:

1. **Prayer over-switching dominated the policy.** At peak (1.77B), v27 had **2,244
   prayer switches per episode** vs Phase 3 top range 242-778 (3-9× more). At peak
   intensity (0.75B), v27 hit **5,824 switches/episode** — switching on 73% of ticks.
2. **Prayer efficiency crashed**: 0.54 correct-prayers-per-switch vs Phase 3's 1.98-5.47.
   Agent was churning prayer toggles, not reading incoming hits.
3. **Mechanism**: `w_correct_danger_prayer` went from 0.05 → 0.3 (6× increase) AND the
   firing condition was extended to include melee blocks. Mid-wave melee NPCs
   (Yt-MejKot, Tz-Kih, Tz-Kek) make up most episode activity. Expected per-episode
   prayer-block reward at ~1200 blocks: **360**. Expected per-episode Jad-kill reward
   at peak ~15% conversion: **225**. Prayer-block reward dominated Jad-kill reward
   in expectation, so the agent optimized prayer-flipping.
4. **The Phase 3 sweep had told us this**: all 9 top Phase 3 runs pinned
   `w_correct_danger_prayer` at the sweep minimum (0.075). I interpreted that as
   "wants to go lower" but designed v27 with it 4× higher. The sweep was right.
5. **Everything else in v27 worked**: damage_dealt formula, Jad/danger mutex, removed
   hparams. Only the two danger-prayer reforms broke things.

What survives from v27 as valid data:
- Removing `shape_reach_wave_*_bonus`, `shape_jad_kill_bonus`, `shape_food_no_threat_penalty`,
  `shape_pot_no_threat_penalty`, `shape_jad_heal_penalty`, `shape_unnecessary_prayer_penalty`:
  confirmed these are not load-bearing
- Merging `w_attack_attempt` into `w_damage_dealt` via `(damage + hits_landed) × w`:
  confirmed this works, no pathology
- Jad/danger prayer mutex: mechanically correct, no double-counting

What v27 tells us to change next:
- Roll back `w_correct_danger_prayer` to a lower weight (v27.2 path), OR
- Amplify Jad-kill + wave-clear rewards to overpower the prayer-farming signal (v27.1 path)

Diff versus v26 (for archival — this is what v27 shipped with before we learned it broke):

**Reward weight changes:**
- `w_damage_dealt: 0.7 → 0.9`, with new `(damage + hits_landed) × w` formula — rolls up w_attack_attempt
- `w_damage_taken: -1.5 → -1.9`
- `w_wave_clear: 15.0 → 30.0`
- `w_jad_kill: 150.0 → 1500.0`
- `w_correct_jad_prayer: 2.0 → 0.3`, now Jad-only mutex with `w_correct_danger_prayer`
- `w_correct_danger_prayer: 0.05 → 0.3`, extended to include melee (proved to be the problem)
- `shape_safespot_attack_reward: 2.3 → 1.5`

**Removed:** `w_attack_attempt`, `w_jad_damage`, `shape_food_no_threat_penalty`, `shape_pot_no_threat_penalty`, `shape_npc_specific_prayer_bonus`, `shape_unnecessary_prayer_penalty`, `shape_jad_heal_penalty`, `shape_reach_wave_60_bonus`, `_61`, `_62`, `_63`, `shape_jad_kill_bonus`.

Backend: struct fields, defaults, plumbing, binding config parses, viewer key parses, overlay entries, and regression tests for removed features all excised. No stale logic. Full surface in `runescape-rl/fc-core/`, `fc-training/`, `fc-viewer/` — see commit history.

---

## v26.1 (2026-04-16, completed — `bhwunrye`)

Status:
- completed normally (but catastrophic late-run collapse)

Goal:
- full 5B run with consensus max config from sweep phase 1 (top 5 average)
- tests the "extreme positioning" approach at full scale

Source: average of sweep phase 1 runs #2-#5 (exalted-music-50, lilac-river-106,
summer-brook-66, whole-donkey-155). These 4 runs independently converged on the
same cap values (`w_wave_clear=30`, `w_player_death=-5`, `safespot=4.0`,
`melee_penalty~-2.0`). The Protein GP optimizer kept suggesting configs near
these values. v26.1 captures this consensus — the "extreme positioning"
approach vs v26's "moderate balanced" approach. See `docs/sweep_history.md`.

Diff versus v26:
- `w_wave_clear: 15.0 -> 30.0`
- `w_player_death: -11.0 -> -5.0`
- `shape_npc_melee_penalty: -0.8 -> -2.0`
- `shape_safespot_attack_reward: 2.3 -> 4.0`
- `ent_coef: 0.01 -> 0.007`

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-1.5`, `w_npc_kill=3.5`, `w_wave_clear=30.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-5.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.05`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-0.3`, `shape_npc_specific_prayer_bonus=0.3`, `shape_npc_melee_penalty=-2.0`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=2.0`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=4.0`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.007`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `bhwunrye`

Results (`bhwunrye`):
- completed normally but **catastrophic collapse after ~4B steps**
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2483.8s`
- throughput:
  - `2.03M SPS`

Final metrics (post-collapse):
- `wave_reached = 25.49`
- `max_wave = 63`
- `most_npcs_slayed = 274`
- `episode_length = 2525`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime_magic = 0.00003`
- `prayer_uptime_range = 0.233`
- `correct_prayer = 133.95`
- `no_prayer_hits = 154.21`
- `prayer_switches = 49.55`
- `damage_blocked = 1922`
- `attack_when_ready_rate = 0.846`
- `pots_used = 0.0`
- `food_eaten = 0.31`

Peak metrics (before collapse):
- peak `jad_kill_rate = 0.1250` (12.5%) at 3.311B — **highest in project history**
- peak `reached_wave_63 = 0.9303` (93%) at 3.173B
- peak `wave_reached = 62.74` at 3.173B
- 750 of 2383 sampled windows (31%) had `jad_kill_rate > 0`
- top jad windows:
  - `3.311B`: `jad_kill_rate = 0.1250`, `reached_wave_63 = 0.8534`
  - `3.303B`: `jad_kill_rate = 0.1034`, `reached_wave_63 = 0.9004`
  - `3.314B`: `jad_kill_rate = 0.1020`, `reached_wave_63 = 0.8571`
  - `3.309B`: `jad_kill_rate = 0.0935`, `reached_wave_63 = 0.8741`
  - `3.213B`: `jad_kill_rate = 0.0900`, `reached_wave_63 = 0.8850`
- nearest eval checkpoints to peak windows:
  - best sampled Jad window (~3.311B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/bhwunrye/0000003304062976.bin`
  - best sampled wave-63 access (~3.173B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/bhwunrye/0000003146776576.bin`

Key sampled progression:
- sampled `wave_reached >= 30` by `130M`
- sampled `wave_reached >= 55` by `1.040B`
- sampled `wave_reached >= 60` by `1.852B`
- peak performance window: `2.0B to 3.5B`
  - `2.0B`: wave=61.45, w63=0.696, jad=0.008
  - `2.5B`: wave=59.82, w63=0.324, jad=0.004
  - `3.0B`: wave=58.36, w63=0.343, jad=0.013
  - `3.5B`: wave=61.24, w63=0.775, jad=0.016
- **collapse begins after ~4.0B**:
  - `4.0B`: wave=58.39, w63=0.255, jad=0.0
  - `4.5B`: wave=31.17, w63=0.0, jad=0.0
  - `4.9B`: wave=22.76, w63=0.0, jad=0.0
- final: wave=25.49, w63=0.0, jad=0.0

Comparison to v26 (`zhqujlo6`):

| Metric | v26.1 peak | v26.1 final | v26 peak | v26 final |
|--------|-----------|------------|----------|----------|
| jad_kill_rate | **0.1250** | 0.0 | 0.0560 | 0.0 |
| reached_wave_63 | 0.9303 | 0.0 | 0.9261 | 0.2444 |
| wave_reached | 62.74 | **25.49** | 62.80 | **59.79** |
| Jad windows | 750 | — | 907 | — |

Config diff (v26.1 vs v26):
- `w_wave_clear: 15 -> 30` (2x wave progression)
- `w_player_death: -11 -> -5` (less death penalty)
- `shape_npc_melee_penalty: -0.8 -> -2.0` (stronger melee avoidance)
- `shape_safespot_attack_reward: 2.3 -> 4.0` (stronger safespot)
- `ent_coef: 0.01 -> 0.007` (less exploration)

Analysis — root causes of the collapse:

1. **`ent_coef = 0.007` (reduced entropy)**: The most likely primary cause.
   Lower entropy means less exploration — the policy commits harder to its
   current strategy. When the policy is good (2-3.5B), this produces stronger
   performance. But when PPO starts drifting (a normal stochastic process),
   low entropy means the policy can't recover — it commits to the degraded
   strategy and spirals downward. v26 with `ent_coef = 0.01` maintained
   enough exploration to self-correct. v26.1 couldn't.

2. **`w_player_death = -5.0` (minimal death penalty)**: With almost no death
   cost, the agent has no pressure to avoid dying. During the collapse phase,
   wave_reached dropped to 25 — the agent was dying on early waves. With
   `w_player_death = -11.0` (v26), the agent had more pressure to at least
   survive, preventing full collapse.

3. **`w_wave_clear = 30.0` (very high wave reward)**: At 30.0, the wave clear
   rewards dominate the total reward signal. When the agent starts dying
   early, it loses massive reward — but with low entropy it can't explore
   its way back to the winning strategy. The high wave_clear amplifies the
   catastrophe: being at wave 25 instead of 60 loses `(60-25) * 30 = 1050`
   reward per episode compared to before.

4. **`shape_safespot_attack_reward = 4.0` and `shape_npc_melee_penalty = -2.0`**:
   These extreme positioning signals may have made the policy too specialized
   in a narrow positioning strategy. When that strategy degraded slightly
   (normal PPO variance), the low entropy prevented recovery and the
   specialized behavior collapsed entirely.

Key insight:
- v26.1 produced the **highest peak jad_kill_rate in project history** (12.5%,
  more than 2x v26's 5.6%)
- but it was **completely unstable** — the aggressive config optimized harder
  but couldn't maintain performance
- v26's moderate config was less aggressive at peak but **stable** — it ended
  at wave 59.79 with 24.4% wave-63 access, not wave 25
- the checkpoint at 3.311B (`bhwunrye/0000003304062976.bin`) is extremely
  valuable — it represents the best Jad-killing policy ever trained in this
  project

Recommendation:
- v26 is the better base config for phase 3 sweep (stable, no collapse)
- the v26.1 checkpoint at 3.3B should be saved as a warm-start candidate
- the collapse pattern suggests `ent_coef` should not go below 0.01 for
  cold-start 5B runs

## v26 (2026-04-16, completed — `zhqujlo6`)

Status:
- completed normally

Goal:
- full 5B run with the top sweep config (dry-snowflake-141, `ojj2ywz4`)
- best balanced run from phase 1 — highest wave_reached (55.75), best melee
  avoidance, food discipline, prayer accuracy at 500M steps
- tests whether the moderate balanced approach scales to 5B

Source: sweep phase 1 run #1 dry-snowflake-141 (`ojj2ywz4`). This was the
single best-performing run in the 200-run sweep. Unlike most other top runs
which pushed params to range caps, this run used moderate values — lower
`w_wave_clear` (15 vs 30), moderate `melee_penalty` (-0.8 vs -2.0), moderate
`safespot_attack_reward` (2.3 vs 4.0). See `docs/sweep_history.md` for full
sweep details.

Source: sweep phase 1 run dry-snowflake-141 (`ojj2ywz4`)

Diff versus v25.9:
- `w_damage_taken: -0.6 -> -1.5`
- `w_wave_clear: 15.0` (unchanged)
- `w_player_death: -20.0 -> -11.0`
- `w_correct_danger_prayer: 0.25 -> 0.05`
- `shape_wrong_prayer_penalty: -1.25 -> -0.3`
- `shape_npc_specific_prayer_bonus: 1.5 -> 0.3`
- `shape_npc_melee_penalty: -1.0 -> -0.8`
- `shape_kiting_reward: 1.0 -> 2.2`
- `shape_safespot_attack_reward: 1.5 -> 2.3`
- `ent_coef: 0.01` (unchanged)

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-1.5`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-11.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.05`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-0.3`, `shape_npc_specific_prayer_bonus=0.3`, `shape_npc_melee_penalty=-0.8`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=2.2`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=2.3`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `zhqujlo6`

Results (`zhqujlo6`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2578.5s`
- throughput:
  - `1.82M SPS`

Final metrics:
- `wave_reached = 59.79`
- `max_wave = 63`
- `most_npcs_slayed = 275`
- `episode_length = 7679`
- `reached_wave_63 = 0.2444`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.0`
- `prayer_uptime_range = 0.238`
- `prayer_uptime_magic = 0.500`
- `correct_prayer = 1334.01`
- `wrong_prayer_hits = 390.57`
- `no_prayer_hits = 43.99`
- `prayer_switches = 123.11`
- `damage_blocked = 162107.5`
- `dmg_taken_avg = 7605.74`
- `attack_when_ready_rate = 0.9481`
- `pots_used = 28.67`
- `avg_prayer_on_pot = 0.4800`
- `pots_wasted = 1.13`
- `food_eaten = 19.68`
- `food_wasted = 4.93`
- `max_wave_ticks = 263.81`
- `max_wave_ticks_wave = 49.93`

Key sampled progression:
- sampled `wave_reached >= 20` by `54.5M`
- sampled `wave_reached >= 30` by `115.3M`
- sampled `wave_reached >= 35` by `524.3M`
- sampled `wave_reached >= 45` by `941.6M`
- sampled `wave_reached >= 55` by `1.357B`
- sampled `wave_reached >= 60` by `1.711B`
- Jad-kill windows appeared in 907 of 2384 sampled rows
- strongest sampled `jad_kill_rate` window:
  - around `3.440B`:
    - `wave_reached = 62.51`
    - `reached_wave_63 = 0.8664`
    - `jad_kill_rate = 0.05603`
- top sampled `jad_kill_rate` windows:
  - `3.440B`: `jad_kill_rate = 0.05603`, `reached_wave_63 = 0.8664`
  - `3.428B`: `jad_kill_rate = 0.05455`, `reached_wave_63 = 0.8364`
  - `3.426B`: `jad_kill_rate = 0.05098`, `reached_wave_63 = 0.8588`
  - `3.447B`: `jad_kill_rate = 0.04912`, `reached_wave_63 = 0.8526`
  - `3.436B`: `jad_kill_rate = 0.04878`, `reached_wave_63 = 0.8780`
- strongest sampled `reached_wave_63` / `wave_reached` window:
  - around `3.337B`:
    - `wave_reached = 62.80`
    - `reached_wave_63 = 0.9261`
    - `jad_kill_rate = 0.01739`
- nearest eval checkpoints to the important windows:
  - best sampled Jad window (~3.440B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/zhqujlo6/0000003461349376.bin`
  - best sampled wave-63 access (~3.337B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/zhqujlo6/0000003356491776.bin`

Cold-start comparison:

| Metric | v26 | v25.9 | v25.4 (warm) |
|--------|-----|-------|-------------|
| wave_reached (final) | **59.79** | 58.67 | 56.95 |
| reached_wave_63 (final) | **0.2444** | 0.0229 | 0.0397 |
| jad_kill_rate (peak) | **0.05603** | 0.00424 | 0.00130 |
| Jad windows (of ~2384) | **907** | 17 | — |
| wave >= 60 by step | 1.711B | 1.275B | — |
| attack_when_ready_rate | 0.948 | 0.934 | 0.965 |
| food_wasted | 4.93 | 5.21 | 5.51 |
| pots_wasted | **1.13** | 1.22 | 8.86 |
| prayer_switches | 123.1 | 6.8 | 2545.7 |

Analysis:
- **best run in the project's history by a massive margin**
- `jad_kill_rate` peak of 5.6% is **13x higher** than v25.9's peak (0.42%) and
  **43x higher** than v25.4's peak (0.13%)
- 907 of 2384 sampled windows (38%) had `jad_kill_rate > 0` — Jad kills were
  a sustained feature, not rare spikes
- `reached_wave_63 = 0.2444` at final — 24.4% of episodes reaching Jad
- peak `reached_wave_63 = 0.9261` — 93% of episodes reaching Jad at best window
- the sweep's "positioning first, prayer later" hypothesis was validated
- reducing prayer signal and increasing damage/positioning signal produced
  dramatically better late-game and Jad conversion
- the agent learned to pray (prayer_switches=123, magic_uptime=50%) despite
  minimal prayer reward signal — it discovered prayer from survival pressure alone
- resource management is excellent: food_wasted=4.93, pots_wasted=1.13

## v25.9.SWEEP (2026-04-15, completed — multi-run sweep; see `docs/sweep_history.md`)

Full sweep details, results, and phase 2 plan: [`docs/sweep_history.md`](sweep_history.md)

## v25.10a (2026-04-14, completed — `e46hw2zw`)

Status:
- completed normally

Goal:
- v25.9 config + prayer drain penalty only
- simpler variant of v25.10 without the obs contract change or wave-clear bonus
- isolates the prayer drain penalty as the single variable versus v25.9

Warm-start source:
- none (cold start)

Config diff versus v25.9:
- `shape_prayer_drain_penalty = -0.01` (new)
  - fires per prayer point drained per tick
  - penalizes leaving prayer on when not needed, incentivizes prayer flicking
- observation contract unchanged: `policy_obs=106`, `puffer_obs=142`
- all other config, code, and backend identical to v25.9

Backend changes:
- `fc_types.h`: added `prayer_drained_this_tick`
- `fc_tick.c`: track prayer drain by comparing `current_prayer` before/after
  `fc_prayer_drain_tick`; clear each tick
- `fc_reward.h`: added `shape_prayer_drain_penalty` param and `prayer_drain`
  breakdown field; fires as `penalty * (prayer_drained_this_tick / 10)` per
  prayer point drained
- `fight_caves.h`, `fight_caves.c`, `binding.c`: plumbed new param

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-20.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-1.0`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=1.5`, `shape_prayer_drain_penalty=-0.01`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `e46hw2zw`

Results (`e46hw2zw`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2574.4s`
- throughput:
  - `1.81M SPS`

Final metrics:
- `wave_reached = 57.67`
- `max_wave = 63`
- `most_npcs_slayed = 273`
- `episode_length = 7300`
- `reached_wave_63 = 0.0392`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.004`
- `prayer_uptime_range = 0.362`
- `prayer_uptime_magic = 0.408`
- `correct_prayer = 1310.71`
- `wrong_prayer_hits = 272.95`
- `no_prayer_hits = 40.31`
- `prayer_switches = 102.97`
- `damage_blocked = 127533.8`
- `dmg_taken_avg = 5080.70`
- `attack_when_ready_rate = 0.9246`
- `pots_used = 29.46`
- `avg_prayer_on_pot = 0.3421`
- `pots_wasted = 1.78`
- `food_eaten = 20.0`
- `food_wasted = 16.72`
- `max_wave_ticks = 265.87`
- `max_wave_ticks_wave = 46.03`

Key sampled progression:
- sampled `wave_reached >= 20` by `56.6M`
- sampled `wave_reached >= 25` by `136.3M`
- sampled `wave_reached >= 30` by `268.4M`
- sampled `wave_reached >= 35` by `1.380B`
- sampled `wave_reached >= 40` by `1.409B`
- sampled `wave_reached >= 45` by `1.451B`
- sampled `wave_reached >= 50` by `1.640B`
- sampled `wave_reached >= 55` by `3.980B`
- never reached `wave_reached >= 60`
- Jad-kill windows appeared in 1 of 2384 sampled rows
- strongest sampled `jad_kill_rate` window:
  - around `4.300B`:
    - `wave_reached = 54.33`
    - `reached_wave_63 = 0.0243`
    - `jad_kill_rate = 0.00304`
- strongest sampled `reached_wave_63` window:
  - around `4.835B`:
    - `reached_wave_63 = 0.0871`
    - `wave_reached = 56.94`
    - `jad_kill_rate = 0.0`
- strongest sampled `wave_reached` window:
  - around `4.989B`:
    - `wave_reached = 58.27`
    - `reached_wave_63 = 0.0441`
    - `jad_kill_rate = 0.0`
- nearest eval checkpoints to the important windows:
  - best sampled Jad window (~4.300B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/e46hw2zw/0000004300210176.bin`
  - best sampled wave-63 access (~4.835B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/e46hw2zw/0000004824498176.bin`
  - best sampled wave_reached (~4.989B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/e46hw2zw/0000004981784576.bin`

Comparison to v25.9 (`8td831nt`, same config minus prayer drain penalty):
- v25.10a regressed from v25.9:
  - `wave_reached: 57.67 vs 58.67`
  - `reached_wave_63: 0.0392 vs 0.0229` (final, v25.10a slightly better at end)
  - `jad_kill_rate: 0.00304 (peak) vs 0.00424 (peak)`
- progression was much slower:
  - wave >= 35 by 1.380B (v25.9: 378M) — stalled at wave 30-35 for ~1.1B steps
  - wave >= 55 by 3.980B (v25.9: 751M) — 5.3x slower
  - never reached wave >= 60 average (v25.9: 1.275B)
- prayer behavior:
  - `prayer_switches: 102.97 vs 6.80` — more switching than v25.9
  - `prayer_uptime_magic: 0.408 vs 0.521` — less magic camping
  - `prayer_uptime_range: 0.362 vs 0.291` — more range prayer
  - `prayer_uptime_melee: 0.004 vs 0.0` — minimal melee prayer
  - `correct_prayer: 1310.7 vs 1370.9` — slightly fewer correct blocks
  - `wrong_prayer_hits: 272.9 vs 404.3` — fewer wrong prayer hits
- resource management:
  - `food_eaten: 20.0 vs 18.88` — eating all sharks
  - `food_wasted: 16.72 vs 5.21` — massive food waste (eating at high HP)
  - `pots_wasted: 1.78 vs 1.22` — similar
  - `avg_hp_on_food: 0.898 vs 0.612` — eating at 90% HP (extreme waste)
  - `avg_prayer_on_pot: 0.342 vs 0.389` — drinking at lower prayer (better)
- `attack_when_ready_rate: 0.925 vs 0.934` — slightly less attacking

Comparison to v25.10 (`z5oie92l`, prayer drain + wave bonus + new obs):
- v25.10a performed better than v25.10:
  - `wave_reached: 57.67 vs 50.22`
  - `reached_wave_63: 0.0392 vs 0.0 (final)`
  - both stalled at wave 30-35 for ~1B steps
  - v25.10a eventually reached wave 57+ while v25.10 plateaued at 50
- confirms the obs contract change and wave-clear bonus in v25.10 were the
  bigger negative factor, not the prayer drain penalty alone

Cold-start comparison:

| Metric | v25.10a | v25.10 | v25.9 | v25.8 | v21.2 |
|--------|---------|--------|-------|-------|-------|
| wave_reached (final) | 57.67 | 50.22 | 58.67 | 29.71 | 48.65 |
| wave >= 35 by step | 1.380B | 1.307B | 378M | never | 457M |
| wave >= 55 by step | 3.980B | 1.678B | 751M | never | 870M |
| jad_kill_rate (peak) | 0.00304 | 0.00365 | 0.00424 | 0.0 | 0.0 |
| prayer_switches | 102.97 | 372.43 | 6.80 | 1702.4 | 2284.7 |
| food_wasted | 16.72 | 7.95 | 5.21 | 1.64 | 4.33 |

Analysis:
- the prayer drain penalty alone (-0.01) caused a significant learning
  slowdown: the agent stalled at wave 30-35 for over 1B steps
- the same wave 30-35 stall appeared in both v25.10 and v25.10a, confirming
  the prayer drain penalty is the common factor, not the obs/wave-clear changes
- the prayer drain penalty did produce more prayer switching (103/ep vs 6.8)
  and less magic camping, but at the cost of much slower progression
- food waste is extreme (16.72 wasted, eating at 90% HP) — the agent appears
  to be compensating for prayer drain by eating food preemptively
- the agent eventually reached similar final wave (57.67 vs 58.67) but took
  3.5x longer to get past wave 55, and its best windows were much later
  (4.3-5.0B vs 1.8B)
- v25.9 without the prayer drain penalty remains the strongest cold-start run

Recommendation:
- the prayer drain penalty at -0.01 is too disruptive for cold-start learning
- it creates a secondary optimization target (minimize prayer drain) that
  competes with the primary one (survive and progress)
- the agent's response — camping food at 90% HP — suggests it's trying to
  compensate for the prayer cost by avoiding damage through food rather than
  through correct prayer
- options for future runs:
  1. drop prayer drain penalty entirely (return to v25.9 config)
  2. reduce to -0.001 or -0.002 (much smaller signal)
  3. warm-start from v25.9's best checkpoint with a small prayer drain penalty
     so the agent already knows how to survive before learning to conserve

## v25.10 (2026-04-14, completed — id not recorded)

Status:
- completed normally

Goal:
- v25.9 config + prayer drain penalty + no-damage wave clear bonus + new obs
- teach the agent to conserve prayer by penalizing prayer drain
- reward clean wave clears where no damage is taken (correct prayer + positioning)
- add binary obs for whether damage has been taken this wave

Warm-start source:
- none (cold start)

Config diff versus v25.9:
- `shape_prayer_drain_penalty = -0.01` (new)
  - fires per prayer point drained per tick
  - penalizes leaving prayer on when not needed, incentivizes prayer flicking
- `shape_no_damage_wave_clear_bonus = 2.0` (new)
  - fires on wave clear if no damage was taken that wave
  - blocked attacks (correct prayer) count as zero damage
  - rewards perfect prayer switching + safe positioning

Observation contract change:
- `FC_OBS_META_SIZE: 9 -> 10`
- `FC_POLICY_OBS_SIZE: 106 -> 107`
- `FC_PUFFER_OBS_SIZE: 142 -> 143`
- added `FC_OBS_META_DMG_WAVE` (meta index 9): binary flag, 1.0 if any damage
  taken this wave, 0.0 if clean so far
- flag sets to 1 on first damage tick, resets to 0 on wave advance and episode
  reset (`memset` zeros it)
- breaks checkpoint compatibility (cold start, no issue)

Backend changes:
- `fc_types.h`: added `prayer_drained_this_tick` (int, prayer points in tenths
  drained this tick), `took_damage_this_wave` (int, binary flag set to 1 on
  first damage)
- `fc_tick.c`:
  - `prayer_drained_this_tick` cleared to 0 each tick in `clear_per_tick_flags`
  - prayer drain tracked by saving `current_prayer` before
    `fc_prayer_drain_tick`, computing delta after; if positive, stored in
    `prayer_drained_this_tick`
  - after hit resolution: if `damage_taken_this_tick > 0`, sets
    `took_damage_this_wave = 1`
- `fc_wave.c`: reset `took_damage_this_wave = 0` on wave advance
- episode reset via `memset` zeroes both fields automatically
- `fc_contracts.h`: added `FC_OBS_META_DMG_WAVE` (index 9), bumped
  `FC_OBS_META_SIZE` 9→10
- `fc_state.c`: writes `took_damage_this_wave` as float to
  `FC_OBS_META_DMG_WAVE`
- `fc_reward.h`:
  - `shape_prayer_drain_penalty`: fires as
    `penalty * (prayer_drained_this_tick / 10)` — per prayer point drained.
    integer division means drain < 10 tenths (< 1 point) produces 0 penalty
    that tick
  - `shape_no_damage_wave_clear_bonus`: fires on wave clear tick if
    `took_damage_this_wave == 0`
  - both added to `FcRewardBreakdown` and total sum
- `fight_caves.h`, `fight_caves.c`, `binding.c`: plumbed new params

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model, `policy_obs=107`, `puffer_obs=143`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-20.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-1.0`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=1.5`, `shape_prayer_drain_penalty=-0.01`, `shape_no_damage_wave_clear_bonus=2.0`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `z5oie92l`

Results (`z5oie92l`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2639.9s`
- throughput:
  - `1.89M SPS`

Final metrics:
- `wave_reached = 50.22`
- `max_wave = 63`
- `most_npcs_slayed = 273`
- `episode_length = 6028`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.047`
- `prayer_uptime_range = 0.291`
- `prayer_uptime_magic = 0.402`
- `correct_prayer = 1008.43`
- `wrong_prayer_hits = 347.54`
- `no_prayer_hits = 36.48`
- `prayer_switches = 372.43`
- `damage_blocked = 99484.4`
- `dmg_taken_avg = 6372.32`
- `attack_when_ready_rate = 0.8164`
- `pots_used = 31.53`
- `avg_prayer_on_pot = 0.7634`
- `pots_wasted = 23.77`
- `food_eaten = 19.78`
- `food_wasted = 7.95`
- `max_wave_ticks = 246.06`
- `max_wave_ticks_wave = 33.25`

Key sampled progression:
- sampled `wave_reached >= 20` by `50.3M`
- sampled `wave_reached >= 25` by `134.2M`
- sampled `wave_reached >= 30` by `199.2M`
- sampled `wave_reached >= 35` by `1.307B`
- sampled `wave_reached >= 40` by `1.388B`
- sampled `wave_reached >= 45` by `1.470B`
- sampled `wave_reached >= 50` by `1.531B`
- sampled `wave_reached >= 55` by `1.678B`
- never reached `wave_reached >= 60`
- Jad-kill windows appeared in 4 of 2408 sampled rows
- strongest sampled `jad_kill_rate` window:
  - around `3.800B`:
    - `wave_reached = 58.98`
    - `reached_wave_63 = 0.0365`
    - `jad_kill_rate = 0.00365`
- top sampled `jad_kill_rate` windows:
  - `3.800B`: `jad_kill_rate = 0.00365`, `reached_wave_63 = 0.0365`
  - `3.704B`: `jad_kill_rate = 0.00361`, `reached_wave_63 = 0.0542`
  - `3.689B`: `jad_kill_rate = 0.00334`, `reached_wave_63 = 0.0435`
  - `3.687B`: `jad_kill_rate = 0.00327`, `reached_wave_63 = 0.0621`
- strongest sampled `reached_wave_63` window:
  - around `3.630B`:
    - `reached_wave_63 = 0.0662`
    - `wave_reached = 58.93`
    - `jad_kill_rate = 0.0`
- strongest sampled `wave_reached` window:
  - around `3.689B`:
    - `wave_reached = 59.60`
    - `reached_wave_63 = 0.0435`
    - `jad_kill_rate = 0.00334`
- nearest eval checkpoints to the important windows:
  - best sampled Jad window (~3.800B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/z5oie92l/0000003828350976.bin`
  - best sampled wave-63 access (~3.630B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/z5oie92l/0000003618635776.bin`
  - best sampled wave_reached (~3.689B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/z5oie92l/0000003671064576.bin`

Comparison to v25.9 (`8td831nt`, same config minus prayer drain/wave bonus/obs):
- v25.10 regressed significantly from v25.9:
  - `wave_reached: 50.22 vs 58.67`
  - `reached_wave_63: 0.0 (final) vs 0.0229`
  - `jad_kill_rate: 0.00365 (peak) vs 0.00424 (peak)`
- progression was much slower:
  - wave >= 30 by 199M (similar to v25.9's 256M)
  - but wave >= 35 didn't happen until 1.307B (v25.9: 378M)
  - wave >= 55 by 1.678B (v25.9: 751M)
  - never reached wave >= 60 average (v25.9: 1.275B)
- the agent stalled in the 30-35 range for ~1B steps before breaking through
- prayer behavior changed:
  - `prayer_switches: 372.4 vs 6.8` — much more switching than v25.9
  - `prayer_uptime_magic: 0.402 vs 0.521` — less magic camping
  - `prayer_uptime_melee: 0.047 vs 0.0` — some melee prayer learned
  - but `correct_prayer: 1008.4 vs 1370.9` — fewer correct blocks overall
- resource management worse:
  - `food_eaten: 19.78 vs 18.88`
  - `food_wasted: 7.95 vs 5.21`
  - `pots_wasted: 23.77 vs 1.22` — massive potion waste
  - `dmg_taken_avg: 6372.3 vs 7660.1` — less damage but dying earlier (lower wave)
- melee exposure much higher:
  - `ketzek_melee_ticks: 20.23 vs 8.39`
  - `tokxil_melee_ticks: 10.17 vs 4.55`
- `attack_when_ready_rate: 0.816 vs 0.934` — attacking less frequently

Analysis:
- the `shape_no_damage_wave_clear_bonus` and obs contract change appear to have
  hurt cold-start learning rather than helped
- the agent stalled at wave 30-35 for over 1B steps — the same Ket-Zek barrier
  seen in v25.8, though it eventually broke through
- the additional obs dimension (107 vs 106) and the no-damage bonus may have
  added noise to the early learning signal
- the prayer drain penalty's effect is hard to isolate from the wave-clear
  bonus since both were added simultaneously
- the agent did learn some prayer switching (372/ep vs v25.9's 6.8) and some
  melee prayer (4.7% uptime), but the overall result was weaker than v25.9
- `pots_wasted = 23.77` is the highest waste in any v25+ run, suggesting the
  prayer drain penalty may have caused the agent to drink potions excessively
  to counteract the drain cost
- the late Jad windows (3.6-3.8B) suggest the agent was still improving but
  needed more steps — v25.9's best windows were at 1.8B

Recommendation:
- the full v25.10 package (prayer drain + wave bonus + new obs) is a net
  regression from v25.9
- v25.10a (prayer drain only, no obs change, no wave bonus) will isolate
  whether the prayer drain penalty alone helps or hurts
- if v25.10a also regresses, the prayer drain penalty itself may be too
  disruptive for cold-start learning at -0.01

## v25.9 (2026-04-14, completed — `8td831nt`)

Status:
- completed normally

Goal:
- cold-start run with safespot/positioning reward tuning
- address the v25.8 cold-start failure where the agent never learned to
  safespot or avoid melee contact, burning through food and stalling at wave 31
- widen kiting band to reward all non-adjacent attack positions (dist 2-10)
- increase melee pressure penalty to strongly discourage any NPC adjacency
- add new safespot attack reward for attacking with no NPC adjacent

Warm-start source:
- none (cold start)

Config diff versus v25.8:
- `shape_kiting_min_dist: 7 -> 2`
  - was: only rewarded attacking from 7-10 tiles (missed safespot range 2-6)
  - now: rewards attacking from any non-adjacent position (2-10 tiles)
- `shape_npc_melee_penalty: -0.3 -> -1.0`
  - was: -0.3 per adjacent NPC per tick (weak relative to +0.7 attack reward)
  - now: -1.0 per adjacent NPC per tick (strongly discourages melee contact)
  - fires on proximity, not on hits — all melee contact penalized regardless
    of prayer, block, or zero-damage
- `shape_safespot_attack_reward: new, 1.5`
  - fires when player attacks and no NPC is within distance <= 1 (adjacent)
  - rewards both safespotting (behind terrain) and kiting (at range)
  - complements the existing `shape_kiting_reward=1.0` which only fires
    within the kiting band

All unchanged from v25.8:
- all reward weights (`w_damage_dealt=0.7`, `w_attack_attempt=0.2`, etc.)
- all other shaping values
- all PPO/runtime params
- cold start (no warm-start)
- combat model (Masori (f) + TBow, range 10)
- observation contract (`policy_obs=106`, `puffer_obs=142`)
- all backend mechanics (LOS, healer aggro, Jad instant terminal, ready-idle
  prayer gate)

Backend changes:
- `fc_types.h`: added `safespot_attack_this_tick` flag to FcState
- `fc_tick.c`: clear flag each tick; after player fires attack, scan all active
  NPCs — if none are within distance <= 1, set flag to 1
- `fc_reward.h`: added `shape_safespot_attack_reward` to FcRewardParams,
  `safespot_attack` to FcRewardBreakdown, computation fires on flag, added to
  reward total
- `fight_caves.h`: added field to FightCaves struct and params mapping
- `fight_caves.c`: added default init
- `binding.c`: added config parsing for `shape_safespot_attack_reward`

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-20.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-1.0`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=2`, `shape_kiting_max_dist=10`, `shape_safespot_attack_reward=1.5`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `8td831nt`

Results (`8td831nt`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2576.6s`
- throughput:
  - `1.84M SPS`

Final metrics:
- `wave_reached = 58.67`
- `max_wave = 63`
- `most_npcs_slayed = 273`
- `episode_length = 7469`
- `reached_wave_63 = 0.0229`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.0`
- `prayer_uptime_range = 0.291`
- `prayer_uptime_magic = 0.521`
- `correct_prayer = 1370.85`
- `wrong_prayer_hits = 404.27`
- `no_prayer_hits = 31.71`
- `prayer_switches = 6.80`
- `damage_blocked = 167495.9`
- `dmg_taken_avg = 7660.07`
- `attack_when_ready_rate = 0.9337`
- `pots_used = 30.02`
- `avg_prayer_on_pot = 0.3886`
- `pots_wasted = 1.22`
- `food_eaten = 18.88`
- `food_wasted = 5.21`
- `max_wave_ticks = 267.34`
- `max_wave_ticks_wave = 47.34`

Key sampled progression:
- sampled `wave_reached >= 20` by `58.7M`
- sampled `wave_reached >= 25` by `178.3M`
- sampled `wave_reached >= 30` by `255.9M`
- sampled `wave_reached >= 35` by `377.5M`
- sampled `wave_reached >= 40` by `408.9M`
- sampled `wave_reached >= 45` by `463.5M`
- sampled `wave_reached >= 50` by `484.4M`
- sampled `wave_reached >= 55` by `750.8M`
- sampled `wave_reached >= 60` by `1.275B`
- Jad-kill windows appeared in 17 of 2384 sampled rows
- strongest sampled `jad_kill_rate` window:
  - around `1.827B`:
    - `wave_reached = 59.66`
    - `reached_wave_63 = 0.1059`
    - `jad_kill_rate = 0.00424`
- top sampled `jad_kill_rate` windows:
  - `1.827B`: `jad_kill_rate = 0.00424`, `reached_wave_63 = 0.1059`
  - `1.892B`: `jad_kill_rate = 0.00424`, `reached_wave_63 = 0.1314`
  - `1.883B`: `jad_kill_rate = 0.00415`, `reached_wave_63 = 0.0747`
  - `1.887B`: `jad_kill_rate = 0.00407`, `reached_wave_63 = 0.1179`
  - `1.917B`: `jad_kill_rate = 0.00394`, `reached_wave_63 = 0.0906`
- strongest sampled `reached_wave_63` window:
  - around `1.892B`:
    - `reached_wave_63 = 0.1314`
    - `wave_reached = 59.59`
    - `jad_kill_rate = 0.00424`
- strongest sampled `wave_reached` window:
  - around `4.726B`:
    - `wave_reached = 60.47`
    - `reached_wave_63 = 0.0551`
    - `jad_kill_rate = 0.0`
- nearest eval checkpoints to the important windows:
  - best sampled Jad window (~1.827B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/8td831nt/0000001836056576.bin`
  - best sampled `reached_wave_63` + Jad window (~1.892B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/8td831nt/0000001888485376.bin`
  - best sampled `wave_reached` (~4.726B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/8td831nt/0000004719640576.bin`

Cold-start comparison table:

| Metric | v26 | v25.9 | v25.8 | v21.2 |
|--------|-----|-------|-------|-------|
| wave_reached (final) | **59.79** | 58.67 | 29.71 | 48.65 |
| max_wave | 63 | 63 | 34 | 63 |
| wave >= 30 by step | 115M | 256M | 480M | 220M |
| wave >= 45 by step | 942M | 464M | never | 612M |
| wave >= 55 by step | 1.357B | 751M | never | 870M |
| wave >= 60 by step | 1.711B | 1.275B | never | 969M |
| reached_wave_63 (final) | **0.2444** | 0.0229 | 0.0 | 0.0067 |
| jad_kill_rate (peak) | **0.05603** | 0.00424 | 0.0 | 0.0 |
| prayer_uptime_magic | 0.500 | 0.521 | 0.00003 | 0.123 |
| prayer_switches | 123.1 | 6.80 | 1702.4 | 2284.7 |
| attack_when_ready_rate | 0.948 | 0.934 | 0.943 | 0.633 |
| pots_used | 28.67 | 30.02 | 10.68 | 0.02 |

Note: v26 is the sweep-optimized successor to v25.9. See `sweep_history.md`
for full sweep details. v26 achieved 13x higher peak jad_kill_rate than v25.9
and 10x higher reached_wave_63 at final.
| food_eaten | 18.88 | 3.58 | 15.89 |

Comparison to v25.8 (`6uem6mf8`, cold start, same backend, old positioning rewards):
- v25.8 stalled at wave 30 and never progressed; v25.9 reached wave 60+
- the safespot/positioning changes completely solved the cold-start wave-31 barrier
- v25.9 learned magic prayer (52.1% uptime) while v25.8 never did (0.003%)
- v25.9 learned to use potions (30.02 used) while v25.8 barely did (10.68)
- prayer switching behavior is radically different: v25.9 switches only 6.8
  times per episode vs v25.8's 1702 — v25.9 found a "camp one prayer" strategy
  rather than active switching

Comparison to v21.2 (`u58coupx`, cold start, range-7, no LOS):
- v25.9 reached higher final wave (58.67 vs 48.65) and higher reached_wave_63
  (0.0229 vs 0.0067)
- v25.9 is the first cold start under the TBow combat model to reach wave 60+
- v25.9 reached wave 60 later than v21.2 (1.275B vs 969M) but progressed
  further overall
- v25.9 produced actual Jad kills (0.424% peak) while v21.2 never did
- v25.9 uses magic prayer heavily (52.1%) while v21.2 used it lightly (12.3%)
- v25.9 barely switches prayer (6.8/ep) — appears to camp magic prayer
  rather than actively switch like v21.2 (2285/ep)
- food usage is higher in v25.9 (18.88 vs 15.89) and waste is higher (5.21 vs
  4.33), suggesting the agent compensates for wrong-prayer melee damage with
  food rather than prayer switching
- wrong_prayer_hits is high (404.3) — the agent is taking many unblocked melee
  hits because it camps magic prayer

Analysis:
- the safespot/positioning reward changes are a decisive success for cold-start
  viability under the TBow + LOS backend
- the agent learned a viable but suboptimal strategy: camp magic prayer,
  safespot melee NPCs, eat through melee damage when safespotting fails
- the low prayer_switches (6.8) and zero melee prayer uptime indicate the agent
  found it more efficient to tank melee hits + eat than to actively switch
  prayers — likely because `shape_npc_melee_penalty=-1.0` penalizes adjacency
  but the prayer gate suppresses prayer rewards while idle, so switching to
  melee prayer doesn't offset the penalty
- the best Jad windows clustered around 1.8-1.9B steps, then Jad activity
  decayed — same PPO forgetting pattern as prior runs
- this is the strongest cold-start result in the project's history:
  first cold start to reach wave 60+ under TBow, first to produce Jad kills

## v25.6 (2026-04-13, completed — `yra7umqg`)

Goal:
- exact v25.4 recipe with no milestone bonuses
- warm-start from the best v25.4 frontier checkpoint (`7qhjnxa2/0000000368050176.bin`)
  rather than the strongest sampled window checkpoint used in v25.5/v25.5a
- test whether the earlier 368M checkpoint (peak Jad engagement window) is a
  better warm-start than the 1.888B checkpoint used by v25.5/v25.5a

Warm-start source:
- `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000000368050176.bin`

Why this checkpoint:
- `7qhjnxa2/0000000368050176.bin` was identified as the best practical
  checkpoint candidate from v25.4
- it sits near the strongest sampled Jad-kill window in the run
- v25.5 and v25.5a both used the later 1.888B checkpoint, which had stronger
  average-wave stability but weaker Jad conversion

Backend changes:
- Jad death now immediately triggers `TERMINAL_CAVE_COMPLETE` and despawns
  surviving Yt-HurKot healers on the same tick (matches real OSRS behavior)
- `w_cave_complete` merged into `w_jad_kill` — single reward of `150.0` fires
  on Jad-death tick instead of separate `w_jad_kill=50 + w_cave_complete=100`
- net reward value on Jad kill is unchanged: `150.0` (was `50 + 100`)

Diff versus v25.5a:
- `w_jad_kill: 50.0 -> 150.0` (absorbs old `w_cave_complete=100.0`)
- `w_cave_complete` removed (Jad kill = cave complete now)
- `shape_reach_wave_63_bonus: 100.0 -> 0.0`
- `shape_jad_kill_bonus: 500.0 -> 0.0`
- warm-start: `7qhjnxa2/0000001888485376.bin -> 7qhjnxa2/0000000368050176.bin`
- backend: Jad death despawns healers and sets terminal immediately
- analytics: added `max_wave_ticks` and `max_wave_ticks_wave` (longest wave per episode)
- analytics: removed `reached_wave_30`

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000000368050176.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-20.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `yra7umqg`

Results (`yra7umqg`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2572.7s`
- throughput:
  - `1.86M SPS`

Final metrics:
- `wave_reached = 57.31`
- `max_wave = 63`
- `most_npcs_slayed = 274`
- `episode_length = 7234`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.255`
- `prayer_uptime_range = 0.272`
- `prayer_uptime_magic = 0.218`
- `correct_prayer = 2283.87`
- `wrong_prayer_hits = 256.61`
- `no_prayer_hits = 19.40`
- `prayer_switches = 2666.21`
- `damage_blocked = 159081.2`
- `dmg_taken_avg = 5518.79`
- `attack_when_ready_rate = 0.9437`
- `pots_used = 31.33`
- `avg_prayer_on_pot = 0.6540`
- `pots_wasted = 15.88`
- `food_eaten = 11.95`
- `food_wasted = 3.86`
- `max_wave_ticks = 283.80`
- `max_wave_ticks_wave = 47.74`

Key sampled progression:
- sampled `wave_reached >= 30` by `14.7M`
- sampled `wave_reached >= 55` by `29.4M`
- sampled `wave_reached >= 60` by `33.6M`
- strongest sampled `jad_kill_rate` window:
  - around `107M`:
    - `wave_reached = 61.10`
    - `reached_wave_63 = 0.8055`
    - `jad_kill_rate = 0.03288`
- strongest sampled `reached_wave_63` / `wave_reached` window:
  - around `793M`:
    - `wave_reached = 62.83`
    - `reached_wave_63 = 0.9490`
    - `jad_kill_rate = 0.0`
- nearest eval checkpoints to the important windows:
  - best sampled Jad window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yra7umqg/0000000105906176.bin`
  - best sampled wave-63 access window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yra7umqg/0000000787480576.bin`

Comparison to `v25.4` (`7qhjnxa2`):
- final late-game profile was similar:
  - `wave_reached: 57.31 vs 56.95`
  - `correct_prayer: 2283.9 vs 2288.1`
  - `attack_when_ready_rate: 0.9437 vs 0.9653`
- peak Jad windows were weaker:
  - `jad_kill_rate: 0.03288 peak vs 0.001301 peak` — v25.6 peaked higher but
    earlier (107M) and the window was likely a small-sample spike
  - `reached_wave_63: 0.9490 peak vs 0.4453 peak` — v25.6 had better late-wave
    access
- final `reached_wave_63` collapsed to `0.0` by end of run (v25.4 ended at `0.0397`)
- the same early-peak-then-decay pattern seen in prior runs

Comparison to `v25.6a` (`pbracd6x`, stopped at 787M, old backend):
- v25.6a had stronger sustained Jad activity through its shorter run
- v25.6a peak `jad_kill_rate = 0.2` at 42M (small sample) vs v25.6 peak `0.033` at 107M
- v25.6a had milestone bonuses active (`shape_reach_wave_63_bonus=100`, `shape_jad_kill_bonus=500`)
  which v25.6 did not

Analysis:
- the backend changes (instant Jad terminal, merged reward) did not produce
  meaningfully different results from v25.4
- the early Jad peak at 107M then decay is consistent with the PPO
  forgetting pattern seen across all runs in this line
- v25.6a's stronger Jad activity may be attributable to the milestone bonuses
  rather than the checkpoint, but the checkpoint came from v25.6a's best window

## v25.8 (2026-04-14, completed — `6uem6mf8`)

Status:
- completed normally

Goal:
- cold-start run with the exact v25.6/v25.7 config and backend
- no warm-start — test how the current reward surface and backend learn
  from scratch without any checkpoint lineage bias
- this is effectively the v26 cold-start concept applied to the v25.6 backend
  (instant Jad terminal, merged `w_jad_kill=150`)

Warm-start source:
- none (cold start)

Diff versus v25.7:
- `load_model_path`: removed (cold start)
- all other config, code, and backend identical to v25.7

Cumulative diff versus v21.2 (the last successful cold start):
- combat model: generic ranged (range 7) → Twisted Bow (range 10) with magic scaling
- player stats: corrected def_stab, def_slash, def_magic, def_ranged, prayer_bonus
- `w_damage_dealt`: 0.5 → 0.7
- `w_attack_attempt`: 0.1 → 0.2
- `w_npc_kill`: 3.0 → 3.5
- `w_wave_clear`: 10.0 → 15.0
- `w_jad_kill`: 50.0 → 150.0 (merged with old `w_cave_complete=100.0`)
- `w_cave_complete`: 100.0 → removed
- `w_correct_jad_prayer`: 0.0 → 2.0
- `shape_kiting_min_dist`: 5 → 7
- `shape_kiting_max_dist`: 7 → 10
- `shape_jad_heal_penalty`: 0.0 → -0.1
- `shape_low_prayer_no_pot_penalty`: -0.01 → removed
- `shape_low_prayer_pot_reward`: 0.15 → removed
- backend: player ranged attacks require LOS (v25)
- backend: auto-approach seeks tiles with range + LOS (v25)
- backend: Yt-HurKot distraction on any hit including 0s (v25)
- backend: ready-idle prayer gate suppresses prayer rewards while idle (v25.1)
- backend: Jad death immediately ends episode, despawns healers (v25.6)

Exact active config:
- run setup: cold start, no `load_model_path`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-20.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `6uem6mf8`

Results (`6uem6mf8`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2430.4s`
- throughput:
  - `2.06M SPS`

Final metrics:
- `wave_reached = 29.71`
- `max_wave = 34`
- `most_npcs_slayed = 128`
- `episode_length = 3639`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.333`
- `prayer_uptime_range = 0.333`
- `prayer_uptime_magic = 0.00003`
- `correct_prayer = 1396.77`
- `wrong_prayer_hits = 135.25`
- `no_prayer_hits = 33.25`
- `prayer_switches = 1702.37`
- `damage_blocked = 23436.07`
- `dmg_taken_avg = 2992.33`
- `attack_when_ready_rate = 0.9428`
- `pots_used = 10.68`
- `avg_prayer_on_pot = 0.4849`
- `pots_wasted = 1.40`
- `food_eaten = 3.58`
- `food_wasted = 1.64`
- `max_wave_ticks = 427.89`
- `max_wave_ticks_wave = 27.31`

Key sampled progression:
- sampled `wave_reached >= 20` by `140.5M`
- sampled `wave_reached >= 25` by `329.3M`
- sampled `wave_reached >= 30` by `480.2M`
- never reached `wave_reached >= 35`
- peak `wave_reached = 31.77` at `610M`
- zero `jad_kill_rate` throughout the entire run
- zero `reached_wave_63` throughout the entire run

Cold-start comparison table:

| Metric | v25.8 | v22 | v21.2 |
|--------|-------|-----|-------|
| combat model | TBow (range 10) | TBow (range 10) | generic (range 7) |
| LOS enforced | yes | no | no |
| ready-idle prayer gate | yes | no | no |
| wave_reached (final) | 29.71 | 27.15 | 48.65 |
| max_wave | 34 | 33 | 63 |
| wave >= 30 by step | 480M | 853M | 220M |
| wave >= 35 by step | never | never | 457M |
| wave >= 60 by step | never | never | 969M |
| reached_wave_63 | 0.0 | 0.0 | 0.0067 |
| jad_kill_rate | 0.0 | 0.0 | 0.0 |
| prayer_uptime_magic | 0.00003 | 0.0 | 0.123 |
| attack_when_ready_rate | 0.943 | 0.456 | 0.633 |
| pots_used | 10.68 | 31.96 | 0.02 |

Comparison to v21.2 (`u58coupx`, cold start, range-7, no LOS):
- v25.8 stalled completely at wave 30-31 and never progressed
- v21.2 broke through wave 30 by 220M and reached wave 60 by 969M
- the wave 30-31 barrier corresponds to Ket-Zek (mage) first appearing
- v25.8 shows zero magic prayer uptime (0.00003) — the agent never learned
  to use protect-from-magic, which is required to survive Ket-Zek
- v21.2 developed 12.3% magic prayer uptime and learned to switch prayers
- v25.8 has high attack_when_ready_rate (0.943) but cannot progress because
  it cannot survive the magic damage from Ket-Zek
- the `max_wave_ticks_wave = 27.3` confirms the agent is dying on the wave
  27-28 range where it first encounters harder multi-NPC compositions

Comparison to v22 (`7vuw9jy8`, cold start, TBow range-10, no LOS):
- v22 also stalled in the low 30s (peak 30.01, final 27.15)
- v22 was a cold start with TBow but WITHOUT LOS enforcement
- v22 had the per-NPC `npc_type` observation which was later removed
- both v22 and v25.8 failed to learn magic prayer switching from cold start
  under the TBow combat model
- this suggests the TBow combat model itself (not just LOS) is a significant
  factor in cold-start difficulty

Analysis:
- v25.8 is a clear cold-start failure under the current config + backend
- the agent learned to attack efficiently (0.943 ready rate) and use melee/range
  prayer switching, but completely failed to learn magic prayer
- without magic prayer, Ket-Zek kills the agent every time it appears (wave 31+)
- the comparison to v22 is critical: v22 also cold-started with TBow and also
  failed at the same barrier, even WITHOUT LOS enforcement
- this strongly suggests the TBow combat model (range 10 + magic scaling) is
  the primary cold-start blocker, not LOS alone
- v21.2 succeeded because range 7 kept the agent closer to NPCs, which may
  have made prayer switching more learnable (more immediate threat feedback)
- the ready-idle prayer gate (v25.1) may also contribute by suppressing the
  dense prayer reward signal that helped v21.2 bootstrap prayer behavior

Recommendation:
- the current v25.6+ config cannot cold-start successfully
- two approaches to fix this:
  1. revert combat model to range 7 for cold start, then warm-start into TBow
  2. keep TBow but restore dense prayer reward signal (remove ready-idle gate,
     restore low-prayer shaping) and possibly add magic-prayer-specific shaping
- the ready-idle prayer gate removal is the lowest-risk change to try first
- if that's insufficient, restoring the v21.2 kiting band (5/7) and combat
  weights (0.5/0.1/3.0/10.0) would bring the cold-start surface closer to
  what v21.2 successfully learned from

## v25.7 (2026-04-13, completed — `pbracd6x`)

Status:
- completed normally

Goal:
- same config as v25.6 but warm-start from the `pbracd6x` 420M checkpoint
  which showed the highest Jad-kill windows in any run
- test whether this checkpoint carries its Jad-killing behavior forward under
  the cleaned-up v25.6 backend (instant Jad terminal, no milestone bonuses)

Warm-start source:
- `<repo-root>/pufferlib_4/checkpoints/fight_caves/pbracd6x/0000000420478976.bin`

Why this checkpoint:
- `pbracd6x` was a run using the v25.5a config + the same `7qhjnxa2/368M`
  warm-start, with milestone bonuses active
- it showed sustained 4-5% Jad-kill windows between 86M-151M
- this tests whether the checkpoint's Jad behavior survives without the
  milestone shaping that originally produced it

Diff versus v25.6:
- warm-start: `7qhjnxa2/0000000368050176.bin -> pbracd6x/0000000420478976.bin`
- all other config, code, and backend identical to v25.6

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/pbracd6x/0000000420478976.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=150.0`, `w_player_death=-20.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=0.0`, `shape_jad_kill_bonus=0.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `w0spvmva`

Results (`w0spvmva`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2682.0s`
- throughput:
  - `1.81M SPS`

Final metrics:
- `wave_reached = 58.23`
- `max_wave = 63`
- `most_npcs_slayed = 273`
- `episode_length = 7535`
- `reached_wave_63 = 0.0451`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.241`
- `prayer_uptime_range = 0.281`
- `prayer_uptime_magic = 0.247`
- `correct_prayer = 2276.98`
- `wrong_prayer_hits = 299.01`
- `no_prayer_hits = 23.62`
- `prayer_switches = 2620.90`
- `damage_blocked = 162161.3`
- `dmg_taken_avg = 5818.08`
- `attack_when_ready_rate = 0.9487`
- `pots_used = 30.56`
- `avg_prayer_on_pot = 0.5751`
- `pots_wasted = 10.74`
- `food_eaten = 10.63`
- `food_wasted = 3.21`
- `max_wave_ticks = 290.02`
- `max_wave_ticks_wave = 46.17`

Key sampled progression:
- sampled `wave_reached >= 30` by `14.7M`
- sampled `wave_reached >= 55` by `29.4M`
- sampled `wave_reached >= 60` by `33.6M`
- sustained `jad_kill_rate > 0` in 142 of 2383 windows
- strongest sampled `jad_kill_rate` window:
  - around `1.273B`:
    - `wave_reached = 62.29`
    - `reached_wave_63 = 0.7253`
    - `jad_kill_rate = 0.01717`
- top sampled `jad_kill_rate` windows:
  - `577M`: `jad_kill_rate = 0.01653`, `reached_wave_63 = 0.8678`
  - `1.273B`: `jad_kill_rate = 0.01717`, `reached_wave_63 = 0.7253`
  - `1.290B`: `jad_kill_rate = 0.01556`, `reached_wave_63 = 0.7198`
  - `1.254B`: `jad_kill_rate = 0.01487`, `reached_wave_63 = 0.4944`
  - `573M`: `jad_kill_rate = 0.01435`, `reached_wave_63 = 0.8278`
- strongest sampled `reached_wave_63` / `wave_reached` window:
  - around `579M`:
    - `wave_reached = 62.85`
    - `reached_wave_63 = 0.9345`
    - `jad_kill_rate = 0.0`
- nearest eval checkpoints to the important windows:
  - best sampled Jad windows (~577M / 1.273B):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/w0spvmva/0000000577765376.bin`
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/w0spvmva/0000001259339776.bin`
  - best sampled wave-63 access window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/w0spvmva/0000000577765376.bin`

Comparison to v25.6 (`yra7umqg`):
- final metrics were very similar:
  - `wave_reached: 58.23 vs 57.31`
  - `reached_wave_63: 0.0451 vs 0.0` (v25.7 slightly better at end)
  - `attack_when_ready_rate: 0.9487 vs 0.9437`
  - `correct_prayer: 2277.0 vs 2283.9`
- Jad kill windows were stronger and more sustained:
  - peak `jad_kill_rate: 0.01717 vs 0.03288` (v25.6 peaked higher but earlier)
  - v25.7 had 142 windows with `jad_kill_rate > 0` vs 258 for v25.6
  - v25.7's Jad windows persisted later into the run (1.273B vs 107M peak)
- the `pbracd6x` checkpoint produced more durable Jad behavior than the
  `7qhjnxa2/368M` checkpoint

Comparison to v25.6a (`pbracd6x`, stopped, old backend):
- v25.6a had sustained 4-5% Jad-kill windows with milestone bonuses active
- v25.7 peaked at 1.7% without milestone bonuses
- the milestone bonuses (`shape_jad_kill_bonus=500`) were likely responsible for
  the ~3x difference in peak Jad-kill rate

Analysis:
- the `pbracd6x/420M` checkpoint carried meaningful Jad behavior forward
  into the v25.6 backend without milestone bonuses
- v25.7 showed more durable Jad activity than v25.6 — the Jad-kill windows
  persisted past 1B steps rather than collapsing by 200M
- but peak Jad-kill rate (1.7%) was still well below the 4-5% sustained
  windows seen in v25.6a with milestone bonuses
- the strongest signal continues to be that milestone bonuses
  (`shape_jad_kill_bonus=500`) are a significant lever for Jad conversion

## v25.6a (2026-04-13, stopped early — `pbracd6x`)

Status:
- stopped early at ~787M steps

Actual run:
- `pbracd6x`

Goal:
- was intended as v25.6 but ran under the old v25.5a config before backend
  changes were applied (no Jad instant-terminal, no healer despawn, old
  separate `w_cave_complete` channel)

Warm-start source:
- `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000000368050176.bin`

Backend:
- old code — cave completion required all NPCs including healers to be dead
- no wave duration analytics

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000000368050176.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=100.0`, `shape_jad_kill_bonus=500.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Key sampled progression:
- sampled `wave_reached >= 60` by `33.6M`
- sustained `jad_kill_rate > 0` from `33.6M` through `637.5M` (89 of 391 windows)
- strongest sampled `jad_kill_rate` window:
  - around `42M`:
    - `wave_reached = 58.30`
    - `reached_wave_63 = 0.5000`
    - `jad_kill_rate = 0.2000`
    - note: small sample size at this early point (likely 5 episodes, 1 Jad kill)
- strongest sustained `jad_kill_rate` windows:
  - around `86M`: `jad_kill_rate = 0.04762`, `reached_wave_63 = 0.4762`
  - around `111M`: `jad_kill_rate = 0.04464`, `reached_wave_63 = 0.8929`
  - around `151M`: `jad_kill_rate = 0.04545`, `reached_wave_63 = 0.9091`
- strongest sampled `reached_wave_63` window:
  - around `40M`:
    - `reached_wave_63 = 0.9122`
    - `wave_reached = 62.81`
    - `jad_kill_rate = 0.006928`
- nearest eval checkpoints to the important windows:
  - best sampled Jad window (42M):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/pbracd6x/0000000053477376.bin`
  - best sustained Jad + high wave-63 (111M):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/pbracd6x/0000000105906176.bin`
  - 420M (near earlier identified best practical checkpoint):
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/pbracd6x/0000000420478976.bin`

Note:
- this run had `shape_reach_wave_63_bonus=100.0` and `shape_jad_kill_bonus=500.0`
  active, giving a total Jad-kill reward of 750 (50+100+100+500) versus 150 in
  v25.6/v25.7
- the 0.2 `jad_kill_rate` at 42M is almost certainly a small-sample artifact
  (very few episodes completed at that early step count)
- the more reliable signal is the sustained 4-5% Jad-kill windows between
  86M-151M with high `reached_wave_63` rates

## v25.5a (2026-04-13, completed — `wiee1ezs`)

Status:
- completed normally

Goal:
- keep the stronger `v25.4` recipe and the `v25.5` checkpoint-refresh structure
- keep only the milestone rewards that are closest to the actual bottleneck:
  reaching wave `63` and killing Jad
- remove the broader wave `60-62` bonuses that appear to have improved generic
  late-wave stability more than true wave-63 / Jad conversion

Planned recipe:
- same as `v25.5`
- same warm-start:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000001888485376.bin`
- additive per-episode milestone bonuses:
  - `shape_reach_wave_60_bonus = 0.0`
  - `shape_reach_wave_61_bonus = 0.0`
  - `shape_reach_wave_62_bonus = 0.0`
  - `shape_reach_wave_63_bonus = 100.0`
  - `shape_jad_kill_bonus = 500.0`

Diff versus `v25.5`:
- `shape_reach_wave_60_bonus: 60.0 -> 0.0`
- `shape_reach_wave_61_bonus: 90.0 -> 0.0`
- `shape_reach_wave_62_bonus: 120.0 -> 0.0`
- `shape_reach_wave_63_bonus: 200.0 -> 100.0`
- `shape_jad_kill_bonus` stays `500.0`
- all other config, code, and warm-start inputs remain identical

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000001888485376.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_reach_wave_60_bonus=0.0`, `shape_reach_wave_61_bonus=0.0`, `shape_reach_wave_62_bonus=0.0`, `shape_reach_wave_63_bonus=100.0`, `shape_jad_kill_bonus=500.0`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Reasoning:
- `v25.5` improved final late-wave stability but did not improve the strongest
  wave-63 / Jad frontier windows
- the broad `60-62` milestone bonuses appear to be paying too much for generic
  late-game survival and not enough specifically for the rare states we care
  about most
- `v25.5a` narrows the shaping to the two events that matter most:
  reaching wave `63` and killing Jad

Backend notes:
- wave number is already present in the observation as `FC_OBS_META_WAVE`
- only `shape_reach_wave_63_bonus` and `shape_jad_kill_bonus` were active
- `shape_reach_wave_60_bonus`, `61`, and `62` were zeroed

Actual run:
- `wiee1ezs`
- local run log:
  - `wiee1ezs.json` (not retained locally)

Results (`wiee1ezs`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2613.1s`
- throughput:
  - `1.89M SPS`

Final metrics:
- `wave_reached = 58.59`
- `max_wave = 63`
- `most_npcs_slayed = 277`
- `episode_length = 7525`
- `reached_wave_63 = 0.1438`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.242`
- `prayer_uptime_range = 0.274`
- `prayer_uptime_magic = 0.251`
- `correct_prayer = 2543.95`
- `wrong_prayer_hits = 310.50`
- `no_prayer_hits = 15.16`
- `prayer_switches = 3045.48`
- `damage_blocked = 176691.8`
- `dmg_taken_avg = 5359.38`
- `attack_when_ready_rate = 0.9634`
- `pots_used = 30.4`
- `avg_prayer_on_pot = 0.5666`
- `pots_wasted = 10.05`
- `food_eaten = 7.35`
- `food_wasted = 0.50`

Key sampled progression:
- strongest sampled `jad_kill_rate` window:
  - around `394M`:
    - `wave_reached = 59.29`
    - `reached_wave_63 = 0.1190`
    - `jad_kill_rate = 0.01190`
- strongest sampled `reached_wave_63` / `wave_reached` window:
  - around `451M`:
    - `wave_reached = 62.82`
    - `reached_wave_63 = 0.9174`
    - `jad_kill_rate = 0.0`
- nearest eval checkpoints to the important windows:
  - best sampled Jad window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/wiee1ezs/0000000368050176.bin`
  - best sampled wave-63 access window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/wiee1ezs/0000000472907776.bin`

Comparison to `v25.5`:
- sampled and final metrics were numerically identical to `sluy9lmm`
- both runs used the same warm-start:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000001888485376.bin`
- practical outcome:
  - narrowing the ladder from `60/61/62/63 + Jad` to only `63 + Jad` did not
    produce any observable behavioral change under this seed/checkpoint

Comparison to `v25.4`:
- final late-game stability remained better than `v25.4`
  - `reached_wave_63: 0.1438 vs 0.0397`
  - `wave_reached: 58.59 vs 56.95`
- but the best frontier windows still did not clearly exceed the best
  `v25.4` windows once the full W&B history is considered
- `v25.4` remains the stronger checkpoint-source run

Analysis:
- `v25.5a` is effectively a duplicate of `v25.5`
- there is no evidence that removing the `wave 60-62` bonuses changed the
  optimization path in any meaningful way
- the strongest interpretation is that the late-wave milestone shaping itself
  is not the main lever right now
- the bigger signal continues to be checkpoint quality:
  - `v25.4` produced the best modern checkpoint windows of this line
  - both `v25.5` and `v25.5a` were better as checkpoint-refresh experiments
    than as new reward-discovery runs

Recommendation:
- do not spend more runs on milestone-ladder variants right now
- keep `7qhjnxa2` as the best checkpoint-source run from this branch
- current best checkpoint candidate remains:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000000368050176.bin`
- next highest-value runs remain:
  - `v26` cold-start on the `v25.4` / `v25.5a` recipe
  - `v26.1` warm-start from the best `v26` checkpoint
- keep `v25.6` as a lower-priority diagnostic only

## v25.5 (2026-04-12, completed — `sluy9lmm`)

Status:
- completed normally

Goal:
- keep the stronger `v25.4` recipe intact
- upweight late-wave trajectories so rare episodes that actually reach
  waves `60-63` and kill Jad get more learning signal

Planned recipe:
- same as `v25.4`
- warm-start from the strongest sampled frontier checkpoint in `v25.4`:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000001888485376.bin`
- additive per-episode milestone bonuses:
  - `shape_reach_wave_60_bonus = 60.0`
  - `shape_reach_wave_61_bonus = 90.0`
  - `shape_reach_wave_62_bonus = 120.0`
  - `shape_reach_wave_63_bonus = 200.0`
  - `shape_jad_kill_bonus = 500.0`
- exact reward weights:
  - `w_damage_dealt = 0.7`
  - `w_attack_attempt = 0.2`
  - `w_damage_taken = -0.6`
  - `w_npc_kill = 3.5`
  - `w_wave_clear = 15.0`
  - `w_jad_damage = 2.0`
  - `w_jad_kill = 50.0`
  - `w_player_death = -20.0`
  - `w_cave_complete = 100.0`
  - `w_correct_jad_prayer = 2.0`
  - `w_correct_danger_prayer = 0.25`
  - `w_invalid_action = -0.1`
  - `w_tick_penalty = -0.005`
- exact shaping:
  - `shape_food_full_waste_penalty = -6.5`
  - `shape_food_waste_scale = -1.2`
  - `shape_food_no_threat_penalty = 0.0`
  - `shape_pot_full_waste_penalty = -6.5`
  - `shape_pot_waste_scale = -1.2`
  - `shape_pot_no_threat_penalty = 0.0`
  - `shape_wrong_prayer_penalty = -1.25`
  - `shape_npc_specific_prayer_bonus = 1.5`
  - `shape_npc_melee_penalty = -0.3`
  - `shape_wasted_attack_penalty = -0.1`
  - `shape_wave_stall_start = 500`
  - `shape_wave_stall_base_penalty = -0.5`
  - `shape_wave_stall_ramp_interval = 50`
  - `shape_wave_stall_cap = -2.0`
  - `shape_not_attacking_grace_ticks = 2`
  - `shape_not_attacking_penalty = -0.01`
  - `shape_kiting_reward = 1.0`
  - `shape_kiting_min_dist = 7`
  - `shape_kiting_max_dist = 10`
  - `shape_unnecessary_prayer_penalty = -0.2`
  - `shape_jad_heal_penalty = -0.1`
  - `shape_resource_threat_window = 2`
- PPO/vector/policy:
  - `total_agents = 4096`
  - `num_buffers = 2`
  - `total_timesteps = 5_000_000_000`
  - `learning_rate = 0.0003`
  - `anneal_lr = 0`
  - `gamma = 0.999`
  - `gae_lambda = 0.95`
  - `clip_coef = 0.2`
  - `vf_coef = 0.5`
  - `ent_coef = 0.01`
  - `max_grad_norm = 0.5`
  - `horizon = 256`
  - `minibatch_size = 4096`
  - `hidden_size = 256`
  - `num_layers = 2`

Backend notes:
- wave number is already present in the observation as `FC_OBS_META_WAVE`
- the new bonuses pay on the actual wave-transition tick when the episode
  enters waves `60`, `61`, `62`, and `63`
- the Jad bonus pays on the Jad-death tick, even before the separate
  cave-complete terminal signal fires

Actual run:
- `sluy9lmm`
- local run log:
  - `sluy9lmm.json` (not retained locally)

Results (`sluy9lmm`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2601.1s`
- throughput:
  - `1.90M SPS`

Final metrics:
- `wave_reached = 58.59`
- `max_wave = 63`
- `most_npcs_slayed = 277`
- `episode_length = 7525`
- `reached_wave_63 = 0.1438`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.242`
- `prayer_uptime_range = 0.274`
- `prayer_uptime_magic = 0.251`
- `correct_prayer = 2543.95`
- `wrong_prayer_hits = 310.50`
- `no_prayer_hits = 15.16`
- `prayer_switches = 3045.48`
- `damage_blocked = 176691.8`
- `dmg_taken_avg = 5359.38`
- `attack_when_ready_rate = 0.9634`
- `pots_used = 30.4`
- `avg_prayer_on_pot = 0.5666`
- `pots_wasted = 10.05`
- `food_eaten = 7.35`
- `food_wasted = 0.50`

Key sampled progression:
- first sampled point around `628M` already showed a strong late-game plateau:
  - `wave_reached = 59.63`
  - `reached_wave_63 = 0.2246`
  - `jad_kill_rate = 0.0000529`
- strongest sampled `jad_kill_rate` window was later:
  - around `3.126B`:
    - `wave_reached = 59.13`
    - `reached_wave_63 = 0.2377`
    - `jad_kill_rate = 0.0002132`
- nearest eval checkpoints to the important windows:
  - early breakout / best sampled Jad window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/sluy9lmm/0000000630194176.bin`
  - strongest sampled late-game plateau:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/sluy9lmm/0000001888485376.bin`
  - best sampled `reached_wave_63` window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/sluy9lmm/0000003146776576.bin`
  - final:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/sluy9lmm/0000004999610368.bin`

Progression versus `v25.4` (`7qhjnxa2`):
- final late-game stability improved:
  - `wave_reached: 58.59 vs 56.95`
  - `reached_wave_63: 0.1438 vs 0.0397`
  - `correct_prayer: 2543.95 vs 2288.13`
  - `damage_blocked: 176691.8 vs 155482.0`
  - `dmg_taken_avg: 5359.4 vs 6249.8`
  - `food_eaten: 7.35 vs 15.63`
  - `food_wasted: 0.50 vs 5.51`
  - `no_prayer_hits: 15.16 vs 17.03`
- but the best frontier windows regressed:
  - `wave_reached` never reached the `60.5-61.0` sampled windows seen in `v25.4`
  - `reached_wave_63` peaked at `0.2377` vs `0.4453`
  - `jad_kill_rate` peaked at `0.0002132` vs `0.0013007`
- prayer/resource churn also remained worse in some dimensions:
  - `prayer_switches: 3045.5 vs 2545.7`
  - `pots_wasted: 10.05 vs 8.86`

Analysis:
- the broad `60/61/62/63 + Jad` bonus ladder did change behavior
- it appears to have improved generic late-game stability and damage control
- but it did not improve the actual rare frontier events we care about most:
  wave-63 conversion and Jad kills
- the likely failure mode is that the wave `60-62` bonuses paid too much for
  strong-but-not-frontier trajectories, nudging PPO toward “stable late-game”
  rather than “push all the way into repeated Jad reps”
- the final signature supports that:
  - stronger blocking
  - lower damage taken
  - much better food discipline
  - but lower true frontier conversion than `v25.4`

Recommendation:
- keep the `v25.4` checkpoint-refresh structure
- remove the broad wave `60-62` milestone rewards
- keep only:
  - a smaller `wave 63` bonus
  - the large Jad-kill bonus
- that is the purpose of `v25.5a`

Reasoning:
- the current bottleneck is not basic survival any more
- the current bottleneck is getting enough high-quality wave-63 / Jad
  trajectories to stabilize the late-game policy
- this run tests whether explicitly upweighting late-wave trajectories helps
  preserve and amplify the rare strong episodes

Current recommendation:
- this is a reasonable experiment to run before `v26`
- but it should be interpreted as trajectory upweighting, not as a true
  curriculum replacement

## v25.4 (2026-04-12, completed — `u58coupx`)

Status:
- completed normally

Goal:
- keep the strongest currently validated `v25.2` recipe intact
- replace the stale `u58coupx` warm-start with a stronger checkpoint trained
  under the newer LOS/healer/Jad reward mechanics
- measure whether the old checkpoint is now anchoring the policy to outdated
  behavior

Why this is the chosen checkpoint:
- `frt9a1j4/0000003146776576.bin` was the best sampled Jad/clear window in
  `v25.2`
- this is a better warm-start candidate than the best average-wave window
  because it represents the strongest observed late-game conversion and Jad
  behavior under the newer mechanics

Config versus `v25.3`:
- scalar `.ini` values remain unchanged
- warm-start source changes:
  - `u58coupx/0000001311768576.bin`
  - ->
  - `frt9a1j4/0000003146776576.bin`
- code-only difference versus `v25.3`:
  - remove the melee extension from the generic correct-prayer reward
  - `correct_danger_prayer` returns to ranged/magic-only

Exact difference versus `v25.3`:
- same scalar config
- same LOS fix
- same healer aggro fix
- same Jad-heal penalty
- same `v25.1` ready-idle prayer gate
- same `w_correct_jad_prayer = 2.0`
- generic `correct_danger_prayer` no longer applies to blocked melee hits
- warm-start path switches to:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/frt9a1j4/0000003146776576.bin`

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/frt9a1j4/0000003146776576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Actual run:
- `7qhjnxa2`
- local run log:
  - `7qhjnxa2.json` (not retained locally)

Results (`7qhjnxa2`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2548.0s`
- throughput:
  - `1.90M SPS`

Final metrics:
- `wave_reached = 56.95`
- `max_wave = 63`
- `most_npcs_slayed = 277`
- `episode_length = 7130`
- `reached_wave_63 = 0.0397`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.289`
- `prayer_uptime_range = 0.283`
- `prayer_uptime_magic = 0.235`
- `correct_prayer = 2288.13`
- `wrong_prayer_hits = 322.32`
- `no_prayer_hits = 17.03`
- `prayer_switches = 2545.70`
- `damage_blocked = 155482.0`
- `dmg_taken_avg = 6249.85`
- `attack_when_ready_rate = 0.9653`
- `pots_used = 30.6`
- `avg_prayer_on_pot = 0.5740`
- `pots_wasted = 8.86`
- `food_eaten = 15.6`
- `food_wasted = 5.51`

Key progression points:
- sampled `wave_reached >= 30` by `14.7M`
- sampled `wave_reached >= 45` by `228.6M`
- sampled `wave_reached >= 50` by `228.6M`
- sampled `wave_reached >= 55` by `228.6M`
- sampled `wave_reached >= 60` by `228.6M`
- progression table showed a sustained `60-62.5` plateau from roughly
  `228.6M` through `3.48B`
- strongest sampled average-wave point in the progression table was around
  `1.497B`:
  - `wave_reached = 62.5`
  - `episode_length = 8337`
- strongest sampled frontier windows from the summary history:
  - around `1.876B`:
    - `wave_reached = 60.96`
    - `reached_wave_63 = 0.4319`
    - `jad_kill_rate = 0.000575`
  - around `3.126B`:
    - `wave_reached = 60.54`
    - `reached_wave_63 = 0.4453`
    - `jad_kill_rate = 0.000250`
- nearest eval checkpoints to the important windows:
  - early breakout:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000000630194176.bin`
  - strongest sampled frontier window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000001888485376.bin`
  - best sampled `reached_wave_63` window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000003146776576.bin`
  - final:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7qhjnxa2/0000004999610368.bin`

Comparison to `v25.2` (`frt9a1j4`):
- `v25.4` supports the checkpoint-refresh hypothesis
- compared to `v25.2`, the refreshed warm-start improved the strongest sampled
  late-game windows:
  - `reached_wave_63: 0.4453 vs 0.4134`
  - `jad_kill_rate: 0.000575 peak vs 0.000475 / 0.000685 windows`
  - `attack_when_ready_rate: 0.9653 vs 0.9549` at final
  - `correct_prayer: 2288.1 vs 2162.8` at final
  - `avg_prayer_on_pot: 0.5740 vs 0.5833`
  - `pots_wasted: 8.86 vs 9.79`
- the final `wave_reached` was slightly lower:
  - `56.95 vs 57.70`
- but the mid-run frontier windows were stronger and more persistent

Comparison to `v25.1` (`zyhv95mi`):
- `v25.1` still has the best final late-game profile of the current line
- but `v25.4` now looks much closer to `v25.1` than `v25.2` did in terms of
  wave-63/Jad opportunity generation

Analysis:
- this run is a real positive result
- the refreshed checkpoint appears to be better aligned with the modern
  LOS/healer/Jad mechanics than the old `u58coupx` warm-start
- the strongest evidence is not the final metric line; it is the sustained
  `60-62.5` plateau and stronger `wave 63` windows through the middle of the
  run
- the run still decayed by the end, so the checkpoint refresh is helpful but
  not sufficient on its own
- this supports continuing with the checkpoint-refresh / cold-start plan

What worked:
- replacing `u58coupx` with the best `v25.2` checkpoint
- keeping the stronger `v25.2` recipe instead of the weaker `v25.3` melee
  variant

What did not work:
- the refreshed checkpoint alone did not solve late-run decay
- final Jad conversion still was not stable

Recommendation:
- the overall direction is correct
- `v25.5`, `v26`, and `v26.1` are all still reasonable next runs
- current priority order:
  - `v25.5`
  - `v26`
  - `v26.1`
- rationale:
  - `v25.4` suggests checkpoint refresh helps
  - `v25.5` is now a reasonable targeted attempt to further upweight rare
    late-wave trajectories under the stronger refreshed-warm-start setup
  - `v26` / `v26.1` remain the cleanest answer to the stale-checkpoint
    question

## v25.3 (2026-04-12, completed — `6gi2pyei`)

Actual run:
- `6gi2pyei`
- local run log:
  - `6gi2pyei.json` (not retained locally)

Status:
- completed normally

Primary goal:
- optimize pre-Jad waves so the policy reaches wave 63 / Jad more often
- get more Jad repetitions by improving pre-Jad prayer correctness and
  conversion, not by adding more Jad-only complexity

Goal:
- keep the full `v25.2` recipe intact
- extend the generic correct-prayer reward so it also applies to successfully
  blocked melee hits
- keep Jad-specific reward, LOS/healer fixes, and the `v25.1` ready-idle gate
  unchanged

Config versus `v25.2`:
- no scalar `.ini` delta
- checked-in config surface was trimmed, but remaining live values were the
  same as `v25.2`
- intended behavior delta was code-only:
  - the generic positive prayer reward path now applied to correctly blocked
    melee hits as well as ranged/magic hits

Code delta versus `v25.2`:
- `correct_danger_prayer` was changed from ranged/magic-only to any correctly
  blocked hit style:
  - melee
  - ranged
  - magic
- the same ready-idle gate still suppressed this positive reward if the agent
  had been attack-ready and idle for 1 full tick
- the NPC-specific bonus path remained separate:
  - Tok-Xil / Ket-Zek / melee cave NPCs still kept
    `shape_npc_specific_prayer_bonus`
  - Jad still kept `w_correct_jad_prayer = 2.0`

Exact difference versus `v25.2`:
- no config-key change
- code-only reward change:
  - a correctly blocked melee hit newly received the generic
    `w_correct_danger_prayer = 0.25`
- resulting total positive blocked-prayer rewards when not idle became:
  - melee cave NPCs currently in the NPC-specific set:
    - `0.25 + 1.5 = 1.75`
  - `Tok-Xil` / `Ket-Zek`:
    - unchanged at `0.25 + 1.5 = 1.75`
  - `Jad` ranged/magic:
    - unchanged at `0.25 + 2.0 = 2.25`

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`6gi2pyei`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2518.0s`
- throughput:
  - `1.94M SPS`

Final metrics:
- `wave_reached = 51.67`
- `max_wave = 63`
- `most_npcs_slayed = 277`
- `episode_length = 6260`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime_melee = 0.228`
- `prayer_uptime_range = 0.348`
- `prayer_uptime_magic = 0.214`
- `correct_prayer = 1590.43`
- `wrong_prayer_hits = 234.31`
- `no_prayer_hits = 16.19`
- `prayer_switches = 1447.49`
- `damage_blocked = 105902.99`
- `dmg_taken_avg = 4441.15`
- `attack_when_ready_rate = 0.8857`
- `pots_used = 29.6`
- `avg_prayer_on_pot = 0.6642`
- `pots_wasted = 14.09`
- `food_eaten = 20.0`
- `food_wasted = 17.28`

Key progression points:
- sampled `wave_reached >= 45` by `629.1M`
- sampled `wave_reached >= 55` by `1.876B`
- sampled `wave_reached >= 59` by `1.876B`
- first real `wave 63` window was also `1.876B`
- best sampled window was around `1.876B`:
  - `wave_reached = 59.14`
  - `reached_wave_63 = 0.3144`
  - `jad_kill_rate = 0.0000358`
  - `attack_when_ready_rate = 0.9067`
  - `damage_blocked = 150257.29`
- after that, the run decayed steadily:
  - `3.126B`: `wave_reached = 55.69`, `reached_wave_63 = 0.0377`
  - `4.376B`: `wave_reached = 54.90`, `reached_wave_63 = 0.0901`
  - final: `wave_reached = 51.67`, `reached_wave_63 = 0.0`
- nearest eval checkpoints to the important windows:
  - early breakout:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/6gi2pyei/0000000630194176.bin`
  - best sampled frontier window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/6gi2pyei/0000001888485376.bin`
  - post-peak decay:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/6gi2pyei/0000003146776576.bin`
  - late partial recovery:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/6gi2pyei/0000004352638976.bin`
  - final:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/6gi2pyei/0000004999610368.bin`

Comparison to `v25.2` (`frt9a1j4`):
- final `wave_reached: 51.67 vs 57.70`
- final `reached_wave_63: 0.0 vs 0.0588`
- final `attack_when_ready_rate: 0.8857 vs 0.9549`
- final `correct_prayer: 1590.4 vs 2162.8`
- final `damage_blocked: 105903 vs 161546`
- final `avg_prayer_on_pot: 0.6642 vs 0.5833`
- final `pots_wasted: 14.09 vs 9.79`
- final `food_wasted: 17.28 vs 2.20`
- even the best `v25.3` window was weaker than the best `v25.2` windows:
  - `wave_reached: 59.14 vs 60.42`
  - `reached_wave_63: 0.3144 vs 0.4080 / 0.4134`
  - `jad_kill_rate: 0.0000358 vs 0.0004748 / 0.0006846`

Comparison to `v25.1` (`zyhv95mi`):
- `v25.1` still has the strongest late-game profile of the `v25.x` line
- `v25.3` underperformed `v25.1` both at peak and at the end

Analysis:
- this run strongly suggests the new melee generic prayer reward was a net
  negative
- the policy became more conservative, not more effective:
  - fewer no-prayer hits and lower damage taken
  - but much lower offensive tempo, much lower blocked-damage output, and much
    worse resource quality
- the likely failure mode is that rewarding blocked melee hits through the same
  generic positive path made defensive melee prayer behavior too valuable
  relative to staying aggressive and converting waves efficiently
- the ready-idle gate was not enough to fully stop this; the policy still found
  a more defensive attractor

What worked:
- the codebase is still capable of producing real `wave 63` / Jad windows under
  the current LOS/healer/Jad logic
- the best `v25.3` checkpoint is usable as a replay/debug target for studying
  the collapse pattern

What did not work:
- keeping the `v25.2` recipe and simply adding melee into the generic prayer
  reward
- the change hurt both the ceiling and the stability of the run

Recommendation:
- the checkpoint-refresh idea is correct
- the cold-start test is also correct
- but I do not recommend carrying the `v25.3` melee-reward change into those
  tests
- the better next sequence is:
  - `v25.4 = v25.2 recipe + frt9a1j4/0000003146776576.bin`
  - `v26 = cold-start on exact v25.2 recipe`
  - `v26.1 = warm-start from the best v26 checkpoint`
- rationale:
  - `v25.3` did not beat `v25.2` even at its best checkpoint, so it is a weak
    base recipe for the checkpoint-refresh experiment
  - `u58coupx` may still be stale, but `v25.3` does not isolate that question
    cleanly because the recipe itself regressed

## v25.2 (2026-04-12, completed — `frt9a1j4`)

Actual run:
- `frt9a1j4`
- local run log:
  - `frt9a1j4.json` (not retained locally)

Status:
- completed normally

Goal:
- keep the full `v25.1` recipe intact
- add a real Jad-specific positive prayer reward so Jad blocks are rewarded
  more strongly than other NPC prayer blocks
- keep the reward timing correct for Jad's deferred prayer lock:
  - no reward on attack-start tick
  - no reward on prayer-lock tick
  - reward only on resolve tick

Config versus `v25.1`:
- one `.ini` delta only:
  - `w_correct_jad_prayer: 0.0 -> 2.0`
- all other reward weights, shaping values, PPO/runtime values, warm-start,
  observation contract, LOS/healer fixes, and the `v25.1` ready-idle prayer
  gate are unchanged

Code versus `v25.1`:
- `w_correct_jad_prayer` is now fully wired through the reward stack:
  - config parsing
  - fc-training parameter plumbing
  - core reward computation
  - viewer reward parsing / overlay
- Jad-specific positive reward uses the same ready-idle gate as the other
  positive blocked-prayer rewards
- no change to Jad attack timing itself:
  - Jad melee still snapshots prayer immediately and rewards on resolve tick
  - Jad ranged/magic still locks prayer 1 tick after attack start and rewards
    only on the later resolve tick using that locked prayer

Exact difference versus `v25.1`:
- `w_correct_jad_prayer = 2.0`
- resulting positive blocked-prayer totals when not idle:
  - `Tok-Xil` / `Ket-Zek`: `0.25 + 1.5 = 1.75`
  - `Jad`: `0.25 + 2.0 = 2.25`
- so Jad correct blocks now pay `0.5` more than the other prayer-critical NPCs

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_jad_prayer=2.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_no_threat_penalty=0.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_no_threat_penalty=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`frt9a1j4`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2553.1s`
- throughput:
  - `1.87M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 57.70`
- `max_wave = 63`
- `most_npcs_slayed = 277`
- `episode_return = 28647.16`
- `episode_length = 7512.73`
- `reached_wave_30 = 0.9608`
- `cleared_wave_30 = 0.9542`
- `reached_wave_31 = 0.9542`
- `reached_wave_63 = 0.0588`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.7013`
- `prayer_uptime_melee = 0.2203`
- `prayer_uptime_range = 0.2524`
- `prayer_uptime_magic = 0.2287`
- `correct_prayer = 2162.81`
- `wrong_prayer_hits = 325.44`
- `no_prayer_hits = 26.51`
- `damage_blocked = 161545.83`
- `dmg_taken_avg = 6189.04`
- `attack_when_ready_rate = 0.9549`
- `pots_used = 27.90`
- `avg_prayer_on_pot = 0.5833`
- `pots_wasted = 9.79`
- `food_eaten = 10.08`
- `avg_hp_on_food = 0.5353`
- `food_wasted = 2.20`
- `tokxil_melee_ticks = 2.31`
- `ketzek_melee_ticks = 3.20`

Key progression points:
- sampled `wave_reached >= 30` by `629.1M`
- sampled `wave_reached >= 35` by `629.1M`
- sampled `wave_reached >= 40` by `629.1M`
- sampled `wave_reached >= 45` by `629.1M`
- sampled `wave_reached >= 50` by `629.1M`
- sampled `wave_reached >= 55` by `1.876B`
- sampled `wave_reached >= 60` by `1.876B`
- sampled `max_wave = 63` by `1.876B`
- first non-zero `cave_complete_rate` window appeared by `1.876B`
- best sampled average-wave window was around `1.876B`:
  - `wave_reached = 60.42`
  - `episode_return = 30778.45`
  - `reached_wave_63 = 0.4080`
  - `jad_kill_rate = 0.000475`
  - `cave_complete_rate = 0.000340`
- best sampled Jad / clear window was around `3.126B`:
  - `wave_reached = 59.66`
  - `episode_return = 30435.22`
  - `reached_wave_63 = 0.4134`
  - `jad_kill_rate = 0.000685`
  - `cave_complete_rate = 0.000403`
- the run decayed steadily after that:
  - `4.376B`: `wave_reached = 58.98`, `reached_wave_63 = 0.1799`
  - final: `wave_reached = 57.70`, `reached_wave_63 = 0.0588`
- nearest eval checkpoints to the important windows:
  - early breakout:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/frt9a1j4/0000000630194176.bin`
  - first `60+` window / best average-wave window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/frt9a1j4/0000001888485376.bin`
  - best sampled Jad / clear window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/frt9a1j4/0000003146776576.bin`
  - late-decay window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/frt9a1j4/0000004352638976.bin`
  - final:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/frt9a1j4/0000004999610368.bin`

Comparison to `v25.1` (`zyhv95mi`):
- `wave_reached: 57.70 vs 60.16`
- `episode_return: 28647.2 vs 30942.2`
- `episode_length: 7512.7 vs 8045.7`
- `reached_wave_63: 0.0588 vs 0.1329`
- `damage_blocked: 161546 vs 179323`
- `correct_prayer: 2162.8 vs 2378.3`
- `wrong_prayer_hits: 325.4 vs 247.1`
- `no_prayer_hits: 26.5 vs 25.9`
- `attack_when_ready_rate: 0.9549 vs 0.9504`
- `prayer_switches: 2678.1 vs 2867.4`
- `pots_used: 27.9 vs 30.6`
- `avg_prayer_on_pot: 0.5833 vs 0.5937`
- `pots_wasted: 9.79 vs 10.37`
- `food_eaten: 10.08 vs 10.43`
- `food_wasted: 2.20 vs 0.77`

Comparison to `v24` (`h2kpwkdk`):
- `score: 0.0 vs 0.0217`
- `cave_complete_rate: 0.0 vs 0.0217`
- `wave_reached: 57.70 vs 61.97`
- `reached_wave_63: 0.0588 vs 0.3582`
- `jad_kill_rate: 0.0 vs 0.0149`
- `damage_blocked: 161546 vs 187217`
- `prayer_uptime_magic: 0.2287 vs 0.5296`
- `prayer_switches: 2678.1 vs 651.3`
- `avg_prayer_on_pot: 0.5833 vs 0.3760`
- `pots_wasted: 9.79 vs 3.07`
- `attack_when_ready_rate: 0.9549 vs 0.7500`

Analysis:
- `v25.2` did not improve the final policy over `v25.1`
- the final run regressed materially on average wave, Jad-wave reach, blocked
  damage, and overall prayer correctness
- however, it did produce an important new signal:
  - unlike `v25.1`, it sampled small but non-zero `cave_complete_rate`
  - so the Jad-specific reward did appear to increase rare successful
    late-game conversion in the mid-run windows
- the problem is that this benefit was not stable and did not survive to the
  final checkpoint

Interpretation:
- the extra Jad reward looks strong enough to create some early/mid-run
  successful Jad trajectories
- but at `2.0` it likely over-pulled the policy toward Jad-specific prayer
  behavior at the expense of the rest of the cave
- the final metric signature supports that:
  - attack tempo remained excellent
  - resource usage improved slightly versus `v25.1`
  - but overall prayer correctness and blocked damage got worse
  - the `60+` plateau weakened instead of strengthening

Important correctness note:
- `jad_kill_rate` and `cave_complete_rate` still diverge in this run
- sampled windows show `jad_kill_rate > cave_complete_rate`
- that should not happen if Jad death immediately implies cave completion
- so the previously identified Jad/healer completion bug is still present and
  still contaminates late-game analytics
- `cave_complete_rate` remains the authoritative metric for actual clears

Findings:
- the Jad-specific reward is active and behaviorally meaningful
- `w_correct_jad_prayer = 2.0` is too aggressive as a full-run setting
- the current backend still has an unresolved correctness bug:
  - Jad can die without the episode immediately terminating as cave complete
- that bug matters more now because `v25.2` finally produced some real
  corrected-mechanics clear signals, so metric/reporting fidelity is important

Recommendation for `v25.3`:
- first fix cave completion semantics:
  - Jad death should immediately imply `TERMINAL_CAVE_COMPLETE`
  - remaining Yt-HurKot healers should not keep the episode alive after Jad dies
- after that, retest Jad reward at a smaller value
  - recommended range: `0.5` to `1.0`
  - keep all other `v25.1` / `v25.2` settings unchanged
- do not keep `w_correct_jad_prayer = 2.0` as-is for the next full run

## v25.1 (2026-04-12, completed — `zyhv95mi`)

Actual run:
- `zyhv95mi`
- local run log:
  - `zyhv95mi.json` (not retained locally)

Status:
- completed normally

Goal:
- keep the `v25` recipe intact
- change only the blocked-prayer reward logic so the agent cannot keep farming
  correct-prayer rewards while sitting attack-ready and idle
- preserve all `v25` LOS/healer correctness fixes and all `v24` / `v22.1`
  config values

Config versus `v25`:
- no `.ini` delta
- identical to `v25` in:
  - warm-start checkpoint
  - corrected `Masori (f) + TBow` combat model
  - observation contract
  - reward-conversion weights
  - `7/10` kiting band
  - low-prayer shaping zeroed
  - LOS/healer correctness fixes
  - Jad-heal penalty
  - PPO/runtime recipe
- only intended behavior delta:
  - when the agent has been attack-ready and idle for 1 full tick, the blocked
    correct-prayer rewards stop applying
  - those rewards immediately re-enable on the next attack attempt

Exact difference versus `v25`:
- no config-key change
- code-only reward gate:
  - `correct_danger_prayer` is suppressed while the attack-ready idle state is
    active
  - `npc_specific_prayer_bonus` is also suppressed during that same idle state
  - the gate clears immediately when `attack_attempt_this_tick > 0`
- implementation detail:
  - reuses the existing `runtime->ticks_since_attack`
  - no new long-lived state/config surface added

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_low_prayer_pot_threshold=0.0`, `shape_low_prayer_no_pot_penalty=0.0`, `shape_low_prayer_pot_reward=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`zyhv95mi`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2552.5s`
- throughput:
  - `1.99M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 60.16`
- `max_wave = 63`
- `most_npcs_slayed = 277`
- `episode_return = 30942.22`
- `episode_length = 8045.71`
- `reached_wave_30 = 1.0`
- `cleared_wave_30 = 1.0`
- `reached_wave_31 = 1.0`
- `reached_wave_63 = 0.1329`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.7217`
- `prayer_uptime_melee = 0.1901`
- `prayer_uptime_range = 0.3171`
- `prayer_uptime_magic = 0.2145`
- `correct_prayer = 2378.29`
- `wrong_prayer_hits = 247.14`
- `no_prayer_hits = 25.94`
- `damage_blocked = 179323.41`
- `dmg_taken_avg = 6053.22`
- `prayer_switches = 2867.40`
- `attack_when_ready_rate = 0.9504`
- `pots_used = 30.60`
- `avg_prayer_on_pot = 0.5937`
- `pots_wasted = 10.37`
- `food_eaten = 10.43`
- `avg_hp_on_food = 0.5360`
- `food_wasted = 0.77`
- `tokxil_melee_ticks = 2.10`
- `ketzek_melee_ticks = 3.10`

Key progression points:
- sampled `wave_reached >= 30` by `629.1M`
- sampled `wave_reached >= 35` by `629.1M`
- sampled `wave_reached >= 40` by `629.1M`
- sampled `wave_reached >= 45` by `629.1M`
- sampled `wave_reached >= 50` by `629.1M`
- sampled `wave_reached >= 55` by `1.876B`
- sampled `wave_reached >= 60` by `1.876B`
- sampled `max_wave = 63` by `1.876B`
- best sampled average-wave window was around `3.126B`:
  - `wave_reached = 61.18`
  - `episode_return = 31638.7`
  - `reached_wave_63 = 0.5394`
- highest sampled `reached_wave_63` was earlier, around `1.876B`:
  - `reached_wave_63 = 0.6099`
  - `jad_kill_rate = 0.0003`
- the run then held a stable `60-61` wave plateau through the back half:
  - `4.377B`: `wave_reached = 60.47`, `reached_wave_63 = 0.1379`
  - final: `wave_reached = 60.16`, `reached_wave_63 = 0.1329`
- nearest eval checkpoints to the important windows:
  - early breakout:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/zyhv95mi/000000525336576.bin`
  - first `60+` / first `max_wave = 63` / highest `reached_wave_63` window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/zyhv95mi/0000001836056576.bin`
  - best average-wave plateau:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/zyhv95mi/0000003094347776.bin`
  - late stable plateau:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/zyhv95mi/0000004352638976.bin`
  - final:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/zyhv95mi/0000004999610368.bin`

Comparison to `v25` (`yf24fz84`):
- `wave_reached: 60.16 vs 45.12`
- `episode_return: 30942.2 vs 17260.3`
- `episode_length: 8046 vs 6101`
- `reached_wave_63: 0.1329 vs 0.0`
- `damage_blocked: 179323 vs 98075`
- `wrong_prayer_hits: 247.1 vs 317.6`
- `no_prayer_hits: 25.9 vs 55.0`
- `attack_when_ready_rate: 0.9504 vs 0.4953`
- `prayer_uptime: 0.7217 vs 0.8298`
- `food_eaten: 10.43 vs 18.37`
- `food_wasted: 0.77 vs 9.29`
- `pots_used: 30.60 vs 28.01`
- `avg_prayer_on_pot: 0.5937 vs 0.5055`
- `pots_wasted: 10.37 vs 6.40`

Comparison to `v24` (`h2kpwkdk`):
- `wave_reached: 60.16 vs 61.97`
- `episode_return: 30942.2 vs 32277.4`
- `episode_length: 8046 vs 8581`
- `reached_wave_63: 0.1329 vs 0.3582`
- `jad_kill_rate: 0.0 vs 0.0149`
- `damage_blocked: 179323 vs 187217`
- `wrong_prayer_hits: 247.1 vs 281.5`
- `no_prayer_hits: 25.9 vs 37.5`
- `attack_when_ready_rate: 0.9504 vs 0.7500`
- `prayer_switches: 2867.4 vs 651.3`
- `prayer_uptime_magic: 0.2145 vs 0.5296`
- `avg_prayer_on_pot: 0.5937 vs 0.3760`
- `pots_wasted: 10.37 vs 3.07`
- `food_eaten: 10.43 vs 8.01`
- `food_wasted: 0.77 vs 1.33`

Analysis:
- `v25.1` is a major recovery from `v25` and the reward-gate change clearly
  fixed the passive blocked-prayer farming behavior
- the strongest evidence is attack tempo:
  - `attack_when_ready_rate` nearly doubled from `0.495 -> 0.950`
  - the run immediately recovered a stable `60+` wave regime instead of
    collapsing into the mid-40s
- survivability also recovered strongly:
  - `damage_blocked` rebounded to near-`v24`
  - both `wrong_prayer_hits` and `no_prayer_hits` improved materially
  - food waste collapsed from `9.29 -> 0.77`
- the run now looks like a high-pressure, high-tempo policy that reaches Jad
  often under the corrected LOS/healer rules, but still burns too much prayer
  and does not yet convert that reach into final kills in the aggregate

Important interpretation note:
- `v24` is not a clean Jad reference anymore because it still benefited from
  the pre-fix Italy-rock wall-shot bug
- so the `v25.1` vs `v24` Jad gap is real in metrics, but not fully
  apples-to-apples on encounter correctness
- the more meaningful comparison is:
  - `v25.1` kept the corrected mechanics from `v25`
  - then almost completely recovered the lost frontier from the idle-gate fix

Findings:
- the idle blocked-prayer reward gate is a keeper
- the remaining regression signature is no longer attack idleness
- the remaining weak point is prayer/resource efficiency:
  - prayer switching is extremely high
  - magic-prayer uptime is still much lower than `v24`
  - prayer pots are still consumed too early and too wastefully
- the run appears to be solving the corrected mechanics by over-switching and
  over-flicking rather than by learning a cleaner Jad/healer resource pattern

Recommendation for `v25.2`:
- keep `v25.1` code intact:
  - keep the idle blocked-prayer gate
  - keep the LOS symmetry fix
  - keep the Yt-HurKot aggro fix
  - keep the Jad-heal penalty
- do not touch attack-tempo shaping next; that part is now working
- make `v25.2` a small resource-discipline experiment only
  - strongest candidate: increase prayer-pot waste punishment
  - specifically, retune only:
    - `shape_pot_full_waste_penalty`
    - and/or `shape_pot_waste_scale`
- reason:
  - `v25.1` already recovered the offensive/tempo side
  - the largest remaining metric regressions are `avg_prayer_on_pot`,
    `pots_wasted`, and excessive prayer switching
  - targeting potion waste is the narrowest next step that addresses the
    current failure signature without undoing the successful idle-gate fix

Suggested `v25.2` framing:
- treat `v25.1` as the new corrected-mechanics baseline
- run one minimal pot-discipline ablation first before touching any broader
  prayer or combat rewards

---

## v25 (2026-04-12, completed — `yf24fz84`)

Actual run:
- `yf24fz84`
- local run log:
  - `yf24fz84.json` (not retained locally)

Status:
- completed normally

Goal:
- keep the successful `v22.1` / `v24` recipe intact
- remove only the inert low-prayer shaping cluster from the live config
- fix the known Jad/healer correctness bugs before the next baseline retry
- verify whether these cleanup/correctness changes preserve the strong `v24`
  behavior

Config versus `v24`:
- identical to `v24` / `v22.1` in:
  - warm-start checkpoint
  - corrected `Masori (f) + TBow` combat model
  - observation contract
  - reward-conversion weights
  - `7/10` kiting band
  - PPO/runtime recipe
- intended learning-behavior delta:
  - disable the low-prayer potion shaping cluster
  - add a small penalty when Yt-HurKot successfully heals Jad
- intended correctness delta:
  - player ranged attacks now require LOS, so Jad can no longer be hit from
    an in-range / no-LOS Italy-rock tile
  - Yt-HurKot now always distracts on landed player hits, including 0s

Exact difference versus `v24`:
- low-prayer shaping:
  - `shape_low_prayer_pot_threshold: 0.0 -> 0.0`
  - `shape_low_prayer_no_pot_penalty: -0.01 -> 0.0`
  - `shape_low_prayer_pot_reward: 0.15 -> 0.0`
- Jad/healer shaping:
  - `shape_jad_heal_penalty: 0.0 -> -0.1`
- code-path fixes:
  - player ranged fire and auto-approach now require LOS symmetry with NPCs
  - Yt-HurKot distraction now triggers on any landed player hit, including 0s

Important implementation note:
- these keys are kept explicitly in the `.ini` and set to `0.0`
- they are not deleted from config because deleting them would fall back to the
  non-zero defaults in `fc_reward_default_params()` and would re-enable the
  feature unintentionally

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_low_prayer_pot_threshold=0.0`, `shape_low_prayer_no_pot_penalty=0.0`, `shape_low_prayer_pot_reward=0.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_jad_heal_penalty=-0.1`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`yf24fz84`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2633.1s`
- throughput:
  - `1.96M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 45.12`
- `max_wave = 63`
- `most_npcs_slayed = 277`
- `episode_return = 17260.33`
- `episode_length = 6101.04`
- `reached_wave_30 = 0.9886`
- `cleared_wave_30 = 0.9829`
- `reached_wave_31 = 0.9829`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.8298`
- `prayer_uptime_melee = 0.2451`
- `prayer_uptime_range = 0.3690`
- `prayer_uptime_magic = 0.2157`
- `correct_prayer = 1730.0`
- `wrong_prayer_hits = 317.55`
- `no_prayer_hits = 54.95`
- `damage_blocked = 98074.67`
- `dmg_taken_avg = 6122.69`
- `prayer_switches = 1345.63`
- `attack_when_ready_rate = 0.4953`
- `pots_used = 28.01`
- `avg_prayer_on_pot = 0.5055`
- `pots_wasted = 6.40`
- `food_eaten = 18.37`
- `avg_hp_on_food = 0.7356`
- `food_wasted = 9.29`
- `tokxil_melee_ticks = 10.07`
- `ketzek_melee_ticks = 10.07`

Key progression points:
- sampled `wave_reached >= 30` by `725.4M`
- sampled `wave_reached >= 35` by `725.4M`
- sampled `wave_reached >= 40` by `725.4M`
- sampled `wave_reached >= 45` by `725.4M`
- sampled `wave_reached >= 50` by `725.4M`
- sampled `wave_reached >= 55` by `725.4M`
- sampled `max_wave = 63` by `1.817B`
- best sampled window was around `1.817B`:
  - `wave_reached = 59.37`
  - `episode_return = 29922.4`
  - `reached_wave_63 = 0.4668`
  - `jad_kill_rate = 0.0007`
- after that peak, the run decayed continuously:
  - `3.126B`: `wave_reached = 50.56`, `reached_wave_63 = 0.0001`
  - `4.376B`: `wave_reached = 48.11`, `reached_wave_63 = 0.0`
  - final: `wave_reached = 45.12`, `reached_wave_63 = 0.0`
- nearest eval checkpoints to the important windows:
  - early breakout / first `55+` window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yf24fz84/0000000735051776.bin`
  - peak window:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yf24fz84/0000001836056576.bin`
  - post-peak decay:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yf24fz84/0000003146776576.bin`
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yf24fz84/0000004352638976.bin`
  - final:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yf24fz84/0000004999610368.bin`

Comparison to `v24` (`h2kpwkdk`):
- `score: 0.0 vs 0.0`
- `wave_reached: 45.12 vs 61.97`
- `max_wave: 63 vs 63`
- `episode_return: 17260.3 vs 32277.4`
- `episode_length: 6101 vs 8581`
- `reached_wave_63: 0.0 vs 0.3582`
- `jad_kill_rate: 0.0 vs 0.0149`
- `prayer_uptime: 0.8298 vs 0.7345`
- `prayer_uptime_magic: 0.2157 vs 0.5296`
- `damage_blocked: 98075 vs 187217`
- `prayer_switches: 1345.6 vs 651.3`
- `attack_when_ready_rate: 0.4953 vs 0.7500`
- `pots_used: 28.01 vs 31.36`
- `avg_prayer_on_pot: 0.5055 vs 0.3760`
- `pots_wasted: 6.40 vs 3.07`
- `food_eaten: 18.37 vs 8.01`
- `food_wasted: 9.29 vs 1.33`

Analysis:
- `v25` is a clear regression from `v24`
- the early/mid-run frontier was briefly strong:
  - by `725M`, it was already sampling `55+` average wave and reaching Jad
  - by `1.817B`, it hit `max_wave = 63` and `reached_wave_63 = 0.4668`
- but the policy never converted that into reliable Jad kills and then decayed
  across the second half of training
- the final metric pattern looks like a more defensive but less effective
  policy:
  - much higher overall prayer uptime
  - much lower magic-prayer uptime
  - much more prayer switching
  - much lower attack tempo
  - much more food usage and food waste
  - worse potion timing than `v24`

Findings:
- the low-prayer config cleanup was expected to be inert
- so the meaningful behavior change in `v25` is most likely coming from the
  LOS/healer correctness bundle, not from zeroing the low-prayer weights
- the run strongly suggests that removing the invalid Jad Italy-rock attack
  path and adding real healer pressure changed the learned Jad solution enough
  that the old warm-start no longer transfers cleanly
- this is a correctness improvement, but it is not a free one; it changed the
  effective task enough to knock down the old `v24` policy

Recommendation:
- do not treat `v25` as the new baseline
- use replay on the `735M`, `1.836B`, `3.147B`, and `4.353B` checkpoints to
  inspect:
  - whether Jad progress is now failing specifically on healer management
  - whether the LOS fix caused broader combat-positioning regressions outside
    Jad
  - whether the large attack-tempo drop is coming from route settling around
    LOS-constrained firing tiles
- if the goal is to preserve `v24` while fixing correctness, the next ablation
  question is not low-prayer; it is which part of the LOS/healer correctness
  bundle is responsible for the regression

---

## v24.s_redo (2026-04-12, completed — multi-run; primary `yptis3c1`, see body for all IDs)

Purpose:
- preserve the original `v24.s` historical record below, but rerun the same
  8-job matrix with the warm-start path actually working before spending the
  full overnight `3B` budget

Historical note:
- the original `v24.s` section below remains unchanged as the record of the
  first attempt and the infrastructure bug it uncovered
- `v24.s_redo` is the corrected rerun path

What was fixed before the redo:
- compiled train mode now loads `load_model_path`
  - `pufferlib_4/pufferlib/pufferl.py`
  - both `_train(...)` and `_train_worker(...)` now call
    `backend.load_weights(...)` when a train-time warm-start checkpoint is
    provided
- the sweep runner now supports `--timesteps`
  - the exact same 8-job matrix can be run at `100M` for validation, then at
    `3B` for the real redo, without maintaining a second sweep config tree
- the sweep runner now emits stable W&B grouping/tagging
  - group: `v24_s_redo`
  - tag: exact matrix label
- trainer backend arch detection was hardened
  - `fc-training/build.sh` now validates `nvidia-smi` compute capability
    output before constructing `sm_*`, preventing bad fallback strings from
    turning into invalid CUDA arch flags

Validation matrix:
- same 8 jobs as `v24.s`
- same config families
- same warm-start checkpoint for nominal warm runs:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`
- validation budget:
  - `100,000,000` timesteps per run
- W&B group:
  - `v24_s_redo`

Validation runs:
- `yptis3c1`
  - `warm_control`
- `gzll8i4d`
  - `cold_control`
- `oqpudmvi`
  - `warm_no_low_prayer`
- `tolxjdbi`
  - `cold_no_low_prayer`
- `j3ps2dxz`
  - `warm_no_reward_retune`
- `kqa052uh`
  - `cold_no_reward_retune`
- `u8vpv9l6`
  - `warm_no_reward_retune_no_low_prayer`
- `rk9du0b6`
  - `cold_no_reward_retune_no_low_prayer`

Validation check:
- passed
- direct proof the warm-start path is now active:
  - training output printed:
    - `Loaded weights from <repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`
  - the warm and cold runs no longer collapse to the same metrics

100M validation results:

| label | id | init | wave_reached | max_wave | cleared_wave_30 | reached_wave_63 | prayer_uptime_magic | damage_blocked | prayer_switches | attack_when_ready_rate | pots_used | avg_prayer_on_pot | pots_wasted | episode_return |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| warm_control | `yptis3c1` | warm | `50.55` | `63` | `1.0` | `0.0054` | `0.0849` | `50161` | `2314.7` | `0.8171` | `0` | `0.0` | `0` | `21022.1` |
| cold_control | `gzll8i4d` | cold | `18.37` | `25` | `0.0` | `0.0` | `0.0011` | `1699` | `287.6` | `0.8297` | `32` | `0.9879` | `32` | `2443.5` |
| warm_no_low_prayer | `oqpudmvi` | warm | `50.55` | `63` | `1.0` | `0.0054` | `0.0849` | `50161` | `2314.7` | `0.8171` | `0` | `0.0` | `0` | `21022.1` |
| cold_no_low_prayer | `tolxjdbi` | cold | `18.37` | `25` | `0.0` | `0.0` | `0.0011` | `1699` | `287.6` | `0.8297` | `32` | `0.9879` | `32` | `2443.5` |
| warm_no_reward_retune | `j3ps2dxz` | warm | `48.95` | `63` | `1.0` | `0.0` | `0.0837` | `48094` | `2218.8` | `0.8271` | `0` | `0.0` | `0` | `13608.3` |
| cold_no_reward_retune | `kqa052uh` | cold | `17.96` | `24` | `0.0` | `0.0` | `0.0011` | `1610` | `289.1` | `0.8444` | `32` | `0.9888` | `32` | `1499.5` |
| warm_no_reward_retune_no_low_prayer | `u8vpv9l6` | warm | `48.95` | `63` | `1.0` | `0.0` | `0.0837` | `48094` | `2218.8` | `0.8271` | `0` | `0.0` | `0` | `13608.3` |
| cold_no_reward_retune_no_low_prayer | `rk9du0b6` | cold | `17.96` | `24` | `0.0` | `0.0` | `0.0011` | `1610` | `289.1` | `0.8444` | `32` | `0.9888` | `32` | `1499.5` |

What the validation proved:
- the sweep is now configured correctly
  - warm-start runs are genuinely warm-started
  - cold runs are genuinely cold
  - tags/groups are applied correctly
  - the 8-job matrix is executing in the intended order
- the original identical warm/cold pairs in `v24.s` were an infrastructure
  bug, not a real learning result

Read of the 100M screen:
- warm vs cold is the dominant axis
  - warm runs immediately jump into the high-wave regime
  - cold runs remain early and burn prayer pots immediately
- low-prayer shaping still looks inert
  - `warm_control` and `warm_no_low_prayer` are effectively identical
  - `cold_control` and `cold_no_low_prayer` are effectively identical
- reward-retune still matters
  - removing it lowers warm-started `wave_reached`
    - `50.55 -> 48.95`
  - removing it lowers warm-started `episode_return`
    - `21022.1 -> 13608.3`
  - removing it also hurts cold-start early progress
    - `18.37 -> 17.96`
    - `25 -> 24` max wave

Observations:
- the warm-started validation runs used `0` prayer pots across the 100M window
  - this is consistent with the checkpoint already carrying the strong
    prayer-control behavior
- cold runs still immediately exhibit the same failure pattern seen before:
  - `pots_used = 32`
  - `avg_prayer_on_pot ≈ 0.988`
  - `pots_wasted = 32`
- low-prayer shaping is still the cleanest removal candidate
- reward-retune should remain in the config for now

Recommendation after the validation:
- proceed with the full corrected `3B` redo suite
- keep the matrix identical to `v24.s`
- do **not** change the reward families again before the redo finishes
- after the corrected `3B` redo:
  - if low-prayer still remains inert in both warm and cold regimes, remove it
    from the live recipe next

3B redo runs:
- `up075ayt`
  - `warm_control`
- `03kfflix`
  - `cold_control`
- `kkntnky8`
  - `warm_no_low_prayer`
- `vrf1fzky`
  - `cold_no_low_prayer`
- `95900x57`
  - `warm_no_reward_retune`
- `i563rr5k`
  - `cold_no_reward_retune`
- `jne1c6dd`
  - `warm_no_reward_retune_no_low_prayer`
- `t0y0be73`
  - `cold_no_reward_retune_no_low_prayer`

Completion status:
- completed through the full corrected 8-run matrix
- all 8 completed runs are in W&B group:
  - `v24_s_redo`
- each completed run carries the expected exact tag
- setup correctness checks passed:
  - warm runs have `load_model_path = u58coupx/0000001311768576.bin`
  - cold runs have `load_model_path = null`
  - warm and cold now diverge sharply, confirming the train-mode checkpoint fix
    is functioning
- there are two extra crashed runs in the same W&B group:
  - `ytmwunek`
  - `11jsdb8w`
  - these were from the manually interrupted launch during setup verification
    and are excluded from the final analysis

3B redo results:

| label | id | init | score | wave_reached | max_wave | cleared_wave_30 | reached_wave_63 | jad_kill_rate | prayer_uptime_magic | damage_blocked | prayer_switches | attack_when_ready_rate | pots_used | avg_prayer_on_pot | pots_wasted | episode_return |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| warm_control | `up075ayt` | warm | `0.00259` | `59.06` | `63` | `1.0` | `0.1458` | `0.00694` | `0.4084` | `129918` | `330.2` | `0.7388` | `30.78` | `0.4216` | `2.84` | `28652.2` |
| cold_control | `03kfflix` | cold | `0.0` | `29.01` | `34` | `0.1970` | `0.0` | `0.0` | `0.0083` | `16133` | `126.8` | `0.5426` | `30.05` | `0.6839` | `16.22` | `7141.8` |
| warm_no_low_prayer | `kkntnky8` | warm | `0.00259` | `59.06` | `63` | `1.0` | `0.1458` | `0.00694` | `0.4084` | `129918` | `330.2` | `0.7388` | `30.78` | `0.4216` | `2.84` | `28652.2` |
| cold_no_low_prayer | `vrf1fzky` | cold | `0.0` | `29.01` | `34` | `0.1970` | `0.0` | `0.0` | `0.0083` | `16133` | `126.8` | `0.5426` | `30.05` | `0.6839` | `16.22` | `7141.8` |
| warm_no_reward_retune | `95900x57` | warm | `0.0` | `37.59` | `63` | `0.8961` | `0.0` | `0.0` | `0.1288` | `65959` | `1113.0` | `0.3010` | `24.03` | `0.3834` | `2.82` | `8244.8` |
| cold_no_reward_retune | `i563rr5k` | cold | `0.0` | `29.80` | `33` | `0.0489` | `0.0` | `0.0` | `0.0` | `20508` | `628.7` | `0.5914` | `26.36` | `0.5923` | `10.46` | `5216.6` |
| warm_no_reward_retune_no_low_prayer | `jne1c6dd` | warm | `0.0` | `37.59` | `63` | `0.8961` | `0.0` | `0.0` | `0.1288` | `65959` | `1113.0` | `0.3010` | `24.03` | `0.3834` | `2.82` | `8244.8` |
| cold_no_reward_retune_no_low_prayer | `t0y0be73` | cold | `0.0` | `29.80` | `33` | `0.0489` | `0.0` | `0.0` | `0.0` | `20508` | `628.7` | `0.5914` | `26.36` | `0.5923` | `10.46` | `5216.6` |

What the corrected 3B suite shows:
- the sweep now went correctly
  - the warm/cold axis is real
  - the `load_model_path` fix worked
  - the original `v24.s` collapse was purely an infrastructure issue
- the matrix again collapsed to only two distinct signatures along the
  low-prayer axis
  - `warm_control` == `warm_no_low_prayer`
  - `cold_control` == `cold_no_low_prayer`
  - `warm_no_reward_retune` == `warm_no_reward_retune_no_low_prayer`
  - `cold_no_reward_retune` == `cold_no_reward_retune_no_low_prayer`
- this is now a trustworthy result, not a setup bug

Main findings:
- `low_prayer` shaping is inert in both warm and cold regimes
  - removing it produced effectively identical results at `3B`
  - warm family:
    - `59.06` vs `59.06` wave reached
    - `0.1458` vs `0.1458` reached wave 63
    - `0.00694` vs `0.00694` jad kill rate
  - cold family:
    - `29.01` vs `29.01` wave reached
    - `0.1970` vs `0.1970` cleared wave 30
    - `16.22` vs `16.22` pots wasted
- `reward_retune` remains important
  - warm-started:
    - `wave_reached: 59.06 -> 37.59`
    - `reached_wave_63: 0.1458 -> 0.0`
    - `jad_kill_rate: 0.00694 -> 0.0`
    - `prayer_uptime_magic: 0.4084 -> 0.1288`
    - `damage_blocked: 129918 -> 65959`
    - `prayer_switches: 330 -> 1113`
    - `attack_when_ready_rate: 0.7388 -> 0.3010`
    - `episode_return: 28652.2 -> 8244.8`
  - cold-started:
    - `wave_reached: 29.01 -> 29.80` improved slightly
    - but `cleared_wave_30: 0.1970 -> 0.0489` collapsed
    - `max_wave: 34 -> 33`
    - `prayer_uptime_magic: 0.0083 -> 0.0`
    - `attack_when_ready_rate: 0.5426 -> 0.5914`
    - `episode_return: 7141.8 -> 5216.6`
  - interpretation:
    - removing the reward-retune cluster does not stop the agent from reaching
      the late-20s, but it cripples conversion beyond wave 30 and heavily
      degrades the warm-started high-wave behavior

Observations:
- the warm-start effect is still enormous
  - `warm_control` is in the frontier regime
  - `cold_control` remains stuck around the late-20s / low-30s regime
  - this sweep confirms the current recipe still relies heavily on the
    `u58coupx` checkpoint for high-wave performance
- `warm_control` is meaningfully weaker than the full `v24` / `v22.1` control
  because this sweep stops at `3B`, not `5B`
  - `wave_reached: 59.06 vs 61.97`
  - `reached_wave_63: 0.1458 vs 0.3582`
  - `jad_kill_rate: 0.00694 vs 0.0149`
  - but it is clearly in the same regime and tracks the expected trajectory
- the reward-retune-off warm runs look qualitatively unstable
  - they still touch `max_wave = 63`, but without stable prayer control or
    consistent conversion into Jad kills
  - the huge jump in prayer switches (`330 -> 1113`) together with the major
    drop in attack tempo (`0.739 -> 0.301`) is the clearest regression signal
- cold runs continue to exhibit poor prayer potion usage
  - even the control family still uses around `30` pots at `avg_prayer_on_pot
    ≈ 0.684`
  - this remains a cold-start weakness, but it is separate from the config
    trimming question

Recommendation:
- remove the full low-prayer cluster next
  - this is now the clearest safe trim
  - both the `100M` validation and the corrected `3B` redo agree on this
- keep the reward-retune cluster
  - do **not** remove `w_damage_dealt`, `w_attack_attempt`, `w_npc_kill`, or
    `w_wave_clear` as a group
- if the goal remains “shrink config while preserving `v22.1` behavior,” the
  next clean step is:
  - `v25 = v24/v22.1 minus low_prayer shaping`
- if you want to keep trimming after that:
  - do not remove the reward-retune cluster wholesale
  - instead test individual trims inside the still-useful recipe

Bottom line:
- the corrected sweep worked
- the original `v24.s` conclusion about low-prayer being inert was correct, but
  only now is it trustworthy under a real warm/cold split
- the main live takeaway is simple:
  - drop `low_prayer`
  - keep `reward_retune`
  - checkpoint dependence is still strong

Current state:
- `v24.s_redo` is complete
- the section now contains both:
  - the `100M` validation proving the setup fix
  - the final corrected `3B` sweep results
- no further sweep rerun is needed for this question

---

## v24.s (2026-04-12, completed with a validity issue — multi-run; primary `6h5qnmjp`, see body for all IDs)

Actual runs:
- `6h5qnmjp`
  - `warm_control`
- `o7uzpppa`
  - `cold_control`
- `kngi2ik3`
  - `warm_no_low_prayer`
- `rrcabwv8`
  - `cold_no_low_prayer`
- `tv3uimai`
  - `warm_no_reward_retune`
- `d7cmxvwx`
  - `cold_no_reward_retune`
- `5sqv1hhk`
  - `warm_no_reward_retune_no_low_prayer`
- `1k4uygff`
  - `cold_no_reward_retune_no_low_prayer`

Status:
- completed through the full 8-run matrix
- all runs reached the planned `3B` budget
- W&B group used:
  - `v24_sweep`
- per-run W&B tags used:
  - the exact matrix labels above

Goal:
- trim the successful `v22.1` recipe down to the smallest config that still
  preserves its behavior
- answer two questions:
  - what can be removed from the `v22.1` recipe?
  - does any of that still help without warm-start?

Matrix that was launched:
- initialization axis:
  - nominal warm-start from `u58coupx/0000001311768576.bin`
  - nominal cold-start from scratch
- reward axis:
  - `control`
  - `no_low_prayer`
  - `no_reward_retune`
  - `no_reward_retune_no_low_prayer`

Critical validity issue:
- the configs and W&B labels were applied correctly
- however, the compiled `_C` training backend does **not** consume
  `load_model_path` during `train`
- evidence in code:
  - `train.sh` threads `LOAD_MODEL_PATH` into CLI args
  - the slow Python backend in
    [torch_pufferl.py](../../pufferlib_4/pufferlib/torch_pufferl.py#L499)
    does load `args['load_model_path']`
  - the compiled backend used by normal training creates its trainer in
    [bindings.cu](../../pufferlib_4/src/bindings.cu#L308)
    and never reads `load_model_path`
  - the compiled backend only exposes `load_weights(...)` as a callable API;
    `_train` does not invoke it for train mode
- practical result:
  - the `warm` and `cold` runs in this sweep were all effectively cold-start
  - the warm/cold axis of this sweep is therefore invalid
- this also means prior nominal warm-start training runs launched through the
  compiled trainer should be reinterpreted with caution until this is fixed

What remains valid:
- the reward configs themselves were applied correctly
- the sweep still gives a valid **cold-start 3B ablation screen** for:
  - `low_prayer shaping ON/OFF`
  - `reward_retune ON/OFF`

Exact config groups actually compared:

Control family:
- `w_damage_dealt = 0.7`
- `w_attack_attempt = 0.2`
- `w_npc_kill = 3.5`
- `w_wave_clear = 15.0`

Low-prayer cluster:
- ON:
  - `shape_low_prayer_no_pot_penalty = -0.01`
  - `shape_low_prayer_pot_reward = 0.15`
- OFF:
  - `shape_low_prayer_no_pot_penalty = 0.0`
  - `shape_low_prayer_pot_reward = 0.0`

Reward-retune-OFF family:
- `w_damage_dealt = 0.5`
- `w_attack_attempt = 0.1`
- `w_npc_kill = 3.0`
- `w_wave_clear = 10.0`

Results table:

| label | id | nominal init | score | wave_reached | max_wave | cleared_wave_30 | reached_wave_63 | jad_kill_rate | prayer_uptime_magic | damage_blocked | prayer_switches | attack_when_ready_rate | pots_used | avg_prayer_on_pot | pots_wasted |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| warm_control | `6h5qnmjp` | warm | `0.0` | `29.01` | `34` | `0.1970` | `0.0` | `0.0` | `0.0083` | `16133` | `126.8` | `0.5426` | `30.05` | `0.6839` | `16.22` |
| cold_control | `o7uzpppa` | cold | `0.0` | `29.01` | `34` | `0.1970` | `0.0` | `0.0` | `0.0083` | `16133` | `126.8` | `0.5426` | `30.05` | `0.6839` | `16.22` |
| warm_no_low_prayer | `kngi2ik3` | warm | `0.0` | `29.01` | `34` | `0.1970` | `0.0` | `0.0` | `0.0083` | `16133` | `126.8` | `0.5426` | `30.05` | `0.6839` | `16.22` |
| cold_no_low_prayer | `rrcabwv8` | cold | `0.0` | `29.01` | `34` | `0.1970` | `0.0` | `0.0` | `0.0083` | `16133` | `126.8` | `0.5426` | `30.05` | `0.6839` | `16.22` |
| warm_no_reward_retune | `tv3uimai` | warm | `0.0` | `29.80` | `33` | `0.0489` | `0.0` | `0.0` | `0.0000` | `20508` | `628.7` | `0.5914` | `26.36` | `0.5923` | `10.46` |
| cold_no_reward_retune | `d7cmxvwx` | cold | `0.0` | `29.80` | `33` | `0.0489` | `0.0` | `0.0` | `0.0000` | `20508` | `628.7` | `0.5914` | `26.36` | `0.5923` | `10.46` |
| warm_no_reward_retune_no_low_prayer | `5sqv1hhk` | warm | `0.0` | `29.80` | `33` | `0.0489` | `0.0` | `0.0` | `0.0000` | `20508` | `628.7` | `0.5914` | `26.36` | `0.5923` | `10.46` |
| cold_no_reward_retune_no_low_prayer | `1k4uygff` | cold | `0.0` | `29.80` | `33` | `0.0489` | `0.0` | `0.0` | `0.0000` | `20508` | `628.7` | `0.5914` | `26.36` | `0.5923` | `10.46` |

Observed collapse pattern:
- the sweep did **not** produce eight distinct outcomes
- it produced only **two** distinct result signatures:
  - control-family signature:
    - `control`
    - `no_low_prayer`
  - reward-retune-off signature:
    - `no_reward_retune`
    - `no_reward_retune_no_low_prayer`
- within each signature:
  - warm/cold were effectively identical
  - low-prayer ON/OFF was effectively identical

What worked:
- the sweep runner itself worked
- configs were applied correctly
- W&B grouping/tagging worked
- the cold-start `3B` screen answered one real question:
  - the low-prayer cluster had no measurable effect in this budget/regime

What did not work:
- the nominal warm-start axis was invalid because compiled training ignored
  `load_model_path`
- the sweep therefore did **not** answer the checkpoint-dependence question
- the `3B` budget was too short to reproduce the `v22.1` frontier regardless;
  all runs remained in the late-20s / low-30s regime

Main read:
- `low_prayer shaping` appears inert in this screen
  - `control` and `no_low_prayer` were effectively identical
  - `no_reward_retune` and `no_reward_retune_no_low_prayer` were effectively
    identical
  - interpretation:
    - at `3B` cold-start, the agent is not learning in a regime where the
      low-prayer rescue terms materially change behavior
- `reward_retune` still matters
  - removing it did **not** improve real progress
  - the reward-retune-off family:
    - touched `wave 30` more often (`0.853 vs 0.625`)
    - but cleared `wave 30` far less often (`0.0489 vs 0.1970`)
    - had lower `max_wave` (`33 vs 34`)
    - had much lower `episode_return` (`5216.6 vs 7141.8`)
    - collapsed magic-prayer usage to `0.0`
  - interpretation:
    - the reward-retune cluster looks important for converting late-20s
      progress into actual wave-30 clears
    - it should be kept for now

Observations:
- none of the `3B` runs reached the `v22.1` frontier regime
- all runs had `score = 0.0`, `reached_wave_63 = 0.0`, `jad_kill_rate = 0.0`
- even the better control-family runs still showed resource slop:
  - `pots_used = 30.05`
  - `avg_prayer_on_pot = 0.684`
  - `pots_wasted = 16.22`
- this means the `3B` screen is mainly useful for eliminating clearly inert
  knobs, not for proving recipe sufficiency

Recommendations:
- keep the reward-retune cluster for now
  - do **not** remove `w_damage_dealt`, `w_attack_attempt`, `w_npc_kill`, or
    `w_wave_clear` yet
- low-prayer shaping is the first removal candidate
  - the current evidence says it is safe to test dropping that cluster next
  - but treat this as a `3B` cold-start finding, not yet a final `5B`
    frontier conclusion
- before doing any future warm/cold sweep:
  - fix compiled train-mode warm-start so `_C.create_pufferl` actually loads
    `load_model_path`
  - then rerun the smallest sanity check:
    - `warm_control`
    - `cold_control`
  - only after those diverge meaningfully should the warm/cold axis be trusted
- next efficient path:
  - either:
    - run a `5B` exact ablation of `low_prayer OFF` against the `v24` control
    - or:
      - fix train-mode warm-start first, then rerun just the 4 nominal
        warm-started ablations

Bottom line:
- this sweep did **not** validate warm-start dependence
- it **did** show that the low-prayer cluster is probably removable before the
  reward-retune cluster
- it also uncovered a larger infrastructure bug:
  - compiled training has likely been ignoring `LOAD_MODEL_PATH` entirely
  - that should be fixed before any future checkpoint-attribution claims are
    treated as settled

---

## v24 (2026-04-11, completed — `h2kpwkdk`)

Actual run:
- `h2kpwkdk`
- W&B path:
  - `jordanbaileypmp-georgia-institute-of-technology/puffer4/h2kpwkdk`
- local run log:
  - `h2kpwkdk.json` (not retained locally)

Status:
- completed to the full `5B` budget
- W&B tagged `v24`

Goal:
- do an exact rerun of `v22.1`
- remove all `v23` / `v23.1` code-path and config changes
- keep the same warm-start source, PPO recipe, corrected combat model, and
  reward stack that produced `721zk2cg`
- use this as the control rerun before any further ablations

Config versus `v22.1`:
- no intended code delta
- no intended config delta
- same warm-start source:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`
- same corrected `Masori (f) + TBow` combat model
- same attack-ready penalty semantics as the original `v22.1`
- no 1-tick flick block bonus

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_low_prayer_pot_threshold=0.0`, `shape_low_prayer_no_pot_penalty=-0.01`, `shape_low_prayer_pot_reward=0.15`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `anneal_lr=0`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Expected read:
- this is the exact control rerun for `v22.1`
- if it returns to the `61-63` wave regime, the `v23` / `v23.1` regressions
  were caused by the bundled reward/code changes rather than variance
- if it does not, then the next suspicion is hidden environment drift rather
  than the explicit `v23` deltas

Results:
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2574s`
- throughput:
  - `1.85M SPS`

Final metrics:
- `score = 0.0217`
- `cave_complete_rate = 0.0217`
- `wave_reached = 61.97`
- `max_wave = 63`
- `most_npcs_slayed = 325`
- `episode_return = 32277.45`
- `episode_length = 8581.45`
- `reached_wave_30 = 1.0`
- `cleared_wave_30 = 1.0`
- `reached_wave_31 = 1.0`
- `reached_wave_63 = 0.3582`
- `jad_kill_rate = 0.0149`
- `prayer_uptime = 0.7345`
- `prayer_uptime_melee = 0.0322`
- `prayer_uptime_range = 0.1727`
- `prayer_uptime_magic = 0.5296`
- `correct_prayer = 1550.93`
- `wrong_prayer_hits = 296.24`
- `no_prayer_hits = 29.76`
- `damage_blocked = 187216.98`
- `dmg_taken_avg = 6445.69`
- `prayer_switches = 651.28`
- `attack_when_ready_rate = 0.7500`
- `pots_used = 31.36`
- `avg_prayer_on_pot = 0.3760`
- `food_eaten = 8.01`
- `avg_hp_on_food = 0.5725`
- `food_wasted = 1.33`
- `pots_wasted = 3.07`
- `tokxil_melee_ticks = 3.85`
- `ketzek_melee_ticks = 5.36`

Key progression points:
- this rerun reproduced the `v22.1` frontier trajectory almost exactly
- strong mid-run breakout:
  - `1.876B`: `wave_reached = 50.5`, `episode_return = 21952.0`
- frontier competence was established by the mid-run sample:
  - `3.126B`: `wave_reached = 61.3`, `episode_return = 31342.2`
- the late window stayed in the same `~62` regime:
  - `4.376B`: `wave_reached = 61.98`, `episode_return = 32250.6`
  - `5.000B`: `wave_reached = 61.97`, `episode_return = 32277.4`

Comparison to `v22.1` (`721zk2cg`):
- `score: 0.0217 vs 0.0217`
- `cave_complete_rate: 0.0217 vs 0.0217`
- `wave_reached: 61.97 vs 61.97`
- `max_wave: 63 vs 63`
- `most_npcs_slayed: 325 vs 325`
- `episode_return: 32277.4 vs 32277.4`
- `episode_length: 8581 vs 8581`
- `reached_wave_63: 0.3582 vs 0.3582`
- `jad_kill_rate: 0.0149 vs 0.0149`
- `prayer_uptime_magic: 0.5296 vs 0.5296`
- `damage_blocked: 187217 vs 187217`
- `prayer_switches: 651.3 vs 651.3`
- `attack_when_ready_rate: 0.7500 vs 0.7500`
- `avg_prayer_on_pot: 0.3760 vs 0.3760`
- `pots_wasted: 3.07 vs 3.07`

Main read:
- `v24` successfully reproduces the `v22.1` branch
- this clears the main uncertainty around hidden repo drift
- the `v22.1` behavior is reproducible on the current codebase when the repo is
  set back to the correct codepath and live config
- `v24` should be treated as the control baseline for all `v24.s` trimming
  ablations

Checkpoint notes:
- early eval checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/h2kpwkdk/0000001888485376.bin`
- frontier checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/h2kpwkdk/0000003146776576.bin`
- late stable checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/h2kpwkdk/0000004614782976.bin`
- final checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/h2kpwkdk/0000004999610368.bin`

---

## v23.1 (2026-04-11, completed — `fdzlxlwm`)

Actual run:
- `fdzlxlwm`
- W&B path:
  - `jordanbaileypmp-georgia-institute-of-technology/puffer4/fdzlxlwm`
- local run log:
  - `fdzlxlwm.json` (not retained locally)

Status:
- completed to the full `5B` budget
- W&B tagged `v23.1`

Goal:
- keep the full `v23` numeric config exactly as-is
- keep the same warm-start source, PPO recipe, corrected combat model, and
  attack-ready timing semantics
- keep the same `shape_one_tick_prayer_block_bonus = 0.1`
- narrow the new flick reward semantics so it only pays on:
  - blocked 1-tick prayer streak
  - followed by `PRAYER_OFF`
- explicitly do not reward prayer-to-prayer switches
- make no LOS, Jad, healer, or combat-model changes in this run

Config versus `v23`:
- identical reward weights
- identical shaping values
- identical runtime / PPO / policy config
- identical warm-start source:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`
- only code-path delta:
  - `shape_one_tick_prayer_block_bonus` now fires only when a blocked
    1-tick prayer streak ends by switching to `OFF`
  - prayer-to-prayer transitions no longer qualify

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=0.0`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_low_prayer_pot_threshold=0.0`, `shape_low_prayer_no_pot_penalty=-0.01`, `shape_low_prayer_pot_reward=0.15`, `shape_wrong_prayer_penalty=-1.25`, `shape_one_tick_prayer_block_bonus=0.1`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `anneal_lr=0`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Expected read:
- this isolates the strongest suspected root cause from `v23`
- if near-full prayer-pot use falls back toward `v22.1` behavior, the original
  `v23` regression was mainly caused by paying the flick bonus on
  prayer-to-prayer switches
- if the regression remains, the next most likely ablations are:
  - restore `w_tick_penalty = -0.005`
  - revert the attack-ready timing change

Results (`fdzlxlwm`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2438s`
- throughput:
  - `1.91M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 43.02`
- `max_wave = 49`
- `most_npcs_slayed = 198`
- `episode_return = 15835.42`
- `episode_length = 6193.56`
- `reached_wave_30 = 1.0`
- `cleared_wave_30 = 0.9879`
- `reached_wave_31 = 0.9879`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.3247`
- `prayer_uptime_melee = 0.0421`
- `prayer_uptime_range = 0.1375`
- `prayer_uptime_magic = 0.1451`
- `correct_prayer = 1174.14`
- `wrong_prayer_hits = 117.98`
- `no_prayer_hits = 103.39`
- `damage_blocked = 89391.76`
- `dmg_taken_avg = 4090.91`
- `prayer_switches = 2183.79`
- `attack_when_ready_rate = 0.3859`
- `pots_used = 32.0`
- `avg_prayer_on_pot = 0.9721`
- `food_eaten = 19.99`
- `avg_hp_on_food = 0.8887`
- `food_wasted = 16.54`
- `pots_wasted = 31.26`
- `tokxil_melee_ticks = 7.34`
- `ketzek_melee_ticks = 7.02`

Key progression points:
- unlike `v23`, this run did keep climbing through the full budget:
  - `0.627B`: `wave_reached = 29.56`, `episode_return = 7195.1`
  - `1.876B`: `wave_reached = 30.89`, `episode_return = 8105.4`
  - `3.126B`: `wave_reached = 35.60`, `episode_return = 10787.9`
  - `4.376B`: `wave_reached = 41.51`, `episode_return = 14634.1`
  - `5.000B`: `wave_reached = 43.02`, `episode_return = 15835.4`
- it restored some late-wave magic-prayer behavior, but never reached the
  `v22.1` frontier window
- `score`, `reached_wave_63`, and `jad_kill_rate` stayed at `0.0` throughout

Comparison to `v23` (`yq3tbole`):
- `wave_reached: 43.02 vs 31.15`
- `max_wave: 49 vs 35`
- `most_npcs_slayed: 198 vs 130`
- `episode_return: 15835.4 vs 8585.6`
- `prayer_uptime_magic: 0.1451 vs 0.0`
- `correct_prayer: 1174.1 vs 812.6`
- `damage_blocked: 89392 vs 12065`
- `prayer_switches: 2183.8 vs 1503.5`
- `attack_when_ready_rate: 0.3859 vs 0.5722`
- `avg_prayer_on_pot: 0.9721 vs 0.9260`
- `pots_wasted: 31.26 vs 30.54`
- `food_eaten: 19.99 vs 8.18`
- `food_wasted: 16.54 vs 6.01`

Comparison to `v22.1` (`721zk2cg`):
- `score: 0.0 vs 0.0217`
- `cave_complete_rate: 0.0 vs 0.0217`
- `wave_reached: 43.02 vs 61.97`
- `max_wave: 49 vs 63`
- `most_npcs_slayed: 198 vs 325`
- `episode_return: 15835.4 vs 32277.4`
- `episode_length: 6194 vs 8581`
- `reached_wave_30: 1.0 vs 1.0`
- `cleared_wave_30: 0.9879 vs 1.0`
- `reached_wave_31: 0.9879 vs 1.0`
- `reached_wave_63: 0.0 vs 0.3582`
- `jad_kill_rate: 0.0 vs 0.0149`
- `prayer_uptime_magic: 0.1451 vs 0.5296`
- `damage_blocked: 89392 vs 187217`
- `prayer_switches: 2183.8 vs 651.3`
- `attack_when_ready_rate: 0.3859 vs 0.7500`
- `avg_prayer_on_pot: 0.9721 vs 0.3760`
- `pots_wasted: 31.26 vs 3.07`
- `food_eaten: 19.99 vs 8.01`
- `food_wasted: 16.54 vs 1.33`
- `no_prayer_hits: 103.39 vs 29.76`

Main read:
- narrowing the flick bonus to only pay on `1-tick -> OFF` did remove the
  direct prayer-to-prayer reward exploit
- that recovered some magic-prayer usage and pushed the run well past the
  `v23` wall
- but it did not fix the overall resource collapse:
  - the agent still potted at near-full prayer
  - prayer switches increased even further
  - attack tempo got worse
  - food timing also regressed badly
- this means `v23.1` is still a clear regression from `v22.1`, even though it
  is materially better than `v23`

Checkpoint notes:
- early eval checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/fdzlxlwm/0000000630194176.bin`
- mid-run breakout checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/fdzlxlwm/0000003146776576.bin`
- late frontier checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/fdzlxlwm/0000004352638976.bin`
- final checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/fdzlxlwm/0000004999610368.bin`

---

## v23 (2026-04-11, completed — `yq3tbole`)

Actual run:
- `yq3tbole`
- W&B path:
  - `jordanbaileypmp-georgia-institute-of-technology/puffer4/yq3tbole`
- local run log:
  - `yq3tbole.json` (not retained locally)

Status:
- completed to the full `5B` budget
- W&B tagged `v23`

Goal:
- keep the `v22.1` recipe, warm-start source, PPO settings, and corrected
  `Masori (f) + TBow` combat model
- remove the global per-tick penalty entirely
- fix the attack-ready stall timing so penalties do not fire on the exact tick
  attack cooldown reaches `0`
- add one small positive signal for an exact 1-tick prayer activation that
  successfully blocks a hit
- make no LOS, Jad, healer, or combat-model changes in this run

Config versus `v22.1`:
- same PPO/train recipe
- same `5B` budget
- same backend structure
- same viewer / reward-parity / analytics infrastructure
- same corrected `Masori (f) + TBow` combat model
- same warm-start source:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`
- same reward weights and shaping except:
  - `w_tick_penalty: -0.005 -> 0.0`
  - add `shape_one_tick_prayer_block_bonus = 0.1`
- code-path delta versus `v22.1`:
  - attack-ready penalty semantics changed from immediate ready-state penalty
    to:
    - first ready-no-attack tick: no penalty
    - second ready-no-attack tick: `wasted_attack`
    - third and later ready-no-attack ticks: `not_attacking`
- explicitly not included:
  - LOS fixes
  - Jad safespot fixes
  - healer reward / penalty changes
  - further combat-model changes

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=0.0`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_low_prayer_pot_threshold=0.0`, `shape_low_prayer_no_pot_penalty=-0.01`, `shape_low_prayer_pot_reward=0.15`, `shape_wrong_prayer_penalty=-1.25`, `shape_one_tick_prayer_block_bonus=0.1`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `anneal_lr=0`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`yq3tbole`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2431s`
- throughput:
  - `1.98M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 31.15`
- `max_wave = 35`
- `most_npcs_slayed = 130`
- `episode_return = 8585.58`
- `episode_length = 3957.24`
- `reached_wave_30 = 0.9810`
- `cleared_wave_30 = 0.9734`
- `reached_wave_31 = 0.9734`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.5904`
- `prayer_uptime_melee = 0.1494`
- `prayer_uptime_range = 0.4410`
- `prayer_uptime_magic = 0.0`
- `correct_prayer = 812.58`
- `wrong_prayer_hits = 49.98`
- `no_prayer_hits = 32.47`
- `damage_blocked = 12064.99`
- `dmg_taken_avg = 2486.96`
- `prayer_switches = 1503.53`
- `attack_when_ready_rate = 0.5722`
- `pots_used = 32.0`
- `avg_prayer_on_pot = 0.9260`
- `food_eaten = 8.18`
- `avg_hp_on_food = 0.8625`
- `food_wasted = 6.01`
- `pots_wasted = 30.54`
- `tokxil_melee_ticks = 4.51`
- `ketzek_melee_ticks = 0.52`

Key progression points:
- there was no `v22.1`-style breakout window anywhere in the run
- the run stayed below wave `30` through the first `~3.1B` steps:
  - `0.627B`: `wave_reached = 25.1`, `episode_return = 5208.7`
  - `1.876B`: `wave_reached = 27.0`, `episode_return = 6072.2`
  - `3.126B`: `wave_reached = 29.1`, `episode_return = 7370.8`
- the best sampled window arrived late, but only reached low-wave-`31`:
  - `4.376B`: `wave_reached = 31.24`, `episode_return = 8654.7`
  - `5.000B`: `wave_reached = 31.15`, `episode_return = 8585.6`
- `score`, `reached_wave_63`, and `jad_kill_rate` stayed at `0.0` across the
  whole sampled trajectory
- this run did eventually become a consistent wave-30 clearer, but never found
  a wave-31-plus frontier policy

Comparison to `v22.1` (`721zk2cg`):
- `score: 0.0 vs 0.0217`
- `cave_complete_rate: 0.0 vs 0.0217`
- `wave_reached: 31.15 vs 61.97`
- `max_wave: 35 vs 63`
- `most_npcs_slayed: 130 vs 325`
- `episode_return: 8585.6 vs 32277.4`
- `episode_length: 3957 vs 8581`
- `reached_wave_30: 0.9810 vs 1.0`
- `cleared_wave_30: 0.9734 vs 1.0`
- `reached_wave_31: 0.9734 vs 1.0`
- `reached_wave_63: 0.0 vs 0.3582`
- `jad_kill_rate: 0.0 vs 0.0149`
- `prayer_uptime: 0.5904 vs 0.7345`
- `prayer_uptime_melee: 0.1494 vs 0.0322`
- `prayer_uptime_range: 0.4410 vs 0.1727`
- `prayer_uptime_magic: 0.0 vs 0.5296`
- `correct_prayer: 812.6 vs 1550.9`
- `no_prayer_hits: 32.5 vs 29.8`
- `damage_blocked: 12065.0 vs 187217.0`
- `prayer_switches: 1503.5 vs 651.3`
- `attack_when_ready_rate: 0.5722 vs 0.7500`
- `avg_prayer_on_pot: 0.9260 vs 0.3760`
- `pots_wasted: 30.54 vs 3.07`
- `avg_hp_on_food: 0.8625 vs 0.5725`
- `food_wasted: 6.01 vs 1.33`

Main read:
- the combined `v23` change set was a hard regression
- the most diagnostic signatures were:
  - zero `protect from magic` usage (`prayer_uptime_magic = 0.0`)
  - much lower attack tempo (`attack_when_ready_rate = 0.5722`)
  - much higher switch count with much lower blocked damage
  - near-full prayer-pot usage (`avg_prayer_on_pot = 0.9260`)
  - much earlier food usage (`avg_hp_on_food = 0.8625`)
- this suggests the `v23` incentives shifted the policy toward noisy
  prayer-on/prayer-switch behavior and worse resource timing, without producing
  the high-wave prayer-control behavior that `v22.1` inherited from the warm
  start
- because `v23` bundled three changes at once, attribution is still confounded:
  - zeroed `w_tick_penalty`
  - added `shape_one_tick_prayer_block_bonus`
  - delayed the attack-ready stall penalties by one tick

Checkpoint notes:
- first eval checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yq3tbole/0000001259339776.bin`
- best sampled-window checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yq3tbole/0000004352638976.bin`
- final checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/yq3tbole/0000004999610368.bin`

---

## v21.3 (2026-04-10, completed but needs to be analyze)

Actual run:
- qiddrsmv
Path
- jordanbaileypmp-georgia-institute-of-technology/puffer4/runs/qiddrsmv

Goal:
- return to the `v21.2` learning recipe as the planning baseline
- keep `v21.2` as the clean reference point while adding only a very narrow
  prayer-potion rescue signal
- use this as the fallback branch if the `v22.x` line does not recover
- keep the corrected `Masori (f) + TBow` combat model and `7/10` kiting band
  from `v22.1`

Config versus `v21.2`:
- same baseline except for one narrow low-prayer potion rule
- same PPO/train recipe
- same `5B` budget
- same backend structure
- same reward weights and shaping terms
- same vectorization / minibatching
- same no-curriculum setup
- same intended learning behavior aside from emergency prayer-pot use
- keep the `v22.1` combat model and kiting distance

Exact starting config:
- reward weights:
  - `w_damage_dealt = 0.5`
  - `w_attack_attempt = 0.1`
  - `w_damage_taken = -0.6`
  - `w_npc_kill = 3.0`
  - `w_wave_clear = 10.0`
  - `w_jad_damage = 2.0`
  - `w_jad_kill = 50.0`
  - `w_player_death = -20.0`
  - `w_cave_complete = 100.0`
  - `w_correct_danger_prayer = 0.25`
  - `w_wrong_danger_prayer = 0.0`
  - `w_invalid_action = -0.1`
  - `w_movement = 0.0`
  - `w_idle = 0.0`
  - `w_tick_penalty = -0.005`
- shaping terms:
  - `shape_food_full_waste_penalty = -6.5`
  - `shape_food_waste_scale = -1.2`
  - `shape_food_safe_hp_threshold = 1.0`
  - `shape_food_no_threat_penalty = 0.0`
  - `shape_pot_full_waste_penalty = -6.5`
  - `shape_pot_waste_scale = -1.2`
  - `shape_pot_safe_prayer_threshold = 1.0`
  - `shape_pot_no_threat_penalty = 0.0`
  - `shape_low_prayer_pot_threshold = 0.05`
  - `shape_low_prayer_no_pot_penalty = -0.01`
  - `shape_low_prayer_pot_reward = 0.0`
  - `shape_wrong_prayer_penalty = -1.25`
  - `shape_npc_specific_prayer_bonus = 1.5`
  - `shape_npc_melee_penalty = -0.3`
  - `shape_wasted_attack_penalty = -0.1`
  - `shape_wave_stall_start = 500`
  - `shape_wave_stall_base_penalty = -0.5`
  - `shape_wave_stall_ramp_interval = 50`
  - `shape_wave_stall_cap = -2.0`
  - `shape_not_attacking_grace_ticks = 2`
  - `shape_not_attacking_penalty = -0.01`
  - `shape_kiting_reward = 1.0`
  - `shape_kiting_min_dist = 7`
  - `shape_kiting_max_dist = 10`
  - `shape_unnecessary_prayer_penalty = -0.2`
  - `shape_resource_threat_window = 2`
- runtime:
  - `total_agents = 4096`
  - `num_buffers = 2`
  - `total_timesteps = 5_000_000_000`
  - `learning_rate = 0.0003`
  - `anneal_lr = 0`
  - `gamma = 0.999`
  - `gae_lambda = 0.95`
  - `clip_coef = 0.2`
  - `vf_coef = 0.5`
  - `ent_coef = 0.01`
  - `max_grad_norm = 0.5`
  - `horizon = 256`
  - `minibatch_size = 4096`
  - `hidden_size = 256`
  - `num_layers = 2`

Current planning stance:
- selected tweak set is intentionally tiny:
  - `shape_low_prayer_pot_threshold = 0.05`
  - `shape_low_prayer_no_pot_penalty = -0.01`
  - `shape_low_prayer_pot_reward = 0.0`
- no checkpoint warm-start
- keep the `v22.1` combat model and `7/10` kiting band
- do not fold in the `v22` / `v22.1` reward retune or obs changes

---

## v22.1 (2026-04-11, completed — `721zk2cg`)

Actual run:
- `721zk2cg`
- local run log:
  - `721zk2cg.json` (not retained locally)

Status:
- completed to the full `5B` budget

Goal:
- recover from the `v22` collapse without giving up the corrected `Masori (f) +
  TBow` combat model
- remove the added per-NPC identity observation from `v22`
- keep the `v22` low-prayer shaping path, but retune it to exact zero prayer
- increase combat-tempo / conversion rewards
- widen the rewarded kiting band for the corrected TBow range
- warm-start from a strong `v21.2` checkpoint

Config versus `v22`:
- same PPO/train recipe
- same `5B` budget
- same backend structure
- same viewer / reward-parity / analytics infrastructure
- same corrected `Masori (f) + TBow` player combat model
- intended learning-behavior changes were:
  - remove the added per-NPC `npc_type` observation channel
  - move the low-prayer shaping threshold from `59` prayer to exact `0`
  - keep `shape_low_prayer_no_pot_penalty = -0.01`
  - keep `shape_low_prayer_pot_reward = 0.15`
  - increase:
    - `w_damage_dealt: 0.5 -> 0.7`
    - `w_attack_attempt: 0.1 -> 0.2`
    - `w_npc_kill: 3.0 -> 3.5`
    - `w_wave_clear: 10.0 -> 15.0`
  - widen the kiting band:
    - `shape_kiting_min_dist: 5 -> 7`
    - `shape_kiting_max_dist: 7 -> 10`
  - warm-start from:
    - `<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000001311768576.bin`, corrected `Masori (f) + TBow` combat model, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.7`, `w_attack_attempt=0.2`, `w_damage_taken=-0.6`, `w_npc_kill=3.5`, `w_wave_clear=15.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_low_prayer_pot_threshold=0.0`, `shape_low_prayer_no_pot_penalty=-0.01`, `shape_low_prayer_pot_reward=0.15`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=7`, `shape_kiting_max_dist=10`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`721zk2cg`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2634s`
- throughput:
  - `1.83M SPS`

Final metrics:
- `score = 0.0217`
- `cave_complete_rate = 0.0217`
- `wave_reached = 61.97`
- `max_wave = 63`
- `most_npcs_slayed = 325`
- `episode_return = 32277.45`
- `episode_length = 8581.45`
- `reached_wave_30 = 1.0`
- `cleared_wave_30 = 1.0`
- `reached_wave_31 = 1.0`
- `reached_wave_63 = 0.3582`
- `jad_kill_rate = 0.0149`
- `prayer_uptime = 0.7345`
- `prayer_uptime_melee = 0.0322`
- `prayer_uptime_range = 0.1727`
- `prayer_uptime_magic = 0.5296`
- `correct_prayer = 1550.93`
- `wrong_prayer_hits = 296.24`
- `no_prayer_hits = 29.76`
- `damage_blocked = 187217.02`
- `dmg_taken_avg = 6445.69`
- `prayer_switches = 651.28`
- `attack_when_ready_rate = 0.7500`
- `pots_used = 31.36`
- `avg_prayer_on_pot = 0.3760`
- `food_eaten = 8.01`
- `avg_hp_on_food = 0.5725`
- `food_wasted = 1.33`
- `pots_wasted = 3.07`
- `tokxil_melee_ticks = 3.85`
- `ketzek_melee_ticks = 5.36`

Key progression points:
- early climb was solid but not special through `~1.5B`:
  - `1.248B`: `wave_reached = 33.5`, `episode_return = 9219.3`
  - `1.516B`: `wave_reached = 35.0`, `episode_return = 10056.0`
- the major jump happened between `1.52B` and `1.82B`:
  - `1.818B`: `wave_reached = 54.2`, `episode_return = 24812.8`
- frontier competence arrived soon after:
  - `2.072B`: `wave_reached = 59.6`, `episode_return = 28984.9`
  - `2.284B`: `wave_reached = 61.4`, `episode_return = 30993.2`
- unlike `v22` and `v21.2`, there was no catastrophic late collapse
- the run stabilized in a `61-62.5` wave regime for the rest of training
- best sampled window was late:
  - `4.127B`: `wave_reached = 62.3`, `episode_return = 32340.6`
  - `4.627B`: `wave_reached = 62.5`, `episode_return = 32827.5`
- nearest eval checkpoint to the best sampled window:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/721zk2cg/0000004614782976.bin`

Comparison to `v22` (`7vuw9jy8`):
- `score: 0.0217 vs 0.0`
- `cave_complete_rate: 0.0217 vs 0.0`
- `wave_reached: 61.97 vs 27.15`
- `max_wave: 63 vs 33`
- `most_npcs_slayed: 325 vs 123`
- `episode_return: 32277.4 vs 4338.1`
- `episode_length: 8581 vs 4386`
- `reached_wave_30: 1.0 vs 0.0648`
- `cleared_wave_30: 1.0 vs 0.0`
- `reached_wave_31: 1.0 vs 0.0`
- `reached_wave_63: 0.3582 vs 0.0`
- `jad_kill_rate: 0.0149 vs 0.0`
- `prayer_uptime: 0.7345 vs 0.9758`
- `prayer_uptime_magic: 0.5296 vs 0.0`
- `attack_when_ready_rate: 0.7500 vs 0.4557`
- `pots_used: 31.36 vs 31.96`
- `avg_prayer_on_pot: 0.3760 vs 0.6929`
- `food_eaten: 8.01 vs 20.0`
- `food_wasted: 1.33 vs 17.09`

Comparison to `v21.2` (`u58coupx`):
- `score: 0.0217 vs 0.0`
- `cave_complete_rate: 0.0217 vs 0.0`
- `wave_reached: 61.97 vs 48.65`
- `max_wave: 63 vs 63`
- `most_npcs_slayed: 325 vs 272`
- `episode_return: 32277.4 vs 14307.1`
- `episode_length: 8581 vs 6493`
- `reached_wave_30: 1.0 vs 0.9733`
- `cleared_wave_30: 1.0 vs 0.9733`
- `reached_wave_31: 1.0 vs 0.9733`
- `reached_wave_63: 0.3582 vs 0.0067`
- `jad_kill_rate: 0.0149 vs 0.0`
- `prayer_uptime: 0.7345 vs 0.2638`
- `prayer_uptime_magic: 0.5296 vs 0.1228`
- `correct_prayer: 1550.9 vs 1143.9`
- `wrong_prayer_hits: 296.2 vs 98.4`
- `no_prayer_hits: 29.8 vs 155.6`
- `damage_blocked: 187217 vs 139493`
- `dmg_taken_avg: 6445.7 vs 6818.0`
- `prayer_switches: 651.3 vs 2284.7`
- `attack_when_ready_rate: 0.7500 vs 0.6329`
- `pots_used: 31.36 vs 0.02`
- `food_eaten: 8.01 vs 15.89`
- `food_wasted: 1.33 vs 4.33`

Comparison to `v21` (`v4ekkk3z`):
- `score: 0.0217 vs 0.0`
- `wave_reached: 61.97 vs 61.40`
- `max_wave: 63 vs 63`
- `most_npcs_slayed: 325 vs 273`
- `episode_return: 32277.4 vs 22940.3`
- `episode_length: 8581 vs 10027`
- `prayer_uptime: 0.7345 vs 0.7345`
- `prayer_uptime_magic: 0.5296 vs 0.5068`
- `correct_prayer: 1550.9 vs 2173.5`
- `wrong_prayer_hits: 296.2 vs 281.5`
- `no_prayer_hits: 29.8 vs 37.5`
- `damage_blocked: 187217 vs 277395`
- `dmg_taken_avg: 6445.7 vs 7028.8`
- `prayer_switches: 651.3 vs 2725.4`
- `attack_when_ready_rate: 0.7500 vs 0.6226`
- `pots_used: 31.36 vs 32.0`
- `avg_prayer_on_pot: 0.3760 vs 0.5754`
- `food_eaten: 8.01 vs 16.94`
- `food_wasted: 1.33 vs 9.02`
- `tokxil_melee_ticks: 3.85 vs 10.76`
- `ketzek_melee_ticks: 5.36 vs 18.08`

Interpretation:
- `v22.1` is a decisive recovery from `v22`
- it does not look like the `v22` prayer-camping collapse at all
- it also clearly beats `v21.2`
- and on the logged metrics that matter most, it likely becomes the new best
  branch so far:
  - first non-zero `cave_complete_rate`
  - first non-zero `jad_kill_rate`
  - sustained `61-62.5` wave regime
  - full wave-30+ stability
- the potion issue is effectively solved in the narrow sense that the agent now
  absolutely knows how to consume prayer pots
- however, it did not learn conservative potion timing:
  - `pots_used = 31.36`
  - `avg_prayer_on_pot = 0.376`
- the food side is much cleaner:
  - only `8.0` food eaten on average
  - much lower waste than both `v22` and `v21`
- the prayer profile is notable:
  - total prayer uptime is almost identical to `v21`
  - magic prayer dominates the uptime mix
  - prayer switches are far lower than `v21`
- that suggests a different high-level policy style than `v21`:
  - more tempo / damage conversion
  - fewer switches
  - fewer panic resources
  - still enough prayer correctness to reach and sometimes finish Jad

Important observation:
- there is a small analytics inconsistency:
  - `cave_complete_rate = 0.0217`
  - `jad_kill_rate = 0.0149`
- logically, cave completion should imply Jad kill
- so `jad_kill_rate` appears to be undercounting relative to terminal cave
  completions in this run
- the run result itself is still clear because `score` / `cave_complete_rate`
  is authoritative for clears, but the new `jad_kill_rate` metric should be
  audited before relying on it as ground truth

Most important new insight:
- the `v22` recovery cannot be attributed cleanly to one change
- `v22.1` changed five things at once relative to `v22`:
  - removed `npc_type` obs
  - changed low-prayer threshold to zero
  - increased combat-conversion rewards
  - widened kiting distance
  - warm-started from `v21.2`
- so this is not a clean ablation
- still, the result is strong enough that the branch itself is worth keeping
- the biggest practical conclusion is:
  - do not abandon the `v22.x` line yet

Recommendations:
- keep `v22.1` as the new working branch
- do not jump straight to `v21.3` as the mainline follow-up
- if the next goal is optimization rather than fallback validation, run a small
  ablation sweep around `v22.1` first
- if the next goal is simplicity / safety, only then use `v21.3` as the
  fallback control branch

Bottom line:
- `v22.1` is a considerable success
- it fully recovers the `v22` collapse
- it beats `v21.2`
- and it likely beats `v21` as the best overall run family so far, because it
  is the first branch with non-zero cave completion

---

## v22 (2026-04-10, completed — `7vuw9jy8`)

Actual run:
- `7vuw9jy8`
- W&B run name:
  - `good-planet-129`
- local run log:
  - `7vuw9jy8.json` (not retained locally)

Status:
- completed to the full `5B` budget

Goal:
- keep the `v21` / `v21.2` PPO stack and overall reward recipe
- fix the player-side combat-model mismatch for the active `Masori (f) + TBow`
  loadout
- give the policy explicit NPC identity in obs so late-wave prayer decisions are
  less slot-order-dependent
- make prayer-potion timing directly teachable without adding heavy or invasive
  shaping

Config versus `v21.2`:
- same PPO/train recipe
- same `5B` budget
- same backend structure
- same viewer / reward-parity / analytics infrastructure
- intended learning-behavior changes were:
  - correct the active player weapon/loadout model
  - add one per-NPC `npc_type` observation channel
  - add a lightweight low-prayer potion teaching signal

Exact differences versus `v21.2`:
- player weapon model:
  - `weapon_kind`: generic ranged -> `Twisted bow`
  - `weapon_range`: `7 -> 10`
  - `weapon_speed`: unchanged at `5` on rapid
  - player ranged attack roll and max-hit path now use a lightweight TBow
    target-magic scaling instead of the old generic ranged formula
  - TBow scaling is simplified and localized:
    - uses target NPC magic level only
    - caps target magic level at `250`
    - does not add a deeper full target-magic-accuracy model
- active loadout-B hardcoded player stats corrected:
  - `def_stab: 115 -> 116`
  - `def_slash: 104 -> 106`
  - `def_crush: 129 -> 129` (unchanged)
  - `def_magic: 148 -> 150`
  - `def_ranged: 111 -> 121`
  - `prayer_bonus: 9 -> 6`
  - offensive totals remain:
    - `ranged_atk = 215`
    - `ranged_str = 99`
- policy observation contract:
  - add explicit `npc_type` per visible NPC slot
  - `FC_OBS_NPC_STRIDE: 10 -> 11`
  - `FC_POLICY_OBS_SIZE: 106 -> 114`
  - Puffer/eval policy input:
    - `142 -> 150` floats (`policy obs + mask5`)
  - full backend buffer:
    - `291 -> 299` floats
- reward shaping:
  - keep all existing `v21.2` weights and shaping terms
  - add three new live shaping keys:
    - `shape_low_prayer_pot_threshold = 0.60`
    - `shape_low_prayer_no_pot_penalty = -0.01`
    - `shape_low_prayer_pot_reward = 0.15`

Exact active config:
- run setup: `load_model_path=null`, corrected `Masori (f) + TBow` combat model, per-NPC `npc_type` obs enabled, `policy_obs=114`, `puffer_obs=150`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_low_prayer_pot_threshold=0.60`, `shape_low_prayer_no_pot_penalty=-0.01`, `shape_low_prayer_pot_reward=0.15`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`7vuw9jy8`):
- completed normally
- final logged trainer step:
  - `4,991,221,760 / 5,000,000,000`
- runtime:
  - `2446s`
- throughput:
  - `1.95M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 27.15`
- `max_wave = 33`
- `most_npcs_slayed = 123`
- `episode_return = 4338.14`
- `episode_length = 4385.76`
- `reached_wave_30 = 0.0648`
- `cleared_wave_30 = 0.0`
- `reached_wave_31 = 0.0`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.9758`
- `prayer_uptime_melee = 0.4382`
- `prayer_uptime_range = 0.5376`
- `prayer_uptime_magic = 0.0`
- `correct_prayer = 1400.37`
- `wrong_prayer_hits = 179.28`
- `no_prayer_hits = 22.09`
- `damage_blocked = 25648.89`
- `dmg_taken_avg = 3090.27`
- `prayer_switches = 616.13`
- `attack_when_ready_rate = 0.4557`
- `pots_used = 31.96`
- `avg_prayer_on_pot = 0.6929`
- `pots_wasted = 14.27`
- `food_eaten = 20.0`
- `avg_hp_on_food = 0.9068`
- `food_wasted = 17.09`
- `tokxil_melee_ticks = 19.89`
- `ketzek_melee_ticks = 0.0`

Key progression points:
- sampled `wave_reached >= 20` by `218M`
- sampled `wave_reached >= 25` by `218M`
- sampled `wave_reached >= 28` by `218M`
- sampled `wave_reached >= 29` by `455M`
- never sampled `wave_reached >= 30`
- strongest window was roughly `1.01B-1.49B`:
  - `1.013B`: `wave_reached = 29.9`, `episode_return = 5507.6`
  - `1.491B`: `wave_reached = 29.2`, `episode_return = 5514.9`
- after `~3.0B`, the run decayed
- the sharp collapse arrived around `4.11B`:
  - sampled `wave_reached = 25.6`
  - sampled `episode_return = 3793.0`
- nearest eval checkpoint to the best sampled average-wave window:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/7vuw9jy8/0000001012924416.bin`

Comparison to `v21.2` (`u58coupx`):
- `score: 0.0 vs 0.0`
- `cave_complete_rate: 0.0 vs 0.0`
- `wave_reached: 27.15 vs 48.65`
- `max_wave: 33 vs 63`
- `most_npcs_slayed: 123 vs 272`
- `episode_return: 4338.1 vs 14307.1`
- `episode_length: 4386 vs 6493`
- `reached_wave_30: 0.0648 vs 0.9733`
- `cleared_wave_30: 0.0 vs 0.9733`
- `reached_wave_31: 0.0 vs 0.9733`
- `reached_wave_63: 0.0 vs 0.0067`
- `jad_kill_rate: 0.0 vs 0.0`
- `prayer_uptime: 0.976 vs 0.264`
- `prayer_uptime_melee: 0.438 vs 0.033`
- `prayer_uptime_range: 0.538 vs 0.108`
- `prayer_uptime_magic: 0.000 vs 0.123`
- `correct_prayer: 1400.4 vs 1143.9`
- `wrong_prayer_hits: 179.3 vs 98.4`
- `no_prayer_hits: 22.1 vs 155.6`
- `damage_blocked: 25649 vs 139493`
- `dmg_taken_avg: 3090 vs 6818`
- `prayer_switches: 616.1 vs 2284.7`
- `attack_when_ready_rate: 0.456 vs 0.633`
- `pots_used: 32.0 vs 0.02`
- `avg_prayer_on_pot: 0.693 vs 0.0018`
- `pots_wasted: 14.27 vs 0.0`
- `food_eaten: 20.0 vs 15.9`
- `food_wasted: 17.09 vs 4.33`
- `tokxil_melee_ticks: 19.89 vs 4.79`
- `ketzek_melee_ticks: 0.0 vs 10.28`

Comparison to `v21` (`v4ekkk3z`):
- `score: 0.0 vs 0.0`
- `wave_reached: 27.15 vs 61.40`
- `max_wave: 33 vs 63`
- `most_npcs_slayed: 123 vs 273`
- `episode_return: 4338.1 vs 22940.3`
- `episode_length: 4386 vs 10027`
- `prayer_uptime: 0.976 vs 0.734`
- `prayer_uptime_melee: 0.438 vs 0.037`
- `prayer_uptime_range: 0.538 vs 0.191`
- `prayer_uptime_magic: 0.000 vs 0.507`
- `correct_prayer: 1400.4 vs 2173.5`
- `wrong_prayer_hits: 179.3 vs 281.5`
- `no_prayer_hits: 22.1 vs 37.5`
- `damage_blocked: 25649 vs 277395`
- `dmg_taken_avg: 3090 vs 7029`
- `prayer_switches: 616.1 vs 2725.4`
- `attack_when_ready_rate: 0.456 vs 0.623`
- `pots_used: 32.0 vs 32.0`
- `avg_prayer_on_pot: 0.693 vs 0.575`
- `pots_wasted: 14.27 vs 9.42`
- `food_eaten: 20.0 vs 16.94`
- `food_wasted: 17.09 vs 9.02`
- `tokxil_melee_ticks: 19.89 vs 10.76`
- `ketzek_melee_ticks: 0.0 vs 18.08`

Interpretation:
- this is a severe regression, not a minor frontier wobble
- `v22` did not preserve the `v21.2` Jad-facing ceiling at all
- instead it fell into a high-prayer, high-consumption, low-offense regime:
  - prayer almost always on
  - all prayer doses consumed
  - all food consumed
  - low prayer-switch count
  - low attack-ready rate
  - no durable post-wave-30 competence
- the behavioral signature looks much closer to the old prayer-camping
  regressions than to the `v21.2` no-pot-collapse regime
- especially important:
  - `prayer_uptime = 0.976`
  - `prayer_uptime_magic = 0.0`
  - `prayer_uptime_range = 0.538`
  - `reached_wave_30 = 0.0648`
  - `attack_when_ready_rate = 0.456`
- this is not a late-wave resource-collapse policy
- it is an early/mid-wave over-defensive policy that never builds enough tempo
  or switching competence to reach the old frontier

Most important new insight:
- the `v22` delta bundle did not fail in a neutral way
- it appears to have pushed the policy into a heavily defensive prayer-potion
  regime very early
- there are three plausible causes in the `v22` delta set:
  - corrected player combat model
  - added `npc_type` obs / larger policy input
  - low-prayer potion teaching signal
- of those, the strongest *behavioral* signature points more toward the new
  low-prayer shaping than toward the combat-model correction:
  - potion use went from near-zero in `v21.2` to essentially full-inventory
  - prayer uptime exploded upward instead of collapsing
- however, because `v22` also changed the policy-input contract from `106` to
  `114`, the added `npc_type` channel is still a valid rollback target

Recommendations:
- revert the added `npc_type` observation as `v22.1`
- keep the corrected player combat model for now
- keep the low-prayer shaping for one more rollback test only if the goal is
  to isolate the obs-contract change first
- if `v22.1` is still strongly regressed, the next primary suspect is the
  low-prayer potion shaping, not the player combat-model correction
- use the early `v22` checkpoint window for replay if needed, not the final
  checkpoint

Bottom line:
- `v22` is a considerable regression from both `v21.2` and `v21`
- it never reached wave 31 in a stable way
- it never reached Jad
- its failure mode is a high-prayer, full-resource-burn, low-tempo regime
- `v22.1` should be the clean rollback of the added `npc_type` observation
  while leaving the other `v22` changes intact

---

## v_tmp2.1 (2026-04-09, completed)

Actual run id:
- `j63y66ed`
- W&B run name:
  - `fallen-water-102`
- local run log:
  - `j63y66ed.json` (temporary-worktree artifact not retained locally)

Mainline direction:
- keep the prayer/combat backend with locked-prayer hit resolution
- keep reward-path alignment to those backend semantics
- no curriculum
- no PPO retune
- only reduce positive prayer shaping and slightly increase wrong-prayer pressure

Exact changes versus the original `v_tmp2` run (`fkhhysfd`):
- `w_correct_danger_prayer: 0.5 -> 0.25`
- `shape_wrong_prayer_penalty: -1.0 -> -1.25`
- `shape_npc_specific_prayer_bonus: 2.5 -> 1.5`

What stayed the same:
- no curriculum buckets
- same PPO/train recipe
- same damage, kill, wave-clear, Jad, melee-pressure, kiting, and stall terms
- same `4096` agents and `2.5B` step budget

Why this run mattered:
- the first `v_tmp2` run was promising on depth and stability, but clearly bad on
  prayer correctness
  - `wrong_prayer_hits = 464.25`
  - `dmg_taken_avg = 7470.13`
  - `prayer_uptime = 0.947`
- `v_tmp2.1` was the first clean test after reward alignment to locked-prayer
  backend semantics

Backend / config provenance:
- this run used the `v_tmp2` backend path, not the old pre-prayer baseline
- evidence:
  - the local log artifact lives under the `v_tmp2` PufferLib tree:
    `j63y66ed.json` (temporary-worktree artifact not retained locally)
  - that artifact records the exact unique `v_tmp2.1` values:
    - `w_correct_danger_prayer = 0.25`
    - `shape_wrong_prayer_penalty = -1.25`
    - `shape_npc_specific_prayer_bonus = 1.5`
    - `curriculum_wave = 0`
    - `curriculum_pct = 0.0`
  - the run was launched with `FORCE_BACKEND_REBUILD=1`, so the trainer backend
    was rebuilt from the `v_tmp2` source tree before training

Exact active config:
- run setup: `load_model_path=null`, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results:
- completed normally
- final trainer step:
  - `2,499,805,184 / 2,500,000,000`
- runtime:
  - `1262.9s`
- throughput:
  - `1.98M SPS`

Final metrics:
- `episode_return = 12739.18`
- `episode_length = 6066.53`
- `wave_reached = 46.12`
- `max_wave = 62`
- `most_npcs_slayed = 270`
- `prayer_uptime = 0.665`
- `prayer_uptime_melee = 0.115`
- `prayer_uptime_range = 0.272`
- `prayer_uptime_magic = 0.278`
- `correct_prayer = 1339.86`
- `wrong_prayer_hits = 191.61`
- `no_prayer_hits = 26.04`
- `damage_blocked = 122207.55`
- `dmg_taken_avg = 5132.12`
- `attack_when_ready_rate = 0.579`
- `prayer_switches = 1212.56`
- `pots_used = 31.94`
- `avg_prayer_on_pot = 0.768`
- `pots_wasted = 23.47`
- `food_eaten = 18.83`
- `avg_hp_on_food = 0.790`
- `food_wasted = 10.65`
- `tokxil_melee_ticks = 7.04`
- `ketzek_melee_ticks = 9.39`

Key progression points:
- `reached_wave_30 >= 0.90` by `490.7M`
- `reached_wave_31 >= 0.90` by `499.1M`
- first sampled `wave_reached >= 40` by `1.74B`
- strongest sustained window was around `2.26B-2.36B`
  - sampled `wave_reached = 52.3`
  - sampled `episode_return = 16430.2`

Comparison to original `v_tmp2` (`fkhhysfd`):
- `wave_reached: 46.12 vs 44.72`
- `max_wave: 62 vs 53`
- `most_npcs_slayed: 270 vs 222`
- `reached_wave_31: 0.981 vs 0.965`
- `first reached_wave_31 >= 0.90: 499M vs 1.01B`
- `prayer_uptime: 0.665 vs 0.947`
- `wrong_prayer_hits: 191.6 vs 464.3`
- `dmg_taken_avg: 5132 vs 7470`
- `tokxil_melee_ticks: 7.04 vs 12.63`
- `attack_when_ready_rate: 0.579 vs 0.656`
- `prayer_switches: 1213 vs 1073`
- `ketzek_melee_ticks: 9.39 vs 4.71`

Interpretation versus original `v_tmp2`:
- this broke the old high-prayer, low-correctness local optimum
- the backend-aligned reward path plus smaller positive prayer rewards materially
  improved the exact metrics that had regressed
- the remaining cost is more specific:
  - higher Ket-Zek melee exposure
  - earlier prayer-potion use and much more potion waste

Comparison to `v_tmp3` (`fg029tll`):
- `wave_reached: 46.12 vs 41.17`
- `max_wave: 62 vs 60`
- `most_npcs_slayed: 270 vs 263`
- `reached_wave_31: 0.981 vs 0.944`
- `attack_when_ready_rate: 0.579 vs 0.349`
- `prayer_switches: 1213 vs 2238`
- `tokxil_melee_ticks: 7.04 vs 17.81`
- `food_wasted: 10.65 vs 18.07`
- `wrong_prayer_hits: 191.6 vs 155.1`
- `dmg_taken_avg: 5132 vs 3727`
- `first reached_wave_31 >= 0.90: 499M vs 218M`

Interpretation versus `v_tmp3`:
- `v_tmp3` still learns deep play faster and remains cleaner on prayer/damage
- `v_tmp2.1` now wins on final depth, late-run stability, and rare-event tail
- this is the first version of the locked-prayer backend that looks strictly
  viable as the forward path rather than an interesting regression branch

Most important new insight:
- the old `v_tmp2` failure mode was mostly reward-shaping around prayer, not an
  observation-space problem
- after aligning reward to the backend snapshot semantics and reducing generic
  prayer incentive, the same backend improved on:
  - `wrong_prayer_hits`
  - `dmg_taken_avg`
  - late-wave milestone timing
  - max-wave / tail depth

Bottom line:
- `v_tmp2.1` is the run that justified merging the locked-prayer backend into
  mainline code
- follow-up tuning should now focus narrowly on:
  - Ket-Zek positioning / melee leakage
  - prayer-potion timing
  - late-run stability

---

## v21.2 (2026-04-10, completed)

Actual run:
- `u58coupx`
- W&B run name:
  - `light-sky-128`
- local run log:
  - `u58coupx.json` (not retained locally)

Status:
- completed normally

Goal:
- restore the exact `v21` reward / PPO recipe after the `v21.1` anti-stall
  regression
- keep the non-learning tooling improvements added around `v21.1`
- test whether removing the `shape_not_attacking_*` retune restores the `v21`
  frontier

Config versus `v21.1`:
- revert the only intended reward changes:
  - `shape_not_attacking_grace_ticks: 6 -> 2`
  - `shape_not_attacking_penalty: -0.03 -> -0.01`

Config versus `v21`:
- no intended learning-behavior delta
- keep these non-learning changes:
  - reward math centralized through the shared `fc_reward.h` helper so viewer
    and training use the same breakdown
  - explicit analytics:
    - `cave_complete_rate`
    - `reached_wave_63`
    - `jad_kill_rate`
  - viewer / debug overlay parity fixes so replay inspection matches training
    reward terms and current observation labels

Exact active config:
- run setup: `load_model_path=null`, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`u58coupx`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2371.9s`
- throughput:
  - `2.01M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 48.65`
- `max_wave = 63`
- `most_npcs_slayed = 272`
- `episode_return = 14307.14`
- `episode_length = 6492.74`
- `reached_wave_30 = 0.9733`
- `cleared_wave_30 = 0.9733`
- `reached_wave_31 = 0.9733`
- `reached_wave_63 = 0.0067`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.2638`
- `prayer_uptime_melee = 0.0331`
- `prayer_uptime_range = 0.1079`
- `prayer_uptime_magic = 0.1228`
- `correct_prayer = 1143.94`
- `wrong_prayer_hits = 98.37`
- `no_prayer_hits = 155.63`
- `damage_blocked = 139492.77`
- `dmg_taken_avg = 6817.96`
- `prayer_switches = 2284.72`
- `attack_when_ready_rate = 0.6329`
- `pots_used = 0.02`
- `avg_prayer_on_pot = 0.0018`
- `pots_wasted = 0.0`
- `food_eaten = 15.89`
- `avg_hp_on_food = 0.6149`
- `food_wasted = 4.33`
- `tokxil_melee_ticks = 4.79`
- `ketzek_melee_ticks = 10.28`

Key progression points:
- sampled `wave_reached >= 30` by `222.3M`
- sampled `wave_reached >= 35` by `459.3M`
- sampled `wave_reached >= 40` by `559.9M`
- sampled `wave_reached >= 45` by `616.6M`
- sampled `wave_reached >= 50` by `690.0M`
- sampled `wave_reached >= 55` by `887.1M`
- sampled `wave_reached >= 60` by `992.0M`
- sampled `max_wave = 63` by `780.1M`
- best sampled window was around `1.013B`:
  - `wave_reached = 60.73`
  - `episode_return = 21935.6`
  - `episode_length = 9092`
- after that peak, the run decayed into the mid-to-high `40s` / low `50s` and
  finished well below `v21`
- nearest eval checkpoint to the peak window:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/u58coupx/0000000997195776.bin`

Comparison to `v21` (`v4ekkk3z`):
- `score: 0.0 vs 0.0`
- `wave_reached: 48.65 vs 61.40`
- `max_wave: 63 vs 63`
- `most_npcs_slayed: 272 vs 273`
- `episode_return: 14307.1 vs 22940.3`
- `episode_length: 6493 vs 10027`
- `prayer_uptime: 0.264 vs 0.734`
- `prayer_uptime_melee: 0.033 vs 0.037`
- `prayer_uptime_range: 0.108 vs 0.191`
- `prayer_uptime_magic: 0.123 vs 0.507`
- `correct_prayer: 1143.9 vs 2173.5`
- `wrong_prayer_hits: 98.4 vs 281.5`
- `no_prayer_hits: 155.6 vs 37.5`
- `damage_blocked: 139493 vs 277395`
- `dmg_taken_avg: 6818 vs 7029`
- `prayer_switches: 2284.7 vs 2725.4`
- `attack_when_ready_rate: 0.633 vs 0.623`
- `pots_used: 0.02 vs 32.0`
- `avg_prayer_on_pot: 0.0018 vs 0.5754`
- `pots_wasted: 0.0 vs 9.42`
- `food_eaten: 15.9 vs 16.9`
- `food_wasted: 4.33 vs 9.02`
- `tokxil_melee_ticks: 4.79 vs 10.76`
- `ketzek_melee_ticks: 10.28 vs 18.08`
- `reached_wave_63: 0.0067 vs not logged in v21`
- `jad_kill_rate: 0.0 vs not logged in v21`
- `SPS: 2.01M vs 2.01M`
- `entropy: 0.658 vs 0.782`
- `clipfrac: 0.157 vs 0.164`
- `value_loss: 21.506 vs 8.213`
- `kl: 0.0225 vs 0.0197`

Comparison to `v21.1` (`nxj1iw0b`):
- `score: 0.0 vs 0.0`
- `cave_complete_rate: 0.0 vs 0.0`
- `wave_reached: 48.65 vs 31.10`
- `max_wave: 63 vs 33`
- `most_npcs_slayed: 272 vs 124`
- `episode_return: 14307.1 vs 6157.9`
- `episode_length: 6493 vs 3448`
- `prayer_uptime: 0.264 vs 0.697`
- `prayer_uptime_melee: 0.033 vs 0.071`
- `prayer_uptime_range: 0.108 vs 0.626`
- `prayer_uptime_magic: 0.123 vs 0.000`
- `correct_prayer: 1143.9 vs 829.0`
- `wrong_prayer_hits: 98.4 vs 39.1`
- `no_prayer_hits: 155.6 vs 17.2`
- `damage_blocked: 139493 vs 11791`
- `dmg_taken_avg: 6818 vs 1930`
- `prayer_switches: 2284.7 vs 220.2`
- `attack_when_ready_rate: 0.633 vs 0.336`
- `pots_used: 0.02 vs 12.2`
- `food_eaten: 15.9 vs 6.7`
- `tokxil_melee_ticks: 4.79 vs 1.51`
- `ketzek_melee_ticks: 10.28 vs 1.03`
- `reached_wave_63: 0.0067 vs 0.0`
- `jad_kill_rate: 0.0 vs 0.0`

Jad / completion audit:
- `reached_wave_63 > 0` confirms real Jad-wave episodes
- `jad_kill_rate = 0.0` confirms no Jad kills
- `cave_complete_rate = 0.0` confirms no clears
- `most_npcs_slayed = 272` is consistent with reaching Jad while killing no
  Jad healers

Interpretation:
- reverting the `v21.1` anti-stall retune clearly fixed the wave-31 collapse
- offensive tempo recovered almost exactly to `v21`:
  - `attack_when_ready_rate: 0.633 vs 0.623`
- but `v21.2` did **not** restore the `v21` sustained late-wave regime
- instead it learned a different failure mode:
  - very low prayer uptime
  - near-zero potion usage
  - high no-prayer hit volume
  - decent offensive tempo, but poor prayer/resource sustain
- the run still has real ceiling:
  - it reached `max_wave = 63`
  - it hit `wave_reached = 60.73` around `1.013B`
- but it could not hold that regime and drifted down for the final `~4B` steps
- even after recovering from `v21.1`, the final `v21.2` policy still finished
  below `v20.2` on average depth (`48.65` vs `55.83`)

Most important new insight:
- reverting the `shape_not_attacking_*` retune is necessary but not sufficient
  to recover `v21`
- `v21.2` regained Jad-wave reach and removed the `v21.1` low-offense shelf
- the remaining failure mode is different from `v21.1`:
  - not attack passivity
  - but prayer/resource collapse
- because `v21.2` nominally restores the `v21` learning config, the remaining
  gap is now either:
  - trainer variance / instability
  - or a non-neutral effect from the retained shared reward-helper refactor

Recommendations:
- use the early-peak `v21.2` checkpoint window for eval, not the final checkpoint
- keep the explicit Jad analytics and viewer parity fixes
- do not reintroduce the `v21.1` `shape_not_attacking_*` retune
- before further reward tuning, run a numeric parity audit between the shared
  `fc_reward.h` path and the pre-refactor `v21` reward computation on identical
  recorded states
- if `v21.3` is needed, debug the prayer/resource regression first, not stall
  shaping

Bottom line:
- `v21.2` substantially recovered from the `v21.1` collapse
- `v21.2` did not reproduce `v21`
- it reached Jad wave but recorded zero Jad kills and zero cave clears
- the next debugging target is training equivalence of the retained shared
  reward path, not another anti-stall retune

---

## v21.1 (2026-04-10, completed)

Actual run:
- `nxj1iw0b`
- W&B run name:
  - `fanciful-disco-127`
- local run log:
  - `nxj1iw0b.json` (not retained locally)

Status:
- completed normally

Goal:
- keep the `v21` reward/backend stack and `5B` run budget
- increase pressure against deterministic “attack-ready but not attacking”
  stalls seen in eval viewer
- make viewer reward/debug output match the current fc-training/core reward
  path exactly so replay assessment is trustworthy

Config versus `v21`:
- same PPO recipe
- same backend
- same loadout
- same prayer / resource / melee-pressure shaping terms
- only intended learning-behavior change:
  - `shape_not_attacking_grace_ticks: 2 -> 6`
  - `shape_not_attacking_penalty: -0.01 -> -0.03`

Tooling / analytics changes kept in this run:
- `viewer.c` loads reward params from `config/fight_caves.ini`
- fc-training reward and viewer reward both route through the shared
  `fc_reward.h` breakdown code
- the viewer reward tab now reflects the current training breakdown instead of
  stale viewer-only math
- explicit run analytics now include:
  - `cave_complete_rate`
  - `reached_wave_63`
  - `jad_kill_rate`

Exact active config:
- run setup: `load_model_path=null`, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=6`, `shape_not_attacking_penalty=-0.03`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`nxj1iw0b`):
- completed normally
- final logged trainer step:
  - `4,999,610,368 / 5,000,000,000`
- runtime:
  - `2669.2s`
- throughput:
  - `1.75M SPS`

Final metrics:
- `score = 0.0`
- `cave_complete_rate = 0.0`
- `wave_reached = 31.10`
- `max_wave = 33`
- `most_npcs_slayed = 124`
- `episode_return = 6157.90`
- `episode_length = 3447.77`
- `reached_wave_30 = 1.0`
- `cleared_wave_30 = 1.0`
- `reached_wave_31 = 1.0`
- `reached_wave_63 = 0.0`
- `jad_kill_rate = 0.0`
- `prayer_uptime = 0.6972`
- `prayer_uptime_melee = 0.0711`
- `prayer_uptime_range = 0.6261`
- `prayer_uptime_magic = 0.0`
- `correct_prayer = 829.04`
- `wrong_prayer_hits = 39.10`
- `no_prayer_hits = 17.24`
- `damage_blocked = 11790.64`
- `dmg_taken_avg = 1929.56`
- `prayer_switches = 220.16`
- `attack_when_ready_rate = 0.3364`
- `pots_used = 12.22`
- `avg_prayer_on_pot = 0.4226`
- `pots_wasted = 0.90`
- `food_eaten = 6.74`
- `avg_hp_on_food = 0.8967`
- `food_wasted = 5.60`
- `tokxil_melee_ticks = 1.51`
- `ketzek_melee_ticks = 1.03`

Key progression points:
- sampled `wave_reached >= 28` by `230.7M`
- sampled `wave_reached >= 30` by `478.2M`
- sampled `wave_reached >= 31` by `1.309B`
- never sampled `wave_reached >= 32`
- never sampled `wave_reached >= 35`
- from roughly `1.3B` onward, the run sat on a narrow `30.9-31.2` shelf for the
  rest of training
- best sampled window was the final one:
  - `wave_reached = 31.2`
  - `episode_return = 6193.6`
  - `episode_length = 3446`

Comparison to `v21` (`v4ekkk3z`):
- `score: 0.0 vs 0.0`
- `wave_reached: 31.10 vs 61.40`
- `max_wave: 33 vs 63`
- `most_npcs_slayed: 124 vs 273`
- `episode_return: 6157.9 vs 22940.3`
- `episode_length: 3448 vs 10027`
- `prayer_uptime: 0.697 vs 0.734`
- `prayer_uptime_melee: 0.071 vs 0.037`
- `prayer_uptime_range: 0.626 vs 0.191`
- `prayer_uptime_magic: 0.000 vs 0.507`
- `correct_prayer: 829.0 vs 2173.5`
- `wrong_prayer_hits: 39.1 vs 281.5`
- `no_prayer_hits: 17.2 vs 37.5`
- `damage_blocked: 11791 vs 277395`
- `dmg_taken_avg: 1930 vs 7029`
- `prayer_switches: 220.2 vs 2725.4`
- `attack_when_ready_rate: 0.336 vs 0.623`
- `pots_used: 12.2 vs 32.0`
- `food_eaten: 6.7 vs 16.9`
- `tokxil_melee_ticks: 1.51 vs 10.76`
- `ketzek_melee_ticks: 1.03 vs 18.08`
- `reached_wave_63: 0.0 vs Jad-wave territory in v21`
- `jad_kill_rate: 0.0 vs not logged in v21`
- `SPS: 1.75M vs 2.01M`
- `entropy: 1.742 vs 0.782`
- `clipfrac: 0.171 vs 0.164`
- `value_loss: 8.317 vs 8.213`
- `kl: 0.0145 vs 0.0197`

Interpretation:
- this was not a “same-depth but cleaner” regression
- it was a complete frontier collapse from a Jad-facing run into a stable
  wave-31 shelf
- the lower damage taken, lower resource use, and lower melee leakage are a
  byproduct of dying much earlier, not evidence of better late-wave play
- the strongest behavioral signatures are:
  - `attack_when_ready_rate` collapsing from `0.623` to `0.336`
  - `prayer_uptime_magic` collapsing from `0.507` to `0.0`
  - `prayer_uptime_range` dominating at `0.626`
  - `prayer_switches` collapsing from `2725` to `220`
- that points to a conservative, low-offense, range-prayer-heavy policy that
  consistently clears wave 30 but fails to convert into deeper late-wave play

Most important new insight:
- the `v21.1` anti-stall retune was clearly harmful on this stack
- the only intended learning-behavior delta from `v21` was the
  `shape_not_attacking_*` change
- the viewer / parity / analytics changes are much less likely to explain a
  collapse this large because they were intended to preserve training behavior
- the most plausible read is that delaying the non-attacking penalty out to `6`
  ready ticks materially weakened offensive pressure around the wave-30/31
  frontier

Recommendations:
- revert `shape_not_attacking_grace_ticks` to `2`
- revert `shape_not_attacking_penalty` to `-0.01`
- keep the shared reward helper, explicit Jad analytics, and viewer parity work
- use the reverted parity baseline as `v21.2`
- if future anti-stall tuning is revisited, do it from the `v21`/`v21.2` base
  with much smaller deltas or shorter diagnostic budgets

Bottom line:
- `v21.1` was a severe regression from `v21`
- it never reached Jad wave
- it plateaued around wave `31` for almost the entire back half of training
- the clean next move is to revert the `shape_not_attacking_*` change and keep
  only the non-learning tooling improvements

---

## v21 (2026-04-09, completed)

Actual run:
- `v4ekkk3z`
- W&B run name:
  - `logical-wave-125`

Status:
- completed normally

Goal:
- run the `v20.2` baseline longer without changing the reward recipe or backend
  behavior
- test whether the strong `v20.2` trajectory continues to improve when given a
  `5B` budget instead of `2.5B`

Config:
- exact `v20.2` PPO recipe
- exact `v20.2` env / reward recipe
- no curriculum
- same loadout
- only intended config change versus `v20.2`:
  - `total_timesteps: 2.5B -> 5.0B`

Backend changes versus `v20.2`:
- none
- keep:
  - locked-prayer combat backend
  - 1-tick prayer flick drain fix
  - resolve-time generic danger-prayer reward
  - mild `v_tmp2.1` prayer reward magnitudes

Exact active config:
- run setup: `load_model_path=null`, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=5_000_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Important caveat:
- this was not a clean “same run, longer” control in trainer internals
- `total_timesteps` also lengthened the prioritized-replay beta anneal horizon
  in the current PufferLib implementation
- so `v21` changed both:
  - the total budget
  - the replay-anneal schedule

Results (`v4ekkk3z`):
- completed normally
- final trainer step: `4,999,610,368 / 5,000,000,000`
- runtime: `2418s`
- throughput: `2.01M SPS`

Final metrics:
- `score = 0.0`
- `wave_reached = 61.40`
- `max_wave = 63`
- `most_npcs_slayed = 273`
- `episode_return = 22940.31`
- `episode_length = 10027.00`
- `reached_wave_30 = 1.0`
- `reached_wave_31 = 1.0`
- `prayer_uptime = 0.7345`
- `prayer_uptime_melee = 0.0371`
- `prayer_uptime_range = 0.1906`
- `prayer_uptime_magic = 0.5068`
- `correct_prayer = 2173.52`
- `wrong_prayer_hits = 281.50`
- `no_prayer_hits = 37.47`
- `damage_blocked = 277395.06`
- `dmg_taken_avg = 7028.79`
- `prayer_switches = 2725.36`
- `attack_when_ready_rate = 0.6226`
- `pots_used = 32.0`
- `avg_prayer_on_pot = 0.5754`
- `pots_wasted = 9.42`
- `food_eaten = 16.94`
- `avg_hp_on_food = 0.7751`
- `food_wasted = 9.02`
- `tokxil_melee_ticks = 10.76`
- `ketzek_melee_ticks = 18.08`

Key progression points:
- sampled `wave_reached >= 30` by `218M`
- sampled `wave_reached >= 35` by `455M`
- sampled `wave_reached >= 40` by `696M`
- sampled `wave_reached >= 45` by `696M`
- sampled `wave_reached >= 50` by `696M`
- sampled `wave_reached >= 55` by `1.013B`
- sampled `wave_reached >= 60` by `1.013B`
- sampled `max_wave = 63` by `1.876B`
- best sampled window was around `3.246B`:
  - `wave_reached = 61.8`
  - `episode_return = 23283.6`
  - `episode_length = 10191`
- after that peak, the run drifted back into the upper-50 / low-60 band for
  the rest of training rather than converting into full clears

Comparison to `v20.2` (`4o8gv87z`):
- `score: 0.0 vs 0.0`
- `wave_reached: 61.40 vs 55.83`
- `max_wave: 63 vs 60`
- `most_npcs_slayed: 273 vs 264`
- `episode_return: 22940.31 vs 19112.53`
- `episode_length: 10027.00 vs 8246.68`
- `sampled wave_reached >= 30: 218M vs 489M`
- `sampled wave_reached >= 40: 696M vs 973M`
- `sampled wave_reached >= 50: 696M vs 1.260B`
- `sampled wave_reached >= 55: 1.013B vs 1.369B`
- `prayer_uptime: 0.734 vs 0.824`
- `correct_prayer: 2173.52 vs 1837.12`
- `wrong_prayer_hits: 281.50 vs 155.01`
- `no_prayer_hits: 37.47 vs 23.55`
- `damage_blocked: 277395 vs 204649`
- `dmg_taken_avg: 7029 vs 5241`
- `prayer_switches: 2725.4 vs 485.9`
- `attack_when_ready_rate: 0.623 vs 0.657`
- `tokxil_melee_ticks: 10.76 vs 2.14`
- `ketzek_melee_ticks: 18.08 vs 4.21`
- `food_wasted: 9.02 vs 7.57`
- `pots_wasted: 9.42 vs 3.99`

Jad / completion audit:
- `score = 0` is correct for this run under the current backend
- in Fight Caves, `score` only increments when terminal code is
  `TERMINAL_CAVE_COMPLETE`
- reaching wave 63 does **not** imply a score increment
- so `max_wave = 63` with `score = 0` means:
  - the run reached Jad-wave episodes
  - no episode actually cleared the cave
- before this audit patch, the backend did **not** preserve a per-episode
  “Jad died at any point” latch in logged analytics
- therefore the old logs cannot prove whether `v21` ever killed Jad in an
  episode and later died to healers
- however, `most_npcs_slayed = 273` is still informative:
  - a full cave without Jad healers is `272` kills total
  - `273` proves the policy reached Jad healer-phase territory in at least one
    episode
  - but it does **not** prove Jad kill, because `273` could be:
    - Jad + 1 healer
    - or 2 healers with Jad still alive
- forward fix applied here:
  - added explicit `reached_wave_63`, `jad_kill_rate`, and
    `cave_complete_rate` analytics so future runs cannot blur those cases

Interpretation:
- `v21` is deeper than `v20.2` on every coarse depth metric that matters:
  average wave, max wave, kill-count ceiling, return, and episode length
- but it is also much sloppier in late-wave execution:
  - far more wrong-prayer hits
  - more no-prayer hits
  - much more prayer thrash
  - much worse Tok-Xil / Ket-Zek melee leakage
  - more potion waste
- this is not the old `v20` / `v20.1` prayer-camping failure mode
- instead it looks like a high-ceiling, low-conversion regime:
  - it gets to Jad territory early and often
  - but it leaks too much damage and resource efficiency to finish
- because the anneal horizon changed with the longer budget, this cannot be
  cleanly interpreted as “5B is better than 2.5B” without qualification

Most important new insight:
- longer training on the `v20.2` recipe does unlock much more frequent Jad-wave
  reach and a higher peak frontier on this stack
- but the current trainer schedule does not turn that extra depth into actual
  cave clears
- the run is good evidence that the recipe has more ceiling left
- it is not good evidence yet that the final `5B` policy is the right control
  for future reward/backend comparisons

Recommendations:
- keep `v20.2` as the clean baseline for config comparisons
- treat `v21` as a promising but schedule-confounded long-run result
- decouple replay beta anneal horizon from `total_timesteps` before the next
  long control (`v21.1` / `v22`)
- use the new explicit `reached_wave_63`, `jad_kill_rate`, and
  `cave_complete_rate` metrics for all future Jad-facing runs
- evaluate the best `v21` checkpoint window, not just the final checkpoint:
  - `<repo-root>/pufferlib_4/checkpoints/fight_caves/v4ekkk3z/0000003251634176.bin`
- if follow-up tuning is needed after the anneal decouple, focus on:
  - late-wave damage leakage
  - Tok-Xil / Ket-Zek melee exposure
  - resource waste during Jad/healer phase
  - not on broad prayer-reward redesign first

Bottom line:
- `v21` was a real frontier extension over `v20.2` in raw depth
- `v21` did **not** produce a cave clear
- old logging could not tell us whether Jad ever died during a failed wave-63
  episode
- the backend audit found that ambiguity and the patch here fixes it for future
  runs

---

## v20 Family Comparison Summary (2026-04-09)

This four-run block cleanly separated three moving pieces:
- the 1-tick prayer drain fix
- snapshot-timed generic prayer reward
- aggressive prayer reward magnitudes

Run matrix:
- `v20`
  - 1-tick flick fix
  - snapshot-timed generic prayer reward
  - aggressive prayer magnitudes
  - NPC-specific bonus disabled
- `v20.1`
  - 1-tick flick fix
  - snapshot-timed generic prayer reward
  - mild `v_tmp2.1` prayer magnitudes
  - NPC-specific bonus restored
- `v20.2`
  - 1-tick flick fix
  - original resolve-time generic prayer reward
  - mild `v_tmp2.1` prayer magnitudes
  - NPC-specific bonus restored
- `v20.3`
  - 1-tick flick fix
  - original resolve-time generic prayer reward
  - aggressive prayer magnitudes
  - NPC-specific bonus disabled

Final ranking by actual cave performance:
- `v20.2` (`4o8gv87z`) — clear winner
- `v_tmp2.1` / `ro7h07qm` — still strong, but now second-best
- `v20` (`t4eudsav`) and `v20.1` (`rp7a0y2e`) — both meaningful regressions
- `v20.3` (`77yha5y7`) — worst of the `v20` family

What the matrix says:
- the 1-tick flick fix itself is strongly positive
  - `v20.2` is the direct evidence
- snapshot-timed generic prayer reward is harmful in this form
  - both `v20` and `v20.1` regressed while keeping that timing change
- aggressive prayer reward magnitudes are also harmful
  - `v20.3` regressed even after reward timing was restored to resolve-time
- the best current recipe is:
  - keep the 1-tick flick fix
  - keep resolve-time generic prayer reward
  - keep the milder `v_tmp2.1` prayer magnitudes
  - keep the NPC-specific bonus enabled at the old value

Key observations across the four runs:
- snapshot timing consistently pushed the policy toward prayer camping
  - `v20`: `prayer_uptime = 0.966`, `prayer_switches = 682`
  - `v20.1`: `prayer_uptime = 0.980`, `prayer_switches = 402`
  - both were much less switchy than `v_tmp2.1` while not getting deeper
- aggressive magnitudes without snapshot timing still overconstrained the policy
  - `v20.3`: `prayer_switches = 57.6`, `no_prayer_hits = 58.9`
  - this was not just “camp more prayer”; it also lost offensive tempo and
    stalled around the low-30 shelf for most of the run
- `v20.2` is qualitatively different from the others
  - it broke into the 40+ range by `948M`
  - hit `45+` by `1.028B`
  - hit `50+` by `1.193B`
  - hit `55+` by `1.351B`
  - no other run in this family came close

Best-to-worst comparison at a glance:
- `wave_reached`
  - `v20.2: 55.83`
  - `v_tmp2.1: 46.12`
  - `v20: 44.66`
  - `v20.1: 44.11`
  - `v20.3: 38.32`
- `max_wave`
  - `v20.2: 60`
  - `v_tmp2.1: 62`
  - `v20: 51`
  - `v20.1: 50`
  - `v20.3: 49`
- `attack_when_ready_rate`
  - `v20.2: 0.657`
  - `v_tmp2.1: 0.579`
  - `v20: 0.537`
  - `v20.1: 0.535`
  - `v20.3: 0.465`
- `tokxil_melee_ticks`
  - `v20.2: 2.14`
  - `v_tmp2.1: 7.04`
  - `v20.1: 12.78`
  - `v20.3: 11.20`
  - `v20: 18.16`
- `ketzek_melee_ticks`
  - `v20.2: 4.21`
  - `v_tmp2.1: 9.39`
  - `v20.3: 8.12`
  - `v20.1: 13.78`
  - `v20: 20.83`

Interpretation:
- the old hypothesis that “maybe the new prayer timing reward just needs softer
  magnitudes” did not hold up
  - `v20.1` disproved that
- the old hypothesis that “maybe the new reward magnitudes were the whole
  problem” also did not hold up by itself
  - `v20.3` disproved that
- the current strongest explanation is:
  - snapshot-timed generic reward creates bad credit assignment pressure
  - aggressive prayer magnitudes also create bad optimization pressure
  - the 1-tick flick fix is independently good and should be kept

Recommendations:
- promote `v20.2` as the new working baseline
- do not train further on the snapshot-timed generic prayer reward path in its
  current form
- do not use the aggressive prayer reward magnitudes from `v20` / `v20.3`
- if prayer reward timing is revisited later, test it only as a much smaller
  auxiliary signal instead of replacing the resolve-time signal
- if further ablations are needed, the clean next one would be:
  - `v20.2` base
  - keep resolve-time reward
  - vary only NPC-specific bonus on/off
  - leave generic prayer reward at `0.25`

Bottom line:
- `v20.2` answered the main question
- the valuable backend change is the 1-tick flick fix
- the valuable training recipe is still the mild `v_tmp2.1` prayer reward band
- the new forward baseline should be `v20.2`, not `v20`, `v20.1`, or `v20.3`

---

## v20.3 (2026-04-09, completed)

Actual run:
- `77yha5y7`
- W&B run name:
  - `bright-firebrand-110`

Status:
- completed normally

Goal:
- test the aggressive prayer reward magnitudes from `v20` without the new
  snapshot-timed prayer reward
- separate “reward strength” from “reward timing”

Config:
- same PPO recipe as `v_tmp2.1`
- no curriculum
- `2.5B` budget
- same loadout and same non-prayer shaping terms as `v_tmp2.1`

Backend changes versus `v_tmp2.1` / `ro7h07qm`:
- keep the 1-tick prayer drain fix
- revert generic danger-prayer reward timing to the original resolve-time path

Config changes versus `v20.2`:
- disable `shape_npc_specific_prayer_bonus`
- raise `w_correct_danger_prayer` from `0.25` to `1.75`

Exact active config:
- run setup: `load_model_path=null`, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=1.75`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`77yha5y7`):
- completed normally
- final trainer step: `2,496,659,456 / 2,500,000,000`
- runtime: `1252s`
- throughput: `1.81M SPS`

Final metrics:
- `wave_reached = 38.32`
- `max_wave = 49`
- `most_npcs_slayed = 195`
- `episode_return = 8819.71`
- `episode_length = 4650.34`
- `reached_wave_30 = 0.9708`
- `reached_wave_31 = 0.9417`
- `prayer_uptime = 0.7789`
- `prayer_uptime_melee = 0.000043`
- `prayer_uptime_range = 0.4839`
- `prayer_uptime_magic = 0.2949`
- `correct_prayer = 1004.14`
- `wrong_prayer_hits = 176.15`
- `no_prayer_hits = 58.86`
- `damage_blocked = 71645.82`
- `dmg_taken_avg = 4080.46`
- `prayer_switches = 57.56`
- `attack_when_ready_rate = 0.4651`
- `pots_used = 24.10`
- `avg_prayer_on_pot = 0.5947`
- `pots_wasted = 9.38`
- `food_eaten = 20.0`
- `avg_hp_on_food = 0.8899`
- `food_wasted = 15.87`
- `tokxil_melee_ticks = 11.20`
- `ketzek_melee_ticks = 8.12`

Key progression points:
- `reached_wave_30 >= 0.90` by `811.6M`
- `reached_wave_31 >= 0.90` by `895.5M`
- never sampled `wave_reached >= 40`
- never sampled `wave_reached >= 45`
- never sampled `wave_reached >= 50`
- never sampled `wave_reached >= 55`

Comparison to `v20.2` (`4o8gv87z`):
- `wave_reached: 38.32 vs 55.83`
- `max_wave: 49 vs 60`
- `most_npcs_slayed: 195 vs 264`
- `episode_return: 8819.71 vs 19112.53`
- `episode_length: 4650.34 vs 8246.68`
- `reached_wave_30: 0.9708 vs 1.0`
- `reached_wave_31: 0.9417 vs 1.0`
- `first reached_wave_30 >= 0.90: 812M vs 436M`
- `first reached_wave_31 >= 0.90: 895M vs 451M`
- `never reached sampled wave_reached >= 40` vs `948M`
- `prayer_uptime: 0.779 vs 0.824`
- `correct_prayer: 1004.14 vs 1837.12`
- `wrong_prayer_hits: 176.15 vs 155.01`
- `no_prayer_hits: 58.86 vs 23.55`
- `damage_blocked: 71646 vs 204649`
- `dmg_taken_avg: 4080 vs 5241`
- `prayer_switches: 57.6 vs 485.9`
- `attack_when_ready_rate: 0.465 vs 0.657`
- `tokxil_melee_ticks: 11.20 vs 2.14`
- `ketzek_melee_ticks: 8.12 vs 4.21`
- `food_eaten: 20.0 vs 15.5`
- `food_wasted: 15.87 vs 7.57`
- `pots_wasted: 9.38 vs 3.99`

Comparison to `v20.1` (`rp7a0y2e`):
- `wave_reached: 38.32 vs 44.11`
- `max_wave: 49 vs 50`
- `most_npcs_slayed: 195 vs 204`
- `episode_return: 8819.71 vs 11739.62`
- `episode_length: 4650.34 vs 5771.71`
- `reached_wave_31: 0.9417 vs 0.9948`
- `first reached_wave_30 >= 0.90: 812M vs 655M`
- `first reached_wave_31 >= 0.90: 895M vs 666M`
- `never reached sampled wave_reached >= 40` vs `1.840B`
- `prayer_uptime: 0.779 vs 0.980`
- `correct_prayer: 1004.14 vs 1681.56`
- `wrong_prayer_hits: 176.15 vs 247.08`
- `no_prayer_hits: 58.86 vs 8.49`
- `damage_blocked: 71646 vs 127795`
- `dmg_taken_avg: 4080 vs 4117`
- `prayer_switches: 57.6 vs 401.6`
- `attack_when_ready_rate: 0.465 vs 0.535`
- `tokxil_melee_ticks: 11.20 vs 12.78`
- `ketzek_melee_ticks: 8.12 vs 13.78`
- `food_wasted: 15.87 vs 16.60`
- `pots_wasted: 9.38 vs 6.26`

Comparison to `v20` (`t4eudsav`):
- `wave_reached: 38.32 vs 44.66`
- `max_wave: 49 vs 51`
- `most_npcs_slayed: 195 vs 211`
- `episode_return: 8819.71 vs 12149.99`
- `episode_length: 4650.34 vs 5904.67`
- `reached_wave_31: 0.9417 vs 0.9946`
- `first reached_wave_30 >= 0.90: 812M vs 302M`
- `first reached_wave_31 >= 0.90: 895M vs 1.009B`
- `never reached sampled wave_reached >= 40` vs `2.141B`
- `prayer_uptime: 0.779 vs 0.966`
- `correct_prayer: 1004.14 vs 1997.74`
- `wrong_prayer_hits: 176.15 vs 231.66`
- `no_prayer_hits: 58.86 vs 7.79`
- `damage_blocked: 71646 vs 131147`
- `dmg_taken_avg: 4080 vs 4093`
- `prayer_switches: 57.6 vs 682.1`
- `attack_when_ready_rate: 0.465 vs 0.537`
- `tokxil_melee_ticks: 11.20 vs 18.16`
- `ketzek_melee_ticks: 8.12 vs 20.83`
- `food_wasted: 15.87 vs 16.83`
- `pots_wasted: 9.38 vs 8.85`

Comparison to `v_tmp2.1` / `ro7h07qm`:
- `wave_reached: 38.32 vs 46.12`
- `max_wave: 49 vs 62`
- `most_npcs_slayed: 195 vs 270`
- `episode_return: 8819.71 vs 12739.18`
- `episode_length: 4650.34 vs 6066.53`
- `reached_wave_30: 0.9708 vs 1.0`
- `reached_wave_31: 0.9417 vs 0.9815`
- `first reached_wave_30 >= 0.90: 812M vs 499M`
- `first reached_wave_31 >= 0.90: 895M vs 499M`
- `never reached sampled wave_reached >= 40` vs `1.743B`
- `prayer_uptime: 0.779 vs 0.665`
- `correct_prayer: 1004.14 vs 1339.86`
- `wrong_prayer_hits: 176.15 vs 191.61`
- `no_prayer_hits: 58.86 vs 26.04`
- `damage_blocked: 71646 vs 122208`
- `dmg_taken_avg: 4080 vs 5132`
- `prayer_switches: 57.6 vs 1212.6`
- `attack_when_ready_rate: 0.465 vs 0.579`
- `tokxil_melee_ticks: 11.20 vs 7.04`
- `ketzek_melee_ticks: 8.12 vs 9.39`
- `food_wasted: 15.87 vs 10.65`
- `pots_wasted: 9.38 vs 23.47`

Interpretation:
- `v20.3` answers the reward-magnitude question directly:
  - the aggressive prayer reward settings are bad even when snapshot timing is
    removed
- this run did not become the same kind of high-prayer camper as `v20` /
  `v20.1`; instead it became a much lower-switch, lower-tempo, range-biased
  policy that stalled near the low-30 shelf
- the strongest failure signatures are:
  - `prayer_switches = 57.6`
  - `attack_when_ready_rate = 0.465`
  - `no_prayer_hits = 58.86`
  - never reaching sampled `wave_reached >= 40`
- compared with `v20.2`, this is not a small degradation; it is a completely
  different and much weaker policy regime

Most likely takeaway:
- aggressive generic prayer reward magnitude is independently harmful
- the `v20` regression was not only about snapshot timing
- the strongest recipe remains the mild `v_tmp2.1` prayer reward band paired
  with resolve-time reward and the 1-tick flick fix

---

## v20.2 (2026-04-09, completed)

Actual run:
- `4o8gv87z`
- W&B run name:
  - `valiant-spaceship-109`

Status:
- completed normally

Goal:
- isolate the 1-tick prayer drain fix by itself
- remove the new snapshot-timed prayer reward change from the experiment
- keep the successful `v_tmp2.1` / `ro7h07qm` reward magnitudes

Config:
- same PPO recipe as `v_tmp2.1`
- no curriculum
- `2.5B` budget
- same loadout and same non-prayer shaping terms as `v_tmp2.1`

Backend changes versus `v_tmp2.1` / `ro7h07qm`:
- keep the 1-tick prayer drain fix:
  - prayer drain only accrues if prayer was active at the start of the tick and
    is still active after that tick's action processing
  - perfect 1-tick flicking is therefore free
- revert generic danger-prayer reward timing to the original state:
  - prayer correctness reward fires on resolve, not on queue / lock
- generic danger-prayer coverage remains on the original path:
  - reward is driven by the old resolve-time danger-prayer signal

Exact active config:
- run setup: `load_model_path=null`, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`4o8gv87z`):
- completed normally
- final trainer step: `2,495,610,880 / 2,500,000,000`
- runtime: `1223s`
- throughput: `2.08M SPS`

Final metrics:
- `wave_reached = 55.83`
- `max_wave = 60`
- `most_npcs_slayed = 264`
- `episode_return = 19112.53`
- `episode_length = 8246.68`
- `reached_wave_30 = 1.0`
- `reached_wave_31 = 1.0`
- `prayer_uptime = 0.8238`
- `prayer_uptime_melee = 0.0417`
- `prayer_uptime_range = 0.2483`
- `prayer_uptime_magic = 0.5338`
- `correct_prayer = 1837.12`
- `wrong_prayer_hits = 155.01`
- `no_prayer_hits = 23.55`
- `damage_blocked = 204648.86`
- `dmg_taken_avg = 5241.05`
- `prayer_switches = 485.93`
- `attack_when_ready_rate = 0.6571`
- `pots_used = 31.66`
- `avg_prayer_on_pot = 0.5217`
- `pots_wasted = 3.99`
- `food_eaten = 15.5`
- `avg_hp_on_food = 0.7686`
- `food_wasted = 7.57`
- `tokxil_melee_ticks = 2.14`
- `ketzek_melee_ticks = 4.21`

Key progression points:
- `reached_wave_30 >= 0.90` by `436.2M`
- `reached_wave_31 >= 0.90` by `450.9M`
- first sampled `wave_reached >= 40` by `947.9M`
- first sampled `wave_reached >= 45` by `1.028B`
- first sampled `wave_reached >= 50` by `1.193B`
- first sampled `wave_reached >= 55` by `1.351B`

Comparison to `v_tmp2.1` / `ro7h07qm`:
- `wave_reached: 55.83 vs 46.12`
- `max_wave: 60 vs 62`
- `most_npcs_slayed: 264 vs 270`
- `episode_return: 19112.53 vs 12739.18`
- `episode_length: 8246.68 vs 6066.53`
- `reached_wave_30: 1.0 vs 1.0`
- `reached_wave_31: 1.0 vs 0.9815`
- `first reached_wave_30 >= 0.90: 436M vs 491M`
- `first reached_wave_31 >= 0.90: 451M vs 499M`
- `first wave_reached >= 40: 948M vs 1.743B`
- `first wave_reached >= 45: 1.028B vs 2.009B`
- `first wave_reached >= 50: 1.193B vs never sampled`
- `prayer_uptime: 0.824 vs 0.665`
- `prayer_switches: 486 vs 1213`
- `attack_when_ready_rate: 0.657 vs 0.579`
- `correct_prayer: 1837.12 vs 1339.86`
- `wrong_prayer_hits: 155.01 vs 191.61`
- `no_prayer_hits: 23.55 vs 26.04`
- `damage_blocked: 204649 vs 122208`
- `dmg_taken_avg: 5241 vs 5132`
- `tokxil_melee_ticks: 2.14 vs 7.04`
- `ketzek_melee_ticks: 4.21 vs 9.39`
- `food_eaten: 15.5 vs 18.83`
- `food_wasted: 7.57 vs 10.65`
- `pots_wasted: 3.99 vs 23.47`

Comparison to `v20.1` (`rp7a0y2e`):
- `wave_reached: 55.83 vs 44.11`
- `max_wave: 60 vs 50`
- `most_npcs_slayed: 264 vs 204`
- `episode_return: 19112.53 vs 11739.62`
- `episode_length: 8246.68 vs 5771.71`
- `reached_wave_31: 1.0 vs 0.9948`
- `first reached_wave_30 >= 0.90: 436M vs 655M`
- `first reached_wave_31 >= 0.90: 451M vs 666M`
- `first wave_reached >= 40: 948M vs 1.840B`
- `first wave_reached >= 45: 1.028B vs never sampled`
- `prayer_uptime: 0.824 vs 0.980`
- `correct_prayer: 1837.12 vs 1681.56`
- `wrong_prayer_hits: 155.01 vs 247.08`
- `no_prayer_hits: 23.55 vs 8.49`
- `damage_blocked: 204649 vs 127795`
- `dmg_taken_avg: 5241 vs 4117`
- `prayer_switches: 486 vs 402`
- `attack_when_ready_rate: 0.657 vs 0.535`
- `tokxil_melee_ticks: 2.14 vs 12.78`
- `ketzek_melee_ticks: 4.21 vs 13.78`
- `food_eaten: 15.5 vs 20.0`
- `food_wasted: 7.57 vs 16.60`
- `pots_wasted: 3.99 vs 6.26`

Comparison to `v20` (`t4eudsav`):
- `wave_reached: 55.83 vs 44.66`
- `max_wave: 60 vs 51`
- `most_npcs_slayed: 264 vs 211`
- `episode_return: 19112.53 vs 12149.99`
- `episode_length: 8246.68 vs 5904.67`
- `reached_wave_31: 1.0 vs 0.9946`
- `first reached_wave_30 >= 0.90: 436M vs 302M`
- `first reached_wave_31 >= 0.90: 451M vs 1.009B`
- `first wave_reached >= 40: 948M vs 2.141B`
- `first wave_reached >= 45: 1.028B vs 2.489B`
- `prayer_uptime: 0.824 vs 0.966`
- `correct_prayer: 1837.12 vs 1997.74`
- `wrong_prayer_hits: 155.01 vs 231.66`
- `no_prayer_hits: 23.55 vs 7.79`
- `damage_blocked: 204649 vs 131147`
- `dmg_taken_avg: 5241 vs 4093`
- `prayer_switches: 486 vs 682`
- `attack_when_ready_rate: 0.657 vs 0.537`
- `tokxil_melee_ticks: 2.14 vs 18.16`
- `ketzek_melee_ticks: 4.21 vs 20.83`
- `food_eaten: 15.5 vs 20.0`
- `food_wasted: 7.57 vs 16.83`
- `pots_wasted: 3.99 vs 8.85`

Interpretation:
- `v20.2` is not just a recovery from `v20` / `v20.1`; it is a major step
  forward over the whole `v_tmp2.1` line on average depth and late-wave
  stability
- the 1-tick flick fix appears strongly positive when paired with the old
  resolve-time reward timing
- compared with `v_tmp2.1`, the policy became:
  - deeper on average
  - much faster at breaking into the 40+ and 45+ ranges
  - much more attack-ready
  - far cleaner on Tok-Xil / Ket-Zek melee exposure
  - much less wasteful on food and pot usage
- the cost is narrow and understandable:
  - slightly higher total damage taken
  - somewhat higher prayer uptime than the old baseline
  - lower prayer-switch count, but without losing prayer correctness

Most likely takeaway:
- the snapshot-timed prayer reward was the actual regression source in the
  `v20` / `v20.1` line
- the 1-tick prayer drain fix itself is highly likely to be a real positive
  change
- this run argues for keeping:
  - the 1-tick flick fix
  - the older resolve-time prayer reward path
  - the milder `v_tmp2.1` prayer reward magnitudes
- `v20.3` is still useful as a final control for aggressive reward magnitude,
  but `v20.2` is already strong evidence that reward timing, not flick logic,
  caused the earlier prayer-camping regressions

---

## v20.1 (2026-04-09, completed)

Actual run:
- `rp7a0y2e`

Status:
- completed normally

Goal:
- keep the backend-side prayer timing improvements from `v20`
  - 1-tick prayer flick drain fix
  - snapshot-timed prayer reward
  - melee / ranged / magic coverage in the generic danger-prayer reward
- revert the config knobs back to the successful `v_tmp2.1` / `ro7h07qm`
  prayer-reward magnitudes

Config:
- same PPO recipe as `v_tmp2.1`
- no curriculum
- `2.5B` budget
- same loadout and same non-prayer shaping terms as `v_tmp2.1`

Backend changes versus `v_tmp2.1` / `ro7h07qm`:
- 1-tick prayer drain fix:
  - prayer drain only accrues if prayer was active at the start of the tick and
    is still active after that tick's action processing
  - perfect 1-tick flicking is therefore free
- generic danger-prayer reward timing:
  - immediate-snapshot attacks reward on the queue tick
  - delayed-snapshot attacks reward on the lock tick
  - reward no longer waits for the later damage-resolve tick
- generic danger-prayer coverage:
  - now covers melee, ranged, and magic
  - not just ranged / magic

Config changes versus `v20`:
- restore `w_correct_danger_prayer` from `1.75` back to `0.25`
- restore `shape_npc_specific_prayer_bonus` from `0.0` back to `1.5`

Exact active config:
- run setup: `load_model_path=null`, snapshot-timed generic prayer reward, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.25`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_specific_prayer_bonus=1.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`rp7a0y2e`):
- completed normally
- final trainer step: `2,499,805,184 / 2,500,000,000`
- runtime: `1225s`
- throughput: `2.01M SPS`

Final metrics:
- `wave_reached = 44.11`
- `max_wave = 50`
- `most_npcs_slayed = 204`
- `episode_return = 11739.62`
- `episode_length = 5771.71`
- `reached_wave_30 = 0.9948`
- `reached_wave_31 = 0.9948`
- `prayer_uptime = 0.9796`
- `wrong_prayer_hits = 247.08`
- `no_prayer_hits = 8.49`
- `dmg_taken_avg = 4117.20`
- `prayer_switches = 401.55`
- `attack_when_ready_rate = 0.5352`
- `tokxil_melee_ticks = 12.78`
- `ketzek_melee_ticks = 13.78`
- `pots_used = 30.91`
- `food_eaten = 20.0`
- `food_wasted = 16.60`
- `pots_wasted = 6.26`

Key progression points:
- `reached_wave_30 >= 0.90` by `655.4M`
- `reached_wave_31 >= 0.90` by `665.8M`
- first sampled `wave_reached >= 40` by `1.840B`
- never sampled `wave_reached >= 45`

Comparison to `v_tmp2.1` / `ro7h07qm`:
- `wave_reached: 44.11 vs 46.12`
- `max_wave: 50 vs 62`
- `most_npcs_slayed: 204 vs 270`
- `episode_return: 11739.62 vs 12739.18`
- `reached_wave_30: 0.9948 vs 1.0`
- `reached_wave_31: 0.9948 vs 0.9815`
- `first reached_wave_30 >= 0.90: 666M vs 499M`
- `first reached_wave_31 >= 0.90: 666M vs 499M`
- `first wave_reached >= 40: 1.840B vs 1.743B`
- `never reached sampled wave_reached >= 45` vs `2.009B`
- `prayer_uptime: 0.980 vs 0.665`
- `wrong_prayer_hits: 247.08 vs 191.61`
- `no_prayer_hits: 8.49 vs 26.04`
- `dmg_taken_avg: 4117 vs 5132`
- `prayer_switches: 402 vs 1213`
- `attack_when_ready_rate: 0.535 vs 0.579`
- `tokxil_melee_ticks: 12.78 vs 7.04`
- `ketzek_melee_ticks: 13.78 vs 9.39`
- `food_wasted: 16.60 vs 10.65`
- `pots_wasted: 6.26 vs 23.47`

Comparison to `v20` (`t4eudsav`):
- `wave_reached: 44.11 vs 44.66`
- `max_wave: 50 vs 51`
- `most_npcs_slayed: 204 vs 211`
- `episode_return: 11739.62 vs 12149.99`
- `prayer_uptime: 0.980 vs 0.966`
- `wrong_prayer_hits: 247.08 vs 231.66`
- `dmg_taken_avg: 4117 vs 4093`
- `prayer_switches: 402 vs 682`
- `tokxil_melee_ticks: 12.78 vs 18.16`
- `ketzek_melee_ticks: 13.78 vs 20.83`
- `first reached_wave_31 >= 0.90: 666M vs 1.009B`
- `first wave_reached >= 40: 1.840B vs 2.141B`

Interpretation:
- `v20.1` is clearly healthier than `v20`, but it still regressed from the
  strong `v_tmp2.1` / `ro7h07qm` line
- the snapshot-timed reward with the mild old reward magnitudes still pushed
  the policy toward heavy prayer camping:
  - very high prayer uptime
  - very low no-prayer exposure
  - much lower prayer switching than the good baseline
- compared with `v20`, restoring the NPC-specific bonus and lowering generic
  prayer reward did help:
  - much better Tok-Xil / Ket-Zek melee exposure
  - much earlier wave-31 stability
  - earlier climb into the 40+ range
- but it still did not recover late-cave ceiling or switching tempo

Most likely takeaway:
- the main regression source is the snapshot-timed prayer reward itself, not
  just the stronger `v20` reward magnitudes
- `v20.2` is the right next control:
  - keep the 1-tick flick fix
  - remove snapshot-timed reward
  - keep the known-good `v_tmp2.1` reward magnitudes

Re-implementing snapshot-timed reward later:
- the reverted implementation was small and localized
- to restore it:
  - add a helper like `fc_note_prayer_snapshot(...)` in `fc_combat.[ch]`
  - call it in [fc_npc.c](../fc-core/src/fc_npc.c) immediately after `queued->prayer_snapshot = p->prayer` for immediate-snapshot attacks
  - call it in [fc_combat.c](../fc-core/src/fc_combat.c) when delayed snapshots fill at `state->tick >= h->prayer_lock_tick`
  - remove the resolve-time `correct_danger_prayer` / `wrong_danger_prayer` flag writes from the later hit-resolution block
  - if desired, broaden the generic signal to include melee there as `v20` / `v20.1` did

---

## v20 (2026-04-09, completed)

Actual run:
- `t4eudsav`

Status:
- completed normally

Goal:
- push prayer learning earlier in the credit-assignment chain
- reward the policy on the tick the backend decides which prayer snapshot will
  govern the incoming hit, rather than waiting for the later resolve tick
- remove the NPC-specific prayer bonus from this trial so the policy learns the
  generic timing rule instead of leaning on monster-specific shaping

Config:
- same PPO recipe as `v_tmp2.1`
- no curriculum
- `2.5B` budget
- same loadout and all non-prayer shaping terms as `v_tmp2.1`

Backend changes versus `v_tmp2.1` / `ro7h07qm`:
- 1-tick prayer drain fix:
  - prayer drain only accrues if prayer was active at the start of the tick and
    is still active after that tick's action processing
  - perfect 1-tick flicking is therefore free
- generic danger-prayer reward timing:
  - immediate-snapshot attacks reward on the queue tick
  - delayed-snapshot attacks reward on the lock tick
  - reward no longer waits for the later damage-resolve tick
- generic danger-prayer coverage:
  - now covers melee, ranged, and magic
  - not just ranged / magic
- NPC-specific prayer bonus:
  - backend path kept
  - config disabled for this run

Exact active config:
- run setup: `load_model_path=null`, snapshot-timed generic prayer reward, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=1.75`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.25`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`t4eudsav`):
- completed normally
- final trainer step: `2,495,610,880 / 2,500,000,000`
- runtime: `1226s`
- throughput: `2.03M SPS`

Final metrics:
- `wave_reached = 44.66`
- `max_wave = 51`
- `most_npcs_slayed = 211`
- `episode_return = 12149.99`
- `episode_length = 5904.67`
- `reached_wave_30 = 0.9946`
- `reached_wave_31 = 0.9946`
- `prayer_uptime = 0.9659`
- `wrong_prayer_hits = 231.66`
- `no_prayer_hits = 7.79`
- `dmg_taken_avg = 4093.19`
- `prayer_switches = 682.05`
- `attack_when_ready_rate = 0.5372`
- `tokxil_melee_ticks = 18.16`
- `ketzek_melee_ticks = 20.83`
- `pots_used = 31.50`
- `food_eaten = 20.0`
- `food_wasted = 16.83`
- `pots_wasted = 8.85`

Key progression points:
- `reached_wave_30 >= 0.90` by `302.0M`
- `reached_wave_31 >= 0.90` by `1.009B`
- first sampled `wave_reached >= 40` by `2.141B`
- first sampled `wave_reached >= 45` by `2.489B`

Comparison to `v_tmp2.1` / `ro7h07qm`:
- `wave_reached: 44.66 vs 46.12`
- `max_wave: 51 vs 62`
- `most_npcs_slayed: 211 vs 270`
- `episode_return: 12149.99 vs 12739.18`
- `reached_wave_30: 0.9946 vs 1.0`
- `reached_wave_31: 0.9946 vs 0.9815`
- `first reached_wave_30 >= 0.90: 302M vs 491M`
- `first reached_wave_31 >= 0.90: 1.009B vs 499M`
- `first wave_reached >= 40: 2.141B vs 1.743B`
- `prayer_uptime: 0.966 vs 0.665`
- `wrong_prayer_hits: 231.66 vs 191.61`
- `no_prayer_hits: 7.79 vs 26.04`
- `dmg_taken_avg: 4093 vs 5132`
- `prayer_switches: 682 vs 1213`
- `attack_when_ready_rate: 0.537 vs 0.579`
- `tokxil_melee_ticks: 18.16 vs 7.04`
- `ketzek_melee_ticks: 20.83 vs 9.39`
- `food_wasted: 16.83 vs 10.65`
- `pots_wasted: 8.85 vs 23.47`

Interpretation:
- `v20` did not improve late-cave performance; it regressed from the strong
  `v_tmp2.1` / `ro7h07qm` line
- the most obvious shift is toward very conservative prayer camping:
  - much higher prayer uptime
  - far fewer switches
  - far fewer no-prayer hits
- that conservative behavior helped early wave-30 consistency, but it did not
  convert into deeper cave performance
- the later-wave handling got worse, not better:
  - lower final depth
  - much lower ceiling
  - far worse Tok-Xil / Ket-Zek melee exposure
  - slower climb from wave 31 into the 40+ range

Most likely takeaway:
- the backend-side timing changes themselves still look promising
- the aggressive reward change in `v20` was the problem:
  - `w_correct_danger_prayer = 1.75` plus disabling the NPC-specific bonus
    appears to have over-incentivized “always keep something protective on”
    rather than fast switching between threats
- this run argues for keeping the new backend timing semantics but restoring the
  milder `v_tmp2.1` reward magnitudes, which is exactly what `v20.1` does

---

## Temporary Comparison Summary (2026-04-08)

These three scratch runs were designed to isolate the impact of the prayer timing fix and the later obs / reward follow-up changes, while holding the training recipe constant.

Final ranking by real late-wave progress:
- `v_tmp2` (`fkhhysfd`, prayer fix only) — strongest final performer
- `v_tmp3` (`fg029tll`, pre-prayer baseline) — also very strong, but less stable / less disciplined than `v_tmp2`
- `v_tmp1` (`qyekq4z3`, prayer fix + obs / reward follow-up) — clear regression

What changed:
- `v_tmp1 -> v_tmp2`:
  - removing the later obs / reward follow-up changes while keeping the prayer timing fix produced a massive recovery
  - this is the strongest evidence in the set that the regression came from the obs / reward follow-up work, not from the prayer timing change itself
- `v_tmp3 -> v_tmp2`:
  - adding the prayer timing fix improved stability and discipline without sacrificing late-wave performance
  - `v_tmp2` took far less damage, switched prayers much less, and had lower Tok-Xil melee exposure than `v_tmp3`
  - `v_tmp3` still showed some upside in ceiling metrics (`max_wave = 60` vs `53`, `most_npcs_slayed = 263` vs `222`), but its overall behavior was noisier and more wasteful
- versus `v19.3`:
  - both `v_tmp2` and `v_tmp3` converted wave-30 entries into wave-31+ much more consistently than `v19.3`
  - `v19.3` remained much cleaner on damage taken, wrong-prayer hits, switching rate, and resource use

Current interpretation:
- the prayer timing fix looks directionally correct and beneficial
- the later `npc_type` obs addition and/or resolved-hit prayer reward follow-up in `v_tmp1` likely caused the collapse
- the next clean path is to treat `v_tmp2` as the strongest current branch and reintroduce follow-up changes one at a time instead of bundling them

---

## v_tmp1 (2026-04-08, completed)

Temporary comparison run:
- actual run id:
  - `qyekq4z3`
- code snapshot:
  - `post-obs-post-prayer-2026-04-08`
  - commit `58768c5b8fd4bd4a3a16c97a7f8db5c752bfa8ee`

Config:
- scratch run
- no checkpoint load
- no curriculum
- `2.5B` budget
- same PPO / reward recipe as the `v19.3` family
- same current end-game loadout

Exact active config:
- run setup: `load_model_path=null`, early prayer-lock timing enabled, per-NPC `npc_type` obs enabled, locked-prayer resolved-hit reward follow-up enabled, `policy_obs=114`, `puffer_obs=150`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.5`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.0`, `shape_npc_specific_prayer_bonus=2.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Backend differences:
- versus `v_tmp2`:
  - adds early prayer-lock timing
  - adds `npc_type` back into policy obs
  - policy contract changes from `106 / 142` to `114 / 150`
  - resolved-hit prayer rewards use the locked prayer snapshot
- versus `v_tmp3`:
  - includes the prayer timing fix
  - includes the obs / reward follow-up changes above

Results (`qyekq4z3`):
- completed normally
- final trainer step: `2,499,805,184 / 2,500,000,000`
- runtime: `1175.7s`
- throughput: `2.13M SPS`

Final metrics:
- `episode_return = 7205.14`
- `episode_length = 5601.45`
- `wave_reached = 28.94`
- `max_wave = 34`
- `most_npcs_slayed = 125`
- `score = 0.0`
- `prayer_uptime = 0.982`
- `prayer_uptime_melee = 0.503`
- `prayer_uptime_range = 0.473`
- `prayer_uptime_magic = 0.006386`
- `correct_prayer = 2819.28`
- `wrong_prayer_hits = 152.87`
- `no_prayer_hits = 37.81`
- `prayer_switches = 1854.04`
- `damage_blocked = 54995.54`
- `dmg_taken_avg = 3395.16`
- `attack_when_ready_rate = 0.315`
- `pots_used = 27.20`
- `avg_prayer_on_pot = 0.514`
- `food_eaten = 20.00`
- `avg_hp_on_food = 0.923`
- `food_wasted = 19.35`
- `pots_wasted = 4.17`
- `tokxil_melee_ticks = 25.32`
- `ketzek_melee_ticks = 0.0`
- `reached_wave_30 = 0.0457`
- `cleared_wave_30 = 0.0051`
- `reached_wave_31 = 0.0051`

Analysis:
- this run underperformed `v19.3` badly on actual cave progress
- final `wave_reached` dropped from `30.66` to `28.94`
- `reached_wave_30` fell from `99.15%` to `4.57%`
- `cleared_wave_30 / reached_wave_31` fell from `54.04%` to `0.51%`
- the run looked more defensive than competent:
  - much higher prayer switching
  - much more blocked damage
  - but also much higher damage taken and more food use
- it looked best around the mid-run window, then regressed hard late instead of converting wave-30 reaches into stable clears

---

## v_tmp2 (2026-04-08, completed)

Temporary comparison run:
- actual run id:
  - `fkhhysfd`
- code snapshot:
  - `post-prayer-pre-obs-2026-04-08`
  - commit `c232f09b`

Config:
- scratch run
- no checkpoint load
- no curriculum
- `2.5B` budget
- same PPO / reward recipe as the `v19.3` family
- same current end-game loadout

Exact active config:
- run setup: `load_model_path=null`, prayer timing fix only, no `npc_type` obs follow-up, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.5`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.0`, `shape_npc_specific_prayer_bonus=2.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Backend differences:
- versus `v_tmp1`:
  - keeps the prayer timing fix
  - removes the `npc_type` obs addition
  - restores the old `106 / 142` policy contract
  - restores the pre-follow-up resolved-hit prayer reward path
- versus `v_tmp3`:
  - adds the prayer timing fix only

Results:
- completed normally
- final trainer step: `2,499,805,184 / 2,500,000,000`
- runtime: `1231.0s`
- throughput: `1.95M SPS`

Final metrics:
- `episode_return = 14745.91`
- `episode_length = 5678.44`
- `wave_reached = 44.72`
- `max_wave = 53`
- `most_npcs_slayed = 222`
- `score = 0.0`
- `prayer_uptime = 0.947`
- `prayer_uptime_melee = 0.264`
- `prayer_uptime_range = 0.238`
- `prayer_uptime_magic = 0.445`
- `correct_prayer = 1861.91`
- `wrong_prayer_hits = 464.25`
- `no_prayer_hits = 18.26`
- `prayer_switches = 1073.48`
- `damage_blocked = 132200.91`
- `dmg_taken_avg = 7470.13`
- `attack_when_ready_rate = 0.656`
- `pots_used = 30.32`
- `avg_prayer_on_pot = 0.607`
- `food_eaten = 19.36`
- `avg_hp_on_food = 0.650`
- `food_wasted = 4.32`
- `pots_wasted = 10.09`
- `tokxil_melee_ticks = 12.63`
- `ketzek_melee_ticks = 4.71`
- `reached_wave_30 = 0.9653`
- `cleared_wave_30 = 0.9653`
- `reached_wave_31 = 0.9653`

Analysis:
- this run was massively stronger than `v_tmp1` on real cave progress
- final `wave_reached` jumped from `28.94` to `44.72`
- `reached_wave_30` jumped from `4.57%` to `96.53%`
- `cleared_wave_30 / reached_wave_31` jumped from `0.51%` to `96.53%`
- `max_wave` jumped from `34` to `53`
- compared with `v19.3`, this run still reached wave 30 slightly less often (`96.53%` vs `99.15%`), but once it got there it converted vastly better (`96.53%` wave 31 vs `54.04%`)
- the tradeoff is that it was much sloppier and more brute-force:
  - much higher `wrong_prayer_hits`
  - much higher `dmg_taken_avg`
  - much higher prayer switching
  - heavier potion usage
- it also attacked far more consistently than both `v_tmp1` and `v19.3`
- Tok-Xil melee exposure improved sharply versus `v_tmp1` (`12.63` vs `25.32`), which points to the prayer-only snapshot retaining much stronger handling of mid/late cave spacing
- unlike `v_tmp1`, this run improved hard after the midpoint and held the gains through the finish instead of regressing late
- provisional takeaway:
  - the prayer timing change itself looks strongly positive
  - the later obs / reward follow-up changes in `v_tmp1` likely caused the regression
  - this snapshot is currently the strongest late-wave performer of the temporary comparison set, even though it is much less clean in damage and resource discipline than `v19.3`

---

## v_tmp3 (2026-04-08, completed)

Temporary comparison run:
- actual run id:
  - `fg029tll`
- code snapshot:
  - `pre-prayer-fix-2026-04-08`
  - commit `7c77cf93`

Config:
- scratch run
- no checkpoint load
- no curriculum
- `2.5B` budget
- same PPO / reward recipe as the `v19.3` family
- same current end-game loadout

Exact active config:
- run setup: `load_model_path=null`, pre-prayer-fix baseline, no `npc_type` obs follow-up, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.5`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.0`, `shape_npc_specific_prayer_bonus=2.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Backend differences:
- versus `v_tmp2`:
  - removes the prayer timing fix
  - keeps the older pre-follow-up obs / reward path
- versus `v_tmp1`:
  - removes both the prayer timing fix and the later obs / reward follow-up changes
  - represents the last committed pre-prayer baseline for this comparison set

Results:
- completed normally
- final trainer step: `2,499,805,184 / 2,500,000,000`
- runtime: `1206.6s`
- throughput: `2.05M SPS`

Final metrics:
- `episode_return = 12958.40`
- `episode_length = 6141.61`
- `wave_reached = 41.17`
- `max_wave = 60`
- `most_npcs_slayed = 263`
- `score = 0.0`
- `prayer_uptime = 0.794`
- `prayer_uptime_melee = 0.401`
- `prayer_uptime_range = 0.259`
- `prayer_uptime_magic = 0.133`
- `correct_prayer = 2706.46`
- `wrong_prayer_hits = 155.09`
- `no_prayer_hits = 32.17`
- `prayer_switches = 2238.11`
- `damage_blocked = 118229.65`
- `dmg_taken_avg = 3726.61`
- `attack_when_ready_rate = 0.349`
- `pots_used = 31.42`
- `avg_prayer_on_pot = 0.625`
- `food_eaten = 20.0`
- `avg_hp_on_food = 0.913`
- `food_wasted = 18.07`
- `pots_wasted = 11.53`
- `tokxil_melee_ticks = 17.81`
- `ketzek_melee_ticks = 6.52`
- `reached_wave_30 = 0.9641`
- `cleared_wave_30 = 0.9436`
- `reached_wave_31 = 0.9436`

Analysis:
- this pre-prayer baseline was far stronger than `v_tmp1` and only modestly weaker than `v_tmp2`
- versus `v_tmp1`:
  - `wave_reached` jumped from `28.94` to `41.17`
  - `reached_wave_30` jumped from `4.57%` to `96.41%`
  - `reached_wave_31` jumped from `0.51%` to `94.36%`
  - `max_wave` jumped from `34` to `60`
- versus `v_tmp2`:
  - slightly worse final late-wave conversion (`94.36%` wave 31 vs `96.53%`)
  - lower final `wave_reached` (`41.17` vs `44.72`)
  - much higher prayer switching (`2238` vs `1073`)
  - much higher Tok-Xil melee exposure (`17.81` vs `12.63`)
  - lower damage taken (`3726.61`) than `v_tmp2` (`7470.13`), but still higher than `v19.3`
- the run shape was strong early and peaked very high mid-run:
  - by ~`0.94B` it was already at `99.72%` wave-30 reach and `99.55%` wave-31 reach
  - it peaked near `49.87` wave average around ~`1.56B`
  - then regressed somewhat by the finish, but still ended as a very strong late-wave policy
- interpretation:
  - the pre-prayer baseline already had excellent ceiling and late-wave competence
  - the prayer timing fix in `v_tmp2` appears to have traded some ceiling for much better stability and much cleaner tactical behavior
  - the collapse in `v_tmp1` was therefore not caused by removing the old pre-prayer behavior alone; it points much more strongly at the later obs / reward follow-up changes

---

## v19.3 (2026-04-08, completed)

Naming note:
- this run used the exact wave-0 checkpoint-transfer recipe that had been
  staged immediately before launch
- to keep the run-history numbering aligned with the latest discussion, the
  executed run is recorded here as `v19.3`
- actual run id:
  - `hl0yb7qa`
- warm-start source run id:
  - `8u6flr5y` (`v19.1`)
- warm-start checkpoint used:
  - `0000000263192576.bin` (not retained locally)

Mainline direction:
- checkpoint transfer run
- warm-start from the best early `v19.1` checkpoint window
- no sweep
- no curriculum
- same backend / same current pruned obs / same no-Jad-telegraph logic

Why `v19.3` exists:
- `v19.1` proved that direct wave-31 exposure can teach real late-wave
  mechanics very quickly
- the next question was whether that competence would transfer back to
  unbiased full-cave play from wave 1
- `v19.3` is the clean test of that transfer:
  - keep the exact `v19` reward / PPO recipe
  - remove curriculum completely
  - warm-start from the best `v19.1` checkpoint before the curriculum run
    regressed

Budget:
- `v19.1`: `2.5B`
- `v19.3`: `2.5B`

Run / trainer shape:
- continuation run, not scratch
- no `[sweep]` section
- no curriculum buckets
- same PPO / reward recipe as `v19`
- warm-start from:
  - `8u6flr5y/0000000263192576.bin`

What stays the same as `v19`:
- `learning_rate = 0.0003`
- `anneal_lr = 0`
- `clip_coef = 0.2`
- `ent_coef = 0.01`
- `num_buffers = 2`
- same reward weights and shaping
- same `w_attack_attempt = 0.1`
- same `2.5B` budget used in `v19.1`

What changes versus `v19.1`:
- `load_model_path: None -> 8u6flr5y/0000000263192576.bin`
- `curriculum_wave: 31 -> 0`
- `curriculum_pct: 1.0 -> 0.0`
- all curriculum buckets disabled

Exact active config:
- run setup: `load_model_path=<repo-root>/pufferlib_4/checkpoints/fight_caves/8u6flr5y/0000000263192576.bin`, no curriculum, `policy_obs=106`, `puffer_obs=142`
- reward weights: `w_damage_dealt=0.5`, `w_attack_attempt=0.1`, `w_damage_taken=-0.6`, `w_npc_kill=3.0`, `w_wave_clear=10.0`, `w_jad_damage=2.0`, `w_jad_kill=50.0`, `w_player_death=-20.0`, `w_cave_complete=100.0`, `w_correct_danger_prayer=0.5`, `w_invalid_action=-0.1`, `w_tick_penalty=-0.005`
- shaping: `shape_food_full_waste_penalty=-6.5`, `shape_food_waste_scale=-1.2`, `shape_food_safe_hp_threshold=1.0`, `shape_pot_full_waste_penalty=-6.5`, `shape_pot_waste_scale=-1.2`, `shape_pot_safe_prayer_threshold=1.0`, `shape_wrong_prayer_penalty=-1.0`, `shape_npc_specific_prayer_bonus=2.5`, `shape_npc_melee_penalty=-0.3`, `shape_wasted_attack_penalty=-0.1`, `shape_wave_stall_start=500`, `shape_wave_stall_base_penalty=-0.5`, `shape_wave_stall_ramp_interval=50`, `shape_wave_stall_cap=-2.0`, `shape_not_attacking_grace_ticks=2`, `shape_not_attacking_penalty=-0.01`, `shape_kiting_reward=1.0`, `shape_kiting_min_dist=5`, `shape_kiting_max_dist=7`, `shape_unnecessary_prayer_penalty=-0.2`, `shape_resource_threat_window=2`
- runtime: `total_agents=4096`, `num_buffers=2`, `total_timesteps=2_500_000_000`, `learning_rate=0.0003`, `gamma=0.999`, `gae_lambda=0.95`, `clip_coef=0.2`, `vf_coef=0.5`, `ent_coef=0.01`, `max_grad_norm=0.5`, `horizon=256`, `minibatch_size=4096`, `hidden_size=256`, `num_layers=2`

Results (`hl0yb7qa`):
- completed normally
- continuation run confirmed:
  - `load_model_path = 8u6flr5y/0000000263192576.bin`
  - no sweep
  - no curriculum
- final trainer step: `2,499,805,184 / 2,500,000,000`
- runtime: `1154.2s`
- throughput: `2.18M SPS`

Final metrics:
- `episode_return = 5539.23`
- `episode_length = 4869.51`
- `wave_reached = 30.66`
- `max_wave = 33`
- `most_npcs_slayed = 124`
- `score = 0.0`
- `prayer_uptime = 0.896`
- `prayer_uptime_melee = 0.356`
- `prayer_uptime_range = 0.540`
- `prayer_uptime_magic = 0.000001`
- `correct_prayer = 1785.32`
- `wrong_prayer_hits = 70.83`
- `no_prayer_hits = 35.02`
- `prayer_switches = 600.17`
- `damage_blocked = 25106.45`
- `dmg_taken_avg = 2502.86`
- `attack_when_ready_rate = 0.301`
- `pots_used = 23.80`
- `avg_prayer_on_pot = 0.562`
- `food_eaten = 13.63`
- `avg_hp_on_food = 0.905`
- `food_wasted = 12.23`
- `pots_wasted = 4.57`
- `tokxil_melee_ticks = 11.99`
- `ketzek_melee_ticks = 0.56`
- `reached_wave_30 = 0.991`
- `cleared_wave_30 = 0.540`
- `reached_wave_31 = 0.540`

Trajectory:
- `126M` steps:
  - `wave_reached = 24.1`
  - `episode_return = 3225.1`
- `239M` steps:
  - `wave_reached = 28.1`
  - `episode_return = 4533.9`
- `369M` steps:
  - `wave_reached = 29.8`
  - `episode_return = 5085.4`
- `973M` steps:
  - `wave_reached = 30.0`
  - `episode_return = 5358.5`
- `1.26B` steps:
  - `wave_reached = 30.0`
  - `episode_return = 5890.7`
- `1.89B` steps:
  - `wave_reached = 30.0`
  - `episode_return = 7203.7`
- `2.26B` steps:
  - `wave_reached = 30.5`
  - `episode_return = 7413.9`
  - this was the strongest window
- final:
  - `wave_reached = 30.66`
  - `episode_return = 5539.23`

What went right:
- this is the best **unbiased full-cave average-wave run so far**
  on the current stack
- it is the first run to combine:
  - `wave_reached > 30.5`
  - `cleared_wave_30 > 0.5`
  - much lower food/pot use
  - much lower no-prayer exposure
- transfer from the `v19.1` curriculum checkpoint was clearly real

Comparison to `v19` (`12gtkmfc`):
- `wave_reached: 30.66 vs 29.29`
- `max_wave: 33 vs 33`
- `episode_return: 5539 vs 5462`
- `reached_wave_30: 0.991 vs 0.594`
- `cleared_wave_30: 0.540 vs 0.0`
- `reached_wave_31: 0.540 vs 0.0`
- `dmg_taken_avg: 2503 vs 3016`
- `pots_used: 23.8 vs 30.4`
- `food_eaten: 13.6 vs 20.0`
- `avg_prayer_on_pot: 0.562 vs 0.762`
- `pots_wasted: 4.57 vs 20.38`
- `food_wasted: 12.23 vs 19.33`
- `no_prayer_hits: 35.0 vs 119.6`
- `wrong_prayer_hits: 70.8 vs 84.0`
- `damage_blocked: 25106 vs 16176`

Interpretation versus `v19`:
- the checkpoint transfer worked
- the biggest gains were not raw late-wave mechanics by themselves
- the biggest gains were:
  - much better resource conservation
  - much lower unprotected damage exposure
  - much stronger conversion of wave-30 entries into actual wave-31 starts
- this strongly supports the view that the old `29-30` wall was not just
  “missing Ket-Zek reps”; it was also a resource / tempo wall

Comparison to `v19.1` (`8u6flr5y`):
- direct raw depth is not comparable because `v19.1` used forced wave-31
  curriculum
- useful side-metric comparisons:
  - `dmg_taken_avg: 2503 vs 4162`
  - `pots_used: 23.8 vs 29.2`
  - `food_eaten: 13.6 vs 19.9`
  - `avg_prayer_on_pot: 0.562 vs 0.434`
  - `pots_wasted: 4.57 vs 4.63`
  - `food_wasted: 12.23 vs 14.83`
  - `attack_when_ready_rate: 0.301 vs 0.138`
  - `prayer_uptime_magic: ~0.000 vs 0.483`

Interpretation versus `v19.1`:
- the curriculum run successfully taught something transferable, but it was
  **not** mainly durable magic-prayer behavior
- what transferred best was:
  - better offensive tempo
  - better potion/food discipline
  - lower damage intake
  - better overall conversion through the `29-30` wall
- what did **not** transfer cleanly was late-wave magic-prayer usage
- `prayer_uptime_magic` collapsing back to ~0 is one of the strongest signs
  that this run did not preserve the full Ket-Zek-specific behavior from
  `v19.1`

Comparison to `v18.1` (`xm6i52ta`):
- `wave_reached: 30.66 vs 29.24`
- `max_wave: 33 vs 34`
- `episode_return: 5539 vs 5535`
- `reached_wave_30: 0.991 vs 0.306`
- `cleared_wave_30: 0.540 vs 0.0`
- `reached_wave_31: 0.540 vs 0.0`
- `dmg_taken_avg: 2503 vs 3580`
- `pots_used: 23.8 vs 31.5`
- `food_eaten: 13.6 vs 20.0`
- `avg_prayer_on_pot: 0.562 vs 0.690`
- `pots_wasted: 4.57 vs 13.3`
- `food_wasted: 12.23 vs 19.4`
- `no_prayer_hits: 35.0 vs 77.5`
- `wrong_prayer_hits: 70.8 vs 143.4`
- `damage_blocked: 25106 vs 34386`

Interpretation versus `v18.1`:
- `v19.3` is much more efficient and much more practical on whole-cave metrics
- it does not match `v18.1` in raw prayer/blocking volume, but it no longer
  needs to; it is getting deeper while spending far fewer resources
- the trade is:
  - less overall blocking activity
  - much better efficiency and much better milestone conversion

Comparison to `v1_retro` (`yjxqnott`):
- `wave_reached: 30.66 vs 30.10`
- `max_wave: 33 vs 34`
- `episode_return: 5539 vs 4662`
- `reached_wave_30: 0.991 vs 1.0`
- `cleared_wave_30: 0.540 vs 0.500`
- `reached_wave_31: 0.540 vs 0.500`
- `dmg_taken_avg: 2503 vs 4286`
- `wrong_prayer_hits: 70.8 vs 210.5`
- `no_prayer_hits: 35.0 vs 89.5`
- `pots_used: 23.8 vs 22.5`
- `food_eaten: 13.6 vs 20.0`
- `food_wasted: 12.23 vs 18.5`
- `pots_wasted: 4.57 vs 7.5`

Interpretation versus `v1_retro`:
- `v19.3` keeps the best part of `v1_retro`:
  - fast, aggressive progress through the cave
- but removes much of the chaos:
  - far lower damage taken
  - far fewer prayer mistakes
  - far less waste
- this is a clear sign that the modern shaping stack can work when seeded from
  the right policy state

Most important new insight:
- the `v19.1` checkpoint did **not** mainly transfer “Ket-Zek skill”
- it transferred a stronger **resource-efficient wave-29/30 policy**
- evidence:
  - dramatic improvement in food/pot use and waste
  - dramatic drop in no-prayer hits
  - large improvement in wave-30 -> wave-31 conversion
  - near-zero magic-prayer uptime in the transferred run
- that means the old bottleneck was more about:
  - resource preservation
  - ready-to-attack tempo
  - range/melee defensive discipline before and around the `29-30` wall
  than about pure Ket-Zek mechanics

Attack readiness signal:
- `attack_when_ready_rate = 0.301`
- this is far better than the late `v19.1` value of `0.138`
- the run still becomes somewhat more passive over time, but it does **not**
  collapse into the same severe passivity seen in the late curriculum run
- this is likely one of the main reasons resource use fell so much

What did not fully transfer:
- magic prayer
- explicit late-wave boss handling
- rare-event tail depth
  - `max_wave` did not exceed the best rare tails from `v18.1` / `v19.1`
- so `v19.3` is best understood as:
  - a very strong full-cave progression run
  - not yet a full late-game/Jad mastery run

Bottom line:
- `v19.3` is the strongest whole-cave run yet on average progress and
  milestone conversion
- the checkpoint transfer idea is validated
- the main benefit of the `v19.1` curriculum checkpoint was not direct
  Ket-Zek mastery; it was improved resource/tempo policy
- the next improvements should focus on preserving these gains while
  reintroducing durable magic-prayer / post-wave-30 competence

Best checkpoint window:
- strongest region appears around `~1.89B-2.26B`
- if replaying or reusing a checkpoint from this run, start in that window,
  not from the final checkpoint

## v19.2 (2026-04-08, staged)

Actual run id:
- none
- `v19.2` was staged only and was not launched under its own label

Mainline direction:
- checkpoint transfer run
- warm-start from `v19.1`
- no sweep
- no curriculum
- same backend / same current pruned obs / same no-Jad-telegraph logic

Why `v19.2` exists:
- `v19.1` proved the agent can learn late-wave mechanics quickly when exposed:
  - real magic-prayer usage
  - real mixed-threat switching
  - much better potion timing
  - repeated deep late-wave progress up to `max_wave = 63`
- that makes `v19.2` the clean transfer test:
  - keep the same `v19` reward/optimizer recipe
  - remove curriculum entirely
  - start from a strong `v19.1` checkpoint
  - see whether the learned late-wave competence transfers back to full-cave
    play from wave 1

Budget:
- `v19.1`: `2.5B`
- `v19.2`: `2.5B`

Checkpoint choice:
- warm-start source run id:
  - `8u6flr5y` (`v19.1`)
- warm-start from:
  - `0000000263192576.bin` (not retained locally)
- rationale:
  - this sits in the first strong `v19.1` peak window (`~237M-367M`)
  - it captures the early late-wave competence before the long-run regression
  - it is earlier and likely cleaner than later checkpoints from the same run

Run / trainer shape:
- continuation run, not scratch
- no `[sweep]` section
- no curriculum buckets
- same PPO / reward recipe as `v19`
- default launch path:
  - `cd <repo-root>/runescape-rl && FORCE_BACKEND_REBUILD=1 LOAD_MODEL_PATH=<repo-root>/pufferlib_4/checkpoints/fight_caves/8u6flr5y/0000000263192576.bin bash train.sh`

What stays the same as `v19`:
- `learning_rate = 0.0003`
- `anneal_lr = 0`
- `clip_coef = 0.2`
- `ent_coef = 0.01`
- `num_buffers = 2`
- same reward weights and shaping
- same `w_attack_attempt = 0.1`
- same `2.5B` late-run budget as `v19.1`

What changes versus `v19.1`:
- `load_model_path: None -> 8u6flr5y/0000000263192576.bin`
- `curriculum_wave: 31 -> 0`
- `curriculum_pct: 1.0 -> 0.0`
- all late-wave curriculum buckets disabled

Results:
- not launched under the `v19.2` label
- this staged recipe was executed unchanged as `v19.3`
- executed run id:
  - `hl0yb7qa`
- warm-start source run id stayed the same:
  - `8u6flr5y` (`v19.1`)
- executed warm-start checkpoint stayed the same:
  - `8u6flr5y/0000000263192576.bin`
- for actual rollout metrics and trajectory, see the `v19.3` section above

Final metrics:
- none recorded under `v19.2` because no run completed under that label
- canonical results for this recipe live under `v19.3`

Why this is the right next test:
- if `v19.2` improves full-cave results from wave 1, then late-wave skill
  transfer is real and the curriculum run taught something reusable
- if `v19.2` still collapses around the old `29-30` shelf, then the problem is
  not just missing late-wave competence; it is likely deeper resource/tempo
  policy quality across the whole cave

What to watch:
- unbiased full-cave metrics again:
  - `wave_reached`
  - `reached_wave_30`
  - `cleared_wave_30`
  - `reached_wave_31`
- whether the `v19.1` improvements transfer:
  - `prayer_uptime_magic`
  - `no_prayer_hits`
  - `avg_prayer_on_pot`
  - `pots_wasted`
  - `attack_when_ready_rate`
- whether the agent arrives late with more resources rather than merely having
  learned late-wave switching in isolation

Live `v19.2` config block:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50

[env]
w_damage_dealt = 0.5
w_attack_attempt = 0.1
w_damage_taken = -0.6
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = 0.0
w_food_used_well = 0.0
w_prayer_pot_used = 0.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 0.5
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.005
shape_food_full_waste_penalty = -6.5
shape_food_waste_scale = -1.2
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = -6.5
shape_pot_waste_scale = -1.2
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.5
shape_npc_melee_penalty = -0.3
shape_wasted_attack_penalty = -0.1
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.5
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -2.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.01
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 2_500_000_000
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

## v19.1 (2026-04-08, completed)

Actual run id:
- `8u6flr5y`

Mainline direction:
- scratch run
- no checkpoint
- no sweep
- forced curriculum
- same backend / same current pruned obs / same no-Jad-telegraph logic

Why `v19.1` exists:
- `v19` validated the `v1_retro` optimizer base and reached the `~29-30`
  shelf very quickly
- but `v19` still failed to convert that shelf into stable wave-31+ progress,
  and `prayer_uptime_magic = 0.0` strongly suggests weak Ket-Zek / late-wave
  magic handling
- `v19.1` is a direct diagnostic run:
  - always start at wave 31
  - keep the exact `v19` optimizer and reward recipe
  - force late-wave Ket-Zek / boss exposure
  - determine whether late-game mechanics are the wall, or whether the real
    wall is arriving there in poor shape from earlier waves

Budget:
- `v19`: `10B`
- `v19.1`: `2.5B`

Run / trainer shape:
- scratch run, not continuation
- no `[sweep]` section
- curriculum always starts at wave 31:
  - `curriculum_wave = 31`
  - `curriculum_pct = 1.0`
  - all other curriculum buckets disabled
- default launch path:
  - `cd <repo-root>/runescape-rl && bash train.sh`

Key differences from `v19`:
- same PPO settings
- same vectorization
- same policy architecture
- same reward shaping
- same `w_attack_attempt = 0.1`
- only two intentional changes:
  - `total_timesteps: 10B -> 2.5B`
  - curriculum forced to wave 31 for 100% of episodes

Why this is worth testing:
- if `v19.1` quickly learns meaningful magic-prayer uptime, Ket-Zek handling,
  and late-wave conversion, then the late-game mechanics themselves are likely
  the missing experience
- if `v19.1` still fails despite constant wave-31 exposure, then the real wall
  is probably broader policy quality:
  - bad early-wave tempo
  - poor resource state arriving late
  - insufficient general combat quality rather than insufficient Ket-Zek reps

Analytic expectation:
- `attack_when_ready_rate` should appear in this run
- the local launch path now rebuilds the backend automatically when
  `fc-training` sources are newer than the compiled extension, so the next
  local `train.sh` launch should pick up the new reward/analytic path instead
  of reusing the stale backend that masked it in `v19`

Live `v19.1` config block:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50

[env]
w_damage_dealt = 0.5
w_attack_attempt = 0.1
w_damage_taken = -0.6
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = 0.0
w_food_used_well = 0.0
w_prayer_pot_used = 0.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 0.5
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.005
shape_food_full_waste_penalty = -6.5
shape_food_waste_scale = -1.2
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = -6.5
shape_pot_waste_scale = -1.2
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.5
shape_npc_melee_penalty = -0.3
shape_wasted_attack_penalty = -0.1
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.5
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -2.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.01
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 31
curriculum_pct = 1.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 2_500_000_000
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

Results (`8u6flr5y`):
- completed normally
- scratch run confirmed:
  - `load_model_path = None`
  - no sweep
  - curriculum fixed at wave 31 for all episodes
- final trainer step: `2,499,805,184 / 2,500,000,000`
- runtime: `1166.3s`
- throughput: `2.11M SPS`

Final metrics:
- `episode_return = 8175.82`
- `episode_length = 6547.15`
- `wave_reached = 44.10`
- `max_wave = 63`
- `most_npcs_slayed = 154`
- `score = 0.0`
- `prayer_uptime = 0.934`
- `prayer_uptime_melee = 0.164`
- `prayer_uptime_range = 0.287`
- `prayer_uptime_magic = 0.483`
- `correct_prayer = 3588.02`
- `wrong_prayer_hits = 186.33`
- `no_prayer_hits = 19.19`
- `prayer_switches = 3795.27`
- `damage_blocked = 294652.41`
- `dmg_taken_avg = 4161.77`
- `attack_when_ready_rate = 0.138`
- `pots_used = 29.22`
- `avg_prayer_on_pot = 0.434`
- `food_eaten = 19.91`
- `avg_hp_on_food = 0.858`
- `food_wasted = 14.83`
- `pots_wasted = 4.63`
- `tokxil_melee_ticks = 7.31`
- `ketzek_melee_ticks = 14.95`
- `reached_wave_30 = 1.0`
- `cleared_wave_30 = 1.0`
- `reached_wave_31 = 1.0`

Critical caveat:
- raw wave milestones are heavily contaminated by curriculum here
- `reached_wave_30`, `cleared_wave_30`, and `reached_wave_31` are trivial
  because every episode starts at wave 31
- `wave_reached = 44.10` is still meaningful as a late-wave survival / progress
  measure, but it is **not** comparable to scratch wave-1 averages from
  `v19`, `v18.1`, or `v1_retro`

Trajectory:
- `2.1M` steps:
  - `wave_reached = 31.2`
  - `episode_return = -31.8`
  - `attack_when_ready_rate = 0.789`
- `123.7M` steps:
  - `wave_reached = 53.6`
  - `episode_return = 13066.5`
- `237.0M` steps:
  - `wave_reached = 57.9`
  - `episode_return = 17175.7`
  - this was the strongest late-wave checkpoint window
- `367.0M` steps:
  - `wave_reached = 57.4`
  - `episode_return = 17213.0`
- `971.0M` steps:
  - `wave_reached = 54.7`
  - `episode_return = 15909.1`
- `1.63B` steps:
  - `wave_reached = 45.0`
  - `episode_return = 8406.8`
- `2.50B` steps:
  - `wave_reached = 44.1`
  - `episode_return = 8175.8`
  - `attack_when_ready_rate = 0.138`

What the run proves:
- direct wave-31 exposure works
- the agent very quickly learned real late-wave mechanics:
  - non-trivial `prayer_uptime_magic = 0.483`
  - very high `correct_prayer`
  - very low `no_prayer_hits`
  - `max_wave = 63`
- this strongly argues that Ket-Zek itself is **not** the main hard wall
- if Ket-Zek were the main blocker, the run would not rapidly climb into the
  mid/upper-50s and repeatedly reach Jad-wave territory

What improved versus `v19`:
- much better late-wave prayer handling:
  - `prayer_uptime_magic: 0.483 vs 0.000`
  - `correct_prayer: 3588 vs 1233`
  - `damage_blocked: 294652 vs 16176`
  - `no_prayer_hits: 19.2 vs 119.6`
  - `prayer_switches: 3795 vs 613`
- much better potion timing:
  - `avg_prayer_on_pot: 0.434 vs 0.762`
  - `pots_wasted: 4.63 vs 20.38`
- somewhat better food timing:
  - `avg_hp_on_food: 0.858 vs 0.929`
  - `food_wasted: 14.83 vs 19.33`

What did not improve enough:
- total resource consumption is still extremely high:
  - `pots_used = 29.22`
  - `food_eaten = 19.91`
- the agent still burns essentially the whole inventory
- `dmg_taken_avg = 4161.77` is worse than `v19` and worse than `v18.1`
- `wrong_prayer_hits = 186.33` is still high in absolute terms
- `score = 0.0` even though `max_wave = 63`

Most important new insight:
- the remaining issue is not “the agent never learned magic prayer”
- the remaining issue looks more like:
  - high-pressure mixed-threat execution is still too sloppy
  - the agent survives long enough to see the late game, but takes too much
    damage and spends too many resources doing it
  - it reaches Jad territory, but not in a strong enough state to convert

Attack readiness signal:
- this was the first run where `attack_when_ready_rate` was successfully logged
- it is extremely informative
- the metric collapses over training:
  - `0.789 -> 0.401 -> 0.230 -> 0.181 -> 0.138`
- interpretation:
  - the policy becomes increasingly passive on ticks where its weapon is ready
  - it over-shifts toward survival / prayer maintenance and gives up offense
  - that likely contributes directly to:
    - longer episodes
    - more incoming attacks
    - more prayer drain
    - more food use
    - more resource depletion before a clear is secured

Comparison to `v19`:
- `v19.1` is **not** directly comparable on raw depth because of curriculum
- but on late-wave side metrics it is decisively more competent:
  - it learned magic prayer
  - it learned switching under dense threat
  - it learned much better potion timing
- that means the absence of late-wave reps in full scratch runs was a real
  weakness
- however, `v19.1` also shows that late-wave reps alone are not sufficient to
  solve the cave from wave 1

Comparison to `v18.1`:
- `v18.1` remained a stronger whole-cave policy in terms of unbiased wave-1
  average
- but `v19.1` teaches a different lesson:
  - the agent can absolutely learn the late-wave mechanics when exposed
  - the late-game wall is not purely a missing-signal problem anymore

Bottom line:
- your current read is partly right, but with one important refinement
- it is **not** mainly “bad potion timing” anymore
- `v19.1` learned potion timing surprisingly well
- the bigger remaining problem is:
  - the agent still takes too much damage in late-wave mixed-threat combat
  - it still spends nearly the full inventory
  - it becomes too passive about attacking when ready
- so the wall looks more like **late-wave tempo + mixed-threat execution +
  resource burn under pressure**, not a simple inability to use magic prayer or
  handle Ket-Zek in isolation

Best checkpoint window:
- if you want replay targets from this run, use the `~237M-367M` region
- the final checkpoint is clearly worse than the early/mid-run peak

## v19 (2026-04-08, completed)

Mainline direction:
- scratch run
- no checkpoint
- no sweep
- no curriculum
- same backend / same current pruned obs / same no-Jad-telegraph logic

Why `v19` exists:
- `v18.1` eventually recovered the wave-29 shelf, but only after a very long
  budget
- `v1_retro` reached comparable depth dramatically faster on the modern stack
- `v19` is the intended hybrid:
  - keep the strong `v1_retro` optimizer pressure and tempo
  - keep a trimmed subset of the modern shaping instead of zeroing nearly
    everything
  - stay fully scratch so we do not inherit the continuation failures seen in
    `v18.2` / `v18.3`

Budget:
- `v1_retro`: `500M`
- `v18.1`: `10B`
- `v19`: `10B`

Run / trainer shape:
- scratch run, not continuation
- no `[sweep]` section
- no curriculum buckets
- default launch path:
  - `cd <repo-root>/runescape-rl && bash train.sh`

Key PPO / vectorization choices:
- copy the `v1_retro` base:
  - `learning_rate = 0.0003`
  - `anneal_lr = 0`
  - `clip_coef = 0.2`
  - `ent_coef = 0.01`
  - `num_buffers = 2`
- keep the stable shared core:
  - `gamma = 0.999`
  - `gae_lambda = 0.95`
  - `vf_coef = 0.5`
  - `max_grad_norm = 0.5`
  - `horizon = 256`
  - `minibatch_size = 4096`
  - `total_agents = 4096`
  - `hidden_size = 256`
  - `num_layers = 2`

Core reward intent:
- keep `v1_retro` offensive pressure:
  - `w_damage_dealt = 0.5`
  - `w_npc_kill = 3.0`
  - `w_tick_penalty = -0.005`
  - new `w_attack_attempt = 0.1`
- soften damage punishment relative to `v18.1`, but not all the way back to
  `v1_retro`:
  - `w_damage_taken = -0.6`
- keep only moderate prayer guidance:
  - `w_correct_danger_prayer = 0.5`
  - `shape_wrong_prayer_penalty = -1.0`
  - `shape_npc_specific_prayer_bonus = 2.5`
- keep modern anti-waste shaping, but stronger:
  - food/pot waste penalties increased beyond `v18.1`
- keep tempo shaping, but reduce overconstraint:
  - `shape_wasted_attack_penalty = -0.1`
  - `shape_wave_stall_base_penalty = -0.5`
  - `shape_wave_stall_cap = -2.0`
  - `shape_not_attacking_penalty = -0.01`

New reward term:
- `w_attack_attempt = 0.1`
- this rewards a real attack cycle being launched, even if the later hit
  misses or rolls zero
- it does **not** replace `w_damage_dealt`
- paired backend fix already landed:
  - “not attacking” now only counts when the weapon cooldown is ready, not
    during time spent waiting between attacks

Config deltas versus `v18.1`:
- `learning_rate: 0.001 -> 0.0003`
- `anneal_lr: 1 -> 0`
- `ent_coef: 0.02 -> 0.01`
- `num_buffers: 1 -> 2`
- `w_damage_dealt: 1.0 -> 0.5`
- `w_attack_attempt: new -> 0.1`
- `w_damage_taken: -0.75 -> -0.6`
- `w_npc_kill: 2.0 -> 3.0`
- `w_tick_penalty: -0.001 -> -0.005`
- `w_food_used: -0.5 -> 0.0`
- `w_food_used_well: 1.0 -> 0.0`
- `w_prayer_pot_used: -1.0 -> 0.0`
- `w_correct_danger_prayer: 1.0 -> 0.5`
- `shape_food_full_waste_penalty: -5.0 -> -6.5`
- `shape_food_waste_scale: -1.0 -> -1.2`
- `shape_pot_full_waste_penalty: -5.0 -> -6.5`
- `shape_pot_waste_scale: -1.0 -> -1.2`
- `shape_npc_specific_prayer_bonus: 2.0 -> 2.5`
- `shape_npc_melee_penalty: -0.5 -> -0.3`
- `shape_wasted_attack_penalty: -0.3 -> -0.1`
- `shape_wave_stall_base_penalty: -0.75 -> -0.5`
- `shape_wave_stall_cap: -3.0 -> -2.0`
- `shape_not_attacking_penalty: -0.5 -> -0.01`

Config deltas versus `v1_retro`:
- same low-pressure PPO settings
- same offensive pressure:
  - `w_damage_dealt = 0.5`
  - `w_npc_kill = 3.0`
  - `w_tick_penalty = -0.005`
- but reintroduce a trimmed set of modern shaping instead of zeroing nearly
  all of it
- add the new attack reward:
  - `w_attack_attempt = 0.1`
- increase waste penalties substantially above `v1_retro`
- keep moderate prayer shaping instead of none

Live `v19` config block:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50

[env]
w_damage_dealt = 0.5
w_attack_attempt = 0.1
w_damage_taken = -0.6
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = 0.0
w_food_used_well = 0.0
w_prayer_pot_used = 0.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 0.5
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.005
shape_food_full_waste_penalty = -6.5
shape_food_waste_scale = -1.2
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = -6.5
shape_pot_waste_scale = -1.2
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.5
shape_npc_melee_penalty = -0.3
shape_wasted_attack_penalty = -0.1
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.5
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -2.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.01
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 10_000_000_000
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

Backend / obs:
- same backend
- same current pruned obs
- same no-Jad-telegraph logic
- same ammo `50,000`
- policy obs remains **106**
- reward features now **19** because of `attack_attempt`
- RL-facing puffer obs remains **142**

Pre-run expectation:
- should retain the fast depth-building behavior seen in `v1_retro`
- should be less sloppy with food/pot usage than `v1_retro`
- should avoid the continuation failures seen in `v18.2` / `v18.3`
- key thing to watch:
  - whether the added shaping keeps the speed while reducing the messiness,
    or whether it overconstrains the policy again

Results (`12gtkmfc`):
- completed normally
- scratch run confirmed:
  - `load_model_path = None`
  - no sweep
  - no curriculum
- final trainer step: `9,999,220,736 / 10,000,000,000`
- runtime: `4962.0s`
- throughput: `1.95M SPS`

Final metrics:
- `episode_return = 5462.44`
- `wave_reached = 29.29`
- `max_wave = 33`
- `reached_wave_30 = 0.594`
- `cleared_wave_30 = 0.0`
- `reached_wave_31 = 0.0`
- `correct_prayer = 1233.13`
- `wrong_prayer_hits = 84.02`
- `no_prayer_hits = 119.59`
- `prayer_switches = 612.80`
- `damage_blocked = 16176.20`
- `dmg_taken_avg = 3015.86`
- `pots_used = 30.42`
- `pots_wasted = 20.38`
- `food_eaten = 20.0`
- `food_wasted = 19.33`
- `avg_prayer_on_pot = 0.762`
- `avg_hp_on_food = 0.929`
- `prayer_uptime = 0.732`
- `prayer_uptime_range = 0.262`
- `prayer_uptime_magic = 0.0`
- `tokxil_melee_ticks = 3.30`
- `ketzek_melee_ticks = 0.0`

Trajectory:
- `1.252B` steps:
  - `wave_reached = 29.18`
  - `episode_return = 5467.62`
  - `reached_wave_30 = 0.783`
- `3.751B` steps:
  - `wave_reached = 30.0009`
  - `episode_return = 6389.86`
  - `reached_wave_30 = 0.977`
  - this was the best window of the run
- `6.251B` steps:
  - `wave_reached = 29.92`
  - `episode_return = 5870.51`
  - `reached_wave_30 = 0.893`
- `8.750B` steps:
  - `wave_reached = 29.58`
  - `episode_return = 5704.41`
  - `reached_wave_30 = 0.600`
- `10.000B` steps:
  - `wave_reached = 29.29`
  - `episode_return = 5462.44`
  - `reached_wave_30 = 0.594`

Comparison to `v1_retro` (`yjxqnott`, 500M):
- `v1_retro` was still stronger on depth and conversion:
  - `wave_reached: 30.10 vs 29.29`
  - `max_wave: 34 vs 33`
  - `reached_wave_30: 1.00 vs 0.594`
  - `cleared_wave_30: 0.50 vs 0.0`
  - `reached_wave_31: 0.50 vs 0.0`
- `v19` was much cleaner defensively:
  - `wrong_prayer_hits: 84.0 vs 210.5`
  - `dmg_taken_avg: 3015.9 vs 4286.5`
  - `tokxil_melee_ticks: 3.30 vs 62.0`
- but `v19` also blocked far less damage and switched far less:
  - `damage_blocked: 16176 vs 29157`
  - `prayer_switches: 613 vs 2286`
- interpretation:
  - `v19` traded away the messy high-ceiling aggression of `v1_retro`
  - it became calmer and safer, but that safety did not translate into
    better late-wave conversion

Comparison to `v18.1` (`xm6i52ta`, 10B):
- final performance was very similar, with `v19` only marginally better on
  average depth:
  - `wave_reached: 29.29 vs 29.24`
  - `episode_return: 5462 vs 5535`
  - `max_wave: 33 vs 34`
- the real improvement was sample efficiency:
  - `v19` was already at `29.18` by `1.25B`
  - `v18.1` was only at `26.28` at the same point
  - `v19` peaked by `3.75B`, while `v18.1` did not peak until `~6.26B`
- defensive profile versus `v18.1` was mixed:
  - better:
    - `wrong_prayer_hits: 84.0 vs 143.4`
    - `dmg_taken_avg: 3015.9 vs 3580.0`
  - worse:
    - `no_prayer_hits: 119.6 vs 77.5`
    - `damage_blocked: 16176 vs 34386`
    - `prayer_uptime: 0.732 vs 0.835`
    - `prayer_uptime_magic: 0.0 vs 0.041`
- interpretation:
  - `v19` reached the old shelf much faster
  - but it underused prayer, especially magic prayer, and never turned that
    faster climb into stronger deep-wave conversion

What worked:
- the `v1_retro` PPO base was validated again:
  - `learning_rate = 0.0003`
  - `anneal_lr = 0`
  - `ent_coef = 0.01`
  - `num_buffers = 2`
- `v19` shows that this base reaches the `~29-30` shelf far faster than
  `v18.1`
- the run was operationally stable for the full `10B` budget

What did not work:
- stronger food/pot waste shaping did **not** produce better resource
  behavior:
  - `pots_wasted` stayed high at `20.38`
  - `avg_prayer_on_pot = 0.762` was worse than both `v1_retro` and `v18.1`
  - `food_wasted` stayed essentially unchanged
- the moderate prayer shaping appears too weak and too static:
  - very low switching relative to prior strong runs
  - almost no magic-prayer uptime
  - much lower blocked damage
- the run peaked around `3.75B` and then steadily regressed

Most important caveat:
- `env/attack_when_ready_rate` is absent from the stored metrics for
  `12gtkmfc`
- that metric should have been present if the rebuilt backend/logging path was
  actually loaded
- so this run likely started from a stale compiled extension before the
  automatic rebuild fix landed
- because the same patchset added `w_attack_attempt`, treat the effect of
  `w_attack_attempt = 0.1` in `v19` as **unverified**

Bottom line:
- `v19` was a success relative to `v18.1` on sample efficiency
- `v19` was not a success relative to `v1_retro` on actual deep-wave
  conversion
- the reintroduced shaping likely overconstrained the policy again, while not
  actually improving the resource metrics it was supposed to improve

Best checkpoint window:
- if you want to replay or inspect the best point from this run, use the
  `~3.75B` region, not the final checkpoint
- that was the strongest combination of:
  - `wave_reached`
  - `episode_return`
  - `reached_wave_30`

## v1_retro (2026-04-08, completed)

Changes from the current `v18.3` reference setup:

This is a **fun retro scratch run**, not the next mainline experiment.

Core intent:
- revisit the original `v1` optimizer / reward recipe that first reached
  the wave-27 shelf
- run it on the **current** backend and **current** pruned obs contract
- keep it isolated from the mainline plan so it does not overwrite the live
  config

What `v1_retro` is:
- scratch run
- no checkpoint
- no sweep
- no curriculum
- `v1`-style PPO settings
- `v1`-style exposed reward weights as closely as the current code allows
- current backend / current obs / current trainer stack

What `v1_retro` is **not**:
- not an exact historical replay of the original `v1`
- not a true reproduction of the 2026-04-03 codebase
- not a replacement for the mainline `v18.3` / `v19` plan

Why it cannot be exact:
- the current obs contract is now the pruned **106-float** contract, not
  the early `v1` obs layout
- the current backend no longer has the old Jad telegraph behavior
- ammo is now effectively unconstrained at **50,000**
- some early `v1` weights are legacy/inactive in the modern reward code:
  - `w_food_used`
  - `w_prayer_pot_used`
  - `w_correct_jad_prayer`
  - `w_wrong_jad_prayer`
  - `w_idle`
- modern shaping terms exist that did not exist in `v1`, so `v1_retro`
  explicitly zeros them to approximate the earlier recipe

Why this run still has value:
- it answers a fun and useful question:
  - how does the original `v1` optimizer / reward style behave on the
    modern pruned backend?
- it gives a clean reference point for how much of the old early success
  came from:
  - the old reward mix
  - the old trainer settings
  - versus the old backend / old obs contract

Config / training changes from the then-current `v18.3` reference:
- change run type:
  - corrected continuation sweep -> isolated scratch train
- remove warm-start:
  - `load_model_path: xm6i52ta/... -> null`
- remove sweep:
  - no `[sweep]` section used
  - run as a normal `train`, not a `sweep`
- change timestep budget:
  - `400M -> 500M`
- restore the documented `v1` PPO settings:
  - `learning_rate: 0.0002 -> 0.0003`
  - `clip_coef: 0.10 -> 0.20`
  - `ent_coef: 0.005 -> 0.01`
  - `num_buffers: 1 -> 2`
- keep the rest of the core trainer shape the same as `v1`:
  - `gamma = 0.999`
  - `gae_lambda = 0.95`
  - `vf_coef = 0.5`
  - `max_grad_norm = 0.5`
  - `horizon = 256`
  - `minibatch_size = 4096`
  - `total_agents = 4096`
  - `hidden_size = 256`
  - `num_layers = 2`
- restore `v1`-style exposed reward weights where they still exist:
  - `w_damage_dealt = 0.5`
  - `w_damage_taken = -0.5`
  - `w_npc_kill = 3.0`
  - `w_wave_clear = 10.0`
  - `w_jad_damage = 2.0`
  - `w_jad_kill = 50.0`
  - `w_player_death = -20.0`
  - `w_cave_complete = 100.0`
  - `w_invalid_action = -0.1`
  - `w_tick_penalty = -0.005`
- explicitly disable most later-added shaping to make the run behave more
  like an early sparse-reward recipe:
  - no food/pot waste shaping
  - no general correct/wrong prayer shaping
  - no NPC-specific prayer bonus
  - no melee-range penalty
  - no wasted-attack penalty
  - no stall penalty
  - no not-attacking penalty
  - no kiting reward
  - no unnecessary-prayer penalty
- keep current backend / obs unchanged:
  - policy obs remains **106**
  - reward features remain **19**
  - RL-facing obs remains **142**
  - current no-telegraph Jad logic remains
  - current invalid-action signal remains live

`v1_retro` config block:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.5
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.05
w_food_used_well = 0.0
w_prayer_pot_used = -0.05
w_correct_jad_prayer = 5.0
w_wrong_jad_prayer = -10.0
w_correct_danger_prayer = 0.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = -0.01
w_tick_penalty = -0.005
shape_food_full_waste_penalty = 0.0
shape_food_waste_scale = 0.0
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = 0.0
shape_pot_waste_scale = 0.0
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = 0.0
shape_npc_specific_prayer_bonus = 0.0
shape_npc_melee_penalty = 0.0
shape_wasted_attack_penalty = 0.0
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = 0.0
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = 0.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = 0.0
shape_kiting_reward = 0.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = 0.0
shape_resource_threat_window = 2
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 500_000_000
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

Hyperparameters:
- fixed:
  - `total_timesteps = 500M`
  - `learning_rate = 0.0003`
  - `anneal_lr = 0`
  - `gamma = 0.999`
  - `gae_lambda = 0.95`
  - `clip_coef = 0.2`
  - `vf_coef = 0.5`
  - `ent_coef = 0.01`
  - `max_grad_norm = 0.5`
  - `horizon = 256`
  - `minibatch_size = 4096`
  - `total_agents = 4096`
  - `num_buffers = 2`
  - `hidden_size = 256`
  - `num_layers = 2`
  - no checkpoint
  - no curriculum
  - no sweep

What `v1_retro` is actually testing:
- whether the original `v1` training recipe still climbs quickly on the
  modern stack
- whether the old wave-27 shelf was mostly:
  - optimizer / reward recipe,
  - or old backend / old obs specific

What `v1_retro` is **not** testing:
- not a pure historical reproduction
- no old obs contract
- no old Jad telegraph backend
- no exact old reward semantics for legacy/inactive weights

Success criteria:
- reproduce the old “fast early climb” flavor on the current stack
- see whether the run still naturally finds the old `~27` shelf
- use the result as a fun sanity reference, not as the main branch for
  future tuning

Operational notes:
- this run was isolated from the then-live `v18.3` sweep setup
- the current active config is now `v19` in `config/fight_caves.ini`

Pre-run expectation:
- likely outcome is not a full return to the exact old `v1` behavior,
  because the backend and obs are materially different now
- the most informative result would be:
  - whether the old low-pressure reward recipe still climbs fast and then
    stalls
  - or whether the modern stack changes that trajectory completely

Results (`yjxqnott`, W&B `snowy-paper-94`):
- completed normally
- state: `finished`
- final trainer step: `499,122,176 / 500,000,000`
- runtime: `253.34s`
- throughput: `2.26M SPS`

Final metrics:
- `episode_return = 4662.17`
- `wave_reached = 30.10`
- `max_wave = 34`
- `correct_prayer = 1144.0`
- `wrong_prayer_hits = 210.5`
- `no_prayer_hits = 89.5`
- `prayer_switches = 2286.5`
- `damage_blocked = 29157.0`
- `dmg_taken_avg = 4286.5`
- `tokxil_melee_ticks = 62.0`
- `reached_wave_30 = 1.0`
- `cleared_wave_30 = 0.5`

Trajectory:
- `65M` steps:
  - `wave_reached = 25.15`
  - `max_wave = 27.83`
  - `reached_wave_30 = 0.434`
  - `cleared_wave_30 = 0.152`
- `189M` steps:
  - `wave_reached = 30.10`
  - `max_wave = 32`
  - `reached_wave_30 = 0.996`
- `313M` steps:
  - `wave_reached = 29.96`
  - `max_wave = 32`
- `438M` steps:
  - `wave_reached = 30.02`
  - `max_wave = 32`
- `499M` steps:
  - `wave_reached = 30.10`
  - `max_wave = 34`
  - `cleared_wave_30 = 0.5`

Comparison to the documented original `v1`:
- original `v1` (doc only, no local JSON retained):
  - `wave_reached = 27.6`
  - `episode_return = 554.3`
  - `max shelf = ~27-28`
- `v1_retro` on the modern stack:
  - `wave_reached = 30.10`
  - `episode_return = 4662.17`
  - `max_wave = 34`

This means:
- the old recipe did **not** merely survive on the modern stack
- it got materially stronger on the modern stack
- so the current backend / obs improvements are not the problem
- the mainline regression is coming from the newer training recipe, not
  from the current environment itself

Comparison to the strong scratch baselines:
- vs `v18` at final (`lxttb7uo`, 1.5B):
  - `wave_reached: 30.10 vs 29.20`
  - `max_wave: 34 vs 31`
  - `damage_blocked: 29157 vs 24912`
  - `reached_wave_30: 1.00 vs 0.35`
  - `episode_return: 4662 vs 4956`
- vs `v18.1` at final (`xm6i52ta`, 10B):
  - `wave_reached: 30.10 vs 29.24`
  - `max_wave: 34 vs 34`
  - `damage_blocked: 29157 vs 34385`
  - `episode_return: 4662 vs 5535`

The striking part is not just the final value. It is the speed:
- `v1_retro` reached `wave_reached ~30` by **~189M** steps
- `v18` did not reach `wave_reached ~27` until **~940M** steps
- `v18` did not reach its `~29` plateau until **~1.31B-1.50B**

Interpretation:
- `v1_retro` is dramatically faster at converting early competence into
  late-wave depth
- but it does so with a much sloppier, higher-risk policy

What is clearly working in `v1_retro`:

1. Lower-pressure PPO settings
- `learning_rate = 0.0003` and `ent_coef = 0.01` appear to be much better
  for fast depth-building on the current stack than the `v18` scratch
  settings of `0.001 / 0.02`
- clipfrac stayed in a healthy learning band:
  - `0.028 -> 0.058`
- unlike the continuation family, this run learned aggressively without
  collapsing or freezing

2. Sparse, objective-heavy reward mix
- the recipe shifts emphasis away from “clean combat” shaping and toward
  actual progress:
  - lower `w_damage_dealt`
  - higher `w_npc_kill`
  - much harsher `w_tick_penalty`
  - most extra shaping disabled
- this appears to force the policy to prioritize:
  - killing things
  - clearing waves
  - moving forward
  over looking tidy on auxiliary metrics

3. Reduced punishment for risky progress
- `w_damage_taken = -0.5` is materially softer than the current `-0.75`
- `v1_retro` clearly accepts more damage in exchange for more progress
- that trade appears to be favorable for reaching late waves quickly

4. Removing prayer-side over-optimization
- `w_correct_danger_prayer = 0.0`
- no wrong-prayer shaping
- no NPC-specific prayer bonus
- the result is not “better prayer discipline”
- the result is a policy that is willing to keep moving and attacking,
  while switching prayers a huge amount
- this looks ugly, but it appears to unlock depth much sooner

5. Stronger time pressure
- `w_tick_penalty = -0.005` vs the modern `-0.001`
- this is a big difference
- even with no explicit stall penalty, the harsher living cost strongly
  rewards making progress quickly

6. `num_buffers = 2`
- this is not proven to be the main driver, but it is one of the few
  structural differences from the later successful scratch runs
- it may be helping with rollout freshness/diversity early in training

What `v1_retro` is missing / where it is clearly worse:

1. It is much sloppier defensively
- `wrong_prayer_hits = 210.5`
- `no_prayer_hits = 89.5`
- `dmg_taken_avg = 4286.5`
- `tokxil_melee_ticks = 62.0`
- compared to `v18`, this is a much messier policy

2. It seems to brute-force depth rather than stabilize it
- prayer switches exploded:
  - `532.9 -> 2286.5`
- blocked damage is high, but so are mistakes and total damage taken
- this looks like hyperactive adaptation, not calm mastery

3. It likely sacrifices long-run cleanliness for short-run progress
- `v1_retro` reaches deep waves much faster
- but the policy looks more chaotic and may be harder to extend cleanly to
  Jad without some later refinement

Main conclusion:
- the modern stack is capable of fast late-wave learning
- the **current mainline reward / PPO recipe is probably over-constrained**
- the biggest thing missing from the current configs is not more state
  information; it is **permission to play more aggressively and optimize the
  real objective sooner**

What `v1_retro` suggests the current configs are doing wrong:
- too much emphasis on clean prayer-side behavior early
- too much punishment for taking damage while progressing
- too many auxiliary shaping terms competing with the real goal
- too little time pressure
- possibly too much entropy / too much learning-rate noise for this
  particular modern stack

Best synthesis for future runs:
- do **not** adopt `v1_retro` wholesale as the new mainline
- it is too sloppy defensively
- but the next mainline scratch run should borrow from it

Most plausible ingredients to steal for `v19`:
- `learning_rate = 0.0003`
- `clip_coef = 0.2`
- `ent_coef = 0.01`
- consider `num_buffers = 2`
- reduce `w_damage_taken` from `-0.75` toward `-0.5` or `-0.6`
- increase objective pressure:
  - `w_npc_kill` toward `3.0`
  - stronger tick pressure than `-0.001`
- drastically simplify or reduce the extra shaping stack instead of adding
  more

Most important takeaway:
- the fast progress in `v1_retro` strongly argues that the current mainline
  recipe is teaching the agent to be **clean before competent**
- `v1_retro` succeeds because it does the opposite:
  - it becomes competent first
  - cleanliness comes later, if at all

---

## v18.3 (2026-04-08, first completed trial `tae450qe`)

Changes from `v18.2`:

This is a **corrected continuation sweep**.

Core intent:
- fix the `v18.2` sweep infrastructure bug so the search space actually
  varies
- remove the small late-wave curriculum so sweep metrics are directly
  comparable to the scratch baselines again
- make the default continuation recipe gentler than `v18.2`
- retry the continuation idea from a stronger `v18.1` checkpoint

Code changes from `v18.2`:
- fix nested `sweep_only` handling in
  `pufferlib_4/pufferlib/sweep.py`
  - `sweep_only` now matches against the full nested key path instead of
    only the leaf name
  - both full paths like `train/learning_rate` and leaf names like
    `learning_rate` now work
- add a fail-fast guard in `Hyperparameters`
  - if the sweep search space resolves to zero parameters, the run now
    errors immediately instead of silently launching repeated defaults
    and failing later in the GP phase

Why these code changes were necessary:
- `v18.2` did not execute a real sweep
- the parser bug filtered out all requested sweep parameters
- that produced **12 identical trials** and then a GP crash on the
  zero-dimensional search space
- `v18.3` has to fix that before any continuation conclusions are
  meaningful

Chosen warm-start checkpoint:
- default `LOAD_MODEL_PATH` is
  `<repo-root>/pufferlib_4/checkpoints/fight_caves/xm6i52ta/0000005977931776.bin`
- this is the `~5.98B` `v18.1` checkpoint
- rationale:
  - it sits in one of the strongest average-wave / return windows from
    `v18.1`
  - it is a better candidate than the early `~2.20B` checkpoint used in
    `v18.2`
- caveat:
  - this is still a training-log choice, not a fixed-eval winner
  - once fixed eval exists, checkpoint selection should move there

Config / training changes from `v18.2`:
- keep run type the same:
  - continuation sweep -> continuation sweep
- change warm-start checkpoint:
  - `xm6i52ta/0000002203058176.bin -> xm6i52ta/0000005977931776.bin`
- remove curriculum entirely:
  - `curriculum_wave: 30 -> 0`
  - `curriculum_pct: 0.05 -> 0.0`
  - `curriculum_wave_2: 31 -> 0`
  - `curriculum_pct_2: 0.02 -> 0.0`
- make the continuation recipe gentler:
  - `learning_rate: 0.0003 -> 0.0002`
  - `clip_coef: 0.15 -> 0.10`
  - `ent_coef: 0.01 -> 0.005`
- reduce per-trial budget:
  - `total_timesteps: 600M -> 400M`
- reduce total sweep budget for the first corrected run:
  - `max_runs: 24 -> 16`
- narrow the sweep space to the continuation knobs only:
  - keep `sweep_only = train/learning_rate, train/clip_coef, train/ent_coef`
  - remove curriculum knobs from the sweep
- keep rewards unchanged from `v18/v18.1/v18.2`:
  - `w_damage_dealt = 1.0`
  - `w_npc_kill = 2.0`
  - `w_correct_danger_prayer = 1.0`
  - no extra threat-aware food/pot penalties
  - capped stall penalty stays on
- keep current backend / obs unchanged:
  - policy obs remains **106**
  - reward features remain **18**
  - RL-facing obs remains **142**
  - Jad telegraph remains removed from both obs and backend
  - ammo remains **50,000**

Why these config changes were necessary:
- `v18.2` showed two separate issues:
  - the sweep never varied
  - the default continuation recipe itself looked like active unlearning
- the next clean test should therefore:
  - eliminate the sweep bug
  - eliminate curriculum contamination
  - move to a softer continuation band
- `v18.3` is meant to answer:
  - whether a **corrected**, **no-curriculum**, **gentler** continuation
    can preserve and improve a strong `v18.1` checkpoint

Live `fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50

[env]
w_damage_dealt = 1.0
w_damage_taken = -0.75
w_npc_kill = 2.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
shape_food_full_waste_penalty = -5.0
shape_food_waste_scale = -1.0
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = -5.0
shape_pot_waste_scale = -1.0
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.0
shape_npc_melee_penalty = -0.5
shape_wasted_attack_penalty = -0.3
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.75
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -3.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.5
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 1

[train]
total_timesteps = 400_000_000
learning_rate = 0.0002
anneal_lr = 0
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.10
vf_coef = 0.5
ent_coef = 0.005
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2

[sweep]
method = Protein
metric = wave_reached
metric_distribution = linear
goal = maximize
max_suggestion_cost = 1200
max_runs = 16
gpus = 1
downsample = 5
use_gpu = True
prune_pareto = True
early_stop_quantile = 0.25
sweep_only = train/learning_rate, train/clip_coef, train/ent_coef

[sweep.train.learning_rate]
distribution = log_normal
min = 0.0001
max = 0.0003
scale = auto

[sweep.train.clip_coef]
distribution = uniform
min = 0.08
max = 0.12
scale = auto

[sweep.train.ent_coef]
distribution = log_normal
min = 0.003
max = 0.008
scale = auto
```

Hyperparameters:
- fixed:
  - gamma=`0.999`
  - gae_lambda=`0.95`
  - vf_coef=`0.5`
  - max_grad_norm=`0.5`
  - horizon=`256`
  - minibatch_size=`4096`
  - total_agents=`4096`
  - hidden_size=`256`
  - num_layers=`2`
  - total_timesteps=`400M`
  - anneal_lr=`0`
  - checkpoint_interval=`50`
  - no curriculum
- sweep space:
  - `learning_rate` in `[0.0001, 0.0003]`
  - `clip_coef` in `[0.08, 0.12]`
  - `ent_coef` in `[0.003, 0.008]`

What `v18.3` is actually testing:
- whether a strong `v18.1` checkpoint can be improved with:
  - smaller continuation steps
  - lower clip
  - lower entropy
  - no curriculum confounds
- whether the late-wave plateau is better attacked by a **gentle
  continuation** rather than a heavier continuation like `v18.2`

What `v18.3` is **not** testing:
- no new observations
- no backend mechanics changes
- no reward rewrite
- no curriculum
- no broad PPO sweep

Success criteria:
- preserve frontier competence from the warm-start checkpoint
- avoid the severe regression seen in `v18.2`
- find at least one continuation setting that is competitive with, or
  better than, the `v18/v18.1` frontier on clean no-curriculum metrics
- keep prayer/combat competence healthy:
  - high `correct_prayer`
  - high `damage_blocked`
  - high `prayer_switches`
  - low `no_prayer_hits`

Operational notes:
- the old `sweep_v18_3.sh` helper was a legacy top-level launcher and is no
  longer kept in the repo root
- this section is historical only; if `v18.3` ever needs to be recreated, use
  `train.sh` / current sweep infrastructure instead of the removed helper

Pre-run expectation:
- if `v18.3` works, the win should show up as:
  - much less regression than `v18.2`
  - preserved prayer/blocking competence
  - at least some runs staying near the `v18/v18.1` frontier instead of
    collapsing to the low-20s
- if `v18.3` still regresses badly, the likely next step is:
  - checkpoint selection by fixed eval before more continuation tuning,
    or
  - a return to scratch-based improvements instead of continuation

First completed trial:
- W&B run `tae450qe` (`wild-puddle-93`)
- state: `finished`
- this trial completed normally to budget; it did **not** crash and did
  **not** early-stop
- final trainer step: `399,507,456 / 400,000,000`
- sampled hyperparameters for this trial:
  - `learning_rate = 0.0002626368403335551`
  - `clip_coef = 0.11023490528350335`
  - `ent_coef = 0.004868108593398205`

Results:
- `wave_reached = 17.92`
- `max_wave = 24`
- `episode_return = 1305.30`
- `correct_prayer = 252.58`
- `wrong_prayer_hits = 34.35`
- `no_prayer_hits = 127.45`
- `prayer_switches = 105.56`
- `damage_blocked = 1712.60`
- `dmg_taken_avg = 2796.51`
- `tokxil_melee_ticks = 7.12`
- `reached_wave_30 = 0.0`
- `cleared_wave_30 = 0.0`
- throughput remained normal:
  - `SPS = 1.99M`
  - `GPU util = 66%` final, with most of the run in the mid-80s
  - `VRAM = ~2.13 GB`
  - `runtime = 200.29s`

Learning dynamics:
- entropy fell from `7.62 -> 6.35`
- clipfrac rose from `0.076 -> 0.179`
- return improved early then faded:
  - `1183.9 -> 1330.6 -> 1429.4 -> 1334.9 -> 1305.3`
- wave reached peaked mid-run then regressed:
  - `18.77 -> 18.64 -> 18.94 -> 18.33 -> 17.92`
- max wave never moved beyond `24`
- prayer-switch frequency collapsed over training:
  - `250.6 -> 161.9 -> 171.9 -> 109.6 -> 105.6`
- prayer correctness improved while switching decreased:
  - `correct_prayer: 179.1 -> 252.6`
  - `wrong_prayer_hits: 76.9 -> 34.3`
  - `damage_blocked: 1533.0 -> 1712.6`
- damage taken did **not** improve meaningfully:
  - `2777.3 -> 2796.5`

Interpretation:
- The **sweep fix worked**. Unlike `v18.2`, this was a real varied trial,
  not one of many duplicated defaults.
- Operationally, `v18.3` is healthy:
  - no sweep parser failure
  - no GP crash
  - no NaNs
  - no throughput anomaly
- Behaviorally, this trial still **regressed hard** relative to the
  `v18/v18.1` frontier and did **not** preserve the strength of the
  `xm6i52ta` warm-start checkpoint.

What improved relative to the `v18.3_control` sanity run `wh0ef384`:
- `episode_return: 1260.75 -> 1305.30`
- `correct_prayer: 212.67 -> 252.58`
- `wrong_prayer_hits: 65.76 -> 34.35`
- `damage_blocked: 1058.92 -> 1712.60`
- `tokxil_melee_ticks: 7.36 -> 7.12`

What did **not** improve:
- `wave_reached: 18.31 -> 17.92`
- `max_wave: 25 -> 24`
- `no_prayer_hits: 98.19 -> 127.45`
- `dmg_taken_avg: 2747.68 -> 2796.51`
- `prayer_switches: 133.26 -> 105.56`

This is the key pattern:
- the sampled `v18.3` point produced a policy that was **cleaner on paper**
  in prayer correctness and blocked damage
- but it did **not** convert that into deeper progression
- instead it looks like a more conservative / less active continuation that
  gives up too much offensive tempo or late-wave adaptability

Comparison to the continuation families:
- vs `v18.2` representative trial `bbqcayi3`
  - `tae450qe` is cleaner and more stable operationally
  - it avoids curriculum contamination
  - it has much better prayer correctness and higher return
  - but that does **not** rescue continuation as a strategy; it is still far
    below the scratch frontier
- vs `v18.3_control` `wh0ef384`
  - `tae450qe` is not a disaster relative to the control
  - it is roughly the same weak continuation regime, just with slightly
    better blocking / prayer hygiene and slightly worse depth

Comparison to the scratch frontier:
- vs `v18` `lxttb7uo`
  - `wave_reached: 17.92 vs 29.20`
  - `episode_return: 1305.30 vs 4955.67`
  - `correct_prayer: 252.58 vs 1720.20`
  - `damage_blocked: 1712.60 vs 24912.17`
- vs `v18.1` `xm6i52ta`
  - `wave_reached: 17.92 vs 29.24`
  - `max_wave: 24 vs 34`
  - `correct_prayer: 252.58 vs 2407.14`
  - `damage_blocked: 1712.60 vs 34385.50`

The gap is too large to explain away as normal continuation noise.
The main conclusion is:
- **continuation is still failing to preserve frontier competence**
- the gentler `v18.3` band is safer than `v18.2`, but it is still not a
  good path to progress

Why this likely happened:
- the checkpoint was chosen from training history, not fixed eval
- the continuation recipe still pushes the policy away from the strong
  frontier behavior instead of consolidating it
- lower entropy / lower clip / lower LR improved defensive neatness but seem
  to have reduced the aggressive switching / tempo needed to actually
  progress deeper
- the current reward mix still appears more effective for **scratch
  learning** than for **checkpoint continuation**

Current `v18.3` verdict:
- sweep infrastructure: **fixed**
- continuation strategy: **still not validated**
- first completed trial: **regression**
- recommendation: **do not promote continuation as the main path forward**

What this means for the next run:
- The evidence now favors returning to a **scratch** run family rather than
  spending more time on checkpoint continuation.
- If continuation is revisited later, it should be after:
  - accelerated replay / checkpoint ranking exists
  - fixed-eval checkpoint selection exists
  - the continuation objective is better aligned with preserving frontier
    competence

Recommended next direction after this result:
- make `v19` a **no-sweep, no-checkpoint scratch run**
- start from the `v18` / `v18.1` recipe, not the continuation recipe
- keep the current backend / obs contract unchanged
- test only small, explicit late-wave changes rather than another broad
  search

Best current `v19` shape:
- scratch run
- no curriculum for the first pass
- no checkpoint warm-start
- keep the strong `v18` PPO base:
  - `learning_rate = 0.001`
  - `clip_coef = 0.2`
  - `ent_coef = 0.02`
- modify only a **small** number of shaping terms aimed at late-wave tempo
  rather than more prayer-side continuation tuning

Most likely useful `v19` experiment:
- preserve the `v18` reward backbone
- make the agent slightly more assertive about actually clearing waves:
  - slightly stronger `shape_wasted_attack_penalty`
  - slightly stronger `shape_not_attacking_penalty`
  - optionally a small increase to `w_npc_kill` or `w_wave_clear`, but not
    both aggressively
- do **not** make another large prayer-shaping jump like `v17`

Reason for this `v19` recommendation:
- the scratch family already learns the task
- the plateau appears more like a late-wave execution / tempo bottleneck
  than a missing prayer-signal bottleneck
- `tae450qe` specifically showed that making the policy “cleaner” on prayer
  alone did **not** buy more depth
- the next controlled change should therefore bias toward turning competent
  defense into faster, cleaner clears

---

## v18.2 (2026-04-07)

Changes from `v18.1`:

This is a **narrow checkpoint fine-tune sweep**, not another scratch run.

Core intent:
- warm-start from a strong `v18.1` checkpoint
- keep the current pruned backend / obs contract unchanged
- keep the `v18/v18.1` reward structure unchanged
- switch from a long scratch budget to a short continuation budget
- sweep only a few continuation knobs around the late-wave frontier

Chosen warm-start checkpoint:
- default `LOAD_MODEL_PATH` is
  `<repo-root>/pufferlib_4/checkpoints/fight_caves/xm6i52ta/0000002203058176.bin`
- this is the `~2.20B` `v18.1` checkpoint, chosen because it sits in the
  early strong plateau window and aligns with one of the best sampled
  `v18.1` return regions
- this is the **default starting point**, not a claim that it is already
  the globally best checkpoint by fixed eval; that should still be
  validated after replay/eval inspection

Config / training changes from `v18.1`:
- change run type:
  - from scratch control -> continuation sweep
- add warm-start:
  - `load_model_path: null -> xm6i52ta/0000002203058176.bin`
- shorten budget:
  - `total_timesteps: 10B -> 600M`
- disable LR annealing for the continuation:
  - `anneal_lr: 1 -> 0`
- change default continuation recipe to the smaller-noise settings:
  - `learning_rate: 0.001 -> 0.0003`
  - `clip_coef: 0.20 -> 0.15`
  - `ent_coef: 0.02 -> 0.01`
- reduce checkpoint spacing:
  - `checkpoint_interval: 100 -> 50`
- introduce a tiny fixed late-wave curriculum:
  - `curriculum_wave: 0 -> 30`
  - `curriculum_pct: 0.0 -> 0.05`
  - `curriculum_wave_2: 0 -> 31`
  - `curriculum_pct_2: 0.0 -> 0.02`
  - `curriculum_wave_3` remains `0`
  - `curriculum_pct_3` remains `0.0`
- keep rewards unchanged from `v18/v18.1`:
  - `w_damage_dealt = 1.0`
  - `w_npc_kill = 2.0`
  - `w_correct_danger_prayer = 1.0`
  - extra threat-aware food/pot penalties stay disabled
  - capped stall penalty stays on
- keep current backend / obs unchanged:
  - policy obs remains **106**
  - reward features remain **18**
  - RL-facing obs remains **142**
  - Jad telegraph remains removed from both obs and backend
  - ammo remains **50,000**

Why this run exists:
- `v18.1` showed that the current stack can occasionally push deeper
  (`max_wave 34`), but the average frontier stayed pinned near the same
  `~29-30` shelf as `v18`
- another long scratch rerun of the same recipe is therefore low-value
- the next useful test is whether a **warm-started, lower-noise
  continuation** can convert those rare deep penetrations into more
  stable late-wave progress
- this is also the right place to introduce sweeps, because the search
  space can now be kept narrow and local to the known-good regime

Live `fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 50

[env]
w_damage_dealt = 1.0
w_damage_taken = -0.75
w_npc_kill = 2.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
shape_food_full_waste_penalty = -5.0
shape_food_waste_scale = -1.0
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = -5.0
shape_pot_waste_scale = -1.0
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.0
shape_npc_melee_penalty = -0.5
shape_wasted_attack_penalty = -0.3
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.75
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -3.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.5
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 30
curriculum_pct = 0.05
curriculum_wave_2 = 31
curriculum_pct_2 = 0.02
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 1

[train]
total_timesteps = 600_000_000
learning_rate = 0.0003
anneal_lr = 0
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.15
vf_coef = 0.5
ent_coef = 0.01
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2

[sweep]
method = Protein
metric = wave_reached
metric_distribution = linear
goal = maximize
max_suggestion_cost = 1200
max_runs = 24
gpus = 1
downsample = 5
use_gpu = True
prune_pareto = True
early_stop_quantile = 0.25
sweep_only = train/learning_rate, train/clip_coef, train/ent_coef, env/curriculum_pct, env/curriculum_pct_2

[sweep.train.learning_rate]
distribution = log_normal
min = 0.00015
max = 0.0005
scale = auto

[sweep.train.clip_coef]
distribution = uniform
min = 0.10
max = 0.20
scale = auto

[sweep.train.ent_coef]
distribution = log_normal
min = 0.005
max = 0.015
scale = auto

[sweep.env.curriculum_pct]
distribution = uniform
min = 0.03
max = 0.08
scale = auto

[sweep.env.curriculum_pct_2]
distribution = uniform
min = 0.0
max = 0.02
scale = auto
```

Hyperparameters:
- fixed:
  - gamma=`0.999`
  - gae_lambda=`0.95`
  - vf_coef=`0.5`
  - max_grad_norm=`0.5`
  - horizon=`256`
  - minibatch_size=`4096`
  - total_agents=`4096`
  - hidden_size=`256`
  - num_layers=`2`
  - total_timesteps=`600M`
  - anneal_lr=`0`
  - checkpoint_interval=`50`
- sweep space:
  - `learning_rate` in `[0.00015, 0.0005]`
  - `clip_coef` in `[0.10, 0.20]`
  - `ent_coef` in `[0.005, 0.015]`
  - `curriculum_pct` in `[0.03, 0.08]`
  - `curriculum_pct_2` in `[0.0, 0.02]`

What the sweep was intended to test:
- whether the current late-wave plateau is best solved by:
  - gentler continuation updates
  - slightly lower exploration
  - a small amount of targeted wave `30/31` rehearsal
- without reintroducing any of the broader `v17` confounds

What the sweep is **not** testing:
- no new observations
- no backend mechanics changes
- no reward rewrite
- no large curriculum
- no broad hyperparameter search across the whole PPO stack

Success criteria:
- materially improve average `wave_reached` beyond the `v18/v18.1`
  `~29.2-29.3` shelf
- improve late-wave milestone conversion:
  - `reached_wave_30`
  - `cleared_wave_30`
  - `reached_wave_31`
- preserve the current strong competence signals:
  - high `correct_prayer`
  - high `damage_blocked`
  - low `tokxil_melee_ticks`
- avoid regressing into the `v17.1` flatline regime

Operational notes:
- the old `sweep_v18_2.sh` helper was a legacy top-level launcher and is no
  longer kept in the repo root
- this section is historical only; if `v18.2` ever needs to be recreated, use
  `train.sh` / current sweep infrastructure instead of the removed helper

Results (attempted sweep, 12 completed trials before failure,
wandb group `v18_2_sweep`):
- completed trial ids:
  - `bbqcayi3`
  - `dtx7o43t`
  - `q2tnsyj9`
  - `7986v0xu`
  - `j98edhkh`
  - `dulc6rym`
  - `mnamq7yv`
  - `4umps8gz`
  - `5vw45smy`
  - `jxnofnjl`
  - `gy73cihv`
  - `6ej6mifr`
- total completed work before failure:
  - **12** successful trials
  - about **7.20B** total agent steps
  - about **56.3 min** of successful trial uptime
- all 12 completed trials used the **same default continuation
  config**, not different sweep suggestions:
  - `learning_rate = 0.0003`
  - `clip_coef = 0.15`
  - `ent_coef = 0.01`
  - `curriculum_pct = 0.05`
  - `curriculum_pct_2 = 0.02`

Representative final trial summary
(`bbqcayi3`; the others are effectively identical):
- SPS: **2.11M**
- uptime: **282.0s**
- wave reached: **20.96**
- episode return: **904.9**
- max wave: **32**
- most NPCs slayed: **82**
- reached_wave_30: **0.533**
- cleared_wave_30: **0.0**
- reached_wave_31: **0.0**
- correct_prayer: **129.9**
- no_prayer_hits: **87.7**
- prayer_switches: **96.9**
- damage_blocked: **1414.6**
- dmg_taken_avg: **3024.5**
- entropy: **6.792**
- clipfrac: **0.127**

Important operational outcome:
- `v18.2` did **not** execute a valid hyperparameter sweep.
- It launched **12 identical continuation trials** and then failed before
  trial 13.
- This means there is **no legitimate sweep winner** from `v18.2`.
- The run is still informative, but it should be read as:
  - a repeated default continuation control, and
  - a sweep infrastructure failure, not a completed sweep experiment

Why the sweep failed:
- The `sweep_only` filter used path-style keys:
  - `train/learning_rate`
  - `train/clip_coef`
  - `train/ent_coef`
  - `env/curriculum_pct`
  - `env/curriculum_pct_2`
- But the recursive sweep parser filters on the **leaf field name**
  after descending into nested dicts.
- That means the parser compared:
  - `learning_rate`
  - `clip_coef`
  - `ent_coef`
  - `curriculum_pct`
  - `curriculum_pct_2`
  against the full path strings above, and filtered them all out.
- Net result:
  - the sweep search space collapsed to **zero dimensions**
  - `Hyperparameters.num = 0`
  - every launched run used the same default config
- The launch count of **12** is also explained by the broken sweep path:
  - `pufferlib.pufferl.sweep()` runs the first **2** experiments without
    calling `suggest()`
  - `Protein` then emits **10** zero-dimensional random/default
    suggestions
  - after those **12** completed trials, the optimizer tries to switch
    into the GP model path and crashes on the empty hyperparameter space
- The failure is reproducible locally from the stored config and lands in
  the GP training path with a shape mismatch on zero-dimensional inputs.

What went well:
- Warm-start loading worked:
  - every completed trial loaded the intended `xm6i52ta` checkpoint
- Local sweep orchestration worked up to the failure point:
  - per-trial W&B runs were created
  - per-trial JSON logs were saved
  - completed trials were isolated cleanly by run id
- Runtime stability was good:
  - no NaNs
  - no early-stop failures
  - stable throughput around **2.11M SPS**
  - stable GPU/VRAM usage:
    - GPU util averaged about **92%**
    - VRAM averaged about **1.99 GB**
- The repeated identical runs also acted as an accidental determinism
  check:
  - outcomes were nearly identical across all 12 trials
  - that is evidence the checkpoint loading + current backend path are
    highly repeatable under fixed settings

What went wrong:
- The most important problem is the sweep bug above: `v18.2` did not
  search the intended hyperparameter space at all.
- The second important problem is that the **default continuation recipe
  itself underperformed badly**, even before the sweep crashed.
- Compared to the source run family (`v18` / `v18.1`), the default
  `v18.2` continuation is a major regression.

Representative trial progression (`bbqcayi3`):
- `~76.5M` steps:
  - wave `22.44`
  - return `326.2`
  - correct_prayer `175.8`
  - prayer_switches `196.1`
  - damage_blocked `2438.9`
  - entropy `7.654`
  - clipfrac `0.070`
- `~226.5M` steps:
  - wave `20.65`
  - return `771.6`
  - correct_prayer `154.7`
  - prayer_switches `99.5`
  - damage_blocked `893.6`
- `~376.4M` steps:
  - wave `20.12`
  - return `903.9`
  - correct_prayer `178.9`
  - no_prayer_hits `102.0`
  - damage_blocked `1205.2`
- `~526.4M` steps:
  - wave `20.22`
  - return `901.5`
  - correct_prayer `172.1`
  - prayer_switches `88.5`
  - damage_blocked `1095.7`
- final `~599.8M` steps:
  - wave `20.96`
  - return `904.9`
  - correct_prayer `129.9`
  - prayer_switches `96.9`
  - damage_blocked `1414.6`
  - entropy `6.792`
  - clipfrac `0.127`

Interpretation of that progression:
- This is **not** a stagnant run that simply failed to update.
- The policy kept moving:
  - entropy fell from `7.65 -> 6.79`
  - clipfrac rose from `0.070 -> 0.127`
- But the movement was not beneficial:
  - average wave fell below the opening continuation window
  - prayer switching volume collapsed
  - blocked damage collapsed
- So the default `v18.2` continuation looks like **active unlearning**,
  not optimizer starvation.

Important metric caveat:
- `v18.2` introduced a small wave `30/31` curriculum:
  - `curriculum_pct = 0.05`
  - `curriculum_pct_2 = 0.02`
- Because of that, several late-wave metrics are **partially confounded**
  and should not be read as directly comparable to scratch runs:
  - `reached_wave_30`
  - `max_wave`
  - `most_npcs_slayed`
- Inference:
  - the raw `wave_reached = 20.96` is also mildly inflated by those
    curriculum starts
  - if you subtract only the minimum wave-floor injected by the
    curriculum (`0.05 * 30 + 0.02 * 31 = 2.12`), the implied
    non-curriculum average would be closer to about **18.84**
  - that is only an approximation, but it shows the continuation was
    almost certainly even weaker than the raw headline number suggests

Comparison vs `v18.1` (`xm6i52ta`):

Top-line:
- `v18.2` default continuation:
  - wave `20.96`
  - return `904.9`
  - max wave `32`
  - correct_prayer `129.9`
  - prayer_switches `96.9`
  - damage_blocked `1414.6`
- `v18.1` final run:
  - wave `29.24`
  - return `5534.5`
  - max wave `34`
  - correct_prayer `2407.1`
  - prayer_switches `2107.0`
  - damage_blocked `34.4k`

What this says:
- The default `v18.2` continuation is dramatically weaker than the
  source recipe:
  - wave down by **8.28**
  - return down by **4629.7**
  - correct_prayer down by about **2277**
  - prayer_switches down by about **2010**
  - damage_blocked down by about **32.97k**
- Because `v18.2` uses curriculum and `v18.1` does not, its raw
  `reached_wave_30` is **not** a valid headline comparison
- The reliable reading is simple:
  - `v18.2` did not consolidate late-wave performance
  - it regressed far below the frontier recovered by `v18.1`

Comparison vs `v18` (`lxttb7uo`):
- `v18` final wave: **29.20**
- `v18.2` default continuation wave: **20.96**
- `v18` correct_prayer: **1720.2**
- `v18.2` correct_prayer: **129.9**
- `v18` damage_blocked: **24.9k**
- `v18.2` damage_blocked: **1.4k**

Interpretation:
- `v18.2` did not preserve the competence that `v18` re-established on
  the pruned backend / obs contract
- So this is not a case where the current checkpoint fine-tune just
  held steady while the sweep infrastructure misbehaved
- The continuation recipe itself is poor in its current form

Comparison vs `v17.1` (`q3ald8bc`):
- `v17.1` wave: **17.17**
- `v18.2` default continuation wave: **20.96**

Interpretation:
- Even the broken `v18.2` continuation remains above the failed
  `v17.1` scratch control
- That is consistent with a checkpoint carrying some competence forward
- But it is still nowhere close to the frontier already proven by
  `v18` / `v18.1`

Main conclusion:
- `v18.2` is a **negative result** in two different ways:
  - the first sweep implementation was not valid
  - the default continuation recipe performed poorly even before the
    sweep infrastructure failed
- There is no basis to choose a hyperparameter winner from this run.
- The only safe conclusions are:
  - the sweep path needs to be fixed before the next sweep
  - the exact default `v18.2` continuation recipe should **not** be
    promoted as the next baseline

Most likely root causes of poor agent performance in this attempt:
- sweep infrastructure bug:
  - the intended hyperparameter search never happened
- checkpoint continuation recipe is too disruptive:
  - the continuation actively moved the policy, but in a harmful
    direction
- late-wave curriculum polluted the evaluation signals:
  - milestone metrics from this run are not cleanly comparable to the
    scratch baselines
- checkpoint selection was not fixed-eval-backed:
  - the chosen `~2.20B` checkpoint was a plausible candidate, but not a
    validated best checkpoint

Recommendation for next run:
- Do **not** treat `v18.2` as a successful sweep.
- Before the next sweep:
  - fix the `sweep_only` / nested-parameter path handling
  - verify that the sweep object actually sees a nonzero search space
  - dry-run one local suggestion and print the sampled hyperparameters
    before launching full experiments
- For the next continuation attempt:
  - choose the warm-start checkpoint by fixed eval, not by intuition
  - remove curriculum for the first corrected sweep so the metrics are
    cleanly comparable
  - test the small continuation recipe again only after the sweep path
    is known-good
- If a single non-sweep continuation control is desired before fixing
  sweeps:
  - run one warm-start continuation with:
    - `anneal_lr = 0`
    - `learning_rate = 0.00015` or `0.0002`
    - `clip_coef = 0.10`
    - `ent_coef = 0.005`
    - **no curriculum**
  - that will tell us whether the poor result was mostly:
    - bad checkpoint + curriculum, or
    - bad continuation dynamics more broadly

---

## v18.1 (2026-04-07)

Changes from `v18`:

This run was intended as a long-budget control on the `v18` recipe.

Nominal config delta:
- `total_timesteps: 1.5B -> 10B`

Important caveat:
- because `anneal_lr = 1`, this was **not** a perfect "more time only"
  control
- extending `total_timesteps` from `1.5B` to `10B` also stretched the
  LR decay by `6.67x`
- at `1.5B` steps, `v18` had annealed essentially to `0`
- at `1.5B` steps in `v18.1`, the effective LR was still about
  `0.00085`
- so `v18.1` tested **more budget plus a much slower decay schedule**,
  not just more wall-clock compute

Everything else stayed identical to `v18`:
- same current pruned obs/backend:
  - policy obs remains **106**
  - reward features remain **18**
  - RL-facing obs remains **142**
  - Jad telegraph stays removed from both obs and backend
  - ammo remains **50,000**
- same PPO recipe:
  - `learning_rate = 0.001`
  - `clip_coef = 0.2`
  - `ent_coef = 0.02`
- same reward/shaping recipe:
  - `w_damage_dealt = 1.0`
  - `w_npc_kill = 2.0`
  - `w_correct_danger_prayer = 1.0`
  - extra threat-aware food/pot penalties remain disabled
  - capped stall penalty remains on
- same no-warm-start / no-curriculum setup

Why this run existed:
- `v18` already proved that the current pruned backend / obs contract is
  healthy and can recover to the wave-29 frontier.
- The main unanswered question is whether the same exact recipe keeps
  climbing with a much larger budget, or whether it simply plateaus
  around the current `~29-31` region.
- `v18.1` is meant to answer that without introducing any new confounds.

Live `fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 100

[env]
w_damage_dealt = 1.0
w_damage_taken = -0.75
w_npc_kill = 2.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
shape_food_full_waste_penalty = -5.0
shape_food_waste_scale = -1.0
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = -5.0
shape_pot_waste_scale = -1.0
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.0
shape_npc_melee_penalty = -0.5
shape_wasted_attack_penalty = -0.3
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.75
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -3.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.5
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 1

[train]
total_timesteps = 10_000_000_000
learning_rate = 0.001
anneal_lr = 1
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=10B, lr=0.001, anneal_lr=1.

Success criteria were:
- determine whether the exact `v18` recipe can break past the current
  wave-29 frontier with more time alone
- preserve the recovered scratch-learning behavior from `v18`
- identify whether the best checkpoint window moves materially beyond
  the `~1.21B-1.29B` region from `v18`
- avoid introducing any new confounds before the shared-backend refactor

If this run still plateaus in the same region:
- that is strong evidence the next step should be targeted fine-tuning
  or a small late-wave curriculum intervention, not another large-budget
  scratch rerun of the same recipe

Results (10B steps, ~87.5min, wandb run `xm6i52ta`):
- SPS: **1.84M**
- Wave reached: **29.24** avg
- Max wave: **34**
- Most NPCs slayed: **128**
- Episode return: **5534.5**
- Episode length: **4402**
- Score: 0
- Epochs: 9536
- Entropy: **5.495**
- Clipfrac: **0.0935**
- Value loss: **40.82**
- Policy loss: `-0.0551`
- KL: `0.00728`

Analytics (final summary window):
- prayer_uptime: **0.835**
- prayer_uptime_melee: **0.564**
- prayer_uptime_range: **0.230**
- prayer_uptime_magic: **0.041**
- correct_prayer: **2407.1**
- wrong_prayer_hits: **143.4**
- no_prayer_hits: **77.5**
- prayer_switches: **2107.0**
- damage_blocked: **34,385.5**
- dmg_taken_avg: **3580.0**
- pots_used: **31.5**
- avg_prayer_on_pot: **0.690**
- food_eaten: **20.0**
- avg_hp_on_food: **0.926**
- food_wasted: **19.4**
- pots_wasted: **13.3**
- tokxil_melee_ticks: **12.8**
- ketzek_melee_ticks: **0.0**
- reached_wave_30: **0.306**
- cleared_wave_30: **0.0**
- reached_wave_31: **0.0**

Progression:
- `v18.1` recovered to the frontier earlier than `v18`:
  - `~512M`: **wave 20.0**
  - `~1.13B`: **wave 29.2**
- After that, the run spent almost the entire remaining budget on the
  same broad plateau:
  - `1.66B`: **wave 29.4**, return **5743.5**
  - `2.17B`: **wave 29.4**, return **5935.9**
  - `4.64B`: **wave 29.5**, return **5340.2**
  - `5.99B`: **wave 29.6**, return **5555.0**
  - `9.60B`: **wave 29.3**, return **5667.1**
  - `9.99B`: **wave 29.3**, return **5594.6**
- The strongest result of the extra budget was not a new average-wave
  breakout. It was a stronger rare-event tail:
  - `max_wave` increased to **34**
  - `most_npcs_slayed` increased to **128**
- Important nuance:
  - the final summary window shows `cleared_wave_30 = 0.0` and
    `reached_wave_31 = 0.0`
  - that does **not** mean the run never broke deeper
  - it means those deep conversions remained rare and did not
    consolidate into the final logging window

Primary interpretation:
- `v18.1` answers the main control question: **more time alone did not
  materially move the average frontier beyond the same `~29-30` shelf**
- The extra budget **did** improve tail performance, total return, and
  episode length
- But it did **not** turn those rare deeper runs into stable average
  progress
- This is the behavior of a policy that can occasionally penetrate the
  frontier, but still lacks a reliable late-wave conversion strategy

Comparison vs `v18` (`lxttb7uo`, same recipe, 1.5B):

Top-line:
- `xm6i52ta`: wave `29.24`, max wave `34`, return `5534.5`
- `lxttb7uo`: wave `29.20`, max wave `31`, return `4955.7`

What improved:
- Better rare-event tail:
  - `max_wave 34` vs `31`
  - `most_npcs_slayed 128` vs `119`
- Higher return and longer-lived episodes:
  - `episode_return 5534.5` vs `4955.7`
  - `episode_length 4402` vs `3451`
- Stronger prayer/combat volume:
  - `correct_prayer 2407.1` vs `1720.2`
  - `prayer_switches 2107.0` vs `1386.6`
  - `damage_blocked 34.4k` vs `24.9k`
- Better movement/threat handling on at least one important proxy:
  - `tokxil_melee_ticks 12.8` vs `23.2`
- Better potion timing:
  - `avg_prayer_on_pot 0.690` vs `0.724`
  - `pots_wasted 13.3` vs `17.4`

What did not improve:
- Average frontier barely changed:
  - `wave 29.24` vs `29.20`
- Final-window late-wave conversion did not improve:
  - `reached_wave_30 0.306` vs `0.348`
  - `cleared_wave_30 0.0` vs `0.0`
- Defensive cleanliness got worse:
  - `no_prayer_hits 77.5` vs `25.8`
  - `dmg_taken_avg 3580.0` vs `3071.9`
  - `prayer_uptime 0.835` vs `0.919`

Interpretation:
- `v18.1` became **deeper and more productive**, but not **cleaner**
- The policy appears to have traded some defensive coverage for longer,
  more aggressive episodes
- That trade improved return and tail depth, but not stable frontier
  advancement

Most important optimizer observation:
- `v18` had essentially annealed to zero by the end:
  - final `clipfrac 0.0053`
  - final `KL 0.00124`
- `v18.1` was still updating materially even at the end:
  - final `clipfrac 0.0935`
  - final `KL 0.00728`
- So this was **not** a case where the optimizer simply froze too early
- The plateau held even while the policy was still moving, which points
  more toward a late-wave exposure / credit-assignment / conversion
  problem than a basic optimization-starvation problem

Comparison vs `v16.2a` (`ge5sma5y`, old strong scratch baseline):

Top-line:
- `xm6i52ta`: wave `29.24`, max wave `34`, return `5534.5`
- `ge5sma5y`: wave `28.82`, max wave `31`, return `4593.7`

What improved:
- Better average wave, return, and episode length
- Better rare tail:
  - `max_wave 34` vs `31`
  - `most_npcs_slayed 128` vs `119`
- Stronger block/switch volume:
  - `correct_prayer 2407.1` vs `1604.2`
  - `prayer_switches 2107.0` vs `1343.2`
  - `damage_blocked 34.4k` vs `23.2k`
- Better Tok-Xil spacing:
  - `tokxil_melee_ticks 12.8` vs `28.4`

What is still not clearly better:
- `no_prayer_hits` is much worse (`77.5` vs `28.3`)
- `dmg_taken_avg` is also worse (`3580.0` vs `3334.6`)
- So `v18.1` is not simply a cleaner version of `ge5sma5y`
- It is a stronger high-ceiling run with more occasional late-wave
  penetration, but still messy in defensive coverage

Comparison vs `v17.1` (`q3ald8bc`, failed scratch control):

Top-line:
- `xm6i52ta`: wave `29.24`, max wave `34`, return `5534.5`
- `q3ald8bc`: wave `17.17`, max wave `24`, return `1315.7`

What this confirms:
- The current pruned backend / obs contract is not the issue
- The major failure in `v17.1` was the weakened PPO + reward recipe
- `v18.1` massively outperforms `v17.1` on every meaningful frontier and
  prayer metric

Comparison vs `v17` warm-start (`mv0snohb`):

Top-line:
- `xm6i52ta`: wave `29.24`, max wave `34`, return `5534.5`
- `mv0snohb`: wave `25.01`, max wave `32`, return `291.8`

What this confirms:
- the strong current scratch recipe is better than the old warm-start +
  curriculum package
- carrying old competence forward was not enough; the better recipe
  matters more than the old transfer scaffolding

Main conclusion:
- `v18.1` is a success in one narrow sense:
  it proves the current stack can keep producing rare deeper runs over a
  very long budget without collapsing
- But it is also a clear negative result:
  **another long scratch rerun of the same recipe is unlikely to solve
  the plateau**
- The average frontier stayed effectively flat after `~1.1B-1.7B`
  despite another `8B+` steps of training

Recommendation for next run:
- Do **not** run another 10B scratch repetition of this exact recipe
- The best next step is a targeted late-wave fine-tune from a strong
  `v18.1` checkpoint, not another full scratch restart

Recommended `v19` direction:
- load from a strong `v18.1` checkpoint chosen by fixed eval, not by
  final-step training average alone
- first checkpoints worth evaluating:
  - `~1.66B`
  - `~2.17B`
  - `~4.90B-6.00B`
  - `~9.18B-9.60B`
- use a gentler fine-tune recipe:
  - `learning_rate = 0.0003`
  - `clip_coef = 0.15`
  - `ent_coef = 0.01`
  - `checkpoint_interval = 50`
- add only a **small** late-wave curriculum:
  - `curriculum_wave = 30`, `curriculum_pct = 0.05`
  - `curriculum_wave_2 = 31`, `curriculum_pct_2 = 0.02`
  - keep `curriculum_wave_3 = 0`
- keep the current `v18/v18.1` reward structure unchanged for the first
  fine-tune

Why this is the right next move:
- `v18.1` shows the agent can occasionally break deeper
- the missing ingredient is not basic competence anymore
- the missing ingredient is **consistent conversion** once it reaches
  the late-wave frontier
- that is exactly the situation where a small late-wave curriculum and a
  lower-noise fine-tune are justified

Secondary recommendation:
- if you want another "time only" control in the future, do not change
  `total_timesteps` with `anneal_lr = 1` and assume the recipe stayed
  identical
- future continuation-style comparisons should either:
  - continue training from the best checkpoint
  - or explicitly control the LR schedule when changing total budget

---

## v18 (2026-04-06)

Changes from `v17.1`:

This is the scratch recovery run that kept the current `fight-caves-rl` pruned
backend/obs contract, but restored the stronger scratch-learning recipe.

Config / training changes from `v17.1`:
- keep **no warm-start**
- keep **no curriculum**
- keep the current pruned obs/backend:
  - policy obs stays at **106**
  - Jad telegraph remains removed from both obs and backend logic
  - ammo remains **50,000**
- restore the learning recipe toward the working `ge5sma5y` regime:
  - `learning_rate: 0.00025 -> 0.001`
  - `clip_coef: 0.12 -> 0.2`
  - `ent_coef: 0.03 -> 0.02`
  - `w_damage_dealt: 0.5 -> 1.0`
  - `w_npc_kill: 3.0 -> 2.0`
  - `w_correct_danger_prayer: 2.0 -> 1.0`
- disable the extra threat-aware food/pot penalties:
  - `shape_food_safe_hp_threshold: 0.70 -> 1.0`
  - `shape_food_no_threat_penalty: -0.50 -> 0.0`
  - `shape_pot_safe_prayer_threshold: 0.50 -> 1.0`
  - `shape_pot_no_threat_penalty: -0.50 -> 0.0`
- keep the current stability fixes:
  - prayer flick reward still disabled
  - capped stall penalty still on
  - incoming-hit timeline summaries and `prayer_drain_counter` still on

Why this run existed:
- `v17.1` answered the control question clearly: the current `v17`
  package was materially worse than `v16.2a` from scratch.
- The unresolved question was whether the failure came from the new
  backend / observation contract, or from the weakened PPO + reward
  recipe around it.
- `v18` isolates that question:
  keep the new backend / pruned obs, but restore the proven scratch
  learning recipe.

Live `fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 100

[env]
w_damage_dealt = 1.0
w_damage_taken = -0.75
w_npc_kill = 2.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
shape_food_full_waste_penalty = -5.0
shape_food_waste_scale = -1.0
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_full_waste_penalty = -5.0
shape_pot_waste_scale = -1.0
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.0
shape_npc_melee_penalty = -0.5
shape_wasted_attack_penalty = -0.3
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.75
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -3.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.5
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 1

[train]
total_timesteps = 1_500_000_000
learning_rate = 0.001
anneal_lr = 1
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.5B, lr=0.001, anneal_lr=1.

Success criteria:
- recover a real scratch breakout instead of the `v17.1` flatline
- beat `wave 20` materially earlier than `v17.1`
- rebuild prayer-switching and damage-blocking metrics toward `ge5sma5y`
- preserve the improved stability from the current backend/stall-cap changes

Results (1.5B steps, ~12.4min, wandb run `lxttb7uo`):
- SPS: **2.02M**
- Wave reached: **29.20** avg
- Max wave: **31**
- Most NPCs slayed: **119**
- Episode return: **4955.7**
- Episode length: **3451**
- Score: 0
- Epochs: 1430
- Entropy: **5.938**
- Clipfrac: **0.0053**
- Value loss: **48.49**
- Policy loss: `-0.0300`
- KL: `0.00124`

Analytics (final):
- prayer_uptime: **0.919**
- prayer_uptime_melee: **0.544**
- prayer_uptime_range: **0.312**
- prayer_uptime_magic: **0.063**
- correct_prayer: **1720.2**
- wrong_prayer_hits: **152.7**
- no_prayer_hits: **25.8**
- prayer_switches: **1386.6**
- damage_blocked: **24,912.2**
- dmg_taken_avg: **3071.9**
- pots_used: **28.7**
- avg_prayer_on_pot: **0.724**
- food_eaten: **20.0**
- avg_hp_on_food: **0.922**
- food_wasted: **18.9**
- pots_wasted: **17.4**
- tokxil_melee_ticks: **23.2**
- ketzek_melee_ticks: **0.0**
- reached_wave_30: **0.348**
- cleared_wave_30: **0.0**
- reached_wave_31: **0.0**

Progression:
- `v18` did **not** look healthy immediately. For the first `~550M`
  steps it still looked like a middling run, mostly living in the
  `18-19` range.
- The decisive breakout started around `~620M-700M`:
  - `616M`: **wave 20.2**
  - `696M`: **wave 22.6**
  - `780M`: **wave 24.0**
  - `853M`: **wave 26.9**
  - `935M`: **wave 28.2**
- The run then established a real frontier plateau:
  - `1.084B`: **wave 28.8**
  - `1.156B`: **wave 29.2**
  - `1.214B`: **wave 29.4**
  - `1.290B`: **wave 29.3**
- Best observed checkpoint window was around `~1.21B-1.29B`, not the
  final step:
  - peak average return: **5079.5** at `1.289B`
  - final average return: **4955.7**
- This is the same broad learning shape as `ge5sma5y`: a long slow
  setup phase, then a hard mid-run breakout into the high-20s.
- Important implication: stopping this recipe around `400M-600M` would
  have produced a false negative.

Comparison vs `v17.1` (`q3ald8bc`, scratch, no curriculum, 1.5B):

Top-line:
- `lxttb7uo`: wave `29.20`, max wave `31`, return `4955.7`
- `q3ald8bc`: wave `17.17`, max wave `24`, return `1315.7`

What improved dramatically:
- Breakout came back:
  - `v17.1` never reached wave 20
  - `v18` reached wave 20 by `~616M`, wave 24 by `~780M`, and wave 29 by `~1.15B`
- Prayer learning recovered:
  - `correct_prayer 1720.2` vs `224.9`
  - `prayer_switches 1386.6` vs `339.4`
  - `damage_blocked 24.9k` vs `1.2k`
  - `no_prayer_hits 25.8` vs `96.5`
- Resource timing improved instead of collapsing:
  - `pots_used 28.7` vs `32.0`
  - `avg_prayer_on_pot 0.724` vs `0.987`
  - `pots_wasted 17.4` vs `32.0`
  - `avg_hp_on_food 0.922` vs `0.926`
  - `food_wasted 18.9` vs `19.5`
- Episode depth recovered:
  - `episode_length 3451` vs `997`

What worsened on paper but is not actually a regression:
- `wrong_prayer_hits` increased (`152.7` vs `62.4`)
- `dmg_taken_avg` increased (`3071.9` vs `2683.5`)
- `tokxil_melee_ticks` increased (`23.2` vs `12.1`)

Interpretation:
- Those raw totals rose because `v18` survives far longer, reaches much
  later waves, and faces many more dangerous encounters per episode.
- The more meaningful signal is that `no_prayer_hits` collapsed while
  `correct_prayer`, `prayer_switches`, and `damage_blocked` exploded
  upward. That is real competence, not just longer suffering.

Comparison vs `v16.2a` (`ge5sma5y`, scratch, no curriculum, 1.5B):

Top-line:
- `lxttb7uo`: wave `29.20`, max wave `31`, return `4955.7`
- `ge5sma5y`: wave `28.82`, max wave `31`, return `4593.7`

What improved:
- Slightly higher frontier:
  - wave `29.20` vs `28.82`
  - return `4955.7` vs `4593.7`
  - episode_length `3451` vs `3281`
- Slightly better combat conversion:
  - `correct_prayer 1720.2` vs `1604.2`
  - `prayer_switches 1386.6` vs `1343.2`
  - `damage_blocked 24.9k` vs `23.2k`
  - `no_prayer_hits 25.8` vs `28.3`
- Better resource discipline:
  - `pots_used 28.7` vs `30.5`
  - `avg_prayer_on_pot 0.724` vs `0.767`
  - `pots_wasted 17.4` vs `19.8`
  - `food_wasted 18.9` vs `19.0`
- Better overall safety totals:
  - `dmg_taken_avg 3071.9` vs `3334.6`
  - `tokxil_melee_ticks 23.2` vs `28.4`
- Slightly better SPS: `2.02M` vs `1.95M`

What changed but is not clearly better or worse:
- `prayer_uptime` rose slightly (`0.919` vs `0.903`)
- melee prayer time fell slightly while range prayer time rose
- value loss was lower in `ge5sma5y`, but that did not translate into a
  stronger frontier than `v18`

Most important comparison:
- At matched checkpoints, `v18` tracked `ge5sma5y` very closely:
  - `~190M`: wave `18.44` vs `18.37`
  - `~564M`: wave `20.34` vs `21.28`
  - `~940M`: wave `27.36` vs `27.37`
  - `~1.31B`: wave `29.29` vs `29.00`
- That is strong evidence that the pruned observation contract did **not**
  destroy learning capacity.

Comparison vs `v17` warm-start (`mv0snohb`):

Top-line:
- `lxttb7uo`: wave `29.20`, max wave `31`, return `4955.7`
- `mv0snohb`: wave `25.01`, max wave `32`, return `291.8`

What this means:
- `v18` from scratch clearly outperformed the warm-started `v17` package.
- The `v17` failure was not that scratch training was too hard for the
  new backend. It was that the `v17` recipe itself was weak enough to
  need inherited competence.
- `v18` removed that ambiguity.

Observations:

1. **The main cause of the `v17.1` collapse was the recipe, not the
   pruned backend / obs contract.**
   - `v18` kept the pruned 106-float observation contract
   - kept Jad telegraph removed
   - kept the capped stall penalty
   - kept the current backend / parity fixes
   - and still recovered to the wave-29 frontier once the PPO/reward
     recipe was restored

2. **The strong scratch recipe still needs patience.**
   - `v18` looked ordinary for hundreds of millions of steps
   - the breakout window again arrived around `~600M-900M`
   - future ablations should not be declared dead too early unless they
     are clearly flat well past that region

3. **The capped stall penalty appears safe.**
   - `ge5sma5y` used the same broad recipe but with `shape_wave_stall_cap = 0.0`
   - `v18` used `-3.0` and still matched or slightly beat the frontier
   - so the cap does not appear to be the source of previous regressions

4. **Resource thrift came back as a consequence of competence.**
   - removing the extra threat-aware resource penalties did not make the
     agent waste resources more
   - it actually improved pot timing substantially
   - this supports the idea that the added `v17` resource penalties were
     distracting scratch learning more than helping it

5. **Rare late-wave milestone metrics are still not ideal as final
   summary indicators.**
   - final `reached_wave_31` and `cleared_wave_30` read as `0.0`
   - but `max_wave = 31` proves the run did reach wave 31 at least once
   - these counters are averaged over the last logging window, not a
     whole-run cumulative truth

Key takeaway:
- `v18` is the successful answer to the `v17.1` control failure.
- The backend / pruned obs changes are compatible with frontier
  learning.
- The real regression source in `v17` / `v17.1` was the weakened PPO
  profile and reward mix, not the observation contract itself.
- `v18` should now replace `ge5sma5y` as the clean scratch baseline on
  the current `fight-caves-rl` codebase.

Recommendation for next step:
- treat `lxttb7uo` as the new baseline run
- use a best checkpoint from the `~1.21B-1.29B` plateau for eval /
  future warm-start experiments rather than blindly using the final step
- do **not** add more reward complexity right away
- if the next question is about backend architecture or parity, that can
  now proceed without the training recipe itself being in doubt

---

## v17.1 (2026-04-06)

Changes from v17:

This is the clean control run for the current `fight-caves-rl` v17 package.
It keeps the new observation surface, reward weights, threat-aware
resource shaping, capped stall penalty, and PPO hyperparameters from
`v17`, but removes the two confounders that made `mv0snohb` hard to
interpret:
- **no warm-start**
- **no curriculum**

Why this run exists:
- `v17` warm-start + mixed curriculum was more stable than old `v16.2`,
  but it did not preserve the `v16.2a` wave-29 frontier.
- That run changed too many things at once:
  checkpoint initialization, curriculum distribution, reward weights,
  and PPO settings.
- `v17.1` isolates the actual question:
  are the current `v17` obs/reward/hyperparameter changes themselves
  better than `v16.2a` when trained from scratch on the same backend?

Config / training changes from `v17`:
- removed warm-start (`LOAD_MODEL_PATH` unset)
- disabled all curriculum buckets:
  - `curriculum_wave=0`, `curriculum_pct=0.0`
  - `curriculum_wave_2=0`, `curriculum_pct_2=0.0`
  - `curriculum_wave_3=0`, `curriculum_pct_3=0.0`
- shortened run length:
  - `total_timesteps: 5B -> 1.5B`

Everything else stays at the current `v17` values:
- `learning_rate = 0.00025`
- `clip_coef = 0.12`
- `ent_coef = 0.03`
- `w_damage_dealt = 0.5`
- `w_npc_kill = 3.0`
- `w_correct_danger_prayer = 2.0`
- threat-aware food/pot shaping kept on
- capped stall penalty kept on

Live `fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 100

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.75
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 2.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
shape_food_full_waste_penalty = -5.0
shape_food_waste_scale = -1.0
shape_food_safe_hp_threshold = 0.70
shape_food_no_threat_penalty = -0.50
shape_pot_full_waste_penalty = -5.0
shape_pot_waste_scale = -1.0
shape_pot_safe_prayer_threshold = 0.50
shape_pot_no_threat_penalty = -0.50
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.0
shape_npc_melee_penalty = -0.5
shape_wasted_attack_penalty = -0.3
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.75
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -3.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.5
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 1

[train]
total_timesteps = 1_500_000_000
learning_rate = 0.00025
anneal_lr = 1
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.12
vf_coef = 0.5
ent_coef = 0.03
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.12,
vf_coef=0.5, ent_coef=0.03, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.5B, lr=0.00025, anneal_lr=1.

Success criteria:
- fair comparison against `v16.2a`
- no checkpoint transfer effects
- no curriculum contamination of average-wave metrics
- answer whether the current `v17` package actually improves prayer
  learning and late-wave competence on its own

Results (1.5B steps, ~12.5min, wandb run `q3ald8bc`):
- SPS: **2.02M**
- Wave reached: **17.17** avg
- Max wave: **24**
- Most NPCs slayed: **86**
- Episode return: **1315.7**
- Episode length: **997**
- Score: 0
- Epochs: 1430
- Entropy: **8.044**
- Clipfrac: **0.0001**
- Value loss: **20.69**
- Policy loss: `+0.0154`

Analytics (final):
- prayer_uptime: **0.647**
- prayer_uptime_melee: **0.386**
- prayer_uptime_range: **0.189**
- prayer_uptime_magic: **0.072**
- correct_prayer: **224.9**
- wrong_prayer_hits: **62.4**
- no_prayer_hits: **96.5**
- prayer_switches: **339.4**
- damage_blocked: **1,220.5**
- dmg_taken_avg: **2683.5**
- pots_used: **32.0**
- avg_prayer_on_pot: **0.987**
- food_eaten: **20.0**
- avg_hp_on_food: **0.926**
- food_wasted: **19.5**
- pots_wasted: **32.0**
- tokxil_melee_ticks: **12.1**
- ketzek_melee_ticks: **0.0**
- reached_wave_30: **0.0**
- cleared_wave_30: **0.0**
- reached_wave_31: **0.0**

Progression:
- This run never had a real breakout phase.
- It briefly touched **wave 18.0** by `~84M` steps, then spent the
  rest of the full `1.5B` run oscillating in a very tight **16.6-17.8**
  band.
- It never reached wave 20, never formed a late inflection, and never
  showed the kind of entropy drop or return expansion that marked the
  `ge5sma5y` breakthrough.
- Unlike `v16.2a`, which built foundations slowly and then broke out
  hard after `~550M-700M`, `v17.1` was flat from start to finish.

Comparison vs `v16.2a` (`ge5sma5y`, scratch, no curriculum, 1.5B):

Top-line:
- `q3ald8bc`: wave `17.17`, max wave `24`, return `1315.7`
- `ge5sma5y`: wave `28.82`, max wave `31`, return `4593.7`

What improved:
- Slightly better SPS: `2.02M` vs `1.95M`
- Lower average damage taken: `2683.5` vs `3334.6`
- Lower Tok-Xil melee exposure than `ge5sma5y`: `12.1` vs `28.4`

What regressed badly:
- Wave progression collapsed:
  - `v17.1` never reached wave 20
  - `v16.2a` reached wave 20 around `~550M`, wave 25 around `~730M`,
    wave 28 around `~963M`, and wave 29 around `~1.17B`
- Combat conversion was dramatically worse:
  - `correct_prayer 224.9` vs `1604.2`
  - `damage_blocked 1.2k` vs `23.2k`
  - `prayer_switches 339.4` vs `1343.2`
  - `episode_length 997` vs `3281`
- Resource behavior was worse, not better:
  - `pots_used 32.0` vs `30.5`
  - `avg_prayer_on_pot 0.987` vs `0.767`
  - `avg_hp_on_food 0.926` vs `0.923`
  - `pots_wasted 32.0` vs `19.8`

Interpretation:
- `v17.1` was safer in a narrow sense, but it did not learn the actual
  Fight Caves task nearly well enough.
- The current `v17` package from scratch is nowhere near the `v16.2a`
  frontier.

Comparison vs `v17` warm-start (`mv0snohb`):

Top-line:
- `q3ald8bc`: wave `17.17`, max wave `24`, return `1315.7`
- `mv0snohb`: wave `25.01`, max wave `32`, return `291.8`

What this means:
- Warm-start + curriculum was clearly carrying a large fraction of the
  `v17` result.
- The current `v17` package does **not** bootstrap well from scratch.
- So the failure mode in `mv0snohb` was not "the package can learn but
  warm-start hurt it a little." The package itself is weak enough that
  it needed inherited competence just to reach the mid-20s.

Comparison vs older `v16+` baselines:
- `v17.1` is only slightly better than the broken `v16` run on average
  wave (`17.17` vs `16.19`) and still far below:
  - `v16.1`: `25.16`
  - `v15.3`: `28.36`
  - `v15.1`: `28.65`
- That is a strong signal that the current `v17` package is not just
  "not yet optimized"; it is materially worse than older working
  scratch recipes.

Diagnosis:

1. **The current `v17` optimizer profile is too conservative for
   scratch training.**
   - `learning_rate=0.00025` and `clip_coef=0.12` appear appropriate
     for fine-tuning or warm-start stabilization, not for learning
     Fight Caves from random weights.
   - Evidence: entropy stayed extremely high (`~8.0`) for the entire
     run, and clipfrac decayed quickly to essentially zero without any
     learning inflection.
   - The policy barely specialized at all.

2. **The current `v17` reward mix is likely under-driving combat
   progress while over-complicating resource behavior.**
   - `w_damage_dealt` was cut to `0.5` from the stronger scratch recipe.
   - threat-aware food/pot penalties were added
   - danger-prayer reward was increased
   - but the resulting policy still wasted nearly every potion dose and
     most food value while failing to learn late-wave combat
   - the extra shaping complexity is not buying the intended behavior

3. **Observation improvements alone did not rescue prayer learning.**
   - `correct_prayer` is non-zero, so the policy is not totally blind
   - but the scale is nowhere near `v16.2a`
   - this suggests the new threat obs are probably useful, but they
     still need a stronger optimization / reward recipe around them

4. **Current resource penalties are not teaching thrift from scratch.**
   - `avg_prayer_on_pot = 0.987`
   - `pots_wasted = 32.0`
   - `avg_hp_on_food = 0.926`
   - `food_wasted = 19.5`
   - This is nearly worst-case usage despite the threat-aware penalties.
   - So those penalties are either too weak, too hard to learn through,
     or simply distracting at this stage.

Key takeaway:
- `v17.1` answered the control question clearly:
  the current `v17` package is **not** a better scratch recipe than
  `v16.2a`.
- The best path forward is not a sweep of the current package.
- The best path forward is to keep the new observation contract and
  stability fixes, but restore a proven learning recipe around them.

Recommendation for `v17.2`:

Run a focused ablation:
- keep the current observation improvements:
  - `effective_style_now`
  - incoming-hit timeline summaries
  - `prayer_drain_counter`
- keep prayer flick reward disabled
- keep the capped stall penalty

But revert the training / reward recipe closer to `ge5sma5y`:
- `learning_rate: 0.00025 -> 0.001`
- `clip_coef: 0.12 -> 0.2`
- `ent_coef: 0.03 -> 0.02`
- `w_damage_dealt: 0.5 -> 1.0`
- `w_npc_kill: 3.0 -> 2.0`
- `w_correct_danger_prayer: 2.0 -> 1.0`
- disable the extra threat-aware food/pot penalties:
  - `shape_food_safe_hp_threshold = 1.0`
  - `shape_food_no_threat_penalty = 0.0`
  - `shape_pot_safe_prayer_threshold = 1.0`
  - `shape_pot_no_threat_penalty = 0.0`
- keep no curriculum
- keep no warm-start
- run `1.5B` again

Why this is the best next step:
- it isolates whether the failure came from the current `v17` reward /
  PPO recipe rather than the new observation contract
- it preserves the likely-good `fight-caves-rl` threat representation work
- it restores the only scratch recipe on current `fight-caves-rl` that has
  actually proven it can reach the wave-29 frontier

What not to do next:
- do not run another long warm-start + curriculum job yet
- do not sweep the current `v17.1` package yet
- do not add more shaping complexity until the baseline learns again

---

## v17 (2026-04-06)

Changes from v16.2a:

Switch back to the staged v17 config on the current `fight-caves-rl` codebase
and warm-start from the best `v16.2a` bridge checkpoint.

Warm-start source:
- Run: `ge5sma5y`
- Recommended checkpoint:
  `<repo-root>/pufferlib_4/checkpoints/fight_caves/ge5sma5y/0000001364197376.bin`

Config / training changes:
- total_timesteps: `1.5B` bridge run → **5B**
- learning_rate: `0.001` → **0.00025**
- clip_coef: `0.2` → **0.12**
- ent_coef: `0.02` → **0.03**
- mixed curriculum restored:
  - `curriculum_wave=30`, `curriculum_pct=0.25`
  - `curriculum_wave_2=31`, `curriculum_pct_2=0.10`
  - `curriculum_wave_3=32`, `curriculum_pct_3=0.05`
- reward weights restored to the staged v17 values:
  - `w_damage_dealt: 1.0 → 0.5`
  - `w_npc_kill: 2.0 → 3.0`
  - `w_correct_danger_prayer: 1.0 → 2.0`
- threat-aware resource penalties restored:
  - `shape_food_safe_hp_threshold = 0.70`
  - `shape_food_no_threat_penalty = -0.50`
  - `shape_pot_safe_prayer_threshold = 0.50`
  - `shape_pot_no_threat_penalty = -0.50`
- wave stall cap restored:
  - `shape_wave_stall_cap = -3.0`

Live `fight_caves.ini`:

```ini
[base]
env_name = fight_caves
checkpoint_interval = 100

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.75
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 2.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
shape_food_full_waste_penalty = -5.0
shape_food_waste_scale = -1.0
shape_food_safe_hp_threshold = 0.70
shape_food_no_threat_penalty = -0.50
shape_pot_full_waste_penalty = -5.0
shape_pot_waste_scale = -1.0
shape_pot_safe_prayer_threshold = 0.50
shape_pot_no_threat_penalty = -0.50
shape_wrong_prayer_penalty = -1.0
shape_npc_specific_prayer_bonus = 2.0
shape_npc_melee_penalty = -0.5
shape_wasted_attack_penalty = -0.3
shape_wave_stall_start = 500
shape_wave_stall_base_penalty = -0.75
shape_wave_stall_ramp_interval = 50
shape_wave_stall_cap = -3.0
shape_not_attacking_grace_ticks = 2
shape_not_attacking_penalty = -0.5
shape_kiting_reward = 1.0
shape_kiting_min_dist = 5
shape_kiting_max_dist = 7
shape_unnecessary_prayer_penalty = -0.2
shape_resource_threat_window = 2
curriculum_wave = 30
curriculum_pct = 0.25
curriculum_wave_2 = 31
curriculum_pct_2 = 0.10
curriculum_wave_3 = 32
curriculum_pct_3 = 0.05
disable_movement = 0

[vec]
total_agents = 4096
num_buffers = 1

[train]
total_timesteps = 5_000_000_000
learning_rate = 0.00025
anneal_lr = 1
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.12
vf_coef = 0.5
ent_coef = 0.03
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.12,
vf_coef=0.5, ent_coef=0.03, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=5B, lr=0.00025, anneal_lr=1.

Why this setup:
- `v16.2a` on `fight-caves-rl` proved the stack is working and reached the
  wave 29 frontier cleanly by `~1.2B`.
- The best saved `v16.2a` checkpoint sits on the `29.2` plateau,
  before the slight fade in the final bridge-run segment.
- Lower LR + tighter clipping should preserve the plateau instead of
  repeating the large-update instability seen in the old 20B `v16.2`.
- Higher entropy and mixed `30/31/32` scaffolding should force more
  late-wave prayer decisions without restarting the policy from scratch.

Results (5B steps, ~42.0min, wandb run `mv0snohb`):
- Warm-started from:
  `<repo-root>/pufferlib_4/checkpoints/fight_caves/ge5sma5y/0000001364197376.bin`
- SPS: **2.03M**
- Wave reached: **25.01** avg
- Max wave: **32**
- Most NPCs slayed: **82**
- Episode return: **291.8**
- Episode length: **824**
- Score: 0
- Epochs: 4768
- Entropy: **7.635**
- Clipfrac: **0.0007**
- Value loss: **19.60**
- Policy loss: `-0.0159`

Analytics (final):
- prayer_uptime: **0.775**
- prayer_uptime_melee: **0.542**
- prayer_uptime_range: **0.141**
- prayer_uptime_magic: **0.091**
- correct_prayer: **230.7**
- wrong_prayer_hits: **58.3**
- no_prayer_hits: **64.2**
- prayer_switches: **228.6**
- damage_blocked: **5,223**
- dmg_taken_avg: **3,049**
- pots_used: **24.8**
- avg_prayer_on_pot: **0.953**
- food_eaten: **20.0**
- avg_hp_on_food: **0.898**
- food_wasted: **16.5**
- pots_wasted: **23.7**
- tokxil_melee_ticks: **4.09**
- ketzek_melee_ticks: **0.0**

Progression:
- The run opened with inherited late-wave competence from the warm
  checkpoint plus mixed curriculum. The first sampled point was
  already wave `30.0` at `2.1M` steps.
- Early adaptation was unstable. After a brief spike to **wave 27.6**
  at `453M`, the run spent most of `0.7B-1.5B` back in the `17-18`
  range.
- Mid-run rebuilt some competence but never recovered the `v16.2a`
  frontier. From `1.8B-4.1B` it climbed into the low-mid 20s, with
  the best later sampled window at **wave 26.5** around `4.11B`.
- Final `~900M` steps were effectively annealed out. Clipfrac fell to
  near zero, entropy stayed high, and the run finished at **25.0**
  without another late-wave breakthrough.

Comparison vs `v16.2a` (`ge5sma5y`, no curriculum, scratch, 1.5B):

What improved:
- Slightly better throughput: `2.03M` vs `1.95M` SPS.
- Much better Tok-Xil spacing: `tokxil_melee_ticks 4.1` vs `28.4`.
- Lower overall damage taken: `3049` vs `3335`.
- Better resource discipline:
  - `pots_used 24.8` vs `30.5`
  - `avg_prayer_on_pot 0.953` vs `0.767`
  - `avg_hp_on_food 0.898` vs `0.923`
  - lower food and pot waste totals

What regressed:
- Wave progression and overall combat conversion dropped sharply:
  - final wave `25.0` vs `28.8`
  - return `291.8` vs `4593.7`
  - episode length `824` vs `3281`
- Prayer performance regressed hard:
  - `correct_prayer 230.7` vs `1604.2`
  - `no_prayer_hits 64.2` vs `28.3`
  - `prayer_switches 228.6` vs `1343.2`
  - `damage_blocked 5.2k` vs `23.2k`
- Even allowing for curriculum/warm-start contamination of the top-line
  metrics, the sampled `v17` peak (`27.6`) never recovered the
  `29.1-29.2` plateau reached by `v16.2a`.

Comparison vs older `v16+` runs:
- `v17` is **more stable** than the old 20B `v16.2`. There was no
  entropy collapse, no catastrophic stall-penalty death spiral, and
  value loss stayed moderate.
- `v17` is **not stronger** than the best frontier runs:
  - `v16.2a`: wave `28.8` final, `29.2` plateau
  - `v16.2`: wave `29.1` peak
  - `v15.3`: wave `28.4` final
  - `v15.1`: wave `28.6` final
- So the current `v17` package improved discipline and stability, but
  reduced the ceiling and lost too much late-wave competence.

Diagnosis:

1. **The observation fixes likely helped, but the full package was too
   conservative.**
   - Lower Tok-Xil melee ticks and lower damage taken suggest the new
     threat representation is doing something useful.
   - But the policy became much less willing to prayer-switch and much
     less able to convert safe play into actual wave clears.

2. **Warm-start + mixed curriculum was not a clean fit.**
   - `ge5sma5y` was a strong no-curriculum frontier policy.
   - `v17` immediately changed reward weights, raised entropy, lowered
     LR, tightened clipping, and injected `30/31/32` starts.
   - That is too many simultaneous shifts for a warm policy. The run
     appears to have unlearned a good portion of the inherited frontier
     behavior before it could stabilize.

3. **The optimizer may have been too gentle to re-form the frontier.**
   - `learning_rate=0.00025` and `clip_coef=0.12` gave a very safe run.
   - Safety was achieved, but the run never made a second decisive move
     back toward wave 29.
   - By late training, clipfrac was nearly zero while entropy was still
     very high. That is a bad combination for a warm-start refinement:
     lots of randomness, very little useful policy movement.

4. **The stronger resource-threat shaping may be over-weighting thrift
   relative to late-wave aggression.**
   - The run drank later, ate later, blocked less, and used less prayer.
   - Those are not bad properties by themselves.
   - But in aggregate they look like the policy learned to be
     "disciplined" faster than it learned to be "decisive."

Key takeaway:
- `v17` did **not** prove that the new package is stronger than
  `v16.2a`.
- It **did** prove that the new package is much less collapse-prone and
  likely has better threat semantics.
- The next step should be a clean control run, not another mixed
  curriculum warm-start.

Recommendation for `v17.1`:

Run a strict baseline/control:
- **No curriculum**
- **No warm-start**
- **1.5B steps**
- Keep the current `v17` observation/reward/hyperparameter package
  otherwise unchanged

Why:
- This isolates the real question: are the `v17` obs/reward changes
  themselves good, independent of curriculum and checkpoint transfer?
- It gives a fair apples-to-apples comparison against `v16.2a`.
- If `v17.1` still underperforms `v16.2a`, the problem is in the
  current `v17` package itself, not in warm-start mechanics.

If `v17.1` is still weak after that, the first two follow-up knobs I
would test are:
- `ent_coef: 0.03 -> 0.02`
- temporarily disabling curriculum even for future warm-start runs
  until the baseline package proves it can naturally rebuild the
  frontier

---

## v16.2a (2026-04-06)

Changes from v16.2:

This was a `v16.2-style` bridge run on the current `fight-caves-rl` codebase,
used for two purposes:
- validate that the isolated `fight-caves-rl` training stack runs end-to-end
- produce a fresh warm-start checkpoint compatible with the new backend

Important caveat:
- This is not a pure legacy reproduction. PPO hyperparameters and the
  dense reward intent stayed `v16.2-style`, but the runtime surface was
  the current `fight-caves-rl` implementation.

`fight-caves-rl` backend / env changes relative to old `rqvxfqmq`:
- `effective_style_now` observation replaced the old misleading
  per-NPC base attack-style feature
- incoming-hit timeline summaries were added to policy observation
- `prayer_drain_counter` was exposed in observation
- prayer flick reward was disabled
- reward shaping moved out of hardcoded logic into config-backed
  `shape_*` controls
- local PufferLib backend build/runtime paths were isolated inside
  `fight-caves-rl`, and the env-core reward path was optimized

Bridge-run config changes:
- total_timesteps: `20B` → **1.5B**
- checkpoint_interval: `200` → **100**
- kept no curriculum
- kept `v16.2-style` PPO settings:
  - `learning_rate = 0.001`
  - `clip_coef = 0.2`
  - `ent_coef = 0.02`
- set the new `shape_*` knobs to emulate documented `v16.1/v16.2`
  behavior on the current code:
  - resource no-threat penalties disabled
  - safe food/prayer thresholds set to `1.0`
  - wave stall cap disabled (`shape_wave_stall_cap = 0.0`)

```ini
[train]
total_timesteps = 1_500_000_000
learning_rate = 0.001
anneal_lr = 1
clip_coef = 0.2
ent_coef = 0.02

[env]
curriculum_wave = 0
curriculum_pct = 0.0
curriculum_wave_2 = 0
curriculum_pct_2 = 0.0
curriculum_wave_3 = 0
curriculum_pct_3 = 0.0
shape_food_safe_hp_threshold = 1.0
shape_food_no_threat_penalty = 0.0
shape_pot_safe_prayer_threshold = 1.0
shape_pot_no_threat_penalty = 0.0
shape_wave_stall_cap = 0.0
```

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.5B, lr=0.001, anneal_lr=1.

Results (1.5B steps, ~12.7min, wandb run ge5sma5y):
- SPS: 1.95M
- Wave reached: **28.82** avg
- Max wave: **31**
- Most NPCs slayed: **119**
- Episode return: **4593.7**
- Episode length: **3281**
- Score: 0
- Epochs: 1430
- Entropy: **6.225**
- Clipfrac: **0.0004**
- Value loss: 66.27

Analytics (final):
- prayer_uptime: 0.903
- prayer_uptime_melee: 0.559
- prayer_uptime_range: 0.275
- prayer_uptime_magic: 0.069
- correct_prayer: 1604.2
- wrong_prayer_hits: 174.7
- no_prayer_hits: 28.3
- prayer_switches: 1343.2
- damage_blocked: 23,200
- dmg_taken_avg: 3334.6
- pots_used: 30.5
- avg_prayer_on_pot: 0.767
- food_eaten: 20.0
- avg_hp_on_food: 0.923
- food_wasted: 19.0
- pots_wasted: 19.8
- tokxil_melee_ticks: 28.4
- reached_wave_30: 0.230
- cleared_wave_30: 0.000
- reached_wave_31: 0.000

Progression:
- 0-550M: Similar early-game build-up to old runs, then accelerated.
- **Wave 20 at 551.6M** steps.
- **Wave 25 at 729.8M** steps.
- **Wave 28 at 962.6M** steps.
- **Wave 29 at 1.168B** steps.
- Peak plateau: `1.21B-1.36B`, wave `29.1-29.2`, return `4779-4896`.
- Final segment (`1.36B-1.50B`) faded slightly to wave `28.8`, but
  never collapsed and still ended near the frontier.

Matched comparison vs old `v16.2` (`rqvxfqmq`) near `1.5B`:
- `ge5sma5y @ 1.499B`: wave `28.82`, return `4593.7`, entropy `6.225`,
  clipfrac `0.0004`, value loss `66.3`
- `rqvxfqmq @ 1.487B`: wave `28.85`, return `4693.2`, entropy `5.665`,
  clipfrac `0.307`, value loss `87.1`

What improved:
- Faster learning curve on `fight-caves-rl`:
  - wave `20`: `551.6M` vs `780.1M`
  - wave `25`: `729.8M` vs `992.0M`
  - wave `28`: `962.6M` vs `992.0M`
  - wave `29`: `1.168B` vs `2.930B`
- Lower value loss than old `v16.2` at the matched `~1.5B` window
- No sign of the destructive collapse pattern within the bridge-run span

What regressed or stayed weaker:
- Final matched wave/return at `~1.5B` were basically tied, not better
- Fewer correct prayer blocks than old `v16.2` at the matched window
- Slightly shorter episodes and no max-wave `32` spike

Diagnosis:
- The `fight-caves-rl` stack is healthy enough to use for the real v17 run.
- The bridge run did not prove a giant average-wave improvement over
  old `v16.2` by `1.5B`, but it did show a much earlier climb to the
  frontier and a cleaner, less destructive optimization profile.
- Best warm-start checkpoint is on the `29.2` plateau:
  `<repo-root>/pufferlib_4/checkpoints/fight_caves/ge5sma5y/0000001364197376.bin`

---

## v16.2 (2026-04-06)

Changes from v16.1:

No reward or signal changes. Same config as v16.1.
- total_timesteps: 1.25B → **20B**. v16.1 showed a late breakthrough
  at 890M steps (wave 20→25 in 200M steps) that was still climbing
  when LR annealed to ~0. 20B gives the agent time for the
  breakthrough phase to fully play out.

```ini
[train]
total_timesteps = 20_000_000_000
learning_rate = 0.001
anneal_lr = 1
```

All other config, loadout, rewards identical to v16.1.

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=20B, lr=0.001, anneal_lr=1.

Results (20B steps, ~191min, wandb run rqvxfqmq):
- SPS: 1.67M
- Wave reached: 12.73 avg (final — collapsed), **29.1 peak** (at ~4.7B)
- Max wave: **32**
- Most NPCs slayed: **121**
- Episode return: 512.6 (final), 5,235 peak
- Episode length: 837 (final), 3,610 peak
- Score: 0
- Epochs: 19,073
- Entropy: **0.025** (final — completely collapsed)
- Clipfrac: 0.003 (final)
- Value loss: 43.5

Analytics (final):
- prayer_uptime: 0.988
- correct_prayer: 0.0
- dmg_taken_avg: 1,395
- pots_used: 7.6
- food_eaten: 0.0

Progression — THREE DISTINCT PHASES:

Phase 1 — Growth (0-4.7B, LR 0.001→0.00077):
- Wave climbed 18.7 → 29.1 over 4.7B steps. Best wave performance
  of any run. Episode return grew from 1,283 to 5,235.
- Agent learned kiting, combat engagement, and basic prayer.
- Entropy declined steadily 8.0→4.8 as policy committed to strategy.
- Clipfrac elevated at 0.4-0.5 — large policy updates happening.
  This is healthy during the growth phase but foreshadows instability.
- Value loss volatile (64-120) — the value function struggled to
  keep up with rapidly changing policy. Normal during fast learning.
- **Key checkpoint: ~4.7B steps = best policy of the entire run.**

Phase 2 — Destabilization (4.7B-9.8B, LR 0.00077→0.00051):
- Wave regressed 29.1 → 27.0 over 5B steps. Slow bleed, not sudden.
- Entropy continued dropping 4.8→1.9 — policy over-committing.
  The agent was specializing into a narrow strategy that worked at
  wave 27-29 but couldn't improve further.
- Clipfrac still 0.3-0.5 — LR was still high enough for destructive
  updates. The policy was near-converged but gradients kept pushing.
- Value loss remained extreme (89-120) — value function diverged
  from actual returns, causing bad advantage estimates, which caused
  bad policy updates, creating a feedback loop.
- Return dropped 5,235→3,498 despite wave only dropping 29→27.
  The agent earned less reward per wave because its strategy was
  degrading (worse kiting, worse combat efficiency).

Phase 3 — Entropy Collapse (9.8B-20B, LR 0.00051→0):
- **Catastrophic entropy collapse at ~9.8B.** Entropy crashed from
  1.9 to 0.027 in one logging interval. Wave dropped 27→13 instantly.
- The policy became deterministic — entropy 0.025 means the agent
  picks the same action in every state with >99.9% probability.
  It cannot explore or adapt. Effectively a fixed program.
- Episode return hit **-269,923** at 14.2B — the escalating wave
  stall penalty generated massive negative reward on a single bad
  episode (1052 ticks at wave 13 = ~550 ticks over limit, scale
  ~11x, penalty ~-8.25/tick for 550 ticks = -4,537 per episode,
  but accumulated over multiple waves within the episode).
- Policy could not recover because entropy=0 means no exploration.
  LR was still 0.0005 but every gradient update just reinforced the
  same frozen action distribution.
- food_eaten=0.0 — agent stopped eating entirely. pots_used=7.6 —
  barely drinking. correct_prayer=0.0 — praying 99% of ticks but
  never the correct type. The agent is a zombie — alive but not
  functioning.
- **The last 10B steps (50% of training) were completely wasted.**

Root cause analysis:

1. **LR annealing too slow for 20B.** Linear anneal from 0.001 over
   20B means LR was still 0.00075 at 5B when the policy was already
   near-converged. Previous 1.25B runs annealed fast enough that LR
   hit ~0 before destabilization could occur. With 20B the dangerous
   LR zone (0.001-0.0005) lasted for ~10B steps.

2. **Clipfrac at 0.4-0.5 for too long.** PPO's clip ratio is designed
   to limit policy updates. Clipfrac >0.3 means >30% of samples are
   being clipped — the policy wants to change faster than clipping
   allows. This sustained pressure eventually finds a way through
   (a bad batch that aligns with the clip direction), causing a
   destructive update that triggers the entropy collapse.

3. **Escalating wave stall penalty amplified the collapse.** Once the
   agent dropped to wave 13 and couldn't recover, the stall penalty
   generated -4K+ per episode. These extreme negative rewards created
   enormous gradients that pushed the policy further into the frozen
   state. The penalty designed to prevent stalling actually made
   recovery impossible.

4. **No entropy floor.** PPO's entropy coefficient (ent_coef=0.02)
   provides a small incentive to maintain entropy, but it was
   overwhelmed by the much larger reward signals. A minimum entropy
   constraint or adaptive entropy would have prevented the 0.025
   collapse.

Key takeaways:

- The v16.1 reward signals ARE WORKING. The agent reached wave 29.1
  with clean, non-exploited returns. The problem is training stability
  over long runs, not reward design.
- **The best policy exists at ~4.7B steps.** The checkpoint at that
  point should be the starting point for future work.
- Long runs need either: lower LR, faster annealing schedule,
  entropy floor, or checkpoint-based early stopping.
- The escalating wave stall penalty needs a cap to prevent
  catastrophic gradient amplification during collapse.

---

## v16.1 (2026-04-05)

Changes from v16:

Fixes for v16 prayer flick exploit + new signals:

Prayer flick reward fixed:
- Now requires ALL of: prayer just toggled, blocked an actual hit,
  no prayer point drained, AND agent has a target selected. Cannot
  be farmed by toggling prayer without engaging in combat.

NPC melee range penalty expanded:
- Now penalizes ALL NPCs at melee range (distance 1), not just
  Ket-Zek/Tok-Xil/MejKot. Includes Tz-Kih, Tz-Kek, Tz-Kek-Sm.
  Agent should kite everything.

NPC-specific prayer bonus expanded:
- Added Tz-Kih, Tz-Kek, Tz-Kek-Sm → protect melee (+2.0).
  Now all combat NPCs have explicit prayer mappings.

Wasted attack penalty (-0.3):
- Fires when attack timer is 0 (ready to fire), agent has a target
  but didn't deal damage this tick. Encourages attack-while-kiting.

Wave stall penalty (escalating after 500 ticks):
- -0.75/tick base after 500 ticks, scales +0.75 every 50 ticks.
  500=-0.75, 550=-1.50, 600=-2.25, etc. Resets on wave clear.

New analytics:
- food_wasted: sharks that overhealed (lower is better)
- pots_wasted: doses that over-restored (lower is better)

All other config unchanged from v16.

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.25B, lr=0.001, anneal_lr=1.

Results (1.25B steps, ~10.5min, wandb run wcfgtykg):
- SPS: 1.93M
- Wave reached: 25.16 avg (final), max wave 28
- Most NPCs slayed: 112
- Episode return: 3,144 (healthy, not inflated)
- Episode length: 2,231 (short — wave stall working)
- Score: 0
- Epochs: 1192
- Entropy: 6.565 (very healthy, still exploring)
- Clipfrac: 0.001 (LR annealed to ~0)
- Value loss: 20.8

Analytics:
- prayer_uptime: 0.800
- dmg_taken_avg: 2,730
- pots_used: 32.0
- food_eaten: 20.0

Progression — LATE BREAKTHROUGH PATTERN:
- 0-890M: Slow climb wave 17 → 20.8. Spent 890M steps building
  foundational skills (kiting, attacking, basic prayer). Much slower
  than previous runs which climbed to 21 in 100M steps.
- **890M: BREAKOUT.** Wave jumped 20.8→22→23.5→25.2 in ~200M steps.
  Entropy dropped 7.4→6.1 (policy committed to a strategy). Value
  loss spiked to 41 (world model updating dramatically). Policy loss
  hit -0.14 (massive policy update). Something clicked.
- 1010M-1250M: Stabilized at 25.0-25.3. Still climbing when LR
  annealed to ~0 and policy stopped updating.

Diagnosis — reward signals working, needs more steps:

First run with NO reward exploit. Episode return 3,144 is healthy
(not inflated). Episode length 2,231 is short (wave stall penalty
working). No prayer farming. The late breakthrough at 890M suggests
the agent needed ~900M steps to learn foundational behaviors before
the "real" learning could begin.

The breakthrough was STILL HAPPENING when training ended. The agent
was climbing from 23 to 25 in the final 200M steps. LR had already
annealed to ~0 (clipfrac=0.001), cutting off further learning. A
longer run would let the agent continue past wave 25.

Recommendation: same config, 2.5B steps. The agent needs ~1B to
build foundations, then ~1.5B at useful LR for the breakthrough
phase. No reward changes — v16.1 signals are clean.

---

## v16 (2026-04-05)

Changes from v15.3:

Major reward shaping overhaul focused on teaching prayer mechanics
and punishing resource waste through domain-specific signals.

New reward signals:
- **NPC-specific prayer bonus (+2.0):** Extra reward when correct
  prayer blocks a hit from a specific dangerous NPC while attacking:
  Ket-Zek→protect magic, Tok-Xil→protect missiles, MejKot→protect
  melee. Stacks with the generic +1.0 correct prayer reward. Explicit
  hint that shortcuts the NPC-prayer mapping the agent needs to learn.
- **Dangerous NPC melee range penalty (-0.5/tick per NPC):** Penalizes
  having Ket-Zek, Tok-Xil, or Yt-MejKot at distance 1. These NPCs
  should be kited. At melee range, dual-mode NPCs use melee attacks,
  making the agent's ranged prayer useless.
- **Prayer flick reward (+0.5):** Fires when prayer was just toggled
  ON this tick AND the drain counter hasn't exceeded resistance
  (meaning no prayer point was drained). Teaches the OSRS prayer
  flicking mechanic: activate prayer briefly to block a hit, then
  deactivate before drain fires. Conserves prayer points.

Changed reward signals:
- **Food/pot rewards removed entirely.** No +1 for good timing. Only
  waste-based penalty remains:
  - Full HP/prayer: -5.0 (complete waste)
  - Otherwise: -(wasted / max_heal_or_restore), 0 to -1.0 scaled
  - Philosophy: consuming scarce resources is never rewarded. The
    agent eats/drinks for survival, not for reward.

Removed legacy signals:
- All old food/pot timing rewards (+1/-1 at thresholds) removed
- Config weights w_food_used, w_food_used_well, w_prayer_pot_used
  are still parsed but completely unused

Analytics changes:
- Replaced `wrong_prayer` with `dmg_taken_avg` (total damage taken
  per episode). More useful for understanding overall survivability.
- Renamed `correct_blocks` to `correct_prayer` — tracks how many
  hits per episode were correctly blocked by matching prayer.

All other config unchanged from v15.3 (no curriculum, Loadout B,
anneal_lr=1, 1.25B steps, kiting +1.0, prayer waste -0.2).

Hardcoded in fight_caves.h:
- NPC-specific prayer: +2.0 (Ket-Zek/magic, Tok-Xil/ranged, MejKot/melee) — requires agent to have a target selected
- Dangerous NPC melee range: -0.5/tick per NPC at distance 1
- Prayer flick: +0.5 when prayer on without drain
- Food waste: -5.0 at full HP, else -(wasted/200)
- Pot waste: -5.0 at full prayer, else -(wasted/170)
- Kiting: +1.0 at range 5-7 while attacking (from v15.3)
- Prayer waste: -0.2/tick praying with no threat (from v15.3)
- Wrong prayer: -1.0 per hit (unchanged)
- Combat idle: -0.5/tick after 2 ticks (unchanged)

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.25B, lr=0.001, anneal_lr=1.

Results (1.25B steps, ~10.2min, wandb run qg2ovzqp):
- SPS: 2.06M
- Wave reached: 16.19 avg (massive regression from v15.3's 28.36)
- Max wave: 31
- Most NPCs slayed: 119
- Episode return: 5,834 (inflated — reward hacking)
- Episode length: 6,479 (stalling)
- Score: 0
- Epochs: 1192
- Entropy: 5.90
- Value loss: 47.3 (unstable)

Analytics:
- prayer_uptime: 0.917 (92% praying)
- correct_prayer: 0.0 (NEVER blocks a hit correctly)
- dmg_taken_avg: 2,604
- pots_used: 31.9
- food_eaten: 20.0

Progression:
- 0-70M: Climbed to wave 19, then immediately collapsed to 15-16.
- 70M-1.25B: Stuck at wave 15-17 for entire run. Episode return
  inflated to 5800-8500 despite low wave — reward hacking.

Diagnosis — PRAYER FLICK FARMING:

The agent exploited the prayer flick reward (+0.5 per tick when
prayer toggled on without drain). It toggles prayer every tick to
farm +0.5 per tick while the prayer_waste penalty is only -0.2.
Net: +0.3/tick for doing nothing useful. Over 6000 ticks this
generates ~+1800 free reward, dominating wave_clear entirely.

correct_prayer=0.0 confirms the agent NEVER actually blocks a hit.
It just toggles prayer for the flick reward without matching the
correct style. The NPC-specific prayer bonus (+2.0) doesn't fire
because prayer never matches the attack style.

The prayer flick reward is fundamentally exploitable — any per-tick
reward for toggling prayer can be farmed without engaging in combat.
It must be removed. Prayer conservation should be taught through
prayer_waste penalty (-0.2) alone. Correct prayer usage through
the on-hit correct_prayer reward (+1.0).

The dangerous NPC melee range penalty (-0.5) and NPC-specific
prayer bonus (+2.0) were not the issue — they couldn't fire because
the agent wasn't engaging correctly to begin with.

---

## v15.3 (2026-04-05)

Changes from v15.2:

Two new reward signals to teach kiting and prayer discipline:

Kiting reward (+1.0 hardcoded):
- Fires when agent deals damage while target NPC is at distance 5-7
  (near max weapon range of 7). Only fires when attacking — no
  reward for just standing at range without engaging.
- Incentivizes ranged combat at distance. Melee NPCs can't reach the
  agent at range 5+, so the agent dodges melee damage entirely.
  Dual-mode NPCs (Tok-Xil, Ket-Zek) use their ranged/magic attacks
  at distance, forcing the agent to learn correct prayer switching
  because only ranged/magic hits land.

Unnecessary prayer penalty (-0.2/tick hardcoded):
- Fires when prayer is active but no NPC is within its attack range
  of the player AND no pending hits are in flight.
- Teaches the agent to turn prayer OFF when nothing is threatening,
  conserving prayer points. Combined with kiting, the agent learns:
  stay at range → melee NPCs out of range → turn prayer off → only
  pray when ranged/magic attack incoming → pray the correct type.

No curriculum — starts at wave 1. Kiting is most impactful in early
waves where all NPCs are melee. Loadout B, anneal_lr=1, 1.25B steps.

Hardcoded in fight_caves.h (new):
- Kiting reward: +1.0 when attacking from range 5-7
- Prayer waste: -0.2/tick when praying with no threat in range

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.25B, lr=0.001, anneal_lr=1.

Results (1.25B steps, ~10.6min, wandb run fjss9fzb):
- SPS: 1.88M
- Wave reached: 28.36 avg
- Max wave: 32
- Most NPCs slayed: 121
- Episode return: 5,491
- Episode length: 4,716
- Entropy: 5.87
- Value loss: 21.9

Analytics:
- prayer_uptime: 0.902
- correct_blocks: 2,922
- wrong_prayer_hits: 110.9 (first time non-zero — kiting working)
- pots_used: 32.0
- food_eaten: 20.0

Progression:
- 0-200M: Climb to wave 28.5.
- 200M-1.25B: Stable 27.9-28.8. No collapse.

Diagnosis: Kiting reward caused wrong_prayer to jump from 0 to 110.
Agent now faces ranged attacks at distance. Hasn't learned to switch
prayer yet but is experiencing the training signal for the first time.

---

## v15.2 (2026-04-05)

Changes from v15.1:

Curriculum only — same loadout and reward weights as v15.1.
- curriculum_wave: 0 → **30**
- curriculum_pct: 0 → **1.0** (100% of episodes start at wave 30)

ALL episodes start at wave 30. Skips 29 waves of melee-only content
the agent already handles perfectly. Forces immediate exposure to
Tok-Xil (ranged), Ket-Zek (magic), and mixed combat requiring prayer
switching — the exact content the agent has never learned.

```ini
[env]
# All weights identical to v15/v15.1
curriculum_wave = 30
curriculum_pct = 1.0

[train]
total_timesteps = 1_250_000_000
learning_rate = 0.001
anneal_lr = 1
```

Loadout: B (Masori (f) + Twisted Bow, 99/99/99/99)
Curriculum: 100% of episodes start at wave 30

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.25B, lr=0.001, anneal_lr=1.

Results (1.25B steps, ~9.7min, wandb run bsf4ecjl):
- SPS: 2.06M
- Wave reached: 30.00 avg (never clears wave 30)
- Max wave: 33 (occasionally breaks through)
- Most NPCs slayed: 7 (per episode avg — barely denting wave 30)
- Episode return: 756
- Episode length: 6,194 (very long — surviving but not killing)
- Score: 0
- Epochs: 1192
- Entropy: 6.14 (healthy)
- Clipfrac: 0.006 (LR annealed)
- Value loss: 14.3

Analytics:
- prayer_uptime: 0.968 (97%! praying almost every tick)
- correct_blocks: 3,203 per episode
- wrong_prayer_hits: **0.0** (still zero across entire run)
- pots_used: 29.2
- food_eaten: **0.0** (never eats — doesn't need to with 99 HP + prayer)

Progression:
- Plateau at wave 30.00 for entire run. Episode return stable 750-890.
- Agent survives ~6200 ticks at wave 30 then runs out of prayer and dies.
- Never learns to clear wave 30 consistently.

Diagnosis — agent camps melee prayer and tanks everything:

wrong_prayer_hits=0.0 across every single run (v13-v15.2) reveals
the fundamental problem: the agent has NEVER been hit while praying
the wrong style. It camps protect melee and everything that reaches
melee range attacks with melee — including Tok-Xil, which is a
dual-mode NPC that switches from ranged to melee at distance 1.

The agent's strategy: stand still, pray melee, let all NPCs walk
into melee range (where they all use melee), block everything, wait
until prayer runs out, die. It works for 6000+ ticks but can't
kill Yt-MejKot fast enough (800 HP + heals nearby NPCs).

Wave 30 spawns: 2x Yt-MejKot (melee, 800 HP, heals) + Tok-Xil
(ranged/melee dual). The agent should kite Tok-Xil at range and
pray ranged, switch to melee for MejKot. Instead it tanks all three
with melee prayer and slowly chips away.

Root cause — misleading observation for dual-mode NPCs:

The `npc_atk_style` observation shows the NPC's BASE style (ranged
for Tok-Xil), not what it's CURRENTLY attacking with (melee when at
range 1). The agent sees "ranged NPC" but gets hit with melee. This
contradicts the prayer matching logic — the observation says "pray
ranged" but melee prayer blocks the actual hit. The agent learns to
ignore `npc_atk_style` and just camp melee because it always works.

---

## v15.1 (2026-04-05)

Changes from v15:

Loadout change only — no reward or training parameter changes.
Switched from Loadout A (mid-level) to Loadout B (end-game) to test
how much player power affects wave progression.

Player stats changed:
- HP: 70 → **99** (990 tenths)
- Prayer: 43 → **99** (990 tenths)
- Defence: 70 → **99**
- Ranged: 70 → **99**

Equipment changed:
- Weapon: Rune Crossbow → **Twisted Bow**
- Armour: Black D'hide → **Masori (f) set**
- Ammo: Adamant Bolts → **Dragon Arrows**
- Added: Ava's assembler, Necklace of anguish, Zaryte vambraces,
  Pegasian boots, Venator ring
- Prayer bonus: 0 → **9** (prayer drains ~30% slower)

```ini
[env]
curriculum_wave = 0
curriculum_pct = 0.0

[train]
total_timesteps = 1_250_000_000
learning_rate = 0.001
anneal_lr = 1
```

Loadout: B (Masori (f) + Twisted Bow, 99/99/99/99)

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.25B, lr=0.001, anneal_lr=1.

Results (1.25B steps, ~10.2min, wandb run wj3p2wj9):
- SPS: 2.08M
- Wave reached: 28.65 avg, 29.2 peak
- Max wave: **32** (first time breaking wave 30)
- Most NPCs slayed: **121**
- Episode return: 5,003
- Episode length: 4,717
- Entropy: 6.07
- Value loss: 34.96

Analytics:
- prayer_uptime: 0.911
- correct_blocks: 2,721
- wrong_prayer_hits: 0.0
- pots_used: 31.9
- food_eaten: 20.0

Progression:
- 0-200M: Fast climb wave 17 → 28.9.
- 200M-1.25B: Stable plateau at 28.6-29.2. No collapse.

Diagnosis: End-game gear broke the wave 22 ceiling. Wave 21.7→29.2
(+7 waves) with zero reward changes. wrong_prayer_hits=0 — agent
still only prays melee, hasn't learned prayer switching.

---

## v15 (2026-04-05)

Changes from v14:

Anti-reward-hacking — reduce all dense signal magnitudes:
- v14's +20 correct prayer reward caused the agent to farm prayer
  blocks at easy melee waves (+43K/episode) rather than progress.
  All dense signals reduced to ±1.0 so wave_clear is the dominant signal.
- w_correct_danger_prayer: 20.0 → **1.0**
- w_damage_dealt: 2.0 → **1.0**
- w_npc_kill: 5.0 → **2.0**
- Wrong prayer: -5.0 → **-1.0** hardcoded
- Food timing: +50/-30 → **+1/-1** hardcoded (threshold unchanged at 70% HP)
- Pot timing: +50/-30 → **+1/-1** hardcoded, threshold changed from
  20% → **70%** prayer. Agent rewarded for drinking at or below 70%
  prayer, penalized above.
- All other weights unchanged from v14.

Principle: wave_clear (10 × wave_num) should be the loudest signal.
At wave 22: wave_clear = +2530 total. Prayer blocks at +1 each =
~+2000. Kills at +2 each = ~+100. No single dense signal dominates.

Training: 1.25B steps (shorter run for faster iteration).

```ini
[env]
w_damage_dealt = 1.0
w_damage_taken = -0.75
w_npc_kill = 2.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_correct_danger_prayer = 1.0
w_invalid_action = -0.1
w_tick_penalty = -0.001
curriculum_wave = 0
curriculum_pct = 0.0
disable_movement = 0

[train]
total_timesteps = 1_250_000_000
learning_rate = 0.001
anneal_lr = 1
```

Hardcoded in fight_caves.h:
- Wrong prayer: -1.0 per hit with wrong prayer active
- Wave clear: w_wave_clear × cleared_wave_number
- Combat idle: -0.5/tick after 2 ticks no damage dealt
- Food at/below 70% HP (pre-eat): +1.0, above 70%: -1.0
- Prayer pot at/below 70% prayer (pre-drink): +1.0, above 70%: -1.0

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.25B, lr=0.001, anneal_lr=1.

Results (1.25B steps, ~10.6min, wandb run f6z3abj1):
- SPS: 1.94M
- Wave reached: 21.65 avg (final), 21.8 peak
- Max wave: 26 (all-time best single episode)
- Most NPCs slayed: 94
- Episode return: 2,680 (healthy, not inflated)
- Episode length: 2,730
- Score: 0
- Epochs: 1192
- Entropy: 5.984 (healthy, slightly rising at end)
- Clipfrac: 0.006 (naturally decayed via LR annealing)
- Value loss: 3.52 (stable 3-4 entire run — best stability ever)
- Policy loss: -0.025

Analytics:
- prayer_uptime: 0.707 (71% of ticks praying)
- correct_blocks: 1,368 per episode
- wrong_prayer_hits: 0.0 (perfect prayer against melee waves)
- pots_used: 32.0 (all doses consumed)
- food_eaten: 20.0 (all sharks consumed)

Progression:
- 0-130M: Fast climb wave 9 → 20.6. Return 163 → 2224.
- 130M-1.25B: Gradual plateau at wave 21.4-21.8. Return slowly
  climbed from 2224 to 2680. Value loss stable at 3-4.6.
- **NO COLLAPSE.** First run without late-training instability.
  LR annealing worked — clipfrac naturally decayed from 0.036 to
  0.006 as LR approached 0. Entropy stayed healthy at 5.8-6.0.

Compared to v14: reward hacking eliminated. Episode return 2,680
vs v14's inflated 42,121. No wave regression — v14 collapsed from
21.8 to 13, v15 held at 21.7 for the entire run.

Compared to v13 (best prior peak): identical wave ceiling (~21.8)
but v15 is stable where v13 collapsed to 16. The reduced signal
magnitudes successfully prevented both reward hacking and late
instability.

Diagnosis — wave 22 ceiling:
The agent plateaus at wave 21-22 in every run (v13, v14, v15).
Wave 22 introduces Tok-Xil (ranged, lv90) alongside melee NPCs.
The agent has never learned to prayer-switch between melee and
ranged because it rarely survives past wave 21 to get exposure.
wrong_prayer_hits = 0 confirms the agent only faces melee content
and prays melee perfectly — it has zero experience with mixed
combat styles.

Food/pot consumption: still 32/20 per episode. With +1/-1 rewards,
the signal is too weak to change behavior. But consumption is now
at the correct thresholds (HP <=70%, prayer <=70%) — the agent
isn't wasting resources, it's just using them as fast as prayer
drains and combat damages HP. This is a resource economy issue,
not a reward shaping issue.

Key takeaway: reward shaping is now clean and stable. The wave 22
wall is a content exposure problem — the agent needs to see Tok-Xil
and Ket-Zek to learn prayer switching. Options for v16: curriculum
(expose agent to wave 22+ content), or longer training at 5B+ to
give more episodes to randomly break through.

---

## v14 (2026-04-05)

Changes from v13:

Prayer reward/penalty overhaul:
- Correct prayer: 1.0 → **20.0**. Massive positive reinforcement when
  the agent has the correct protection prayer active and a hit of that
  style resolves. Only fires when prayer is active at resolution time —
  toggling too late gives nothing. Uses config weight w_correct_danger_prayer.
- Wrong prayer: **NEW, -5.0 hardcoded**. When a hit resolves and the agent
  has a prayer active that does NOT match the attack style, -5.0 penalty.
  Fires even on 0-damage hits (misses). Previously there was no penalty
  for wrong prayer — the agent could pray missiles against melee-only
  NPCs with zero signal that anything was wrong. Now the agent gets
  explicit negative feedback for having the wrong prayer on.
- The +20/-5 asymmetry is intentional: correct prayer should be strongly
  rewarded to encourage the agent to learn which prayer matches which NPC,
  while wrong prayer gets a moderate penalty. The quadratic damage_taken
  penalty already heavily punishes unblocked hits on top of the -5.

Food and prayer pot — flat reward/penalty with pre-consumption check:
- Replaced old waste-scaled system with flat values. Reward function
  checks HP/prayer BEFORE the heal/restore is applied (using
  pre_eat_hp / pre_drink_prayer saved in fc_tick.c). Previous versions
  checked post-consumption values which made the pot reward impossible
  to trigger (pot restore always pushed prayer above 20% threshold).
- **Food at or below 70% HP (pre-eat): +50.0 reward.**
- **Food above 70% HP (pre-eat): -30.0 penalty.**
- **Prayer pot at or below 20% prayer (pre-drink): +50.0 reward.**
- **Prayer pot above 20% prayer (pre-drink): -30.0 penalty.**
- Note: PufferLib 4 CUDA kernel does NOT enforce action masks at the
  sampling level. Masks are observation features the policy learns
  from, not hard constraints. Standard masks (no food, on cooldown,
  full HP/prayer) are still set but are advisory only.

Hit source observation:
- NEW: player_hit_source (obs index 20). When an NPC hits the player,
  the agent now sees which NPC type dealt the damage. Normalized as
  npc_type / NPC_TYPE_COUNT. Lets the agent learn "Ket-Zek = magic,
  Tok-Xil = ranged" directly rather than inferring from pending attacks.

Training stability:
- anneal_lr: 0 → **1**. Linear LR decay from 0.001 to 0 over training.
  Every run with constant lr=0.001 has collapsed late (v10 at ~1B,
  v11 at ~1.2B, v13 at ~1.4B). Annealing gives aggressive early
  learning with stable late fine-tuning.
- total_timesteps: 1.5B → **5B**. Much longer run to take advantage of
  LR annealing and give the new prayer signals time to shape behavior.

No curriculum. No movement changes. Wave-scaled clear reward unchanged.

```ini
[env]
w_damage_dealt = 2.0
w_damage_taken = -0.75
w_npc_kill = 5.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_danger_prayer = 20.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_tick_penalty = -0.001
curriculum_wave = 0
curriculum_pct = 0.0
disable_movement = 0

[train]
total_timesteps = 5_000_000_000
learning_rate = 0.001
anneal_lr = 1
```

Hardcoded in fight_caves.h:
- Wrong prayer: -5.0 per hit with wrong prayer active
- Wave clear: w_wave_clear × cleared_wave_number
- Combat idle: -0.5/tick after 2 ticks no damage dealt
- Food at/below 70% HP (pre-eat): +50.0, above 70%: -30.0
- Prayer pot at/below 20% prayer (pre-drink): +50.0, above 20%: -30.0

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=5B, lr=0.001, anneal_lr=1.

Results (5B steps, ~41.5min, wandb run tutxtn7x):
- SPS: 1.94M
- Wave reached: 13.03 avg (final), 21.8 peak (at ~1B steps)
- Max wave: 26 (all-time best single episode)
- Most NPCs slayed: 94
- Episode return: 42,121 (final — massively inflated)
- Episode length: 3,436 (stable, no stalling)
- Score: 0
- Epochs: 4768
- Entropy: 5.287 (healthy)
- Clipfrac: 0.097 (collapsed to near-zero by end — LR annealed out)
- Value loss: 48.4 (unstable at end)
- Policy loss: -0.009

Analytics:
- prayer_uptime: 0.762 (praying 76% of ticks)
- correct_blocks: 2,167 per episode
- wrong_prayer_hits: 0.0 (never has wrong prayer when hit)
- pots_used: 32.0 (all doses consumed every episode)
- food_eaten: 20.0 (all sharks consumed every episode)

Progression:
- 0-1B: Climbed wave 9 → 21.8. Return 26K-30K.
- ~1.2B: Collapsed wave 21.8 → 13.0. Return INCREASED to 41K-43K.
- 1.5B-5B: Stuck at wave 13.0 for 3.5B steps. Never recovered.

Diagnosis — REWARD HACKING via prayer blocks:

The agent earns MORE reward at wave 13 than at wave 22. The +20.0
correct prayer reward completely dominates all other signals:
- correct_blocks per episode: 2,167 × +20.0 = **+43,340**
- wave_clear rewards (waves 1-13): 10+20+...+130 = **+910**
- Prayer blocks = **47x** more valuable than wave progression

Waves 1-13 are all melee NPCs. The agent prays protect melee, blocks
every hit (+20 each), and has zero incentive to push to wave 14+ where
Tok-Xil (ranged) would require risky prayer switching. Farming melee
prayer blocks at easy waves is overwhelmingly more profitable than
progressing.

The +50 food/pot rewards also contribute to inflation (eating 20
sharks at +50 each = +1000, drinking 32 doses at +50 each = +1600)
but prayer blocks are the dominant factor.

LR annealing (0.001 → 0) did NOT prevent the collapse — it happened
at ~1.2B when LR was still 0.00075. Annealing did prevent the late
value-loss explosion seen in v13 (value loss stable at 5-11 for most
of training vs v13's 3→11 spike). But by the end (LR≈0), clipfrac
dropped to 0.097 and the policy stopped updating entirely.

Lesson: any dense per-hit reward that fires thousands of times per
episode will dominate sparse event rewards (wave clear, kills). The
reward magnitude must be proportional to rarity: frequent signals
need small weights, rare milestones need large weights.

---

## v13 (2026-04-05)

Changes from v12:

Wave clear reward scaling:
- Wave clear now scales with wave number: reward = w_wave_clear × wave.
  Wave 1 clear = 10. Wave 15 = 150. Wave 30 = 300. Wave 63 = 630.
  Previously a flat 10.0 regardless of wave. The flat reward meant the
  agent earned the same +10 clearing wave 1 as wave 30, so farming easy
  waves (fewer per-tick penalties, simpler combat) was equally profitable.
  Scaling makes pushing to higher waves overwhelmingly more valuable than
  any per-tick exploit.

  v12 analysis showed the agent earned ~600 episode_return at both wave 22
  (before collapse) and wave 13 (after collapse). Per-tick rewards (combat
  idle, prayer, damage) accumulated over longer episodes at easy waves and
  equaled the wave-clear bonuses from harder waves. Scaling breaks this:
  clearing waves 1-22 gives 10+20+...+220 = 2530 total. Clearing waves
  1-13 gives only 10+20+...+130 = 910 total. The agent can't earn more
  by farming easy waves.

All other config unchanged from v12. Movement enabled, no curriculum,
anneal_lr=0, combat idle -0.5 after 2 ticks.

```ini
[env]
# unchanged from v12, wave_clear scaling is in code not config
w_wave_clear = 10.0

[train]
total_timesteps = 1_500_000_000
```

Hardcoded in fight_caves.h:
- Wave clear: w_wave_clear × cleared_wave_number (scales with progression)
- Combat idle: -0.5/tick after 2 ticks no damage dealt
- Food timing: +1.0 for eating with 20+ HP missing
- Prayer pot below 20%: +2.0, above 80%: -3.0

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.5B, lr=0.001, anneal_lr=0.

Results (1.5B steps, ~12.5min, wandb run 0pwza2dq):
- SPS: 1.94M
- Wave reached: 16.01 avg (final), 21.8 peak (at ~1428M steps)
- Max wave: 26 (all-time best single episode)
- Most NPCs slayed: 94
- Episode return: 2040.5 (final), 2846 peak (at ~1428M)
- Episode length: 2737 (stable throughout, no stalling)
- Score: 0
- Epochs: 1430
- Entropy: 4.915 (final — dropped from ~5.8 plateau)
- Clipfrac: 0.275 (elevated at end)
- Value loss: 11.26 (spiked at end — was ~3 for most of training)
- Policy loss: 0.023
- KL: 0.022

Progression:
- 0-80M: Fast climb, wave 9 → 19.5. Agent learns basic combat quickly.
- 80M-1430M: Plateaued at wave 21-22 for the entire training run.
  Wave oscillated 21.1-21.8, return slowly climbed 2224 → 2846.
  Agent never broke past wave 22 consistently. Entropy stable at
  5.7-5.8, value loss stable at 3-4, clipfrac ~0.22. Training was
  healthy but the agent could not progress.
- 1430M-1500M: Late collapse. Wave dropped 21.8 → 16.0. Value loss
  spiked 3.0 → 11.3. Entropy dropped 5.7 → 4.9. Clipfrac rose to
  0.27. This is a catastrophic policy update — the value function
  diverged, bad advantages were computed, and a large destructive
  policy update destroyed learned behavior.

Compared to v11 (best prior run, an75g72f):
- Wave: 21.8 peak → **29.23** (v13 is 8 waves worse at peak)
- Return: 2846 peak → 2045 (comparable)
- Entropy: 5.7-5.8 → 5.56 (similar)
- Value loss: 3-4 → 28.1 (v13 much healthier until collapse)
- Episode length: 2700-2900 → 2973 (similar, no stalling in either)

Diagnosis — why v13 regressed 8 waves from v11:

1. **No curriculum is the primary cause.** v11 used curriculum_wave=28,
   curriculum_pct=0.5 — half of all episodes started at wave 28, so the
   agent regularly faced Tok-Xil+Yt-MejKot combos and occasionally saw
   Ket-Zek (wave 31+). In v13, the agent starts wave 1 every episode and
   must grind through 21 waves of easy melee-only content before seeing
   anything challenging. It never gets enough exposure to hard content
   to learn how to handle it.

2. **Movement adds complexity.** v11 disabled movement entirely. v13
   re-enabled it. With 17 move actions available every tick, the agent
   has much more to explore. The combination of movement + no curriculum
   means the agent has a harder problem (larger action space) and less
   exposure to the hard content that would teach it to use prayer
   switching and kiting effectively.

3. **Wave-scaled rewards may be underweighting early progress.** With
   scaling, clearing waves 1-10 gives 10+20+...+100 = 550 total. With
   flat 10.0 (v11), waves 1-10 gave 100 total. The scaled reward
   makes early waves worth MORE, but the agent can't distinguish "I
   should push harder" from "I'm already getting good reward." The
   flat reward in v11 was simpler and the agent found it equally
   worth pushing to wave 29 as wave 10 — the only difference was
   duration. This interacts with curriculum: with curriculum, the
   agent saw waves 28-31 often enough that the high wave-clear
   rewards there (~300-310) dominated. Without curriculum, the agent
   never sees those rewards.

4. **Late collapse from constant LR.** anneal_lr=0 with lr=0.001 is
   too aggressive for late training. Same failure mode as v10 and v11
   late instability. At 1430M steps the policy was near-converged at
   wave 21.8, and a single bad batch caused value loss to spike 3→11
   and wave to drop to 16. LR decay or a lower constant LR (0.0003)
   would reduce this risk.

Recommendations for v14:
- Re-enable curriculum: curriculum_wave=30, curriculum_pct=0.5. This
  was the single biggest factor in v11's success. The agent needs
  regular exposure to Ket-Zek (magic, wave 31+) and Tok-Xil+MejKot
  combos to learn prayer switching and target priority.
- Consider anneal_lr=1 or reducing LR to 0.0003. Every run with
  constant 0.001 eventually collapses (v10, v11, v13). The late
  instability is a recurring pattern.
- Keep movement enabled. v11 hit wave 29 without movement but couldn't
  push past it. Movement is needed for kiting Yt-MejKot and dodging
  multi-NPC combos in waves 30+. The solution to movement complexity
  is curriculum exposure, not disabling movement.
- Keep wave-scaled rewards. The concept is sound (prevent farming easy
  waves) but it needs curriculum to expose the agent to the high-value
  wave clears that make scaling worthwhile.

---

## v12 (2026-04-04)

Changes from v11:

Movement:
- RE-ENABLED. disable_movement=0. Full action space restored. v10-v11
  disabled movement to isolate combat learning, but without movement the
  agent couldn't get past wave 14-15 (Yt-MejKot with 800HP + healing).
  Movement is part of how the agent survives (kiting, repositioning).

Curriculum:
- Disabled (wave 0). Agent starts wave 1 every episode.

Pre-impact prayer window:
- REMOVED. The +0.5 per pending hit per tick within 3-tick window was
  being EXPLOITED by the agent. With multiple NPCs attacking, correct
  prayer generated +1.5-2.0/tick of free reward — 10x more than combat
  rewards (damage_dealt ≈ +0.05/tick). The agent learned to just turn
  prayer on and stop fighting, collecting prayer reward passively.
  This caused the deterministic collapse at ~40M steps seen in 3
  consecutive runs: agent rapidly reaches wave 20+, discovers the prayer
  exploit, commits to it, stops killing NPCs, waves stall, dies.
  The correct-prayer-at-resolve reward (+1.0 per hit) remains and is
  sufficient — it fires once per hit, not per tick, so can't be farmed.

Combat idle punishment:
- NEW. -0.5 per tick when agent hasn't dealt damage in 2+ ticks and
  NPCs are alive. Forces the agent to engage in combat instead of
  standing idle with prayer on. Counter resets when damage is dealt.
  Tracked via ticks_since_attack in FightCaves struct.

New metrics:
- max_wave: highest wave any single episode reached (all-time max).
- most_npcs_slayed: highest NPC kill count in any single episode.
  Both tracked as globals, visible in wandb User Stats.

```ini
[env]
w_damage_dealt = 2.0
w_damage_taken = -0.75
w_npc_kill = 5.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_tick_penalty = -0.001
curriculum_wave = 0
curriculum_pct = 0.0
disable_movement = 0

[train]
total_timesteps = 1_500_000_000
learning_rate = 0.001
anneal_lr = 0
```

Hardcoded rewards in fight_caves.h (not in config):
- food_used_well: +1.0 for eating with 20+ HP missing
- prayer_pot below 20%: +2.0 for drinking when low
- prayer_pot above 80%: -3.0 extra for wasteful drinking
- combat idle: -0.5/tick after 2 ticks of no damage dealt

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.5B, lr=0.001, anneal_lr=0.

Results: not run — v12 config was superseded by v13 before training.

---

## v11 (2026-04-04)

Changes from v10 (based on policy replay of v10 peak + collapse):

Prayer correctness:
- Correct prayer reward now fires for ALL attack styles (melee, ranged,
  magic), not just ranged/magic. In v9/v10 the agent had zero signal for
  melee prayer in waves 1-28 (all melee). It randomly switched between
  protect ranged and magic because those were the only styles that gave
  reward. Now protect melee is rewarded in early waves.
- Pre-impact window (+0.5) also fires for ALL styles, not just ranged/magic.

Prayer drain cost:
- REMOVED entirely. The -0.01/tick cost from v9/v10 taught the agent to
  never pray (v9: 67K tick stalling, v10: contributed to value loss
  explosion). Prayer conservation now handled by potion timing reward.

Prayer potion timing:
- Below 20% prayer: +2.0 reward for drinking (was +1.0 in v10)
- Above 80% prayer: -3.0 extra penalty for drinking (on top of existing
  waste scaling). Agent punished for wasteful chugging.
- Between 20-80%: only the waste scaling applies (base -1.0)

No wrong-prayer penalty: kept at 0 (removed since v9). damage_taken
quadratic penalty handles this implicitly — wrong prayer = full damage =
harsh penalty proportional to hit size.

Movement: still disabled (disable_movement=1).
anneal_lr: still 0 (constant LR).
All config weights unchanged from v10.

Code changes (fight_caves.h):
- fc_puffer_compute_reward: prayer correctness checks ATTACK_NONE instead
  of ATTACK_RANGED||ATTACK_MAGIC. Pre-impact window same change.
- Removed prayer drain cost block.
- Prayer potion timing: +2.0 below 20%, -3.0 above 80%.

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 2.0
w_damage_taken = -0.75
w_npc_kill = 5.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
curriculum_wave = 28
curriculum_pct = 0.5
disable_movement = 1

[train]
total_timesteps = 1_250_000_000
learning_rate = 0.001
anneal_lr = 0
```

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=1.25B, lr=0.001, anneal_lr=0.

Results (1.25B steps, ~10.1min, wandb run an75g72f):
- SPS: 2.0M
- Wave reached: 29.23 avg
- Max wave: N/A (metric added after this run started)
- Most NPCs slayed: N/A (metric added after this run started)
- Episode length: 2973 (stable, not stalling)
- Episode return: 2045 (up from v10's 993)
- Score: 0
- Epochs: 1192
- Entropy: 5.56 (healthy — not collapsing, not random)
- Clipfrac: 0.310 (high but stable — not diverging like v10)
- Value loss: 28.1 (healthy, similar to v7's 31.7)
- Policy loss: 0.124
- KL: 0.029

Progression notes: Agent hit wave 30.0 by 67M steps and stayed near 30
for most of training. Episode return climbed steadily from 747 to 2658
at 889M steps. Then a late dip at 1187M (value_loss spiked to 154,
return dropped to 497) before recovering to 2045. The late instability
may be from anneal_lr=0 keeping LR too high (0.001) when policy should
be fine-tuning.

Compared to v10:
- Wave: 28.74 → 29.23 (improved, though not 30.0)
- Episode return: 993 → 2045 (2x improvement)
- Value loss: 325 → 28 (12x better — stable training)
- Entropy: 3.13 → 5.56 (much healthier, not over-converging)
- Episode length: 1732 → 2973 (surviving longer)

Key improvements from v11 changes:
- Removing prayer drain cost eliminated the v9/v10 death spiral.
- Correct prayer reward for ALL styles (including melee) gave the agent
  a learning signal in early waves. Previously had zero signal for the
  first 28 waves of melee.
- Prayer potion timing (+2 below 20%, -3 above 80%) should help with
  pot conservation, though replay needed to verify.

Still stuck at wave 30 wall. Agent dies during wave 30 (Yt-MejKot +
Tok-Xil + 2× Tz-Kek = 6+ simultaneous NPCs). Has never seen Ket-Zek
(wave 31+). Consider curriculum at wave 30 or 31 to expose agent to
harder content.

Late training instability (1187M spike) suggests anneal_lr=0 with
lr=0.001 is too aggressive for late fine-tuning. Consider anneal_lr=1
with lower starting LR, or reducing LR to 0.0003.

---

## v10 (2026-04-04)

Changes from v8 (based on v8 regression analysis + policy replay):

Philosophy shift: stop punishing wrong prayer explicitly. Let damage_taken
handle it implicitly. Use positive reinforcement and natural costs instead.

Reward changes:
- w_correct_danger_prayer: 2.0 → 1.0, now RANGED/MAGIC ONLY. Positive
  reinforcement when correct prayer blocks a dangerous hit. Melee excluded.
- w_wrong_danger_prayer: -5.0 → 0.0. Removed entirely. The quadratic
  damage_taken penalty already punishes wrong prayer (wrong prayer = full
  damage = big penalty). Explicit punishment created too much noise in v8.
- Prayer drain cost: NEW, hardcoded -0.01/tick when any prayer is active.
  Incentivizes turning prayer OFF when not needed. Over 100 ticks of
  leaving prayer on = -1.0. Pushes toward prayer flicking (on before hit,
  off after). Teaches prayer conservation without prayer potions.
- Prayer potion timing: NEW, +1.0 for drinking when prayer below 20%
  (86/430 tenths). Combined with existing waste scaling, agent learns to
  drink only when actually low.

Code changes:
- fight_caves.h: prayer correctness check restricted to ranged/magic only,
  removed wrong-prayer branch, added per-tick prayer drain cost, added
  prayer potion timing bonus.

All other weights unchanged from v8:
- w_damage_dealt=2.0, w_npc_kill=5.0, w_food_used=-0.5, w_prayer_pot=-1.0
- w_damage_taken=-0.75 (quadratic), w_player_death=-20.0

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 2.0
w_damage_taken = -0.75
w_npc_kill = 5.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
curriculum_wave = 28
curriculum_pct = 0.5

[vec]
total_agents = 4096
num_buffers = 1

[train]
total_timesteps = 1_250_000_000
learning_rate = 0.001
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Results (1.25B steps, ~10.6min, wandb run njxrz8l2):
- SPS: 1.9M
- Wave reached: 29.81 avg (recovered from v8's 28.05, close to v7's 29.96)
- Episode length: 67505 (STALLING AGAIN — back to v6.1 behavior)
- Episode return: 1158
- Score: 0
- Epochs: 1192
- Entropy: 8.16 (shot UP — policy became near-random)
- Clipfrac: 0.0002 (policy stopped updating entirely)
- Value loss: 20.8 (healthy, reward signal stable)
- Policy loss: -0.017
- KL: 0.0004

Diagnosis: Removing wrong-prayer penalty fixed the instability from v8
(value loss back to healthy 20 vs 222). But the -0.01/tick prayer drain
cost taught the agent to turn prayer OFF entirely. No prayer = no drain
cost. Agent found a strategy to avoid damage without prayer (safespotting/
kiting) and converged to it. Clipfrac dropped to 0 — policy stopped
learning. Entropy rose to 8.16 (near-random) because the policy gave up
on combat actions. Episode length 67505 = stalling again like v6.1.

Lesson: per-tick prayer cost incentivizes no-prayer-ever, not prayer
flicking. Need positive incentives for correct prayer usage, not costs
for having prayer on.

---

## v10 (2026-04-04)

Changes from v9:

Observation improvements:
- Added 2 new per-NPC features (NPC_STRIDE 13→15, OBS_SIZE 171→187):
  - npcN_pending_attack_style: attack style of projectile in flight from
    this NPC (0=none, 0.33=melee, 0.67=ranged, 1.0=magic)
  - npcN_pending_attack_ticks: ticks until that hit resolves (normalized /10)
  Agent now sees "Ket-Zek magic attack landing in 2 ticks" and can switch
  prayer preemptively.

Reward changes:
- Pre-impact prayer window: NEW, +0.5 per tick when correct prayer is
  active within 3 ticks of a ranged/magic hit resolving. Rewards having
  correct prayer up BEFORE impact.
- Prayer drain cost: kept at -0.01/tick (from v9).
- All other weights unchanged from v9.

Training changes:
- anneal_lr: 1 → 0. Constant learning rate (0.001) throughout training.
  Prevents late-training stalling from LR decay.
- disable_movement: NEW flag = 1. Agent's movement action forced to idle.
  Cannot walk or run. NPCs come to the agent. Forces learning of combat
  actions (prayer, attack, eat, drink) without movement as a crutch.

```ini
[env]
w_damage_dealt = 2.0
w_damage_taken = -0.75
w_npc_kill = 5.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_danger_prayer = 1.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
curriculum_wave = 28
curriculum_pct = 0.5
disable_movement = 1

[train]
anneal_lr = 0
total_timesteps = 1_250_000_000
```

Results (1.25B steps, ~10.8min, wandb run m8nr3tnt):
- SPS: 1.8M
- Wave reached: 28.74 avg (down from v9's 29.81)
- Episode length: 1732 (SHORT — agent dying fast without movement)
- Episode return: 993
- Score: 0
- Epochs: 1192
- Entropy: 3.13 (very low — agent converged hard)
- Clipfrac: 0.414 (VERY HIGH — policy thrashing)
- Value loss: 325 (CATASTROPHIC — worse than v8's 222)
- Policy loss: 0.009
- KL: 0.158 (extremely high — policy diverging)

Progression notes: Agent peaked at wave 30 and ep_return ~2400 around
500M-700M steps. Then value_loss exploded (11→325) and the agent
collapsed. Wave dropped 30→28.7, episode length 3000→1732. The policy
destabilized in the second half of training.

Diagnosis: Two problems compounded:
1. Without movement, the agent takes more damage (can't kite/dodge).
   This amplifies the quadratic damage_taken penalty, making rewards
   more volatile → value_loss explosion.
2. The -0.01/tick prayer drain cost is STILL causing problems. With no
   movement, prayer is more important (can't avoid hits). But the drain
   cost pushes the agent to turn prayer off, causing more damage, which
   causes more penalty, creating a death spiral.
3. anneal_lr=0 kept the LR at 0.001 throughout, which may have caused
   the late-training instability (policy kept making large updates even
   as it should have been fine-tuning).

---

## v9 (2026-04-04)

Changes from v7 (based on policy replay observations):

Prayer fix:
- Prayer correctness now fires for ALL attack types including melee
  (v7 only checked ranged/magic). Fires on zero-damage hits too.
- No prayer active when hit = same punishment as wrong prayer.
- w_correct_danger_prayer: 2.0 (unchanged)
- w_wrong_danger_prayer: 0.0 → -5.0 (harsh punishment for wrong prayer)

Combat incentive:
- w_damage_dealt: 0.5 → 2.0 (4x stronger incentive to attack)
- w_npc_kill: 3.0 → 5.0 (bigger kill reward)

```ini
[env]
w_damage_dealt = 2.0
w_damage_taken = -0.75
w_npc_kill = 5.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_danger_prayer = 2.0
w_wrong_danger_prayer = -5.0
w_invalid_action = -0.1
w_idle = 0.0
w_tick_penalty = -0.001
curriculum_wave = 28
curriculum_pct = 0.5
```

Hyperparameters: gamma=0.999, gae_lambda=0.95, clip_coef=0.2,
vf_coef=0.5, ent_coef=0.02, max_grad_norm=0.5, horizon=256,
minibatch_size=4096, total_agents=4096, hidden_size=256,
num_layers=2. total_timesteps=2.5B, lr=0.001, anneal_lr=1.

Results (2.5B steps, ~23min, wandb run w8nsir07):
- SPS: 1.7M (down from v7's 2.0M)
- Wave reached: 28.05 avg (REGRESSED from v7's 29.96 — lost 2 waves)
- Episode length: 2615 (down from v7's 5438 — dying 2x faster)
- Episode return: 2564 (down from v7's 3421)
- Score: 0
- Epochs: 2384
- Entropy: 4.56 (lower than v7's 5.3)
- Clipfrac: 0.280 (4.5x v7's 0.061 — UNSTABLE, policy thrashing)
- Value loss: 222.6 (7x v7's 31.7 — CATASTROPHIC)
- Policy loss: -0.053
- KL: 0.032

Diagnosis: w_wrong_danger_prayer=-5.0 destroyed training. In waves 1-28,
melee NPCs attack every 4 ticks. Each hit with wrong prayer = -5.0.
Over a 3000-tick episode that's ~3750 in prayer penalties, drowning out
all other rewards. Value loss exploded 7x, clipfrac hit 0.28 (28% of
updates clipped). Agent regressed 2 waves and learned nothing useful.

Lesson: don't apply equal punishment for all prayer mismatches. Melee
attacks are frequent and low-damage. Punishing them at -5.0 per hit
overwhelms the reward signal. Let damage_taken handle it implicitly.

---

## v7 (2026-04-04)

Changes from v6/v6.1 (based on policy replay observations):

Critical bug fix:
- Collision map was never loaded during training. PufferLib runs from
  pufferlib_4/ directory where none of the search paths in fc_state.c
  resolved, so setup_arena() fell back to an open arena (all tiles
  walkable, border walls only). ALL v1-v6.1 runs trained without cave
  walls. Agent walked through terrain because walls didn't exist in
  training. Fixed by copying fightcaves.collision to pufferlib_4/assets/.

Reward shaping changes:
- w_food_used: -0.01 → -0.5. Waste scaling (overheal fraction × 9)
  now produces meaningful punishment. Eating at 1 HP missing = -4.8.
  Eating at 20+ HP missing = -0.5 (acceptable base cost).
- w_food_used_well: NEW, +1.0. Positive reward for eating when 20+ HP
  missing (200+ tenths). Net reward for well-timed eat = +0.5.
  Agent learns to eat when actually hurt, not for chip damage.
- w_prayer_pot_used: -0.01 → -1.0. Same waste scaling logic. Drinking
  at 1 prayer missing = -9.5. Drinking at 17+ missing = -1.0.
- w_damage_taken: -0.75 (unchanged). Quadratic scaling kept — agent must
  learn that big hits are catastrophic. This is damage taken, not HP
  missing — directly punishes taking hits from hard-hitting NPCs.
- w_correct_danger_prayer: 0.0 → 2.0. NEW reward feature. Fires per-hit
  when correct prayer blocks a ranged/magic attack from ANY NPC.
- w_wrong_danger_prayer: 0.0 → -2.0. NEW reward feature. Fires per-hit
  when wrong/no prayer active and hit by ranged/magic. Symmetrical with
  correct prayer (+2/-2). Agent needs this signal to learn prayer switching.

Backend changes:
- Added correct_danger_prayer / wrong_danger_prayer flags to FcState
- Tracking in fc_combat.c: fires when ranged/magic hit resolves, checks
  if active prayer matches attack style. Per-hit, not per-tick.
- FC_RWD_CORRECT_DANGER_PRAY (16), FC_RWD_WRONG_DANGER_PRAY (17)
- FC_REWARD_FEATURES: 16 → 18
- w_food_used_well field added to FightCaves struct + binding.c

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.75
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.5
w_food_used_well = 1.0
w_prayer_pot_used = -1.0
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 2.0
w_wrong_danger_prayer = -2.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
curriculum_wave = 28
curriculum_pct = 0.5

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 2_500_000_000
learning_rate = 0.001
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Results (2.5B steps, ~21min, wandb run er56r0lu):
- SPS: 2.0M (down from v6's 2.3M — collision-enabled pathfinding overhead)
- Wave reached: 29.96 avg (same wave 30 wall as v6)
- Episode length: 5438 ticks (down from v6.1's 200K — agent dying, not stalling)
- Episode return: 3421 (up from v6's 37.7 — new rewards providing dense signal)
- Score: 0 (no cave completions)
- Epochs: 2384
- Entropy: 8.3 → 5.3 (converging, possibly too fast)
- Clipfrac: 0.061 (healthy — policy updating)
- Value loss: 31.7 (very high — reward variance too large for value network)
- Policy loss: -0.028
- KL: 0.005

Key observations:
- Agent no longer stalls indefinitely. Episode length 5438 means it engages
  and dies, unlike v6.1's 200K-tick survival strategy. The consumable waste
  penalties are working — agent isn't chugging pots/food mindlessly.
- Collision is loaded and working for the first time. No more open arena.
- Wave 30 wall persists. Reaches ~29.8 within first 200M steps, then flat.
  Ket-Zek (magic+melee, size 5) still the blocker. Agent may not be
  learning to prayer switch despite the new prayer rewards.
- Value loss 31.7 is concerning — means reward signal has high variance.
  The value network can't predict expected returns, which hurts GAE
  advantage estimation and slows learning.
- Entropy dropped to 5.3 from 8.3 — agent may be converging prematurely.

Next: replay v7 policy to observe prayer switching behavior, consumable
usage, and movement with real collision. Compare to v6 replay.

---

## v6.1 (2026-04-04) — Tick cap experiment (archived)

Same config as v6 except FC_MAX_EPISODE_TICKS: 30000 → 200000.
See v6.1 section below for full results.

---

## v6 (2026-04-04) (archived)

Changes from v5:

Observation improvements (agent gets better info to make decisions):
- Player slot 19: hit_style_this_tick — WHAT attack style hit the agent
  this tick (0=none, 0.33=melee, 0.67=ranged, 1.0=magic). Previously
  the agent only knew damage amount, not type. Now it can directly
  correlate "hit by magic → should pray magic."
- Player slot 20: attack_target — which NPC slot the agent is currently
  targeting (0=none, 0.125-1.0=slot 0-7). Agent can now track its own
  target for decision-making about switching targets.
- NPC slot 12: LOS — line of sight from player to NPC (1=clear, 0=blocked).
  Critical for safespotting strategies. Agent can learn to break LOS
  behind terrain to avoid ranged/magic attacks.
- FC_OBS_PLAYER_SIZE: 20 → 21. NPC_STRIDE: 12 → 13.
  FC_POLICY_OBS_SIZE: 126 → 135. FC_PUFFER_OBS_SIZE: 162 → 171.

Reward improvements:
- Damage taken penalty now scales quadratically — big hits hurt
  disproportionately more than small ones:
    1 HP: penalty ≈ -0.0002 (barely noticeable, chip damage is ok)
    10 HP: penalty ≈ -1.1 (moderate, should eat soon)
    20 HP: penalty ≈ -4.3 (harsh, should have been praying)
    50 HP: penalty ≈ -26.8 (catastrophic, Jad without prayer)
  This teaches the agent that praying against big hits matters far
  more than avoiding small melee pokes.
- All explicit prayer rewards remain at 0 (organic learning).

Curriculum learning:
- 50% of episodes start at wave 28 (where agent was dying in v5).
  Gives the agent more practice on Ket-Zek+ waves instead of
  spending most training time on easy waves 1-20.
- Remaining 50% start wave 1 to maintain full-game learning.
- Player restored to full HP/prayer at curriculum start wave.
- New config: curriculum_wave=28, curriculum_pct=0.5.

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.75
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.01
w_prayer_pot_used = -0.01
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 0.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001
curriculum_wave = 28
curriculum_pct = 0.5

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 5_000_000_000
learning_rate = 0.001
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

Results (2.5B/5B steps, stopped early — wave 30 wall, wandb run cz6dyc3p):
- SPS: 2.3M
- Wave reached: 30.0 avg (same wall as v5, reached immediately)
- Episode length: 30001 ticks (hit tick cap — agent surviving but stuck)
- Episode return: 37.7 (dropped massively from v5's 636 — quadratic penalty)
- Score: 0 (no cave completions)
- Params: 446.5K (slightly more from new obs features)
- Epochs: 2374
- Entropy: 7.15 (higher than v5's 6.57 — more exploration)
- Clipfrac: 0.051 (finally non-zero! policy actually updating)
- Value loss: 0.001 (very stable)
- Policy loss: -0.008
- KL: 0.005
- Old KL: 0.006
- Uptime: 18m 42s

Diagnosis: New observations (hit style, LOS, target) and curriculum at
wave 28 didn't break the wave 30 wall. Agent reaches wave 30 very quickly
(within first few epochs) but can't progress further. Episode length hit
30001 (tick cap) meaning agent survives indefinitely but can't clear the
wave — possibly stuck idle or praying without attacking. Episode return
dropped from 636→38 due to quadratic damage penalty and tick penalty
accumulating over 30K ticks. Clipfrac finally non-zero (0.051) which is
healthy — policy is actually updating now unlike v1-v5.

Key insight: episode_length = 30001 = FC_MAX_EPISODE_TICKS + 1. The agent
is hitting the tick cap, meaning it's surviving but not clearing waves.
It may have learned to just stand still and pray, avoiding damage but not
fighting. Need to visually observe agent behavior.

Notable from wandb config dump:
- Network: MinGRU (PufferLib default, not plain MLP — has recurrent state)
- Encoder/Decoder: DefaultEncoder/DefaultDecoder
- anneal_lr: 1 (LR annealing is ON — LR decreases over training)
- replay_ratio: 1.0, vtrace enabled (V-trace importance sampling active)
- cudagraphs: 10 (CUDA graph optimization enabled)
- eval_episodes: 10000 (large eval batch)
- checkpoint_interval: 200 epochs

Interesting observations:
- MinGRU gives the agent memory across ticks — it can remember what
  happened several ticks ago (e.g., "Ket-Zek attacked with magic 3 ticks
  ago, I should still be praying magic"). This is better than a stateless
  MLP for prayer switching.
- anneal_lr=1 means the LR of 0.001 decreases toward 0 over the 5B steps.
  By 2.5B steps it's already at 0.0005. This could explain why the agent
  stops improving — LR becomes too small. Consider setting anneal_lr=0.
- Episode length hitting 30001 (tick cap) is suspicious. With curriculum
  starting 50% of episodes at wave 28, the agent may have learned to just
  survive at wave 28-30 without progressing (no deaths = no tick penalty
  accumulation worth worrying about at -0.001/tick).

Next steps: implement policy replay in fc-viewer viewer to visually observe
what the agent is actually doing at wave 30.

---

## v6.1 (2026-04-04) — Tick cap experiment

Same config as v6 except:
- FC_MAX_EPISODE_TICKS: 30000 → 200000 (force prayer pot drain)
- total_timesteps: 5B → 2B (quick test)
- Hypothesis: agent is hiding behind prayer, hitting 30K tick cap.
  200K ticks should drain all 32 prayer pot doses and expose whether
  the agent actually learned to fight.

Results (2.0B steps, ~14m38s, wandb run ss966rf9):
- SPS: 2.3M
- Wave reached: 30.0 avg (still wave 30 wall — unchanged)
- Episode length: 199,939 ticks (HITTING 200K CAP! agent still not dying!)
- Episode return: -135.1 (NEGATIVE — tick penalty accumulated over 200K ticks)
- Score: 0 (no cave completions)
- Episodes completed: 4,239 (far fewer eps — each one is 6.7x longer)
- Entropy: 6.90
- Clipfrac: 0.001 (barely updating)
- Value loss: 0.0002 (extremely stable — agent found a fixed strategy)
- Policy loss: -0.006
- KL: 0.0007
- VRAM: 3.1/15G (higher than v6's 1.9G — longer episodes need more memory)

KEY FINDINGS:
1. Agent survives 200K ticks (33+ hours of game time) without dying.
   Even after all prayer pots are drained, it still doesn't die. This means
   the agent learned something beyond just "pray and idle":
   - It may be safespotting (breaking LOS behind terrain so NPCs can't attack)
   - It may be kiting (running away from melee NPCs, staying out of range)
   - It may be exploiting a game mechanic (NPC deaggro? stuck pathfinding?)

2. Episode return is NEGATIVE (-135) because tick_penalty × 200K ticks = -200.
   The agent's reward from damage/kills doesn't outweigh the tick penalty over
   such a long episode. This is actually a problem — the agent learned that
   surviving (even at negative reward) is better than dying quickly.

3. Wave 30 is the hard ceiling. Even with infinite time, the agent can't
   clear it. This isn't a resource issue — it's a strategy/capability issue.

4. Value loss near 0 (0.0002) means the agent has perfectly predicted its
   expected return — it knows it will survive indefinitely at wave 30 with
   a slightly negative reward. It has fully converged to this strategy.

5. Compared to v6 (episode_length 30001 → 199939): the agent was already
   surviving indefinitely in v6, we just couldn't see it because of the cap.
   The 30K cap was hiding the true behavior.

IMPLICATIONS FOR v7:
- The agent has found a degenerate strategy (survive forever, never clear).
  The current reward structure actually rewards this: dying is -20, but
  surviving indefinitely at wave 30 only costs -0.001/tick = -200 over 200K
  ticks, which is much better than dying (-20 death + losing all wave rewards).
- MUST fix the reward to make clearing waves more valuable than surviving.
  Options:
    a. Increase tick penalty significantly (but risks making agent rush/die)
    b. Add a "wave stall" penalty — if wave doesn't advance for N ticks,
       heavy penalty. This directly targets the degenerate strategy.
    c. Cap episodes at death or wave clear — no tick cap at all. If the agent
       doesn't clear the wave within some timeout, it's a death equivalent.
    d. Make death less punishing (-5 instead of -20) so the agent is less
       afraid to try aggressive strategies that might fail.
- The agent MUST be visually observed to confirm what it's actually doing.
  If it's safespotting, that's actually impressive and we should build on it.
  If it's exploiting a bug, we need to fix the game logic.

---

## v5 (2026-04-03)

Results (5B steps, ~37min, wandb run plpi9rgf):
- SPS: 2.3M (restored from v3's 1.5M)
- Wave reached: 30.0 avg (up from v2's 28.2)
- Episode length: 5056 ticks (up from v2's 3101 — surviving 63% longer)
- Episode return: 636.5
- Score: 0 (no cave completions)
- Episodes completed: 10,006
- Entropy: 6.57
- Clipfrac: 0.000005 (basically zero — still barely updating)
- Value loss: 0.014 (very stable)
- Policy loss: 0.002
- KL: 0.0001

Diagnosis: Removing explicit prayer rewards improved wave 28→30.
Agent learned to use prayer organically through damage_taken signal.
Episode length nearly doubled — agent surviving much longer. But still
stuck at wave 30. Clipfrac ~0 means policy converged to local optimum.
Agent doesn't know WHAT style hit it (only that damage occurred), making
prayer switching harder to learn. Added hit_style observation in v6.

Changes from v4:
- Removed ALL explicit prayer rewards and penalties (Jad and non-Jad).
  All prayer weights set to 0. Prayer reward logic commented out.
- Reasoning: the agent should discover prayer's value organically.
  Correct prayer → blocks 100% damage → w_damage_taken never fires.
  Wrong/no prayer → full damage → w_damage_taken penalty (-0.75).
  The damage signal already encodes whether prayer was correct.
  Explicit prayer rewards caused reward inflation (v3) and added
  complexity without improving wave reached (v4 untested due to v3 crash).
- Cleaner reward function: only 11 active weights, no prayer-specific
  code running per tick. Should restore v1/v2 SPS levels (~2.2M).

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.75
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.01
w_prayer_pot_used = -0.01
w_correct_jad_prayer = 0.0
w_wrong_jad_prayer = 0.0
w_correct_danger_prayer = 0.0
w_wrong_danger_prayer = 0.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 5_000_000_000
learning_rate = 0.001
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

---

## v4 (2026-04-03) — not trained, superseded by v5

Changes from v3:
- CRITICAL FIX: Prayer reward was firing every tick an NPC existed, not
  when hits actually landed. Caused massive reward inflation (561→3682
  episode return) and the agent learned to idle with prayer on.
- Prayer reward now only fires when: (a) player takes damage and had
  wrong prayer, or (b) NPC attacked and correct prayer blocked it.
  Max one prayer reward per tick to prevent spam.
- w_correct_danger_prayer: 2.0 → 0.5 (was too dominant vs other rewards)
- w_wrong_danger_prayer: -3.0 → -1.0 (proportional reduction)
- horizon: 512 → 256 (512 slowed SPS from 2.2M to 1.5M)

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.75
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.01
w_prayer_pot_used = -0.01
w_correct_jad_prayer = 5.0
w_wrong_jad_prayer = -10.0
w_correct_danger_prayer = 0.5
w_wrong_danger_prayer = -1.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 5_000_000_000
learning_rate = 0.001
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

---

## v3 (2026-04-03)

Results (1.7B/5B steps, stopped early — broken, wandb run crb1jwbv):
- SPS: 1.5M (down from 2.2M — horizon 512 too slow)
- Wave reached: 15.2 avg (CRASHED from v2's 28.2)
- Episode length: 4115 ticks (longer but worse — agent idling)
- Episode return: 3682.5 (massively inflated by prayer reward spam)
- Score: 0 (no cave completions)
- Episodes completed: ~10K
- Entropy: 5.7
- Clipfrac: 0.262 (now clipping — updates too aggressive)
- Value loss: 1.9 (30x higher than v2's 0.06 — unstable)
- Policy loss: 0.018
- KL: 0.019

Diagnosis: Prayer reward fired every tick per NPC, not per hit. Agent
learned to turn prayer on and idle, collecting +4-8 reward/tick from
multiple NPCs. Reward inflation destabilized training completely.
Value network couldn't predict highly variable rewards. Clipfrac spiked
to 0.26 (from 0.00) indicating policy updates too large. Wave reached
dropped from 28→15 — agent forgot how to fight, just stood still praying.

Changes from v2:
- MAJOR: Added general prayer protection reward for ALL NPCs (not just Jad).
  New config fields: w_correct_danger_prayer=2.0, w_wrong_danger_prayer=-3.0
  BUG: Reward fired every tick, not per-hit. Fixed in v4.
- w_damage_taken: -0.5 → -0.75
- w_food_used: -0.05 → -0.01 (waste scaling handles heavy penalty)
- w_prayer_pot_used: -0.05 → -0.01
- w_idle: -0.01 → 0.0
- w_tick_penalty: -0.005 → -0.001
- ent_coef: 0.05 → 0.02
- horizon: 256 → 512 (caused SPS drop, reverted in v4)

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.75
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.01
w_prayer_pot_used = -0.01
w_correct_jad_prayer = 5.0
w_wrong_jad_prayer = -10.0
w_correct_danger_prayer = 2.0
w_wrong_danger_prayer = -3.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = 0.0
w_tick_penalty = -0.001

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 5_000_000_000
learning_rate = 0.001
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.02
max_grad_norm = 0.5
horizon = 512
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

---

## v2 (2026-04-03)

Results (2.5B steps, ~19min, wandb run lsw5jibn):
- SPS: 2.2M
- Wave reached: 28.2 avg (barely improved from v1's 27.6)
- Episode length: 3101 ticks
- Episode return: 561.2
- Score: 0 (no cave completions)
- Episodes completed: 10,221
- Entropy: 6.9 (healthier than v1's 5.8)
- Clipfrac: 0.000 (STILL zero — policy not updating meaningfully)
- Value loss: 0.057
- Policy loss: -0.007
- KL: 0.00007

Diagnosis: Agent stuck at wave 28 (Ket-Zek). 5x more training and 3x
higher LR didn't help. Clipfrac still 0 — policy barely changing.
Root cause: no prayer reward for non-Jad NPCs. Agent has zero incentive
to use protection prayers before Jad, so it dies to Ket-Zek magic.
Higher entropy coef (0.05) kept exploration alive (6.9 vs 5.8) but
without the right reward signal, exploration doesn't help.

Changes from v1:
- total_timesteps: 500M → 2.5B
- learning_rate: 0.0003 → 0.001 (clipfrac=0 in v1)
- ent_coef: 0.01 → 0.05 (prevent premature convergence)
- w_correct_jad_prayer: 5.0 → 10.0
- w_wrong_jad_prayer: -10.0 → -20.0
- Food/pot waste penalty: scales by actual overheal amount.
  Shark heals 20 HP — penalty = base × (1 + wasted/heal × 9).
  Eat at 50/70: 0 waste → base only. Eat at 68/70: 90% waste → 10x.

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.5
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.05
w_prayer_pot_used = -0.05
w_correct_jad_prayer = 10.0
w_wrong_jad_prayer = -20.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = -0.01
w_tick_penalty = -0.005

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 2_500_000_000
learning_rate = 0.001
gamma = 0.999
gae_lambda = 0.95
clip_coef = 0.2
vf_coef = 0.5
ent_coef = 0.05
max_grad_norm = 0.5
horizon = 256
minibatch_size = 4096

[policy]
hidden_size = 256
num_layers = 2
```

---

## v1 (2026-04-03) — First training run

Results (500M steps, ~3m43s, wandb run xgsb170g):
- SPS: 2.2M
- Wave reached: 27.6 avg
- Episode length: 3110 ticks avg
- Episode return: 554.3
- Score (cave completions): 0
- Episodes completed: 10,174
- Entropy: 8.3 → 5.8
- Clipfrac: 0.000
- Value loss: 0.057
- Policy loss: -0.007
- KL: 0.00007

Observations:
- Agent plateaus around wave 27-28 (Ket-Zek appears with magic+melee)
- Never reached Jad (wave 63)
- Clipfrac=0 means learning rate too conservative
- Entropy dropped ~30%, agent specializing but possibly too early
- Strong early learning (wave 1→27 quickly) but no late-game progress

```ini
[base]
env_name = fight_caves

[env]
w_damage_dealt = 0.5
w_damage_taken = -0.5
w_npc_kill = 3.0
w_wave_clear = 10.0
w_jad_damage = 2.0
w_jad_kill = 50.0
w_player_death = -20.0
w_cave_complete = 100.0
w_food_used = -0.05
w_prayer_pot_used = -0.05
w_correct_jad_prayer = 5.0
w_wrong_jad_prayer = -10.0
w_invalid_action = -0.1
w_movement = 0.0
w_idle = -0.01
w_tick_penalty = -0.005

[vec]
total_agents = 4096
num_buffers = 2

[train]
total_timesteps = 500_000_000
learning_rate = 0.0003
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
