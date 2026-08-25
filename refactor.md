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

### 6. Remove obsolete core APIs

Estimated reduction: roughly 100 LOC directly, or potentially 250 LOC when
including test-only infrastructure.

Confirmed zero production callers:

- `fc_npc_melee_max_hit`
- `fc_melee_hit_delay`
- `fc_magic_hit_delay`
- `fc_move_toward_dynamic`
- `fc_npc_step_toward`
- `fc_action_attempt_is_invalid`
- `fc_terminal_code`

Additionally, `fc_npc_step_toward_sized` and
`fc_pathfind_bfs_sized_dynamic` are only exercised by tests. Live NPC movement
uses `fc_npc_step_toward_sized_dynamic`. Retain the test-only APIs only if they
represent deliberately planned public functionality.

### 7. Store reward configuration once in training

Estimated reduction: approximately 80-120 LOC.

- `FightCaves` individually mirrors every member of `FcRewardParams`.
- `fc_reward_params_from_env()` copies all of them back into an
  `FcRewardParams` every tick.
- `binding.c` and the standalone launcher separately initialize the same
  fields.
- Store `FcRewardParams reward_params` directly in `FightCaves`, initialize
  defaults once, and override configured members.

## Larger maintainability refactors

### Viewer event reconstruction

The viewer snapshots pending hits before `fc_step()` and diffs queues afterward
to infer NPC attacks, projectiles, and impacts. Extend the presentation-only
`FcRenderEvents` contract to report these authoritative events. This should
eliminate inference code and prevent visual drift without changing gameplay.
Add strong render-event tests before removing the reconstruction path.

### Animation mixing

Player and NPC pose/action/interleave application contain very similar blocks
near the end of `runescape-rl/fc-viewer/src/viewer.c`. A shared animation-track
mixer would remove duplication and make future animation fixes apply
consistently.

### Episode metrics

Training's terminal logging and the viewer's policy-episode JSON independently
derive and name many of the same metrics. A shared read-only
`FcEpisodeSummary` would prevent evaluator/training metric drift.

### Training build structure

`runescape-rl/fc-training/fight_caves.h` directly includes every core `.c`
file. Compiling and linking `fc_core` normally would make the Puffer adapter
smaller and less coupled. This is primarily an architectural improvement rather
than a major source-LOC reduction.

## Additional `fc-core` opportunities

The following items came from a second pass over all `fc-core` headers and
implementations. They are separate from the arena-loader and obsolete-API
items already listed above.

### Consolidate spawn-position search and NPC-slot allocation

Estimated reduction: approximately 50-80 LOC.

- `find_valid_split_spawn()` in `fc_npc.c` and
  `find_valid_healer_spawn()` in `fc_tick.c` are effectively identical
  expanding-ring searches.
- `find_valid_spawn()` in `fc_wave.c` is the same operation with a radius-five
  limit and a different failure policy.
- Wave spawning, Tz-Kek splitting, and Jad-healer spawning also independently
  scan for the first inactive NPC slot before calling `fc_npc_spawn()`.
- Add one nearest-available-footprint helper with an explicit maximum radius
  and boolean result, plus one helper that allocates and initializes the first
  free NPC slot. Keep wave counters, split pre-counting, healer generation
  flags, and each caller's failure behavior outside those helpers.
- Preserve the current ring traversal and first-free-slot order exactly; both
  are determinism-sensitive tie-breakers.

### Separate NPC style selection from common attack launch

Estimated reduction: approximately 25-40 LOC.

- `jad_attack()` and `npc_generic_attack()` correctly have different style
  selection rules, but duplicate the attack-level lookup, attack and defence
  rolls, hit chance, damage roll, pending-hit queueing, and cooldown reset.
- Keep style selection in the specialized functions and move only the common
  launch/queue operation into a helper.
- Make Jad's minimum projectile delay, delayed prayer-lock policy, and zero
  prayer drain explicit parameters or an explicit launch-policy value. Do not
  hide these differences behind an incidental `npc_type` check.
- Verify that the helper consumes RNG in the same order and only when the old
  paths did.

### Centralize core distance primitives

Estimated reduction: approximately 15-30 LOC.

- The attack-approach route trimming in `fc_tick.c` manually reimplements
  `fc_distance_to_npc()` for every route tile.
- `fc_npc_position_can_attack_player()` copies an entire `FcNpc` only to
  substitute candidate coordinates before asking for the same footprint
  distance.
- `npc_anchor_distance()` separately implements point-to-point Chebyshev
  distance for healer behavior.
- Introduce small, unit-explicit primitives such as
  `fc_chebyshev_distance()` and `fc_distance_to_footprint()`, then keep
  `fc_distance_to_npc()` as a thin convenience wrapper. This removes the
  handwritten variants without coupling callers to `FcNpc` layout.

### Consolidate repeated action and healing mutations

Estimated reduction: approximately 30-50 LOC.

- The shark and combo-eat branches in `process_player_actions()` duplicate HP
  bookkeeping, over-heal analytics, inventory decrement, clamping, and event
  flags. Select the heal amount and cooldown first, then apply the shared
  mutation once.
- Consumable legality is also encoded once in `fc_state.c` for masks and again
  in `fc_tick.c` for execution. Shared semantic predicates for eating and
  drinking would prevent the mask and executor from drifting apart.
- `yt_mejkot_try_heal()` and `yt_hurkot_heal_cycle()` duplicate add-and-clamp
  healing before calling `record_npc_heal()`. One helper can apply a bounded
  NPC heal and return the actual amount restored; Jad-specific proc accounting
  should remain explicit in the HurKot caller.
- Player facing uses the same angle conversion in attack-facing, routed
  movement, and directional movement. A focused facing-from-delta helper would
  keep that coordinate convention in one place.

### Decompose the two largest core transitions

Expected source-LOC reduction: small or neutral; maintainability improvement is
the primary benefit.

- `process_player_actions()` in `fc_tick.c` is approximately 425 LOC and mixes
  action analytics, prayer, supplies, target binding, approach routing, attack
  launch, movement, facing, render events, and target metrics.
- Split it into phase-named private helpers while preserving the documented
  tick order. Useful boundaries include action metrics, supplies, target and
  approach planning, attack launch, movement, and post-action metrics.
- `fc_resolve_npc_pending_hits()` in `fc_combat.c` mixes queue advancement,
  damage analytics, healer tagging, death state, Jad completion, healer
  despawn, Tz-Kek splitting, and wave accounting. Extract an NPC-death handler
  so pending-hit resolution is easier to verify independently.
- The repeated longest-wave-duration update in Jad death and normal wave
  advancement can then become one small metrics helper.

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
