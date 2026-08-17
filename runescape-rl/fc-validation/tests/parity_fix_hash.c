#include "fc_api.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET UINT32_C(0x811c9dc5)
#define FNV_PRIME UINT32_C(0x01000193)

static uint32_t ref_u8(uint32_t hash, uint8_t value) {
    return (hash ^ value) * FNV_PRIME;
}

static uint32_t ref_u32(uint32_t hash, uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        hash = ref_u8(hash, (uint8_t)(value >> shift));
    }
    return hash;
}

static uint32_t ref_i32(uint32_t hash, int value) {
    return ref_u32(hash, (uint32_t)(int32_t)value);
}

static uint32_t ref_f32(uint32_t hash, float value) {
    uint32_t bits;
    _Static_assert(sizeof(bits) == sizeof(value), "canonical hash requires 32-bit float");
    memcpy(&bits, &value, sizeof(bits));
    return ref_u32(hash, bits);
}

#define HASH_I32(value) hash = ref_i32(hash, (value))
#define HASH_U32(value) hash = ref_u32(hash, (value))
#define HASH_F32(value) hash = ref_f32(hash, (value))

static uint32_t reference_pending_hit(uint32_t hash, const FcPendingHit* hit) {
    HASH_I32(hit->active);
    HASH_I32(hit->damage);
    HASH_I32(hit->ticks_remaining);
    HASH_I32(hit->attack_style);
    HASH_I32(hit->source_npc_idx);
    HASH_I32(hit->prayer_drain);
    HASH_I32(hit->prayer_snapshot);
    HASH_I32(hit->prayer_lock_tick);
    return hash;
}

static uint32_t reference_player(uint32_t hash, const FcPlayer* player) {
    HASH_I32(player->x);
    HASH_I32(player->y);
    HASH_I32(player->current_hp);
    HASH_I32(player->max_hp);
    HASH_I32(player->current_prayer);
    HASH_I32(player->max_prayer);
    HASH_I32(player->prayer);
    HASH_I32(player->prayer_at_tick_start);
    HASH_I32(player->prayer_drain_counter);
    HASH_I32(player->sharks_remaining);
    HASH_I32(player->prayer_doses_remaining);
    HASH_I32(player->attack_timer);
    HASH_I32(player->food_timer);
    HASH_I32(player->potion_timer);
    HASH_I32(player->combo_timer);
    HASH_I32(player->run_energy);
    HASH_I32(player->is_running);
    HASH_I32(player->attack_level);
    HASH_I32(player->strength_level);
    HASH_I32(player->defence_level);
    HASH_I32(player->ranged_level);
    HASH_I32(player->prayer_level);
    HASH_I32(player->magic_level);
    HASH_I32(player->weapon_kind);
    HASH_I32(player->weapon_uses_ammo);
    HASH_I32(player->crystal_piece_mask);
    HASH_I32(player->weapon_speed);
    HASH_I32(player->weapon_range);
    HASH_I32(player->ranged_attack_bonus);
    HASH_I32(player->ranged_strength_bonus);
    HASH_I32(player->defence_stab);
    HASH_I32(player->defence_slash);
    HASH_I32(player->defence_crush);
    HASH_I32(player->defence_magic);
    HASH_I32(player->defence_ranged);
    HASH_I32(player->prayer_bonus);
    HASH_I32(player->ammo_count);
    HASH_I32(player->hp_regen_counter);
    for (int i = 0; i < FC_MAX_ROUTE; ++i) {
        HASH_I32(player->route_x[i]);
        HASH_I32(player->route_y[i]);
    }
    HASH_I32(player->route_len);
    HASH_I32(player->route_idx);
    HASH_F32(player->facing_angle);
    HASH_I32(player->attack_target_idx);
    HASH_I32(player->approach_target);
    for (int i = 0; i < FC_MAX_PENDING_HITS; ++i) {
        hash = reference_pending_hit(hash, &player->pending_hits[i]);
    }
    HASH_I32(player->num_pending_hits);
    HASH_I32(player->damage_taken_this_tick);
    HASH_I32(player->hit_style_this_tick);
    HASH_I32(player->hit_source_npc_type);
    HASH_I32(player->hit_locked_prayer_this_tick);
    HASH_I32(player->hit_blocked_this_tick);
    HASH_I32(player->hit_landed_this_tick);
    HASH_I32(player->food_eaten_this_tick);
    HASH_I32(player->potion_used_this_tick);
    HASH_I32(player->prayer_changed_this_tick);
    HASH_I32(player->total_damage_taken);
    HASH_I32(player->total_food_eaten);
    HASH_I32(player->total_potions_used);
    return hash;
}

