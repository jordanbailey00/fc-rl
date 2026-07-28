# v3 Simple Reward Hyperparameter Sweep Plan

## Sweep Results

Status: Completed successfully on 2026-07-27

All 140 configured runs finished in W&B, produced valid `jad_kill_rate`
histories, and reached their expected horizon-rounded 750M-step budgets. The
following runs are ranked by the highest `jad_kill_rate` recorded at any point
in each complete W&B history. Final evaluation is included to show how much of
the peak performance each policy retained at the end of training.

| Rank | W&B run | Peak | Final evaluation |
|---:|---|---:|---:|
| 1 | [mmyxbyn4](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/mmyxbyn4) | **98.0952%** | 95.5813% |
| 2 | [il0xq0uf](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/il0xq0uf) | **97.8070%** | 96.0813% |
| 3 | [bixy85bf](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/bixy85bf) | **97.1831%** | 92.6293% |
| 4 | [xe9rvvfj](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/xe9rvvfj) | **96.8504%** | 91.2122% |
| 5 | [ldovi5g0](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/ldovi5g0) | **96.4602%** | 91.3785% |
| 6 | [xbz4zm0j](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/xbz4zm0j) | **96.3964%** | 91.1726% |
| 7 | [e03dqmuz](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/e03dqmuz) | **95.9350%** | 87.6508% |
| 8 | [frk81x9g](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/frk81x9g) | **95.0192%** | 90.0555% |
| 9 | [x4orpdpz](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/x4orpdpz) | **94.4954%** | 83.5648% |
| 10 | [kw7ayozo](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/kw7ayozo) | **94.2623%** | 86.1540% |
| 11 | [0gj6sdcg](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/0gj6sdcg) | **94.1909%** | 90.6478% |
| 12 | [2byn1gfh](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/2byn1gfh) | **93.9130%** | 86.7177% |
| 13 | [olf9nt2m](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/olf9nt2m) | **93.2432%** | 83.6684% |
| 14 | [snba6rho](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/snba6rho) | **93.1174%** | 90.3677% |
| 15 | [cyjt50sz](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/cyjt50sz) | **93.1035%** | 86.0310% |

### High-Performer Convergence

The top 15 form a clear common configuration family rather than 15 unrelated
solutions. After normalizing every swept parameter by its configured search
range and distribution, their mean pairwise distance is `0.133`, compared with
`0.263` across all 140 runs. The top group is therefore roughly half as spread
out as the complete sweep.

| Hyperparameter | Top-15 pattern | Interpretation |
|---|---|---|
| `learning_rate` | Median `0.004`; 8/15 at the `0.004` upper bound; all at least `0.00208` | Strong preference for a substantially higher learning rate than the old baseline. |
| `ent_coef` | Median `0.0005`; 8/15 at the lower bound; 14/15 below `0.001` | Strong convergence on much less random exploration. |
| `gamma` | `0.99826` to `0.99983`; no meaningful all-run rank correlation | Remains variable and is not resolved by this sweep. |
| `gae_lambda` | 14/15 at the `0.90` lower bound; remaining run `0.9315` | Very strong lower-bound convergence. |
| `horizon` | 15/15 use `256` | Complete convergence among the top 15. |
| `minibatch_size` | 10/15 use `32768`, 4 use `16384`, 1 uses `8192` | Clear preference for larger minibatches, though not a single exact value. |
| `replay_ratio` | Median `2.02`; range `1.38` to `2.45` | Moderate convergence around roughly two replay passes. |
| `clip_coef` | 11/15 at the `0.05` lower bound; maximum `0.1345` | Strong preference for conservative policy updates. |
| `prio_alpha` | Median `0.943`; range `0.781` to `1.0` | Consistent preference for strong priority weighting. |
| `prio_beta0` | Median `0.169`; range `0.0` to `0.304` | Low-to-moderate values work; no exact convergence point yet. |
| `vtrace_rho_clip` | Median `2.0`; 10/15 at the `2.0` upper bound; minimum `1.624` | Strong upper-bound convergence toward accepting larger corrections. |
| `vtrace_c_clip` | Median `0.983`; range `0.777` to `1.337` | Stable cluster around approximately `1.0`. |

