#include "fc_api.h"
#include "fc_combat.h"
#include "fc_contracts.h"
#include "fc_npc.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    PIPE_TOK_ADJACENT,
    PIPE_TOK_DISTANT,
    PIPE_KET_ADJACENT,
    PIPE_KET_DISTANT,
    PIPE_JAD_ADJACENT,
    PIPE_JAD_DISTANT,
} PipelineKind;

typedef struct {
    PipelineKind kind;
    int npc_type;
    int npc_x;
    const char* label;
    unsigned required_style_mask;
} PipelineCase;

static const PipelineCase PIPELINE_CASES[] = {
    {PIPE_TOK_ADJACENT, NPC_TOK_XIL, 11, "Tok-Xil adjacent",
     1u << ATTACK_MELEE},
    {PIPE_TOK_DISTANT, NPC_TOK_XIL, 20, "Tok-Xil distant",
     1u << ATTACK_RANGED},
    {PIPE_KET_ADJACENT, NPC_KET_ZEK, 11, "Ket-Zek adjacent",
     (1u << ATTACK_MELEE) | (1u << ATTACK_MAGIC)},
    {PIPE_KET_DISTANT, NPC_KET_ZEK, 20, "Ket-Zek distant",
     1u << ATTACK_MAGIC},
    {PIPE_JAD_ADJACENT, NPC_TZTOK_JAD, 11, "Jad adjacent",
     (1u << ATTACK_MELEE) | (1u << ATTACK_RANGED) |
         (1u << ATTACK_MAGIC)},
    {PIPE_JAD_DISTANT, NPC_TZTOK_JAD, 20, "Jad distant",
     (1u << ATTACK_RANGED) | (1u << ATTACK_MAGIC)},
};

static void make_attack_state(FcState* state, int npc_type, int npc_x) {
    fc_init(state);
    fc_reset(state, 123u);
    memset(state->npcs, 0, sizeof(state->npcs));
    memset(state->walkable, 1, sizeof(state->walkable));
    state->terminal = TERMINAL_NONE;
    state->current_wave = 1;
    state->npcs_remaining = 1;
    state->next_spawn_index = 1;
    state->player.x = 10;
    state->player.y = 10;
    state->player.prayer = PRAYER_NONE;
    state->player.attack_target_idx = -1;
    state->player.route_len = 0;
    state->player.route_idx = 0;
    state->player.num_pending_hits = 0;

    /* Use the exact SOTA defence vector so attack-type routing affects the
     * externally visible hit/miss decision. */
    state->player.defence_level = 99;
    state->player.magic_level = 1;
    state->player.defence_stab = 116;
    state->player.defence_slash = 106;
    state->player.defence_crush = 129;
    state->player.defence_magic = 150;
    state->player.defence_ranged = 121;

    fc_npc_spawn(&state->npcs[0], npc_type, npc_x, 10, 0);
    state->npcs[0].movement_speed = 0;
    state->npcs[0].attack_timer = 0;
}

static int expected_style(FcState* oracle, PipelineKind kind) {
    switch (kind) {
        case PIPE_TOK_ADJACENT: return ATTACK_MELEE;
        case PIPE_TOK_DISTANT: return ATTACK_RANGED;
        case PIPE_KET_ADJACENT:
            return fc_rng_int(oracle, 2) == 0 ? ATTACK_MAGIC : ATTACK_MELEE;
        case PIPE_KET_DISTANT: return ATTACK_MAGIC;
        case PIPE_JAD_ADJACENT: {
            int choice = fc_rng_int(oracle, 3);
            if (choice == 0) return ATTACK_MELEE;
            if (choice == 1) return ATTACK_MAGIC;
            return ATTACK_RANGED;
        }
        case PIPE_JAD_DISTANT:
            return fc_rng_int(oracle, 2) == 0 ? ATTACK_MAGIC : ATTACK_RANGED;
        default: return ATTACK_NONE;
    }
}

static int expected_level(int npc_type, int style) {
    switch (npc_type) {
        case NPC_TOK_XIL:
            return style == ATTACK_MELEE ? 80 : 120;
        case NPC_KET_ZEK:
            return style == ATTACK_MELEE ? 320 : 240;
        case NPC_TZTOK_JAD:
            if (style == ATTACK_MELEE) return 640;
            if (style == ATTACK_RANGED) return 960;
            return 480;
        default:
            return 0;
    }
}

