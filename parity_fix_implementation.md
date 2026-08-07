# Parity fix implementation record

This document records execution evidence for `parity_fix.md`,
`parity_fix_tests.md`, and `parity_fix_config.md`. The three plan documents
remain authoritative and are not used as result logs.

## A0 — untouched baseline

Status: complete on 2026-08-01.

### Baseline identity

- Implementation-start `HEAD`: `b0571368ebaf8fd6630a1732e4776a1e7d197dda`
  (`feat(fight-caves): promote v4_simple_reward with OSRS-rate HP regen`,
  2026-07-27), branch `dev` tracking `fc-rl/dev`.
- Capture time: `2026-08-01T22:46:05-04:00`.
- The implementation-start worktree was already dirty. Pre-existing changes
  were `.gitignore`, `TODO.md`, the root `AGENTS.md`, and the module
  `AGENTS.md` files under `pufferlib_4` and `runescape-rl`. These were not
  modified by A0.
- Host: Ubuntu kernel `7.0.0-28-generic`, x86-64, Intel Core i7-14700F
  (20 cores/28 logical CPUs).
- CMake `3.28.3`; GCC/`cc` `13.3.0`; build compiler `/usr/bin/cc`.
- Fresh build directory: `runescape-rl/build-parity-baseline`.
- Build type: `RelWithDebInfo`; effective standard flags
  `-O2 -g -DNDEBUG`.

Plan hashes at implementation start:

| Document | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `87e8b7a8a1c090c4d6697f0fdf0b382e2550feebb1669f14ab44e0450d32a1e6` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

### Fresh build and existing tests

Commands:

```bash
cmake -S runescape-rl -B runescape-rl/build-parity-baseline -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build runescape-rl/build-parity-baseline -j2
ctest --test-dir runescape-rl/build-parity-baseline -LE known_red --output-on-failure
ctest --test-dir runescape-rl/build-parity-baseline -L known_red --output-on-failure
```

Results:

- Configure and build succeeded.
- The complete existing suite passed: 70/70 tests, zero failures. This included
  `test_headless`, asset validation, core guardrails, training guardrails, CUDA
  architecture guardrails, and the Python static/config/adapter checks.
- The baseline contains no tests labelled `known_red`; the label-only CTest run
  reported no tests and exited successfully.
- Direct `test_headless` execution passed 43/43 internal assertions. Its
  fixed-seed determinism check matched all 500 ticks, and its multi-episode
  diagnostic completed 5,000 ticks over 10 episodes without a crash.
- The build emitted pre-existing viewer `-Wformat-truncation` warnings from
  `fc-viewer/src/ui.c` (`draw_skills`, `open_decoded_context`, and
  `decoded_left_click`). No core, training, validation, or link failure
  occurred. These warnings are recorded as baseline evidence and were not
  changed during A0.

Selected fresh-build artifact hashes:

| Artifact | SHA-256 |
|---|---|
| `fc-core/libfc_core.a` | `ba99b974ab2a94cb84907bb9878111be158258a3ac2272f825f6f5967d59cec2` |
| `fc-viewer/test_headless` | `14f62d3dff9dbbb8d4a557d7ca750440fda943ad9baf6b3934aeabb825aaac7a` |
| `fc-validation/phase2_guardrails_core` | `bdba20993f4c3eea335ae7b21327de85f2f99d1a401d43b0bbeae68daf21039c` |
| `fc-validation/phase2_guardrails_training` | `de4cf29f24cbca0bc12e324786c36618f7fad6352057100e299d83701fcadaa6` |

### Deterministic trace and state diagnostics

The existing `test_headless` corpus was reproduced through a transient
release-build diagnostic linked to the fresh `fc_core` archive and the current
viewer hash implementation. The corpus uses simulator seed `777`, an
independent xorshift32 action seed of `99999`, each generated action modulo the
current head dimension, movement forced idle on two of three ticks, and heads
5/6 forced to zero. Two complete invocations produced byte-identical checkpoint
diagnostics, and each invocation replayed its 500 recorded actions with a hash
match at every tick.

| Tick | State hash | RNG state | Wave | HP | Prayer | Overhead | Ammo | NPCs | Actions |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 1 | `50454709` | `0c454419` | 1 | 990 | 990 | 3 | 50000 | 1 | `0,4,2,0,1,0,0` |
| 2 | `5c38dd58` | `0c454419` | 1 | 990 | 990 | 2 | 50000 | 1 | `0,1,3,0,0,0,0` |
| 3 | `7046b449` | `0c454419` | 1 | 990 | 990 | 1 | 50000 | 1 | `9,0,4,1,0,0,0` |
| 10 | `4dd13c09` | `47d2bd89` | 1 | 990 | 990 | 0 | 49999 | 1 | `0,1,0,0,1,0,0` |
| 50 | `2a0ec6c9` | `18b11ef9` | 3 | 990 | 990 | 1 | 49994 | 2 | `0,6,4,0,0,0,0` |
| 100 | `d33287e1` | `ac7e3a8a` | 4 | 990 | 990 | 3 | 49985 | 3 | `7,1,2,1,1,0,0` |
| 250 | `469bba7f` | `0b34865c` | 6 | 990 | 980 | 2 | 49958 | 4 | `0,3,0,0,0,0,0` |
| 500 | `de5d01d5` | `6ace27c4` | 10 | 990 | 720 | 2 | 49922 | 3 | `0,5,3,0,0,0,0` |

Final replay result: `MATCH`, 500 ticks, final hash `de5d01d5`, final RNG
state `6ace27c4`. The diagnostic binary was
`/tmp/fc_parity_a0_trace` with SHA-256
`938032d864f2b02a6fc7e107508e7e3fce3452be67cdde98acc81d71e122e686`.

This is pre-parity diagnostic evidence, not a post-fix golden. At A0,
`fc_state_hash` is still implemented in the viewer source and has no declared
`FC_STATE_HASH_VERSION`; the plan intentionally replaces that arrangement with
the shared, versioned canonical hash.

### Contract and configuration snapshot

- Full action heads: 7, dimensions `{17, 9, 5, 3, 2, 65, 65}`.
- Puffer action heads: 3, dimensions `{17, 9, 5}`.
- Policy observations: 285; reward features: 20; pre-mask total: 305.
- Puffer mask/input: 31/316; full mask/observation buffer: 166/471.
- Manifest schema: `1`.
- Observation version:
  `fight_caves_puffer_policy_obs_v7_npc_prayer_drain_healing_aggro_kill_events_mask_heads_0_2_no_supplies`.
- Action version: `fight_caves_multidiscrete_3_head_no_supplies_v1`.
- Reward version: `fight_caves_v3_progress_npc_heal_penalty_m0005`.
- Canonical and Puffer-mirror INIs were byte-identical, both SHA-256
  `509af1168dca12f1c9f72ffc51d31bc2e7c7476536785915f20b034418127514`.

### Performance baseline

The repository's standalone smoke executable is time-seeded and completed only
856 steps in a reported `0.00s`, so its one observed result (568,393 SPS) was
too short and non-reproducible to use for `PERF-001`.

The accepted baseline instead used a transient direct-included Puffer-path
harness compiled with GCC 13.3.0 using `-O2 -DNDEBUG -DFC_NO_HASH`. Each run
executed exactly 5,000,000 `c_step` calls pinned to CPU 0. The workload used
environment seed 73, independent action seed `0x5eed1234`, current three-head
dimensions, a deterministic sparse-action schedule, no supplies, incoming
aggregate ablation enabled, default reward parameters, observation writes, and
both float/native action-mask writes. The fixed checksum and episode count
prove all five timed runs executed the same corpus.

| Run | Elapsed seconds | SPS | Checksum | Episodes |
|---:|---:|---:|---:|---:|
| 1 | 4.792106 | 1,043,383 | `83b928c6` | 6,477 |
| 2 | 4.796312 | 1,042,468 | `83b928c6` | 6,477 |
| 3 | 4.794887 | 1,042,777 | `83b928c6` | 6,477 |
| 4 | 4.793165 | 1,043,152 | `83b928c6` | 6,477 |
| 5 | 4.795812 | 1,042,576 | `83b928c6` | 6,477 |

Median baseline: **1,042,777 SPS**. The transient benchmark binary was
`/tmp/fc_parity_a0_bench` with SHA-256
`49c8bdb0874f8d5a160badca084df348df7f14f51e728063c668e05d1232852c`.
This result remains historical smoke evidence, but it is direct-included and
therefore is not the accepted `PERF-001` comparison after the approved test-plan
expansion below.

### Retrospective expanded A0 reconstruction

Status: complete on 2026-08-02. The expanded performance protocol was approved
after A1 had been implemented, so A0 was reconstructed before any A1
revalidation or A2 work. The revised `parity_fix_tests.md` has SHA-256
`164e09c4908375b383401ac5210d8593ef54c89da5933978b9b11248a282759c`.
Neither `parity_fix.md` nor `parity_fix_config.md` changed.

An isolated local clone at
`/tmp/fc-parity-expanded-a0.vhuNgK/repo` was detached at exact commit
`b0571368ebaf8fd6630a1732e4776a1e7d197dda`. `git status --short` was empty
before and after validation. The benchmark harness was supplied from the
implementation worktree rather than committed into A0; its Python driver hash
was `e5e9f0465519df7e7a854a5580e89df042ae9c4ba68413094a320e79b34f0ff2`
and its linked-core C workload hash was
`44f09fae0db364ab66febf60fa62a514abbf4f667aa929b224b7a2dcabe6e5fd`.
The only files copied into ignored dependency directories were the two
pre-existing Raylib archives required to link the full viewer and Puffer
backend; both had SHA-256
`3323cbf14ec6e640e19d63e230e68c3a530be3e9a9d416435eab82160e9e9ccc`.
No A0 tracked source was changed.

The clone was configured and built with CMake `Release`; the complete exact-A0
CTest suite passed 70/70. The linked core archive had SHA-256
`b55be223574fd8650adf5b9a3f7aab3e60edfc6188047c8799be939a75caeecc`.
The Puffer CUDA backend was built through `runescape-rl/fc-training/build.sh`
with `CUDA_HOME=/usr/local/cuda`, compiled for `sm_120`, and had SHA-256
`06d25a9789658624b9c2b58c8bfb1f945d0578b69538b26b371d21527cf208d8`.

All performance runs used seed 73, five fresh processes, GCC 13.3.0, CUDA
13.3 (`V13.3.73`), an NVIDIA GeForce RTX 5070 Ti with driver 580.173.02,
and the Intel Core i7-14700F host under the recorded `powersave` governor.
The version-native A0 Puffer contract was asserted before timing:
observation stride 316 floats, native-mask stride 31 bytes, action stride 3,
and action dimensions `{17, 9, 5}`. Canonical configuration was preserved as
`fc-validation/tests/baselines/parity_fix_a0/fight_caves.ini`, SHA-256
`509af1168dca12f1c9f72ffc51d31bc2e7c7476536785915f20b034418127514`.

#### PERF-001 — normally linked `fc_core`

The external workload was compiled with
`-O3 -DNDEBUG -std=c11 -Wall -Wextra -Werror`, linked to the CMake Release
`libfc_core.a`, and pinned to CPU 0. Each fresh process performed 250,000
unmeasured warmup steps followed by 5,000,000 measured steps. The deterministic
scenario schedule covered every A0 NPC family, available dual-style attacks,
pending hits, TBow and Bowfa loadouts, Prayer commands 0-4 and depletion,
observation/mask generation, movement/pathfinding, and resets. Hashing was not
called in the hot loop. Because this corpus covers the complete A0 mechanic
set, it is both the common-comparison and A0-native coverage workload; the
approved plan does not require timing an identical corpus twice.

```bash
python3 runescape-rl/fc-validation/tests/parity_perf.py core \
  --repo-root /tmp/fc-parity-expanded-a0.vhuNgK/repo \
  --binary /tmp/fc-parity-expanded-a0.vhuNgK/parity_perf_core_a0 \
  --trials 5 --warmup-steps 250000 --measure-steps 5000000 \
  --scenario-span 512 --seed 73 --prayer-limit 5 --cpu-affinity 0 \
  --build-flags '-O3 -DNDEBUG -std=c11 -Wall -Wextra -Werror; linked CMake Release libfc_core.a' \
  --output runescape-rl/fc-validation/tests/baselines/parity_fix_a0/core.json
```

| Trial | Measured seconds | SPS | Peak RSS KiB | Checksum |
|---:|---:|---:|---:|---|
| 1 | 5.172749 | 966,603.974 | 23,572 | `d0b6c52340db0306` |
| 2 | 5.167491 | 967,587.577 | 23,572 | `d0b6c52340db0306` |
| 3 | 5.160775 | 968,846.711 | 23,572 | `d0b6c52340db0306` |
| 4 | 5.190709 | 963,259.534 | 23,572 | `d0b6c52340db0306` |
| 5 | 5.203564 | 960,879.971 | 23,572 | `d0b6c52340db0306` |

Accepted median: **966,603.974 SPS**; minimum 960,879.971; maximum
968,846.711; sample standard deviation 3,283.321; sample variance
10,780,199.969; coefficient of variation **0.340%**. All trials performed
19,124 resets and used approximately one CPU. The process allocated no GPU
memory; GPU identity was still captured for host parity.

#### PERF-002 — real Puffer `VecEnv` rollout

The benchmark loaded the exact A0 compiled Fight Caves binding, allocated the
canonical 4,096 environments with two buffers and 16 worker threads, and used
the synchronous `VecEnv.cpu_step` path with native hard masks and a fixed
three-head action corpus. The qualifying command pinned the process and workers
to logical CPUs 0-15, warmed up for 16,384 vector steps, and measured 32,768
vector steps (134,217,728 transitions) per fresh process.

```bash
python3 runescape-rl/fc-validation/tests/parity_perf.py puffer-rollout \
  --repo-root /tmp/fc-parity-expanded-a0.vhuNgK/repo \
  --puffer-dir /tmp/fc-parity-expanded-a0.vhuNgK/repo/pufferlib_4 \
  --trials 5 --warmup-steps 16384 --measure-steps 32768 \
  --action-cycle 256 --prayer-limit 5 \
  --expected-obs 316 --expected-mask 31 --expected-action-dims 17,9,5 \
  --cpu-affinity 0-15 \
  --output runescape-rl/fc-validation/tests/baselines/parity_fix_a0/puffer_rollout.json
```

| Trial | Measured seconds | SPS | Peak RSS KiB | CPU utilization | Checksum |
|---:|---:|---:|---:|---:|---|
| 1 | 26.022049 | 5,157,846.175 | 770,320 | 1,373.0% | `29cb0500` |
| 2 | 26.529382 | 5,059,210.525 | 770,456 | 1,387.0% | `29cb0500` |
| 3 | 27.625413 | 4,858,487.584 | 770,380 | 1,318.4% | `29cb0500` |
| 4 | 26.299618 | 5,103,409.783 | 770,476 | 1,400.3% | `29cb0500` |
| 5 | 25.977182 | 5,166,754.673 | 770,400 | 1,427.7% | `29cb0500` |

Accepted median: **5,103,409.783 SPS**; minimum 4,858,487.584; maximum
5,166,754.673; sample standard deviation 125,543.680; sample variance
15,761,215,682.079; coefficient of variation **2.477%**. Each trial recorded
32,591 resets and sampled every A0 prayer command, so the one A0 corpus serves
as both the common-comparison and version-native rollout workload. Observation
and mask time are included in the batch-step measurement but are not separately
exported by the A0 binding. The CPU VecEnv path allocated no GPU memory.

Two preliminary protocols were rejected rather than admitted as baselines. A
roughly one-second measurement had 25.41% CV. Extending the window under
unrestricted hybrid P/E-core scheduling reduced CV to 5.15%, still above the
5% gate. Pinning to CPUs 0-15 with only a short warmup produced 5.24% CV due to
a cold first trial. The recorded steady-state warmup resolved that source of
noise without weakening the threshold.

#### PERF-003 — complete Puffer rollout and PPO learner

The benchmark used the same compiled CUDA backend and canonical training
configuration as the production entry point: 4,096 environments, two buffers,
16 threads, horizon 256, minibatch 32,768, replay ratio
2.055184291514704, hidden size 256, three hidden layers, and 678,912 model
parameters. The preserved INI records learning rate, entropy coefficient,
discount/GAE, clipping, value, gradient, V-trace, prioritized replay, and Adam
settings exactly. Each cold-start fresh process discarded 16 complete updates,
then timed 32 rollout-plus-learner updates (33,554,432 sampled transitions).
Health-only finite/gradient instrumentation was disabled. A checkpoint was
saved and loaded outside each measured window.

```bash
python3 runescape-rl/fc-validation/tests/parity_perf.py puffer-training \
  --repo-root /tmp/fc-parity-expanded-a0.vhuNgK/repo \
  --puffer-dir /tmp/fc-parity-expanded-a0.vhuNgK/repo/pufferlib_4 \
  --trials 5 --warmup-updates 16 --measure-updates 32 \
  --expected-obs 316 --expected-mask 31 --expected-action-dims 17,9,5 \
  --cpu-affinity 0-15 \
  --output runescape-rl/fc-validation/tests/baselines/parity_fix_a0/puffer_training.json
```

| Trial | Total SPS | Rollout SPS | Seconds | Peak RSS KiB | Peak GPU GiB | Mean GPU util. |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1,581,472.867 | 2,931,323.093 | 21.217204 | 1,242,372 | 3.303 | 67.29% |
| 2 | 1,574,892.725 | 2,904,261.428 | 21.305852 | 1,242,052 | 3.292 | 66.76% |
| 3 | 1,579,558.629 | 2,918,079.911 | 21.242916 | 1,241,996 | 3.292 | 59.49% |
| 4 | 1,576,079.574 | 2,908,914.350 | 21.289808 | 1,242,320 | 3.292 | 64.12% |
| 5 | 1,566,317.501 | 2,884,695.321 | 21.422497 | 1,242,140 | 3.292 | 62.06% |

Accepted median total-training throughput: **1,576,079.574 SPS**; minimum
1,566,317.501; maximum 1,581,472.867; sample standard deviation 5,853.112;
sample variance 34,258,922.239; coefficient of variation **0.371%**. Every
trial completed 32 measured PPO updates, produced the same 34,284 reset count,
reported finite loss channels, had no sampler/CUDA/mask/buffer error, and
successfully saved and reloaded its checkpoint. Per-trial rollout,
policy-inference, environment-step, learner, checkpoint, utilization, memory,
loss, and reward/episode metrics remain in the JSON and raw logs.

Machine-readable results and all 15 raw trial logs are under
`runescape-rl/fc-validation/tests/baselines/parity_fix_a0/`. Artifact hashes:

| Artifact | SHA-256 |
|---|---|
| `ctest-release.log` | `5ed1ac9c3fd33eb413d4cf9a37a9c07658252f9b601b0f647cf9bb35359194b8` |
| `core.json` | `1e4cbf947b4e54610cd79878b7a243c482df1762419b682ad4db2ec44d18779a` |
| `puffer_rollout.json` | `df60a9f361f756b103f4e8d007f68f76e61793effa10451d428d8a594b2dd64e` |
| `puffer_training.json` | `a64f685123011b69d972ed60a597c22c43753711dede387614f791a70c80c18c` |
| `fight_caves.ini` | `509af1168dca12f1c9f72ffc51d31bc2e7c7476536785915f20b034418127514` |

`TRAIN-001` itself was not run at A0: its eight-prayer, post-fix observation,
mask, checkpoint, and evaluator instrumentation does not exist in A0. Its
report validator was syntax-checked and tested with a complete synthetic
report plus a deliberate no-legal-action mutation. This validates scaffolding,
not post-fix training health.

### A0 gate conclusion

A0 passes: the untouched existing suite is green, no pre-existing test failure
requires a plan pivot, deterministic replay is stable for the saved corpus,
and all three expanded five-run performance baselines meet the variance gate.
No A1 validation or A2 gameplay test was run during the retrospective
reconstruction.

## A1 — compile-only scaffolding

Status: complete on 2026-08-01. Work stopped at the A1 boundary; no A2
behavior tranche or parity assertion was added.

### Scope and compatibility boundary

A1 added the types, explicit units, public helper boundaries, derived metadata,
and reset-safe fields needed by the three later behavior workstreams:

- `FcAttackType` separates stab/slash/crush/ranged/magic equipment-defence
  selection from the broad melee/ranged/magic prayer style.
- `FcNpcStats` now has style-specific `*_max_hit_tenths` fields, separate
  Attack/Ranged/Magic levels, `ranged_def_bonus`, and `melee_attack_type`.
  The ambiguous primary state field is now `FcNpc.max_hit_tenths`; explicit
  tenths and whole-HP maximum accessors and a validation API were added.
- Base/final Ranged roll and whole-HP max-hit APIs, Twisted-bow multiplier
  boundaries, and player/NPC damage-roll APIs were declared and implemented.
- Crystal armour piece bits, per-piece basis-point constants, loadout metadata,
  player metadata, and propagation through reset/viewer loadout selection were
  added. Only the Bowfa/crystal row has mask `7`.
- `FcPrayerTransition`, `prayer_at_tick_start`, transition-returning prayer
  actions, the transition-aware drain signature, and the centralized
  Prayer-loss API were added.
- `FcState.active_loadout`, `FC_STATE_HASH_VERSION`, and a compilation-path
  independent `fc_state_hash` declaration were added with safe initialization.
  Hash version `0` identifies the retained viewer-owned pre-canonical hash.

The following A1 compatibility fallbacks are deliberate and are marked in the
source for removal by their corresponding workstreams/A3:

- The new NPC schema temporarily contains the old runtime values and maps all
  current melee attacks to crush, preserving the old offensive/defensive
  outcomes. Workstream 1 replaces these with the reviewed exact table, exact
  style-level selection, and corrected defence formulas.
- New Ranged/TBow boundaries reproduce the old formulas, and the current
  attack path continues through the tenths compatibility wrapper. Workstream 2
  activates staged integer formulas, whole-HP damage sampling, crystal effects,
  and corrected loadouts.
- Flick command IDs `5`-`7` are reserved, but `FC_PRAYER_DIM` remains `5` and
  those commands are runtime no-ops. Current drain and ordinary attack timing
  remain unchanged. Workstream 3 activates the commands and transition/timing
  semantics together with all downstream contract consumers.
- The complete core-owned canonical hash is deferred until all future-relevant
  parity fields stabilize. Version `0` intentionally retains the A0 algorithm
  so the no-gameplay-change trace can be compared exactly.

### Builds and compile checks

Fresh normal build:

```bash
cmake -S runescape-rl -B runescape-rl/build-parity-a1 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build runescape-rl/build-parity-a1 -j2
```

The build completed successfully. During the first incremental compile,
`NULL` use in each of the new combat and prayer helpers exposed a missing
`<stddef.h>` include; both translation units were corrected before any runtime
validation. The completed normal build emitted only the viewer formatting
warnings already captured in A0, with no new scaffold warning.

The Puffer direct-included/amalgamated path compiled in both supported
standalone modes:

