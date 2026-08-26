# Fight Caves Refactor Backlog

This document records verified cleanup and maintainability opportunities in
`fc-training`, `fc-core`, and `fc-viewer`. The goal is to simplify the code,
reduce duplication and source size, and preserve existing gameplay and viewer
behavior. LOC estimates are approximate and should be confirmed when each
change is implemented.

## Highest-confidence cleanup

### 1. Remove the unused training renderer — completed

Completed reduction: 611 net LOC.

- Removed the 570-line `runescape-rl/fc-training/fc_render.h`; its
  `fcr_render_frame()` entry point had no callers.
- Removed the nonfunctional `--render` build mode and every training-side
  Raylib include, download, compile definition, and link dependency.
- Removed the same unnecessary Raylib dependency from the training-health
  probe build.
- Preserved Puffer's required no-op `c_render()` and `c_close()` hooks. The
  external `fc-viewer` and its intentional Raylib dependency are unchanged.
- Verified standalone, optimized standalone, CPU/Puffer, and CUDA/Puffer
  builds; all 169 tests pass.
- The post-refactor 100M regression run `lfdszoiz` exactly matched all 124
  recorded environment metrics and all learning/loss values from pre-refactor
  baseline `m916qfsv`. Only wall-clock, throughput, and utilization telemetry
  differed; see `baseline.md`.

### 2. Remove the viewer's unreachable old side panel — completed

Completed reduction: 836 net LOC, plus five obsolete sprite assets.

- Removed the unreachable renderer, click handler, controls, state, textures,
  asset-generation paths, and asset validation requirements.
- Removed the stale fixed-width click exclusion so uncaptured clicks anywhere
  in the resizable window can reach the world.
- Preserved the active RuneC UI and prayer interface; the latter is now named
  `draw_runec_prayer_tab()` to reflect that it is live code.

### 3. Remove two dead viewer debugging systems

Estimated reduction: approximately 475 LOC.

- `runescape-rl/fc-viewer/src/fc_debug.c` and `fc_debug.h` are compiled but
  have no runtime callers. Their only outside dependency is a validation test
  that inspects the files' text.
- `dbg_draw_entity_info()` and `dbg_draw_obs()` in
  `runescape-rl/fc-viewer/src/fc_debug_overlay.h` are also unreachable. The
  active tabbed debug panel supersedes their information.
- Update or remove the static validation assertions that preserve the unused
  action-trace subsystem.

### 4. Consolidate duplicated asset and atlas loaders — completed

Completed on 2026-08-24. Net reduction: 219 LOC.

- Replaced `asset_raylib.h` and the implementation in `fc_asset_raylib.h`
  with one normal `fc_asset_raylib.c`/`.h` module for texture, image, and font
  decoding.
- Added one `FcAnimatedAtlas` component that owns atlas loading, `.tanim`
  parsing, pixel updates, and destruction for both model and object consumers.
- Converted `fc_assets.h` from a header implementation into a normal
  `fc_assets.c`/`.h` module, giving the viewer one asset/repository resolver
  state instead of one private copy per translation unit.
- Removed the unused asset/repository root setter APIs.
- Preserved distinct model parsing, object parsing, and material assignment;
  only the byte-for-byte duplicated resource mechanics were consolidated.
- Verified a real viewer startup, all required asset resolution from an
  unrelated working directory, warning-clean compilation of the new modules,
  and the complete 169-test suite.
- The 100M regression run `7vgbe9bb` exactly matched the baseline on all 124
  environment metrics and all recorded learning metrics.

### 5. Consolidate the three core arena-data loaders — completed

Completed on 2026-08-24. Net reduction: 58 LOC.

- Replaced `load_collision_once()`, `load_movement_once()`, and
  `load_los_once()` with one private `load_required_arena_map()` implementation.
- Kept collision, movement, and LOS in three distinct cache instances with
  their original filenames and environment overrides.
- Replaced three synchronized path lists with one shared path-format table.
- Preserved override precedence, fallback search order, exact read size,
  `[y][x]` to `[x][y]` transposition, one-time caching, initialization order,
  and fail-closed diagnostics.
- No public API, state, hash, observation, action, reward, or configuration
  contract changed.
- Verified the missing-asset guardrail for each file, state-hash golden,
  linked and Puffer replay parity, all 80 guardrails, and all 169 tests.
- The 100M regression run `dgykda7r` exactly matched the baseline on all 124
  environment metrics and all recorded learning metrics.

### 6. Remove obsolete core APIs — completed

Completed on 2026-08-24. Net reduction: 297 LOC.

