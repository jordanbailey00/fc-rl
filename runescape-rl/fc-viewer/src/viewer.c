/*
 * viewer.c — Fight Caves playable debug viewer.
 *
 * Phase 8: Human-playable Fight Caves with all backend systems connected.
 *
 * Controls:
 *   WASD        — move (N/W/S/E)            Space    — pause/resume
 *   1/2/3       — protect melee/range/magic  Right    — single-step tick
 *   F           — eat shark                  Clan tab — TPS buttons
 *   P           — drink prayer potion        R        — reset episode
 *   Tab         — cycle attack target        A        — toggle auto/manual
 *   O or D*     — toggle debug overlay       G        — grid  C — collision
 *   4/5         — camera presets             L        — toggle camera lock
 *   Scroll      — zoom                       Right-drag — orbit camera
 *
 *   * D only toggles the overlay when not being used for east movement.
 *   Policy replay mode (`--policy-pipe`) also adds 1/2/4/0 playback presets.
 *   In replay mode, use Shift+4 / 5 for camera presets.
 */

#include "raylib.h"
#include "rlgl.h"
#include "fc_types.h"
#include "fc_contracts.h"
#include "fc_api.h"
#include "fc_npc.h"
#include "fc_combat.h"
#include "fc_pathfinding.h"
#include "fc_reward.h"
#include "fc_wave.h"
#include "fc_terrain_loader.h"
#include "fc_objects_loader.h"
#include "fc_npc_models.h"
#include "fc_anim_loader.h"
#include "fc_spotanims.h"
#include "fc_asset_raylib.h"
#include "fc_debug_overlay.h"
#include "ui.h"
#include "ui_reference.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TILE_SIZE       16
#define PANEL_WIDTH     220
#define HEADER_HEIGHT   40
#define WINDOW_W        (FC_ARENA_WIDTH * TILE_SIZE + PANEL_WIDTH)
#define WINDOW_H        (FC_ARENA_HEIGHT * TILE_SIZE + HEADER_HEIGHT)
#define MAX_TPS         60.0f
#define MIN_TPS         0.25f
#define HALF_TPS        0.50f
#define NORMAL_TPS      (5.0f / 3.0f)
#define MAX_HITSPLATS   32
#define POLICY_REPLAY_BASE_TPS 2

/* Player animation sequence IDs (from osrs-dumps seq.sym) */
#define PLAYER_ANIM_IDLE   4591  /* xbows_human_ready */
#define PLAYER_ANIM_WALK   4226  /* xbows_human_walk_f */
#define PLAYER_ANIM_RUN    4228  /* xbows_human_run */
#define PLAYER_ANIM_ATTACK 4230  /* xbows_human_fire_and_reload */
#define PLAYER_ANIM_EAT    829   /* human_eat */
#define PLAYER_ANIM_DEATH  836   /* human_death */

/* NPC animation sequence IDs (from osrs-dumps seq.sym) */
/* NPC animation IDs — Jad uses lordmagmus anims (same model as FC Jad in cache) */
static const uint16_t NPC_ANIM_IDLE[] = {
    0, 2618, 2624, 2624, 2631, 2636, 2642, 2650, 2636  /* indexed by NPC type 0-8 */
};
static const uint16_t NPC_ANIM_WALK[] = {
    0, 2619, 2623, 2623, 2632, 2634, 2643, 2651, 2634
};
static const uint16_t NPC_ANIM_ATTACK[] = {
    0, 2621, 2625, 2625, 2628, 2637, 2644, 2655, 2637
};
static const uint16_t NPC_ANIM_DEATH[] = {
    0, 2620, 2627, 2627, 2630, 2638, 2646, 2654, 2638
};
#define JAD_ANIM_RANGED 2652
#define JAD_ANIM_MELEE  2655
#define JAD_ANIM_MAGIC  2656

/* Projectile spotanim IDs for model lookup */
#define PROJ_CROSSBOW_BOLT      27
#define PROJ_JAD_MAGIC_LAUNCH   439
#define PROJ_JAD_RANGE_LAUNCH   440
#define PROJ_TOK_XIL_SPINE      443
#define PROJ_TOK_XIL_IMPACT     444
#define PROJ_KET_ZEK_FIRE       445
#define PROJ_KET_ZEK_IMPACT     446
#define PROJ_JAD_MAGIC_TRAVEL   445
#define PROJ_JAD_MAGIC_IMPACT   446
#define PROJ_JAD_RANGED_TRAVEL  448
#define PROJ_JAD_RANGED_IMPACT  451
#define PROJ_SPOTANIM_MODEL_BASE 0xA2000000u

#define FC_UI_ITEM_VIAL          229u
#define FC_UI_ITEM_SHARK         385u
#define FC_UI_ITEM_PRAYER_POT_3  139u
#define FC_UI_ITEM_PRAYER_POT_2  141u
#define FC_UI_ITEM_PRAYER_POT_1  143u
#define FC_UI_ITEM_PRAYER_POT_4  2434u

/* Colors */
#define COL_BG          CLITERAL(Color){ 80, 80, 85, 255 }
#define COL_HEADER      CLITERAL(Color){ 30, 30, 40, 255 }
#define COL_PANEL       CLITERAL(Color){ 62, 53, 41, 255 }
#define COL_PANEL_BORDER CLITERAL(Color){ 42, 36, 28, 255 }
#define COL_TEXT_YELLOW CLITERAL(Color){ 255, 255,  0, 255 }
#define COL_TEXT_WHITE  CLITERAL(Color){ 255, 255, 255, 255 }
#define COL_TEXT_SHADOW CLITERAL(Color){   0,   0,   0, 255 }
#define COL_TEXT_DIM    CLITERAL(Color){ 130, 130, 140, 255 }
#define COL_TEXT_GREEN  CLITERAL(Color){ 100, 255, 100, 255 }
#define COL_HP_GREEN    CLITERAL(Color){  30, 255,  30, 255 }
#define COL_HP_RED      CLITERAL(Color){ 120,   0,   0, 255 }
#define COL_PRAY_BLUE   CLITERAL(Color){  50, 120, 210, 255 }
#define COL_PLAYER      CLITERAL(Color){  80, 140, 255, 255 }
#define COL_GRID        CLITERAL(Color){  30,  30,  30, 80  }
#define COL_BLOCKED     CLITERAL(Color){ 180,  30,  30, 60  }
#define COL_WALKABLE    CLITERAL(Color){  30, 120,  30, 30  }
#define COL_TARGET      CLITERAL(Color){ 255, 255,   0, 120 }
#define COL_HIT_RED     CLITERAL(Color){ 255,  50,  50, 255 }
#define COL_HIT_BLUE    CLITERAL(Color){  50, 100, 255, 255 }

#define FC_REWARD_CONFIG_PATH_MAX 256
#define FC_WORLD_ORIGIN_X 2368
#define FC_WORLD_ORIGIN_Y 5056
#define VIEWER_RUNEC_UI_COMPONENT_ID(group_id, file_id) \
    ((((uint32_t)(group_id)) << 16) | ((uint32_t)(file_id) & 0xffffu))
#define VIEWER_RUNEC_TOP_SIDE_CONTAINER \
    VIEWER_RUNEC_UI_COMPONENT_ID(161u, 73u)

typedef enum {
    HITSPLAT_DAMAGE = 0,
    HITSPLAT_HEAL = 1,
    HITSPLAT_PRAYER_DRAIN = 2,
} HitsplatKind;

/* Hitsplat/status splat rendered as a 2D overlay. */
typedef struct {
    int active;
    float world_x, world_y, world_z;  /* 3D position (entity center) */
    int damage;           /* damage in tenths (0 = miss) */
    int kind;             /* HitsplatKind */
    int frames_left;      /* lifetime in render frames */
} Hitsplat;

/* Visual projectile — travels from source to destination over tick duration */
#define MAX_PROJECTILES 16
typedef struct {
    int active;
    float src_x, src_y, src_z;
    float dst_x, dst_y, dst_z;
    float total_time;
    float elapsed;
    Color color;
    float radius;
    uint32_t spot_id;            /* travel spotanim ID (0 = no travel visual) */
    uint32_t launch_spot_id;
    uint32_t impact_spot_id;
    int profiled;
    float client_start_time;
    float client_end_time;
    float projectile_angle;
    float projectile_progress;
    AnimModelState* anim_state;
    uint16_t anim_seq;
    int anim_frame;
    float anim_timer;
} VisualProjectile;

#define MAX_VISUAL_EFFECTS 32
typedef struct {
    int active;
    float x, y, z;
    float total_time;
    float elapsed;
    Color color;
    float radius;
    uint32_t spot_id;
    float yaw_degrees;
    AnimModelState* anim_state;
    uint16_t anim_seq;
    int anim_frame;
    float anim_timer;
} VisualEffect;

typedef struct {
    AnimModelState* anim_state;
    uint16_t anim_seq;
    int anim_frame;
    float anim_timer;
} ObjectAnimRuntime;

/* NPC colors by type */
static const Color NPC_COLORS[] = {
    {128,128,128,255}, /* 0: none */
    {180,160,60,255},  /* 1: Tz-Kih (yellow) */
    {100,180,60,255},  /* 2: Tz-Kek (green) */
    {80,150,50,255},   /* 3: Tz-Kek small */
    {60,60,200,255},   /* 4: Tok-Xil (blue) */
    {200,100,60,255},  /* 5: Yt-MejKot (orange) */
    {160,40,160,255},  /* 6: Ket-Zek (purple) */
    {200,40,40,255},   /* 7: TzTok-Jad (RED) */
    {60,200,200,255},  /* 8: Yt-HurKot (cyan) */
};

/* Viewer state */
typedef struct {
    FcState state;
    RuneCUiState ui;
    FcRenderEntity entities[FC_MAX_RENDER_ENTITIES];
    int entity_count;
    int paused, step_once;
    float tps;
    float tick_acc;
    int show_debug, show_grid, show_collision;
    int auto_mode;       /* 1 = random actions, 0 = human control */
    Camera3D camera;
    float cam_yaw, cam_pitch, cam_dist;
    int camera_locked;
    int actions[FC_NUM_ACTION_HEADS];
    uint32_t seed, last_hash;
    int episode_count;
    int attack_target;   /* NPC slot index for attack (-1 = none) */
    /* Terrain + Objects + NPC models */
    TerrainMesh* terrain;
    ObjectMesh* objects;
    ObjectAnimSet* object_anims;
    NpcModelSet* object_anim_models;
    ObjectAnimRuntime* object_anim_runtimes;
    int object_anim_runtime_count;
    NpcModelSet* npc_models;
    NpcModelSet* player_model;
    NpcModelSet* projectile_models;
    SpotAnimSet* spotanims;
    /* Animation cache (shared by player + all NPCs) */
    AnimCache* anim_cache;
    /* Player animation state */
    AnimModelState* player_anim_state;
    uint16_t player_anim_seq;
    int player_anim_frame;
    float player_anim_timer;
    uint16_t player_pose_anim_seq;
    int player_pose_anim_frame;
    float player_pose_anim_timer;
    uint16_t player_action_anim_seq;
    int player_action_anim_frame;
    float player_action_anim_timer;
    uint16_t player_visual_lock_seq;
    float player_visual_lock_timer;
    float player_visual_lock_face_angle;
    int player_attack_visual_target_idx;
    /* Per-NPC animation state */
    AnimModelState* npc_anim_states[FC_MAX_NPCS];
    uint16_t npc_anim_seq[FC_MAX_NPCS];
    int npc_anim_frame[FC_MAX_NPCS];
    float npc_anim_timer[FC_MAX_NPCS];
    int npc_attack_visual_style[FC_MAX_NPCS];
    float npc_attack_visual_timer[FC_MAX_NPCS];
    /* Hitsplats */
    Hitsplat hitsplats[MAX_HITSPLATS];
    /* Buffered key inputs (captured every frame, consumed on tick) */
    int pending_prayer, pending_eat, pending_drink;
    /* Clickable NPC health bars in side panel (filled during draw, checked on click) */
    int panel_npc_slot[8];   /* NPC array index for each panel row */
    int panel_npc_y[8];      /* screen Y of each panel row */
    int panel_npc_count;     /* how many rows drawn */
    int clan_wave_dropdown_open;
    int clan_wave_scroll;
    /* Visual projectiles in flight */
    VisualProjectile projectiles[MAX_PROJECTILES];
    VisualEffect effects[MAX_VISUAL_EFFECTS];
    /* Prayer overhead icon textures */
    Texture2D pray_melee_tex, pray_missiles_tex, pray_magic_tex;
    /* Side panel tabs (Phase 8h) */
    int active_tab;     /* 0=inventory, 1=combat, 2=prayer, 3=stats */
    int active_loadout; /* index into FC_LOADOUTS[] */
    int tab_area_y;     /* screen Y where tab buttons start (set during draw) */
    int combat_style;   /* 0=accurate, 1=rapid, 2=long range */
    /* Item / tab sprites */
    Texture2D tex_ppot, tex_shark;
    Texture2D tex_pray_melee_on, tex_pray_melee_off;
    Texture2D tex_pray_range_on, tex_pray_range_off;
    Texture2D tex_pray_magic_on, tex_pray_magic_off;
    Texture2D tex_tab_inv, tex_tab_combat, tex_tab_prayer;
    /* Agent action test mode (Phase 9a verification) */
    int test_mode;       /* 1 = running scripted agent tests */
    int test_id;         /* current test index */
    int test_tick;       /* ticks elapsed in current test */
    /* Debug overlay (Phase 9c) — toggled with O key */
    int dbg_flags;       /* bitmask of DBG_* flags from fc_debug_overlay.h */
    int dbg_tab;         /* 0=player, 1=obs, 2=mask, 3=reward (side panel debug tab) */
    int dbg_tab_y;       /* screen Y where debug tab buttons start (set during draw) */
    /* Debug toggles */
    int godmode;        /* 1 = player can't die */
    int debug_spawn;    /* NPC type to spawn (0 = off, 1-8 = type) */
    int policy_pipe;    /* 1 = read actions from stdin, write obs to stdout */
    int policy_episode_limit; /* 0 = unlimited auto-reset, >0 = stop after N episodes */
    int policy_episode_count; /* number of completed policy-pipe episodes */
    int start_wave;     /* 0 = wave 1 (default), >0 = skip to this wave on reset */
    int initial_sharks;
    int initial_prayer_doses;
    FcRewardParams reward_params;
    FcRewardRuntime reward_runtime;
    FcRewardBreakdown reward_breakdown;
    int reward_breakdown_tick;
    int reward_config_loaded;
    char reward_config_path[FC_REWARD_CONFIG_PATH_MAX];
    /* Obs ablation flags (matches FightCaves env). Applied AFTER fc_write_obs
     * in write_obs_to_pipe so policy replay sees the same obs distribution it
     * was trained on. See fc_apply_obs_ablation in fc-core/src/fc_state.c. */
    int obs_ablate_npc_distance;
    int obs_ablate_incoming_aggregates;
    int obs_ablate_npc_valid;
    /* Smooth movement interpolation — stored by stable slot (player + NPC array index) */
    float prev_player_x, prev_player_y;
    int prev_player_route_idx;
    int prev_player_route_len;
    int prev_player_route_x;
    int prev_player_route_y;
    float prev_npc_x[FC_MAX_NPCS];
    float prev_npc_y[FC_MAX_NPCS];
    int prev_npc_active[FC_MAX_NPCS];  /* was this NPC active last tick? */
    float tick_frac;
} ViewerState;

/* Forward declarations */
static int process_tab_click(ViewerState* v, float mx, float my);
static void draw_tex_fit(Texture2D tex, int dx, int dy, int dw, int dh,
                         Color tint);

static void set_ui_slot(RuneCUiSlot* slot, uint32_t item_id,
                        uint32_t icon_item_id, int quantity,
                        const char* label) {
    if (!slot) return;
    memset(slot, 0, sizeof(*slot));
    if (item_id == 0 || quantity <= 0) return;
    slot->item_id = item_id;
    slot->icon_item_id = icon_item_id ? icon_item_id : item_id;
    slot->quantity = quantity;
    snprintf(slot->label, sizeof(slot->label), "%s", label ? label : "Item");
    slot->enabled = 1;
}

static uint32_t prayer_potion_item_id_for_doses(int doses) {
    switch (doses) {
        case 4: return FC_UI_ITEM_PRAYER_POT_4;
        case 3: return FC_UI_ITEM_PRAYER_POT_3;
        case 2: return FC_UI_ITEM_PRAYER_POT_2;
        case 1: return FC_UI_ITEM_PRAYER_POT_1;
        default: return FC_UI_ITEM_VIAL;
    }
}

static const char* prayer_potion_label_for_doses(int doses) {
    switch (doses) {
        case 4: return "Prayer potion(4)";
        case 3: return "Prayer potion(3)";
        case 2: return "Prayer potion(2)";
        case 1: return "Prayer potion(1)";
        default: return "Vial";
    }
}

static uint32_t fc_ui_active_prayer_bits(int prayer) {
    switch (prayer) {
        case PRAYER_PROTECT_MAGIC: return 1u << 16;
        case PRAYER_PROTECT_RANGE: return 1u << 17;
        case PRAYER_PROTECT_MELEE: return 1u << 18;
        default: return 0;
    }
}

static int fc_ui_prayer_action_for_slot(const FcPlayer* p, int slot) {
    int prayer = PRAYER_NONE;
    int action = FC_PRAYER_OFF;
    if (slot == 16) {
        prayer = PRAYER_PROTECT_MAGIC;
        action = FC_PRAYER_MAGIC;
    } else if (slot == 17) {
        prayer = PRAYER_PROTECT_RANGE;
        action = FC_PRAYER_RANGE;
    } else if (slot == 18) {
        prayer = PRAYER_PROTECT_MELEE;
        action = FC_PRAYER_MELEE;
    } else {
        return 0;
    }
    if (!p || p->current_prayer <= 0) return 0;
    return p->prayer == prayer ? FC_PRAYER_OFF : action;
}

static void queue_viewer_prayer_button(ViewerState* v, int prayer, int action) {
    if (!v) return;
    FcPlayer* p = &v->state.player;
    if (p->current_prayer <= 0)
        return;
    v->pending_prayer = (p->prayer == prayer) ? FC_PRAYER_OFF : action;
}

static void load_ui_item_icon(RuneCUiState* ui, uint32_t item_id) {
    if (!ui || item_id == 0) return;
    char path[128];
    snprintf(path, sizeof(path), "data/sprites/items/item_%u.png", item_id);
    if (!fc_asset_exists(path)) return;
    Texture2D tex = fc_load_texture_asset(path);
    if (tex.id == 0) return;
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    runec_ui_set_item_icon(ui, item_id, tex);
}

static void load_fc_ui_item_icons(ViewerState* v) {
    static const uint32_t ids[] = {
        FC_UI_ITEM_VIAL, FC_UI_ITEM_SHARK,
        FC_UI_ITEM_PRAYER_POT_1, FC_UI_ITEM_PRAYER_POT_2,
        FC_UI_ITEM_PRAYER_POT_3, FC_UI_ITEM_PRAYER_POT_4,
    };
    if (!v) return;
    for (int i = 0; i < (int)(sizeof(ids) / sizeof(ids[0])); i++)
        load_ui_item_icon(&v->ui, ids[i]);
    for (int li = 0; li < FC_NUM_LOADOUTS; li++) {
        const FcLoadout* lo = &FC_LOADOUTS[li];
        for (int ei = 0; ei < lo->equipment_count; ei++) {
            uint32_t icon_id = lo->equipment[ei].icon_item_id
                ? lo->equipment[ei].icon_item_id
                : lo->equipment[ei].item_id;
            load_ui_item_icon(&v->ui, icon_id);
        }
    }
}

static void sync_fc_ui_items(ViewerState* v) {
    if (!v) return;
    FcPlayer* p = &v->state.player;
    for (int i = 0; i < RUNEC_UI_INV_SLOT_COUNT; i++)
        memset(&v->ui.inventory[i], 0, sizeof(v->ui.inventory[i]));

    int doses = p->prayer_doses_remaining;
    if (doses < 0) doses = 0;
    if (doses > FC_MAX_PRAYER_DOSES) doses = FC_MAX_PRAYER_DOSES;
    int full_pots = doses / 4;
    int partial = doses % 4;
    for (int slot = 0; slot < 8; slot++) {
        int slot_doses = 0;
        if (slot < full_pots) slot_doses = 4;
        else if (slot == full_pots && partial > 0) slot_doses = partial;
        uint32_t item_id = prayer_potion_item_id_for_doses(slot_doses);
        set_ui_slot(&v->ui.inventory[slot], item_id, item_id, 1,
                    prayer_potion_label_for_doses(slot_doses));
    }
    for (int slot = 8; slot < RUNEC_UI_INV_SLOT_COUNT; slot++) {
        if (slot - 8 < p->sharks_remaining) {
            set_ui_slot(&v->ui.inventory[slot], FC_UI_ITEM_SHARK,
                        FC_UI_ITEM_SHARK, 1, "Shark");
        }
    }

    for (int i = 0; i < RUNEC_UI_EQUIP_SLOT_COUNT; i++)
        memset(&v->ui.equipment[i], 0, sizeof(v->ui.equipment[i]));

    int loadout = v->active_loadout;
    if (loadout < 0 || loadout >= FC_NUM_LOADOUTS)
        loadout = FC_ACTIVE_LOADOUT;
    const FcLoadout* lo = &FC_LOADOUTS[loadout];
    for (int i = 0; i < lo->equipment_count; i++) {
        const FcLoadoutEquipmentItem* equip = &lo->equipment[i];
        if (equip->slot >= 0 && equip->slot < RUNEC_UI_EQUIP_SLOT_COUNT) {
            uint32_t icon_id = equip->icon_item_id ? equip->icon_item_id : equip->item_id;
            int quantity = equip->slot == FC_EQUIP_SLOT_AMMO ? p->ammo_count : 1;
            set_ui_slot(&v->ui.equipment[equip->slot], equip->item_id,
                        icon_id, quantity, equip->label);
        }
    }
}

static void sync_fc_ui_status(ViewerState* v) {
    if (!v) return;
    FcPlayer* p = &v->state.player;
    runec_ui_sync_status(&v->ui, FC_WORLD_ORIGIN_X + p->x,
                         FC_WORLD_ORIGIN_Y + p->y, p->x, p->y,
                         (uint32_t)v->state.tick, p->is_running, v->paused);
    v->ui.hitpoints = p->current_hp > 0 ? (p->current_hp + 9) / 10 : 0;
    v->ui.hitpoints_max = p->max_hp > 0 ? (p->max_hp + 9) / 10 : 0;
    v->ui.prayer_points = p->current_prayer > 0 ? (p->current_prayer + 9) / 10 : 0;
    v->ui.prayer_points_max = p->max_prayer > 0 ? (p->max_prayer + 9) / 10 : 0;
    v->ui.active_prayers = fc_ui_active_prayer_bits(p->prayer);
    v->ui.run_energy = p->run_energy / 100;
    if (v->ui.run_energy < 0) v->ui.run_energy = 0;
    if (v->ui.run_energy > 100) v->ui.run_energy = 100;
    v->ui.selected_combat_style = v->combat_style == 2 ? 3 : v->combat_style;
    v->ui.auto_retaliate = 1;
    v->ui.special_attack_energy = 100;
    v->ui.combat_level = 126;
    int loadout = v->active_loadout;
    if (loadout < 0 || loadout >= FC_NUM_LOADOUTS)
        loadout = FC_ACTIVE_LOADOUT;
    const FcLoadout* lo = &FC_LOADOUTS[loadout];
    runec_ui_set_combat_weapon_name(&v->ui, lo->weapon_name);
    runec_ui_set_combat_style_profile(&v->ui, lo->combat_style_profile);

    for (int i = 0; i < RUNEC_UI_SKILL_COUNT; i++) {
        v->ui.skill_current[i] = 1;
        v->ui.skill_base[i] = 1;
    }
    v->ui.skill_current[0] = v->ui.skill_base[0] = p->attack_level;
    v->ui.skill_current[1] = v->ui.skill_base[1] = p->strength_level;
    v->ui.skill_current[2] = v->ui.skill_base[2] = p->defence_level;
    v->ui.skill_current[3] = v->ui.skill_base[3] = p->ranged_level;
    v->ui.skill_current[4] = v->ui.skill_base[4] = p->prayer_level;
    v->ui.skill_current[5] = v->ui.skill_base[5] = p->magic_level;
    v->ui.skill_current[8] = v->ui.skill_base[8] = p->max_hp / 10;
    int total = 0;
    for (int i = 0; i < RUNEC_UI_SKILL_COUNT; i++)
        total += v->ui.skill_base[i];
    v->ui.skill_total = total;
}

static Color minimap_color_for_tile(ViewerState* v, int x, int y) {
    if (!v || x < 0 || x >= FC_ARENA_WIDTH || y < 0 || y >= FC_ARENA_HEIGHT)
        return BLANK;
    if (!v->state.walkable[x][y])
        return (Color){42, 34, 26, 255};
    if (v->terrain && v->terrain->loaded)
        return (Color){82, 70, 52, 255};
    return (Color){64, 88, 48, 255};
}

static void sync_fc_ui_minimap(ViewerState* v) {
    if (!v) return;
    FcPlayer* p = &v->state.player;
    Color pixels[152 * 152];
    const float scale = 4.0f;
    for (int y = 0; y < 152; y++) {
        for (int x = 0; x < 152; x++) {
            float sx = (float)x - 76.0f;
            float sy = (float)y - 76.0f;
            if (sx * sx + sy * sy > 75.0f * 75.0f) {
                pixels[x + y * 152] = BLANK;
                continue;
            }
            int tx = (int)roundf((float)p->x + sx / scale);
            int ty = (int)roundf((float)p->y - sy / scale);
            pixels[x + y * 152] = minimap_color_for_tile(v, tx, ty);
        }
    }
    runec_ui_update_minimap(&v->ui, pixels, 152, 152);

    runec_ui_clear_minimap(&v->ui);
    runec_ui_add_minimap_dot(&v->ui, 0.0f, 0.0f, RUNEC_UI_MINIMAP_DOT_PLAYER);
    if (p->route_idx < p->route_len && p->route_len > 0) {
        int tx = p->route_x[p->route_len - 1];
        int ty = p->route_y[p->route_len - 1];
        runec_ui_add_minimap_dot(&v->ui, (float)tx - (float)p->x,
                                 (float)ty - (float)p->y,
                                 RUNEC_UI_MINIMAP_DOT_DESTINATION);
    }
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* n = &v->state.npcs[i];
        if (!n->active || n->is_dead) continue;
        float nx = (float)n->x + (float)n->size * 0.5f;
        float ny = (float)n->y + (float)n->size * 0.5f;
        runec_ui_add_minimap_dot(&v->ui, nx - (float)p->x,
                                 ny - (float)p->y,
                                 RUNEC_UI_MINIMAP_DOT_NPC);
    }
}

static void sync_fc_ui(ViewerState* v) {
    sync_fc_ui_items(v);
    sync_fc_ui_status(v);
    sync_fc_ui_minimap(v);
}

static void route_player_to_tile(ViewerState* v, int tx, int ty) {
    if (!v || tx < 0 || tx >= FC_ARENA_WIDTH || ty < 0 || ty >= FC_ARENA_HEIGHT)
        return;
    FcPlayer* p = &v->state.player;
    if (!v->state.walkable[tx][ty]) return;
    int steps = fc_pathfind_bfs(p->x, p->y, tx, ty, v->state.walkable,
                                p->route_x, p->route_y, FC_MAX_ROUTE);
    p->route_len = steps;
    p->route_idx = 0;
    p->attack_target_idx = -1;
    p->approach_target = 0;
}

