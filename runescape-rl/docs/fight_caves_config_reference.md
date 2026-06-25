# Fight Caves Config Reference

This document explains the current Fight Caves training config end to end.

Current source config:

- `runescape-rl/config/fight_caves.ini`

Generated/synced training copy:

- `pufferlib_4/config/fight_caves.ini`

Last reviewed: 2026-06-25.

## Reward Sweeps Vs One-At-A-Time Tests

Short answer: use both, but for different jobs.

A zero-inclusive reward sweep is good for screening. It can tell us which
weights might be too large, too small, or possibly better at zero. It is most
useful when we have enough GPU capacity to run many trials in parallel.

One-at-a-time removal is better for confirmation. It tells us whether a single
change actually caused the improvement or regression. It is slower if we test
many terms serially, but the result is easier to trust.

For this repo, a sweep is not automatically less time-consuming on one GPU. A
useful reward sweep needs multiple full-enough runs. A single 1B run takes about
15 minutes right now. A 20-run serial sweep would still cost about 5 hours. If
we run it on multiple GPUs, the wall-clock cost changes a lot.

Important caution: if a short sweep favors `0`, that does not prove the reward
term is bad. It may mean:

- the term is bad,
- the tested range was too high,
- the reward interacts badly with another term,
- the short 1B budget did not run long enough,
- or the result was training noise.

Recommended Phase 7 process:

1. Keep core outcome terms fixed at first: damage dealt, damage taken, NPC kill,
   wave clear, Jad kill, and player death.
2. Run zero-inclusive sweeps only over shaping terms and diagnostics.
3. Treat sweep winners as candidates, not final answers.
4. Confirm the best candidates with one-change runs.
5. Only update the live 3B config after a shorter run beats the corrected
   Phase 6 baseline.

The Phase 7 no-kiting trial already tested one extreme point:
`shape_kiting_reward = 0.0`. It was materially worse at 1B steps, so future
kiting tests should try smaller reductions, such as `1.0`, before removing it.

## End-To-End Flow

1. `runescape-rl/train.sh` chooses a config with `CONFIG_PATH`, defaulting to
   `runescape-rl/config/fight_caves.ini`.
2. `train.sh` copies that config to `pufferlib_4/config/fight_caves.ini`,
   because PufferLib loads configs from its own `config/` directory.
3. PufferLib reads `pufferlib_4/config/default.ini` first, then overlays
   `pufferlib_4/config/fight_caves.ini`.
4. `runescape-rl/fc-training/binding.c` copies `[env]` values into the C env.
5. `c_reset` initializes a fresh Fight Caves episode.
6. `c_step` reads the five action heads, advances one game tick, computes
   reward, writes the next observation, and logs terminal episode stats.
7. `fc_reward_compute_breakdown` converts per-tick game events into named
   reward channels.
8. Native PufferLib clamps scalar training rewards to `[-1, 1]` before PPO
   training. Reward-channel logs still report the channel totals.
9. W&B/local JSON logs use the metric from `[sweep].metric`, currently
   `jad_kill_rate`.

## Current Config Sections

| Section | Purpose |
| --- | --- |
| `[base]` | Environment name and basic run cadence. |
| `[env]` | Fight Caves supplies, reward weights, reward shaping, and obs ablations. |
| `[vec]` | Number of parallel env instances and rollout buffers. |
| `[train]` | PufferLib/PPO optimization hyperparameters. |
| `[policy]` | Neural network size. |
| `[run]` | Manifest and contract metadata. Mostly for reproducibility. |
| `[sweep]` | Metric used by Puffer sweep code and W&B logging gate. |

## Base Config

| Key | Current value | Meaning |
| --- | ---: | --- |
| `env_name` | `fight_caves` | Tells PufferLib which env config/backend to load. |
| `checkpoint_interval` | `50` | Save a checkpoint every 50 train/eval loop epochs and at final train epoch. |

Inherited from `pufferlib_4/config/default.ini` unless overridden:

| Key | Effective value | Meaning |
| --- | ---: | --- |
| `checkpoint_dir` | `checkpoints` | Root under `pufferlib_4/` for saved models. |
| `log_dir` | `logs` | Root under `pufferlib_4/` for local JSON logs. |
| `eval_episodes` | `10000` | Training loop keeps eval running until this many eval episodes are logged. |
| `cudagraphs` | `10` | Native backend captures CUDA graphs after warmup for speed. |
| `seed` | `73` | Native PufferLib training RNG seed. |
| `reset_state` | `True` | Resets recurrent state for sampled training minibatches. |
| `rank`, `world_size`, `gpu_id` | `0`, `1`, `0` | Single-GPU defaults. |

## Environment Supplies

| Key | Current value | Meaning |
| --- | ---: | --- |
| `initial_sharks` | `20` | Sets the player's starting sharks on reset. Clamped to `0..20`. |
| `initial_prayer_doses` | `32` | Sets starting prayer potion doses on reset. Clamped to `0..32`. |

## Scalar Reward Weights

The scalar reward is the sum of the channels below. Native PufferLib then
hard-clamps the scalar reward to `[-1, 1]` for training.

| Key | Current value | What it does |
| --- | ---: | --- |
| `w_damage_dealt` | `0.9` | Rewards damaging NPC hits. Formula is `(damage_dealt_this_tick / 1000 + damaging_hit_count) * weight`. Zero-damage hits do not pay this reward. |
| `w_damage_taken` | `-1.9` | Penalizes player damage. Formula is `(damage_frac * damage_frac * 70) * weight`, so larger hits hurt more than small hits. |
| `w_npc_kill` | `3.5` | Rewards each NPC killed on the current tick. |
| `w_wave_clear` | `15.0` | Rewards clearing a wave. Formula is `weight * cleared_wave_number`, so later waves pay more. |
| `w_jad_kill` | `2000.0` | Rewards killing Jad. In normal completion this is the main cave-completion payoff. |
| `w_player_death` | `-11.0` | Penalizes player death. |
| `w_correct_jad_prayer` | `1.5` | Rewards correct protection prayer when a Jad hit resolves. Suppressed during the idle-prayer gate described below. |
| `w_correct_danger_prayer` | `0.25` | Rewards correct protection prayer against non-Jad styled hits. Suppressed during the idle-prayer gate described below. |
| `w_invalid_action` | `-0.1` | Penalizes any invalid action on exposed action heads 0-4. Invalid-action classes are also logged separately. |
| `w_tick_penalty` | `-0.005` | Small time penalty every tick. Encourages faster clears. |

Prayer reward idle gate:

- If the policy has gone at least one tick without attacking while it could be
  attacking, correct-prayer rewards are suppressed for that tick.
- Wrong-prayer penalties still apply.
- This is meant to avoid rewarding a policy that only stands still and prays.

## Reward Shaping Terms

These are extra behavior-shaping terms. They are more likely than core outcome
rewards to be sweep/removal candidates.

