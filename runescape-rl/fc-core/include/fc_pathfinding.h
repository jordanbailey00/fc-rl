#ifndef FC_PATHFINDING_H
#define FC_PATHFINDING_H

#include "fc_types.h"

/* ======================================================================== */
/* Tile queries                                                              */
/* ======================================================================== */

/* Check if a single tile is walkable (bounds + collision). */
int fc_tile_walkable(int x, int y,
                     const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Check if an entity of given size can stand at (x,y).
 * All tiles in the [x..x+size-1, y..y+size-1] footprint must be walkable. */
int fc_footprint_walkable(int x, int y, int size,
                          const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Check whether one sized movement step is legal on static terrain. Diagonal
 * movement checks the final footprint and both cardinal side footprints. */
int fc_footprint_step_walkable(
    int x, int y, int dx, int dy, int size,
    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* ======================================================================== */
/* Dynamic occupancy                                                         */
/* ======================================================================== */

/* Clear an occupancy grid to all-free. */
void fc_clear_occupancy(uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Mark all in-bounds tiles in a footprint occupied. Out-of-bounds tiles are
 * ignored here; availability checks still reject out-of-bounds footprints via
 * the static walkability check. */
void fc_mark_footprint_occupied(uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                                int x, int y, int size);

/* Build an occupancy grid from the current live entities. Pass ignore_npc_idx
 * to omit the moving NPC's own current footprint. Set ignore_player when
 * validating player movement or intentionally ignoring the player. */
void fc_build_occupancy(const FcState* state,
                        uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                        int ignore_npc_idx,
                        int ignore_player);

/* Check static terrain plus a caller-provided dynamic occupancy grid. */
int fc_footprint_available_dynamic(
    int x, int y, int size,
    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Check whether one sized movement step is available. Diagonal movement checks
 * the final footprint and both cardinal side footprints. */
int fc_footprint_step_available_dynamic(
    int x, int y, int dx, int dy, int size,
    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Convenience wrapper that builds current occupancy from the state before
 * checking the requested footprint. */
int fc_footprint_available_for_entity(const FcState* state,
                                      int x, int y, int size,
                                      int moving_npc_idx,
                                      int ignore_player);

/* ======================================================================== */
/* Movement                                                                  */
/* ======================================================================== */

/* Move a size-1 entity from (x,y) toward offset (dx,dy) for up to max_steps.
 * Diagonal-first fallback. Returns number of tiles moved. Updates *x,*y. */
int fc_move_toward(int* x, int* y, int dx, int dy, int max_steps,
                   const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                   const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Same movement operation, additionally returning each successfully consumed
 * tile in step_x/step_y up to step_capacity entries. */
int fc_move_toward_traced(
    int* x, int* y, int dx, int dy, int max_steps,
    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    int* step_x, int* step_y, int step_capacity);

/* Dynamic-aware size-1 movement. The occupied grid should already omit the
 * moving entity's own current footprint and include any start-of-tick
 * reservations that should remain blocked for this movement pass. */
int fc_move_toward_dynamic(
    int* x, int* y, int dx, int dy, int max_steps,
    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Move an NPC of given size one tile closer to (target_x, target_y).
 * Checks that the full NPC footprint fits at the destination.
 * Diagonal-first, then cardinal fallback.
 * Returns 1 if moved, 0 if blocked. */
int fc_npc_step_toward_sized(int* x, int* y, int target_x, int target_y,
                             int size,
                             const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                             const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Dynamic-aware sized step. Diagonal movement checks the final footprint and
 * both cardinal side footprints to prevent static or dynamic corner clipping. */
int fc_npc_step_toward_sized_dynamic(
    int* x, int* y, int target_x, int target_y, int size,
    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Legacy size-1 wrapper (used by existing code). */
int fc_npc_step_toward(int* x, int* y, int target_x, int target_y,
                       const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                       const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* ======================================================================== */
/* Line of sight                                                             */
/* ======================================================================== */

/* Directional projectile LOS check between two tiles. Source and destination
 * order is significant because directional collision is not assumed symmetric. */
int fc_has_line_of_sight(int x0, int y0, int x1, int y1,
                         const uint8_t los_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Footprint-aware directional LOS. Returns true when any valid pair of edge
 * tiles in the source and destination areas has projectile LOS. */
int fc_has_los_between_areas(
    int src_x, int src_y, int src_size,
    int dst_x, int dst_y, int dst_size,
    const uint8_t los_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* Returns 1 if the player is in actual melee contact with any tile of the NPC
 * footprint. Diagonal contact is only valid when the corner is open, matching
 * the same corner-cut rules used by movement/pathing. */
int fc_npc_can_melee_player(int player_x, int player_y,
                            int npc_x, int npc_y, int npc_size,
                            const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                            const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

/* ======================================================================== */
/* BFS pathfinding (for click-to-move)                                       */
/* ======================================================================== */

/* Find a path from (sx,sy) to (dx,dy) using BFS on the walkable grid.
 * Stores the path (sequence of tile coordinates) in out_x[], out_y[].
 * Returns the number of steps (0 if no path or already there).
 * max_steps limits output array size. Path does NOT include the start tile. */
int fc_pathfind_bfs(int sx, int sy, int dx, int dy,
                    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                    const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
                    int out_x[], int out_y[], int max_steps);

/* Dynamic-aware sized route finder. Every candidate route step must satisfy
 * static terrain, dynamic occupancy, full-footprint availability, and diagonal
 * corner checks. This is helper infrastructure for later movement integration. */
int fc_pathfind_bfs_sized_dynamic(
    int sx, int sy, int dx, int dy, int size,
    const uint8_t walkable[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t movement_flags[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    const uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT],
    int out_x[], int out_y[], int max_steps);

#endif /* FC_PATHFINDING_H */