```bash
./runescape-rl/fc-training/build.sh --local
./runescape-rl/fc-training/build.sh --fast
./pufferlib_4/fight_caves
```

Both builds succeeded with the path's `-Wall` checks. The optimized standalone
binary completed its 100-episode smoke run with exit status 0 (846 total ticks,
reported 869,476 SPS, maximum wave 1). This short, time-seeded smoke result is
not a replacement for the controlled A0 `PERF-001` baseline.

A strict warning build used
`-Wall -Wextra -Werror`. The production `fc_core`, `fc_capi`, and both Phase 2
guardrail targets compiled without warnings. Attempting to extend that
non-plan flag set to the pre-existing `test_headless.c` source found its
pre-existing unused local `err` at line 253; A1 did not edit the test merely to
support an optional stricter flag set. The normal and sanitizer test builds
compile and run that target successfully.

A transient public-API client compiled with C11 and
`-Wall -Wextra -Werror`, linked against the normal `libfc_core.a`, and exercised
every new non-hash declaration successfully. Its output was:

```text
a1_api_smoke=1524 hash_version=0 loadout=1 crystal_mask=0
```

Selected A1 artifact hashes:

| Artifact | SHA-256 |
|---|---|
| `fc-core/libfc_core.a` | `4de85c1276d09959582ac85f8e3bfc4a65b20f9b115aab809e4d341e43867c6c` |
| `fc-training/libfc_capi.so` | `07863263ed258d763150d347b43252eb1cd6ce614ec0a218fecf7405c23d90c8` |
| `fc-viewer/test_headless` | `96372ebdcdf17e35272121614e49599acb872148f894e74b90434efefc8a2d7b` |
| Puffer `fight_caves` | `fa900ed7d3060a7fb7c31c82bb346153e03af5b8be95039133e07a19991f30f3` |
| transient A1 API smoke | `bfbddfc8e1fe81baba2ccf47de0d468cf2bb931c7fc8dfbd782f24ac228b0cdc` |
| transient A1 trace | `a15f119b4d7e4c6f20369afe3503792d0ec7f1f502f32103eccb751130a1e197` |

### Preservation and deterministic evidence

The complete existing CTest suite passed in the fresh A1 release build:
70/70 tests, zero failures. This retains all A0 movement, collision, LOS,
attack ordering, wave, NPC special behavior, observation, reward, action-mask,
training, and static-contract guardrails.

The exact A0 deterministic corpus was rebuilt against the A1 `fc_core` archive
and version-0 viewer hash. Two invocations were byte-identical and each replayed
all 500 recorded actions without a mismatch. Every recorded A0 checkpoint was
unchanged; in particular:

| Tick | State hash | RNG state | Wave | HP | Prayer | Overhead | Ammo | NPCs |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | `50454709` | `0c454419` | 1 | 990 | 990 | 3 | 50000 | 1 |
| 10 | `4dd13c09` | `47d2bd89` | 1 | 990 | 990 | 0 | 49999 | 1 |
| 100 | `d33287e1` | `ac7e3a8a` | 4 | 990 | 990 | 3 | 49985 | 3 |
| 250 | `469bba7f` | `0b34865c` | 6 | 990 | 980 | 2 | 49958 | 4 |
| 500 | `de5d01d5` | `6ace27c4` | 10 | 990 | 720 | 2 | 49922 | 3 |

Final result: `MATCH`, 500 ticks, final hash `de5d01d5`, final RNG state
`6ace27c4`. Because the action corpus, every checkpoint hash, and RNG state are
identical to A0, A1 introduced no intended gameplay outcome change.

### Sanitizer evidence

A complete Debug build was compiled with GCC 13.3.0 using
`-fsanitize=address,undefined -fno-omit-frame-pointer`, with the sanitizer
flags also applied at link time. All 70 CTest entries passed with
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`; no address or undefined-
behavior diagnostic occurred.

Leak detection was disabled only because this managed workspace executes tests
under process tracing: with `detect_leaks=1`, LeakSanitizer aborted before test
code with its standard `LeakSanitizer does not work under ptrace` fatal error.
That environment abort is not treated as an expected-red product result, and
it does not replace the later checked-in reproducible `MEM-001` leak gate.

### A1 gate conclusion

A1 passes. Normal-core and Puffer direct-include builds succeed, production
targets are warning-clean under the strict check, the existing suite and
ASan/UBSan suite are green, and the exact A0 deterministic trace is preserved.
No parity behavior test, configuration/contract expansion, or A2 behavior was
started. No plan pivot was required, and the three plan documents remain
byte-identical to their A0 hashes.

### A1 revalidation after expanded A0

Status: passed on 2026-08-02 at commit
`d1aac645726dca08c8a589678e83aa622eb01c16`. This revalidation occurred only
after the retrospective expanded A0 gate completed. Before testing, the
approved failure-handling rule was added to `parity_fix_tests.md`: preserve and
diagnose any unexpected failure without corrective edits, report its root
cause and scope, and wait for approval before fixing or advancing. The updated
test-plan SHA-256 is
`1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac`.
`parity_fix.md` and `parity_fix_config.md` were unchanged.

Fresh `RelWithDebInfo` configuration and the complete normal build succeeded
in `runescape-rl/build-parity-a1-revalidation`. The opt-in linked
`parity_perf_core` target also compiled. The full preservation suite passed
70/70 in 0.25 seconds, and no `known_red` tests were registered. Direct
`test_headless` execution passed 43/43 assertions, matched its same-version
500-tick replay, and completed 5,000 stability ticks over ten episodes.

The Puffer amalgamated/direct-include path compiled in both `--local` and
`--fast` modes without warnings. The optimized standalone smoke completed 100
episodes and 1,001 ticks with exit status zero. A separate Release build with
`-Wall -Wextra -Werror` compiled `fc_core`, `fc_capi`, both guardrail targets,
and `parity_perf_core` without a warning. The full normal/viewer build retained
only pre-existing viewer formatting-truncation warnings; A1 did not modify the
warning sites, and no warning came from scaffold code.

A transient strictly compiled public-API test exercised the new NPC schema and
unit accessors, attack-type defence boundary, Ranged/TBow/damage boundaries,
crystal/loadout propagation, Prayer transition/drain/loss APIs, safe defaults,
and hash-version declaration. It passed with:

```text
PASS: A1 public API and safe-default smoke; hash_version=0 loadout=1 crystal_mask=0
```

The exact A0 seed-777/action-seed-99999 preservation corpus was rebuilt against
the fresh A1 linked core and version-0 viewer hash. All saved checkpoints at
ticks 1, 2, 3, 10, 50, 100, 250, and 500 matched; the final hash remained
`de5d01d5` and final RNG state remained `6ace27c4`. One execution of the
expanded A0 linked-core workload also reproduced checksum
`d0b6c52340db0306`, 19,124 resets, and the exact per-prayer action counts. Its
single-run SPS was diagnostic only and is not a performance comparison.

A fresh Debug ASan/UBSan build passed all 70 tests with
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`. LeakSanitizer remains
disabled for the already documented managed-workspace `ptrace` limitation.
No sanitizer finding occurred.

Selected revalidation hashes:

| Artifact | SHA-256 |
|---|---|
| `fc-core/libfc_core.a` | `7bcb0f1ecccabb62e91d2b606132f0af66703b6ebe25cb8eee0dbafdf1c828dc` |
| `fc-training/libfc_capi.so` | `bf5b8ae38aa889e00de081d36998101639a2d2aa8c381bb3a6261e7319de7a6d` |
| `phase2_guardrails_core` | `6e9bb113b0ee1a7ed94f6adc4a4d7806548d519b42b5861944010f32ff7a14fc` |
| `phase2_guardrails_training` | `8ebcfa9771ac35e8c833a47f6afeab4161cd6ab248125dae495545a2728fbe66` |
| `parity_perf_core` | `b6acf264509e5d7180325affc9b9172881ea243bf0cffc4a48f644528f22b574` |
| Puffer `fight_caves` | `fa900ed7d3060a7fb7c31c82bb346153e03af5b8be95039133e07a19991f30f3` |
| transient API smoke | `f99d2ed4d6648c710271fec6f65b6a2fa9f9257fbbc2e5a78e9908c484bbfdb2` |
| transient preservation trace | `086093471075b218cb0b5291c73610d4451cb579d41aa4191f8062a339ff8dff` |

No A1 test failed, so no failure diagnosis or corrective change was needed.
No A2 test or implementation work was started, and the five-trial performance
suite was not rerun.

## A2 Workstream 1 — NPC schema/accuracy and player defence

### Focused expected-red baseline

Status: paused for review on 2026-08-02 at commit
`d1aac645726dca08c8a589678e83aa622eb01c16`. This phase added tests only; no
`fc-core`, training, viewer, configuration, or plan behavior was changed.
Workstream 2 was not started.

Two permanent linked-core test executables were added and registered as ten
separate CTest cases:

- `fc-validation/tests/parity_fix_core.c` covers `NPC-001` through `NPC-003`,
  `DEF-001` through `DEF-003`, and `DEF-005`.
- `fc-validation/tests/parity_fix_integration.c` covers the real attack-pipeline
  cases `NPC-004`, `NPC-005`, and `DEF-004`.

The integration corpus uses fixed seeds and public `fc_core` APIs. It checks
style coverage, infers hit/miss decisions from deterministic RNG draw counts,
keeps broad pending-hit styles separate from exact equipment-defence types,
and mutates the compatibility maximum to ensure it is not a combat source of
truth. It does not pin the legacy tenths damage-sampling mapping, so the later
whole-HP damage workstream will not invalidate these Workstream 1 tests.

Test source hashes at this checkpoint:

| Artifact | SHA-256 |
|---|---|
| `parity_fix_core.c` | `6ed5a93dbb7b68cc25a2b4c1f490547943d162e803d2fb13e7fa0d95ab301f1b` |
| `parity_fix_integration.c` | `63249363bc8d12ba0d37337a19603534b0fa608056a1df9f5527a9d91a156b12` |

The three plan documents were not changed. Their hashes remain:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

### Build and preservation results

GCC 13.3.0 and CMake 3.28.3 were used for a fresh `RelWithDebInfo` build:

```bash
cmake -S runescape-rl -B runescape-rl/build-parity-a2-ws1 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build runescape-rl/build-parity-a2-ws1 -j2
ctest --test-dir runescape-rl/build-parity-a2-ws1 \
  -LE known_red --output-on-failure
```

The complete normal build succeeded. The only warnings were the pre-existing
viewer formatting-truncation warnings already recorded during A0/A1; neither
new test source emitted a warning. The non-red preservation suite passed
71/71 in 0.19 seconds. This includes the existing 70 tests plus `DEF-005`,
which confirms the shared hit-chance formula remains unchanged.

A separate `RelWithDebInfo` configuration using
`-Wall -Wextra -Werror` compiled `fc_core`, `parity_fix_core`, and
`parity_fix_integration` cleanly. A Debug ASan/UBSan build used
`-fsanitize=address,undefined -fno-omit-frame-pointer`; all ten focused tests
reached the same intended assertions, `DEF-005` passed, and no sanitizer or
undefined-behavior diagnostic occurred. Leak detection used the previously
documented `detect_leaks=0` setting required by the managed `ptrace`
environment.

### Expected failures and diagnosis

The temporary `known_red` run produced nine assertion-level failures in 0.01
seconds. Every test compiled, started, completed its deterministic setup, and
failed at its intended parity assertion:

| Test | First observed mismatch |
|---|---|
| `NPC-001` | Tz-Kih `ranged_level=20`; expected `30` |
| `NPC-002` | Tok-Xil Ranged maximum `140`; expected `130` tenths |
| `NPC-003` | Tok-Xil melee offensive roll `8,256`; expected `5,696` |
| `NPC-004` | Tok-Xil adjacent seed 9 used the wrong style-specific roll |
| `NPC-005` | spawned Tok-Xil compatibility maximum `140`; expected `130` |
| `DEF-001` | SOTA Stab defence roll `19,440`; expected `19,260` |
| `DEF-002` | isolated negative-bonus roll `3,476`; expected `3,432` |
| `DEF-003` | physical defence did not use `Defence + 8` |
| `DEF-004` | Tz-Kih ignored Stab defence; 500/512 guarded attacks dealt nonzero damage |

Read-only source diagnosis found two planned A1 compatibility causes:

1. `fc_npc.c` still contains the legacy table values. In addition to the
   first mismatches above, the retained table has zero/legacy Magic inputs,
   Tok-Xil Attack 120 and Ranged max 140, Ket-Zek 54/49 maxima and Attack 240,
   reversed Jad Ranged/Magic 95/97 maxima with Attack/Ranged both 480, Crush
   mappings for the planned Stab NPCs, and no healer Ranged-defence bonus.
   Both real attack paths still construct accuracy from `stats->att_level`
   regardless of the selected style, and the generic primary-style path still
   reads mutable `npc->max_hit_tenths`.
2. `fc_combat.c::fc_player_def_roll` selects the new five-way equipment field
   but retains the old effective-level arithmetic: physical defence uses
   `Defence + 9`, while Magic defence combines floating-point 30/70 terms,
   truncates once, and then adds 9. The Tz-Kih, Ket-Zek, and Jad table entries
   also still route their melee attacks to Crush rather than Stab.

The affected production scope is bounded to the authoritative NPC table,
spawn compatibility maximum, Jad/generic attack selection, and player defence
formula in `fc-core`. Correcting the table will intentionally affect NPC
hit/miss outcomes and maxima, healer Ranged defence, and the target-Magic
input later consumed by the TBow workstream. Existing `fc_state.c` prayer-drain
normalization already uses the style-named maximum accessor, and
`fc_tick.c` already reads `ranged_def_bonus`; these consumers were checked but
do not require a parallel formula.

No corrective edit was made after these failures. The workstream remains at
its expected-red review boundary pending approval to implement the planned
Workstream 1 behavior.

### Workstream 1 correction attempt and paused cross-workstream failure

Implementation was approved and started on 2026-08-02. The bounded production
changes completed before validation were:

- replace the A1 NPC fallback values with the exact Workstream 1 table;
- select Attack, Ranged, or Magic level only after the real attack style is
  chosen;
- select Stab/Crush/Ranged/Magic through one core-owned style-to-type helper;
- obtain both Jad and generic NPC combat maxima from the style-named accessor
  instead of mutable `FcNpc.max_hit_tenths`;
- validate all supported NPC maxima during `fc_init` in release builds and
  abort with the NPC type and all three tenths maxima if the hardcoded table is
  invalid; and
- use integer `Defence + 8` for physical defence and separately truncated
  `3 * Defence / 10 + 7 * Magic / 10 + 8` for Magic defence.

The focused targets compiled successfully. Running the nine temporarily
labelled Workstream 1 cases produced eight passes and one newly exposed
failure:

```text
8/9 passed
FAIL DEF-003: loadout 2 contains a combat skill below 1
```

This is an assertion-level failure, not a compiler, crash, setup, or sanitizer
failure. The formula assertions that previously stopped `DEF-003` now pass.
The remaining assertion reached the `FC_LOADOUT_LOW_DEF_RCB` row, whose
Attack, Strength, and Magic levels are still zero. Read-only inspection found
the same zero fields in loadouts 2 through 8; the two original loadouts already
use the OSRS minimum of 1.

The failure exposes a sequencing dependency in the approved documents:

- `parity_fix.md` section 2 and `DEF-003` require every normal loadout combat
  skill to be at least 1, and Gate B requires every `DEF-*` case green.
- The exact edits replacing all zero Attack/Strength/Magic fields with 1 are
  also listed under section 6, and the approved A2 grouping assigns loadout
  corrections to Workstream 2/Gate C.

The default SOTA loadout is unaffected because its skills are already 1/1/99/
99/99/1. The affected alternate loadouts would gain the required minimum
Attack, Strength, and Magic values; Magic is immediately relevant to their
new Workstream 1 Magic-defence roll, while Attack and Strength remain outside
the current Ranged-only combat path.

Per the diagnose-before-repair rule, no loadout, test, CTest label, plan, or
other source was changed after this failure. The `known_red` labels remain in
place, the full preservation/strict/sanitizer post-implementation gates were
not started, and Workstream 1 is paused for review of whether the minimum-skill
portion of the planned loadout correction should be pulled into Gate B or the
`DEF-003` loadout-wide assertion should remain deferred to `LOAD-003`/Gate C.

### Approved resolution and Gate B completion

The minimum-skill sequencing dependency was reviewed and approved on
2026-08-02. Per that approval, only the zero Attack, Strength, and Magic fields
in loadouts 2 through 8 were changed to the OSRS minimum of 1. No other
Workstream 2 loadout total, equipment metadata, Ranged formula, conditional
effect, or test was pulled forward.

The focused targets were rebuilt, then all nine temporarily labelled cases
were rerun before label removal:

```text
9/9 known-red-labelled NPC/DEF cases passed
```

The nine temporary `known_red` labels were then removed. Reconfiguration
confirmed zero registered `known_red` cases. The final focused suite passed
10/10, including the unchanged `DEF-005` probability formula, in 0.02 seconds.

The fresh `RelWithDebInfo` normal path completed successfully:

```bash
cmake -S runescape-rl -B runescape-rl/build-parity-a2-ws1 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build runescape-rl/build-parity-a2-ws1 -j2
ctest --test-dir runescape-rl/build-parity-a2-ws1 --output-on-failure
```

All 80 CTest entries passed in 0.22 seconds. This includes every focused
`NPC-*`/`DEF-*` test plus the existing movement, LOS, collision, NPC behavior,
wave, observation, reward, training-adapter, and static contract guardrails.
Direct `test_headless` execution passed 43/43 assertions, matched all 500 ticks
of its same-version deterministic replay, and completed 5,000 stability ticks
over ten episodes with a maximum observed wave of 7.

A separate `RelWithDebInfo` build using `-Wall -Wextra -Werror` compiled these
targets without a warning:

- `fc_core`
- `fc_capi`
- `parity_fix_core`
- `parity_fix_integration`
- `phase2_guardrails_core`
- `phase2_guardrails_training`

The complete viewer build retained only its already documented formatting-
truncation warnings. No changed production or validation source emitted a new
warning.

Both direct-included Puffer standalone paths compiled successfully:

```bash
./runescape-rl/fc-training/build.sh --local
./runescape-rl/fc-training/build.sh --fast
./pufferlib_4/fight_caves
```

The optimized runtime smoke completed 100 episodes and 722 ticks with exit
status zero. Its reported 507,022 SPS is a short time-seeded diagnostic, not a
replacement for `PERF-002` or `PERF-003` and not a comparison with A0.

A Debug ASan/UBSan build compiled the complete normal tree and passed all
80 CTest entries in 0.59 seconds with:

```text
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
```

No address or undefined-behavior diagnostic occurred. Leak detection remains
disabled only for the previously documented managed-workspace `ptrace`
limitation.

The final simplification/stale-consumer audit found:

- combat no longer reads `FcNpc.max_hit_tenths`; its only `fc_npc.c` write is
  the required primary-style compatibility value at spawn;
- direct `stats->att_level` selection exists only inside the centralized
  style-to-level helper;
- `fc_state.c` continues to use the style-named maximum accessor for Tz-Kih
  prayer-drain normalization;
- `fc_tick.c` continues to consume the renamed `ranged_def_bonus` for player
  Ranged accuracy;
- no loadout combat skill remains below 1; and
- no Workstream 1 `known_red` label or A1 NPC/defence fallback path remains.

Selected final hashes:

| Artifact | SHA-256 |
|---|---|
| `fc_npc.c` | `a7ee49748e7160381eb945761a69444dd6dd3d80faf513e8395ca3bc8ef594b4` |
| `fc_combat.c` | `63d32dc46d44e59f60b5af2b6082328e10ba30499c322a1acd831aff7251fd5a` |
| `fc_state.c` | `ee45f80405b8c154df2a08d135b14cda6a8a8540dc98ef0020cce07a6161c72d` |
| `fc_player_init.h` | `982a506db5873b632638b890f96cc3c1c8091dce197988a410e2ceafcb2caacd` |
| linked `libfc_core.a` | `780b4579faaa5e3593d0256492eb7696fa6cc3b83f791aba2e1a5a92df9442cb` |
| `parity_fix_core` | `f0410d658c8fbe3b4adea7e72771bc4cbd7a6aaf9462163fc142304e6589ab3d` |
| `parity_fix_integration` | `dcd52b1f93604fdc076b9730ee85f398558ca7d6d8b0e65dba578af1f6aeaf49` |
| `libfc_capi.so` | `b8ab89f3a7f30a082f784e561b8d568d5fa9992283f772a82e47618cce06a50d` |
| Puffer `fight_caves` | `47e4f2f578be2b4eb0879fb04c9ca4ef0afd0478267598e7d1bfe44c3bb91783` |

The three plan files remain byte-identical to their pre-workstream hashes.
The approved sequencing resolution and its scope are recorded here rather
than rewriting the plan. `PERF-001` through `PERF-003` and `TRAIN-001` were not
run because their post-fix comparisons belong to the final release gate, not
this intermediate mechanics gate.

Gate B is complete. Workstream 2/Gate C has not started.

## A2 Workstream 2 — Ranged/TBow/damage/crystal/loadouts

### Focused expected-red baseline

Status: paused for failure review on 2026-08-02. Only the permanent focused
tests and their CTest registrations were added. No Workstream 2 production
correction, CTest-label removal, or Workstream 3 work has started.

The exact Gate C scope from the unchanged plans was reviewed before editing:

- base Ranged formulas and all nine raw loadout vectors (`RNG-*`);
- staged-integer Twisted-bow multipliers and effect gates (`TBOW-*`);
- whole-HP damage RNG with tenths storage at all three call sites (`DMG-*`);
- exact per-piece crystal modifiers and Bowfa-only gating (`CRY-*`); and
- the nine-row loadout table, ammo/effect metadata, relationships, and reset
  propagation (`LOAD-*`).

The focused layout now contains:

- additions to `runescape-rl/fc-validation/tests/parity_fix_core.c` for
  `RNG-001`, `TBOW-001` through `TBOW-003`, `CRY-001` through `CRY-004`, and
  `LOAD-001` through `LOAD-004`; and
- new `runescape-rl/fc-validation/tests/parity_fix_damage.c` for `DMG-000`
  through `DMG-008`.

Each named case is registered separately. Thirteen assertions for known A1
runtime drift temporarily carry `known_red`; the eight already-correct
preservation cases do not. The slower nine-value `FC_ACTIVE_LOADOUT` compile
matrix remains deferred to the release gate exactly as allowed by `LOAD-004`;
the current active-loadout reset and per-row equivalent-player checks run now.

The fresh linked-core setup and focused build were:

```bash
cmake -S runescape-rl -B runescape-rl/build-parity-a2-ws2 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build runescape-rl/build-parity-a2-ws2 -j2 \
  --target parity_fix_core parity_fix_damage parity_fix_integration
```

All three targets compiled and linked successfully. `git diff --check` also
passed. There was no compile failure, crash, or test-setup failure. The later
complete build retained only the pre-existing `fc-viewer/src/ui.c`
format-truncation warnings previously recorded for Workstream 1; no new core
or validation warning was emitted.

