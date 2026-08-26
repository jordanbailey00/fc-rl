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

## Comparison 7: player/NPC animation mixing consolidated

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`2geucpgs`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/2geucpgs) (`snowy-gorge-1232`) |
| Tag | `v4.5_refactor_animation_mixer_100m` |
| Git commit at run start | `660a12145` plus the documented uncommitted viewer-only animation-mixer refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `52245be037b67e9908a54e5b4966d83b8c5c3b0995aac853d02f6e97e31fb5bd` |
| Backend source SHA-256 | `341abd5096380b9c1f0834e06c09684a59eb9e79a2e953626d839fa58256ca3c` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T020249Z-train-1087311.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and all prior comparison runs. All 124 `env/*` metrics are byte-for-byte
equal, as are agent steps, epoch, and every recorded
policy/value/entropy/KL/loss value. Each summary contains the same 150 keys;
136 values match exactly and the 14 differences are limited to timestamps,
wall-clock performance, throughput, and CPU/GPU memory/utilization telemetry.

This refactor is entirely inside the standalone viewer animation runtime, so
the training backend source hash, binary hash, model contract, configuration,
and policy-visible behavior are unchanged from Comparison 6. Throughput was
951,313 SPS and training uptime was 105.982924 seconds, within ordinary
run-to-run machine-load variation.

## Comparison 8: shared episode-summary metric derivation

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`r9fnqnxh`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/r9fnqnxh) (`revived-plant-1233`) |
| Tag | `v4.5_refactor_episode_summary_100m` |
| Git commit at run start | `1d7299e62` plus the documented uncommitted shared episode-summary refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `bbb22c411a712f616138268678e725ebbf5b03a6590067a9be80cf0f0636f016` |
| Backend source SHA-256 | `98e032c1310f3817a200dd1d2a858970c82bb620c73c72305b969880dd983f3e` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T025440Z-train-1122331.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and all prior comparison runs. All 124 `env/*` metrics are byte-for-byte
equal, as are agent steps, epoch, and every recorded
policy/value/entropy/KL/loss value. Each summary contains the same 150 keys;
136 values match exactly and the 14 differences are limited to timestamps,
wall-clock performance, throughput, and CPU/GPU memory/utilization telemetry.

The backend source and binary hashes changed because the training adapter now
consumes the same read-only `FcEpisodeSummary` derivation as the evaluator.
State transitions, state hashing, observations, actions, rewards, configuration,
and the model contract are unchanged. Throughput was 902,805 SPS and training
uptime was 105.876713 seconds, within run-to-run machine-load variation.

## Comparison 9: normal `fc_core` linkage

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`1axxklis`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/1axxklis) (`royal-dawn-1234`) |
| Tag | `v4.5_refactor_normal_core_link_100m` |
| Git commit at run start | `8bce65f69` plus the documented uncommitted normal-link refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `ebdaf82bbc958416a8d3f1e216b5be45df0694dd222edd7f4eb99b6104f1d67a` |
| Backend source SHA-256 | `c355fae57e738c31aee9bbd0d710f9b4910fdfea687af16d508ad914490739b8` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T030700Z-train-1129599.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and the immediately preceding comparison `r9fnqnxh`. All three summaries have
the same 150 keys. All 124 `env/*` metrics are byte-for-byte equal, as are
agent steps, epoch, and every recorded policy/value/entropy/KL/loss value. In
each comparison, 136 of 150 summary values match exactly; the 14 differences
are only timestamps, wall-clock performance, throughput, and CPU/GPU
memory/utilization telemetry.

The backend source and binary hashes changed because gameplay is now compiled
once into `libfc_core.a` and normally linked into the training extension rather
than directly included through `fight_caves.h`. The configuration, model
contract, state transitions, observations, actions, rewards, state hashes, and
policy-visible behavior are unchanged. Throughput was 947,046 SPS and training
uptime was 104.255678 seconds, within ordinary run-to-run machine-load
variation.