static void handle_runec_ui_intent(ViewerState* v) {
    if (!v) return;
    RuneCUiIntent* intent = &v->ui.last_intent;
    FcPlayer* p = &v->state.player;
    switch (intent->kind) {
        case RUNEC_UI_INTENT_INVENTORY_SLOT:
            if (intent->primary >= 0 && intent->primary < 8) {
                int full_pots = p->prayer_doses_remaining / 4;
                int partial = p->prayer_doses_remaining % 4;
                if (intent->primary < full_pots ||
                        (intent->primary == full_pots && partial > 0))
                    v->pending_drink = FC_DRINK_PRAYER_POT;
            } else if (intent->primary >= 8 && intent->primary < 28) {
                if (intent->primary - 8 < p->sharks_remaining)
                    v->pending_eat = FC_EAT_SHARK;
            }
            break;
        case RUNEC_UI_INTENT_INVENTORY_ACTION:
            if (strcmp(intent->text, "Use") == 0 || strcmp(intent->text, "Drink") == 0)
                v->pending_drink = FC_DRINK_PRAYER_POT;
            else if (strcmp(intent->text, "Eat") == 0)
                v->pending_eat = FC_EAT_SHARK;
            break;
        case RUNEC_UI_INTENT_PRAYER_SLOT: {
            int action = fc_ui_prayer_action_for_slot(p, intent->primary);
            if (action) v->pending_prayer = action;
            break;
        }
        case RUNEC_UI_INTENT_COMBAT_STYLE:
            v->combat_style = intent->primary == 3 ? 2 : intent->primary;
            if (v->combat_style < 0) v->combat_style = 0;
            if (v->combat_style > 2) v->combat_style = 2;
            break;
        case RUNEC_UI_INTENT_RUN_TOGGLE:
            p->is_running = !p->is_running;
            break;
        case RUNEC_UI_INTENT_MINIMAP_CLICK: {
            int dx = (intent->primary - 72) / 4;
            int dy = (75 - intent->secondary) / 4;
            route_player_to_tile(v, p->x + dx, p->y + dy);
            break;
        }
        default:
            break;
    }
}

/* Helpers */
static uint32_t viewer_player_model_id(const ViewerState* v) {
    int loadout = v ? v->active_loadout : FC_ACTIVE_LOADOUT;
    if (loadout < 0) loadout = 0;
    if (loadout >= FC_NUM_LOADOUTS) loadout = FC_ACTIVE_LOADOUT;
    return FC_LOADOUTS[loadout].player_model_id;
}

static NpcModelEntry* viewer_player_model_entry(ViewerState* v) {
    if (!v || !v->player_model) return NULL;
    NpcModelEntry* entry = fc_npc_model_find(v->player_model,
                                             viewer_player_model_id(v));
    if (!entry && v->player_model->count > 0)
        entry = &v->player_model->entries[0];
    return (entry && entry->loaded) ? entry : NULL;
}

static void apply_anim_frame_to_entry(NpcModelEntry* entry,
                                      AnimModelState* state,
                                      const AnimFrameData* frame_data,
                                      const AnimFrameBase* fb) {
    if (!entry || !entry->loaded || !state || !frame_data || !fb) return;
    anim_apply_frame(state, entry->base_verts, frame_data, fb);
    models_recompute_texture_uvs_from_vertices(entry, state->verts);

    float* mesh_verts = entry->model.meshes[0].vertices;
    anim_update_mesh(mesh_verts, state, entry->face_indices, entry->face_count);

    int evc = entry->face_count * 3;
    for (int vi = 0; vi < evc; vi++) {
        mesh_verts[vi*3+0] /=  128.0f;
        mesh_verts[vi*3+1] /=  128.0f;
        mesh_verts[vi*3+2] /= -128.0f;
    }

    UpdateMeshBuffer(entry->model.meshes[0], 0, mesh_verts,
                     evc * 3 * sizeof(float), 0);
}

static void upload_anim_state_to_entry(NpcModelEntry* entry,
                                       AnimModelState* state) {
    if (!entry || !entry->loaded || !state) return;
    models_recompute_texture_uvs_from_vertices(entry, state->verts);

    float* mesh_verts = entry->model.meshes[0].vertices;
    anim_update_mesh(mesh_verts, state, entry->face_indices, entry->face_count);

    int evc = entry->face_count * 3;
    for (int vi = 0; vi < evc; vi++) {
        mesh_verts[vi*3+0] /=  128.0f;
        mesh_verts[vi*3+1] /=  128.0f;
        mesh_verts[vi*3+2] /= -128.0f;
    }

    UpdateMeshBuffer(entry->model.meshes[0], 0, mesh_verts,
                     evc * 3 * sizeof(float), 0);
}

static AnimSequence* advance_anim_track(AnimCache* cache,
                                        uint16_t desired_seq,
                                        uint16_t* current_seq,
                                        int* frame_index,
                                        float* frame_timer,
                                        float dt) {
    if (!cache || desired_seq == 0 || !current_seq ||
        !frame_index || !frame_timer)
        return NULL;

    AnimSequence* seq = anim_get_sequence(cache, desired_seq);
    if (!seq || seq->frame_count <= 0) return NULL;

    if (*current_seq != desired_seq) {
        *current_seq = desired_seq;
        *frame_index = 0;
        *frame_timer = 0.0f;
    }
    if (*frame_index < 0 || *frame_index >= seq->frame_count)
        *frame_index = 0;

    *frame_timer -= dt;
    if (*frame_timer <= 0.0f) {
        *frame_index = (*frame_index + 1) % seq->frame_count;
        float frame_delay = (float)seq->frames[*frame_index].delay * 0.02f;
        if (frame_delay < 0.016f) frame_delay = 0.016f;
        *frame_timer = frame_delay;
    }

    return seq;
}

static AnimSequence* advance_anim_track_once(AnimCache* cache,
                                             uint16_t desired_seq,
                                             uint16_t* current_seq,
                                             int* frame_index,
                                             float* frame_timer,
                                             float dt) {
    if (!cache || desired_seq == 0 || !current_seq ||
        !frame_index || !frame_timer)
        return NULL;

    AnimSequence* seq = anim_get_sequence(cache, desired_seq);
    if (!seq || seq->frame_count <= 0) return NULL;

    if (*current_seq != desired_seq) {
        *current_seq = desired_seq;
        *frame_index = 0;
        *frame_timer = 0.0f;
    }
    if (*frame_index < 0 || *frame_index >= seq->frame_count)
        *frame_index = 0;

    *frame_timer -= dt;
    while (*frame_timer <= 0.0f && *frame_index < seq->frame_count - 1) {
        *frame_index += 1;
        float frame_delay = (float)seq->frames[*frame_index].delay * 0.02f;
        if (frame_delay < 0.016f) frame_delay = 0.016f;
        *frame_timer += frame_delay;
    }
    if (*frame_timer <= 0.0f)
        *frame_timer = 0.016f;

    return seq;
}

static float anim_sequence_duration_seconds(const AnimSequence* seq) {
    if (!seq || seq->frame_count <= 0) return 0.45f;
    float total = 0.0f;
    for (int i = 0; i < seq->frame_count; i++) {
        float frame_delay = (float)seq->frames[i].delay * 0.02f;
        if (frame_delay < 0.016f) frame_delay = 0.016f;
        total += frame_delay;
    }
    if (total < 0.35f) total = 0.35f;
    return total;
}

static void update_entry_animation(NpcModelEntry* entry,
                                   AnimCache* cache,
                                   AnimModelState** state,
                                   uint16_t* current_seq,
                                   int* frame_index,
                                   float* frame_timer,
                                   int animation_id,
                                   float dt,
                                   float phase_ticks) {
    if (!entry || !entry->loaded || !cache || animation_id < 0 ||
        !entry->vertex_skins || !state || !current_seq ||
        !frame_index || !frame_timer)
        return;

    AnimSequence* seq = anim_get_sequence(cache, (uint16_t)animation_id);
    if (!seq || seq->frame_count <= 0) return;

    if (!*state || (*state)->vert_count != entry->base_vert_count) {
        if (*state) anim_model_state_free(*state);
        *state = anim_model_state_create(entry->vertex_skins,
                                         entry->base_vert_count);
        *current_seq = (uint16_t)animation_id;
        *frame_index = seq->frame_count > 0
            ? ((int)phase_ticks % seq->frame_count) : 0;
        if (*frame_index < 0) *frame_index = 0;
        *frame_timer = 0.0f;
    }

    if (*current_seq != (uint16_t)animation_id) {
        *current_seq = (uint16_t)animation_id;
        *frame_index = 0;
        *frame_timer = 0.0f;
    }

    *frame_timer -= dt;
    if (*frame_timer <= 0.0f) {
        *frame_index = (*frame_index + 1) % seq->frame_count;
        float frame_delay = (float)seq->frames[*frame_index].delay * 0.02f;
        if (frame_delay < 0.016f) frame_delay = 0.016f;
        *frame_timer = frame_delay;
    }

    AnimFrameData* fd = &seq->frames[*frame_index].frame;
    AnimFrameBase* fb = anim_get_framebase(cache, fd->framebase_id);
    if (fb) apply_anim_frame_to_entry(entry, *state, fd, fb);
}

static void free_projectile(VisualProjectile* vp) {
    if (!vp) return;
    if (vp->anim_state) {
        anim_model_state_free(vp->anim_state);
        vp->anim_state = NULL;
    }
    memset(vp, 0, sizeof(*vp));
}

static void free_effect(VisualEffect* fx) {
    if (!fx) return;
    if (fx->anim_state) {
        anim_model_state_free(fx->anim_state);
        fx->anim_state = NULL;
    }
    memset(fx, 0, sizeof(*fx));
}

static void clear_visuals(ViewerState* v) {
    if (!v) return;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        free_projectile(&v->projectiles[i]);
    for (int i = 0; i < MAX_VISUAL_EFFECTS; i++)
        free_effect(&v->effects[i]);
}

static void recreate_player_anim_state(ViewerState* v, NpcModelEntry* pm) {
    if (!v || !pm || !pm->loaded || !pm->vertex_skins) return;
    if (v->player_anim_state &&
        v->player_anim_state->vert_count == pm->base_vert_count)
        return;
    if (v->player_anim_state)
        anim_model_state_free(v->player_anim_state);
    v->player_anim_state = anim_model_state_create(pm->vertex_skins,
                                                   pm->base_vert_count);
    v->player_anim_seq = PLAYER_ANIM_IDLE;
    v->player_anim_frame = 0;
    v->player_anim_timer = 0.0f;
    v->player_pose_anim_seq = PLAYER_ANIM_IDLE;
    v->player_pose_anim_frame = 0;
    v->player_pose_anim_timer = 0.0f;
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
    v->player_visual_lock_seq = 0;
    v->player_visual_lock_timer = 0.0f;
    v->player_visual_lock_face_angle = 0.0f;
    v->player_attack_visual_target_idx = -1;
    fprintf(stderr, "Player animation state created (%d base verts, model %u)\n",
            pm->base_vert_count, pm->model_id);
}

static NpcModelEntry* projectile_model_for_spot(ViewerState* v,
                                                uint32_t spot_id,
                                                const SpotAnimDef** out_spot) {
    const SpotAnimDef* spot = (spot_id > 0 && v && v->spotanims)
        ? spotanim_find(v->spotanims, (int)spot_id) : NULL;
    if (out_spot) *out_spot = spot;
    if (!v || spot_id == 0 || !v->projectile_models) return NULL;

    NpcModelEntry* pm = fc_npc_model_find(v->projectile_models,
                                          PROJ_SPOTANIM_MODEL_BASE + spot_id);
    if (!pm && spot && spot->model_id >= 0)
        pm = fc_npc_model_find(v->projectile_models, (uint32_t)spot->model_id);
    if (!pm)
        pm = fc_npc_model_find(v->projectile_models, spot_id);
    return (pm && pm->loaded) ? pm : NULL;
}

static uint16_t npc_attack_animation_id(int npc_type, int attack_style) {
    if (npc_type == NPC_TZTOK_JAD) {
        if (attack_style == ATTACK_MAGIC) return JAD_ANIM_MAGIC;
        if (attack_style == ATTACK_RANGED) return JAD_ANIM_RANGED;
        if (attack_style == ATTACK_MELEE) return JAD_ANIM_MELEE;
    }
    if (npc_type > 0 && npc_type < 9)
        return NPC_ANIM_ATTACK[npc_type];
    return 0;
}

static void mark_npc_attack_visual(ViewerState* v, int npc_idx,
                                   int attack_style) {
    if (!v || npc_idx < 0 || npc_idx >= FC_MAX_NPCS ||
        attack_style == ATTACK_NONE)
        return;
    v->npc_attack_visual_style[npc_idx] = attack_style;
    v->npc_attack_visual_timer[npc_idx] = 1.15f;
}

static float lerp_f(float a, float b, float t);
static float face_angle_between_tiles(float ax, float ay, float bx, float by);
static int player_visual_lock_active(const ViewerState* v);

static float policy_replay_time_scale(const ViewerState* v) {
    if (!v || !v->policy_pipe || v->tps <= 0.0f) return 1.0f;
    float scale = v->tps / (float)POLICY_REPLAY_BASE_TPS;
    if (scale < 1.0f) scale = 1.0f;
    return scale;
}

static float policy_replay_anim_dt(const ViewerState* v, float dt) {
    return dt * policy_replay_time_scale(v);
}

static float policy_replay_duration(const ViewerState* v, float seconds) {
    float scale = policy_replay_time_scale(v);
    if (scale > 1.0f)
        seconds /= scale;
    float min_seconds = 0.02f;
    if (v && v->policy_pipe && v->tps > 0.0f) {
        float max_seconds = 0.45f / v->tps;
        min_seconds = 0.15f / v->tps;
        if (seconds > max_seconds)
            seconds = max_seconds;
    }
    if (seconds < min_seconds)
        seconds = min_seconds;
    return seconds;
}

static void mark_player_attack_visual(ViewerState* v) {
    if (!v) return;
    AnimSequence* seq = v->anim_cache
        ? anim_get_sequence(v->anim_cache, PLAYER_ANIM_ATTACK) : NULL;
    float attack_x = (float)v->state.player.x;
    float attack_y = (float)v->state.player.y;
    if (v->policy_pipe) {
        attack_x = v->prev_player_x;
        attack_y = v->prev_player_y;
    }
    v->player_visual_lock_seq = PLAYER_ANIM_ATTACK;
    v->player_visual_lock_timer = policy_replay_duration(
        v, anim_sequence_duration_seconds(seq));
    v->player_attack_visual_target_idx = v->state.player.attack_target_idx;
    v->player_visual_lock_face_angle = v->state.player.facing_angle;
    if (v->player_attack_visual_target_idx >= 0 &&
        v->player_attack_visual_target_idx < FC_MAX_NPCS) {
        FcNpc* target = &v->state.npcs[v->player_attack_visual_target_idx];
        if (target->active && !target->is_dead) {
            float tx = (float)target->x + (float)target->size * 0.5f;
            float ty = (float)target->y + (float)target->size * 0.5f;
            v->player_visual_lock_face_angle = face_angle_between_tiles(
                attack_x + 0.5f, attack_y + 0.5f, tx, ty);
        }
    }
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
}

static int player_visual_lock_active(const ViewerState* v) {
    return v && v->player_visual_lock_seq != 0 &&
           v->player_visual_lock_timer > 0.0f;
}

static void text_s(const char* t, int x, int y, int sz, Color c) {
    DrawText(t, x+1, y+1, sz, COL_TEXT_SHADOW);
    DrawText(t, x, y, sz, c);
}

static const char* fc_terminal_name(int terminal) {
    switch (terminal) {
        case TERMINAL_PLAYER_DEATH: return "player_death";
        case TERMINAL_CAVE_COMPLETE: return "cave_complete";
        case TERMINAL_TICK_CAP: return "tick_cap";
        default: return "none";
    }
}

static int float_near(float a, float b) {
    return fabsf(a - b) < 0.0001f;
}

static char* trim_ascii(char* s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;

    char* end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

static void reward_params_apply_key(FcRewardParams* params,
                                    const char* key,
                                    const char* value) {
    if (strcmp(key, "w_damage_dealt") == 0) params->w_damage_dealt = strtof(value, NULL);
    else if (strcmp(key, "w_progress") == 0) params->w_progress = strtof(value, NULL);
    else if (strcmp(key, "negative_progress_multiplier") == 0) params->negative_progress_multiplier = strtof(value, NULL);
    else if (strcmp(key, "w_damage_taken") == 0) params->w_damage_taken = strtof(value, NULL);
    else if (strcmp(key, "w_npc_kill") == 0) params->w_npc_kill = strtof(value, NULL);
    else if (strcmp(key, "w_wave_clear") == 0) params->w_wave_clear = strtof(value, NULL);
    else if (strcmp(key, "w_jad_kill") == 0) params->w_jad_kill = strtof(value, NULL);
    else if (strcmp(key, "w_cave_complete") == 0) params->w_cave_complete = strtof(value, NULL);
    else if (strcmp(key, "w_player_death") == 0) params->w_player_death = strtof(value, NULL);
    else if (strcmp(key, "scale_player_death_with_progress") == 0) params->scale_player_death_with_progress = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "player_death_min_scale") == 0) params->player_death_min_scale = strtof(value, NULL);
    else if (strcmp(key, "w_correct_jad_prayer") == 0) params->w_correct_jad_prayer = strtof(value, NULL);
    else if (strcmp(key, "w_correct_danger_prayer") == 0) params->w_correct_danger_prayer = strtof(value, NULL);
    else if (strcmp(key, "w_prayer_lost") == 0) params->w_prayer_lost = strtof(value, NULL);
    else if (strcmp(key, "w_invalid_action") == 0) params->w_invalid_action = strtof(value, NULL);
    else if (strcmp(key, "w_tick_penalty") == 0) params->w_tick_penalty = strtof(value, NULL);
    else if (strcmp(key, "shape_unnecessary_prayer_penalty") == 0) params->shape_unnecessary_prayer_penalty = strtof(value, NULL);
    else if (strcmp(key, "shape_wave_stall_base_penalty") == 0) params->shape_wave_stall_base_penalty = strtof(value, NULL);
    else if (strcmp(key, "shape_wave_stall_cap") == 0) params->shape_wave_stall_cap = strtof(value, NULL);
    else if (strcmp(key, "shape_wave_stall_start") == 0) params->shape_wave_stall_start = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "shape_wave_stall_ramp_interval") == 0) params->shape_wave_stall_ramp_interval = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "shape_jad_heal_penalty") == 0) params->shape_jad_heal_penalty = strtof(value, NULL);
    else if (strcmp(key, "shape_npc_heal_penalty") == 0) params->shape_npc_heal_penalty = strtof(value, NULL);
    else if (strcmp(key, "shape_no_progress_penalty_1") == 0) params->shape_no_progress_penalty_1 = strtof(value, NULL);
    else if (strcmp(key, "shape_no_progress_penalty_2") == 0) params->shape_no_progress_penalty_2 = strtof(value, NULL);
    else if (strcmp(key, "shape_no_progress_penalty_3") == 0) params->shape_no_progress_penalty_3 = strtof(value, NULL);
    else if (strcmp(key, "shape_no_attack_base_penalty") == 0) params->shape_no_attack_base_penalty = strtof(value, NULL);
    else if (strcmp(key, "shape_no_attack_wave_scale") == 0) params->shape_no_attack_wave_scale = strtof(value, NULL);
    else if (strcmp(key, "shape_no_progress_start_1") == 0) params->shape_no_progress_start_1 = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "shape_no_progress_start_2") == 0) params->shape_no_progress_start_2 = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "shape_no_progress_start_3") == 0) params->shape_no_progress_start_3 = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "shape_no_attack_start") == 0) params->shape_no_attack_start = (int)strtol(value, NULL, 10);
}

static void obs_ablation_apply_key(ViewerState* v,
                                   const char* key,
                                   const char* value) {
    if (strcmp(key, "obs_ablate_npc_distance") == 0)
        v->obs_ablate_npc_distance = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "obs_ablate_incoming_aggregates") == 0)
        v->obs_ablate_incoming_aggregates = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "obs_ablate_npc_valid") == 0)
        v->obs_ablate_npc_valid = (int)strtol(value, NULL, 10);
}

static void initial_supplies_apply_key(ViewerState* v,
                                       const char* key,
                                       const char* value) {
    if (strcmp(key, "initial_sharks") == 0)
        v->initial_sharks = (int)strtol(value, NULL, 10);
    else if (strcmp(key, "initial_prayer_doses") == 0)
        v->initial_prayer_doses = (int)strtol(value, NULL, 10);
}

static void apply_initial_supplies(ViewerState* v) {
    if (v->initial_sharks < 0) v->initial_sharks = 0;
    if (v->initial_sharks > FC_MAX_SHARKS) v->initial_sharks = FC_MAX_SHARKS;
    if (v->initial_prayer_doses < 0) v->initial_prayer_doses = 0;
    if (v->initial_prayer_doses > FC_MAX_PRAYER_DOSES)
        v->initial_prayer_doses = FC_MAX_PRAYER_DOSES;
    v->state.player.sharks_remaining = v->initial_sharks;
    v->state.player.prayer_doses_remaining = v->initial_prayer_doses;
}

static void load_reward_params(ViewerState* v) {
    v->reward_params = fc_reward_default_params();
    v->initial_sharks = 0;
    v->initial_prayer_doses = 0;
    v->obs_ablate_npc_distance = 0;
    v->obs_ablate_incoming_aggregates = 0;
    v->obs_ablate_npc_valid = 0;
    v->reward_config_loaded = 0;
    snprintf(v->reward_config_path, sizeof(v->reward_config_path), "%s", "defaults");

    {
        char config_path[FC_ASSET_PATH_MAX];
        FILE* f;
        if (!fc_repo_resolve_path("config/fight_caves.ini",
                                  config_path, sizeof(config_path))) {
            return;
        }
        f = fopen(config_path, "r");
        if (!f) return;

        char line[512];
        int in_env = 0;
        while (fgets(line, sizeof(line), f)) {
            char* comment = strchr(line, '#');
            if (comment) *comment = '\0';

            char* text = trim_ascii(line);
            if (*text == '\0') continue;

            if (*text == '[') {
                char* close = strchr(text, ']');
                if (!close) continue;
                *close = '\0';
                in_env = (strcmp(text + 1, "env") == 0);
                continue;
            }

            if (!in_env) continue;

            char* eq = strchr(text, '=');
            if (!eq) continue;
            *eq = '\0';

            char* key = trim_ascii(text);
            char* value = trim_ascii(eq + 1);
            if (*key == '\0' || *value == '\0') continue;

            reward_params_apply_key(&v->reward_params, key, value);
            obs_ablation_apply_key(v, key, value);
            initial_supplies_apply_key(v, key, value);
        }

        fclose(f);
        v->reward_config_loaded = 1;
        strncpy(v->reward_config_path, config_path, sizeof(v->reward_config_path) - 1);
        v->reward_config_path[sizeof(v->reward_config_path) - 1] = '\0';
        return;
    }
}

static void reset_reward_tracking(ViewerState* v) {
    fc_reward_runtime_reset(&v->reward_runtime);
    memset(&v->reward_breakdown, 0, sizeof(v->reward_breakdown));
    v->reward_breakdown_tick = -1;
}

static void update_reward_breakdown(ViewerState* v) {
    if (v->reward_breakdown_tick == v->state.tick) return;
    v->reward_breakdown = fc_reward_compute_breakdown(
        &v->state, &v->reward_params, &v->reward_runtime);
    fc_reward_sync_progress_state(&v->state, &v->reward_runtime);
    v->reward_breakdown_tick = v->state.tick;
}

static const float MANUAL_TPS_PRESETS[] = {
    0.25f, 0.50f, NORMAL_TPS, 4.0f, 10.0f, 15.0f, 30.0f, 60.0f
};
static const char* MANUAL_TPS_LABELS[] = {
    "0.25", "0.5", "5/3", "4", "10", "15", "30", "60"
};
#define NUM_MANUAL_TPS_PRESETS ((int)(sizeof(MANUAL_TPS_PRESETS) / sizeof(MANUAL_TPS_PRESETS[0])))

static float policy_replay_multiplier_to_tps(int multiplier) {
    switch (multiplier) {
        case 1: return (float)POLICY_REPLAY_BASE_TPS;
        case 2: return (float)(POLICY_REPLAY_BASE_TPS * 2);
        case 4: return (float)(POLICY_REPLAY_BASE_TPS * 4);
        case 10: return (float)(POLICY_REPLAY_BASE_TPS * 10);
        default: return (float)POLICY_REPLAY_BASE_TPS;
    }
}

static int policy_replay_tps_to_multiplier(float tps) {
    if (float_near(tps, (float)POLICY_REPLAY_BASE_TPS)) return 1;
    if (float_near(tps, (float)(POLICY_REPLAY_BASE_TPS * 2))) return 2;
    if (float_near(tps, (float)(POLICY_REPLAY_BASE_TPS * 4))) return 4;
    if (float_near(tps, (float)(POLICY_REPLAY_BASE_TPS * 10))) return 10;
    return 0;
}

static int policy_replay_normalize_multiplier(int multiplier) {
    switch (multiplier) {
        case 1:
        case 2:
        case 4:
        case 10:
            return multiplier;
        default:
            return 1;
    }
}

static void set_viewer_tps(ViewerState* v, float tps) {
    if (tps < MIN_TPS) tps = MIN_TPS;
    if (tps > MAX_TPS) tps = MAX_TPS;
    v->tps = tps;
    if (v->tick_acc >= 1.0f)
        v->tick_acc = fmodf(v->tick_acc, 1.0f);
}

static void set_policy_replay_speed(ViewerState* v, int multiplier) {
    int normalized = policy_replay_normalize_multiplier(multiplier);
    set_viewer_tps(v, policy_replay_multiplier_to_tps(normalized));
    fprintf(stderr, "[policy-pipe] Replay speed set to %dx (%.0f TPS)\n", normalized, v->tps);
}

static void cycle_policy_replay_speed(ViewerState* v, int direction) {
    static const int presets[] = {1, 2, 4, 10};
    int current = policy_replay_tps_to_multiplier(v->tps);
    int idx = 0;

    for (int i = 0; i < 4; i++) {
        if (presets[i] == current) {
            idx = i;
            break;
        }
    }

    idx += direction;
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;
    set_policy_replay_speed(v, presets[idx]);
}

static void set_manual_speed(ViewerState* v, float tps) {
    float best = MANUAL_TPS_PRESETS[0];
    float best_diff = fabsf(tps - best);
    for (int i = 1; i < NUM_MANUAL_TPS_PRESETS; i++) {
        float diff = fabsf(tps - MANUAL_TPS_PRESETS[i]);
        if (diff < best_diff) {
            best = MANUAL_TPS_PRESETS[i];
            best_diff = diff;
        }
    }
    set_viewer_tps(v, best);
}

static void format_speed_label(const ViewerState* v, char* buf, size_t buf_size) {
    if (v->policy_pipe) {
        int multiplier = policy_replay_tps_to_multiplier(v->tps);
        if (float_near(v->tps, NORMAL_TPS)) {
            snprintf(buf, buf_size, "Replay:5/3 TPS");
        } else if (multiplier > 0) {
            snprintf(buf, buf_size, "Replay:%dx", multiplier);
        } else {
            snprintf(buf, buf_size, "Replay:%.2f TPS", v->tps);
        }
        return;
    }
    if (float_near(v->tps, NORMAL_TPS)) {
        snprintf(buf, buf_size, "TPS:5/3");
        return;
    }
    if (float_near(v->tps, roundf(v->tps))) {
        snprintf(buf, buf_size, "TPS:%.0f", v->tps);
    } else {
        snprintf(buf, buf_size, "TPS:%.2f", v->tps);
    }
}

