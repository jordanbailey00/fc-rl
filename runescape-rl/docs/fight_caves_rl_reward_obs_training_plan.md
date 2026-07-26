# Fight Caves RL: Reward, Observation, and Training Remediation Plan

**Repository:** `jordanbailey00/fc-rl`  
**Branch:** `testing`  
**Purpose:** implementation handoff for a coding agent  
**Primary objective:** train an agent that discovers effective Fight Caves behavior rather than being directly rewarded for human-selected tactics.

> The repository did not contain a `testing` branch when this document was prepared. The branch was created from the then-current `main` branch before adding this document.

---

## Executive decision

Do **all** of the following, but in a strict order:

1. **Freeze and archive the current system** as a reproducible legacy control.
2. **Fix trainer and environment correctness** without changing the legacy rewards or hyperparameters.
3. **Fix observation and action semantics**, especially masks and target identity.
4. **Prune the reward substantially** instead of sweeping twenty shaping coefficients.
5. **Add a start-state curriculum** so sparse or minimally shaped objectives are learnable.
6. **Run grouped reward and observation ablations.**
7. **Sweep PPO hyperparameters last**, after the task, observations, masks, and reward scale are stable.

Do not change rewards, observations, action semantics, curriculum, and PPO hyperparameters in one experiment. That makes any result impossible to attribute.

The long-term production reward should usually contain:

- the true terminal objective;
- at most one strategy-neutral progress signal;
- optionally one genuine secondary objective, only if it is part of how the final policy is evaluated.

Prayer switching, safespotting, kiting, target priority, eating, and potion timing should normally emerge because they improve cave completion—not because each has its own permanent reward channel.

---

## Current system snapshot

The current configuration uses:

- 4,096 environments;
- a 256-tick rollout horizon;
- a three-layer, 256-hidden-unit MinGRU;
- five policy heads: movement, attack target, prayer, eating, and drinking;
- 122 game-state features plus 36 action-mask values;
- a large reward stack containing roughly twenty positive and negative channels;
- PPO plus V-trace-style correction and prioritized trajectory replay.

The current reward configuration includes, among other terms:

- `w_damage_dealt = 0.9`
- `w_damage_taken = -1.9`
- `w_npc_kill = 3.5`
- `w_wave_clear = 15.0`
- `w_jad_kill = 2000.0`
- `w_player_death = -11.0`
- prayer correctness rewards and penalties
- kiting and “safespot” rewards
- proximity, resource-waste, invalid-action, attack, time, healer, and wave-stall penalties

This recipe has produced strong results for at least one loadout, so it should remain available as a regression control. It should not, however, be treated as evidence that each shaping term is necessary or correctly aligned.

---

# P0: correctness issues to fix before reward work

## 1. Training rewards are hard-clamped to `[-1, 1]`

In `pufferlib_4/src/pufferlib.cu`, the transposed rollout rewards are hard-clamped before advantage calculation:

```cpp
clamp_precision_kernel(..., -1.0f, 1.0f, ...);
```

This radically changes the meaning of the configured coefficients.

Examples:

- a late wave clear worth hundreds of raw reward becomes `+1`;
- a Jad kill worth `+2000` becomes `+1`;
- a sufficiently large ordinary positive shaping event also becomes `+1`;
- death and many substantial damage events become `-1`.

The weights mainly affect whether same-tick terms cancel and whether the final sum crosses a clipping boundary. They do not retain their nominal proportional importance.

The environment logs raw reward-channel totals, while PPO trains on the clipped scalar. Therefore, existing reward dashboards can be misleading about the signal optimized by the policy.

### Required change

Make reward clipping configurable and instrument it.

Add metrics for:

- raw scalar reward;
- actual training reward;
- positive and negative clipping fractions;
- reward percentiles;
- reward by event after all trainer-side transforms.

For the corrected legacy control, preserve clipping initially so the comparison isolates the other fixes. For the new reward design, scale rewards so clipping is unnecessary, then disable it. The normal target should be effectively zero clipped samples.

---

## 2. Action masks are now enforced constraints

