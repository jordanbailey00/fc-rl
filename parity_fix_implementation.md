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
Post-fix comparison must use the same compiler flags, CPU pinning, step count,
seeds, action schedule, adapter work, and reward/ablation settings. A median
below 990,638 SPS is a regression greater than 5% and requires investigation.

### A0 gate conclusion

A0 passes: the untouched existing suite is green, no pre-existing test failure
requires a plan pivot, deterministic replay is stable for the saved corpus,
and a reproducible five-run SPS baseline is recorded. No gameplay, contract,
configuration, test, or plan source was changed during A0.

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