static const char* policy_metric_npc_name(int npc_type) {
    switch (npc_type) {
        case NPC_TZ_KIH:    return "tz_kih";
        case NPC_TZ_KEK:    return "tz_kek";
        case NPC_TZ_KEK_SM: return "tz_kek_sm";
        case NPC_TOK_XIL:   return "tok_xil";
        case NPC_YT_MEJKOT: return "yt_mejkot";
        case NPC_KET_ZEK:   return "ket_zek";
        case NPC_TZTOK_JAD: return "tztok_jad";
        case NPC_YT_HURKOT: return "yt_hurkot";
        default:            return "unknown";
    }
}

static void print_policy_episode_summary(const ViewerState* v) {
    const FcState* s = &v->state;
    const FcPlayer* p = &s->player;
    int episode_length = s->tick;
    float prayer_uptime_melee = (episode_length > 0)
        ? (float)s->ep_ticks_pray_melee / (float)episode_length : 0.0f;
    float prayer_uptime_range = (episode_length > 0)
        ? (float)s->ep_ticks_pray_range / (float)episode_length : 0.0f;
    float prayer_uptime_magic = (episode_length > 0)
        ? (float)s->ep_ticks_pray_magic / (float)episode_length : 0.0f;
    float attack_when_ready_rate = (s->ep_attack_ready_ticks > 0)
        ? (float)s->ep_attack_attempt_ticks / (float)s->ep_attack_ready_ticks : 0.0f;

    fprintf(stderr,
        "[policy-pipe] episode_summary "
        "{\"episode\":%d,\"seed\":%u,\"terminal\":\"%s\","
        "\"env/episode_length\":%d,"
        "\"env/wave_reached\":%d,"
        "\"env/most_npcs_slayed\":%d,"
        "\"env/prayer_uptime_melee\":%.6f,"
        "\"env/prayer_uptime_range\":%.6f,"
        "\"env/prayer_uptime_magic\":%.6f,"
        "\"env/correct_prayer\":%d,"
        "\"env/wrong_prayer_hits\":%d,"
        "\"env/no_prayer_hits\":%d,"
        "\"env/prayer_switches\":%d,"
        "\"env/damage_blocked\":%d,"
        "\"env/dmg_taken_avg\":%d,"
        "\"env/attack_when_ready_rate\":%.6f,"
        "\"env/tokxil_melee_ticks\":%d,"
        "\"env/ketzek_melee_ticks\":%d,"
        "\"env/max_wave_ticks\":%d,"
        "\"env/max_wave_ticks_wave\":%d,"
        "\"env/reached_wave_63\":%d,"
        "\"env/jad_kill_rate\":%d,"
        "\"env/target_held_ticks\":%d,"
        "\"env/no_target_ticks\":%d,"
        "\"env/target_in_range_los_ticks\":%d,"
        "\"env/target_out_of_range_or_los_ticks\":%d,"
        "\"env/attack_cooldown_wait_ticks\":%d,"
        "\"env/ready_but_no_attack_ticks\":%d,"
        "\"env/action_move_idle_ticks\":%d,"
        "\"env/action_move_walk_ticks\":%d,"
        "\"env/action_move_run_ticks\":%d,"
        "\"env/action_attack_none_ticks\":%d,"
        "\"env/action_attack_target_ticks\":%d,"
        "\"env/action_prayer_noop_ticks\":%d,"
        "\"env/action_prayer_cmd_ticks\":%d",
        v->policy_episode_count + 1,
        v->seed,
        fc_terminal_name(s->terminal),
        episode_length,
        s->current_wave,
        s->total_npcs_killed,
        prayer_uptime_melee,
        prayer_uptime_range,
        prayer_uptime_magic,
        s->ep_correct_blocks,
        s->ep_wrong_prayer_hits,
        s->ep_no_prayer_hits,
        s->ep_prayer_switches,
        s->ep_damage_blocked,
        p->total_damage_taken,
        attack_when_ready_rate,
        s->ep_tokxil_melee_ticks,
        s->ep_ketzek_melee_ticks,
        s->ep_max_wave_ticks,
        s->ep_max_wave_ticks_wave,
        s->ep_reached_wave_63,
        s->ep_jad_killed,
        s->ep_target_held_ticks,
        s->ep_no_target_ticks,
        s->ep_target_in_range_los_ticks,
        s->ep_target_out_of_range_or_los_ticks,
        s->ep_attack_cooldown_wait_ticks,
        s->ep_ready_but_no_attack_ticks,
        s->ep_action_move_idle_ticks,
        s->ep_action_move_walk_ticks,
        s->ep_action_move_run_ticks,
        s->ep_action_attack_none_ticks,
        s->ep_action_attack_target_ticks,
        s->ep_action_prayer_noop_ticks,
        s->ep_action_prayer_cmd_ticks);

    for (int i = 1; i < NPC_TYPE_COUNT; i++) {
        const char* npc = policy_metric_npc_name(i);
        fprintf(stderr,
            ",\"env/dmg_to_%s\":%d"
            ",\"env/resolved_hits_to_%s\":%d"
            ",\"env/damaging_hits_to_%s\":%d"
            ",\"env/attack_cycles_to_%s\":%d"
            ",\"env/target_ticks_%s\":%d",
            npc, s->ep_damage_to_npc_type[i],
            npc, s->ep_resolved_hits_to_npc_type[i],
            npc, s->ep_damaging_hits_to_npc_type[i],
            npc, s->ep_attack_cycles_to_npc_type[i],
            npc, s->ep_target_ticks_by_npc_type[i]);
    }

    fprintf(stderr, ",\"env/n\":1.0}\n");
}

static void reset_ep(ViewerState* v) {
    load_reward_params(v);
    reset_reward_tracking(v);
    v->seed = (uint32_t)GetRandomValue(1, 999999);
    fc_reset(&v->state, v->seed);
    /* Skip to start_wave if set */
    if (v->start_wave > 1 && v->start_wave <= FC_NUM_WAVES) {
        for (int i = 0; i < FC_MAX_NPCS; i++) {
            v->state.npcs[i].active = 0;
            v->state.npcs[i].is_dead = 0;
        }
        v->state.npcs_remaining = 0;
        v->state.current_wave = v->start_wave;
        fc_wave_spawn(&v->state, v->start_wave);
        v->state.player.current_hp = v->state.player.max_hp;
        v->state.player.current_prayer = v->state.player.max_prayer;
    }
    apply_initial_supplies(v);
    fc_reward_runtime_begin_episode(&v->reward_runtime, &v->state);
    fc_fill_render_entities(&v->state, v->entities, &v->entity_count);
    v->last_hash = fc_state_hash(&v->state);
    v->episode_count++;
    v->attack_target = -1;
    v->tick_frac = 1.0f;
    memset(v->actions, 0, sizeof(v->actions));
    memset(v->hitsplats, 0, sizeof(v->hitsplats));
    clear_visuals(v);
    v->pending_prayer = 0;
    v->pending_eat = 0;
    v->pending_drink = 0;
    dbg_log_clear();
    /* Initialize prev positions */
    v->prev_player_x = (float)v->state.player.x;
    v->prev_player_y = (float)v->state.player.y;
    v->prev_player_route_idx = v->state.player.route_idx;
    v->prev_player_route_len = v->state.player.route_len;
    v->prev_player_route_x = v->state.player.x;
    v->prev_player_route_y = v->state.player.y;
    v->player_pose_anim_seq = PLAYER_ANIM_IDLE;
    v->player_pose_anim_frame = 0;
    v->player_pose_anim_timer = 0.0f;
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
    v->player_visual_lock_seq = 0;
    v->player_visual_lock_timer = 0.0f;
    v->player_visual_lock_face_angle = 0.0f;
    v->player_attack_visual_target_idx = -1;
    /* Reset NPC animation states */
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        v->prev_npc_x[i] = (float)v->state.npcs[i].x;
        v->prev_npc_y[i] = (float)v->state.npcs[i].y;
        v->prev_npc_active[i] = v->state.npcs[i].active;
        if (v->npc_anim_states[i]) {
            anim_model_state_free(v->npc_anim_states[i]);
            v->npc_anim_states[i] = NULL;
        }
        v->npc_anim_seq[i] = 0;
        v->npc_anim_frame[i] = 0;
        v->npc_anim_timer[i] = 0;
        v->npc_attack_visual_style[i] = ATTACK_NONE;
        v->npc_attack_visual_timer[i] = 0.0f;
    }
}

static void viewer_jump_to_wave(ViewerState* v, int wave) {
    if (!v) return;
    if (wave < 1) wave = 1;
    if (wave > FC_NUM_WAVES) wave = FC_NUM_WAVES;

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        v->state.npcs[i].active = 0;
        v->state.npcs[i].is_dead = 0;
    }
    v->state.npcs_remaining = 0;
    v->state.current_wave = wave;
    fc_wave_spawn(&v->state, wave);
    v->state.player.current_hp = v->state.player.max_hp;
    v->state.player.current_prayer = v->state.player.max_prayer;
    v->state.terminal = TERMINAL_NONE;
    v->state.jad_healers_spawned = 0;
    reset_reward_tracking(v);
    fc_reward_runtime_begin_episode(&v->reward_runtime, &v->state);
    fc_fill_render_entities(&v->state, v->entities, &v->entity_count);
    memset(v->hitsplats, 0, sizeof(v->hitsplats));
    clear_visuals(v);
    v->attack_target = -1;
    v->tick_frac = 1.0f;
    dbg_log_clear();
    v->prev_player_x = (float)v->state.player.x;
    v->prev_player_y = (float)v->state.player.y;
    v->prev_player_route_idx = v->state.player.route_idx;
    v->prev_player_route_len = v->state.player.route_len;
    v->prev_player_route_x = v->state.player.x;
    v->prev_player_route_y = v->state.player.y;
    v->player_pose_anim_seq = PLAYER_ANIM_IDLE;
    v->player_pose_anim_frame = 0;
    v->player_pose_anim_timer = 0.0f;
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
    v->player_visual_lock_seq = 0;
    v->player_visual_lock_timer = 0.0f;
    v->player_visual_lock_face_angle = 0.0f;
    v->player_attack_visual_target_idx = -1;

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        v->prev_npc_x[i] = (float)v->state.npcs[i].x;
        v->prev_npc_y[i] = (float)v->state.npcs[i].y;
        v->prev_npc_active[i] = v->state.npcs[i].active;
        if (v->npc_anim_states[i]) {
            anim_model_state_free(v->npc_anim_states[i]);
            v->npc_anim_states[i] = NULL;
        }
        v->npc_anim_seq[i] = 0;
        v->npc_anim_frame[i] = 0;
        v->npc_anim_timer[i] = 0.0f;
        v->npc_attack_visual_style[i] = ATTACK_NONE;
        v->npc_attack_visual_timer[i] = 0.0f;
    }
}

static void spawn_spot_effect(ViewerState* v, uint32_t spot_id,
                              float x, float y, float z,
                              float duration, Color col, float radius,
                              float yaw_degrees) {
    if (!v || spot_id == 0) return;
    for (int i = 0; i < MAX_VISUAL_EFFECTS; i++) {
        if (!v->effects[i].active) {
            VisualEffect* fx = &v->effects[i];
            memset(fx, 0, sizeof(*fx));
            fx->active = 1;
            fx->x = x; fx->y = y; fx->z = z;
            fx->total_time = duration;
            fx->elapsed = 0.0f;
            fx->color = col;
            fx->radius = radius;
            fx->spot_id = spot_id;
            fx->yaw_degrees = yaw_degrees;
            return;
        }
    }
}

static VisualProjectile* spawn_projectile(ViewerState* v,
                                          float sx, float sy, float sz,
                                          float dx, float dy, float dz,
                                          float travel_secs, Color col,
                                          float radius, uint32_t spot_id,
                                          uint32_t launch_spot_id,
                                          uint32_t impact_spot_id) {
    if (!v || (spot_id == 0 && launch_spot_id == 0 && impact_spot_id == 0))
        return NULL;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!v->projectiles[i].active) {
            VisualProjectile* vp = &v->projectiles[i];
            free_projectile(vp);
            vp->active = 1;
            vp->src_x = sx; vp->src_y = sy; vp->src_z = sz;
            vp->dst_x = dx; vp->dst_y = dy; vp->dst_z = dz;
            vp->total_time = travel_secs; vp->elapsed = 0;
            vp->color = col; vp->radius = radius;
            vp->spot_id = spot_id;
            vp->launch_spot_id = launch_spot_id;
            vp->impact_spot_id = impact_spot_id;
            return vp;
        }
    }
    return NULL;
}

static void apply_projectile_profile(VisualProjectile* vp,
                                     float client_start_time,
                                     float client_end_time,
                                     float projectile_angle,
                                     float projectile_progress) {
    if (!vp || client_end_time <= client_start_time) return;
    vp->profiled = 1;
    vp->client_start_time = client_start_time;
    vp->client_end_time = client_end_time;
    vp->projectile_angle = projectile_angle;
    vp->projectile_progress = projectile_progress;
}

static void spawn_status_splat(ViewerState* v, float wx, float wy, float wz,
                               int amount_tenths, int kind) {
    for (int i = 0; i < MAX_HITSPLATS; i++) {
        if (!v->hitsplats[i].active) {
            v->hitsplats[i].active = 1;
            v->hitsplats[i].world_x = wx;
            v->hitsplats[i].world_y = wy;
            v->hitsplats[i].world_z = wz;
            v->hitsplats[i].damage = amount_tenths;
            v->hitsplats[i].kind = kind;
            v->hitsplats[i].frames_left = 60;  /* 1 second at 60fps */
            return;
        }
    }
}

static void spawn_hitsplat(ViewerState* v, float wx, float wy, float wz,
                           int damage_tenths) {
    spawn_status_splat(v, wx, wy, wz, damage_tenths, HITSPLAT_DAMAGE);
}

/* Terrain loader — the terrain mesh is the floor heightmap.
 * The red/black lava pattern comes from the objects mesh (fightcaves.objects),
 * not the terrain. The terrain is just the ground surface.
 * We keep original cache colors (dark base) without modification. */
static TerrainMesh* load_terrain(ViewerState* v) {
    (void)v;
    TerrainMesh* tm = terrain_load("fightcaves.terrain");
    if (tm && tm->loaded) {
        terrain_offset(tm, FC_WORLD_ORIGIN_X, FC_WORLD_ORIGIN_Y);
        return tm;
    }
    return NULL;
}

/* Load collision map for use during object height compression */
static int load_collision_for_objects(uint8_t coll[64][64]) {
    FILE* f = fc_repo_fopen("fc-core/assets/fightcaves.collision", "rb");
    uint8_t buf[64*64];
    if (!f) return 0;
    if (!fc_read_exact(f, buf, 1, sizeof(buf),
                       "fc-core/assets/fightcaves.collision", "collision map")) {
        fclose(f);
        return 0;
    }
    fclose(f);
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            coll[x][y] = buf[y * 64 + x];
    return 1;
}

/* Objects loader — no modifications */
static ObjectMesh* load_objects_with_terrain(TerrainMesh* tm) {
    (void)tm;
    ObjectMesh* om = objects_load("fightcaves.objects");
    if (om && om->loaded) {
        objects_offset(om, FC_WORLD_ORIGIN_X, FC_WORLD_ORIGIN_Y);

        /* No modifications to objects mesh — original cache data */

        return om;
    }
    return NULL;
}

/* Forward declaration */
static float ground_y(ViewerState* v, int tile_x, int tile_y);

/* ======================================================================== */
/* Human input → action heads                                                */
/* ======================================================================== */

/* Raycast from mouse position to find the tile coordinate on the ground plane */
static int raycast_to_tile(ViewerState* v, int* out_x, int* out_y) {
    Ray ray = GetScreenToWorldRay(GetMousePosition(), v->camera);
    /* Intersect with Y = ground_y plane */
    float gy = ground_y(v, 32, 32);
    if (fabsf(ray.direction.y) < 0.001f) return 0;  /* ray parallel to ground */
    float t = (gy - ray.position.y) / ray.direction.y;
    if (t < 0) return 0;  /* behind camera */
    float wx = ray.position.x + ray.direction.x * t;
    float wz = ray.position.z + ray.direction.z * t;
    /* Convert to tile coords: X = world X, tile Y = -world Z */
    int tx = (int)floorf(wx);
    int ty = (int)floorf(-wz);
    if (tx < 0 || tx >= FC_ARENA_WIDTH || ty < 0 || ty >= FC_ARENA_HEIGHT) return 0;
    *out_x = tx;
    *out_y = ty;
    return 1;
}

/* Find NPC at clicked tile — checks LIVE state, not render snapshot.
 * Returns NPC array index (0..FC_MAX_NPCS-1) or -1 if no NPC there. */
static int find_clicked_npc_idx(ViewerState* v, int tile_x, int tile_y) {
    int best = -1;
    int best_dist = 999;
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* n = &v->state.npcs[i];
        if (!n->active || n->is_dead) continue;
        /* Check if tile is within the NPC's footprint (or 1 tile adjacent) */
        if (tile_x >= n->x - 1 && tile_x <= n->x + n->size &&
            tile_y >= n->y - 1 && tile_y <= n->y + n->size) {
            /* Prefer the closest NPC center */
            int cx = n->x + n->size/2;
            int cy = n->y + n->size/2;
            int d = abs(tile_x - cx) + abs(tile_y - cy);
            if (d < best_dist) { best_dist = d; best = i; }
        }
    }
    return best;
}

/* Called EVERY FRAME to capture clicks (which only fire once at 60fps).
 * Sets routes and targets on the player struct directly. */
static void process_human_clicks(ViewerState* v, int ui_capture) {
    FcPlayer* p = &v->state.player;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (ui_capture) return;
        Vector2 mpos = GetMousePosition();
        if (mpos.x < FC_ARENA_WIDTH * TILE_SIZE) {
            int tx, ty;
            int rc = raycast_to_tile(v, &tx, &ty);
            fprintf(stderr, "CLICK mouse=(%.0f,%.0f) raycast=%d tile=(%d,%d) player=(%d,%d)",
                    mpos.x, mpos.y, rc, tx, ty, p->x, p->y);
            if (rc) {
                int npc_idx = find_clicked_npc_idx(v, tx, ty);
                if (npc_idx >= 0) {
                    /* Click on/near NPC → set as attack target + approach */
                    p->attack_target_idx = npc_idx;
                    p->approach_target = 1;
                    p->route_len = 0;
                    p->route_idx = 0;
                    fprintf(stderr, " → ATTACK npc_idx=%d\n", npc_idx);
                } else {
                    int walkable = (tx >= 0 && tx < FC_ARENA_WIDTH &&
                                    ty >= 0 && ty < FC_ARENA_HEIGHT) ?
                                   v->state.walkable[tx][ty] : 0;
                    fprintf(stderr, " walkable=%d", walkable);
                    if (walkable) {
                        int steps = fc_pathfind_bfs(
                            p->x, p->y, tx, ty, v->state.walkable,
                            p->route_x, p->route_y, FC_MAX_ROUTE);
                        p->route_len = steps;
                        p->route_idx = 0;
                        p->attack_target_idx = -1;
                        p->approach_target = 0;
                        fprintf(stderr, " → ROUTE %d steps\n", steps);
                    } else {
                        fprintf(stderr, " → BLOCKED\n");
                    }
                }
            } else {
                fprintf(stderr, " → MISS (raycast failed)\n");
            }
        } else {
            /* Click in side panel — try tabs first, then NPC health bars */
            if (!process_tab_click(v, mpos.x, mpos.y)) {
                for (int pi = 0; pi < v->panel_npc_count; pi++) {
                    if (mpos.y >= v->panel_npc_y[pi] && mpos.y < v->panel_npc_y[pi] + 12) {
                        int ni = v->panel_npc_slot[pi];
                        p->attack_target_idx = ni;
                        p->approach_target = 1;
                        p->route_len = 0;
                        p->route_idx = 0;
                        fprintf(stderr, "PANEL CLICK → ATTACK npc_idx=%d\n", ni);
                        break;
                    }
                }
            }
        }
    }
}

/* Agent test data (defined here so key handler can reference it) */
typedef struct {
    const char* name;
    const char* desc;
    int duration;
    int actions[7];
} AgentTest;

static const AgentTest AGENT_TESTS[] = {
    /* --- Movement tests (PASSED) ---
    { "Walk North",     "Head 0 = 1 (walk N, 3 ticks)",        3,  {1, 0,0,0,0, 0,0} },
    { "Walk East",      "Head 0 = 3 (walk E, 3 ticks)",        3,  {3, 0,0,0,0, 0,0} },
    { "Walk South",     "Head 0 = 5 (walk S, 3 ticks)",        3,  {5, 0,0,0,0, 0,0} },
    { "Walk West",      "Head 0 = 7 (walk W, 3 ticks)",        3,  {7, 0,0,0,0, 0,0} },
    { "Walk NE",        "Head 0 = 2 (walk NE, 3 ticks)",       3,  {2, 0,0,0,0, 0,0} },
    { "Walk SE",        "Head 0 = 4 (walk SE, 3 ticks)",       3,  {4, 0,0,0,0, 0,0} },
    { "Walk SW",        "Head 0 = 6 (walk SW, 3 ticks)",       3,  {6, 0,0,0,0, 0,0} },
    { "Walk NW",        "Head 0 = 8 (walk NW, 3 ticks)",       3,  {8, 0,0,0,0, 0,0} },
    { "Run North",      "Head 0 = 9 (run N, 3 ticks = 6 tiles)",  3,  {9, 0,0,0,0, 0,0} },
    { "Run SE",         "Head 0 = 12 (run SE, 3 ticks = 6 tiles)", 3,  {12,0,0,0,0, 0,0} },
    { "Walk-to-tile",   "Heads 5+6 = (26,31) -> tile (25,30)",  10, {0, 0,0,0,0, 26,31} },
    { "Walk-to-tile 2", "Heads 5+6 = (36,36) -> tile (35,35)",  10, {0, 0,0,0,0, 36,36} },
    */

    /* --- Combat tests (PASSED) ---
    { "Attack slot 1",  "Head 1=1: target closest NPC, auto-approach+attack", 15, {0, 1,0,0,0, 0,0} },
    { "Switch to slot 2","Head 1=2: retarget to 2nd closest NPC",             10, {0, 2,0,0,0, 0,0} },
    { "Attack + walk",  "Head 1=1 + walk S: attack while moving south",       10, {5, 1,0,0,0, 0,0} },
    */

    /* --- Prayer tests (PASSED) ---
    { "Prot Magic",     "Head 2=2: activate Protect from Magic",               3, {0, 0,2,0,0, 0,0} },
    { "Prot Range",     "Head 2=3: switch to Protect from Range",              3, {0, 0,3,0,0, 0,0} },
    { "Prot Melee",     "Head 2=4: switch to Protect from Melee",              3, {0, 0,4,0,0, 0,0} },
    { "Prayer off",     "Head 2=1: deactivate prayer",                         3, {0, 0,1,0,0, 0,0} },
    */

    /* --- Consumable tests (PASSED) ---
    { "Eat shark",      "Head 3=1: eat shark, watch HP heal +20",              5, {0, 0,0,1,0, 0,0} },
    { "Eat cooldown",   "Head 3=1: try eat again (3-tick cooldown, may fail)", 2, {0, 0,0,1,0, 0,0} },
    { "Drink ppot",     "Head 4=1: drink prayer pot, watch prayer restore",    5, {0, 0,0,0,1, 0,0} },
    { "Drink cooldown", "Head 4=1: try drink again (2-tick cooldown)",         2, {0, 0,0,0,1, 0,0} },
    { "Eat + drink",    "Head 3=1 + 4=1: both same tick (separate cooldowns)", 5, {0, 0,0,1,1, 0,0} },
    */

    /* --- Combined tests (PASSED) ---
    { "Run+eat+pray",   "Run N + eat shark + prot magic (all same tick)",      5, {9, 0,2,1,0, 0,0} },
    { "Attack+pray+pot","Attack slot 1 + prot range + drink ppot",            10, {0, 1,3,0,1, 0,0} },
    { "WalkTile+attack","Walk to (30,30) + attack slot 1 (walk cancels)",      8, {0, 1,0,0,0, 31,31} },
    */

    /* --- Debug overlay tests (9c-A: Collision/LOS/Path/Range) --- */
    /* Press O to toggle overlays ON before starting these tests */
    { "Collision",      "Press O first! Green=walkable, red=blocked tiles",     5, {0, 0,0,0,0, 0,0} },
    { "LOS rays",       "Green lines=LOS clear, red=blocked. Walk near NPCs",  8, {5, 0,0,0,0, 0,0} },
    { "Path viz",       "Walk to tile (30,25) — yellow path shows route",       10,{0, 0,0,0,0, 31,26} },
    { "Attack range",   "Blue ring = current player weapon range",               5, {0, 0,0,0,0, 0,0} },
};
#define NUM_AGENT_TESTS (int)(sizeof(AGENT_TESTS)/sizeof(AGENT_TESTS[0]))

/* Called EVERY FRAME for key presses. Buffers actions for next tick. */
static void process_human_keys(ViewerState* v) {
    FcPlayer* p = &v->state.player;
    if (IsKeyPressed(KEY_ONE))   v->pending_prayer = (p->prayer == PRAYER_PROTECT_MELEE) ? FC_PRAYER_OFF : FC_PRAYER_MELEE;
    if (IsKeyPressed(KEY_TWO))   v->pending_prayer = (p->prayer == PRAYER_PROTECT_RANGE) ? FC_PRAYER_OFF : FC_PRAYER_RANGE;
    if (IsKeyPressed(KEY_THREE)) v->pending_prayer = (p->prayer == PRAYER_PROTECT_MAGIC) ? FC_PRAYER_OFF : FC_PRAYER_MAGIC;
    if (IsKeyPressed(KEY_F))     v->pending_eat = FC_EAT_SHARK;
    if (IsKeyPressed(KEY_P))     v->pending_drink = FC_DRINK_PRAYER_POT;
    if (IsKeyPressed(KEY_X))     p->is_running = !p->is_running;

    /* T: agent action test mode — start or advance to next test */
    if (IsKeyPressed(KEY_T)) {
        if (!v->test_mode) {
            /* Start test mode from current test_id */
            v->test_mode = 1;
            v->test_tick = 0;
            v->paused = 0;
            fprintf(stderr, "TEST MODE: starting test %d/%d\n", v->test_id + 1, NUM_AGENT_TESTS);
        } else if (v->test_tick >= AGENT_TESTS[v->test_id].duration) {
            /* Current test done — advance to next */
            v->test_id++;
            v->test_tick = 0;
            if (v->test_id >= NUM_AGENT_TESTS) {
                v->test_mode = 0;
                fprintf(stderr, "TEST MODE: all tests complete\n");
            } else {
                v->paused = 0;
                fprintf(stderr, "TEST MODE: starting test %d/%d\n", v->test_id + 1, NUM_AGENT_TESTS);
            }
        }
    }

    /* --- Debug toggles (testing only) --- */
    /* F9: toggle godmode (player can't die) */
    if (IsKeyPressed(KEY_F9)) {
        v->godmode = !v->godmode;
        fprintf(stderr, "GODMODE: %s\n", v->godmode ? "ON" : "OFF");
    }
    /* F1-F8: spawn NPC type 1-8 near the player */
    for (int fk = 0; fk < 8; fk++) {
        if (IsKeyPressed(KEY_F1 + fk)) {
            int npc_type = fk + 1;
            /* Find free NPC slot */
            for (int si = 0; si < FC_MAX_NPCS; si++) {
                if (!v->state.npcs[si].active) {
                    int sx = p->x + 5, sy = p->y;
                    const FcNpcStats* stats = fc_npc_get_stats(npc_type);
                    /* Find nearby walkable tile */
                    for (int r = 0; r < 10; r++) {
                        for (int dx = -r; dx <= r; dx++) {
                            int ty = p->y + r, tx = p->x + dx + 3;
                            if (tx >= 0 && tx < FC_ARENA_WIDTH && ty >= 0 && ty < FC_ARENA_HEIGHT &&
                                fc_footprint_walkable(tx, ty, stats->size, v->state.walkable)) {
                                sx = tx; sy = ty; r = 99; break;
                            }
                        }
                    }
                    fc_npc_spawn(&v->state.npcs[si], npc_type, sx, sy,
                                 v->state.next_spawn_index++);
                    /* Don't increment npcs_remaining — debug spawns shouldn't
                     * affect wave progression. Wave clear checks npcs_remaining. */
                    fprintf(stderr, "DEBUG SPAWN: NPC type %d at (%d,%d)\n", npc_type, sx, sy);
                    break;
                }
            }
        }
    }
}

