#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_player_init.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int max_hp;
    int attack_style;
    int attack_speed;
    int attack_range;
    int melee_max_hit_tenths;
    int ranged_max_hit_tenths;
    int magic_max_hit_tenths;
    int att_level;
    int ranged_level;
    int magic_level;
    int att_bonus;
    int def_level;
    int ranged_def_bonus;
    int melee_attack_type;
    int size;
    int movement_speed;
    int prayer_drain;
    int heal_amount;
    int heal_interval;
} ExpectedNpcStats;

static const ExpectedNpcStats EXPECTED_NPCS[NPC_TYPE_COUNT] = {
    [NPC_NONE] = {0},
    [NPC_TZ_KIH] = {
        100, ATTACK_MELEE, 4, 1, 40, 0, 0,
        20, 30, 15, 0, 15, 0, FC_ATTACK_TYPE_STAB,
        1, 1, 10, 0, 0,
    },
    [NPC_TZ_KEK] = {
        200, ATTACK_MELEE, 4, 1, 70, 0, 0,
        40, 60, 30, 0, 30, 0, FC_ATTACK_TYPE_CRUSH,
        2, 1, 0, 0, 0,
    },
    [NPC_TZ_KEK_SM] = {
        100, ATTACK_MELEE, 4, 1, 40, 0, 0,
        20, 30, 15, 0, 15, 0, FC_ATTACK_TYPE_CRUSH,
        1, 1, 0, 0, 0,
    },
    [NPC_TOK_XIL] = {
        400, ATTACK_RANGED, 4, 14, 130, 130, 0,
        80, 120, 60, 0, 60, 0, FC_ATTACK_TYPE_CRUSH,
        3, 1, 0, 0, 0,
    },
    [NPC_YT_MEJKOT] = {
        800, ATTACK_MELEE, 4, 1, 250, 0, 0,
        160, 240, 120, 0, 120, 0, FC_ATTACK_TYPE_CRUSH,
        4, 1, 0, 100, 0,
    },
    [NPC_KET_ZEK] = {
        1600, ATTACK_MAGIC, 4, 14, 550, 0, 520,
        320, 480, 240, 0, 240, 0, FC_ATTACK_TYPE_STAB,
        5, 1, 0, 0, 0,
    },
    [NPC_TZTOK_JAD] = {
        2500, ATTACK_MAGIC, 8, 14, 970, 970, 950,
        640, 960, 480, 0, 480, 0, FC_ATTACK_TYPE_STAB,
        5, 1, 0, 0, 0,
    },
    [NPC_YT_HURKOT] = {
        600, ATTACK_MELEE, 4, 1, 140, 0, 0,
        140, 120, 120, 0, 60, 100, FC_ATTACK_TYPE_CRUSH,
        1, 1, 0, 50, 4,
    },
};

static int fail_int(const char* test, int npc_type, const char* field,
                    int actual, int expected) {
    fprintf(stderr,
            "FAIL %s: NPC type %d field %s=%d, expected %d\n",
            test, npc_type, field, actual, expected);
    return 1;
}

#define CHECK_NPC_FIELD(test_name, npc_type, actual, expected, field) \
    do { \
        if ((actual) != (expected)) \
            return fail_int((test_name), (npc_type), (field), \
                            (actual), (expected)); \
    } while (0)

static int test_npc_001(void) {
    const FcNpcStats zero = {0};
    const FcNpcStats* none = fc_npc_get_stats(NPC_NONE);

    if (memcmp(none, &zero, sizeof(zero)) != 0) {
        fprintf(stderr, "FAIL NPC-001: NPC_NONE is not an all-zero sentinel\n");
        return 1;
    }
    if (fc_npc_get_stats(-1) != none ||
        fc_npc_get_stats(NPC_TYPE_COUNT) != none ||
        fc_npc_get_stats(NPC_TYPE_COUNT + 100) != none) {
        fprintf(stderr, "FAIL NPC-001: invalid types do not return the sentinel\n");
        return 1;
    }

    for (int type = NPC_TZ_KIH; type < NPC_TYPE_COUNT; type++) {
        const FcNpcStats* actual = fc_npc_get_stats(type);
        const ExpectedNpcStats* expected = &EXPECTED_NPCS[type];

        CHECK_NPC_FIELD("NPC-001", type, actual->max_hp,
                        expected->max_hp, "max_hp");
        CHECK_NPC_FIELD("NPC-001", type, actual->attack_style,
                        expected->attack_style, "attack_style");
        CHECK_NPC_FIELD("NPC-001", type, actual->attack_speed,
                        expected->attack_speed, "attack_speed");
        CHECK_NPC_FIELD("NPC-001", type, actual->attack_range,
                        expected->attack_range, "attack_range");
        CHECK_NPC_FIELD("NPC-001", type, actual->melee_max_hit_tenths,
                        expected->melee_max_hit_tenths,
                        "melee_max_hit_tenths");
        CHECK_NPC_FIELD("NPC-001", type, actual->ranged_max_hit_tenths,
                        expected->ranged_max_hit_tenths,
                        "ranged_max_hit_tenths");
        CHECK_NPC_FIELD("NPC-001", type, actual->magic_max_hit_tenths,
                        expected->magic_max_hit_tenths,
                        "magic_max_hit_tenths");
        CHECK_NPC_FIELD("NPC-001", type, actual->att_level,
                        expected->att_level, "att_level");
        CHECK_NPC_FIELD("NPC-001", type, actual->ranged_level,
                        expected->ranged_level, "ranged_level");
        CHECK_NPC_FIELD("NPC-001", type, actual->magic_level,
                        expected->magic_level, "magic_level");
        CHECK_NPC_FIELD("NPC-001", type, actual->att_bonus,
                        expected->att_bonus, "att_bonus");
        CHECK_NPC_FIELD("NPC-001", type, actual->def_level,
                        expected->def_level, "def_level");
        CHECK_NPC_FIELD("NPC-001", type, actual->ranged_def_bonus,
                        expected->ranged_def_bonus, "ranged_def_bonus");
        CHECK_NPC_FIELD("NPC-001", type, actual->melee_attack_type,
                        expected->melee_attack_type, "melee_attack_type");
        CHECK_NPC_FIELD("NPC-001", type, actual->size,
                        expected->size, "size");
        CHECK_NPC_FIELD("NPC-001", type, actual->movement_speed,
                        expected->movement_speed, "movement_speed");
        CHECK_NPC_FIELD("NPC-001", type, actual->prayer_drain,
                        expected->prayer_drain, "prayer_drain");
        CHECK_NPC_FIELD("NPC-001", type, actual->heal_amount,
                        expected->heal_amount, "heal_amount");
        CHECK_NPC_FIELD("NPC-001", type, actual->heal_interval,
                        expected->heal_interval, "heal_interval");

        if (!fc_npc_stats_valid(actual)) {
            fprintf(stderr,
                    "FAIL NPC-001: production table row %d fails validation\n",
                    type);
            return 1;
        }
    }

    const int expected_magic[] = {15, 30, 15, 60, 120, 240, 480, 120};
    for (int i = 0; i < 8; i++) {
        int type = NPC_TZ_KIH + i;
        CHECK_NPC_FIELD("NPC-001", type,
                        fc_npc_get_stats(type)->magic_level,
                        expected_magic[i], "enum_order_magic_level");
    }

    FcNpcStats invalid = *fc_npc_get_stats(NPC_TZ_KIH);
    invalid.melee_max_hit_tenths = -10;
    if (fc_npc_stats_valid(&invalid)) {
        fprintf(stderr, "FAIL NPC-001: negative maximum passed validation\n");
        return 1;
    }
    invalid = *fc_npc_get_stats(NPC_TZ_KIH);
    invalid.melee_max_hit_tenths = 41;
    if (fc_npc_stats_valid(&invalid)) {
        fprintf(stderr,
                "FAIL NPC-001: non-integral tenths maximum passed validation\n");
        return 1;
    }

    printf("PASS NPC-001: exact NPC table and release validator\n");
    return 0;
}

