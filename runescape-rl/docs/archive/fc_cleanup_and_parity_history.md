# Fight Caves Parity, Refactor, and Code-Cleanup History

Status: historical and complete through 2026-08-26

This document consolidates the completed work previously recorded in the root
`osrs_parity.md`, `refactor.md`, and `code_analysis.md` files. It is evidence of
what was audited, why changes were made, what logic was removed or
consolidated, and how behavior was validated. It is not an active work list.
All remaining or deferred work is tracked only in the root `TODO.md`.

## OSRS movement, routing, collision, and LOS parity

The parity audit began from commit `b4d05f1ff` on 2026-08-21. It compared the
Fight Caves core and viewer against the local RSMod, LostCity, Void, Client3,
and 2006Scape references. RSMod and LostCity were the primary routing,
collision, reach, and LOS references; Client3 and 2006Scape were the primary
presentation references.

Commit `779648780` implemented the approved findings. This deliberately
changed deterministic simulation behavior and raised the state-hash contract
from version 2 to version 3. Observation, action, and reward layouts did not
change.

### Backend corrections

- Melee contact changed from permissive perimeter adjacency to
  rectangular-exclusive cardinal reach. Diagonal corner contact and actor
  footprint overlap no longer count as ordinary melee arrival.
- Large-actor LOS changed from testing every perimeter-point pair until any ray
  succeeded to the native closest-point, fixed-point ray calculation.
- NPC occupancy changed from whole-tick start-position reservations to
  sequential current-position occupancy, allowing a later actor to enter a
  tile vacated earlier in the same tick.
- Size-aware movement adopted the native leading-edge masks for large diagonal
  steps. An exhaustive comparison across the Fight Caves map, all eight
  directions, and sizes one through five found no differences from the RSMod
  masks after implementation.
- Player combat approach routes began tracking the target's current footprint
  and recalculating when the target moved.
- Playable click routes gained move-near behavior for blocked or unreachable
  requested tiles.
- Player routing replaced the greedy-first route shape with native BFS
  expansion and turning-point compression.
- Playable movement and interaction clicks began entering through the core
  action lifecycle instead of directly mutating route and target fields from
  the viewer.

### Viewer corrections

- Terrain, overhead icons, and projectile endpoints began sampling height at
  the centered actor or target position instead of the southwest footprint
  anchor.
- Visual actor movement adopted the native queue bound and the two-tile
  resynchronization snap, preventing long presentation backlogs during rapid
  policy replay.
- Untargeted actors now use the native half-speed turning rule while aligning
  to a new movement direction.
- Yt-HurKot healers visually face their authoritative heal target rather than
  being forced to face the player.

### Confirmed behavior retained

- Separate movement, collision-wall, and projectile/LOS maps remain
  authoritative and required; missing data fails closed.
- Size-one cardinal and diagonal movement already matched the reference.
- NPC pursuit remains footprint-aware and attempts validated diagonal,
  horizontal, then vertical steps.
- Players do not treat NPC occupancy as a movement blocker; ordinary NPC
  movement accounts for player and NPC occupancy.
- Distance to an NPC remains nearest-footprint Chebyshev distance.
- The viewer retains the native 128 local units per tile, 4/6/8 client-step
  movement speeds, doubled running speed, per-axis interpolation, directional
  walk sequences, gradual turning, and combat target-facing.
- Compatible locomotion and upper-body action sequences may mix. An attack
  animation does not inherently require authoritative movement to stop.

### Encounter-wide audit results

The later encounter audit verified all 63 wave compositions across all 15
rotations, baseline NPC hitpoints, combat levels, max hits, footprints, attack
speeds and ranges, Tz-Kih Prayer drain, Tz-Kek splitting, Jad's styles and
eight-tick attack speed, Yt-HurKot spawning and healing basics, Jad completion,
and one-hitpoint-per-minute player regeneration.

The audit also found additional mechanics that were not part of the movement
patch. They were intentionally left unimplemented pending review. Their active
status and priority now live only in `TODO.md`.

## Behavior-preserving refactor program

The refactor program ran during 2026-08-24 through 2026-08-26. Its goal was to
reduce duplicate ownership and dead paths while preserving simulation,
training, and presentation behavior. Every core or training change used
focused tests and deterministic comparisons. The dedicated 100M W&B regression
series is recorded in the root `baseline.md`; each completed refactor matched
baseline `m916qfsv` on all behavioral and learning metrics apart from expected
performance telemetry.