- Removed seven public helpers with zero callers:
  `fc_npc_melee_max_hit`, `fc_melee_hit_delay`, `fc_magic_hit_delay`,
  `fc_move_toward_dynamic`, `fc_npc_step_toward`,
  `fc_action_attempt_is_invalid`, and `fc_terminal_code`.
- Removed the test-only `fc_npc_step_toward_sized` and
  `fc_pathfind_bfs_sized_dynamic` APIs. The latter's private BFS implementation
  and two tests existed solely to preserve functionality that was never
  integrated into gameplay.
- Retargeted the useful static wall and corner-clipping checks onto the live
  `fc_npc_step_toward_sized_dynamic` implementation with an empty occupancy
  map.
- Preserved all live combat, hit-delay, action-validation, terminal, movement,
  footprint, route, and collision implementations. No viewer or gameplay call
  site changed.
- Removed seven stale tests: the two dynamic-BFS cases, the orphan action-trace
  provenance case, two historical config-preservation cases, the completed
  sweep-specific case, and the obsolete hash-ownership source guardrail.
- Verified a full rebuild and all 162 remaining tests. The suite has seven fewer
  tests after the approved stale and historical-guardrail cleanup.
- The 100M regression run `4qmy1v9y` exactly matched the baseline on all 124
  environment metrics and all recorded learning metrics.

### 7. Store reward configuration once in training — completed

Completed on 2026-08-24. Net reduction: 97 LOC.

- Replaced the individual reward and shaping members in `FightCaves` with one
  `FcRewardParams reward_params` member.
- Removed `fc_reward_params_from_env()` and its complete per-tick field copy;
  reward calculation now consumes the stored parameters directly.
- Initialized the defaults once in both Puffer and standalone environments.
  The Puffer binding applies the same named float and integer config overrides
  directly to the stored structure.
- Preserved all config keys, default values, reward semantics, clipping,
  observations, actions, metrics, and contract versions.
- Verified the standalone and CPU Puffer builds, deterministic linked/Puffer
  replay parity, and all 162 tests.
- The 100M regression run `e6o99d6d` exactly matched the baseline on all 124
  environment metrics and all recorded learning metrics.

## Larger maintainability refactors

### Viewer event reconstruction — completed

Completed on 2026-08-24.

- Extended the presentation-only `FcRenderEvents` contract with bounded NPC
  launch and hit-resolution event lists.
- NPC launch events preserve the authoritative source NPC and tile, target
  tile, selected style, hit delay, delayed Prayer-lock tick, and whether the
  pending hit was successfully queued.
- Hit events report player and NPC impacts at resolution time, including
  misses and Prayer-blocked hits whose final damage is zero.
- Removed the viewer's pending-hit snapshots, multiset queue matching,
  queue-shrink impact detection, cooldown-reset launch fallback, Jad-melee
  special case, and attack-timer animation fallback.
- The debug Prayer-window indicator now follows the lock boundary carried by
  the launch event instead of inspecting pending-hit queues.
- The viewer now consumes core events read-only for player launches, NPC
  launches, movement, Prayer transitions, and hit resolutions. Combat state,
  timing, observations, actions, rewards, and policy contracts are unchanged.
- Added deterministic coverage for player impacts, delayed NPC projectiles,
  blocked zero-damage resolutions, same-tick melee launches and impacts, Jad's
  delayed Prayer lock, and next-tick event clearing. All 162 tests pass.

### Animation mixing — completed

Implemented on 2026-08-24. Production reduction: 7 net LOC; 72 lines of
duplicated orchestration were removed from `viewer.c`.

- Added one `anim_mix_pose_action()` runtime path for pose-only, full-action,
  and OSRS interleaved pose/action composition.
- Player and NPC sequence selection, track timing, one-shot behavior,
  locomotion phase preservation, movement locks, and animation state ownership
  remain separate because those behaviors legitimately differ.
- Mesh upload remains caller-owned: the player uploads its unique mesh after
  mixing, while same-type NPCs retain their required per-actor upload at draw
  time.
- Added a focused synthetic test covering pose-only application, full-action
  override, interleaved slot ownership, and missing-action fallback.
- The complete automated suite passes all 163 tests. Manual viewer validation
  confirmed player/NPC animation mixing and the viewer now releases lethal
  projectile damage, hitsplats, health-bar changes, and NPC death animation
  together at visual impact without altering authoritative combat timing.

### Episode metrics — completed

Implemented on 2026-08-24.

- Added a read-only `FcEpisodeSummary` core API that derives the shared scalar,
  per-NPC, action, targeting, Prayer, damage, wave, and terminal metrics once.
