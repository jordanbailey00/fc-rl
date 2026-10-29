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
    /* 286 + 34 = 320 floats and three heads with the default capabilities. */
    _Static_assert(FC_POLICY_OBS_SIZE == 286 + 3 * FC_CAP_CONSUMABLES,
                   "policy observation contract drifted");
    _Static_assert(FC_PUFFER_MASK_SIZE == 34 + 5 * FC_CAP_CONSUMABLES,
                   "Puffer mask contract drifted");
    _Static_assert(FC_PUFFER_OBS_SIZE == FC_POLICY_OBS_SIZE + FC_PUFFER_MASK_SIZE,
                   "Puffer observation contract drifted");
    _Static_assert(FC_PUFFER_NUM_ATNS == 3 + 2 * FC_CAP_CONSUMABLES,
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

    if (steps != 483 || fc_state_hash(&first) != 0x5c53c26du)
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
    }
    return 0;
}

/* The gear presets, default levels and NPC tables are compile-time constants,
 * checked here rather than on every fc_init. The viewer has each preset's
 * combat tab and its weapon's animations. */
static int constant_tables(void) {
    char err[256];
    FcScenario scenario = fc_default_scenario();
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
        CHECK(fc_player_visual_profile(&player)->projectile.travel_spot != 0);
        for (int type = NPC_TZ_KIH; type < NPC_TYPE_COUNT; type++) {
            FcNpc target = {.npc_type = type};
            int attack_roll, max_hit;
            fc_player_ranged_rolls(&player, &target, &attack_roll, &max_hit);
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
    FcScenario scenario = fc_default_scenario();
    scenario.gear_set = fc_gear_preset_find("blowpipe_pure");
    CHECK(scenario.gear_set >= 0);
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

/* Each preset is wearable at exactly its items' Ranged, Defence and Hitpoints
 * requirements and not one level below any of them. At the default levels it
 * fights: a few hundred ticks of attacking the nearest NPC land hits, and each
 * shot spends one arrow, bolt or loaded dart. */
static int gear_presets(void) {
    const int skills[] = {FC_SKILL_RANGED, FC_SKILL_DEFENCE, FC_SKILL_HITPOINTS};
    char err[256], prefix[64], expected[96];
    for (int p = 0; p < FC_GEAR_PRESET_COUNT; p++) {
        const FcGearPreset *preset = &FC_GEAR_PRESETS[p];
        FcScenario scenario = fc_default_scenario();
        scenario.gear_set = p;
        FcScenario minimum = scenario;
        for (int k = 0; k < 3; k++) minimum.levels[skills[k]] = 1;
        for (int i = 0; i < preset->equipment_count; i++) {
            const FcItemDef *item = fc_item_definition(preset->equipment[i]);
            const int needs[] = {item->ranged_level, item->defence_level,
                                 item->hitpoints_level};
            for (int k = 0; k < 3; k++)
                if (needs[k] > minimum.levels[skills[k]])
                    minimum.levels[skills[k]] = needs[k];
        }
        CHECK(fc_scenario_check(&minimum, err, sizeof(err)) == 0);
        snprintf(prefix, sizeof(prefix), "gear_set_1 = %s: ", preset->name);
        for (int k = 0; k < 3; k++) {
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
        CHECK(s.player.ammo_count == left);
        CHECK(left == (s.player.weapon_uses_ammo ? preset->ammo - attacks : 0));
        printf("gear preset %s: wearable from %d ranged, %d defence, %d hitpoints; "
               "%d attacks and %d hits in %d ticks\n", preset->name,
               minimum.levels[FC_SKILL_RANGED], minimum.levels[FC_SKILL_DEFENCE],
               minimum.levels[FC_SKILL_HITPOINTS], attacks, hits, ticks);
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

/* Every gear preset composes from its equipment, and its weapon's animations
 * and projectile spot animations ship. With FC_TEST_OUTPUT_DIR set, each is
 * drawn to <preset>.png there: idle from the front and side, then two frames
 * of its attack. */
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
        FcScenario scenario = fc_default_scenario();
        scenario.gear_set = p;
        fc_reset(&state, 101);
        fc_apply_scenario(&state, &scenario);
        if (fc_player_appearance_sync(appearance, &state.player, FC_PLAYER_MODEL_ID) < 0)
            return 1;
        ModelEntry *entry = appearance->model->entries;
        const FcPlayerVisualProfile *profile = fc_player_visual_profile(&state.player);
        const uint16_t sequences[] = {
            profile->idle_anim, profile->walk_anim, profile->walk_back_anim,
            profile->walk_left_anim, profile->walk_right_anim, profile->turn_anim,
            profile->run_anim, profile->attack_anim,
        };
        for (int i = 0; i < (int)(sizeof(sequences) / sizeof(sequences[0])); i++)
            if (!anim_get_sequence(cache, sequences[i])) return 1;
        const FcProjectileProfile *shot = &profile->projectile;
        const uint32_t spot_ids[] = {shot->travel_spot, shot->launch_spot, shot->impact_spot};
        if (!shot->travel_spot) return 1;
        for (int i = 0; i < 3; i++)
            if (spot_ids[i] && (!spotanim_find(spots, (int)spot_ids[i]) ||
                !model_find(projectiles, PROJ_SPOTANIM_MODEL_BASE + spot_ids[i])))
                return 1;
        if (dir) {
            int frames = anim_get_sequence(cache, profile->attack_anim)->frame_count;
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
    Dict empty = {0};
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

    Dict overrides = {0};
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
    dict_set(&kwargs, "consumables", FC_CAP_CONSUMABLES);

    puf_ini_set(&kwargs, "gear_set_1", "dragon_claws");
    expect_config_error(&kwargs, "gear_set_1 = dragon_claws: no such gear preset; "
                        "the presets are tbow_masori rcb_black_dhide rcb_pure "
                        "msb_pure blowpipe_pure acb_armadyl bowfa_crystal "
                        "tbow_masori_no_ring");
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

#if FC_CAP_CONSUMABLES
/* The eat and drink heads reach the core through the policy actions, and
 * their masks and cooldowns are observed. */
static void consumables_test(void) {
    enum { EAT = 3, DRINK = 4 };
    const int eat_mask = FC_MOVE_DIM + FC_ATTACK_DIM + FC_PRAYER_DIM;
    assert(FC_PUFFER_ACTION_HEADS[EAT] == FC_HEAD_EAT &&
           FC_PUFFER_ACTION_HEADS[DRINK] == FC_HEAD_DRINK);
    Dict kwargs = {0};
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
    terminal_and_log_test();
    /* The shipped INI with this build's capabilities, as on the command line. */
    char consumables[] = "--env.consumables=1";
    char* overrides[] = {consumables};
    Ini ini = {0};
    puf_ini_load_env(&ini, "fight_caves", FC_CAP_CONSUMABLES ? 1 : 0, overrides);
    Dict* kwargs = puf_ini_section(&ini, "env", 0);
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
    if (core_contract_test() || equipment_test() || episode_analytics_test() ||
        context_menu_test() || click_feedback_test() || model_picking_test() ||
        adapter_test(graphical)) return 1;
    return graphical ? equipment_appearance_test() : 0;
}