| Key | Current value | What it does |
| --- | ---: | --- |
| `shape_food_waste_scale` | `-1.2` | When a shark is eaten, penalizes the fraction of its 200 HP heal that was wasted. |
| `shape_pot_waste_scale` | `-1.2` | When a prayer potion dose is used, penalizes the fraction of its 170 prayer restore that was wasted. |
| `shape_wrong_prayer_penalty` | `-0.3` | Penalizes wrong protection prayer on non-Jad styled hits. Current scalar reward does not separately use `WRONG_JAD_PRAY`; wrong Jad prayer is punished mostly through damage taken. |
| `shape_npc_melee_penalty` | `-0.8` | Per tick, penalizes each adjacent melee-pressure NPC. This discourages standing next to dangerous NPCs. |
| `shape_wasted_attack_penalty` | `-0.1` | Penalizes a tick where the player is attack-ready, has a target, NPCs remain, but no damage was dealt. |
| `shape_kiting_reward` | `2.2` | Rewards dealing damage while the current target is within the configured distance band. Phase 7 trial 1 showed setting this to zero was too aggressive. |
| `shape_kiting_min_dist` | `2` | Lower distance bound for the kiting reward. |
| `shape_kiting_max_dist` | `10` | Upper distance bound for the kiting reward. |
| `shape_safespot_attack_reward` | `0.0` | Deprecated no-op. Direct safespot reward is disabled. LOS safespotting still works through combat/pathing mechanics. |
| `shape_unnecessary_prayer_penalty` | `-0.2` | Penalizes having any protection prayer active when there is no detected threat. |
| `shape_wave_stall_start` | `1400` | Number of ticks in a wave before the wave-stall penalty can begin. |
| `shape_wave_stall_ramp_interval` | `150` | After stall starts, penalty magnitude increases every this many ticks. |
| `shape_wave_stall_base_penalty` | `-0.5` | First per-tick wave-stall penalty once the wave is too slow. |
| `shape_wave_stall_cap` | `-2.0` | Most negative wave-stall penalty after ramping. |
| `shape_resource_threat_window` | `2` | Used to compute `imminent_threat` in the reward threat context. In current reward code, no scalar reward term actually reads `imminent_threat`, so this appears behaviorally dead today. |
| `shape_jad_heal_penalty` | `-0.3` | Penalizes each Yt-HurKot healer proc that lands on Jad. |

## Raw Reward Features

These are the 19 raw per-tick event features defined in `fc_contracts.h`.
They are not part of the current Puffer-facing policy observation. The reward
code reads them internally to build the scalar reward.

| Feature | Raw meaning | Used by current scalar reward? |
| --- | --- | --- |
| `FC_RWD_DAMAGE_DEALT` | NPC HP reduced this tick, normalized by `1000`. | Yes, through `w_damage_dealt`. |
| `FC_RWD_DAMAGE_TAKEN` | Player HP reduced this tick, normalized by max HP. | Yes, through `w_damage_taken`. |
| `FC_RWD_NPC_KILL` | Number of NPCs killed this tick. | Yes, through `w_npc_kill`. |
| `FC_RWD_WAVE_CLEAR` | Current wave cleared this tick. | Yes, through `w_wave_clear`. |
| `FC_RWD_JAD_DAMAGE` | Jad HP reduced this tick, normalized by `1000`. | No separate scalar term. Jad damage still contributes through normal damage dealt. |
| `FC_RWD_JAD_KILL` | Jad defeated. | Yes, through `w_jad_kill`. |
| `FC_RWD_PLAYER_DEATH` | Player died. | Yes, through `w_player_death`. |
| `FC_RWD_CAVE_COMPLETE` | Cave completed. | No separate scalar term. Completion should coincide with Jad kill. |
| `FC_RWD_FOOD_USED` | Shark consumed this tick. | Yes, but only to compute food-waste penalty. |
| `FC_RWD_PRAYER_POT_USED` | Prayer potion dose consumed this tick. | Yes, but only to compute potion-waste penalty. |
| `FC_RWD_CORRECT_JAD_PRAY` | Prayer matched a resolving Jad hit. | Yes, through `w_correct_jad_prayer`. |
| `FC_RWD_WRONG_JAD_PRAY` | Prayer failed to match a resolving Jad hit. | No direct scalar term today. Damage taken still penalizes it. |
| `FC_RWD_INVALID_ACTION` | Invalid exposed action attempted. | Yes, through `w_invalid_action`. |
| `FC_RWD_MOVEMENT` | Movement action executed. | No direct scalar term. |
| `FC_RWD_IDLE` | Idle/wait action. | No direct scalar term. |
| `FC_RWD_TICK_PENALTY` | Always `1.0` every tick. | Yes, through `w_tick_penalty`. |
| `FC_RWD_CORRECT_DANGER_PRAY` | Correct prayer on non-Jad styled hit. | Yes, through `w_correct_danger_prayer`. |
| `FC_RWD_WRONG_DANGER_PRAY` | Wrong prayer on non-Jad styled hit. | Yes, through `shape_wrong_prayer_penalty`. |
| `FC_RWD_ATTACK_ATTEMPT` | Valid attack cycle launched this tick. | Indirectly. It resets attack-idle tracking and affects the prayer reward idle gate. |