- Training aggregates that summary into Puffer's `Log`; adapter-owned reward
  and no-progress diagnostics remain in the training adapter.
- Policy replay serializes the same summary, and training/evaluation share one
  stable NPC metric-name mapping.
- The summary accepts the consumer's episode length so adapter step counts and
  standalone tools retain their existing semantics.
- A focused test pins every summary field, zero-denominator behavior, stable
  metric names, and read-only state access. The complete suite passes all 164
  tests.
- The 100M W&B regression `r9fnqnxh` exactly matches baseline `m916qfsv` on all
  124 `env/*` values and all policy/value/entropy/KL/loss values.

### Training build structure

Implemented on 2026-08-24.

- Added one canonical `fc-core/core_sources.txt` manifest consumed by both
  CMake and the production training build.
- `fight_caves.h` now includes only public core interfaces. It no longer
  compiles gameplay implementation files into every adapter translation unit.
- Standalone, CPU, and CUDA training builds compile `libfc_core.a` once and
  link it after the small Puffer adapter archive.
- Training validation links the normal `fc_core` CMake target. The obsolete
  validation-only direct-included duplicate core and its now-trivial parity
  comparison were removed; same-version deterministic replay remains.
- Contract source hashing now covers the canonical source manifest, and the
  CUDA health probe links both adapter and core archives.
- Added an architecture guardrail that rejects implementation includes,
  missing manifest entries, and training builds that stop linking the core
  archive. Standalone, CPU, and CUDA production builds pass, including verified
  `sm_120` CUDA device code, and the complete suite passes all 164 tests.
- The 100M W&B regression `1axxklis` exactly matches baseline `m916qfsv` on all
  124 `env/*` values and every non-performance summary value.

## Additional `fc-core` opportunities

The following items came from a second pass over all `fc-core` headers and
implementations. They are separate from the arena-loader and obsolete-API
items already listed above.

### Consolidate spawn-position search and NPC-slot allocation

Implemented.

- Added one private spawn module that owns the expanding-ring footprint search
  and lowest-free-slot allocation used by wave spawns, Tz-Kek splits, and Jad
  healers.
- The search accepts an explicit radius and reports failure, while callers keep
  their distinct policies: wave spawning retains its radius-five/original-tile
  fallback, and split/healer spawning retain the full-arena search.
- The helper preserves the prior ring traversal and first-free-slot ordering,
  builds occupancy once per search, and leaves wave counters, split counting,
  and healer-generation state in their original callers.
- Added a guardrail test pinning blocked-tile relocation, lowest-slot reuse,
  spawn-index assignment, and NPC-count changes. Existing split and healer
  tests continue to pin their exact behavior.
- The deterministic 628-line trace is byte-for-byte identical before and after
  the refactor, all 165 tests pass, and local, CPU, and CUDA production builds
  pass.
- The 100M W&B regression `og4lc7gn` exactly matches baseline `m916qfsv` and
  preceding run `1axxklis` on all 124 `env/*` values and every non-performance
  summary value.

### Separate NPC style selection from common attack launch

Implemented.

- `jad_attack()` and `npc_generic_attack()` retain their distinct style,
  range/LOS, hit-delay, prayer-drain, and prayer-lock decisions.
- One private `launch_npc_attack()` helper now owns their duplicated attack and
  defence rolls, hit/damage RNG, pending-hit initialization, render event, and
  cooldown reset. Jad-specific behavior is passed explicitly rather than
  inferred from NPC type.
- Production code decreased by 9 LOC with no new public API or test code.
- Existing generic/Jad RNG, damage, prayer-lock, and call-site tests pass; the
  full 165-test suite passes; and the 628-line deterministic trace is exactly
  unchanged.
- The 100M W&B regression `y4vimecf` exactly matches baseline `m916qfsv` and
  preceding run `og4lc7gn` on all 124 `env/*` values and every
  non-performance summary value.

### Centralize core distance primitives

Implemented using the existing `fc_distance_between_areas()` primitive.

- Attack-route endpoint checks already used the canonical rectangle distance,
  so no change was needed there and no new distance API was introduced.
- `fc_distance_to_npc()` is now a thin player-to-footprint wrapper around the
  same rectangle primitive.
- Candidate NPC attack-position checks calculate directly from the candidate
  coordinates instead of copying an entire `FcNpc` merely to change `x/y`.
- Healer anchor distance retains its intentional top-left-anchor semantics but
  delegates the arithmetic to the canonical primitive using two 1x1 areas.
- Production code decreased by 8 LOC with no new tests or public API.
- Seven focused footprint/healer tests and the full 165-test suite pass; the
  628-line deterministic trace is exactly unchanged.