static uint32_t reference_npc(uint32_t hash, const FcNpc* npc) {
    HASH_I32(npc->active);
    HASH_I32(npc->npc_type);
    HASH_I32(npc->spawn_index);
    HASH_I32(npc->x);
    HASH_I32(npc->y);
    HASH_I32(npc->size);
    HASH_I32(npc->current_hp);
    HASH_I32(npc->max_hp);
    HASH_I32(npc->is_dead);
    HASH_I32(npc->death_timer);
    HASH_I32(npc->attack_style);
    HASH_I32(npc->attack_timer);
    HASH_I32(npc->attack_speed);
    HASH_I32(npc->attack_range);
    HASH_I32(npc->max_hit_tenths);
    HASH_I32(npc->movement_speed);
    HASH_I32(npc->heal_timer);
    HASH_I32(npc->heal_amount);
    HASH_I32(npc->healer_distracted);
    HASH_I32(npc->heal_target_idx);
    HASH_I32(npc->is_respawned_jad_healer);
    HASH_I32(npc->damage_taken_this_tick);
    HASH_I32(npc->prayer_drain_dealt_this_tick);
    HASH_I32(npc->healing_received_this_tick);
    HASH_I32(npc->healing_given_this_tick);
    HASH_I32(npc->healed_by_mejkot_this_tick);
    HASH_I32(npc->healed_by_hurkot_this_tick);
    HASH_I32(npc->healed_self_this_tick);
    HASH_I32(npc->died_this_tick);
    for (int i = 0; i < FC_MAX_PENDING_HITS; ++i) {
        hash = reference_pending_hit(hash, &npc->pending_hits[i]);
    }
    HASH_I32(npc->num_pending_hits);
    return hash;
}

