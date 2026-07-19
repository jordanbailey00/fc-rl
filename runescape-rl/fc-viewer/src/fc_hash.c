#include "fc_api.h"
#include <string.h>

/*
 * fc_hash.c — Deterministic state hash over explicit field values.
 *
 * Design: FNV-1a hash fed with explicit field values, NOT raw struct bytes.
 * This avoids false drift from:
 *   - Compiler-inserted padding bytes (may be uninitialized)
 *   - Struct layout differences across compilers/platforms
 *   - Floating-point representation differences
 *
 * The hash covers all state that affects simulation determinism.
 * Cosmetic/derived fields (like per-tick event flags that are recomputed
 * each tick from the actions) are included because they are part of the
 * observable state and are useful for catching drift.
 */

/* FNV-1a constants for 32-bit */
#define FNV_OFFSET 0x811c9dc5u
#define FNV_PRIME  0x01000193u

static inline uint32_t fnv_feed_int(uint32_t hash, int value) {
    const uint8_t* bytes = (const uint8_t*)&value;
    for (int i = 0; i < (int)sizeof(int); i++) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

static inline uint32_t fnv_feed_uint32(uint32_t hash, uint32_t value) {
    const uint8_t* bytes = (const uint8_t*)&value;
    for (int i = 0; i < (int)sizeof(uint32_t); i++) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

static inline uint32_t fnv_feed_uint8(uint32_t hash, uint8_t value) {
    hash ^= value;
    hash *= FNV_PRIME;
    return hash;
}

static uint32_t hash_pending_hits(uint32_t h, const FcPendingHit hits[], int count) {
    for (int i = 0; i < count; i++) {
        const FcPendingHit* hit = &hits[i];
        h = fnv_feed_int(h, hit->active);
        h = fnv_feed_int(h, hit->damage);
        h = fnv_feed_int(h, hit->ticks_remaining);
        h = fnv_feed_int(h, hit->attack_style);
        h = fnv_feed_int(h, hit->source_npc_idx);
        h = fnv_feed_int(h, hit->prayer_drain);
    }
    return h;
}

static uint32_t hash_player(uint32_t h, const FcPlayer* p) {
    h = fnv_feed_int(h, p->x);
    h = fnv_feed_int(h, p->y);
    h = fnv_feed_int(h, p->current_hp);
    h = fnv_feed_int(h, p->max_hp);
    h = fnv_feed_int(h, p->current_prayer);
    h = fnv_feed_int(h, p->max_prayer);
    h = fnv_feed_int(h, p->prayer);
    h = fnv_feed_int(h, p->sharks_remaining);
    h = fnv_feed_int(h, p->prayer_doses_remaining);
    h = fnv_feed_int(h, p->attack_timer);
    h = fnv_feed_int(h, p->food_timer);
    h = fnv_feed_int(h, p->potion_timer);
    h = fnv_feed_int(h, p->combo_timer);
    h = fnv_feed_int(h, p->run_energy);
    h = fnv_feed_int(h, p->is_running);
    h = fnv_feed_int(h, p->defence_level);
    h = fnv_feed_int(h, p->ranged_level);
    h = fnv_feed_int(h, p->ammo_count);
    h = fnv_feed_int(h, p->num_pending_hits);
    h = hash_pending_hits(h, p->pending_hits, FC_MAX_PENDING_HITS);
    h = fnv_feed_int(h, p->damage_taken_this_tick);
    h = fnv_feed_int(h, p->hit_landed_this_tick);
    h = fnv_feed_int(h, p->total_damage_taken);
    return h;
}

static uint32_t hash_npc(uint32_t h, const FcNpc* n) {
    h = fnv_feed_int(h, n->active);
    h = fnv_feed_int(h, n->npc_type);
    h = fnv_feed_int(h, n->spawn_index);
    h = fnv_feed_int(h, n->x);
    h = fnv_feed_int(h, n->y);
    h = fnv_feed_int(h, n->size);
    h = fnv_feed_int(h, n->current_hp);
    h = fnv_feed_int(h, n->max_hp);
    h = fnv_feed_int(h, n->is_dead);
    h = fnv_feed_int(h, n->attack_style);
    h = fnv_feed_int(h, n->attack_timer);
    h = fnv_feed_int(h, n->attack_speed);
    h = fnv_feed_int(h, n->attack_range);
    h = fnv_feed_int(h, n->max_hit);
    h = fnv_feed_int(h, n->movement_speed);
    h = fnv_feed_int(h, n->heal_timer);
    h = fnv_feed_int(h, n->healer_distracted);
    h = fnv_feed_int(h, n->heal_target_idx);
    h = fnv_feed_int(h, n->damage_taken_this_tick);
    h = fnv_feed_int(h, n->prayer_drain_dealt_this_tick);
    h = fnv_feed_int(h, n->healing_received_this_tick);
    h = fnv_feed_int(h, n->healing_given_this_tick);
    h = fnv_feed_int(h, n->healed_by_mejkot_this_tick);
    h = fnv_feed_int(h, n->healed_by_hurkot_this_tick);
    h = fnv_feed_int(h, n->healed_self_this_tick);
    h = fnv_feed_int(h, n->died_this_tick);
    h = fnv_feed_int(h, n->num_pending_hits);
    h = hash_pending_hits(h, n->pending_hits, FC_MAX_PENDING_HITS);
    return h;
}

uint32_t fc_state_hash(const FcState* state) {
    uint32_t h = FNV_OFFSET;

    /* Player */
    h = hash_player(h, &state->player);

    /* All NPC slots (including inactive — their zero state is deterministic) */
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        h = hash_npc(h, &state->npcs[i]);
    }

    /* Wave/meta */
    h = fnv_feed_int(h, state->current_wave);
    h = fnv_feed_int(h, state->rotation_id);
    h = fnv_feed_int(h, state->npcs_remaining);
    h = fnv_feed_int(h, state->total_npcs_killed);
    h = fnv_feed_int(h, state->next_spawn_index);
    h = fnv_feed_int(h, state->tick);
    h = fnv_feed_int(h, state->terminal);

    /* RNG state (critical for determinism — if RNG diverges, everything diverges) */
    h = fnv_feed_uint32(h, state->rng_state);

    /* Arena walkability */
    for (int x = 0; x < FC_ARENA_WIDTH; x++) {
        for (int y = 0; y < FC_ARENA_HEIGHT; y++) {
            h = fnv_feed_uint8(h, state->walkable[x][y]);
        }
    }

    /* Per-tick event flags */
    h = fnv_feed_int(h, state->damage_dealt_this_tick);
    h = fnv_feed_int(h, state->damage_taken_this_tick);
    h = fnv_feed_int(h, state->prayer_lost_this_tick);
    h = fnv_feed_int(h, state->overhead_prayer_lost_this_tick);
    h = fnv_feed_int(h, state->tz_kih_prayer_drain_this_tick);
    h = fnv_feed_int(h, state->npc_heal_amount_this_tick);
    h = fnv_feed_int(h, state->npcs_killed_this_tick);
    h = fnv_feed_int(h, state->wave_just_cleared);

    return h;
}
