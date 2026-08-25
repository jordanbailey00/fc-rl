# 100M Refactor Training Baseline

This is the pre-refactor behavioral baseline for the cleanup work tracked in
`refactor.md`. After each behavior-preserving refactor, run the same dedicated
100M configuration from a cold start and compare its final W&B summary with the
values below. Deterministic core/replay tests remain the primary correctness
guardrail; this training run is an additional end-to-end regression check.

## Run identity

| Field | Baseline |
| --- | --- |
| W&B run | [`m916qfsv`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/m916qfsv) (`brisk-snow-1225`) |
| Tag | `v4.5_refactor_baseline_100m_pre` |
| Git commit | `e32d718fc79661896a04903e0780b10107ddf0fb` |
| Start time | `2026-08-24T23:25:37Z` |
| Requested steps | `100,000,000` |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Cold start | Yes |
| Seed | `73` |
| Active loadout | `FC_LOADOUT_SOTA_TBOW` |
| Config | `runescape-rl/config/experiments/fight_caves_txqsiahp_refactor_baseline_100m.ini` |
| Config difference from canonical `txqsiahp` baseline | Only `train.total_timesteps`: `750,000,000` to `100,000,000` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `7f590ca318b4acc9bfb54eb83583b3cc4ea56ee6bf08904e3f3ed5b346a10d87` |
| Backend source SHA-256 | `bf5adc455ef657f15a1b4b140119d4c3b16be3c45cee6c71f2b7425410b71d54` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260824T232534Z-train-975255.json` |

The policy is a 512-wide, three-layer MinGRU with 4,096 agents. Observation,
action, reward, and Prayer-timing versions are unchanged from `txqsiahp`.

## Final outcome and progression

These are the exact final W&B summary values, not values transcribed from an
intermediate terminal display.

| Metric | Value |
| --- | ---: |
| Episodes logged (`env/n`) | 10,434 |
| Episode length | 1,803.967651 |
| Wave reached | 31.117405 |
| NPCs slayed | 119.117310 |
| Cave progress | 0.483755648 |
| Current-wave progress | 0.359162390 |
| Player death rate | 1.000000 |
| Reached wave 63 | 0.000000 |
| Jad kill rate | 0.000000 |
| Required work at start | 1,617.213013 |
| Required work remaining | 1,038.404297 |
| Gross damage dealt | 41,753.398438 |
| Gross damage/net-progress ratio | 1.306380510 |

At 100M steps this policy has not reached Jad. This baseline is therefore an
early/mid-wave behavior regression check, not a Jad-performance benchmark.

## Combat and Prayer behavior

| Metric | Value |
| --- | ---: |
| Average damage taken | 1,308.232666 |
| Damage blocked | 2,885.918213 |
| Correct Prayer | 240.551468 |
| Wrong-Prayer hits | 119.020897 |
| No-Prayer hits | 49.395916 |
| Prayer switches | 1,570.822144 |
| Melee Prayer uptime | 0.348955959 |
| Ranged Prayer uptime | 0.352579474 |
| Magic Prayer uptime | 0.120798074 |
| Attack-when-ready rate | 0.876631320 |
| Tok-Xil melee ticks | 16.151524 |
| Ket-Zek melee ticks | 0.780525 |
| Maximum wave ticks | 128.968658 |
| Wave containing maximum | 26.562775 |
| NPC healing total | 1,688.041016 |
| Yt-MejKot healing total | 1,688.041016 |
| Jad healing total | 0.000000 |

## Action and targeting behavior

| Metric | Value |
| --- | ---: |
| Move idle ticks | 77.991661 |
| Move walk ticks | 791.520691 |
| Move run ticks | 934.453796 |
| Attack-none ticks | 503.819244 |
| Attack-target ticks | 1,300.146973 |
| Prayer-noop ticks | 107.917290 |
| Prayer-command ticks | 1,696.050171 |
| Target-held ticks | 1,307.495239 |
| No-target ticks | 496.470947 |
| Target in range and LOS ticks | 1,075.071655 |
| Target out of range or LOS ticks | 232.423523 |
| Attack-cooldown wait ticks | 723.925903 |
| Ready but no attack ticks | 0.000000 |
| Invalid move | 0.000000 |
| Invalid attack | 0.000000 |
| Invalid Prayer | 0.000000 |

## Per-NPC combat behavior

| NPC | Damage | Resolved hits | Damaging hits | Attack cycles | Target ticks |
| --- | ---: | ---: | ---: | ---: | ---: |
| Tz-Kih | 3,642.275635 | 44.949875 | 42.533066 | 44.951794 | 159.655548 |
| Tz-Kek | 5,327.041016 | 54.492046 | 50.554726 | 54.492046 | 277.153534 |
| Small Tz-Kek | 6,012.098145 | 74.315315 | 70.304771 | 74.315315 | 268.934448 |
| Tok-Xil | 8,773.945312 | 72.007095 | 64.801804 | 72.007286 | 273.215637 |
| Yt-MejKot | 17,162.414062 | 101.585876 | 87.891701 | 101.585968 | 312.825775 |
| Ket-Zek | 835.588440 | 3.674046 | 2.978340 | 3.793368 | 15.710274 |
| TzTok-Jad | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| Yt-HurKot | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |

## Reward summary

| Metric | Value |
| --- | ---: |
| Pre-clip reward | 29.276245 |
| Post-clip reward | 29.351397 |
| Progress reward total | 31.963959 |
| Player-death reward total | -1.000000 |
| Prayer-lost reward total | -1.088354 |
| Damage-taken reward total | -0.330362 |
| NPC-heal reward total | -0.085040 |
| Tick-penalty reward total | -0.180399 |
| No-attack reward total | -0.003811 |
| No-progress reward total | -0.000201 |
| Invalid-action reward total | 0.000000 |
| Cave-complete reward total | 0.000000 |
| Jad-kill reward total | 0.000000 |
| Correct-Jad-Prayer reward total | 0.000000 |
| Correct-danger-Prayer reward total | 0.000000 |
| Wave-clear reward total | 0.000000 |
| Jad-heal reward total | 0.000000 |
| Wave-stall reward total | 0.000000 |
| Unnecessary-Prayer reward total | 0.000000 |

## Training and performance summary

| Metric | Value |
| --- | ---: |
| SPS | 1,027,114 |
| Training uptime | 98.871387 seconds |
| W&B runtime including setup/sync | 115.789678 seconds |
| Policy loss | 0.007506505 |
| Value loss | 0.006997682 |
| Entropy | 5.048208237 |
| Total loss | 0.010882234 |
| KL | 0.001431169 |
| Old KL | 0.001512979 |
| Clip fraction | 0.279610515 |

## Comparison 1: unused training renderer removed

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`lfdszoiz`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/lfdszoiz) (`fresh-shadow-1226`) |
| Tag | `v4.5_refactor_training_renderer_removed_100m` |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `e1de1e38783ec9c3f85462ccb728745de5c0af38f87ba3d7777646aa8e620430` |
| Backend source SHA-256 | `3df0c77ba08d94be6fd8fd534e605aafe19efea7836e144cc5b93a536271cd08` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260824T233614Z-train-982164.json` |