/* Called on TICK frames: build action array from buffered inputs. */
static void build_human_actions(ViewerState* v) {
    memset(v->actions, 0, sizeof(v->actions));
    /* Movement comes from route deque in fc_tick.c, not action head */
    v->actions[0] = FC_MOVE_IDLE;
    /* Attack comes from attack_target_idx in fc_tick.c, not action head */
    v->actions[1] = FC_ATTACK_NONE;
    /* Buffered prayer/eat/drink */
    v->actions[2] = v->pending_prayer;
    v->actions[3] = v->pending_eat;
    v->actions[4] = v->pending_drink;
    /* Clear buffers */
    v->pending_prayer = 0;
    v->pending_eat = 0;
    v->pending_drink = 0;
}

/* ======================================================================== */
/* Agent action test mode (Phase 9a verification)                           */
/* ======================================================================== */

/* Build actions for current test. Returns 1 if test is active, 0 if done. */
static int build_test_actions(ViewerState* v) {
    if (!v->test_mode) return 0;
    if (v->test_id >= NUM_AGENT_TESTS) {
        v->test_mode = 0;
        return 0;
    }

    const AgentTest* t = &AGENT_TESTS[v->test_id];

    if (v->test_tick >= t->duration) {
        /* Test finished — pause and wait for user to advance */
        v->paused = 1;
        return 0;
    }

    /* Send the test actions */
    memset(v->actions, 0, sizeof(v->actions));
    for (int i = 0; i < 7; i++) {
        v->actions[i] = t->actions[i];
    }

    /* For walk-to-tile tests, only send the coordinates on the first tick
     * (the route persists, subsequent ticks just let it play out) */
    if (v->test_tick > 0 && (t->actions[5] > 0 || t->actions[6] > 0)) {
        v->actions[5] = 0;
        v->actions[6] = 0;
    }

    v->test_tick++;
    return 1;
}

/* Draw test overlay showing what's being tested */
static void draw_test_overlay(ViewerState* v) {
    if (!v->test_mode && v->test_id == 0) return;
    if (v->test_id >= NUM_AGENT_TESTS) return;

    const AgentTest* t = &AGENT_TESTS[v->test_id];

    int bx = 10, bw = 460, bh = 60;
    int arena_bottom = HEADER_HEIGHT + FC_ARENA_HEIGHT * TILE_SIZE;
    int by = arena_bottom - bh - 10;
    DrawRectangle(bx, by, bw, bh, CLITERAL(Color){0,0,0,200});
    DrawRectangleLinesEx((Rectangle){(float)bx,(float)by,(float)bw,(float)bh}, 2,
                         COL_TEXT_YELLOW);

    char buf[128];
    snprintf(buf, sizeof(buf), "TEST %d/%d: %s", v->test_id + 1, NUM_AGENT_TESTS, t->name);
    DrawText(buf, bx + 8, by + 6, 16, COL_TEXT_YELLOW);
    DrawText(t->desc, bx + 8, by + 26, 12, COL_TEXT_WHITE);

    if (v->paused && v->test_tick >= t->duration) {
        snprintf(buf, sizeof(buf), "DONE — press T for next test   (tick %d/%d)",
                 v->test_tick, t->duration);
        DrawText(buf, bx + 8, by + 42, 11, COL_TEXT_GREEN);
    } else {
        snprintf(buf, sizeof(buf), "Running tick %d/%d   Player: (%d,%d)",
                 v->test_tick, t->duration,
                 v->state.player.x, v->state.player.y);
        DrawText(buf, bx + 8, by + 42, 11, COL_TEXT_DIM);
    }
}

/* ======================================================================== */
/* Policy pipe mode — read actions from stdin, write obs to stdout          */
/* ======================================================================== */

static int read_policy_actions(ViewerState* v) {
    for (int i = 0; i < FC_PUFFER_NUM_ATNS; i++) {
        int action;
        if (scanf("%d", &action) != 1)
            return 0;
        v->actions[i] = action;
    }
    for (int i = FC_PUFFER_NUM_ATNS; i < FC_NUM_ACTION_HEADS; i++) v->actions[i] = 0;
    return 1;
}

static void write_obs_to_pipe(ViewerState* v) {
    /* Write the same policy obs + action mask contract used by Puffer training. */
    float obs_buf[FC_OBS_SIZE];
    fc_write_obs(&v->state, obs_buf);
    /* Mirror training-time obs ablation so the policy sees the distribution
     * it was trained on (no-op when all flags are 0). */
    fc_apply_obs_ablation(obs_buf,
                          v->obs_ablate_npc_distance,
                          v->obs_ablate_incoming_aggregates,
                          v->obs_ablate_npc_valid);
    float mask_buf[FC_ACTION_MASK_SIZE];
    fc_write_mask(&v->state, mask_buf);

    /* Policy obs: first FC_POLICY_OBS_SIZE floats */
    for (int i = 0; i < FC_POLICY_OBS_SIZE; i++)
        printf("%.6f ", obs_buf[i]);
    for (int i = 0; i < FC_PUFFER_MASK_SIZE; i++)
        printf("%.6f ", mask_buf[i]);
    printf("\n");
    fflush(stdout);
}

/* Entity ground Y — slightly above terrain so entities stand on the flattened cracks */
static float ground_y(ViewerState* v, int tile_x, int tile_y) {
    if (v->terrain && v->terrain->loaded) {
        return terrain_height_at(v->terrain, tile_x, tile_y) + 0.1f;
    }
    return 0.0f;
}

typedef struct {
    float x;
    float y;
    float face_angle;
    int moving;
} EntityRenderPose;

static int sign_i(int v) {
    return (v > 0) - (v < 0);
}

static int max_i(int a, int b) {
    return a > b ? a : b;
}

static float lerp_f(float a, float b, float t) {
    return a + (b - a) * t;
}

static float face_angle_between_tiles(float ax, float ay, float bx, float by) {
    float dx = bx - ax;
    float dy = by - ay;
    if (dx == 0.0f && dy == 0.0f) return 0.0f;
    return atan2f(dx, -dy) * (180.0f / 3.14159f);
}

static int player_route_midpoint(const ViewerState* v,
                                 int prev_x, int prev_y,
                                 int cur_x, int cur_y,
                                 int* mid_x, int* mid_y) {
    if (!v || !mid_x || !mid_y) return 0;
    if (v->prev_player_route_idx < 0 ||
        v->prev_player_route_idx >= v->prev_player_route_len) {
        return 0;
    }

    int rx = v->prev_player_route_x;
    int ry = v->prev_player_route_y;
    int step_dx = rx - prev_x;
    int step_dy = ry - prev_y;
    if (max_i(abs(step_dx), abs(step_dy)) != 1) return 0;
    if (rx == cur_x && ry == cur_y) return 0;

    *mid_x = rx;
    *mid_y = ry;
    return 1;
}

static EntityRenderPose entity_render_pose(const ViewerState* v,
                                           const FcRenderEntity* e) {
    EntityRenderPose pose = {0};
    if (!v || !e) return pose;

    float prev_fx;
    float prev_fy;
    if (e->entity_type == ENTITY_PLAYER) {
        prev_fx = v->prev_player_x;
        prev_fy = v->prev_player_y;
    } else {
        prev_fx = v->prev_npc_x[e->npc_slot];
        prev_fy = v->prev_npc_y[e->npc_slot];
    }

    int prev_x = (int)prev_fx;
    int prev_y = (int)prev_fy;
    int cur_x = e->x;
    int cur_y = e->y;
    int dx = cur_x - prev_x;
    int dy = cur_y - prev_y;
    int steps = max_i(abs(dx), abs(dy));
    float t = v->tick_frac;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    if (e->entity_type == ENTITY_PLAYER && v->policy_pipe &&
        player_visual_lock_active(v) && (dx != 0 || dy != 0)) {
        const float hold = 0.45f;
        t = (t < hold) ? 0.0f : (t - hold) / (1.0f - hold);
    }

    float tile_x = prev_fx;
    float tile_y = prev_fy;
    float seg_ax = prev_fx;
    float seg_ay = prev_fy;
    float seg_bx = (float)cur_x;
    float seg_by = (float)cur_y;

    pose.moving = (dx != 0 || dy != 0);
    if (pose.moving && steps > 1) {
        int mid_x;
        int mid_y;
        int has_mid = 0;
        if (e->entity_type == ENTITY_PLAYER) {
            has_mid = player_route_midpoint(v, prev_x, prev_y, cur_x, cur_y,
                                            &mid_x, &mid_y);
        }
        if (!has_mid) {
            mid_x = prev_x + sign_i(dx);
            mid_y = prev_y + sign_i(dy);
        }

        if (t < 0.5f) {
            float st = t * 2.0f;
            tile_x = lerp_f(prev_fx, (float)mid_x, st);
            tile_y = lerp_f(prev_fy, (float)mid_y, st);
            seg_ax = prev_fx;
            seg_ay = prev_fy;
            seg_bx = (float)mid_x;
            seg_by = (float)mid_y;
        } else {
            float st = (t - 0.5f) * 2.0f;
            tile_x = lerp_f((float)mid_x, (float)cur_x, st);
            tile_y = lerp_f((float)mid_y, (float)cur_y, st);
            seg_ax = (float)mid_x;
            seg_ay = (float)mid_y;
            seg_bx = (float)cur_x;
            seg_by = (float)cur_y;
        }
    } else {
        tile_x = lerp_f(prev_fx, (float)cur_x, t);
        tile_y = lerp_f(prev_fy, (float)cur_y, t);
    }

    pose.x = tile_x + (float)e->size * 0.5f;
    pose.y = tile_y + (float)e->size * 0.5f;
    pose.face_angle = face_angle_between_tiles(seg_ax, seg_ay, seg_bx, seg_by);
    return pose;
}

static Vector3 camera_follow_target(const ViewerState* v) {
    float cx = FC_ARENA_WIDTH * 0.5f;
    float cy = -(FC_ARENA_HEIGHT * 0.5f);
    if (v->entity_count > 0) {
        EntityRenderPose pose = entity_render_pose(v, &v->entities[0]);
        cx = pose.x;
        cy = -pose.y;
    }
    return (Vector3){cx, 0.5f, cy};
}

/* ======================================================================== */
/* Scene drawing                                                             */
/* ======================================================================== */

static int object_anim_row_visible(const ObjectAnimPlacement* row) {
    if (!row) return 0;
    return (row->flags & OANM_FLAG_DYNAMIC_REPLACEMENT) == 0;
}

static void draw_animated_objects(ViewerState* v) {
    if (!v || !v->object_anims || !v->object_anims->loaded ||
        !v->object_anim_models || !v->object_anim_runtimes)
        return;

    float dt = GetFrameTime();
    rlDisableBackfaceCulling();
    for (int i = 0; i < v->object_anims->count; i++) {
        ObjectAnimPlacement* row = &v->object_anims->rows[i];
        if (!object_anim_row_visible(row)) continue;

        NpcModelEntry* entry = fc_npc_model_find(v->object_anim_models,
                                                 row->model_id);
        if (!entry || !entry->loaded) continue;

        if (row->animation_id >= 0 && v->anim_cache) {
            ObjectAnimRuntime* rt = &v->object_anim_runtimes[i];
            update_entry_animation(entry, v->anim_cache, &rt->anim_state,
                                   &rt->anim_seq, &rt->anim_frame,
                                   &rt->anim_timer, row->animation_id, dt,
                                   row->phase_ticks);
        }

        DrawModelEx(entry->model,
                    (Vector3){row->pos_x, row->pos_y, row->pos_z},
                    (Vector3){0, 1, 0}, 0.0f,
                    (Vector3){1, 1, 1}, WHITE);
    }
    rlEnableBackfaceCulling();
}

static void draw_scene(ViewerState* v) {
    if (v->camera_locked) {
        v->camera.target = camera_follow_target(v);
    }
    v->camera.position = (Vector3){
        v->camera.target.x + v->cam_dist*cosf(v->cam_pitch)*sinf(v->cam_yaw),
        v->cam_dist*sinf(v->cam_pitch),
        v->camera.target.z + v->cam_dist*cosf(v->cam_pitch)*cosf(v->cam_yaw) };
    BeginMode3D(v->camera);

    /* Terrain + objects */
    if (v->terrain && v->terrain->loaded) {
        rlDisableBackfaceCulling();
        DrawModel(v->terrain->model, (Vector3){0,0,0}, 1.0f, WHITE);
        rlEnableBackfaceCulling();
    }
    if (v->objects && v->objects->loaded) {
        rlDisableBackfaceCulling();
        DrawModel(v->objects->model, (Vector3){0,0,0}, 1.0f, WHITE);
        rlEnableBackfaceCulling();
    }
    draw_animated_objects(v);

    /* Grid overlay */
    if (v->show_grid) {
        for (int x = 0; x <= FC_ARENA_WIDTH; x++)
            DrawLine3D((Vector3){(float)x,0.01f,0}, (Vector3){(float)x,0.01f,-(float)FC_ARENA_HEIGHT}, COL_GRID);
        for (int z = 0; z <= FC_ARENA_HEIGHT; z++)
            DrawLine3D((Vector3){0,0.01f,-(float)z}, (Vector3){(float)FC_ARENA_WIDTH,0.01f,-(float)z}, COL_GRID);
    }

    /* Collision overlay */
    if (v->show_collision) {
        for (int tx = 0; tx < FC_ARENA_WIDTH; tx++) {
            for (int ty = 0; ty < FC_ARENA_HEIGHT; ty++) {
                Color c = v->state.walkable[tx][ty] ? COL_WALKABLE : COL_BLOCKED;
                DrawCube((Vector3){tx+0.5f, 0.02f, -(ty+0.5f)}, 0.9f, 0.02f, 0.9f, c);
            }
        }
    }

    /* Entities */
    int target_entity_idx = -1;  /* for highlight ring */
    EntityRenderPose player_pose = {0};
    if (v->entity_count > 0) {
        player_pose = entity_render_pose(v, &v->entities[0]);
    }
    for (int i = 0; i < v->entity_count; i++) {
        FcRenderEntity* e = &v->entities[i];

        EntityRenderPose pose = entity_render_pose(v, e);
        float ex = pose.x;
        float ey = -pose.y;

        /* Sample terrain height at entity position */
        float gy = ground_y(v, e->x, e->y);

        if (e->entity_type == ENTITY_PLAYER) {
            /* Player model or fallback cylinder */
            NpcModelEntry* pm = viewer_player_model_entry(v);
            if (pm && pm->loaded) {
                Vector3 pos = {ex, gy, ey};
                float face_angle = player_visual_lock_active(v)
                    ? v->player_visual_lock_face_angle
                    : v->state.player.facing_angle;
                if (!player_visual_lock_active(v) &&
                    pose.moving && v->tick_frac < 0.98f) {
                    face_angle = pose.face_angle;
                }
                rlDisableBackfaceCulling();
                DrawModelEx(pm->model, pos, (Vector3){0,1,0}, face_angle, (Vector3){1,1,1}, WHITE);
                rlEnableBackfaceCulling();
            } else {
                DrawCylinder((Vector3){ex, gy, ey}, 0.4f, 0.4f, 2.0f, 8, COL_PLAYER);
                DrawCylinderWires((Vector3){ex, gy, ey}, 0.4f, 0.4f, 2.0f, 8, WHITE);
            }

            /* Prayer icon above player — rendered as 2D text after EndMode3D */
            /* (handled below in the 2D overlay section) */
        } else if (!e->is_dead) {
            /* NPC: try to render actual model, fallback to colored cube */
            uint32_t mid = fc_npc_type_to_model_id(e->npc_type);
            NpcModelEntry* nme = v->npc_models ? fc_npc_model_find(v->npc_models, mid) : NULL;

            if (nme) {
                /* Render NPC model facing movement while walking, otherwise toward player. */
                Vector3 pos = {ex, gy, ey};
                float player_ex = player_pose.x;
                float player_ey = -player_pose.y;
                float face_angle = atan2f(player_ex - ex, player_ey - ey) * (180.0f / 3.14159f);
                if (pose.moving && v->tick_frac < 0.98f) {
                    face_angle = pose.face_angle;
                }
                rlDisableBackfaceCulling();
                DrawModelEx(nme->model, pos, (Vector3){0,1,0}, face_angle, (Vector3){1,1,1}, WHITE);
                rlEnableBackfaceCulling();
            } else {
                /* Fallback: colored cube */
                float s = (float)e->size * 0.45f;
                float h = 1.0f + (float)e->size * 0.5f;
                Color col = (e->npc_type > 0 && e->npc_type < 9) ? NPC_COLORS[e->npc_type] : GRAY;
                if (e->died_this_tick) { h *= 0.3f; col.a = 100; }
                DrawCube((Vector3){ex, gy + h*0.5f, ey}, s*2, h, s*2, col);
                DrawCubeWires((Vector3){ex, gy + h*0.5f, ey}, s*2, h, s*2, WHITE);
            }

            /* Track attack target for highlight */
            if (i > 0 && (i-1) == v->attack_target) target_entity_idx = i;
        }

        /* Blue circle indicator under NPC for visibility */
        if (e->entity_type == ENTITY_NPC && !e->is_dead) {
            float cr = (float)e->size * 0.5f;
            if (cr < 0.4f) cr = 0.4f;
            DrawCircle3D((Vector3){ex, gy + 0.05f, ey}, cr,
                         (Vector3){1,0,0}, 90.0f, CLITERAL(Color){80, 180, 255, 255});
        }

        /* HP bar — floating above entity */
        if (e->max_hp > 0 && !e->is_dead) {
            float hf = (float)e->current_hp/(float)e->max_hp;
            float bar_h = (e->entity_type==ENTITY_PLAYER) ? 1.0f + (float)e->size*0.5f : 1.0f + (float)e->size*0.5f;
            float by = gy + bar_h + 0.8f;
            float bw = (float)e->size*0.8f;
            if (bw < 0.6f) bw = 0.6f;
            DrawCube((Vector3){ex,by,ey}, bw,0.08f,0.08f, COL_HP_RED);
            DrawCube((Vector3){ex-bw*(1.0f-hf)*0.5f,by,ey}, bw*hf,0.08f,0.08f, COL_HP_GREEN);
        }
    }

    /* Attack target highlight ring */
    if (target_entity_idx >= 0) {
        FcRenderEntity* e = &v->entities[target_entity_idx];
        float ex = (float)e->x + (float)e->size*0.5f;
        float ey = -((float)e->y + (float)e->size*0.5f);
        float r = (float)e->size * 0.6f;
        DrawCircle3D((Vector3){ex, 0.05f, ey}, r, (Vector3){1,0,0}, 90.0f, COL_TARGET);
    }

    /* Draw active visual projectiles — use cache-backed spotanim models when present. */
    for (int pi = 0; pi < MAX_PROJECTILES; pi++) {
        VisualProjectile* vp = &v->projectiles[pi];
        if (!vp->active) continue;
        if (vp->spot_id == 0) continue;
        float t = (vp->total_time > 0) ? vp->elapsed / vp->total_time : 1.0f;
        if (t > 1.0f) t = 1.0f;
        float px = vp->src_x + (vp->dst_x - vp->src_x) * t;
        float py = vp->src_y + (vp->dst_y - vp->src_y) * t;
        float pz = vp->src_z + (vp->dst_z - vp->src_z) * t;

        if (vp->profiled) {
            float client_time = t * (vp->client_end_time + 1.0f);
            if (client_time < vp->client_start_time)
                continue;

            float sx = vp->src_x;
            float sy = vp->src_y;
            float sz = vp->src_z;
            float dx = vp->dst_x - sx;
            float dz = vp->dst_z - sz;
            float horizontal = sqrtf(dx * dx + dz * dz);
            float dir_x = 0.0f;
            float dir_z = 1.0f;
            if (horizontal > 0.00001f) {
                dir_x = dx / horizontal;
                dir_z = dz / horizontal;
            }

            float start_pos = vp->projectile_progress >= 0.0f
                ? vp->projectile_progress / 128.0f : 0.0f;
            sx += dir_x * start_pos;
            sz += dir_z * start_pos;

            float travel_time = vp->client_end_time + 1.0f -
                                vp->client_start_time;
            if (travel_time < 1.0f) travel_time = 1.0f;
            float client_t = client_time - vp->client_start_time;
            if (client_t < 0.0f) client_t = 0.0f;
            if (client_t > travel_time) client_t = travel_time;

            float speed_x = (vp->dst_x - sx) / travel_time;
            float speed_z = (vp->dst_z - sz) / travel_time;
            float horizontal_speed = sqrtf(speed_x * speed_x +
                                           speed_z * speed_z);
            float slope = vp->projectile_angle >= 0.0f
                ? vp->projectile_angle : 15.0f;
            float speed_y = horizontal_speed *
                tanf(slope * (3.14159265f / 128.0f));
            float accel_y = 2.0f * (vp->dst_y - sy -
                                    speed_y * travel_time) /
                            (travel_time * travel_time);

            px = sx + speed_x * client_t;
            py = sy + speed_y * client_t +
                 0.5f * accel_y * client_t * client_t;
            pz = sz + speed_z * client_t;
        }

        const SpotAnimDef* spot = NULL;
        NpcModelEntry* pm = projectile_model_for_spot(v, vp->spot_id, &spot);
        if (pm && pm->loaded) {
            /* Rotate projectile to face travel direction */
            float ddx = vp->dst_x - vp->src_x;
            float ddz = vp->dst_z - vp->src_z;
            float angle = atan2f(ddx, ddz) * (180.0f / 3.14159f);
            float scale_xy = (spot && spot->resize_xy > 0)
                ? (float)spot->resize_xy / 128.0f : 1.0f;
            float scale_z = (spot && spot->resize_z > 0)
                ? (float)spot->resize_z / 128.0f : 1.0f;
            if (spot) angle += (float)spot->rotation;
            if (spot && spot->animation_id >= 0) {
                update_entry_animation(pm, v->anim_cache, &vp->anim_state,
                                       &vp->anim_seq, &vp->anim_frame,
                                       &vp->anim_timer, spot->animation_id,
                                       GetFrameTime(), 0.0f);
            }
            rlDisableBackfaceCulling();
            DrawModelEx(pm->model, (Vector3){px, py, pz},
                        (Vector3){0,1,0}, angle,
                        (Vector3){scale_xy, scale_z, scale_xy}, WHITE);
            rlEnableBackfaceCulling();
        } else if (vp->radius > 0.0f) {
            DrawSphere((Vector3){px, py, pz}, vp->radius, vp->color);
        }
    }

    for (int ei = 0; ei < MAX_VISUAL_EFFECTS; ei++) {
        VisualEffect* fx = &v->effects[ei];
        if (!fx->active) continue;
        const SpotAnimDef* spot = NULL;
        NpcModelEntry* pm = projectile_model_for_spot(v, fx->spot_id, &spot);
        if (pm && pm->loaded) {
            float scale_xy = (spot && spot->resize_xy > 0)
                ? (float)spot->resize_xy / 128.0f : 1.0f;
            float scale_z = (spot && spot->resize_z > 0)
                ? (float)spot->resize_z / 128.0f : 1.0f;
            if (spot && spot->animation_id >= 0) {
                update_entry_animation(pm, v->anim_cache, &fx->anim_state,
                                       &fx->anim_seq, &fx->anim_frame,
                                       &fx->anim_timer, spot->animation_id,
                                       GetFrameTime(), 0.0f);
            }
            rlDisableBackfaceCulling();
            DrawModelEx(pm->model, (Vector3){fx->x, fx->y, fx->z},
                        (Vector3){0,1,0},
                        fx->yaw_degrees + (spot ? (float)spot->rotation : 0.0f),
                        (Vector3){scale_xy, scale_z, scale_xy}, WHITE);
            rlEnableBackfaceCulling();
        } else {
            DrawSphere((Vector3){fx->x, fx->y, fx->z},
                       fx->radius, fx->color);
        }
    }

    /* Debug overlays — 3D collision tiles (before EndMode3D) */
    if (v->dbg_flags) debug_overlay_3d(&v->state, v->dbg_flags);

    EndMode3D();

    /* Debug overlays — 2D screen-space (LOS, path, range — after EndMode3D) */
    if (v->dbg_flags) debug_overlay_screen(&v->state, v->camera, v->dbg_flags);

    /* Hitsplats — 2D screen-space damage/status numbers projected from 3D.
     * Drawn AFTER EndMode3D so they render on top of everything.
     * Red circle + white number for damage, blue circle + "0" for miss. */
    for (int i = 0; i < MAX_HITSPLATS; i++) {
        Hitsplat* h = &v->hitsplats[i];
        if (!h->active) continue;

        /* Float upward over lifetime */
        float rise = (float)(60 - h->frames_left) * 0.02f;
        float alpha = (float)h->frames_left / 60.0f;

        /* Project 3D position to 2D screen coordinates */
        Vector3 world_pos = {h->world_x, h->world_y + rise, h->world_z};
        Vector2 screen = GetWorldToScreen(world_pos, v->camera);

        /* Skip if off-screen */
        if (screen.x < -50 || screen.x > WINDOW_W + 50 ||
            screen.y < -50 || screen.y > WINDOW_H + 50) continue;

        int value = h->kind == HITSPLAT_DAMAGE
            ? h->damage / 10
            : (h->damage + 9) / 10;
        int sx = (int)screen.x;
        int sy = (int)screen.y;

        /* Draw OSRS-style splat: colored circle background + damage number */
        Color bg_col, text_col;
        if (h->kind == HITSPLAT_HEAL) {
            bg_col = (Color){24, 130, 42, (unsigned char)(alpha * 230)};
            text_col = (Color){255, 255, 255, (unsigned char)(alpha * 255)};
        } else if (h->kind == HITSPLAT_PRAYER_DRAIN) {
            bg_col = (Color){28, 104, 170, (unsigned char)(alpha * 230)};
            text_col = (Color){255, 255, 255, (unsigned char)(alpha * 255)};
        } else if (h->damage > 0) {
            bg_col = (Color){187, 0, 0, (unsigned char)(alpha * 230)};      /* dark red */
            text_col = (Color){255, 255, 255, (unsigned char)(alpha * 255)}; /* white */
        } else {
            bg_col = (Color){0, 100, 200, (unsigned char)(alpha * 230)};     /* blue */
            text_col = (Color){255, 255, 255, (unsigned char)(alpha * 255)};
        }

        /* Circle background */
        DrawCircle(sx, sy, 12, bg_col);
        DrawCircleLines(sx, sy, 12, (Color){0, 0, 0, (unsigned char)(alpha * 200)});

        /* Damage number centered in circle */
        char dmg_str[16];
        if (h->kind == HITSPLAT_HEAL) {
            snprintf(dmg_str, sizeof(dmg_str), "+%d", value);
        } else if (h->kind == HITSPLAT_PRAYER_DRAIN) {
            snprintf(dmg_str, sizeof(dmg_str), "-%d", value);
        } else {
            snprintf(dmg_str, sizeof(dmg_str), "%d", value);
        }
        int tw = MeasureText(dmg_str, 14);
        DrawText(dmg_str, sx - tw/2, sy - 7, 14, text_col);
    }

    /* Prayer overhead icon — 2D projected from player head position */
    if (v->entity_count > 0 && v->entities[0].prayer != PRAYER_NONE) {
        EntityRenderPose pose = entity_render_pose(v, &v->entities[0]);
        float p_gy = ground_y(v, v->entities[0].x, v->entities[0].y);
        Vector3 head_pos = {pose.x, p_gy + 3.0f, -pose.y};
        Vector2 scr = GetWorldToScreen(head_pos, v->camera);
        int px = (int)scr.x, py = (int)scr.y;

        /* Draw actual prayer sprite texture */
        Texture2D tex = {0};
        if (v->entities[0].prayer == PRAYER_PROTECT_MELEE && v->pray_melee_tex.id > 0)
            tex = v->pray_melee_tex;
        else if (v->entities[0].prayer == PRAYER_PROTECT_RANGE && v->pray_missiles_tex.id > 0)
            tex = v->pray_missiles_tex;
        else if (v->entities[0].prayer == PRAYER_PROTECT_MAGIC && v->pray_magic_tex.id > 0)
            tex = v->pray_magic_tex;

        if (tex.id > 0) {
            /* Scale sprite to ~28x28 pixels and center on projected position */
            float scale = 28.0f / (float)tex.width;
            int dw = (int)(tex.width * scale);
            int dh = (int)(tex.height * scale);
            DrawTextureEx(tex, (Vector2){(float)(px - dw/2), (float)(py - dh/2)},
                          0.0f, scale, WHITE);
        } else {
            /* Fallback: letter if textures not loaded */
            const char* icon_txt = "?";
            if (v->entities[0].prayer == PRAYER_PROTECT_MELEE) icon_txt = "M";
            else if (v->entities[0].prayer == PRAYER_PROTECT_RANGE) icon_txt = "R";
            else icon_txt = "W";
            DrawCircle(px, py, 14, (Color){255,255,255,220});
            int itw = MeasureText(icon_txt, 18);
            DrawText(icon_txt, px - itw/2, py - 9, 18, (Color){0,0,0,255});
        }
    }
}