Completed 2026-07-22. `runescape-rl/fc-training/fight_caves.h` publishes the
31 move/attack/prayer legality flags through PufferLib's dedicated native mask
channel. The same flags remain appended to the policy input to preserve the
existing 307-float observation contract and checkpoint compatibility.

The CUDA rollout sampler excludes invalid logits. The exact mask used for each
sample is stored in the rollout and reused for PPO new-policy log-probability,
entropy, KL/ratio, and gradient calculations. Invalid logits receive zero PPO
gradient.

### Implemented contract

Masks are applied consistently in:

- rollout action sampling;
- rollout log-probability calculation;
- PPO new-policy log-probability calculation;
- entropy calculation;
- KL and diagnostic calculations.

The trainer checks that mask width equals the sum of discrete head sizes. The
environment adapter tests that every head has at least one legal action and that
the native byte mask exactly matches the retained observation mask.

Follow-up rules:

- use `w_invalid_action = 0` in new experiments after the mask is trusted;
- retain an invalid-action counter as an assertion/diagnostic;
- retune entropy because the valid action-set size and entropy scale will change;
- preferably report per-head entropy normalized by the maximum entropy of the valid action set.

Acceptance result: a 20M-step native smoke run completed with
`invalid_move=0`, `invalid_attack=0`, and `invalid_prayer=0` throughout. The
next gate is an unchanged 2.5B `v2_simple_reward` comparison against W&B run
`ur7t6c4n`.

---

## 3. Attack slots can resolve to a different NPC than the policy observed

The observation writer in `fc_state.c` orders NPCs using Chebyshev distance to the NPC's top-left coordinate. The action resolver in `fc_tick.c` orders NPCs using the footprint-aware `fc_distance_to_npc`.

Those orderings differ for multi-tile NPCs.

There is a second mismatch: movement is processed before attack-slot resolution. A move-and-attack action is selected from the pre-movement observation, but the attack slot is resolved after movement using a newly calculated order. Even size-one NPCs can swap positions in that ordering.

### Preferred fix

Use stable entity slots.

A good contract is:

- expose all 16 internal NPC slots;
- preserve slot identity for the lifetime of each NPC;
- have the attack head target those exact slots;
- expose a validity bit and stable spawn identifier;
- do not resort target identities between observation and action execution.

If retaining eight visible slots temporarily, save the exact slot-to-NPC mapping when writing the observation and consume that saved mapping for the next action.

Acceptance tests must include:

- simultaneous movement and attack;
- Jad and other large NPCs;
- tied distances;
- NPC death and replacement;
- more than eight active NPCs.

---

## 4. Environment RNG streams begin identically

`vecenv.h` assigns `env->rng = num_envs`, but `my_init` resets `seed_counter` to zero and the reset path seeds the game using only the incremented counter.

As a result, every environment starts its first episode with seed `1`, then reuses overlapping episode-seed sequences. Policy sampling causes later divergence, but simulator rotations and random streams are unnecessarily correlated.

### Required change

Derive each episode seed from a stable hash of:

- global experiment seed;
- environment identity or `env->rng`;
- episode counter.

For example:

```text
episode_seed = hash64(global_seed, env_id, episode_index)
```

Acceptance criterion: vector resets produce diverse rotations and RNG sequences while remaining exactly reproducible for the same global seed.

---

## 5. One transition/reward is dropped at each rollout boundary

The rollout callback stores the reward generated by the previous action at the next time index. GAE then uses `reward[t + 1]` and iterates only to `horizon - 2`.

The reward produced by the final action of each 256-step rollout is copied into index zero of the next rollout, where it is never consumed by that shifted GAE calculation. This drops approximately one transition out of every 256 and can drop important terminal events at unlucky boundaries.

### Required change

Use a conventional aligned rollout contract:

```text
obs[t], action[t], logprob[t], value[t]
step environment
reward[t], done[t], next_obs
```

Store one separate bootstrap value for the state after the final action.

Add a deterministic toy-environment test where every action has a known reward and verify that every reward contributes exactly once, including horizon-boundary and terminal rewards.

---

## 6. Scalar and vector advantage implementations disagree

The scalar path computes a TD error equivalent to:

```text
rho * reward + gamma * next_value - value
```