Result: behavioral parity is exact. Both W&B summaries contain the same 150
keys. All 124 `env/*` metrics are byte-for-byte equal, as are agent steps,
epoch, and every recorded policy/value/entropy/KL/loss value. In total, 136 of
150 summary values are exact matches. The 14 differences are limited to
timestamps, wall-clock performance, throughput, and CPU/GPU memory/utilization
telemetry.

The renderer removal changed the backend source and binary hashes as expected,
but did not change the model contract or training behavior. Throughput was
1,027,114 SPS before and 962,000 SPS after (6.34% lower in this single sample),
and training uptime was 98.871387 seconds before and 103.138911 seconds after.
Those timing differences are normal run-to-run machine-load noise and are not
evidence of behavioral drift.

## Comparison 2: asset and animated-atlas loaders consolidated

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`7vgbe9bb`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/7vgbe9bb) (`desert-fog-1227`) |
| Tag | `v4.5_refactor_asset_atlas_loaders_100m` |
| Git commit at run start | `0be25564a941e63d4c3a92bf14ed7a8b3b329f0a` plus the documented uncommitted viewer-only refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `e1de1e38783ec9c3f85462ccb728745de5c0af38f87ba3d7777646aa8e620430` |
| Backend source SHA-256 | `3df0c77ba08d94be6fd8fd534e605aafe19efea7836e144cc5b93a536271cd08` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260824T235422Z-train-995703.json` |

Result: behavioral parity is exact against both the original baseline
`m916qfsv` and the post-renderer-removal run `lfdszoiz`. Each W&B summary has
the same 150 keys. All 124 `env/*` metrics are byte-for-byte equal, as are
agent steps, epoch, and every recorded policy/value/entropy/KL/loss value. In
each comparison, 136 of 150 summary values are exact matches. The 14 differences
are limited to timestamps, wall-clock performance, throughput, and CPU/GPU
memory/utilization telemetry.

The backend binary, source hash, model contract, and training configuration are
identical to Comparison 1 because this refactor changes only viewer resource
loading. Throughput was 956,177 SPS and training uptime was 102.337610 seconds.
That is 0.61% lower SPS and 0.78% faster uptime than Comparison 1; the small
timing variation is normal machine-load noise.

## Comparison 3: arena map loaders consolidated

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`dgykda7r`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/dgykda7r) (`pleasant-energy-1228`) |
| Tag | `v4.5_refactor_arena_map_loaders_100m` |
| Git commit at run start | `0be25564a941e63d4c3a92bf14ed7a8b3b329f0a` plus the documented uncommitted refactors |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `0587a3fe57b2dc6b3fc24ca479bc36092ce28389ead0fe76164b3a98e0340578` |
| Backend source SHA-256 | `281f1012c04ccf38ae3ca0ca76edbc040265e36779d23fa77eba3cfdff335a86` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T000959Z-train-1008510.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and both prior comparison runs. All 124 `env/*` metrics are byte-for-byte
equal, as are agent steps, epoch, and every recorded
policy/value/entropy/KL/loss value. Against the original baseline, 136 of 150
summary values are exact matches; the 14 differences are only timestamps,
wall-clock performance, throughput, and CPU/GPU memory/utilization telemetry.

This core refactor caused the training backend to rebuild, so its source and
binary hashes changed as expected. The model contract and training
configuration remained identical. Throughput was 958,764 SPS and training
uptime was 102.297909 seconds, effectively unchanged from Comparison 2.

## Comparison 4: obsolete core APIs removed

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`4qmy1v9y`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/4qmy1v9y) (`feasible-snowball-1229`) |
| Tag | `v4.5_refactor_obsolete_core_apis_100m` |
| Git commit at run start | `b5758392addf8310b2246b91adc0e721f76afe82` plus the documented uncommitted API cleanup |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `425f43fd131406f40ae46b4392345ccf48cbae60bfbee16af54cfb9c1406cafe` |
| Backend source SHA-256 | `2c989a1bdca7b870a11f5694ae3fa53e3fa6c66526011c62a591ce3a4ef5576d` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T002638Z-train-1020502.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and all prior comparison runs. All 124 `env/*` metrics are byte-for-byte equal,
as are agent steps, epoch, and every recorded policy/value/entropy/KL/loss
value. Against the original baseline, 135 of 150 summary values are exact
matches; the 15 differences are only timestamps, wall-clock performance,
throughput, and CPU/GPU memory/utilization telemetry.

This core refactor caused the training backend to rebuild, so its source and
binary hashes changed as expected. The model contract and training
configuration remained identical. Throughput was 965,306 SPS and training
uptime was 104.000881 seconds. The complete regression suite passes all 167
remaining tests; the count decreased from 169 only because two tests existed
solely for the removed, never-integrated dynamic-BFS API.

## Comparison 5: reward parameters stored directly

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`e6o99d6d`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/e6o99d6d) (`olive-serenity-1230`) |
| Tag | `v4.5_refactor_reward_params_storage_100m` |
| Git commit at run start | `4618a0ea0215ec57bf4ef90c900cb3e2c8a687e8` plus the documented uncommitted reward-parameter refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `0161d2f6d4b494277b0f7389a68b12bb32f18326d00d0f20bdcd0f59ad0ec997` |
| Backend source SHA-256 | `bf670d0037ed711415ad009274a724518cf92b12dce2122a4b6b8fa69f7610a7` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T005942Z-train-1044013.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and the immediately preceding comparison `4qmy1v9y`. All 124 `env/*` metrics
are byte-for-byte equal, as are agent steps, epoch, and every recorded
policy/value/entropy/KL/loss value. Each summary contains the same 150 keys;
135 values match exactly and the 15 differences are limited to timestamps,
wall-clock performance, throughput, and CPU/GPU memory/utilization telemetry.

The training backend source and binary hashes changed because reward parameter
storage moved from duplicated adapter fields into one `FcRewardParams` member.
The configuration and model contract remained identical. Throughput was
957,587 SPS and training uptime was 103.107399 seconds, effectively unchanged
from Comparison 4.

## Comparison 6: authoritative viewer render events

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`gg503t3n`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/gg503t3n) (`vital-sea-1231`) |
| Tag | `v4.5_refactor_render_events_100m` |
| Git commit at run start | `665385ddc` plus the documented uncommitted render-event refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `52245be037b67e9908a54e5b4966d83b8c5c3b0995aac853d02f6e97e31fb5bd` |
| Backend source SHA-256 | `341abd5096380b9c1f0834e06c09684a59eb9e79a2e953626d839fa58256ca3c` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T012351Z-train-1055935.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and all prior comparison runs. All 124 `env/*` metrics are byte-for-byte equal,
as are agent steps, epoch, and every recorded policy/value/entropy/KL/loss
value. Each summary contains the same 150 keys; 136 values match exactly and
the 14 differences are limited to timestamps, wall-clock performance,
throughput, and CPU/GPU memory/utilization telemetry.

The core now publishes presentation-only NPC launch and hit-resolution events,
which changes the backend source and binary hashes but not the state hash,
model contract, training configuration, or policy-visible behavior. Throughput
was 990,767 SPS and training uptime was 100.094437 seconds, within ordinary
run-to-run variation and faster than Comparison 5 in this single sample.

## Reproduction command

```bash
cd /home/joe/projects/runescape-rl/fight-cave-rl_clones/v38

./runescape-rl/train.sh train \
  --config runescape-rl/config/experiments/fight_caves_txqsiahp_refactor_baseline_100m.ini \
  --tag <refactor-specific-tag>
```

Use a cold start, the same seed, and no CLI overrides. Record each comparison
run below or in a linked follow-up section with its Git commit, backend hashes,
W&B run ID, exact final summary, and differences from this baseline.
