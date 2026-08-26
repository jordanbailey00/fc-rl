# C Code Static and Dynamic Analysis

Audit date: 2026-08-25
Audited commit: `cae3a48efbac` (`dev`)
Scope: `runescape-rl/fc-core`, `runescape-rl/fc-training`, and
`runescape-rl/fc-viewer`

## Executive summary

The audit found no orphan production `.c` files, no sanitizer failures, no
Valgrind errors, and no confirmed dead implementation in `fc-training`.

There is, however, real cleanup available:

- `fc-core` has two public pathfinding wrappers with no production callers.
  Follow-up removal validation found three test-only references that the initial
  source-reference inventory missed.
- `fc-viewer` has five unused private functions, several unused tables/fields,
  four dead stores, an unreachable random-action mode, and a stale comment that
  advertises a nonexistent `--auto` option.
- The viewer also exposes a dormant set of UI wrapper APIs that the production
  viewer, tests, and other modules never call. A roughly 180-line UI self-test
  subgraph is compiled but never invoked; it should either become a real test or
  be removed.
- A block of approximately 42 lines contains old, commented-out viewer test
  cases marked `PASSED`. It is documentation debris rather than executable
  code.

Approximately 150-180 lines are high-confidence mechanical cleanup. A further
roughly 280-330 lines are dormant UI API/self-test/commented-test material whose
removal depends on whether that API is intentionally being reserved for future
viewer work. These estimates include declarations and comments and are not a
promise of an exact final diff.

No source or configuration was changed during this audit. The analysis tools,
instrumented builds, logs, and coverage data were created under `/tmp`.

## Remediation status

The two unused public core pathfinding APIs were removed in commit `7a8e87fb2`.
The caller-free viewer-function set identified below was then removed: 27
definitions plus their public declarations, totaling 429 deleted lines. This
includes the high-confidence loader/drawing helpers, dormant UI convenience
wrappers, and the uninvoked runtime self-test subgraph. Active lower-level UI,
asset, animation, and rendering paths remain intact.

Post-remediation validation passed all 163 CTest tests. A repeat strict Clang,
Cppcheck, and linker-reachability scan found no remaining caller-free function
from the audited set. Clang still reports header-defined animation helpers as
unused in a small test translation unit, but the production viewer calls those
same helpers; these remain confirmed false positives. The 100M deterministic
training comparison is recorded as Comparison 20 in `baseline.md` and matches
the original baseline exactly on every behavioral and learning metric.

## Tools built for this audit

### Static analysis tool

The temporary tool `/tmp/fc_code_analysis_static.sh` performed:

1. A clean Clang 18 CMake build outside the repository.
2. A separate compilation pass over the three training translation units.
3. Exhaustive Cppcheck 2.13 analysis over core, training, and viewer sources.
4. Function/data-section linking with linker garbage-collection reporting.
5. `nm` symbol inventory, source-reference checks, and stale-marker searches.
6. A follow-up Clang Static Analyzer pass over every core and viewer `.c` file.

The strict compiler pass used:

```text
-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
-Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
-Wmissing-declarations -Wredundant-decls -Wswitch-enum
-Wunreachable-code -Wunreachable-code-aggressive
-Wunused-function -Wunused-parameter -Wunused-variable
-Wunused-const-variable -Wformat=2 -Wnull-dereference
-Wdouble-promotion -Wimplicit-fallthrough
```

The reachability link additionally used:

```text
-ffunction-sections -fdata-sections
-Wl,--gc-sections -Wl,--print-gc-sections
```

Cppcheck used:

```text
--enable=warning,style,performance,portability,unusedFunction
--inconclusive --force --max-configs=64
```

Raw output is under `/tmp/fc-code-static-audit-20260825`.

One direct Clang parse of `fc-training/binding.c` could not find Clang's
`omp.h`; this was a toolchain include-path limitation in that standalone audit
command. The same binding and core built successfully with GCC in the
instrumented CMake run, and the full test suite passed. It is not a repository
build failure.

### Dynamic analysis tool

The temporary tool `/tmp/fc_code_analysis_dynamic.sh` built all CMake targets
outside the repository using GCC 13 with:

```text
-O0 -g --coverage -fprofile-abs-path -fno-omit-frame-pointer
-fsanitize=address,undefined -fno-sanitize-recover=all
```

It then ran:

- all 164 CTest tests under AddressSanitizer and UndefinedBehaviorSanitizer;
- a 100-episode randomized standalone training smoke test;
- an instrumented real Raylib viewer startup/render/shutdown at wave 63;
- Gcov/gcovr line coverage;
- Valgrind Memcheck on the 51-test headless backend executable; and
- Valgrind Memcheck on a separate 100-episode training executable.

LeakSanitizer was disabled because this managed execution environment runs
processes under `ptrace`, which LeakSanitizer rejects. Valgrind supplied the
independent heap/leak check instead.

Raw output is under `/tmp/fc-code-dynamic-audit4-20260825`.

## Dynamic results

| Check | Result |
|---|---|
| CTest with ASan + UBSan | 164/164 passed |
| Randomized training with ASan + UBSan | 100 episodes completed; no sanitizer diagnostic |
| Instrumented Raylib viewer | Initialized assets and OpenGL, rendered, and shut down with status 0; no sanitizer diagnostic |
| Valgrind headless backend | 51/51 checks passed; 0 errors; 0 bytes in use at exit |
| Valgrind standalone training | 100 episodes completed; 0 errors; 0 bytes in use at exit |

Combined test plus viewer-smoke line coverage was:

| Module | Executed / instrumented lines | Coverage |
|---|---:|---:|
| `fc-core` | 2,692 / 2,773 | 97.1% |
| `fc-training` in the CMake/test run | 238 / 260 | 91.5% |
| `fc-viewer` | 3,363 / 8,692 | 38.7% |

The separate randomized training executable covered 97.7% of its adapter code
and 74.0% of core. Viewer coverage is expected to be lower because one bounded
render run cannot click every tab, toggle every debug mode, exercise every
loader failure, or play every visual sequence. A zero-hit viewer line was not
called dead unless static call/reference evidence independently agreed.

## Confirmed dead or stale code

### `fc-core`

| Location | Finding | Evidence | Recommended disposition |
|---|---|---|---|
| `fc-core/src/fc_pathfinding.c:96`, `fc-core/include/fc_pathfinding.h:63` | `fc_footprint_available_for_entity()` has no production caller. | Both production viewer and standalone training links discard its section. Follow-up validation found one guardrail whose sole subject was this wrapper. | Remove the declaration, documentation, definition, and wrapper-only guardrail. |
| `fc-core/src/fc_pathfinding.c:613`, `fc-core/include/fc_pathfinding.h:124` | `fc_pathfind_bfs()` has no production caller. The live click-to-move path uses `fc_pathfind_bfs_move_near()`. | Both production links discard it. Follow-up validation found two route guardrails that called this exact-destination wrapper. | Remove the wrapper and declaration, and redirect the route guardrails to the live move-near BFS entry point using reachable destinations. |

No internal static core function was diagnosed as unused. Every source listed in
`fc-core/core_sources.txt` is built, and every core `.c` file is present in that
canonical list.

### `fc-training`

No confirmed dead training function or source file was found.

The following superficially suspicious functions are live integration hooks,
not cleanup candidates:

- `c_reset()`, `c_step()`, `c_render()`, and `c_close()` are the interface
  expected by the Puffer adapter. `c_render()` intentionally remains a no-op.
- `c_close()` calls `fc_destroy()` and is part of lifecycle symmetry.
- `fight_caves.c` is the standalone `--local`/`--fast` diagnostic launcher.
- `contract_dump.c` is used by the binding and contract validation path; it had
  100% line coverage in the test run.

The strict compile reports missing prototypes for Puffer hook names because the
Puffer macro consumes those names later in `binding.c`. That is an interface
shape issue, not evidence that the hooks are dead.

### `fc-viewer`: high-confidence mechanical cleanup

These functions have no call site in the complete repository. Private functions
were also reported by `-Wunused-function`; externally visible functions were
discarded from the real viewer executable by the linker.