static uint32_t reference_state_hash_v2(const FcState* state) {
    uint32_t hash = FNV_OFFSET;
    hash = reference_player(hash, &state->player);
    for (int i = 0; i < FC_MAX_NPCS; ++i) {
        hash = reference_npc(hash, &state->npcs[i]);
    }

    HASH_I32(state->active_loadout);
    HASH_I32(state->current_wave);
    HASH_I32(state->rotation_id);
    HASH_I32(state->npcs_remaining);
    HASH_I32(state->total_npcs_killed);
    HASH_I32(state->next_spawn_index);
    HASH_I32(state->tick);
    HASH_I32(state->terminal);
    HASH_U32(state->rng_state);
    HASH_U32(state->rng_seed);
    for (int x = 0; x < FC_ARENA_WIDTH; ++x) {
        for (int y = 0; y < FC_ARENA_HEIGHT; ++y) {
            hash = ref_u8(hash, state->walkable[x][y]);
        }
    }
    for (int x = 0; x < FC_ARENA_WIDTH; ++x) {
        for (int y = 0; y < FC_ARENA_HEIGHT; ++y) {
            hash = ref_u8(hash, state->movement_flags[x][y]);
        }
    }
    for (int x = 0; x < FC_ARENA_WIDTH; ++x) {
        for (int y = 0; y < FC_ARENA_HEIGHT; ++y) {
            hash = ref_u8(hash, state->los_flags[x][y]);
        }
    }

    HASH_I32(state->movement_start_occupied_valid);
    HASH_I32(state->movement_start_player_x);
    HASH_I32(state->movement_start_player_y);
    for (int i = 0; i < FC_MAX_NPCS; ++i) {
        HASH_I32(state->movement_start_npc_x[i]);
        HASH_I32(state->movement_start_npc_y[i]);
        HASH_I32(state->movement_start_npc_size[i]);
        HASH_I32(state->movement_start_npc_active[i]);
    }
    HASH_I32(state->jad_healers_spawned);
    HASH_I32(state->jad_healer_spawn_generations);

    HASH_I32(state->damage_dealt_this_tick);
    HASH_I32(state->hits_landed_this_tick);
    HASH_I32(state->damage_taken_this_tick);
    HASH_I32(state->prayer_lost_this_tick);
    HASH_I32(state->overhead_prayer_lost_this_tick);
    HASH_I32(state->tz_kih_prayer_drain_this_tick);
    HASH_I32(state->npcs_killed_this_tick);
    HASH_I32(state->respawned_jad_healers_killed_this_tick);
    HASH_I32(state->wave_just_cleared);
    HASH_I32(state->jad_damage_this_tick);
    HASH_I32(state->jad_killed);
    HASH_I32(state->correct_jad_prayer);
    HASH_I32(state->wrong_jad_prayer);
    HASH_I32(state->correct_danger_prayer);
    HASH_I32(state->wrong_danger_prayer);
    HASH_I32(state->attack_attempt_this_tick);
    HASH_I32(state->invalid_action_this_tick);
    for (int i = 0; i < FC_INVALID_ACTION_CLASS_COUNT; ++i) {
        HASH_I32(state->invalid_action_class_this_tick[i]);
    }
    HASH_I32(state->movement_this_tick);
    HASH_I32(state->idle_this_tick);
    HASH_I32(state->food_used_this_tick);
    HASH_I32(state->prayer_potion_used_this_tick);
    HASH_I32(state->pre_eat_hp);
    HASH_I32(state->pre_drink_prayer);
    HASH_I32(state->jad_heal_procs_this_tick);
    HASH_I32(state->npc_heal_procs_this_tick);
    HASH_I32(state->npc_heal_amount_this_tick);
    HASH_I32(state->mejkot_heal_amount_this_tick);
    HASH_I32(state->jad_heal_amount_this_tick);

    HASH_F32(state->progress_required_work_start);
    HASH_F32(state->progress_required_work_remaining);
    HASH_F32(state->progress_current_wave_progress);
    HASH_F32(state->progress_cave_progress);
    HASH_F32(state->progress_delta_this_tick);
    HASH_I32(state->progress_ticks_since_positive);

    HASH_I32(state->ep_ticks_pray_melee);
    HASH_I32(state->ep_ticks_pray_range);
    HASH_I32(state->ep_ticks_pray_magic);
    HASH_I32(state->ep_correct_blocks);
    HASH_I32(state->ep_wrong_prayer_hits);
    HASH_I32(state->ep_no_prayer_hits);
    HASH_I32(state->ep_damage_blocked);
    HASH_I32(state->ep_prayer_switches);
    HASH_I32(state->ep_pots_used);
    HASH_I32(state->ep_pots_wasted);
    HASH_I32(state->ep_pot_pre_prayer_sum);
    HASH_I32(state->ep_food_eaten);
    HASH_I32(state->ep_food_pre_hp_sum);
    HASH_I32(state->ep_food_overhealed);
    HASH_I32(state->ep_pots_overrestored);
    HASH_I32(state->ep_tokxil_melee_ticks);
    HASH_I32(state->ep_ketzek_melee_ticks);
    HASH_I32(state->ep_attack_ready_ticks);
    HASH_I32(state->ep_attack_attempt_ticks);
    HASH_I32(state->safespot_attack_this_tick);
    for (int i = 0; i < FC_INVALID_ACTION_CLASS_COUNT; ++i) {
        HASH_I32(state->ep_invalid_action_classes[i]);
    }
    for (int i = 0; i < NPC_TYPE_COUNT; ++i) {
        HASH_I32(state->ep_damage_to_npc_type[i]);
        HASH_I32(state->ep_resolved_hits_to_npc_type[i]);
        HASH_I32(state->ep_damaging_hits_to_npc_type[i]);
        HASH_I32(state->ep_attack_cycles_to_npc_type[i]);
        HASH_I32(state->ep_target_ticks_by_npc_type[i]);
    }
    HASH_I32(state->ep_target_held_ticks);
    HASH_I32(state->ep_no_target_ticks);
    HASH_I32(state->ep_target_in_range_los_ticks);
    HASH_I32(state->ep_target_out_of_range_or_los_ticks);
    HASH_I32(state->ep_attack_cooldown_wait_ticks);
    HASH_I32(state->ep_ready_but_no_attack_ticks);
    HASH_I32(state->ep_action_move_idle_ticks);
    HASH_I32(state->ep_action_move_walk_ticks);
    HASH_I32(state->ep_action_move_run_ticks);
    HASH_I32(state->ep_action_attack_none_ticks);
    HASH_I32(state->ep_action_attack_target_ticks);
    HASH_I32(state->ep_action_prayer_noop_ticks);
    HASH_I32(state->ep_action_prayer_cmd_ticks);
    HASH_I32(state->ep_reached_wave_63);
    HASH_I32(state->ep_jad_killed);
    HASH_I32(state->wave_start_tick);
    HASH_I32(state->ep_max_wave_ticks);
    HASH_I32(state->ep_max_wave_ticks_wave);
    return hash;
}