## Observation Overview

Current Puffer-facing observation size:

- `158` floats total.
- First `122` floats are policy state observations.
- Last `36` floats are action-mask values for heads 0-4.
- The raw reward features are not exposed to Puffer policy input in the current
  adapter.

Important masking detail:

- The PyTorch `--slowly` backend applies the appended mask to logits.
- The native CUDA backend used by normal training appears to treat the mask as
  ordinary input features. It does not hard-force invalid logits to `-inf`.
- That means invalid actions can still be sampled in normal training. The env
  rejects/penalizes/logs them.

All state observations are normalized floats, usually in `[0, 1]`.

## Player Observation Features

There are 17 player features.

| Offset | Name | Meaning |
| ---: | --- | --- |
| `0` | `player_hp` | Current HP divided by max HP. |
| `1` | `player_prayer` | Current prayer divided by max prayer. |
| `2` | `player_x` | Player x tile divided by arena width. |
| `3` | `player_y` | Player y tile divided by arena height. |
| `4` | `player_attack_timer` | Attack cooldown divided by weapon speed. |
| `5` | `pray_melee_on` | `1` if Protect from Melee is active. |
| `6` | `pray_range_on` | `1` if Protect from Missiles is active. |
| `7` | `pray_magic_on` | `1` if Protect from Magic is active. |
| `8` | `sharks_remaining` | Sharks remaining divided by max sharks. |
| `9` | `prayer_doses_remaining` | Prayer doses remaining divided by max doses. |
| `10` | `incoming_melee_1t` | Count of melee hits landing in 1 tick, clamped at 4 and divided by 4. |
| `11` | `incoming_range_1t` | Count of ranged hits landing in 1 tick, clamped at 4 and divided by 4. |
| `12` | `incoming_magic_1t` | Count of magic hits landing in 1 tick, clamped at 4 and divided by 4. |
| `13` | `incoming_melee_2t` | Count of melee hits landing in 2 ticks, clamped at 4 and divided by 4. |
| `14` | `incoming_range_2t` | Count of ranged hits landing in 2 ticks, clamped at 4 and divided by 4. |
| `15` | `incoming_magic_2t` | Count of magic hits landing in 2 ticks, clamped at 4 and divided by 4. |
| `16` | `current_attack_target_slot` | Current attack target visible slot. `0` means none, otherwise `(slot + 1) / 8`. |

Current config note:

- `obs_ablate_incoming_aggregates = 1`, so offsets `10..15` are zeroed today.

## NPC Observation Features

There are 8 visible NPC slots. Each slot has 12 features, for 96 total NPC
features.

Slot ordering:

1. Only active, alive NPCs are eligible.
2. Sort by Chebyshev distance to the nearest tile of the NPC footprint.
3. Break ties by `spawn_index`.
4. Keep the closest 8.
5. Empty slots are all zero.

Overflow behavior:

- NPCs beyond the closest 8 are still simulated and can attack/move/take
  damage.
- The policy cannot directly choose an overflow NPC through the attack head.

| Offset in slot | Name | Meaning |
| ---: | --- | --- |
| `0` | `npc_valid` | `1` if this slot contains an active NPC. |
| `1` | `npc_x` | NPC x tile divided by arena width. |
| `2` | `npc_y` | NPC y tile divided by arena height. |
| `3` | `npc_hp` | NPC current HP divided by max HP. |
| `4` | `npc_distance` | Chebyshev distance to NPC footprint divided by arena width. |
| `5` | `telegraph_melee` | `1` if this NPC would use melee from current distance. |
| `6` | `telegraph_range` | `1` if this NPC would use ranged from current distance. |
| `7` | `telegraph_magic` | `1` if this NPC would use magic from current distance. |
| `8` | `npc_attack_timer` | NPC attack cooldown divided by NPC attack speed. |
| `9` | `npc_los` | `1` if player has line of sight to this NPC. |
| `10` | `pending_style` | Incoming attack style from this NPC. `0` none, otherwise style divided by `3`. |
| `11` | `pending_ticks` | Ticks until this NPC's pending hit resolves, divided by `10`. |

