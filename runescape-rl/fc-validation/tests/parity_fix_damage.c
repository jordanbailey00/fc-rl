#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_player_init.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t oracle_next(uint32_t state) {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static float oracle_float(uint32_t state) {
    return (float)(oracle_next(state) & 0x00FFFFFFu) /
           (float)0x01000000u;
}

static uint32_t seed_for_raw(int maximum, int raw) {
    for (uint32_t seed = 1; seed < 1000000u; seed++) {
        if ((int)(oracle_next(seed) % (uint32_t)(maximum + 1)) == raw)
            return seed;
    }
    return 0;
}

static void player_from_loadout(FcPlayer* player, int loadout_id) {
    const FcLoadout* loadout = &FC_LOADOUTS[loadout_id];
    memset(player, 0, sizeof(*player));
    player->max_hp = loadout->max_hp;
    player->current_hp = loadout->max_hp;
    player->max_prayer = loadout->max_prayer;
    player->current_prayer = loadout->max_prayer;
    player->attack_level = loadout->attack_lvl;
    player->strength_level = loadout->strength_lvl;
    player->defence_level = loadout->defence_lvl;
    player->ranged_level = loadout->ranged_lvl;
    player->prayer_level = loadout->prayer_lvl;
    player->magic_level = loadout->magic_lvl;
    player->weapon_kind = loadout->weapon_kind;
    player->weapon_uses_ammo = loadout->weapon_uses_ammo;
    player->crystal_piece_mask = loadout->crystal_piece_mask;
    player->weapon_speed = loadout->weapon_speed;
    player->weapon_range = loadout->weapon_range;
    player->ranged_attack_bonus = loadout->ranged_atk;
    player->ranged_strength_bonus = loadout->ranged_str;
    player->defence_stab = loadout->def_stab;
    player->defence_slash = loadout->def_slash;
    player->defence_crush = loadout->def_crush;
    player->defence_magic = loadout->def_magic;
    player->defence_ranged = loadout->def_ranged;
    player->prayer_bonus = loadout->prayer_bonus;
    player->ammo_count = loadout->ammo;
    player->attack_target_idx = -1;
}

static void make_open_state(FcState* state, int loadout_id) {
    fc_init(state);
    fc_reset(state, 123);
    memset(state->npcs, 0, sizeof(state->npcs));
    memset(state->walkable, 1, sizeof(state->walkable));
    state->terminal = TERMINAL_NONE;
    state->current_wave = 1;
    state->npcs_remaining = 0;
    state->next_spawn_index = 0;
    player_from_loadout(&state->player, loadout_id);
    state->player.x = 10;
    state->player.y = 10;
    state->player.attack_timer = 0;
    state->player.route_len = 0;
    state->player.route_idx = 0;
}

static void make_player_attack_state(FcState* state, int loadout_id,
                                     int npc_type, int npc_x) {
    make_open_state(state, loadout_id);
    fc_npc_spawn(&state->npcs[0], npc_type, npc_x, 10, 0);
    state->npcs_remaining = 1;
    state->next_spawn_index = 1;
    state->npcs[0].movement_speed = 0;
    state->npcs[0].attack_timer = 999;
}

static void make_npc_attack_state(FcState* state, int npc_type, int npc_x) {
    make_open_state(state, FC_LOADOUT_SOTA_TBOW);
    fc_npc_spawn(&state->npcs[0], npc_type, npc_x, 10, 0);
    state->npcs_remaining = 1;
    state->next_spawn_index = 1;
    state->npcs[0].movement_speed = 0;
    state->npcs[0].attack_timer = 0;
}