The required preservation command was:

```bash
cmake --build runescape-rl/build-parity-a2-ws2 -j2
ctest --test-dir runescape-rl/build-parity-a2-ws2 \
  -LE known_red --output-on-failure
```

It passed 88/88 entries in 0.24 seconds. In addition to the complete prior
suite, the following new Workstream 2 invariants were green before correction:

- `RNG-001`: all nine raw base attack/max-hit vectors and integer rounding;
- `TBOW-003`: only TBows read target Magic, targetless TBow is raw, and
  synthetic crystal metadata cannot enter the wrong effect branch;
- `CRY-003`: crystal item IDs, explicit masks, and active reset propagation;
- `LOAD-002`: weapon kind, ammo use/count, Bowfa zero-ammo firing, and exactly
  one ammo decrement for every ammo-consuming loadout;
- `LOAD-004`: per-row player-field propagation and compile-selected reset;
- `DMG-003`: misses consume only the accuracy draw while retaining attack
  cycle, ammo, pending-hit, and attempt metrics;
- `DMG-006`: queued 170-tenths damage remains 170 through HP, event,
  render-snapshot, observation, and reward consumers; and
- `DMG-008`: Tz-Kih damage-plus-one coupling, accurate/blocked zero behavior,
  capped Prayer loss, metrics, and observation reporting.

The expected-red command was:

```bash
ctest --test-dir runescape-rl/build-parity-a2-ws2 \
  -L known_red --output-on-failure
```

All 13 registered cases failed at their assertion boundary in 0.01 seconds:

| Case | Preserved failure evidence |
|---|---|
| `TBOW-001` | Magic 0 returned `39%/53%`, expected `40%/54%`. |
| `TBOW-002` | The exhaustive staged oracle first diverged at Magic 0 with the same `39/53` versus `40/54`. |
| `CRY-001` | Helm mask 1 stayed `27,621/26`, expected `29,002/26`. |
| `CRY-002` | The cross-product first stopped in its TBow row because the legacy TBow truncation differs before the Bowfa rows are reached. |
| `CRY-004` | Mask 1 failed the exact order/purity/raw-stat vector because no modifier was applied. |
| `LOAD-001` | `RCB_PURE.def_stab` was 35, expected 48. |
| `LOAD-003` | `LOW_DEF_RCB` and `RCB_PURE` combat fields were not identical. |
| `DMG-000` | A synthetic zero-HP player maximum consumed an RNG draw. |
| `DMG-001` | A seed whose whole-HP raw outcome is zero produced player/NPC `10/10`; the NPC result should remain zero. |
| `DMG-002` | The independent RNG oracle likewise observed NPC max 3/raw 0 as 10 instead of 0. |
| `DMG-004` | The live player tick path produced 85 tenths, proving tenth-step damage at a 170-tenths maximum. |
| `DMG-005` | Full-crystal Bowfa remained at base max 26 instead of final max 29. |
| `DMG-007` | A successful generic player roll returned 24 tenths, violating whole-HP granularity and the player minimum-hit rule. |

### Read-only failure diagnosis

Every failure is reproducible and maps to the planned A1 fallback or stale
table; no unrelated regression was found:

1. `fc_tbow_accuracy_multiplier_pct` and
   `fc_tbow_damage_multiplier_pct` still use the A1-preserving floating-point
   expression. It retains intermediate fractions instead of separately
   truncating `3*M/10`, the linear terms, and the square term. That is the
   direct cause of both `TBOW-*` failures. `CRY-002` deliberately crosses all
   weapon kinds, so its first row-level mismatch is this same TBow issue; after
   the TBow correction, its Bowfa masks will still expose the separately
   missing crystal modifier.
2. `fc_player_ranged_attack_roll` and
   `fc_player_ranged_final_max_hit_hp` contain no Bowfa/crystal branch. This
   causes `CRY-001`, the Bowfa portion of `CRY-002`, `CRY-004`, and the Bowfa
   vector in `DMG-005`.
3. The three planned stale pure rows remain unchanged: `RCB_PURE` and
   `MSBI_PURE` still use `35/45/54/40/38`, and `BLOWPIPE_PURE` still uses
   `32/42/51/37/35`. This causes `LOAD-001` and `LOAD-003`. The previously
   approved minimum skill correction is green and is not part of this failure.
4. Both public roll wrappers still call one A1 compatibility function,
   `fc_roll_legacy_tenths_damage`, which samples `0..final_max_hit_hp*10` and
   advances RNG even when the maximum is zero. The player and NPC wrappers
   therefore do not yet have distinct zero behavior. This causes `DMG-000`,
   `DMG-001`, `DMG-002`, and the direct-helper portion of `DMG-007`.
5. The live player, Jad, and generic-NPC attack sites still pass tenths maxima
   directly to `fc_rng_int`; the player site also uses the deliberately
   ambiguous A1 compatibility helper `fc_player_ranged_max_hit`. This is the
   cause of `DMG-004` and the functional minimum-hit failure in `DMG-007`.

The blast radius is confined to the planned Gate C combat distribution and
conditional-effect/loadout values. Observation, reward, HP, Prayer, pending
hit, render, ammo, reset, miss-path RNG, and prior NPC/defence behavior all
passed their preservation assertions.

Per the diagnose-before-repair rule, no production source or test expectation
was changed after these failures. All 13 temporary labels remain registered,
and Workstream 2 is paused for review before implementing the planned
corrections.

The three plan files remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

### Correction implementation and Gate C completion

After review and approval, the planned Workstream 2 corrections were
implemented without changing the scope or expectations of any plan:

- TBow accuracy and damage multipliers now use the specified signed 64-bit,
  stage-by-stage integer arithmetic and truncation order, with target Magic
  clamped before evaluation.
- Bowfa crystal armour accuracy and damage modifiers are computed from the
  sanitized three-piece mask in basis points. The effect is exclusive to
  Bowfa and cannot stack with or leak into TBow or other weapons.
- Player and NPC damage now sample whole HP and convert the result to tenths
  afterward. Positive player hits clamp a sampled zero to one HP; NPC hits
  preserve zero. A non-positive maximum consumes no damage RNG draw, while a
  positive successful roll consumes exactly one.
- The live player, generic NPC, and Jad attack paths now use the explicit
  whole-HP maximum and damage helpers. Misses retain their pre-existing
  no-damage-draw behavior.
- The stale `RCB_PURE`, `MSBI_PURE`, and `BLOWPIPE_PURE` defensive bonuses
  were corrected to the planned values.
- Initialization and reset now apply every active loadout combat field through
  one shared helper. Release validation covers all loadout rows, their skills,
  weapon/ammo/crystal metadata, capacities, and positive attack/max-hit
  results against every NPC target.
- The ambiguous A1-only max-hit compatibility API and the legacy tenths damage
  sampler were removed after all production call sites migrated.

The focused implementation run first retained the 13 `known_red` labels as a
selection mechanism. All 13 formerly red cases passed in 0.02 seconds. The
temporary labels were then removed, and `ctest -N -L known_red` reported zero
tests. The complete parity-labelled set passed 31/31 in 0.04 seconds.

Gate C validation results:

| Validation | Result |
|---|---|
| Normal `RelWithDebInfo` build | Passed. |
| Full CTest suite | 101/101 passed in 0.24 seconds. |
| Direct `test_headless` run | 43/43 assertions passed; the 500-tick deterministic replay matched and 10 episodes completed 4,971 ticks through maximum wave 7. |
| Strict warning build | Passed with `-Wall -Wextra -Werror` for `fc_core`, `fc_capi`, both phase-two guardrails, and all three parity executables. |
| Direct-included Puffer builds | `fc-training/build.sh --local` and `--fast` both passed. |
| Puffer runtime smoke | 100 episodes and 954 ticks completed; the reported 515,119 SPS is diagnostic only and is not a performance-gate comparison. |
| Debug sanitizers | 101/101 passed in 0.77 seconds under ASan/UBSan with halt-on-error; leak detection remained disabled because of the previously recorded ptrace limitation. |
| Stale-symbol/source audit | No legacy tenths sampler, ambiguous max-hit API, float TBow path, direct tenths-max RNG call, or stale pure-defence vector remains. |

The canonical Workstream 2 source/artifact hashes after validation are:

| Artifact | SHA-256 |
|---|---|
| `fc-core/include/fc_combat.h` | `9597a4bc89acbb5cb7dd421b37a17546cbfc4aa91ec024452e4a8a0e17f5b1d0` |
| `fc-core/include/fc_player_init.h` | `ddb09e4dd033fec4f52874a7b5af06ca9a4c603148db347d8a226b7e23a4bf92` |
| `fc-core/src/fc_combat.c` | `f0c1a9f578401d4735b717b9788376cc13a195e883b9984965f88027cc1ff6a5` |
| `fc-core/src/fc_npc.c` | `226e3aba3282bf0568f8ea2cced80373dcb3a41f6eca375a8e49b7a9930f386f` |
| `fc-core/src/fc_state.c` | `a4c4d0cec23394d3d9e4c2d2c51b12b842a59e9c1bd9ab54ac43aac9451e2000` |
| `fc-core/src/fc_tick.c` | `9885352dae1f95a0b52fd4e5edf1ad1e7e2c5c537cdd53a075025ec7eb438471` |
| `fc-validation/tests/parity_fix_core.c` | `41ae87b6e89d2214e94bfa4a5249dce3d52ff12c5c56cc2ccc9e0143bbac5067` |
| `fc-validation/tests/parity_fix_damage.c` | `7be2e554c1c10b64bef04d878c3628a9b5227456c8c81c00b8df449536918b1c` |

The expanded A0/post-fix performance comparisons, bounded training-health
smoke, and full active-loadout compile matrix remain assigned to their later
plan gates; they were not substituted with the diagnostic runtime smoke here.

The three plan files remain byte-identical at the hashes recorded above. Gate
C is complete, and no Workstream 3 implementation has begun.

## A2 Workstream 3 — prayer behavior

### Focused expected-red baseline

Status: paused for failure review on 2026-08-02. Only the permanent focused
tests and their CTest registrations were added. No Workstream 3 production
correction, `known_red` label removal, contract/config integration, or later
workstream work has started.

The exact `PRAY-001` through `PRAY-008` scope from the unchanged plans was
reviewed before editing:

- stable prayer command IDs 0-7, dimension/validation, and mask canaries;
- the full transition-result truth table, including unsuccessful zero-Prayer
  activation and same-final-state flick metrics;
- activation, direct-switch, OFF, and explicit-flick drain decisions;
- resistance boundaries for bonuses 0, 6, 8, and 11, multi-drain behavior,
  fractional-counter persistence, depletion, potion, and reset behavior;
- one centralized depletion invariant exercised directly and through passive
  drain and Tz-Kih, plus same-tick prayer/potion and hit-ordering cases;
- immutable ordinary start-of-tick snapshots across select, OFF, direct
  switch, and both flick forms;
- two 100-tick matching-attack flick traces and an uninterrupted-drain
  control; and
- Jad Magic/Ranged reveal, one-decision window, pre-action delayed lock,
  late-action control, longer flight, post-lock flick, impact, observation,
  minimum-delay clamp, and ordinary-projectile aging.

The required direct-prayer-switch evidence was captured before the tests were
written. A temporary JUnit fixture was run through the complete trusted
reference implementation at
`/home/joe/Desktop/projects/runescape-rl/runescape-rl-reference/current_fightcaves_demo`.
The Gradle test passed and printed:

```text
PRAYER_SWITCH_ORACLE before=24 after_action=24 after_tick=36
```

The fixture SHA-256 was
`f9291289e92807db1becf3b0d8108c122c571a9c4249229ae5fb3e9a20b7569c`.
Its two source authorities matched the canonical reference hashes:

| Reference source | SHA-256 |
|---|---|
| `PrayerDrain.kt` | `3457d005236f9de1365e7c332971e9abdd8b3d89b599a2e3e93b2b023053e69b` |
| `FightCaveBackendActionAdapter.kt` | `ceb0cd8b428796792a13f6f7993fb59fb7aa2fb10d08adc538ebae9223eb397a` |

This confirms the unchanged plan contract: a direct Magic-to-Range action
preserves the existing 24 counter on the action edge and advances it to 36
when drain is processed. Both temporary oracle work directories were removed
after capture; the executable oracle is a one-time parity fixture, not part of
the recurring CTest suite.

The new linked-core validation target is
`runescape-rl/fc-validation/tests/parity_fix_prayer.c`, SHA-256
`2279670cca8c08c996355b2fc5dabda7b11378baa7150c899e82e75ef640d3d7`.
It contains separately selectable `PRAY-001` through `PRAY-008` cases and is
registered through `runescape-rl/fc-validation/CMakeLists.txt`. All eight
currently carry temporary `known_red` labels.

The fresh build command was:

```bash
cmake -S runescape-rl -B runescape-rl/build-parity-a2-ws3 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build runescape-rl/build-parity-a2-ws3 -j \
  --target parity_fix_prayer
```

The linked `fc_core` and prayer target compiled successfully. The later full
build also succeeded. No new core or validation warning was emitted; the only
diagnostics were the already-recorded `fc-viewer/src/ui.c` `snprintf`
truncation warnings. A separate build of `fc_core` and `parity_fix_prayer`
also passed with `-Wall -Wextra -Werror`. `git diff --check` passed.

The preservation gate was:

```bash
cmake --build runescape-rl/build-parity-a2-ws3 -j
ctest --test-dir runescape-rl/build-parity-a2-ws3 \
  -LE known_red --output-on-failure
```

All 101 non-red tests passed in 0.26 seconds. This includes the complete prior
suite and all 31 already-green Workstream 1/2 parity tests. No preservation
regression was found.

The expected-red command was:

```bash
ctest --test-dir runescape-rl/build-parity-a2-ws3 \
  -L known_red --output-on-failure
```

All eight cases reached their intended assertion boundary in 0.01 seconds:

| Case | Preserved failure evidence |
|---|---|
| `PRAY-001` | `FC_PRAYER_DIM` was 5, expected 8. IDs 0-7, invalid values -1/8, and the declared-buffer canaries were reached first without failure. |
| `PRAY-002` | Off plus `FLICK_MAGIC` returned prior/requested/actual `0/0/0`, all edge flags zero, and no explicit-flick classification. |
| `PRAY-003` | Same-prayer flick advanced the counter from 24 to 36 instead of preserving 24. The executable direct-switch golden fixture passed before this assertion. |
| `PRAY-004` | A cross-prayer explicit flick ended on Range with counter 55 instead of ending on Magic with the preserved counter 43. All bonus, equality, strict-boundary, multi-drain, OFF, and direct-switch assertions passed first. |
| `PRAY-005` | The direct centralized-loss API reduced points by 10 to zero but left live Magic and counter 37 instead of forcing Off/zero. |
| `PRAY-006` | Starting Off and selecting Magic queued an ordinary Magic attack with live snapshot Magic (`3`) instead of tick-start Off (`0`). |
| `PRAY-007` | The first matching flick tick blocked the hit but advanced the counter to 12 and emitted neither the per-tick transition flag nor episode switch metric. |
| `PRAY-008` | Close Magic reveal queued lock tick 1 and post-aging delay 1 instead of lock tick 2 and clamped post-aging delay 2; style and unset snapshot were correct. |

Every result is an assertion-level known-red failure. There was no compile
failure, crash, timeout, sanitizer symptom, fixture error, or failure before
the intended parity assertion.

### Read-only failure diagnosis

The eight failures map directly to the planned A1 scaffolding and current
runtime paths:

1. `fc_contracts.h` retains the five-action runtime dimension while IDs 5-7
   remain reserved. Derived action/mask sizes therefore still describe the
   pre-prayer contract. This is the direct cause of `PRAY-001`.
2. `fc_prayer_apply_action` only handles commands 0-4. It has no compound
   OFF-then-ON cases and cannot populate the requested/performed/succeeded
   flick distinctions. This causes `PRAY-002` and the wrong final prayers in
   `PRAY-003`/`PRAY-004`.
3. `fc_prayer_drain_tick` explicitly ignores its transition argument and
   decides only from active-at-start and active-at-end. An unimplemented flick
   that remains active therefore accrues 12 like uninterrupted prayer. This
   causes the counter failures in `PRAY-003`, `PRAY-004`, and `PRAY-007`.
4. `fc_prayer_apply_loss_tenths` only clamps/subtracts points. Passive drain
   separately implements depletion cleanup, while the Tz-Kih resolver still
   subtracts Prayer directly. The shared post-loss Off/counter-zero invariant
   is therefore absent, causing `PRAY-005`.
5. Generic NPC attacks store `p->prayer` after action/drain processing instead
   of the already-captured `p->prayer_at_tick_start`. This causes `PRAY-006`
   and would also discard the immutable start snapshot in the passive-first
   depletion ordering case.
6. `process_player_actions` marks a prayer transition only when the final live
   enum differs from its prior value. A valid same-prayer OFF-then-ON flick
   cannot increment the per-tick or episode metric under that rule, which is
   the metric half of `PRAY-007`.
7. Jad currently queues ranged/magic locks at `state->tick + 1`; close Magic
   is clamped only to delay 2. Pending resolution fills an unset lock after the
   current tick's action rather than in a dedicated boundary/pre-action phase.
   These are the timing causes captured by `PRAY-008`.

The blast radius is confined to the planned prayer/action/timing tranche. The
existing ordinary hit resolver correctly consumes an immutable snapshot once
one is supplied, the exact resistance formula and strict-greater-than boundary
passed, OFF preserves the fractional counter, direct switches currently
advance it as required, and all prior mechanics remain green.

Per the diagnose-before-repair rule, no production source or test expectation
was changed after these failures. All eight temporary labels remain, and
Workstream 3 is paused for review before implementing the planned corrections.
The action/mask/Puffer/config/checkpoint contract integration remains assigned
to the later `parity_fix_config.md` sequence and was not started here.

The three plan files remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

### Correction implementation — validation pause

Implementation began after the expected-red failures were reviewed and
approved. The planned core changes now present in the working tree are:

- the prayer head dimension is 8 and all core/Puffer sizes remain derived;
- compound flick commands emit explicit OFF/ON transition metadata;
- drain distinguishes activation, uninterrupted/direct-switch, OFF, and
  successful explicit-flick ticks;
- passive drain and Tz-Kih use the centralized post-loss invariant;
- ordinary NPC attacks and Jad melee store `prayer_at_tick_start`;
- Jad ranged/magic uses a minimum queued delay of 3, lock tick `T+2`, and a
  dedicated end-of-tick boundary lock before the next observation/action; and
- pending-hit resolution no longer falls back to re-reading live Prayer.

The first focused implementation run built `parity_fix_prayer` and the derived
Puffer guardrail successfully. `PRAY-001` through `PRAY-007` passed. `PRAY-008`
then stopped at:

```text
FAIL PRAY-008: long Magic reveal contract mismatch
```

Read-only diagnosis found a validation-fixture defect rather than a mechanics
failure. `test_long_jad_delay` places the player at x=10 and Jad at x=30, a
20-tile anchor distance. Jad's authoritative distance-style attack range is
14, so the NPC correctly queues no attack and the intended long-flight
assertion is unreachable. An in-range x=24 fixture gives distance 14 and a
calculated Magic delay of 4; after the existing same-tick aging it leaves 3
ticks, which exercises the intended `>2` longer-flight branch while preserving
the exact same production behavior and plan expectation.

The proposed correction is test-only: change the long-delay fixture's Jad
x-coordinate from 30 to 24. This is not a plan pivot. Per the failure-review
rule, neither the fixture nor production code was changed after the failure;
implementation validation is paused pending approval of that correction.

The x=24 correction was approved and applied. All eight focused `PRAY-*`
cases then passed in 0.01 seconds, and their temporary `known_red` labels were
removed; `ctest -N -L known_red` reported zero tests.

The subsequent 109-entry preservation run exposed one stale manual test in
`fc-viewer/tests/test_headless.c`: 108 CTest entries passed, while
`test_headless` passed 41/43 internal assertions and failed its two Jad prayer
reward-timing assertions. That fixture queues a pending Jad hit with the old
`T+1` lock and calls `fc_resolve_player_pending_hits` directly, expecting the
resolver to capture live Prayer. The approved implementation deliberately
removed that live-prayer resolver behavior; Jad now locks only in the
tick-boundary phase, which the manual fixture bypasses. Its snapshot therefore
remains `-1`, and impact correctly treats the missing immutable snapshot as
Off rather than re-reading live Prayer.

The proposed correction is again test-only: make this reward-integration
fixture represent the new boundary explicitly by assigning the immutable
Magic snapshot at the simulated lock boundary, assert that no reward fires at
the boundary, and resolve it on the following simulated tick. `PRAY-008`
already exercises the production boundary phase end to end, so exposing a
production test hook or restoring resolver-side locking would duplicate or
contradict the new source of truth. No source was changed after this second
failure; validation is paused for review.

The manual timeline correction was approved and applied. The focused rerun
passed all eight `parity_pray_001` through `parity_pray_008` cases in 0.01
seconds, `test_headless` passed, and `ctest -N -L known_red` reported zero
tests. A fresh complete `RelWithDebInfo` build and CTest run then passed all
109/109 entries in 0.26 seconds. The now-inaccurate temporary-label comment in
`fc-validation/CMakeLists.txt` was removed; no plan document changed.

### Strict-warning validation pause

The affected `fc_core`, `fc_capi`, phase-two guardrail, and all four parity
targets compiled successfully with `-Wall -Wextra -Werror`. The same strict
build stopped while compiling `test_headless` at:

```text
runescape-rl/fc-viewer/tests/test_headless.c:253:10: error: unused variable ‘err’ [-Werror=unused-variable]
```

Read-only attribution shows that `char err[128];` in `test_action_mask` is not
used anywhere in that function, is outside the approved Jad-timeline edit,
and has existed unchanged since commit `2f3e48c837`. Normal builds and runtime
tests accept it because they do not promote this warning to an error. The
failure is therefore pre-existing test-source warning debt exposed by adding
`test_headless` to this workstream's strict target set, not a prayer behavior,
contract-dimension, or fixture-timeline failure.

The smallest proposed correction is to delete only that unused local
declaration. No correction was applied, and the remaining direct-included
Puffer and sanitizer checks were not started, pending review under the
diagnose-before-repair rule.

The deletion was approved after reconfirming the complete `test_action_mask`
function and all `err` declarations in the file. The local at line 253 had no
read, write, macro expansion, or later scope use. Only that declaration was
deleted.

### Completed core-prayer validation

The strict-warning build was resumed without changing compiler flags. All
affected targets, including `test_headless`, compiled with
`-Wall -Wextra -Werror`; the selected strict-build runtime set passed 42/42 in
0.07 seconds. `git diff --check` also passed.

Both direct-included training build modes completed:

```text
bash runescape-rl/fc-training/build.sh --local  PASS
bash runescape-rl/fc-training/build.sh --fast   PASS
```

