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

Expected source-LOC reduction: small or neutral; this reduces coupling,
compile work, and duplicate internal data.

- `fc_reward.h` contains approximately 400 LOC of `static inline`
  implementation in addition to its types. Move the implementation to
  `fc_reward.c`; leave only the small supported reward API and data types in
  the header. Helpers such as clamping, wave-progress calculation, and threat
  collection can become private.
- `fc_player_init.h` defines the full nine-entry `static const FC_LOADOUTS`
  table, so each separately compiled translation unit receives an internal
  copy. Move the table to a single `fc_loadouts.c` definition and expose a
  read-only declaration or accessor.
- Remove the confirmed unused loadout aliases after that move:
  `FC_PLAYER_ATTACK_LVL`, `FC_PLAYER_STRENGTH_LVL`,
  `FC_PLAYER_WEAPON_KIND`, `FC_PLAYER_CRYSTAL_PIECE_MASK`,
  `FC_EQUIP_DEF_STAB`, `FC_EQUIP_DEF_SLASH`,
  `FC_EQUIP_PRAYER_BONUS`, and `FC_PLAYER_AMMO`.
- This work should follow the normal-core-library build refactor above so a new
  `.c` file does not have to be wired into both a conventional build and the
  current training amalgamation.

### Narrow the public core surface

Estimated reduction: only a few LOC, but it makes ownership clearer.

- `fc_wave_get()` and `fc_wave_spawn_dir()` have no callers outside
  `fc_wave.c`; make them private and remove them from `fc_wave.h`.
- `fc_has_line_of_sight()` has no callers outside `fc_pathfinding.c`; expose
  only the area-to-area LOS operation that consumers actually use.
- Keep low-level math and pathfinding functions public when focused validation
  tests intentionally exercise their contracts. Do not make functions private
  solely to reduce the header count.

### Defer state-field deletion until an intentional hash-contract change

The second pass also found fields that appear to have no live consumer beyond
assignment, deterministic hashing, and tests: `pre_eat_hp`,
`pre_drink_prayer`, `safespot_attack_this_tick`, and
`progress_delta_this_tick`. `FcNpc.max_hit_tenths` is explicitly a compatibility
field rather than the live combat authority.

These are not behavior-neutral deletions under the current guardrails because
all are part of the versioned state hash. Revisit them together only during an
intentional state-hash version change, after confirming that no external replay
or diagnostic tooling consumes them. Do not mix their removal into the
behavior-preserving refactors above.

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
  refactors.
- Separate deletion-only cleanup from structural refactors so behavioral drift
  is easy to isolate.
- Run the complete core, training-contract, viewer, and replay validation suites
  after each independently reviewable change.
- Compare a deterministic seed-and-action trace before and after any core or
  adapter refactor.