static int expected_max_tenths(int npc_type, int style) {
    switch (npc_type) {
        case NPC_TOK_XIL:
            return style == ATTACK_MAGIC ? 0 : 130;
        case NPC_KET_ZEK:
            return style == ATTACK_MELEE ? 550 :
                   (style == ATTACK_MAGIC ? 520 : 0);
        case NPC_TZTOK_JAD:
            if (style == ATTACK_MELEE || style == ATTACK_RANGED) return 970;
            return style == ATTACK_MAGIC ? 950 : 0;
        default:
            return 0;
    }
}

static FcAttackType expected_attack_type(int npc_type, int style) {
    if (style == ATTACK_RANGED) return FC_ATTACK_TYPE_RANGED;
    if (style == ATTACK_MAGIC) return FC_ATTACK_TYPE_MAGIC;
    if (npc_type == NPC_KET_ZEK || npc_type == NPC_TZTOK_JAD ||
        npc_type == NPC_TZ_KIH) {
        return FC_ATTACK_TYPE_STAB;
    }
    return FC_ATTACK_TYPE_CRUSH;
}

static int independent_defence_roll(const FcPlayer* player,
                                    FcAttackType type) {
    int bonus = 0;
    switch (type) {
        case FC_ATTACK_TYPE_STAB: bonus = player->defence_stab; break;
        case FC_ATTACK_TYPE_SLASH: bonus = player->defence_slash; break;
        case FC_ATTACK_TYPE_CRUSH: bonus = player->defence_crush; break;
        case FC_ATTACK_TYPE_RANGED: bonus = player->defence_ranged; break;
        case FC_ATTACK_TYPE_MAGIC: bonus = player->defence_magic; break;
        default: break;
    }

    int effective = player->defence_level + 8;
    if (type == FC_ATTACK_TYPE_MAGIC) {
        effective = 3 * player->defence_level / 10 +
                    7 * player->magic_level / 10 + 8;
    }
    return effective * (bonus + 64);
}

static int test_npc_004(void) {
    for (size_t case_idx = 0;
         case_idx < sizeof(PIPELINE_CASES) / sizeof(PIPELINE_CASES[0]);
         case_idx++) {
        const PipelineCase* pipeline = &PIPELINE_CASES[case_idx];
        unsigned observed_styles = 0;

        for (uint32_t seed = 1; seed <= 512; seed++) {
            FcState state;
            make_attack_state(&state, pipeline->npc_type, pipeline->npc_x);
            fc_rng_seed(&state, seed);
            FcState oracle = state;

            int style = expected_style(&oracle, pipeline->kind);
            observed_styles |= 1u << style;
            int attack_roll = (expected_level(pipeline->npc_type, style) + 9) * 64;
            int defence_roll = independent_defence_roll(
                &oracle.player,
                expected_attack_type(pipeline->npc_type, style));
            float chance = fc_hit_chance(attack_roll, defence_roll);
            int expected_hit = fc_rng_float(&oracle) < chance;
            if (expected_hit) (void)fc_rng_next(&oracle);

            fc_npc_tick(&state, 0);
            if (state.player.num_pending_hits != 1) {
                fprintf(stderr,
                        "FAIL NPC-004: %s seed %u queued %d hits, expected 1\n",
                        pipeline->label, seed, state.player.num_pending_hits);
                fc_destroy(&state);
                return 1;
            }
            const FcPendingHit* hit = &state.player.pending_hits[0];
            if (hit->attack_style != style) {
                fprintf(stderr,
                        "FAIL NPC-004: %s seed %u selected broad style %d, expected %d\n",
                        pipeline->label, seed, hit->attack_style, style);
                fc_destroy(&state);
                return 1;
            }
            if (state.rng_state != oracle.rng_state) {
                fprintf(stderr,
                        "FAIL NPC-004: %s seed %u used the wrong style-specific offensive/defensive roll (style %d)\n",
                        pipeline->label, seed, style);
                fc_destroy(&state);
                return 1;
            }
            int maximum = expected_max_tenths(pipeline->npc_type, style);
            if (hit->damage < 0 || hit->damage > maximum) {
                fprintf(stderr,
                        "FAIL NPC-004: %s seed %u style %d queued damage %d outside [0,%d]\n",
                        pipeline->label, seed, style, hit->damage, maximum);
                fc_destroy(&state);
                return 1;
            }
            fc_destroy(&state);
        }

        if ((observed_styles & pipeline->required_style_mask) !=
            pipeline->required_style_mask) {
            fprintf(stderr,
                    "FAIL NPC-004: fixed corpus did not cover every %s style (mask %u, expected %u)\n",
                    pipeline->label, observed_styles,
                    pipeline->required_style_mask);
            return 1;
        }
    }

    printf("PASS NPC-004: real dual-style attack routing\n");
    return 0;
}

