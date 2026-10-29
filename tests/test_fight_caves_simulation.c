/* Deterministic simulation, equipment, CPU policy, native 5.0 adapter and viewer checks.
 * Private snapshot assertions include binding.c; independent C/C++ linkage is
 * exercised separately by fight_caves_linkage.c. No graphics unless --render. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "binding.c"
#define FC_CPU_TEST /* CPU inference helpers without the standalone main */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function" /* replay serves that main */
#include "fight_caves.c"
#pragma GCC diagnostic pop
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static void fail(const char* message) {
    fprintf(stderr, "core_contract_test: %s\n", message);
    exit(EXIT_FAILURE);
}

static void wave_rotation_test(void) {
    /* FNV-1a fingerprints captured from the expanded table before factoring.
     * Each wave covers all 15 rotations and all six slots, including padding. */
    static const uint32_t expected[FC_NUM_WAVES] = {
        0x94a6ae61u, 0xb43e53bdu, 0x5ae991d9u, 0xb87f83cdu,
        0x7ae7bc55u, 0xc2eb4881u, 0x6f091049u, 0x2c80a9b9u,
        0xff5bb385u, 0x0460e561u, 0x3fd38821u, 0x70d9d565u,
        0x3033fbb1u, 0x39ff4ee5u, 0xfb88abe1u, 0x42db8009u,
        0xcd6d9305u, 0x75d5d409u, 0xecd94691u, 0xe481f925u,
        0x047e78e1u, 0xbcec175du, 0x28ee1f11u, 0xe225e405u,
        0xee2ea345u, 0x6e841145u, 0x12b64e1du, 0xf7a76349u,
        0xf41e3f01u, 0xf9d9e13du, 0x94a6ae61u, 0x7b92c141u,
        0xe6d41269u, 0xb87f83cdu, 0x4efe59e1u, 0xd0ef2aa9u,
        0x7cf6df61u, 0x2c80a9b9u, 0xf9ba9da9u, 0xffa136e9u,
        0x3fd38821u, 0x179231cdu, 0x45be2b59u, 0x1c645eddu,
        0x1ea54635u, 0x42db8009u, 0x538afaf1u, 0x8c6cad2du,
        0xecd94691u, 0x0cf73279u, 0xa339fc69u, 0xfeddd769u,
        0x28ee1f11u, 0x67f6d72du, 0x1b3ebe39u, 0x6e841145u,
        0xbc40ffe9u, 0x66ac8f75u, 0xf9552f4du, 0x3de75731u,
        0x4a20ae1du, 0xb43e53bdu, 0x5ae991d9u,
    };
    for (int wave = 1; wave <= FC_NUM_WAVES; wave++) {
        uint32_t hash = 2166136261u;
        for (int rotation = 0; rotation < FC_NUM_ROTATIONS; rotation++) {
            for (int slot = 0; slot < FC_MAX_SPAWNS_PER_WAVE; slot++) {
                int direction = fc_wave_spawn_dir(wave, rotation, slot);
                if (direction < SPAWN_SOUTH || direction > SPAWN_CENTER)
                    fail("wave spawn direction is out of range");
                hash = (hash ^ (uint32_t)direction) * 16777619u;
            }
        }
        if (hash != expected[wave - 1]) {
            fprintf(stderr, "wave_rotation_test: wave %d changed\n", wave);
            fail("wave rotation fixture mismatch");
        }
    }
    if (fc_wave_spawn_dir(0, 0, 0) != SPAWN_CENTER ||
        fc_wave_spawn_dir(FC_NUM_WAVES + 1, 0, 0) != SPAWN_CENTER ||
        fc_wave_spawn_dir(1, -1, 0) != SPAWN_CENTER ||
        fc_wave_spawn_dir(1, FC_NUM_ROTATIONS, 0) != SPAWN_CENTER ||
        fc_wave_spawn_dir(1, 0, -1) != SPAWN_CENTER ||
        fc_wave_spawn_dir(1, 0, FC_MAX_SPAWNS_PER_WAVE) != SPAWN_CENTER)
        fail("invalid wave lookup fallback changed");
    puts("wave_rotation_test: all waves, rotations, slots and invalid inputs passed");
}

static void check_observation(const FcState* state) {
    float obs[FC_POLICY_OBS_SIZE];
    float mask[FC_ACTION_MASK_SIZE];
    fc_write_obs(state, obs);
    fc_write_mask(state, mask);
    for (int i = 0; i < FC_POLICY_OBS_SIZE; i++) {
        if (!isfinite(obs[i])) fail("observation contains a non-finite value");
    }
    for (int i = 0; i < FC_ACTION_MASK_SIZE; i++) {
        if (mask[i] != 0.0f && mask[i] != 1.0f)
            fail("action mask contains a value other than zero or one");
    }
    int offset = 0;
    for (int head = 0; head < FC_NUM_ACTION_HEADS; head++) {
        int legal = 0;
        for (int action = 0; action < FC_ACTION_DIMS[head]; action++)
            legal += mask[offset + action] == 1.0f;
        if (legal == 0) fail("an action head has no legal action");
        offset += FC_ACTION_DIMS[head];
    }
}

static int core_contract_test(void) {
    _Static_assert(FC_STATE_HASH_VERSION == 6, "analytics state hash version drifted");
    /* 286 + 34 = 320 floats and three heads with the default capabilities;
     * spellbook = standard adds 20 floats and a 21-value head, ancient 48
     * floats and a 17-value head; special_attacks = 1 3 floats and a
     * 2-value head. */
    _Static_assert(FC_POLICY_OBS_SIZE ==
                       286 + 3 * FC_CAP_CONSUMABLES + 25 * FC_CAP_PRAYERS +
                       6 * FC_GEAR_SWITCHING * FC_CAP_GEAR_SETS +
                       (FC_CAP_SPELLBOOK == 1 ? 20 : FC_CAP_SPELLBOOK == 2 ? 48 : 0) +
                       3 * FC_CAP_SPECIAL_ATTACKS,
                   "policy observation contract drifted");
    _Static_assert(FC_PUFFER_MASK_SIZE ==
                       34 + 5 * FC_CAP_CONSUMABLES + 25 * FC_CAP_PRAYERS +
                       FC_GEAR_SWITCHING * (1 + FC_CAP_GEAR_SETS) +
                       (FC_CAP_SPELLBOOK == 1 ? 21 : FC_CAP_SPELLBOOK == 2 ? 17 : 0) +
                       2 * FC_CAP_SPECIAL_ATTACKS,
                   "Puffer mask contract drifted");
    _Static_assert(FC_PUFFER_OBS_SIZE == FC_POLICY_OBS_SIZE + FC_PUFFER_MASK_SIZE,
                   "Puffer observation contract drifted");
    _Static_assert(FC_PUFFER_NUM_ATNS ==
                       3 + 2 * FC_CAP_CONSUMABLES + FC_CAP_PRAYERS + FC_GEAR_SWITCHING +
                       FC_SPELLBOOK_HEAD + FC_CAP_SPECIAL_ATTACKS,
                   "Puffer action-head count drifted");

    FcState first;
    FcState second;
    fc_init(&first);
    fc_init(&second);
    fc_reset(&first, 0x12345678u);
    fc_reset(&second, 0x12345678u);
    if (fc_state_hash(&first) != fc_state_hash(&second))
        fail("same-seed resets are not deterministic");
    check_observation(&first);

    int steps = 0;
    for (; steps < 4096 && !fc_is_terminal(&first); steps++) {
        int actions[FC_NUM_ACTION_HEADS] = {0};
        actions[0] = steps % FC_PUFFER_ACTION_DIMS[0];
        actions[1] = (steps / 3) % FC_PUFFER_ACTION_DIMS[1];
        actions[2] = (steps / 7) % FC_PUFFER_ACTION_DIMS[2];
        fc_step(&first, actions);
        fc_step(&second, actions);
        if (fc_state_hash(&first) != fc_state_hash(&second))
            fail("same-seed trajectories diverged");
        check_observation(&first);
    }
    if (steps == 0) fail("simulation did not advance");
    if (!fc_is_terminal(&first))
        fail("test trajectory did not exercise a terminal transition");

    /* Capability builds also hash FcState.caps, so only the default pins it. */
    if (steps != 483 || (FC_DEFAULT_CAPABILITIES && fc_state_hash(&first) != 0x5c53c26du))
        fail("fixed-seed trajectory changed; review and update the contract fixture intentionally");

    printf("core_contract_test: passed (%d steps, hash=%08x)\n",
           steps, fc_state_hash(&first));
    fc_destroy(&first);
    fc_destroy(&second);
    return EXIT_SUCCESS;
}

/* Equipment transaction scenarios. */
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "equipment: line %d: %s\n", __LINE__, #test); return 1; \
} } while (0)

static void reset(FcState *s, int empty_inventory) {
    fc_init(s);
    fc_reset(s, 101);
    if (empty_inventory) reset_supplies(&s->player, 0, 0);
}

static int item_slot(const FcPlayer *p, int id) {
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (p->inventory[i].item_id == id) return i;
    return -1;
}

/* A scenario with preset `preset` in every gear set this build has. */
static FcScenario preset_scenario(int preset) {
    FcScenario scenario = fc_default_scenario();
    for (int k = 0; k < FC_CAP_GEAR_SETS; k++) scenario.gear_set[k] = preset;
    return scenario;
}

/* The levels each magic and melee preset was made for, as fc-rl-4.0-dlc's
 * loadouts table (docs/loadouts.md) gives them, in FcSkill order: Attack,
 * Strength, Defence, Ranged, Magic, Prayer, Hitpoints. */
static const struct { const char *name; int levels[FC_SKILL_COUNT]; } PRESET_LEVELS[] = {
    {"smoke_staff_mystic", {60, 60, 60, 60, 60, 43, 65}},
    {"kodai_virtus", {80, 80, 80, 80, 94, 80, 90}},
    {"harmonised_ancestral", {99, 99, 99, 99, 99, 99, 99}},
    {"dragon_scimitar_torso", {60, 60, 60, 60, 60, 43, 65}},
    {"macuahuitl_blood_moon", {70, 75, 70, 70, 70, 70, 80}},
    {"fang_bandos", {82, 85, 80, 80, 80, 80, 90}},
    {"scythe_torva", {99, 99, 99, 99, 99, 99, 99}},
};
#define PRESET_LEVEL_COUNT ((int)(sizeof(PRESET_LEVELS) / sizeof(PRESET_LEVELS[0])))

/* preset_scenario for a preset, at its PRESET_LEVELS levels if it has them. */
static FcScenario levels_scenario(const char *name) {
    FcScenario scenario = preset_scenario(fc_gear_preset_find(name));
    for (int i = 0; i < PRESET_LEVEL_COUNT; i++)
        if (strcmp(PRESET_LEVELS[i].name, name) == 0)
            memcpy(scenario.levels, PRESET_LEVELS[i].levels, sizeof(scenario.levels));
    return scenario;
}

static int episode_analytics_test(void) {
    FcState s;
    FcRewardRuntime runtime;
    FcRewardParams params = fc_reward_default_params();
    FcEpisodeSummary summary;
    reset(&s, 0);
    memset(s.npcs, 0, sizeof(s.npcs));
    s.current_wave = 63;
    int jad = fc_spawn_npc_first_free(&s, NPC_TZTOK_JAD, 10, 10);
    CHECK(jad >= 0);
    s.npcs[jad].current_hp -= 100;
    fc_reward_runtime_begin_episode(&runtime, &s);

    /* Idle, damage, healing, idle: only the two idle ticks count as zero
     * progress. Healing totals use effective HP restored, capped at max HP. */
    fc_reward_compute_breakdown(&s, &params, &runtime);
    s.npcs[jad].current_hp -= 10;
    fc_reward_compute_breakdown(&s, &params, &runtime);
    CHECK(apply_npc_heal(&s, &s.npcs[jad], &s.npcs[jad], 200) == 110);
    fc_reward_compute_breakdown(&s, &params, &runtime);
    clear_per_tick_flags(&s);
    fc_reward_compute_breakdown(&s, &params, &runtime);
    CHECK(runtime.zero_progress_ticks == 2);
    CHECK(runtime.npc_healing_total == 110 && runtime.jad_healing_total == 110);

    /* No prayer and correct prayer are excluded from wrong-prayer hits,
     * including when an attack rolls zero damage. */
    const int prayers[] = {PRAYER_NONE, PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_RANGE};
    for (int i = 0; i < 3; i++) {
        CHECK(fc_queue_pending_hit(s.player.pending_hits, &s.player.num_pending_hits,
            FC_MAX_PENDING_HITS, 0, 1, ATTACK_RANGED, jad, 0));
        s.player.pending_hits[i].prayer_snapshot = prayers[i];
    }
    fc_resolve_player_pending_hits(&s);
    CHECK(s.ep_wrong_prayer_hits == 1);

    const int actions[FC_NUM_ACTION_HEADS] = {0};
    for (int prayer = PRAYER_PROTECT_MELEE; prayer <= PRAYER_PROTECT_MAGIC; prayer++) {
        s.player.prayer = prayer;
        fc_step(&s, actions);
    }
    complete_fight_caves(&s);
    fc_episode_summary_build(&s, &runtime, 3, &summary);
    CHECK(summary.wave_reached == 63 && summary.reached_wave_63 == 1);
    CHECK(summary.jad_kill_rate == 1 && summary.wrong_prayer_hits == 1);
    CHECK(summary.prayer_uptime_melee == 1.0f / 3.0f);
    CHECK(summary.prayer_uptime_range == 1.0f / 3.0f);
    CHECK(summary.prayer_uptime_magic == 1.0f / 3.0f);

    fc_reset(&s, 101);
    fc_reward_runtime_begin_episode(&runtime, &s);
    fc_episode_summary_build(&s, &runtime, 0, &summary);
    CHECK(summary.wave_reached == 1 && summary.episode_length == 0);
    CHECK(summary.zero_progress_ticks == 0 && summary.wrong_prayer_hits == 0);
    CHECK(summary.reached_wave_63 == 0 && summary.jad_kill_rate == 0);
    CHECK(summary.npc_healing_total == 0 && summary.jad_healing_total == 0);
    CHECK(summary.prayer_uptime_melee == 0 && summary.prayer_uptime_range == 0 &&
          summary.prayer_uptime_magic == 0);
    /* Multiple invalid heads still incur one invalid-action penalty. */
    const int invalid_actions[FC_NUM_ACTION_HEADS] = {999, 999, 999};
    fc_step(&s, invalid_actions);
    CHECK(s.invalid_action_this_tick == 1);
    fc_destroy(&s);
    puts("episode_analytics_test: progress, healing, prayers, Jad and reset passed");
    return 0;
}

static int preset_totals(void) {
    /* Each preset's totals by id, pinned independently of the item definitions. */
    static const FcPlayer expected[] = {
        [FC_LOADOUT_RCB_BLACK_DHIDE] = {
            .ranged_attack_bonus = 153, .ranged_strength_bonus = 100, .defence_stab = 97,
            .defence_slash = 84, .defence_crush = 110, .defence_magic = 91,
            .defence_ranged = 90, .prayer_bonus = 0, .weapon_kind = FC_WEAPON_GENERIC_RANGED,
            .weapon_speed = 5, .weapon_range = 7, .weapon_uses_ammo = 1,
            .crystal_piece_mask = 0, .ammo_count = 50000,
        },
        [FC_LOADOUT_SOTA_TBOW] = {
            .ranged_attack_bonus = 215, .ranged_strength_bonus = 99, .defence_stab = 116,
            .defence_slash = 106, .defence_crush = 129, .defence_magic = 150,
            .defence_ranged = 121, .prayer_bonus = 6, .weapon_kind = FC_WEAPON_TWISTED_BOW,
            .weapon_speed = 5, .weapon_range = 10, .weapon_uses_ammo = 1,
            .crystal_piece_mask = 0, .ammo_count = 50000,
        },
        [FC_LOADOUT_RCB_PURE] = {
            .ranged_attack_bonus = 166, .ranged_strength_bonus = 100, .defence_stab = 48,
            .defence_slash = 49, .defence_crush = 62, .defence_magic = 42,
            .defence_ranged = 46, .prayer_bonus = 8, .weapon_kind = FC_WEAPON_GENERIC_RANGED,
            .weapon_speed = 5, .weapon_range = 7, .weapon_uses_ammo = 1,
            .crystal_piece_mask = 0, .ammo_count = 50000,
        },
        [FC_LOADOUT_MSB_PURE] = {
            .ranged_attack_bonus = 141, .ranged_strength_bonus = 49, .defence_stab = 48,
            .defence_slash = 49, .defence_crush = 62, .defence_magic = 42,
            .defence_ranged = 46, .prayer_bonus = 3, .weapon_kind = FC_WEAPON_GENERIC_RANGED,
            .weapon_speed = 3, .weapon_range = 7, .weapon_uses_ammo = 1,
            .crystal_piece_mask = 0, .ammo_count = 50000,
        },
        [FC_LOADOUT_BLOWPIPE_PURE] = {
            .ranged_attack_bonus = 101, .ranged_strength_bonus = 42, .defence_stab = 45,
            .defence_slash = 46, .defence_crush = 59, .defence_magic = 39,
            .defence_ranged = 43, .prayer_bonus = 2, .weapon_kind = FC_WEAPON_GENERIC_RANGED,
            .weapon_speed = 2, .weapon_range = 5, .weapon_uses_ammo = 1,
            .crystal_piece_mask = 0, .ammo_count = 50000,
        },
        [FC_LOADOUT_ACB_ARMADYL] = {
            .ranged_attack_bonus = 220, .ranged_strength_bonus = 129, .defence_stab = 112,
            .defence_slash = 100, .defence_crush = 123, .defence_magic = 139,
            .defence_ranged = 117, .prayer_bonus = 11, .weapon_kind = FC_WEAPON_GENERIC_RANGED,
            .weapon_speed = 5, .weapon_range = 8, .weapon_uses_ammo = 1,
            .crystal_piece_mask = 0, .ammo_count = 50000,
        },
        [FC_LOADOUT_BOWFA_CRYSTAL] = {
            .ranged_attack_bonus = 233, .ranged_strength_bonus = 113, .defence_stab = 102,
            .defence_slash = 85, .defence_crush = 110, .defence_magic = 107,
            .defence_ranged = 143, .prayer_bonus = 9, .weapon_kind = FC_WEAPON_BOW_OF_FAERDHINEN,
            .weapon_speed = 4, .weapon_range = 10, .weapon_uses_ammo = 0,
            .crystal_piece_mask = FC_CRYSTAL_PIECE_ALL, .ammo_count = 0,
        },
        [FC_LOADOUT_TBOW_MASORI_NO_RING] = {
            .ranged_attack_bonus = 205, .ranged_strength_bonus = 97, .defence_stab = 116,
            .defence_slash = 106, .defence_crush = 129, .defence_magic = 150,
            .defence_ranged = 121, .prayer_bonus = 6, .weapon_kind = FC_WEAPON_TWISTED_BOW,
            .weapon_speed = 5, .weapon_range = 10, .weapon_uses_ammo = 1,
            .crystal_piece_mask = 0, .ammo_count = 50000,
        },
        /* The magic presets, from RuneC's items.bin rows; the staves' speed
         * and range are their bash's. */
        [FC_LOADOUT_SMOKE_STAFF_MYSTIC] = {
            .ranged_attack_bonus = 0, .ranged_strength_bonus = 0, .defence_stab = 3,
            .defence_slash = 4, .defence_crush = 3, .defence_magic = 90,
            .defence_ranged = 0, .prayer_bonus = 0, .weapon_kind = FC_WEAPON_STAFF,
            .weapon_speed = 5, .weapon_range = 1, .weapon_uses_ammo = 0,
            .crystal_piece_mask = 0, .ammo_count = 0,
        },
        [FC_LOADOUT_KODAI_VIRTUS] = {
            .ranged_attack_bonus = -23, .ranged_strength_bonus = 0, .defence_stab = 106,
            .defence_slash = 92, .defence_crush = 126, .defence_magic = 114,
            .defence_ranged = 11, .prayer_bonus = 11, .weapon_kind = FC_WEAPON_STAFF,
            .weapon_speed = 4, .weapon_range = 1, .weapon_uses_ammo = 0,
            .crystal_piece_mask = 0, .ammo_count = 0,
        },
        [FC_LOADOUT_HARMONISED_ANCESTRAL] = {
            .ranged_attack_bonus = -17, .ranged_strength_bonus = 0, .defence_stab = 137,
            .defence_slash = 124, .defence_crush = 170, .defence_magic = 84,
            .defence_ranged = 52, .prayer_bonus = 8, .weapon_kind = FC_WEAPON_STAFF,
            .weapon_speed = 5, .weapon_range = 1, .weapon_uses_ammo = 0,
            .crystal_piece_mask = 0, .ammo_count = 0,
        },
        /* The melee presets, from RuneC's items.bin rows (the first three
         * equal fc-rl-4.0-dlc's totals; scythe_torva's Avernic treads (max)
         * have no bonuses in RuneC), with their crush and Strength bonuses. */
        [FC_LOADOUT_DRAGON_SCIMITAR_TORSO] = {
            .ranged_attack_bonus = 9, .ranged_strength_bonus = 0, .defence_stab = 240,
            .defence_slash = 260, .defence_crush = 242, .defence_magic = 18,
            .defence_ranged = 198, .prayer_bonus = 10, .weapon_kind = FC_WEAPON_MELEE,
            .weapon_speed = 4, .weapon_range = 1, .weapon_uses_ammo = 0,
            .crystal_piece_mask = 0, .ammo_count = 0,
            .melee_attack_bonus = 44, .melee_strength_bonus = 111,
        },
        [FC_LOADOUT_MACUAHUITL_BLOOD_MOON] = {
            .ranged_attack_bonus = 3, .ranged_strength_bonus = 0, .defence_stab = 164,
            .defence_slash = 216, .defence_crush = 227, .defence_magic = 111,
            .defence_ranged = 192, .prayer_bonus = 7, .weapon_kind = FC_WEAPON_MELEE,
            .weapon_speed = 4, .weapon_range = 1, .weapon_uses_ammo = 0,
            .crystal_piece_mask = 0, .ammo_count = 0,
            .melee_attack_bonus = 144, .melee_strength_bonus = 127,
        },
        [FC_LOADOUT_FANG_BANDOS] = {
            .ranged_attack_bonus = -37, .ranged_strength_bonus = 0, .defence_stab = 268,
            .defence_slash = 252, .defence_crush = 278, .defence_magic = -1,
            .defence_ranged = 267, .prayer_bonus = 11, .weapon_kind = FC_WEAPON_MELEE,
            .weapon_speed = 5, .weapon_range = 1, .weapon_uses_ammo = 0,
            .crystal_piece_mask = 0, .ammo_count = 0,
            .melee_attack_bonus = 62, .melee_strength_bonus = 164,
        },
        [FC_LOADOUT_SCYTHE_TORVA] = {
            .ranged_attack_bonus = -53, .ranged_strength_bonus = 0, .defence_stab = 273,
            .defence_slash = 269, .defence_crush = 280, .defence_magic = -10,
            .defence_ranged = 313, .prayer_bonus = 9, .weapon_kind = FC_WEAPON_MELEE,
            .weapon_speed = 5, .weapon_range = 1, .weapon_uses_ammo = 0,
            .crystal_piece_mask = 0, .ammo_count = 0,
            .melee_attack_bonus = 75, .melee_strength_bonus = 139,
        },
    };
    int pinned = 0;
    for (unsigned id = 0; id < sizeof(expected) / sizeof(expected[0]); id++)
        pinned += expected[id].weapon_speed > 0;
    CHECK(pinned == FC_GEAR_PRESET_COUNT);
    for (int i = 0; i < FC_GEAR_PRESET_COUNT; i++) {
        const FcGearPreset *preset = &FC_GEAR_PRESETS[i];
        CHECK(preset->id >= 0 &&
              preset->id < (int)(sizeof(expected) / sizeof(expected[0])));
        const FcPlayer *e = &expected[preset->id];
        FcPlayer p = {0};
        apply_levels(&p, FC_DEFAULT_LEVELS);
        fc_items_init(&p, preset);
        CHECK(p.ranged_attack_bonus == e->ranged_attack_bonus);
        CHECK(p.ranged_strength_bonus == e->ranged_strength_bonus);
        CHECK(p.defence_stab == e->defence_stab);
        CHECK(p.defence_slash == e->defence_slash);
        CHECK(p.defence_crush == e->defence_crush);
        CHECK(p.defence_magic == e->defence_magic);
        CHECK(p.defence_ranged == e->defence_ranged);
        CHECK(p.prayer_bonus == e->prayer_bonus);
        CHECK(p.weapon_kind == e->weapon_kind);
        CHECK(p.weapon_speed == e->weapon_speed);
        CHECK(p.weapon_range == e->weapon_range);
        CHECK(p.weapon_uses_ammo == e->weapon_uses_ammo);
        CHECK(p.crystal_piece_mask == e->crystal_piece_mask);
        CHECK(p.ammo_count == e->ammo_count);
        if (e->weapon_kind == FC_WEAPON_MELEE)
            CHECK(p.melee_attack_bonus == e->melee_attack_bonus &&
                  p.melee_strength_bonus == e->melee_strength_bonus);
    }
    return 0;
}

/* The gear presets, default levels and NPC tables are compile-time constants,
 * checked here rather than on every fc_init. The viewer has each preset's
 * combat tab and its weapon's animations. */
static int constant_tables(void) {
    char err[256];
    FcScenario scenario = preset_scenario(0);
    CHECK(fc_scenario_check(&scenario, err, sizeof(err)) == 0);
    for (int p = 0; p < FC_GEAR_PRESET_COUNT; p++) {
        const FcGearPreset *preset = &FC_GEAR_PRESETS[p];
        CHECK(fc_gear_preset_find(preset->name) == p);
        CHECK(fc_gear_preset_by_id(preset->id) == preset);
        CHECK(preset->ammo >= 0 && preset->equipment_count > 0 &&
              preset->equipment_count <= FC_LOADOUT_EQUIP_MAX);
        int occupied[FC_EQUIPMENT_SLOTS] = {0};
        for (int i = 0; i < preset->equipment_count; i++) {
            const FcItemDef *item = fc_item_definition(preset->equipment[i]);
            CHECK(item && item->slot >= 0 && item->slot < FC_EQUIPMENT_SLOTS);
            CHECK(occupied[item->slot]++ == 0);
        }
        FcPlayer player = {0};
        apply_levels(&player, FC_DEFAULT_LEVELS);
        fc_items_init(&player, preset);
        CHECK(combat_profile_for_weapon_category(preset->combat_style_profile)
                  ->weapon_category == preset->combat_style_profile);
        /* A magic preset's projectile is its spell's; a melee weapon fires
         * none, has its RuneC weapon type as its combat tab and attacks in
         * an attack style it has. Every other preset punches or bashes
         * Accurate. */
        const FcItemDef *weapon = fc_gear_preset_weapon(preset);
        int melee = weapon->weapon_kind == FC_WEAPON_MELEE;
        CHECK((preset->autocast
               ? FC_SPELL_VISUALS[preset->autocast - 1].projectile.travel_spot
               : fc_player_visual_profile(&player)->projectile.travel_spot) != 0 || melee);
        FcMeleeStyle style;
        if (melee)
            CHECK(fc_player_visual_profile(&player)->projectile.travel_spot == 0 &&
                  preset->combat_style_profile == weapon->weapon_type &&
                  fc_melee_style(weapon->weapon_type, preset->attack_style, &style));
        else
            CHECK(preset->attack_style == 0);
        CHECK(!preset->runes || (preset->autocast && preset->runes == FC_PRESET_RUNE_QUANTITY));
        for (int type = NPC_TZ_KIH; type < NPC_TYPE_COUNT; type++) {
            FcNpc target = {.npc_type = type};
            FcPrayerBonus no_prayer = {0};
            int attack_roll, max_hit;
            fc_player_ranged_rolls(&player, &no_prayer, &target, &attack_roll, &max_hit);
            CHECK(max_hit > 0);
        }
    }
    /* NPC reach and attack rules assume a melee-style NPC has a melee hit. */
    for (int type = NPC_TZ_KIH; type < NPC_TYPE_COUNT; type++) {
        const FcNpcStats *stats = fc_npc_get_stats(type);
        CHECK(stats->attack_style != ATTACK_MELEE || stats->max_hit[ATTACK_MELEE] > 0);
    }
    return 0;
}