| Location | Dead item | Notes |
|---|---|---|
| `fc-viewer/src/viewer.c:1613` | `format_speed_label()` | The active console constructs its labels elsewhere. |
| `fc-viewer/src/viewer.c:2263` | `load_collision_for_objects()` | Object loading no longer calls this old collision-copy path. |
| `fc-viewer/src/fc_terrain_loader.h:216` | `terrain_height_avg()` | Current placement uses the active terrain sampling functions instead. |
| `fc-viewer/src/fc_models.h:460` | `models_set_shader()` | No loaded model set uses this bulk shader setter. |
| `fc-viewer/src/ui.c:2812` | `draw_slot_box()` | Replaced by the decoded/asset-backed UI drawing path. |
| `fc-viewer/src/fc_assets.c:231` | `fc_repo_exists()` | Declaration and definition only. |
| `fc-viewer/src/fc_assets.c:251` | `fc_repo_fopen()` | Its only apparent caller is the already-dead `load_collision_for_objects()`. `fc_repo_resolve_path()` itself remains live for config lookup and validation. |
| `fc-viewer/src/fc_osrs_text.c:157` | `fc_osrs_text_ready()` | Declaration and definition only. Initialization success is checked directly. |
| `fc-viewer/src/ui_interfaces.c:947` | `runec_ui_interfaces_draw_group()` | Unused convenience wrapper; the live path calls `runec_ui_interfaces_draw_group_ex()`. |
| `fc-viewer/src/ui_interfaces.c:1100` | `runec_ui_interfaces_hit_test()` | Unused convenience wrapper; the live path calls `runec_ui_interfaces_hit_test_ex()`. |

Confirmed unused data and dead stores:

| Location | Finding |
|---|---|
| `fc-viewer/src/ui.c:92` | `g_tab_icon` is never read. |
| `fc-viewer/src/ui.c:213` | `g_skill_names` is never read. The decoded UI uses `RUNEC_OSRS_SKILLS`. |
| `fc-viewer/src/ui.c:220` | `g_skill_icon_index` is never read. |
| `fc-viewer/src/viewer.c:448` | `ViewerState.debug_spawn` is never read or written after zero-initialization. Direct F-key debug spawning uses local actions and does not use this field. |
| `fc-viewer/src/fc_debug_overlay.h:617` | `content_w` is calculated and never used. |
| `fc-viewer/src/viewer.c:1927` | The player-side assignment `size = 1` is overwritten/not read; only the NPC branch uses `size` to compute height. |
| `fc-viewer/src/viewer.c:3299` | Initializing `icon_txt` to `"?"` is dead because every following branch assigns `M`, `R`, or `W` before its first read. |
| `fc-viewer/src/viewer.c:4737` | Initializing `pose_seq` before the exhaustive switch is dead because every switch path, including `default`, assigns it. |
| `fc-viewer/src/fc_debug_overlay.h:356` | `reward_params` is an unused parameter of the internal debug-panel helper and can be removed from that helper and its call sites. |

### Unreachable viewer random-action mode

`ViewerState.auto_mode` at `fc-viewer/src/viewer.c:362` is initialized to zero,
is explicitly reset to zero in policy-pipe mode, and is never assigned one.
Nevertheless, it still controls input gating at line 4243 and retains a random
action branch at line 4258.

The comment at lines 4169-4170 says to use `--auto`, but the command-line parser
does not implement `--auto`. Therefore this is not merely an untested optional
mode: the enabling path no longer exists. Remove the field, conditions, random
branch, and stale comment unless random mode is deliberately restored with an
explicit CLI contract and test.

### Dormant viewer UI API surface

The following exported functions are declared in `ui.h` but have no production
or test caller. Linker garbage collection removes each from `fc_viewer`:

- `runec_ui_open_context()`
- `runec_ui_clear_component_overrides()`
- `runec_ui_set_component_model()`
- `runec_ui_set_component_animation()`
- `runec_ui_set_component_color()`
- `runec_ui_set_component_scroll()`
- `runec_ui_open_interface()`
- `runec_ui_open_overlay()`
- `runec_ui_open_modal()`
- `runec_ui_move_interface()`
- `runec_ui_close_interface()`

Some closely related lower-level functions are live, so this is not a reason to
remove the decoded-interface system. These are unused wrappers/override
operations around that live system. If `ui.h` is not intended as an external
library contract, remove them and their declarations. If it is intended as a
future public contract, explicitly document that decision so subsequent dead
code audits do not repeatedly rediscover them.

### UI self-test that is not a test

`runec_ui_runtime_selftest()` at `fc-viewer/src/ui.c:2006` and its private helper
subgraph at lines 1948-2004 are never invoked by the viewer or CTest. Linker
garbage collection removes the whole subgraph. The function checks useful UI
assumptions, but currently provides no protection.

Preferred disposition: move it into a real viewer test and register/invoke it.
If maintaining those checks is not valuable, remove it and its private helpers.
Leaving an uninvoked self-test in production source gives a false impression of
coverage.

### Commented-out viewer tests