The strongest shared recipe is: high learning rate, very low entropy, low GAE
lambda, horizon 256, a large minibatch, low policy clipping, high priority
alpha, high V-trace rho, and V-trace c near 1.0. Gamma, replay ratio,
`prio_beta0`, and exact minibatch size still vary enough that this sweep does
not identify one uniquely optimal value for them.

Peak performance and final stability are also not identical. The median
peak-to-final decline among the top 15 is `5.22` percentage points, ranging
from `1.73` to `10.93`. `mmyxbyn4` has the highest peak, while `il0xq0uf` has
the highest final evaluation and the smaller decline of the top two. Finalist
selection should therefore include both, followed by independent-seed reruns
rather than choosing solely from the single highest peak.

## Objective

Find a policy that reliably reaches wave 63, kills Jad, and completes the Fight
Caves using the current v3 simple-reward environment. Final selection is based
on `jad_kill_rate`, with `reached_wave_63`, `cave_progress`, stability, and
multi-seed reproducibility used as supporting criteria.

The fixed pre-sweep benchmark is W&B run `8rg9wurg`. Its exact configuration is
the live `runescape-rl/config/fight_caves.ini` with SHA-256
`62edaa59d0f3cfbcacc9bcaa2aa5b0da8d68fa81d21f283afc7676017fce7057`.

## Fixed Experimental Contract

Keep these unchanged throughout all trainer-hyperparameter stages:

- Current committed Fight Caves core, backend, and native hard action masks.
- SOTA Twisted Bow loadout through `FC_LOADOUT_SOTA_TBOW`.
- No food and no prayer potions.
- v7 observation contract and three-head no-supplies action contract.
- v3 reward contract, including `shape_npc_heal_penalty=-0.005`.
- Native trainer seed `73` during adaptive searches.
- One GPU, 4,096 agents, two buffers, and 16 environment threads unless a
  later stage explicitly tests batch size.
- A fixed 750M-step budget for every adaptive-sweep trial.
- Final evaluation over 10,000 episodes.

Do not mix reward, observation, loadout, action, backend, or core changes into a
trainer sweep. Record every W&B run ID and effective configuration before
starting the next stage.

## Stage 1: Core Learning Sweep

Status: Completed successfully on 2026-07-27

Configuration:
`runescape-rl/config/experiments/v3_simple_reward_sweep1.ini`

| Field | Value |
|---|---:|
| Method | PufferLib `Protein` |
| Trials | 140 |
| Timesteps per trial | 750,000,000 |
| Primary metric | `jad_kill_rate` |
| Metric transform | `linear` |
| Goal | maximize |
| Native trainer seed | 73 |
| Concurrent trials | 1 |

`linear` is intentional. Puffer's `percentile` mode logit-transforms a metric
and clips very small probabilities; that would erase useful distinctions among
sub-1% Jad-kill rates. Early stopping uses a conservative `0.1` quantile so a
policy has time to reach late waves before being judged.

Only these parameters are exposed through `sweep_only`:

| Hyperparameter | Distribution | Minimum | Maximum | Baseline |
|---|---|---:|---:|---:|
| `train.learning_rate` | log normal | 0.00015 | 0.004 | 0.0009393226 |
| `train.ent_coef` | log normal | 0.0005 | 0.04 | 0.0064446392 |
| `train.gamma` | logit normal | 0.995 | 0.99995 | 0.9995188470 |
| `train.gae_lambda` | logit normal | 0.90 | 0.9999 | 0.9995 |
| `train.horizon` | powers of two | 128 | 1024 | 256 |
| `train.minibatch_size` | powers of two | 4096 | 32768 | 4096 |
| `train.replay_ratio` | uniform | 0.35 | 3.0 | 1.3335898490 |
| `train.clip_coef` | uniform | 0.05 | 0.35 | 0.1323603389 |
| `train.prio_alpha` | uniform | 0.1 | 1.0 | 0.9682355929 |
| `train.prio_beta0` | uniform | 0.0 | 0.8 | 0.0 |
| `train.vtrace_rho_clip` | uniform | 0.1 | 2.0 | 0.5 |
| `train.vtrace_c_clip` | uniform | 0.1 | 2.0 | 0.5037274755 |