static int transactions(void) {
    FcState s;
    reset(&s, 0);
    uint32_t before = fc_state_hash(&s);
    CHECK(fc_unequip_item(&s, FC_EQUIP_SLOT_HEAD) == FC_ITEM_NO_SPACE);
    CHECK(fc_state_hash(&s) == before);
    CHECK(fc_equip_item(&s, -1) == FC_ITEM_INVALID);
    CHECK(fc_equip_item(&s, 28) == FC_ITEM_INVALID);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_INVALID); /* food is not equipment */
    CHECK(fc_unequip_item(&s, 6) == FC_ITEM_INVALID); /* client-only arms */
    CHECK(fc_state_hash(&s) == before);
    reset(&s, 1);
    s.player.current_hp = 400;
    s.player.current_prayer = 370;
    s.player.attack_timer = 4;
    s.player.prayer_drain_counter = 21;
    s.player.attack_target_idx = 0;
    CHECK(fc_unequip_item(&s, FC_EQUIP_SLOT_HEAD) == FC_ITEM_OK);
    CHECK(s.player.inventory[0].item_id == 27235);
    CHECK(s.player.ranged_attack_bonus == 203 && s.player.ranged_strength_bonus == 97);
    CHECK(s.player.defence_stab == 108 && s.player.prayer_bonus == 5);
    CHECK(s.player.attack_target_idx == 0);
    uint32_t rng = s.rng_state;
    s.player.defence_level = 79;
    before = fc_state_hash(&s);
    CHECK(fc_equip_item(&s, 0) == FC_ITEM_REQUIREMENTS);
    CHECK(fc_state_hash(&s) == before);
    s.player.defence_level = 99;
    CHECK(fc_equip_item(&s, 0) == FC_ITEM_OK);
    CHECK(s.player.attack_target_idx == -1);
    CHECK(s.player.ranged_attack_bonus == 215 && s.player.ranged_strength_bonus == 99);
    CHECK(s.player.current_hp == 400 && s.player.current_prayer == 370);
    CHECK(s.player.attack_timer == 4 && s.player.prayer_drain_counter == 21);
    CHECK(s.rng_state == rng && s.tick == 0);
    for (int i = 0; i < FC_EQUIPMENT_SLOTS; i++)
        if (s.player.equipment[i].item_id) CHECK(fc_unequip_item(&s, i) == FC_ITEM_OK);
    CHECK(s.player.weapon_kind == FC_WEAPON_UNARMED);
    CHECK(s.player.ranged_attack_bonus == 0 && s.player.defence_stab == 0);
    CHECK(s.player.prayer_bonus == 0 && s.player.ammo_count == 0);
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (s.player.inventory[i].item_id) CHECK(fc_equip_item(&s, i) == FC_ITEM_OK);
    CHECK(s.player.ranged_attack_bonus == 215 && s.player.ranged_strength_bonus == 99);
    CHECK(s.player.equipment[13].quantity == 50000);
    s.terminal = TERMINAL_PLAYER_DEATH;
    before = fc_state_hash(&s);
    CHECK(fc_unequip_item(&s, 0) == FC_ITEM_BUSY);
    CHECK(fc_state_hash(&s) == before);
    return 0;
}

static int two_handed_and_stacks(void) {
    FcState s;
    reset(&s, 0);
    /* Full inventory: a one-for-one shield swap may use the source slot for
     * the two-handed bow when the old shield slot is empty. */
    s.player.inventory[8] = (FcItemStack){12610, 1, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.inventory[8].item_id == 20997);
    CHECK(s.player.equipment[FC_EQUIP_SLOT_SHIELD].item_id == 12610);
    CHECK(s.player.weapon_kind == FC_WEAPON_UNARMED);
    /* With a crossbow AND shield worn, bow needs a second inventory slot. */
    s.player.inventory[9] = (FcItemStack){9185, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    s.player.inventory[9] = (FcItemStack){385, 1, 0};
    uint32_t before = fc_state_hash(&s);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_NO_SPACE);
    CHECK(fc_state_hash(&s) == before);
    s.player.inventory[14] = (FcItemStack){0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.inventory[8].item_id == 9185);
    CHECK(s.player.inventory[14].item_id == 12610);
    CHECK(s.player.equipment[FC_EQUIP_SLOT_SHIELD].item_id == 0);
    reset(&s, 0);
    s.player.inventory[8] = (FcItemStack){11212, 7, 0};
    CHECK(fc_unequip_item(&s, 13) == FC_ITEM_OK); /* merge despite full inventory */
    CHECK(s.player.inventory[8].quantity == 50007);
    CHECK(s.player.ammo_count == 0);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 50007);
    s.player.inventory[8] = (FcItemStack){11212, INT_MAX - 50006, 0};
    before = fc_state_hash(&s);
    CHECK(fc_unequip_item(&s, 13) == FC_ITEM_NO_SPACE); /* whole-stack overflow */
    CHECK(fc_state_hash(&s) == before);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK); /* equip only the portion that fits */
    CHECK(s.player.equipment[13].quantity == INT_MAX);
    CHECK(s.player.inventory[8].quantity == 1);
    before = fc_state_hash(&s);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_NO_SPACE);
    CHECK(fc_state_hash(&s) == before);
    s.player.inventory[8] = (FcItemStack){9143, 100, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 0); /* bolts do not work in a bow */
    CHECK(s.player.inventory[8].item_id == 11212);
    s.player.inventory[9] = (FcItemStack){12788, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK); /* dragon arrows in quiver */
    CHECK(s.player.ammo_count == 0); /* MSB cannot fire dragon arrows */
    s.player.inventory[8] = (FcItemStack){892, 10, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 10);
    s.player.inventory[9] = (FcItemStack){9185, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    s.player.inventory[8] = (FcItemStack){21946, 10, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 0); /* rune crossbow cannot fire dragon bolts */
    s.player.inventory[9] = (FcItemStack){11785, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 10);
    return 0;
}

static int supplies_and_loaded_weapon(void) {
    FcState s;
    reset(&s, 1);
    reset_supplies(&s.player, 2, 5);
    CHECK(s.player.inventory[0].item_id == 2434 && s.player.inventory[1].item_id == 143);
    CHECK(s.player.inventory[2].item_id == 385 && s.player.inventory[3].item_id == 385);
    CHECK(fc_inventory_swap(&s, 1, 27) == FC_ITEM_OK);
    CHECK(fc_select_consumable(&s, 27) == FC_ITEM_OK);
    CHECK(fc_select_consumable(&s, 3) == FC_ITEM_OK);
    s.player.current_hp = 400;
    s.player.current_prayer = 100;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[3] = FC_EAT_SHARK;
    actions[4] = FC_DRINK_PRAYER_POT;
    fc_step(&s, actions);
    CHECK(s.player.inventory[27].item_id == 229);
    CHECK(s.player.inventory[0].item_id == 2434);
    CHECK(s.player.inventory[3].item_id == 0 && s.player.inventory[2].item_id == 385);
    CHECK(s.player.sharks_remaining == 1 && s.player.prayer_doses_remaining == 4);
    CHECK(s.player.selected_food_slot == -1 && s.player.selected_potion_slot == -1);
    FcScenario scenario = preset_scenario(fc_gear_preset_find("blowpipe_pure"));
    CHECK(scenario.gear_set[0] >= 0);
    reset(&s, 1);
    fc_apply_scenario(&s, &scenario);
    CHECK(s.active_loadout == FC_LOADOUT_BLOWPIPE_PURE);
    fc_items_spend_ammo(&s.player);
    CHECK(s.player.equipment[3].charges == 49999);
    CHECK(s.player.equipment[13].item_id == 0);
    CHECK(fc_unequip_item(&s, 3) == FC_ITEM_OK);
    CHECK(s.player.inventory[0].charges == 49999);
    CHECK(fc_equip_item(&s, 0) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 49999);
    return 0;
}

static int combat(void) {
    FcState s;
    reset(&s, 1);
    memset(s.npcs, 0, sizeof(s.npcs));
    memset(s.walkable, 1, sizeof(s.walkable));
    memset(s.movement_flags, 0, sizeof(s.movement_flags));
    memset(s.los_flags, 0, sizeof(s.los_flags));
    s.player.x = 20; s.player.y = 20;
    fc_npc_spawn(&s.npcs[0], NPC_YT_MEJKOT, 20, 25, 1);
    s.npcs_remaining = 1;
    s.npcs[0].attack_timer = 100;
    s.player.attack_target_idx = 0;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    fc_step(&s, actions);
    CHECK(s.render_events.player_attack_fired);
    CHECK(s.player.ammo_count == 49999 && s.player.equipment[13].quantity == 49999);
    FcPendingHit hit = s.npcs[0].pending_hits[0];
    int cooldown = s.player.attack_timer;
    CHECK(fc_unequip_item(&s, 3) == FC_ITEM_OK);
    CHECK(s.player.attack_target_idx == 0 && s.player.attack_timer == cooldown);
    CHECK(memcmp(&hit, &s.npcs[0].pending_hits[0], sizeof(hit)) == 0);
    CHECK(s.player.weapon_range == 1 && s.player.weapon_speed == 4 && !s.player.weapon_uses_ammo);
    s.player.attack_timer = 0;
    s.npcs[0].x = 21; s.npcs[0].y = 21;
    fc_step(&s, actions);
    CHECK(!s.render_events.player_attack_fired); /* diagonal is not melee contact */
    s.npcs[0].x = 21; s.npcs[0].y = 20;
    fc_step(&s, actions);
    CHECK(s.render_events.player_attack_fired);
    CHECK(s.render_events.player_attack_hit_delay_ticks == 1);
    CHECK(s.player.attack_timer == 3 && s.player.ammo_count == 0); /* end-of-tick decrement */
    int melee_hit = 0;
    for (int i = 0; i < s.render_events.hit_count; i++)
        if (s.render_events.hits[i].target_entity_type == ENTITY_NPC &&
            s.render_events.hits[i].attack_style == ATTACK_MELEE) melee_hit = 1;
    CHECK(melee_hit); /* delay-one queues resolve during this tick's hit phase */
    CHECK(fc_equip_item(&s, item_slot(&s.player, 20997)) == FC_ITEM_OK);
    CHECK(s.player.attack_timer == 3 && s.player.attack_target_idx == -1);
    int route_x[64], route_y[64];
    int len = fc_pathfind_attack_position(20, 20, 25, 25, 5, FC_ROUTE_MELEE_RANGE,
        s.walkable, s.movement_flags, s.los_flags, route_x, route_y, 64);
    CHECK(len > 0);
    CHECK(fc_npc_can_melee_player(route_x[len-1], route_y[len-1], 25, 25, 5,
                                s.movement_flags));
    return 0;
}

/* Each preset is wearable at exactly its items' Ranged, Defence, Hitpoints,
 * Magic, Attack and Strength requirements and not one level below any of
 * them. At the default levels, or a magic or melee preset's own, it fights: a
 * few hundred ticks of attacking the nearest NPC land hits, and each shot
 * spends one arrow, bolt or loaded dart (the magic tests check the casts and
 * runes, the melee tests the melee attacks). */
static int gear_presets(void) {
    const int skills[] = {FC_SKILL_RANGED, FC_SKILL_DEFENCE, FC_SKILL_HITPOINTS,
                          FC_SKILL_MAGIC, FC_SKILL_ATTACK, FC_SKILL_STRENGTH};
    const int skill_count = (int)(sizeof(skills) / sizeof(skills[0]));
    char err[256], prefix[64], expected[96];
    for (int p = 0; p < FC_GEAR_PRESET_COUNT; p++) {
        const FcGearPreset *preset = &FC_GEAR_PRESETS[p];
        FcScenario scenario = levels_scenario(preset->name);
        FcScenario minimum = preset_scenario(p);
        for (int k = 0; k < skill_count; k++) minimum.levels[skills[k]] = 1;
        for (int i = 0; i < preset->equipment_count; i++) {
            const FcItemDef *item = fc_item_definition(preset->equipment[i]);
            const int needs[] = {item->ranged_level, item->defence_level,
                                 item->hitpoints_level, item->magic_level,
                                 item->attack_level, item->strength_level};
            for (int k = 0; k < skill_count; k++)
                if (needs[k] > minimum.levels[skills[k]])
                    minimum.levels[skills[k]] = needs[k];
        }
        CHECK(fc_scenario_check(&minimum, err, sizeof(err)) == 0);
        snprintf(prefix, sizeof(prefix), "gear_set_1 = %s: ", preset->name);
        for (int k = 0; k < skill_count; k++) {
            int skill = skills[k], need = minimum.levels[skill];
            if (need == 1) continue;
            FcScenario below = minimum;
            below.levels[skill] = need - 1;
            CHECK(fc_scenario_check(&below, err, sizeof(err)) != 0);
            snprintf(expected, sizeof(expected), " needs %d %s, but %s_level = %d",
                     need, FC_SKILL_NAMES[skill], FC_SKILL_NAMES[skill], need - 1);
            CHECK(strncmp(err, prefix, strlen(prefix)) == 0 && strstr(err, expected));
        }

        FcState s;
        CHECK(fc_scenario_check(&scenario, err, sizeof(err)) == 0);
        fc_init(&s);
        fc_reset(&s, 101);
        fc_apply_scenario(&s, &scenario);
        CHECK(s.active_loadout == preset->id);
        int actions[FC_NUM_ACTION_HEADS] = {0};
        actions[FC_HEAD_ATTACK] = 1;
        int ticks = 0, attacks = 0, hits = 0;
        for (; ticks < 300 && !fc_is_terminal(&s); ticks++) {
            fc_step(&s, actions);
            attacks += s.render_events.player_attack_fired;
            hits += s.hits_landed_this_tick;
        }
        CHECK(hits > 0);
        const FcItemStack *weapon = &s.player.equipment[FC_EQUIP_SLOT_WEAPON];
        int left = weapon->item_id == FC_ITEM_ID_TOXIC_BLOWPIPE
            ? weapon->charges : s.player.equipment[FC_EQUIP_SLOT_AMMO].quantity;
        if (preset->runes || s.player.weapon_kind == FC_WEAPON_MELEE) {
            CHECK(!s.player.weapon_uses_ammo && s.player.ammo_count == 0);
        } else {
            CHECK(s.player.ammo_count == left);
            CHECK(left == (s.player.weapon_uses_ammo ? preset->ammo - attacks : 0));
        }
        printf("gear preset %s: wearable from %d ranged, %d defence, %d hitpoints, "
               "%d magic, %d attack, %d strength; %d attacks and %d hits in %d ticks\n",
               preset->name, minimum.levels[FC_SKILL_RANGED],
               minimum.levels[FC_SKILL_DEFENCE], minimum.levels[FC_SKILL_HITPOINTS],
               minimum.levels[FC_SKILL_MAGIC], minimum.levels[FC_SKILL_ATTACK],
               minimum.levels[FC_SKILL_STRENGTH], attacks, hits, ticks);
    }
    return 0;
}

static int equipment_test(void) {
    if (preset_totals() || constant_tables() || transactions() ||
        two_handed_and_stacks() || supplies_and_loaded_weapon() || combat() ||
        gear_presets()) return 1;
    puts("equipment: preset totals, tables, atomic transfers, two-handed swaps, stacks, "
         "ammunition, requirements, supplies, loaded darts and combat passed");
    return 0;
}

#undef CHECK


#include <assert.h>
#include <stdio.h>
#include <string.h>

static int context_menu_test(void) {
    FcMenuLayout menu = fc_menu_layout(400, 200, 800, 600, 150, 4);
    assert(menu.x == 319 && menu.y == 200 && menu.width == 162);
    assert(menu.height == FC_MENU_HEADER_HEIGHT + 4 * FC_MENU_ROW_HEIGHT + 4);
    assert(fc_menu_action_at(menu, 4, 330, 200) == -1); /* title is not a choice */
    for (int row = 0; row < 4; row++) {
        int y = menu.y + FC_MENU_HEADER_HEIGHT + row * FC_MENU_ROW_HEIGHT;
        assert(fc_menu_action_at(menu, 4, 330, y) == row);
        assert(fc_menu_action_at(menu, 4, 330, y + FC_MENU_ROW_HEIGHT - 1) == row);
    }
    assert(fc_menu_action_at(menu, 4, 330, menu.y + menu.height - 1) == -1);
    assert(fc_menu_action_at(menu, 4, menu.x - 1, 230) == -1);
    assert(fc_menu_action_at(menu, 4, menu.x + menu.width, 230) == -1);
    assert(fc_menu_contains(menu, menu.x - 9, 230, FC_MENU_DISMISS_MARGIN));
    assert(!fc_menu_contains(menu, menu.x - 11, 230, FC_MENU_DISMISS_MARGIN));
    menu = fc_menu_layout(799, 599, 800, 600, 220, 3);
    assert(menu.x + menu.width == 800 && menu.y + menu.height == 600);
    assert(fc_menu_action_at(menu, 3, 790, menu.y + FC_MENU_HEADER_HEIGHT) == 0);
    menu = fc_menu_layout(0, 0, 800, 600, 150, 4);
    assert(menu.x == 0 && menu.y == 0);
    menu = fc_menu_layout(0, 0, 80, 200, 150, 4);
    assert(menu.width == 80 && menu.x == 0);
    const int levels[] = {0, 22, 45, 22, 90, 180, 360, 702, 108};
    for (int type = 1; type <= 8; type++)
        assert(fc_menu_npc_info(type).level == levels[type]);
    assert(strcmp(fc_menu_npc_info(7).name, "TzTok-Jad") == 0);
    assert(strcmp(fc_menu_npc_info(2).name, fc_menu_npc_info(3).name) == 0);
    assert(fc_menu_npc_info(0).level == 0 && fc_menu_npc_info(-1).level == 0);
    assert(fc_menu_npc_info(9).level == 0);
    const uint32_t colors[] = {
        0xff0000, 0xff3000, 0xff3000, 0xff3000, 0xff7000, 0xff7000,
        0xff7000, 0xffb000, 0xffb000, 0xffb000, 0xffff00,
        0xc0ff00, 0xc0ff00, 0xc0ff00, 0x80ff00, 0x80ff00, 0x80ff00,
        0x40ff00, 0x40ff00, 0x40ff00, 0x00ff00
    };
    for (int difference = -10; difference <= 10; difference++)
        assert(fc_menu_level_color(100 + difference, 100) == colors[difference + 10]);
    assert(fc_menu_level_color(126, 702) == 0xff0000);
    assert(fc_menu_level_color(126, 108) == 0x00ff00);
    puts("context menu: anchoring, screen edges, rows, title and dismissal margin passed");
    return 0;
}


#include <assert.h>
#include <stdio.h>
#include <string.h>

static void make_open_state(FcState* state) {
    memset(state, 0, sizeof(*state));
    for (int x = 0; x < FC_ARENA_WIDTH; x++) {
        for (int y = 0; y < FC_ARENA_HEIGHT; y++) {
            state->walkable[x][y] = 1;
        }
    }
    state->player.x = 1;
    state->player.y = 1;
}

static int click_feedback_test(void) {
    FcState state;
    FcClickFeedback feedback;
    const int* route_x = NULL;
    const int* route_y = NULL;
    int start = -1;
    int len = -1;

    make_open_state(&state);
    FcState unchanged = state;
    fc_click_feedback_reset(&feedback);
    fc_click_feedback_select_move(&feedback, &state, 5, 3, 120.0f, 80.0f);

    assert(feedback.destination_active);
    assert(feedback.destination_x == 5 && feedback.destination_y == 3);
    assert(feedback.preview_pending);
    assert(feedback.preview_route_len > 0);
    assert(feedback.preview_route_x[feedback.preview_route_len - 1] == 5);
    assert(feedback.preview_route_y[feedback.preview_route_len - 1] == 3);
    assert(feedback.cross_kind == FC_CLICK_CROSS_MOVE);
    assert(fc_click_feedback_cross_frame(&feedback) == 0);
    assert(fc_click_feedback_route(&feedback, &state, &route_x, &route_y,
                                   &start, &len));
    assert(route_x == feedback.preview_route_x);
    assert(route_y == feedback.preview_route_y);
    assert(start == 0 && len == feedback.preview_route_len);

    fc_click_feedback_update(&feedback, 0.11f);
    assert(fc_click_feedback_cross_frame(&feedback) == 1);
    fc_click_feedback_update(&feedback, 0.30f);
    assert(feedback.cross_kind == FC_CLICK_CROSS_NONE);

    state.player.route_x[0] = 2;
    state.player.route_y[0] = 2;
    state.player.route_x[1] = 3;
    state.player.route_y[1] = 3;
    state.player.route_len = 2;
    state.player.route_idx = 1;
    fc_click_feedback_accept_move_tick(&feedback, &state);
    assert(!feedback.preview_pending);
    assert(feedback.destination_active);
    assert(fc_click_feedback_route(&feedback, &state, &route_x, &route_y,
                                   &start, &len));
    assert(route_x == state.player.route_x && route_y == state.player.route_y);
    assert(start == 1 && len == 2);

    state.player.route_idx = state.player.route_len;
    fc_click_feedback_sync(&feedback, &state);
    assert(!feedback.destination_active);

    fc_click_feedback_select_move(&feedback, &state, 7, 7, 10.0f, 20.0f);
    fc_click_feedback_select_interaction(&feedback, 30.0f, 40.0f);
    assert(!feedback.destination_active);
    assert(!feedback.preview_pending);
    assert(feedback.preview_route_len == 0);
    assert(feedback.cross_kind == FC_CLICK_CROSS_INTERACTION);
    assert(feedback.cross_screen_x == 30.0f && feedback.cross_screen_y == 40.0f);
    assert(fc_click_feedback_cross_frame(&feedback) == 0);
    fc_click_feedback_update(&feedback, 0.099f);
    assert(fc_click_feedback_cross_frame(&feedback) == 0);
    fc_click_feedback_update(&feedback, 0.002f);
    assert(fc_click_feedback_cross_frame(&feedback) == 1);
    /* Test only the presentation operation against a complete state snapshot. */
    FcState before_interaction = state;
    fc_click_feedback_select_interaction(&feedback, 70.0f, 80.0f);
    assert(memcmp(&state, &before_interaction, sizeof(state)) == 0);
    fc_click_feedback_select_move(&feedback, &unchanged, 3, 5, 10, 20);
    FcState fresh;
    make_open_state(&fresh);
    assert(memcmp(&fresh, &unchanged, sizeof(fresh)) == 0);

    puts("click feedback tests passed");
    return 0;
}

#include "raymath.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int model_picking_test(void) {
    /* Tall two-triangle actor. The uploaded mesh deliberately belongs to a
     * different actor: picking must use this instance's pose or the rest mesh. */
    float rest[] = {-0.6f,0,0, 0.6f,0,0, 0.6f,4,0, -0.6f,0,0, 0.6f,4,0, -0.6f,4,0};
    float other_actor[18] = {100,100,100};
    uint16_t faces[] = {0,1,2,0,2,3};
    int16_t pose[] = {-77,0,0, 77,0,0, 77,-512,0, -77,-512,0};
    float original_rest[18]; int16_t original_pose[12];
    memcpy(original_rest, rest, sizeof(rest));
    memcpy(original_pose, pose, sizeof(pose));
    Mesh uploaded = {.vertices=other_actor};
    ModelEntry entry = {.rest_verts=rest, .face_count=2,
                       .face_indices=faces, .base_vert_count=4};
    entry.model.transform = MatrixIdentity();
    entry.model.meshes = &uploaded;
    Camera3D camera = {.position={0,4,10}, .target={0,2,0}, .up={0,1,0},
                       .fovy=50, .projection=CAMERA_PERSPECTIVE};
    Vector3 origin = {0};
    Vector2 head = GetWorldToScreenEx((Vector3){0,3.8f,0}, camera, 800, 600);
    assert(models_pick_depth(&entry, NULL, origin, 0, camera, head, 800, 600) > 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera, head, 800, 600) > 0);
    /* A head click projects onto ground far behind the actor: the old tile
     * halo would miss, even though the pointer is inside the visible model. */
    Vector3 ray = Vector3Subtract((Vector3){0,3.8f,0}, camera.position);
    float ground_z = camera.position.z + ray.z * (-camera.position.y / ray.y);
    assert(fabsf(ground_z) > 2);

    Vector2 top = GetWorldToScreenEx((Vector3){0,4,0}, camera, 800, 600);
    assert(models_pick_depth(&entry, pose, origin, 0, camera,
        (Vector2){top.x, top.y - 4}, 800, 600) > 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera,
        (Vector2){top.x, top.y - 6}, 800, 600) < 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera, (Vector2){10,10}, 800,600) < 0);

    /* Moving and turned models are picked where drawn, not at their old tile. */
    Vector3 moved = {4,0,0};
    Vector2 moved_head = GetWorldToScreenEx((Vector3){4,3.8f,0}, camera, 800,600);
    assert(models_pick_depth(&entry, pose, moved, 90, camera, moved_head, 800,600) > 0);
    assert(models_pick_depth(&entry, pose, moved, 90, camera, head, 800,600) < 0);
    /* Two instances of the same model can be in different animation poses. */
    int16_t shifted[12]; memcpy(shifted, pose, sizeof(pose));
    for (int i = 0; i < 4; i++) shifted[i*3] += 512;
    assert(models_pick_depth(&entry, shifted, origin, 0, camera, moved_head, 800,600) > 0);
    assert(models_pick_depth(&entry, shifted, origin, 0, camera, head, 800,600) < 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera, head, 800,600) > 0);

    Vector2 center = GetWorldToScreenEx((Vector3){0,2,0}, camera, 800,600);
    float front = models_pick_depth(&entry, pose, origin, 0, camera, center, 800,600);
    float back = models_pick_depth(&entry, pose, (Vector3){0,0,-4}, 0, camera, center, 800,600);
    assert(front > 0 && back > front); /* overlap ordering */
    assert(models_pick_depth(&entry, pose, (Vector3){0,0,30}, 0, camera, center, 800,600) < 0);
    assert(models_pick_depth(NULL, pose, origin, 0, camera, center, 800,600) < 0);
    assert(memcmp(rest, original_rest, sizeof(rest)) == 0);
    assert(memcmp(pose, original_pose, sizeof(pose)) == 0);
    assert(other_actor[0] == 100 && other_actor[3] == 0);
    puts("model picking: head, padding, rotation, movement, per-instance pose and depth passed");
    return 0;
}

#include <stdio.h>

/* Draws the composed player into cell `cell` of a row of 300x320 cells, posed
 * at `frame` of `sequence` and turned by `yaw` degrees, with a caption. */
static void draw_player_cell(ModelEntry *entry, AnimCache *cache, int cell,
                             int sequence, int frame, float yaw,
                             const char *caption) {
    AnimModelState *pose = NULL;
    AnimTrack track = {0};
    fc_model_animation_update(entry, cache, &pose, &track, sequence, 0, (float)frame);
    /* One orthographic camera, 2.4 tiles over the 320-pixel height and so 2.25
     * across a cell, moved over the cell. */
    float x = (cell - 1.5f) * 2.25f;
    Camera3D camera = {.position={x, 1.55f, -7}, .target={x, 0.95f, 0},
                       .up={0, 1, 0}, .fovy=2.4f, .projection=CAMERA_ORTHOGRAPHIC};
    BeginScissorMode(cell * 300, 0, 300, 320);
    BeginMode3D(camera);
    DrawModelEx(entry->model, (Vector3){0, 0, 0}, (Vector3){0, 1, 0},
                yaw, (Vector3){1, 1, 1}, WHITE);
    EndMode3D();
    DrawText(caption, cell * 300 + 8, 8, 10, RAYWHITE);
    EndScissorMode();
    anim_model_state_free(pose);
}

/* Every gear preset composes from its equipment, and a ranged preset's
 * weapon animations and projectile spot animations ship. The pinned assets
 * predate the magic and melee presets: their items without models are left
 * out of the figure, and their missing stances, attacks, casts and spell
 * graphics are listed; melee weapons fire nothing. With FC_TEST_OUTPUT_DIR
 * set, each is drawn to <preset>.png there: idle from the front and side,
 * then two frames of its attack. */