The vector path computes:

```text
rho * (reward + gamma * next_value - value)
```

The live horizon of 256 selects the vectorized path, but changing to a horizon not divisible by the vector width changes the learning algorithm.

### Required change

Choose the intended V-trace equation and make scalar/vector implementations bitwise or numerically equivalent. Add parity tests across float32/BF16 and several horizons.

---

## 7. Analytics have data races

Episode analytics and reward-channel counters are process-global floats updated inside OpenMP-parallel environment steps.

This can corrupt:

- Jad kill rate;
- reach-wave-63 rate;
- reward-channel totals and fire counts;
- sweep rankings;
- checkpoint selection.

### Required change

Accumulate metrics per environment or per worker and perform a deterministic reduction outside the parallel step. Do not use unsynchronized shared `+=` operations.

---

# Environment and reward-mechanics issues

## Reward pays for zero-damage projectiles

When a player projectile resolves, `hits_landed_this_tick` increments even when damage is zero. The damage reward is:

```text
(damage / 1000 + hits_landed) * w_damage_dealt
```

A miss therefore receives a substantial positive reward. This favors attack frequency and weapon speed independently of actual progress.

### Change

Remove the base reward per resolved projectile. If damage progress is retained, reward only actual state progress, not the existence of an attack cycle.

---

## “Safespot” reward does not identify a safespot

The flag is set when the player launches a ranged attack while no NPC is adjacent. It does not establish that:

- cover is being used;
- the target is unable to attack;
- the position is a genuine safespot;
- the position is strategically desirable.

It therefore rewards ordinary ranged combat at distance.

### Change

Remove this reward from the discovery-oriented objective. Track geometric behavior only as an evaluation diagnostic.

---

## Ranged and magic NPCs may stop behind cover

Generic ranged/magic NPC movement occurs only when distance exceeds attack range. If an NPC is within nominal range but lacks line of sight, it cannot attack and also does not move. Jad follows the same basic pattern.

### Change

Move when either:

```text
distance > attack_range
OR
line_of_sight == false
```

A stronger implementation should path toward a valid attack tile rather than greedily walking toward the player center.

---

## Jad healer spawn positions are not validated

Healers are spawned using offsets around Jad without checking collision or entity overlap. Jad is size five; some offsets lie inside its footprint when its coordinates represent the footprint's top-left tile.

### Change

Find valid spawn tiles outside Jad's footprint that are:

- walkable;
- unoccupied;
- inside the arena;
- reachable or otherwise consistent with intended simulation rules.

Add deterministic healer-spawn tests for all rotations and boundary positions.

---

# Observation strategy

The priority is not “more observations” in general. It is **correct, causal, stable observations**.

## Changes to make before observation ablations

1. Fix target identity and action-slot semantics.
2. Restore incoming-hit aggregates; the current live config sets `obs_ablate_incoming_aggregates = 1`.
3. Add NPC type. In the real game, NPC appearance reveals type, so this is not privileged tactical advice.
4. Add stable identity or spawn ID.
5. Expose all active NPCs, preferably all 16 slots, or use an entity encoder.
6. Include size or enough type information to derive footprint mechanics.
7. Keep pending attack style/timing and line of sight.
8. Hard-enforce action masks separately from the observation encoder.

Useful physical/causal features include:

- player HP, prayer, supplies, cooldowns, and position;
- NPC type, HP, position, size, attack timer, telegraph, LOS, and pending hit;
- incoming-hit timeline;
- current target;
- wave and remaining-enemy state.

Avoid solution labels such as:

- correct prayer to select;
- best target;
- “currently safespotted”;
- recommended movement;
- recommended food/potion threshold.

Those labels teach a policy rather than describe the environment.

## Stable slots versus entity encoder

The lowest-risk patch is stable 16-slot encoding. The better long-term architecture is an entity encoder with:

- shared per-NPC embedding;
- pooling or attention over active entities;
- a target head tied directly to entity embeddings.

Do not combine an entity-architecture rewrite with the first reward experiment. Establish a stable-slot baseline first.

---

# Recommended reward redesign

## Preserve the current reward as a legacy control