static int test_dmg_000(void) {
    FcState state;
    memset(&state, 0, sizeof(state));
    fc_rng_seed(&state, 12345);
    uint32_t before = state.rng_state;
    if (fc_roll_player_damage_tenths(&state, 0) != 0 ||
        state.rng_state != before) {
        fprintf(stderr,
                "FAIL DMG-000: zero-HP player maximum consumed RNG or manufactured damage\n");
        return 1;
    }
    if (fc_roll_npc_damage_tenths(&state, -1) != 0 ||
        state.rng_state != before) {
        fprintf(stderr,
                "FAIL DMG-000: negative NPC maximum was not rejected without RNG\n");
        return 1;
    }

    static const int targets[] = {
        NPC_TZ_KIH, NPC_TZ_KEK, NPC_TOK_XIL, NPC_YT_MEJKOT,
        NPC_KET_ZEK, NPC_TZTOK_JAD,
    };
    for (int loadout_id = 0; loadout_id < FC_NUM_LOADOUTS; loadout_id++) {
        FcPlayer player;
        player_from_loadout(&player, loadout_id);
        int base = fc_player_ranged_base_max_hit_hp(&player);
        if (base <= 0) {
            fprintf(stderr,
                    "FAIL DMG-000: production loadout %d has non-positive base maximum\n",
                    loadout_id);
            return 1;
        }
        for (size_t target_idx = 0;
             target_idx < sizeof(targets) / sizeof(targets[0]); target_idx++) {
            FcNpc target;
            memset(&target, 0, sizeof(target));
            target.npc_type = targets[target_idx];
            int final_hp = fc_player_ranged_final_max_hit_hp(&player, &target);
            if (final_hp <= 0) {
                fprintf(stderr,
                        "FAIL DMG-000: loadout %d target %d has non-positive final maximum\n",
                        loadout_id, targets[target_idx]);
                return 1;
            }
            for (uint32_t seed = 1; seed <= 32; seed++) {
                memset(&state, 0, sizeof(state));
                fc_rng_seed(&state, seed);
                int damage = fc_roll_player_damage_tenths(&state, final_hp);
                if (damage < 0 || damage > final_hp * 10 || damage % 10 != 0) {
                    fprintf(stderr,
                            "FAIL DMG-000: loadout %d target %d max %d returned %d tenths\n",
                            loadout_id, targets[target_idx], final_hp, damage);
                    return 1;
                }
            }
        }
    }

    printf("PASS DMG-000: unit-explicit max-hit and damage boundaries\n");
    return 0;
}

static int test_dmg_001(void) {
    static const int expected_player[] = {10, 10, 20, 30};
    static const int expected_npc[] = {0, 10, 20, 30};
    for (int raw = 0; raw <= 3; raw++) {
        uint32_t seed = seed_for_raw(3, raw);
        if (seed == 0) {
            fprintf(stderr, "FAIL DMG-001: no deterministic seed for raw %d\n", raw);
            return 1;
        }
        FcState player_state;
        FcState npc_state;
        memset(&player_state, 0, sizeof(player_state));
        memset(&npc_state, 0, sizeof(npc_state));
        fc_rng_seed(&player_state, seed);
        fc_rng_seed(&npc_state, seed);
        int player_damage = fc_roll_player_damage_tenths(&player_state, 3);
        int npc_damage = fc_roll_npc_damage_tenths(&npc_state, 3);
        uint32_t expected_state = oracle_next(seed);
        if (player_damage != expected_player[raw] ||
            npc_damage != expected_npc[raw] ||
            player_state.rng_state != expected_state ||
            npc_state.rng_state != expected_state) {
            fprintf(stderr,
                    "FAIL DMG-001: raw %d mapped player/NPC to %d/%d with RNG %u/%u, expected %d/%d and %u\n",
                    raw, player_damage, npc_damage,
                    player_state.rng_state, npc_state.rng_state,
                    expected_player[raw], expected_npc[raw], expected_state);
            return 1;
        }
    }

    FcState zero;
    memset(&zero, 0, sizeof(zero));
    fc_rng_seed(&zero, 999);
    uint32_t before = zero.rng_state;
    if (fc_roll_player_damage_tenths(&zero, 0) != 0 ||
        zero.rng_state != before) {
        fprintf(stderr,
                "FAIL DMG-001: synthetic zero maximum sampled or exceeded its bound\n");
        return 1;
    }

    printf("PASS DMG-001: exact player/NPC raw-roll mapping\n");
    return 0;
}