static int test_npc_002(void) {
    for (int type = NPC_NONE; type < NPC_TYPE_COUNT; type++) {
        const FcNpcStats* stats = fc_npc_get_stats(type);
        const int expected_tenths[4] = {
            0,
            EXPECTED_NPCS[type].melee_max_hit_tenths,
            EXPECTED_NPCS[type].ranged_max_hit_tenths,
            EXPECTED_NPCS[type].magic_max_hit_tenths,
        };
        for (int style = ATTACK_MELEE; style <= ATTACK_MAGIC; style++) {
            int actual_tenths =
                fc_npc_max_hit_tenths_for_style(stats, style);
            int actual_hp = fc_npc_max_hit_hp_for_style(stats, style);
            if (actual_tenths != expected_tenths[style]) {
                return fail_int("NPC-002", type, "style maximum tenths",
                                actual_tenths, expected_tenths[style]);
            }
            if (actual_hp != expected_tenths[style] / 10) {
                return fail_int("NPC-002", type, "style maximum HP",
                                actual_hp, expected_tenths[style] / 10);
            }
        }
    }

    FcNpcStats invalid = *fc_npc_get_stats(NPC_TZ_KIH);
    invalid.melee_max_hit_tenths = 41;
    if (fc_npc_max_hit_hp_for_style(&invalid, ATTACK_MELEE) != 0 ||
        fc_npc_max_hit_tenths_for_style(NULL, ATTACK_MELEE) != 0 ||
        fc_npc_max_hit_hp_for_style(NULL, ATTACK_MELEE) != 0 ||
        fc_npc_max_hit_tenths_for_style(&invalid, ATTACK_NONE) != 0 ||
        fc_npc_max_hit_tenths_for_style(&invalid, 99) != 0) {
        fprintf(stderr,
                "FAIL NPC-002: invalid or unsupported maximum was not rejected\n");
        return 1;
    }

    printf("PASS NPC-002: unit-named style maximum accessors\n");
    return 0;
}

typedef struct {
    int npc_type;
    int attack_style;
    int expected_roll;
} AttackRollCase;

static int level_for_style(const FcNpcStats* stats, int style) {
    if (style == ATTACK_MELEE) return stats->att_level;
    if (style == ATTACK_RANGED) return stats->ranged_level;
    if (style == ATTACK_MAGIC) return stats->magic_level;
    return 0;
}

static int test_npc_003(void) {
    static const AttackRollCase cases[] = {
        {NPC_TZ_KIH, ATTACK_MELEE, 1856},
        {NPC_TZ_KEK, ATTACK_MELEE, 3136},
        {NPC_TZ_KEK_SM, ATTACK_MELEE, 1856},
        {NPC_TOK_XIL, ATTACK_MELEE, 5696},
        {NPC_TOK_XIL, ATTACK_RANGED, 8256},
        {NPC_YT_MEJKOT, ATTACK_MELEE, 10816},
        {NPC_KET_ZEK, ATTACK_MELEE, 21056},
        {NPC_KET_ZEK, ATTACK_MAGIC, 15936},
        {NPC_TZTOK_JAD, ATTACK_MELEE, 41536},
        {NPC_TZTOK_JAD, ATTACK_RANGED, 62016},
        {NPC_TZTOK_JAD, ATTACK_MAGIC, 31296},
        {NPC_YT_HURKOT, ATTACK_MELEE, 9536},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const AttackRollCase* c = &cases[i];
        const FcNpcStats* stats = fc_npc_get_stats(c->npc_type);
        int actual = fc_npc_attack_roll(level_for_style(stats, c->attack_style),
                                        stats->att_bonus);
        if (actual != c->expected_roll) {
            return fail_int("NPC-003", c->npc_type, "offensive roll",
                            actual, c->expected_roll);
        }
    }

    const FcNpcStats* stats = fc_npc_get_stats(NPC_TZTOK_JAD);
    const int base[3] = {
        fc_npc_attack_roll(stats->att_level, stats->att_bonus),
        fc_npc_attack_roll(stats->ranged_level, stats->att_bonus),
        fc_npc_attack_roll(stats->magic_level, stats->att_bonus),
    };
    const int levels[3] = {stats->att_level, stats->ranged_level,
                           stats->magic_level};
    for (int changed = 0; changed < 3; changed++) {
        for (int observed = 0; observed < 3; observed++) {
            int level = levels[observed] + (changed == observed ? 1 : 0);
            int roll = fc_npc_attack_roll(level, stats->att_bonus);
            int expected = base[observed] +
                (changed == observed ? stats->att_bonus + 64 : 0);
            if (roll != expected) {
                fprintf(stderr,
                        "FAIL NPC-003: changing level %d altered roll %d incorrectly\n",
                        changed, observed);
                return 1;
            }
        }
    }

    printf("PASS NPC-003: exact style-specific NPC offensive rolls\n");
    return 0;
}

static FcPlayer sota_player(void) {
    FcPlayer player;
    memset(&player, 0, sizeof(player));
    player.defence_level = 99;
    player.magic_level = 1;
    player.defence_stab = 116;
    player.defence_slash = 106;
    player.defence_crush = 129;
    player.defence_magic = 150;
    player.defence_ranged = 121;
    return player;
}

static int test_def_001(void) {
    FcPlayer player = sota_player();
    static const struct {
        FcAttackType type;
        int expected;
    } cases[] = {
        {FC_ATTACK_TYPE_STAB, 19260},
        {FC_ATTACK_TYPE_SLASH, 18190},
        {FC_ATTACK_TYPE_CRUSH, 20651},
        {FC_ATTACK_TYPE_RANGED, 19795},
        {FC_ATTACK_TYPE_MAGIC, 7918},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int actual = fc_player_def_roll(&player, cases[i].type);
        if (actual != cases[i].expected) {
            return fail_int("DEF-001", 0, "SOTA defence roll",
                            actual, cases[i].expected);
        }
    }

    printf("PASS DEF-001: exact SOTA defence rolls\n");
    return 0;
}

static int* defence_field(FcPlayer* player, int index) {
    switch (index) {
        case 0: return &player->defence_stab;
        case 1: return &player->defence_slash;
        case 2: return &player->defence_crush;
        case 3: return &player->defence_ranged;
        case 4: return &player->defence_magic;
        default: return NULL;
    }
}