## Comparison 10: shared spawn search and NPC-slot allocation

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`og4lc7gn`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/og4lc7gn) (`true-wood-1235`) |
| Tag | `v4.5_refactor_spawn_allocation_100m` |
| Git commit at run start | `478caefdd` plus the documented uncommitted spawn/allocation refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `143709fb7174ba665bde0715e06948e393159fca49dec1b788248bb8f7861cdb` |
| Backend source SHA-256 | `a64d082b5f02c7d49a74706484a3f09c25305d636588804990f7ea1469aa4374` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T045159Z-train-1158078.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and the immediately preceding comparison `1axxklis`. All three summaries have
the same 150 keys. All 124 `env/*` metrics are byte-for-byte equal, as are
agent steps, epoch, and every recorded policy/value/entropy/KL/loss value. In
each comparison, 136 of 150 summary values match exactly; the 14 differences
are only timestamps, wall-clock performance, throughput, and CPU/GPU
memory/utilization telemetry.

The backend source and binary hashes changed because wave spawning, Tz-Kek
splitting, and Jad-healer spawning now share one private footprint search and
one first-free-slot allocator. Their caller-specific search radii, failure
policies, counters, deterministic traversal order, and slot ordering remain
unchanged. Throughput was 973,021 SPS and training uptime was 101.206108
seconds, within ordinary run-to-run machine-load variation.