Do not delete the current implementation or overwrite the only config.

Create an immutable config such as:

```text
config/archive/fight_caves_v35_1_legacy.ini
```

Keep the existing best checkpoint and evaluation results.

Then create a corrected-legacy config that changes only implementation correctness:

```text
config/fight_caves_corrected_legacy.ini
```

This control answers: “What do masks, target identity, seeds, rollout alignment, and metric correctness change while the nominal recipe is held fixed?”

## New production objective

### Terminal objective

Begin with:

- `+1` for cave completion;
- `0` for death/tick-cap if maximizing success probability;
- optionally `-1` for failure if that symmetric margin is explicitly desired.

Do not add a death penalty merely because it feels intuitive. Completion already requires survival. The choice between `0` and `-1` changes the objective and should be treated as an ablation.

### Strategy-neutral progress potential

Define cave progress:

```text
P(s) = ((completed_waves) + current_wave_progress) / 63
```

`current_wave_progress` should reflect mandatory enemy HP eliminated:

- account correctly for Tz-Kek split children;
- on Jad, use Jad HP progress;
- exclude healer HP because killing healers is not required;
- allow Jad healing to decrease progress naturally.

Use potential-based shaping:

```text
r_shaped = r_terminal + beta * (gamma * Phi(next_state) - Phi(state))
```

with `Phi = P`.

Implement absorbing terminal states consistently and unit-test telescoping behavior. For curriculum starts, record the initial potential so comparisons between task distributions remain interpretable.

Suggested initial `beta` values for ablation are small, such as `0.1`, `0.25`, and `0.5`, because the terminal objective should remain dominant. These values are proposed experiment points, not presumed optima.

### Terms to set to zero in the minimal reward

Set the following to zero for the discovery experiment:

- base per-hit/projectile reward;
- NPC-kill reward, if progress potential already accounts for mandatory HP;
- wave-index-scaled wave-clear reward;
- correct-prayer rewards;
- wrong-prayer shaping beyond actual damage/resource consequences;
- kiting reward;
- safespot reward;
- melee-pressure penalty;
- wasted-attack penalty;
- unnecessary-prayer penalty;
- food-waste penalty;
- potion-waste penalty;
- Jad-heal penalty;
- invalid-action penalty after masks are enforced;
- wave-stall penalty;
- generic tick penalty unless speed is explicitly part of the objective.

Keep all channels available for logging even when their training weights are zero.

## Secondary objectives

Only introduce resource use, damage taken, or completion speed after completion performance is strong.

Prefer one of:

- constrained evaluation: maximize success subject to a resource or time threshold;
- lexicographic training: success first, efficiency second;
- a very small secondary coefficient verified not to reduce success or alter desired risk tolerance.

Do not simultaneously add several secondary penalties and then infer which one changed behavior.

---

# Curriculum instead of tactical shaping

A wave-1-only start combined with terminal-only success is an extreme exploration problem. The current `gamma` also places very little weight on rewards thousands of ticks in the future.

Use start-state curriculum rather than behavioral rewards.

A practical curriculum distribution can include:

- Jad-only starts;
- waves 55–63;
- middle-wave starts;
- early-wave starts;
- full wave-1 starts.

Within each stage, randomize plausible:

- rotation;
- player HP and prayer;
- supplies;
- cooldowns;
- wave-local state, when snapshots are valid.

Maintain tasks around a moderate success band, then gradually increase the probability of earlier starts. Always select final checkpoints using a fixed full-cave evaluation suite starting at wave 1.

Curriculum changes where learning begins. It does not prescribe whether the policy should kite, safespot, tank, pray, or prioritize a specific target.

---

# Configuration plan

Create explicit, versioned configs. Never rely on undocumented defaults for important experimental variables.

Recommended files:

```text
config/archive/fight_caves_v35_1_legacy.ini
config/fight_caves_corrected_legacy.ini
config/fight_caves_sparse_curriculum.ini
config/fight_caves_potential_curriculum.ini
config/fight_caves_obs_stable16.ini
```

Add metadata fields or equivalent run tags for:

```text
experiment_version
reward_version
observation_version
action_contract_version
curriculum_version
loadout
global_seed
git_commit
reward_clip_enabled
```