static FcAttackType defence_type(int index) {
    static const FcAttackType types[5] = {
        FC_ATTACK_TYPE_STAB,
        FC_ATTACK_TYPE_SLASH,
        FC_ATTACK_TYPE_CRUSH,
        FC_ATTACK_TYPE_RANGED,
        FC_ATTACK_TYPE_MAGIC,
    };
    return types[index];
}

static int test_def_002(void) {
    FcPlayer base;
    memset(&base, 0, sizeof(base));
    base.defence_level = 70;
    base.magic_level = 50;
    base.defence_stab = -20;
    base.defence_slash = -10;
    base.defence_crush = 0;
    base.defence_ranged = 10;
    base.defence_magic = 20;

    const int effective[5] = {78, 78, 78, 78, 64};
    int original[5];
    for (int type = 0; type < 5; type++) {
        original[type] = fc_player_def_roll(&base, defence_type(type));
        int bonus = *defence_field(&base, type);
        int expected = effective[type] * (bonus + 64);
        if (original[type] != expected) {
            return fail_int("DEF-002", 0, "isolated base roll",
                            original[type], expected);
        }
    }

    for (int changed = 0; changed < 5; changed++) {
        FcPlayer modified = base;
        (*defence_field(&modified, changed))++;
        for (int observed = 0; observed < 5; observed++) {
            int actual = fc_player_def_roll(&modified, defence_type(observed));
            int expected = original[observed] +
                (changed == observed ? effective[observed] : 0);
            if (actual != expected) {
                fprintf(stderr,
                        "FAIL DEF-002: field %d changed incoming type %d from %d to %d, expected %d\n",
                        changed, observed, original[observed], actual, expected);
                return 1;
            }
        }
    }

    printf("PASS DEF-002: equipment defence fields are isolated\n");
    return 0;
}

static int test_def_003(void) {
    FcPlayer player;
    memset(&player, 0, sizeof(player));
    player.defence_level = 50;
    player.magic_level = 10;
    if (fc_player_def_roll(&player, FC_ATTACK_TYPE_STAB) != 58 * 64) {
        fprintf(stderr,
                "FAIL DEF-003: physical defence does not use Defence + 8\n");
        return 1;
    }
    player.magic_level = 99;
    if (fc_player_def_roll(&player, FC_ATTACK_TYPE_STAB) != 58 * 64) {
        fprintf(stderr, "FAIL DEF-003: physical defence depends on Magic\n");
        return 1;
    }

    player = sota_player();
    if (fc_player_def_roll(&player, FC_ATTACK_TYPE_MAGIC) != 7918) {
        fprintf(stderr,
                "FAIL DEF-003: 99 Defence/1 Magic did not truncate to effective level 37\n");
        return 1;
    }

    memset(&player, 0, sizeof(player));
    player.defence_level = 1;
    player.magic_level = 1;
    if (fc_player_def_roll(&player, FC_ATTACK_TYPE_MAGIC) != 8 * 64) {
        fprintf(stderr,
                "FAIL DEF-003: 1 Defence/1 Magic did not produce effective level 8\n");
        return 1;
    }
    player.defence_level = 90;
    player.magic_level = 90;
    if (fc_player_def_roll(&player, FC_ATTACK_TYPE_MAGIC) != 98 * 64) {
        fprintf(stderr,
                "FAIL DEF-003: exact 90/90 Magic-defence vector is wrong\n");
        return 1;
    }

    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        const FcLoadout* loadout = &FC_LOADOUTS[i];
        if (loadout->attack_lvl < 1 || loadout->strength_lvl < 1 ||
            loadout->defence_lvl < 1 || loadout->ranged_lvl < 1 ||
            loadout->prayer_lvl < 1 || loadout->magic_lvl < 1) {
            fprintf(stderr,
                    "FAIL DEF-003: loadout %d contains a combat skill below 1\n",
                    i);
            return 1;
        }
    }

    printf("PASS DEF-003: skill isolation and separate Magic truncation\n");
    return 0;
}

static int test_def_005(void) {
    const float eps = 1.0e-6f;
    const float high_branch = fc_hit_chance(100, 50);
    const float equal_branch = fc_hit_chance(100, 100);

    if (fabsf(high_branch - 0.742574257f) > eps) {
        fprintf(stderr, "FAIL DEF-005: attack>defence branch changed: %.9f\n",
                high_branch);
        return 1;
    }
    if (fabsf(equal_branch - 0.495049505f) > eps) {
        fprintf(stderr, "FAIL DEF-005: equality branch changed: %.9f\n",
                equal_branch);
        return 1;
    }
    if (fc_hit_chance(0, 0) != 0.0f || fc_hit_chance(0, 100) != 0.0f) {
        fprintf(stderr, "FAIL DEF-005: zero attack roll has nonzero chance\n");
        return 1;
    }

    for (int attack = 0; attack <= 200; attack++) {
        float previous_defence = 1.0f;
        for (int defence = 0; defence <= 200; defence++) {
            float chance = fc_hit_chance(attack, defence);
            if (!isfinite(chance) || chance < 0.0f || chance > 1.0f) {
                fprintf(stderr,
                        "FAIL DEF-005: chance out of range at attack=%d defence=%d\n",
                        attack, defence);
                return 1;
            }
            if (chance > previous_defence + eps) {
                fprintf(stderr,
                        "FAIL DEF-005: raising defence increased hit chance\n");
                return 1;
            }
            previous_defence = chance;
        }
    }
    for (int defence = 0; defence <= 200; defence++) {
        float previous_attack = 0.0f;
        for (int attack = 0; attack <= 200; attack++) {
            float chance = fc_hit_chance(attack, defence);
            if (chance + eps < previous_attack) {
                fprintf(stderr,
                        "FAIL DEF-005: raising attack reduced hit chance\n");
                return 1;
            }
            previous_attack = chance;
        }
    }

    printf("PASS DEF-005: shared hit-chance formula preserved\n");
    return 0;
}

typedef struct {
    int max_hp;
    int max_prayer;
    int attack_level;
    int strength_level;
    int defence_level;
    int ranged_level;
    int prayer_level;
    int magic_level;
    int weapon_kind;
    int weapon_uses_ammo;
    int crystal_piece_mask;
    int weapon_speed;
    int weapon_range;
    int ranged_attack_bonus;
    int ranged_strength_bonus;
    int defence_stab;
    int defence_slash;
    int defence_crush;
    int defence_magic;
    int defence_ranged;
    int prayer_bonus;
    int ammo;
    int base_attack_roll;
    int base_max_hit_hp;
} ExpectedLoadout;