The optimized standalone Puffer-path diagnostic smoke completed 100 random
episodes and 937 ticks, reported 562,763 SPS, and produced no runtime or
buffer error. This short smoke is integration evidence only; it is not a
`PERF-002`, `PERF-003`, or `TRAIN-001` result and is not compared against the
A0 performance baselines.

A fresh debug build with AddressSanitizer and UndefinedBehaviorSanitizer then
passed 109/109 CTest entries in 0.78 seconds using halt-on-error. Leak
detection remained disabled for the previously recorded ptrace environment
limitation. The build printed the existing viewer `snprintf` truncation
warning at `fc-viewer/src/viewer.c:3732`; it did not occur in an affected
strict target and is unrelated to prayer behavior.

The final stale-path audit found one direct `current_prayer` subtraction, in
the centralized `fc_prayer_apply_loss_tenths` helper itself, and one live
Prayer snapshot assignment, in the authoritative end-of-tick lock-boundary
helper. It found no legacy tenths sampler, ambiguous ranged-max API, old Jad
`T+1` lock assignment, five-action Prayer dimension, stale 316-value Puffer
expectation, or remaining `known_red` label. No per-tick allocation, duplicate
Prayer-loss path, or resolver-side live-Prayer fallback was introduced.

The canonical Workstream 3 source hashes after validation are:

| Artifact | SHA-256 |
|---|---|
| `fc-core/include/fc_contracts.h` | `ab0fcb33933cd1c3bd66826f32b75b65ffe8496db613e539b6099e34d3ae0ad0` |
| `fc-core/include/fc_prayer.h` | `41f4ae3e9db703d971ce6bc0214f986510981943bd91dc86dbfedb13194d9b06` |
| `fc-core/src/fc_prayer.c` | `8faaa14b868630b03483cc62eef2530d0cc6a3668db80c6d620a1a91c9c14426` |
| `fc-core/src/fc_combat.c` | `298efbd744adc2924eaa555235ed7d31c464e01b2052479085db205a254027d6` |
| `fc-core/src/fc_npc.c` | `de8591a0d4014c941027724dd86b1e697e7d8b90bd46b317619c900f0d4c09cf` |
| `fc-core/src/fc_tick.c` | `e306fecc365da0681cb1535af46072c74156cacbb2247d8563d1d3206ad85825` |
| `fc-validation/tests/parity_fix_prayer.c` | `1043d4cc79de5915a265bad24d6ef5a158b8de1c29188dcdf2b07bc7808cf2ac` |
| `fc-viewer/tests/test_headless.c` | `b891ee1396403dd1bee8f281af85d352df64124e830b439a05b8da1923fc0af4` |

The three plan documents remain byte-identical at their recorded hashes:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

A2 Workstream 3's core Prayer mechanics are implemented and validated. Gate
D as a whole is not yet complete: the planned downstream action/mask,
observation/deadline, reward/metric, Puffer/C API/evaluator, version,
preflight/manifest, and checkpoint integration remains in the later
`parity_fix_config.md` workstreams. None of that later work was started here.

## Config workstream 1 — prayer action dimensions and masks

### Focused expected-red baseline

Status: paused for failure review on 2026-08-02. This tranche is limited to
`parity_fix_config.md` implementation-order item 2 and the single-environment
parts of `CONTRACT-001` through `CONTRACT-003`: shared dimensions and offsets,
unrestricted eight-command Prayer masks, synchronized Puffer float/native
masks, and declared-buffer canaries. Observation/deadline semantics, reward
and metric consumers, C API batch strides, evaluator/checkpoint behavior,
versions, hashing, manifests, and the complete stale-literal gate remain in
their later workstreams.

The permanent focused coverage extends
`fc-validation/tests/phase2_guardrails_training.c` and the existing static
guardrail dispatcher. It registers four separately selectable cases:

| Case | Scope | Baseline result |
|---|---|---|
| `parity_contract_001_exact_dimensions` | Compile-time/runtime dimensions, exact arrays, sizes, and full-mask offsets. | Passed. |
| `parity_contract_002_puffer_mask_values` | Reset/post-step byte/float mask equality, at least one legal value per head, all commands 0-7 legal at zero Prayer, and commands 5-7 accepted. | Passed. |
| `parity_contract_003_single_env_canaries` | Guarded 319-float Puffer observation, three-action input, 34-byte native mask, and 474-float core buffer across reset/write/step. | Passed. |
| `parity_contract_002_standalone_action_range` | The standalone random Prayer sampler consumes `FC_PRAYER_DIM` instead of an independent literal. | Expected-red assertion failure. |

The new runtime target compiled normally. The three initially green contract
cases passed in 0.00 seconds. The preservation command was:

```bash
cmake --build runescape-rl/build-parity-config-ws1 -j
ctest --test-dir runescape-rl/build-parity-config-ws1 \
  -LE known_red --output-on-failure
```

All 112 non-red tests passed in 0.25 seconds. The build emitted only the
previously recorded viewer `snprintf` truncation warnings; the affected
training guardrail emitted no warning.

The single expected-red command reached its intended assertion in 0.02
seconds:

```text
FAIL CONTRACT-002: standalone prayer sampler does not use [0, FC_PRAYER_DIM)
  fc-training/fight_caves.c: (rand() % 10 == 0) ? (float)(rand() % 5) : 0.0f
```

### Read-only failure diagnosis

The standalone diagnostic executable still samples its Prayer value with
`rand() % 5`, so it can emit only commands 0-4 even though its action buffer,
Puffer dimensions, masks, and simulator now expose commands 0-7. This does
not make rollout actions invalid or overrun a buffer, but it prevents that
executable's randomized smoke workload from exercising the three new flick
commands and violates the shared-contract requirement.

A repository scan found no second stale Prayer range in a runnable action
producer. The `rand() % 5` on the preceding attack line is only the frequency
gate for whether an attack is sampled; the head value itself uses range 9.
Similarly, `test_rand(5) == 0` in `test_headless.c` is only a sampling
frequency and its Prayer value already uses `test_rand(FC_PRAYER_DIM)`.

The smallest proposed correction is to replace only the Prayer value range in
`fc-training/fight_caves.c` with `rand() % FC_PRAYER_DIM`. No new helper or
parallel source of truth is needed. Per the diagnose-before-repair rule, that
production line and the temporary `known_red` label remain unchanged pending
approval.

The plan files remain byte-identical at their previously recorded hashes.

### Approved correction — validation pause

The one-line production correction was approved and applied: the standalone
Prayer value now samples `rand() % FC_PRAYER_DIM`. The temporary `known_red`
label was removed before the focused rerun.

The three runtime contract tests remained green, but the static sampler test
then failed with a new harness-level message:

```text
FAIL CONTRACT-002: standalone prayer-action assignment was not found
```

Read-only diagnosis confirmed that the corrected assignment is present and
uses `FC_PRAYER_DIM`. The edit formatted the conditional across two lines,
while the test's `re.search(r"env\.actions\[2\]\s*=\s*(.*?);", text)` uses
Python's default mode, where `.` does not match a newline. The test therefore
stops before inspecting the correct range. A multiline source search finds
the assignment at lines 83-84, and `git diff --check` passes.

This is a test-parser formatting defect, not a production contract failure.
The smallest proposed correction is test-only: add `flags=re.DOTALL` to that
narrow assignment search so it accepts normal multiline C formatting while
retaining the existing semantic assertion that the expression contains
`rand() % FC_PRAYER_DIM`. No test or source was changed after this failure,
and broader validation is paused pending review.

The multiline-regex correction was approved and applied. All four focused
contract cases then passed in 0.02 seconds, and `ctest -N -L known_red`
reported zero tests.

### Completed action/mask-contract validation

The final production change in this workstream is deliberately one source
line: the standalone smoke generator now derives its Prayer value range from
`FC_PRAYER_DIM`. Existing core/Puffer mask and buffer writers already derived
their bounds from the shared constants and needed no production rewrite.

The complete normal suite passed 113/113 in 0.28 seconds. The affected
direct-included training guardrail compiled with `-Wall -Wextra -Werror`; its
pre-existing native-mask case plus the four new contract cases passed 5/5.
Both `fc-training/build.sh --local` and `--fast` passed. The optimized
standalone diagnostic then completed 100 random episodes and 697 ticks,
reporting 549,251 SPS with no runtime or buffer error. That SPS is diagnostic
only and is not a post-fix performance-gate result.

A fresh ASan/UBSan build passed 113/113 tests in 0.82 seconds with
halt-on-error and leak detection disabled for the previously recorded ptrace
limitation. The only build diagnostic was the existing unrelated viewer
`snprintf` warning. `git diff --check` passed.

During the required simplification pass, the older native-mask adapter test
was changed to reuse the new byte/float-mask assertion helper instead of
retaining a second copy of the head iteration and legality checks. Normal,
strict, and sanitized reruns of the affected five cases all passed, followed
by fresh complete normal and sanitizer suite runs at the totals above.

The final runnable-source audit found no Prayer action producer using a
five-value range and no remaining `known_red` label. The standalone generator
has one authoritative value expression:

```text
rand() % FC_PRAYER_DIM
```

Final workstream artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `fc-training/fight_caves.c` | `5af9e52257154d0390727c797bdf4f07285f393aa8cf12eae036ea661bcc1568` |
| `fc-validation/tests/phase2_guardrails_training.c` | `a8f3031c5279935e12b31abb23c584acd3e8c1b99fe24d0ed76de9472d586ae7` |
| `tools/validation/tests/phase2_static_guardrails.py` | `323d03a1a1b674322bb58d6a717c93307efd84724e3641dcc5dc073661f8822d` |
| `fc-validation/CMakeLists.txt` | `a028efcbe579e6e917d5ad834d8d930743da702617143d5e18f1a1dd3cc29f2d` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config workstream 1, shared Prayer action dimensions and masks, is complete.
The C API batch-stride, evaluator, and checkpoint portions of
`CONTRACT-002`/`CONTRACT-003` remain assigned to their later integration
workstreams. Observation/deadline semantics have not begun.

## Config workstream 2 — observation and deadline semantics

### Focused expected-red baseline

Status: paused for failure review on 2026-08-02. This tranche is limited to
`parity_fix_config.md` implementation-order item 3: final Prayer-state and
loss observations, actionable Prayer-window boundaries, deadline urgency,
Jad timing, and incoming-aggregate ablation behavior. Reward and metric
consumers, Puffer/C API/evaluator integration, versions, hashing,
preflight/manifest behavior, and checkpoint isolation remain assigned to
later workstreams.

Four focused cases were added to `fc-validation/tests/parity_fix_prayer.c`
and registered separately in CTest:

| Case | Scope | Baseline result |
|---|---|---|
| `OBS-001` | Final overhead, persisted/reset drain fraction, actual capped Prayer loss, passive-depletion bit, and Tz-Kih drain observation. | Passed. |
| `OBS-002` | Production Jad reveal path exposes the final valid Prayer decision with maximum player/per-NPC urgency. | Expected-red assertion failure. |
| `OBS-003` | A hit at its lock tick is non-actionable even if its snapshot is manually left unset. | Expected-red assertion failure. |
| `OBS-004` | Incoming-aggregate ablation zeros only aggregate timing counts while preserving per-NPC pending-hit and deadline fields. | Passed. |

The focused green cases passed 2/2 in 0.00 seconds. A fresh
`RelWithDebInfo` build completed normally, with only the previously recorded
viewer `snprintf` warnings. The preservation command was:

```bash
ctest --test-dir runescape-rl/build-parity-config-ws2 \
  -LE known_red --output-on-failure
```

All 115 non-red tests passed in 0.29 seconds. The two expected-red cases then
reached their intended assertions in 0.00 seconds:

```text
FAIL OBS-002: final valid decision urgency player/window/npc=0.750/1.000/0.750, expected 1/1/1
FAIL OBS-003: lock-tick unset snapshot reports player/window/npc/style/ticks=1.000/1.000/1.000/0.667/0.300
```

### Read-only failure diagnosis

Both failures originate in the shared observation helpers in
`fc-core/src/fc_state.c`.

For `OBS-002`, the current urgency formula computes
`(4 - ticks_until_lock) / 4`. The final valid decision occurs one tick before
the Prayer lock, so the helper reports `0.75` instead of the required maximum
urgency of `1.0`. The Prayer-window bit itself remains correctly nonzero.

For `OBS-003`, `pending_hit_prayer_actionable` checks that the hit is active,
the snapshot is unset, the lock tick exists, and the style is protectable,
but it does not compare the current tick with the lock tick. Consequently, a
manually unset snapshot at or after the boundary is still advertised as an
actionable window. Pending style and landing-time fields correctly remain
visible; only the actionability window and deadlines should be zero.

One older preservation fixture,
`phase2_guardrails_core.c::test_prayer_deadline_observation_fields`, sets an
unset snapshot with `prayer_lock_tick == state.tick` and expects the window
and deadlines to be `1.0`. That expectation encodes the superseded boundary
semantics. Once the production boundary is corrected, this fixture will need
an approved test-only adjustment to place its intentionally actionable hit at
`state.tick + 1`. The new `OBS-003` case independently protects the required
at-lock zero behavior, so this adjustment would not weaken coverage.

The smallest proposed correction is:

1. Pass `FcState` into the actionability helper and require
   `state->tick < prayer_lock_tick` in addition to the existing conditions.
2. Normalize deadline urgency so one remaining decision tick maps to exactly
   `1.0`, while earlier actionable ticks remain nonzero.
3. Apply the state-aware helper consistently to player aggregate deadlines
   and per-NPC window/deadline fields.
4. Move only the older guardrail's actionable fixture from
   `prayer_lock_tick == state.tick` to `state.tick + 1`.

No production or corrective test change has been made after these failures.
Implementation is paused pending approval. The three plan files remain
byte-identical at their previously recorded SHA-256 hashes.

### Approved correction and completed validation

The four proposed corrections were approved and applied without expanding
the workstream. `pending_hit_prayer_actionable` now receives the current
state and rejects a hit when `state->tick >= prayer_lock_tick`, even if a
malformed or manually constructed hit still has an unset snapshot. Both the
player-level maximum-by-style deadline and the per-NPC window/deadline use
this same state-aware predicate.

The urgency scale retains a nonzero floor for earlier actionable windows and
now maps the final valid decision exactly as follows: one tick until lock is
`1.0`, two is `0.75`, three is `0.5`, and four or more is `0.25`. At or after
the lock boundary it is `0.0` because the hit is no longer actionable.

The older `test_prayer_deadline_observation_fields` fixture was corrected
from `prayer_lock_tick == state.tick` to `state.tick + 1`, so its explicitly
actionable case exercises the final valid decision rather than a locked hit.
`OBS-003` remains the independent regression proving that the at-lock case
reports zero window and deadline. The temporary `known_red` labels were
removed from `OBS-002` and `OBS-003`.

Focused validation passed all five relevant cases in 0.01 seconds:

```text
parity_obs_001_final_overhead_and_loss        passed
parity_obs_002_final_decision_urgency         passed
parity_obs_003_lock_tick_nonactionable        passed
parity_obs_004_ablation_preserves_deadline    passed
step3_prayer_deadline_observation_fields      passed
```

`ctest -N -L known_red` reported zero tests. The complete normal suite passed
117/117 tests in 0.28 seconds. A fresh Debug build using
`-fsanitize=address,undefined -fno-omit-frame-pointer` passed the same 117/117
tests in 0.85 seconds with halt-on-error enabled and leak detection disabled
for the previously documented managed-workspace `ptrace` limitation. No
address- or undefined-behavior diagnostic occurred.

A separate `RelWithDebInfo` configuration with
`-Wall -Wextra -Werror` compiled `fc_core`, `fc_capi`, both Phase 2 guardrail
targets, and `parity_fix_prayer` without warnings. The direct-included Puffer
backend compiled successfully through both `fc-training/build.sh --local`
and `--fast`. Its optimized standalone diagnostic completed 100 random
episodes and 1,187 ticks without a runtime or buffer error. The reported
684,150 SPS is diagnostic only and is not a `PERF-002` or `PERF-003` result.
`git diff --check` passed.

Final workstream artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `fc-core/src/fc_state.c` | `1e53c456d7bbd163620480bde956e2135247b759778cc85d662a4af8625db6ba` |
| `fc-validation/tests/parity_fix_prayer.c` | `dcee69cc2f8ea16c92bd06b3afdad6630611276981d9de65afafefefbae015c7` |
| `fc-validation/tests/phase2_guardrails_core.c` | `ef95fff072f21d9912fac8fbd320f35412cf92a46432768f6b453dd5e49e165a` |
| `fc-validation/CMakeLists.txt` | `b10f18b4025ebe958809a56fc5e851e47833c8b0677a0c2a5a29046c3de6e080` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config workstream 2, observation and deadline semantics, is complete. No
reward/metric, evaluator/C API, version, preflight/manifest, or checkpoint
work was started.

## Config workstream 3 — reward-event and metric consumers

### Focused expected-red baseline

Status: paused for failure review on 2026-08-02. This tranche is limited to
`parity_fix_config.md` implementation-order item 4: Prayer-related reward
events, hit and transition analytics, tick-start Prayer uptime, Prayer-command
diagnostics, and their Puffer log publication. Evaluator/C API integration,
contract versions, hashing, preflight/manifests, and checkpoint isolation
remain assigned to later workstreams.

Five focused cases were added across the normally linked core test and the
direct-included Puffer adapter guardrail:

| Case | Scope | Baseline result |
|---|---|---|
| `RWD-001` | Ordinary and Jad correct/wrong/no-Prayer events, scalar correct-prayer reward, and hit metrics all follow the immutable hit snapshot after the live overhead changes. | Passed. |
| `RWD-002` | Drain-free flick, passive drain, Tz-Kih drain during a flick, and capped Tz-Kih loss feed the existing Prayer-loss feature and scalar penalty in actual tenths. | Passed. |
| `RWD-003` | The 20-feature reward layout and Prayer defaults are preserved; commands 5-7 are valid, out-of-range commands remain invalid, and unnecessary-Prayer shaping reads final overhead state. | Passed. |
| `METRIC-001` | Activation, deactivation, direct switch, cross-prayer flick, and depletion ticks count uptime from the effective tick-start Prayer. | Expected-red assertion failure. |
| `METRIC-002` | The Puffer terminal log publishes tick-start uptime plus flick-transition, Prayer-command, invalid-action, and reward-channel diagnostics. | Expected-red assertion failure. |

The existing live-config static guardrail was extended to include the planned
preserved `w_invalid_action = -0.1` value. Its existing checks already cover
`w_correct_jad_prayer = 0.0`, `w_correct_danger_prayer = 0.005`,
`w_prayer_lost = -0.02`, and
`shape_unnecessary_prayer_penalty = 0.0`.

The two affected C targets compiled normally. The three reward tests and the
live-config guardrail passed 4/4 in 0.02 seconds. The preservation command was:

```bash
ctest --test-dir runescape-rl/build-parity-config-ws2 \
  -LE known_red --output-on-failure
```

All 120 non-red tests passed in 0.27 seconds. The two expected-red tests then
reached their intended assertions in 0.00 seconds:

```text
FAIL METRIC-001: activation tick counted final prayer uptime=0/0/1
FAIL METRIC-002: Puffer prayer uptime/switch/cmd/invalid publication=0.333/0.333/0.333 3/3/2 0/0 0.000/0
```

### Read-only failure diagnosis

Both failures have one authoritative root cause. In the episode-analytics
phase of `fc-core/src/fc_tick.c`, all three `ep_ticks_pray_*` counters test
`state->player.prayer`, which is the final overhead after the current action,
drain, and any depletion. The plan defines uptime from
`state->player.prayer_at_tick_start`, the immutable overhead that actually
protected during that simulated tick.

As a result, an Off-to-Magic activation incorrectly counts Magic immediately,
a later deactivation omits the tick on which Magic still applied, and a
cross-prayer switch/flick attributes the tick to the new final style. The
three-command Puffer trace therefore publishes one third uptime for Magic,
Range, and Melee. Its actual effective trace is Off, Magic, Range, so the
correct publication is one third Magic, one third Range, and zero Melee.

The Puffer adapter is not independently recomputing uptime. It already divides
and publishes the authoritative core `ep_ticks_pray_*` counters. The same
trace correctly reports three explicit flick transitions, three non-noop
Prayer commands, two nonterminal no-progress Prayer-command ticks, zero
invalid Prayer actions, and zero invalid-action reward value/fires. No
training-adapter correction is indicated by the failure.

The reward-event tests confirm that the rest of this workstream's production
data flow is already correct: combat, reward flags, reward features, scalar
reward, and correct/wrong/no-Prayer hit analytics all use the immutable hit
snapshot; Prayer-loss reward uses actual capped passive plus Tz-Kih loss; a
drain-free flick has no Prayer-loss penalty; and commands 5-7 do not produce
invalid-action events.

The smallest proposed correction is to change only the three uptime
classifications in `fc_tick.c` from `state->player.prayer` to
`state->player.prayer_at_tick_start`, and clarify the nearby analytics comment
if useful. No reward formula, event producer, training publisher, state field,
or config value needs to change. After approval, the two temporary
`known_red` labels would be removed when these tests turn green.

No production correction was made after the failures. `git diff --check`
passes, and the three plan files remain byte-identical at their previously
recorded SHA-256 hashes.

### Approved correction and completed validation

The proposed correction was approved and applied without expanding the
workstream. The three existing `ep_ticks_pray_melee/range/magic`
classifications now read `player.prayer_at_tick_start` instead of the final
`player.prayer`. This reuses the same authoritative tick-start snapshot
already used by ordinary hit calculation and Prayer drain; no new state,
helper, abstraction, reward event, or adapter-side timing implementation was
added.

The Puffer publication path required no production change. It continues to
publish the core episode counters, which now have the corrected timing. The
temporary `known_red` labels were removed from `METRIC-001` and `METRIC-002`.

Focused validation passed all eight related cases in 0.01 seconds: the three
reward cases, the two uptime/publication cases, and the existing transition,
tick-start snapshot, and repeated-flick traces. `ctest -N -L known_red`
reported zero tests.

The complete normal suite passed 122/122 tests in 0.29 seconds. A freshened
Debug ASan/UBSan build passed the same 122/122 tests in 0.93 seconds with
halt-on-error enabled and leak detection disabled for the previously
documented managed-workspace `ptrace` limitation. No address- or
undefined-behavior diagnostic occurred.

A `RelWithDebInfo` configuration with `-Wall -Wextra -Werror` compiled
`fc_core`, `fc_capi`, both Phase 2 guardrail targets, and
`parity_fix_prayer` cleanly; all eight affected strict-build tests passed.
The direct-included Puffer backend compiled successfully through both
`fc-training/build.sh --local` and `--fast`. Its optimized standalone
diagnostic completed 100 random episodes and 652 ticks without a runtime or
buffer error. The reported 498,090 SPS is diagnostic only and is not a
`PERF-002` or `PERF-003` result.

The simplification audit confirmed there is one uptime classification in
core and one thin Puffer publication path; no parallel final-Prayer uptime
calculation remains. Reward formulas, the 20-feature layout, active reward
weights, hit-result event production, action validity, and Puffer log fields
remain unchanged. `git diff --check` passed.