static int test_npc_005(void) {
    static const struct {
        int max_hp;
        int size;
        int style;
        int speed;
        int range;
        int primary_max_tenths;
    } expected[NPC_TYPE_COUNT] = {
        [NPC_TZ_KIH] = {100, 1, ATTACK_MELEE, 4, 1, 40},
        [NPC_TZ_KEK] = {200, 2, ATTACK_MELEE, 4, 1, 70},
        [NPC_TZ_KEK_SM] = {100, 1, ATTACK_MELEE, 4, 1, 40},
        [NPC_TOK_XIL] = {400, 3, ATTACK_RANGED, 4, 14, 130},
        [NPC_YT_MEJKOT] = {800, 4, ATTACK_MELEE, 4, 1, 250},
        [NPC_KET_ZEK] = {1600, 5, ATTACK_MAGIC, 4, 14, 520},
        [NPC_TZTOK_JAD] = {2500, 5, ATTACK_MAGIC, 8, 14, 950},
        [NPC_YT_HURKOT] = {600, 1, ATTACK_MELEE, 4, 1, 140},
    };

    for (int type = NPC_TZ_KIH; type < NPC_TYPE_COUNT; type++) {
        FcNpc npc;
        memset(&npc, 0xA5, sizeof(npc));
        fc_npc_spawn(&npc, type, 7, 9, 11);
        if (!npc.active || npc.npc_type != type || npc.spawn_index != 11 ||
            npc.x != 7 || npc.y != 9 ||
            npc.current_hp != expected[type].max_hp ||
            npc.max_hp != expected[type].max_hp ||
            npc.size != expected[type].size ||
            npc.attack_style != expected[type].style ||
            npc.attack_speed != expected[type].speed ||
            npc.attack_range != expected[type].range ||
            npc.max_hit_tenths != expected[type].primary_max_tenths) {
            fprintf(stderr,
                    "FAIL NPC-005: spawned NPC type %d disagrees with its table (hp=%d/%d size=%d style=%d speed=%d range=%d max=%d)\n",
                    type, npc.current_hp, npc.max_hp, npc.size,
                    npc.attack_style, npc.attack_speed, npc.attack_range,
                    npc.max_hit_tenths);
            return 1;
        }
    }

    /* The compatibility maximum is state for display/observation consumers;
     * changing it must not change combat's style-selected maximum. */
    for (uint32_t seed = 1; seed <= 64; seed++) {
        FcState state;
        make_attack_state(&state, NPC_TOK_XIL, 20);
        state.npcs[0].max_hit_tenths = 100000;
        state.player.defence_level = 1;
        state.player.magic_level = 1;
        state.player.defence_ranged = -64;
        fc_rng_seed(&state, seed);
        fc_npc_tick(&state, 0);
        if (state.player.num_pending_hits != 1 ||
            state.player.pending_hits[0].attack_style != ATTACK_RANGED) {
            fprintf(stderr,
                    "FAIL NPC-005: Tok-Xil compatibility-field probe did not queue a ranged hit\n");
            fc_destroy(&state);
            return 1;
        }
        if (state.player.pending_hits[0].damage > 130) {
            fprintf(stderr,
                    "FAIL NPC-005: combat consumed compatibility max_hit_tenths (%d damage)\n",
                    state.player.pending_hits[0].damage);
            fc_destroy(&state);
            return 1;
        }
        fc_destroy(&state);
    }

    /* Tz-Kih observation normalization also obtains its maximum from the
     * style accessor, not from the mutable compatibility field. */
    FcState state;
    float obs[FC_OBS_SIZE];
    make_attack_state(&state, NPC_TZ_KIH, 11);
    state.npcs[0].max_hit_tenths = 100000;
    state.npcs[0].prayer_drain_dealt_this_tick = 50;
    fc_write_obs(&state, obs);
    float normalized = obs[FC_OBS_NPC_START + FC_NPC_PRAYER_DRAIN_DEALT];
    if (fabsf(normalized - 1.0f) > 1.0e-6f) {
        fprintf(stderr,
                "FAIL NPC-005: downstream maximum normalization used compatibility state (%.9f)\n",
                normalized);
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    printf("PASS NPC-005: spawn and downstream style consumers\n");
    return 0;
}

static int* selected_defence_field(FcPlayer* player, FcAttackType type) {
    switch (type) {
        case FC_ATTACK_TYPE_STAB: return &player->defence_stab;
        case FC_ATTACK_TYPE_SLASH: return &player->defence_slash;
        case FC_ATTACK_TYPE_CRUSH: return &player->defence_crush;
        case FC_ATTACK_TYPE_RANGED: return &player->defence_ranged;
        case FC_ATTACK_TYPE_MAGIC: return &player->defence_magic;
        default: return NULL;
    }
}

static int run_defence_probe(int npc_type, int npc_x, uint32_t seed,
                             FcAttackType guarded_type, int guarded,
                             int* style, int* damage) {
    FcState state;
    make_attack_state(&state, npc_type, npc_x);
    state.player.defence_stab = -64;
    state.player.defence_slash = -64;
    state.player.defence_crush = -64;
    state.player.defence_ranged = -64;
    state.player.defence_magic = -64;
    if (guarded) *selected_defence_field(&state.player, guarded_type) = 10000000;
    fc_rng_seed(&state, seed);
    fc_npc_tick(&state, 0);
    if (state.player.num_pending_hits != 1) {
        fc_destroy(&state);
        return 1;
    }
    *style = state.player.pending_hits[0].attack_style;
    *damage = state.player.pending_hits[0].damage;
    fc_destroy(&state);
    return 0;
}

static int test_def_004(void) {
    static const struct {
        int npc_type;
        int npc_x;
        int broad_style;
        FcAttackType exact_type;
        const char* label;
    } cases[] = {
        {NPC_TZ_KIH, 11, ATTACK_MELEE, FC_ATTACK_TYPE_STAB,
         "Tz-Kih stab"},
        {NPC_KET_ZEK, 11, ATTACK_MELEE, FC_ATTACK_TYPE_STAB,
         "Ket-Zek melee stab"},
        {NPC_KET_ZEK, 11, ATTACK_MAGIC, FC_ATTACK_TYPE_MAGIC,
         "Ket-Zek Magic"},
        {NPC_TZTOK_JAD, 11, ATTACK_MELEE, FC_ATTACK_TYPE_STAB,
         "Jad melee stab"},
        {NPC_TZTOK_JAD, 11, ATTACK_RANGED, FC_ATTACK_TYPE_RANGED,
         "Jad Ranged"},
        {NPC_TZTOK_JAD, 11, ATTACK_MAGIC, FC_ATTACK_TYPE_MAGIC,
         "Jad Magic"},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int observed = 0;
        int permissive_nonzero = 0;
        int guarded_nonzero = 0;
        for (uint32_t seed = 1; seed <= 512; seed++) {
            int permissive_style;
            int permissive_damage;
            int guarded_style;
            int guarded_damage;
            if (run_defence_probe(cases[i].npc_type, cases[i].npc_x, seed,
                                  cases[i].exact_type, 0,
                                  &permissive_style, &permissive_damage) ||
                run_defence_probe(cases[i].npc_type, cases[i].npc_x, seed,
                                  cases[i].exact_type, 1,
                                  &guarded_style, &guarded_damage)) {
                fprintf(stderr,
                        "FAIL DEF-004: %s did not queue exactly one real attack\n",
                        cases[i].label);
                return 1;
            }
            if (permissive_style != guarded_style) {
                fprintf(stderr,
                        "FAIL DEF-004: defence field changed %s style selection\n",
                        cases[i].label);
                return 1;
            }
            if (permissive_style != cases[i].broad_style) continue;
            observed++;
            if (permissive_damage > 0) permissive_nonzero++;
            if (guarded_damage > 0) guarded_nonzero++;
        }

        if (observed == 0 || permissive_nonzero == 0) {
            fprintf(stderr,
                    "FAIL DEF-004: fixed corpus did not exercise %s\n",
                    cases[i].label);
            return 1;
        }
        if (guarded_nonzero != 0) {
            fprintf(stderr,
                    "FAIL DEF-004: %s ignored its exact defence field (%d/%d guarded attacks dealt nonzero damage)\n",
                    cases[i].label, guarded_nonzero, observed);
            return 1;
        }
    }

    printf("PASS DEF-004: exact attack type reaches defence while pending hits retain broad style\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <npc_004|npc_005|def_004>\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "npc_004") == 0) return test_npc_004();
    if (strcmp(argv[1], "npc_005") == 0) return test_npc_005();
    if (strcmp(argv[1], "def_004") == 0) return test_def_004();

    fprintf(stderr, "unknown parity integration test: %s\n", argv[1]);
    return 2;
}