static int check_rng_oracle_case(int maximum, int raw, int player_roll) {
    uint32_t seed = seed_for_raw(maximum, raw);
    if (seed == 0) return 1;
    FcState state;
    memset(&state, 0, sizeof(state));
    fc_rng_seed(&state, seed);
    int actual = player_roll ?
        fc_roll_player_damage_tenths(&state, maximum) :
        fc_roll_npc_damage_tenths(&state, maximum);
    int expected_raw = player_roll && raw == 0 ? 1 : raw;
    int expected = expected_raw * 10;
    if (actual != expected || state.rng_state != oracle_next(seed)) {
        fprintf(stderr,
                "FAIL DMG-002: %s max=%d raw=%d damage=%d rng=%u, expected %d/%u\n",
                player_roll ? "player" : "NPC", maximum, raw,
                actual, state.rng_state, expected, oracle_next(seed));
        return 1;
    }
    return 0;
}

static int test_dmg_002(void) {
    static const int maxima[] = {
        3, 4, 7, 13, 14, 17, 18, 20, 21, 25, 26, 27,
        29, 39, 52, 55, 56, 58, 95, 97,
    };
    for (size_t i = 0; i < sizeof(maxima) / sizeof(maxima[0]); i++) {
        int maximum = maxima[i];
        int raw_values[] = {0, 1, maximum / 2, maximum};
        for (size_t j = 0; j < sizeof(raw_values) / sizeof(raw_values[0]); j++) {
            if (check_rng_oracle_case(maximum, raw_values[j], 1) ||
                check_rng_oracle_case(maximum, raw_values[j], 0)) {
                return 1;
            }
        }
    }

    printf("PASS DMG-002: independent xorshift state oracle\n");
    return 0;
}

static uint32_t seed_for_npc_miss(float chance) {
    for (uint32_t seed = 1; seed < 1000000u; seed++) {
        if (oracle_float(seed) >= chance) return seed;
    }
    return 0;
}