## Comparison 11: shared NPC attack launch

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`y4vimecf`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/y4vimecf) (`apricot-sky-1236`) |
| Tag | `v4.5_refactor_npc_attack_launch_100m` |
| Git commit at run start | `d7aa69094` plus the documented uncommitted attack-launch refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `ba12a668f67db0c878489bd22f6d274322ebf6ee0d1f163040815a8c5e6014fe` |
| Backend source SHA-256 | `555da98e01eb501a08705f67f3ff123354029313a2dcf383e591619b6b0b9c94` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T172803Z-train-1190109.json` |

Result: behavioral parity is exact against the original baseline `m916qfsv`
and preceding comparison `og4lc7gn`. All 124 `env/*` metrics, agent steps,
epoch, and policy/value/entropy/KL/loss values are byte-for-byte equal. Of 150
summary values, 135 match exactly; the 15 differences are only timestamps,
wall-clock performance, throughput, and CPU/GPU utilization telemetry.

Generic NPCs and Jad now share one private attack-launch calculation while
retaining separate style, range, delay, drain, and prayer-lock policies.
Production code decreased by 9 LOC. Throughput was 960,078 SPS and training
uptime was 102.956156 seconds, within ordinary machine-load variation.

## Comparison 12: centralized distance primitives

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`ef65qg3w`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/ef65qg3w) (`rural-cloud-1237`) |
| Tag | `v4.5_refactor_distance_primitives_100m` |
| Git commit at run start | `ea5cfe9e0` plus the documented uncommitted distance refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `6f421d8c5b9c45a7dde96acb6e625d034234e3b3b8db3b583d2b5c56c440e613` |
| Backend source SHA-256 | `99b571fa28d0088a861da32ed5839a0d0c129e3060e7059a9474c6d51f842028` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T173638Z-train-1195567.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `y4vimecf`. All 124 `env/*` metrics, agent steps, epoch, and
policy/value/entropy/KL/loss values are byte-for-byte equal. Against the
baseline, 136 of 150 summary values match exactly; the 14 differences are only
timestamps, wall-clock performance, throughput, and utilization telemetry.

Player-to-NPC, candidate-position, and healer-anchor distance calculations now
reuse the existing rectangle-distance primitive. Production code decreased by
8 LOC. Throughput was 941,835 SPS and training uptime was 104.997709 seconds,
within ordinary machine-load variation.

## Comparison 13: consolidated food, healing, and facing mutations

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`1pmv1yzs`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/1pmv1yzs) (`unique-flower-1239`) |
| Tag | `v4.5_refactor_food_heal_facing_final_100m` |
| Git commit at run start | `dc6234297` plus the documented uncommitted food/healing/facing refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `1ddba24706b92e8821b2922174d9c88dc41b5d5b32327639c189b63e77034df2` |
| Backend source SHA-256 | `650f3067724e667c349839ee6b4066190b0cf94e18a3c989f8cc3a7101b4997b` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T175439Z-train-1211001.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `ef65qg3w`. All 124 `env/*` metrics, agent steps, epoch, and
policy/value/entropy/KL/loss values are byte-for-byte equal. Against both runs,
136 of 150 summary values match exactly; the 14 differences are only
timestamps, wall-clock performance, throughput, and utilization telemetry.

Food mutations, consumable legality, bounded NPC healing, and player-facing
conversion now each have one implementation while retaining their existing
caller-specific values and accounting. Production code decreased by 5 LOC.
Throughput was 956,771 SPS and training uptime was 104.176655 seconds, within
ordinary run-to-run machine-load variation.

## Comparison 14: decomposed core transitions

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`sq77fz7t`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/sq77fz7t) (`morning-armadillo-1240`) |
| Tag | `v4.5_refactor_core_transitions_100m` |
| Git commit at run start | `1b180a591` plus the documented uncommitted transition decomposition |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `e1f6de5d2748af173354125f40cd3f721de69a49232c75d4faae0aa04fa4569d` |
| Backend source SHA-256 | `641539d2a8ad4ce587956cb6a8e70dcc02ad11e940254988a45f58362e5b79f6` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T191857Z-train-1234677.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `1pmv1yzs`. All 124 `env/*` metrics, agent steps, epoch, and
policy/value/entropy/KL/loss values are byte-for-byte equal. Against both runs,
136 of 150 summary values match exactly; the 14 differences are only
timestamps, wall-clock performance, throughput, and utilization telemetry.

The large player-action and NPC-pending-hit transitions are now short
coordinators over phase-named private helpers, and wave-duration accounting has
one implementation. Production code increased by 53 LOC; the benefit is
smaller independently reviewable transitions and explicit ordering rather than
code removal. Throughput was 864,150 SPS and training uptime was 115.646568
seconds. The final telemetry sample reported only 59% GPU utilization versus
100% in `1pmv1yzs`; GPU evaluation and training time both increased, so this
run's lower throughput is recorded as machine/GPU-load variance rather than a
core behavior change.

## Comparison 15: reward and loadout modules moved out of headers

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`nffnh657`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/nffnh657) (`woven-shape-1241`) |
| Tag | `v4.5_refactor_reward_loadout_modules_100m` |
| Git commit at run start | `db4bcfd94` plus the documented uncommitted reward/loadout module refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `8b8ebf5bd9061509337fbf59b4d785eecead85dd3ee40ca212fd698b0c5dd8d5` |
| Backend source SHA-256 | `acaeb381878cc8ab53244cf884c92caef887dd6e29efc11e09d7e20efac62292` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T214638Z-train-1289930.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `sq77fz7t`. All 124 `env/*` metrics, agent steps, epoch, and
policy/value/entropy/KL/loss values are byte-for-byte equal. Against both runs,
136 of 150 summary values match exactly; the 14 differences are only
timestamps, wall-clock performance, throughput, and utilization telemetry.

Reward logic and immutable loadout data now each have one compiled definition.
The two affected public headers decreased from 1,101 lines combined to 257,
while total production source and build code decreased by 8 LOC. Throughput was
953,842 SPS and training uptime was 103.832579 seconds, comparable to the
956,771 SPS and 104.176655 seconds of comparison `1pmv1yzs` under full GPU
utilization.

## Comparison 16: narrowed public core API

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`z8rraat7`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/z8rraat7) (`copper-serenity-1242`) |
| Tag | `v4.5_refactor_narrow_core_api_100m` |
| Git commit at run start | `109a22c69` plus the documented uncommitted public-API refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `dc1b01a9c97dd8beaa925773e8ae8843224dee5810e8885e935dd4c9dfb4208c` |
| Backend binary SHA-256 | `f4b9222354f30f7bac13bde5adfba1d3fe2d16b5da40abdd26d667ea21bd8167` |
| Backend source SHA-256 | `7a66a4d22ab950b9fec98d4c066956606da145a40309676c27455dcd6998b6af` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260825T225811Z-train-1303256.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `nffnh657`. All 124 `env/*` metrics, agent steps, epoch, and
policy/value/entropy/KL/loss values are byte-for-byte equal. Against both runs,
136 of 150 summary values match exactly; the 14 differences are only
timestamps, wall-clock performance, throughput, and utilization telemetry.

Three implementation-only functions are now private to their owning modules
and absent from the library's exported symbol table. Public behavior and test
coverage are preserved through the supported wave and area-LOS operations.
Production code decreased by 9 LOC. Throughput was 946,812 SPS and training
uptime was 105.416005 seconds, comparable to the preceding full-utilization
run's 953,842 SPS and 103.832579 seconds.

## Comparison 17: removed unused hashed state fields

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`3zvy6nuq`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/3zvy6nuq) (`ruby-durian-1243`) |
| Tag | `v4.5_refactor_hashed_state_fields_100m` |
| Git commit at run start | `965c05321` plus the documented uncommitted state/hash refactor |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `0483e5a17265fec97b090d7f613716de2115754e14091cb99b955505e971673c` |
| Backend source SHA-256 | `3f171ba9e2b9c53ae11364df2fb15bc258dcca239126cf9901a5c4b8749ae3f8` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T001025Z-train-1347584.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `z8rraat7`. All 124 `env/*` metrics, agent steps, epoch, and
policy/value/entropy/KL/loss values are byte-for-byte equal. Against both runs,
135 of 150 summary values match exactly; the 15 differences are only
timestamps, wall-clock performance, throughput, and utilization telemetry.

Four unused `FcState` fields and one redundant `FcNpc` field were removed, and
the obsolete per-attack safespot scan disappeared with its dead output. This is
an intentional diagnostic contract migration from state-hash version 3 to 4;
observation, action, reward, and model shapes did not change. Production code
decreased by 25 LOC. Throughput was 951,098 SPS and training uptime was
102.968839 seconds, comparable to the preceding run's 946,812 SPS and
105.416005 seconds.

## Comparison 18: removed dead viewer debugging systems

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`gg2cqj2d`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/gg2cqj2d) (`comfy-firefly-1244`) |
| Tag | `v4.5_refactor_dead_viewer_debug_100m` |
| Git commit at run start | `3dd2f9958` plus the documented uncommitted viewer-debug cleanup |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `087d3461e5e6ec5e2d5f1d8871eb6245cc056cd4298cccc456920e0b5c520cd9` |
| Backend source SHA-256 | `97f12929254f2a8c4e30ecffdc90ca41b2ac39a9081b7f5c050892bdccf5fce6` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T011955Z-train-1372891.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `3zvy6nuq`. All 124 `env/*` metrics, agent steps, epoch, and every
recorded policy/value/entropy/KL/loss value are byte-for-byte equal. The only
differences are wall-clock performance, throughput, and CPU/GPU utilization
telemetry.

The unused viewer debug module, two unreachable overlay panels, their dead
mode-selection states, and newly orphaned helpers were removed. Production and
build code decreased by 512 LOC. The core and training inputs are unchanged;
throughput was 828,034 SPS and training uptime was 120.156476 seconds.

## Comparison 19: removed unused public pathfinding APIs

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`nqgg0y6a`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/nqgg0y6a) (`lemon-water-1245`) |
| Tag | `v4.5_refactor_unused_pathfinding_apis_100m` |
| Git commit at run start | `cae3a48ef` plus the documented uncommitted pathfinding API cleanup |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `e0015300b9d84123fda37f2539db41155ff6020f262beb50674837cc8b0d88a9` |
| Backend source SHA-256 | `f0a929522f88ee174c13762df2570fddbfeb1d0e23df0f67a1f40d0c6e2a72e3` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T022738Z-train-1412289.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `gg2cqj2d`. All 124 `env/*` metrics, agent steps, epoch, and every
recorded policy/value/entropy/KL/loss value are byte-for-byte equal. In both
comparisons, 135 of 150 summary values match exactly; the 15 differences are
only timestamps, wall-clock performance, throughput, and utilization telemetry.

The unused exact-destination BFS wrapper and entity-footprint convenience
wrapper were removed from the core implementation and public header. Two route
guardrails now exercise the live move-near BFS API, while the test whose sole
subject was the deleted footprint wrapper was removed. The complete suite now
passes 163/163 tests. Production code decreased by 34 LOC. Throughput was
1,004,402 SPS and training uptime was 102.189196 seconds.

## Comparison 20: removed unused viewer functions

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`c7i6etqu`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/c7i6etqu) (`ruby-butterfly-1246`) |
| Tag | `v4.5_refactor_unused_viewer_functions_100m` |
| Git commit at run start | `7a8e87fb2` plus the documented uncommitted viewer-function cleanup |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `e0015300b9d84123fda37f2539db41155ff6020f262beb50674837cc8b0d88a9` |
| Backend source SHA-256 | `f0a929522f88ee174c13762df2570fddbfeb1d0e23df0f67a1f40d0c6e2a72e3` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T024254Z-train-1424515.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `nqgg0y6a`. All 124 `env/*` metrics, agent steps, epoch, and all
seven policy/value/entropy/KL/loss values are byte-for-byte equal. The current
summary matches 136 of 150 baseline fields exactly; the 14 differences are
only timestamps, wall-clock performance, throughput, and CPU/GPU utilization
telemetry.

Twenty-seven caller-free viewer functions and their declarations were removed,
including obsolete loader helpers, UI convenience wrappers, and the uninvoked
runtime self-test subgraph. No live rendering or gameplay path was changed.
Production code decreased by 429 LOC. The complete suite passes 163/163 tests.
Throughput was 954,976 SPS and training uptime was 103.612085 seconds.

## Comparison 21: removed unused viewer state and unreachable action mode

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`zecd2zxw`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/zecd2zxw) (`stilted-wood-1247`) |
| Tag | `v4.5_refactor_unused_viewer_state_100m` |
| Git commit at run start | `5f2e2da83` plus the documented uncommitted viewer-state cleanup |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `e0015300b9d84123fda37f2539db41155ff6020f262beb50674837cc8b0d88a9` |
| Backend source SHA-256 | `f0a929522f88ee174c13762df2570fddbfeb1d0e23df0f67a1f40d0c6e2a72e3` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T025503Z-train-1438807.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `c7i6etqu`. All 124 `env/*` metrics, agent steps, epoch, and all
seven policy/value/entropy/KL/loss values are byte-for-byte equal. The current
summary matches 135 of 150 baseline fields exactly; the 15 differences are
only timestamps, wall-clock performance, throughput, and CPU/GPU utilization
telemetry.

Three unused static UI tables, one unused viewer-state field, four dead stores,
one unused internal debug parameter, and the unreachable random-action mode
were removed. No live viewer, gameplay, training, or policy-pipe behavior was
changed. Production code decreased by 41 LOC. The complete suite passes 163/163
tests. Throughput was 928,032 SPS and training uptime was 104.673579 seconds.

## Comparison 22: removed unused inline viewer file-size helper

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`2bzeigyh`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/2bzeigyh) (`peach-donkey-1248`) |
| Tag | `v4.5_refactor_unused_viewer_state_final_100m` |
| Git commit at run start | `5f2e2da83` plus the complete documented uncommitted viewer-state cleanup |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `e0015300b9d84123fda37f2539db41155ff6020f262beb50674837cc8b0d88a9` |
| Backend source SHA-256 | `f0a929522f88ee174c13762df2570fddbfeb1d0e23df0f67a1f40d0c6e2a72e3` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T030154Z-train-1444589.json` |

Result: behavioral parity remains exact against baseline `m916qfsv`, the
preceding committed-state comparison `c7i6etqu`, and intermediate comparison
`zecd2zxw`. All 124 `env/*` metrics, agent steps, epoch, and all seven
policy/value/entropy/KL/loss values are byte-for-byte equal. The final summary
matches 135 of 150 baseline fields exactly; the 15 differences are only
timestamps, wall-clock performance, throughput, and CPU/GPU utilization
telemetry.

A focused Cppcheck pass found the unused inline `fc_file_size()` helper after
Comparison 21; removing it deleted another 11 LOC, bringing this cleanup to 52
net production LOC removed. No live behavior or backend input changed. The
complete suite again passes 163/163 tests. Throughput was 639,593 SPS and
training uptime was 139.876110 seconds; this timing-only slowdown occurred
without any behavioral or learning-metric difference.

## Comparison 23: removed dormant decoded-UI state

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`r4hne4fg`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/r4hne4fg) (`drawn-water-1249`) |
| Tag | `v4.5_refactor_viewer_ui_state_100m` |
| Git commit at run start | `950d0af5f` plus the documented uncommitted viewer-only cleanup |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `e0015300b9d84123fda37f2539db41155ff6020f262beb50674837cc8b0d88a9` |
| Backend source SHA-256 | `f0a929522f88ee174c13762df2570fddbfeb1d0e23df0f67a1f40d0c6e2a72e3` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T033427Z-train-1461774.json` |

Result: behavioral parity is exact against baseline `m916qfsv` and preceding
comparison `2bzeigyh`. All 124 `env/*` metrics, agent steps, epoch, and all
seven policy/value/entropy/KL/loss values are byte-for-byte equal. The current
summary matches 136 of 150 baseline fields and 135 of 150 preceding-run fields
exactly; all differences are timestamps, wall-clock performance, throughput,
and CPU/GPU utilization telemetry.

Dead component override modes, the copied event-mask cache, unused decoded
metadata and payload storage, dormant interface/status/chat fields, and their
unreachable helpers were removed. The decoder still consumes skipped fields so
the `interfaces.bin` format remains aligned. Production code decreased by 229
LOC excluding blank and comment-only lines (240 net physical source lines).
The complete suite passes 163/163 tests, and decoded-interface startup was
smoke-tested successfully. Throughput was 953,900 SPS and training uptime was
100.651127 seconds.

## Comparison 24: removed decoded-interface subsystem

| Field | Post-refactor result |
| --- | --- |
| W&B run | [`ww4xmjoi`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/ww4xmjoi) (`splendid-dawn-1250`) |
| Tag | `v4.5_refactor_decoded_ui_removed_100m` |
| Git commit at run start | `ca77977a1` plus the documented uncommitted decoded-interface removal |
| Realized agent steps | `99,614,720` |
| Epoch | `95` |
| Contract checkpoint identity | `446a78a42f8a88b9435a52169914a7ee724980170aff5e0e15010bc92959ab4b` |
| Backend binary SHA-256 | `e0015300b9d84123fda37f2539db41155ff6020f262beb50674837cc8b0d88a9` |
| Backend source SHA-256 | `f0a929522f88ee174c13762df2570fddbfeb1d0e23df0f67a1f40d0c6e2a72e3` |
| Manifest | `pufferlib_4/logs/fight_caves/manifests/20260826T035601Z-train-1474060.json` |

Result: behavioral parity remains exact against baseline `m916qfsv` and the
preceding decoded-state cleanup `r4hne4fg`. All 124 `env/*` metrics, agent
steps, epoch, and all seven policy/value/entropy/KL/loss values are
byte-for-byte equal. Each complete 150-field summary has 136 exact matches;
the 14 differences are only timestamps, wall-clock performance, throughput,
and CPU/GPU memory telemetry.

The unused decoded-interface subsystem, its 266,898-byte asset, and its export
pipeline were removed. This includes decoded rendering and hit testing,
generic interface/modal/overlay management, override synchronization, and
listener/trigger dispatch. The fixed RuneC UI and its future-facing
non-functional controls remain. The change removes 3,611 net physical source
lines (3,319 non-comment code lines), and the suite passes 163/163 tests. A
real Raylib/OpenGL viewer startup and render smoke test also passed. Throughput
was 899,453 SPS and training uptime was 128.337457 seconds.

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