#undef HASH_I32
#undef HASH_U32
#undef HASH_F32

static void make_golden_state(FcState* state) {
    memset(state, 0, sizeof(*state));
    state->player.x = -3;
    state->player.current_hp = 731;
    state->player.prayer = PRAYER_PROTECT_MAGIC;
    state->player.prayer_at_tick_start = PRAYER_PROTECT_RANGE;
    state->player.prayer_drain_counter = 47;
    state->player.weapon_kind = 2;
    state->player.crystal_piece_mask = 5;
    state->player.route_x[0] = 19;
    state->player.route_y[0] = 23;
    state->player.route_len = 1;
    state->player.facing_angle = -12.5f;
    state->player.pending_hits[0].active = 1;
    state->player.pending_hits[0].damage = 970;
    state->player.pending_hits[0].ticks_remaining = 2;
    state->player.pending_hits[0].attack_style = ATTACK_MAGIC;
    state->player.pending_hits[0].source_npc_idx = 4;
    state->player.pending_hits[0].prayer_snapshot = -1;
    state->player.pending_hits[0].prayer_lock_tick = 102;
    state->player.num_pending_hits = 1;

    state->npcs[4].active = 1;
    state->npcs[4].npc_type = NPC_TZTOK_JAD;
    state->npcs[4].spawn_index = 9;
    state->npcs[4].current_hp = 2500;
    state->npcs[4].attack_style = ATTACK_MAGIC;
    state->npcs[4].death_timer = 3;
    state->npcs[4].heal_target_idx = -1;
    state->active_loadout = 7;
    state->current_wave = 63;
    state->tick = 100;
    state->rng_state = UINT32_C(0x89abcdef);
    state->rng_seed = UINT32_C(0x12345678);
    state->walkable[2][3] = 1;
    state->movement_start_npc_x[4] = 31;
    state->movement_start_npc_active[4] = 1;
    state->correct_jad_prayer = 1;
    state->progress_cave_progress = 0.75f;
    state->ep_prayer_switches = 11;
    state->ep_damage_to_npc_type[NPC_TZTOK_JAD] = 420;
    state->terminal = TERMINAL_NONE;
}