static int test_dmg_003(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[1] = 1;
    make_player_attack_state(&state, FC_LOADOUT_BLACK_DHIDE_RCB,
                             NPC_TZ_KIH, 12);
    state.player.ranged_level = -8;
    state.player.ranged_attack_bonus = 0;
    state.player.ammo_count = 10;
    fc_rng_seed(&state, 1234);
    uint32_t expected_rng = oracle_next(1234);
    fc_tick(&state, actions);
    if (!state.attack_attempt_this_tick || state.player.ammo_count != 9 ||
        state.rng_state != expected_rng ||
        state.damage_dealt_this_tick != 0 ||
        state.ep_attack_cycles_to_npc_type[NPC_TZ_KIH] != 1) {
        fprintf(stderr,
                "FAIL DMG-003: player miss attempt/ammo/RNG pipeline is %d/%d/%u/%d/%d\n",
                state.attack_attempt_this_tick, state.player.ammo_count,
                state.rng_state, state.damage_dealt_this_tick,
                state.ep_attack_cycles_to_npc_type[NPC_TZ_KIH]);
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    make_npc_attack_state(&state, NPC_TZ_KIH, 11);
    const FcNpcStats* stats = fc_npc_get_stats(NPC_TZ_KIH);
    int attack_roll = fc_npc_attack_roll(stats->att_level,
                                         stats->melee_attack_bonus);
    int defence_roll = fc_player_def_roll(&state.player,
                                          FC_ATTACK_TYPE_STAB);
    float chance = fc_hit_chance(attack_roll, defence_roll);
    uint32_t seed = seed_for_npc_miss(chance);
    if (seed == 0) {
        fprintf(stderr, "FAIL DMG-003: no deterministic NPC miss seed\n");
        fc_destroy(&state);
        return 1;
    }
    fc_rng_seed(&state, seed);
    fc_npc_tick(&state, 0);
    if (state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].damage != 0 ||
        state.rng_state != oracle_next(seed) ||
        state.npcs[0].attack_timer != state.npcs[0].attack_speed) {
        fprintf(stderr,
                "FAIL DMG-003: NPC miss queued/RNG/cycle invariant failed\n");
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    printf("PASS DMG-003: misses consume only accuracy RNG and preserve cycles\n");
    return 0;
}

static int test_dmg_004(void) {
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[1] = 1;
    int player_successes = 0;
    int generic_successes = 0;
    int jad_successes = 0;

    for (uint32_t seed = 1; seed <= 512; seed++) {
        FcState state;
        make_player_attack_state(&state, FC_LOADOUT_SOTA_TBOW,
                                 NPC_TZ_KIH, 12);
        fc_rng_seed(&state, seed);
        fc_tick(&state, actions);
        int damage = state.damage_dealt_this_tick;
        if (damage > 0) {
            player_successes++;
            if (damage % 10 != 0 || damage > 170) {
                fprintf(stderr,
                        "FAIL DMG-004: player tick call site produced %d with max 170\n",
                        damage);
                fc_destroy(&state);
                return 1;
            }
        }
        fc_destroy(&state);

        make_npc_attack_state(&state, NPC_TZ_KIH, 11);
        fc_rng_seed(&state, seed);
        fc_npc_tick(&state, 0);
        if (state.player.num_pending_hits != 1) {
            fprintf(stderr,
                    "FAIL DMG-004: generic NPC did not queue one hit\n");
            fc_destroy(&state);
            return 1;
        }
        damage = state.player.pending_hits[0].damage;
        if (damage > 0) {
            generic_successes++;
            if (damage % 10 != 0 || damage > 40) {
                fprintf(stderr,
                        "FAIL DMG-004: generic NPC call site produced %d with max 40\n",
                        damage);
                fc_destroy(&state);
                return 1;
            }
        }
        fc_destroy(&state);

        make_npc_attack_state(&state, NPC_TZTOK_JAD, 20);
        fc_rng_seed(&state, seed);
        fc_npc_tick(&state, 0);
        if (state.player.num_pending_hits != 1) {
            fprintf(stderr, "FAIL DMG-004: Jad did not queue one hit\n");
            fc_destroy(&state);
            return 1;
        }
        damage = state.player.pending_hits[0].damage;
        int style = state.player.pending_hits[0].attack_style;
        int max_tenths = fc_npc_max_hit_tenths_for_style(
            fc_npc_get_stats(NPC_TZTOK_JAD), style);
        if (damage > 0) {
            jad_successes++;
            if (damage % 10 != 0 || damage > max_tenths) {
                fprintf(stderr,
                        "FAIL DMG-004: Jad style %d call site produced %d with max %d\n",
                        style, damage, max_tenths);
                fc_destroy(&state);
                return 1;
            }
        }
        fc_destroy(&state);
    }

    if (!player_successes || !generic_successes || !jad_successes) {
        fprintf(stderr,
                "FAIL DMG-004: corpus missed successful player/generic/Jad attacks\n");
        return 1;
    }

    printf("PASS DMG-004: all three production damage-generation sites\n");
    return 0;
}

static int test_dmg_005(void) {
    FcPlayer player;
    FcNpc low;
    FcNpc jad;
    memset(&low, 0, sizeof(low));
    memset(&jad, 0, sizeof(jad));
    low.npc_type = NPC_TZ_KIH;
    jad.npc_type = NPC_TZTOK_JAD;

    player_from_loadout(&player, FC_LOADOUT_SOTA_TBOW);
    if (fc_player_ranged_base_max_hit_hp(&player) != 27 ||
        fc_player_ranged_final_max_hit_hp(&player, &low) != 17 ||
        fc_player_ranged_final_max_hit_hp(&player, &jad) != 58) {
        fprintf(stderr,
                "FAIL DMG-005: SOTA TBow did not use 27 -> 17/58 final maxima\n");
        return 1;
    }

    player_from_loadout(&player, FC_LOADOUT_BOWFA_CRYSTAL);
    if (fc_player_ranged_base_max_hit_hp(&player) != 26 ||
        fc_player_ranged_final_max_hit_hp(&player, &jad) != 29) {
        fprintf(stderr,
                "FAIL DMG-005: full-crystal Bowfa did not use 26 -> 29 final maximum\n");
        return 1;
    }

    player_from_loadout(&player, FC_LOADOUT_BLACK_DHIDE_RCB);
    if (fc_player_ranged_final_max_hit_hp(&player, &low) != 20 ||
        fc_player_ranged_final_max_hit_hp(&player, &jad) != 20) {
        fprintf(stderr,
                "FAIL DMG-005: generic weapon final maximum changed with target\n");
        return 1;
    }

    printf("PASS DMG-005: damage uses final post-effect maximum\n");
    return 0;
}

static int test_dmg_006(void) {
    FcState state;
    make_open_state(&state, FC_LOADOUT_SOTA_TBOW);
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 15, 10, 0);
    state.npcs_remaining = 1;
    state.npcs[0].current_hp = 500;
    if (!fc_queue_pending_hit(state.npcs[0].pending_hits,
                              &state.npcs[0].num_pending_hits,
                              FC_MAX_PENDING_HITS,
                              170, 1, ATTACK_RANGED, -1, 0)) {
        fprintf(stderr, "FAIL DMG-006: could not queue player damage fixture\n");
        fc_destroy(&state);
        return 1;
    }
    if (state.npcs[0].pending_hits[0].damage != 170) {
        fprintf(stderr, "FAIL DMG-006: pending damage did not retain 170\n");
        fc_destroy(&state);
        return 1;
    }
    fc_resolve_npc_pending_hits(&state, 0);
    if (state.npcs[0].current_hp != 330 ||
        state.npcs[0].damage_taken_this_tick != 170 ||
        state.damage_dealt_this_tick != 170 ||
        state.jad_damage_this_tick != 170 ||
        state.ep_damage_to_npc_type[NPC_TZTOK_JAD] != 170 ||
        state.ep_resolved_hits_to_npc_type[NPC_TZTOK_JAD] != 1 ||
        state.ep_damaging_hits_to_npc_type[NPC_TZTOK_JAD] != 1) {
        fprintf(stderr,
                "FAIL DMG-006: NPC HP/events did not consume 170 tenths exactly\n");
        fc_destroy(&state);
        return 1;
    }

    FcRenderEntity entities[FC_MAX_RENDER_ENTITIES];
    int entity_count = 0;
    fc_fill_render_entities(&state, entities, &entity_count);
    if (entity_count < 2 || entities[1].damage_taken_this_tick != 170) {
        fprintf(stderr,
                "FAIL DMG-006: render snapshot rescaled 170 tenths\n");
        fc_destroy(&state);
        return 1;
    }
    float obs[FC_OBS_SIZE];
    float rewards[FC_REWARD_FEATURES];
    fc_write_obs(&state, obs);
    fc_write_reward_features(&state, rewards);
    if (fabsf(rewards[FC_RWD_DAMAGE_DEALT] - 0.17f) > 1.0e-6f ||
        fabsf(rewards[FC_RWD_JAD_DAMAGE] - 0.17f) > 1.0e-6f) {
        fprintf(stderr,
                "FAIL DMG-006: rewards double-scaled damage %.6f/%.6f\n",
                rewards[FC_RWD_DAMAGE_DEALT], rewards[FC_RWD_JAD_DAMAGE]);
        fc_destroy(&state);
        return 1;
    }
    for (int i = 0; i < FC_OBS_SIZE; i++) {
        if (!isfinite(obs[i])) {
            fprintf(stderr, "FAIL DMG-006: non-finite observation at %d\n", i);
            fc_destroy(&state);
            return 1;
        }
    }

    state.player.current_hp = 500;
    state.player.max_hp = 990;
    state.player.num_pending_hits = 0;
    state.player.damage_taken_this_tick = 0;
    state.damage_taken_this_tick = 0;
    fc_npc_spawn(&state.npcs[1], NPC_TZ_KIH, 11, 10, 1);
    if (!fc_queue_pending_hit(state.player.pending_hits,
                              &state.player.num_pending_hits,
                              FC_MAX_PENDING_HITS,
                              170, 1, ATTACK_MELEE, 1, 0)) {
        fprintf(stderr, "FAIL DMG-006: could not queue NPC damage fixture\n");
        fc_destroy(&state);
        return 1;
    }
    state.player.pending_hits[0].prayer_snapshot = PRAYER_NONE;
    fc_resolve_player_pending_hits(&state);
    if (state.player.current_hp != 330 ||
        state.player.damage_taken_this_tick != 170 ||
        state.damage_taken_this_tick != 170 ||
        state.player.total_damage_taken != 170) {
        fprintf(stderr,
                "FAIL DMG-006: player HP/events did not consume 170 tenths exactly\n");
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    printf("PASS DMG-006: tenths storage and downstream consumers\n");
    return 0;
}

static int test_dmg_007(void) {
    static const int loadouts[] = {
        FC_LOADOUT_BLACK_DHIDE_RCB,
        FC_LOADOUT_SOTA_TBOW,
        FC_LOADOUT_BOWFA_CRYSTAL,
    };
    FcNpc target;
    memset(&target, 0, sizeof(target));
    target.npc_type = NPC_TZTOK_JAD;
    for (size_t i = 0; i < sizeof(loadouts) / sizeof(loadouts[0]); i++) {
        FcPlayer player;
        player_from_loadout(&player, loadouts[i]);
        int maximum = fc_player_ranged_final_max_hit_hp(&player, &target);
        for (uint32_t seed = 1; seed <= 4096; seed++) {
            FcState state;
            memset(&state, 0, sizeof(state));
            fc_rng_seed(&state, seed);
            int damage = fc_roll_player_damage_tenths(&state, maximum);
            if (damage <= 0 || damage > maximum * 10 || damage % 10 != 0) {
                fprintf(stderr,
                        "FAIL DMG-007: loadout %d successful roll returned %d with max %d\n",
                        loadouts[i], damage, maximum);
                return 1;
            }
        }
    }

    FcState state;
    make_open_state(&state, FC_LOADOUT_BLACK_DHIDE_RCB);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.npcs[0].current_hp = 10;
    uint32_t zero_seed = seed_for_raw(20, 0);
    fc_rng_seed(&state, zero_seed);
    int killing_damage = fc_roll_player_damage_tenths(&state, 20);
    fc_queue_pending_hit(state.npcs[0].pending_hits,
                         &state.npcs[0].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         killing_damage, 1, ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, 0);
    if (!state.npcs[0].is_dead || killing_damage != 10) {
        fprintf(stderr,
                "FAIL DMG-007: accurate raw-zero hit did not kill 1-HP target (%d)\n",
                killing_damage);
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    make_open_state(&state, FC_LOADOUT_BLACK_DHIDE_RCB);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.npcs[0].current_hp = 10;
    fc_queue_pending_hit(state.npcs[0].pending_hits,
                         &state.npcs[0].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         0, 1, ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, 0);
    if (state.npcs[0].is_dead || state.npcs[0].current_hp != 10) {
        fprintf(stderr, "FAIL DMG-007: miss killed 1-HP target\n");
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    uint32_t npc_zero_seed = seed_for_raw(97, 0);
    memset(&state, 0, sizeof(state));
    fc_rng_seed(&state, npc_zero_seed);
    if (fc_roll_npc_damage_tenths(&state, 97) != 0) {
        fprintf(stderr, "FAIL DMG-007: accurate NPC zero was removed\n");
        return 1;
    }

    printf("PASS DMG-007: player minimum hit and NPC zero semantics\n");
    return 0;
}

static int run_tz_kih_case(int damage, int prayer_snapshot,
                           int initial_prayer, int expected_hp_damage,
                           int expected_prayer_loss) {
    FcState state;
    make_open_state(&state, FC_LOADOUT_SOTA_TBOW);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.npcs_remaining = 1;
    state.player.current_prayer = initial_prayer;
    int hp_before = state.player.current_hp;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         damage, 1, ATTACK_MELEE, 0,
                         fc_npc_get_stats(NPC_TZ_KIH)->prayer_drain);
    state.player.pending_hits[0].prayer_snapshot = prayer_snapshot;
    fc_resolve_player_pending_hits(&state);

    float obs[FC_OBS_SIZE];
    fc_write_obs(&state, obs);
    float expected_total = (float)expected_prayer_loss /
                           (float)state.player.max_prayer;
    float expected_source = (float)expected_prayer_loss / 50.0f;
    if (hp_before - state.player.current_hp != expected_hp_damage ||
        initial_prayer - state.player.current_prayer != expected_prayer_loss ||
        state.prayer_lost_this_tick != expected_prayer_loss ||
        state.tz_kih_prayer_drain_this_tick != expected_prayer_loss ||
        state.npcs[0].prayer_drain_dealt_this_tick != expected_prayer_loss ||
        state.player.current_prayer < 0 ||
        fabsf(obs[FC_OBS_PLAYER_PRAYER_LOST] - expected_total) > 1.0e-6f ||
        fabsf(obs[FC_OBS_NPC_START + FC_NPC_PRAYER_DRAIN_DEALT] -
              expected_source) > 1.0e-6f) {
        fprintf(stderr,
                "FAIL DMG-008: damage=%d snapshot=%d prayer=%d produced hp/prayer %d/%d metrics %d/%d/%d\n",
                damage, prayer_snapshot, initial_prayer,
                hp_before - state.player.current_hp,
                initial_prayer - state.player.current_prayer,
                state.prayer_lost_this_tick,
                state.tz_kih_prayer_drain_this_tick,
                state.npcs[0].prayer_drain_dealt_this_tick);
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);
    return 0;
}

static int test_dmg_008(void) {
    if (run_tz_kih_case(30, PRAYER_NONE, 990, 30, 40) ||
        run_tz_kih_case(0, PRAYER_NONE, 990, 0, 10) ||
        run_tz_kih_case(30, PRAYER_PROTECT_MELEE, 990, 0, 10) ||
        run_tz_kih_case(30, PRAYER_NONE, 5, 30, 5)) {
        return 1;
    }

    printf("PASS DMG-008: Tz-Kih damage/prayer coupling and capped metrics\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr,
                "usage: %s <dmg_000|dmg_001|dmg_002|dmg_003|dmg_004|dmg_005|dmg_006|dmg_007|dmg_008>\n",
                argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "dmg_000") == 0) return test_dmg_000();
    if (strcmp(argv[1], "dmg_001") == 0) return test_dmg_001();
    if (strcmp(argv[1], "dmg_002") == 0) return test_dmg_002();
    if (strcmp(argv[1], "dmg_003") == 0) return test_dmg_003();
    if (strcmp(argv[1], "dmg_004") == 0) return test_dmg_004();
    if (strcmp(argv[1], "dmg_005") == 0) return test_dmg_005();
    if (strcmp(argv[1], "dmg_006") == 0) return test_dmg_006();
    if (strcmp(argv[1], "dmg_007") == 0) return test_dmg_007();
    if (strcmp(argv[1], "dmg_008") == 0) return test_dmg_008();

    fprintf(stderr, "unknown parity damage test: %s\n", argv[1]);
    return 2;
}