static const ExpectedLoadout EXPECTED_LOADOUTS[FC_NUM_LOADOUTS] = {
    [FC_LOADOUT_BLACK_DHIDE_RCB] = {
        700, 430, 1, 1, 70, 70, 43, 1,
        FC_WEAPON_GENERIC_RANGED, 1, 0, 5, 7, 153, 100,
        97, 84, 110, 91, 90, 0, 50000, 16926, 20,
    },
    [FC_LOADOUT_SOTA_TBOW] = {
        990, 990, 1, 1, 99, 99, 99, 1,
        FC_WEAPON_TWISTED_BOW, 1, 0, 5, 10, 215, 99,
        116, 106, 129, 150, 121, 6, 50000, 29853, 27,
    },
    [FC_LOADOUT_LOW_DEF_RCB] = {
        550, 430, 1, 1, 1, 61, 43, 1,
        FC_WEAPON_GENERIC_RANGED, 1, 0, 5, 7, 166, 100,
        48, 49, 62, 42, 46, 8, 50000, 15870, 18,
    },
    [FC_LOADOUT_RCB_PURE] = {
        550, 430, 1, 1, 1, 61, 43, 1,
        FC_WEAPON_GENERIC_RANGED, 1, 0, 5, 7, 166, 100,
        48, 49, 62, 42, 46, 8, 50000, 15870, 18,
    },
    [FC_LOADOUT_MSBI_PURE] = {
        600, 430, 1, 1, 1, 70, 43, 1,
        FC_WEAPON_GENERIC_RANGED, 1, 0, 3, 7, 141, 49,
        48, 49, 62, 42, 46, 3, 50000, 15990, 14,
    },
    [FC_LOADOUT_BLOWPIPE_PURE] = {
        750, 430, 1, 1, 1, 75, 43, 1,
        FC_WEAPON_GENERIC_RANGED, 1, 0, 2, 5, 101, 42,
        45, 46, 59, 39, 43, 2, 50000, 13695, 14,
    },
    [FC_LOADOUT_ACB_ARMADYL] = {
        800, 700, 1, 1, 75, 80, 70, 1,
        FC_WEAPON_GENERIC_RANGED, 1, 0, 5, 8, 220, 129,
        112, 100, 123, 139, 117, 11, 50000, 24992, 27,
    },
    [FC_LOADOUT_BOWFA_CRYSTAL] = {
        850, 700, 1, 1, 75, 85, 70, 1,
        FC_WEAPON_BOW_OF_FAERDHINEN, 0, FC_CRYSTAL_PIECE_ALL,
        4, 10, 233, 113, 102, 85, 110, 107, 143, 9, 0, 27621, 26,
    },
    [FC_LOADOUT_TBOW_MASORI] = {
        990, 770, 1, 1, 80, 99, 77, 1,
        FC_WEAPON_TWISTED_BOW, 1, 0, 5, 10, 205, 97,
        116, 106, 129, 150, 121, 6, 50000, 28783, 27,
    },
};

static void player_from_loadout(FcPlayer* player, const FcLoadout* loadout) {
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

static FcNpc target_of_type(int npc_type) {
    FcNpc target;
    memset(&target, 0, sizeof(target));
    target.npc_type = npc_type;
    return target;
}

static int test_rng_001(void) {
    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        FcPlayer player;
        player_from_loadout(&player, &FC_LOADOUTS[i]);
        int attack = fc_player_ranged_base_attack_roll(&player);
        int maximum = fc_player_ranged_base_max_hit_hp(&player);
        if (attack != EXPECTED_LOADOUTS[i].base_attack_roll ||
            maximum != EXPECTED_LOADOUTS[i].base_max_hit_hp) {
            fprintf(stderr,
                    "FAIL RNG-001: loadout %d base vector %d/%d, expected %d/%d\n",
                    i, attack, maximum,
                    EXPECTED_LOADOUTS[i].base_attack_roll,
                    EXPECTED_LOADOUTS[i].base_max_hit_hp);
            return 1;
        }
    }

    int below_half_seen = 0;
    int at_half_seen = 0;
    FcPlayer custom;
    memset(&custom, 0, sizeof(custom));
    for (int level = 0; level <= 180; level++) {
        for (int bonus = -63; bonus <= 260; bonus++) {
            int effective = level + 8;
            int product = effective * (bonus + 64);
            int expected = (product + 320) / 640;
            int remainder = product % 640;
            custom.ranged_level = level;
            custom.ranged_strength_bonus = bonus;
            int actual = fc_player_ranged_base_max_hit_hp(&custom);
            if (actual != expected) {
                fprintf(stderr,
                        "FAIL RNG-001: rounding boundary level=%d bonus=%d produced %d, expected %d\n",
                        level, bonus, actual, expected);
                return 1;
            }
            if (remainder == 319) below_half_seen = 1;
            if (remainder == 320) at_half_seen = 1;
        }
    }
    if (!below_half_seen || !at_half_seen) {
        fprintf(stderr,
                "FAIL RNG-001: custom corpus missed one side of the half-up boundary\n");
        return 1;
    }

    printf("PASS RNG-001: exact raw Ranged vectors and integer rounding\n");
    return 0;
}

static int tbow_accuracy_oracle(int magic_level) {
    int64_t magic = magic_level;
    if (magic < 0) magic = 0;
    if (magic > 250) magic = 250;
    int64_t inner = 3 * magic / 10;
    int64_t delta = inner - 100;
    int64_t pct = 140 + (3 * magic - 10) / 100 - delta * delta / 100;
    if (pct < 0) pct = 0;
    if (pct > 140) pct = 140;
    return (int)pct;
}

static int tbow_damage_oracle(int magic_level) {
    int64_t magic = magic_level;
    if (magic < 0) magic = 0;
    if (magic > 250) magic = 250;
    int64_t inner = 3 * magic / 10;
    int64_t delta = inner - 140;
    int64_t pct = 250 + (3 * magic - 14) / 100 - delta * delta / 100;
    if (pct < 0) pct = 0;
    if (pct > 250) pct = 250;
    return (int)pct;
}

static int test_tbow_001(void) {
    static const struct {
        int magic;
        int npc_type;
        int accuracy_pct;
        int damage_pct;
        int sota_attack;
        int sota_max;
    } vectors[] = {
        {0, NPC_NONE, 40, 54, 11941, 14},
        {15, NPC_TZ_KIH, 48, 66, 14329, 17},
        {30, NPC_TZ_KEK, 58, 79, 17314, 21},
        {60, NPC_TOK_XIL, 74, 103, 22091, 27},
        {120, NPC_YT_MEJKOT, 103, 145, 30748, 39},
        {240, NPC_KET_ZEK, 140, 211, 41794, 56},
        {250, NPC_TZTOK_JAD, 140, 215, 41794, 58},
        {480, NPC_TZTOK_JAD, 140, 215, 41794, 58},
    };
    static const int masori_attack[] = {
        11513, 13815, 16694, 21299, 29646, 40296, 40296, 40296,
    };
    static const int masori_max[] = {14, 17, 21, 27, 39, 56, 58, 58};

    FcPlayer sota;
    FcPlayer masori;
    player_from_loadout(&sota, &FC_LOADOUTS[FC_LOADOUT_SOTA_TBOW]);
    player_from_loadout(&masori, &FC_LOADOUTS[FC_LOADOUT_TBOW_MASORI]);

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        int accuracy_pct = fc_tbow_accuracy_multiplier_pct(vectors[i].magic);
        int damage_pct = fc_tbow_damage_multiplier_pct(vectors[i].magic);
        if (accuracy_pct != vectors[i].accuracy_pct ||
            damage_pct != vectors[i].damage_pct) {
            fprintf(stderr,
                    "FAIL TBOW-001: Magic %d multipliers %d/%d, expected %d/%d\n",
                    vectors[i].magic, accuracy_pct, damage_pct,
                    vectors[i].accuracy_pct, vectors[i].damage_pct);
            return 1;
        }

        FcNpc target = target_of_type(vectors[i].npc_type);
        int sota_attack = fc_player_ranged_attack_roll(&sota, &target);
        int sota_max = fc_player_ranged_final_max_hit_hp(&sota, &target);
        int masori_attack_actual = fc_player_ranged_attack_roll(&masori, &target);
        int masori_max_actual = fc_player_ranged_final_max_hit_hp(&masori, &target);
        if (sota_attack != vectors[i].sota_attack ||
            sota_max != vectors[i].sota_max ||
            masori_attack_actual != masori_attack[i] ||
            masori_max_actual != masori_max[i]) {
            fprintf(stderr,
                    "FAIL TBOW-001: Magic %d final SOTA %d/%d Masori %d/%d\n",
                    vectors[i].magic, sota_attack, sota_max,
                    masori_attack_actual, masori_max_actual);
            return 1;
        }
    }

    printf("PASS TBOW-001: exact target-Magic multiplier vectors\n");
    return 0;
}