Puffer runs the unchanged baseline for the first two trials, then uses Protein
suggestions. These duplicate baseline trials validate deterministic setup; they
are not independent-seed replications.

### Estimated Runtime

The prior 96-run 750M Fight Caves sweep took `16.92` total GPU-hours, averaging
`10.58` minutes per trial with a `6.90` to `14.05` minute 10th-to-90th
percentile range. At the same average speed, 140 trials would take about `24.7`
hours. Stage 1 deliberately uses wider replay, horizon, and minibatch ranges, so
the practical estimate is `30-45 hours` on the current single GPU. Allow up to
roughly `60 hours` if Protein selects many high-replay or otherwise slow trials.

Stage 1 analysis must rank final policies by Jad kill/completion rate, then use
wave-63 reach, cave progress, Jad conversion, healing behavior, timeout/no-op
behavior, prayer use, stability over training, SPS, and runtime to explain each
result. Do not choose a winner from cave progress alone.

## Stage 2: Value And Optimizer Sweep

Status: Planned after Stage 1 finalist selection

Use the strongest robust Stage 1 trainer configuration as the fixed center.
Sweep only:

| Hyperparameter | Planned range |
|---|---:|
| `train.vf_coef` | 0.4 to 2.5 |
| `train.vf_clip_coef` | 0.05 to 0.4 |
| `train.max_grad_norm` | 0.1 to 1.0 |
| `train.beta1` | 0.85 to 0.99 |

Keep Stage 1's selected learning rate, entropy, discounting, rollout, replay,
priority, and V-trace values fixed so this stage measures critic and optimizer
effects cleanly.

## Stage 3: Learning-Rate Schedule

Status: Planned after Stage 2

Run controlled schedule variants around the best trainer configuration:

| Variant | `anneal_lr` | `min_lr_ratio` |
|---|---:|---:|
| Constant baseline | 0 | ignored |
| Decay to zero | 1 | 0.0 |
| Low final rate | 1 | 0.05 |
| Moderate final rate | 1 | 0.10 |
| Active final rate | 1 | 0.25 |

This stage tests whether reducing update size late in training preserves a
policy after it learns deep-wave or Jad behavior. Keep total timesteps fixed
because Puffer schedules annealing against the configured run budget.

## Stage 4: Architecture And Batch Sweep

Status: Planned after trainer settings stabilize

Test architecture and rollout capacity separately from the trainer search:

| Hyperparameter | Planned values |
|---|---|
| `policy.hidden_size` | 128, 256, 512 |
| `policy.num_layers` | 2, 3, 4 |
| `vec.total_agents` | 2048, 4096, 8192 |

Compare Jad kills, robustness, parameter count, VRAM, SPS, and wall-clock cost.
Do not select a larger network from cave progress alone if it does not improve
Jad conversion or reproducibility.

## Final Confirmation

1. Rerun the strongest configurations at the same 750M budget with at least
   seeds `73`, `101`, and `202`.
2. Select finalists using aggregate Jad kill/completion rate and consistency,
   not one lucky run.
3. Run the top two or three configurations at fixed 1.5B and/or 2.5B budgets.
4. Account for Puffer's priority-beta schedule when comparing different total
   timestep budgets; a longer configured run is not merely an identical run
   allowed to continue longer.
5. Rerun a selected sweep configuration outside sweep mode when checkpoints
   are needed. Native Puffer intentionally does not save/upload trial models
   while `sweep_obj` is active.

## Parameters Deliberately Not Swept

| Parameter | Reason |
|---|---|
| `total_timesteps` | Fixed compute budget and priority schedule are required for fair comparison. |
| `seed` | A replication axis, not an optimization target. |
| `beta2`, `eps` | Parsed but unused by this native CUDA optimizer. |
| `expansion_factor` | Ignored by the current native backend. |
| `num_buffers`, `num_threads`, `gpus`, `cudagraphs` | Hardware and throughput controls; benchmark separately. |
| `checkpoint_interval`, `eval_episodes`, `profile` | Logging, evaluation, and instrumentation controls. |
| `reset_state` | Keep true; disabled training behavior is untested and can mix recurrent state. |
| Rewards and observation ablations | They change the learning objective/task and require separate experiments. |
| Loadout, backend, core, actions, and masks | Frozen to preserve the controlled v3 comparison. |