Current config notes:

- `obs_ablate_npc_distance = 0`, so `npc_distance` is visible.
- `obs_ablate_npc_valid = 0`, so `npc_valid` is visible.
- Telegraph bits do not check LOS. LOS is a separate feature, which lets the
  agent distinguish blocked safespot threats from active threats.
- Healers do not telegraph protection prayer style.
- Jad only telegraphs after it has committed a pending hit.

## Meta Observation Features

There are 9 meta features.

| Offset | Name | Meaning |
| ---: | --- | --- |
| `0` | `wave` | Current wave divided by number of waves. |
| `1` | `rotation` | Spawn rotation id divided by number of rotations. |
| `2` | `npcs_remaining` | NPCs remaining divided by max NPCs. |
| `3` | `prayer_drain_progress` | Prayer drain counter divided by drain resistance. |
| `4` | `incoming_melee_3t` | Count of melee hits landing in 3 ticks, clamped at 4 and divided by 4. |
| `5` | `incoming_range_3t` | Count of ranged hits landing in 3 ticks, clamped at 4 and divided by 4. |
| `6` | `incoming_magic_3t` | Count of magic hits landing in 3 ticks, clamped at 4 and divided by 4. |
| `7` | `damage_taken_this_tick` | Damage taken this tick divided by max HP. |
| `8` | `wave_just_cleared` | `1` on the tick a wave clears. |

Current config note:

- `obs_ablate_incoming_aggregates = 1`, so offsets `4..6` are zeroed today.

## Observation Ablation Flags

These are experimental switches in `[env]`.

| Key | Current value | Effect when set to `1` |
| --- | ---: | --- |
| `obs_ablate_npc_distance` | `0` | Zeroes `npc_distance` for all visible NPC slots. |
| `obs_ablate_incoming_aggregates` | `1` | Zeroes incoming-hit aggregate counts for 1, 2, and 3 ticks. Per-NPC pending style/ticks remain visible. |
| `obs_ablate_npc_valid` | `0` | Zeroes `npc_valid` for all visible NPC slots. Empty slots still have other zero features. |

## Action Heads

The policy outputs 5 independent discrete heads.

| Head | Size | Values |
| ---: | ---: | --- |
| `0` | `17` | Move. `0` idle, `1..8` walk N/NE/E/SE/S/SW/W/NW, `9..16` run N/NE/E/SE/S/SW/W/NW. |
| `1` | `9` | Attack. `0` none, `1..8` attack visible NPC slot `0..7`. |
| `2` | `5` | Prayer. `0` no change, `1` off, `2` magic, `3` range, `4` melee. |
| `3` | `3` | Eat. `0` none, `1` shark, `2` combo eat. |
| `4` | `2` | Drink. `0` none, `1` prayer potion. |

The backend has path-target heads 5 and 6 for click-to-tile routing, but normal
Puffer training does not expose them. They are forced to zero by the adapter.

## Action Mask Features

The appended 36 mask floats are laid out as:

| Mask region | Size | Meaning |
| --- | ---: | --- |
| `move` | `17` | `1` for legal movement choices, `0` for blocked/out-of-bounds/no-energy choices. |
| `attack` | `9` | `1` for visible NPC target slots, `0` for empty slots. Attack none is always valid. |
| `prayer` | `5` | Fully unmasked. Prayer choices are always presented to the policy. |
| `eat` | `3` | Shark/combo masked if no sharks, food cooldown, combo cooldown, or full HP. |
| `drink` | `2` | Prayer potion masked if no doses, potion cooldown, or full prayer. |

Again, normal native training currently exposes these mask values as inputs but
does not hard-apply them to logits.

## Vectorization Hyperparameters

| Key | Current value | Meaning |
| --- | ---: | --- |
| `total_agents` | `4096` | Number of parallel Fight Caves env instances. One rollout step advances all 4096 envs. |
| `num_buffers` | `2` | Splits envs into rollout buffers for native Puffer scheduling. With 4096 agents, this is 2048 envs per buffer. |
| `num_threads` | `16` inherited | CPU worker threads for env stepping. Not set in `fight_caves.ini`, inherited from default config. |