static int test_tbow_002(void) {
    static const int edge_inputs[] = {-1000, -1, 501, INT_MAX};
    for (int magic = 0; magic <= 500; magic++) {
        int accuracy = fc_tbow_accuracy_multiplier_pct(magic);
        int damage = fc_tbow_damage_multiplier_pct(magic);
        if (accuracy != tbow_accuracy_oracle(magic) ||
            damage != tbow_damage_oracle(magic)) {
            fprintf(stderr,
                    "FAIL TBOW-002: Magic %d staged oracle %d/%d, production %d/%d\n",
                    magic, tbow_accuracy_oracle(magic),
                    tbow_damage_oracle(magic), accuracy, damage);
            return 1;
        }
    }
    for (size_t i = 0; i < sizeof(edge_inputs) / sizeof(edge_inputs[0]); i++) {
        int magic = edge_inputs[i];
        if (fc_tbow_accuracy_multiplier_pct(magic) !=
                tbow_accuracy_oracle(magic) ||
            fc_tbow_damage_multiplier_pct(magic) !=
                tbow_damage_oracle(magic)) {
            fprintf(stderr,
                    "FAIL TBOW-002: clamped edge input %d disagrees with staged oracle\n",
                    magic);
            return 1;
        }
    }

    printf("PASS TBOW-002: exhaustive staged-integer oracle\n");
    return 0;
}

static int test_tbow_003(void) {
    FcNpc low = target_of_type(NPC_TZ_KIH);
    FcNpc high = target_of_type(NPC_TZTOK_JAD);

    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        FcPlayer player;
        player_from_loadout(&player, &FC_LOADOUTS[i]);
        int low_attack = fc_player_ranged_attack_roll(&player, &low);
        int high_attack = fc_player_ranged_attack_roll(&player, &high);
        int low_max = fc_player_ranged_final_max_hit_hp(&player, &low);
        int high_max = fc_player_ranged_final_max_hit_hp(&player, &high);
        int is_tbow = player.weapon_kind == FC_WEAPON_TWISTED_BOW;
        if (is_tbow ? (low_attack == high_attack || low_max == high_max)
                    : (low_attack != high_attack || low_max != high_max)) {
            fprintf(stderr,
                    "FAIL TBOW-003: loadout %d target-Magic gate produced low/high %d/%d and %d/%d\n",
                    i, low_attack, high_attack, low_max, high_max);
            return 1;
        }
    }

    FcPlayer tbow;
    player_from_loadout(&tbow, &FC_LOADOUTS[FC_LOADOUT_SOTA_TBOW]);
    if (fc_player_ranged_attack_roll(&tbow, NULL) != 29853 ||
        fc_player_ranged_final_max_hit_hp(&tbow, NULL) != 27) {
        fprintf(stderr, "FAIL TBOW-003: targetless TBow did not return raw values\n");
        return 1;
    }
    tbow.crystal_piece_mask = FC_CRYSTAL_PIECE_ALL;
    if (fc_player_ranged_attack_roll(&tbow, &high) != 41794 ||
        fc_player_ranged_final_max_hit_hp(&tbow, &high) != 58) {
        fprintf(stderr,
                "FAIL TBOW-003: artificial crystal metadata changed TBow-only effect\n");
        return 1;
    }

    FcPlayer generic;
    player_from_loadout(&generic, &FC_LOADOUTS[FC_LOADOUT_BLACK_DHIDE_RCB]);
    generic.crystal_piece_mask = FC_CRYSTAL_PIECE_ALL;
    if (fc_player_ranged_attack_roll(&generic, &low) !=
            fc_player_ranged_attack_roll(&generic, &high) ||
        fc_player_ranged_final_max_hit_hp(&generic, &low) !=
            fc_player_ranged_final_max_hit_hp(&generic, &high)) {
        fprintf(stderr,
                "FAIL TBOW-003: non-TBow crystal metadata entered target-Magic branch\n");
        return 1;
    }

    printf("PASS TBOW-003: target-Magic and effect gates are isolated\n");
    return 0;
}

static void crystal_basis_points(int mask, int* accuracy, int* damage) {
    int valid = mask & FC_CRYSTAL_PIECE_ALL;
    *accuracy = 0;
    *damage = 0;
    if (valid & FC_CRYSTAL_PIECE_HELM) {
        *accuracy += FC_CRYSTAL_HELM_ACCURACY_BP;
        *damage += FC_CRYSTAL_HELM_DAMAGE_BP;
    }
    if (valid & FC_CRYSTAL_PIECE_BODY) {
        *accuracy += FC_CRYSTAL_BODY_ACCURACY_BP;
        *damage += FC_CRYSTAL_BODY_DAMAGE_BP;
    }
    if (valid & FC_CRYSTAL_PIECE_LEGS) {
        *accuracy += FC_CRYSTAL_LEGS_ACCURACY_BP;
        *damage += FC_CRYSTAL_LEGS_DAMAGE_BP;
    }
}

static int crystal_expected_attack(int base, int mask) {
    int accuracy;
    int damage;
    crystal_basis_points(mask, &accuracy, &damage);
    (void)damage;
    return (int)((int64_t)base * (10000 + accuracy) / 10000);
}

static int crystal_expected_max(int base, int mask) {
    int accuracy;
    int damage;
    crystal_basis_points(mask, &accuracy, &damage);
    (void)accuracy;
    return (int)((int64_t)base * (10000 + damage) / 10000);
}

static int test_cry_001(void) {
    static const int expected_attack[8] = {
        27621, 29002, 31764, 33145, 30383, 31764, 34526, 35907,
    };
    static const int expected_max[8] = {26, 26, 27, 28, 27, 27, 29, 29};
    FcPlayer bowfa;
    player_from_loadout(&bowfa, &FC_LOADOUTS[FC_LOADOUT_BOWFA_CRYSTAL]);

    for (int mask = 0; mask <= FC_CRYSTAL_PIECE_ALL; mask++) {
        bowfa.crystal_piece_mask = mask;
        int attack = fc_player_ranged_attack_roll(&bowfa, NULL);
        int maximum = fc_player_ranged_final_max_hit_hp(&bowfa, NULL);
        if (attack != expected_attack[mask] || maximum != expected_max[mask]) {
            fprintf(stderr,
                    "FAIL CRY-001: mask %d produced %d/%d, expected %d/%d\n",
                    mask, attack, maximum,
                    expected_attack[mask], expected_max[mask]);
            return 1;
        }
    }

    printf("PASS CRY-001: all crystal-piece masks\n");
    return 0;
}