### Training cleanup

- Commit `0be25564a` removed the unused 570-line training renderer, its dead
  `--render` mode, and training-side Raylib download/link dependencies. The
  required Puffer `c_render()` and `c_close()` lifecycle hooks remain. Net
  reduction: 611 LOC.
- Commit `665385ddc` replaced individually mirrored reward fields with one
  stored `FcRewardParams`, removed the per-tick reconstruction, and retained
  the same config keys and reward semantics. Net reduction: 97 LOC.
- Commit `478caefdd` stopped including core implementation `.c` files through
  `fight_caves.h`. CMake and the local, CPU, and CUDA training builds now use
  the canonical `core_sources.txt`, compile `libfc_core.a` once, and link it
  normally.

### Core consolidation

- Commit `b5758392a` consolidated the collision, movement, and LOS loaders into
  one required-map loader while keeping three distinct maps, environment
  overrides, caches, and fail-closed diagnostics. Net reduction: 58 LOC.
- Commit `4618a0ea0` removed seven unused public helpers and two test-only
  movement/pathfinding APIs, along with seven approved stale tests. Live
  combat, movement, action, and terminal paths were unchanged. Net reduction:
  297 LOC.
- Commit `d7aa69094` centralized expanding-ring footprint searches and
  lowest-free NPC-slot allocation used by waves, Tz-Kek splits, and Jad
  healers. Callers retained their distinct fallback radii and bookkeeping.
- Commit `ea5cfe9e0` separated NPC style selection from a shared attack-launch
  mutation covering rolls, RNG damage, pending hits, render events, and
  cooldown reset.
- Commit `dc6234297` made the existing rectangle-distance primitive the common
  distance source for attacks and healer calculations.
- Commit `1b180a591` consolidated shared food, potion-legality, NPC-healing,
  and facing mutations while retaining their distinct inputs and analytics.
- Commit `db4bcfd94` split the two largest transition functions into explicit
  ordered phases. The player transition became a short coordinator for Prayer,
  supplies, interaction, attack, movement, and accounting; NPC pending-hit
  resolution delegated death, Jad completion, and Tz-Kek splitting to focused
  handlers. This increased production LOC slightly in exchange for explicit
  ownership and phase ordering.
- Commit `109a22c69` moved reward implementation into `fc_reward.c` and the
  immutable loadout table into `fc_loadouts.c`. Their headers now expose data
  types and supported declarations instead of compiling implementations and
  data copies into consumers.
- Commit `965c05321` made wave-table lookups, spawn-direction lookup, and the
  low-level single-ray LOS helper private. Consumers retain the supported
  footprint-aware public operations.
- Commit `3dd2f9958` removed unused hashed state copies and the stale NPC
  `max_hit_tenths` compatibility field. This was an explicit diagnostic
  state-hash migration from version 3 to version 4; gameplay, RNG,
  observations, rewards, and action behavior remained unchanged.
- Commit `7a8e87fb2` removed two additional caller-free public pathfinding
  wrappers found by the later static/linker audit.

### Viewer consolidation and presentation ownership

- Commit `bf627a21f` removed the unreachable legacy side panel, click handler,
  state, textures, and five obsolete sprites. The active RuneC console and
  Prayer interface remained. Net reduction: 836 LOC.
- Commit `b5758392a` replaced duplicate asset/image/font loaders with one
  `fc_asset_raylib` module, introduced a shared `FcAnimatedAtlas` component,
  and converted asset-root resolution into a single normal module. Net
  reduction across the asset and atlas work at that point: 219 LOC.
- Commit `660a12145` extended presentation-only render events so the viewer
  consumes authoritative NPC launch and hit-resolution events. It removed
  pending-hit queue snapshots, queue diffs, cooldown-reset inference, Jad
  special cases, and other reconstructed combat timing.
- Commit `1d7299e62` introduced one animation mixing path for pose-only,
  full-action, and interleaved pose/action composition while retaining distinct
  player/NPC track selection and mesh ownership.
- Commit `8bce65f69` introduced a read-only `FcEpisodeSummary` so training and
  policy replay derive and name shared episode metrics from one source.