static int gear_preset_appearances(FcPlayerAppearance *appearance, AnimCache *cache) {
    const char *dir = getenv("FC_TEST_OUTPUT_DIR");
    SpotAnimSet *spots = spotanims_load("fc_spotanims.bin");
    Image white = GenImageColor(1, 1, WHITE);
    Texture2D atlas = LoadTextureFromImage(white); /* the models are only looked up */
    UnloadImage(white);
    ModelSet *projectiles = models_load("fc_projectiles.models", atlas);
    if (!spots || !projectiles) return 1;
    RenderTexture2D strip = LoadRenderTexture(1200, 320);
    FcState state;
    fc_init(&state);
    for (int p = 0; p < FC_GEAR_PRESET_COUNT; p++) {
        const FcGearPreset *preset = &FC_GEAR_PRESETS[p];
        FcScenario scenario = preset_scenario(p);
        fc_reset(&state, 101);
        fc_apply_scenario(&state, &scenario);
        if (fc_player_appearance_sync(appearance, &state.player, FC_PLAYER_MODEL_ID) < 0)
            return 1;
        ModelEntry *entry = appearance->model->entries;
        FcPlayerVisualProfile magic_profile;
        const FcPlayerVisualProfile *profile = fc_player_visual_profile(&state.player);
        if (preset->autocast) {
            /* The spell's cast and projectile with the staff's stance. */
            magic_profile = *profile;
            magic_profile.attack_anim = FC_SPELL_VISUALS[preset->autocast - 1].cast_anim;
            magic_profile.projectile = FC_SPELL_VISUALS[preset->autocast - 1].projectile;
            profile = &magic_profile;
        }
        const uint16_t sequences[] = {
            profile->idle_anim, profile->walk_anim, profile->walk_back_anim,
            profile->walk_left_anim, profile->walk_right_anim, profile->turn_anim,
            profile->run_anim, profile->attack_anim,
        };
        const FcProjectileProfile *shot = &profile->projectile;
        const uint32_t spot_ids[] = {shot->travel_spot, shot->launch_spot, shot->impact_spot};
        int melee = state.player.weapon_kind == FC_WEAPON_MELEE;
        int newer = preset->autocast || melee;   /* the bundle predates it */
        if ((shot->travel_spot == 0) != melee) return 1;
        char missing[256] = "";
        for (int i = 0; i < (int)(sizeof(sequences) / sizeof(sequences[0])); i++) {
            if (anim_get_sequence(cache, sequences[i])) continue;
            if (!newer) return 1;
            snprintf(missing + strlen(missing), sizeof(missing) - strlen(missing),
                     " sequence %d", sequences[i]);
        }
        for (int i = 0; i < 3; i++) {
            if (!spot_ids[i] || (spotanim_find(spots, (int)spot_ids[i]) &&
                model_find(projectiles, PROJ_SPOTANIM_MODEL_BASE + spot_ids[i]))) continue;
            if (!newer) return 1;
            snprintf(missing + strlen(missing), sizeof(missing) - strlen(missing),
                     " spot %u", spot_ids[i]);
        }
        for (int i = 0; i < preset->equipment_count; i++) {
            if (appearance_record(appearance, preset->equipment[i]) >= 0 ||
                fc_item_definition(preset->equipment[i])->slot == FC_EQUIP_SLOT_AMMO ||
                fc_item_definition(preset->equipment[i])->slot == FC_EQUIP_SLOT_RING)
                continue;
            if (!newer) return 1;
            snprintf(missing + strlen(missing), sizeof(missing) - strlen(missing),
                     " item %d", preset->equipment[i]);
        }
        if (missing[0]) printf("appearance preset %s lacks:%s\n", preset->name, missing);
        if (dir) {
            const AnimSequence *attack = anim_get_sequence(cache, profile->attack_anim);
            int frames = attack ? attack->frame_count : 0;
            char captions[4][64], path[512];
            snprintf(captions[0], sizeof(captions[0]), "%s: idle %d, front",
                     preset->name, profile->idle_anim);
            snprintf(captions[1], sizeof(captions[1]), "idle %d, side", profile->idle_anim);
            snprintf(captions[2], sizeof(captions[2]), "attack %d, frame %d of %d",
                     profile->attack_anim, frames / 3, frames);
            snprintf(captions[3], sizeof(captions[3]), "attack %d, frame %d of %d",
                     profile->attack_anim, 2 * frames / 3, frames);
            BeginTextureMode(strip);
            ClearBackground((Color){45, 45, 45, 255});
            draw_player_cell(entry, cache, 0, profile->idle_anim, 0, 180, captions[0]);
            draw_player_cell(entry, cache, 1, profile->idle_anim, 0, 270, captions[1]);
            draw_player_cell(entry, cache, 2, profile->attack_anim, frames / 3, 225,
                             captions[2]);
            draw_player_cell(entry, cache, 3, profile->attack_anim, 2 * frames / 3, 270,
                             captions[3]);
            EndTextureMode();
            Image image = LoadImageFromTexture(strip.texture);
            ImageFlipVertical(&image);
            snprintf(path, sizeof(path), "%s/%s.png", dir, preset->name);
            int ok = ExportImage(image, path);
            UnloadImage(image);
            if (!ok) return 1;
        }
        printf("appearance preset %s: %d vertices, %d faces, idle %d, attack %d, "
               "projectile %u\n", preset->name, entry->base_vert_count,
               entry->face_count, profile->idle_anim, profile->attack_anim,
               shot->travel_spot);
    }
    UnloadRenderTexture(strip);
    models_free(projectiles);
    UnloadTexture(atlas);
    spotanims_free(spots);
    return 0;
}

/* Explicit graphics test, enabled with --render. Captures a contact sheet
 * in the working directory: outfit, no helmet/body/legs/gloves/boots/bow, bare.
 * Then checks every gear preset with gear_preset_appearances. */
static int equipment_appearance_test(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1200, 640, "Equipment appearance validation");
    if (!IsWindowReady()) return 1;
    FcPlayerAppearance appearance;
    if (!fc_player_appearance_load(&appearance)) return 1;
    AnimCache *cache = anim_cache_load("fc_all.anims");
    if (!cache || !anim_get_sequence(cache, 422)) return 1;
    RenderTexture2D target = LoadRenderTexture(1200, 640);
    BeginTextureMode(target);
    ClearBackground((Color){45, 45, 45, 255});
    const int removed[] = {-1, 0, 4, 7, 9, 10, 3, -2};
    FcState state;
    fc_init(&state);
    for (int variant = 0; variant < 8; variant++) {
        fc_reset(&state, 101);
        reset_supplies(&state.player, 0, 0);
        if (removed[variant] == -2) {
            for (int slot = 0; slot < FC_EQUIPMENT_SLOTS; slot++)
                if (state.player.equipment[slot].item_id &&
                    fc_unequip_item(&state, slot) != FC_ITEM_OK) return 1;
        } else if (removed[variant] >= 0 &&
                   fc_unequip_item(&state, removed[variant]) != FC_ITEM_OK) return 1;
        uint32_t before = fc_state_hash(&state);
        if (fc_player_appearance_sync(&appearance, &state.player, FC_PLAYER_MODEL_ID) != 1)
            return 1;
        if (fc_player_appearance_sync(&appearance, &state.player, FC_PLAYER_MODEL_ID) != 0)
            return 1;
        ModelEntry *entry = appearance.model->entries;
        for (int f = 0; f < entry->face_count * 3; f++)
            if (entry->face_indices[f] >= entry->base_vert_count) return 1;
        AnimModelState *pose = NULL;
        AnimTrack track = {0};
        fc_model_animation_update(entry, cache, &pose, &track, 808, 0, 0);
        Camera3D camera = {.position={0, 1.6f, -7}, .target={0, 1.0f, 0},
                           .up={0, 1, 0}, .fovy=4.4f, .projection=CAMERA_ORTHOGRAPHIC};
        BeginScissorMode((variant % 4) * 300, (variant / 4) * 320, 300, 320);
        /* Move the model through one common camera to avoid changing meshes
         * or animation coordinates for the contact-sheet layout. */
        camera.position.x = camera.target.x = ((variant % 4) - 1.5f) * 2.0625f;
        camera.position.y += (variant / 4 ? 1 : -1) * 1.1f;
        camera.target.y += (variant / 4 ? 1 : -1) * 1.1f;
        BeginMode3D(camera);
        DrawModelEx(entry->model, (Vector3){0, 0, 0}, (Vector3){0, 1, 0},
                    180, (Vector3){1, 1, 1}, WHITE);
        EndMode3D();
        EndScissorMode();
        anim_model_state_free(pose);
        if (fc_state_hash(&state) != before) return 1;
        printf("appearance variant %d: %d vertices, %d faces\n",
               variant, entry->base_vert_count, entry->face_count);
    }
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    int ok = ExportImage(image, getenv("FC_TEST_OUTPUT") ? getenv("FC_TEST_OUTPUT") : "equipment-appearance.png");
    UnloadImage(image);
    UnloadRenderTexture(target);
    if (ok && gear_preset_appearances(&appearance, cache)) ok = 0;
    anim_cache_free(cache);
    fc_player_appearance_free(&appearance);
    CloseWindow();
    return ok ? 0 : 1;
}


#undef CHECK

_Static_assert(OBS_SIZE == FC_PUFFER_OBS_SIZE && NUM_ATNS == FC_PUFFER_NUM_ATNS,
               "the Puffer contract is the core's");
#if FC_DEFAULT_CAPABILITIES
_Static_assert(OBS_SIZE == 320, "observation contract changed");
_Static_assert(NUM_ATNS == 3, "action head count changed");
#endif
_Static_assert(sizeof(obs_t) == sizeof(float), "observations must be floats");
_Static_assert(offsetof(Log, n) + sizeof(float) == sizeof(Log), "n must be last");

/* Set gear_set_2 .. gear_set_N, which a build with gear_sets = N needs, to
 * preset; without gear sets this sets nothing. */
static void other_gear_sets(Dict* kwargs, const char* preset) {
    for (int k = 2; k <= FC_CAP_GEAR_SETS; k++) {
        char key[32];
        snprintf(key, sizeof(key), "gear_set_%d", k);
        puf_ini_set(kwargs, key, preset);
    }
}

static FightCaves* test_env(Dict* kwargs) {
    FightCaves* env = calloc(1, sizeof(*env));
    assert(env);
    env->rng = 73;
    puf_init(env, kwargs);
    /* Match Puffer: initialize before assigning agent buffers. */
    assert(env->num_agents == 1 && env->agents[0].policy == 0);
    assert(env->tag == 0 && env->boundary_reached == 0 && env->rng == 73);
    assert(env->viewer == NULL && env->agents[0].observations == NULL);
    Agent* a = &env->agents[0];
    a->observations = calloc(OBS_SIZE, sizeof(obs_t));
    a->actions = calloc(NUM_ATNS, sizeof(float));
    a->rewards = calloc(1, sizeof(float));
    a->terminals = calloc(1, sizeof(float));
    a->action_mask = calloc(FC_PUFFER_MASK_SIZE, 1);
    assert(a->observations && a->actions && a->rewards && a->terminals && a->action_mask);
    puf_reset(env);
    return env;
}

static void test_free(FightCaves* env) {
    puf_close(env);
    Agent* a = &env->agents[0];
    free(a->observations);
    free(a->actions);
    free(a->rewards);
    free(a->terminals);
    free(a->action_mask);
    free(env);
}

static void check_obs(FightCaves* env) {
    float obs[FC_POLICY_OBS_SIZE], mask[FC_ACTION_MASK_SIZE];
    fc_write_obs(&env->state, obs);
    fc_write_mask(&env->state, mask);
    Agent* a = &env->agents[0];
    assert(memcmp(obs, a->observations, FC_POLICY_OBS_SIZE * sizeof(float)) == 0);
    /* Each policy head's mask segment, in policy order. */
    int i = 0;
    for (int h = 0; h < FC_PUFFER_NUM_ATNS; h++) {
        int head = FC_PUFFER_ACTION_HEADS[h];
        assert(FC_PUFFER_ACTION_DIMS[h] == FC_ACTION_DIMS[head]);
        for (int v = 0; v < FC_ACTION_DIMS[head]; v++, i++) {
            float expected = mask[FC_MASK_HEAD_START[head] + v];
            assert(a->observations[FC_POLICY_OBS_SIZE + i] == expected);
            assert(a->action_mask[i] == (expected != 0.0f));
        }
    }
    assert(i == FC_PUFFER_MASK_SIZE);
}

/* Independent core calls check action translation and returned outputs. */
static void checked_step(FightCaves* env) {
    FcState expected = env->state;
    FcRewardRuntime runtime = env->reward_runtime;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    for (int h = 0; h < NUM_ATNS; h++)
        actions[FC_PUFFER_ACTION_HEADS[h]] = (int)env->agents[0].actions[h];
    fc_step(&expected, actions);
    FcRewardBreakdown reward = fc_reward_compute_breakdown(
        &expected, &env->reward_params, &runtime);
    fc_reward_sync_progress_state(&expected, &runtime);
    float prior_episodes = env->log.n;
    uint32_t prior_seed_counter = env->seed_counter;
    int terminal = fc_is_terminal(&expected);
    puf_step(env);
    assert(env->agents[0].rewards[0] == reward.total);
    assert(env->agents[0].terminals[0] == (float)terminal);
    assert(env->log.n == prior_episodes + terminal);
    if (terminal) {
        assert(env->seed_counter == prior_seed_counter + 1);
        assert(env->state.tick == 0 && !fc_is_terminal(&env->state));
        assert(env->state.player.current_hp > 0);
    } else {
        assert(fc_state_hash(&expected) == fc_state_hash(&env->state));
        assert(env->seed_counter == prior_seed_counter);
    }
    check_obs(env);
    if (env->viewer) {
        assert(env->viewer->pending_frame);
        assert(fc_state_hash(&env->viewer->pending_state) == fc_state_hash(&expected));
        assert(env->viewer->pending_reward_breakdown.total == reward.total);
        assert(memcmp(env->viewer->pending_actions, actions, sizeof(actions)) == 0);
    }
}

static void native_config_test(void) {
    Ini ini = {0};
    puf_ini_load_env(&ini, "fight_caves", 0, NULL);
    const struct { const char* section; const char* key; double value; } expected[] = {
        {"base", "seed", 73}, {"base", "cudagraphs", 1},
        {"base", "reset_every_horizon", 1}, {"base", "async", 0},
        {"vec", "total_agents", 4096}, {"vec", "num_buffers", 2},
        {"vec", "num_threads", 16}, {"policy", "hidden_size", 512},
        {"policy", "num_layers", 3}, {"train", "total_timesteps", 500000000},
        {"train", "horizon", 256}, {"train", "minibatch_size", 32768},
        {"train", "learning_rate", 0.000800000038},
        {"train", "ent_coef", 0.000221859314},
        {"train", "replay_ratio", 2.08926511},
        {"train", "clip_coef", 0.0466209911},
        {"train", "vf_coef", 0.158951879},
        {"train", "vf_clip_coef", 0.185629278},
        {"train", "max_grad_norm", 0.400000006},
        {"train", "momentum", 0.994000018}, {"train", "vtrace", 1},
        {"train", "vtrace_rho_clip", 2.0},
        {"train", "vtrace_c_clip", 0.9746667741536915},
    };
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
        assert(puf_ini_get(&ini, expected[i].section, expected[i].key) == expected[i].value);
    assert(strcmp(puf_ini_get_str(&ini, "sweep", "metric"), "jad_kill_rate") == 0);
    int agents = (int)puf_ini_get(&ini, "vec", "total_agents");
    int horizon = (int)puf_ini_get(&ini, "train", "horizon");
    int minibatch = (int)puf_ini_get(&ini, "train", "minibatch_size");
    assert(minibatch % horizon == 0 && agents % (minibatch / horizon) == 0);
    assert(horizon % 8 == 0 && minibatch <= agents * horizon);
    /* The compiled reward defaults are the shipped [env] values. */
    FcRewardParams defaults = fc_reward_default_params();
    for (int i = 0; i < FC_REWARD_SETTING_COUNT; i++) {
        FcRewardParams shipped = defaults;
        fc_reward_setting_store(&shipped, &FC_REWARD_SETTINGS[i],
                                puf_ini_get(&ini, "env", FC_REWARD_SETTINGS[i].key));
        assert(memcmp(&shipped, &defaults, sizeof(defaults)) == 0);
    }
    puf_ini_free(&ini);

    /* Native CLI overrides must apply after the environment file. */
    char* args[] = {"--train.total_timesteps=100_000_000", "--train.momentum=0.95",
                   "--train.vtrace=0", "--base.reset_every_horizon=0", "--base.async=1"};
    puf_ini_load_env(&ini, "fight_caves", 5, args);
    assert(puf_ini_get(&ini, "train", "total_timesteps") == 100000000);
    assert(puf_ini_get(&ini, "train", "momentum") == 0.95);
    assert(puf_ini_get(&ini, "train", "vtrace") == 0);
    assert(puf_ini_get(&ini, "base", "reset_every_horizon") == 0);
    assert(puf_ini_get(&ini, "base", "async") == 1);
    assert(puf_ini_get(&ini, "env", "shape_npc_heal_penalty") == -0.005);
    assert(puf_ini_get(&ini, "env", "shape_no_attack_start") == 50);
    puf_ini_free(&ini);
    puts("native config: merged settings, reward defaults, batch geometry and CLI precedence passed");
}

static void init_and_config_test(void) {
    /* No settings but the gear sets past the first, which gear_sets needs. */
    Dict empty = {0};
    other_gear_sets(&empty, FC_GEAR_PRESETS[0].name);
    Env* probe = calloc(1, sizeof(*probe));
    assert(probe);
    puf_init(probe, &empty);
    puf_close(probe); /* Native graphical evaluation does this without buffers. */
    free(probe);

    int sizes[] = ACT_SIZES;
    assert(sizes[0] == 17 && sizes[1] == 9 && sizes[2] == 8);
    Env* env = test_env(&empty);
    FcRewardParams defaults = fc_reward_default_params();
    assert(env->reward_params.w_progress == defaults.w_progress);
    assert(env->scenario.sharks == 0 && env->scenario.prayer_doses == 0);
    check_obs(env);
    test_free(env);
    dict_clear(&empty);

    Dict overrides = {0};
    other_gear_sets(&overrides, FC_GEAR_PRESETS[0].name);
    dict_set(&overrides, "w_progress", 0.25);
    dict_set(&overrides, "shape_no_attack_start", 17);
    dict_set(&overrides, "initial_sharks", FC_MAX_SHARKS + 1);
    dict_set(&overrides, "initial_prayer_doses", -1);
    env = test_env(&overrides);
    assert(env->reward_params.w_progress == 0.25f);
    assert(env->reward_params.shape_no_attack_start == 17);
    assert(env->reward_params.w_player_death == defaults.w_player_death);
    assert(env->state.player.sharks_remaining == FC_MAX_SHARKS);
    assert(env->state.player.prayer_doses_remaining == 0);
    check_obs(env);
    checked_step(env);
    test_free(env);
    dict_clear(&overrides);
    puts("adapter init/config: bufferless probe, defaults, overrides and supplies passed");
}

static void terminal_and_log_test(void) {
    Dict empty = {0};
    other_gear_sets(&empty, FC_GEAR_PRESETS[0].name);
    Env* env = test_env(&empty);
    env->viewer = calloc(1, sizeof(*env->viewer));
    assert(env->viewer);
    /* Isolate terminal rewards so reset cannot silently erase them. */
    memset(&env->reward_params, 0, sizeof(env->reward_params));
    env->reward_params.w_player_death = -2.0f;
    env->reward_params.w_cave_complete = 3.0f;
    env->state.player.current_hp = 0;
    checked_step(env);
    assert(env->viewer->pending_state.terminal == TERMINAL_PLAYER_DEATH);
    assert(env->agents[0].rewards[0] == -2.0f && env->agents[0].terminals[0] == 1.0f);
    checked_step(env);
    assert(env->agents[0].rewards[0] == 0.0f && env->agents[0].terminals[0] == 0.0f);

    env->state.current_wave = FC_NUM_WAVES;
    env->state.npcs_remaining = 0;
    env->state.ep_jad_killed = 1; /* Fixture: Jad is dead and the last wave is empty. */
    memset(env->state.npcs, 0, sizeof(env->state.npcs));
    checked_step(env);
    assert(env->viewer->pending_state.terminal == TERMINAL_CAVE_COMPLETE);
    assert(env->agents[0].rewards[0] == 3.0f && env->agents[0].terminals[0] == 1.0f);
    assert(env->log.n == 2.0f && env->log.jad_kill_rate == 1.0f);
    assert(env->viewer->reset_state.tick == 0);

    /* Puffer normalizes Log before calling puf_log; pass known averaged values. */
    Log averaged = {.jad_kill_rate = 0.5f, .episode_length = 123.0f, .n = 1.0f};
    Dict out = {0};
    puf_log(&averaged, &out);
    assert(out.size == 13);
    assert(dict_get(&out, "perf") == 0.5 && dict_get(&out, "score") == 0.5);
    assert(dict_get(&out, "jad_kill_rate") == 0.5);
    assert(dict_get(&out, "episode_length") == 123.0);
    dict_clear(&out);
    checked_step(env);
    assert(env->log.n == 2.0f);
    free(env->viewer);
    env->viewer = NULL;
    test_free(env);
    dict_clear(&empty);
    puts("adapter terminals/logs: death, completion, autoreset, snapshots and perf/score passed");
}

static uint32_t digest(uint32_t hash, const void* data, size_t size) {
    const unsigned char* bytes = data;
    for (size_t i = 0; i < size; i++) hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
}

static void replay_controls_test(FightCaves* env) {
    ViewerState* v = env->viewer;
    assert(v->policy_replay);
    uint32_t hash = fc_state_hash(&v->state);
    queue_player_tile_request(v, 20, 20, 0, 0);
    queue_player_attack_request(v, 0, 0, 0);
    use_inventory_slot(v, 0);
    viewer_jump_to_wave(v, 63);
    toggle_godmode(v);
    assert(v->pending_tile_x == -1 && v->pending_attack_npc == -1);
    assert(!v->godmode && fc_state_hash(&v->state) == hash);
    v->paused = 1;
    assert(fc_viewer_frame(v) == 0);
    v->step_once = 1;
    assert(fc_viewer_frame(v) == 1);
    assert(fc_viewer_frame(v) == 0);
    assert(fc_state_hash(&v->state) == hash);
    assert(fc_state_hash(&env->state) == hash);
    v->paused = 0;
    puts("native replay controls: read-only gameplay, pause and single-step passed");
}

static void expect_config_error(Dict* kwargs, const char* expected) {
    FcRewardParams reward;
    FcScenario scenario;
    char err[512] = {0};
    assert(fc_read_env_config(kwargs, &reward, &scenario, err, sizeof(err)) != 0);
    if (strstr(err, expected) == NULL) {
        fprintf(stderr, "config error \"%s\" lacks \"%s\"\n", err, expected);
        abort();
    }
}

static void scenario_config_test(void) {
    Dict kwargs = {0};
    /* A capability key that is set must say what this build has. */
    dict_set(&kwargs, "consumables", !FC_CAP_CONSUMABLES);
    expect_config_error(&kwargs, FC_CAP_CONSUMABLES
        ? "consumables = 0, but this build has consumables = 1"
        : "consumables = 1, but this build has consumables = 0");
    dict_set(&kwargs, "consumables", 2);
    expect_config_error(&kwargs, "consumables = 2: must be 0 or 1");
    dict_set(&kwargs, "consumables", FC_CAP_CONSUMABLES);
    /* Named values match without case. */
    puf_ini_set(&kwargs, "prayers", FC_CAP_PRAYERS ? "Protection" : "ALL");
    expect_config_error(&kwargs, FC_CAP_PRAYERS
        ? "prayers = protection, but this build has prayers = all"
        : "prayers = all, but this build has prayers = protection");
    puf_ini_set(&kwargs, "prayers", "1");
    expect_config_error(&kwargs, "prayers = 1: must be protection or all");
    puf_ini_set(&kwargs, "prayers", FC_CAP_PRAYERS ? "all" : "protection");
    dict_set(&kwargs, "gear_sets", FC_CAP_GEAR_SETS == 1 ? 2 : 1);
    expect_config_error(&kwargs, FC_CAP_GEAR_SETS == 1
        ? "gear_sets = 2, but this build has gear_sets = 1"
        : "gear_sets = 1, but this build has gear_sets = ");
    dict_set(&kwargs, "gear_sets", 5);
    expect_config_error(&kwargs, "gear_sets = 5: must be 1 or 2 or 3 or 4");
    dict_set(&kwargs, "gear_sets", FC_CAP_GEAR_SETS);
    static const char *const books[] = {"none", "standard", "ancient"};
    char wrong_book[96];
    puf_ini_set(&kwargs, "spellbook", books[(FC_CAP_SPELLBOOK + 1) % 3]);
    snprintf(wrong_book, sizeof(wrong_book), "spellbook = %s, but this build has spellbook = %s",
             books[(FC_CAP_SPELLBOOK + 1) % 3], books[FC_CAP_SPELLBOOK]);
    expect_config_error(&kwargs, wrong_book);
    puf_ini_set(&kwargs, "spellbook", "2");
    expect_config_error(&kwargs, "spellbook = 2: must be none or standard or ancient");
    puf_ini_set(&kwargs, "spellbook", FC_CAP_SPELLBOOK == 2 ? "Ancient" : books[FC_CAP_SPELLBOOK]);
    dict_set(&kwargs, "special_attacks", !FC_CAP_SPECIAL_ATTACKS);
    expect_config_error(&kwargs, FC_CAP_SPECIAL_ATTACKS
        ? "special_attacks = 0, but this build has special_attacks = 1"
        : "special_attacks = 1, but this build has special_attacks = 0");
    dict_set(&kwargs, "special_attacks", 2);
    expect_config_error(&kwargs, "special_attacks = 2: must be 0 or 1");
    dict_set(&kwargs, "special_attacks", FC_CAP_SPECIAL_ATTACKS);

    /* Every gear set up to gear_sets names a preset, and no other is set. */
    Dict gear = {0};
    if (FC_CAP_GEAR_SETS > 1)
        expect_config_error(&gear, "gear_set_2 is missing: gear_sets = ");
    other_gear_sets(&gear, "tbow_masori");
    if (FC_CAP_GEAR_SETS < FC_MAX_GEAR_SETS) {
        char key[32], expected[96];
        snprintf(key, sizeof(key), "gear_set_%d", FC_CAP_GEAR_SETS + 1);
        snprintf(expected, sizeof(expected), "%s is set, but this build has "
                 "gear_sets = %d", key, FC_CAP_GEAR_SETS);
        puf_ini_set(&gear, key, "tbow_masori");
        expect_config_error(&gear, expected);
    }
    dict_clear(&gear);
    puf_ini_set(&gear, "gear_set_4", "dragon_claws");
    expect_config_error(&gear, "gear_set_4 = dragon_claws: no such gear preset");
    dict_clear(&gear);
    other_gear_sets(&kwargs, "tbow_masori");

    puf_ini_set(&kwargs, "gear_set_1", "dragon_claws");
    expect_config_error(&kwargs, "gear_set_1 = dragon_claws: no such gear preset; "
                        "the presets are tbow_masori rcb_black_dhide rcb_pure "
                        "msb_pure blowpipe_pure acb_armadyl bowfa_crystal "
                        "tbow_masori_no_ring smoke_staff_mystic "
                        "kodai_virtus harmonised_ancestral dragon_scimitar_torso "
                        "macuahuitl_blood_moon fang_bandos scythe_torva");
    puf_ini_set(&kwargs, "gear_set_1", "tbow_masori");
    dict_set(&kwargs, "defence_level", 0);
    expect_config_error(&kwargs, "defence_level = 0: levels must be 1 to 99");
    dict_set(&kwargs, "defence_level", 99);
    dict_set(&kwargs, "ranged_level", 70);
    expect_config_error(&kwargs, "gear_set_1 = tbow_masori: Masori mask (f) needs "
                        "80 ranged, but ranged_level = 70");

    /* Levels set hitpoints, prayer points and the prayer potion restore. */
    dict_set(&kwargs, "ranged_level", 90);
    dict_set(&kwargs, "hitpoints_level", 80);
    dict_set(&kwargs, "prayer_level", 70);
    dict_set(&kwargs, "initial_prayer_doses", 4);
    FightCaves* env = test_env(&kwargs);
    const FcPlayer* p = &env->state.player;
    assert(p->ranged_level == 90 && p->max_hp == 800 && p->current_hp == 800);
    assert(p->prayer_level == 70 && p->max_prayer == 700 && p->current_prayer == 700);
    assert(p->prayer_doses_remaining == 4 && p->sharks_remaining == 0);
    assert(env->state.active_loadout == FC_LOADOUT_SOTA_TBOW);
    check_obs(env);
    test_free(env);

    /* Another preset by name, at the levels its gear needs. */
    puf_ini_set(&kwargs, "gear_set_1", "blowpipe_pure");
    other_gear_sets(&kwargs, "blowpipe_pure");
    dict_set(&kwargs, "defence_level", 1);
    dict_set(&kwargs, "hitpoints_level", 74);
    expect_config_error(&kwargs, "gear_set_1 = blowpipe_pure: Necklace of anguish "
                        "needs 75 hitpoints, but hitpoints_level = 74");
    dict_set(&kwargs, "hitpoints_level", 75);
    env = test_env(&kwargs);
    p = &env->state.player;
    assert(env->state.active_loadout == FC_LOADOUT_BLOWPIPE_PURE);
    assert(p->equipment[FC_EQUIP_SLOT_WEAPON].item_id == FC_ITEM_ID_TOXIC_BLOWPIPE);
    assert(p->defence_level == 1 && p->max_hp == 750 && p->ammo_count == 50000);
    check_obs(env);
    test_free(env);
    dict_clear(&kwargs);
    puts("scenario config: capability check, presets, levels and requirements passed");
}

