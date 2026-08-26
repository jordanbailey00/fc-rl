# OSRS Movement, Routing, Collision, and LOS Parity Audit

Audit date: 2026-08-21

Repository baseline: `b4d05f1ff` (`dev`)

Status: Implemented on `dev` after the documented review and approval.

## Implementation record

All findings B1-B7 and F1-F5 are implemented. The authoritative core now uses rectangular-exclusive melee reach, closest-point fixed-point LOS, sequential current occupancy, native size-aware leading-edge masks, live moving-target combat routes, move-near click routing, and native BFS expansion order. The viewer now samples centered terrain height, uses the native visual queue bound and resynchronization behavior, applies untargeted turn slowdown, faces healers toward authoritative heal targets, and submits movement/interaction requests through the core action lifecycle instead of writing player route or target fields.

These mechanics intentionally changed deterministic simulation behavior and
raised `FC_STATE_HASH_VERSION` to 3. The later redundant-state cleanup tracked
in `refactor.md` raised the diagnostic schema to version 4 without changing
gameplay. Existing policy observation, action, and reward layouts remain
unchanged, but pre-parity checkpoints should not be treated as a post-parity
training baseline.

Validation includes focused cases for every backend finding, the concrete Jad `(13,9) -> (14,10)` size-5 transition from this audit, visual queue/snap/turn behavior, heal-target identity, deterministic hash ownership, linked-versus-Puffer replay agreement, and the full repository test suite. An independent exhaustive check of every valid Fight Caves anchor, all eight directions, and actor sizes 1-5 found zero differences between the implemented step validator and the RSMod masks.

## Broader Fight Caves encounter audit

Audit date: 2026-08-23

The implemented findings in this document cover movement, routing, collision,
LOS, and their viewer presentation. A subsequent encounter-wide audit covered
all 63 waves, every configured NPC, combat and prayer timing, NPC special
mechanics, healing, wave transitions, and completion. It did not establish that
every Fight Caves mechanic is already exact OSRS parity.

### Verified encounter behavior

- All 63 wave compositions and all 15 rotations match the strongest local
  Fight Caves content reference. The complete comparison covered 1,008
  wave/rotation entries with zero mismatches.
- Baseline NPC hitpoints, combat levels, max hits, footprints, attack speeds,
  and attack ranges match the audited OSRS data.
- Tz-Kih drains the final damage dealt plus one Prayer point, including the
  minimum one-point drain when damage is zero.
- A large Tz-Kek splits into two small Tz-Keks.
- Jad's available attack styles and eight-tick attack speed match.
- Four Yt-HurKot healers spawn outside the northeast region and heal Jad for
  five hitpoints every four ticks when within five tiles.
- Killing Jad completes the cave without requiring the healers to be killed.
- Player natural hitpoint regeneration is one hitpoint per minute.

### Confirmed remaining differences

1. Run energy does not drain or regenerate. A player can therefore run two
   tiles per game tick indefinitely, which materially strengthens kiting.
2. Ket-Zek and Jad are missing their +60 Magic attack bonus. The shared attack
   bonus is zero for every style, so their unprotected Magic accuracy is too
   low. With the active loadout, the calculated Ket-Zek hit chance is about
   75.2% instead of 87.2%, and Jad's is about 87.3% instead of 93.5%.
3. Jad healers spawn below 150 hitpoints rather than at or below 50% of Jad's
   250 hitpoints, which is 125.
4. Landing one attack on a Jad healer permanently prevents it from healing.
   OSRS healers can lose aggression, return to Jad, and heal while close enough.
5. The next wave spawns on the same simulation tick that the final NPC is
   killed. Audited references wait for death/despawn processing and provide an
   inter-wave preparation interval.
6. Each rotation region uses one fixed exact spawn tile. The region pattern is
   correct, but the strongest content reference selects a tile within that
   region.
7. The one-point recoil for melee attacks against a large Tz-Kek is absent.
   This currently has no effect on ranged-only trained policies.

### Mechanics requiring stronger evidence

- Yt-MejKot currently heals on every ready attack cycle when a valid target is
  below half health. Local references use different heal probabilities, so the
  current frequency is probably too high but the exact modern OSRS probability
  remains unverified.