/* ======================================================================== */
/* UI drawing                                                                */
/* ======================================================================== */

static void draw_header(ViewerState* v) {
    DrawRectangle(0,0,WINDOW_W,HEADER_HEIGHT,COL_HEADER);
    char b[128];
    char speed_label[32];
    const char* mode = v->policy_pipe ? "REPLAY" : (v->auto_mode ? "AUTO" : "PLAY");
    const char* cam_mode = v->camera_locked ? "LOCK" : "FREE";
    format_speed_label(v, speed_label, sizeof(speed_label));
    snprintf(b,sizeof(b),"Fight Caves — Wave %d/%d  [%s]",
        v->state.current_wave, FC_NUM_WAVES, mode);
    text_s(b,10,4,16,COL_TEXT_YELLOW);
    snprintf(b,sizeof(b),"Ep:%d Seed:%u Tick:%d %s Cam:%s",
        v->episode_count, v->seed, v->state.tick, speed_label, cam_mode);
    text_s(b,10,22,10,COL_TEXT_DIM);
    FcPlayer* p=&v->state.player;
    snprintf(b,sizeof(b),"HP %d/%d  Pray %d/%d",
        p->current_hp/10, p->max_hp/10, p->current_prayer/10, p->max_prayer/10);
    text_s(b,WINDOW_W-MeasureText(b,14)-10,8,14,COL_TEXT_WHITE);
}

static Rectangle runec_side_content_rect(const ViewerState* v) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    if (v && v->ui.decoded_ui_enabled && v->ui.decoded_ui_ready) {
        Rectangle screen = {0, 0, (float)screen_w, (float)screen_h};
        Rectangle rect = {0};
        if (runec_ui_interfaces_component_rect_by_id(
                &v->ui.interfaces, "toplevel_osrs_stretch",
                VIEWER_RUNEC_TOP_SIDE_CONTAINER, screen, &rect)) {
            return rect;
        }
    }

    Rectangle side = {
        (float)screen_w - RUNEC_OSRS_SIDE_MENU_W,
        (float)screen_h - RUNEC_OSRS_SIDE_MENU_H,
        RUNEC_OSRS_SIDE_MENU_W,
        RUNEC_OSRS_SIDE_MENU_H
    };
    return (Rectangle){
        side.x + RUNEC_OSRS_SIDE_CONTENT_X,
        side.y + RUNEC_OSRS_SIDE_CONTENT_Y,
        RUNEC_OSRS_SIDE_CONTENT_W,
        RUNEC_OSRS_SIDE_CONTENT_H
    };
}

static void draw_runec_debug_friends_tab(ViewerState* v, Rectangle content) {
    BeginScissorMode((int)content.x, (int)content.y,
                     (int)content.width, (int)content.height);

    int px = (int)content.x;
    int x = px + 4;
    int by = (int)content.y + 3;
    int pw = (int)content.width;
    char b[160];
    char speed_label[32];
    format_speed_label(v, speed_label, sizeof(speed_label));

    snprintf(b, sizeof(b), "%s %s  Wave %d  Tick %d",
             v->paused ? "PAUSED" : "RUN", speed_label,
             v->state.current_wave, v->state.tick);
    DrawText(b, x, by, 8, COL_TEXT_YELLOW); by += 10;
    snprintf(b, sizeof(b), "P:(%d,%d) Ent:%d Hash:%08x",
             v->state.player.x, v->state.player.y,
             v->entity_count, v->last_hash);
    DrawText(b, x, by, 8, COL_TEXT_DIM); by += 10;
    snprintf(b, sizeof(b), "Dmg +%d/-%d Kills:%d%s",
             v->state.damage_dealt_this_tick / 10,
             v->state.damage_taken_this_tick / 10,
             v->state.npcs_killed_this_tick,
             v->godmode ? " GOD" : "");
    DrawText(b, x, by, 8, v->godmode ? COL_TEXT_YELLOW : COL_TEXT_DIM);
    by += 12;

    if (v->dbg_flags) {
        v->dbg_tab_y = by;
        dbg_draw_panel_tabs(&v->state, &v->reward_params,
                            &v->reward_breakdown, &v->reward_runtime,
                            v->reward_config_loaded,
                            v->reward_config_path,
                            px, x, by, pw, v->dbg_tab);
    } else {
        v->dbg_tab_y = 0;
        DrawLine(px + 4, by, px + pw - 4, by, COL_PANEL_BORDER);
        by += 10;
        DrawText("Debug overlay off", x, by, 9, COL_TEXT_DIM);
    }

    EndScissorMode();
}

#define CLAN_NPC_MAX_ROWS 8
#define CLAN_NPC_ROW_H 18
#define CLAN_SPEED_BTN_COLS 4
#define CLAN_SPEED_BTN_H 16
#define CLAN_SPEED_BTN_GAP 4
#define CLAN_SPEED_BOX_PAD 5
#define CLAN_SPEED_LABEL_H 16

static int clan_visible_npc_count(const ViewerState* v) {
    if (!v) return 0;
    int shown = 0;
    for (int ni = 0; ni < FC_MAX_NPCS && shown < CLAN_NPC_MAX_ROWS; ni++) {
        const FcNpc* n = &v->state.npcs[ni];
        if (!n->active || n->is_dead)
            continue;
        shown++;
    }
    return shown;
}

static Rectangle clan_wave_button_rect(const ViewerState* v, Rectangle content) {
    int shown = clan_visible_npc_count(v);
    int y = (int)content.y + 4 + 16;
    y += shown > 0 ? shown * CLAN_NPC_ROW_H : 16;
    y += 7;
    int btn_rows = (NUM_MANUAL_TPS_PRESETS + CLAN_SPEED_BTN_COLS - 1) /
        CLAN_SPEED_BTN_COLS;
    int speed_box_h = CLAN_SPEED_BOX_PAD * 2 + CLAN_SPEED_LABEL_H +
        btn_rows * CLAN_SPEED_BTN_H + (btn_rows - 1) * CLAN_SPEED_BTN_GAP;
    int max_y = (int)(content.y + content.height) - 22 - 6 - speed_box_h;
    if (y > max_y) y = max_y;
    if (y < (int)content.y + 22) y = (int)content.y + 22;
    return (Rectangle){content.x + 4.0f, (float)y, content.width - 8.0f, 20.0f};
}

static Rectangle clan_speed_box_rect(const ViewerState* v, Rectangle content) {
    Rectangle wave_btn = clan_wave_button_rect(v, content);
    int btn_rows = (NUM_MANUAL_TPS_PRESETS + CLAN_SPEED_BTN_COLS - 1) /
        CLAN_SPEED_BTN_COLS;
    int box_h = CLAN_SPEED_BOX_PAD * 2 + CLAN_SPEED_LABEL_H +
        btn_rows * CLAN_SPEED_BTN_H + (btn_rows - 1) * CLAN_SPEED_BTN_GAP;
    int y = (int)(wave_btn.y + wave_btn.height + 6.0f);
    int max_y = (int)(content.y + content.height) - box_h - 2;
    if (y > max_y) y = max_y;
    if (y < (int)content.y + 2) y = (int)content.y + 2;
    return (Rectangle){content.x + 4.0f, (float)y,
                       content.width - 8.0f, (float)box_h};
}

static Rectangle clan_speed_button_rect(const ViewerState* v, Rectangle content,
                                        int index) {
    Rectangle box = clan_speed_box_rect(v, content);
    int inner_x = (int)box.x + CLAN_SPEED_BOX_PAD;
    int inner_w = (int)box.width - CLAN_SPEED_BOX_PAD * 2;
    int btn_w = (inner_w - CLAN_SPEED_BTN_GAP *
        (CLAN_SPEED_BTN_COLS - 1)) / CLAN_SPEED_BTN_COLS;
    int row = index / CLAN_SPEED_BTN_COLS;
    int col = index % CLAN_SPEED_BTN_COLS;
    int x = inner_x + col * (btn_w + CLAN_SPEED_BTN_GAP);
    int y = (int)box.y + CLAN_SPEED_BOX_PAD + CLAN_SPEED_LABEL_H +
        row * (CLAN_SPEED_BTN_H + CLAN_SPEED_BTN_GAP);
    return (Rectangle){(float)x, (float)y, (float)btn_w,
                       (float)CLAN_SPEED_BTN_H};
}

static int clan_wave_dropdown_visible_count(Rectangle content, Rectangle button) {
    int item_h = 14;
    int available = (int)(button.y - content.y - 5);
    int visible = available / item_h;
    if (visible < 4) visible = 4;
    if (visible > 14) visible = 14;
    if (visible > FC_NUM_WAVES) visible = FC_NUM_WAVES;
    return visible;
}

static Rectangle clan_wave_dropdown_rect(Rectangle content, Rectangle button,
                                         int visible) {
    int item_h = 14;
    int h = visible * item_h;
    int y = (int)button.y - h;
    if (y < (int)content.y + 2)
        y = (int)content.y + 2;
    return (Rectangle){button.x, (float)y, button.width, (float)h};
}

static void clamp_clan_wave_scroll(ViewerState* v, int visible) {
    if (!v) return;
    int max_scroll = FC_NUM_WAVES - visible;
    if (max_scroll < 0) max_scroll = 0;
    if (v->clan_wave_scroll < 0) v->clan_wave_scroll = 0;
    if (v->clan_wave_scroll > max_scroll) v->clan_wave_scroll = max_scroll;
}

static Rectangle runec_legacy_prayer_button_rect(Rectangle content, int index) {
    const int btn_h = 34;
    const int gap = 3;
    int x = (int)content.x + 8;
    int y = (int)content.y + 8 + 34 + index * (btn_h + gap);
    int w = (int)content.width - 16;
    if (w < 120) w = 120;
    return (Rectangle){(float)x, (float)y, (float)w, (float)btn_h};
}

static void draw_runec_legacy_prayer_tab(ViewerState* v, Rectangle content) {
    BeginScissorMode((int)content.x, (int)content.y,
                     (int)content.width, (int)content.height);

    DrawRectangleRec(content, COL_PANEL);

    FcPlayer* p = &v->state.player;
    int x = (int)content.x + 8;
    int by = (int)content.y + 8;
    int right = (int)(content.x + content.width) - 4;
    char b[64];

    snprintf(b, sizeof(b), "Prayer: %d / %d",
             p->current_prayer / 10, p->max_prayer / 10);
    text_s(b, x, by, 10, COL_PRAY_BLUE);
    by += 16;

    if (p->prayer != PRAYER_NONE) {
        int resistance = 60 + 2 * p->prayer_bonus;
        snprintf(b, sizeof(b), "Drain rate: 12 / %d resist", resistance);
        text_s(b, x, by, 8, COL_TEXT_DIM);
    } else {
        text_s("No prayer active", x, by, 8, COL_TEXT_DIM);
    }
    by += 14;

    DrawLine((int)content.x + 4, by - 2, right, by - 2, COL_PANEL_BORDER);

    static const char* pray_names[] = {
        "Prot. Melee", "Prot. Range", "Prot. Magic"
    };
    static const int pray_vals[] = {
        PRAYER_PROTECT_MELEE, PRAYER_PROTECT_RANGE, PRAYER_PROTECT_MAGIC
    };
    Color pray_colors[] = { COL_TEXT_YELLOW, COL_TEXT_GREEN, COL_PRAY_BLUE };
    Texture2D tex_on[] = {
        v->tex_pray_melee_on, v->tex_pray_range_on, v->tex_pray_magic_on
    };
    Texture2D tex_off[] = {
        v->tex_pray_melee_off, v->tex_pray_range_off, v->tex_pray_magic_off
    };

    int no_points = (p->current_prayer <= 0);
    Vector2 mouse = GetMousePosition();
    const Color slot_empty = CLITERAL(Color){30, 26, 20, 255};
    const Color pray_active = CLITERAL(Color){60, 120, 200, 200};
    const Color tab_hover = CLITERAL(Color){72, 63, 51, 255};
    const Color pray_button = CLITERAL(Color){50, 44, 36, 255};
    for (int i = 0; i < 3; i++) {
        Rectangle br = runec_legacy_prayer_button_rect(content, i);
        int is_active = (p->prayer == pray_vals[i]);
        int hovered = CheckCollisionPointRec(mouse, br);

        Color bg;
        if (no_points) {
            bg = slot_empty;
        } else if (is_active) {
            bg = pray_active;
        } else if (hovered) {
            bg = tab_hover;
        } else {
            bg = pray_button;
        }
        DrawRectangleRec(br, bg);
        DrawRectangleLinesEx(br, is_active ? 2 : 1,
                             is_active ? pray_colors[i] : COL_PANEL_BORDER);

        Texture2D icon = is_active ? tex_on[i] : tex_off[i];
        Color icon_tint = no_points ? CLITERAL(Color){80,80,80,255} : WHITE;
        draw_tex_fit(icon, (int)br.x + 4, (int)br.y + 2, 30, 30, icon_tint);

        Color tc = no_points ? COL_TEXT_DIM
            : (is_active ? COL_TEXT_WHITE : pray_colors[i]);
        text_s(pray_names[i], (int)br.x + 38, (int)br.y + 7, 10, tc);

        snprintf(b, sizeof(b), "[%d]", i + 1);
        text_s(b, (int)br.x + 38, (int)br.y + 21, 8, COL_TEXT_DIM);
        if (is_active) {
            text_s("ACTIVE", (int)(br.x + br.width) - 44,
                   (int)br.y + 21, 8, COL_TEXT_WHITE);
        }
    }

    by = (int)runec_legacy_prayer_button_rect(content, 2).y + 34 + 9;
    snprintf(b, sizeof(b), "Prayer bonus: +%d", p->prayer_bonus);
    text_s(b, x, by, 8, COL_TEXT_DIM);

    EndScissorMode();
}

static void draw_runec_clan_npc_tab(ViewerState* v, Rectangle content) {
    BeginScissorMode((int)content.x, (int)content.y,
                     (int)content.width, (int)content.height);

    int x = (int)content.x + 4;
    int by = (int)content.y + 4;
    int row_h = CLAN_NPC_ROW_H;
    int bar_x = x + 66;
    int bar_w = (int)content.width - 92;
    char b[128];
    static const char* NPC_SHORT[] = {"?","Tz-Kih","Tz-Kek","Kek-Sm","Tok-Xil",
        "MejKot","Ket-Zek","Jad","HurKot"};

    v->panel_npc_count = 0;
    DrawText("NPC Targets", x, by, 10, COL_TEXT_YELLOW);
    by += 16;
    DrawLine((int)content.x + 3, by - 3,
             (int)(content.x + content.width - 3), by - 3, COL_PANEL_BORDER);

    int shown = 0;
    Vector2 mouse = GetMousePosition();
    for (int ni = 0; ni < FC_MAX_NPCS && shown < CLAN_NPC_MAX_ROWS; ni++) {
        FcNpc* n = &v->state.npcs[ni];
        if (!n->active || n->is_dead)
            continue;

        Rectangle row = {
            content.x + 2,
            (float)by - 2,
            content.width - 4,
            (float)row_h
        };
        int hovered = CheckCollisionPointRec(mouse, row);
        int is_target = (ni == v->state.player.attack_target_idx);
        if (hovered || is_target) {
            DrawRectangleRec(row, is_target
                ? CLITERAL(Color){82,73,61,190}
                : CLITERAL(Color){52,45,35,180});
        }

        if (v->panel_npc_count < 8) {
            v->panel_npc_slot[v->panel_npc_count] = ni;
            v->panel_npc_y[v->panel_npc_count] = by - 2;
            v->panel_npc_count++;
        }

        const char* nname = (n->npc_type > 0 && n->npc_type < 9) ? NPC_SHORT[n->npc_type] : "?";
        if (is_target)
            DrawText(">", x, by + 1, 9, COL_TEXT_YELLOW);
        snprintf(b, sizeof(b), "%s[%d]", nname, ni);
        DrawText(b, x + 10, by, 8, is_target ? COL_TEXT_YELLOW : COL_TEXT_WHITE);

        float hp_frac = (n->max_hp > 0) ? (float)n->current_hp / (float)n->max_hp : 0.0f;
        if (hp_frac < 0.0f) hp_frac = 0.0f;
        if (hp_frac > 1.0f) hp_frac = 1.0f;
        DrawRectangle(bar_x, by + 1, bar_w, 7, COL_HP_RED);
        DrawRectangle(bar_x, by + 1, (int)((float)bar_w * hp_frac), 7, COL_HP_GREEN);
        snprintf(b, sizeof(b), "%d", n->current_hp / 10);
        DrawText(b, bar_x + bar_w + 3, by, 8, COL_TEXT_DIM);

        int dist = fc_distance_to_npc(v->state.player.x, v->state.player.y, n);
        snprintf(b, sizeof(b), "dist:%d atk:%d", dist, n->attack_timer);
        DrawText(b, x + 10, by + 10, 7, COL_TEXT_DIM);

        by += row_h;
        shown++;
    }

    if (shown == 0) {
        DrawText("No NPCs alive", x, by, 9, COL_TEXT_DIM);
        by += 16;
    }

    Rectangle wave_btn = clan_wave_button_rect(v, content);
    DrawLine((int)content.x + 3, (int)wave_btn.y - 5,
             (int)(content.x + content.width - 3), (int)wave_btn.y - 5,
             COL_PANEL_BORDER);
    int btn_hover = CheckCollisionPointRec(mouse, wave_btn);
    DrawRectangleRec(wave_btn, btn_hover
        ? CLITERAL(Color){72,63,51,230}
        : CLITERAL(Color){42,36,28,220});
    DrawRectangleLinesEx(wave_btn, 1, COL_PANEL_BORDER);
    snprintf(b, sizeof(b), "Jump to Wave: %d", v->state.current_wave);
    DrawText(b, (int)wave_btn.x + 6, (int)wave_btn.y + 5, 9, COL_TEXT_YELLOW);
    DrawText(v->clan_wave_dropdown_open ? "^" : "v",
             (int)(wave_btn.x + wave_btn.width) - 16,
             (int)wave_btn.y + 5, 9, COL_TEXT_DIM);

    if (v->clan_wave_dropdown_open) {
        int item_h = 14;
        int visible = clan_wave_dropdown_visible_count(content, wave_btn);
        clamp_clan_wave_scroll(v, visible);
        Rectangle list = clan_wave_dropdown_rect(content, wave_btn, visible);
        DrawRectangleRec(list, CLITERAL(Color){20,18,14,245});
        DrawRectangleLinesEx(list, 1, COL_PANEL_BORDER);
        for (int i = 0; i < visible; i++) {
            int wave = v->clan_wave_scroll + i + 1;
            if (wave > FC_NUM_WAVES)
                break;
            Rectangle row = {
                list.x + 1.0f,
                list.y + (float)(i * item_h),
                list.width - 2.0f,
                (float)item_h
            };
            int hover = CheckCollisionPointRec(mouse, row);
            int current = (wave == v->state.current_wave);
            if (current)
                DrawRectangleRec(row, CLITERAL(Color){60,80,40,245});
            else if (hover)
                DrawRectangleRec(row, CLITERAL(Color){72,63,51,230});
            snprintf(b, sizeof(b), "Wave %d", wave);
            DrawText(b, (int)row.x + 5, (int)row.y + 2, 9,
                     current ? COL_TEXT_YELLOW : COL_TEXT_WHITE);
        }
    }

    Rectangle speed_box = clan_speed_box_rect(v, content);
    DrawRectangleRec(speed_box, CLITERAL(Color){20, 18, 14, 220});
    DrawRectangleLinesEx(speed_box, 1, COL_PANEL_BORDER);
    DrawText(v->policy_pipe ? "Replay TPS" : "TPS Presets",
             (int)speed_box.x + CLAN_SPEED_BOX_PAD,
             (int)speed_box.y + 4, 9, COL_TEXT_DIM);

    for (int i = 0; i < NUM_MANUAL_TPS_PRESETS; i++) {
        Rectangle br = clan_speed_button_rect(v, content, i);
        int hovered = CheckCollisionPointRec(mouse, br);
        int selected = float_near(v->tps, MANUAL_TPS_PRESETS[i]);
        Color bg = selected ? CLITERAL(Color){82,73,61,255}
            : (hovered ? CLITERAL(Color){72,63,51,255}
                       : CLITERAL(Color){52,45,35,255});
        Color tc = selected ? COL_TEXT_YELLOW : COL_TEXT_WHITE;

        DrawRectangleRec(br, bg);
        DrawRectangleLinesEx(br, 1, COL_PANEL_BORDER);
        DrawText(MANUAL_TPS_LABELS[i],
                 (int)br.x + ((int)br.width -
                     MeasureText(MANUAL_TPS_LABELS[i], 9)) / 2,
                 (int)br.y + 4, 9, tc);
    }

    EndScissorMode();
}

static void draw_runec_debug_tabs(ViewerState* v) {
    Rectangle content = runec_side_content_rect(v);
    if (v->ui.active_tab == RUNEC_UI_TAB_FRIENDS) {
        draw_runec_debug_friends_tab(v, content);
    } else if (v->ui.active_tab == RUNEC_UI_TAB_CLAN_CHAT) {
        draw_runec_clan_npc_tab(v, content);
    } else if (v->ui.active_tab == RUNEC_UI_TAB_PRAYER) {
        v->panel_npc_count = 0;
        v->dbg_tab_y = 0;
        draw_runec_legacy_prayer_tab(v, content);
    } else {
        v->panel_npc_count = 0;
        v->dbg_tab_y = 0;
    }
}

static int process_runec_debug_tab_clicks(ViewerState* v) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return 0;

    Vector2 mouse = GetMousePosition();
    Rectangle content = runec_side_content_rect(v);
    if (!CheckCollisionPointRec(mouse, content))
        return 0;

    if (v->ui.active_tab == RUNEC_UI_TAB_FRIENDS) {
        if (v->dbg_flags && v->dbg_tab_y > 0) {
            int px = (int)content.x;
            int dtab_w = ((int)content.width - 12) / 5;
            int dtab_h = 16;
            int dtab_y = v->dbg_tab_y + 2;
            for (int t = 0; t < 5; t++) {
                int tx = px + 4 + t * dtab_w;
                if (mouse.x >= tx && mouse.x < tx + dtab_w &&
                    mouse.y >= dtab_y && mouse.y < dtab_y + dtab_h) {
                    v->dbg_tab = t;
                    return 1;
                }
            }
        }
        return 1;
    }

    if (v->ui.active_tab == RUNEC_UI_TAB_CLAN_CHAT) {
        FcPlayer* p = &v->state.player;

        Rectangle wave_btn = clan_wave_button_rect(v, content);
        int visible = clan_wave_dropdown_visible_count(content, wave_btn);
        Rectangle wave_list = clan_wave_dropdown_rect(content, wave_btn, visible);
        clamp_clan_wave_scroll(v, visible);

        if (v->clan_wave_dropdown_open && CheckCollisionPointRec(mouse, wave_list)) {
            int item_h = 14;
            int row = (int)((mouse.y - wave_list.y) / (float)item_h);
            int wave = v->clan_wave_scroll + row + 1;
            if (wave >= 1 && wave <= FC_NUM_WAVES) {
                viewer_jump_to_wave(v, wave);
                v->clan_wave_dropdown_open = 0;
            }
            return 1;
        }

        if (CheckCollisionPointRec(mouse, wave_btn)) {
            v->clan_wave_dropdown_open = !v->clan_wave_dropdown_open;
            v->clan_wave_scroll = v->state.current_wave - visible / 2 - 1;
            clamp_clan_wave_scroll(v, visible);
            return 1;
        }

        for (int i = 0; i < NUM_MANUAL_TPS_PRESETS; i++) {
            Rectangle br = clan_speed_button_rect(v, content, i);
            if (!CheckCollisionPointRec(mouse, br))
                continue;
            set_manual_speed(v, MANUAL_TPS_PRESETS[i]);
            v->clan_wave_dropdown_open = 0;
            return 1;
        }

        int by = (int)content.y + 4 + 16;
        int shown = 0;
        for (int ni = 0; ni < FC_MAX_NPCS && shown < CLAN_NPC_MAX_ROWS; ni++) {
            FcNpc* n = &v->state.npcs[ni];
            if (!n->active || n->is_dead)
                continue;
            Rectangle row = {
                content.x + 2.0f,
                (float)by - 2.0f,
                content.width - 4.0f,
                (float)CLAN_NPC_ROW_H
            };
            if (CheckCollisionPointRec(mouse, row)) {
                p->attack_target_idx = ni;
                p->approach_target = 1;
                p->route_len = 0;
                p->route_idx = 0;
                fprintf(stderr, "CLAN TAB CLICK -> ATTACK npc_idx=%d\n", ni);
                return 1;
            }
            by += CLAN_NPC_ROW_H;
            shown++;
        }
        return 1;
    }

    return 0;
}