#if FC_CAP_PRAYERS
#define BOOK(prayer) (1u << FC_BOOK_##prayer)

/* An idle cave for the prayer book rules: no NPC to attack or drain the
 * player, and a wave that is not cleared. Prayer bonus 0 gives the drain
 * resistance of RuneC's tests, 60. */
static void prayer_fixture(FcState* s, int prayer_level, int defence_level) {
    fc_init(s);
    fc_reset(s, 101);
    memset(s->npcs, 0, sizeof(s->npcs));
    s->npcs_remaining = 1;
    s->player.prayer_level = prayer_level;
    s->player.max_prayer = s->player.current_prayer = 10 * prayer_level;
    s->player.defence_level = defence_level;
    s->player.prayer_bonus = 0;
}

/* One tick toggling FcBookPrayer prayer (-1 for none) with an overhead
 * FC_PRAYER_* action. */
static void book_step(FcState* s, int prayer, int overhead) {
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_PRAYER] = overhead;
    actions[FC_HEAD_PRAYER_BOOK] = prayer + 1;
    fc_step(s, actions);
}

/* The book prayers that are off and could be switched on now. */
static uint32_t book_available(const FcState* s) {
    uint32_t available = 0;
    for (int b = 0; b < FC_BOOK_PRAYER_COUNT; b++)
        if (!(s->caps.prayers & (1u << b)) && fc_book_action_valid(s, b + 1))
            available |= 1u << b;
    return available;
}

/* The prayer book rules against RuneC_v4: rc-core/prayer.c and
 * combat_formula.c, and the expected values of
 * tests/test_prayer_spell_actions_runtime.c (prayer runtime) and
 * tests/test_combat_phase5_formula_core.c (formula core). */
static void prayer_book_test(void) {
    const uint32_t all = (1u << FC_BOOK_PRAYER_COUNT) - 1;
    FcState s;
    FcPrayerBonus bonus;

    /* The definitions: prayer runtime test_definition_validation's groups, by
     * RuneC id less Protect Item, the protection prayers, Retribution,
     * Redemption and Smite, and main's spot checks. */
    static const int groups[FC_BOOK_PRAYER_COUNT] =
        {1, 2, 4, 6, 6, 1, 2, 4, 0, 0, 6, 6, 1, 2, 4, 6, 6, 0, 7, 6, 6, 7, 7, 7};
    for (int b = 0; b < FC_BOOK_PRAYER_COUNT; b++)
        assert(FC_BOOK_PRAYERS[b].groups == groups[b]);
    assert(FC_BOOK_PRAYERS[FC_BOOK_AUGURY].level == 77 &&
           FC_BOOK_PRAYERS[FC_BOOK_AUGURY].magic_damage == 4);
    assert(FC_BOOK_PRAYERS[FC_BOOK_DEADEYE].defence == 5 &&
           FC_BOOK_PRAYERS[FC_BOOK_DEADEYE].ranged_strength == 18);
    assert(FC_BOOK_PRAYERS[FC_BOOK_THICK_SKIN].drain == 1);
    s.caps.prayers = BOOK(PIETY);
    bonus = fc_prayer_bonus(&s);
    assert(bonus.attack == 20 && bonus.strength == 23 && bonus.defence == 25);
    s.caps.prayers = BOOK(RIGOUR);
    bonus = fc_prayer_bonus(&s);
    assert(bonus.ranged_attack == 20 && bonus.ranged_strength == 23);
    s.caps.prayers = BOOK(AUGURY);
    assert(fc_prayer_bonus(&s).magic_defence == 25);

    /* Requirements (requirements() and rc_prayer_available): Prayer 1 allows
     * only Thick Skin; Piety needs Prayer 70, then Defence 70 (RC_PRAYER_LEVEL,
     * then RC_PRAYER_DEFENCE); Chivalry needs Defence 65. */
    prayer_fixture(&s, 1, 1);
    assert(book_available(&s) == BOOK(THICK_SKIN));
    prayer_fixture(&s, 99, 64);
    assert(book_available(&s) == (all & ~(BOOK(CHIVALRY) | BOOK(PIETY) |
           BOOK(RIGOUR) | BOOK(AUGURY) | BOOK(EAGLE_EYE) | BOOK(MYSTIC_MIGHT))));
    s.player.defence_level = 69;
    assert(book_available(&s) & BOOK(CHIVALRY));
    assert(!(book_available(&s) & (BOOK(PIETY) | BOOK(RIGOUR) | BOOK(AUGURY))));
    s.player.defence_level = 70;
    assert(book_available(&s) == (all & ~(BOOK(EAGLE_EYE) | BOOK(MYSTIC_MIGHT))));
    /* Deadeye (Prayer 62) and Mystic Vigour (63) replace Eagle Eye and Mystic
     * Might once available (RC_PRAYER_REPLACED). */
    const uint32_t eyes = BOOK(EAGLE_EYE) | BOOK(DEADEYE) | BOOK(MYSTIC_MIGHT) |
                          BOOK(MYSTIC_VIGOUR);
    s.player.prayer_level = 61;
    assert((book_available(&s) & eyes) == (BOOK(EAGLE_EYE) | BOOK(MYSTIC_MIGHT)));
    s.player.prayer_level = 62;
    assert((book_available(&s) & eyes) == (BOOK(DEADEYE) | BOOK(MYSTIC_MIGHT)));
    s.player.prayer_level = 63;
    assert((book_available(&s) & eyes) == (BOOK(DEADEYE) | BOOK(MYSTIC_VIGOUR)));
    /* Nothing switches on without Prayer points (RC_PRAYER_EMPTY); switching
     * a prayer off always works, even one whose levels are not met. */
    s.player.current_prayer = 0;
    assert(book_available(&s) == 0 && fc_book_action_valid(&s, FC_PRAYER_BOOK_NO_CHANGE));
    assert(!fc_book_action_valid(&s, -1) && !fc_book_action_valid(&s, FC_PRAYER_BOOK_DIM));
    prayer_fixture(&s, 1, 1);
    s.caps.prayers = BOOK(RIGOUR);
    assert(fc_book_action_valid(&s, FC_BOOK_RIGOUR + 1));
    book_step(&s, FC_BOOK_RIGOUR, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == 0 && s.player.prayer_changed_this_tick);
    book_step(&s, FC_BOOK_PIETY, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == 0 && !s.player.prayer_changed_this_tick);

    /* Conflicts (select_prayer; prayer runtime test_collision_matrix):
     * switching b on after a leaves b, and a too when they share no group. */
    prayer_fixture(&s, 99, 99);
    const uint32_t available = book_available(&s);
    for (int a = 0; a < FC_BOOK_PRAYER_COUNT; a++) {
        for (int b = 0; b < FC_BOOK_PRAYER_COUNT; b++) {
            if (a == b || !(available & (1u << a)) || !(available & (1u << b)))
                continue;
            s.caps.prayers = 1u << a;
            assert(fc_book_apply_action(&s, b + 1));
            uint32_t expected = 1u << b;
            if (!(FC_BOOK_PRAYERS[a].groups & FC_BOOK_PRAYERS[b].groups))
                expected |= 1u << a;
            assert(s.caps.prayers == expected);
        }
    }
    /* test_requirements_and_lifecycle's sequence through the head; the
     * overhead has its own head and stays on. */
    prayer_fixture(&s, 99, 70);
    book_step(&s, FC_BOOK_PIETY, FC_PRAYER_MELEE);
    assert(s.caps.prayers == BOOK(PIETY) && s.player.prayer == PRAYER_PROTECT_MELEE);
    book_step(&s, FC_BOOK_MYSTIC_VIGOUR, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == BOOK(MYSTIC_VIGOUR));
    book_step(&s, FC_BOOK_MYSTIC_MIGHT, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == BOOK(MYSTIC_VIGOUR));
    book_step(&s, FC_BOOK_BURST_OF_STRENGTH, FC_PRAYER_NO_CHANGE);
    book_step(&s, FC_BOOK_SHARP_EYE, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == BOOK(SHARP_EYE));
    book_step(&s, FC_BOOK_THICK_SKIN, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == (BOOK(SHARP_EYE) | BOOK(THICK_SKIN)));
    assert(s.player.prayer == PRAYER_PROTECT_MELEE);
    /* Prayer runtime main: Thick Skin and Deadeye together add their Defence
     * to 10; Rigour then replaces both. */
    prayer_fixture(&s, 99, 99);
    book_step(&s, FC_BOOK_THICK_SKIN, FC_PRAYER_NO_CHANGE);
    book_step(&s, FC_BOOK_DEADEYE, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == (BOOK(THICK_SKIN) | BOOK(DEADEYE)));
    assert(fc_prayer_bonus(&s).defence == 10);
    book_step(&s, FC_BOOK_RIGOUR, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == BOOK(RIGOUR));

    /* Drain (rc_prayer_drain_tick; test_drain_resource_and_quick): Thick Skin,
     * 1 per tick against resistance 60, starts the tick after it goes on; 60
     * ticks reach counter 60 with no point lost, the 61st loses one. */
    prayer_fixture(&s, 99, 99);
    book_step(&s, FC_BOOK_THICK_SKIN, FC_PRAYER_NO_CHANGE);
    assert(s.player.prayer_drain_counter == 0);
    for (int i = 0; i < 60; i++) book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_prayer == 990 && s.player.prayer_drain_counter == 60);
    book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_prayer == 980 && s.player.prayer_drain_counter == 1);
    /* test_transition_edges: a prayer switched on while another drains is
     * charged from that tick, one switched off is not. */
    prayer_fixture(&s, 99, 99);
    book_step(&s, FC_BOOK_THICK_SKIN, FC_PRAYER_NO_CHANGE);
    book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.prayer_drain_counter == 1);
    book_step(&s, FC_BOOK_BURST_OF_STRENGTH, FC_PRAYER_NO_CHANGE);
    assert(s.player.prayer_drain_counter == 3);
    book_step(&s, FC_BOOK_BURST_OF_STRENGTH, FC_PRAYER_NO_CHANGE);
    assert(s.player.prayer_drain_counter == 4);
    /* With the overhead: Protect from Melee 12 and Piety 24 share the counter,
     * 36 a tick from the tick after they go on: 36, 72 (a point), 48, 84 (a
     * point), 60. */
    prayer_fixture(&s, 99, 99);
    book_step(&s, FC_BOOK_PIETY, FC_PRAYER_MELEE);
    assert(s.player.prayer_drain_counter == 0);
    for (int i = 0; i < 5; i++) book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_prayer == 970 && s.player.prayer_drain_counter == 60);
    /* A book prayer on for one tick costs nothing. A flick keeps the drain
     * while a book prayer stays on, since the set never empties; without one
     * it does not drain, as before. */
    prayer_fixture(&s, 99, 99);
    book_step(&s, FC_BOOK_PIETY, FC_PRAYER_NO_CHANGE);
    book_step(&s, FC_BOOK_PIETY, FC_PRAYER_NO_CHANGE);
    assert(s.caps.prayers == 0 && s.player.prayer_drain_counter == 0);
    book_step(&s, FC_BOOK_RAPID_RESTORE, FC_PRAYER_MELEE);
    book_step(&s, -1, FC_PRAYER_FLICK_MAGIC);
    assert(s.player.prayer == PRAYER_PROTECT_MAGIC && s.player.prayer_drain_counter == 13);
    book_step(&s, FC_BOOK_RAPID_RESTORE, FC_PRAYER_NO_CHANGE);
    assert(s.player.prayer_drain_counter == 25);
    book_step(&s, -1, FC_PRAYER_FLICK_MELEE);
    assert(s.player.prayer_drain_counter == 25);
    /* Running out switches every prayer off and clears the counter, from the
     * drain and from any other Prayer loss. */
    prayer_fixture(&s, 99, 99);
    book_step(&s, FC_BOOK_PIETY, FC_PRAYER_MELEE);
    s.player.current_prayer = 10;
    s.player.prayer_drain_counter = 50;
    book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_prayer == 0 && s.player.prayer == PRAYER_NONE);
    assert(s.caps.prayers == 0 && s.player.prayer_drain_counter == 0);
    s.player.current_prayer = 500;
    s.caps.prayers = BOOK(PIETY);
    assert(fc_prayer_apply_loss_tenths(&s, 600) == 500 && s.caps.prayers == 0);

    /* Combat percentages, as combat_formula.c applies them to the level before
     * the stance and 8. Ranged, formula core
     * test_ranged_formula_uses_ranged_prayers_and_strength_bonus: Ranged 75,
     * Rigour, rapid, +90 attack and +40 strength: levels 98 and 100, roll
     * 15092, max hit 16. */
    FcNpc target = {.npc_type = NPC_TZ_KIH};
    int roll, max_hit;
    FcPlayer ranged = {.weapon_kind = FC_WEAPON_GENERIC_RANGED, .ranged_level = 75,
                       .ranged_attack_bonus = 90, .ranged_strength_bonus = 40};
    s.caps.prayers = BOOK(RIGOUR);
    bonus = fc_prayer_bonus(&s);
    fc_player_ranged_rolls(&ranged, &bonus, &target, &roll, &max_hit);
    assert(roll == 15092 && max_hit == 16);
    /* Melee, test_melee_formula_uses_boosts_prayer_equipment_and_npc_defence's
     * Piety, Attack 70, Strength 80, Aggressive, +100 attack and +50
     * strength: Attack 70 * 120 / 100 + 8 = 92 and Strength
     * 80 * 123 / 100 + 3 + 8 = 109, so roll 92 * 164 = 15088 and max hit
     * (109 * 114 + 320) / 640 = 19; the Accurate punch's Attack is 95, roll
     * 95 * 164 = 15580, and Strength 106. */
    s.caps.prayers = BOOK(PIETY);
    bonus = fc_prayer_bonus(&s);
    fc_melee_rolls(70, 80, 100, 50, FC_STANCE_AGGRESSIVE, &bonus, &roll, &max_hit);
    assert(roll == 15088 && max_hit == 19);
    FcPlayer melee = {.attack_level = 70, .strength_level = 80,
                      .weapon_kind = FC_WEAPON_UNARMED,
                      .melee_attack_bonus = 100, .melee_strength_bonus = 50};
    const FcMeleeStyle punch = {FC_ATTACK_TYPE_CRUSH, FC_STANCE_ACCURATE};
    fc_player_melee_rolls(&melee, &bonus, &punch, &roll, &max_hit);
    assert(roll == 15580 && max_hit == 19);
    /* Prayer runtime test_prayer_stats: at level 10 the 5% Burst of Strength
     * and Sharp Eye add one Strength and ranged Strength level. */
    assert(fc_prayer_level(10, 5) == 10 && fc_prayer_strength_level(10, 5) == 11);
    /* Defence (rc_player_effective_defence_level, no stance here): Piety at
     * Defence 99 is 99 * 125 / 100 + 8 = 131, against +64 crush defence. */
    FcPlayer defender = {.defence_level = 99, .magic_level = 1, .defence_crush = 64};
    assert(fc_player_def_roll(&defender, &bonus, 0, FC_ATTACK_TYPE_CRUSH) == 131 * 128);
    /* Magic defence, test_magic_formula_uses_magic_prayer_damage_and_defence_blend:
     * Magic 80, Defence 50, +30 magic defence and 25% magic defence give
     * 100 * 7 / 10 + 50 * 3 / 10 + 8 = 93 and roll 8742. Its Augury has no
     * Defence; RuneC's real Augury adds 25%: 70 + 62 * 3 / 10 + 8 = 96. */
    FcPlayer caster = {.defence_level = 50, .magic_level = 80, .defence_magic = 30};
    FcPrayerBonus magic_only = {.magic_defence = 25};
    assert(fc_player_def_roll(&caster, &magic_only, 0, FC_ATTACK_TYPE_MAGIC) == 8742);
    s.caps.prayers = BOOK(AUGURY);
    bonus = fc_prayer_bonus(&s);
    assert(fc_player_def_roll(&caster, &bonus, 0, FC_ATTACK_TYPE_MAGIC) == 96 * 94);

    /* Rapid Heal (rc_stat_restore_tick; test_prayer_stats): switching it on
     * restarts the timer, and the next HP comes 50 ticks later; switching it
     * off restores 100. */
    prayer_fixture(&s, 99, 99);
    s.player.current_hp = 500;
    s.player.hp_regen_counter = 37;
    book_step(&s, FC_BOOK_RAPID_HEAL, FC_PRAYER_NO_CHANGE);
    assert(s.player.hp_regen_counter == 0);
    for (int i = 0; i < 49; i++) book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_hp == 500);
    book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_hp == 510);
    book_step(&s, FC_BOOK_RAPID_HEAL, FC_PRAYER_NO_CHANGE);
    for (int i = 0; i < 99; i++) book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_hp == 510);
    book_step(&s, -1, FC_PRAYER_NO_CHANGE);
    assert(s.player.current_hp == 520);
    fc_destroy(&s);
    puts("prayer book: definitions, requirements, conflicts, drain, combat percentages "
         "and Rapid Heal passed");
}

/* The prayer book head reaches the core through the policy actions; its mask
 * and the active prayers and their drain are observed. */
static void prayer_book_adapter_test(void) {
    enum { PRAYER = 2 };
    /* The book is the last policy head but for the gear set, autocast and
     * special attack heads. */
    const int book = FC_PUFFER_NUM_ATNS - 1 - FC_GEAR_SWITCHING - FC_SPELLBOOK_HEAD -
                     FC_CAP_SPECIAL_ATTACKS;
    const int book_mask = FC_PUFFER_MASK_SIZE - FC_PRAYER_BOOK_DIM -
                          FC_GEAR_SWITCHING * FC_GEAR_SET_DIM -
                          FC_SPELLBOOK_HEAD * FC_AUTOCAST_DIM -
                          FC_CAP_SPECIAL_ATTACKS * FC_SPECIAL_ATTACK_DIM;
    assert(FC_PUFFER_ACTION_HEADS[PRAYER] == FC_HEAD_PRAYER &&
           FC_PUFFER_ACTION_HEADS[book] == FC_HEAD_PRAYER_BOOK);
    Dict kwargs = {0};
    other_gear_sets(&kwargs, FC_GEAR_PRESETS[0].name);
    dict_set(&kwargs, "prayer_level", 70);
    FightCaves* env = test_env(&kwargs);
    FcPlayer* p = &env->state.player;
    Agent* a = &env->agents[0];
    const float* obs = a->observations + FC_OBS_BOOK_START;
    /* Prayer 70 masks Rigour and Augury, and Eagle Eye and Mystic Might,
     * which Deadeye and Mystic Vigour replace. */
    const uint32_t masked = BOOK(RIGOUR) | BOOK(AUGURY) | BOOK(EAGLE_EYE) |
                            BOOK(MYSTIC_MIGHT);
    assert(a->action_mask[book_mask + FC_PRAYER_BOOK_NO_CHANGE] == 1);
    for (int b = 0; b < FC_BOOK_PRAYER_COUNT; b++)
        assert(a->action_mask[book_mask + 1 + b] == !(masked & (1u << b)));
    for (int i = 0; i < FC_OBS_BOOK_SIZE; i++) assert(obs[i] == 0.0f);
    /* Piety with Protect from Melee: 12 + 24 against resistance 60 + 2 * 6
     * (tbow_masori's prayer bonus). */
    a->actions[PRAYER] = FC_PRAYER_MELEE;
    a->actions[book] = FC_BOOK_PIETY + 1;
    checked_step(env);
    for (int b = 0; b < FC_BOOK_PRAYER_COUNT; b++)
        assert(obs[FC_OBS_BOOK_ACTIVE + b] == (b == FC_BOOK_PIETY ? 1.0f : 0.0f));
    assert(obs[FC_OBS_BOOK_DRAIN] == 36.0f / 72.0f);
    /* Out of Prayer points every toggle that would switch a prayer on is
     * masked, but Piety can still go off; the next tick switches it off. */
    a->actions[PRAYER] = FC_PRAYER_NO_CHANGE;
    a->actions[book] = FC_PRAYER_BOOK_NO_CHANGE;
    p->current_prayer = 0;
    fc_puffer_write_obs(env);
    check_obs(env);
    for (int b = 0; b < FC_BOOK_PRAYER_COUNT; b++)
        assert(a->action_mask[book_mask + 1 + b] == (b == FC_BOOK_PIETY));
    checked_step(env);
    assert(env->state.caps.prayers == 0 && p->prayer == PRAYER_NONE);
    for (int i = 0; i < FC_OBS_BOOK_SIZE; i++) assert(obs[i] == 0.0f);
    for (int b = 0; b < FC_PRAYER_BOOK_DIM; b++)
        assert(a->action_mask[book_mask + b] == (b == FC_PRAYER_BOOK_NO_CHANGE));
    test_free(env);
    dict_clear(&kwargs);
    puts("prayer book: head, masks, active prayers and drain observed passed");
}
#undef BOOK
#endif

#if FC_GEAR_SWITCHING
/* The switching tests' gear sets as far as gear_sets goes: tbow_masori worn,
 * then acb_armadyl, bowfa_crystal and blowpipe_pure. */
static const char* const GEAR_LINEUP[FC_MAX_GEAR_SETS] =
    {"tbow_masori", "acb_armadyl", "bowfa_crystal", "blowpipe_pure"};

/* The items each of those sets adds to the inventory, in order: what
 * tbow_masori and an earlier set do not have. */
static const int GEAR_SWITCH_ITEMS[FC_MAX_GEAR_SETS][8] = {
    {0},
    {11826, 11785, 11828, 12610, 21946, 11830, 7462},   /* Armadyl, book, bolts, gloves */
    {23971, 25867, 23975, 23979},                       /* crystal; acb's gloves */
    {2581, 10499, FC_ITEM_ID_TOXIC_BLOWPIPE, 12596, 2495, 11126, 2577},
};
enum { DRAGON_ARROW = 11212, DIAMOND_BOLTS = 21946, BOOK_OF_LAW = 12610 };

static FcScenario gear_scenario(int sharks, int doses) {
    FcScenario scenario = fc_default_scenario();
    for (int k = 0; k < FC_CAP_GEAR_SETS; k++)
        scenario.gear_set[k] = fc_gear_preset_find(GEAR_LINEUP[k]);
    scenario.sharks = sharks;
    scenario.prayer_doses = doses;
    return scenario;
}

static void gear_fixture(FcState* s, const FcScenario* scenario) {
    fc_init(s);
    fc_reset(s, 101);
    fc_apply_scenario(s, scenario);
}

/* Whether the player wears every item of preset `name`. */
static int wears_preset(const FcPlayer* p, const char* name) {
    const FcGearPreset* preset = &FC_GEAR_PRESETS[fc_gear_preset_find(name)];
    for (int i = 0; i < preset->equipment_count; i++) {
        const FcItemDef* item = fc_item_definition(preset->equipment[i]);
        if (p->equipment[item->slot].item_id != item->id) return 0;
    }
    return 1;
}

/* The gear_set mask: bit a for GEAR_SET action a. */
static unsigned gear_mask(const FcState* s) {
    float mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(s, mask);
    unsigned bits = 0;
    for (int a = 0; a < FC_GEAR_SET_DIM; a++)
        if (mask[FC_MASK_GEAR_START + a] != 0.0f) bits |= 1u << a;
    return bits;
}

/* How many of item id the player has, worn and in the inventory. */
static int item_total(const FcPlayer* p, int id) {
    int total = 0;
    for (int i = 0; i < FC_EQUIPMENT_SLOTS; i++)
        if (p->equipment[i].item_id == id) total += p->equipment[i].quantity;
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (p->inventory[i].item_id == id) total += p->inventory[i].quantity;
    return total;
}

/* The scenario checks, the inventory at reset, a switch and back, the mask
 * and the switch's place in the tick. */