- The 100M W&B regression `ef65qg3w` exactly matches baseline `m916qfsv` and
  preceding run `y4vimecf` on all 124 `env/*` values and every
  non-performance summary value.

### Consolidate repeated action and healing mutations

Implemented.

- Shark and combo eating now select their heal amount and cooldown before one
  shared HP, inventory, analytics, and event mutation.
- The action mask and executor now use the same private eating and drinking
  legality predicates. These remain internal to `fc-core` and do not expand
  its public API.
- One private NPC-heal helper now applies the bounded heal, records the actual
  amount restored, and updates common heal analytics. HurKot's Jad-specific
  proc counter remains explicit in its caller.
- Attack-facing, routed movement, and directional movement now share one
  facing-from-delta conversion with the existing coordinate convention and
  floating-point expression preserved.
- Production code decreased by 5 LOC with no new test code or public API.
- Food/mask and healer tests pass, the full 165-test suite passes, and local,
  CPU, and CUDA production builds pass. The deterministic 628-line trace is
  byte-for-byte unchanged at SHA-256
  `74ec5a04a9625d31ab95febf1b655a61631229541c2149f3a280d21bea16023c`.
- The 100M W&B regression `1pmv1yzs` exactly matches baseline `m916qfsv` and
  preceding run `ef65qg3w` on all 124 `env/*` values and every
  non-performance summary value.

### Decompose the two largest core transitions

Status: implemented. This intentionally favors explicit phase boundaries over
source-LOC reduction; production code increased by 53 LOC.

- The former approximately 425-line `process_player_actions()` transition is
  now a 43-line coordinator. Private helpers separately own action analytics,
  prayer application, supplies, interaction intent, attack launch, target
  metrics, movement, and post-action analytics.
- The coordinator preserves the original order exactly: prayer, supplies,
  interaction preparation, target/attack processing, movement, then outcome
  accounting. Pre-action NPC-slot binding and all render-event writes remain at
  their original semantic points.
- `fc_resolve_npc_pending_hits()` now owns only queue advancement, resolved-hit
  accounting, damage application, and healer-distraction tagging. Private death
  handlers own NPC death bookkeeping, Jad completion and healer despawn, and
  Tz-Kek splitting/wave accounting.
- Jad completion and normal wave advancement now use one private
  `fc_wave_record_current_duration()` helper. It is declared in an internal
  header and does not widen the public core API.
- No state layout, RNG call, action, observation, reward, render-event, or
  contract changed. Focused player/death tests pass, all 165 C tests pass, and
  local, CPU, and CUDA production builds pass. The deterministic 628-line trace
  is byte-for-byte unchanged at SHA-256
  `74ec5a04a9625d31ab95febf1b655a61631229541c2149f3a280d21bea16023c`.
- The 100M W&B regression `sq77fz7t` exactly matches baseline `m916qfsv` and
  preceding run `1pmv1yzs` on all 124 `env/*` values and every non-performance
  summary value.

### Move large header implementations and data into normal modules

Status: implemented. Production source and build code decreased by 8 LOC, but
the primary result is one compiled definition of each implementation and data
set. The two public headers decreased from 1,101 lines combined to 257.

- `fc_reward.h` now contains only reward data types, channel names, and the
  supported function declarations. The complete implementation is compiled
  once in `fc_reward.c`; clamping, wave-progress calculation, cave-progress
  calculation, and threat collection are private to that module.
- The immutable nine-entry `FC_LOADOUTS` table is now defined once in
  `fc_loadouts.c` and exposed through a read-only declaration in
  `fc_player_init.h`. Core, training, viewer, validation, and the asset pipeline
  all consume this same source of truth.
- The asset validator now links `fc_core`; this replaces the private loadout
  copy it previously received from the header. `build_fc_assets.py` now reads
  table initializers from `fc_loadouts.c` while reading public identifiers from
  `fc_player_init.h`.
- Removed the confirmed unused loadout aliases:
  `FC_PLAYER_ATTACK_LVL`, `FC_PLAYER_STRENGTH_LVL`,
  `FC_PLAYER_WEAPON_KIND`, `FC_PLAYER_CRYSTAL_PIECE_MASK`,
  `FC_EQUIP_DEF_STAB`, `FC_EQUIP_DEF_SLASH`,
  `FC_EQUIP_PRAYER_BONUS`, and `FC_PLAYER_AMMO`.
- Both new modules use the canonical `core_sources.txt`, so CMake and local,
  CPU, and CUDA training builds compile and link the same files.