Make supplies explicit in every config. The README describes a no-consumables live diagnostic, while the checked-in config omits `initial_sharks` and `initial_prayer_doses`; the binding defaults missing values to maximum supplies. This documentation/config drift should be eliminated.

At startup, print and log the fully resolved configuration after defaults and environment variables are applied.

---

# Experiment sequence

## Phase 0 — freeze the legacy baseline

- Archive the exact current config.
- Record the exact commit, loadout, seed, observation size, action sizes, and trainer build flags.
- Preserve the current best checkpoint.
- Build a fixed evaluation set with held-out seeds and rotations.
- Record full-cave metrics from the legacy checkpoint.

No learning conclusions should be drawn until metric races are fixed.

## Phase 1 — correctness-only run

Keep:

- current raw reward implementation;
- current reward clipping;
- current PPO hyperparameters;
- current observation dimensions.

Change only:

- hard action masks;
- stable attack target mapping;
- independent environment seeds;
- rollout/reward alignment;
- scalar/vector advantage parity;
- race-free analytics;
- LOS and healer-spawn bugs.

Train from cold start. This run measures the effect of correctness fixes.

## Phase 2 — observation baseline

Starting from the corrected trainer:

- restore incoming-hit aggregates;
- add stable target identity;
- add NPC type;
- expand to 16 stable NPC slots.

Hold rewards and PPO hyperparameters fixed. Train from cold start because observation semantics changed.

## Phase 3 — reward ablations

Compare:

- **R0:** corrected legacy reward;
- **R1:** terminal reward plus curriculum;
- **R2:** terminal reward plus potential progress, no curriculum;
- **R3:** terminal reward plus potential progress and curriculum.

Use the same observation contract and PPO settings for all four.

This establishes whether potential shaping is necessary once exploration is handled by curriculum.

## Phase 4 — grouped observation ablations

Using the selected reward:

- incoming-hit aggregates on/off;
- NPC type on/off;
- 8 sorted slots versus 16 stable slots;
- rotation ID on/off;
- flat encoding versus entity encoder, only after the stable-slot baseline is established.

Do not ablate individual floats in a large combinatorial sweep. Test coherent information groups.

## Phase 5 — PPO simplification and sweep

First establish a vanilla-control configuration:

- replay ratio `1`;
- no prioritized replay;
- no V-trace correction beyond what on-policy PPO requires;
- standard, documented optimizer settings.

Then reintroduce custom components one at a time.

Only after the reward/observation design is selected should a focused sweep cover:

- learning rate;
- gamma;
- GAE lambda;
- entropy coefficient;
- rollout horizon;
- PPO clip coefficient;
- value coefficient and value clipping;
- replay ratio;
- priority alpha/beta, if prioritized replay remains beneficial.

Suggested coarse ranges, to refine after short pilots:

```text
learning_rate: 3e-4, 6e-4, 9e-4
gamma:         0.9975, 0.9990, 0.9995, 0.9998
gae_lambda:    0.95, 0.97, 0.985
horizon:       256, 512, 1024
ent_coef:      retune after hard masks; use normalized per-head entropy if possible
replay_ratio:  1.0 first, then 1.5 and 2.0
prio_alpha:    0 first, then a small targeted comparison
```

Do not sweep optimizer betas, epsilon, reward coefficients, observations, and PPO settings simultaneously.

The existing swept hyperparameters are a useful starting point for the corrected legacy control, but they are not expected to remain optimal after reward scale, masking, observations, and curriculum change.

---

# Evaluation protocol

Use unshaped, externally meaningful metrics:

- full-cave Jad kill rate;
- reach-wave-63 rate;
- conditional Jad conversion: `jad_kill_rate / reach_wave_63_rate`;
- cave completion ticks;
- damage taken;
- food and prayer doses consumed;
- timeout rate;
- performance by loadout;
- performance by rotation and held-out seed.

Select checkpoints primarily by full-cave success, not shaped episodic return.

Use several independent training seeds. Report:

- median;
- interquartile range;
- per-seed curves;
- a fixed number of evaluation episodes;
- confidence intervals where practical.