static void gear_switch_test(void) {
    const unsigned all = (1u << FC_GEAR_SET_DIM) - 1;
    char err[256], expected[160];
    FcState s;

    /* Each set's items must be wearable, and the error names the set. */
    for (int k = 0; k < FC_CAP_GEAR_SETS; k++) {
        FcScenario scenario = preset_scenario(fc_gear_preset_find("rcb_pure"));
        scenario.gear_set[k] = fc_gear_preset_find("blowpipe_pure");
        scenario.levels[FC_SKILL_DEFENCE] = 1;
        scenario.levels[FC_SKILL_RANGED] = 75;
        scenario.levels[FC_SKILL_HITPOINTS] = 74;
        assert(fc_scenario_check(&scenario, err, sizeof(err)) != 0);
        snprintf(expected, sizeof(expected), "gear_set_%d = blowpipe_pure: Necklace of "
                 "anguish needs 75 hitpoints, but hitpoints_level = 74", k + 1);
        assert(strcmp(err, expected) == 0);
        scenario.levels[FC_SKILL_HITPOINTS] = 75;
        assert(fc_scenario_check(&scenario, err, sizeof(err)) == 0);
    }
    FcScenario missing = gear_scenario(0, 0);
    missing.gear_set[FC_CAP_GEAR_SETS - 1] = -1;
    assert(fc_scenario_check(&missing, err, sizeof(err)) != 0);
    snprintf(expected, sizeof(expected), "gear_set_%d is missing: gear_sets = %d needs a "
             "gear preset for each set", FC_CAP_GEAR_SETS, FC_CAP_GEAR_SETS);
    assert(strcmp(err, expected) == 0);

    /* The switch items and the supplies must fit in the inventory. */
    FcScenario scenario = gear_scenario(0, 8);   /* two potions */
    FcItemStack items[FC_MAX_SWITCH_ITEMS];
    int switch_items = fc_gear_switch_items(&scenario, items), listed = 0;
    for (int k = 1; k < FC_CAP_GEAR_SETS; k++)
        for (int i = 0; i < 8; i++) listed += GEAR_SWITCH_ITEMS[k][i] != 0;
    assert(switch_items == listed && (FC_CAP_GEAR_SETS < 4 || switch_items == 18));
    scenario.sharks = FC_INVENTORY_SLOTS - switch_items - 2;
    assert(scenario.sharks <= FC_MAX_SHARKS);
    assert(fc_scenario_check(&scenario, err, sizeof(err)) == 0);
    scenario.sharks++;
    assert(fc_scenario_check(&scenario, err, sizeof(err)) != 0);
    snprintf(expected, sizeof(expected), "gear sets 2-%d put %d items in the inventory and "
             "the supplies take %d slots: %d is more than its 28", FC_CAP_GEAR_SETS,
             switch_items, scenario.sharks + 2, switch_items + scenario.sharks + 2);
    assert(strcmp(err, expected) == 0);

    /* At reset set 1 is worn, and the other sets' items lead the inventory,
     * each once, ammunition as one stack, then the supplies. */
    scenario = gear_scenario(3, 6);
    gear_fixture(&s, &scenario);
    FcPlayer* p = &s.player;
    int slot = 0;
    for (int k = 1; k < FC_CAP_GEAR_SETS; k++) {
        for (int i = 0; i < 8 && GEAR_SWITCH_ITEMS[k][i]; i++, slot++) {
            int id = GEAR_SWITCH_ITEMS[k][i];
            assert(p->inventory[slot].item_id == id);
            assert(p->inventory[slot].quantity == (id == DIAMOND_BOLTS ? 50000 : 1));
            assert(p->inventory[slot].charges ==
                   (id == FC_ITEM_ID_TOXIC_BLOWPIPE ? 50000 : 0));
        }
        assert(s.caps.gear_set_loadouts[k] ==
               FC_GEAR_PRESETS[fc_gear_preset_find(GEAR_LINEUP[k])].id);
    }
    assert(slot == switch_items);
    assert(p->inventory[slot].item_id == FC_ITEM_ID_PRAYER_POTION_4 &&
           p->inventory[slot + 1].item_id == FC_ITEM_ID_PRAYER_POTION_2);
    for (int i = slot + 2; i < FC_INVENTORY_SLOTS; i++)
        assert(p->inventory[i].item_id == (i < slot + 5 ? FC_ITEM_ID_SHARK : 0));
    assert(p->sharks_remaining == 3 && p->prayer_doses_remaining == 6);
    assert(wears_preset(p, "tbow_masori") && p->ranged_attack_bonus == 215);
    assert(s.active_loadout == FC_LOADOUT_SOTA_TBOW && s.caps.gear_set == 0 &&
           s.caps.gear_set_loadouts[0] == FC_LOADOUT_SOTA_TBOW);
    /* The worn set is masked, every other one can be switched to. */
    assert(gear_mask(&s) == (all & ~(1u << 1)));

    /* Switch to acb_armadyl: its items go on where they lie and what they
     * replace takes their slots. A switch keeps the attack timer, the attacks
     * in flight and the RNG, and drops the target as an equip does. */
    FcItemStack reset_inventory[FC_INVENTORY_SLOTS];
    memcpy(reset_inventory, p->inventory, sizeof(reset_inventory));
    s.npcs[0].pending_hits[0] = (FcPendingHit){1, 100, 2, ATTACK_RANGED, -1, 0, PRAYER_NONE, -1};
    s.npcs[0].num_pending_hits = 1;
    FcNpc target = s.npcs[0];
    p->attack_target_idx = 0;
    p->attack_timer = 3;
    uint32_t rng = s.rng_state;
    assert(fc_gear_apply_action(&s, 2));
    assert(wears_preset(p, "acb_armadyl") &&
           p->equipment[FC_EQUIP_SLOT_RING].item_id == 28310);   /* the Venator ring stays */
    /* acb_armadyl's totals and the ring's +10 ranged attack, +2 strength. */
    assert(p->ranged_attack_bonus == 230 && p->ranged_strength_bonus == 131);
    assert(p->defence_stab == 112 && p->defence_magic == 139 && p->prayer_bonus == 11);
    assert(p->weapon_speed == 5 && p->weapon_range == 8 && p->ammo_count == 50000);
    static const int displaced[] = {27235, 20997, 27238, 0, DRAGON_ARROW, 27241, 26235};
    for (int i = 0; i < 7; i++) assert(p->inventory[i].item_id == displaced[i]);
    assert(p->inventory[4].quantity == 50000);
    assert(memcmp(p->inventory + 7, reset_inventory + 7,
                  sizeof(FcItemStack) * (FC_INVENTORY_SLOTS - 7)) == 0);
    assert(s.active_loadout == FC_LOADOUT_ACB_ARMADYL && s.caps.gear_set == 1);
    assert(p->attack_timer == 3 && p->attack_target_idx == -1 && s.rng_state == rng);
    assert(memcmp(&target, &s.npcs[0], sizeof(target)) == 0 && s.tick == 0);
    assert(gear_mask(&s) == (all & ~(1u << 2)));
    /* And back: the inventory is as it was at reset, the book going to the
     * first free slot when the two-handed bow takes both hands. */
    assert(fc_gear_apply_action(&s, 1));
    assert(memcmp(p->inventory, reset_inventory, sizeof(reset_inventory)) == 0);
    assert(wears_preset(p, "tbow_masori") &&
           p->equipment[FC_EQUIP_SLOT_SHIELD].item_id == 0);
    assert(p->ranged_attack_bonus == 215 && p->ranged_strength_bonus == 99 &&
           p->prayer_bonus == 6 && p->weapon_speed == 5 && p->weapon_range == 10);
    assert(s.active_loadout == FC_LOADOUT_SOTA_TBOW && s.caps.gear_set == 0);
    assert(gear_mask(&s) == (all & ~(1u << 1)));

    /* A masked switch changes nothing. */
    uint32_t hash = fc_state_hash(&s);
    assert(!fc_gear_apply_action(&s, 1) && fc_state_hash(&s) == hash);
    if (FC_CAP_GEAR_SETS >= 3) {
        /* A set with an item missing cannot be switched to. */
        FcState lost = s;
        lost.player.inventory[item_slot(&lost.player, 25867)] = (FcItemStack){0};
        assert(!(gear_mask(&lost) & (1u << 3)) && (gear_mask(&lost) & (1u << 2)));
        /* From acb_armadyl to bowfa_crystal the two-handed bow takes both
         * hands, so the book needs a free slot: with none the switch is
         * masked, with one it goes there. */
        FcState full = s;
        assert(fc_gear_apply_action(&full, 2));
        for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
            if (!full.player.inventory[i].item_id)
                full.player.inventory[i] = (FcItemStack){FC_ITEM_ID_SHARK, 1, 0};
        hash = fc_state_hash(&full);
        assert(!(gear_mask(&full) & (1u << 3)));
        assert(!fc_gear_apply_action(&full, 3) && fc_state_hash(&full) == hash);
        full.player.inventory[20] = (FcItemStack){0};
        assert(gear_mask(&full) & (1u << 3));
        assert(fc_gear_apply_action(&full, 3) && wears_preset(&full.player, "bowfa_crystal"));
        assert(full.player.inventory[20].item_id == BOOK_OF_LAW &&
               full.player.equipment[FC_EQUIP_SLOT_SHIELD].item_id == 0);
        assert(full.active_loadout == FC_LOADOUT_BOWFA_CRYSTAL && full.caps.gear_set == 2);
    }
    /* Nothing changes once the episode is over. */
    s.terminal = TERMINAL_PLAYER_DEATH;
    assert(gear_mask(&s) == 1u);
    s.terminal = TERMINAL_NONE;

    /* Through fc_step the switch precedes the attack: chosen together, the
     * attack fires with the new crossbow and its bolts. */
    gear_fixture(&s, &scenario);
    memset(s.npcs, 0, sizeof(s.npcs));
    memset(s.walkable, 1, sizeof(s.walkable));
    memset(s.movement_flags, 0, sizeof(s.movement_flags));
    memset(s.los_flags, 0, sizeof(s.los_flags));
    p->x = 20; p->y = 20;
    fc_npc_spawn(&s.npcs[0], NPC_YT_MEJKOT, 20, 25, 1);
    s.npcs_remaining = 1;
    s.npcs[0].attack_timer = 100;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_GEAR_SET] = 2;
    fc_step(&s, actions);
    assert(s.caps.gear_set == 1 && s.render_events.player_attack_fired);
    assert(p->attack_target_idx == 0 && p->attack_timer == 5 - 1);
    assert(item_total(p, DIAMOND_BOLTS) == 49999 && item_total(p, DRAGON_ARROW) == 50000);
    /* Switching back without the attack head drops the target: no attack
     * follows, and the cooldown runs on. */
    actions[FC_HEAD_ATTACK] = FC_ATTACK_NONE;
    actions[FC_HEAD_GEAR_SET] = 1;
    fc_step(&s, actions);
    assert(s.caps.gear_set == 0 && p->attack_target_idx == -1 && p->attack_timer == 3);
    actions[FC_HEAD_GEAR_SET] = FC_GEAR_SET_NO_CHANGE;
    for (int t = 0; t < 6; t++) {
        fc_step(&s, actions);
        assert(!s.render_events.player_attack_fired);
    }
    fc_destroy(&s);
    puts("gear sets: requirements per set, missing set, inventory overflow, reset "
         "inventory, switch and back, masks and tick order passed");
}

/* An episode that switches between tbow_masori and acb_armadyl every 25
 * ticks while attacking the nearest NPC, deterministically: both sets
 * attack, and each spends its own ammunition. */
static void gear_episode_test(void) {
    FcScenario scenario = gear_scenario(0, 0);
    FcState s, again;
    gear_fixture(&s, &scenario);
    gear_fixture(&again, &scenario);
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
    int attacks[2] = {0}, hits = 0, switches = 0, ticks = 0;
    for (; ticks < 400 && !fc_is_terminal(&s); ticks++) {
        int set = ticks / 25 % 2;
        actions[FC_HEAD_GEAR_SET] = s.caps.gear_set != set ? set + 1 : FC_GEAR_SET_NO_CHANGE;
        assert(gear_mask(&s) & (1u << actions[FC_HEAD_GEAR_SET]));
        fc_step(&s, actions);
        fc_step(&again, actions);
        assert(fc_state_hash(&s) == fc_state_hash(&again));
        assert(s.caps.gear_set == set);
        assert(wears_preset(&s.player, GEAR_LINEUP[set]));
        switches += actions[FC_HEAD_GEAR_SET] != FC_GEAR_SET_NO_CHANGE;
        attacks[set] += s.render_events.player_attack_fired;
        hits += s.hits_landed_this_tick;
    }
    assert(ticks == 400 && switches == 15);
    assert(attacks[0] > 0 && attacks[1] > 0 && hits > 0);
    assert(item_total(&s.player, DRAGON_ARROW) == 50000 - attacks[0]);
    assert(item_total(&s.player, DIAMOND_BOLTS) == 50000 - attacks[1]);
    printf("gear sets: %d switches in %d ticks, %d attacks with tbow_masori and %d "
           "with acb_armadyl, %d hits passed\n", switches, ticks, attacks[0],
           attacks[1], hits);
}

/* The gear_set head reaches the core through the policy actions; its mask
 * and the gear block are observed. */
static void gear_adapter_test(void) {
    const int head = FC_PUFFER_NUM_ATNS - 1 - FC_SPELLBOOK_HEAD - FC_CAP_SPECIAL_ATTACKS;
    const int mask = FC_PUFFER_MASK_SIZE - FC_GEAR_SET_DIM -
                     FC_SPELLBOOK_HEAD * FC_AUTOCAST_DIM -
                     FC_CAP_SPECIAL_ATTACKS * FC_SPECIAL_ATTACK_DIM;
    assert(FC_PUFFER_ACTION_HEADS[head] == FC_HEAD_GEAR_SET &&
           strcmp(FC_HEAD_NAMES[FC_HEAD_GEAR_SET], "gear_set") == 0 &&
           FC_PUFFER_ACTION_DIMS[head] == FC_CAP_GEAR_SETS + 1);
    Dict kwargs = {0};
    for (int k = 0; k < FC_CAP_GEAR_SETS; k++) {
        char key[32];
        snprintf(key, sizeof(key), "gear_set_%d", k + 1);
        puf_ini_set(&kwargs, key, GEAR_LINEUP[k]);
    }
    FightCaves* env = test_env(&kwargs);
    Agent* a = &env->agents[0];
    const float* obs = a->observations + FC_OBS_GEAR_START;
    /* Every set's weapon shoots: the Twisted bow, Armadyl crossbow, Bow of
     * faerdhinen and blowpipe reach 10, 8, 10 and 5 tiles every 5, 5, 4 and
     * 2 ticks. */
    static const int range[FC_MAX_GEAR_SETS] = {10, 8, 10, 5};
    static const int speed[FC_MAX_GEAR_SETS] = {5, 5, 4, 2};
    for (int step = 0; step < 2; step++) {
        int worn = step;   /* gear_set_1, then gear_set_2 */
        for (int k = 0; k < FC_CAP_GEAR_SETS; k++) {
            const float* set = obs + FC_OBS_GEAR_WEAPONS + k * FC_OBS_GEAR_STRIDE;
            assert(obs[FC_OBS_GEAR_WORN + k] == (k == worn ? 1.0f : 0.0f));
            assert(set[FC_OBS_GEAR_RANGED] == 1.0f && set[FC_OBS_GEAR_MAGIC] == 0.0f &&
                   set[FC_OBS_GEAR_MELEE] == 0.0f);
            assert(set[FC_OBS_GEAR_RANGE] == (float)range[k] / 10.0f &&
                   set[FC_OBS_GEAR_SPEED] == (float)speed[k] / 10.0f);
        }
        for (int v = 0; v < FC_GEAR_SET_DIM; v++)
            assert(a->action_mask[mask + v] == (v != worn + 1));
        a->actions[head] = 2;
        checked_step(env);
    }
    assert(env->state.caps.gear_set == 1 &&
           env->state.active_loadout == FC_LOADOUT_ACB_ARMADYL);
    test_free(env);
    dict_clear(&kwargs);
    puts("gear sets: head, masks, worn set and weapons observed passed");
}
#endif

/* Magic, against RuneC_v4: the definitions of data/defs/spells.bin (SPEL
 * v2) and data/defs/items.bin, rc-core's spells.c, combat.c and
 * combat_formula.c, rc-content/combat's magic.c and runes.c, and the expected
 * values of tests/test_combat_phase5_formula_core.c (formula core) and
 * tests/test_magic_runtime.c (magic runtime). */

/* A cave without walls: the player at (20, 20) in scenario's gear set 1 and
 * one NPC of npc_type at (x, y) that does not attack. */
static void magic_fixture(FcState *s, const FcScenario *scenario, int npc_type,
                          int x, int y) {
    fc_init(s);
    fc_reset(s, 101);
    fc_apply_scenario(s, scenario);
    memset(s->npcs, 0, sizeof(s->npcs));
    memset(s->caps.npc_spells, 0, sizeof(s->caps.npc_spells));
    memset(s->walkable, 1, sizeof(s->walkable));
    memset(s->movement_flags, 0, sizeof(s->movement_flags));
    memset(s->los_flags, 0, sizeof(s->los_flags));
    s->player.x = 20;
    s->player.y = 20;
    fc_npc_spawn(&s->npcs[0], npc_type, x, y, 1);
    s->npcs_remaining = 1;
    s->npcs[0].attack_timer = 100000;
}

/* How many of a rune the inventory holds. */
static int rune_total(const FcPlayer *p, int rune) {
    int total = 0;
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (p->inventory[i].item_id == rune) total += p->inventory[i].quantity;
    return total;
}

/* The runes one cast spends, the worn staff supplying its elements. */
static int cast_cost(const FcPlayer *p, int spell, int rune) {
    const FcItemDef *weapon = fc_item_definition(p->equipment[FC_EQUIP_SLOT_WEAPON].item_id);
    for (int r = 0; r < FC_SPELLS[spell].rune_count; r++)
        if (FC_SPELLS[spell].runes[r].id == rune)
            return weapon && (weapon->staff_runes & fc_rune_element(rune))
                ? 0 : FC_SPELLS[spell].runes[r].quantity;
    return 0;
}

static void magic_definitions_test(void) {
    /* spells.bin rows 29, 158, 27, 96, 117 and 3: book, level, max hit and
     * runes in RuneC's order. */
    const FcSpellDef *wind = &FC_SPELLS[FC_SPELL_WIND_STRIKE];
    assert(wind->book == FC_SPELLBOOK_STANDARD && wind->level == 1 && wind->max_hit == 2 &&
           wind->rune_count == 2 && wind->runes[0].id == FC_RUNE_AIR &&
           wind->runes[0].quantity == 1 && wind->runes[1].id == FC_RUNE_MIND &&
           wind->runes[1].quantity == 1);
    const FcSpellDef *surge = &FC_SPELLS[FC_SPELL_WATER_SURGE];
    assert(surge->level == 85 && surge->max_hit == 22 && surge->rune_count == 3 &&
           surge->runes[0].id == FC_RUNE_AIR && surge->runes[0].quantity == 7 &&
           surge->runes[1].id == FC_RUNE_WATER && surge->runes[1].quantity == 10 &&
           surge->runes[2].id == FC_RUNE_WRATH && surge->runes[2].quantity == 1);
    const FcSpellDef *smoke = &FC_SPELLS[FC_SPELL_SMOKE_RUSH];
    assert(smoke->book == FC_SPELLBOOK_ANCIENT && smoke->level == 50 && smoke->max_hit == 13 &&
           smoke->rune_count == 4 && smoke->runes[2].id == FC_RUNE_CHAOS &&
           smoke->runes[2].quantity == 2 && smoke->runes[3].id == FC_RUNE_DEATH);
    const FcSpellDef *shadow = &FC_SPELLS[FC_SPELL_SHADOW_BARRAGE];
    assert(shadow->level == 88 && shadow->max_hit == 28 && shadow->runes[3].id == FC_RUNE_SOUL &&
           shadow->runes[3].quantity == 3);
    const FcSpellDef *blood = &FC_SPELLS[FC_SPELL_BLOOD_BLITZ];
    assert(blood->level == 80 && blood->max_hit == 25 && blood->rune_count == 2 &&
           blood->runes[0].id == FC_RUNE_BLOOD && blood->runes[0].quantity == 4);
    const FcSpellDef *ice = &FC_SPELLS[FC_SPELL_ICE_BARRAGE];
    assert(ice->level == 94 && ice->max_hit == 30 && ice->runes[0].id == FC_RUNE_WATER &&
           ice->runes[0].quantity == 6 && ice->runes[2].id == FC_RUNE_DEATH &&
           ice->runes[2].quantity == 4);
    /* Each book in level order, and rc_content_magic_hit's effects by
     * element (Smoke, Shadow, Blood, Ice) and tier (Rush .. Barrage). */
    for (int spell = 0; spell < FC_SPELL_COUNT; spell++) {
        const FcSpellDef *def = &FC_SPELLS[spell];
        int ancient = spell >= FC_STANDARD_SPELL_COUNT;
        int index = spell - (ancient ? FC_SPELL_SMOKE_RUSH : 0);
        int tier = index / 4, element = index % 4;
        assert(def->book == (ancient ? FC_SPELLBOOK_ANCIENT : FC_SPELLBOOK_STANDARD));
        if (index > 0) assert(def->level > FC_SPELLS[spell - 1].level);
        assert(def->freeze_ticks == (ancient && element == 3 ? 8 * (tier + 1) : 0));
        assert(def->poison == (ancient && element == 0 ? (tier < 2 ? 2 : 4) : 0));
        assert(def->drain_percent == (ancient && element == 1 ? (tier < 2 ? 10 : 15) : 0));
        assert(def->heals == (ancient && element == 2));
        /* The magic presets carry every rune either book uses. */
        for (int r = 0; r < def->rune_count; r++) {
            int carried = 0;
            for (int k = 0; k < FC_PRESET_RUNE_COUNT; k++)
                carried |= FC_PRESET_RUNES[k] == def->runes[r].id;
            assert(carried);
        }
    }
    assert(FC_STANDARD_SPELL_COUNT + FC_ANCIENT_SPELL_COUNT == FC_SPELL_COUNT);
    /* rc_content_prepare_magic's elemental tables are the Standard spells'
     * levels and max hits, so a spell hits as hard as the strongest of its
     * tier the Magic level reaches. */
    static const int levels[5][4] = {{1, 5, 9, 13}, {17, 23, 29, 35}, {41, 47, 53, 59},
                                     {62, 65, 70, 75}, {81, 85, 90, 95}};
    static const int hits[5][4] = {{2, 4, 6, 8}, {9, 10, 11, 12}, {13, 14, 15, 16},
                                   {17, 18, 19, 20}, {21, 22, 23, 24}};
    for (int tier = 0; tier < 5; tier++)
        for (int j = 0; j < 4; j++)
            assert(FC_SPELLS[tier * 4 + j].level == levels[tier][j] &&
                   FC_SPELLS[tier * 4 + j].max_hit == hits[tier][j]);
    assert(fc_spell_base_max_hit(FC_SPELL_WATER_BLAST, 47) == 14 &&
           fc_spell_base_max_hit(FC_SPELL_WATER_BLAST, 59) == 16);
    assert(fc_spell_base_max_hit(FC_SPELL_WATER_WAVE, 69) == 18 &&
           fc_spell_base_max_hit(FC_SPELL_WATER_WAVE, 75) == 20);
    assert(fc_spell_base_max_hit(FC_SPELL_WATER_SURGE, 94) == 23 &&
           fc_spell_base_max_hit(FC_SPELL_WATER_SURGE, 99) == 24);
    assert(fc_spell_base_max_hit(FC_SPELL_ICE_BARRAGE, 99) == 30);
    puts("magic: spell definitions, effects and elemental scaling against RuneC passed");
}

static void magic_formula_test(void) {
    /* Formula core test_magic_formula_uses_magic_prayer_damage_and_defence_blend:
     * Magic 80 with Augury's 25% accuracy and 4% damage, +70 Magic attack and
     * +10 Magic damage: (80 * 125 / 100 + 1 + 8) * (70 + 64) = 14606 and
     * 20 + 20 * 14 / 100 = 22 from a max hit of 20; a Magic 70 NPC without a
     * defence bonus rolls (70 + 9) * 64 = 5056. */
    FcPrayerBonus augury = {.magic_attack = 25, .magic_damage = 4};
    int roll, max_hit;
    fc_magic_rolls(80, 70, 10, &augury, 20, &roll, &max_hit);
    assert(roll == 14606 && max_hit == 22);
    assert(fc_npc_roll(70, 0) == 5056);
    /* The cave's NPCs roll their Magic level: Ket-Zek's 240. */
    assert(fc_npc_roll(fc_npc_get_stats(NPC_KET_ZEK)->attack_level[ATTACK_MAGIC], 0) ==
           249 * 64);
    /* The presets' worn bonuses (items.bin) and their spell's rolls at their
     * levels: smoke_staff_mystic's Water Blast at Magic 60 hits as Fire
     * Blast's 16, kodai_virtus's Water Surge at 94 as Earth Surge's 23. */
    static const struct {
        const char *name;
        int attack, damage, spell, roll, max_hit;
    } presets[] = {
        {"smoke_staff_mystic", 100, 2, FC_SPELL_WATER_BLAST, 69 * 164, 16},
        {"kodai_virtus", 159, 37, FC_SPELL_WATER_SURGE, 103 * 223, 23 + 23 * 37 / 100},
        {"harmonised_ancestral", 152, 38, FC_SPELL_WATER_SURGE, 108 * 216, 24 + 24 * 38 / 100},
    };
    FcState s;
    for (int i = 0; i < (int)(sizeof(presets) / sizeof(presets[0])); i++) {
        FcScenario scenario = levels_scenario(presets[i].name);
        fc_init(&s);
        fc_reset(&s, 101);
        fc_apply_scenario(&s, &scenario);
        int attack, damage;
        fc_player_magic_bonuses(&s.player, &attack, &damage);
        assert(attack == presets[i].attack && damage == presets[i].damage);
        FcPrayerBonus none = {0};
        fc_player_magic_rolls(&s.player, &none, presets[i].spell, &roll, &max_hit);
        assert(roll == presets[i].roll && max_hit == presets[i].max_hit);
    }
#if FC_CAP_PRAYERS
    /* The book prayers' magic percentages, the best active one's. */
    s.caps.prayers = 1u << FC_BOOK_AUGURY;
    FcPrayerBonus bonus = fc_prayer_bonus(&s);
    assert(bonus.magic_attack == 25 && bonus.magic_damage == 4);
    s.caps.prayers = 1u << FC_BOOK_MYSTIC_LORE;
    bonus = fc_prayer_bonus(&s);
    assert(bonus.magic_attack == 10 && bonus.magic_damage == 1);
    fc_player_magic_rolls(&s.player, &bonus, FC_SPELL_WATER_SURGE, &roll, &max_hit);
    assert(roll == (99 * 110 / 100 + 9) * 216 && max_hit == 24 + 24 * 39 / 100);
#endif
    puts("magic: accuracy and max hit against RuneC's formula core passed");
}

static void magic_rune_test(void) {
    FcPlayer p = {0};
    FcItemStack paid[FC_INVENTORY_SLOTS];
    /* Magic runtime rune_sources: one mist rune cannot pay two air runes and
     * a water rune; with an air rune too it pays, spending both. */
    const FcSpellDef cost = {.rune_count = 2, .runes = {{FC_RUNE_AIR, 2}, {FC_RUNE_WATER, 1}}};
    p.inventory[0] = (FcItemStack){FC_RUNE_MIST, 1, 0};
    assert(!fc_spell_rune_plan(&p, &cost, paid));
    p.inventory[1] = (FcItemStack){FC_RUNE_AIR, 1, 0};
    assert(fc_spell_rune_plan(&p, &cost, paid));
    assert(paid[0].item_id == 0 && paid[1].item_id == 0);
    /* plan_payment spends a combination rune first for two elements at
     * once: Water Surge from 5 mist, 2 air, 9 water and a wrath rune takes
     * the 5 mist, the 2 air and 5 water. */
    memset(p.inventory, 0, sizeof(p.inventory));
    p.inventory[0] = (FcItemStack){FC_RUNE_MIST, 5, 0};
    p.inventory[1] = (FcItemStack){FC_RUNE_AIR, 2, 0};
    p.inventory[2] = (FcItemStack){FC_RUNE_WATER, 9, 0};
    p.inventory[3] = (FcItemStack){FC_RUNE_WRATH, 1, 0};
    assert(fc_spell_rune_plan(&p, &FC_SPELLS[FC_SPELL_WATER_SURGE], paid));
    assert(!paid[0].item_id && !paid[1].item_id && paid[2].quantity == 4 && !paid[3].item_id);
    /* Staves supply their elements (staff_elements): the Mystic smoke staff
     * air and fire, so Water Blast takes 3 water and a death rune; the Kodai
     * wand water, so Water Surge takes 7 air and a wrath rune; the
     * Harmonised nightmare staff none. */
    memset(p.inventory, 0, sizeof(p.inventory));
    p.equipment[FC_EQUIP_SLOT_WEAPON] = (FcItemStack){12000, 1, 0};
    p.inventory[0] = (FcItemStack){FC_RUNE_WATER, 3, 0};
    p.inventory[1] = (FcItemStack){FC_RUNE_DEATH, 1, 0};
    assert(fc_spell_rune_plan(&p, &FC_SPELLS[FC_SPELL_WATER_BLAST], paid));
    assert(!paid[0].item_id && !paid[1].item_id);
    p.inventory[0].quantity = 2;
    assert(!fc_spell_rune_plan(&p, &FC_SPELLS[FC_SPELL_WATER_BLAST], paid));
    p.equipment[FC_EQUIP_SLOT_WEAPON] = (FcItemStack){21006, 1, 0};
    p.inventory[0] = (FcItemStack){FC_RUNE_AIR, 7, 0};
    p.inventory[1] = (FcItemStack){FC_RUNE_WRATH, 1, 0};
    assert(fc_spell_rune_plan(&p, &FC_SPELLS[FC_SPELL_WATER_SURGE], paid));
    p.equipment[FC_EQUIP_SLOT_WEAPON] = (FcItemStack){24423, 1, 0};
    assert(!fc_spell_rune_plan(&p, &FC_SPELLS[FC_SPELL_WATER_SURGE], paid));
    puts("magic: rune payment, combination runes and staff-supplied runes passed");
}

/* Autocasting through fc_step: the spell set at reset (or, with the Ancient
 * book, by fc_spell_set_autocast), casts at range on the cast's speed, the
 * hit's arrival, the runes paid, RuneC's launch rejections, and the presets
 * that bash or punch. */