static int test_cry_002(void) {
    FcNpc low = target_of_type(NPC_TZ_KIH);
    FcNpc high = target_of_type(NPC_TZTOK_JAD);
    static const int kinds[] = {
        FC_WEAPON_GENERIC_RANGED,
        FC_WEAPON_TWISTED_BOW,
        FC_WEAPON_BOW_OF_FAERDHINEN,
    };

    for (size_t kind_idx = 0; kind_idx < sizeof(kinds) / sizeof(kinds[0]);
         kind_idx++) {
        for (int mask = 0; mask <= FC_CRYSTAL_PIECE_ALL; mask++) {
            FcPlayer player;
            player_from_loadout(&player, &FC_LOADOUTS[FC_LOADOUT_BOWFA_CRYSTAL]);
            player.weapon_kind = kinds[kind_idx];
            player.crystal_piece_mask = mask;
            int low_attack = fc_player_ranged_attack_roll(&player, &low);
            int high_attack = fc_player_ranged_attack_roll(&player, &high);
            int low_max = fc_player_ranged_final_max_hit_hp(&player, &low);
            int high_max = fc_player_ranged_final_max_hit_hp(&player, &high);

            if (player.weapon_kind == FC_WEAPON_BOW_OF_FAERDHINEN) {
                int expected_attack = crystal_expected_attack(27621, mask);
                int expected_max = crystal_expected_max(26, mask);
                if (low_attack != expected_attack || high_attack != expected_attack ||
                    low_max != expected_max || high_max != expected_max) {
                    fprintf(stderr,
                            "FAIL CRY-002: Bowfa mask %d target cross-product %d/%d %d/%d\n",
                            mask, low_attack, high_attack, low_max, high_max);
                    return 1;
                }
            } else if (player.weapon_kind == FC_WEAPON_GENERIC_RANGED) {
                if (low_attack != 27621 || high_attack != 27621 ||
                    low_max != 26 || high_max != 26) {
                    fprintf(stderr,
                            "FAIL CRY-002: generic weapon received crystal/target effect for mask %d\n",
                            mask);
                    return 1;
                }
            } else {
                int expected_low_attack = 27621 * 48 / 100;
                int expected_high_attack = 27621 * 140 / 100;
                int expected_low_max = 26 * 66 / 100;
                int expected_high_max = 26 * 215 / 100;
                if (low_attack != expected_low_attack ||
                    high_attack != expected_high_attack ||
                    low_max != expected_low_max || high_max != expected_high_max) {
                    fprintf(stderr,
                            "FAIL CRY-002: TBow/crystal mutual exclusion failed for mask %d\n",
                            mask);
                    return 1;
                }
            }
        }
    }

    printf("PASS CRY-002: compatible-weapon cross-product\n");
    return 0;
}

static int derived_crystal_mask(const FcLoadout* loadout) {
    int mask = 0;
    for (int i = 0; i < loadout->equipment_count; i++) {
        const FcLoadoutEquipmentItem* item = &loadout->equipment[i];
        if (item->slot == FC_EQUIP_SLOT_HEAD && item->item_id == 23971)
            mask |= FC_CRYSTAL_PIECE_HELM;
        if (item->slot == FC_EQUIP_SLOT_BODY && item->item_id == 23975)
            mask |= FC_CRYSTAL_PIECE_BODY;
        if (item->slot == FC_EQUIP_SLOT_LEGS && item->item_id == 23979)
            mask |= FC_CRYSTAL_PIECE_LEGS;
    }
    return mask;
}

static int test_cry_003(void) {
    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        int expected = i == FC_LOADOUT_BOWFA_CRYSTAL ?
            FC_CRYSTAL_PIECE_ALL : FC_CRYSTAL_PIECE_NONE;
        int derived = derived_crystal_mask(&FC_LOADOUTS[i]);
        if (derived != expected || FC_LOADOUTS[i].crystal_piece_mask != expected) {
            fprintf(stderr,
                    "FAIL CRY-003: loadout %d metadata/IDs mask %d/%d, expected %d\n",
                    i, FC_LOADOUTS[i].crystal_piece_mask, derived, expected);
            return 1;
        }
    }

    FcState state;
    fc_init(&state);
    fc_reset(&state, 123);
    if (state.player.crystal_piece_mask !=
            FC_LOADOUTS[FC_ACTIVE_LOADOUT].crystal_piece_mask) {
        fprintf(stderr,
                "FAIL CRY-003: reset copied mask %d, expected active loadout mask %d\n",
                state.player.crystal_piece_mask,
                FC_LOADOUTS[FC_ACTIVE_LOADOUT].crystal_piece_mask);
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    printf("PASS CRY-003: item IDs, metadata, and reset propagation agree\n");
    return 0;
}

static int test_cry_004(void) {
    FcPlayer bowfa;
    player_from_loadout(&bowfa, &FC_LOADOUTS[FC_LOADOUT_BOWFA_CRYSTAL]);
    for (int mask = 0; mask <= FC_CRYSTAL_PIECE_ALL; mask++) {
        bowfa.crystal_piece_mask = mask;
        int attack_first = fc_player_ranged_attack_roll(&bowfa, NULL);
        int maximum_first = fc_player_ranged_final_max_hit_hp(&bowfa, NULL);
        int attack_second = fc_player_ranged_attack_roll(&bowfa, NULL);
        int maximum_second = fc_player_ranged_final_max_hit_hp(&bowfa, NULL);
        if (attack_first != crystal_expected_attack(27621, mask) ||
            maximum_first != crystal_expected_max(26, mask) ||
            attack_second != attack_first || maximum_second != maximum_first ||
            bowfa.ranged_attack_bonus != 233 ||
            bowfa.ranged_strength_bonus != 113) {
            fprintf(stderr,
                    "FAIL CRY-004: mask %d order/purity/raw-stat invariant failed\n",
                    mask);
            return 1;
        }

        bowfa.crystal_piece_mask = mask | 0x80;
        if (fc_player_ranged_attack_roll(&bowfa, NULL) != attack_first ||
            fc_player_ranged_final_max_hit_hp(&bowfa, NULL) != maximum_first) {
            fprintf(stderr,
                    "FAIL CRY-004: unknown mask bit granted or changed a bonus\n");
            return 1;
        }
    }

    printf("PASS CRY-004: exact integer order, bounds, and purity\n");
    return 0;
}

#define CHECK_LOADOUT_FIELD(id, actual, expected, field) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, \
                    "FAIL LOAD-001: loadout %d field %s=%d, expected %d\n", \
                    (id), (field), (actual), (expected)); \
            return 1; \
        } \
    } while (0)