static int process_runec_clan_wave_scroll(ViewerState* v) {
    if (!v || v->ui.active_tab != RUNEC_UI_TAB_CLAN_CHAT ||
        !v->clan_wave_dropdown_open)
        return 0;

    Rectangle content = runec_side_content_rect(v);
    Vector2 mouse = GetMousePosition();
    Rectangle wave_btn = clan_wave_button_rect(v, content);
    int visible = clan_wave_dropdown_visible_count(content, wave_btn);
    Rectangle wave_list = clan_wave_dropdown_rect(content, wave_btn, visible);
    if (!CheckCollisionPointRec(mouse, wave_list))
        return 0;

    float wheel = GetMouseWheelMove();
    if (wheel == 0.0f)
        return 1;

    v->clan_wave_scroll -= (int)wheel * 3;
    clamp_clan_wave_scroll(v, visible);
    return 1;
}

static int process_runec_prayer_click(ViewerState* v) {
    if (!v || v->ui.active_tab != RUNEC_UI_TAB_PRAYER)
        return 0;
    int left_click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    int right_click = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if (!left_click && !right_click)
        return 0;

    Rectangle content = runec_side_content_rect(v);
    Vector2 mouse = GetMousePosition();
    if (!CheckCollisionPointRec(mouse, content))
        return 0;

    if (right_click)
        return 1;

    static const struct {
        int prayer;
        int action;
    } buttons[] = {
        {PRAYER_PROTECT_MELEE, FC_PRAYER_MELEE},
        {PRAYER_PROTECT_RANGE, FC_PRAYER_RANGE},
        {PRAYER_PROTECT_MAGIC, FC_PRAYER_MAGIC},
    };

    for (int i = 0; i < (int)(sizeof(buttons) / sizeof(buttons[0])); i++) {
        Rectangle r = runec_legacy_prayer_button_rect(content, i);
        if (!CheckCollisionPointRec(mouse, r))
            continue;
        queue_viewer_prayer_button(v, buttons[i].prayer, buttons[i].action);
        return 1;
    }

    return 1;
}

/* ======================================================================== */
/* Side panel tab contents (Phase 8h)                                       */
/* ======================================================================== */

/* Tab button colors */
#define COL_TAB_ACTIVE   CLITERAL(Color){ 82, 73, 61, 255 }
#define COL_TAB_INACTIVE CLITERAL(Color){ 52, 45, 35, 255 }
#define COL_TAB_HOVER    CLITERAL(Color){ 72, 63, 51, 255 }
#define COL_SLOT_BG      CLITERAL(Color){ 40, 35, 28, 255 }
#define COL_SLOT_HOVER   CLITERAL(Color){ 60, 52, 40, 255 }
#define COL_SLOT_EMPTY   CLITERAL(Color){ 30, 26, 20, 255 }
#define COL_PRAY_ACTIVE  CLITERAL(Color){ 60, 120, 200, 200 }
#define COL_PRAY_BUTTON  CLITERAL(Color){ 50, 44, 36, 255 }
#define COL_COMBAT_BTN   CLITERAL(Color){ 50, 44, 36, 255 }

/* Inventory slot dimensions */
#define INV_COLS   4
#define INV_ROWS   7
#define INV_SLOTS  28

/* Helper: draw a texture scaled to fit a rectangle, centered */
static void draw_tex_fit(Texture2D tex, int dx, int dy, int dw, int dh, Color tint) {
    if (tex.id == 0) return;
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    /* Fit within dw x dh preserving aspect ratio */
    float sx = (float)dw / (float)tex.width;
    float sy = (float)dh / (float)tex.height;
    float s = (sx < sy) ? sx : sy;
    int rw = (int)(tex.width * s);
    int rh = (int)(tex.height * s);
    Rectangle dst = { (float)(dx + (dw - rw) / 2), (float)(dy + (dh - rh) / 2), (float)rw, (float)rh };
    DrawTexturePro(tex, src, dst, (Vector2){0,0}, 0, tint);
}

/* Inventory slot layout:
 *   Slots 0-7:   prayer potions (8 pots, 4 doses each)
 *   Slots 8-27:  sharks (20 sharks)
 * Consumed items empty from the end of their group. */
static int draw_inventory_tab(ViewerState* v, int px, int x, int by) {
    FcPlayer* p = &v->state.player;
    char b[32];

    int doses = p->prayer_doses_remaining;  /* 0..32 */
    int sharks = p->sharks_remaining;       /* 0..20 */

    int slot_w = (PANEL_WIDTH - 20) / INV_COLS;
    int slot_h = 32;
    int icon_sz = 20;  /* sprite draw size within slot */

    for (int row = 0; row < INV_ROWS; row++) {
        for (int col = 0; col < INV_COLS; col++) {
            int slot = row * INV_COLS + col;
            int sx = x + col * slot_w;
            int sy = by + row * (slot_h + 2);
            Rectangle sr = { (float)sx, (float)sy, (float)slot_w - 2, (float)slot_h };

            int has_item = 0;
            int is_ppot = 0;
            int ppot_dose = 0;  /* 1-4, or 0 = vial */
            int is_shark = 0;

            if (slot < 8) {
                /* Prayer potion slots (0-7) */
                int full_pots = doses / 4;
                int partial = doses % 4;
                int consumed_pots = 8 - full_pots - (partial > 0 ? 1 : 0);
                if (slot < full_pots) {
                    has_item = 1; is_ppot = 1; ppot_dose = 4;
                } else if (slot == full_pots && partial > 0) {
                    has_item = 1; is_ppot = 1; ppot_dose = partial;
                } else if (slot < 8 && slot >= full_pots + (partial > 0 ? 1 : 0)) {
                    /* Consumed pot → vial */
                    has_item = 1; is_ppot = 1; ppot_dose = 0;
                }
            } else {
                int shark_idx = slot - 8;
                if (shark_idx < sharks) {
                    has_item = 1; is_shark = 1;
                }
            }

            /* Draw slot background */
            int hovered = CheckCollisionPointRec(GetMousePosition(), sr);
            Color bg;
            if (is_ppot && ppot_dose == 0) {
                bg = COL_SLOT_EMPTY;  /* vial = dark */
            } else if (has_item) {
                bg = hovered ? COL_SLOT_HOVER : COL_SLOT_BG;
            } else {
                bg = COL_SLOT_EMPTY;
            }
            DrawRectangleRec(sr, bg);
            DrawRectangleLinesEx(sr, 1, COL_PANEL_BORDER);

            if (is_ppot && ppot_dose > 0) {
                /* Prayer potion with sprite + dose label */
                Color tint = WHITE;
                if (ppot_dose == 3) tint = CLITERAL(Color){220,220,255,255};
                if (ppot_dose == 2) tint = CLITERAL(Color){200,200,240,255};
                if (ppot_dose == 1) tint = CLITERAL(Color){180,180,220,255};
                draw_tex_fit(v->tex_ppot, sx + 1, sy + 1, icon_sz, slot_h - 6, tint);
                snprintf(b, sizeof(b), "(%d)", ppot_dose);
                int tw = MeasureText(b, 9);
                text_s(b, sx + slot_w - 2 - tw - 2, sy + slot_h - 12, 9, COL_PRAY_BLUE);
            } else if (is_ppot && ppot_dose == 0) {
                /* Empty vial */
                text_s("Vial", sx + 6, sy + 10, 8, COL_TEXT_DIM);
            } else if (is_shark) {
                draw_tex_fit(v->tex_shark, sx + 2, sy + 2, slot_w - 6, slot_h - 4, WHITE);
            }
        }
    }
    return by + INV_ROWS * (slot_h + 2);
}

static int draw_combat_tab(ViewerState* v, int px, int x, int by) {
    FcPlayer* p = &v->state.player;
    char b[128];
    int loadout = v->active_loadout;
    if (loadout < 0 || loadout >= FC_NUM_LOADOUTS)
        loadout = FC_ACTIVE_LOADOUT;
    const char* weapon_name = FC_LOADOUTS[loadout].weapon_name;

    /* Weapon */
    text_s(weapon_name, x, by, 10, COL_TEXT_YELLOW); by += 16;
    DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);

    /* Attack styles — read from v->combat_style */
    text_s("Attack style:", x, by, 9, COL_TEXT_DIM); by += 14;
    static const char* styles[] = { "Accurate", "Rapid", "Long range" };
    static const char* style_desc[] = { "+3 Ranged", "1 tick faster", "+2 Defence" };
    for (int i = 0; i < 3; i++) {
        int btn_y = by;
        int btn_w = PANEL_WIDTH - 20;
        int btn_h = 28;
        Rectangle br = { (float)x, (float)btn_y, (float)btn_w, (float)btn_h };
        int hovered = CheckCollisionPointRec(GetMousePosition(), br);
        int selected = (i == v->combat_style);
        Color bg = selected ? COL_TAB_ACTIVE : (hovered ? COL_TAB_HOVER : COL_COMBAT_BTN);
        DrawRectangleRec(br, bg);
        Color border = selected ? COL_TEXT_YELLOW : COL_PANEL_BORDER;
        DrawRectangleLinesEx(br, selected ? 2 : 1, border);
        Color tc = selected ? COL_TEXT_YELLOW : COL_TEXT_WHITE;
        text_s(styles[i], x + 8, btn_y + 4, 10, tc);
        text_s(style_desc[i], x + 8, btn_y + 16, 8, selected ? COL_TEXT_DIM : CLITERAL(Color){100,90,80,255});
        by += btn_h + 3;
    }
    by += 6;

    /* Auto retaliate */
    text_s("Auto retaliate:", x, by, 9, COL_TEXT_DIM);
    text_s("ON", x + 96, by, 9, COL_TEXT_GREEN);
    by += 18;

    /* Weapon stats */
    DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);
    int spd = (v->combat_style == 1) ? 5 : 6;  /* Rapid = 5 ticks, others = 6 */
    snprintf(b, sizeof(b), "Speed: %d ticks (%s)", spd, styles[v->combat_style]);
    text_s(b, x, by, 9, COL_TEXT_DIM); by += 12;
    snprintf(b, sizeof(b), "Range: %d tiles", p->weapon_range);
    text_s(b, x, by, 9, COL_TEXT_DIM); by += 12;
    snprintf(b, sizeof(b), "Ammo: %d bolts", p->ammo_count);
    text_s(b, x, by, 9, COL_TEXT_WHITE); by += 16;

    /* Current target */
    DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);
    if (p->attack_target_idx >= 0) {
        static const char* NPC_NAMES[] = {"?","Tz-Kih","Tz-Kek","Kek-Sm","Tok-Xil",
            "MejKot","Ket-Zek","Jad","HurKot"};
        FcNpc* tgt = &v->state.npcs[p->attack_target_idx];
        const char* name = (tgt->npc_type > 0 && tgt->npc_type < 9) ? NPC_NAMES[tgt->npc_type] : "?";
        snprintf(b, sizeof(b), "Target: %s", name);
        text_s(b, x, by, 10, COL_TEXT_YELLOW); by += 14;
        int bar_w = PANEL_WIDTH - 20;
        float hp_frac = (tgt->max_hp > 0) ? (float)tgt->current_hp / (float)tgt->max_hp : 0;
        DrawRectangle(x, by, bar_w, 10, COL_HP_RED);
        DrawRectangle(x, by, (int)(bar_w * hp_frac), 10, COL_HP_GREEN);
        snprintf(b, sizeof(b), "%d/%d", tgt->current_hp / 10, tgt->max_hp / 10);
        text_s(b, x + bar_w / 2 - MeasureText(b, 9) / 2, by + 1, 9, COL_TEXT_WHITE);
        by += 14;
        int dist = fc_distance_to_npc(p->x, p->y, tgt);
        snprintf(b, sizeof(b), "Distance: %d tiles", dist);
        text_s(b, x, by, 9, COL_TEXT_DIM); by += 12;
    } else {
        text_s("Target: none [Tab]", x, by, 10, COL_TEXT_DIM); by += 14;
    }
    return by;
}

static int draw_prayer_tab(ViewerState* v, int px, int x, int by) {
    FcPlayer* p = &v->state.player;
    char b[64];

    /* Prayer points */
    snprintf(b, sizeof(b), "Prayer: %d / %d", p->current_prayer / 10, p->max_prayer / 10);
    text_s(b, x, by, 10, COL_PRAY_BLUE); by += 16;

    /* Drain info */
    if (p->prayer != PRAYER_NONE) {
        int resistance = 60 + 2 * p->prayer_bonus;
        snprintf(b, sizeof(b), "Drain rate: 12 / %d resist", resistance);
        text_s(b, x, by, 8, COL_TEXT_DIM);
    } else {
        text_s("No prayer active", x, by, 8, COL_TEXT_DIM);
    }
    by += 14;

    DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);
    by += 4;

    /* Prayer buttons — 3 clickable buttons with icon sprites */
    static const char* pray_names[] = { "Prot. Melee", "Prot. Range", "Prot. Magic" };
    static const int pray_vals[] = { PRAYER_PROTECT_MELEE, PRAYER_PROTECT_RANGE, PRAYER_PROTECT_MAGIC };
    Color pray_colors[] = { COL_TEXT_YELLOW, COL_TEXT_GREEN, COL_PRAY_BLUE };

    /* On/off texture pairs indexed by prayer [melee=0, range=1, magic=2] */
    Texture2D tex_on[] = { v->tex_pray_melee_on, v->tex_pray_range_on, v->tex_pray_magic_on };
    Texture2D tex_off[] = { v->tex_pray_melee_off, v->tex_pray_range_off, v->tex_pray_magic_off };

    int btn_w = PANEL_WIDTH - 20;
    int btn_h = 34;
    int no_points = (p->current_prayer <= 0);

    for (int i = 0; i < 3; i++) {
        int btn_y = by;
        Rectangle br = { (float)x, (float)btn_y, (float)btn_w, (float)btn_h };
        int is_active = (p->prayer == pray_vals[i]);
        int hovered = CheckCollisionPointRec(GetMousePosition(), br);

        Color bg;
        if (no_points) {
            bg = COL_SLOT_EMPTY;
        } else if (is_active) {
            bg = COL_PRAY_ACTIVE;
        } else if (hovered) {
            bg = COL_TAB_HOVER;
        } else {
            bg = COL_PRAY_BUTTON;
        }
        DrawRectangleRec(br, bg);
        DrawRectangleLinesEx(br, is_active ? 2 : 1, is_active ? pray_colors[i] : COL_PANEL_BORDER);

        /* Prayer icon sprite */
        Texture2D icon = is_active ? tex_on[i] : tex_off[i];
        Color icon_tint = no_points ? CLITERAL(Color){80,80,80,255} : WHITE;
        draw_tex_fit(icon, x + 4, btn_y + 2, 30, 30, icon_tint);

        /* Label */
        Color tc = no_points ? COL_TEXT_DIM : (is_active ? COL_TEXT_WHITE : pray_colors[i]);
        text_s(pray_names[i], x + 38, btn_y + 7, 10, tc);

        /* Hotkey + status */
        snprintf(b, sizeof(b), "[%d]", i + 1);
        text_s(b, x + 38, btn_y + 21, 8, COL_TEXT_DIM);
        if (is_active) {
            text_s("ACTIVE", x + btn_w - 44, btn_y + 21, 8, COL_TEXT_WHITE);
        }

        by += btn_h + 3;
    }

    by += 6;
    snprintf(b, sizeof(b), "Prayer bonus: +%d", p->prayer_bonus);
    text_s(b, x, by, 8, COL_TEXT_DIM); by += 12;
    return by;
}

/* Handle clicks in the tab area. Returns 1 if click was consumed. */
static int process_tab_click(ViewerState* v, float mx, float my) {
    int px = FC_ARENA_WIDTH * TILE_SIZE;
    int x = px + 8;
    int tab_y = v->tab_area_y;
    if (tab_y <= 0) return 0;  /* tabs not drawn yet */
    FcPlayer* p = &v->state.player;

    /* Tab buttons: 4 equal-width buttons at tab_y */
    int num_tabs = 4;
    int tab_w = (PANEL_WIDTH - 12) / num_tabs;
    int tab_h = 18;
    for (int t = 0; t < num_tabs; t++) {
        int tx = px + 4 + t * tab_w;
        if (mx >= tx && mx < tx + tab_w && my >= tab_y && my < tab_y + tab_h) {
            v->active_tab = t;
            return 1;
        }
    }

    /* Content area starts below tab buttons */
    int content_y = tab_y + tab_h + 4;
    if (my < content_y) return 0;

    if (v->active_tab == 0) {
        /* Inventory tab clicks */
        int slot_w = (PANEL_WIDTH - 20) / INV_COLS;
        int slot_h = 32;
        for (int row = 0; row < INV_ROWS; row++) {
            for (int col = 0; col < INV_COLS; col++) {
                int sx = x + col * slot_w;
                int sy = content_y + row * (slot_h + 2);
                if (mx >= sx && mx < sx + slot_w - 2 && my >= sy && my < sy + slot_h) {
                    int slot = row * INV_COLS + col;
                    if (slot < 8) {
                        /* Prayer potion slot — check if it has doses */
                        int full_pots = p->prayer_doses_remaining / 4;
                        int partial = p->prayer_doses_remaining % 4;
                        if (slot < full_pots || (slot == full_pots && partial > 0)) {
                            v->pending_drink = FC_DRINK_PRAYER_POT;
                            return 1;
                        }
                    } else {
                        /* Shark slot */
                        int shark_idx = slot - 8;
                        if (shark_idx < p->sharks_remaining) {
                            v->pending_eat = FC_EAT_SHARK;
                            return 1;
                        }
                    }
                }
            }
        }
    } else if (v->active_tab == 1) {
        /* Combat tab clicks — attack style buttons */
        /* Skip weapon label (16px) + separator + "Attack style:" (14px) = 30px offset */
        int btn_w = PANEL_WIDTH - 20;
        int btn_h = 28;
        int btn_start = content_y + 30;
        for (int i = 0; i < 3; i++) {
            int btn_y = btn_start + i * (btn_h + 3);
            if (mx >= x && mx < x + btn_w && my >= btn_y && my < btn_y + btn_h) {
                v->combat_style = i;
                return 1;
            }
        }
    } else if (v->active_tab == 2) {
        /* Prayer tab clicks — check prayer buttons */
        int btn_w = PANEL_WIDTH - 20;
        int btn_h = 34;
        /* Skip past prayer points text + drain info + separator (approx 34px) */
        int btn_start = content_y + 34;
        for (int i = 0; i < 3; i++) {
            int btn_y = btn_start + i * (btn_h + 3);
            if (mx >= x && mx < x + btn_w && my >= btn_y && my < btn_y + btn_h) {
                if (p->current_prayer > 0) {
                    static const int pray_enum[] = { PRAYER_PROTECT_MELEE, PRAYER_PROTECT_RANGE, PRAYER_PROTECT_MAGIC };
                    static const int pray_action[] = { FC_PRAYER_MELEE, FC_PRAYER_RANGE, FC_PRAYER_MAGIC };
                    if (p->prayer == pray_enum[i]) {
                        v->pending_prayer = FC_PRAYER_OFF;
                    } else {
                        v->pending_prayer = pray_action[i];
                    }
                    return 1;
                }
            }
        }
    }

    /* Debug panel tab clicks */
    if (v->dbg_flags && v->dbg_tab_y > 0) {
        int dtab_w = (PANEL_WIDTH - 12) / 5;
        int dtab_h = 16;
        int dtab_btn_y = v->dbg_tab_y + 2;
        for (int t = 0; t < 5; t++) {
            int tx = px + 4 + t * dtab_w;
            if (mx >= tx && mx < tx + dtab_w && my >= dtab_btn_y && my < dtab_btn_y + dtab_h) {
                v->dbg_tab = t;
                return 1;
            }
        }
    }

    return 0;
}

/* ======================================================================== */
/* Main panel draw                                                           */
/* ======================================================================== */

/* Draw NPC health bars (clickable to target). Returns Y position after bars. */
static int draw_npc_bars(ViewerState* v, int px, int x, int by) {
    char b[128];
    DrawLine(px+4, by, px+PANEL_WIDTH-4, by, COL_PANEL_BORDER); by += 4;
    v->panel_npc_count = 0;
    static const char* NPC_SHORT[] = {"?","Tz-Kih","Tz-Kek","Kek-Sm","Tok-Xil",
        "MejKot","Ket-Zek","Jad","HurKot"};
    int shown = 0;
    for (int ni = 0; ni < FC_MAX_NPCS && shown < 8; ni++) {
        FcNpc* n = &v->state.npcs[ni];
        if (!n->active || n->is_dead) continue;
        if (v->panel_npc_count < 8) {
            v->panel_npc_slot[v->panel_npc_count] = ni;
            v->panel_npc_y[v->panel_npc_count] = by;
            v->panel_npc_count++;
        }
        const char* nname = (n->npc_type > 0 && n->npc_type < 9) ? NPC_SHORT[n->npc_type] : "?";
        int is_target = (ni == v->state.player.attack_target_idx);
        if (is_target) text_s(">", x, by, 10, COL_TEXT_YELLOW);
        Color name_col = is_target ? COL_TEXT_YELLOW : COL_TEXT_WHITE;
        text_s(nname, x + 10, by, 9, name_col);
        int bar_x = x + 65;
        int bar_w = PANEL_WIDTH - 82;
        float hp_frac = (n->max_hp > 0) ? (float)n->current_hp / (float)n->max_hp : 0;
        DrawRectangle(bar_x, by + 1, bar_w, 8, COL_HP_RED);
        DrawRectangle(bar_x, by + 1, (int)(bar_w * hp_frac), 8, COL_HP_GREEN);
        snprintf(b, sizeof(b), "%d", n->current_hp / 10);
        DrawText(b, bar_x + bar_w + 2, by, 9, COL_TEXT_DIM);
        by += 12;
        shown++;
    }
    if (shown == 0) {
        text_s("No NPCs alive", x, by, 9, COL_TEXT_DIM);
        by += 12;
    }
    return by + 4;
}