static void magic_autocast_test(void) {
    FcState s;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
#if FC_CAP_SPELLBOOK == 2
    /* kodai_virtus's Water Surge is the other book's, so it is not set; Ice
     * Rush is. */
    FcScenario scenario = levels_scenario("kodai_virtus");
    const int spell = FC_SPELL_ICE_RUSH;
    magic_fixture(&s, &scenario, NPC_TOK_XIL, 20, 28);
    assert(s.caps.autocast == 0);
    assert(fc_spell_check(&s, FC_SPELL_WATER_SURGE) == FC_CAST_WRONG_BOOK);
    assert(fc_spell_set_autocast(&s, spell) == FC_CAST_OK);
#else
    FcScenario scenario = levels_scenario("smoke_staff_mystic");
    const int spell = FC_SPELL_WATER_BLAST;
    magic_fixture(&s, &scenario, NPC_TOK_XIL, 20, 28);
    assert(fc_spell_check(&s, FC_SPELL_ICE_BARRAGE) == FC_CAST_WRONG_BOOK);
#endif
    assert(s.caps.autocast == spell + 1 && fc_player_cast_spell(&s) == spell);
    for (int r = 0; r < FC_PRESET_RUNE_COUNT; r++) {
        assert(s.player.inventory[r].item_id == FC_PRESET_RUNES[r]);
        assert(s.player.inventory[r].quantity == FC_PRESET_RUNE_QUANTITY);
    }
    /* The Tok-Xil's footprint is 8 tiles away, its centre tile 9: the cast
     * needs no approach, lands 2 + (9 + 1) / 3 = 5 ticks after it
     * (rc_content_player_hit_delay), and the next comes 5 ticks later. */
    fc_step(&s, actions);
    FcRenderEvents cast = s.render_events;
    assert(cast.player_attack_fired && cast.player_attack_style == ATTACK_MAGIC &&
           cast.player_attack_spell == spell + 1);
    assert(s.player.x == 20 && s.player.y == 20 && s.player.attack_timer == 5 - 1);
    assert(cast.player_attack_hit_delay_ticks == 3 + (9 + 1) / 3);
    const FcPendingHit *hit = &s.npcs[0].pending_hits[0];
    assert(s.npcs[0].num_pending_hits == 1 && hit->attack_style == ATTACK_MAGIC &&
           hit->prayer_drain == spell + 1);
    for (int r = 0; r < FC_PRESET_RUNE_COUNT; r++) {
        int rune = FC_PRESET_RUNES[r];
        assert(rune_total(&s.player, rune) ==
               FC_PRESET_RUNE_QUANTITY - cast_cost(&s.player, spell, rune));
    }
    int landed = 0, next_cast = 0;
    FcPrayerBonus none = {0};
    int roll, max_hit;
    fc_player_magic_rolls(&s.player, &none, spell, &roll, &max_hit);
    for (int t = 1; t <= 6; t++) {
        fc_step(&s, actions);
        for (int i = 0; i < s.render_events.hit_count; i++) {
            const FcRenderHit *h = &s.render_events.hits[i];
            if (h->target_entity_type != ENTITY_NPC || h->attack_style != ATTACK_MAGIC) continue;
            assert(!landed && h->damage <= 10 * max_hit);
            landed = t;
        }
        if (s.render_events.player_attack_fired && !next_cast) next_cast = t;
    }
    assert(landed == 5 && next_cast == 5);

    /* Below the spell's Magic level RuneC's launch rejects the cast and
     * keeps the spell; without its runes (magic runtime: "enough runes") it
     * clears the spell, and the staff then bashes. */
    FcState low = s;
    low.player.attack_timer = 0;
    low.player.magic_level = FC_SPELLS[spell].level - 1;
    fc_step(&low, actions);
    assert(!low.render_events.player_attack_fired && low.caps.autocast == spell + 1);
    FcState dry = s;
    dry.player.attack_timer = 0;
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (dry.player.inventory[i].item_id == FC_RUNE_DEATH)
            dry.player.inventory[i] = (FcItemStack){0};
    assert(fc_spell_check(&dry, spell) == FC_CAST_RUNES);
    fc_step(&dry, actions);
    assert(!dry.render_events.player_attack_fired && dry.caps.autocast == 0 &&
           fc_player_cast_spell(&dry) == -1);
    fc_npc_spawn(&dry.npcs[0], NPC_TZ_KIH, 21, 20, 2);
    dry.npcs[0].attack_timer = 100000;
    dry.player.attack_timer = 0;
    fc_step(&dry, actions);
    assert(dry.render_events.player_attack_fired &&
           dry.render_events.player_attack_style == ATTACK_MELEE &&
           dry.player.attack_timer == dry.player.weapon_speed - 1);

    /* The Harmonised nightmare staff autocasts only Standard spells, every 4
     * ticks (rc_content_prepare_magic; magic runtime). */
    FcScenario harmonised = levels_scenario("harmonised_ancestral");
    magic_fixture(&s, &harmonised, NPC_TOK_XIL, 20, 28);
#if FC_CAP_SPELLBOOK == 2
    assert(s.caps.autocast == 0 && fc_spell_check(&s, FC_SPELL_ICE_BARRAGE) == FC_CAST_WEAPON);
#else
    assert(s.caps.autocast == FC_SPELL_WATER_SURGE + 1);
    fc_step(&s, actions);
    assert(s.render_events.player_attack_spell == FC_SPELL_WATER_SURGE + 1 &&
           s.player.attack_timer == 4 - 1);
#endif
    /* No staff: rc_player_weapon_can_autocast fails; dead: RC_SPELL_DEAD. */
    fc_init(&s);
    fc_reset(&s, 101);
    s.player.magic_level = 99;
    assert(fc_spell_check(&s, FC_BOOK_FIRST_SPELL) == FC_CAST_WEAPON);
    s.player.current_hp = 0;
    assert(fc_spell_check(&s, FC_BOOK_FIRST_SPELL) == FC_CAST_DEAD);
    assert(fc_spell_check(&s, -1) == FC_CAST_INVALID);
    puts("magic: autocast through fc_step, cast timing, runes spent and launch "
         "rejections passed");
}

/* The landed effects (rc_content_magic_hit; magic runtime's 20-damage
 * checks), and through fc_step a frozen NPC that stays put and poison that
 * lands. */
static void magic_effects_test(void) {
    FcState s;
    FcScenario scenario = levels_scenario("kodai_virtus");
    magic_fixture(&s, &scenario, NPC_TOK_XIL, 20, 28);
    FcNpcSpellEffects *effects = &s.caps.npc_spells[0];
    /* Blood heals a quarter of the damage in whole HP, up to max HP. */
    s.player.max_hp = 990;
    s.player.current_hp = 500;
    fc_spell_land(&s, 0, FC_SPELL_BLOOD_BARRAGE, 200);
    assert(s.player.current_hp == 550);
    fc_spell_land(&s, 0, FC_SPELL_BLOOD_RUSH, 70);
    assert(s.player.current_hp == 560);
    s.player.current_hp = 985;
    fc_spell_land(&s, 0, FC_SPELL_BLOOD_BLITZ, 200);
    assert(s.player.current_hp == 990);
    fc_spell_land(&s, 0, FC_SPELL_WATER_SURGE, 200);
    assert(s.player.current_hp == 990 && effects->frozen_until == 0 &&
           effects->poison_damage == 0 && effects->attack_drain == 0);
    /* Ice freezes for its ticks and leaves the NPC immune 5 more; while
     * immune another Ice spell changes nothing. */
    s.tick = 100;
    fc_spell_land(&s, 0, FC_SPELL_ICE_BARRAGE, 10);
    assert(effects->frozen_until == 132 && effects->freeze_immune_until == 137);
    s.tick = 136;
    fc_spell_land(&s, 0, FC_SPELL_ICE_RUSH, 10);
    assert(effects->frozen_until == 132 && effects->freeze_immune_until == 137);
    s.tick = 137;
    fc_spell_land(&s, 0, FC_SPELL_ICE_RUSH, 10);
    assert(effects->frozen_until == 145 && effects->freeze_immune_until == 150);
    /* Smoke poisons with the larger damage, 30 ticks to the first hit. */
    fc_spell_land(&s, 0, FC_SPELL_SMOKE_RUSH, 10);
    assert(effects->poison_damage == 2 && effects->poison_timer == 30);
    fc_spell_land(&s, 0, FC_SPELL_SMOKE_BARRAGE, 10);
    effects->poison_timer = 5;
    fc_spell_land(&s, 0, FC_SPELL_SMOKE_RUSH, 10);
    assert(effects->poison_damage == 4 && effects->poison_timer == 30);
    /* Shadow lowers Attack to (100 - percent)%: Tok-Xil's 80 to 68 with
     * Barrage, which Rush's 72 leaves; its melee attack rolls with it. */
    fc_spell_land(&s, 0, FC_SPELL_SHADOW_RUSH, 10);
    assert(effects->attack_drain == 8);
    fc_spell_land(&s, 0, FC_SPELL_SHADOW_BARRAGE, 10);
    fc_spell_land(&s, 0, FC_SPELL_SHADOW_RUSH, 10);
    assert(effects->attack_drain == 12);
    assert(fc_npc_attack_level(&s, 0, ATTACK_MELEE) == 68 &&
           fc_npc_attack_level(&s, 0, ATTACK_RANGED) == 120);
    /* A new NPC in the slot has none of it. */
    s.npcs[0].active = 0;
    assert(fc_spawn_npc_first_free(&s, NPC_TZ_KIH, 10, 10) == 0);
    FcNpcSpellEffects clear = {0};
    assert(memcmp(effects, &clear, sizeof(clear)) == 0);

    /* Through the hit: a splash leaves no effect; a hit freezes Tz-Kih, which
     * walks no further until the freeze ends 32 ticks after it lands. */
    int idle[FC_NUM_ACTION_HEADS] = {0};
    magic_fixture(&s, &scenario, NPC_TZ_KIH, 20, 32);
    FcNpc *npc = &s.npcs[0];
    fc_queue_pending_hit(npc->pending_hits, &npc->num_pending_hits, FC_MAX_PENDING_HITS,
                         0, 1, ATTACK_MAGIC, -1, FC_SPELL_ICE_BARRAGE + 1);
    fc_step(&s, idle);
    assert(effects->frozen_until == 0 && npc->y == 31);
    int landed = s.tick;
    fc_queue_pending_hit(npc->pending_hits, &npc->num_pending_hits, FC_MAX_PENDING_HITS,
                         10, 1, ATTACK_MAGIC, -1, FC_SPELL_ICE_BARRAGE + 1);
    fc_step(&s, idle);
    assert(effects->frozen_until == landed + 32 && npc->y == 30);
    for (int t = 0; t < 31; t++) fc_step(&s, idle);
    assert(npc->y == 30);
    fc_step(&s, idle);
    assert(npc->y == 29);
    /* Poison: the first hit 31 ticks after the spell lands, then one less
     * every 31 ticks; it is the NPC's status, not a player hit. */
    magic_fixture(&s, &scenario, NPC_TOK_XIL, 20, 28);
    npc = &s.npcs[0];
    fc_queue_pending_hit(npc->pending_hits, &npc->num_pending_hits, FC_MAX_PENDING_HITS,
                         10, 1, ATTACK_MAGIC, -1, FC_SPELL_SMOKE_BARRAGE + 1);
    fc_step(&s, idle);
    int hp = npc->current_hp;
    assert(effects->poison_damage == 4 && hp == npc->max_hp - 10);
    for (int damage = 4; damage > 0; damage--) {
        for (int t = 0; t < 30; t++) fc_step(&s, idle);
        assert(npc->current_hp == hp);
        fc_step(&s, idle);
        hp -= damage * 10;
        assert(npc->current_hp == hp && s.hits_landed_this_tick == 0);
        assert(s.render_events.hit_count == 1 &&
               s.render_events.hits[0].attack_style == ATTACK_NONE);
    }
    assert(effects->poison_damage == 0);
    for (int t = 0; t < 40; t++) fc_step(&s, idle);
    assert(npc->current_hp == hp);
    puts("magic: freeze, poison, drain and heal as RuneC lands them passed");
}

/* Each magic preset at its levels attacks the nearest NPC for a few hundred
 * ticks and lands hits: casting its spell when it can (with the Ancient
 * book the Kodai wand's Ice Barrage), spending its runes, or else bashing or
 * punching. */
static void magic_preset_episodes(void) {
    for (int i = 0; i < PRESET_LEVEL_COUNT; i++) {
        if (!FC_GEAR_PRESETS[fc_gear_preset_find(PRESET_LEVELS[i].name)].autocast) continue;
        FcScenario scenario = levels_scenario(PRESET_LEVELS[i].name);
        FcState s;
        fc_init(&s);
        fc_reset(&s, 101);
        fc_apply_scenario(&s, &scenario);
#if FC_CAP_SPELLBOOK == 2
        fc_spell_set_autocast(&s, FC_SPELL_ICE_BARRAGE);
#endif
        int spell = fc_player_cast_spell(&s);
        int actions[FC_NUM_ACTION_HEADS] = {0};
        actions[FC_HEAD_ATTACK] = 1;
        int attacks = 0, casts = 0, hits = 0, ticks = 0;
        for (; ticks < 300 && !fc_is_terminal(&s); ticks++) {
            fc_step(&s, actions);
            attacks += s.render_events.player_attack_fired;
            casts += s.render_events.player_attack_spell > 0;
            hits += s.hits_landed_this_tick;
        }
        assert(hits > 0 && attacks > 0);
        if (spell >= 0) {
            assert(casts == attacks);
            for (int r = 0; r < FC_PRESET_RUNE_COUNT; r++)
                assert(rune_total(&s.player, FC_PRESET_RUNES[r]) == FC_PRESET_RUNE_QUANTITY -
                       casts * cast_cost(&s.player, spell, FC_PRESET_RUNES[r]));
        } else {
            assert(casts == 0);
        }
        printf("magic preset %s: autocasts %s, %d casts of %d attacks, %d hits in %d ticks\n",
               PRESET_LEVELS[i].name, spell >= 0 ? FC_SPELLS[spell].name : "nothing",
               casts, attacks, hits, ticks);
    }
}

#if FC_GEAR_SWITCHING
/* Switching between ranged and magic sets: tbow_masori worn, then
 * kodai_virtus, acb_armadyl and tbow_masori_no_ring as far as gear_sets
 * goes. */
static const char *const MAGIC_LINEUP[FC_MAX_GEAR_SETS] =
    {"tbow_masori", "kodai_virtus", "acb_armadyl", "tbow_masori_no_ring"};

static void magic_gear_switch_test(void) {
    FcScenario scenario = fc_default_scenario();
    for (int k = 0; k < FC_CAP_GEAR_SETS; k++)
        scenario.gear_set[k] = fc_gear_preset_find(MAGIC_LINEUP[k]);
    const int levels[FC_SKILL_COUNT] = {1, 1, 99, 99, 94, 99, 99};
    memcpy(scenario.levels, levels, sizeof(levels));
    char err[256], expected[192];
    /* kodai_virtus's 11 items, acb_armadyl's 7 and the 10 runes must fit
     * beside the supplies. */
    FcItemStack items[FC_MAX_SWITCH_ITEMS];
    int switch_items = fc_gear_switch_items(&scenario, items);
    assert(switch_items == (FC_CAP_GEAR_SETS >= 3 ? 18 : 11));
    scenario.sharks = FC_INVENTORY_SLOTS - switch_items - FC_PRESET_RUNE_COUNT;
    assert(fc_scenario_check(&scenario, err, sizeof(err)) == 0);
    scenario.sharks++;
    assert(fc_scenario_check(&scenario, err, sizeof(err)) != 0);
    snprintf(expected, sizeof(expected), "gear sets 2-%d put %d items in the inventory, the "
             "magic sets carry 10 rune stacks and the supplies take %d slots: %d is more "
             "than its 28", FC_CAP_GEAR_SETS, switch_items, scenario.sharks,
             switch_items + 10 + scenario.sharks);
    assert(strcmp(err, expected) == 0);
    scenario.sharks = 0;

    FcState s, again;
    gear_fixture(&s, &scenario);
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++) {
        int rune = i - switch_items;
        assert(s.player.inventory[i].item_id ==
               (i < switch_items ? items[i].item_id
                : rune < FC_PRESET_RUNE_COUNT ? FC_PRESET_RUNES[rune] : 0));
    }
    assert(s.caps.autocast == 0 && wears_preset(&s.player, "tbow_masori"));
    /* The gear block: the bow shoots, the Kodai wand casts 10 tiles every 5
     * ticks. */
    float obs[FC_POLICY_OBS_SIZE];
    fc_write_obs(&s, obs);
    const float *kodai = obs + FC_OBS_GEAR_START + FC_OBS_GEAR_WEAPONS + FC_OBS_GEAR_STRIDE;
    assert(obs[FC_OBS_GEAR_START + FC_OBS_GEAR_WEAPONS + FC_OBS_GEAR_RANGED] == 1.0f);
    assert(kodai[FC_OBS_GEAR_MAGIC] == 1.0f && kodai[FC_OBS_GEAR_RANGE] == 1.0f &&
           kodai[FC_OBS_GEAR_SPEED] == 0.5f);

    memset(s.npcs, 0, sizeof(s.npcs));
    memset(s.walkable, 1, sizeof(s.walkable));
    memset(s.movement_flags, 0, sizeof(s.movement_flags));
    memset(s.los_flags, 0, sizeof(s.los_flags));
    s.player.x = 20;
    s.player.y = 20;
    fc_npc_spawn(&s.npcs[0], NPC_TOK_XIL, 20, 28, 1);
    s.npcs_remaining = 1;
    s.npcs[0].attack_timer = 100000;
    s.npcs[0].current_hp = s.npcs[0].max_hp = 100000;
    again = s;
    /* The switch wields the Kodai wand and sets kodai_virtus's Water Surge,
     * which the attack in the same tick casts. With the Ancient book Water
     * Surge cannot be set, and the autocast head's Ice Barrage is. */
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_GEAR_SET] = 2;
#if FC_SPELLBOOK_HEAD
    if (FC_PLAYER_SPELLBOOK == FC_SPELLBOOK_ANCIENT)
        actions[FC_HEAD_AUTOCAST] = FC_SPELL_ICE_BARRAGE - FC_BOOK_FIRST_SPELL + 1;
#endif
    const int spell = FC_PLAYER_SPELLBOOK == FC_SPELLBOOK_ANCIENT
        ? FC_SPELL_ICE_BARRAGE : FC_SPELL_WATER_SURGE;
    fc_step(&s, actions);
    assert(s.caps.gear_set == 1 && wears_preset(&s.player, "kodai_virtus"));
    assert(s.caps.autocast == spell + 1 && s.render_events.player_attack_spell == spell + 1);
    /* Back to the bow: the spell stays set, the bow shoots arrows. */
    actions[FC_HEAD_GEAR_SET] = 1;
#if FC_SPELLBOOK_HEAD
    actions[FC_HEAD_AUTOCAST] = FC_AUTOCAST_KEEP;
#endif
    s.player.attack_timer = 0;
    fc_step(&s, actions);
    assert(s.caps.gear_set == 0 && s.caps.autocast == spell + 1 &&
           fc_player_cast_spell(&s) == -1);
    assert(s.render_events.player_attack_style == ATTACK_RANGED &&
           item_total(&s.player, 11212) == 49999);

    /* An episode switching every 25 ticks: both sets attack, the Kodai wand
     * casting, deterministically. */
    s = again;
    int attacks[2] = {0}, casts = 0;
    for (int tick = 0; tick < 400; tick++) {
        int set = tick / 25 % 2;
        actions[FC_HEAD_GEAR_SET] = s.caps.gear_set != set ? set + 1 : FC_GEAR_SET_NO_CHANGE;
#if FC_SPELLBOOK_HEAD
        actions[FC_HEAD_AUTOCAST] = tick == 25 && FC_PLAYER_SPELLBOOK == FC_SPELLBOOK_ANCIENT
            ? FC_SPELL_ICE_BARRAGE - FC_BOOK_FIRST_SPELL + 1 : FC_AUTOCAST_KEEP;
#endif
        fc_step(&s, actions);
        fc_step(&again, actions);
        assert(fc_state_hash(&s) == fc_state_hash(&again));
        attacks[set] += s.render_events.player_attack_fired;
        casts += s.render_events.player_attack_spell == spell + 1;
    }
    assert(attacks[0] > 0 && attacks[1] > 0 && casts == attacks[1]);
    assert(item_total(&s.player, 11212) == 50000 - attacks[0]);
    for (int r = 0; r < FC_PRESET_RUNE_COUNT; r++)
        assert(rune_total(&s.player, FC_PRESET_RUNES[r]) == FC_PRESET_RUNE_QUANTITY -
               casts * cast_cost(&s.player, spell, FC_PRESET_RUNES[r]));
    printf("magic gear sets: %d arrows with tbow_masori and %d casts of %s with "
           "kodai_virtus in 400 ticks passed\n", attacks[0], casts, FC_SPELLS[spell].name);
}
#endif

#if FC_SPELLBOOK_HEAD
/* The autocast head reaches the core through the policy actions; its mask
 * and the spellbook block are observed. */
static void spellbook_adapter_test(void) {
    const int head = FC_PUFFER_NUM_ATNS - 1 - FC_CAP_SPECIAL_ATTACKS;
    const int mask = FC_PUFFER_MASK_SIZE - FC_AUTOCAST_DIM -
                     FC_CAP_SPECIAL_ATTACKS * FC_SPECIAL_ATTACK_DIM;
    assert(FC_PUFFER_ACTION_HEADS[head] == FC_HEAD_AUTOCAST &&
           strcmp(FC_HEAD_NAMES[FC_HEAD_AUTOCAST], "autocast") == 0 &&
           FC_PUFFER_ACTION_DIMS[head] == FC_AUTOCAST_DIM);
    Dict kwargs = {0};
    puf_ini_set(&kwargs, "gear_set_1", "kodai_virtus");
    other_gear_sets(&kwargs, "kodai_virtus");
    dict_set(&kwargs, "magic_level", 80);
    dict_set(&kwargs, "hitpoints_level", 90);
    FightCaves* env = test_env(&kwargs);
    Agent* a = &env->agents[0];
    FcPlayer* p = &env->state.player;
    const float* obs = a->observations + FC_OBS_SPELL_START;
    /* At Magic 80 the book's spells above it are masked; kodai_virtus's
     * Water Surge needs 85 (and with the Ancient book is the other book's),
     * so nothing is autocast. */
    assert(env->state.caps.autocast == 0);
    assert(a->action_mask[mask + FC_AUTOCAST_KEEP] == 1);
    for (int v = 1; v < FC_AUTOCAST_DIM; v++)
        assert(a->action_mask[mask + v] == (FC_SPELLS[FC_BOOK_FIRST_SPELL + v - 1].level <= 80));
    for (int i = 0; i < FC_OBS_SPELL_SIZE; i++) assert(obs[i] == 0.0f);
    /* Autocast the book's first spell: it is observed. */
    a->actions[head] = 1;
    checked_step(env);
    assert(env->state.caps.autocast == FC_BOOK_FIRST_SPELL + 1);
    for (int k = 0; k < FC_BOOK_SPELL_COUNT; k++)
        assert(obs[FC_OBS_SPELL_AUTOCAST + k] == (k == 0 ? 1.0f : 0.0f));
    /* Without death runes the spells that need them are masked. */
    a->actions[head] = FC_AUTOCAST_KEEP;
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (p->inventory[i].item_id == FC_RUNE_DEATH) p->inventory[i] = (FcItemStack){0};
    fc_puffer_write_obs(env);
    check_obs(env);
    for (int v = 1; v < FC_AUTOCAST_DIM; v++) {
        int spell = FC_BOOK_FIRST_SPELL + v - 1;
        assert(a->action_mask[mask + v] ==
               (FC_SPELLS[spell].level <= 80 && !cast_cost(p, spell, FC_RUNE_DEATH)));
    }
#if FC_CAP_SPELLBOOK == 2
    /* Each visible NPC slot's spell effects. */
    int visible[FC_VISIBLE_NPCS];
    assert(fc_visible_npc_indices(&env->state, visible) > 0);
    int attack = fc_npc_get_stats(env->state.npcs[visible[0]].npc_type)
                     ->attack_level[ATTACK_MELEE];
    env->state.caps.npc_spells[visible[0]] = (FcNpcSpellEffects){
        env->state.tick + 16, env->state.tick + 21, 2, 30, attack / 10};
    fc_puffer_write_obs(env);
    check_obs(env);
    const float* effects = obs + FC_OBS_SPELL_EFFECTS;
    assert(effects[FC_OBS_SPELL_FROZEN] == 0.5f &&
           effects[FC_OBS_SPELL_FREEZE_IMMUNE] == 21.0f / 37.0f &&
           effects[FC_OBS_SPELL_POISON] == 0.5f &&
           effects[FC_OBS_SPELL_DRAIN] == (float)(attack / 10) / (float)attack);
    for (int i = FC_OBS_SPELL_STRIDE; i < FC_OBS_SPELL_STRIDE * FC_OBS_NPC_SLOTS; i++)
        assert(effects[i] == 0.0f);
#endif
    /* Without the wand nothing can be autocast. */
    assert(fc_unequip_item(&env->state, FC_EQUIP_SLOT_WEAPON) == FC_ITEM_OK);
    fc_puffer_write_obs(env);
    check_obs(env);
    for (int v = 0; v < FC_AUTOCAST_DIM; v++)
        assert(a->action_mask[mask + v] == (v == FC_AUTOCAST_KEEP));
    checked_step(env);
    test_free(env);
    dict_clear(&kwargs);
    puts("spellbook: autocast head, masks, autocast spell and effects observed passed");
}
#endif

/* The magic presets' requirement and inventory errors. */
static void magic_config_test(void) {
    Dict kwargs = {0};
    puf_ini_set(&kwargs, "gear_set_1", "kodai_virtus");
    other_gear_sets(&kwargs, "kodai_virtus");
    dict_set(&kwargs, "magic_level", 74);
    expect_config_error(&kwargs, "gear_set_1 = kodai_virtus: Kodai wand needs 75 magic, "
                        "but magic_level = 74");
    puf_ini_set(&kwargs, "gear_set_1", "smoke_staff_mystic");
    other_gear_sets(&kwargs, "smoke_staff_mystic");
    dict_set(&kwargs, "magic_level", 60);
    dict_set(&kwargs, "attack_level", 39);
    expect_config_error(&kwargs, "gear_set_1 = smoke_staff_mystic: Mystic smoke staff "
                        "needs 40 attack, but attack_level = 39");
    dict_set(&kwargs, "attack_level", 40);
    /* The 10 rune stacks and the supplies share the inventory. */
    char expected[160];
    dict_set(&kwargs, "initial_sharks", 19);
    if (FC_CAP_GEAR_SETS == 1)
        snprintf(expected, sizeof(expected), "gear_set_1 = smoke_staff_mystic carries 10 "
                 "rune stacks and the supplies take 19 slots: 29 is more than its 28");
    else
        snprintf(expected, sizeof(expected), "gear sets 2-%d put 0 items in the inventory, "
                 "the magic sets carry 10 rune stacks and the supplies take 19 slots: 29 "
                 "is more than its 28", FC_CAP_GEAR_SETS);
    expect_config_error(&kwargs, expected);
    dict_set(&kwargs, "initial_sharks", 18);
    FightCaves* env = test_env(&kwargs);
    const FcPlayer* p = &env->state.player;
    assert(env->state.active_loadout == FC_LOADOUT_SMOKE_STAFF_MYSTIC);
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        assert(p->inventory[i].item_id == (i < FC_PRESET_RUNE_COUNT
               ? FC_PRESET_RUNES[i] : FC_ITEM_ID_SHARK));
    assert(env->state.caps.autocast == (FC_PLAYER_SPELLBOOK == FC_SPELLBOOK_STANDARD
                                        ? FC_SPELL_WATER_BLAST + 1 : 0));
    check_obs(env);
    test_free(env);
    dict_clear(&kwargs);
    puts("magic config: Magic and Attack requirements and the runes' inventory passed");
}

/* Melee, against RuneC_v4: rc-core/combat_formula.c (the stance bonuses,
 * rc_refresh_player_combat_style, rc_calc_melee, rc_player_attack_speed and
 * rc_player_attack_range), combat.c's melee reach (player_can_attack_npc_now),
 * rc-content/combat/magic.c's rc_content_player_hit_delay, the items of
 * data/defs/items.bin, and the expected values of
 * tests/test_combat_phase5_formula_core.c (formula core) and
 * tests/test_combat_phase4_weapon_styles_cycle.c (weapon styles). */

/* The melee hits that landed on an NPC this tick. */
static int melee_hits_on_npcs(const FcState *s) {
    int hits = 0;
    for (int i = 0; i < s->render_events.hit_count; i++)
        hits += s->render_events.hits[i].target_entity_type == ENTITY_NPC &&
                s->render_events.hits[i].attack_style == ATTACK_MELEE;
    return hits;
}