static int test_load_001(void) {
    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        const FcLoadout* actual = &FC_LOADOUTS[i];
        const ExpectedLoadout* expected = &EXPECTED_LOADOUTS[i];
        CHECK_LOADOUT_FIELD(i, actual->max_hp, expected->max_hp, "max_hp");
        CHECK_LOADOUT_FIELD(i, actual->max_prayer, expected->max_prayer, "max_prayer");
        CHECK_LOADOUT_FIELD(i, actual->attack_lvl, expected->attack_level, "attack_lvl");
        CHECK_LOADOUT_FIELD(i, actual->strength_lvl, expected->strength_level, "strength_lvl");
        CHECK_LOADOUT_FIELD(i, actual->defence_lvl, expected->defence_level, "defence_lvl");
        CHECK_LOADOUT_FIELD(i, actual->ranged_lvl, expected->ranged_level, "ranged_lvl");
        CHECK_LOADOUT_FIELD(i, actual->prayer_lvl, expected->prayer_level, "prayer_lvl");
        CHECK_LOADOUT_FIELD(i, actual->magic_lvl, expected->magic_level, "magic_lvl");
        CHECK_LOADOUT_FIELD(i, actual->weapon_speed, expected->weapon_speed, "weapon_speed");
        CHECK_LOADOUT_FIELD(i, actual->weapon_range, expected->weapon_range, "weapon_range");
        CHECK_LOADOUT_FIELD(i, actual->ranged_atk, expected->ranged_attack_bonus, "ranged_atk");
        CHECK_LOADOUT_FIELD(i, actual->ranged_str, expected->ranged_strength_bonus, "ranged_str");
        CHECK_LOADOUT_FIELD(i, actual->def_stab, expected->defence_stab, "def_stab");
        CHECK_LOADOUT_FIELD(i, actual->def_slash, expected->defence_slash, "def_slash");
        CHECK_LOADOUT_FIELD(i, actual->def_crush, expected->defence_crush, "def_crush");
        CHECK_LOADOUT_FIELD(i, actual->def_magic, expected->defence_magic, "def_magic");
        CHECK_LOADOUT_FIELD(i, actual->def_ranged, expected->defence_ranged, "def_ranged");
        CHECK_LOADOUT_FIELD(i, actual->prayer_bonus, expected->prayer_bonus, "prayer_bonus");
        if (!actual->name || !actual->weapon_name ||
            actual->equipment_count < 0 ||
            actual->equipment_count > FC_LOADOUT_EQUIP_MAX ||
            actual->model_item_count < 0 ||
            actual->model_item_count > FC_LOADOUT_MODEL_ITEM_MAX) {
            fprintf(stderr,
                    "FAIL LOAD-001: loadout %d name/count/capacity invariant failed\n",
                    i);
            return 1;
        }
        for (int j = 0; j < i; j++) {
            if (strcmp(actual->name, FC_LOADOUTS[j].name) == 0 ||
                actual->player_model_id == FC_LOADOUTS[j].player_model_id) {
                fprintf(stderr,
                        "FAIL LOAD-001: loadout %d duplicates name or model ID with %d\n",
                        i, j);
                return 1;
            }
        }
    }

    printf("PASS LOAD-001: exact nine-row raw loadout table\n");
    return 0;
}

static void make_loadout_attack_state(FcState* state, int loadout_id,
                                      int ammo_count) {
    fc_init(state);
    fc_reset(state, 123);
    memset(state->npcs, 0, sizeof(state->npcs));
    memset(state->walkable, 1, sizeof(state->walkable));
    state->terminal = TERMINAL_NONE;
    state->current_wave = 1;
    state->npcs_remaining = 1;
    state->next_spawn_index = 1;
    player_from_loadout(&state->player, &FC_LOADOUTS[loadout_id]);
    state->player.x = 10;
    state->player.y = 10;
    state->player.ammo_count = ammo_count;
    fc_npc_spawn(&state->npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state->npcs[0].movement_speed = 0;
    state->npcs[0].attack_timer = 999;
}

static int test_load_002(void) {
    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        const FcLoadout* loadout = &FC_LOADOUTS[i];
        const ExpectedLoadout* expected = &EXPECTED_LOADOUTS[i];
        if (loadout->weapon_kind != expected->weapon_kind ||
            loadout->weapon_uses_ammo != expected->weapon_uses_ammo ||
            loadout->ammo != expected->ammo ||
            loadout->crystal_piece_mask != expected->crystal_piece_mask) {
            fprintf(stderr,
                    "FAIL LOAD-002: loadout %d weapon/ammo/effect metadata is %d/%d/%d/%d\n",
                    i, loadout->weapon_kind, loadout->weapon_uses_ammo,
                    loadout->ammo, loadout->crystal_piece_mask);
            return 1;
        }

        FcState state;
        int actions[FC_NUM_ACTION_HEADS] = {0};
        actions[1] = 1;
        make_loadout_attack_state(&state, i, loadout->ammo);
        fc_tick(&state, actions);
        int expected_ammo = loadout->ammo - (loadout->weapon_uses_ammo ? 1 : 0);
        if (!state.attack_attempt_this_tick ||
            state.player.ammo_count != expected_ammo) {
            fprintf(stderr,
                    "FAIL LOAD-002: loadout %d fired=%d ammo=%d, expected fired=1 ammo=%d\n",
                    i, state.attack_attempt_this_tick,
                    state.player.ammo_count, expected_ammo);
            fc_destroy(&state);
            return 1;
        }
        fc_destroy(&state);

        make_loadout_attack_state(&state, i, 0);
        fc_tick(&state, actions);
        int expected_fire = loadout->weapon_uses_ammo ? 0 : 1;
        if (state.attack_attempt_this_tick != expected_fire ||
            state.player.ammo_count != 0) {
            fprintf(stderr,
                    "FAIL LOAD-002: loadout %d zero-ammo fired=%d ammo=%d, expected %d/0\n",
                    i, state.attack_attempt_this_tick,
                    state.player.ammo_count, expected_fire);
            fc_destroy(&state);
            return 1;
        }
        fc_destroy(&state);
    }

    printf("PASS LOAD-002: weapon, ammunition, and effect metadata\n");
    return 0;
}

static int same_five_defences(const FcLoadout* a, const FcLoadout* b) {
    return a->def_stab == b->def_stab &&
           a->def_slash == b->def_slash &&
           a->def_crush == b->def_crush &&
           a->def_magic == b->def_magic &&
           a->def_ranged == b->def_ranged;
}