Final workstream artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `fc-core/src/fc_tick.c` | `eec617a511ac99518ac3f1318ce590b7215d9ff463b2b5c9d945254065bd6bda` |
| `fc-validation/tests/parity_fix_prayer.c` | `f941a42f8bef8b8d92a092bb26ee78dc9812302fa9135f255e5c6fe45c5adb1e` |
| `fc-validation/tests/phase2_guardrails_training.c` | `43eb66821ebe5d82c64933c6b9aeff0c7d2345bff833a84093c0bf6722fec0d4` |
| `fc-validation/CMakeLists.txt` | `96aeceb2560c57618df43d712e00e7de5c9adc53f8ca192601d32d93639fe8d2` |
| `tools/validation/tests/phase2_static_guardrails.py` | `d4c6957dc8c444f3a6aa7a97d2a466f7b61a964e3a96d879b69ad6e87e4c1bd5` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config workstream 3, reward-event and metric consumers, is complete. No
evaluator/C API, version, hash, preflight/manifest, or checkpoint work was
started.

## Config workstream 4 — adapter, C API, evaluator, and buffers

### Focused expected-red baseline

Status: paused for failure review on 2026-08-02. This tranche is limited to
`parity_fix_config.md` implementation-order item 5: the project-owned Puffer
adapter and binding contract, plain C API dimensions and batch strides,
evaluator-derived dimensions and action buffers, standalone/random action
generation, and buffer canaries. Per user direction, `pufferlib_4/` is
strictly read-only and no file under it was modified. Contract versions,
canonical hashing, preflight/manifests, and checkpoint isolation remain later
workstreams.

Focused coverage was added in `fc-validation/tests/`:

| Case | Scope | Baseline result |
|---|---|---|
| `CONTRACT-002 C API dimensions` | Every existing C API getter agrees with the shared 474/285/20/169 sizes and `{17,9,8,3,2,65,65}` action dimensions; reset and step expose all eight Prayer mask values. | Passed. |
| `CONTRACT-003 C API batch canaries` | A normally linked four-environment batch uses seven-action and 474-float strides, executes distinct Prayer commands 0/5/6/7, preserves caller canaries, and matches four independently seeded single-env observations/rewards/terminals. | Passed. |
| `CONTRACT-002 evaluator dimensions` | The evaluator derives policy input 285, Puffer input 319, dimensions `{17,9,8}`, and mask 34 from `fc_contracts.h`. | Passed. |
| `CONTRACT-003 evaluator buffers` | The evaluator consumes exactly 319 floats, rejects a short line, samples the full action dimensions including Prayer action 7, and emits exactly three actions. | Passed. |
| `CONTRACT-002 standalone action range` | The standalone sampler uses `[0, FC_PRAYER_DIM)` rather than a stale five-action literal. | Passed. |
| `CONTRACT-002 public C API header` | The installed header declares the same contiguous batch API implemented and exported by `fc_capi.c`, and the implementation compiles against that header. | Expected-red assertion failure. |

The configured `RelWithDebInfo` build completed successfully, including
`fc_core`, the direct-included training guardrail, `fc_capi`, the new normally
linked C API contract executable, and the viewer. The viewer build emitted
existing format-truncation warnings in unchanged `fc-viewer/src/ui.c`; no
workstream-4 file produced a compiler diagnostic and no warning was treated
as an expected-red result.

The preservation command was:

```bash
ctest --test-dir runescape-rl/build-parity \
  -LE known_red --output-on-failure
```

All 126 non-red tests passed in 0.48 seconds. The expected-red command then
ran one test and reached its intended assertion:

```text
FAIL CONTRACT-002: public C API header does not match its batch implementation:
  fc_capi.h is missing 'typedef struct FcBatchCtx FcBatchCtx;'
  fc_capi.h is missing the five implemented batch entry points
  fc_capi.h declares obsolete, unimplemented fc_capi_batch_step
  fc_capi.c does not compile its definitions against fc_capi.h
```

### Read-only failure diagnosis

The runtime implementation in `fc-core/src/fc_capi.c` already exports the
contiguous batch interface used by the new test:
`fc_capi_batch_create/destroy/reset/step_flat/get_obs`. Its action stride is
derived from `FC_NUM_ACTION_HEADS`, its observation stride is derived from
`FC_OBS_SIZE`, and its four-env results exactly match the independently
stepped single-env oracles. The failure is therefore not a stale runtime
stride or buffer overwrite.

The root cause is that `fc-core/include/fc_capi.h` was never synchronized
with that implementation. It declares an older pointer-array function named
`fc_capi_batch_step` that has no definition, while omitting `FcBatchCtx` and
all five real batch functions. In addition, `fc_capi.c` does not include the
public header, so the compiler cannot detect this declaration/definition
drift; its opaque context types are currently declared independently in the
source.

The smallest proposed correction is project-local:

1. Make the implementation include `fc_capi.h` and define the two opaque
   context struct tags declared by that header.
2. Replace the obsolete, unimplemented batch declaration with the five
   existing contiguous-batch declarations and add the opaque `FcBatchCtx`
   declaration.
3. Remove the temporary `known_red` label only after the header test turns
   green, then run the focused, full, strict/direct-adapter, and sanitizer
   gates for this workstream.

No runtime algorithm, scalar reward recipe, Puffer source, action dimension,
observation layout, evaluator checkpoint policy, config version, preflight,
manifest, or checkpoint behavior needs to change for this failure. No
production correction was made after the assertion. `git diff --check`
passes, and the three plan documents remain byte-identical at their recorded
SHA-256 hashes.

### Approved correction and completed validation

The proposed correction was approved and applied without expanding the
workstream. `fc_capi.h` now declares the opaque `FcBatchCtx` plus the five
contiguous batch functions that `fc_capi.c` actually implements. The obsolete,
undefined pointer-array `fc_capi_batch_step` declaration was removed.
`fc_capi.c` now includes the public header and defines the named `FcEnvCtx` and
`FcBatchCtx` struct tags behind those opaque declarations, so future signature
drift is a compile-time error.

No C API runtime loop, scalar reward calculation, action or observation
dimension, Puffer adapter behavior, evaluator logic, version, preflight,
manifest, checkpoint behavior, or Puffer source was changed. The temporary
`known_red` label was removed after the public-header assertion turned green.

Focused validation passed all eight `CONTRACT-002/003` adapter, standalone,
evaluator, C API, and canary cases in 0.20 seconds. `ctest -N -L known_red`
reported zero tests. The complete normal suite passed 127/127 tests in 0.47
seconds.

A separate `RelWithDebInfo` build with `-Wall -Wextra -Werror` compiled
`fc_core`, `fc_capi`, the C API contract client, the direct-included training
guardrail, and the headless viewer test without warnings. Equivalent optimized
direct-included `binding.c` and standalone builds were compiled manually to
`/tmp` so validation did not write into `pufferlib_4/`. The standalone
diagnostic completed 100 episodes and 1,261 ticks without a runtime or buffer
error. Its reported 610,654 SPS is diagnostic only and is not `PERF-002` or
`PERF-003`. Both temporary build artifacts were deleted afterward.

A fresh Debug ASan/UBSan build passed the complete 127/127 suite in 1.05
seconds with halt-on-error enabled and leak detection disabled for the
previously documented managed-workspace `ptrace` limitation. No address- or
undefined-behavior diagnostic occurred, including in the four-environment C
API batch canary test. The full sanitizer build emitted existing
format-truncation warnings from unchanged viewer rendering code; none came
from a workstream-4 source or test.

The final audit found only the five implemented contiguous batch API names in
runnable project code; the obsolete batch name is absent. `git diff --check`
passed. `git diff -- pufferlib_4` is empty, and no build or test wrote into the
Puffer checkout.

Final workstream artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `fc-core/include/fc_capi.h` | `2a06cc844f8f4d969bb0a77c642c657fcc4fd9dfb6bd0764f32969eeeeed8a56` |
| `fc-core/src/fc_capi.c` | `6bc20135593c9b91e1e51f6db40465070033a64aca0310a7f1e263537b1a9f22` |
| `fc-training/CMakeLists.txt` | `8e3c9afe64feb332ec24c6f034f898b000b63a1e38e16753b6ee4146c3bf1a86` |
| `fc-validation/CMakeLists.txt` | `a46a0c6883354bf29cdef45808fc15657c2bcac2522e1397474f865d8dc443bf` |
| `fc-validation/tests/parity_fix_capi.c` | `8bff9836fd901a6b32be1f6855bb2e41a18f682010fbc94ce6d6b1f4f8d61a57` |
| `fc-validation/tests/parity_fix_evaluator.py` | `090bc1a2e7bfb8db855e46ffa32ffb2a386453b5b5a979c93633a93f49282642` |
| `tools/validation/tests/phase2_static_guardrails.py` | `cee7665de427bf09647e768638c45bceac00763e3f66e67efd21ee9d6f54d471` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config workstream 4, project-owned adapter/C API/evaluator/random-action and
buffer integration, is complete. Workstream 5 contract-version changes were
not started.

## Config workstream 5 — contract and configuration versions

### Focused expected-red baseline

Status: paused for failure and boundary review on 2026-08-05. This tranche is
limited to `parity_fix_config.md` implementation-order item 6: bumping the
active observation/action/reward contract identities, updating experiment
configs intended to run on the parity backend, and synchronizing the generated
runtime config. Canonical hashing, compiled preflight, manifest schema 2,
evaluator hard failures, stale-literal scanning, and checkpoint isolation
remain later workstreams.

Four focused cases were added in `fc-validation/tests/parity_fix_config.py`:

| Case | Scope | Baseline result |
|---|---|---|
| `CONFIG-001` | The canonical config uses the exact v8 observation, v2 eight-Prayer-action, and v4 prayer-event reward identities required by the plan. | Expected-red assertion failure. |
| `CONFIG-002` | All four top-level experiment configs use the parity observation/action identities and a new reward identity that preserves their reward-family provenance. | Expected-red assertion failure. |
| `CONFIG-003` | The generated Puffer runtime mirror is byte-identical to the canonical config and advertises the same exact parity identities. | Expected-red assertion failure. |
| `CONFIG-004` | After normalizing only the three version lines, canonical and runnable config hashes remain unchanged so reward weights, trainer values, comments, and experiment provenance cannot drift in this migration. | Passed. |

The runnable experiment inventory for this baseline is every top-level INI in
`config/experiments/`: the two promoted HP-regeneration recipes, the v1
mechanics sweep recipe, and the v3 simple-reward sweep recipe. Files under
`config/experiments/temporary/` remain treated as historical/temporary in this
tranche; later launch preflight must reject their old contracts if selected.

The build completed normally. The preservation command was:

```bash
ctest --test-dir runescape-rl/build-parity \
  -LE known_red --output-on-failure
```

All 128 non-red tests passed in 0.52 seconds. The three expected-red tests then
reached only their intended version assertions in 0.06 seconds:

```text
CONFIG-001: canonical config still reports observation v7, action v1, reward v3
CONFIG-002: four top-level experiments still report observation v6/v7,
            action v1, and their pre-parity reward identities
CONFIG-003: generated Puffer mirror still reports observation v7,
            action v1, and reward v3
```

### Read-only failure diagnosis and Puffer boundary

The failures have the expected root cause: none of the version strings has
been bumped yet. The mechanics and contracts have already changed, so the old
identities now falsely describe checkpoint compatibility. The canonical and
Puffer mirror files are currently byte-identical at SHA-256
`509af1168dca12f1c9f72ffc51d31bc2e7c7476536785915f20b034418127514`;
`CONFIG-003` fails because both files are stale, not because they differ.

`run_manifest.py` currently records whichever three strings it reads but does
not enforce them; that validation and schema-2 expansion remain workstream 7.
The existing `live_no_supplies_simplified_config` guardrail also intentionally
pins the old strings and will need its version-only expectations updated with
the approved migration. Its reward/trainer value assertions remain valid and
are independently protected by `CONFIG-004`.

No Puffer code change is needed. However, the exact plan and current config
module instructions define `pufferlib_4/config/fight_caves.ini` as a generated
runtime mirror, and `train.sh` unconditionally writes it with `cp` before
launch. Puffer's unmodified `load_config` searches only its own
`config/**/*.ini`; it exposes no external config-path option. The current
launcher also writes build products, checkpoints, logs, and W&B state under
the Puffer checkout.

Therefore the phrase “never modify Puffer” has two materially different
boundaries:

1. If it means no changes to vendored Puffer source, the smallest plan-aligned
   correction is to update the five project-owned configs and regenerate only
   the already-defined runtime mirror. No Puffer source or library behavior
   changes.
2. If it means no writes anywhere under `pufferlib_4/`, the current launcher
   architecture already violates that rule. Completing this item would require
   an approved plan pivot to stage or invoke Puffer from a separate writable
   runtime tree, then redirect configs, builds, logs, and checkpoints there.

No production config, experiment config, launcher, manifest, test expectation,
or Puffer file was changed after the failures. `git diff -- pufferlib_4` is
empty, `git diff --check` passes, and the three plan documents remain
byte-identical at their recorded SHA-256 hashes. Implementation is paused for
approval of the intended Puffer boundary and the five project-owned version
updates.

### Approved correction and completed validation

The first boundary option was approved: vendored Puffer source and behavior
remain untouched, while the existing generated runtime mirror may be
synchronized as required by the current launcher. The correction changed only
the three contract-version assignments in the canonical config, all four
top-level runnable experiment configs, and
`pufferlib_4/config/fight_caves.ini`. Temporary/historical experiment configs
were not migrated; their rejection remains part of the later preflight
workstream.

The canonical config and the three experiments using its reward family now
advertise these exact identities:

```text
observation_version = fight_caves_puffer_policy_obs_v8_prayer_timing_mask8_no_supplies
action_version      = fight_caves_multidiscrete_3_head_no_supplies_v2_prayer8
reward_version      = fight_caves_v4_progress_npc_heal_penalty_m0005_prayer_snapshot_flick_drain
```

The v1 mechanics sweep keeps its distinct reward-family provenance and adds
the new prayer semantics:

```text
fight_caves_v38_fc_revamp_step2_raw_work_progress_prayer_conserve_no_attack_prayer_snapshot_flick_drain
```

The existing static guardrail's version-only expectations were synchronized,
and the three temporary `known_red` labels were removed only after the focused
assertions passed. `CONFIG-004` continued to pass, confirming that no
non-version config value, comment, trainer setting, or reward weight changed.

Focused validation passed all five relevant config and static-guardrail cases
in 0.10 seconds, with zero tests left under `known_red`. The complete normal
suite then passed 131/131 tests in 0.52 seconds. A fresh Debug ASan/UBSan build
with halt-on-error enabled passed the same 131/131 tests in 1.19 seconds; no
address- or undefined-behavior diagnostic occurred.

A temporary manifest was generated through the normal project tool. It
contained all three new versions, the active `FC_LOADOUT_SOTA_TBOW` loadout,
and identical canonical/source and synchronized-runtime config hashes. Its
schema remains version 1 intentionally because manifest schema 2 belongs to a
later workstream. The temporary manifest directory was deleted after the
check.

The final audit passed `git diff --check`. The canonical config and generated
Puffer mirror are byte-identical at SHA-256
`998ece054fa027b116f6f68081566f98d177e214a901126833188374ca9d6655`.
The only tracked Puffer diff is the three version lines in that generated
mirror; no Puffer source was changed.

Final workstream artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `config/fight_caves.ini` | `998ece054fa027b116f6f68081566f98d177e214a901126833188374ca9d6655` |
| `config/experiments/fight_caves_il0xq0uf_hp_regen_60s_1p5b.ini` | `dedb9d1237bb76ae311cf88d1348c010d366ec06f566b0d44f4c098dc87b19b0` |
| `config/experiments/fight_caves_mmyxbyn4_hp_regen_60s_1p5b.ini` | `1f84abdf77869d6b5785367aef8d581672644fd4c4eb2f4d91bcd5a194c8e5de` |
| `config/experiments/fight_caves_v1_mechanics_hparam_sweep_750m.ini` | `61de6e70dd7223c5af4f3c9b3dc59b8962cae2984238030d89ee948bc1d88a6f` |
| `config/experiments/v3_simple_reward_sweep1.ini` | `7f3c5e3a2e1a1a43d568e6011fc98c4f44ee4e7fc246dfcf6da0331c6c0567aa` |
| `pufferlib_4/config/fight_caves.ini` | `998ece054fa027b116f6f68081566f98d177e214a901126833188374ca9d6655` |
| `fc-validation/tests/parity_fix_config.py` | `94848fa6b7d14f4fd639e8cf18f3da2ce74f552647026f8496ef269f74416f04` |
| `fc-validation/CMakeLists.txt` | `46b66d0f58981b13a6cf30f45e660271b0890fa05dcdcab1958e2525d04515b2` |
| `tools/validation/tests/phase2_static_guardrails.py` | `1c36402d0878bf1d5d7591b2f00e89876eb551f90ff1c9725d01c5efdc921513` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config workstream 5, contract and configuration versions, is complete.
Workstream 6 canonical hashing was not started.

## Config workstream 6 — canonical state hash

### Focused expected-red baseline

Status: paused for failure review on 2026-08-05. This tranche is limited to
`parity_fix_config.md` implementation-order item 7: move and version the
canonical hash in core, make normal-core, direct-included training,
validation, and viewer compilation paths consume that one implementation, and
record its version in action-trace metadata. Full wrapper replay comparison,
core-versus-Puffer trace comparison, manifest schema 2, launch preflight,
checkpoint filtering, and stale-literal scanning remain later workstreams.

Permanent DET-001 scaffolding was added in
`fc-validation/tests/parity_fix_hash.c` and
`fc-validation/tests/parity_fix_hash_contract.py`. The C test contains an
independent version-1 oracle that serializes each explicit integer as four
little-endian bytes, each arena byte directly, and each float by its 32-bit
representation. It does not inspect raw `FcState` memory. The synthetic-state
version-1 golden is pinned to `0xda423548`. The mutation case changes each
future-relevant state-field class independently and reports every class whose
mutation the production hash misses rather than stopping at the first one.
`FcRewardRuntime` is deliberately outside this test because it is not owned by
`FcState`; the plan requires it to be compared separately in the later wrapper
replay.

The existing version-0 viewer hash is compiled into the test executable only
for this red baseline. This permits runtime assertions without making the
preservation build fail to link before core owns the symbol. The permanent
test executable will link only the core implementation when the approved
correction removes that temporary source entry. No production hash, core,
training, viewer, config, or Puffer file was changed in establishing this
baseline.

The preservation command was:

```bash
ctest --test-dir runescape-rl/build-parity \
  -LE known_red --output-on-failure
```

The complete build succeeded and all 132 non-red tests passed in 0.54 seconds.
The build emitted only the previously documented viewer `snprintf`
format-truncation warnings. The new presentation-exclusion preservation case
also passed, confirming that mutating a separate `FcRenderEntity` does not
alter an unchanged `FcState` hash.

The isolated expected-red command was:

```bash
ctest --test-dir runescape-rl/build-parity \
  -L known_red --output-on-failure
```

All four focused cases failed for their intended pre-implementation reasons
in 0.02 seconds:

| Case | Expected-red result |
|---|---|
| `parity_det_001_hash_version` | `FC_STATE_HASH_VERSION` is still the A1 placeholder value 0 rather than canonical version 1. |
| `parity_det_001_field_coverage` | The viewer hash omitted 45 future-relevant field classes. |
| `parity_det_001_versioned_golden` | The viewer hash produced `0x461d4a08`, not the version-1 canonical golden `0xda423548`. |
| `parity_det_001_core_hash_ownership` | Core, direct-included training, viewer ownership, `FC_NO_HASH`, and action-trace version-metadata assertions all reported the expected old architecture. |

The 45 omitted mutation classes cover the exact material gaps expected from
the old viewer-only implementation: tick-start Prayer and its drain counter;
player combat/equipment/weapon/crystal, route, targeting, pending lock, event,
and cumulative state; NPC death/healing/pending-lock state; active loadout;
saved RNG seed; movement reservations; Jad healer lifecycle; prayer, invalid
action, action, consumable, healing, and progress events; and every tested
episode analytics family.

### Read-only root-cause diagnosis and proposed correction

The root cause is one stale, partial implementation at
`fc-viewer/src/fc_hash.c`. It predates much of the current `FcState` schema,
defines the symbol outside core, hashes only a subset of fields, and feeds
native `int` object bytes rather than a fixed-width byte order. Consequently:

- `fc_core` declares `fc_state_hash` but does not define it;
- the viewer and its headless test compile their own algorithm;
- the direct-included training backend does not include any hash source;
- both training build paths explicitly define `FC_NO_HASH`;
- `FC_STATE_HASH_VERSION` remains the A1 placeholder 0; and
- the viewer action trace stores per-tick hash values without recording which
  hash version produced them.

The smallest plan-aligned correction is:

1. Move the implementation to `fc-core/src/fc_hash.c`, replace the partial
   native-width feed with the tested explicit version-1 serialization, and add
   it to `fc_core`.
2. Change `FC_STATE_HASH_VERSION` from 0 to 1.
3. Include that same core source in the project-owned direct-included training
   backend and remove `FC_NO_HASH` from the project training builds. This makes
   the function available but does not call it in the training hot loop.
4. Stop compiling the viewer-local implementation, delete the superseded
   algorithm, and let the viewer/headless paths consume the linked core symbol.
5. Record `FC_STATE_HASH_VERSION` in `FcActionTrace` initialization/reset
   metadata, switch the validation target from its red-baseline viewer source
   to core only, and remove `known_red` labels only after all assertions pass.

This correction does not change gameplay, observations, masks, rewards,
configuration values, Puffer source, manifest schema, checkpoint handling, or
per-tick production behavior. It only centralizes an on-demand diagnostic
function and completes its serialized state coverage. Core-versus-Puffer
per-tick replay equivalence and `FcRewardRuntime` comparisons remain required
later and are not claimed by DET-001.

Test-scaffolding hashes at this pause are:

| Artifact | SHA-256 |
|---|---|
| `fc-validation/CMakeLists.txt` | `4aa3cd44ab2cad80d9b1660d2eb5ed14234ca32174d7693daafe2cf4966f48c2` |
| `fc-validation/tests/parity_fix_hash.c` | `4d8362823fae56da37f375e41da5914d6a512009f3d6b64bfbc2d9e6a495bb11` |
| `fc-validation/tests/parity_fix_hash_contract.py` | `8d43a93e8aaef1d2c723ad7dd0fc40e437515c10b12ec46c643984ae9b6b501a` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Implementation is paused for approval of the correction above. Workstream 7
preflight and manifest schema 2 were not started.

### Approved implementation and strict-build validation pause

The proposed Workstream 6 correction was approved and implemented. Core now
owns the only `fc_state_hash` definition in `fc-core/src/fc_hash.c`, and
`FC_STATE_HASH_VERSION` is 1. The implementation follows the independent test
oracle's explicit fixed-width, little-endian serialization and covers the full
`FcState` field set without reading raw struct storage or padding.