static void draw_panel(ViewerState* v) {
    int px=FC_ARENA_WIDTH*TILE_SIZE, py=HEADER_HEIGHT;
    DrawRectangle(px,py,PANEL_WIDTH,WINDOW_H-py,COL_PANEL);
    DrawLine(px,py,px,WINDOW_H,COL_PANEL_BORDER);
    FcPlayer* p=&v->state.player; int x=px+8; char b[128];
    int by=py+6;

    /* HP bar */
    float hf=(p->max_hp>0)?(float)p->current_hp/(float)p->max_hp:0;
    DrawRectangle(x,by,PANEL_WIDTH-16,12,COL_HP_RED);
    DrawRectangle(x,by,(int)((PANEL_WIDTH-16)*hf),12,COL_HP_GREEN);
    snprintf(b,sizeof(b),"HP %d/%d",p->current_hp/10,p->max_hp/10);
    text_s(b,x+(PANEL_WIDTH-16)/2-MeasureText(b,10)/2,by+1,10,COL_TEXT_WHITE); by+=16;

    /* Prayer bar */
    float pf=(p->max_prayer>0)?(float)p->current_prayer/(float)p->max_prayer:0;
    DrawRectangle(x,by,PANEL_WIDTH-16,12,CLITERAL(Color){40,40,80,255});
    DrawRectangle(x,by,(int)((PANEL_WIDTH-16)*pf),12,COL_PRAY_BLUE);
    snprintf(b,sizeof(b),"Pray %d/%d",p->current_prayer/10,p->max_prayer/10);
    text_s(b,x+(PANEL_WIDTH-16)/2-MeasureText(b,10)/2,by+1,10,COL_TEXT_WHITE); by+=18;

    /* Wave info + timers */
    snprintf(b,sizeof(b),"Wave %d/%d  NPCs:%d",v->state.current_wave,FC_NUM_WAVES,v->state.npcs_remaining);
    text_s(b,x,by,10,COL_TEXT_YELLOW); by+=14;
    snprintf(b,sizeof(b),"AtkTmr:%d Food:%d Pot:%d",
        p->attack_timer, p->food_timer, p->potion_timer);
    text_s(b,x,by,10,COL_TEXT_DIM); by+=14;

    /* Terminal state */
    if (v->state.terminal != TERMINAL_NONE) {
        const char* t = "???";
        Color tc = COL_HP_RED;
        switch (v->state.terminal) {
            case TERMINAL_CAVE_COMPLETE: t="VICTORY!"; tc=COL_TEXT_GREEN; break;
            case TERMINAL_PLAYER_DEATH:  t="DEATH"; break;
            case TERMINAL_TICK_CAP:      t="TICK CAP"; break;
        }
        text_s(t, x, by, 16, tc); by += 20;
        text_s("[R] to restart", x, by, 10, COL_TEXT_DIM); by += 16;
    }

    /* ---- Tab buttons ---- */
    DrawLine(px+4, by, px+PANEL_WIDTH-4, by, COL_PANEL_BORDER); by += 2;
    v->tab_area_y = by;

    int num_tabs = 4;
    int tab_w = (PANEL_WIDTH - 12) / num_tabs;
    int tab_h = 18;
    static const char* tab_labels[] = { "Inven", "Combat", "Prayer", "Stats" };
    Texture2D tab_icons[] = { v->tex_tab_inv, v->tex_tab_combat, v->tex_tab_prayer, (Texture2D){0} };
    for (int t = 0; t < num_tabs; t++) {
        int tx = px + 4 + t * tab_w;
        Rectangle tr = { (float)tx, (float)by, (float)tab_w, (float)tab_h };
        int hovered = CheckCollisionPointRec(GetMousePosition(), tr);
        Color bg = (t == v->active_tab) ? COL_TAB_ACTIVE : (hovered ? COL_TAB_HOVER : COL_TAB_INACTIVE);
        DrawRectangleRec(tr, bg);
        Color tc = (t == v->active_tab) ? COL_TEXT_YELLOW : COL_TEXT_DIM;
        if (tab_icons[t].id > 0) {
            Color icon_tint = (t == v->active_tab) ? WHITE : CLITERAL(Color){160,160,160,255};
            draw_tex_fit(tab_icons[t], tx + 2, by + 1, 16, 16, icon_tint);
            text_s(tab_labels[t], tx + 20, by + 5, 8, tc);
        } else {
            int tw = MeasureText(tab_labels[t], 9);
            text_s(tab_labels[t], tx + (tab_w - tw) / 2, by + 5, 9, tc);
        }
        if (t == v->active_tab) {
            DrawLine(tx + 2, by + tab_h - 1, tx + tab_w - 2, by + tab_h - 1, COL_TEXT_YELLOW);
        }
    }
    by += tab_h + 4;

    /* ---- Tab content ---- */
    int tab_end_y = by;
    switch (v->active_tab) {
        case 0: tab_end_y = draw_inventory_tab(v, px, x, by); break;
        case 1: tab_end_y = draw_combat_tab(v, px, x, by); break;
        case 2: tab_end_y = draw_prayer_tab(v, px, x, by); break;
        case 3: {
            /* Stats tab — show current loadout, skills, and equipment */
            char b2[128];
            int lh = 12;
            const FcPlayer* sp = &v->state.player;
            const FcLoadout* lo = &FC_LOADOUTS[v->active_loadout];

            text_s(lo->name, x, by, 10, COL_TEXT_YELLOW); by += lh + 2;

            DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);
            text_s("Skills", x, by, 9, COL_TEXT_DIM); by += lh;
            snprintf(b2,sizeof(b2),"HP: %d/%d  Prayer: %d/%d",
                sp->current_hp/10, sp->max_hp/10, sp->current_prayer/10, sp->max_prayer/10);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh - 1;
            snprintf(b2,sizeof(b2),"Ranged: %d  Defence: %d", sp->ranged_level, sp->defence_level);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh - 1;
            snprintf(b2,sizeof(b2),"Prayer: %d  Magic: %d", sp->prayer_level, sp->magic_level);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh + 2;

            DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);
            text_s("Offence", x, by, 9, COL_TEXT_DIM); by += lh;
            snprintf(b2,sizeof(b2),"Rng Atk: %d  Rng Str: %d", sp->ranged_attack_bonus, sp->ranged_strength_bonus);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh + 2;

            DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);
            text_s("Defence", x, by, 9, COL_TEXT_DIM); by += lh;
            snprintf(b2,sizeof(b2),"Stab: %d  Slash: %d", sp->defence_stab, sp->defence_slash);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh - 1;
            snprintf(b2,sizeof(b2),"Crush: %d  Magic: %d", sp->defence_crush, sp->defence_magic);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh - 1;
            snprintf(b2,sizeof(b2),"Ranged: %d  Prayer: %d", sp->defence_ranged, sp->prayer_bonus);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh + 2;

            DrawLine(px+4, by-2, px+PANEL_WIDTH-4, by-2, COL_PANEL_BORDER);
            text_s("Resources", x, by, 9, COL_TEXT_DIM); by += lh;
            snprintf(b2,sizeof(b2),"Food: %d/%d  Pots: %d/%d",
                sp->sharks_remaining, FC_MAX_SHARKS, sp->prayer_doses_remaining, FC_MAX_PRAYER_DOSES);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh - 1;
            snprintf(b2,sizeof(b2),"Ammo: %d", sp->ammo_count);
            text_s(b2, x, by, 8, COL_TEXT_WHITE); by += lh;

            tab_end_y = by;
            break;
        }
    }

    /* ---- NPC health bars (always visible, below tab content) ---- */
    int npc_end_y = draw_npc_bars(v, px, x, tab_end_y);

    /* ---- Debug info tabs (Phase 9c, shown when O is toggled on) ---- */
    if (v->dbg_flags) {
        v->dbg_tab_y = npc_end_y;
        npc_end_y = dbg_draw_panel_tabs(&v->state, &v->reward_params,
                                        &v->reward_breakdown, &v->reward_runtime,
                                        v->reward_config_loaded,
                                        v->reward_config_path,
                                        px, x, npc_end_y, PANEL_WIDTH, v->dbg_tab);
    } else {
        v->dbg_tab_y = 0;
    }

    int controls_y = npc_end_y + 34;
    static int loadout_open = 0;

    /* ---- Wave jump dropdown ---- */
    {
        int wy = controls_y;
        static int dropdown_open = 0;

        /* Button: "Jump to Wave: N  v" */
        char wbuf[32];
        snprintf(wbuf, sizeof(wbuf), "Jump to Wave: %d", v->state.current_wave);
        Rectangle btn_r = { (float)(px + 8), (float)wy, (float)(PANEL_WIDTH - 16), 20.0f };
        int btn_hover = CheckCollisionPointRec(GetMousePosition(), btn_r);
        DrawRectangleRec(btn_r, btn_hover ? COL_TAB_HOVER : COL_TAB_INACTIVE);
        DrawRectangleLinesEx(btn_r, 1, COL_PANEL_BORDER);
        DrawText(wbuf, px + 14, wy + 4, 10, COL_TEXT_YELLOW);
        DrawText("v", px + PANEL_WIDTH - 22, wy + 4, 10, COL_TEXT_DIM);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btn_hover)
            dropdown_open = !dropdown_open;

        /* Dropdown list */
        if (dropdown_open) {
            int item_h = 16;
            int list_h = FC_NUM_WAVES * item_h;
            int list_y = wy - list_h;  /* open upward so it doesn't go off screen */
            if (list_y < HEADER_HEIGHT) list_y = HEADER_HEIGHT;
            int visible = (wy - list_y) / item_h;

            /* Scroll offset based on current wave (center it) */
            static int scroll = 0;
            int max_visible = visible;
            if (max_visible > FC_NUM_WAVES) max_visible = FC_NUM_WAVES;

            /* Mouse wheel scrolls the list */
            float wheel = GetMouseWheelMove();
            if (wheel != 0 && CheckCollisionPointRec(GetMousePosition(),
                    (Rectangle){(float)(px+8), (float)list_y, (float)(PANEL_WIDTH-16), (float)(wy-list_y)})) {
                scroll -= (int)wheel * 3;
                if (scroll < 0) scroll = 0;
                if (scroll > FC_NUM_WAVES - max_visible) scroll = FC_NUM_WAVES - max_visible;
            }

            /* Background */
            DrawRectangle(px + 8, list_y, PANEL_WIDTH - 16, wy - list_y,
                          CLITERAL(Color){20, 18, 14, 240});
            DrawRectangleLinesEx(
                (Rectangle){(float)(px+8), (float)list_y, (float)(PANEL_WIDTH-16), (float)(wy-list_y)},
                1, COL_PANEL_BORDER);

            /* Wave items */
            for (int w = scroll; w < FC_NUM_WAVES && (w - scroll) < max_visible; w++) {
                int iy = list_y + (w - scroll) * item_h;
                Rectangle item_r = { (float)(px + 9), (float)iy, (float)(PANEL_WIDTH - 18), (float)item_h };
                int hover = CheckCollisionPointRec(GetMousePosition(), item_r);
                int current = (w + 1 == v->state.current_wave);

                if (current) DrawRectangleRec(item_r, CLITERAL(Color){60, 80, 40, 255});
                else if (hover) DrawRectangleRec(item_r, COL_TAB_HOVER);

                char label[16];
                snprintf(label, sizeof(label), "Wave %d", w + 1);
                DrawText(label, px + 14, iy + 2, 10,
                         current ? COL_TEXT_YELLOW : COL_TEXT_WHITE);

                /* Click to jump */
                if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int jump = w + 1;
                    viewer_jump_to_wave(v, jump);
                    dropdown_open = 0;
                }
            }
        }
        controls_y = wy + 26;
    }

    /* ---- Loadout dropdown ---- */
    {
        int ly = controls_y;
        char lbuf[48];
        snprintf(lbuf, sizeof(lbuf), "Loadout: %s", FC_LOADOUTS[v->active_loadout].name);
        Rectangle lbtn = { (float)(px + 8), (float)ly, (float)(PANEL_WIDTH - 16), 20.0f };
        int lhover = CheckCollisionPointRec(GetMousePosition(), lbtn);
        DrawRectangleRec(lbtn, lhover ? COL_TAB_HOVER : COL_TAB_INACTIVE);
        DrawRectangleLinesEx(lbtn, 1, COL_PANEL_BORDER);
        DrawText(lbuf, px + 14, ly + 4, 9, COL_TEXT_YELLOW);
        DrawText("v", px + PANEL_WIDTH - 22, ly + 4, 10, COL_TEXT_DIM);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && lhover)
            loadout_open = !loadout_open;
        if (loadout_open) {
            int item_h = 18;
            int list_y = ly + 22;  /* open downward below the button */
            DrawRectangle(px + 8, list_y, PANEL_WIDTH - 16, FC_NUM_LOADOUTS * item_h,
                          CLITERAL(Color){20, 18, 14, 240});
            DrawRectangleLinesEx(
                (Rectangle){(float)(px+8), (float)list_y, (float)(PANEL_WIDTH-16),
                            (float)(FC_NUM_LOADOUTS * item_h)}, 1, COL_PANEL_BORDER);
            for (int li = 0; li < FC_NUM_LOADOUTS; li++) {
                int iy = list_y + li * item_h;
                Rectangle ir = { (float)(px + 9), (float)iy, (float)(PANEL_WIDTH - 18), (float)item_h };
                int ih = CheckCollisionPointRec(GetMousePosition(), ir);
                int cur = (li == v->active_loadout);
                if (cur) DrawRectangleRec(ir, CLITERAL(Color){60, 80, 40, 255});
                else if (ih) DrawRectangleRec(ir, COL_TAB_HOVER);
                DrawText(FC_LOADOUTS[li].name, px + 14, iy + 3, 9,
                         cur ? COL_TEXT_YELLOW : COL_TEXT_WHITE);
                if (ih && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    v->active_loadout = li;
                    if (v->player_anim_state) {
                        anim_model_state_free(v->player_anim_state);
                        v->player_anim_state = NULL;
                    }
                    recreate_player_anim_state(v, viewer_player_model_entry(v));
                    const FcLoadout* lo = &FC_LOADOUTS[li];
                    FcPlayer* p = &v->state.player;
                    p->max_hp = lo->max_hp; p->current_hp = lo->max_hp;
                    p->max_prayer = lo->max_prayer; p->current_prayer = lo->max_prayer;
                    p->attack_level = lo->attack_lvl; p->strength_level = lo->strength_lvl;
                    p->defence_level = lo->defence_lvl; p->ranged_level = lo->ranged_lvl;
                    p->prayer_level = lo->prayer_lvl; p->magic_level = lo->magic_lvl;
                    p->weapon_kind = lo->weapon_kind;
                    p->weapon_uses_ammo = lo->weapon_uses_ammo;
                    p->weapon_speed = lo->weapon_speed;
                    p->weapon_range = lo->weapon_range;
                    p->ranged_attack_bonus = lo->ranged_atk; p->ranged_strength_bonus = lo->ranged_str;
                    p->defence_stab = lo->def_stab; p->defence_slash = lo->def_slash;
                    p->defence_crush = lo->def_crush; p->defence_magic = lo->def_magic;
                    p->defence_ranged = lo->def_ranged; p->prayer_bonus = lo->prayer_bonus;
                    p->ammo_count = lo->ammo;
                    apply_initial_supplies(v);
                    reset_reward_tracking(v);
                    fc_reward_runtime_begin_episode(&v->reward_runtime, &v->state);
                    loadout_open = 0;
                }
            }
            controls_y = list_y + FC_NUM_LOADOUTS * item_h + 10;
        } else {
            controls_y = ly + 26;
        }
    }

    /* ---- TPS preset buttons (manual and replay modes) ---- */
    {
        int box_x = px + 8;
        int box_w = PANEL_WIDTH - 16;
        int box_y = controls_y;
        int btn_gap = 6;
        int btn_cols = 3;
        int btn_h = 18;
        int btn_rows = (NUM_MANUAL_TPS_PRESETS + btn_cols - 1) / btn_cols;
        int box_h = 24 + btn_rows * btn_h + (btn_rows - 1) * 6 + 8;
        int label_y = box_y + 6;
        int btn_y = box_y + 24;
        int btn_w = (box_w - btn_gap * (btn_cols - 1)) / btn_cols;

        DrawRectangle(box_x, box_y, box_w, box_h, CLITERAL(Color){20, 18, 14, 220});
        DrawRectangleLines(box_x, box_y, box_w, box_h, COL_PANEL_BORDER);
        text_s(v->policy_pipe ? "Replay TPS" : "TPS Presets",
               box_x + 6, label_y, 9, COL_TEXT_DIM);

        for (int i = 0; i < NUM_MANUAL_TPS_PRESETS; i++) {
            int row = i / btn_cols;
            int col = i % btn_cols;
            int bx = box_x + col * (btn_w + btn_gap);
            int by_btn = btn_y + row * (btn_h + 6);
            Rectangle br = { (float)bx, (float)by_btn, (float)btn_w, (float)btn_h };
            int hovered = CheckCollisionPointRec(GetMousePosition(), br);
            int selected = float_near(v->tps, MANUAL_TPS_PRESETS[i]);
            Color bg = selected ? COL_TAB_ACTIVE : (hovered ? COL_TAB_HOVER : COL_TAB_INACTIVE);
            Color tc = selected ? COL_TEXT_YELLOW : COL_TEXT_WHITE;

            DrawRectangleRec(br, bg);
            DrawRectangleLinesEx(br, 1, COL_PANEL_BORDER);
            DrawText(MANUAL_TPS_LABELS[i],
                     bx + (btn_w - MeasureText(MANUAL_TPS_LABELS[i], 9)) / 2,
                     by_btn + 5, 9, tc);

            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                set_manual_speed(v, MANUAL_TPS_PRESETS[i]);
            }
        }
    }
}

static void draw_debug(ViewerState* v) {
    if (!v->show_debug) return;
    int x=10, y=HEADER_HEIGHT+6; char b[256];
    char speed_label[32];
    format_speed_label(v, speed_label, sizeof(speed_label));
    DrawRectangle(5,y-2,500,70,CLITERAL(Color){0,0,0,200});
    snprintf(b,sizeof(b),"%s | %s | CAM p=%.2f d=%.1f",
        v->paused ? "PAUSED" : "RUNNING", speed_label, v->cam_pitch, v->cam_dist);
    text_s(b,x,y,10,COL_TEXT_GREEN); y+=14;
    snprintf(b,sizeof(b),"Hash:0x%08x Player:(%d,%d) Entities:%d",
        v->last_hash, v->state.player.x, v->state.player.y, v->entity_count);
    text_s(b,x,y,10,COL_TEXT_DIM); y+=14;
    snprintf(b,sizeof(b),"Terrain:%s Objects:%s | Mode:%s",
        (v->terrain && v->terrain->loaded)?"ON":"off",
        (v->objects && v->objects->loaded)?"ON":"off",
        v->auto_mode?"AUTO":"HUMAN");
    text_s(b,x,y,10,COL_TEXT_DIM); y+=14;
    snprintf(b,sizeof(b),"DmgDealt:%d DmgTaken:%d Kills:%d",
        v->state.damage_dealt_this_tick, v->state.damage_taken_this_tick,
        v->state.npcs_killed_this_tick);
    text_s(b,x,y,10,COL_TEXT_DIM); y+=14;
    if (v->godmode) text_s("GODMODE ON (F9)", x, y, 10, COL_TEXT_YELLOW);
    text_s("F1-F8: spawn NPC | F9: godmode", x, y + (v->godmode ? 14 : 0), 9, COL_TEXT_DIM);
}

/* ======================================================================== */
/* Main                                                                      */
/* ======================================================================== */