- Tok-Xil always chooses melee while adjacent. References disagree about the
  melee/ranged selection probability.
- Jad ranged/Magic prayer is locked at backend attack tick `T+2`. The visual
  description is consistent with OSRS guidance, but the exact server-cycle
  boundary could not be proven from the available sources.
- Generic NPC natural regeneration is absent. References show generic NPC
  regeneration, but no authoritative modern Fight-Caves-specific rate was
  found.

These remaining differences were audited only. They are not implemented by the
movement/LOS parity patch recorded above.

## Scope and evidence

This audit compares the Fight Caves gameplay core and external viewer with the mechanically relevant repositories under `/home/joe/projects/runescape-reference/`. The strongest references were:

- `rsmod/engine/routefinder` for collision flags, footprint step validation, reach strategies, routefinding, and line-of-sight raycasting.
- `rsmod/api/route` and `rsmod/api/game-process` for NPC local chasing and per-cycle entity collision updates.
- `lostcity-engine/src/engine/routefinder` as an independent implementation of the same routefinding, step-validation, and LOS rules.
- `void_rsps` for moving-target combat routing, overlap handling, melee arrival, and Fight Caves behavior.
- `Client3` and the `2006Scape` client for local-coordinate interpolation, movement queues, turning, terrain-height sampling, facing, and visual resynchronization.

The rest of the reference directory was surveyed for relevant implementations. Older or incomplete implementations were treated as secondary evidence rather than as the authority when they conflicted with the sources above.

Two exhaustive comparisons were run against the repository's actual 64-by-64 Fight Caves collision data:

- Size-1 movement matched RSMod for every valid actor anchor and all eight directions.
- For each actor size from 2 through 5, our validator rejected 18 transitions that RSMod allowed. It did not allow any transition that RSMod rejected.

The LOS algorithms were also compared on the actual Fight Caves LOS data at representative valid size-5 NPC anchors. Depending on the anchor, our algorithm allowed 21 to 54 additional player-to-NPC sight lines and 33 to 60 additional NPC-to-player sight lines. No sampled sight line was allowed by RSMod but rejected by our implementation.

## Confirmed parity

The following behavior is already aligned closely enough that it should not be replaced merely to make the code look more like a reference repository:

1. The arena uses separate whole-tile walkability, directional movement-wall flags, and directional projectile/LOS flags extracted from cache data. Missing required map data fails closed.
2. Size-1 cardinal and diagonal collision validation matches RSMod across the actual arena map.
3. NPC pursuit uses an actor-footprint-aware naive destination, then attempts diagonal, horizontal, and vertical validated steps in that order. This matches RSMod's local NPC stepping model.
4. Players do not use NPC occupancy as a movement blocker, while ordinary NPC movement accounts for player and NPC occupancy. This agrees with the reference entity-collision categories.
5. Distance to an NPC is measured from the nearest point of its footprint using Chebyshev distance, which matches the relevant Void combat calculations.
6. The viewer uses 128 local units per tile, a four-unit base walking speed, six/eight-unit backlog catch-up speeds, doubled running speed, per-axis interpolation, directional walk animations, gradual turning, and combat target-facing. Those fundamentals agree with Client3/2006Scape.
7. A running server tick exposes both player movement waypoints to the viewer, and the viewer marks both transitions as running. That agrees with the two-step client protocol.
8. All currently configured Fight Caves NPCs have a movement speed of one tile per game tick. Inferring the NPC's visual transition from its previous and final tile is therefore currently exact, although it would not be sufficient for a future multi-step NPC.
9. Allowing locomotion and a compatible upper-body attack sequence to play together is normal client behavior. Movement should only be blocked by sequences whose movement properties actually require it.

## Backend gameplay differences

These findings affect authoritative simulation behavior. Fixing them can change learned behavior, safespots, episode results, and checkpoint performance, so they should be introduced with regression tests and followed by retraining.

### B1. Melee reach permits diagonal contact and footprint overlap

Severity: High

Our behavior:

- `fc_npc_can_melee_player()` in `fc-core/src/fc_pathfinding.c` searches every tile in the NPC footprint.
- It accepts any neighboring tile that a size-1 actor could enter, including a diagonal neighbor.
- When the player is underneath a large NPC, a different tile within that footprint can still be adjacent to the player, so the function can report that melee contact is valid.

OSRS-reference behavior:

- RSMod's rectangular-exclusive reach strategy accepts contact along a shared north, south, east, or west edge.
- It explicitly rejects the four diagonal corners and rejects positions inside the target footprint.
- Void's combat movement treats overlap as not arrived and makes an NPC step out when standing underneath its target is not allowed.

Gameplay example:

A Yt-MejKot stands northeast of the player so that their tiles touch only at a corner. Our simulation can let the Yt-MejKot melee from that diagonal position. Under the reference rules, it has not reached the player and must take another step until one of its cardinal edges touches the player's tile.

A second example occurs when the player runs underneath Jad's size-5 footprint. Our simulation can consider Jad in melee contact while the player is underneath him. Under the reference rules, overlap is not a valid melee arrival position; the actors must separate before ordinary melee reach is satisfied.

Primary evidence:

- Ours: `fc-core/src/fc_pathfinding.c`, `fc_npc_can_melee_player()`.
- RSMod: `engine/routefinder/.../reach/RectangularBounds.kt` and `RectangularExclusiveReachStrategyTest.kt`.
- Void: `engine/.../mode/move/Movement.kt`, `stepOut()` and `arrived()`.

### B2. Large-actor line of sight is too permissive

Severity: High

Our behavior:

- `fc_has_los_between_areas()` tries every perimeter tile of the source footprint against every perimeter tile of the destination footprint.
- It grants LOS if any one of those rays succeeds.

OSRS-reference behavior:

- RSMod and LostCity choose one closest coordinate on each actor rectangle and raycast between those canonical points.
- The ray also applies the source-location blocker and special final-target-tile handling used by the client-compatible LOS algorithm.

Gameplay example:

The player stands beside a rock with most of a size-5 Ket-Zek hidden behind it, but one farther corner of Ket-Zek's footprint has a clear ray around the rock. Our simulation can permit the player to shoot because it finds that clear corner-to-corner ray. The reference algorithm uses the closest points between the player and Ket-Zek; if the rock blocks that ray, the player must move farther around the rock before attacking.

The inverse can also happen with Jad. Our simulation can let Jad attack from behind a rock because one remote edge of his footprint can see the player, while the reference closest-point ray remains blocked.

For ordinary hard cover, reference projectile LOS is symmetric: if the rock blocks the canonical ray, neither side can attack through it. A one-sided ranged safespot can still exist elsewhere when the player's weapon outranges the NPC, but that does not apply to the configured Fight Caves ranged threats: Tok-Xil, Ket-Zek, and Jad have range 14 while the active player loadout has range 10. A player hiding Jad behind a rock must step into mutual LOS to damage him, then protect against or avoid the resulting attack.

Primary evidence:

- Ours: `fc-core/src/fc_pathfinding.c`, `fc_has_los_between_areas()`.
- RSMod: `engine/routefinder/.../LineValidator.kt` and `LineRouteFinding.coordinate()`.
- LostCity: `src/engine/routefinder/LineValidator.ts`.

### B3. Start-of-tick occupancy reservations overblock vacated tiles

Severity: Medium to high

Our behavior:

- `build_movement_start_reservations()` records every actor's starting footprint.
- NPC movement checks both current occupancy and all other actors' starting footprints for the remainder of the game tick.
- A tile can therefore remain blocked after its occupant has already moved away.

OSRS-reference behavior:

- RSMod removes the moving actor's collision at its old coordinate, validates and processes its steps, and adds collision at its final coordinate.
- An actor processed later can use a tile that an earlier actor vacated, while it still cannot use the earlier actor's new occupied tile.

Gameplay example:

Two NPCs approach the player single-file through a one-tile gap. The leading NPC moves forward and vacates its previous tile. Our simulation still reserves that old tile, so the following NPC waits for the next game tick. In the reference processing order, if the follower is processed after the leader, it can immediately step into the newly vacated tile. Our NPC train therefore stretches out and can bunch or pause around narrow rocks.

Primary evidence:

- Ours: `fc-core/src/fc_tick.c`, `build_movement_start_reservations()`, and `fc-core/src/fc_npc.c`, `build_npc_movement_occupancy()`.
- RSMod: `api/game-process/.../NpcMovementProcessor.kt` and `PlayerMovementProcessor.kt`.

### B4. Diagonal movement for size-2 through size-5 actors is too restrictive

Severity: Medium

Our behavior:

- A diagonal footprint step must pass four complete cardinal side sweeps plus the leading diagonal-corner check.
- This checks more wall edges than the native size-aware rule requires.

OSRS-reference behavior:

- RSMod and LostCity use precise composite masks on the two leading edges and the leading corner, with specialized cases for sizes 1, 2, and larger actors.

Gameplay example:

One actual mismatching arena transition has Jad's southwest footprint anchor at local tile `(13,9)`, with the stationary player northeast at `(32,28)`. Jad occupies `(13..17,9..13)`, so the player is 15 tiles beyond Jad's nearest edge—one tile outside Jad's configured range of 14. Jad therefore tries to chase northeast. The reference size-5 masks allow Jad to move diagonally to anchor `(14,10)`. Our four-sweep check rejects that diagonal and also finds the east-only step blocked, so Jad moves north to `(13,10)` instead. The player's action is irrelevant to the mismatch as long as the player remains at that destination; the difference is which legal chase step Jad selects.

Measured scope:

- Size 1: zero mismatches on the Fight Caves map.
- Sizes 2, 3, 4, and 5: 18 reference-legal transitions rejected per size.
- No measured transition was incorrectly allowed by our code.

Primary evidence:

- Ours: `fc-core/src/fc_pathfinding.c`, `fc_footprint_step_walkable()`.
- RSMod: `engine/routefinder/.../StepValidator.kt`.
- LostCity: `src/engine/routefinder/StepValidator.ts`.

### B5. Player combat approach routes can follow a stale target position

Severity: Medium

Our behavior:

- When the player is out of range or lacks LOS, the core creates a route toward the NPC's center.
- It creates another route only after the current route has been exhausted.
- A moving target can therefore leave the position for which the active route was calculated.

OSRS-reference behavior:

- Void compares the live target destination with the route destination and recalculates when the target moves.
- Actor interactions route against the target rectangle/reach strategy rather than treating its center as an ordinary walkable tile.

Gameplay example:

The player clicks a Tok-Xil that is walking around a rock toward the player. Our player begins following a route to Tok-Xil's old center and can continue around the wrong side of the rock until that route ends. The reference combat route notices that Tok-Xil moved, recalculates, and approaches its current footprint instead.

Primary evidence:

- Ours: `fc-core/src/fc_tick.c`, the auto-attack approach route in `process_player_actions()`.
- Void: `engine/.../mode/move/Movement.kt`, `recalculate()`, and `CombatMovement.kt`.
- RSMod: `api/route/.../StepFactory.kt` and rectangle reach strategies.

### B6. Exact-tile routes lack native move-near behavior

Severity: Low for training; medium for the playable viewer

Our behavior:

- `fc_pathfind_bfs()` immediately fails when the requested destination tile is blocked.
- If an exact destination cannot be reached, it returns no route instead of selecting a nearby reachable endpoint.
- `route_player_to_tile()` in the viewer also rejects an unwalkable clicked tile before asking the pathfinder.

OSRS-reference behavior:

- RSMod and LostCity can search for the closest reachable approach point around an unreachable destination when `moveNear` is enabled.

Gameplay example:

The user clicks directly on a Fight Caves rock. Our playable viewer does nothing. The OSRS client/server route walks the player to the best reachable tile next to or near the clicked location.

Training impact:

The current Puffer policy uses only move, attack, and prayer. Core walk-to-tile action heads 5 and 6 are forced to zero in `fc-training/fight_caves.h`, so this exact-click discrepancy does not currently affect policy training.

Primary evidence:

- Ours: `fc-core/src/fc_pathfinding.c`, `fc_pathfind_bfs()`, and `fc-viewer/src/viewer.c`, `route_player_to_tile()`.
- RSMod: `engine/routefinder/.../RouteFinding.kt`, `findClosestApproachPoint()`.
- LostCity: `src/engine/routefinder/PathFinder.ts`.