static int test_version(void) {
    if (FC_STATE_HASH_VERSION != 2u) {
        fprintf(stderr, "FAIL DET-001: FC_STATE_HASH_VERSION=%u, expected 2\n",
                (unsigned)FC_STATE_HASH_VERSION);
        return 1;
    }
    printf("PASS DET-001: canonical state-hash version is 2\n");
    return 0;
}

#define MUTATE(label, expression) do {                                      \
    FcState changed = base;                                                 \
    expression;                                                             \
    uint32_t expected = reference_state_hash_v2(&changed);                  \
    uint32_t actual = fc_state_hash(&changed);                              \
    if (expected == reference_base) {                                       \
        fprintf(stderr, "FAIL DET-001 oracle: %s did not change reference hash\n", label); \
        return 1;                                                           \
    }                                                                       \
    if (actual == production_base) {                                        \
        fprintf(stderr, "  omitted: %s\n", label);                         \
        failures++;                                                         \
    }                                                                       \
} while (0)

static int test_field_coverage(void) {
    FcState base;
    int failures = 0;
    fc_init(&base);
    fc_reset(&base, UINT32_C(0x31415926));
    uint32_t reference_base = reference_state_hash_v2(&base);
    uint32_t production_base = fc_state_hash(&base);

    MUTATE("player.position", changed.player.x ^= 1);
    MUTATE("player.vitals", changed.player.current_hp ^= 1);
    MUTATE("player.live_prayer", changed.player.prayer ^= 1);
    MUTATE("player.tick_start_prayer", changed.player.prayer_at_tick_start ^= 1);
    MUTATE("player.prayer_drain_counter", changed.player.prayer_drain_counter ^= 1);
    MUTATE("player.consumables", changed.player.sharks_remaining ^= 1);
    MUTATE("player.combat_timer", changed.player.attack_timer ^= 1);
    MUTATE("player.run_state", changed.player.run_energy ^= 1);
    MUTATE("player.combat_stats", changed.player.attack_level ^= 1);
    MUTATE("player.weapon_kind", changed.player.weapon_kind ^= 1);
    MUTATE("player.weapon_uses_ammo", changed.player.weapon_uses_ammo ^= 1);
    MUTATE("player.crystal_piece_mask", changed.player.crystal_piece_mask ^= 1);
    MUTATE("player.weapon_timing", changed.player.weapon_speed ^= 1);
    MUTATE("player.equipment_bonuses", changed.player.ranged_attack_bonus ^= 1);
    MUTATE("player.ammo_count", changed.player.ammo_count ^= 1);
    MUTATE("player.hp_regen_counter", changed.player.hp_regen_counter ^= 1);
    MUTATE("player.route_coordinates", changed.player.route_x[0] ^= 1);
    MUTATE("player.route_cursor", changed.player.route_idx ^= 1);
    MUTATE("player.facing", changed.player.facing_angle += 1.0f);
    MUTATE("player.attack_target", changed.player.attack_target_idx ^= 1);
    MUTATE("player.approach_target", changed.player.approach_target ^= 1);
    MUTATE("player.pending.active", changed.player.pending_hits[0].active ^= 1);
    MUTATE("player.pending.damage", changed.player.pending_hits[0].damage ^= 1);
    MUTATE("player.pending.delay", changed.player.pending_hits[0].ticks_remaining ^= 1);
    MUTATE("player.pending.style", changed.player.pending_hits[0].attack_style ^= 1);
    MUTATE("player.pending.source", changed.player.pending_hits[0].source_npc_idx ^= 1);
    MUTATE("player.pending.prayer_drain", changed.player.pending_hits[0].prayer_drain ^= 1);
    MUTATE("player.pending.prayer_snapshot", changed.player.pending_hits[0].prayer_snapshot ^= 1);
    MUTATE("player.pending.prayer_lock_tick", changed.player.pending_hits[0].prayer_lock_tick ^= 1);
    MUTATE("player.pending.count", changed.player.num_pending_hits ^= 1);
    MUTATE("player.hit_event", changed.player.hit_locked_prayer_this_tick ^= 1);
    MUTATE("player.consumable_event", changed.player.food_eaten_this_tick ^= 1);
    MUTATE("player.prayer_transition_event", changed.player.prayer_changed_this_tick ^= 1);
    MUTATE("player.cumulative_stats", changed.player.total_food_eaten ^= 1);

    MUTATE("npc.identity", changed.npcs[0].npc_type ^= 1);
    MUTATE("npc.position", changed.npcs[0].x ^= 1);
    MUTATE("npc.vitals", changed.npcs[0].current_hp ^= 1);
    MUTATE("npc.death_timer", changed.npcs[0].death_timer ^= 1);
    MUTATE("npc.combat_style", changed.npcs[0].attack_style ^= 1);
    MUTATE("npc.combat_timer", changed.npcs[0].attack_timer ^= 1);
    MUTATE("npc.combat_stats", changed.npcs[0].max_hit_tenths ^= 1);
    MUTATE("npc.movement", changed.npcs[0].movement_speed ^= 1);
    MUTATE("npc.heal_timer", changed.npcs[0].heal_timer ^= 1);
    MUTATE("npc.heal_amount", changed.npcs[0].heal_amount ^= 1);
    MUTATE("npc.healer_target", changed.npcs[0].heal_target_idx ^= 1);
    MUTATE("npc.healer_runtime", changed.npcs[0].is_respawned_jad_healer ^= 1);
    MUTATE("npc.combat_event", changed.npcs[0].damage_taken_this_tick ^= 1);
    MUTATE("npc.heal_event", changed.npcs[0].healing_received_this_tick ^= 1);
    MUTATE("npc.pending.active", changed.npcs[0].pending_hits[0].active ^= 1);
    MUTATE("npc.pending.damage", changed.npcs[0].pending_hits[0].damage ^= 1);
    MUTATE("npc.pending.delay", changed.npcs[0].pending_hits[0].ticks_remaining ^= 1);
    MUTATE("npc.pending.style", changed.npcs[0].pending_hits[0].attack_style ^= 1);
    MUTATE("npc.pending.source", changed.npcs[0].pending_hits[0].source_npc_idx ^= 1);
    MUTATE("npc.pending.prayer_drain", changed.npcs[0].pending_hits[0].prayer_drain ^= 1);
    MUTATE("npc.pending.prayer_snapshot", changed.npcs[0].pending_hits[0].prayer_snapshot ^= 1);
    MUTATE("npc.pending.prayer_lock_tick", changed.npcs[0].pending_hits[0].prayer_lock_tick ^= 1);
    MUTATE("npc.pending.count", changed.npcs[0].num_pending_hits ^= 1);

    MUTATE("active_loadout", changed.active_loadout ^= 1);
    MUTATE("wave_state", changed.current_wave ^= 1);
    MUTATE("tick", changed.tick ^= 1);
    MUTATE("terminal", changed.terminal ^= 1);
    MUTATE("rng_state", changed.rng_state ^= UINT32_C(1));
    MUTATE("rng_seed", changed.rng_seed ^= UINT32_C(1));
    MUTATE("arena_walkability", changed.walkable[0][0] ^= UINT8_C(1));
    MUTATE("arena_movement_flags", changed.movement_flags[0][0] ^= UINT8_C(1));
    MUTATE("arena_los_flags", changed.los_flags[0][0] ^= UINT8_C(1));
    MUTATE("movement_reservations", changed.movement_start_npc_x[0] ^= 1);
    MUTATE("jad_healer_state", changed.jad_healer_spawn_generations ^= 1);
    MUTATE("reward_damage_event", changed.damage_dealt_this_tick ^= 1);
    MUTATE("reward_prayer_event", changed.correct_danger_prayer ^= 1);
    MUTATE("invalid_action_event", changed.invalid_action_class_this_tick[0] ^= 1);
    MUTATE("action_event", changed.attack_attempt_this_tick ^= 1);
    MUTATE("consumable_event", changed.pre_drink_prayer ^= 1);
    MUTATE("heal_event", changed.jad_heal_amount_this_tick ^= 1);
    MUTATE("progress_state", changed.progress_required_work_remaining += 1.0f);
    MUTATE("progress_timer", changed.progress_ticks_since_positive ^= 1);
    MUTATE("episode_prayer_analytics", changed.ep_prayer_switches ^= 1);
    MUTATE("episode_consumable_analytics", changed.ep_pots_used ^= 1);
    MUTATE("episode_combat_analytics", changed.ep_attack_ready_ticks ^= 1);
    MUTATE("episode_invalid_analytics", changed.ep_invalid_action_classes[0] ^= 1);
    MUTATE("episode_npc_analytics", changed.ep_damage_to_npc_type[1] ^= 1);
    MUTATE("episode_target_analytics", changed.ep_target_held_ticks ^= 1);
    MUTATE("episode_action_analytics", changed.ep_action_prayer_cmd_ticks ^= 1);
    MUTATE("episode_terminal_analytics", changed.ep_jad_killed ^= 1);
    MUTATE("episode_wave_analytics", changed.ep_max_wave_ticks ^= 1);

    if (failures) {
        fprintf(stderr, "FAIL DET-001: canonical hash missed %d future-relevant field classes\n",
                failures);
        return 1;
    }
    printf("PASS DET-001: every future-relevant field class changes the hash\n");
    return 0;
}