The normal `fc_core` target compiles the hash. The project-owned Puffer adapter
directly includes that same source, and the obsolete `FC_NO_HASH` definitions
were removed from both project training build paths. This only makes the
on-demand function available; no training reset, step, observation, mask, or
reward hot path calls it. The viewer-local hash implementation was deleted,
and the viewer and headless test now link the core symbol. `FcActionTrace`
records `FC_STATE_HASH_VERSION` during both initialization and reset. No
Puffer source was changed.

Focused validation passed all eight hash, headless, direct-included training,
and contract cases in 0.04 seconds. The complete normal suite passed 136/136
tests in 0.60 seconds, and `ctest -N -L known_red` reported zero tests. A fresh
Debug ASan/UBSan build with halt-on-error enabled and leak detection disabled
passed the complete 136/136 suite in 1.15 seconds with no address- or
undefined-behavior diagnostic. The sanitizer build emitted one existing
viewer `snprintf` truncation warning unrelated to this workstream.

An additional `RelWithDebInfo` build was configured with
`-Wall -Wextra -Werror`. `fc_core`, including the new production hash, compiled
cleanly. The build then stopped while compiling the new validation test:

```text
fc-validation/tests/parity_fix_hash.c:483:20:
error: variable 'presentation' set but not used [-Werror=unused-but-set-variable]
```

The root cause is limited to the DET-001 presentation-exclusion fixture. It
mutates a local `FcRenderEntity` to represent viewer-only presentation state,
then verifies that the separate `FcState` hash is unchanged. The semantic
assertion passes in normal and sanitizer builds, but the test never reads the
local presentation object after assigning its fields, so GCC correctly treats
those assignments as unused under the strict warning gate. No production
source, hash result, state coverage, or runtime behavior is implicated.

The smallest proposed correction is test-only: assert that the two
presentation fields contain the assigned fixture values before checking the
unchanged state hash. That makes the intended mutation observable to the
compiler and strengthens the test instead of merely suppressing the warning.
No warning flag, production source, threshold, expected hash, or plan needs to
change.

No edit was made after this strict-build failure. Workstream 6 remains paused
for approval of that test-only correction; Workstream 7 was not started.

### Approved test correction and completed validation

The test-only correction was approved and applied. The presentation-exclusion
case now first asserts that its separate `FcRenderEntity` fixture received the
intended mutations, then proves that the unchanged `FcState` retains the same
canonical hash. No production source or expected hash changed as part of this
correction.

The `RelWithDebInfo` `-Wall -Wextra -Werror` build then compiled `fc_core`, the
hash test, the direct-included training guardrail, headless viewer test, C API,
and C API contract test cleanly. All nine focused strict-build tests passed in
0.04 seconds.

After the correction, the complete normal suite again passed 136/136 tests.
The complete ASan/UBSan suite also passed 136/136 tests in 1.18 seconds with
halt-on-error enabled and no address- or undefined-behavior diagnostic. Zero
tests remain under `known_red`.

The final scope audit established all of the following:

- `fc-core/src/fc_hash.c` is the only production definition of
  `fc_state_hash`.
- The linked `fc_core` archive and direct-included training guardrail both
  export that same symbol.
- No core or training step/reset/observation/mask/reward function calls the
  hash, so it remains outside the production hot loop.
- `FC_NO_HASH` is absent from project build paths.
- The superseded `fc-viewer/src/fc_hash.c` file and both viewer-local CMake
  source entries are deleted.
- `git diff --check` passes.
- The only tracked Puffer diff remains the previously approved Workstream 5
  generated config mirror; no Puffer source changed in Workstream 6.

Final Workstream 6 artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `fc-core/CMakeLists.txt` | `01c3f448be1fc180dd7c315af503c70a95c35c155910eb5bf3839560af767d8d` |
| `fc-core/include/fc_api.h` | `fd655a6eecb7824fc8aeaad34a50dd7a81695bca3abf58fe25855ffc0f53639f` |
| `fc-core/src/fc_hash.c` | `f09bd0365cacba300d2f043d163d6156ac571563678c7ca99a9c8043ab5fb80d` |
| `fc-training/CMakeLists.txt` | `52004ba9478a0c6cfc36feb235d048565f6ca02942f47820c34dac71b666eab8` |
| `fc-training/build.sh` | `3bbce849030fb6a9185960ca1a31e3a668c465b8c5b1d074dfbaf4a21ae92673` |
| `fc-training/fight_caves.h` | `152cecf448596cc57efda4dd5bd11f90aca4f86e4657bfdaf27b9d236cc224fe` |
| `fc-viewer/CMakeLists.txt` | `3a37c5c1f5efdbf4f67e845867c4618347176af6081e8bc84c60e3a0eb5febad` |
| `fc-viewer/src/fc_debug.c` | `949339759931da6373b6dd78b4003a75f1c55a9edef71e753fa27239c821bbc8` |
| `fc-viewer/src/fc_debug.h` | `1bc50d1f50b7d18d68e1f1ed9898a8f17064b2406f57c8e7dd98c9fe33a42539` |
| `fc-viewer/tests/test_headless.c` | `d14f0920cb54c20fef3efd6be44478bd476c153a4851e82db77cbec2e608e860` |
| `fc-validation/CMakeLists.txt` | `445a1f5a6b18c631d266f63f88342e437db498b22667e5c74341333259b7bc42` |
| `fc-validation/tests/parity_fix_hash.c` | `ac887f5b15472616e668d569507e55dd5263564c1b1f3d48605dc240871e4a89` |
| `fc-validation/tests/parity_fix_hash_contract.py` | `8d43a93e8aaef1d2c723ad7dd0fc40e437515c10b12ec46c643984ae9b6b501a` |

The deleted viewer hash has no final file hash because its independent
algorithm was intentionally removed rather than retained as a wrapper.

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config Workstream 6, the shared versioned canonical state hash, is complete.
Workstream 7 compiled preflight and manifest schema 2 were not started.

## Config Workstream 7: compiled preflight and manifest schema 2

### Test-first scope and preservation result

Workstream 7 is limited to the shared compiled-contract dump/preflight,
source/copied-config equality, launch/evaluator consumption of the dump, and
manifest schema 2. Contract-specific `latest` checkpoint discovery, sidecar
matching, supplied-checkpoint rejection, and removal of evaluator random
fallback remain assigned to the next workstream. No production implementation
was changed in this test-first phase, and no Puffer source was changed.

Five permanent `CONTRACT-004` cases were added with `known_red` labels:

| Test | Contract |
|---|---|
| `parity_contract_004_compiled_preflight` | A compiled backend dump must report the exact dimensions, version identifiers, hash version, and active loadout, and exact source/copied configs must produce a verified machine-readable preflight artifact. |
| `parity_contract_004_preflight_rejections` | Every compiled dimension/version/loadout field, every selected config version, and a one-byte source/copy mismatch are rejected independently with expected/actual diagnostics. |
| `parity_contract_004_manifest_schema2` | A cold-start schema-2 manifest records the verified compiled contract, config hashes/equality, source/build/compiler identity, and explicit checkpoint state; schema 1 and omitted required fields fail closed. |
| `parity_contract_004_consumer_order` | Training syncs and builds before preflight, then preflights before command/checkpoint/manifest use; the manifest receives the verified artifact, and evaluation consumes the same compiled helper before model construction rather than parsing header arithmetic. |
| `parity_contract_004_active_configs` | The canonical config, every runnable experiment, and the generated Puffer mirror use manifest schema 2; each runnable reward-family version passes against a correspondingly compiled backend contract. |

`fc-validation/tests/parity_contract_fixture.c` is a test-only shared-library
fixture exporting the planned compiled JSON symbol. Its environment override
allows each field to be corrupted independently without repeatedly compiling
the real CUDA backend. It does not add test behavior to production code.

The Release build completed. The preservation command was:

```text
ctest --test-dir runescape-rl/build -LE known_red --output-on-failure
```

All 136 preservation tests passed in 0.59 seconds. This includes all completed
gameplay, Prayer, observation, reward, metric, C API, evaluator-buffer,
configuration-version, deterministic-hash, viewer, and training-adapter tests.

### Expected-red result and root-cause diagnosis

The focused command was:

```text
ctest --test-dir runescape-rl/build -L known_red --output-on-failure
```

All five new tests failed as expected in 0.17 seconds:

- The shared `tools/validation/contract_preflight.py` helper does not exist,
  so neither exact dump validation nor independent mismatch rejection can run.
- `run_manifest.py` only accepts and emits schema 1. It has no verified
  compiled-contract, backend-binary, or checkpoint-request inputs and cannot
  emit the required numerical contract or fail-closed provenance.
- `train.sh` does not invoke a compiled preflight or pass a verified contract
  to the manifest writer.
- `eval_viewer.py` still parses dimension arithmetic from C header text rather
  than consuming the selected compiled backend contract.
- The canonical config, four runnable experiment configs, and generated Puffer
  mirror still declare `manifest_schema_version = 1`.
- The current backend stamp records loadout, Python, CUDA architecture/version,
  and backend hash, but not the complete source/build/compiler identity needed
  for a schema-2 manifest.

The source config and generated Puffer mirror are currently byte-identical;
that invariant did not fail. The failures therefore describe missing planned
behavior, not a regression or an unexpected implementation defect.

### Proposed plan-aligned correction

The smallest correction is:

1. Export one default-visible `fc_training_contract_json` symbol from the
   project-owned training binding. It will serialize constants compiled from
   `fc_contracts.h`, `FC_STATE_HASH_VERSION`, the selected loadout, and compiled
   observation/action/reward/Prayer-timing identifiers. The actual Puffer
   extension links this project-owned object, so no Puffer source edit is
   needed.
2. Add one shared `contract_preflight.py` module/CLI. It will load that symbol
   from the selected backend, validate the exact parity contract and supported
   runnable version identifiers, compare the selected config to the compiled
   identifiers, require schema 2, verify byte identity, print expected/actual
   diagnostics on failure, and write one verified JSON artifact on success.
   The runnable experiment with its distinct reward-family name remains
   supported by compiling its selected reward identifier and validating it
   against the single shared supported-version mapping.
3. Make backend freshness include the selected contract version identifiers,
   record complete source/build/compiler identity in the stamp, and invoke the
   preflight in `train.sh` after config sync/backend build but before command
   construction, checkpoint use, manifest generation, or training execution.
4. Replace evaluator header parsing with the shared compiled-contract loader
   before policy-model construction. Do not change checkpoint fallback/error
   behavior in this workstream.
5. Upgrade `run_manifest.py` to fail-closed schema 2 and populate its numerical
   contract only from the verified preflight artifact. Implement the explicit
   cold-start record now; contract-specific warm/latest resolution and sidecar
   validation remain for the next workstream.
6. Set manifest schema 2 in the canonical and runnable configs, resync the
   generated Puffer config, and extend `CONFIG-004` normalization to the
   manifest-schema line so that this planned identifier-only migration does
   not falsely appear to change training or reward values.
7. Remove `known_red` only after the five focused cases, the complete normal
   suite, strict warning builds, and ASan/UBSan validation pass.

Test-scaffolding hashes at this pause are:

| Artifact | SHA-256 |
|---|---|
| `fc-validation/CMakeLists.txt` | `de130fb08cdf736d1b0fb3d745b80d5b010e41259320a468b9df04fe58fb27f6` |
| `fc-validation/tests/parity_contract_fixture.c` | `5e507b5d5689e75771adfc183ca5084ac1496f031757f4c7c279d73380c27799` |
| `fc-validation/tests/parity_fix_preflight.py` | `33b104dd1805b1787c79c413b55a840e8f9825a3f527eab68a7704ec9dedea06` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Implementation is paused for approval of the correction above. No Workstream
7 production behavior has been changed.

### Approved implementation and sanitizer-invocation pause

The approved Workstream 7 correction was implemented. The project-owned
training binding now exports its machine-readable contract through
`fc-training/contract_dump.c`; the real Puffer extension consumes that source
without a Puffer source edit. `contract_preflight.py` loads the selected
compiled binary, validates the exact numerical and versioned contract, checks
the selected loadout and byte-identical config mirror, and writes a verified
artifact. The exact-success test uses the production export implementation;
the test-only fixture is retained only for independent corruption cases.

`train.sh` now incorporates selected contract identifiers into backend
freshness/build identity, runs compiled preflight after backend build and
before command/checkpoint/manifest use, and passes the verified artifact to
the manifest writer. The evaluator consumes the same compiled loader before
model construction instead of parsing header arithmetic. `run_manifest.py`
emits fail-closed schema 2 cold-start manifests with numerical contract,
version/hash/loadout, config, backend source/binary/build/compiler, and
explicit checkpoint-mode identity. Warm/latest checkpoint resolution remains
assigned to the next workstream and currently fails closed rather than
claiming a warm start.

The canonical config, four runnable experiment configs, and generated Puffer
mirror now declare manifest schema 2. Their observation/action/reward values
and all non-contract trainer/reward content remain preserved; `CONFIG-004`
maps the planned schema bump back to 1 for its preservation hash comparison.
No Puffer source was changed.

Validation completed before the pause:

- Both focused runs passed 5/5 Workstream 7 cases. The second run exercised
  the production compiled-contract export for the exact-success path.
- The complete Release suite passed 141/141 tests in 2.09 seconds.
- `ctest -N -L known_red` reported zero tests.
- A `RelWithDebInfo` `-Wall -Wextra -Werror` build compiled all affected core,
  contract-export, training-adapter, C API, viewer-headless, and parity targets
  cleanly. Its 12 affected contract/config/evaluator tests passed.

The complete ASan/UBSan gate then passed 135/141 tests but failed six
Python/ctypes tests before their assertions ran:

```text
ASan runtime does not come first in initial library list; you should either
link runtime to your application or manually preload it with LD_PRELOAD.
```

The affected tests were both evaluator ctypes tests and four compiled-contract
tests. Read-only linkage inspection confirmed that both sanitizer-build shared
libraries link `libasan.so.8` and `libubsan.so.1`, while the system Python
interpreter does not link either runtime. Therefore an unsanitized Python
process aborts when ctypes loads the instrumented library. C executables and
all Python tests that do not load an instrumented shared library passed. This
is a sanitizer invocation-order issue, not a simulator, preflight, evaluator,
manifest, or memory-safety diagnostic.

The smallest proposed correction is invocation-only: rerun the unchanged
ASan/UBSan suite with the compiler-selected `libasan.so` preloaded into the
Python/CTest process, while retaining `detect_leaks=0`,
`ASAN_OPTIONS=halt_on_error=1`, and `UBSAN_OPTIONS=halt_on_error=1`. The local
compiler reports `/usr/lib/gcc/x86_64-linux-gnu/13/libasan.so`. No production,
test, plan, threshold, or sanitizer flag needs to change.

No correction or rerun was performed after this failure. Workstream 7 is
paused for approval of the sanitizer invocation correction.

### Approved sanitizer rerun and CUDA adapter-build pause

The invocation-only sanitizer correction was approved. The unchanged complete
suite was rerun with the compiler-selected ASan runtime preloaded:

```text
LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/13/libasan.so
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1
ctest --test-dir runescape-rl/build-parity-asan --output-on-failure
```

All 141/141 tests passed in 3.74 seconds with no ASan or UBSan diagnostic. This
confirms the prior six failures were solely runtime load order.

The final planned adapter audit then attempted to build the actual CUDA Puffer
extension without starting training. Compilation did not begin because the
command explicitly selected `runescape-rl/.venv/bin/python3`, and the build
script correctly rejected that interpreter for missing all three required
packages: NumPy, pybind11, and PyTorch. Read-only diagnosis found:

```text
runescape-rl/.venv/bin/python3: numpy=False, pybind11=False, torch=False
/usr/bin/python3:               numpy=True,  pybind11=True,  torch=True
```

This is an interpreter-selection error in the manual validation command, not a
source, contract, CUDA, or test failure. The normal `train.sh` selection logic
would skip that incomplete virtualenv and select `/usr/bin/python3` because it
passes the same dependency probe.

The smallest proposed correction is invocation-only: rerun the same
non-training CUDA adapter build with `PYTHON_BIN=/usr/bin/python3`, then use
`contract_preflight.py dump` on the resulting extension to confirm that the
real linked Puffer backend exports the exact compiled contract. No source,
test, plan, dependency installation, or Puffer source change is required.

No retry or alternate interpreter was used after this failure. Workstream 7
is paused for approval of the CUDA build-command correction.

### Corrected interpreter and CUDA-architecture pause

The corrected interpreter command was approved and rerun with
`PYTHON_BIN=/usr/bin/python3`. The dependency probe passed and the build began
by compiling the project-owned static Fight Caves binding. It then stopped
before CUDA compilation because automatic architecture detection could not
query the NVIDIA driver:

```text
Error: nvidia-smi could not query GPU compute capability.
Set NVCC_ARCH explicitly (for example, NVCC_ARCH=sm_120).
```

Read-only diagnosis confirmed that both `nvidia-smi -L` and its compute
capability query currently fail to communicate with the NVIDIA driver. The
existing Fight Caves backend build stamp records `NVCC_ARCH=sm_120` and CUDA
13.3, while the installed compiler is CUDA 13.3 (`nvcc` 13.3.73). Therefore
the repository already has an exact prior architecture identity; no
architecture needs to be guessed.

The smallest proposed correction is invocation-only: rerun the same
non-training build with `PYTHON_BIN=/usr/bin/python3` and
`NVCC_ARCH=sm_120`. The build script will compile for the stamped target and
verify the resulting binary's embedded device code with `cuobjdump`; it does
not need to execute the binary on the currently unavailable driver. Then run
the contract dump against the linked extension. No source, test, plan,
dependency, GPU-driver, or Puffer source change is proposed.

No architecture override or retry was performed after this failure.
Workstream 7 remains paused for approval of the stamped-architecture command.

### Approved stamped architecture and completed validation

The stamped `sm_120` command was approved. The actual project-owned Puffer
CUDA extension built successfully with `/usr/bin/python3` and CUDA 13.3. The
build's `cuobjdump` guard verified embedded `sm_120` device code. No training
job was started.

Dynamic-symbol inspection found the default-visible
`fc_training_contract_json` export in the resulting
`_C.cpython-312-x86_64-linux-gnu.so`. The shared helper loaded that real
extension and reported the exact contract:

- policy/Puffer/core observation sizes `285`, `319`, and `474`;
- Puffer action dimensions `{17,9,8}` and mask size `34`;
- core dimensions `{17,9,8,3,2,65,65}` and mask size `169`;
- reward feature count `20`;
- exact observation, action, reward, and Prayer-timing identifiers;
- state-hash version `1`; and
- active loadout `FC_LOADOUT_SOTA_TBOW`.

The real-backend `check` command then passed against the canonical source
config and generated Puffer mirror. Both files were byte-identical with SHA-256
`71afdc57de97ec432752130bae9de4165affa564661116f5d79bda728f87faa9`.
The resolved contract identity was
`29e4e754afdb7378a06ea38428c9cc50b258888b711835cd6fe6033ee813d71a`.

Final Workstream 7 validation is:

- focused Release behavior: 5/5 passed, including the production contract
  export for the exact-success path;
- complete Release suite: 141/141 passed in 2.09 seconds;
- affected strict `-Wall -Wextra -Werror` build and tests: clean, 12/12 passed;
- complete ASan/UBSan suite with the approved runtime preload: 141/141 passed
  in 3.74 seconds with no sanitizer diagnostic;
- actual CUDA adapter build: passed with verified `sm_120` device code;
- actual linked-extension contract dump and source/mirror preflight: passed;
- `ctest -N -L known_red`: zero tests;
- `git diff --check`: passed; and
- only the generated config mirror changed under `pufferlib_4`; no Puffer
  source changed.

Final Workstream 7 artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `fc-core/include/fc_contracts.h` | `87a1a88c38e29ba3eb568e5a9d38b1dfb7b9985c399fcf41e557d9f0e8214fc1` |
| `fc-training/binding.c` | `6781cd0a4514737082e3ab0fbe7a8cdd4d15433f22122547c9036c1127a19590` |
| `fc-training/contract_dump.c` | `3638a1cd4edb81901a5e16c0cc8fbd45e7ad52e42c1fd9c5014b203a00569f8b` |
| `fc-training/build.sh` | `d278f02c3d0fc8a88f29678db41caa94b1fe3a8a7faaaf60e1412c8931bbcbf8` |
| `train.sh` | `254d941c1c0e2b1be15a953fb1502e099cab1e5994785598442b82eadc8b887c` |
| `tools/validation/contract_preflight.py` | `2515a7cee87fd5cac6dfb8f30b92a0299b8510be417d532b154403472eeec607` |
| `tools/validation/run_manifest.py` | `71095e6cbaee508ea27baed1f34fc45c829978ae37bf20bd37a052f1c29a4f07` |
| `fc-viewer/eval_viewer.py` | `2a6c6d49f721e9ff03b91002565152c0ba27d5b941611a6423f4d19cc3d47a7f` |
| `fc-validation/CMakeLists.txt` | `c43982b431dad2e27d2519239ff751a712f164e49c5e6ddf83c2327a3b745ee4` |
| `fc-validation/tests/parity_contract_fixture.c` | `5e507b5d5689e75771adfc183ca5084ac1496f031757f4c7c279d73380c27799` |
| `fc-validation/tests/parity_fix_preflight.py` | `adf36cbd8cbb4f8d56a4b5036e117c4e03c4d2a9833b3beee232472a62deecbf` |
| `fc-validation/tests/parity_fix_evaluator.py` | `ea6b844a754254df5b5f0b878a6ff69b91361d02c421c839383c3408938ac5c5` |
| `fc-validation/tests/parity_fix_config.py` | `c9aec088ae32caa425996a5d610646fb1f4ff635a8cadb09c72673612500abc7` |
| `tools/validation/tests/phase2_static_guardrails.py` | `62485cc23b06d4fee5b9781f74695ce5f6fc1bffde8dc72a69a0180027ed862d` |
| `config/fight_caves.ini` | `71afdc57de97ec432752130bae9de4165affa564661116f5d79bda728f87faa9` |
| `config/experiments/fight_caves_il0xq0uf_hp_regen_60s_1p5b.ini` | `dc527d5be574a010701374b000a3ab6eec9dd7c554d7d3a023da0e2f49a1c0e4` |
| `config/experiments/fight_caves_mmyxbyn4_hp_regen_60s_1p5b.ini` | `2144a48f064a4c637401a155f8c33729517ef99f7775fc2b412a738de5dfcf56` |
| `config/experiments/fight_caves_v1_mechanics_hparam_sweep_750m.ini` | `8c095cc2e30635f0b4cb50f0442943b70674f44fb79f8b3b4cb77446ace65406` |
| `config/experiments/v3_simple_reward_sweep1.ini` | `46e0df4b76e24d9372793bb36315292c052f4ab0fc7d34ab47a82c8d5b38c3de` |
| `pufferlib_4/config/fight_caves.ini` | `71afdc57de97ec432752130bae9de4165affa564661116f5d79bda728f87faa9` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config Workstream 7, compiled preflight and manifest schema 2, is complete.
Contract-specific checkpoint discovery, sidecar enforcement, old-checkpoint
rejection, and evaluator hard-failure behavior remain assigned to the next
workstream and were not implemented here.