static int test_load_003(void) {
    const FcLoadout* low = &FC_LOADOUTS[FC_LOADOUT_LOW_DEF_RCB];
    const FcLoadout* rcb = &FC_LOADOUTS[FC_LOADOUT_RCB_PURE];
    const FcLoadout* msbi = &FC_LOADOUTS[FC_LOADOUT_MSBI_PURE];
    const FcLoadout* blowpipe = &FC_LOADOUTS[FC_LOADOUT_BLOWPIPE_PURE];
    if (memcmp(&EXPECTED_LOADOUTS[FC_LOADOUT_LOW_DEF_RCB],
               &EXPECTED_LOADOUTS[FC_LOADOUT_RCB_PURE],
               sizeof(ExpectedLoadout)) != 0 ||
        low->max_hp != rcb->max_hp || low->max_prayer != rcb->max_prayer ||
        low->attack_lvl != rcb->attack_lvl ||
        low->strength_lvl != rcb->strength_lvl ||
        low->defence_lvl != rcb->defence_lvl ||
        low->ranged_lvl != rcb->ranged_lvl ||
        low->prayer_lvl != rcb->prayer_lvl || low->magic_lvl != rcb->magic_lvl ||
        low->weapon_speed != rcb->weapon_speed ||
        low->weapon_range != rcb->weapon_range ||
        low->ranged_atk != rcb->ranged_atk ||
        low->ranged_str != rcb->ranged_str ||
        !same_five_defences(low, rcb) ||
        low->prayer_bonus != rcb->prayer_bonus) {
        fprintf(stderr,
                "FAIL LOAD-003: LOW_DEF_RCB and RCB_PURE combat fields differ\n");
        return 1;
    }
    if (!same_five_defences(low, msbi) ||
        msbi->ranged_atk == low->ranged_atk ||
        msbi->prayer_bonus == low->prayer_bonus) {
        fprintf(stderr,
                "FAIL LOAD-003: MSBI_PURE defence/attack/prayer relationship failed\n");
        return 1;
    }
    if (blowpipe->def_stab != low->def_stab - 3 ||
        blowpipe->def_slash != low->def_slash - 3 ||
        blowpipe->def_crush != low->def_crush - 3 ||
        blowpipe->def_magic != low->def_magic - 3 ||
        blowpipe->def_ranged != low->def_ranged - 3) {
        fprintf(stderr,
                "FAIL LOAD-003: BLOWPIPE_PURE defences are not exactly three below Glory setup\n");
        return 1;
    }

    int tbow_count = 0;
    int bowfa_count = 0;
    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        const FcLoadout* loadout = &FC_LOADOUTS[i];
        if (loadout->weapon_kind == FC_WEAPON_TWISTED_BOW) tbow_count++;
        if (loadout->weapon_kind == FC_WEAPON_BOW_OF_FAERDHINEN) {
            bowfa_count++;
            if (loadout->crystal_piece_mask != FC_CRYSTAL_PIECE_ALL) {
                fprintf(stderr, "FAIL LOAD-003: Bowfa lacks exact crystal mask\n");
                return 1;
            }
        } else if (loadout->crystal_piece_mask != FC_CRYSTAL_PIECE_NONE) {
            fprintf(stderr,
                    "FAIL LOAD-003: non-Bowfa loadout %d has crystal metadata\n",
                    i);
            return 1;
        }
        if (loadout->attack_lvl < 1 || loadout->strength_lvl < 1 ||
            loadout->defence_lvl < 1 || loadout->ranged_lvl < 1 ||
            loadout->prayer_lvl < 1 || loadout->magic_lvl < 1) {
            fprintf(stderr,
                    "FAIL LOAD-003: loadout %d has a combat skill below 1\n", i);
            return 1;
        }
    }
    if (tbow_count != 2 || bowfa_count != 1) {
        fprintf(stderr,
                "FAIL LOAD-003: weapon-kind counts TBow=%d Bowfa=%d\n",
                tbow_count, bowfa_count);
        return 1;
    }

    printf("PASS LOAD-003: exact cross-loadout relationships\n");
    return 0;
}

static int player_matches_loadout(const FcPlayer* player,
                                  const FcLoadout* loadout) {
    return player->max_hp == loadout->max_hp &&
           player->max_prayer == loadout->max_prayer &&
           player->attack_level == loadout->attack_lvl &&
           player->strength_level == loadout->strength_lvl &&
           player->defence_level == loadout->defence_lvl &&
           player->ranged_level == loadout->ranged_lvl &&
           player->prayer_level == loadout->prayer_lvl &&
           player->magic_level == loadout->magic_lvl &&
           player->weapon_kind == loadout->weapon_kind &&
           player->weapon_uses_ammo == loadout->weapon_uses_ammo &&
           player->crystal_piece_mask == loadout->crystal_piece_mask &&
           player->weapon_speed == loadout->weapon_speed &&
           player->weapon_range == loadout->weapon_range &&
           player->ranged_attack_bonus == loadout->ranged_atk &&
           player->ranged_strength_bonus == loadout->ranged_str &&
           player->defence_stab == loadout->def_stab &&
           player->defence_slash == loadout->def_slash &&
           player->defence_crush == loadout->def_crush &&
           player->defence_magic == loadout->def_magic &&
           player->defence_ranged == loadout->def_ranged &&
           player->prayer_bonus == loadout->prayer_bonus &&
           player->ammo_count == loadout->ammo;
}

static int test_load_004(void) {
    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        FcPlayer player;
        player_from_loadout(&player, &FC_LOADOUTS[i]);
        if (!player_matches_loadout(&player, &FC_LOADOUTS[i]) ||
            fc_player_ranged_base_attack_roll(&player) !=
                EXPECTED_LOADOUTS[i].base_attack_roll ||
            fc_player_ranged_base_max_hit_hp(&player) !=
                EXPECTED_LOADOUTS[i].base_max_hit_hp) {
            fprintf(stderr,
                    "FAIL LOAD-004: equivalent player for loadout %d is incomplete\n",
                    i);
            return 1;
        }
    }

    FcState state;
    fc_init(&state);
    fc_reset(&state, 777);
    if (state.active_loadout != FC_ACTIVE_LOADOUT ||
        !player_matches_loadout(&state.player,
                                &FC_LOADOUTS[FC_ACTIVE_LOADOUT])) {
        fprintf(stderr,
                "FAIL LOAD-004: reset did not copy active loadout %d exactly\n",
                FC_ACTIVE_LOADOUT);
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    printf("PASS LOAD-004: player/reset propagation (compile matrix deferred)\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr,
                "usage: %s <npc_001|npc_002|npc_003|def_001|def_002|def_003|def_005|rng_001|tbow_001|tbow_002|tbow_003|cry_001|cry_002|cry_003|cry_004|load_001|load_002|load_003|load_004>\n",
                argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "npc_001") == 0) return test_npc_001();
    if (strcmp(argv[1], "npc_002") == 0) return test_npc_002();
    if (strcmp(argv[1], "npc_003") == 0) return test_npc_003();
    if (strcmp(argv[1], "def_001") == 0) return test_def_001();
    if (strcmp(argv[1], "def_002") == 0) return test_def_002();
    if (strcmp(argv[1], "def_003") == 0) return test_def_003();
    if (strcmp(argv[1], "def_005") == 0) return test_def_005();
    if (strcmp(argv[1], "rng_001") == 0) return test_rng_001();
    if (strcmp(argv[1], "tbow_001") == 0) return test_tbow_001();
    if (strcmp(argv[1], "tbow_002") == 0) return test_tbow_002();
    if (strcmp(argv[1], "tbow_003") == 0) return test_tbow_003();
    if (strcmp(argv[1], "cry_001") == 0) return test_cry_001();
    if (strcmp(argv[1], "cry_002") == 0) return test_cry_002();
    if (strcmp(argv[1], "cry_003") == 0) return test_cry_003();
    if (strcmp(argv[1], "cry_004") == 0) return test_cry_004();
    if (strcmp(argv[1], "load_001") == 0) return test_load_001();
    if (strcmp(argv[1], "load_002") == 0) return test_load_002();
    if (strcmp(argv[1], "load_003") == 0) return test_load_003();
    if (strcmp(argv[1], "load_004") == 0) return test_load_004();

    fprintf(stderr, "unknown parity core test: %s\n", argv[1]);
    return 2;
}