`fc-viewer/src/viewer.c:2379-2419` retains old movement, combat, prayer,
consumable, and combined `AgentTest` entries inside comments marked `PASSED`.
The four active overlay tests below that block still run through the `T` key and
are not dead. The old commented cases should be deleted or converted into real
automated tests; keeping them as commented source does not validate behavior.

## Small redundant branches worth simplifying

These are not meaningful subsystems, but they add noise and were independently
identified by compiler/analyzer data flow:

- `fc-core/src/fc_state.c:357`: `slot >= 0` is redundant after action zero and
  out-of-range actions have already returned.
- `fc-core/src/fc_tick.c:420-421`: `target_x >= 0` and `target_y >= 0` are
  redundant because both action values were required to be greater than zero
  before subtracting one. The upper-bound checks remain necessary.
- `fc-viewer/src/fc_animated_atlas.c:181`: `center_h <= 0` cannot be true. A
  zero-height row is skipped, and excessive padding is reset to zero first.
- Several checks compare unsigned Raylib IDs or unsigned animation frame counts
  with `<= 0`. They are functionally valid zero checks, but spelling them `== 0`
  removes the impossible negative half and eliminates static-analysis noise.

The compile-time `FC_ACTIVE_LOADOUT` validation in `fc_state.c:189` was reported
unreachable for the current valid constant. It is intentionally retained as a
fail-fast guard for builds that override the macro and is not dead code.

## Findings rejected as false positives or non-dead code

- Clang Static Analyzer reported possible division by zero in
  `fc_pathfinding.c:386`. The branch requires `dx_abs > dy_abs`; after the
  same-tile early return this implies `dx_abs > 0`. The division is safe.
- It reported a possible null dereference in `ui_interfaces.c:113`.
  `free_component()` is private and is called only with addresses of elements
  inside a valid component array. No null call path exists.
- Cppcheck reported `clicked` as always false in
  `process_runec_console_input()`. `clicked` comes from Raylib's
  `IsMouseButtonPressed()`; the analyzer does not model external frame input.
- Header-defined animation functions such as `anim_cache_load()`,
  `anim_get_sequence()`, and `anim_update_mesh()` were reported unused in small
  test translation units that include `fc_anim_loader.h`. They are called by
  the production viewer and are not dead. Moving large header implementations
  into normal `.c/.h` modules would prevent duplicate compilation and these
  false positives, but that is a structural refactor rather than deletion.
- Low coverage in debug tabs, optional UI screens, loader error handling, and
  policy-pipe-only rendering does not establish deadness. Static references and
  production linking show those paths remain reachable.
- Cppcheck's duplicate width/length expressions in NPC calculations reflect the
  intentionally square `N x N` actor footprint model; they are not duplicate
  behavior to delete.

## Compiler/analyzer hygiene recommendation

The repository already has opt-in CMake AddressSanitizer and
UndefinedBehaviorSanitizer support. Adding another sanitizer mechanism would be
redundant. The useful additions would be:

1. Add a separate strict-warning CI/audit mode rather than immediately making
   every warning fatal. Start with:

   ```text
   -Wall -Wextra -Wpedantic -Wshadow -Wunreachable-code
   -Wunused-function -Wunused-variable -Wunused-const-variable
   -Wformat=2
   ```

2. After the current conversion warnings are triaged, introduce `-Wconversion`
   and `-Wsign-conversion` selectively. Enabling them as errors now would bury
   useful dead-code signals in many benign rendering conversions.
3. Use function/data sections and linker garbage-collection reporting in an
   audit build. This uniquely caught unused exported APIs that
   `-Wunused-function` cannot report.
4. Run Cppcheck against first-party code while explicitly excluding vendored
   `fc-viewer/raylib`; third-party Raylib diagnostics dominated unfiltered
   output and are not actionable here.
5. Reconsider the blanket `-Wno-unused-but-set-variable` in
   `fc-training/build.sh`. If NVCC needs it, keep it only on that compilation
   path and retain an unsuppressed CPU/Clang audit build.

## Suggested cleanup order

1. Remove the two unused core wrappers and the high-confidence private viewer
   functions/tables/dead stores.
2. Remove the unreachable `auto_mode` branch and correct the stale CLI comment.
3. Decide whether the UI self-test should become a real CTest test or be
   deleted.
4. Remove the dormant UI wrappers if `ui.h` is not intended as a future external
   API.
5. Delete or automate the commented-out `AgentTest` cases.
6. Add the focused warning/link-reachability audit mode and rerun the same
   sanitizer, coverage, viewer smoke, and Valgrind checks after cleanup.