## Training Hyperparameters

| Key | Current value | Meaning |
| --- | ---: | --- |
| `total_timesteps` | `3,000,000,000` | Target agent steps for the live run. Steps are `agents * horizon * epochs`, so final logged steps may be slightly under or over. |
| `anneal_lr` | `0` | Disables cosine learning-rate decay. LR stays fixed. |
| `learning_rate` | `0.0009000000000000007` | Muon optimizer learning rate. |
| `ent_coef` | `0.02423374579539897` | Entropy bonus weight. Higher means more exploration/randomness. |
| `gamma` | `0.9963272487242703` | Discount factor for future reward. Higher values care more about late waves/Jad. |
| `gae_lambda` | `0.96405274026941` | Advantage smoothing parameter. Higher values use longer-horizon credit assignment. |
| `clip_coef` | `0.17830599245832296` | PPO policy-ratio clipping width. Lower is more conservative. |
| `vf_coef` | `1` | Value loss weight in the total loss. |
| `vf_clip_coef` | `0.15124043205980495` | PPO-style value prediction clipping width. |
| `max_grad_norm` | `0.25` | Global gradient clipping threshold before optimizer step. |
| `horizon` | `256` | Number of ticks collected per env per rollout. Batch size before replay is `total_agents * horizon`. |
| `minibatch_size` | `4096` | Training minibatch size. Must be divisible by `horizon`. Here it is 16 rollout segments of 256 ticks. |
| `replay_ratio` | `1.567733336432473` | Number of training minibatches per rollout batch, approximately `replay_ratio * batch_size / minibatch_size`. |
| `vtrace_rho_clip` | `0.5` | Clips V-trace importance weights for value/advantage correction. |
| `vtrace_c_clip` | `0.5037274754757021` | Clips V-trace trace coefficients. |
| `prio_alpha` | `0.9682355928752012` | Controls prioritized segment sampling from rollout advantages. Near `1` means strong prioritization. |
| `prio_beta0` | `0` | Initial importance-correction exponent for prioritized replay. It anneals upward during training based on `prio_alpha`. |
| `beta1` | `0.95` | Muon optimizer momentum parameter. |
| `beta2` | `0.9995810484472892` | Present in native config/hypers, but the current native Muon init path appears to use `beta1` and `eps`; verify before treating `beta2` as active. |
| `eps` | `1e-10` | Muon optimizer numerical epsilon. |
| `gpus` | `1` inherited | Number of GPUs for training. Not set in `fight_caves.ini`, inherited from default config. |
| `min_lr_ratio` | `0.0` inherited | Minimum LR ratio if `anneal_lr=1`. Currently irrelevant because annealing is off. |

Training loop notes:

- Batch size per rollout is `4096 * 256 = 1,048,576` agent steps.
- Train epochs are approximately `total_timesteps / batch_size`.
- After train epochs, PufferLib runs an eval phase and may save a final/eval
  checkpoint slightly beyond the requested train-step count.
- Rewards are clamped to `[-1, 1]` in native training before advantage
  computation.

## Policy Hyperparameters

| Key | Current value | Meaning |
| --- | ---: | --- |
| `hidden_size` | `256` | Width of the default linear encoder, MinGRU hidden state, and decoder. |
| `num_layers` | `3` | Number of MinGRU recurrent layers. |

Effective native policy shape:

1. Input: 158 floats.
2. Default linear encoder: `158 -> hidden_size`.
3. MinGRU recurrent network: `num_layers` layers.
4. Decoder: one categorical logit vector per action head plus one value output.
5. Action logits total: `17 + 9 + 5 + 3 + 2 = 36`.

Inherited PyTorch config, mostly relevant only for `--slowly`:

| Key | Effective value | Meaning |
| --- | --- | --- |
| `torch.network` | `MinGRU` | PyTorch recurrent network class. |
| `torch.encoder` | `DefaultEncoder` | PyTorch linear encoder class. |
| `torch.decoder` | `DefaultDecoder` | PyTorch decoder/action-value head class. |

