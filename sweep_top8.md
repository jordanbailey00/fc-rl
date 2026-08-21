# V4.5 750M Sweep: Top Eight Runs

This document records the eight highest final-Jad performers from the
`v45_value_arch_batch_sweep_750m_130_corrected` W&B sweep. Every run completed
750 million agent steps and used the same Fight Caves environment, observation,
action, reward, and training contract. Only the seven hyperparameters in the
per-run configuration table were swept.

The W&B runs share the tag `v4.5_sweep_top8_review`.

## Main conclusion

The configurations are not wildly different. The sweep narrowed strongly on
one architecture:

- All eight runs use three recurrent layers.
- Seven of eight use a hidden size of 512.
- Six of eight use 4,096 agents.
- Five runs use exactly the `512 hidden / 3 layers / 4,096 agents` architecture.
- The two 8,192-agent finalists both use `512 hidden / 3 layers` and form a
  recognizable high-ceiling, late-learning cluster.
- `4pfap621` is the only architectural outlier: it uses 256 hidden units and is
  the compute-efficient finalist.

The architecture and agent count converged much more strongly than the value
optimizer settings. Within the five conventional `512 / 3 / 4,096` finalists,
`vf_clip_coef` is confined to approximately 0.120-0.185 and `beta1` to
approximately 0.970-0.984. `vf_coef` remains much less settled, ranging from
0.4 to 1.49, which suggests a broad viable region rather than one precise value.

The two successful 8,192-agent configurations occupy a different region: high
`vf_coef` (1.48-1.82), high `vf_clip_coef` (0.389-0.4), and moderately high
gradient clipping (0.379-0.489). Both learned very late, so that cluster needs
multi-seed replication before it should replace the more conventional region.

## Shared fixed configuration

These values were identical across all eight runs. Combining this block with
one row from the swept-parameter table gives the relevant training configuration
for that run.

```ini
runtime_seed = 73
train.seed = 42
train.total_timesteps = 750000000
train.gpus = 1

torch.network = MinGRU
torch.encoder = DefaultEncoder
torch.decoder = DefaultDecoder
policy.expansion_factor = 1

vec.num_buffers = 2
vec.num_threads = 16

train.learning_rate = 0.00207567504650331
train.anneal_lr = 0
train.min_lr_ratio = 0.0
train.gamma = 0.9991261141073255
train.gae_lambda = 0.9
train.replay_ratio = 2.055184291514704
train.clip_coef = 0.05
train.ent_coef = 0.000625460620549345
train.beta2 = 0.9995810484472892
train.eps = 1e-10
train.minibatch_size = 32768
train.horizon = 256
train.vtrace_rho_clip = 2.0
train.vtrace_c_clip = 0.9746667741536915
train.prio_alpha = 0.9110743956381228
train.prio_beta0 = 0.2258134371255269
```

All environment and contract settings were also identical. Their source is
`runescape-rl/config/experiments/fight_caves_v45_value_arch_batch_sweep_750m.ini`.

## Exact swept hyperparameters

| Final rank | Run | Hidden | Layers | Agents | `vf_coef` | `vf_clip_coef` | `max_grad_norm` | `beta1` |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | `pozhjer2` | 512 | 3 | 8,192 | 1.8243429381 | 0.3888287699 | 0.4893593831 | 0.9446346008 |
| 2 | `2xvu5y6a` | 512 | 3 | 4,096 | 0.8474506971 | 0.1847517992 | 0.2404474725 | 0.9713133016 |
| 3 | `1nvvx5qu` | 512 | 3 | 4,096 | 0.9336215312 | 0.1679154628 | 0.1418276517 | 0.9832670364 |
| 4 | `j9ddz37u` | 512 | 3 | 8,192 | 1.4806917954 | 0.4000000000 | 0.3786414236 | 0.9696150575 |
| 5 | `c359wtxd` | 512 | 3 | 4,096 | 0.4191263550 | 0.1345112616 | 0.1000000000 | 0.9701210751 |
| 6 | `sacqd01s` | 512 | 3 | 4,096 | 0.4000000000 | 0.1447507965 | 0.1000000000 | 0.9837612602 |
| 7 | `4pfap621` | 256 | 3 | 4,096 | 0.9274496686 | 0.0741388368 | 0.1000000000 | 0.9877729688 |
| 8 | `pj0azlw6` | 512 | 3 | 4,096 | 1.4912812282 | 0.1196776731 | 0.1000000000 | 0.9829191138 |

Across the finalists, the swept ranges are:

| Parameter | Finalist range | Interpretation |
|---|---:|---|
| Hidden size | 256-512 | Strong preference for 512; 256 remains efficient and competitive |
| Layers | 3 only | Fully converged among the finalists |
| Agents | 4,096-8,192 | 4,096 is robust; 8,192 is the late-learning/high-ceiling branch |
| `vf_coef` | 0.4-1.8243 | Broad viable range; no single narrow optimum |
| `vf_clip_coef` | 0.0741-0.4 | Conventional cluster is much narrower; 8,192 branch is near 0.4 |
| `max_grad_norm` | 0.1-0.4894 | Four finalists use the 0.1 lower boundary |
| `beta1` | 0.9446-0.9878 | Most conventional finalists cluster near 0.97-0.984 |

## Performance and learning-curve data

`Sustained 90%` is the first of five consecutive W&B reports with at least a
90% Jad kill rate. `Late mean` and `late q10` use all logged Jad measurements in
the final quarter of training. These distinguish a stable policy from a policy
that only became strong at the end.

| Run | Final Jad | Wave 63 reached | First 90% | Sustained 90% | Late mean | Late q10 | Eval episodes |
|---|---:|---:|---:|---:|---:|---:|---:|
| `pozhjer2` | **96.41%** | **97.85%** | 692.1M | 692.1M | 56.80% | 9.04% | 10,070 |
| `2xvu5y6a` | 94.94% | 95.74% | 338.7M | 363.9M | 94.01% | 91.64% | 10,171 |
| `1nvvx5qu` | 94.79% | 95.94% | 316.7M | 338.7M | 94.98% | **93.20%** | 10,170 |
| `j9ddz37u` | 94.67% | 97.04% | 696.3M | 713.0M | 73.25% | 32.24% | 10,303 |
| `c359wtxd` | 94.54% | 95.65% | 363.9M | 376.4M | 94.61% | 92.79% | 10,159 |
| `sacqd01s` | 94.32% | 95.39% | 396.4M | 412.1M | 93.16% | 91.10% | 10,078 |
| `4pfap621` | 94.29% | 95.75% | 384.8M | 395.3M | 93.50% | 91.25% | 10,099 |
| `pj0azlw6` | 94.27% | 94.73% | 319.8M | 330.3M | 93.86% | 91.51% | 10,122 |

The final evaluation differences among the conventional top runs are small.
Their approximate binomial confidence intervals overlap, so their exact final
ordering should not be treated as proven until the configurations are retrained
across multiple seeds. `pozhjer2` has a clearly higher terminal result in this
evaluation, but its late transition makes reproducibility the central question.

## Normalized behavior data

These values come from the final post-training evaluation. Rates normalized by
ticks are more comparable than raw counts because episode lengths differ.

| Run | Move | Run | Target held | Target in range/LOS | Attack when ready | Prayer active | Prayer switches/tick |
|---|---:|---:|---:|---:|---:|---:|---:|
| `pozhjer2` | 94.68% | 53.41% | 74.00% | 75.37% | 89.21% | 86.29% | 96.76% |
| `2xvu5y6a` | 93.28% | 57.06% | 83.91% | 83.95% | 96.34% | 91.49% | 97.12% |
| `1nvvx5qu` | 89.56% | 43.60% | 79.43% | 84.24% | 95.55% | 88.74% | 95.46% |
| `j9ddz37u` | 94.37% | 57.56% | 71.12% | 85.42% | 96.03% | 82.84% | 94.32% |
| `c359wtxd` | 93.31% | 54.02% | 82.10% | 83.27% | **96.90%** | 88.86% | 97.51% |
| `sacqd01s` | 92.87% | 45.19% | 77.16% | 79.74% | 95.31% | 83.48% | 93.46% |
| `4pfap621` | 88.35% | 42.69% | **85.08%** | 75.03% | 94.30% | 94.09% | 98.83% |
| `pj0azlw6` | 92.26% | 39.76% | 75.48% | 84.12% | 95.78% | **97.21%** | **98.96%** |

All eight issue prayer commands on approximately 97-100% of ticks and produce
essentially zero invalid actions. They are variations of the same broad learned
solution: near-constant movement, aggressive prayer flicking, and persistent
ranged attacks.

## Prayer, damage, and healing data

| Run | Correct prayer / 1K ticks | Wrong hits / 1K | No-prayer hits / 1K | Damage taken | Total NPC healing | Jad healing |
|---|---:|---:|---:|---:|---:|---:|
| `pozhjer2` | 247.43 | 21.10 | 3.95 | **794** | 3,293 | 2,003 |
| `2xvu5y6a` | 280.54 | 31.74 | 1.53 | 903 | 1,769 | 731 |
| `1nvvx5qu` | **295.18** | 24.67 | 3.26 | 878 | 1,807 | 850 |
| `j9ddz37u` | 266.04 | 27.24 | 6.94 | 935 | 2,661 | 1,567 |
| `c359wtxd` | 290.58 | 32.41 | 2.98 | 906 | **1,557** | **660** |
| `sacqd01s` | 284.10 | 23.15 | 3.19 | 905 | 2,220 | 1,161 |
| `4pfap621` | 269.03 | 36.34 | 1.23 | 907 | 1,955 | 722 |
| `pj0azlw6` | 289.34 | 26.65 | **0.98** | 880 | 2,460 | 1,422 |