## Config Workstream 8 — checkpoint/replay isolation test-first pause

Workstream 8 began with a read-only audit of the checkpoint, evaluator,
manifest, launch, and action-trace paths. No production correction was made.
The audit confirmed the expected pre-implementation state:

- `eval_viewer.py` recursively chooses the newest checkpoint without a
  contract filter and catches every policy/checkpoint exception before
  silently switching to random actions;
- `train.sh` passes the unresolved value of `LOAD_MODEL_PATH`, including the
  literal `latest`, directly to Puffer and does not scope checkpoint output to
  a contract-specific directory;
- `run_manifest.py` deliberately rejects every non-cold checkpoint request
  until this workstream supplies validated resolution provenance;
- `contract_preflight.py` has no checkpoint resolver, schema-2 checkpoint
  sidecar/directory validator, or trace-provenance validator; and
- the in-memory viewer action trace records only seed, actions, tick hashes,
  and state-hash version, so old Prayer-timing artifacts have no fail-closed
  metadata gate yet.

Permanent Workstream 8 tests were then added without changing those production
paths. The coverage includes:

- a real evaluator subprocess proving `--random` remains explicit and valid;
- real evaluator subprocess failures for missing, malformed, short, long,
  unused, old-contract, and model-construction cases, with exact dimensional,
  byte-size, and version diagnostics;
- explicit and `latest` checkpoint resolution against schema-2 contract
  directory metadata, including a newer stale checkpoint and an even newer
  unmarked checkpoint that must not win;
- schema-2 explicit/latest manifest checkpoint provenance and forged-size
  rejection;
- launch/evaluator ordering and contract-specific checkpoint-directory use;
- trace provenance validation, including acceptance of syntactically valid
  actions 0-4 under the new timing contract and rejection of old-timing
  goldens; and
- the planned stale-literal scan across production code, current tests, the
  canonical config, and top-level runnable experiment configs. Documented A0
  baselines and `config/experiments/temporary` remain archival exclusions.

Exactly five correction-dependent tests were labeled `known_red`. Explicit
random evaluation and stale-literal scanning were kept in the ordinary
preservation suite because the current implementation already satisfies them.

### Preservation and expected-red results

The Release tree configured and built successfully. The full suite excluding
only `known_red` passed 143/143 tests in 2.31 seconds. This includes the two new
green Workstream 8 preservation cases. The five isolated expected-red tests
then ran in 1.20 seconds and failed at their intended assertion boundaries:

| Test | Result and root cause |
|---|---|
| `parity_contract_004_evaluator_hard_failures` | All six classes fell through to replay/random because the evaluator catch-all still sets `args.random = True`; long weights are only warned as unused. |
| `parity_contract_004_latest_checkpoint_filter` | The shared helper has no `resolve-checkpoint` command, so no sidecar/contract-aware explicit or `latest` resolution exists. |
| `parity_contract_004_manifest_warm_modes` | The manifest CLI has no checkpoint-resolution artifact input and `build_checkpoint_record` intentionally hard-rejects every non-cold mode. |
| `parity_contract_004_checkpoint_consumer_order` | Launch still passes unresolved `LOAD_MODEL_PATH`, has no contract checkpoint root/provenance input, and the evaluator retains unrestricted recursive discovery. |
| `parity_det_002_trace_provenance` | The shared helper has no trace-artifact validator, so required provenance and old Prayer-timing rejection are not enforceable. |

No mechanics, configuration, Puffer source, evaluator, launch, manifest, or
validation-helper production correction was made after these failures.

### Test-order issue found during diagnosis

One new static assertion should be corrected before production implementation.
The existing green Workstream 7 ordering test intentionally permits creation
of the base Python command after compiled preflight but before checkpoint
handling. The new Workstream 8 test currently demands checkpoint resolution
before base command construction. These are unnecessarily contradictory.

The behavior that matters is that compiled preflight happens before resolution,
and validated resolution happens before a load argument is appended, a
manifest is written, or Puffer executes. The proposed test-only correction is
therefore to require:

```text
compiled preflight -> base command construction -> checkpoint resolution and
validation -> manifest provenance -> Puffer execution
```

The raw `LOAD_MODEL_PATH` value, especially `latest`, must still never be
passed to Puffer. This changes no plan requirement and weakens no gate; it only
removes the conflict between two tests.

### Proposed production correction after approval

After the test-order correction is approved, the smallest plan-aligned
production change is:

1. Extend the project-owned compiled-contract helper with fail-closed
   checkpoint-directory metadata, explicit/`latest` resolution, resolution
   artifacts, and trace-provenance validation. A schema-2 contract marker must
   retain the exact compiled contract and its identity. Resolution must emit
   the exact selected path, file size, file hash, marker path, request mode,
   and validated identity with expected/actual diagnostics.
2. Scope Puffer checkpoint output to a directory keyed by the active compiled
   contract identity. `train.sh` creates/validates that marker after compiled
   preflight. Cold runs remain cold. Explicit and `latest` requests resolve to
   one validated concrete path before the load argument is appended; Puffer
   never receives the literal `latest`. This requires no Puffer source edit.
3. Let `run_manifest.py` accept and independently revalidate the resolution
   artifact for explicit/latest runs, then record the resolved path/size,
   sidecar identity/path, request mode, and `cold_start = false`. A missing,
   stale, or forged artifact remains a hard error.
4. Make `eval_viewer.py` use the same compiled preflight and checkpoint
   resolver before model construction. Compute the exact expected raw model
   bytes, compare them to the file size before loading, require every supplied
   float to be consumed, and turn missing/malformed/short/long/version/model
   failures into nonzero exits with all planned dimension, byte, and version
   diagnostics. Random behavior remains available only with explicit
   `--random`.
5. Validate saved action-trace provenance against the verified active
   contract, backend hash, and config hash. Missing metadata and old
   Prayer-timing traces must fail; action values 0-4 alone do not make a trace
   old. The later same-version replay gate will exercise the actual trace
   producer and per-tick replay using this artifact contract.

The test scaffold adds no long-running gate: the non-red full suite took 2.31
seconds and all five expected-red cases took 1.20 seconds.

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Workstream 8 is paused before both the test-order correction and production
implementation so they can be reviewed and approved.

### Approved correction and completed Workstream 8 implementation

The ordering correction and proposed production implementation were approved.
The static ordering assertion now agrees with the existing Workstream 7 gate:
compiled preflight precedes base command construction, validated checkpoint
resolution follows, manifest provenance is written next, and Puffer executes
last. The launcher still cannot append or use an unresolved checkpoint during
base command construction.

Two test-fixture details were corrected during implementation before the
post-change gate ran. The fake evaluator MinGRU originally modeled each layer
as `H x H`; the real Puffer `MinGRU` stores one `3H x H` matrix per layer. The
fixture and expected raw byte count now use the real layout. The evaluator
failure fixture also now has an active contract-directory marker so malformed,
short, and long inputs reach the intended byte-size check instead of stopping
earlier for a missing sidecar. The old-contract case retains a deliberately
mismatched adjacent sidecar. These corrections preserve the planned
assertions and make each negative case test its named failure class.

The approved production correction is implemented entirely in project-owned
paths:

- `contract_preflight.py` now creates and validates schema-2 checkpoint
  directory markers under `checkpoints/contracts/<contract identity>`, resolves
  explicit and `latest` requests, and writes a resolution artifact containing
  request mode, concrete path, exact file size, checkpoint SHA-256, sidecar
  path, and validated identity. `latest` considers only directories whose full
  compiled contract and identity match; newer stale and unmarked files are
  ignored. Explicit paths require either an adjacent sidecar or a compatible
  ancestor directory marker.
- The same helper independently reloads and validates resolution artifacts for
  downstream consumers. It also validates action-trace schema, seed,
  tick/action stride and ranges, per-tick hashes, full numerical/versioned
  contract, loadout, state-hash version, backend hash, and config hash. Traces
  using only prayer actions 0-4 remain syntactically valid under the new timing
  contract; an old Prayer-timing identifier is rejected.
- `train.sh` prepares the active contract directory after compiled preflight,
  passes it to Puffer through the ordinary `--checkpoint-dir` option, resolves
  explicit/`latest` requests through the shared helper, and appends only the
  returned concrete path. Puffer never receives the literal `latest`. The
  validated resolution artifact is passed to the manifest writer before
  training executes. When used by training, resolution also derives the exact
  2,721,792-byte canonical model size from the selected/default configs and
  rejects a mismatch before Puffer allocation with full dimensional and
  version diagnostics.
- `run_manifest.py` keeps the explicit cold-start record for cold runs. For
  explicit/latest runs it requires and independently revalidates the
  resolution artifact and records `cold_start = false`, request mode, exact
  path/size/hash, and sidecar identity/path. A missing artifact, mismatched
  mode/identity, stale sidecar, or forged file size/hash is a hard error.
- `eval_viewer.py` performs the compiled/config preflight first, uses the same
  contract-aware resolver, derives the canonical raw model size from the
  active `319` input, `{17,9,8}` output, and `256 x 3` MinGRU topology, and
  requires the file to be exactly 680,448 float32 values / 2,721,792 bytes. It
  independently confirms the constructed PyTorch state layout has that size
  and requires every supplied float to be consumed. Missing sidecars/files,
  malformed/short/long/old weights, unused values, and model construction or
  loading failures now exit nonzero with expected/actual dimensions, bytes,
  and all relevant contract versions. Random behavior remains available only
  through explicit `--random`.

No Puffer source file was edited. The only tracked Puffer-tree difference
remains the generated `pufferlib_4/config/fight_caves.ini` mirror from the
earlier config workstream.

Validation results:

- Python byte-compilation, `bash -n runescape-rl/train.sh`, and
  `git diff --check` passed.
- The five focused correction-dependent tests passed 5/5 in 1.32 seconds.
- Their temporary `known_red` labels were removed; `ctest -N -L known_red`
  reports zero tests.
- A fresh Release reconfigure/build succeeded.
- After the final training-size diagnostic assertion, all seven Workstream 8
  tests passed 7/7 in 1.59 seconds.
- The complete Release suite passed 148/148 tests in 3.69 seconds.
- A real `/usr/bin/python3` construction of the canonical Puffer PyTorch model
  independently reported 680,448 raw parameters and 2,721,792 bytes, exactly
  matching the evaluator calculation. Its network keys were the three expected
  `network.layers.{0,1,2}.weight` matrices.
- The stale-literal gate is green across production, current tests, and
  runnable configs.

Final Workstream 8 artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `tools/validation/contract_preflight.py` | `3c4bc086a5aa85c1705d59a12f024e7780578ade09a0727b7d48c04fc4004ef0` |
| `tools/validation/run_manifest.py` | `49d51a2b60a77385a94ab28995b1f5b4cba20e8dbb51c4f02c838cadc373db88` |
| `train.sh` | `acf99aa9e64bb98c5ce3e065b10d88591d998a34558ba83fda7157b3febfb51b` |
| `fc-viewer/eval_viewer.py` | `55f27fb6153ea44efc40db9e24547a3ac05a1832b3992c5d224dcfa91a11b6a4` |
| `fc-validation/tests/parity_fix_checkpoint.py` | `f6ff01e0fc5495679227831bb3e3d9fd5ab77ac9e16460287370fdb0a2b5e3a7` |
| `fc-validation/CMakeLists.txt` | `26c77b833b1c20780509a2242fae76c652fa1bd33610c737d5bde6bfac62e3e2` |
| `tools/validation/tests/phase2_static_guardrails.py` | `26ed108850c8593e063c9f7fe06384ede507211c1eede384086a2c3b8c718a20` |

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

Config Workstream 8 is complete. The later cross-system/release workstream
still owns actual same-version core/Puffer trace generation and replay,
sanitizer/soak/mutation gates, post-fix performance comparisons, and bounded
training health; none of those later gates was pulled forward here.

## Workstream 9 — final cross-system/release validation

Workstream 9 began by re-reading the root and module `AGENTS.md` files and the
exact final-gate requirements. A fresh Release configure/build and the complete
existing suite passed 148/148 tests in 3.62 seconds. There were zero
`known_red` tests. The build repeated pre-existing `fc-viewer/src/ui.c`
format-truncation warnings; no warning correction was made as part of this
test-first tranche.

The initial permanent final-gate scaffold added:

- repository-wide opt-in `FC_ENABLE_ASAN` and `FC_ENABLE_UBSAN` CMake options;
- one deterministic trace runner intended to compile against both normally
  linked `fc_core` and the Puffer direct-included core source path;
- a comparator that saves same-version and cross-compilation trace artifacts
  and reports the first structured category difference;
- explicit Prayer-action RNG draw-count coverage; and
- a replayable 32-seed by 5,000-tick fast invariant soak runner.

The narrow scaffold build stopped at its first failure. The linked replay
target compiled, but the direct-included target failed to link because
`fc-training/fight_caves.c` is the standalone executable entry point and
unconditionally defines `main`. Linking it with the replay runner therefore
produced two `main` definitions. No test executed, no production correction
was attempted, and the later release gates were not started.

The proposed test-only correction is to replace `fight_caves.c` in that target
with a small validation translation unit that includes `fight_caves.h` and
nothing else. That header is the actual direct-included/amalgamated source path
consumed by the binding, while the test-only translation unit introduces no
second entry point. This avoids changing the standalone training program or
any Puffer source. Approval is required before applying the correction.

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

### Approved replay-boundary correction and first final-gate results

The approved correction replaced the standalone `fight_caves.c` source in the
direct-included replay target with a validation-only translation unit whose
only inclusion is `fight_caves.h`. This compiles the exact core amalgamation
used by the Fight Caves binding without introducing the standalone program's
second `main`. Neither `fight_caves.c` nor any Puffer source was changed.

The corrected scaffold and focused gates produced these results:

- all four new replay/RNG/soak targets compiled successfully;
- the complete DET-001 through DET-004 group passed 11/11 tests in 0.70
  seconds, including byte-exact same-version traces and linked-core versus
  Puffer-direct-include traces;
- SOAK-001 passed 32 fixed seeds by 5,000 ticks in 0.82 seconds; and
- the active-loadout matrix passed all nine compiled loadouts. Each loadout
  passed exact reset propagation, linked/direct replay comparison, and a
  focused soak. The 45 recorded commands completed in 29.81 seconds and are
  saved under `fc-validation/tests/baselines/parity_fix_post/loadout_matrix.json`.

SOAK-002 then stopped on its first loadout and first invariant failure. For
`FC_LOADOUT_BLACK_DHIDE_RCB`, seed 66610 (matrix seed index 17), tick 9,
policy observation index 277 was `1.21428573`. Index 277 is
`FC_OBS_META_DMG_T_TICK`, defined as aggregate damage taken this tick divided
by maximum HP. Several wave-63 pending hits resolved on the same tick for 850
tenths of aggregate damage against the loadout's 700-tenths maximum HP, so the
ratio legitimately exceeded one under the current writer.

The root cause is a test-range error, not a parity implementation failure.
The soak scaffold incorrectly applied `[0,1]` to every policy feature. The
contract requires each normalized field to obey its own documented range.
`FC_OBS_META_DMG_T_TICK` is an unclamped aggregate ratio, and
`FC_NPC_PENDING_TICKS` is an unclamped tick-count divided by ten; both can
legitimately exceed one. All policy observations must still be finite, while
explicit proportions, one-hot fields, and clamped values remain in `[0,1]`.

The proposed test-only correction is to make the soak range assertion
field-aware: require every policy value to be finite and non-negative, retain
`[0,1]` for fields whose contract documents that bound, and allow the two
unclamped ratio/count fields above their mechanics-derived non-negative range.
No production observation, reward, damage, or combat behavior should change.
Approval is required before correcting the soak test and rerunning SOAK-002.

The Debug reproduction used to identify the exact array index traversed prior
seeds to their 200,000-tick cap and was therefore unexpectedly slow. Further
diagnosis and release execution will use optimized builds and direct
machine-readable failure details rather than that approach.

### Approved soak-range correction and completed soak gates

The approved validation-only range correction made SOAK-002 field-aware. It
continues to require every policy observation to be finite and non-negative,
continues to enforce `[0,1]` on bounded fields, and permits values above one
only for `FC_NPC_PENDING_TICKS` and `FC_OBS_META_DMG_T_TICK`, whose documented
writers are intentionally unclamped. No production observation or mechanics
code changed.

After that correction:

- the complete nine-loadout matrix again passed all 45 configure/build/reset,
  linked/direct replay, and focused-soak commands in 5.77 seconds; and
- SOAK-002 passed 256 fixed seeds for every one of the nine loadouts, using a
  200,000-tick cap or terminal episode, in 2.63 seconds.

The successful artifacts are saved as
`fc-validation/tests/baselines/parity_fix_post/loadout_matrix_after_soak_range_correction.json`
and
`fc-validation/tests/baselines/parity_fix_post/extended_soak_after_range_correction.json`.
The original failed SOAK-002 artifact remains preserved for provenance.

### MEM-001 sanitizer runner failure — awaiting review

The combined AddressSanitizer and UndefinedBehaviorSanitizer Release build
configured in 0.58 seconds and compiled successfully in 11.10 seconds. The
full 154-test CTest invocation then reported 136 failures in 4.50 seconds.
Every affected sanitized C executable stopped before its test logic with the
same runtime diagnostic:

`LeakSanitizer has encountered a fatal error` and
`LeakSanitizer does not work under ptrace (strace, gdb, etc)`.

This is an execution-environment failure rather than a reported leak,
out-of-bounds access, use-after-free, or undefined-behavior finding. As a
non-mutating diagnosis, the identical sanitized `npc_001` executable was run
outside the traced sandbox with the same `ASAN_OPTIONS` and `UBSAN_OPTIONS`;
it passed. This confirms that the binaries and sanitizer configuration work
when LeakSanitizer is not under `ptrace`.

No sanitizer option was weakened, no test or production correction was made,
and the mutation, performance, training-health, and final release gates were
not started. The proposed environment-only next action is to rerun MEM-001
outside the traced sandbox while preserving leak detection and all other
sanitizer settings. Approval is required before that rerun.

The failed sanitizer artifact and raw log are preserved under
`fc-validation/tests/baselines/parity_fix_post/sanitizers.json` and
`fc-validation/tests/baselines/parity_fix_post/sanitizers/`.

The three plan documents remain byte-identical:

| Plan | SHA-256 |
|---|---|
| `parity_fix.md` | `da16bb3b7d5000f27a566f645d9d493146bf55eab2b6b3990fd826f08a960e5d` |
| `parity_fix_tests.md` | `1b366dde1af39ce0fee205996d5f888e4291cf074282f0f5883102ba2fbe2aac` |
| `parity_fix_config.md` | `699d35191526434cf6e8ed9607c74b4b1afdd05085605cab4a8ed19a9a55b10d` |

### Approved unsandboxed MEM-001 rerun — two harness failures

The approved MEM-001 rerun executed outside the traced sandbox with leak
detection, AddressSanitizer, and UndefinedBehaviorSanitizer unchanged. The
previous LeakSanitizer/`ptrace` fatal error disappeared. The full suite then
passed 144/154 tests and stopped on ten validation-harness failures; it did
not report any leak, out-of-bounds access, use-after-free, or undefined
behavior.

Nine failures share one runner-integration cause. The affected Python
contract tests load sanitizer-built native libraries or invoke sanitizer-built
helpers from a normal Python interpreter. AddressSanitizer rejects this before
the contract logic runs because its runtime is not first in the process's
library list. The proposed test-runner correction is to locate the active
compiler's `libasan` and preload it for sanitizer-gate child processes. This
keeps `detect_leaks=1`, `halt_on_error=1`, strict string checking, and UBSan
unchanged; it only initializes the sanitizer runtime early enough for Python
native-library tests.

The tenth failure is the stale-contract static guard. The new replay corpus
contains `actions[2] = tick % 5 ...` to schedule a flick every five simulation
ticks. The guard intentionally rejects that textual form because it is also
the historical signature of selecting among five Prayer actions. This is not
a five-action runtime contract—the replay also cycles all eight Prayer
commands—but the validation source violates the repository-wide stale-literal
rule. The proposed test-only correction is to name the temporal period and
take the modulo by that named tick-period constant, preserving the exact
replay behavior without looking like an action-dimension literal.

No correction has been applied. MEM-001 remains incomplete, and later gates
remain paused pending review of these two test-harness corrections.

### Approved harness corrections and CPython LeakSanitizer diagnosis

The two approved validation-only corrections were applied. Sanitizer runtime
preloading was scoped as a recorded runtime-command environment override, and
the replay's five-tick flick schedule now uses a named temporal constant. The
stale-literal guard then passed directly. No production or Puffer source was
changed.

The corrected unsandboxed MEM-001 run built successfully and executed all 154
CTest entries, but stopped with 142 passes and 12 failures. This run produced
no parity-core out-of-bounds, use-after-free, signed-overflow, invalid-shift,
or undefined-behavior report. Instead, preloading LeakSanitizer into every
CTest process exposed shutdown allocations retained by the uninstrumented
system CPython and NumPy runtimes. Several affected tests printed their own
`PASS` result before LeakSanitizer changed the process exit status. The stacks
begin in `/usr/bin/python3.12` allocation APIs such as `PyObject_Malloc`,
`PyUnicode_FromKindAndData`, and Python initialization/import paths; they do
not identify a parity core allocation site.

A non-mutating single-test diagnostic set `PYTHONMALLOC=malloc` while keeping
leak detection enabled. The contract assertion still passed, after which
LeakSanitizer reported 529,138 bytes retained in 10,282 CPython/NumPy
allocations. This rules out Python's small-object allocator as a correction
and confirms that globally leak-checking an uninstrumented Python host cannot
serve as a meaningful native-backend leak gate on this machine.

The proposed runner-only correction is to split the same complete CTest set
into two explicit phases:

1. Run all sanitizer-linked native parity, contract, preservation, canary,
   replay, and soak executables with `detect_leaks=1`, ASan, and UBSan. Run
   pure Python/static contract tests normally in this phase.
2. Run only the nine Python-hosted tests that load sanitizer-built native
   libraries with `libasan` preloaded and `detect_leaks=0`. ASan address
   checking and UBSan remain enabled for the loaded native code; leak
   detection is disabled only for the uninstrumented Python host whose own
   retained allocations are not attributable. The native C targets and every
   reduced loadout soak retain leak detection.

Together the two phases still execute all 154 tests. This also prevents
expected-failure subprocess tests from passing merely because CPython itself
exited on a LeakSanitizer report. The exact phase commands and environment
overrides would remain recorded in the MEM-001 artifact. No further
correction has been applied pending approval.

### Approved two-phase sanitizer runner and MEM-001 pass