int main(int argc, char** argv) {
    int screenshot_mode = 0;
    const char* screenshot_path = NULL;
    int policy_pipe_flag = 0;
    int policy_speed_flag = 1;
    int policy_episode_limit_flag = 0;
    int start_wave_flag = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--screenshot") == 0 && i+1 < argc) {
            screenshot_mode = 1;
            screenshot_path = argv[++i];
        } else if (strcmp(argv[i], "--policy-pipe") == 0) {
            policy_pipe_flag = 1;
        } else if (strcmp(argv[i], "--speed") == 0 && i+1 < argc) {
            policy_speed_flag = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--episodes") == 0 && i+1 < argc) {
            policy_episode_limit_flag = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--start-wave") == 0 && i+1 < argc) {
            start_wave_flag = atoi(argv[++i]);
        }
    }
    fprintf(stderr,"=== Fight Caves Viewer (Phase 8 — Playable) ===\n");
    /* In policy-pipe mode, suppress Raylib's INFO logs which go to stdout
     * and would corrupt the pipe protocol. */
    if (policy_pipe_flag)
        SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE|FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_W, WINDOW_H, "Fight Caves RL — Playable Viewer");
    SetTargetFPS(60);

    ViewerState v; memset(&v, 0, sizeof(v));
    fc_init(&v.state);
    runec_ui_init(&v.ui);
    load_fc_ui_item_icons(&v);
    v.paused = 1; v.tps = NORMAL_TPS; v.show_debug = 1; v.auto_mode = 0;
    v.active_loadout = FC_ACTIVE_LOADOUT;
    v.attack_target = -1;
    v.cam_yaw = 0; v.cam_pitch = 0.8f; v.cam_dist = 30;
    v.camera_locked = 1;
    v.camera.up = (Vector3){0,1,0}; v.camera.fovy = 32;
    v.camera.projection = CAMERA_PERSPECTIVE;
    v.camera.target = (Vector3){FC_ARENA_WIDTH * 0.5f, 0.5f, -(FC_ARENA_HEIGHT * 0.5f)};

    v.terrain = load_terrain(&v);
    v.objects = load_objects_with_terrain(v.terrain);
    if (fc_asset_exists("fightcaves.oanim"))
        v.object_anims = object_anims_load("fightcaves.oanim");
    if (v.object_anims)
        object_anims_offset(v.object_anims, FC_WORLD_ORIGIN_X, FC_WORLD_ORIGIN_Y);
    if (fc_asset_exists("fightcaves.object_anim.models"))
        v.object_anim_models = fc_npc_models_load("fightcaves.object_anim.models");
    if (v.object_anims && v.object_anims->count > 0) {
        v.object_anim_runtimes = (ObjectAnimRuntime*)calloc(
            (size_t)v.object_anims->count, sizeof(*v.object_anim_runtimes));
        if (v.object_anim_runtimes)
            v.object_anim_runtime_count = v.object_anims->count;
    }
    if (!v.terrain || !v.terrain->loaded) v.show_grid = 1;

    /* Load NPC models */
    {
        if (fc_asset_exists("fc_npcs.models"))
            v.npc_models = fc_npc_models_load("fc_npcs.models");
        if (!v.npc_models) fprintf(stderr, "warning: NPC models not found\n");
    }

    /* Load player model */
    {
        if (fc_asset_exists("fc_player.models"))
            v.player_model = fc_npc_models_load("fc_player.models");
    }

    /* Load animations (combined NPC + player) */
    {
        if (fc_asset_exists("fc_all.anims"))
            v.anim_cache = anim_cache_load("fc_all.anims");
        if (v.anim_cache)
            recreate_player_anim_state(&v, viewer_player_model_entry(&v));
        /* Create NPC animation states */
        if (v.anim_cache && v.npc_models) {
            for (int i = 0; i < v.npc_models->count; i++) {
                NpcModelEntry* nm = &v.npc_models->entries[i];
                if (nm->loaded && nm->vertex_skins) {
                    /* Find which NPC type this model corresponds to */
                    for (int t = 1; t <= 8; t++) {
                        if (fc_npc_type_to_model_id(t) == nm->model_id) {
                            /* Store anim state indexed by model entry, not NPC type */
                            break;
                        }
                    }
                }
            }
        }
    }

    /* Load projectile models */
    {
        if (fc_asset_exists("fc_projectiles.models"))
            v.projectile_models = fc_npc_models_load("fc_projectiles.models");
    }

    /* Load spotanim metadata for projectile model/scale lookup */
    {
        if (fc_asset_exists("fc_spotanims.bin"))
            v.spotanims = spotanims_load("fc_spotanims.bin");
    }

    /* Load prayer overhead icon textures */
    {
        if (fc_asset_exists("data/sprites/ui/prayeron_14.png")) {
            v.pray_melee_tex = fc_load_texture_asset("data/sprites/ui/prayeron_14.png");
            v.pray_missiles_tex = fc_load_texture_asset("data/sprites/ui/prayeron_13.png");
            v.pray_magic_tex = fc_load_texture_asset("data/sprites/ui/prayeron_12.png");
            fprintf(stderr, "Prayer icons loaded from %s\n", fc_asset_root());
        } else {
            fprintf(stderr, "warning: prayer icons not found under asset root %s\n",
                    fc_asset_root());
        }
    }

    /* Load tab/inventory sprites (Phase 8h) */
    {
        if (fc_asset_exists("sprites/prayer_potion.png")) {
            v.tex_ppot = fc_load_texture_asset("sprites/prayer_potion.png");
            v.tex_shark = fc_load_texture_asset("sprites/shark.png");
            v.tex_pray_melee_on = fc_load_texture_asset("data/sprites/ui/prayeron_14.png");
            v.tex_pray_melee_off = fc_load_texture_asset("data/sprites/ui/prayeroff_14.png");
            v.tex_pray_range_on = fc_load_texture_asset("data/sprites/ui/prayeron_13.png");
            v.tex_pray_range_off = fc_load_texture_asset("data/sprites/ui/prayeroff_13.png");
            v.tex_pray_magic_on = fc_load_texture_asset("data/sprites/ui/prayeron_12.png");
            v.tex_pray_magic_off = fc_load_texture_asset("data/sprites/ui/prayeroff_12.png");
            if (v.tex_pray_melee_on.id == 0)
                v.tex_pray_melee_on = fc_load_texture_asset("sprites/protect_melee_on.png");
            if (v.tex_pray_melee_off.id == 0)
                v.tex_pray_melee_off = fc_load_texture_asset("sprites/protect_melee_off.png");
            if (v.tex_pray_range_on.id == 0)
                v.tex_pray_range_on = fc_load_texture_asset("sprites/protect_missiles_on.png");
            if (v.tex_pray_range_off.id == 0)
                v.tex_pray_range_off = fc_load_texture_asset("sprites/protect_missiles_off.png");
            if (v.tex_pray_magic_on.id == 0)
                v.tex_pray_magic_on = fc_load_texture_asset("sprites/protect_magic_on.png");
            if (v.tex_pray_magic_off.id == 0)
                v.tex_pray_magic_off = fc_load_texture_asset("sprites/protect_magic_off.png");
            v.tex_tab_inv = fc_load_texture_asset("sprites/tab_inventory.png");
            v.tex_tab_combat = fc_load_texture_asset("sprites/tab_combat.png");
            v.tex_tab_prayer = fc_load_texture_asset("sprites/tab_prayer.png");
            fprintf(stderr, "Tab sprites loaded from %s\n", fc_asset_root());
        } else {
            fprintf(stderr, "warning: tab/inventory sprites not found under asset root %s\n",
                    fc_asset_root());
        }
    }

    v.combat_style = 1;  /* Rapid default */
    v.policy_pipe = policy_pipe_flag;
    v.policy_episode_limit = policy_episode_limit_flag;
    v.start_wave = start_wave_flag;
    if (v.policy_pipe)
        set_policy_replay_speed(&v, policy_speed_flag);

    reset_ep(&v);

    /* Policy pipe: write initial obs so Python can send first action */
    if (v.policy_pipe) {
        v.paused = 0;
        v.auto_mode = 0;
        fprintf(stderr, "[policy-pipe] Mode active. Reading actions from stdin.\n");
        write_obs_to_pipe(&v);
    }

    int frame_count = 0;

    while (!WindowShouldClose()) {
        int quit_after_tick = 0;
        int ui_capture = 0;
        /* Screenshot mode */
        if (screenshot_mode && frame_count == 5) {
            TakeScreenshot(screenshot_path);
            fprintf(stderr, "Screenshot saved to %s\n", screenshot_path);
            break;
        }
        frame_count++;

        /* Global keys (always active) */
        if (IsKeyPressed(KEY_Q)) break;
        if (IsKeyPressed(KEY_SPACE)) v.paused = !v.paused;
        if (IsKeyPressed(KEY_RIGHT)) v.step_once = 1;
        if (v.policy_pipe) {
            if (IsKeyPressed(KEY_ONE)) set_policy_replay_speed(&v, 1);
            if (IsKeyPressed(KEY_TWO)) set_policy_replay_speed(&v, 2);
            if (!IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) &&
                IsKeyPressed(KEY_FOUR)) set_policy_replay_speed(&v, 4);
            if (IsKeyPressed(KEY_ZERO)) set_policy_replay_speed(&v, 10);
            if (IsKeyPressed(KEY_UP)) cycle_policy_replay_speed(&v, +1);
            if (IsKeyPressed(KEY_DOWN)) cycle_policy_replay_speed(&v, -1);
        }
        if (IsKeyPressed(KEY_R)) reset_ep(&v);
        if (IsKeyPressed(KEY_L)) {
            if (v.camera_locked) {
                v.camera.target = camera_follow_target(&v);
            }
            v.camera_locked = !v.camera_locked;
        }

        /* Toggle keys */
        /* Auto mode removed — was too easy to accidentally toggle with 'A' key.
         * Use --auto command line flag if needed for testing. */
        if (IsKeyPressed(KEY_G)) v.show_grid = !v.show_grid;
        if (IsKeyPressed(KEY_C)) v.show_collision = !v.show_collision;
        /* O: cycle debug overlay modes. O=all on/off, Shift+O=cycle sub-modes */
        if (IsKeyPressed(KEY_O)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                /* Cycle through individual modes */
                if (v.dbg_flags == 0) v.dbg_flags = DBG_COLLISION;
                else if (v.dbg_flags == DBG_COLLISION) v.dbg_flags = DBG_LOS;
                else if (v.dbg_flags == DBG_LOS) v.dbg_flags = DBG_PATH | DBG_RANGE;
                else if (v.dbg_flags == (DBG_PATH | DBG_RANGE)) v.dbg_flags = DBG_ENTITY_INFO;
                else if (v.dbg_flags == DBG_ENTITY_INFO) v.dbg_flags = DBG_OBS | DBG_MASK | DBG_REWARD;
                else v.dbg_flags = 0;
            } else {
                /* Toggle all on/off */
                v.dbg_flags = v.dbg_flags ? 0 : DBG_ALL;
            }
            if (v.dbg_flags)
                v.ui.active_tab = RUNEC_UI_TAB_FRIENDS;
        }
        /* D: match the on-screen controls without interfering with east movement */
        if (IsKeyPressed(KEY_D) && !IsKeyDown(KEY_W) && !IsKeyDown(KEY_A) && !IsKeyDown(KEY_S)) {
            v.dbg_flags = v.dbg_flags ? 0 : DBG_ALL;
            if (v.dbg_flags)
                v.ui.active_tab = RUNEC_UI_TAB_FRIENDS;
        }
        if (IsKeyPressed(KEY_GRAVE)) v.show_debug = !v.show_debug;

        /* Camera presets */
        if ((!v.policy_pipe && IsKeyPressed(KEY_FOUR)) ||
            (v.policy_pipe &&
             (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) &&
             IsKeyPressed(KEY_FOUR))) {
            v.cam_yaw=0; v.cam_pitch=1.35f; v.cam_dist=120;
        }
        if ((!v.policy_pipe && IsKeyPressed(KEY_FIVE)) ||
            (v.policy_pipe &&
             (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) &&
             IsKeyPressed(KEY_FIVE))) {
            v.cam_yaw=0; v.cam_pitch=0.6f; v.cam_dist=50;
        }

        sync_fc_ui(&v);
        ui_capture = process_runec_prayer_click(&v);
        if (!ui_capture) {
            ui_capture = runec_ui_handle_input(&v.ui, GetScreenWidth(), GetScreenHeight());
            handle_runec_ui_intent(&v);
        } else {
            v.ui.last_intent.kind = RUNEC_UI_INTENT_NONE;
        }
        if (process_runec_clan_wave_scroll(&v))
            ui_capture = 1;
        if (process_runec_debug_tab_clicks(&v))
            ui_capture = 1;

        /* Camera orbit + zoom */
        if (!ui_capture && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 d = GetMouseDelta();
            v.cam_yaw += d.x*0.005f; v.cam_pitch -= d.y*0.005f;
            if (v.cam_pitch < 0.1f) v.cam_pitch = 0.1f;
            if (v.cam_pitch > 1.4f) v.cam_pitch = 1.4f;
        }
        float wh = GetMouseWheelMove();
        if (!ui_capture && wh != 0) {
            v.cam_dist *= (wh > 0) ? (1.0f/1.15f) : 1.15f;
            if (v.cam_dist < 5) v.cam_dist = 5;
            if (v.cam_dist > 300) v.cam_dist = 300;
        }

        /* Tick processing */
        int tick = 0;
        if (!v.paused) {
            v.tick_acc += GetFrameTime() * (float)v.tps;
            if (v.tick_acc >= 1.0f) {
                v.tick_acc = fmodf(v.tick_acc, 1.0f);
                tick = 1;
            }
            /* tick_frac: 0.0 right after tick, approaches 1.0 just before next tick */
            v.tick_frac = v.tick_acc;
            if (v.tick_frac > 1.0f) v.tick_frac = 1.0f;
        }
        if (v.step_once) { tick = 1; v.step_once = 0; }

        /* Capture clicks and key presses EVERY frame (60fps).
         * These set routes/targets/buffers on the player struct.
         * The tick loop reads them when the next tick fires. */
        if (!v.auto_mode && !v.policy_pipe && v.state.terminal == TERMINAL_NONE) {
            process_human_clicks(&v, ui_capture);
            process_human_keys(&v);
        }

        if (tick && v.state.terminal == TERMINAL_NONE) {
            /* Build action array for this tick */
            if (v.policy_pipe) {
                if (!read_policy_actions(&v)) {
                    fprintf(stderr, "[policy-pipe] EOF on stdin, stopping.\n");
                    break;
                }
            } else if (v.test_mode && build_test_actions(&v)) {
                /* Test mode provided actions */
            } else if (v.auto_mode) {
                for (int h = 0; h < FC_NUM_ACTION_HEADS; h++)
                    v.actions[h] = GetRandomValue(0, FC_ACTION_DIMS[h]-1);
                if (GetRandomValue(0,2) == 0) v.actions[0] = 0;
            } else {
                build_human_actions(&v);
            }

            /* Save previous positions by stable NPC array index */
            v.prev_player_x = (float)v.state.player.x;
            v.prev_player_y = (float)v.state.player.y;
            v.prev_player_route_idx = v.state.player.route_idx;
            v.prev_player_route_len = v.state.player.route_len;
            v.prev_player_route_x = v.state.player.x;
            v.prev_player_route_y = v.state.player.y;
            if (v.state.player.route_idx < v.state.player.route_len) {
                int ri = v.state.player.route_idx;
                v.prev_player_route_x = v.state.player.route_x[ri];
                v.prev_player_route_y = v.state.player.route_y[ri];
            }
            for (int ni = 0; ni < FC_MAX_NPCS; ni++) {
                v.prev_npc_x[ni] = (float)v.state.npcs[ni].x;
                v.prev_npc_y[ni] = (float)v.state.npcs[ni].y;
                v.prev_npc_active[ni] = v.state.npcs[ni].active;
            }

            /* Snapshot pending hit counts BEFORE tick (to detect new projectiles) */
            int prev_player_hits = v.state.player.num_pending_hits;
            int prev_npc_hits[FC_MAX_NPCS];
            for (int ni = 0; ni < FC_MAX_NPCS; ni++)
                prev_npc_hits[ni] = v.state.npcs[ni].num_pending_hits;

            /* Step simulation */
            fc_step(&v.state, v.actions);
            update_reward_breakdown(&v);
            if (v.state.attack_attempt_this_tick)
                mark_player_attack_visual(&v);
            v.tick_frac = 0.0f;

            /* Debug event log — record events from this tick */
            if (v.dbg_flags) dbg_log_tick(&v.state);

            /* Debug: godmode — prevent player death */
            if (v.godmode && v.state.player.current_hp <= 0) {
                v.state.player.current_hp = v.state.player.max_hp;
                v.state.terminal = TERMINAL_NONE;
            }

            /* Snap prev positions for newly spawned NPCs so they don't fly.
             * An NPC that wasn't active last tick but is now = new spawn. */
            for (int ni = 0; ni < FC_MAX_NPCS; ni++) {
                if (v.state.npcs[ni].active && !v.prev_npc_active[ni]) {
                    v.prev_npc_x[ni] = (float)v.state.npcs[ni].x;
                    v.prev_npc_y[ni] = (float)v.state.npcs[ni].y;
                }
            }

            fc_fill_render_entities(&v.state, v.entities, &v.entity_count);
            v.last_hash = fc_state_hash(&v.state);

            /* Generate hitsplats and projectiles from per-tick events */
            {
                float vis_px = (float)v.state.player.x;
                float vis_py = (float)v.state.player.y;
                int vis_tile_x = (int)floorf(vis_px);
                int vis_tile_y = (int)floorf(vis_py);
                if (vis_tile_x < 0) vis_tile_x = 0;
                if (vis_tile_y < 0) vis_tile_y = 0;
                if (vis_tile_x >= FC_ARENA_WIDTH) vis_tile_x = FC_ARENA_WIDTH - 1;
                if (vis_tile_y >= FC_ARENA_HEIGHT) vis_tile_y = FC_ARENA_HEIGHT - 1;
                float gy_p = ground_y(&v, vis_tile_x, vis_tile_y);
                float p3x = vis_px + 0.5f;
                float p3y = gy_p + 1.5f;
                float p3z = -(vis_py + 0.5f);
                float tick_sec = (v.tps > 0) ? (1.0f / (float)v.tps) : 0.5f;

                /* Player hitsplat: only show when an incoming NPC hit resolved.
                 * hit_landed_this_tick is overloaded in the core and also flips
                 * when the player fires their own ranged attack, which would
                 * otherwise create false blue 0 splats on the player. */
                if (v.state.player.hit_source_npc_type != NPC_NONE) {
                    spawn_hitsplat(&v, p3x, gy_p + 2.5f, p3z,
                                   v.state.player.damage_taken_this_tick);
                }
                if (v.state.tz_kih_prayer_drain_this_tick > 0) {
                    spawn_status_splat(&v, p3x + 0.3f, gy_p + 3.0f, p3z,
                                       v.state.tz_kih_prayer_drain_this_tick,
                                       HITSPLAT_PRAYER_DRAIN);
                }

                /* Player fired ranged attack → ONE crossbow bolt projectile.
                 * Detect: attack_timer just reset to 5 (player fired this tick). */
                if (v.state.player.attack_target_idx >= 0 && v.state.player.attack_timer == 5) {
                    int ti = v.state.player.attack_target_idx;
                    FcNpc* tn = &v.state.npcs[ti];
                    if (tn->active) {
                        float n3x = (float)tn->x + (float)tn->size*0.5f;
                        float n3y = ground_y(&v, tn->x, tn->y) + 1.0f + (float)tn->size*0.3f;
                        float n3z = -((float)tn->y + (float)tn->size*0.5f);
                        int pdist = fc_distance_to_npc(v.state.player.x, v.state.player.y, tn);
                        float travel = (float)fc_ranged_hit_delay(pdist) * tick_sec;
                        if (travel < 0.1f) travel = 0.1f;
                        spawn_projectile(&v, p3x, p3y, p3z, n3x, n3y, n3z,
                                         travel,
                                         CLITERAL(Color){200, 200, 50, 255}, 0.12f,
                                         PROJ_CROSSBOW_BOLT, 0, 0);
                    }
                }

                /* NPC damage taken — hitsplats */
                for (int i = 0; i < FC_MAX_NPCS; i++) {
                    FcNpc* n = &v.state.npcs[i];
                    if (!n->active && !n->died_this_tick && n->damage_taken_this_tick == 0) continue;
                    int hit_resolved = n->damage_taken_this_tick > 0 || n->died_this_tick ||
                                       (prev_npc_hits[i] > n->num_pending_hits);
                    if (hit_resolved) {
                        float gy_n = ground_y(&v, n->x, n->y);
                        float nx = (float)n->x + (float)n->size*0.5f;
                        float nz = -((float)n->y + (float)n->size*0.5f);
                        float nh = gy_n + 1.0f + (float)n->size*0.5f;
                        spawn_hitsplat(&v, nx, nh, nz, n->damage_taken_this_tick);
                    }
                    if (n->healing_received_this_tick > 0) {
                        float gy_n = ground_y(&v, n->x, n->y);
                        float nx = (float)n->x + (float)n->size*0.5f;
                        float nz = -((float)n->y + (float)n->size*0.5f);
                        float nh = gy_n + 1.4f + (float)n->size*0.5f;
                        spawn_status_splat(&v, nx, nh, nz,
                                           n->healing_received_this_tick,
                                           HITSPLAT_HEAL);
                    }
                }

                /* NPC ranged/magic fired → ONE projectile per new pending hit on player.
                 * Compare player pending hit count before/after tick. */
                for (int hi = prev_player_hits; hi < v.state.player.num_pending_hits; hi++) {
                    FcPendingHit* ph = &v.state.player.pending_hits[hi];
                    if (!ph->active || ph->source_npc_idx < 0) continue;
                    FcNpc* src = &v.state.npcs[ph->source_npc_idx];
                    if (!src->active) continue;
                    mark_npc_attack_visual(&v, ph->source_npc_idx,
                                           ph->attack_style);
                    if (ph->attack_style == ATTACK_MELEE) continue;  /* no projectile for melee */
                    int sx_tile = src->x;
                    int sy_tile = src->y;
                    if (src->size > 1) {
                        if (v.state.player.x < src->x) sx_tile = src->x;
                        else if (v.state.player.x >= src->x + src->size) sx_tile = src->x + src->size - 1;
                        else sx_tile = v.state.player.x;
                        if (v.state.player.y < src->y) sy_tile = src->y;
                        else if (v.state.player.y >= src->y + src->size) sy_tile = src->y + src->size - 1;
                        else sy_tile = v.state.player.y;
                    }
                    float src_ground = ground_y(&v, sx_tile, sy_tile);
                    float s3x = (float)sx_tile + 0.5f;
                    float s3y = src_ground + 1.0f + (float)src->size*0.3f;
                    float s3z = -((float)sy_tile + 0.5f);
                    float dst_y = p3y;
                    float travel = (float)ph->ticks_remaining * tick_sec;
                    if (travel < 0.1f) travel = 0.1f;
                    Color pc = (ph->attack_style == ATTACK_MAGIC)
                        ? CLITERAL(Color){255, 104, 36, 235}
                        : CLITERAL(Color){218, 178, 92, 235};
                    float rad = (src->npc_type == NPC_TZTOK_JAD) ? 0.3f : 0.15f;
                    uint32_t travel_spot = 0;
                    uint32_t launch_spot = 0;
                    uint32_t impact_spot = 0;
                    int use_profile = 0;
                    float profile_start_time = 0.0f;
                    float profile_end_time = 0.0f;
                    float profile_angle = -1.0f;
                    float profile_progress = -1.0f;
                    float launch_y = s3y;
                    if (src->npc_type == NPC_TOK_XIL) {
                        travel_spot = PROJ_TOK_XIL_SPINE;
                        impact_spot = PROJ_TOK_XIL_IMPACT;
                    } else if (src->npc_type == NPC_KET_ZEK) {
                        travel_spot = PROJ_KET_ZEK_FIRE;
                        impact_spot = PROJ_KET_ZEK_IMPACT;
                    } else if (src->npc_type == NPC_TZTOK_JAD &&
                               ph->attack_style == ATTACK_MAGIC) {
                        launch_spot = PROJ_JAD_MAGIC_LAUNCH;
                        travel_spot = PROJ_JAD_MAGIC_TRAVEL;
                        impact_spot = PROJ_JAD_MAGIC_IMPACT;
                        launch_y = src_ground + 92.0f / 128.0f;
                        s3y = src_ground + 172.0f / 128.0f;
                        dst_y = gy_p + 124.0f / 128.0f;
                        profile_start_time = 41.0f;
                        profile_angle = 16.0f;
                        profile_progress = 64.0f;
                        use_profile = 1;
                    } else if (src->npc_type == NPC_TZTOK_JAD &&
                               ph->attack_style == ATTACK_RANGED) {
                        launch_spot = PROJ_JAD_RANGE_LAUNCH;
                        travel_spot = PROJ_JAD_RANGED_TRAVEL;
                        impact_spot = PROJ_JAD_RANGED_IMPACT;
                        launch_y = src_ground + 96.0f / 128.0f;
                        s3y = src_ground + 163.0f / 128.0f;
                        dst_y = gy_p + 146.0f / 128.0f;
                        profile_start_time = 32.0f;
                        profile_angle = 15.0f;
                        profile_progress = 11.0f;
                        use_profile = 1;
                    }
                    if (use_profile) {
                        int dx = sx_tile > v.state.player.x
                            ? sx_tile - v.state.player.x
                            : v.state.player.x - sx_tile;
                        int dy = sy_tile > v.state.player.y
                            ? sy_tile - v.state.player.y
                            : v.state.player.y - sy_tile;
                        int dist = dx > dy ? dx : dy;
                        profile_end_time = profile_start_time + 5.0f * (float)dist;
                        int profile_ticks = 1 + (int)(profile_end_time / 30.0f);
                        float profile_travel = (float)profile_ticks * tick_sec;
                        if (travel < profile_travel) travel = profile_travel;
                        float launch_secs = (profile_start_time > 30.0f
                            ? profile_start_time : 30.0f) / 30.0f * tick_sec;
                        float launch_yaw =
                            atan2f(p3x - s3x, p3z - s3z) * (180.0f / 3.14159f);
                        spawn_spot_effect(&v, launch_spot, s3x, launch_y, s3z,
                                          launch_secs, pc, rad, launch_yaw);
                    }
                    VisualProjectile* vp =
                        spawn_projectile(&v, s3x, s3y, s3z, p3x, dst_y, p3z,
                                         travel, pc, rad, travel_spot,
                                         launch_spot, impact_spot);
                    if (use_profile)
                        apply_projectile_profile(vp, profile_start_time,
                                                 profile_end_time,
                                                 profile_angle,
                                                 profile_progress);
                }

                for (int ni = 0; ni < FC_MAX_NPCS; ni++) {
                    FcNpc* n = &v.state.npcs[ni];
                    if (!n->active || n->npc_type != NPC_TZTOK_JAD ||
                        n->attack_timer != n->attack_speed)
                        continue;
                    if (fc_npc_can_melee_player(v.state.player.x,
                                                v.state.player.y,
                                                n->x, n->y, n->size,
                                                v.state.walkable)) {
                        mark_npc_attack_visual(&v, ni, ATTACK_MELEE);
                    }
                }
            }

            /* Sync viewer attack_target with player's backend target */
            v.attack_target = v.state.player.attack_target_idx;
            /* Auto-clear if target NPC died */
            if (v.state.player.attack_target_idx >= 0) {
                FcNpc* tn = &v.state.npcs[v.state.player.attack_target_idx];
                if (!tn->active || tn->is_dead) {
                    v.state.player.attack_target_idx = -1;
                    v.attack_target = -1;
                }
            }

            if (v.state.terminal != TERMINAL_NONE) {
                if (v.policy_pipe) {
                    print_policy_episode_summary(&v);
                    v.policy_episode_count++;
                    /* Write terminal obs, then auto-reset unless a fixed episode limit was requested. */
                    write_obs_to_pipe(&v);
                    if (v.policy_episode_limit > 0 &&
                        v.policy_episode_count >= v.policy_episode_limit) {
                        quit_after_tick = 1;
                    } else {
                        reset_ep(&v);
                    }
                } else {
                    v.paused = 1;
                }
            } else if (v.policy_pipe) {
                write_obs_to_pipe(&v);
            }
        }

        if (quit_after_tick) {
            fprintf(stderr, "[policy-pipe] Episode limit reached, exiting viewer.\n");
            break;
        }

        /* Update hitsplat lifetimes */
        for (int i = 0; i < MAX_HITSPLATS; i++) {
            if (v.hitsplats[i].active) {
                v.hitsplats[i].frames_left--;
                if (v.hitsplats[i].frames_left <= 0) v.hitsplats[i].active = 0;
            }
        }

        /* Update visual projectiles */
        {
            float dt = GetFrameTime();
            if (v.player_visual_lock_timer > 0.0f) {
                v.player_visual_lock_timer -= dt;
                if (v.player_visual_lock_timer <= 0.0f) {
                    v.player_visual_lock_timer = 0.0f;
                    v.player_visual_lock_seq = 0;
                    v.player_attack_visual_target_idx = -1;
                    v.player_action_anim_seq = 0;
                    v.player_action_anim_frame = 0;
                    v.player_action_anim_timer = 0.0f;
                }
            }
            objects_update_texture_anims(v.objects, dt);
            models_update_texture_anims(v.object_anim_models, dt);
            models_update_texture_anims(v.projectile_models, dt);
            models_update_texture_anims(v.npc_models, dt);
            models_update_texture_anims(v.player_model, dt);
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (v.projectiles[i].active) {
                    v.projectiles[i].elapsed += dt;
                    if (v.projectiles[i].elapsed >= v.projectiles[i].total_time) {
                        float effect_secs = (v.tps > 0.0f)
                            ? (3.0f / (float)v.tps) : 1.8f;
                        if (effect_secs < 0.1f) effect_secs = 0.1f;
                        spawn_spot_effect(&v, v.projectiles[i].impact_spot_id,
                                          v.projectiles[i].dst_x,
                                          v.projectiles[i].dst_y,
                                          v.projectiles[i].dst_z,
                                          effect_secs, v.projectiles[i].color,
                                          v.projectiles[i].radius * 1.4f,
                                          0.0f);
                        free_projectile(&v.projectiles[i]);
                    }
                }
            }
            for (int i = 0; i < MAX_VISUAL_EFFECTS; i++) {
                if (v.effects[i].active) {
                    v.effects[i].elapsed += dt;
                    if (v.effects[i].elapsed >= v.effects[i].total_time)
                        free_effect(&v.effects[i]);
                }
            }
        }

        /* Update player animation */
        if (v.anim_cache && v.player_model && v.player_model->count > 0) {
            NpcModelEntry* pm = viewer_player_model_entry(&v);
            float dt = GetFrameTime();
            float anim_dt = policy_replay_anim_dt(&v, dt);
            recreate_player_anim_state(&v, pm);
            if (!v.player_anim_state || !pm) goto skip_player_anim_update;

            int visual_locked = player_visual_lock_active(&v);
            int player_moved = ((int)v.prev_player_x != v.state.player.x ||
                                (int)v.prev_player_y != v.state.player.y);
            uint16_t pose_seq = PLAYER_ANIM_IDLE;
            if (player_moved)
                pose_seq = v.state.player.is_running
                    ? PLAYER_ANIM_RUN : PLAYER_ANIM_WALK;

            /* OSRS applies movement pose and action sequence independently.
             * In policy replay, movement and attack can be valid on the same
             * tick, so attack visuals are driven by the actual attack event. */
            uint16_t action_seq = 0;
            if (v.state.terminal == TERMINAL_PLAYER_DEATH) {
                action_seq = PLAYER_ANIM_DEATH;
            } else if (v.state.player.food_eaten_this_tick) {
                action_seq = PLAYER_ANIM_EAT;
            } else if (visual_locked) {
                action_seq = v.player_visual_lock_seq;
            }

            AnimSequence* pose = advance_anim_track(
                v.anim_cache, pose_seq,
                &v.player_pose_anim_seq, &v.player_pose_anim_frame,
                &v.player_pose_anim_timer, anim_dt);
            AnimSequence* action = NULL;
            if (action_seq != 0) {
                action = visual_locked
                    ? advance_anim_track_once(
                        v.anim_cache, action_seq,
                        &v.player_action_anim_seq,
                        &v.player_action_anim_frame,
                        &v.player_action_anim_timer, anim_dt)
                    : advance_anim_track(
                        v.anim_cache, action_seq,
                        &v.player_action_anim_seq,
                        &v.player_action_anim_frame,
                        &v.player_action_anim_timer, anim_dt);
            } else {
                v.player_action_anim_seq = 0;
                v.player_action_anim_frame = 0;
                v.player_action_anim_timer = 0.0f;
            }

            v.player_anim_seq = action ? action_seq : pose_seq;
            v.player_anim_frame = action
                ? v.player_action_anim_frame : v.player_pose_anim_frame;

            int applied = 0;
            if (action) {
                AnimFrameData* action_fd =
                    &action->frames[v.player_action_anim_frame].frame;
                AnimFrameBase* action_fb =
                    anim_get_framebase(v.anim_cache, action_fd->framebase_id);
                if (action_fb && player_moved && !visual_locked && pose &&
                    action->interleave_count > 0 && action->interleave_order) {
                    AnimFrameData* pose_fd =
                        &pose->frames[v.player_pose_anim_frame].frame;
                    AnimFrameBase* pose_fb =
                        anim_get_framebase(v.anim_cache, pose_fd->framebase_id);
                    if (pose_fb) {
                        anim_apply_frame_interleaved(
                            v.player_anim_state, pm->base_verts,
                            pose_fd, pose_fb, action_fd, action_fb,
                            action->interleave_order,
                            action->interleave_count);
                        upload_anim_state_to_entry(pm, v.player_anim_state);
                        applied = 1;
                    }
                } else if (action_fb) {
                    apply_anim_frame_to_entry(pm, v.player_anim_state,
                                              action_fd, action_fb);
                    applied = 1;
                }
            }

            if (!applied && pose) {
                AnimFrameData* pose_fd =
                    &pose->frames[v.player_pose_anim_frame].frame;
                AnimFrameBase* pose_fb =
                    anim_get_framebase(v.anim_cache, pose_fd->framebase_id);
                if (pose_fb) {
                    apply_anim_frame_to_entry(pm, v.player_anim_state,
                                              pose_fd, pose_fb);
                }
            }
        }
skip_player_anim_update:

        /* Update NPC animations */
        if (v.anim_cache && v.npc_models) {
            float dt = GetFrameTime();
            float anim_dt = policy_replay_anim_dt(&v, dt);
            for (int ni = 0; ni < FC_MAX_NPCS; ni++) {
                FcNpc* n = &v.state.npcs[ni];
                if (!n->active && !n->died_this_tick) {
                    /* NPC gone — free anim state */
                    if (v.npc_anim_states[ni]) {
                        anim_model_state_free(v.npc_anim_states[ni]);
                        v.npc_anim_states[ni] = NULL;
                    }
                    v.npc_attack_visual_style[ni] = ATTACK_NONE;
                    v.npc_attack_visual_timer[ni] = 0.0f;
                    continue;
                }

                /* Find model entry for this NPC type */
                uint32_t mid = fc_npc_type_to_model_id(n->npc_type);
                NpcModelEntry* nme = fc_npc_model_find(v.npc_models, mid);
                if (!nme || !nme->loaded || !nme->vertex_skins) continue;

                /* Recreate anim state if this slot now holds a different model size. */
                if (!v.npc_anim_states[ni] ||
                    v.npc_anim_states[ni]->vert_count != nme->base_vert_count) {
                    if (v.npc_anim_states[ni]) {
                        anim_model_state_free(v.npc_anim_states[ni]);
                    }
                    v.npc_anim_states[ni] = anim_model_state_create(
                        nme->vertex_skins, nme->base_vert_count);
                    v.npc_anim_seq[ni] = (n->npc_type > 0 && n->npc_type < 9)
                        ? NPC_ANIM_IDLE[n->npc_type] : 0;
                    v.npc_anim_frame[ni] = 0;
                    v.npc_anim_timer[ni] = 0;
                }

                /* Select animation based on this NPC's own movement/state. */
                uint16_t desired = (n->npc_type > 0 && n->npc_type < 9)
                    ? NPC_ANIM_IDLE[n->npc_type] : 0;
                int npc_moved = ((int)v.prev_npc_x[ni] != n->x ||
                                 (int)v.prev_npc_y[ni] != n->y);
                if (n->is_dead || n->died_this_tick) {
                    desired = (n->npc_type > 0 && n->npc_type < 9)
                        ? NPC_ANIM_DEATH[n->npc_type] : 0;
                } else if (npc_moved) {
                    desired = (n->npc_type > 0 && n->npc_type < 9)
                        ? NPC_ANIM_WALK[n->npc_type] : 0;
                } else if (v.npc_attack_visual_timer[ni] > 0.0f) {
                    desired = npc_attack_animation_id(
                        n->npc_type, v.npc_attack_visual_style[ni]);
                } else if (n->damage_taken_this_tick > 0) {
                    /* NPC just got hit — brief defend/flinch, handled by staying on current */
                } else if (n->attack_timer == n->attack_speed) {
                    /* NPC just attacked */
                    desired = npc_attack_animation_id(n->npc_type,
                                                      n->attack_style);
                }

                AnimSequence* seq = advance_anim_track(
                    v.anim_cache, desired,
                    &v.npc_anim_seq[ni], &v.npc_anim_frame[ni],
                    &v.npc_anim_timer[ni], anim_dt);
                if (seq) {
                    AnimFrameData* fd = &seq->frames[v.npc_anim_frame[ni]].frame;
                    AnimFrameBase* fb = anim_get_framebase(v.anim_cache, fd->framebase_id);
                    if (fb) {
                        apply_anim_frame_to_entry(nme, v.npc_anim_states[ni],
                                                  fd, fb);
                    }
                }

                if (v.npc_attack_visual_timer[ni] > 0.0f) {
                    v.npc_attack_visual_timer[ni] -= dt;
                    if (v.npc_attack_visual_timer[ni] <= 0.0f) {
                        v.npc_attack_visual_timer[ni] = 0.0f;
                        v.npc_attack_visual_style[ni] = ATTACK_NONE;
                    }
                }
            }
        }

        /* Draw */
        BeginDrawing();
        ClearBackground(COL_BG);
        draw_scene(&v);
        sync_fc_ui(&v);
        runec_ui_draw(&v.ui, GetScreenWidth(), GetScreenHeight());
        draw_runec_debug_tabs(&v);
        draw_test_overlay(&v);

        EndDrawing();
    }

    if (v.pray_melee_tex.id > 0) UnloadTexture(v.pray_melee_tex);
    if (v.pray_missiles_tex.id > 0) UnloadTexture(v.pray_missiles_tex);
    if (v.pray_magic_tex.id > 0) UnloadTexture(v.pray_magic_tex);
    /* Phase 8h sprites */
    if (v.tex_ppot.id > 0) UnloadTexture(v.tex_ppot);
    if (v.tex_shark.id > 0) UnloadTexture(v.tex_shark);
    if (v.tex_pray_melee_on.id > 0) UnloadTexture(v.tex_pray_melee_on);
    if (v.tex_pray_melee_off.id > 0) UnloadTexture(v.tex_pray_melee_off);
    if (v.tex_pray_range_on.id > 0) UnloadTexture(v.tex_pray_range_on);
    if (v.tex_pray_range_off.id > 0) UnloadTexture(v.tex_pray_range_off);
    if (v.tex_pray_magic_on.id > 0) UnloadTexture(v.tex_pray_magic_on);
    if (v.tex_pray_magic_off.id > 0) UnloadTexture(v.tex_pray_magic_off);
    if (v.tex_tab_inv.id > 0) UnloadTexture(v.tex_tab_inv);
    if (v.tex_tab_combat.id > 0) UnloadTexture(v.tex_tab_combat);
    if (v.tex_tab_prayer.id > 0) UnloadTexture(v.tex_tab_prayer);
    clear_visuals(&v);
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        if (v.npc_anim_states[i]) anim_model_state_free(v.npc_anim_states[i]);
    }
    if (v.object_anim_runtimes) {
        for (int i = 0; i < v.object_anim_runtime_count; i++) {
            if (v.object_anim_runtimes[i].anim_state)
                anim_model_state_free(v.object_anim_runtimes[i].anim_state);
        }
        free(v.object_anim_runtimes);
    }
    if (v.player_anim_state) anim_model_state_free(v.player_anim_state);
    if (v.anim_cache) anim_cache_free(v.anim_cache);
    if (v.spotanims) spotanims_free(v.spotanims);
    if (v.projectile_models) fc_npc_models_unload(v.projectile_models);
    if (v.player_model) fc_npc_models_unload(v.player_model);
    if (v.npc_models) fc_npc_models_unload(v.npc_models);
    if (v.object_anim_models) fc_npc_models_unload(v.object_anim_models);
    if (v.object_anims) object_anims_free(v.object_anims);
    objects_free(v.objects);
    if (v.terrain && v.terrain->loaded) { UnloadModel(v.terrain->model); free(v.terrain->heightmap); free(v.terrain); }
    runec_ui_shutdown(&v.ui);
    CloseWindow();
    return 0;
}