The curriculum evaluation distribution must be separate from the final full-cave evaluation distribution.

---

# Required tests and acceptance gates

Do not start expensive sweeps until these pass.

## Trainer

- [x] Hard masks produce zero invalid sampled actions.
- [ ] Rollout and PPO masked log-probabilities match a reference implementation.
- [ ] Masked entropy matches a reference implementation.
- [ ] Every environment reward is consumed exactly once by GAE.
- [ ] Horizon-boundary rewards are retained.
- [ ] Terminal rewards at every possible rollout index are retained.
- [ ] Scalar and vector advantage kernels agree.
- [ ] Raw and actual training reward are both logged.
- [ ] Reward clipping fraction is logged.
- [ ] Recurrent state is reset on episode boundaries or intentionally handled and tested.

## Environment

- [ ] Observed target slot always resolves to the same NPC.
- [ ] Joint movement/attack does not change target identity.
- [ ] Large-NPC footprint distances are consistent.
- [ ] More-than-eight-NPC behavior is explicitly tested.
- [ ] Vector environments receive distinct reproducible RNG streams.
- [ ] Ranged/magic NPCs reposition when LOS is blocked.
- [ ] Healers spawn only on valid non-overlapping tiles.
- [ ] Potential progress handles Tz-Kek splitting.
- [ ] Jad healing decreases potential progress.
- [ ] Killing/damaging healers does not directly increase mandatory progress.
- [ ] Global analytics match a single-thread reference.

## Experiment tooling

- [ ] Every run logs resolved config, commit, loadout, seed, reward version, and observation version.
- [ ] Checkpoint selection uses unshaped full-cave metrics.
- [ ] Cold-start and warm-start runs are clearly distinguished.
- [ ] Observation-contract changes invalidate incompatible checkpoints.
- [ ] The fixed evaluation suite is not used for curriculum adaptation.

---

# Proposed coding-agent task order

1. Archive the current config and add run-manifest logging.
2. Add reward-transport and GAE boundary tests.
3. Fix rollout alignment and scalar/vector advantage parity.
4. Implement hard masks in rollout and PPO. Completed 2026-07-22; full-run A/B pending.
5. Fix stable NPC target identity.
6. Fix per-environment seeding.
7. Replace global analytics with race-free reductions.
8. Fix LOS repositioning and healer spawn validation.
9. Restore incoming-hit aggregates.
10. Add NPC type and stable 16-slot observations.
11. Add a configurable reward mode:
    - `legacy`
    - `terminal`
    - `potential`
12. Implement and unit-test progress potential.
13. Add configurable start-wave/start-state curriculum.
14. Run the corrected legacy control.
15. Run reward-mode ablations.
16. Run grouped observation ablations.
17. Establish vanilla PPO control.
18. Run focused PPO sweeps.

Each numbered item should ideally be its own small pull request or logically isolated commit.

---

# What not to do

- Do not tune twenty shaping coefficients before fixing clipping, masks, rollout alignment, target identity, seeds, and metric races.
- Do not delete the existing SOTA config or checkpoint.
- Do not select policies using shaped episodic reward.
- Do not assume a reward coefficient is influential merely because its raw logged total is large.
- Do not add more tactic labels to compensate for missing or unstable observations.
- Do not use invalid-action penalties as a substitute for hard action masks.
- Do not run a single giant sweep over reward, observations, architecture, and PPO.
- Do not warm-start across changed observation semantics and treat the result as a clean comparison.
- Do not infer general optimality from one loadout.
- Do not treat curriculum as undesirable guidance; it changes exploration difficulty without specifying tactics.

---

## Final recommendation

The correct sequence is:

```text
freeze legacy
→ fix correctness
→ fix observation/action semantics
→ prune to terminal + optional progress potential
→ add curriculum
→ run grouped ablations
→ sweep PPO
```

The permanent reward should describe **what success is**, not **how a human expects success to be achieved**. The observation should make the world Markov enough to solve, action masks should make illegal choices impossible, and curriculum should make successful trajectories discoverable. Once those pieces are correct, hyperparameter sweeps become informative rather than compensating for structural bugs.