static void melee_formula_test(void) {
    /* Formula core test_melee_formula_uses_boosts_prayer_equipment_and_npc_defence:
     * Piety's 20% Attack and 23% Strength, Attack 70, Strength 80,
     * Aggressive, +100 slash and +50 Strength: roll 15088 and max hit 19,
     * against a Defence 40 NPC without a bonus, 3136. */
    FcPrayerBonus piety = {.attack = 20, .strength = 23, .defence = 25}, none = {0};
    int roll, max_hit;
    fc_melee_rolls(70, 80, 100, 50, FC_STANCE_AGGRESSIVE, &piety, &roll, &max_hit);
    assert(roll == 15088 && max_hit == 19);
    assert(fc_npc_roll(40, 0) == 3136);
    /* Each stance's invisible levels (stance_attack_bonus,
     * stance_strength_bonus, stance_defence_bonus) and the rolls at Attack
     * 70, Strength 80, +100 attack and +150 Strength without prayers:
     * (70 + stance + 8) * 164 and ((80 + stance + 8) * 214 + 320) / 640. */
    static const struct { int stance, attack, strength, defence, roll, max_hit; } stances[] = {
        {FC_STANCE_ACCURATE, 3, 0, 0, 13284, 29},
        {FC_STANCE_AGGRESSIVE, 0, 3, 0, 12792, 30},
        {FC_STANCE_DEFENSIVE, 0, 0, 3, 12792, 29},
        {FC_STANCE_CONTROLLED, 1, 1, 1, 12956, 30},
    };
    for (int i = 0; i < 4; i++) {
        int attack, strength, defence;
        fc_stance_levels(stances[i].stance, &attack, &strength, &defence);
        assert(attack == stances[i].attack && strength == stances[i].strength &&
               defence == stances[i].defence);
        fc_melee_rolls(70, 80, 100, 150, stances[i].stance, &none, &roll, &max_hit);
        assert(roll == stances[i].roll && max_hit == stances[i].max_hit);
    }
    /* Formula core test_npc_formula_uses_style_stat_and_player_defence_roll:
     * Defensive adds 3 to Defence 50 and Magic 90 blended, 89, against +30
     * magic defence: 8366. Controlled adds 1 to Defence: (50 + 1 + 8) * 128. */
    FcPlayer defender = {.defence_level = 50, .magic_level = 90, .defence_magic = 30,
                         .defence_crush = 64};
    assert(fc_player_def_roll(&defender, &none, 3, FC_ATTACK_TYPE_MAGIC) == 8366);
    assert(fc_player_def_roll(&defender, &none, 1, FC_ATTACK_TYPE_CRUSH) == 59 * 128);

    /* rc_refresh_player_combat_style by weapon type and attack style index
     * (-1: unavailable). Weapon styles' test_category_specific_and_unavailable_slots
     * pins unarmed 2 and 3, spiked 2 and staff 2;
     * test_slash_sword_table_sets_style_stance_xp_and_metadata the slash
     * sword's 0 and 2; the rest is combat_formula.c's tables. */
    enum { T_ST = FC_ATTACK_TYPE_STAB, T_SL = FC_ATTACK_TYPE_SLASH,
           T_CR = FC_ATTACK_TYPE_CRUSH, S_ACC = FC_STANCE_ACCURATE,
           S_AGG = FC_STANCE_AGGRESSIVE, S_DEF = FC_STANCE_DEFENSIVE,
           S_CON = FC_STANCE_CONTROLLED };
    static const struct { int type, styles[4][2]; } table[] = {
        {FC_WEAPON_TYPE_UNARMED, {{T_CR, S_ACC}, {T_CR, S_AGG}, {-1, -1}, {T_CR, S_DEF}}},
        {FC_WEAPON_TYPE_STAFF, {{T_CR, S_ACC}, {T_CR, S_AGG}, {-1, -1}, {T_CR, S_DEF}}},
        {FC_WEAPON_TYPE_SCYTHE, {{T_SL, S_ACC}, {T_SL, S_AGG}, {T_CR, S_AGG}, {T_SL, S_DEF}}},
        {FC_WEAPON_TYPE_SLASH_SWORD,
         {{T_SL, S_ACC}, {T_SL, S_AGG}, {T_ST, S_CON}, {T_SL, S_DEF}}},
        {FC_WEAPON_TYPE_SPIKED, {{T_CR, S_ACC}, {T_CR, S_AGG}, {T_ST, S_CON}, {T_CR, S_DEF}}},
        {FC_WEAPON_TYPE_STAB_SWORD,
         {{T_ST, S_ACC}, {T_ST, S_AGG}, {T_SL, S_CON}, {T_ST, S_DEF}}},
    };
    for (int t = 0; t < (int)(sizeof(table) / sizeof(table[0])); t++) {
        for (int index = 0; index < 4; index++) {
            FcMeleeStyle style;
            int available = fc_melee_style(table[t].type, index, &style);
            assert(available == (table[t].styles[index][0] >= 0));
            if (available)
                assert(style.attack_type == table[t].styles[index][0] &&
                       style.stance == table[t].styles[index][1]);
        }
    }
    FcMeleeStyle style;
    assert(!fc_melee_style(FC_WEAPON_TYPE_SLASH_SWORD, -1, &style) &&
           !fc_melee_style(FC_WEAPON_TYPE_SLASH_SWORD, 4, &style) &&
           !fc_melee_style(25, 0, &style));   /* a bow attacks with Ranged */

    /* The melee presets at their levels: their style's attack type, the worn
     * bonuses for it and the Strength bonus (items.bin), and the rolls,
     * Accurate: (Attack + 3 + 8) * (bonus + 64) and
     * ((Strength + 8) * (Strength bonus + 64) + 320) / 640. */
    static const struct {
        const char *name;
        int attack_type, bonus, strength, roll, max_hit, speed;
    } presets[] = {
        {"dragon_scimitar_torso", T_SL, 114, 111, 71 * 178, 19, 4},
        {"macuahuitl_blood_moon", T_CR, 144, 127, 81 * 208, 25, 4},
        {"fang_bandos", T_ST, 169, 164, 93 * 233, 33, 5},
        {"scythe_torva", T_SL, 170, 139, 110 * 234, 34, 5},
    };
    FcState s;
    for (int i = 0; i < 4; i++) {
        FcScenario scenario = levels_scenario(presets[i].name);
        fc_init(&s);
        fc_reset(&s, 101);
        fc_apply_scenario(&s, &scenario);
        assert(s.player.weapon_kind == FC_WEAPON_MELEE && s.player.weapon_range == 1 &&
               fc_player_attack_speed(&s) == presets[i].speed);
        assert(fc_player_melee_style(&s, &style) &&
               style.attack_type == presets[i].attack_type && style.stance == S_ACC);
        assert(fc_player_melee_bonus(&s.player, style.attack_type) == presets[i].bonus &&
               s.player.melee_strength_bonus == presets[i].strength);
        fc_player_melee_rolls(&s.player, &none, &style, &roll, &max_hit);
        assert(roll == presets[i].roll && max_hit == presets[i].max_hit);
        assert(fc_player_stance_defence(&s) == 0);   /* Accurate */
    }
#if FC_CAP_PRAYERS
    /* Piety on scythe_torva: Attack 99 * 120 / 100 + 3 + 8 = 129 and Strength
     * 99 * 123 / 100 + 8 = 129, so ((129 * 203) + 320) / 640 = 41. */
    s.caps.prayers = 1u << FC_BOOK_PIETY;
    FcPrayerBonus bonus = fc_prayer_bonus(&s);
    fc_player_melee_rolls(&s.player, &bonus, &style, &roll, &max_hit);
    assert(roll == 129 * 234 && max_hit == 41);
#endif
    /* The ranged presets punch in their style 0, Accurate crush, with the
     * crush bonus; a bow does not attack in melee. */
    FcScenario scenario = preset_scenario(0);
    fc_init(&s);
    fc_reset(&s, 101);
    fc_apply_scenario(&s, &scenario);
    assert(!fc_player_melee_style(&s, &style) && fc_player_stance_defence(&s) == 0);
    assert(fc_unequip_item(&s, FC_EQUIP_SLOT_WEAPON) == FC_ITEM_OK);
    assert(fc_player_melee_style(&s, &style) && style.attack_type == T_CR &&
           style.stance == S_ACC && fc_player_melee_bonus(&s.player, T_CR) == -8);
    puts("melee: stances, weapon styles, rolls and the presets' bonuses against RuneC passed");
}

/* A melee weapon through fc_step: the approach to open cardinal contact, the
 * hit landing the tick after the attack (the punch's lands in its tick, see
 * combat), the weapon's speed, and the reach of 1. */
static void melee_attack_test(void) {
    FcScenario scenario = levels_scenario("dragon_scimitar_torso");
    FcState s;
    magic_fixture(&s, &scenario, NPC_TZ_KEK, 27, 20);
    s.npcs[0].current_hp = s.npcs[0].max_hp = 100000;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
    int ticks = 0;
    for (; ticks < 20 && !s.render_events.player_attack_fired; ticks++) {
        assert(melee_hits_on_npcs(&s) == 0);
        fc_step(&s, actions);
    }
    /* It walked to a tile beside the NPC and attacked from there. */
    const FcNpc *npc = &s.npcs[0];
    assert(s.render_events.player_attack_fired && ticks > 1);
    assert(fc_npc_can_melee_player(s.player.x, s.player.y, npc->x, npc->y, npc->size,
                                   s.movement_flags));
    assert(s.render_events.player_attack_style == ATTACK_MELEE &&
           s.render_events.player_attack_spell == 0 &&
           s.render_events.player_attack_hit_delay_ticks == 2);
    assert(npc->num_pending_hits == 1 && melee_hits_on_npcs(&s) == 0);
    assert(s.player.attack_timer == 3);   /* speed 4, end-of-tick decrement */
    fc_step(&s, actions);
    assert(!s.render_events.player_attack_fired && melee_hits_on_npcs(&s) == 1 &&
           npc->num_pending_hits == 0);
    int gap = 1;
    for (; gap < 10 && !s.render_events.player_attack_fired; gap++) fc_step(&s, actions);
    assert(gap == 4);
    /* Reach 1 is open cardinal contact (RuneC's rc_route_target_reached for an
     * adjacent melee attack): not a corner, two tiles away, the footprint
     * itself or across a wall. */
    assert(fc_can_attack_from(26, 20, 27, 20, 2, FC_ROUTE_MELEE_RANGE,
                              s.movement_flags, s.los_flags));
    assert(!fc_can_attack_from(26, 19, 27, 20, 2, FC_ROUTE_MELEE_RANGE,
                               s.movement_flags, s.los_flags));
    assert(!fc_can_attack_from(25, 20, 27, 20, 2, FC_ROUTE_MELEE_RANGE,
                               s.movement_flags, s.los_flags));
    assert(!fc_can_attack_from(27, 20, 27, 20, 2, FC_ROUTE_MELEE_RANGE,
                               s.movement_flags, s.los_flags));
    s.movement_flags[26][20] = FC_MOVE_WALL_EAST;
    assert(!fc_can_attack_from(26, 20, 27, 20, 2, FC_ROUTE_MELEE_RANGE,
                               s.movement_flags, s.los_flags));
    /* The Osmumten's fang attacks every 5 ticks. */
    scenario = levels_scenario("fang_bandos");
    magic_fixture(&s, &scenario, NPC_TZ_KEK, 21, 20);
    s.npcs[0].current_hp = s.npcs[0].max_hp = 100000;
    int attacks = 0;
    for (int tick = 0; tick < 21; tick++) {
        fc_step(&s, actions);
        attacks += s.render_events.player_attack_fired;
    }
    assert(attacks == 5);
    puts("melee: approach, cardinal reach, next-tick hits and weapon speeds passed");
}

/* Each melee preset at its levels attacks the nearest NPC for a few hundred
 * ticks: melee attacks at no faster than its weapon's speed, landing hits,
 * spending nothing. */
static void melee_preset_episodes(void) {
    for (int p = 0; p < FC_GEAR_PRESET_COUNT; p++) {
        const FcGearPreset *preset = &FC_GEAR_PRESETS[p];
        if (fc_gear_preset_weapon(preset)->weapon_kind != FC_WEAPON_MELEE) continue;
        FcScenario scenario = levels_scenario(preset->name);
        FcState s;
        fc_init(&s);
        fc_reset(&s, 101);
        fc_apply_scenario(&s, &scenario);
        FcItemStack equipment[FC_EQUIPMENT_SLOTS];
        memcpy(equipment, s.player.equipment, sizeof(equipment));
        int actions[FC_NUM_ACTION_HEADS] = {0};
        actions[FC_HEAD_ATTACK] = 1;
        int attacks = 0, hits = 0, ticks = 0, last = -100;
        for (; ticks < 300 && !fc_is_terminal(&s); ticks++) {
            fc_step(&s, actions);
            if (s.render_events.player_attack_fired) {
                assert(s.render_events.player_attack_style == ATTACK_MELEE &&
                       s.render_events.player_attack_hit_delay_ticks == 2);
                assert(ticks - last >= s.player.weapon_speed);
                last = ticks;
                attacks++;
            }
            hits += s.hits_landed_this_tick;
        }
        assert(attacks > 0 && hits > 0);
        assert(memcmp(equipment, s.player.equipment, sizeof(equipment)) == 0);
        printf("melee preset %s: %s, speed %d: %d attacks, %d hits in %d ticks\n",
               preset->name, fc_gear_preset_weapon(preset)->name, s.player.weapon_speed,
               attacks, hits, ticks);
    }
}

#if FC_GEAR_SWITCHING
/* Switching between magic, melee and ranged sets: kodai_virtus worn, then
 * macuahuitl_blood_moon and rcb_black_dhide as far as gear_sets goes; a
 * fourth set repeats kodai_virtus, which is worn. */
static const char *const MELEE_LINEUP[FC_MAX_GEAR_SETS] =
    {"kodai_virtus", "macuahuitl_blood_moon", "rcb_black_dhide", "kodai_virtus"};

static void melee_gear_switch_test(void) {
    FcScenario scenario = fc_default_scenario();
    for (int k = 0; k < FC_CAP_GEAR_SETS; k++)
        scenario.gear_set[k] = fc_gear_preset_find(MELEE_LINEUP[k]);
    /* Water Surge's 85 Magic, Eternal boots' 75 Defence, the crossbow set's
     * 70 Ranged; the Blood moon set needs nothing in RuneC's items.bin. */
    const int levels[FC_SKILL_COUNT] = {70, 75, 75, 70, 94, 70, 90};
    memcpy(scenario.levels, levels, sizeof(levels));
    char err[256];
    FcItemStack items[FC_MAX_SWITCH_ITEMS];
    int switch_items = fc_gear_switch_items(&scenario, items);
    const int sets = FC_CAP_GEAR_SETS < 3 ? FC_CAP_GEAR_SETS : 3;
    assert(switch_items == (sets == 3 ? 9 + 7 : 9));
    scenario.sharks = FC_INVENTORY_SLOTS - switch_items - FC_PRESET_RUNE_COUNT;
    assert(fc_scenario_check(&scenario, err, sizeof(err)) == 0);
    scenario.sharks = 0;

    FcState s, again;
    gear_fixture(&s, &scenario);
    assert(wears_preset(&s.player, "kodai_virtus"));
    /* The gear block: the Kodai wand casts, the Dual macuahuitl attacks in
     * melee from 1 tile every 4 ticks, the Rune crossbow shoots 7 tiles every
     * 5. */
    float obs[FC_POLICY_OBS_SIZE];
    fc_write_obs(&s, obs);
    const float *weapons = obs + FC_OBS_GEAR_START + FC_OBS_GEAR_WEAPONS;
    const float *melee_set = weapons + FC_OBS_GEAR_STRIDE;
    assert(weapons[FC_OBS_GEAR_MAGIC] == 1.0f && weapons[FC_OBS_GEAR_MELEE] == 0.0f);
    assert(melee_set[FC_OBS_GEAR_MELEE] == 1.0f && melee_set[FC_OBS_GEAR_RANGED] == 0.0f &&
           melee_set[FC_OBS_GEAR_MAGIC] == 0.0f && melee_set[FC_OBS_GEAR_RANGE] == 0.1f &&
           melee_set[FC_OBS_GEAR_SPEED] == 0.4f);
    if (sets == 3) {
        const float *crossbow = weapons + 2 * FC_OBS_GEAR_STRIDE;
        assert(crossbow[FC_OBS_GEAR_RANGED] == 1.0f && crossbow[FC_OBS_GEAR_MELEE] == 0.0f &&
               crossbow[FC_OBS_GEAR_RANGE] == 0.7f && crossbow[FC_OBS_GEAR_SPEED] == 0.5f);
    }

    /* A Tok-Xil beside the player, in reach of every set. */
    memset(s.npcs, 0, sizeof(s.npcs));
    memset(s.walkable, 1, sizeof(s.walkable));
    memset(s.movement_flags, 0, sizeof(s.movement_flags));
    memset(s.los_flags, 0, sizeof(s.los_flags));
    s.player.x = 20;
    s.player.y = 20;
    fc_npc_spawn(&s.npcs[0], NPC_TOK_XIL, 21, 19, 1);
    s.npcs_remaining = 1;
    s.npcs[0].attack_timer = 100000;
    s.npcs[0].current_hp = s.npcs[0].max_hp = 100000;
    again = s;
    /* The wand casts kodai_virtus's Water Surge, or with the Ancient book the
     * autocast head's Ice Barrage. */
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
    const int ancient = FC_PLAYER_SPELLBOOK == FC_SPELLBOOK_ANCIENT;
    const int spell = ancient ? FC_SPELL_ICE_BARRAGE : FC_SPELL_WATER_SURGE;
#if FC_SPELLBOOK_HEAD
    actions[FC_HEAD_AUTOCAST] = ancient ? FC_SPELL_ICE_BARRAGE - FC_BOOK_FIRST_SPELL + 1
                                        : FC_AUTOCAST_KEEP;
#endif
    fc_step(&s, actions);
    assert(s.render_events.player_attack_spell == spell + 1);
#if FC_SPELLBOOK_HEAD
    actions[FC_HEAD_AUTOCAST] = FC_AUTOCAST_KEEP;
#endif
    /* The switch wields the Dual macuahuitl, which attacks in the same tick:
     * Pound, an Accurate crush landing the next tick; the spell stays set. */
    actions[FC_HEAD_GEAR_SET] = 2;
    s.player.attack_timer = 0;
    fc_step(&s, actions);
    FcMeleeStyle style;
    assert(s.caps.gear_set == 1 && s.active_loadout == FC_LOADOUT_MACUAHUITL_BLOOD_MOON &&
           wears_preset(&s.player, "macuahuitl_blood_moon"));
    assert(s.render_events.player_attack_fired &&
           s.render_events.player_attack_style == ATTACK_MELEE &&
           s.render_events.player_attack_hit_delay_ticks == 2);
    assert(s.caps.autocast == spell + 1 && fc_player_cast_spell(&s) == -1);
    assert(fc_player_melee_style(&s, &style) && style.attack_type == FC_ATTACK_TYPE_CRUSH &&
           style.stance == FC_STANCE_ACCURATE);
    assert(item_total(&s.player, 21006) == 1);   /* the Kodai wand, put away */
    if (sets == 3) {
        /* The crossbow shoots a bolt. */
        actions[FC_HEAD_GEAR_SET] = 3;
        s.player.attack_timer = 0;
        fc_step(&s, actions);
        assert(wears_preset(&s.player, "rcb_black_dhide") &&
               s.render_events.player_attack_style == ATTACK_RANGED &&
               item_total(&s.player, 9143) == 49999);
    }
    /* Back to the wand: it casts again. */
    actions[FC_HEAD_GEAR_SET] = 1;
    s.player.attack_timer = 0;
    fc_step(&s, actions);
    assert(s.caps.gear_set == 0 && s.render_events.player_attack_spell == spell + 1);

    /* An episode switching every 25 ticks, ending on the wand: every set
     * attacks in its style, deterministically. */
    s = again;
    int attacks[3] = {0}, casts = 0, melee = 0;
    for (int tick = 0; tick < 625; tick++) {
        int set = tick / 25 % sets;
        actions[FC_HEAD_GEAR_SET] = s.caps.gear_set != set ? set + 1 : FC_GEAR_SET_NO_CHANGE;
#if FC_SPELLBOOK_HEAD
        actions[FC_HEAD_AUTOCAST] = tick == 0 && ancient
            ? FC_SPELL_ICE_BARRAGE - FC_BOOK_FIRST_SPELL + 1 : FC_AUTOCAST_KEEP;
#endif
        fc_step(&s, actions);
        fc_step(&again, actions);
        assert(fc_state_hash(&s) == fc_state_hash(&again));
        attacks[set] += s.render_events.player_attack_fired;
        casts += s.render_events.player_attack_spell == spell + 1;
        melee += s.render_events.player_attack_fired &&
                 s.render_events.player_attack_style == ATTACK_MELEE;
    }
    assert(s.caps.gear_set == 0 && attacks[0] > 0 && attacks[1] > 0);
    assert(casts == attacks[0] && melee == attacks[1]);
    if (sets == 3) assert(attacks[2] > 0 && item_total(&s.player, 9143) == 50000 - attacks[2]);
    for (int r = 0; r < FC_PRESET_RUNE_COUNT; r++)
        assert(rune_total(&s.player, FC_PRESET_RUNES[r]) == FC_PRESET_RUNE_QUANTITY -
               casts * cast_cost(&s.player, spell, FC_PRESET_RUNES[r]));
    printf("melee gear sets: %d casts with kodai_virtus, %d melee attacks with "
           "macuahuitl_blood_moon and %d bolts with rcb_black_dhide in 625 ticks passed\n",
           casts, melee, sets == 3 ? attacks[2] : 0);
}
#endif

/* The melee presets' requirement errors, the Strength requirement on
 * equipping, and a melee preset through the adapter. */
static void melee_config_test(void) {
    Dict kwargs = {0};
    puf_ini_set(&kwargs, "gear_set_1", "scythe_torva");
    other_gear_sets(&kwargs, "scythe_torva");
    dict_set(&kwargs, "attack_level", 99);
    dict_set(&kwargs, "strength_level", 74);
    expect_config_error(&kwargs, "gear_set_1 = scythe_torva: Scythe of vitur needs 75 "
                        "strength, but strength_level = 74");
    puf_ini_set(&kwargs, "gear_set_1", "fang_bandos");
    other_gear_sets(&kwargs, "fang_bandos");
    expect_config_error(&kwargs, "gear_set_1 = fang_bandos: Primordial boots needs 75 "
                        "strength, but strength_level = 74");
    puf_ini_set(&kwargs, "gear_set_1", "dragon_scimitar_torso");
    other_gear_sets(&kwargs, "dragon_scimitar_torso");
    dict_set(&kwargs, "attack_level", 59);
    expect_config_error(&kwargs, "gear_set_1 = dragon_scimitar_torso: Dragon scimitar "
                        "needs 60 attack, but attack_level = 59");
    puf_ini_set(&kwargs, "gear_set_1", "fang_bandos");
    other_gear_sets(&kwargs, "fang_bandos");
    dict_set(&kwargs, "attack_level", 99);
    dict_set(&kwargs, "strength_level", 75);
    FightCaves* env = test_env(&kwargs);
    FcState* st = &env->state;
    assert(st->active_loadout == FC_LOADOUT_FANG_BANDOS && st->caps.autocast == 0);
    assert(st->player.equipment[FC_EQUIP_SLOT_WEAPON].item_id == 26219 &&
           st->player.weapon_kind == FC_WEAPON_MELEE);
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++) assert(st->player.inventory[i].item_id == 0);
    check_obs(env);
    /* Wearing the Scythe of vitur needs 75 Strength; it is two-handed, so
     * the Avernic defender comes off with the fang. */
    st->player.inventory[0] = (FcItemStack){22325, 1, 0};
    st->player.strength_level = 74;
    assert(fc_equip_item(st, 0) == FC_ITEM_REQUIREMENTS);
    st->player.strength_level = 75;
    assert(fc_equip_item(st, 0) == FC_ITEM_OK);
    assert(st->player.equipment[FC_EQUIP_SLOT_WEAPON].item_id == 22325 &&
           st->player.equipment[FC_EQUIP_SLOT_SHIELD].item_id == 0 &&
           item_slot(&st->player, 26219) >= 0 && item_slot(&st->player, 22322) >= 0);
    fc_puffer_write_obs(env);
    check_obs(env);
    test_free(env);
    dict_clear(&kwargs);
    puts("melee config: Attack and Strength requirements and a melee preset passed");
}

#if FC_CAP_SPECIAL_ATTACKS
/* Special attacks, against RuneC_v4: rc-core/combat.h's energy constants;
 * rc-core/combat.c's rc_combat_toggle_special, the special check and payment
 * of rc_combat_tick_player's launch and the recovery of
 * rc_combat_tick_player_status; rc-content/combat/regular_npc_combat.c's
 * g_player_specials rows for the Toxic blowpipe (5000 energy, 150%) and the
 * Armadyl crossbow (5000, 100%) and regular_modify_player_special_damage;
 * and the expected values of tests/test_combat_phase10_resources_specials.c's
 * test_special_spends_energy_and_recovers (a special spends 10000 down to
 * 5000 and disarms; 49 recovery ticks leave 5000, the 50th makes 6000) and
 * test_content_specials_spend_energy_and_modify_damage (a content special
 * spends 5000, disarms and scales the hit by its percentage). RuneC has no
 * test of these two weapons' specials. */

/* Whether the SPECIAL_ATTACK toggle takes effect now, by the mask. */
static int special_mask(const FcState *s) {
    float mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(s, mask);
    assert(mask[FC_MASK_SPECIAL_START + FC_SPECIAL_NO_CHANGE] == 1.0f);
    return mask[FC_MASK_SPECIAL_START + FC_SPECIAL_TOGGLE] != 0.0f;
}

/* magic_fixture for a preset in every gear set, with a Tz-Kih at (x, 20)
 * that does not attack or die. */
static void special_fixture(FcState *s, const char *preset, int x) {
    FcScenario scenario = preset_scenario(fc_gear_preset_find(preset));
    magic_fixture(s, &scenario, NPC_TZ_KIH, x, 20);
    s->npcs[0].current_hp = s->npcs[0].max_hp = 100000;
}

/* One step of a copy of s that attacks the NPC, arming the special in the
 * same step or not. Returns the Hitpoints (tenths) the NPC lost. */
static int special_shot(const FcState *s, int arm, FcState *after) {
    *after = *s;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_SPECIAL_ATTACK] = arm ? FC_SPECIAL_TOGGLE : FC_SPECIAL_NO_CHANGE;
    int hp = after->npcs[0].current_hp;
    fc_step(after, actions);
    assert(after->render_events.player_attack_fired);
    return hp - after->npcs[0].current_hp;
}

/* The energy: full at the start, the cost of a special, and RuneC's
 * recovery and its count. */
static void special_energy_test(void) {
    FcState s;
    special_fixture(&s, "acb_armadyl", 25);
    assert(s.caps.special_energy == FC_SPECIAL_ENERGY_MAX && !s.caps.special_armed &&
           s.caps.special_recover_ticks == 0);
    int actions[FC_NUM_ACTION_HEADS] = {0};
    fc_step(&s, actions);   /* full: the count stays 0 */
    assert(s.caps.special_energy == FC_SPECIAL_ENERGY_MAX && s.caps.special_recover_ticks == 0);
    /* Arm and attack in one tick: the special fires, spends 5000 and
     * disarms, and the recovery counts that tick, after the attack. */
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_TOGGLE;
    fc_step(&s, actions);
    assert(s.render_events.player_attack_fired);
    assert(s.caps.special_energy == 5000 && !s.caps.special_armed &&
           s.caps.special_recover_ticks == 1);
    actions[FC_HEAD_ATTACK] = FC_ATTACK_NONE;
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_NO_CHANGE;
    for (int i = 0; i < FC_SPECIAL_RECOVER_TICKS - 2; i++) fc_step(&s, actions);
    assert(s.caps.special_energy == 5000 &&
           s.caps.special_recover_ticks == FC_SPECIAL_RECOVER_TICKS - 1);
    fc_step(&s, actions);
    assert(s.caps.special_energy == 6000 && s.caps.special_recover_ticks == 0);
    /* Spending does not restart the count. */
    s.caps.special_energy = 7000;
    s.caps.special_recover_ticks = 20;
    s.player.attack_timer = 0;
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_TOGGLE;
    fc_step(&s, actions);
    assert(s.render_events.player_attack_fired);
    assert(s.caps.special_energy == 2000 && s.caps.special_recover_ticks == 21);
    /* Recovery stops at full, where the count restarts. */
    actions[FC_HEAD_ATTACK] = FC_ATTACK_NONE;
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_NO_CHANGE;
    s.caps.special_energy = 9500;
    s.caps.special_recover_ticks = FC_SPECIAL_RECOVER_TICKS - 1;
    fc_step(&s, actions);
    assert(s.caps.special_energy == FC_SPECIAL_ENERGY_MAX && s.caps.special_recover_ticks == 0);
    fc_step(&s, actions);
    assert(s.caps.special_energy == FC_SPECIAL_ENERGY_MAX && s.caps.special_recover_ticks == 0);
    /* Nothing recovers without Hitpoints, as RuneC's status tick skips a dead
     * player. */
    s.caps.special_energy = 5000;
    s.caps.special_recover_ticks = 10;
    s.player.current_hp = 0;
    fc_special_recover(&s);
    assert(s.caps.special_energy == 5000 && s.caps.special_recover_ticks == 10);
    puts("special attacks: full start, cost, recovery every 50 ticks and its count passed");
}