## Run Metadata

| Key | Current value | Meaning |
| --- | --- | --- |
| `manifest_path` | `''` | Filled by `train.sh` with the generated run manifest path. |
| `manifest_schema_version` | `1` | Manifest schema version. |
| `observation_version` | `fight_caves_puffer_policy_obs_v1_mask_heads_0_4` | Human-readable observation contract label. |
| `action_version` | `fight_caves_multidiscrete_5_head_v1` | Human-readable action contract label. |
| `reward_version` | `fight_caves_v38_phase4_reward_cleanup` | Human-readable reward contract label. |
| `reward_clip_enabled` | `1` | Metadata saying reward clipping is expected. Native Puffer currently clamps rewards regardless of this config flag. |
| `reward_clip_min` | `-1.0` | Metadata lower clip bound. |
| `reward_clip_max` | `1.0` | Metadata upper clip bound. |

## Sweep Metadata

| Key | Current value | Meaning |
| --- | --- | --- |
| `metric` | `jad_kill_rate` | Target metric for Puffer sweep optimization and for the W&B logging gate in normal training. |

## Good Manual-Review Candidates

These are not automatic change recommendations, but they are the first places
worth reviewing.

| Item | Why it stands out |
| --- | --- |
| `shape_resource_threat_window` | Parsed and used to compute `imminent_threat`, but no scalar reward term currently reads `imminent_threat`. Likely behaviorally dead today. |
| `shape_safespot_attack_reward` | Deprecated no-op and already `0.0`. Kept for config compatibility. |
| `FC_RWD_WRONG_JAD_PRAY` | Raw feature exists, but there is no direct wrong-Jad-prayer penalty. Damage taken still punishes failures. |
| `FC_RWD_JAD_DAMAGE` | Raw feature exists, but Jad damage only receives normal damage reward, not a separate Jad-damage reward. |
| `FC_RWD_CAVE_COMPLETE` | Raw feature exists, but scalar reward uses Jad kill rather than a separate cave-complete reward. |
| `FC_RWD_MOVEMENT` and `FC_RWD_IDLE` | Raw features exist, but no direct scalar reward terms currently use them. |
| `obs_ablate_incoming_aggregates = 1` | The current live config hides aggregate incoming-hit counts, while per-NPC pending style/ticks remain visible. This may be intentional from prior obs sweeps, but it is worth remembering. |
| Native action mask behavior | Normal native training appears not to hard-apply the appended mask to logits. Invalid actions are still possible and are handled by env rejection, penalties, and diagnostics. |
| `beta2` | Present in config/native hypers, but current native Muon initialization appears to use `beta1` and `eps`; verify before sweeping it. |

## Suggested Reward Sweep Scope

A practical zero-inclusive Phase 7 reward sweep should avoid changing too many
things at once.

Keep fixed initially:

- `w_damage_dealt`
- `w_damage_taken`
- `w_npc_kill`
- `w_wave_clear`
- `w_jad_kill`
- `w_player_death`

Reason: these are the main progress/survival outcome signals. Removing them can
make the run hard to interpret.

Good first sweep candidates:

- `shape_wasted_attack_penalty`: include `0`.
- `shape_wrong_prayer_penalty`: include `0` and smaller penalties.
- `shape_npc_melee_penalty`: include `0` and smaller penalties.
- `shape_unnecessary_prayer_penalty`: include `0`.
- `shape_wave_stall_base_penalty`: include `0`.
- `shape_jad_heal_penalty`: include `0`.
- `w_invalid_action`: include `0`.
- `w_correct_danger_prayer`: include `0`.
- `w_correct_jad_prayer`: include `0`.

Treat kiting separately:

- Do not immediately sweep kiting all the way to zero again as the only test.
- The 1B no-kiting run already failed hard.
- Better next values are around `0.5`, `1.0`, and `1.5`, compared against the
  current `2.2`.

Do not mix reward sweeps with hyperparameter sweeps at the same time. If both
reward and PPO settings move together, it becomes hard to tell what caused the
result.