## Where episodes fail

`Pre-Jad failure` is one minus the wave-63 reach rate. `Jad failure after reach`
is the fraction of wave-63 episodes that did not end in a Jad kill.

| Run | Pre-Jad failure | Jad failure after reach | Behavioral implication |
|---|---:|---:|---|
| `pozhjer2` | **2.15%** | 1.47% | Best regular-wave survival; tolerates slow kills and healing |
| `2xvu5y6a` | 4.26% | 0.84% | Aggressive and reliable once it reaches Jad |
| `1nvvx5qu` | 4.06% | 1.20% | Most balanced complete-cave behavior |
| `j9ddz37u` | 2.96% | 2.44% | Strong regular-wave survival but weakest Jad closure here |
| `c359wtxd` | 4.35% | 1.16% | Highest combat tempo and lowest healing allowed |
| `sacqd01s` | 4.61% | 1.11% | Lower prayer-switch frequency and moderate combat pressure |
| `4pfap621` | 4.25% | 1.53% | Most target-committed; less precise prayer selection |
| `pj0azlw6` | 5.27% | **0.49%** | Weakest pre-Jad survival but best Jad finisher |

## Per-run interpretation and W&B links

### `pozhjer2` - highest terminal score

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/pozhjer2>
- Role: high-ceiling 8,192-agent candidate.
- Behavior: mobile and survival-oriented, with lower target pressure and much
  more tolerated NPC/Jad healing.
- Risk: only became strong near the end of the 750M-step budget.

### `2xvu5y6a` - aggressive conventional candidate

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/2xvu5y6a>
- Role: high-performing `512 / 3 / 4,096` candidate.
- Behavior: high target uptime, high run use, and strong attack tempo.
- Risk: slightly weaker prayer accuracy and regular-wave survival than the most
  balanced candidate.

### `1nvvx5qu` - strongest all-around candidate

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/1nvvx5qu>
- Role: current recommended default.
- Behavior: best normalized correct-prayer count, low damage, good target
  availability, and no late-training collapse.
- Strength: highest final-quarter consistency floor.

### `j9ddz37u` - second 8,192-agent candidate

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/j9ddz37u>
- Role: replication evidence for the high-ceiling 8,192-agent region.
- Behavior: highly mobile with low target-held time and high tolerated healing.
- Risk: late learning and the highest conditional Jad failure among these runs.

### `c359wtxd` - highest combat tempo

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/c359wtxd>
- Role: fast, aggressive conventional candidate.
- Behavior: highest attack-when-ready rate and least NPC/Jad healing allowed.
- Risk: relatively high wrong-prayer hit rate.

### `sacqd01s` - lower-switch defensive variant

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/sacqd01s>
- Role: stable conventional alternative.
- Behavior: lowest prayer-switch rate and relatively few wrong-prayer hits, but
  permits more healing than the faster conventional policies.

### `4pfap621` - compute-efficient candidate

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/4pfap621>
- Role: only 256-hidden finalist and substantially faster to train.
- Behavior: highest target-held rate and very high prayer activity, but the
  highest wrong-prayer hit rate and more target time outside range/LOS.

### `pj0azlw6` - Jad-finishing specialist

- W&B: <https://wandb.ai/jbailey8531-oakton-college/fight+caves+rl/runs/pj0azlw6>
- Role: fast-learning conventional alternative.
- Behavior: highest prayer uptime, lowest no-prayer hit rate, and best Jad kill
  rate conditional on reaching wave 63.
- Risk: lowest wave-63 reach rate of these eight.

## Recommended validation set

The eight finalists do not all need equal follow-up. A compact validation set
that preserves the meaningful regimes is:

1. `1nvvx5qu` for the robust `512 / 3 / 4,096` region.
2. `pozhjer2` for the high-ceiling `512 / 3 / 8,192` region.
3. `4pfap621` for the compute-efficient `256 / 3 / 4,096` region.
4. `2xvu5y6a` as a second conventional high performer.

These should be retrained with saved checkpoints and multiple seeds. The sweep
used the same seed for every run and did not retain sweep checkpoints, so it can
identify promising configurations but cannot establish seed robustness or
support direct visual replay of the original policies.