/* Which weapons have a special, arming and its mask, and the observation. */
static void special_mask_test(void) {
    /* Of the presets' weapons, only the Toxic blowpipe and Armadyl crossbow
     * have a g_player_specials row. */
    for (int i = 0; i < FC_GEAR_PRESET_COUNT; i++) {
        const FcItemDef *weapon = fc_gear_preset_weapon(&FC_GEAR_PRESETS[i]);
        const FcSpecialDef *def = fc_special_def(weapon->id);
        if (weapon->id == FC_ITEM_ID_TOXIC_BLOWPIPE)
            assert(def && def->cost == 5000 && def->damage_pct == 150);
        else if (weapon->id == 11785)
            assert(def && def->cost == 5000 && def->damage_pct == 100);
        else
            assert(def == NULL);
    }
    FcState s;
    float obs[FC_POLICY_OBS_SIZE];
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_TOGGLE;
    /* The Twisted bow has none: arming is masked and does nothing. */
    special_fixture(&s, "tbow_masori", 25);
    assert(!special_mask(&s) && fc_special_cost(&s.player) == 0);
    fc_write_obs(&s, obs);
    assert(obs[FC_OBS_SPECIAL_START + FC_OBS_SPECIAL_ENERGY] == 1.0f &&
           obs[FC_OBS_SPECIAL_START + FC_OBS_SPECIAL_ARMED] == 0.0f &&
           obs[FC_OBS_SPECIAL_START + FC_OBS_SPECIAL_COST] == 0.0f);
    fc_step(&s, actions);
    assert(!s.caps.special_armed);
    /* The Armadyl crossbow arms while the energy covers its 5000. */
    special_fixture(&s, "acb_armadyl", 25);
    assert(special_mask(&s) && fc_special_cost(&s.player) == 5000);
    s.caps.special_energy = 4999;
    assert(!special_mask(&s));
    fc_step(&s, actions);
    assert(!s.caps.special_armed);
    s.caps.special_energy = 5000;
    assert(special_mask(&s));
    fc_step(&s, actions);
    assert(s.caps.special_armed && s.caps.special_energy == 5000);
    fc_write_obs(&s, obs);
    assert(obs[FC_OBS_SPECIAL_START + FC_OBS_SPECIAL_ENERGY] == 0.5f &&
           obs[FC_OBS_SPECIAL_START + FC_OBS_SPECIAL_ARMED] == 1.0f &&
           obs[FC_OBS_SPECIAL_START + FC_OBS_SPECIAL_COST] == 0.5f);
    /* Disarming always takes effect. */
    s.caps.special_energy = 0;
    assert(special_mask(&s));
    fc_step(&s, actions);
    assert(!s.caps.special_armed);
    assert(!fc_special_action_valid(&s, FC_SPECIAL_ATTACK_DIM));
    puts("special attacks: weapons with a special, arming, its mask and the observation passed");
}

/* Each special against an ordinary attack from the same state: the same
 * rolls, the damage scaled by the special's percentage, the cost paid, the
 * special disarmed and one piece of ammunition spent either way. An armed
 * special the worn weapon does not have stops the attack. */
static void special_attack_test(void) {
    static const struct { const char *preset; int weapon, x, pct; } cases[] = {
        {"acb_armadyl", 11785, 25, 100},
        {"blowpipe_pure", FC_ITEM_ID_TOXIC_BLOWPIPE, 24, 150},
    };
    for (int c = 0; c < 2; c++) {
        FcState s, normal, special;
        special_fixture(&s, cases[c].preset, cases[c].x);
        assert(s.player.equipment[FC_EQUIP_SLOT_WEAPON].item_id == cases[c].weapon);
        int hits = 0, scaled = 0;
        for (int trial = 0; trial < 200; trial++) {
            s.rng_state = 0x9e3779b9u * (uint32_t)(trial + 1);
            int plain = special_shot(&s, 0, &normal);
            int dealt = special_shot(&s, 1, &special);
            assert(special.rng_state == normal.rng_state);
            assert(dealt == plain / 10 * cases[c].pct / 100 * 10);
            assert(normal.caps.special_energy == FC_SPECIAL_ENERGY_MAX &&
                   special.caps.special_energy == FC_SPECIAL_ENERGY_MAX - 5000 &&
                   !special.caps.special_armed);
            assert(special.player.ammo_count == s.player.ammo_count - 1 &&
                   normal.player.ammo_count == s.player.ammo_count - 1);
            assert(special.player.attack_timer == normal.player.attack_timer);
            hits += plain > 0;
            scaled += dealt != plain;
        }
        assert(hits > 0 && (cases[c].pct == 100 ? scaled == 0 : scaled > 0));
    }

    /* reject_launch: armed with the crossbow, then unarmed beside the NPC.
     * The punch does not launch, the special disarms, the target goes and
     * the energy stays; the next punch is an ordinary one. */
    FcState s;
    special_fixture(&s, "acb_armadyl", 21);
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_TOGGLE;
    fc_step(&s, actions);
    assert(s.caps.special_armed);
    assert(fc_unequip_item(&s, FC_EQUIP_SLOT_WEAPON) == FC_ITEM_OK &&
           s.player.weapon_kind == FC_WEAPON_UNARMED);
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_NO_CHANGE;
    actions[FC_HEAD_ATTACK] = 1;
    int hp = s.npcs[0].current_hp;
    fc_step(&s, actions);
    assert(!s.render_events.player_attack_fired && !s.caps.special_armed &&
           s.player.attack_target_idx == -1 && s.npcs[0].current_hp == hp &&
           s.caps.special_energy == FC_SPECIAL_ENERGY_MAX && s.player.attack_timer == 0);
    fc_step(&s, actions);
    assert(s.render_events.player_attack_fired &&
           s.caps.special_energy == FC_SPECIAL_ENERGY_MAX);
    puts("special attacks: Armadyl crossbow and Toxic blowpipe specials against the "
         "same attacks, and a special the weapon lacks, passed");
}

/* An acb_armadyl episode that arms the special whenever it can while
 * attacking the nearest NPC, deterministically. */
static void special_episode_test(void) {
    FcScenario scenario = preset_scenario(fc_gear_preset_find("acb_armadyl"));
    FcState s, again;
    fc_init(&s);
    fc_reset(&s, 7);
    fc_apply_scenario(&s, &scenario);
    again = s;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[FC_HEAD_ATTACK] = 1;
    int specials = 0, attacks = 0, ticks = 0;
    for (; ticks < 600 && !fc_is_terminal(&s); ticks++) {
        actions[FC_HEAD_SPECIAL_ATTACK] = !s.caps.special_armed && special_mask(&s)
            ? FC_SPECIAL_TOGGLE : FC_SPECIAL_NO_CHANGE;
        int energy = s.caps.special_energy;
        fc_step(&s, actions);
        fc_step(&again, actions);
        assert(fc_state_hash(&s) == fc_state_hash(&again));
        assert(s.caps.special_energy >= 0 && s.caps.special_energy <= FC_SPECIAL_ENERGY_MAX);
        specials += s.caps.special_energy < energy;
        attacks += s.render_events.player_attack_fired;
    }
    assert(specials >= 2 && attacks > specials);
    printf("special attacks: %d specials in %d attacks over %d ticks with acb_armadyl passed\n",
           specials, attacks, ticks);
}

#if FC_GEAR_SWITCHING
/* Switching and the special in one tick, as RuneC orders them: the switch
 * and the toggle are input before the attack, whose launch checks the
 * weapon worn then. GEAR_LINEUP's sets: tbow_masori worn, acb_armadyl, then
 * bowfa_crystal and blowpipe_pure as far as gear_sets goes. */
static void special_gear_switch_test(void) {
    FcScenario scenario = gear_scenario(0, 0);
    FcState s, normal, special;
    gear_fixture(&s, &scenario);
    memset(s.npcs, 0, sizeof(s.npcs));
    memset(s.walkable, 1, sizeof(s.walkable));
    memset(s.movement_flags, 0, sizeof(s.movement_flags));
    memset(s.los_flags, 0, sizeof(s.los_flags));
    s.player.x = 20;
    s.player.y = 20;
    fc_npc_spawn(&s.npcs[0], NPC_TZ_KIH, 24, 20, 1);
    s.npcs_remaining = 1;
    s.npcs[0].attack_timer = 100000;
    s.npcs[0].current_hp = s.npcs[0].max_hp = 100000;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    /* The Twisted bow worn: arming is masked. Switch to the crossbow, then
     * arm and attack: its special fires. */
    assert(!special_mask(&s));
    actions[FC_HEAD_GEAR_SET] = 2;
    fc_step(&s, actions);
    assert(wears_preset(&s.player, "acb_armadyl") && special_mask(&s));
    actions[FC_HEAD_GEAR_SET] = FC_GEAR_SET_NO_CHANGE;
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_TOGGLE;
    s.player.attack_timer = 0;
    fc_step(&s, actions);
    assert(s.render_events.player_attack_fired && !s.caps.special_armed &&
           s.caps.special_energy == 5000);

    /* Armed with the crossbow, then switching to the bow while attacking:
     * the bow has no special, so the attack stops (reject_launch). */
    actions[FC_HEAD_ATTACK] = FC_ATTACK_NONE;
    fc_step(&s, actions);
    assert(s.caps.special_armed);
    actions[FC_HEAD_GEAR_SET] = 1;
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_NO_CHANGE;
    s.player.attack_timer = 0;
    fc_step(&s, actions);
    assert(wears_preset(&s.player, "tbow_masori") && !s.render_events.player_attack_fired &&
           !s.caps.special_armed && s.player.attack_target_idx == -1 &&
           s.caps.special_energy == 5000);

    /* Arming in the tick that switches from the crossbow to the bow arms
     * nothing, the toggle applying after the switch; the bow shoots as
     * usual. */
    s.caps.special_energy = FC_SPECIAL_ENERGY_MAX;
    actions[FC_HEAD_GEAR_SET] = 2;
    actions[FC_HEAD_ATTACK] = FC_ATTACK_NONE;
    fc_step(&s, actions);
    assert(wears_preset(&s.player, "acb_armadyl") && special_mask(&s));
    actions[FC_HEAD_GEAR_SET] = 1;
    actions[FC_HEAD_ATTACK] = 1;
    actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_TOGGLE;
    s.player.attack_timer = 0;
    fc_step(&s, actions);
    assert(wears_preset(&s.player, "tbow_masori") && s.render_events.player_attack_fired &&
           !s.caps.special_armed && s.caps.special_energy == FC_SPECIAL_ENERGY_MAX);

    /* From the crossbow to the blowpipe with the toggle and an attack in one
     * tick: the blowpipe's special arms and fires, against the same tick
     * without the toggle. */
    if (FC_CAP_GEAR_SETS >= 4) {
        actions[FC_HEAD_GEAR_SET] = 2;
        actions[FC_HEAD_ATTACK] = FC_ATTACK_NONE;
        actions[FC_HEAD_SPECIAL_ATTACK] = FC_SPECIAL_NO_CHANGE;
        fc_step(&s, actions);
        assert(wears_preset(&s.player, "acb_armadyl") && special_mask(&s));
        s.player.attack_timer = 0;
        int hits = 0, scaled = 0;
        for (int trial = 0; trial < 200; trial++) {
            s.rng_state = 0x9e3779b9u * (uint32_t)(trial + 1);
            int dealt[2];
            for (int arm = 0; arm < 2; arm++) {
                FcState *after = arm ? &special : &normal;
                *after = s;
                actions[FC_HEAD_GEAR_SET] = 4;
                actions[FC_HEAD_ATTACK] = 1;
                actions[FC_HEAD_SPECIAL_ATTACK] = arm ? FC_SPECIAL_TOGGLE
                                                      : FC_SPECIAL_NO_CHANGE;
                int hp = after->npcs[0].current_hp;
                fc_step(after, actions);
                assert(wears_preset(&after->player, "blowpipe_pure") &&
                       after->render_events.player_attack_fired);
                dealt[arm] = hp - after->npcs[0].current_hp;
            }
            assert(special.rng_state == normal.rng_state &&
                   dealt[1] == dealt[0] / 10 * 150 / 100 * 10 &&
                   !special.caps.special_armed &&
                   special.caps.special_energy == FC_SPECIAL_ENERGY_MAX - 5000 &&
                   normal.caps.special_energy == FC_SPECIAL_ENERGY_MAX);
            hits += dealt[0] > 0;
            scaled += dealt[1] != dealt[0];
        }
        assert(hits > 0 && scaled > 0);
    }
    puts("special attacks: switching then arming, a switch away from an armed special, "
         "arming while switching and switching to another special passed");
}
#endif

/* The special attack head reaches the core through the policy actions; its
 * mask and the special block are observed. */
static void special_adapter_test(void) {
    const int head = FC_PUFFER_NUM_ATNS - 1;
    const int mask = FC_PUFFER_MASK_SIZE - FC_SPECIAL_ATTACK_DIM;
    assert(FC_PUFFER_ACTION_HEADS[head] == FC_HEAD_SPECIAL_ATTACK &&
           strcmp(FC_HEAD_NAMES[FC_HEAD_SPECIAL_ATTACK], "special_attack") == 0 &&
           FC_PUFFER_ACTION_DIMS[head] == FC_SPECIAL_ATTACK_DIM);
    Dict kwargs = {0};
    puf_ini_set(&kwargs, "gear_set_1", "acb_armadyl");
    other_gear_sets(&kwargs, "acb_armadyl");
    FightCaves* env = test_env(&kwargs);
    Agent* a = &env->agents[0];
    const float* obs = a->observations + FC_OBS_SPECIAL_START;
    assert(obs[FC_OBS_SPECIAL_ENERGY] == 1.0f && obs[FC_OBS_SPECIAL_ARMED] == 0.0f &&
           obs[FC_OBS_SPECIAL_COST] == 0.5f);
    assert(a->action_mask[mask + FC_SPECIAL_NO_CHANGE] == 1 &&
           a->action_mask[mask + FC_SPECIAL_TOGGLE] == 1);
    a->actions[head] = FC_SPECIAL_TOGGLE;
    checked_step(env);
    assert(env->state.caps.special_armed && obs[FC_OBS_SPECIAL_ARMED] == 1.0f &&
           a->action_mask[mask + FC_SPECIAL_TOGGLE] == 1);
    checked_step(env);
    assert(!env->state.caps.special_armed && obs[FC_OBS_SPECIAL_ARMED] == 0.0f);
    /* Short of the crossbow's 5000, arming is masked. */
    env->state.caps.special_energy = 4000;
    fc_puffer_write_obs(env);
    check_obs(env);
    assert(obs[FC_OBS_SPECIAL_ENERGY] == 0.4f &&
           a->action_mask[mask + FC_SPECIAL_TOGGLE] == 0);
    a->actions[head] = FC_SPECIAL_NO_CHANGE;
    checked_step(env);
    test_free(env);
    dict_clear(&kwargs);
    puts("special attacks: head, masks, energy, armed flag and cost observed passed");
}
#endif

/* README.md's presets table has one row per preset, with levels that wear it
 * and, for the magic and melee presets, those of PRESET_LEVELS. Columns: the
 * preset, its main gear, then the levels in FcSkill order. */
static void readme_presets_test(void) {
    FILE *f = fopen("ocean/fight_caves/README.md", "r");
    assert(f);
    int rows[FC_GEAR_PRESET_COUNT] = {0};
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        const char *row = line + strspn(line, " ");
        char name[64];
        if (sscanf(row, "| `%63[^`]`", name) != 1 || strcmp(name, "gear_set_1") == 0)
            continue;
        int preset = fc_gear_preset_find(name);
        assert(preset >= 0);
        rows[preset]++;
        int levels[FC_SKILL_COUNT], cells = 0;
        const char *cell = row;
        for (int c = 0; (cell = strchr(cell, '|')) != NULL; c++, cell++) {
            if (c < 2 || c >= 2 + FC_SKILL_COUNT) continue;
            int parsed = sscanf(cell + 1, "%d", &levels[c - 2]);
            assert(parsed == 1);
            cells++;
        }
        assert(cells == FC_SKILL_COUNT);
        FcScenario scenario = preset_scenario(preset);
        memcpy(scenario.levels, levels, sizeof(levels));
        char err[256];
        assert(fc_scenario_check(&scenario, err, sizeof(err)) == 0);
        for (int i = 0; i < PRESET_LEVEL_COUNT; i++)
            if (strcmp(PRESET_LEVELS[i].name, name) == 0)
                assert(memcmp(PRESET_LEVELS[i].levels, levels, sizeof(levels)) == 0);
    }
    fclose(f);
    for (int p = 0; p < FC_GEAR_PRESET_COUNT; p++) assert(rows[p] == 1);
    printf("README presets table: %d presets with the levels they were made for passed\n",
           FC_GEAR_PRESET_COUNT);
}

#if FC_CAP_CONSUMABLES
/* The eat and drink heads reach the core through the policy actions, and
 * their masks and cooldowns are observed. */
static void consumables_test(void) {
    enum { EAT = 3, DRINK = 4 };
    const int eat_mask = FC_MOVE_DIM + FC_ATTACK_DIM + FC_PRAYER_DIM;
    assert(FC_PUFFER_ACTION_HEADS[EAT] == FC_HEAD_EAT &&
           FC_PUFFER_ACTION_HEADS[DRINK] == FC_HEAD_DRINK);
    Dict kwargs = {0};
    other_gear_sets(&kwargs, FC_GEAR_PRESETS[0].name);
    dict_set(&kwargs, "initial_sharks", 2);
    dict_set(&kwargs, "initial_prayer_doses", 4);
    FightCaves* env = test_env(&kwargs);
    FcPlayer* p = &env->state.player;
    Agent* a = &env->agents[0];
    assert(a->action_mask[eat_mask + FC_EAT_SHARK] == 0);  /* full hitpoints */
    p->current_hp -= 300;
    p->current_prayer -= 300;
    fc_puffer_write_obs(env);
    check_obs(env);
    assert(a->action_mask[eat_mask + FC_EAT_SHARK] == 1);
    assert(a->action_mask[eat_mask + FC_EAT_DIM + FC_DRINK_PRAYER_POT] == 1);
    a->actions[EAT] = FC_EAT_SHARK;
    a->actions[DRINK] = FC_DRINK_PRAYER_POT;
    checked_step(env);
    assert(p->sharks_remaining == 1 && p->prayer_doses_remaining == 3);
    const float* supply = a->observations + FC_OBS_SUPPLY_START;
    assert(supply[FC_OBS_SUPPLY_FOOD_TIMER] > 0 && supply[FC_OBS_SUPPLY_POTION_TIMER] > 0);
    assert(a->action_mask[eat_mask + FC_EAT_SHARK] == 0);  /* food delay */
    test_free(env);
    dict_clear(&kwargs);
    puts("consumables: eat and drink heads, masks and cooldowns passed");
}
#endif

static int adapter_test(int graphical) {
    native_config_test();
    init_and_config_test();
    scenario_config_test();
#if FC_CAP_CONSUMABLES
    consumables_test();
#endif
#if FC_CAP_PRAYERS
    prayer_book_adapter_test();
#endif
#if FC_GEAR_SWITCHING
    gear_adapter_test();
#endif
#if FC_SPELLBOOK_HEAD
    spellbook_adapter_test();
#endif
#if FC_CAP_SPECIAL_ATTACKS
    special_adapter_test();
#endif
    magic_config_test();
    melee_config_test();
    terminal_and_log_test();
    /* The shipped INI with this build's capabilities, as on the command line.
     * Puffer's overrides replace only keys the INI has, so gear sets past the
     * first, which a gear_sets build needs, go into the section; the trace
     * never switches. */
    char consumables[] = "--env.consumables=1", prayers[] = "--env.prayers=all";
    char special_attacks[] = "--env.special_attacks=1";
    char gear_sets[32], spellbook[32];
    snprintf(gear_sets, sizeof(gear_sets), "--env.gear_sets=%d", FC_CAP_GEAR_SETS);
    snprintf(spellbook, sizeof(spellbook), "--env.spellbook=%s",
             FC_CAP_SPELLBOOK == 2 ? "ancient" : "standard");
    char* overrides[5] = {0};
    int override_count = 0;
    if (FC_CAP_CONSUMABLES) overrides[override_count++] = consumables;
    if (FC_CAP_PRAYERS) overrides[override_count++] = prayers;
    if (FC_GEAR_SWITCHING) overrides[override_count++] = gear_sets;
    if (FC_SPELLBOOK_HEAD) overrides[override_count++] = spellbook;
    if (FC_CAP_SPECIAL_ATTACKS) overrides[override_count++] = special_attacks;
    Ini ini = {0};
    puf_ini_load_env(&ini, "fight_caves", override_count, overrides);
    Dict* kwargs = puf_ini_section(&ini, "env", 0);
    static const char* const other_sets[FC_MAX_GEAR_SETS] =
        {NULL, "acb_armadyl", "bowfa_crystal", "blowpipe_pure"};
    for (int k = 1; k < FC_CAP_GEAR_SETS; k++) {
        char key[32];
        snprintf(key, sizeof(key), "gear_set_%d", k + 1);
        puf_ini_set(kwargs, key, other_sets[k]);
    }
    FightCaves* env = test_env(kwargs);
    FightCaves* rendered = test_env(kwargs);
    assert(env->reward_params.w_progress == (float)dict_get(kwargs, "w_progress"));
    puf_ini_free(&ini);
    uint32_t trace = 2166136261u;
    int episodes = 0;
    if (graphical) {
        puf_render(rendered);
        /* Batch regression checks must finish even if the test window is
         * minimized while another application is in use. */
        SetWindowState(FLAG_WINDOW_ALWAYS_RUN);
        replay_controls_test(rendered);
        rendered->viewer->tps = 60.0f;
    } else {
        rendered->viewer = calloc(1, sizeof(*rendered->viewer));
        assert(rendered->viewer);
    }
    Agent* a = &env->agents[0];
    Agent* b = &rendered->agents[0];
    for (int tick = 0; tick < 2048; tick++) {
        a->actions[0] = tick % 17;
        a->actions[1] = (tick / 3) % 9;
        a->actions[2] = (tick / 7) % 8;
        memcpy(b->actions, a->actions, sizeof(float) * NUM_ATNS);
        checked_step(env);
        checked_step(rendered);
        uint32_t hash = fc_state_hash(&env->state);
        trace = digest(trace, &hash, sizeof(hash));
        trace = digest(trace, a->observations, sizeof(obs_t) * OBS_SIZE);
        trace = digest(trace, a->action_mask, FC_PUFFER_MASK_SIZE);
        trace = digest(trace, a->rewards, sizeof(float));
        trace = digest(trace, a->terminals, sizeof(float));
        episodes += a->terminals[0] != 0;
        assert(hash == fc_state_hash(&rendered->state));
        assert(memcmp(a->observations, b->observations, sizeof(obs_t) * OBS_SIZE) == 0);
        assert(memcmp(a->action_mask, b->action_mask, FC_PUFFER_MASK_SIZE) == 0);
        assert(a->rewards[0] == b->rewards[0] && a->terminals[0] == b->terminals[0]);
        if (graphical) {
            puf_render(rendered);
            assert(hash == fc_state_hash(&rendered->state));
            assert(!rendered->viewer->pending_frame);
            if (a->terminals[0]) assert(fc_is_terminal(&rendered->viewer->state));
        }
    }
    assert(episodes > 0);
    puf_reset(env);
    puf_reset(rendered);
    if (graphical) {
        puf_render(rendered);
        assert(fc_state_hash(&rendered->viewer->state) == fc_state_hash(&env->state));
    }
    printf("adapter trace: steps=2048 episodes=%d digest=%08x\n", episodes, trace);
    /* Pinned like the core contract: a change here changes what training
     * sees. Other capabilities observe more, so only the default is pinned. */
    assert(episodes == 2);
#if FC_DEFAULT_CAPABILITIES
    assert(trace == 0xe9b2fd18u);
#endif
    if (!graphical) {
        free(rendered->viewer);
        rendered->viewer = NULL;
    }
    test_free(rendered);
    test_free(env);
    return 0;
}

/* Native CPU policy plumbing used by CPU replay, independent of CUDA and graphics. */
static void cpu_policy_test(void) {
#if FC_DEFAULT_CAPABILITIES
    assert(fc_policy_weight_count(512, 3) == 2541056);
#endif
    assert(fc_policy_weight_count(0, 3) == 0);
    assert(fc_policy_weight_count(15, 3) == 0);
    assert(fc_policy_weight_count(512, 0) == 0);
    assert(fc_policy_weight_count(INT_MAX, INT_MAX) == 0);
    int count = fc_policy_weight_count(8, 2);
    Weights* weights = calloc(1, sizeof(*weights) + (count + 7) * sizeof(float));
    assert(weights);
    weights->data = (float*)(weights + 1);
    weights->size = count + 7;
    int dims[] = ACT_SIZES;
    PufferNet* carried = make_puffernet(weights, 1, OBS_SIZE, 8, 2, dims, NUM_ATNS);
    weights->idx = 0;
    PufferNet* fresh = make_puffernet(weights, 1, OBS_SIZE, 8, 2, dims, NUM_ATNS);
    float obs[OBS_SIZE] = {0}, actions[NUM_ATNS], other[NUM_ATNS];
    unsigned char mask[FC_PUFFER_MASK_SIZE] = {0};
    /* One legal action per head: 2, 3 and 4 for move, attack and prayer, and
     * the last action of any capability head. */
    mask[2] = mask[17 + 3] = mask[17 + 9 + 4] = 1;
    for (int h = 3, start = 17 + 9 + 8; h < NUM_ATNS; start += dims[h++])
        mask[start + dims[h] - 1] = 1;
    float terminal = 0;
    forward_puffernet(carried, obs, actions, mask, &terminal);
    float once[16];
    memcpy(once, carried->mingru->state, sizeof(once));
    forward_puffernet(carried, obs, actions, mask, &terminal);
    assert(memcmp(once, carried->mingru->state, sizeof(once)) != 0);
    assert(actions[0] == 2 && actions[1] == 3 && actions[2] == 4);
    for (int h = 3; h < NUM_ATNS; h++) assert(actions[h] == dims[h] - 1);
    terminal = 1;
    forward_puffernet(carried, obs, actions, mask, &terminal);
    terminal = 0;
    forward_puffernet(fresh, obs, other, mask, &terminal);
    assert(memcmp(carried->mingru->state, fresh->mingru->state, sizeof(once)) == 0);
    assert(memcmp(actions, other, sizeof(actions)) == 0);
    free_puffernet(carried);
    free_puffernet(fresh);
    free(weights);
    puts("CPU policy: native layout, masks, carry and terminal resets passed");
}

int main(int argc, char** argv) {
    int graphical = argc == 2 && strcmp(argv[1], "--render") == 0;
    if (argc > 1 && !graphical) {
        fprintf(stderr, "Usage: %s [--render]\n", argv[0]);
        return 2;
    }
    wave_rotation_test();
    cpu_policy_test();
#if FC_CAP_PRAYERS
    prayer_book_test();
#endif
#if FC_GEAR_SWITCHING
    gear_switch_test();
    gear_episode_test();
    magic_gear_switch_test();
    melee_gear_switch_test();
#endif
    magic_definitions_test();
    magic_formula_test();
    magic_rune_test();
    magic_autocast_test();
    magic_effects_test();
    magic_preset_episodes();
    melee_formula_test();
    melee_attack_test();
    melee_preset_episodes();
#if FC_CAP_SPECIAL_ATTACKS
    special_energy_test();
    special_mask_test();
    special_attack_test();
    special_episode_test();
#if FC_GEAR_SWITCHING
    special_gear_switch_test();
#endif
#endif
    readme_presets_test();
    if (core_contract_test() || equipment_test() || episode_analytics_test() ||
        context_menu_test() || click_feedback_test() || model_picking_test() ||
        adapter_test(graphical)) return 1;
    return graphical ? equipment_appearance_test() : 0;
}