- Exact loadout, combat, reward, viewer, and asset tests pass; all 165 CTest
  tests pass. The asset parser resolves all nine expected model definitions.
  The deterministic 628-line trace is byte-for-byte unchanged at SHA-256
  `74ec5a04a9625d31ab95febf1b655a61631229541c2149f3a280d21bea16023c`.
- The 100M W&B regression `nffnh657` exactly matches baseline `m916qfsv` and
  preceding run `sq77fz7t` on all 124 `env/*` values and every non-performance
  summary value.

### Narrow the public core surface

Status: implemented. Three implementation-only functions no longer appear in
public headers or the library's exported symbol table. Production code
decreased by 9 LOC.

- `fc_wave_get()` and `fc_wave_spawn_dir()` are now private to `fc_wave.c`.
  `fc_wave.h` exposes wave spawning and advancement, but not the table lookup
  used to implement spawning.
- `fc_has_line_of_sight()` is now private to `fc_pathfinding.c`. Consumers use
  the supported footprint-aware `fc_has_los_between_areas()` operation.
- The spawn allocation test now asserts the authoritative wave-1/rotation-0
  center expectation instead of deriving its expected value through the same
  private lookup used by production. The alternate-ray fixture exercises the
  public area LOS operation with 1x1 footprints.
- Other low-level math and pathfinding declarations remain public because
  focused validation tests intentionally exercise their contracts.
- All 157 CTest tests and the viewer, standalone, CPU, and CUDA builds pass. The
  deterministic 628-line trace is byte-for-byte unchanged at SHA-256
  `74ec5a04a9625d31ab95febf1b655a61631229541c2149f3a280d21bea16023c`.
- The 100M W&B regression `z8rraat7` exactly matches baseline `m916qfsv` and
  preceding run `nffnh657` on all 124 `env/*` values and every non-performance
  summary value.

### Revisit unused hashed state fields

Status: implemented as an intentional state-hash contract migration from
version 3 to version 4. Production code decreased by 25 LOC.

- Removed `pre_eat_hp` and `pre_drink_prayer` from `FcState`. Their only live
  calculation now remains local to supply handling, while the existing
  cumulative consumable analytics retain the same values.
- Removed `progress_delta_this_tick`; the authoritative value remains in
  `FcRewardRuntime`, and no observation, reward, metric, or viewer consumed the
  state copy.
- Removed `safespot_attack_this_tick` and its per-attack scan over every NPC.
  No reward or observation consumed the flag. The test that only assigned this
  dead field and asserted a zero reward was removed; actual LOS/safespot
  mechanics and reward-channel tests remain.
- Removed `FcNpc.max_hit_tenths`. Combat, observations, validation, replay
  diagnostics, and the viewer now use the immutable style-specific NPC stats.
  The debug overlay consequently reports the maximum for the NPC's current
  attack style rather than a spawn-time compatibility value.
- `FC_STATE_HASH_VERSION` is now 4, with an independent version-4 field oracle
  and synthetic golden hash `0x569a1fb6`. Compiled-contract fixtures,
  preflight, manifests, checkpoint validation, and evaluator expectations were
  updated together. Version-3 checkpoint sidecars intentionally do not satisfy
  the version-4 contract identity.
- The version-4 trace contains the same 628 ticks as version 3 and is
  byte-for-byte equal after normalizing the intentionally changed hash/version
  fields. RNG, transitions, observations, masks, reward features, and scalar
  rewards are unchanged. Its raw SHA-256 is
  `e81867f5b0e9dd4c60917862030bb2daf438eff1b20b540d3019b7c1af70e628`.
- All 164 repository tests and the viewer, standalone, CPU, and CUDA builds
  pass. The 100M W&B regression `3zvy6nuq` exactly matches baseline `m916qfsv`
  and preceding run `z8rraat7` on all 124 `env/*` values and every
  non-performance summary value.

## Preserve these components

- The large wave-rotation table is authoritative simulation data, not
  accidental complexity.
- The loadout table is data-heavy but legitimate.
- `fc_actor_visual.c` is the active client-style movement system and is
  necessary.
- The RuneC UI and interface parsers are large but active.
- Puffer's no-op `c_render()` and `c_close()` hooks satisfy the environment
  interface and must remain.

## Implementation guardrails

- Do not change gameplay rules, observations, action semantics, rewards,
  terminal behavior, state hashes, or checkpoint contracts as part of these
  refactors, except for the explicit versioned hash migration recorded above.
- Separate deletion-only cleanup from structural refactors so behavioral drift
  is easy to isolate.
- Run the complete core, training-contract, viewer, and replay validation suites
  after each independently reviewable change.
- Compare a deterministic seed-and-action trace before and after any core or
  adapter refactor.