- Commit `cae3a48ef` removed the unused debug/action-trace module, unreachable
  entity and observation panels, dead overlay modes, and orphaned helpers. The
  active Player, Obs, Mask, Reward, Log, collision, LOS, path, range, and
  Prayer-window diagnostics remain. Net reduction: 512 LOC.

## Static and dynamic dead-code audit

The deep audit at commit `cae3a48ef` used strict Clang warnings, Cppcheck,
Clang Static Analyzer, linker section garbage-collection reports, symbol and
reference inventories, ASan, UBSan, coverage, a real Raylib smoke run, a
100-episode randomized training run, and Valgrind.

The initial dynamic results were:

| Check | Historical result |
| --- | --- |
| CTest under ASan and UBSan | 164/164 passed |
| Randomized training | 100 episodes; no sanitizer diagnostic |
| Raylib viewer smoke | initialized, rendered, and shut down cleanly |
| Valgrind core and training checks | zero errors and zero bytes live at exit |
| Core line coverage | 97.1% |
| Training-adapter line coverage | 91.5% |
| Viewer line coverage | 38.7%; low coverage alone was not treated as deadness |

The audit found no orphan production source file, no sanitizer or Valgrind
failure, and no confirmed dead training implementation. It did identify
caller-free core wrappers and a large viewer cleanup set.

### Cleanup performed from the audit

- Commit `5f2e2da83` removed unused viewer functions and their declarations.
- Commit `950d0af5f` removed unused viewer state, static tables, dead stores,
  an unused parameter, and the unreachable random-action mode.
- Commit `ca77977a` removed dormant component overrides, duplicated event-mask
  caches, unused decoded metadata, interface state, status synchronization, and
  dead chat state.
- Commit `adc821530` removed the unused decoded-interface subsystem, its binary
  asset/export path, generic interface management, component override
  synchronization, decoded drawing/hit testing, listener dispatch, and modal
  or side-overlay branches. The active non-decoded RuneC UI remained.
- Commit `2fbb18f3d` removed the complete AgentTest scripted-action harness.
  Production no longer injects scripted player actions.
- Commit `078c6fbc9` removed only the 560 verified duplicate numeric sprite
  files while retaining unique UI sprites.
- Commit `f79df58bf` removed the obsolete `fc_player.anims` asset, duplicate
  Prayer fallback sprites and branches, unused bold font, unused intent enum,
  and analyzer-confirmed redundant conditions.
- Commit `75ed67a88` made model and object consumers share the same loaded
  texture atlas resource and allocation while keeping distinct model/object
  parsing and material behavior.

All of these deletion phases passed the repository test suite. The applicable
100M regression comparisons in `baseline.md` remained exactly aligned with
`m916qfsv` on behavioral and learning values.

### Findings deliberately retained or rejected

- The legacy `MINA`/animation-v1 reader remains intentionally supported for
  possible future asset files and export pipelines, even though current assets
  and exporters use `ANM2`.
- The large wave-rotation table and loadout table are authoritative data, not
  accidental duplication.
- `fc_actor_visual.c` is the active client-style movement implementation.
- Puffer's no-op `c_render()` and `c_close()` hooks are required adapter
  lifecycle entries.
- Raylib input values confused Cppcheck in a few places; those paths were live.
- Header-defined animation helpers reported unused in small test translation
  units were called by the production viewer.
- Square width/length expressions are intentional consequences of the `N x N`
  NPC footprint model.
- The active-loadout bounds guard is intentionally fail-closed for compile-time
  overrides.

## Historical validation discipline

Behavior-preserving core and training refactors were checked with focused
guardrails, the complete CTest suite, normal local/CPU/CUDA builds where
applicable, deterministic linked-versus-Puffer replay, and a fixed 100M
training comparison. Visual changes were also validated in the playable
viewer. The exact run identities, metrics, and comparisons remain in
`baseline.md` and `runescape-rl/docs/run_history.md`.

The strict compiler, reachability, analyzer, sanitizer, and Valgrind setup was
used as an explicit audit—not as a requirement for every implementation. Any
future on-demand static-analysis test and all other remaining cleanup,
documentation, parity, training, or release work is tracked in `TODO.md`.