### B7. Greedy-first player routes can use a different shortest-path shape

Severity: Low

Our behavior:

- `fc_pathfind_bfs()` first walks greedily toward the exact destination and invokes BFS only if the greedy path becomes stuck.

OSRS-reference behavior:

- RSMod and LostCity build the route with breadth-first search and then compress the result into turning-point waypoints.

Gameplay example:

When two equally short paths exist around a symmetrical rock, our greedy route may remain committed to the initially preferred side until it becomes blocked. The reference BFS can select a different side because its expansion order and reach strategy determine the route from the beginning. Both routes may be legal and equally short, but the visible path shape can differ.

Primary evidence:

- Ours: `fc-core/src/fc_pathfinding.c`, `greedy_walk()` and `fc_pathfind_bfs()`.
- RSMod: `engine/routefinder/.../RouteFinding.kt`.
- LostCity: `src/engine/routefinder/PathFinder.ts`.

## Frontend and presentation differences

These findings can be fixed without changing authoritative movement, combat, collision, or training behavior, provided the viewer continues consuming core state and render events read-only.

### F1. Terrain height is sampled at the southwest footprint anchor

Severity: High visual impact

Our behavior:

- Actor visual positions are correctly centered at `tile * 128 + size * 64`, expressed as tile-space centers in the renderer.
- Before calling `ground_y_smooth()`, the viewer subtracts half the actor size from that centered position.
- This samples the southwest footprint anchor instead of the actor's interpolated center. Projectiles and prayer icons use the same incorrect base height.

OSRS-reference behavior:

- Client3 samples terrain height using the actor's centered local `x` and `z` coordinates for players, NPCs, and projectile targets.

Gameplay example:

The player runs across a tile whose four terrain corners have different heights. The model is halfway across the tile, but our viewer still asks for height at the tile's southwest anchor. The character can appear to float, sink, or slide vertically. For a size-5 NPC such as Jad, the sampled point is several tiles away from the model center, making the mismatch more visible.

Gameplay consequence:

There is no authoritative gameplay change. Actor tiles, collision, LOS, attacks, damage, rewards, and observations are identical. This affects only where models, projectile endpoints, and overhead icons are drawn vertically, so its consequence is visual grounding rather than simulation drift.

Primary evidence:

- Ours: `fc-viewer/src/viewer.c`, `visual_actor_world_point()`, entity drawing, and prayer-icon drawing.
- Client3: `src/entry/client.c`, `pushNpcs()`, `pushPlayers()`, and `pushProjectiles()`.

### F2. The visual movement queue lacks the native two-tile resynchronization snap

Severity: High visual impact, especially during checkpoint replay

Our behavior:

- The viewer interpolates toward every queued waypoint at the normal 4/6/8-unit speeds, doubled for running.
- It resets only when one newly received server transition is more than 16 tiles.
- Its queue holds 16 destinations.

OSRS-reference behavior:

- Client3 and 2006Scape snap the local actor position to the next queued destination when either axis is more than 256 local units, or two tiles, behind.
- The native path queue has ten entries and caps active path length at nine.

Gameplay example:

A checkpoint policy changes movement every game tick while running. The renderer accumulates more positions than it can visually consume. Our player keeps smoothly traveling through stale positions, so the model visibly glides after the authoritative player has moved elsewhere. The native client bounds the queue and snaps back into synchronization once it falls more than two tiles behind.

Primary evidence:

- Ours: `fc-viewer/src/fc_actor_visual.h`, `FC_VISUAL_PATH_CAPACITY`, and `fc-viewer/src/fc_actor_visual.c`, `update_actor_movement()`.
- Client3: `src/entry/client.c`, `updateMovement()`, and `src/pathingentity.c`.
- 2006Scape: client `Game.java`, actor movement update.

### F3. Untargeted turning does not use the native half-speed rule

Severity: Medium visual impact

Our behavior:

- Visual movement begins at four local units per client tick even while an actor without a target is still rotating toward its movement direction.

OSRS-reference behavior:

- The client reduces movement to two local units per client tick while yaw differs from movement yaw and the actor has no interaction target.