The approved runner-only correction split the complete test inventory without
skipping any test. The leak-detecting phase ran 145 sanitizer-built native and
ordinary Python/static tests with `detect_leaks=1`. The second phase ran the
nine Python-hosted native-library contract tests with `libasan` preloaded,
ASan address checks and UBSan enabled, and leak accounting disabled only for
the uninstrumented CPython host. All reduced native loadout checks retained
`detect_leaks=1`.

MEM-001 passed all 49 recorded commands in 65.51 seconds:

- both CTest phases passed, covering all 154 tests exactly once;
- all nine sanitizer loadout configurations and builds passed;
- every loadout passed reset propagation and linked-core versus
  direct-included replay comparison; and
- every loadout passed the two-seed by 5,000-tick reduced soak.

There were no ASan, LeakSanitizer, or UBSan findings. Exact commands,
per-command environment overrides, raw logs, compiler/runtime metadata, and
timings are saved in
`fc-validation/tests/baselines/parity_fix_post/sanitizers.json` and its
adjacent `sanitizers/` log directory. MEM-001 is complete; the next planned
gate is the targeted mutation audit.

### Targeted mutation audit baseline failure

A reusable validation-only mutation runner was added at
`fc-validation/tests/parity_mutation_audit.py`. It uses exact, count-checked
source substitutions in a disposable source snapshot, builds and runs only
the focused tests assigned to each planned mutation, restores and rebuilds
after every mutant, stops on the first survivor or harness failure, and
verifies that the original source hashes did not change. The runner passed
Python syntax and repository whitespace checks.

The first audit attempt stopped before applying any mutation because the
isolated focused baseline passed 30/31 selected tests. The sole failure was
`parity_contract_004_manifest_schema2`, which reported that the backend/source
Git commit was unavailable. The disposable snapshot contains the relevant
`runescape-rl` and Puffer configuration/source files but no Git metadata at
its temporary workspace root. The manifest test derives that root from its
copied location and correctly rejects a workspace for which `git rev-parse
HEAD` returns no commit. This is an isolation-runner omission, not a parity,
manifest, production, or Puffer failure; the test passes in the real Git
workspace. No mutant ran and the original source hashes remained unchanged.

The proposed validation-only correction is to initialize the disposable
workspace as a temporary Git repository and create a commit containing its
isolated snapshot before configuring the audit build. This gives the manifest
test a real snapshot-local commit without copying or pointing at the original
repository metadata and without modifying production or Puffer sources. No
correction has been applied pending approval.

### Approved snapshot Git correction and mutation 27 stop

The approved runner correction now initializes, stages, and commits the
disposable `runescape-rl` plus required Puffer configuration/source snapshot
before configuring the isolated build. The setup commands are recorded in the
artifact. The real repository metadata and original Puffer source remain
untouched. The isolated focused baseline then passed.

The targeted audit killed mutations 1 through 24 with their assigned tests and
killed mutations 25 and 26 with the compile-time mask-size guards. It stopped
at mutation 27, `hash_omits_prayer_counter`, before running later mutations.
The apparent survivor is an audit-runner mapping error rather than missing
hash coverage: the runner requested the nonexistent CTest name
`parity_det_001_hash_field_coverage`. CTest reports `No tests were found` with
a successful exit status for that regex, so the runner classified the mutant
as surviving. The registered test is named
`parity_det_001_field_coverage`, and its independent reference-hash table
explicitly mutates `player.prayer_drain_counter`.

The proposed validation-only correction is to use the registered CTest name
for all four hash-omission mutations and add a runner guard that verifies the
exact requested test count before accepting either the baseline or a mutant
result. This prevents an empty or partially matched CTest selection from ever
being treated as a pass. The failed audit artifact and raw logs are saved at
`fc-validation/tests/baselines/parity_fix_post/mutation_audit.json` and its
adjacent `mutation_audit/` directory. The mutation was restored, the
disposable tree was removed, and the original source hashes were unchanged.
No correction has been applied pending approval.

### Approved CTest selection correction and mutation 31 stop

The approved runner correction replaced all four stale hash-test mappings
with the registered `parity_det_001_field_coverage` name. The audit now reads
the generated CTest JSON inventory and requires every requested test name to
exist exactly once before it will run or accept the focused baseline or any
mutant result. Test names are escaped when building the anchored selection
regex. The corrected focused baseline passed, mutations 1 through 24 and 27
through 30 were killed by their assigned tests, and mutations 25 and 26 were
killed by compile-time mask-size guards.

The audit stopped at mutation 31, `replay_omits_reward_runtime`. Both
`parity_det_002_same_version_replay` and
`parity_det_003_core_puffer_replay` passed after the replay writer's
`print_runtime` call was removed. The replay record still contained the key
`reward_runtime=` but with an empty value. The current Python replay checker
only compares one complete line to another. Both repeated linked runs omit the
same value, and both the linked-core and direct-included binaries are compiled
from the same mutated replay writer, so equality remains true on both sides.
The checker does not independently validate that each tick record contains a
nonempty, structurally complete reward-runtime payload.

The proposed validation-only correction is to add replay-record schema
validation before comparison. Every tick record would be required to contain
the expected fields, and `reward_runtime` would be required to contain its
exact 15 comma-separated components. This makes an identically incomplete
pair fail before equality comparison while preserving the existing
same-version and linked-versus-direct determinism checks. The mutation was
restored, the disposable tree was removed, and original source hashes were
unchanged. No correction has been applied pending approval.

### Approved replay completeness correction and final mutation stop

The approved validation-only correction now checks every replay tick's exact
field order before comparison and requires `reward_runtime` to contain exactly
15 nonempty components. Both unmutated replay tests passed. In the restarted
audit, mutations 1 through 30 were killed again, mutation 31's reward-runtime
omission was killed by the new completeness check, and the evaluator fallback,
incompatible-latest, and schema-1 configuration mutations 32 through 34 were
killed by their assigned tests.

The audit stopped at the final mutation,
`checkpoint_accepts_schema_one`. The assigned
`parity_contract_004_latest_checkpoint_filter` fixture currently provides a
compatible schema-2 sidecar, an incompatible schema-2 sidecar, and an unmarked
checkpoint. It does not provide a schema-1 checkpoint sidecar. Changing the
validator from schema 2 only to accepting schemas 1 or 2 therefore does not
alter any exercised input, and the test passes despite the defect.

The proposed validation-only correction is to add an otherwise fully
compatible schema-1 sidecar and a newer checkpoint to the existing latest
filter fixture. The correct implementation must ignore it and retain the
compatible schema-2 checkpoint; the mutant would select the newer schema-1
checkpoint. The same fixture should also assert that explicitly requesting
the schema-1 checkpoint fails with the schema expected/actual diagnostic.
This directly covers both selection and hard-rejection behavior without
changing production or Puffer code. The mutation was restored, the disposable
tree was removed, and original source hashes were unchanged. No correction
has been applied pending approval.

### Approved schema-1 checkpoint coverage and mutation-audit pass

The approved checkpoint fixture now includes an otherwise compatible schema-1
sidecar with a newer checkpoint. The unmutated contract test confirms that
`latest` ignores it in favor of the compatible schema-2 checkpoint and that an
explicit schema-1 request fails with `expected=2` and `actual=1`. The focused
checkpoint test passed before the audit rerun.

The complete isolated targeted mutation audit then passed all 35 planned
mutations in 103.24 seconds. Thirty-three mutations were killed by their
assigned assertion tests and the two stale mask-size mutations were killed by
compile-time guards. The exact CTest-selection inventory guard passed, every
mutant was restored and rebuilt before continuing, the disposable snapshot
was removed, and the original source hashes were unchanged. The final JSON
artifact and recorded raw command logs are saved at
`fc-validation/tests/baselines/parity_fix_post/mutation_audit.json` and its
adjacent `mutation_audit/` directory. The targeted mutation audit is complete.

### PERF-001 linked-core throughput pass

The checked-in Release benchmark was built against the normal linked
`fc_core` target. The current harness SHA-256 exactly matches the retrospective
A0 harness. Both post-fix workloads used five fresh processes, CPU 0, seed 73,
250,000 unmeasured warmup steps, 5,000,000 measured steps, and scenario span
512.

The common five-Prayer-action workload recorded a median 1,012,151.235 SPS
with 0.3204% coefficient of variation, versus the A0 median 966,603.974 SPS.
This is a 4.7121% improvement, so neither regression threshold is triggered.
Peak RSS was 23,852 KiB versus 23,572 KiB at A0. All five trials produced the
same deterministic checksum and action counts.

The post-fix native workload exercised all eight Prayer commands and recorded
a median 1,013,315.140 SPS with 0.0729% coefficient of variation and peak RSS
23,692 KiB. Every Prayer action had more than 623,000 measured selections in
each deterministic corpus, and all trials shared the same checksum. The
benchmark source does not call the canonical state hash in its timed loop.
Exact commands, trial logs, metadata, and statistics are saved in
`fc-validation/tests/baselines/parity_fix_post/perf_core_common.json` and
`perf_core_native.json` with their adjacent raw-log directories. PERF-001 is
complete.

### PERF-002 Puffer vectorized rollout pass

The real compiled Fight Caves binding and Puffer vectorized path ran five
fresh-process trials for each workload with the A0 harness hash and matching
4,096 environments, 16 threads, CPU affinity 0-15, action cycle 256, 16,384
warmup vector steps, and 32,768 measured vector steps. The compiled post-fix
contract reported observation stride 319 floats, native mask stride 34 bytes,
action stride 3, and action dimensions `{17, 9, 8}` in every trial.

The common five-Prayer-action workload recorded a median 5,203,501.152 SPS
with 1.6334% coefficient of variation, versus the A0 median 5,103,409.783 SPS.
This is a 1.9613% improvement. The native eight-action workload recorded a
median 5,360,014.808 SPS with 1.9942% coefficient of variation. Its fixed
corpus selected every Prayer action exactly 16,777,216 times per trial. Peak
RSS was 770,804 KiB for common and 770,844 KiB for native, versus 770,476 KiB
at A0.

All trials produced 134,217,728 measured transitions, stable reset counts and
the same final-observation checksum within each workload. No sampler,
contract, mask, buffer, NaN, Inf, CUDA, or process error was recorded. Exact
commands, raw trials, environment metrics, resource samples, and statistics
are saved in
`fc-validation/tests/baselines/parity_fix_post/perf_puffer_rollout_common.json`
and `perf_puffer_rollout_native.json` with their adjacent raw-log directories.
PERF-002 is complete.

### PERF-003 sandbox CUDA stop

The first post-fix end-to-end training trial stopped before warmup or timing
with process abort code `-6`. The compiled Puffer trainer asserted in
`src/bindings.cu:393` that no CUDA device was available. Non-mutating
diagnostics showed that the host GPU is present (`NVIDIA GeForce RTX 5070 Ti`),
but the restricted execution environment exposes no `/dev/nvidia*` device
nodes to Python. PyTorch 2.10.0+cu128 consequently reports
`torch.cuda.is_available() == False` and a device count of zero. The CPU-only
PERF-002 path was unaffected.

This is an execution-sandbox device-visibility failure, not a parity,
contract, mask, buffer, learner, Puffer-source, or CUDA-kernel result. No
measured training trial was accepted and no setting or source was changed. The
captured failure is saved in
`fc-validation/tests/baselines/parity_fix_post/perf_puffer_training/trial-01.log`.
The proposed operational correction is to rerun the exact same five-trial
PERF-003 command outside the restricted sandbox so the existing host NVIDIA
device nodes are visible. No correction or rerun has been performed pending
approval.

### Approved unsandboxed PERF-003 run and repeat threshold

The approved unchanged unsandboxed run exposed the host GPU and completed all
five cold-start trials, including 16 warmup and 32 measured updates per trial.
Every trial reported the post-fix `319`/`34`/`{17, 9, 8}` contract, completed
33,554,432 measured transitions and 32 PPO updates, saved and loaded its
checkpoint successfully, and recorded no sampler, mask, buffer, NaN, Inf,
CUDA, learner, or checkpoint error.

The median total-training throughput was 1,494,567.499 SPS with 2.0882%
coefficient of variation, versus the A0 median 1,576,079.574 SPS. This is a
5.1718% regression: it crosses the plan's 5% investigation-and-repeat
threshold but remains below the 10% failure threshold. The expected contract
shape change increased the policy from 678,912 to 680,448 parameters. Peak GPU
memory increased from approximately 3.3034 GiB to 3.3359 GiB and peak RSS from
1,242,372 KiB to 1,246,748 KiB.

Timing decomposition localizes the difference to rollout environment stepping,
not policy inference or the learner. A0 environment stepping was 9.22-9.42
seconds; post-fix trial 1 was 9.14 seconds, while trials 2-5 were 10.03-10.12
seconds. Policy inference remained about 2.02-2.07 seconds and learner time
about 9.57-10.13 seconds in both versions. PERF-001 and PERF-002 were faster
than A0, so there is no evidence of a general core or vectorized-environment
hot-loop regression. No competing GPU compute process remained afterward.
The first post-fix trial being close to A0 while later identical trials slowed
is consistent with host sustained-run state or scheduling, but does not by
itself establish attribution.

Per the plan, the proposed next action is an unchanged controlled five-trial
repeat saved to a separate artifact. No settings or source would change, and
the first result remains preserved. The repeat has not been run pending
approval.

### Approved PERF-003 controlled repeat and pass

The approved unchanged controlled repeat completed all five trials with a
median 1,497,197.484 SPS and 0.5370% coefficient of variation. This is a
5.0050% regression from the A0 median, effectively reproducing the first
run's borderline result while substantially reducing run-to-run variation.
It remains well inside the 10% failure threshold. Per review direction, no
further marginal-performance rerun or tuning will be pursued.

All repeat trials completed 32 PPO updates and 33,554,432 measured
transitions, used the 680,448-parameter post-fix policy, saved and reloaded
checkpoints successfully, and recorded no sampler, mask, buffer, NaN, Inf,
CUDA, learner, or checkpoint errors. Peak RSS was 1,246,656 KiB and peak GPU
memory was 3.3380 GiB. The original and controlled-repeat artifacts are both
preserved at `perf_puffer_training.json` and
`perf_puffer_training_repeat.json` with their adjacent raw logs and
checkpoints. The required investigation and repeat are complete; PERF-003
passes.

### TRAIN-001 producer gap

Preparation for the next gate found that the checked-in TRAIN-001 scaffolding
contains only `fc-validation/tests/parity_training_health.py`, which validates
the required machine-readable health-report schema. There is no checked-in
producer, command, `train.sh` option, or native diagnostic API that emits that
report. A repository-wide source search found no implementation of the
required `health_instrumentation`, finite-channel results,
`invalid_prayer_action_counts`, post-warmup memory samples, evaluator-load
result, or report serialization outside the validator itself.

PERF-002 and PERF-003 establish several underlying facts—finite public
observations/rewards/loss metrics, completed rollout and PPO updates, the
compiled native-mask contract, absence of sampler/CUDA/buffer errors, and
native checkpoint save/load—but they do not expose or independently verify
all TRAIN-001 channels. In particular, the production Puffer Python binding
does not expose tensor storage needed to inspect logits, advantages, returns,
gradients, or stored PPO-recomputation masks, and its normal log does not
publish per-Prayer action counts or the required memory-sample series.
Constructing a synthetic passing JSON report from the performance artifacts
would therefore not satisfy the plan.

No TRAIN-001 result has been claimed and no source has been changed to work
around this gap. Any correction must preserve the explicit rule that the
Puffer source tree is never modified. The recommended direction is a
validation-owned instrumented health runner/companion that uses the same
compiled Fight Caves backend and canonical configuration, combined with a
bounded real `train.sh` cold-start invocation for entry-point, preflight,
checkpoint, and evaluator validation. The exact design requires review before
implementation because the current binding does not expose all learner
internals.

### Approved TRAIN-001 companion, bounded run, and pass

The approved correction added a validation-owned producer without adding a
production hook or changing PufferLib source. The checked-in components are:

- `fc-validation/tests/parity_training_health_probe.cu`, a diagnostic module
  that accepts the real compiled `_C.PuffeRL` object and copies only reduced
  finite/mask/action-count results back from its native GPU buffers;
- `fc-validation/tests/build_parity_training_health_probe.sh`, an incremental
  isolated build into the ignored main build directory; and
- `fc-validation/tests/parity_training_health_run.py`, which runs the bounded
  production entry point, evaluator checkpoint load, native diagnostic
  blocks, report serialization, and the existing report validator.

The companion calls no rollout, PPO, optimizer, or environment implementation
of its own. Normal `_C.rollouts()` and `_C.train()` remain the only behavior
under test. The probe reads rollout observations/rewards/log-probabilities,
decoder outputs, action/mask snapshots, PPO recomputation action/mask buffers,
advantages, returns, loss accumulators, and gradient buffers after those
normal calls. It is absent from PERF-002, PERF-003, and production training.
`git diff -- pufferlib_4/src` remained empty.

An initial bounded pass succeeded, after which harness review found an
ordering weakness: a stale pre-existing production extension could have been
loaded before `train.sh` made its normal rebuild decision. The validation
runner was tightened before acceptance so the real `train.sh` cold start now
establishes the current compiled backend first; the probe is built and loaded
only afterward. The final corrected-order command was:

```bash
python3 fc-validation/tests/parity_training_health_run.py \
  --repo-root . \
  --puffer-dir ../pufferlib_4 \
  --warmup-updates 2 \
  --measured-updates 4 \
  --output fc-validation/tests/baselines/parity_fix_post/training_health.json \
  --raw-dir fc-validation/tests/baselines/parity_fix_post/training_health
```

The final run passed in approximately 21 seconds. Its real `train.sh` phase
took 10.718 seconds, completed the compiled-contract preflight and schema-2 run
manifest path, saved a final checkpoint at exactly 6,291,456 transitions, and
then loaded all 680,448 weights through the CPU evaluator. The evaluator
printed `Policy ready (CPU)` and did not enter or report random fallback. The
checkpoint is 2,721,792 bytes with SHA-256
`d89b92df69dc7b3f989917475b63a502758c117afee7df10bca7771ca16203cd`.

The instrumented cold start completed exactly six PPO updates and 6,291,456
transitions. Every required finite channel passed: observations, logits,
rewards, advantages, returns, policy/value losses, entropy, KL, and gradients.
Rollout masks and stored PPO-recomputation masks were valid; every head had a
legal action; and zero invalid buffer events occurred. Fixed-seed Prayer action
counts were:

```text
[947663, 906702, 495368, 1164780, 575267, 704125, 747928, 749623]
```

All eight corresponding invalid-action counts were zero, including commands
5-7. The bounded run emitted 1,998 completed episodes and all 18 reward-channel
metric keys. Four post-warmup GPU-memory samples were exactly 3,365.125 MiB;
RSS samples were 1,219.543, 1,219.559, 1,219.570, and 1,219.570 MiB. Both pass
the 64 MiB plateau tolerance.

The machine-readable report passed an independent invocation of
`parity_training_health.py` with the exact transition and update budgets. Final
artifact hashes are:

| Artifact | SHA-256 |
|---|---|
| `parity_training_health_run.py` | `9187a009351b938e1ecbdda426dad5da95b73312a3e18e942d0a9e5db035481c` |
| `parity_training_health_probe.cu` | `8808ed3c8a2803b784efd05cfe90784441283c8fc5aea44f2e5993a8c832ce46` |
| `build_parity_training_health_probe.sh` | `85046f3149adf87fbb62eec4a5984f46f6dc3aeb4416c8664c8d313cbea59f2c` |
| compiled validation probe | `b024ff1c731d055e1e22c0a9a54831ecd8c3901859349760af049370ed971873` |
| `training_health.json` | `67c48ba16ccb36745c2db08aaa0aa4a972ad9da596f90314bf7ae3540120d0f4` |

TRAIN-001 itself passes.

### Post-TRAIN-001 preservation-suite stop and diagnosis

The subsequent broad non-red preservation command

```bash
ctest --test-dir build-parity -LE known_red --output-on-failure
```

passed 134 of 136 tests in 0.58 seconds. Two evaluator contract tests stopped
at their command-line usage guard because CTest supplied only the case name and
omitted the required compiled-fixture path:

```text
parity_contract_002_evaluator_dimensions
parity_contract_003_evaluator_buffers
```

Read-only diagnosis shows this is a stale generated build registration, not a
TRAIN-001, evaluator-contract, fixture, or gameplay assertion failure. The
current `fc-validation/CMakeLists.txt` defines `parity_contract_fixture` and
passes `$<TARGET_FILE:parity_contract_fixture>` to both commands. The generated
`build-parity/fc-validation/CTestTestfile.cmake` predates that source by more
than eight hours, contains the older two-argument commands, and its build graph
has no `parity_contract_fixture` target. All 127 tests that preceded these
workstreams had passed from this build before the later CMake registrations
were added, which explains why the stale directory had not surfaced here.

No source, test, configuration, build registration, or plan was changed to
resolve this preservation failure. The proposed operational correction is to
reconfigure and rebuild the existing `build-parity` directory from the current
source, then repeat the exact non-red CTest command. This is pending approval.

### Approved build regeneration and preservation pass

The approved operational correction regenerated the existing Release build
without changing source:

```bash
cmake -S . -B build-parity -DCMAKE_BUILD_TYPE=Release
cmake --build build-parity -j2
ctest --test-dir build-parity -LE known_red --output-on-failure
```

The regenerated CTest commands included the required
`libparity_contract_fixture.so` argument, and the fixture target built
successfully. Regeneration also exposed all current registrations: the suite
increased from the stale directory's 136 tests to 154 tests. All 154 passed in
4.70 seconds. This includes both formerly mislaunched evaluator cases and the
later deterministic replay, fast soak, compiled preflight, rejection,
schema-2 manifest, consumer-order, active-config, evaluator hard-failure,
checkpoint isolation, and trace-provenance gates that were absent from the
stale generated registration.

The preservation stop is resolved with no code or test correction. TRAIN-001
and the complete current non-red Release suite are green.

### Final fresh Release closeout

The approved final closeout used a brand-new build directory that was confirmed
absent before configuration. The exact commands were:

```bash
cmake -S . -B build-parity-closeout-20260806 -DCMAKE_BUILD_TYPE=Release
cmake --build build-parity-closeout-20260806 -j2
ctest --test-dir build-parity-closeout-20260806 --output-on-failure
```

Configuration identified GNU C 13.3.0 and generated the fresh Release build
successfully. The complete build succeeded. It emitted pre-existing
`fc-viewer/src/ui.c` string-truncation warnings but no errors.

The unfiltered CTest run passed all 154 of 154 registered tests in 4.69 seconds
with zero failures. This includes all 84 `parity_fix`-labelled tests, the full
preservation suite, deterministic replay and hash checks, contract and buffer
checks, the fast soak, configuration/preflight/manifest checks, evaluator and
checkpoint isolation checks, and core-versus-Puffer replay.

This satisfies the final-release requirement that the full CTest suite pass
from a genuinely fresh Release build after all parity-fix tests were
registered. No production source, test, configuration, plan, or PufferLib
source change was required for this closeout. All planned parity-fix
implementation and validation work is complete.