#undef MUTATE

static int test_golden(void) {
    const uint32_t expected_v2 = UINT32_C(0x73d93548);
    FcState state;
    make_golden_state(&state);
    uint32_t oracle = reference_state_hash_v2(&state);
    uint32_t actual = fc_state_hash(&state);

    if (oracle != expected_v2) {
        fprintf(stderr,
                "FAIL DET-001 oracle golden: got 0x%08" PRIx32 ", expected 0x%08" PRIx32 "\n",
                oracle, expected_v2);
        return 1;
    }
    if (actual != expected_v2) {
        fprintf(stderr,
                "FAIL DET-001 canonical golden: got 0x%08" PRIx32 ", expected 0x%08" PRIx32 "\n",
                actual, expected_v2);
        return 1;
    }
    printf("PASS DET-001: version-2 synthetic-state golden is 0x%08" PRIx32 "\n",
           expected_v2);
    return 0;
}

static int test_presentation_exclusion(void) {
    FcState state;
    FcRenderEntity presentation = {0};
    fc_init(&state);
    fc_reset(&state, 9u);
    uint32_t before = fc_state_hash(&state);
    presentation.x = 41;
    presentation.damage_taken_this_tick = 990;
    state.render_events.player_attack_fired = 1;
    state.render_events.player_attack_source_x = 17;
    state.render_events.prayer_flick_performed = 1;
    if (presentation.x != 41 || presentation.damage_taken_this_tick != 990) {
        fprintf(stderr, "FAIL DET-001: viewer-only presentation fixture did not mutate\n");
        return 1;
    }
    if (fc_state_hash(&state) != before) {
        fprintf(stderr, "FAIL DET-001: viewer-only presentation changed FcState hash\n");
        return 1;
    }
    printf("PASS DET-001: viewer-only entities/events are outside FcState hash\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <version|field_coverage|golden|presentation>\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "version") == 0) return test_version();
    if (strcmp(argv[1], "field_coverage") == 0) return test_field_coverage();
    if (strcmp(argv[1], "golden") == 0) return test_golden();
    if (strcmp(argv[1], "presentation") == 0) return test_presentation_exclusion();
    fprintf(stderr, "unknown case: %s\n", argv[1]);
    return 2;
}