Gameplay example:

The player is not attacking anything and makes a sharp turn around a rock. Our model turns while continuing at full walking speed, producing a wider or more slippery-looking corner. OSRS temporarily moves at half speed while the model aligns, which makes the turn look tighter and more grounded.

Primary evidence:

- Ours: `fc-viewer/src/fc_actor_visual.c`, `update_actor_movement()`.
- Client3: `src/entry/client.c`, `updateMovement()`.
- 2006Scape: client `Game.java`, actor movement update.

### F4. Every active NPC is visually forced to face the player

Severity: Medium for affected NPCs

Our behavior:

- `update_visual_actor_targets()` assigns the player as the visual target of every active NPC.
- It does not inspect `heal_target_idx` or another authoritative non-player target.

OSRS-reference behavior:

- Actors face the entity they are interacting with or watching.

Gameplay example:

An untagged Yt-HurKot walks toward Jad and heals him. The core correctly records Jad as `heal_target_idx`, but our viewer renders the healer looking toward the player. In OSRS, the healer faces or watches Jad while following and healing him. Once tagged and attacking the player, facing the player is correct.

Primary evidence:

- Ours: `fc-viewer/src/viewer.c`, `update_visual_actor_targets()`.
- Client3/2006Scape: pathing-entity target-facing update.

### F5. The playable viewer directly mutates gameplay routes and targets

Severity: Architectural risk; no confirmed current parity failure by itself

Our behavior:

- World/minimap interactions call the core pathfinder and directly write `FcPlayer.route_*`, attack target, approach state, and related fields.

Reference behavior:

- The client submits a movement or interaction request. The authoritative game layer owns route creation and movement state.

Gameplay example:

Today, clicking an ordinary open tile generally produces the intended movement. The risk appears when the core later adds new route cancellation, interaction, or movement restrictions: training will use the authoritative action lifecycle, while the viewer click path can bypass it and continue behaving differently. This is a parity hazard rather than proof of a current wrong step.

Primary evidence:

- Ours: `fc-viewer/src/viewer.c`, `route_player_to_tile()` and NPC-click handling.

## Findings that were considered but rejected

1. The viewer's 4/6/8 movement speeds and doubled running speed are not the source of the mismatch; they match Client3 and 2006Scape.
2. Target-facing overriding movement-facing is normal client behavior. The client selects forward, backward, or side-walk animations relative to that facing direction.
3. Playing an attack sequence while moving is not automatically wrong. Client sequence movement flags decide whether locomotion pauses or the action is mixed with locomotion.
4. Ignoring diagonal projectile-wall bits in the reduced LOS export is not itself a discrepancy. The reference raycaster uses cardinal projectile-wall flags plus the full projectile/location blocker.
5. NPC render events do not currently need multiple movement waypoints because every configured NPC moves only one tile per game tick. This must be revisited if an NPC with movement speed greater than one is introduced.

## Recommended implementation order

### Phase 1: Presentation-only corrections

These should improve the viewer without changing environment behavior or invalidating trained policies:

1. Sample terrain height at the actor/projectile center.
2. Implement the two-tile client resynchronization snap and native queue bound.
3. Apply the half-speed untargeted-turn rule.
4. Face healers toward their authoritative heal target.

### Phase 2: High-impact gameplay parity

These require explicit approval, focused regression tests, and retraining:

1. Replace melee contact with cardinal rectangular-exclusive reach and reject overlap.
2. Replace perimeter-any LOS with the canonical closest-point ray.
3. Replace whole-tick starting-position reservations with sequential current-position occupancy.

### Phase 3: Routing refinements

1. Adopt the exact size-aware leading-edge masks for large diagonal movement.
2. Recalculate combat approach routes when a target moves and route against its footprint/reach strategy.
3. Add move-near behavior for playable tile clicks.
4. Decide whether exact OSRS BFS path-shape parity is valuable enough to replace greedy-first routing.

Each backend change should be covered by small scenario tests for cardinal versus diagonal melee, overlap, closest-point LOS around a blocker, vacated-tile following, large-actor wall-corner movement, and moving-target route recalculation before a new training baseline is accepted.
