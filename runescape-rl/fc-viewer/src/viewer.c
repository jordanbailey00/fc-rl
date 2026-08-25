/*
 * viewer.c — Fight Caves playable debug viewer.
 *
 * Phase 8: Human-playable Fight Caves with all backend systems connected.
 *
 * Controls:
 *   WASD        — move (N/W/S/E)            Space    — pause/resume
 *   1/2/3       — protect melee/range/magic  Right    — single-step tick
 *   F           — eat shark                  Console  — targets/wave/TPS
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
#include "raymath.h"
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
#include "fc_actor_visual.h"
#include "fc_click_feedback.h"
#include "fc_minimap.h"
#include "fc_osrs_text.h"
#include "fc_projectile_visual.h"
#include "fc_debug_overlay.h"
#include "ui.h"
#include "ui_reference.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DEFAULT_WINDOW_W 1244
#define DEFAULT_WINDOW_H 1064
#define MAX_TPS         60.0f
#define MIN_TPS         0.25f
#define HALF_TPS        0.50f
#define NORMAL_TPS      (5.0f / 3.0f)
#define MAX_HITSPLATS   32
#define OSRS_HITSPLAT_SECONDS  1.0f  /* b237 regular hitmark stickTime=50 */
#define OSRS_HEALTHBAR_SECONDS 6.0f  /* b237 health_30 stickTime=300 */
#define POLICY_REPLAY_BASE_TPS NORMAL_TPS

/* Player animation sequence IDs (from OSRS cache/reference data). */
#define PLAYER_ANIM_HUMAN_IDLE 808
#define PLAYER_ANIM_HUMAN_WALK 819
#define PLAYER_ANIM_HUMAN_WALK_BACK 820
#define PLAYER_ANIM_HUMAN_WALK_RIGHT 821
#define PLAYER_ANIM_HUMAN_WALK_LEFT 822
#define PLAYER_ANIM_HUMAN_TURN 823
#define PLAYER_ANIM_HUMAN_RUN  824
#define PLAYER_ANIM_BOW_ATTACK 426
#define PLAYER_ANIM_XBOW_IDLE  4591
#define PLAYER_ANIM_XBOW_WALK  4226
#define PLAYER_ANIM_XBOW_RUN   4228
#define PLAYER_ANIM_XBOW_ATTACK 7552
#define PLAYER_ANIM_BLOWPIPE_ATTACK 5061
#define PLAYER_ANIM_EAT    829   /* human_eat */
#define PLAYER_ANIM_DEATH  836   /* human_death */

typedef struct {
    uint16_t idle_anim;
    uint16_t walk_anim;
    uint16_t walk_back_anim;
    uint16_t walk_left_anim;
    uint16_t walk_right_anim;
    uint16_t turn_anim;
    uint16_t run_anim;
    uint16_t attack_anim;
    uint32_t projectile_travel_spot;
    uint32_t projectile_launch_spot;
    uint32_t projectile_impact_spot;
    Color projectile_color;
    float projectile_radius;
    float projectile_start_height;
    float projectile_end_height;
    float projectile_launch_delay_client_ticks;
    float projectile_angle;
    float projectile_length_adjustment;
    float projectile_progress;
    float projectile_step_multiplier;
} PlayerVisualProfile;

/* Visuals are keyed by loadout because FC_WEAPON_GENERIC_RANGED includes
 * bows, crossbows, and the blowpipe. IDs come from RuneC's RSMod-enriched
 * combat visual table and the matching OSRS cache spotanim definitions. */
static const PlayerVisualProfile PLAYER_VISUALS[FC_NUM_LOADOUTS] = {
    [FC_LOADOUT_BLACK_DHIDE_RCB] = {
        PLAYER_ANIM_XBOW_IDLE, PLAYER_ANIM_XBOW_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_XBOW_RUN, PLAYER_ANIM_XBOW_ATTACK,
        27, 0, 0, {200, 200, 50, 255}, 0.12f,
        155.0f, 146.0f, 41.0f, 5.0f, 5.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_SOTA_TBOW] = {
        PLAYER_ANIM_HUMAN_IDLE, PLAYER_ANIM_HUMAN_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_HUMAN_RUN, PLAYER_ANIM_BOW_ATTACK,
        1120, 1116, 0, {190, 120, 55, 255}, 0.13f,
        163.0f, 146.0f, 41.0f, 15.0f, 5.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_LOW_DEF_RCB] = {
        PLAYER_ANIM_XBOW_IDLE, PLAYER_ANIM_XBOW_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_XBOW_RUN, PLAYER_ANIM_XBOW_ATTACK,
        27, 0, 0, {200, 200, 50, 255}, 0.12f,
        155.0f, 146.0f, 41.0f, 5.0f, 5.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_RCB_PURE] = {
        PLAYER_ANIM_XBOW_IDLE, PLAYER_ANIM_XBOW_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_XBOW_RUN, PLAYER_ANIM_XBOW_ATTACK,
        27, 0, 0, {200, 200, 50, 255}, 0.12f,
        155.0f, 146.0f, 41.0f, 5.0f, 5.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_MSBI_PURE] = {
        PLAYER_ANIM_HUMAN_IDLE, PLAYER_ANIM_HUMAN_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_HUMAN_RUN, PLAYER_ANIM_BOW_ATTACK,
        15, 24, 0, {145, 155, 165, 255}, 0.10f,
        163.0f, 146.0f, 41.0f, 15.0f, 5.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_BLOWPIPE_PURE] = {
        PLAYER_ANIM_HUMAN_IDLE, PLAYER_ANIM_HUMAN_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_HUMAN_RUN, PLAYER_ANIM_BLOWPIPE_ATTACK,
        230, 236, 0, {115, 175, 85, 255}, 0.09f,
        163.0f, 146.0f, 32.0f, 15.0f, 0.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_ACB_ARMADYL] = {
        PLAYER_ANIM_XBOW_IDLE, PLAYER_ANIM_XBOW_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_XBOW_RUN, PLAYER_ANIM_XBOW_ATTACK,
        1468, 0, 0, {165, 210, 240, 255}, 0.12f,
        155.0f, 146.0f, 41.0f, 5.0f, 5.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_BOWFA_CRYSTAL] = {
        PLAYER_ANIM_HUMAN_IDLE, PLAYER_ANIM_HUMAN_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_HUMAN_RUN, PLAYER_ANIM_BOW_ATTACK,
        1922, 1923, 0, {120, 235, 225, 255}, 0.13f,
        163.0f, 146.0f, 41.0f, 15.0f, 5.0f, 11.0f, 5.0f,
    },
    [FC_LOADOUT_TBOW_MASORI] = {
        PLAYER_ANIM_HUMAN_IDLE, PLAYER_ANIM_HUMAN_WALK,
        PLAYER_ANIM_HUMAN_WALK_BACK, PLAYER_ANIM_HUMAN_WALK_LEFT,
        PLAYER_ANIM_HUMAN_WALK_RIGHT, PLAYER_ANIM_HUMAN_TURN,
        PLAYER_ANIM_HUMAN_RUN, PLAYER_ANIM_BOW_ATTACK,
        1120, 1116, 0, {190, 120, 55, 255}, 0.13f,
        163.0f, 146.0f, 41.0f, 15.0f, 5.0f, 11.0f, 5.0f,
    },
};

static const PlayerVisualProfile* player_visual_profile(int loadout) {
    if (loadout < 0 || loadout >= FC_NUM_LOADOUTS)
        loadout = FC_ACTIVE_LOADOUT;
    return &PLAYER_VISUALS[loadout];
}

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
#define PROJ_JAD_MAGIC_LAUNCH   439
#define PROJ_TOK_XIL_SPINE      443
#define PROJ_TOK_XIL_IMPACT     444
#define PROJ_KET_ZEK_FIRE       445
#define PROJ_KET_ZEK_IMPACT     446
#define PROJ_JAD_MAGIC_TRAVEL   448
#define PROJ_JAD_MAGIC_IMPACT   157
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
    int actor_kind;        /* FcVisualTargetKind; keeps splat on moving actor */
    int actor_slot;
    int overlay_slot;      /* native four-splat screen-space layout */
    int damage;           /* damage in tenths (0 = miss) */
    int kind;             /* HitsplatKind */
    float seconds_left;
} Hitsplat;

/* Visual projectile — travels from source to destination over tick duration */
#define MAX_PROJECTILES 16
typedef struct {
    int active;
    float src_x, src_y, src_z;
    float dst_x, dst_y, dst_z;
    float x, y, z;
    float velocity_x, velocity_y, velocity_z;
    float total_time;
    float elapsed;
    float launch_delay;
    int launched;
    FcVisualTargetKind source_kind;
    int source_slot;
    float source_y_offset;
    FcVisualTargetKind target_kind;
    int target_slot;
    float target_y_offset;
    int track_target;
    int attack_style;
    int launch_tick;
    int has_deferred_hitsplat;
    FcVisualTargetKind hitsplat_actor_kind;
    int hitsplat_actor_slot;
    float hitsplat_world_x, hitsplat_world_y, hitsplat_world_z;
    int hitsplat_damage;
    Color color;
    float radius;
    uint32_t spot_id;            /* travel spotanim ID (0 = no travel visual) */
    uint32_t launch_spot_id;
    uint32_t impact_spot_id;
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
    int attached;
    FcVisualTargetKind attached_kind;
    int attached_slot;
    float attached_y_offset;
    FcVisualTargetKind face_kind;
    int face_slot;
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
    FcRenderEvents render_events;
    FcVisualScene visual_scene;
    RuneCUiState ui;
    FcRenderEntity entities[FC_MAX_RENDER_ENTITIES];
    int entity_count;
    int paused, step_once;
    float tps;
    float tick_acc;
    int show_grid, show_collision;
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
    FcMinimapScene minimap_scene;
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
    int player_attack_visual_target_idx;
    float prayer_flick_visual_timer;
    /* Per-NPC animation state */
    AnimModelState* npc_anim_states[FC_MAX_NPCS];
    uint16_t npc_anim_seq[FC_MAX_NPCS];
    int npc_anim_frame[FC_MAX_NPCS];
    float npc_anim_timer[FC_MAX_NPCS];
    uint16_t npc_action_anim_seq[FC_MAX_NPCS];
    int npc_action_anim_frame[FC_MAX_NPCS];
    float npc_action_anim_timer[FC_MAX_NPCS];
    int npc_attack_visual_style[FC_MAX_NPCS];
    float npc_attack_visual_timer[FC_MAX_NPCS];
    float npc_prayer_indicator_timer[FC_MAX_NPCS];
    int npc_prayer_lock_tick[FC_MAX_NPCS];
    /* Hitsplats */
    Hitsplat hitsplats[MAX_HITSPLATS];
    float player_healthbar_timer;
    float npc_healthbar_timer[FC_MAX_NPCS];
    /* Buffered key inputs (captured every frame, consumed on tick) */
    int pending_prayer, pending_eat, pending_drink;
    int pending_attack_npc;
    int pending_tile_x, pending_tile_y;
    FcClickFeedback click_feedback;
    int console_tab;              /* controls, player, obs, mask, reward, log */
    int console_wave_dropdown_open;
    int console_scroll[4];        /* player/obs/mask/reward vertical offsets */
    int console_content_height[4];
    /* Visual projectiles in flight */
    VisualProjectile projectiles[MAX_PROJECTILES];
    VisualEffect effects[MAX_VISUAL_EFFECTS];
    /* Prayer overhead icon textures */
    Texture2D pray_melee_tex, pray_missiles_tex, pray_magic_tex;
    /* Cache-authored OSRS actor overhead sprites. */
    Texture2D hitsplat_zero_tex, hitsplat_damage_tex;
    Texture2D hitsplat_heal_tex, hitsplat_prayer_drain_tex;
    Texture2D healthbar_full_tex, healthbar_empty_tex;
    Texture2D click_cross_tex[FC_CLICK_CROSS_FRAME_COUNT * 2];
    int active_loadout; /* index into FC_LOADOUTS[] */
    int combat_style;   /* 0=accurate, 1=rapid, 2=long range */
    /* Prayer-tab sprites used by the active RuneC side interface. */
    Texture2D tex_pray_melee_on, tex_pray_melee_off;
    Texture2D tex_pray_range_on, tex_pray_range_off;
    Texture2D tex_pray_magic_on, tex_pray_magic_off;
    /* Agent action test mode (Phase 9a verification) */
    int test_mode;       /* 1 = running scripted agent tests */
    int test_id;         /* current test index */
    int test_tick;       /* ticks elapsed in current test */
    /* Debug overlay (Phase 9c) — toggled with O key */
    int dbg_flags;       /* bitmask of DBG_* flags from fc_debug_overlay.h */
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
    /* Previous authoritative NPC snapshots feed the presentation-only actor
     * path queue after each simulator tick. */
    float prev_npc_x[FC_MAX_NPCS];
    float prev_npc_y[FC_MAX_NPCS];
    int prev_npc_active[FC_MAX_NPCS];  /* was this NPC active last tick? */
} ViewerState;

/* Forward declarations */
static void draw_tex_fit(Texture2D tex, int dx, int dy, int dw, int dh,
                         Color tint);
static int viewer_render_prayer(const ViewerState* v);

static void viewer_trace_log_to_stderr(int log_level, const char* text,
                                       va_list args) {
    (void)log_level;
    vfprintf(stderr, text, args);
    fputc('\n', stderr);
}

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
    v->ui.active_prayers = fc_ui_active_prayer_bits(viewer_render_prayer(v));
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

static void sync_fc_ui_minimap(ViewerState* v) {
    if (!v) return;
    FcPlayer* p = &v->state.player;
    FcVisualPose player_pose = fc_visual_scene_player_pose(&v->visual_scene);
    float player_x = v->visual_scene.player.active
        ? player_pose.x : (float)p->x + 0.5f;
    float player_y = v->visual_scene.player.active
        ? player_pose.y : (float)p->y + 0.5f;
    Color pixels[FC_MINIMAP_DISPLAY_SIZE * FC_MINIMAP_DISPLAY_SIZE];
    fc_minimap_render(&v->minimap_scene, player_x, player_y, v->cam_yaw,
                      pixels);
    runec_ui_update_minimap(&v->ui, pixels, FC_MINIMAP_DISPLAY_SIZE,
                            FC_MINIMAP_DISPLAY_SIZE);
    runec_ui_set_minimap_rotation(&v->ui, v->cam_yaw);

    runec_ui_clear_minimap(&v->ui);
    runec_ui_add_minimap_dot(&v->ui, 0.0f, 0.0f, RUNEC_UI_MINIMAP_DOT_PLAYER);
    if (v->click_feedback.destination_active) {
        int tx = v->click_feedback.destination_x;
        int ty = v->click_feedback.destination_y;
        Vector2 offset = fc_minimap_rotate_offset(
            (float)tx + 0.5f - player_x,
            (float)ty + 0.5f - player_y, v->cam_yaw);
        runec_ui_add_minimap_dot(&v->ui, offset.x, offset.y,
                                 RUNEC_UI_MINIMAP_DOT_DESTINATION);
    } else if (p->route_idx < p->route_len && p->route_len > 0) {
        int tx = p->route_x[p->route_len - 1];
        int ty = p->route_y[p->route_len - 1];
        Vector2 offset = fc_minimap_rotate_offset(
            (float)tx + 0.5f - player_x,
            (float)ty + 0.5f - player_y, v->cam_yaw);
        runec_ui_add_minimap_dot(&v->ui, offset.x, offset.y,
                                 RUNEC_UI_MINIMAP_DOT_DESTINATION);
    }
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* n = &v->state.npcs[i];
        if (!n->active || n->is_dead) continue;
        FcVisualPose npc_pose = fc_visual_scene_npc_pose(&v->visual_scene, i);
        float npc_x = v->visual_scene.npcs[i].active
            ? npc_pose.x : (float)n->x + (float)n->size * 0.5f;
        float npc_y = v->visual_scene.npcs[i].active
            ? npc_pose.y : (float)n->y + (float)n->size * 0.5f;
        Vector2 offset = fc_minimap_rotate_offset(
            npc_x - player_x, npc_y - player_y, v->cam_yaw);
        runec_ui_add_minimap_dot(&v->ui, offset.x, offset.y,
                                 RUNEC_UI_MINIMAP_DOT_NPC);
    }
}

static void sync_fc_ui(ViewerState* v) {
    sync_fc_ui_items(v);
    sync_fc_ui_status(v);
    sync_fc_ui_minimap(v);
}

static void queue_player_tile_request(ViewerState* v, int tx, int ty,
                                      float screen_x, float screen_y) {
    if (!v || tx < 0 || tx >= FC_ARENA_WIDTH || ty < 0 || ty >= FC_ARENA_HEIGHT)
        return;
    v->pending_tile_x = tx;
    v->pending_tile_y = ty;
    v->pending_attack_npc = -1;
    fc_click_feedback_select_move(&v->click_feedback, &v->state, tx, ty,
                                  screen_x, screen_y);
}

static void queue_player_attack_request(ViewerState* v, int npc_idx,
                                        float screen_x, float screen_y) {
    if (!v || npc_idx < 0 || npc_idx >= FC_MAX_NPCS) return;
    v->pending_attack_npc = npc_idx;
    v->pending_tile_x = -1;
    v->pending_tile_y = -1;
    fc_click_feedback_select_interaction(&v->click_feedback,
                                         screen_x, screen_y);
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
            fc_request_set_running(&v->state, !p->is_running);
            break;
        case RUNEC_UI_INTENT_MINIMAP_CLICK: {
            FcVisualPose player_pose =
                fc_visual_scene_player_pose(&v->visual_scene);
            float player_x = v->visual_scene.player.active
                ? player_pose.x : (float)p->x + 0.5f;
            float player_y = v->visual_scene.player.active
                ? player_pose.y : (float)p->y + 0.5f;
            int tile_x = -1;
            int tile_y = -1;
            Vector2 mouse = GetMousePosition();
            if (fc_minimap_click_to_tile(
                    (float)intent->primary, (float)intent->secondary,
                    player_x, player_y, v->cam_yaw, &tile_x, &tile_y)) {
                queue_player_tile_request(v, tile_x, tile_y,
                                          mouse.x, mouse.y);
            }
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
        *frame_timer = (float)seq->frames[0].delay * 0.02f;
        if (*frame_timer < 0.016f) *frame_timer = 0.016f;
    }
    if (*frame_index < 0 || *frame_index >= seq->frame_count)
        *frame_index = 0;

    *frame_timer -= dt;
    while (*frame_timer <= 0.0f) {
        *frame_index += 1;
        if (*frame_index >= seq->frame_count) {
            if (seq->frame_step > 0 && seq->frame_step <= seq->frame_count)
                *frame_index -= seq->frame_step;
            else
                *frame_index = 0;
        }
        float frame_delay = (float)seq->frames[*frame_index].delay * 0.02f;
        if (frame_delay < 0.016f) frame_delay = 0.016f;
        *frame_timer += frame_delay;
    }

    return seq;
}

static float anim_frame_duration_seconds(const AnimSequence* seq,
                                         int frame_index) {
    if (!seq || frame_index < 0 || frame_index >= seq->frame_count)
        return 0.016f;
    float duration = (float)seq->frames[frame_index].delay * 0.02f;
    return duration < 0.016f ? 0.016f : duration;
}

static float anim_track_duration_seconds(const AnimSequence* seq) {
    if (!seq || seq->frame_count <= 0) return 0.0f;
    float duration = 0.0f;
    for (int i = 0; i < seq->frame_count; i++)
        duration += anim_frame_duration_seconds(seq, i);
    return duration;
}

/* Movement direction changes do not restart the client movement cycle. Map
 * the elapsed phase into the newly selected locomotion sequence so rapid
 * forward/side/back changes cannot pin the legs to frame zero. */
static void retarget_anim_track_preserving_phase(
    AnimCache* cache, uint16_t desired_seq, uint16_t* current_seq,
    int* frame_index, float* frame_timer) {
    if (!cache || desired_seq == 0 || !current_seq || !frame_index ||
        !frame_timer || *current_seq == 0 || *current_seq == desired_seq)
        return;

    AnimSequence* old_seq = anim_get_sequence(cache, *current_seq);
    AnimSequence* new_seq = anim_get_sequence(cache, desired_seq);
    if (!old_seq || old_seq->frame_count <= 0 ||
        !new_seq || new_seq->frame_count <= 0)
        return;

    int old_frame = *frame_index;
    if (old_frame < 0 || old_frame >= old_seq->frame_count)
        old_frame = 0;
    float old_total = anim_track_duration_seconds(old_seq);
    float new_total = anim_track_duration_seconds(new_seq);
    if (old_total <= 0.0f || new_total <= 0.0f) return;

    float old_elapsed = 0.0f;
    for (int i = 0; i < old_frame; i++)
        old_elapsed += anim_frame_duration_seconds(old_seq, i);
    float old_frame_duration = anim_frame_duration_seconds(old_seq, old_frame);
    float old_remaining = *frame_timer;
    if (old_remaining < 0.0f) old_remaining = 0.0f;
    if (old_remaining > old_frame_duration) old_remaining = old_frame_duration;
    old_elapsed += old_frame_duration - old_remaining;

    float target_elapsed = fmodf(old_elapsed, old_total) / old_total * new_total;
    float elapsed_before_frame = 0.0f;
    int new_frame = 0;
    for (; new_frame < new_seq->frame_count - 1; new_frame++) {
        float duration = anim_frame_duration_seconds(new_seq, new_frame);
        if (target_elapsed < elapsed_before_frame + duration) break;
        elapsed_before_frame += duration;
    }

    float new_frame_duration = anim_frame_duration_seconds(new_seq, new_frame);
    *current_seq = desired_seq;
    *frame_index = new_frame;
    *frame_timer = elapsed_before_frame + new_frame_duration - target_elapsed;
    if (*frame_timer < 0.001f) *frame_timer = 0.001f;
}

static int player_profile_is_movement_sequence(
    const PlayerVisualProfile* profile, uint16_t sequence_id) {
    if (!profile || sequence_id == 0) return 0;
    return sequence_id == profile->walk_anim ||
           sequence_id == profile->walk_back_anim ||
           sequence_id == profile->walk_left_anim ||
           sequence_id == profile->walk_right_anim ||
           sequence_id == profile->run_anim;
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
        *frame_timer = (float)seq->frames[0].delay * 0.02f;
        if (*frame_timer < 0.016f) *frame_timer = 0.016f;
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

static float anim_sequence_duration_client_cycles(const AnimSequence* seq) {
    if (!seq || seq->frame_count <= 0) return 0.0f;
    float total = 0.0f;
    for (int i = 0; i < seq->frame_count; i++)
        total += seq->frames[i].delay > 0 ? seq->frames[i].delay : 1;
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
        *frame_timer = (float)seq->frames[*frame_index].delay * 0.02f;
        if (*frame_timer < 0.016f) *frame_timer = 0.016f;
    }

    if (*current_seq != (uint16_t)animation_id) {
        *current_seq = (uint16_t)animation_id;
        *frame_index = 0;
        *frame_timer = (float)seq->frames[0].delay * 0.02f;
        if (*frame_timer < 0.016f) *frame_timer = 0.016f;
    }

    *frame_timer -= dt;
    while (*frame_timer <= 0.0f) {
        *frame_index = (*frame_index + 1) % seq->frame_count;
        float frame_delay = (float)seq->frames[*frame_index].delay * 0.02f;
        if (frame_delay < 0.016f) frame_delay = 0.016f;
        *frame_timer += frame_delay;
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

static int player_visual_lock_active(const ViewerState* v);

static void clear_visuals(ViewerState* v) {
    if (!v) return;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        free_projectile(&v->projectiles[i]);
    for (int i = 0; i < MAX_VISUAL_EFFECTS; i++)
        free_effect(&v->effects[i]);
}

static void reset_visual_actor_scene(ViewerState* v) {
    if (!v) return;
    fc_visual_scene_init(&v->visual_scene);
    fc_visual_scene_reset_player(&v->visual_scene,
                                 v->state.player.x, v->state.player.y, 1,
                                 v->state.player.facing_angle);
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* npc = &v->state.npcs[i];
        if (npc->active || npc->died_this_tick) {
            fc_visual_scene_reset_npc(&v->visual_scene, i,
                                      npc->x, npc->y, npc->size,
                                      0.0f);
        }
    }
}

static void ingest_visual_actor_tick(ViewerState* v) {
    if (!v) return;
    FcVisualActor* player = &v->visual_scene.player;
    if (!player->active) {
        fc_visual_scene_reset_player(&v->visual_scene,
                                     v->render_events.player_move_start_x,
                                     v->render_events.player_move_start_y,
                                     1, v->state.player.facing_angle);
    }

    int waypoint_count = v->render_events.player_move_waypoint_count;
    int running = waypoint_count > 1;
    for (int i = 0; i < waypoint_count; i++) {
        fc_visual_actor_enqueue_tile(
            player,
            v->render_events.player_move_waypoint_x[i],
            v->render_events.player_move_waypoint_y[i],
            running);
    }
    if (waypoint_count == 0 &&
        (player->server_tile_x != v->state.player.x ||
         player->server_tile_y != v->state.player.y)) {
        fc_visual_actor_enqueue_transition(
            player, player->server_tile_x, player->server_tile_y,
            v->state.player.x, v->state.player.y,
            v->state.player.is_running);
    }

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* npc = &v->state.npcs[i];
        FcVisualActor* visual = &v->visual_scene.npcs[i];
        if (npc->active && !v->prev_npc_active[i]) {
            fc_visual_scene_reset_npc(&v->visual_scene, i,
                                      npc->x, npc->y, npc->size, 0.0f);
        } else if ((npc->active || npc->died_this_tick) && visual->active) {
            fc_visual_actor_enqueue_transition(
                visual, (int)v->prev_npc_x[i], (int)v->prev_npc_y[i],
                npc->x, npc->y, 0);
        } else if (!npc->active && !npc->died_this_tick) {
            fc_visual_scene_deactivate_npc(&v->visual_scene, i);
        }
    }
}

static void update_visual_actor_targets(ViewerState* v) {
    if (!v) return;
    int target = player_visual_lock_active(v)
        ? v->player_attack_visual_target_idx
        : v->state.player.attack_target_idx;
    if (target >= 0 && target < FC_MAX_NPCS &&
        v->visual_scene.npcs[target].active) {
        fc_visual_actor_set_target(&v->visual_scene.player,
                                   FC_VISUAL_TARGET_NPC, target);
    } else {
        fc_visual_actor_set_target(&v->visual_scene.player,
                                   FC_VISUAL_TARGET_NONE, -1);
    }

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcVisualActor* actor = &v->visual_scene.npcs[i];
        if (actor->active && v->state.npcs[i].active) {
            int heal_target = v->state.npcs[i].heal_target_idx;
            if (heal_target >= 0 && heal_target < FC_MAX_NPCS &&
                heal_target != i && v->visual_scene.npcs[heal_target].active) {
                fc_visual_actor_set_target(actor, FC_VISUAL_TARGET_NPC,
                                           heal_target);
            } else if (heal_target == i) {
                fc_visual_actor_set_target(actor, FC_VISUAL_TARGET_NONE, -1);
            } else {
                fc_visual_actor_set_target(actor, FC_VISUAL_TARGET_PLAYER, 0);
            }
        } else {
            fc_visual_actor_set_target(actor, FC_VISUAL_TARGET_NONE, -1);
        }
    }
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
    const PlayerVisualProfile* profile = player_visual_profile(v->active_loadout);
    v->player_anim_seq = profile->idle_anim;
    v->player_anim_frame = 0;
    v->player_anim_timer = 0.0f;
    v->player_pose_anim_seq = profile->idle_anim;
    v->player_pose_anim_frame = 0;
    v->player_pose_anim_timer = 0.0f;
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
    v->player_visual_lock_seq = 0;
    v->player_visual_lock_timer = 0.0f;
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

static float projectile_effect_duration(ViewerState* v, uint32_t spot_id,
                                        float retain_client_cycles) {
    float animation_client_cycles = 0.0f;
    const SpotAnimDef* spot = (v && v->spotanims && spot_id > 0)
        ? spotanim_find(v->spotanims, (int)spot_id) : NULL;
    if (spot && spot->animation_id >= 0 && v->anim_cache) {
        AnimSequence* seq = anim_get_sequence(
            v->anim_cache, (uint16_t)spot->animation_id);
        animation_client_cycles = anim_sequence_duration_client_cycles(seq);
    }
    return fc_projectile_effect_duration_seconds(
        animation_client_cycles, retain_client_cycles,
        v && v->tps > 0.0f ? v->tps : (float)POLICY_REPLAY_BASE_TPS);
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
    v->npc_prayer_indicator_timer[npc_idx] = 0.30f;
}

static float policy_replay_time_scale(const ViewerState* v) {
    if (!v || v->tps <= 0.0f) return 1.0f;
    float scale = v->tps / (float)POLICY_REPLAY_BASE_TPS;
    if (scale < 0.05f) scale = 0.05f;
    if (scale > 36.0f) scale = 36.0f;
    return scale;
}

static float policy_replay_anim_dt(const ViewerState* v, float dt) {
    return dt * policy_replay_time_scale(v);
}

static float policy_replay_duration(const ViewerState* v, float seconds) {
    float scale = policy_replay_time_scale(v);
    seconds /= scale;
    if (seconds < 0.05f) seconds = 0.05f;
    return seconds;
}

static void mark_player_attack_visual(ViewerState* v) {
    if (!v || !v->render_events.player_attack_fired) return;
    const PlayerVisualProfile* profile = player_visual_profile(v->active_loadout);
    AnimSequence* seq = v->anim_cache
        ? anim_get_sequence(v->anim_cache, profile->attack_anim) : NULL;
    v->player_visual_lock_seq = profile->attack_anim;
    v->player_visual_lock_timer = policy_replay_duration(
        v, anim_sequence_duration_seconds(seq));
    v->player_attack_visual_target_idx =
        v->render_events.player_attack_target_npc_slot;
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
}

static int player_visual_lock_active(const ViewerState* v) {
    return v && v->player_visual_lock_seq != 0 &&
           v->player_visual_lock_timer > 0.0f;
}

static uint16_t player_visual_action_sequence(const ViewerState* v) {
    if (!v) return 0;
    if (v->state.terminal == TERMINAL_PLAYER_DEATH) return PLAYER_ANIM_DEATH;
    if (v->state.player.food_eaten_this_tick) return PLAYER_ANIM_EAT;
    if (player_visual_lock_active(v)) return v->player_visual_lock_seq;
    return 0;
}

static int visual_sequence_blocks_movement(const ViewerState* v,
                                           uint16_t sequence_id) {
    if (!v || !v->anim_cache || sequence_id == 0) return 0;
    AnimSequence* sequence = anim_get_sequence(v->anim_cache, sequence_id);
    return sequence && sequence->postanim_move == 0;
}

static int viewer_render_prayer(const ViewerState* v) {
    if (!v || v->prayer_flick_visual_timer > 0.0f)
        return PRAYER_NONE;
    return v->state.player.prayer;
}

static void mark_prayer_flick_visual(ViewerState* v) {
    if (!v || !v->render_events.prayer_flick_performed) return;
    v->prayer_flick_visual_timer = policy_replay_duration(v, 0.10f);
}

static void text_s(const char* t, int x, int y, int sz, Color c) {
    fc_osrs_draw_text(t, x+1, y+1, sz, COL_TEXT_SHADOW);
    fc_osrs_draw_text(t, x, y, sz, c);
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
    fprintf(stderr, "[policy-pipe] Replay speed set to %dx (%.2f TPS)\n",
            normalized, v->tps);
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

static void toggle_debug_overlay(ViewerState* v) {
    if (!v) return;
    v->dbg_flags = v->dbg_flags ? 0 : DBG_ALL;
}

static void toggle_godmode(ViewerState* v) {
    if (!v) return;
    v->godmode = !v->godmode;
    fprintf(stderr, "GODMODE: %s\n", v->godmode ? "ON" : "OFF");
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
    fc_fill_render_events(&v->state, &v->render_events);
    v->last_hash = fc_state_hash(&v->state);
    v->episode_count++;
    v->attack_target = -1;
    memset(v->actions, 0, sizeof(v->actions));
    memset(v->hitsplats, 0, sizeof(v->hitsplats));
    v->player_healthbar_timer = 0.0f;
    memset(v->npc_healthbar_timer, 0, sizeof(v->npc_healthbar_timer));
    clear_visuals(v);
    reset_visual_actor_scene(v);
    v->pending_prayer = 0;
    v->pending_eat = 0;
    v->pending_drink = 0;
    v->pending_attack_npc = -1;
    v->pending_tile_x = -1;
    v->pending_tile_y = -1;
    fc_click_feedback_reset(&v->click_feedback);
    dbg_log_clear();
    /* Initialize prev positions */
    v->player_pose_anim_seq =
        player_visual_profile(v->active_loadout)->idle_anim;
    v->player_pose_anim_frame = 0;
    v->player_pose_anim_timer = 0.0f;
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
    v->player_visual_lock_seq = 0;
    v->player_visual_lock_timer = 0.0f;
    v->player_attack_visual_target_idx = -1;
    v->prayer_flick_visual_timer = 0.0f;
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
        v->npc_action_anim_seq[i] = 0;
        v->npc_action_anim_frame[i] = 0;
        v->npc_action_anim_timer[i] = 0.0f;
        v->npc_attack_visual_style[i] = ATTACK_NONE;
        v->npc_attack_visual_timer[i] = 0.0f;
        v->npc_prayer_indicator_timer[i] = 0.0f;
        v->npc_prayer_lock_tick[i] = -1;
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
    fc_fill_render_events(&v->state, &v->render_events);
    memset(v->hitsplats, 0, sizeof(v->hitsplats));
    v->player_healthbar_timer = 0.0f;
    memset(v->npc_healthbar_timer, 0, sizeof(v->npc_healthbar_timer));
    clear_visuals(v);
    reset_visual_actor_scene(v);
    v->attack_target = -1;
    fc_click_feedback_reset(&v->click_feedback);
    dbg_log_clear();
    v->player_pose_anim_seq =
        player_visual_profile(v->active_loadout)->idle_anim;
    v->player_pose_anim_frame = 0;
    v->player_pose_anim_timer = 0.0f;
    v->player_action_anim_seq = 0;
    v->player_action_anim_frame = 0;
    v->player_action_anim_timer = 0.0f;
    v->player_visual_lock_seq = 0;
    v->player_visual_lock_timer = 0.0f;
    v->player_attack_visual_target_idx = -1;
    v->prayer_flick_visual_timer = 0.0f;

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
        v->npc_action_anim_seq[i] = 0;
        v->npc_action_anim_frame[i] = 0;
        v->npc_action_anim_timer[i] = 0.0f;
        v->npc_attack_visual_style[i] = ATTACK_NONE;
        v->npc_attack_visual_timer[i] = 0.0f;
        v->npc_prayer_indicator_timer[i] = 0.0f;
        v->npc_prayer_lock_tick[i] = -1;
    }
}

static float ground_y(ViewerState* v, int tile_x, int tile_y);
static float ground_y_smooth(ViewerState* v, float tile_x, float tile_y);

static VisualEffect* spawn_spot_effect(ViewerState* v, uint32_t spot_id,
                                       float x, float y, float z,
                                       float duration, Color col, float radius,
                                       float yaw_degrees) {
    if (!v || spot_id == 0) return NULL;
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
            return fx;
        }
    }
    return NULL;
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
            vp->x = sx; vp->y = sy; vp->z = sz;
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

static int visual_actor_world_point(ViewerState* v,
                                    FcVisualTargetKind kind, int slot,
                                    float* x, float* y, float* z) {
    if (!v || !x || !y || !z) return 0;
    FcVisualPose pose;
    int size;
    float height;
    if (kind == FC_VISUAL_TARGET_PLAYER) {
        if (!v->visual_scene.player.active) return 0;
        pose = fc_visual_scene_player_pose(&v->visual_scene);
        size = 1;
        height = 1.5f;
    } else if (kind == FC_VISUAL_TARGET_NPC &&
               slot >= 0 && slot < FC_MAX_NPCS &&
               v->visual_scene.npcs[slot].active) {
        pose = fc_visual_scene_npc_pose(&v->visual_scene, slot);
        size = v->visual_scene.npcs[slot].size;
        height = 1.0f + (float)size * 0.3f;
    } else {
        return 0;
    }
    *x = pose.x;
    *z = -pose.y;
    *y = ground_y_smooth(v, pose.x, pose.y) + height;
    return 1;
}

static int projectile_tile_distance(int source_x, int source_y,
                                    int target_x, int target_y) {
    int dx = abs(target_x - source_x);
    int dy = abs(target_y - source_y);
    return dx > dy ? dx : dy;
}

static void configure_projectile_tracking(
    ViewerState* v, VisualProjectile* vp,
    FcVisualTargetKind source_kind, int source_slot,
    FcVisualTargetKind target_kind, int target_slot,
    int attack_style,
    float launch_delay_client_ticks, float end_time_client_ticks,
    float angle, float progress, int track_target) {
    if (!v || !vp) return;
    vp->source_kind = source_kind;
    vp->source_slot = source_slot;
    vp->target_kind = target_kind;
    vp->target_slot = target_slot;
    vp->track_target = track_target;
    vp->attack_style = attack_style;
    vp->launch_tick = v->state.tick;
    vp->projectile_angle = angle >= 0.0f ? angle : 15.0f;
    vp->projectile_progress = progress >= 0.0f ? progress : 0.0f;
    /* RuneC's launch and endpoint metadata is already the client-authoritative
     * visual clock. Preserve those client cycles instead of compressing the
     * path into the simulator's coarser whole-tick pending-hit countdown. */
    FcProjectileTiming timing = {0};
    if (fc_projectile_timing_from_client_cycles(
            launch_delay_client_ticks, end_time_client_ticks,
            v->tps, &timing)) {
        vp->launch_delay = timing.launch_delay;
        vp->total_time = timing.total_duration;
    } else {
        vp->launch_delay = 0.0f;
    }

    {
        float resolved_x, resolved_y, resolved_z;
        if (visual_actor_world_point(v, source_kind, source_slot,
                                     &resolved_x, &resolved_y, &resolved_z)) {
            vp->source_y_offset = vp->src_y - resolved_y;
            vp->src_x = resolved_x;
            vp->src_y = resolved_y + vp->source_y_offset;
            vp->src_z = resolved_z;
        }
        if (track_target &&
            visual_actor_world_point(v, target_kind, target_slot,
                                     &resolved_x, &resolved_y, &resolved_z)) {
            vp->target_y_offset = vp->dst_y - resolved_y;
            vp->dst_x = resolved_x;
            vp->dst_y = resolved_y + vp->target_y_offset;
            vp->dst_z = resolved_z;
        }
    }
    vp->x = vp->src_x;
    vp->y = vp->src_y;
    vp->z = vp->src_z;

    if (vp->launch_spot_id != 0) {
        float launch_retain = launch_delay_client_ticks > 30.0f
            ? launch_delay_client_ticks : 30.0f;
        float launch_duration = projectile_effect_duration(
            v, vp->launch_spot_id, launch_retain);
        float launch_yaw = atan2f(vp->dst_x - vp->src_x,
                                  vp->dst_z - vp->src_z) * RAD2DEG;
        VisualEffect* launch = spawn_spot_effect(
            v, vp->launch_spot_id,
            vp->src_x, vp->src_y, vp->src_z,
            launch_duration, vp->color, vp->radius * 1.4f, launch_yaw);
        if (launch) {
            launch->attached = 1;
            launch->attached_kind = source_kind;
            launch->attached_slot = source_slot;
            launch->face_kind = target_kind;
            launch->face_slot = target_slot;
            {
                float x, y, z;
                if (visual_actor_world_point(v, source_kind, source_slot,
                                             &x, &y, &z))
                    launch->attached_y_offset = launch->y - y;
            }
        }
    }
}

static void refresh_projectile_actor_points(ViewerState* v,
                                            VisualProjectile* vp,
                                            int refresh_source) {
    float x, y, z;
    if (refresh_source && visual_actor_world_point(
            v, vp->source_kind, vp->source_slot, &x, &y, &z)) {
        vp->src_x = x;
        vp->src_y = y + vp->source_y_offset;
        vp->src_z = z;
    }
    if (vp->track_target && visual_actor_world_point(
            v, vp->target_kind, vp->target_slot, &x, &y, &z)) {
        vp->dst_x = x;
        vp->dst_y = y + vp->target_y_offset;
        vp->dst_z = z;
    }
}

static int update_visual_projectile(ViewerState* v, VisualProjectile* vp,
                                    float dt) {
    if (!v || !vp || !vp->active || dt <= 0.0f) return 0;
    float start_elapsed = vp->elapsed;
    float end_elapsed = start_elapsed + dt;
    if (end_elapsed > vp->total_time) end_elapsed = vp->total_time;

    refresh_projectile_actor_points(v, vp, !vp->launched);
    if (end_elapsed < vp->launch_delay) {
        vp->x = vp->src_x;
        vp->y = vp->src_y;
        vp->z = vp->src_z;
        vp->elapsed = end_elapsed;
        return 0;
    }

    if (!vp->launched) {
        vp->launched = 1;
    }

    float flight_duration = vp->total_time - vp->launch_delay;
    if (flight_duration < 0.001f) flight_duration = 0.001f;
    FcProjectilePath path = {
        .source_x = vp->src_x,
        .source_y = vp->src_y,
        .source_z = vp->src_z,
        .target_x = vp->dst_x,
        .target_y = vp->dst_y,
        .target_z = vp->dst_z,
        .duration = flight_duration,
        .angle = vp->projectile_angle,
        .progress = vp->projectile_progress / 128.0f,
    };
    FcProjectileSample sample = {0};
    if (fc_projectile_path_sample(
            &path, end_elapsed - vp->launch_delay, &sample)) {
        vp->x = sample.x;
        vp->y = sample.y;
        vp->z = sample.z;
        vp->velocity_x = sample.velocity_x;
        vp->velocity_y = sample.velocity_y;
        vp->velocity_z = sample.velocity_z;
    }

    vp->elapsed = end_elapsed;
    if (vp->elapsed >= vp->total_time) {
        vp->x = vp->dst_x;
        vp->y = vp->dst_y;
        vp->z = vp->dst_z;
        return 1;
    }
    return 0;
}

static void show_actor_healthbar(ViewerState* v, FcVisualTargetKind actor_kind,
                                 int actor_slot) {
    if (!v) return;
    if (actor_kind == FC_VISUAL_TARGET_PLAYER) {
        v->player_healthbar_timer = OSRS_HEALTHBAR_SECONDS;
    } else if (actor_kind == FC_VISUAL_TARGET_NPC &&
               actor_slot >= 0 && actor_slot < FC_MAX_NPCS) {
        v->npc_healthbar_timer[actor_slot] = OSRS_HEALTHBAR_SECONDS;
    }
}

static int next_hitsplat_overlay_slot(const ViewerState* v,
                                      FcVisualTargetKind actor_kind,
                                      int actor_slot) {
    unsigned int used = 0;
    if (!v) return 0;
    for (int i = 0; i < MAX_HITSPLATS; i++) {
        const Hitsplat* hit = &v->hitsplats[i];
        if (hit->active && hit->actor_kind == actor_kind &&
            hit->actor_slot == actor_slot &&
            hit->overlay_slot >= 0 && hit->overlay_slot < 4) {
            used |= 1u << hit->overlay_slot;
        }
    }
    for (int slot = 0; slot < 4; slot++) {
        if ((used & (1u << slot)) == 0) return slot;
    }
    return 0;
}

static void spawn_status_splat(ViewerState* v,
                               FcVisualTargetKind actor_kind, int actor_slot,
                               float wx, float wy, float wz,
                               int amount_tenths, int kind) {
    for (int i = 0; i < MAX_HITSPLATS; i++) {
        if (!v->hitsplats[i].active) {
            int overlay_slot = next_hitsplat_overlay_slot(
                v, actor_kind, actor_slot);
            v->hitsplats[i].active = 1;
            v->hitsplats[i].world_x = wx;
            v->hitsplats[i].world_y = wy;
            v->hitsplats[i].world_z = wz;
            v->hitsplats[i].actor_kind = actor_kind;
            v->hitsplats[i].actor_slot = actor_slot;
            v->hitsplats[i].overlay_slot = overlay_slot;
            v->hitsplats[i].damage = amount_tenths;
            v->hitsplats[i].kind = kind;
            v->hitsplats[i].seconds_left = OSRS_HITSPLAT_SECONDS;
            if (kind != HITSPLAT_PRAYER_DRAIN)
                show_actor_healthbar(v, actor_kind, actor_slot);
            return;
        }
    }
}

static void spawn_hitsplat(ViewerState* v,
                           FcVisualTargetKind actor_kind, int actor_slot,
                           float wx, float wy, float wz, int damage_tenths) {
    spawn_status_splat(v, actor_kind, actor_slot, wx, wy, wz,
                       damage_tenths, HITSPLAT_DAMAGE);
}

/* Associate a resolved ranged/magic hit with the oldest matching projectile.
 * Combat damage remains authoritative at the simulation tick boundary; only
 * the visible splat is retained until the client-side projectile arrives. */
static int defer_hitsplat_to_projectile(
    ViewerState* v, const FcRenderHit* hit,
    FcVisualTargetKind actor_kind, int actor_slot,
    float wx, float wy, float wz) {
    if (!v || !hit || hit->attack_style == ATTACK_MELEE) return 0;

    FcVisualTargetKind source_kind;
    FcVisualTargetKind target_kind;
    int source_slot;
    int target_slot;
    if (hit->target_entity_type == ENTITY_PLAYER) {
        if (hit->source_npc_slot < 0) return 0;
        source_kind = FC_VISUAL_TARGET_NPC;
        source_slot = hit->source_npc_slot;
        target_kind = FC_VISUAL_TARGET_PLAYER;
        target_slot = 0;
    } else if (hit->target_entity_type == ENTITY_NPC) {
        if (hit->target_npc_slot < 0) return 0;
        source_kind = FC_VISUAL_TARGET_PLAYER;
        source_slot = 0;
        target_kind = FC_VISUAL_TARGET_NPC;
        target_slot = hit->target_npc_slot;
    } else {
        return 0;
    }

    VisualProjectile* match = NULL;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        VisualProjectile* vp = &v->projectiles[i];
        if (!vp->active || vp->has_deferred_hitsplat ||
            vp->launch_tick >= v->state.tick ||
            vp->source_kind != source_kind || vp->source_slot != source_slot ||
            vp->target_kind != target_kind || vp->target_slot != target_slot ||
            vp->attack_style != hit->attack_style) {
            continue;
        }
        if (!match || vp->elapsed > match->elapsed) match = vp;
    }
    if (!match) return 0;

    match->has_deferred_hitsplat = 1;
    match->hitsplat_actor_kind = actor_kind;
    match->hitsplat_actor_slot = actor_slot;
    match->hitsplat_world_x = wx;
    match->hitsplat_world_y = wy;
    match->hitsplat_world_z = wz;
    match->hitsplat_damage = hit->damage;
    return 1;
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
 * Buffers authoritative actions and starts presentation-only feedback. */
static void process_human_clicks(ViewerState* v, int ui_capture) {
    FcPlayer* p = &v->state.player;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (ui_capture) return;
        Vector2 mpos = GetMousePosition();
        int tx = -1;
        int ty = -1;
        int rc = raycast_to_tile(v, &tx, &ty);
        fprintf(stderr, "CLICK mouse=(%.0f,%.0f) raycast=%d tile=(%d,%d) player=(%d,%d)",
                mpos.x, mpos.y, rc, tx, ty, p->x, p->y);
        if (rc) {
            int npc_idx = find_clicked_npc_idx(v, tx, ty);
            if (npc_idx >= 0) {
                queue_player_attack_request(v, npc_idx, mpos.x, mpos.y);
                fprintf(stderr, " → ATTACK npc_idx=%d\n", npc_idx);
            } else {
                int walkable = v->state.walkable[tx][ty];
                fprintf(stderr, " walkable=%d", walkable);
                queue_player_tile_request(v, tx, ty, mpos.x, mpos.y);
                fprintf(stderr, " → MOVE%s\n", walkable ? "" : "-NEAR");
            }
        } else {
            fprintf(stderr, " → MISS (raycast failed)\n");
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
    if (IsKeyPressed(KEY_X))
        fc_request_set_running(&v->state, !p->is_running);

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
    if (IsKeyPressed(KEY_F9)) toggle_godmode(v);
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
    v->actions[0] = FC_MOVE_IDLE;
    v->actions[1] = FC_ATTACK_NONE;
    if (v->pending_attack_npc >= 0) {
        int visible[FC_VISIBLE_NPCS];
        int count = fc_visible_npc_indices(&v->state, visible);
        for (int slot = 0; slot < count; slot++) {
            if (visible[slot] == v->pending_attack_npc) {
                v->actions[1] = slot + 1;
                break;
            }
        }
    }
    if (v->pending_tile_x >= 0 && v->pending_tile_y >= 0) {
        v->actions[5] = v->pending_tile_x + 1;
        v->actions[6] = v->pending_tile_y + 1;
    }
    /* Buffered prayer/eat/drink */
    v->actions[2] = v->pending_prayer;
    v->actions[3] = v->pending_eat;
    v->actions[4] = v->pending_drink;
    /* Clear buffers */
    v->pending_prayer = 0;
    v->pending_eat = 0;
    v->pending_drink = 0;
    v->pending_attack_npc = -1;
    v->pending_tile_x = -1;
    v->pending_tile_y = -1;
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
    int by = GetScreenHeight() - bh - 10;
    DrawRectangle(bx, by, bw, bh, CLITERAL(Color){0,0,0,200});
    DrawRectangleLinesEx((Rectangle){(float)bx,(float)by,(float)bw,(float)bh}, 2,
                         COL_TEXT_YELLOW);

    char buf[128];
    snprintf(buf, sizeof(buf), "TEST %d/%d: %s", v->test_id + 1, NUM_AGENT_TESTS, t->name);
    fc_osrs_draw_text(buf, bx + 8, by + 6, 16, COL_TEXT_YELLOW);
    fc_osrs_draw_text(t->desc, bx + 8, by + 26, 12, COL_TEXT_WHITE);

    if (v->paused && v->test_tick >= t->duration) {
        snprintf(buf, sizeof(buf), "DONE — press T for next test   (tick %d/%d)",
                 v->test_tick, t->duration);
        fc_osrs_draw_text(buf, bx + 8, by + 42, 11, COL_TEXT_GREEN);
    } else {
        snprintf(buf, sizeof(buf), "Running tick %d/%d   Player: (%d,%d)",
                 v->test_tick, t->duration,
                 v->state.player.x, v->state.player.y);
        fc_osrs_draw_text(buf, bx + 8, by + 42, 11, COL_TEXT_DIM);
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

static float ground_y_smooth(ViewerState* v, float tile_x, float tile_y) {
    int x0 = (int)floorf(tile_x);
    int y0 = (int)floorf(tile_y);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x0 >= FC_ARENA_WIDTH) x0 = FC_ARENA_WIDTH - 1;
    if (y0 >= FC_ARENA_HEIGHT) y0 = FC_ARENA_HEIGHT - 1;
    int x1 = x0 + 1 < FC_ARENA_WIDTH ? x0 + 1 : x0;
    int y1 = y0 + 1 < FC_ARENA_HEIGHT ? y0 + 1 : y0;
    float tx = tile_x - floorf(tile_x);
    float ty = tile_y - floorf(tile_y);
    float h00 = ground_y(v, x0, y0);
    float h10 = ground_y(v, x1, y0);
    float h01 = ground_y(v, x0, y1);
    float h11 = ground_y(v, x1, y1);
    float h0 = h00 + (h10 - h00) * tx;
    float h1 = h01 + (h11 - h01) * tx;
    return h0 + (h1 - h0) * ty;
}

typedef struct {
    float x;
    float y;
    float face_angle;
    int moving;
    FcVisualLocomotion locomotion;
} EntityRenderPose;

static EntityRenderPose entity_render_pose(const ViewerState* v,
                                           const FcRenderEntity* e) {
    EntityRenderPose pose = {0};
    if (!v || !e) return pose;
    FcVisualPose visual = e->entity_type == ENTITY_PLAYER
        ? fc_visual_scene_player_pose(&v->visual_scene)
        : fc_visual_scene_npc_pose(&v->visual_scene, e->npc_slot);
    pose.x = visual.x;
    pose.y = visual.y;
    pose.face_angle = visual.yaw_degrees;
    pose.moving = visual.moving;
    pose.locomotion = visual.locomotion;
    return pose;
}

static Vector2 click_route_point_to_screen(ViewerState* v,
                                           float tile_x, float tile_y) {
    Vector3 world = {
        tile_x,
        ground_y_smooth(v, tile_x, tile_y) + 0.08f,
        -tile_y,
    };
    return GetWorldToScreen(world, v->camera);
}

static void draw_click_destination_3d(ViewerState* v) {
    if (!v || !v->click_feedback.destination_active) return;
    int tx = v->click_feedback.destination_x;
    int ty = v->click_feedback.destination_y;
    if (tx < 0 || tx >= FC_ARENA_WIDTH ||
        ty < 0 || ty >= FC_ARENA_HEIGHT) {
        return;
    }

    float y = ground_y(v, tx, ty) + 0.025f;
    Vector3 center = {(float)tx + 0.5f, y, -((float)ty + 0.5f)};
    Color fill = {255, 215, 0, 70};
    Color edge = {255, 235, 70, 230};
    DrawCube(center, 0.92f, 0.025f, 0.92f, fill);
    DrawCubeWires(center, 0.92f, 0.025f, 0.92f, edge);
}

static void draw_click_route_2d(ViewerState* v) {
    if (!v) return;
    const int* route_x = NULL;
    const int* route_y = NULL;
    int start = 0;
    int len = 0;
    if (!fc_click_feedback_route(&v->click_feedback, &v->state,
                                 &route_x, &route_y, &start, &len)) {
        return;
    }

    FcVisualPose player = fc_visual_scene_player_pose(&v->visual_scene);
    Vector2 previous = click_route_point_to_screen(v, player.x, player.y);
    Color line = {255, 220, 30, 210};
    Color point = {255, 245, 120, 240};
    for (int i = start; i < len; i++) {
        Vector2 next = click_route_point_to_screen(
            v, (float)route_x[i] + 0.5f, (float)route_y[i] + 0.5f);
        DrawLineEx(previous, next, 2.0f, line);
        DrawCircleV(next, 2.5f, point);
        previous = next;
    }
}

static void draw_click_cross(ViewerState* v) {
    if (!v) return;
    int frame = fc_click_feedback_cross_frame(&v->click_feedback);
    if (frame < 0) return;
    int base = v->click_feedback.cross_kind == FC_CLICK_CROSS_INTERACTION
        ? FC_CLICK_CROSS_FRAME_COUNT : 0;
    Texture2D texture = v->click_cross_tex[base + frame];
    if (texture.id <= 0) return;
    DrawTexture(texture,
                (int)roundf(v->click_feedback.cross_screen_x) -
                    texture.width / 2,
                (int)roundf(v->click_feedback.cross_screen_y) -
                    texture.height / 2,
                WHITE);
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

static void draw_npc_prayer_window_indicators(ViewerState* v) {
    if (!v || !v->dbg_flags) return;

    for (int i = 0; i < v->entity_count; i++) {
        const FcRenderEntity* entity = &v->entities[i];
        int npc_idx = entity->npc_slot;
        if (entity->entity_type != ENTITY_NPC || entity->is_dead ||
            npc_idx < 0 || npc_idx >= FC_MAX_NPCS ||
            (v->npc_prayer_indicator_timer[npc_idx] <= 0.0f &&
             (v->npc_prayer_lock_tick[npc_idx] < 0 ||
              v->state.tick >= v->npc_prayer_lock_tick[npc_idx]))) {
            continue;
        }

        EntityRenderPose pose = entity_render_pose(v, entity);
        Vector3 anchor = {
            pose.x,
            ground_y_smooth(v, pose.x, pose.y) +
                2.2f + (float)entity->size * 0.8f,
            -pose.y
        };
        dbg_draw_prayer_window_indicator(anchor, v->camera);
    }
}

static const FcRenderEntity* find_render_actor(const ViewerState* v,
                                                int actor_kind,
                                                int actor_slot) {
    if (!v) return NULL;
    for (int i = 0; i < v->entity_count; i++) {
        const FcRenderEntity* entity = &v->entities[i];
        if (actor_kind == FC_VISUAL_TARGET_PLAYER &&
            entity->entity_type == ENTITY_PLAYER) {
            return entity;
        }
        if (actor_kind == FC_VISUAL_TARGET_NPC &&
            entity->entity_type == ENTITY_NPC &&
            entity->npc_slot == actor_slot) {
            return entity;
        }
    }
    return NULL;
}

static float render_entity_model_top(ViewerState* v,
                                     const FcRenderEntity* entity) {
    Model* model = NULL;
    if (!v || !entity) return 2.0f;
    if (entity->entity_type == ENTITY_PLAYER) {
        NpcModelEntry* entry = viewer_player_model_entry(v);
        if (entry && entry->loaded) model = &entry->model;
    } else {
        uint32_t model_id = fc_npc_type_to_model_id(entity->npc_type);
        NpcModelEntry* entry = v->npc_models
            ? fc_npc_model_find(v->npc_models, model_id) : NULL;
        if (entry && entry->loaded) model = &entry->model;
    }
    if (model) {
        BoundingBox bounds = GetModelBoundingBox(*model);
        if (bounds.max.y > 0.1f && bounds.max.y < 20.0f)
            return bounds.max.y;
    }
    if (entity->entity_type == ENTITY_PLAYER) return 2.0f;
    return 1.3f + (float)entity->size * 0.5f;
}

static Vector3 render_entity_overlay_anchor(ViewerState* v,
                                             const FcRenderEntity* entity,
                                             float height_fraction,
                                             float extra_height) {
    EntityRenderPose pose = entity_render_pose(v, entity);
    float ground = ground_y_smooth(v, pose.x, pose.y);
    float top = render_entity_model_top(v, entity);
    return (Vector3){
        pose.x,
        ground + top * height_fraction + extra_height,
        -pose.y
    };
}

static void draw_osrs_healthbars(ViewerState* v) {
    if (!v) return;
    for (int i = 0; i < v->entity_count; i++) {
        const FcRenderEntity* entity = &v->entities[i];
        float timer = 0.0f;
        if (entity->entity_type == ENTITY_PLAYER) {
            timer = v->player_healthbar_timer;
        } else if (entity->npc_slot >= 0 &&
                   entity->npc_slot < FC_MAX_NPCS) {
            timer = v->npc_healthbar_timer[entity->npc_slot];
        }
        if (timer <= 0.0f || entity->max_hp <= 0) continue;

        Vector3 anchor = render_entity_overlay_anchor(v, entity, 1.0f, 0.12f);
        Vector2 screen = GetWorldToScreen(anchor, v->camera);
        if (screen.x < -40.0f || screen.x > (float)GetScreenWidth() + 40.0f ||
            screen.y < -20.0f || screen.y > (float)GetScreenHeight() + 20.0f) {
            continue;
        }

        int fill = entity->current_hp * 30 / entity->max_hp;
        if (fill < 0) fill = 0;
        if (fill > 30) fill = 30;
        int x = (int)roundf(screen.x) - 15;
        int y = (int)roundf(screen.y) - 3;

        if (v->healthbar_empty_tex.id > 0 &&
            v->healthbar_full_tex.id > 0) {
            DrawTexture(v->healthbar_empty_tex, x, y, WHITE);
            if (fill > 0) {
                Rectangle src = {0.0f, 0.0f, (float)fill, 5.0f};
                DrawTextureRec(v->healthbar_full_tex, src,
                               (Vector2){(float)x, (float)y}, WHITE);
            }
        } else {
            DrawRectangle(x, y, 30, 5, RED);
            DrawRectangle(x, y, fill, 5, GREEN);
        }
    }
}

static Texture2D osrs_hitsplat_texture(const ViewerState* v,
                                       const Hitsplat* hit) {
    Texture2D empty = {0};
    if (!v || !hit) return empty;
    if (hit->kind == HITSPLAT_HEAL) return v->hitsplat_heal_tex;
    if (hit->kind == HITSPLAT_PRAYER_DRAIN)
        return v->hitsplat_prayer_drain_tex;
    return hit->damage > 0 ? v->hitsplat_damage_tex : v->hitsplat_zero_tex;
}

static void draw_osrs_hitsplats(ViewerState* v) {
    static const int slot_x[4] = {0, 0, -15, 15};
    static const int slot_y[4] = {0, -20, -10, -10};
    if (!v) return;

    for (int i = 0; i < MAX_HITSPLATS; i++) {
        const Hitsplat* hit = &v->hitsplats[i];
        if (!hit->active) continue;

        Vector3 world = {hit->world_x, hit->world_y, hit->world_z};
        const FcRenderEntity* entity = find_render_actor(
            v, hit->actor_kind, hit->actor_slot);
        if (entity)
            world = render_entity_overlay_anchor(v, entity, 0.5f, 0.0f);
        Vector2 screen = GetWorldToScreen(world, v->camera);
        if (screen.x < -50.0f || screen.x > (float)GetScreenWidth() + 50.0f ||
            screen.y < -50.0f || screen.y > (float)GetScreenHeight() + 50.0f) {
            continue;
        }

        int slot = hit->overlay_slot;
        if (slot < 0 || slot >= 4) slot = 0;
        int center_x = (int)roundf(screen.x) + slot_x[slot];
        int center_y = (int)roundf(screen.y) + slot_y[slot];
        Texture2D texture = osrs_hitsplat_texture(v, hit);
        if (texture.id > 0)
            DrawTexture(texture, center_x - 12, center_y - 12, WHITE);

        int value = hit->kind == HITSPLAT_DAMAGE
            ? hit->damage / 10
            : (hit->damage + 9) / 10;
        char text[16];
        snprintf(text, sizeof(text), "%d", value);

        /* Native clients use plain11 and draw the shadow one pixel down/right.
         * RuneScape Small is the repo's point-filtered plain11-compatible font. */
        const float font_size = 12.0f;
        Font font = runec_ui_font_for_size(&v->ui.assets, font_size);
        Vector2 measured = MeasureTextEx(font, text, font_size, 0.0f);
        float text_x = floorf((float)center_x - 1.0f - measured.x * 0.5f);
        float text_y = floorf((float)center_y - 6.0f);
        DrawTextEx(font, text, (Vector2){text_x + 1.0f, text_y + 1.0f},
                   font_size, 0.0f, BLACK);
        DrawTextEx(font, text, (Vector2){text_x, text_y},
                   font_size, 0.0f, WHITE);
    }
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

static void draw_actor_footprint(ViewerState* v,
                                 const FcRenderEntity* entity) {
    if (!v || !entity || entity->is_dead || entity->size <= 0) {
        return;
    }

    const int is_player = entity->entity_type == ENTITY_PLAYER;
    const Color fill = is_player
        ? CLITERAL(Color){50, 220, 100, 72}
        : CLITERAL(Color){50, 140, 255, 62};
    const Color outline = is_player
        ? CLITERAL(Color){80, 255, 130, 225}
        : CLITERAL(Color){80, 180, 255, 210};
    for (int offset_x = 0; offset_x < entity->size; offset_x++) {
        for (int offset_y = 0; offset_y < entity->size; offset_y++) {
            float tile_x = (float)(entity->x + offset_x) + 0.5f;
            float tile_y = (float)(entity->y + offset_y) + 0.5f;
            float height = ground_y_smooth(v, tile_x, tile_y) + 0.035f;
            Vector3 center = {tile_x, height, -tile_y};
            DrawCube(center, 0.94f, 0.02f, 0.94f, fill);
        }
    }

    float min_x = (float)entity->x + 0.03f;
    float max_x = (float)(entity->x + entity->size) - 0.03f;
    float min_y = (float)entity->y + 0.03f;
    float max_y = (float)(entity->y + entity->size) - 0.03f;
    Vector3 northwest = {
        min_x, ground_y_smooth(v, min_x, min_y) + 0.055f, -min_y};
    Vector3 northeast = {
        max_x, ground_y_smooth(v, max_x, min_y) + 0.055f, -min_y};
    Vector3 southeast = {
        max_x, ground_y_smooth(v, max_x, max_y) + 0.055f, -max_y};
    Vector3 southwest = {
        min_x, ground_y_smooth(v, min_x, max_y) + 0.055f, -max_y};
    DrawLine3D(northwest, northeast, outline);
    DrawLine3D(northeast, southeast, outline);
    DrawLine3D(southeast, southwest, outline);
    DrawLine3D(southwest, northwest, outline);
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

    /* Movement feedback is immediate, even though the selected action remains
     * buffered until the next authoritative simulation tick. */
    draw_click_destination_3d(v);

    /* Entities */
    for (int i = 0; i < v->entity_count; i++) {
        FcRenderEntity* e = &v->entities[i];

        EntityRenderPose pose = entity_render_pose(v, e);
        float ex = pose.x;
        float ey = -pose.y;

        /* Sample terrain continuously along the interpolated movement path. */
        float gy = ground_y_smooth(v, pose.x, pose.y);

        if (e->entity_type == ENTITY_PLAYER) {
            draw_actor_footprint(v, e);

            /* Player model or fallback cylinder */
            NpcModelEntry* pm = viewer_player_model_entry(v);
            if (pm && pm->loaded) {
                Vector3 pos = {ex, gy, ey};
                float face_angle = pose.face_angle;
                rlDisableBackfaceCulling();
                DrawModelEx(pm->model, pos, (Vector3){0,1,0}, face_angle, (Vector3){1,1,1}, WHITE);
                rlEnableBackfaceCulling();
            } else {
                DrawCylinder((Vector3){ex, gy, ey}, 0.4f, 0.4f, 2.0f, 8, COL_PLAYER);
                DrawCylinderWires((Vector3){ex, gy, ey}, 0.4f, 0.4f, 2.0f, 8, WHITE);
            }

            /* Prayer icon above player — rendered as 2D text after EndMode3D */
            /* (handled below in the 2D overlay section) */
        } else {
            draw_actor_footprint(v, e);

            /* NPC: try to render actual model, fallback to colored cube */
            uint32_t mid = fc_npc_type_to_model_id(e->npc_type);
            NpcModelEntry* nme = v->npc_models ? fc_npc_model_find(v->npc_models, mid) : NULL;

            if (nme) {
                /* Facing is maintained by the client-style actor runtime. */
                Vector3 pos = {ex, gy, ey};
                float face_angle = pose.face_angle;
                if (e->npc_slot >= 0 && e->npc_slot < FC_MAX_NPCS &&
                    v->npc_anim_states[e->npc_slot]) {
                    /* Same-type NPCs share an asset mesh. Upload this actor's
                     * transformed vertices immediately before its draw call. */
                    upload_anim_state_to_entry(
                        nme, v->npc_anim_states[e->npc_slot]);
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

        }
    }

    /* Draw active visual projectiles — use cache-backed spotanim models when present. */
    for (int pi = 0; pi < MAX_PROJECTILES; pi++) {
        VisualProjectile* vp = &v->projectiles[pi];
        if (!vp->active) continue;
        if (vp->spot_id == 0) continue;
        if (!vp->launched) continue;
        float px = vp->x;
        float py = vp->y;
        float pz = vp->z;

        const SpotAnimDef* spot = NULL;
        NpcModelEntry* pm = projectile_model_for_spot(v, vp->spot_id, &spot);
        if (pm && pm->loaded) {
            /* Rotate projectile to face travel direction */
            float horizontal_speed = sqrtf(
                vp->velocity_x * vp->velocity_x +
                vp->velocity_z * vp->velocity_z);
            float angle = atan2f(vp->velocity_x, vp->velocity_z) * RAD2DEG;
            /* Cache spotanim meshes carry their intended model orientation.
             * RuneC only applies trajectory pitch to raw projectile models. */
            float pitch = spot ? 0.0f
                : atan2f(vp->velocity_y, horizontal_speed);
            float scale_xy = (spot && spot->resize_xy > 0)
                ? (float)spot->resize_xy / 128.0f : 1.0f;
            float scale_z = (spot && spot->resize_z > 0)
                ? (float)spot->resize_z / 128.0f : 1.0f;
            if (spot) angle += (float)spot->rotation;
            if (spot && spot->animation_id >= 0) {
                update_entry_animation(pm, v->anim_cache, &vp->anim_state,
                                       &vp->anim_seq, &vp->anim_frame,
                                       &vp->anim_timer, spot->animation_id,
                                       policy_replay_anim_dt(v, GetFrameTime()),
                                       0.0f);
            }
            Quaternion yaw_rotation = QuaternionFromAxisAngle(
                (Vector3){0, 1, 0}, angle * DEG2RAD);
            Quaternion pitch_rotation = QuaternionFromAxisAngle(
                (Vector3){1, 0, 0}, -pitch);
            Quaternion rotation = QuaternionMultiply(yaw_rotation,
                                                     pitch_rotation);
            Vector3 rotation_axis = {0, 1, 0};
            float rotation_angle = 0.0f;
            QuaternionToAxisAngle(rotation, &rotation_axis, &rotation_angle);
            rlDisableBackfaceCulling();
            DrawModelEx(pm->model, (Vector3){px, py, pz},
                        rotation_axis,
                        rotation_angle * RAD2DEG,
                        (Vector3){scale_xy, scale_z, scale_xy}, WHITE);
            rlEnableBackfaceCulling();
        } else if (vp->radius > 0.0f) {
            DrawSphere((Vector3){px, py, pz}, vp->radius, vp->color);
        }
    }

    for (int ei = 0; ei < MAX_VISUAL_EFFECTS; ei++) {
        VisualEffect* fx = &v->effects[ei];
        if (!fx->active) continue;
        float effect_x = fx->x;
        float effect_y = fx->y;
        float effect_z = fx->z;
        if (fx->attached) {
            float x, y, z;
            if (visual_actor_world_point(v, fx->attached_kind,
                                         fx->attached_slot, &x, &y, &z)) {
                effect_x = x;
                effect_y = y + fx->attached_y_offset;
                effect_z = z;
            }
        }
        float effect_yaw = fx->yaw_degrees;
        if (fx->face_kind != FC_VISUAL_TARGET_NONE) {
            float target_x, target_y, target_z;
            if (visual_actor_world_point(v, fx->face_kind, fx->face_slot,
                                         &target_x, &target_y, &target_z)) {
                (void)target_y;
                effect_yaw = atan2f(target_x - effect_x,
                                    target_z - effect_z) * RAD2DEG;
            }
        }
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
                                       policy_replay_anim_dt(v, GetFrameTime()),
                                       0.0f);
            }
            rlDisableBackfaceCulling();
            DrawModelEx(pm->model, (Vector3){effect_x, effect_y, effect_z},
                        (Vector3){0,1,0},
                        effect_yaw + (spot ? (float)spot->rotation : 0.0f),
                        (Vector3){scale_xy, scale_z, scale_xy}, WHITE);
            rlEnableBackfaceCulling();
        } else {
            DrawSphere((Vector3){effect_x, effect_y, effect_z},
                       fx->radius, fx->color);
        }
    }

    /* Debug overlays — 3D collision tiles (before EndMode3D) */
    if (v->dbg_flags) debug_overlay_3d(&v->state, v->dbg_flags);

    EndMode3D();

    /* Native client actor overheads are fixed-size screen-space sprites. */
    draw_osrs_healthbars(v);

    /* Debug overlays — 2D screen-space (LOS, path, range — after EndMode3D) */
    if (v->dbg_flags) {
        int debug_flags = v->dbg_flags;
        if (v->click_feedback.destination_active)
            debug_flags &= ~DBG_PATH;
        debug_overlay_screen(&v->state, v->camera, debug_flags);
        draw_npc_prayer_window_indicators(v);
    }

    draw_click_route_2d(v);

    draw_osrs_hitsplats(v);

    /* Prayer overhead icon — 2D projected from player head position */
    int rendered_prayer = viewer_render_prayer(v);
    if (v->entity_count > 0 && rendered_prayer != PRAYER_NONE) {
        EntityRenderPose pose = entity_render_pose(v, &v->entities[0]);
        float p_gy = ground_y_smooth(v, pose.x, pose.y);
        Vector3 head_pos = {pose.x, p_gy + 3.0f, -pose.y};
        Vector2 scr = GetWorldToScreen(head_pos, v->camera);
        int px = (int)scr.x, py = (int)scr.y;

        /* Draw actual prayer sprite texture */
        Texture2D tex = {0};
        if (rendered_prayer == PRAYER_PROTECT_MELEE && v->pray_melee_tex.id > 0)
            tex = v->pray_melee_tex;
        else if (rendered_prayer == PRAYER_PROTECT_RANGE && v->pray_missiles_tex.id > 0)
            tex = v->pray_missiles_tex;
        else if (rendered_prayer == PRAYER_PROTECT_MAGIC && v->pray_magic_tex.id > 0)
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
            if (rendered_prayer == PRAYER_PROTECT_MELEE) icon_txt = "M";
            else if (rendered_prayer == PRAYER_PROTECT_RANGE) icon_txt = "R";
            else icon_txt = "W";
            DrawCircle(px, py, 14, (Color){255,255,255,220});
            int itw = fc_osrs_measure_text(icon_txt, 18);
            fc_osrs_draw_text(icon_txt, px - itw/2, py - 9, 18, (Color){0,0,0,255});
        }
    }
}

/* ======================================================================== */
/* UI drawing                                                                */
/* ======================================================================== */

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

#define RUNEC_CONSOLE_TAB_COUNT 6
#define RUNEC_CONSOLE_NPC_ROWS 8
#define RUNEC_CONSOLE_NPC_ROW_H 13
#define RUNEC_CONSOLE_TPS_COLS 4

static Rectangle runec_console_panel_rect(const ViewerState* v) {
    return runec_ui_chat_panel_rect(v ? &v->ui : NULL,
                                    GetScreenWidth(), GetScreenHeight());
}

static Rectangle runec_console_body_rect(Rectangle panel) {
    return (Rectangle){panel.x + 7.0f, panel.y + 7.0f,
                       panel.width - 13.0f, panel.height - 39.0f};
}

static Rectangle runec_console_tab_rect(Rectangle panel, int index) {
    float width = (panel.width - 6.0f) / (float)RUNEC_CONSOLE_TAB_COUNT;
    return (Rectangle){panel.x + 3.0f + width * (float)index,
                       panel.y + panel.height - 23.0f,
                       width, 21.0f};
}

static Rectangle runec_console_debug_button_rect(Rectangle body) {
    float right_x = body.x + 286.0f;
    float width = body.x + body.width - right_x;
    return (Rectangle){right_x, body.y + 15.0f,
                       (width - 4.0f) * 0.5f, 18.0f};
}

static Rectangle runec_console_god_button_rect(Rectangle body) {
    Rectangle debug = runec_console_debug_button_rect(body);
    return (Rectangle){debug.x + debug.width + 4.0f, debug.y,
                       debug.width, debug.height};
}

static Rectangle runec_console_wave_button_rect(Rectangle body) {
    return (Rectangle){body.x + 286.0f, body.y + 37.0f,
                       body.width - 286.0f, 20.0f};
}

static Rectangle runec_console_tps_button_rect(Rectangle body,
                                               int index) {
    float box_x = body.x + 286.0f;
    float box_w = body.width - 286.0f;
    const float gap = 3.0f;
    float button_w = (box_w - gap * (RUNEC_CONSOLE_TPS_COLS - 1)) /
                     RUNEC_CONSOLE_TPS_COLS;
    int row = index / RUNEC_CONSOLE_TPS_COLS;
    int col = index % RUNEC_CONSOLE_TPS_COLS;
    return (Rectangle){box_x + col * (button_w + gap),
                       body.y + 78.0f + row * 20.0f,
                       button_w, 17.0f};
}

static Rectangle runec_console_target_row_rect(Rectangle body,
                                               int row) {
    return (Rectangle){body.x + 1.0f,
                       body.y + 16.0f + row * RUNEC_CONSOLE_NPC_ROW_H,
                       277.0f, RUNEC_CONSOLE_NPC_ROW_H};
}

static Rectangle runec_console_wave_cell_rect(Rectangle body,
                                              int wave) {
    const int columns = 9;
    const float gap = 1.0f;
    float cell_w = (body.width - 6.0f - gap * (columns - 1)) / columns;
    int index = wave - 1;
    int row = index / columns;
    int col = index % columns;
    return (Rectangle){body.x + 3.0f + col * (cell_w + gap),
                       body.y + 18.0f + row * 15.0f,
                       cell_w, 14.0f};
}

static void draw_runec_console_button(Rectangle rect, const char* label,
                                      int selected) {
    int hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    Color background = selected ? CLITERAL(Color){82, 73, 61, 245}
        : (hovered ? CLITERAL(Color){72, 63, 51, 245}
                   : CLITERAL(Color){42, 36, 28, 235});
    Color text = selected ? COL_TEXT_YELLOW : COL_TEXT_WHITE;
    DrawRectangleRec(rect, background);
    DrawRectangleLinesEx(rect, 1, COL_PANEL_BORDER);
    int font_size = 8;
    fc_osrs_draw_text(label,
             (int)(rect.x + (rect.width - fc_osrs_measure_text(label, font_size)) * 0.5f),
             (int)(rect.y + (rect.height - font_size) * 0.5f),
             font_size, text);
}

static void draw_runec_console_wave_grid(ViewerState* v, Rectangle body) {
    Vector2 mouse = GetMousePosition();
    DrawRectangleRec(body, CLITERAL(Color){18, 16, 13, 252});
    DrawRectangleLinesEx(body, 1, COL_PANEL_BORDER);
    fc_osrs_draw_text("Select Wave", (int)body.x + 4, (int)body.y + 3,
             9, COL_TEXT_YELLOW);
    fc_osrs_draw_text("click current selection to close",
             (int)(body.x + body.width) - 155, (int)body.y + 4,
             7, COL_TEXT_DIM);
    for (int wave = 1; wave <= FC_NUM_WAVES; wave++) {
        Rectangle cell = runec_console_wave_cell_rect(body, wave);
        int current = wave == v->state.current_wave;
        int hovered = CheckCollisionPointRec(mouse, cell);
        Color bg = current ? CLITERAL(Color){60, 80, 40, 250}
            : (hovered ? CLITERAL(Color){72, 63, 51, 245}
                       : CLITERAL(Color){35, 30, 24, 240});
        DrawRectangleRec(cell, bg);
        DrawRectangleLinesEx(cell, 1, COL_PANEL_BORDER);
        char label[8];
        snprintf(label, sizeof(label), "%d", wave);
        fc_osrs_draw_text(label,
                 (int)(cell.x + (cell.width - fc_osrs_measure_text(label, 8)) * 0.5f),
                 (int)cell.y + 3, 8,
                 current ? COL_TEXT_YELLOW : COL_TEXT_WHITE);
    }
}

static void draw_runec_console_controls(ViewerState* v, Rectangle body) {
    static const char* NPC_SHORT[] = {
        "?", "Tz-Kih", "Tz-Kek", "Kek-Sm", "Tok-Xil",
        "MejKot", "Ket-Zek", "Jad", "HurKot"
    };
    Vector2 mouse = GetMousePosition();
    int x = (int)body.x + 4;
    char text[128];

    fc_osrs_draw_text("NPC Targets", x, (int)body.y + 3, 9, COL_TEXT_YELLOW);
    int shown = 0;
    for (int ni = 0; ni < FC_MAX_NPCS && shown < RUNEC_CONSOLE_NPC_ROWS; ni++) {
        FcNpc* npc = &v->state.npcs[ni];
        if (!npc->active || npc->is_dead) continue;
        Rectangle row = runec_console_target_row_rect(body, shown);
        int selected = ni == v->state.player.attack_target_idx;
        int hovered = CheckCollisionPointRec(mouse, row);
        if (selected || hovered) {
            DrawRectangleRec(row, selected
                ? CLITERAL(Color){82, 73, 61, 205}
                : CLITERAL(Color){52, 45, 35, 190});
        }
        const char* name = npc->npc_type > 0 && npc->npc_type < 9
            ? NPC_SHORT[npc->npc_type] : "?";
        snprintf(text, sizeof(text), "%s%s[%d] d:%d",
                 selected ? ">" : " ", name, ni,
                 fc_distance_to_npc(v->state.player.x,
                                    v->state.player.y, npc));
        fc_osrs_draw_text(text, (int)row.x + 2, (int)row.y + 3, 8,
                 selected ? COL_TEXT_YELLOW : COL_TEXT_WHITE);

        int bar_x = (int)row.x + 116;
        int bar_w = 116;
        float hp = npc->max_hp > 0
            ? (float)npc->current_hp / (float)npc->max_hp : 0.0f;
        if (hp < 0.0f) hp = 0.0f;
        if (hp > 1.0f) hp = 1.0f;
        DrawRectangle(bar_x, (int)row.y + 3, bar_w, 7, COL_HP_RED);
        DrawRectangle(bar_x, (int)row.y + 3,
                      (int)((float)bar_w * hp), 7, COL_HP_GREEN);
        snprintf(text, sizeof(text), "%d", npc->current_hp / 10);
        fc_osrs_draw_text(text, bar_x + bar_w + 4, (int)row.y + 2,
                 8, COL_TEXT_DIM);
        shown++;
    }
    if (shown == 0)
        fc_osrs_draw_text("No NPCs alive", x + 2, (int)body.y + 20,
                 8, COL_TEXT_DIM);

    int right_x = (int)body.x + 286;
    DrawLine(right_x - 5, (int)body.y + 2,
             right_x - 5, (int)(body.y + body.height) - 2,
             COL_PANEL_BORDER);
    fc_osrs_draw_text("Viewer Controls", right_x, (int)body.y + 3,
             9, COL_TEXT_YELLOW);

    char debug_label[24];
    snprintf(debug_label, sizeof(debug_label), "Debug: %s",
             v->dbg_flags ? "ON" : "OFF");
    draw_runec_console_button(runec_console_debug_button_rect(body),
                              debug_label, v->dbg_flags != 0);
    char god_label[24];
    snprintf(god_label, sizeof(god_label), "God: %s",
             v->godmode ? "ON" : "OFF");
    draw_runec_console_button(runec_console_god_button_rect(body),
                              god_label, v->godmode);

    Rectangle wave = runec_console_wave_button_rect(body);
    snprintf(text, sizeof(text), "Jump to Wave: %d  v",
             v->state.current_wave);
    draw_runec_console_button(wave, text, 0);

    fc_osrs_draw_text(v->policy_pipe ? "Replay TPS" : "TPS Presets",
             right_x, (int)body.y + 62, 8, COL_TEXT_DIM);
    for (int i = 0; i < NUM_MANUAL_TPS_PRESETS; i++) {
        draw_runec_console_button(runec_console_tps_button_rect(body, i),
                                  MANUAL_TPS_LABELS[i],
                                  float_near(v->tps, MANUAL_TPS_PRESETS[i]));
    }

    if (v->console_wave_dropdown_open)
        draw_runec_console_wave_grid(v, body);
}

static void draw_runec_console_diagnostics(ViewerState* v, Rectangle body) {
    int debug_tab = v->console_tab - 1;
    int available_height = (int)body.height - 6;
    int scroll = 0;
    if (debug_tab >= 0 && debug_tab < 4) {
        int max_scroll = v->console_content_height[debug_tab] - available_height;
        if (max_scroll < 0) max_scroll = 0;
        if (v->console_scroll[debug_tab] < 0)
            v->console_scroll[debug_tab] = 0;
        if (v->console_scroll[debug_tab] > max_scroll)
            v->console_scroll[debug_tab] = max_scroll;
        scroll = v->console_scroll[debug_tab];
    }

    int content_y = (int)body.y + 3 - scroll;
    BeginScissorMode((int)body.x, (int)body.y,
                     (int)body.width, (int)body.height);
    int end_y = dbg_draw_panel_tabs(
        &v->state, &v->reward_params,
        &v->reward_breakdown, &v->reward_runtime,
        v->reward_config_loaded, v->reward_config_path,
        (int)body.x, (int)body.x + 4, content_y,
        (int)body.width, debug_tab, 0, available_height);
    EndScissorMode();

    if (debug_tab >= 0 && debug_tab < 4) {
        int height = end_y - content_y;
        v->console_content_height[debug_tab] = height > 0 ? height : 0;
        int max_scroll = height - available_height;
        if (max_scroll < 0) max_scroll = 0;
        if (v->console_scroll[debug_tab] > max_scroll)
            v->console_scroll[debug_tab] = max_scroll;
        if (max_scroll > 0) {
            int track_x = (int)(body.x + body.width) - 6;
            int track_y = (int)body.y + 3;
            int track_h = available_height;
            DrawRectangle(track_x, track_y, 4, track_h,
                          CLITERAL(Color){30, 26, 20, 230});
            float visible_fraction =
                (float)available_height / (float)height;
            int thumb_h = (int)((float)track_h * visible_fraction);
            if (thumb_h < 12) thumb_h = 12;
            float scroll_fraction = max_scroll > 0
                ? (float)v->console_scroll[debug_tab] / (float)max_scroll
                : 0.0f;
            int thumb_y = track_y +
                (int)((float)(track_h - thumb_h) * scroll_fraction);
            DrawRectangle(track_x, thumb_y, 4, thumb_h,
                          CLITERAL(Color){120, 110, 90, 255});
        }
    }
}

static void draw_runec_console(ViewerState* v) {
    static const char* labels[RUNEC_CONSOLE_TAB_COUNT] = {
        "Controls", "Player", "Obs", "Mask", "Reward", "Log"
    };
    Rectangle panel = runec_console_panel_rect(v);
    Rectangle body = runec_console_body_rect(panel);
    DrawRectangleRec(body, CLITERAL(Color){0, 0, 0, 76});
    if (v->console_tab == 0)
        draw_runec_console_controls(v, body);
    else
        draw_runec_console_diagnostics(v, body);

    for (int tab = 0; tab < RUNEC_CONSOLE_TAB_COUNT; tab++) {
        Rectangle rect = runec_console_tab_rect(panel, tab);
        runec_ui_draw_asset(&v->ui.assets, "chat_tab_button_0", rect, WHITE);
        int selected = tab == v->console_tab;
        int hovered = CheckCollisionPointRec(GetMousePosition(), rect);
        DrawRectangleRec(rect, selected
            ? CLITERAL(Color){82, 73, 61, 96}
            : (hovered ? CLITERAL(Color){90, 78, 62, 72}
                       : CLITERAL(Color){0, 0, 0, 18}));
        if (selected)
            DrawLine((int)rect.x + 2, (int)(rect.y + rect.height) - 2,
                     (int)(rect.x + rect.width) - 2,
                     (int)(rect.y + rect.height) - 2,
                     COL_TEXT_YELLOW);
        fc_osrs_draw_text(labels[tab],
                 (int)(rect.x +
                     (rect.width - fc_osrs_measure_text(labels[tab], 8)) * 0.5f),
                 (int)rect.y + 7, 8,
                 selected ? COL_TEXT_YELLOW : COL_TEXT_WHITE);
    }
}

static int process_runec_console_input(ViewerState* v) {
    if (!v) return 0;
    Rectangle panel = runec_console_panel_rect(v);
    Vector2 mouse = GetMousePosition();
    if (!CheckCollisionPointRec(mouse, panel))
        return 0;

    int clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (clicked) {
        for (int tab = 0; tab < RUNEC_CONSOLE_TAB_COUNT; tab++) {
            if (!CheckCollisionPointRec(mouse,
                                        runec_console_tab_rect(panel, tab)))
                continue;
            v->console_tab = tab;
            v->console_wave_dropdown_open = 0;
            return 1;
        }
    }

    Rectangle body = runec_console_body_rect(panel);
    if (v->console_tab == 0 && clicked) {
        if (v->console_wave_dropdown_open) {
            for (int wave = 1; wave <= FC_NUM_WAVES; wave++) {
                if (!CheckCollisionPointRec(
                        mouse, runec_console_wave_cell_rect(body, wave)))
                    continue;
                if (wave != v->state.current_wave)
                    viewer_jump_to_wave(v, wave);
                v->console_wave_dropdown_open = 0;
                return 1;
            }
            v->console_wave_dropdown_open = 0;
            return 1;
        }

        if (CheckCollisionPointRec(mouse,
                                   runec_console_debug_button_rect(body))) {
            toggle_debug_overlay(v);
            return 1;
        }
        if (CheckCollisionPointRec(mouse,
                                   runec_console_god_button_rect(body))) {
            toggle_godmode(v);
            return 1;
        }
        if (CheckCollisionPointRec(mouse,
                                   runec_console_wave_button_rect(body))) {
            v->console_wave_dropdown_open = 1;
            return 1;
        }
        for (int i = 0; i < NUM_MANUAL_TPS_PRESETS; i++) {
            if (!CheckCollisionPointRec(
                    mouse, runec_console_tps_button_rect(body, i)))
                continue;
            set_manual_speed(v, MANUAL_TPS_PRESETS[i]);
            return 1;
        }

        int shown = 0;
        for (int ni = 0; ni < FC_MAX_NPCS &&
                 shown < RUNEC_CONSOLE_NPC_ROWS; ni++) {
            FcNpc* npc = &v->state.npcs[ni];
            if (!npc->active || npc->is_dead) continue;
            if (CheckCollisionPointRec(
                    mouse, runec_console_target_row_rect(body, shown))) {
                queue_player_attack_request(v, ni, mouse.x, mouse.y);
                fprintf(stderr, "CONSOLE CLICK -> ATTACK npc_idx=%d\n", ni);
                return 1;
            }
            shown++;
        }
    }

    if (v->console_tab >= 1 && v->console_tab <= 4 &&
        CheckCollisionPointRec(mouse, body)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            int index = v->console_tab - 1;
            int max_scroll = v->console_content_height[index] -
                             ((int)body.height - 6);
            if (max_scroll < 0) max_scroll = 0;
            v->console_scroll[index] -= (int)wheel * 24;
            if (v->console_scroll[index] < 0)
                v->console_scroll[index] = 0;
            if (v->console_scroll[index] > max_scroll)
                v->console_scroll[index] = max_scroll;
        }
    }
    return 1;
}

static void draw_tex_fit(Texture2D tex, int dx, int dy, int dw, int dh,
                         Color tint) {
    if (tex.id == 0) return;
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    float sx = (float)dw / (float)tex.width;
    float sy = (float)dh / (float)tex.height;
    float scale = sx < sy ? sx : sy;
    int rw = (int)(tex.width * scale);
    int rh = (int)(tex.height * scale);
    Rectangle dst = {
        (float)(dx + (dw - rw) / 2),
        (float)(dy + (dh - rh) / 2),
        (float)rw,
        (float)rh,
    };
    DrawTexturePro(tex, src, dst, (Vector2){0, 0}, 0, tint);
}

static Rectangle runec_prayer_button_rect(Rectangle content, int index) {
    const int btn_h = 34;
    const int gap = 3;
    int x = (int)content.x + 8;
    int y = (int)content.y + 8 + 34 + index * (btn_h + gap);
    int w = (int)content.width - 16;
    if (w < 120) w = 120;
    return (Rectangle){(float)x, (float)y, (float)w, (float)btn_h};
}

static void draw_runec_prayer_tab(ViewerState* v, Rectangle content) {
    BeginScissorMode((int)content.x, (int)content.y,
                     (int)content.width, (int)content.height);

    DrawRectangleRec(content, COL_PANEL);

    FcPlayer* p = &v->state.player;
    int rendered_prayer = viewer_render_prayer(v);
    int x = (int)content.x + 8;
    int by = (int)content.y + 8;
    int right = (int)(content.x + content.width) - 4;
    char b[64];

    snprintf(b, sizeof(b), "Prayer: %d / %d",
             p->current_prayer / 10, p->max_prayer / 10);
    text_s(b, x, by, 10, COL_PRAY_BLUE);
    by += 16;

    if (rendered_prayer != PRAYER_NONE) {
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
        Rectangle br = runec_prayer_button_rect(content, i);
        int is_active = (rendered_prayer == pray_vals[i]);
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

    by = (int)runec_prayer_button_rect(content, 2).y + 34 + 9;
    snprintf(b, sizeof(b), "Prayer bonus: +%d", p->prayer_bonus);
    text_s(b, x, by, 8, COL_TEXT_DIM);

    EndScissorMode();
}

static void draw_runec_side_overrides(ViewerState* v) {
    Rectangle content = runec_side_content_rect(v);
    if (v->ui.active_tab == RUNEC_UI_TAB_PRAYER)
        draw_runec_prayer_tab(v, content);
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
        Rectangle r = runec_prayer_button_rect(content, i);
        if (!CheckCollisionPointRec(mouse, r))
            continue;
        queue_viewer_prayer_button(v, buttons[i].prayer, buttons[i].action);
        return 1;
    }

    return 1;
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
    if (policy_pipe_flag) {
        SetTraceLogCallback(viewer_trace_log_to_stderr);
        SetTraceLogLevel(LOG_WARNING);
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE|FLAG_MSAA_4X_HINT);
    InitWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
               "Fight Caves RL — Playable Viewer");
    SetTargetFPS(60);

    ViewerState v; memset(&v, 0, sizeof(v));
    fc_init(&v.state);
    runec_ui_init(&v.ui);
    if (!fc_osrs_text_init()) {
        fprintf(stderr,
                "error: required OSRS viewer fonts failed to load\n");
        runec_ui_shutdown(&v.ui);
        CloseWindow();
        return 1;
    }
    load_fc_ui_item_icons(&v);
    v.paused = 1; v.tps = NORMAL_TPS; v.auto_mode = 0;
    v.active_loadout = FC_ACTIVE_LOADOUT;
    v.attack_target = -1;
    v.cam_yaw = 0; v.cam_pitch = 0.8f; v.cam_dist = 30;
    v.camera_locked = 1;
    v.camera.up = (Vector3){0,1,0}; v.camera.fovy = 32;
    v.camera.projection = CAMERA_PERSPECTIVE;
    v.camera.target = (Vector3){FC_ARENA_WIDTH * 0.5f, 0.5f, -(FC_ARENA_HEIGHT * 0.5f)};

    v.terrain = load_terrain(&v);
    /* OSRS rasterizes the current 104x104 scene from cache terrain and
     * locations into a 512x512 minimap. This asset contains the Fight Caves
     * mapsquare centered in that same scene format; runtime only crops and
     * rotates it around the player. */
    Image minimap_image = fc_load_image_asset("fightcaves.minimap.png");
    if (minimap_image.data) {
        Color* minimap_pixels = LoadImageColors(minimap_image);
        if (!minimap_pixels || !fc_minimap_scene_load_pixels(
                &v.minimap_scene, minimap_pixels,
                minimap_image.width, minimap_image.height)) {
            fprintf(stderr, "warning: Fight Caves minimap failed to load\n");
        } else {
            fprintf(stderr,
                    "minimap: loaded cache scene raster %dx%d\n",
                    minimap_image.width, minimap_image.height);
        }
        UnloadImageColors(minimap_pixels);
        UnloadImage(minimap_image);
    } else {
        fprintf(stderr, "warning: missing fightcaves.minimap.png\n");
    }
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

    /* Load the exact b237 cache sprites selected by the regular Fight Caves
     * hitmark and default 30-segment headbar definitions. */
    {
        v.hitsplat_zero_tex = fc_load_texture_asset(
            "data/sprites/ui/hitsplat_zero.png");
        v.hitsplat_damage_tex = fc_load_texture_asset(
            "data/sprites/ui/hitsplat_damage.png");
        v.hitsplat_heal_tex = fc_load_texture_asset(
            "data/sprites/ui/hitsplat_heal.png");
        v.hitsplat_prayer_drain_tex = fc_load_texture_asset(
            "data/sprites/ui/hitsplat_prayer_drain.png");
        v.healthbar_full_tex = fc_load_texture_asset(
            "data/sprites/ui/healthbar_full_30.png");
        v.healthbar_empty_tex = fc_load_texture_asset(
            "data/sprites/ui/healthbar_empty_30.png");
        Texture2D* overhead_textures[] = {
            &v.hitsplat_zero_tex, &v.hitsplat_damage_tex,
            &v.hitsplat_heal_tex, &v.hitsplat_prayer_drain_tex,
            &v.healthbar_full_tex, &v.healthbar_empty_tex,
        };
        int loaded = 0;
        for (int i = 0; i < (int)(sizeof(overhead_textures) /
                                  sizeof(overhead_textures[0])); i++) {
            if (overhead_textures[i]->id > 0) {
                SetTextureFilter(*overhead_textures[i], TEXTURE_FILTER_POINT);
                loaded++;
            }
        }
        fprintf(stderr, "Actor overhead sprites loaded: %d/6\n", loaded);
    }

    /* Native b237 click crosses: frames 0-3 are movement (yellow), frames
     * 4-7 are interaction (red). RuneC advances one frame every 100 ms. */
    {
        int loaded = 0;
        for (int i = 0; i < FC_CLICK_CROSS_FRAME_COUNT * 2; i++) {
            char path[64];
            snprintf(path, sizeof(path),
                     "data/sprites/ui/cross_%d.png", i);
            v.click_cross_tex[i] = fc_load_texture_asset(path);
            if (v.click_cross_tex[i].id > 0) {
                SetTextureFilter(v.click_cross_tex[i], TEXTURE_FILTER_POINT);
                loaded++;
            }
        }
        fprintf(stderr, "Click cross sprites loaded: %d/8\n", loaded);
    }

    /* Load prayer icons used by the active RuneC prayer override. */
    {
        v.tex_pray_melee_on = fc_load_texture_asset(
            "data/sprites/ui/prayeron_14.png");
        v.tex_pray_melee_off = fc_load_texture_asset(
            "data/sprites/ui/prayeroff_14.png");
        v.tex_pray_range_on = fc_load_texture_asset(
            "data/sprites/ui/prayeron_13.png");
        v.tex_pray_range_off = fc_load_texture_asset(
            "data/sprites/ui/prayeroff_13.png");
        v.tex_pray_magic_on = fc_load_texture_asset(
            "data/sprites/ui/prayeron_12.png");
        v.tex_pray_magic_off = fc_load_texture_asset(
            "data/sprites/ui/prayeroff_12.png");
        if (v.tex_pray_melee_on.id == 0)
            v.tex_pray_melee_on = fc_load_texture_asset(
                "sprites/protect_melee_on.png");
        if (v.tex_pray_melee_off.id == 0)
            v.tex_pray_melee_off = fc_load_texture_asset(
                "sprites/protect_melee_off.png");
        if (v.tex_pray_range_on.id == 0)
            v.tex_pray_range_on = fc_load_texture_asset(
                "sprites/protect_missiles_on.png");
        if (v.tex_pray_range_off.id == 0)
            v.tex_pray_range_off = fc_load_texture_asset(
                "sprites/protect_missiles_off.png");
        if (v.tex_pray_magic_on.id == 0)
            v.tex_pray_magic_on = fc_load_texture_asset(
                "sprites/protect_magic_on.png");
        if (v.tex_pray_magic_off.id == 0)
            v.tex_pray_magic_off = fc_load_texture_asset(
                "sprites/protect_magic_off.png");
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
                toggle_debug_overlay(&v);
            }
        }
        /* D: match the on-screen controls without interfering with east movement */
        if (IsKeyPressed(KEY_D) && !IsKeyDown(KEY_W) && !IsKeyDown(KEY_A) && !IsKeyDown(KEY_S)) {
            toggle_debug_overlay(&v);
        }
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
        if (!ui_capture)
            ui_capture = process_runec_console_input(&v);
        if (!ui_capture) {
            ui_capture = runec_ui_handle_input(&v.ui, GetScreenWidth(), GetScreenHeight());
            handle_runec_ui_intent(&v);
        } else {
            v.ui.last_intent.kind = RUNEC_UI_INTENT_NONE;
        }

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
            int used_human_actions = 0;
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
                used_human_actions = 1;
            }

            /* Save previous NPC positions by stable array index. */
            for (int ni = 0; ni < FC_MAX_NPCS; ni++) {
                v.prev_npc_x[ni] = (float)v.state.npcs[ni].x;
                v.prev_npc_y[ni] = (float)v.state.npcs[ni].y;
                v.prev_npc_active[ni] = v.state.npcs[ni].active;
            }

            /* Step simulation */
            fc_step(&v.state, v.actions);
            if (used_human_actions && v.actions[5] > 0 && v.actions[6] > 0)
                fc_click_feedback_accept_move_tick(&v.click_feedback,
                                                   &v.state);
            fc_click_feedback_sync(&v.click_feedback, &v.state);

            /* Playable-viewer test aid only. The simulator has already
             * resolved the hit; keep the local session alive at one HP. */
            if (v.godmode &&
                v.state.terminal == TERMINAL_PLAYER_DEATH) {
                v.state.player.current_hp = 10;
                v.state.terminal = TERMINAL_NONE;
            }
            fc_fill_render_events(&v.state, &v.render_events);
            ingest_visual_actor_tick(&v);
            update_reward_breakdown(&v);
            if (v.render_events.player_attack_fired)
                mark_player_attack_visual(&v);
            mark_prayer_flick_visual(&v);

            /* Debug event log — record events from this tick */
            dbg_log_tick(&v.state);

            /* Snap prev positions for newly spawned NPCs so they don't fly.
             * An NPC that wasn't active last tick but is now = new spawn. */
            for (int ni = 0; ni < FC_MAX_NPCS; ni++) {
                if (v.state.npcs[ni].active && !v.prev_npc_active[ni]) {
                    v.prev_npc_x[ni] = (float)v.state.npcs[ni].x;
                    v.prev_npc_y[ni] = (float)v.state.npcs[ni].y;
                    v.npc_healthbar_timer[ni] = 0.0f;
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
                float p3z = -(vis_py + 0.5f);
                if (v.state.tz_kih_prayer_drain_this_tick > 0) {
                    spawn_status_splat(&v, FC_VISUAL_TARGET_PLAYER, 0,
                                       p3x + 0.3f, gy_p + 3.0f, p3z,
                                       v.state.tz_kih_prayer_drain_this_tick,
                                       HITSPLAT_PRAYER_DRAIN);
                }

                /* Player ranged visuals use the authoritative stationary
                 * attack event and the active loadout's OSRS visual profile. */
                if (v.render_events.player_attack_fired) {
                    const PlayerVisualProfile* profile =
                        player_visual_profile(v.active_loadout);
                    int sx = v.render_events.player_attack_source_x;
                    int sy = v.render_events.player_attack_source_y;
                    int tx = v.render_events.player_attack_target_x;
                    int ty = v.render_events.player_attack_target_y;
                    int target_size = v.render_events.player_attack_target_size;
                    float src_x = (float)sx + 0.5f;
                    float src_y = ground_y(&v, sx, sy) +
                                  profile->projectile_start_height / 128.0f;
                    float src_z = -((float)sy + 0.5f);
                    float dst_x = (float)tx + (float)target_size * 0.5f;
                    float dst_y = ground_y(&v, tx, ty) +
                                  profile->projectile_end_height / 128.0f;
                    float dst_z = -((float)ty + (float)target_size * 0.5f);
                    int projectile_distance = projectile_tile_distance(
                        sx, sy, tx, ty);
                    float profile_end_time = fc_projectile_profile_end_cycle(
                        profile->projectile_launch_delay_client_ticks,
                        profile->projectile_length_adjustment,
                        profile->projectile_step_multiplier,
                        projectile_distance);
                    VisualProjectile* vp = spawn_projectile(
                        &v, src_x, src_y, src_z, dst_x, dst_y, dst_z,
                        0.1f, profile->projectile_color,
                        profile->projectile_radius,
                        profile->projectile_travel_spot,
                        profile->projectile_launch_spot,
                        profile->projectile_impact_spot);
                    configure_projectile_tracking(
                        &v, vp,
                        FC_VISUAL_TARGET_PLAYER, 0,
                        FC_VISUAL_TARGET_NPC,
                        v.render_events.player_attack_target_npc_slot,
                        ATTACK_RANGED,
                        profile->projectile_launch_delay_client_ticks,
                        profile_end_time,
                        profile->projectile_angle,
                        profile->projectile_progress, 1);
                }

                /* Hit-resolution events preserve zero-damage misses/blocks and
                 * multiple same-tick impacts without inspecting hit queues. */
                for (int hi = 0; hi < v.render_events.hit_count; hi++) {
                    const FcRenderHit* hit = &v.render_events.hits[hi];
                    if (hit->target_entity_type == ENTITY_PLAYER) {
                        if (!defer_hitsplat_to_projectile(
                                &v, hit, FC_VISUAL_TARGET_PLAYER, 0,
                                p3x, gy_p + 2.5f, p3z)) {
                            spawn_hitsplat(&v, FC_VISUAL_TARGET_PLAYER, 0,
                                           p3x, gy_p + 2.5f, p3z,
                                           hit->damage);
                        }
                        continue;
                    }
                    if (hit->target_entity_type != ENTITY_NPC ||
                        hit->target_npc_slot < 0 ||
                        hit->target_npc_slot >= FC_MAX_NPCS) {
                        continue;
                    }
                    FcNpc* target = &v.state.npcs[hit->target_npc_slot];
                    float gy_n = ground_y(&v, target->x, target->y);
                    float nx = (float)target->x + (float)target->size * 0.5f;
                    float nz = -((float)target->y +
                                 (float)target->size * 0.5f);
                    float nh = gy_n + 1.0f + (float)target->size * 0.5f;
                    if (!defer_hitsplat_to_projectile(
                            &v, hit, FC_VISUAL_TARGET_NPC,
                            hit->target_npc_slot, nx, nh, nz)) {
                        spawn_hitsplat(&v, FC_VISUAL_TARGET_NPC,
                                       hit->target_npc_slot,
                                       nx, nh, nz, hit->damage);
                    }
                }

                /* NPC healing remains a final-state per-tick presentation
                 * value; unlike attacks and hits, it is not reconstructed. */
                for (int i = 0; i < FC_MAX_NPCS; i++) {
                    FcNpc* n = &v.state.npcs[i];
                    if (n->healing_received_this_tick > 0) {
                        float gy_n = ground_y(&v, n->x, n->y);
                        float nx = (float)n->x + (float)n->size*0.5f;
                        float nz = -((float)n->y + (float)n->size*0.5f);
                        float nh = gy_n + 1.4f + (float)n->size*0.5f;
                        spawn_status_splat(&v, FC_VISUAL_TARGET_NPC, i,
                                           nx, nh, nz,
                                           n->healing_received_this_tick,
                                           HITSPLAT_HEAL);
                    }
                }

                /* NPC launches are reported directly by the AI transition, so
                 * animations and projectiles no longer depend on queue diffs. */
                for (int ai = 0; ai < v.render_events.npc_attack_count; ai++) {
                    const FcRenderNpcAttack* attack =
                        &v.render_events.npc_attacks[ai];
                    int npc_idx = attack->npc_slot;
                    if (npc_idx < 0 || npc_idx >= FC_MAX_NPCS) continue;
                    mark_npc_attack_visual(&v, npc_idx,
                                           attack->attack_style);
                    v.npc_prayer_lock_tick[npc_idx] =
                        attack->prayer_lock_tick;
                    if (!attack->hit_queued ||
                        attack->attack_style == ATTACK_MELEE) {
                        continue;
                    }

                    int sx_tile = attack->source_x;
                    int sy_tile = attack->source_y;
                    if (attack->source_size > 1) {
                        if (attack->target_x < attack->source_x)
                            sx_tile = attack->source_x;
                        else if (attack->target_x >=
                                 attack->source_x + attack->source_size)
                            sx_tile = attack->source_x +
                                      attack->source_size - 1;
                        else
                            sx_tile = attack->target_x;
                        if (attack->target_y < attack->source_y)
                            sy_tile = attack->source_y;
                        else if (attack->target_y >=
                                 attack->source_y + attack->source_size)
                            sy_tile = attack->source_y +
                                      attack->source_size - 1;
                        else
                            sy_tile = attack->target_y;
                    }
                    float src_ground = ground_y(&v, sx_tile, sy_tile);
                    float s3x = (float)sx_tile + 0.5f;
                    float s3y = src_ground + 1.0f +
                                (float)attack->source_size * 0.3f;
                    float s3z = -((float)sy_tile + 0.5f);
                    float target_ground = ground_y(
                        &v, attack->target_x, attack->target_y);
                    float target_x = (float)attack->target_x + 0.5f;
                    float target_y = target_ground + 1.5f;
                    float target_z = -((float)attack->target_y + 0.5f);
                    Color pc = (attack->attack_style == ATTACK_MAGIC)
                        ? CLITERAL(Color){255, 104, 36, 235}
                        : CLITERAL(Color){218, 178, 92, 235};
                    float rad = (attack->npc_type == NPC_TZTOK_JAD)
                        ? 0.3f : 0.15f;
                    uint32_t travel_spot = 0;
                    uint32_t launch_spot = 0;
                    uint32_t impact_spot = 0;
                    float profile_start_height = -1.0f;
                    float profile_end_height = -1.0f;
                    float profile_start_time = 0.0f;
                    float profile_angle = -1.0f;
                    float profile_length_adjustment = 0.0f;
                    float profile_progress = -1.0f;
                    float profile_step_multiplier = 0.0f;
                    float fixed_profile_end_time = -1.0f;
                    int track_target = 1;
                    if (attack->npc_type == NPC_TOK_XIL) {
                        travel_spot = PROJ_TOK_XIL_SPINE;
                        impact_spot = PROJ_TOK_XIL_IMPACT;
                        profile_start_height = 296.0f;
                        profile_end_height = 40.0f;
                        profile_start_time = 32.0f;
                        profile_angle = 16.0f;
                        profile_progress = 0.0f;
                        profile_step_multiplier = 5.0f;
                    } else if (attack->npc_type == NPC_KET_ZEK) {
                        travel_spot = PROJ_KET_ZEK_FIRE;
                        impact_spot = PROJ_KET_ZEK_IMPACT;
                        profile_start_height = 192.0f;
                        profile_end_height = 40.0f;
                        profile_start_time = 28.0f;
                        profile_angle = 16.0f;
                        profile_length_adjustment = 8.0f;
                        profile_progress = 0.0f;
                        profile_step_multiplier = 8.0f;
                    } else if (attack->npc_type == NPC_TZTOK_JAD &&
                               attack->attack_style == ATTACK_MAGIC) {
                        launch_spot = PROJ_JAD_MAGIC_LAUNCH;
                        travel_spot = PROJ_JAD_MAGIC_TRAVEL;
                        impact_spot = PROJ_JAD_MAGIC_IMPACT;
                        profile_start_height = 172.0f;
                        profile_end_height = 124.0f;
                        profile_start_time = 41.0f;
                        profile_angle = 16.0f;
                        profile_progress = 64.0f;
                        profile_step_multiplier = 5.0f;
                    } else if (attack->npc_type == NPC_TZTOK_JAD &&
                               attack->attack_style == ATTACK_RANGED) {
                        /* RuneC models Jad ranged as a delayed impact on the
                         * attacked tile, not as a homing travel projectile. */
                        impact_spot = PROJ_JAD_RANGED_IMPACT;
                        profile_start_height = 768.0f;
                        profile_end_height = 52.0f;
                        profile_angle = 0.0f;
                        profile_progress = 0.0f;
                        fixed_profile_end_time = 60.0f;
                        track_target = 0;
                    }
                    if (profile_start_height >= 0.0f)
                        s3y = src_ground + profile_start_height / 128.0f;
                    if (profile_end_height >= 0.0f)
                        target_y = target_ground +
                                   profile_end_height / 128.0f;
                    int projectile_distance = projectile_tile_distance(
                        sx_tile, sy_tile,
                        attack->target_x, attack->target_y);
                    float profile_end_time = fixed_profile_end_time >= 0.0f
                        ? fixed_profile_end_time
                        : fc_projectile_profile_end_cycle(
                            profile_start_time, profile_length_adjustment,
                            profile_step_multiplier, projectile_distance);
                    VisualProjectile* vp =
                        spawn_projectile(&v, s3x, s3y, s3z,
                                         target_x, target_y, target_z,
                                         0.1f, pc, rad, travel_spot,
                                         launch_spot, impact_spot);
                    configure_projectile_tracking(
                        &v, vp,
                        FC_VISUAL_TARGET_NPC, npc_idx,
                        FC_VISUAL_TARGET_PLAYER, 0,
                        attack->attack_style,
                        profile_start_time, profile_end_time,
                        profile_angle, profile_progress, track_target);
                }

                /* End delayed indicators at the lock boundary reported by the
                 * launch event; immediate attacks retain only the short pulse. */
                for (int ni = 0; ni < FC_MAX_NPCS; ni++) {
                    if (v.npc_prayer_lock_tick[ni] >= 0 &&
                        v.state.tick >= v.npc_prayer_lock_tick[ni]) {
                        v.npc_prayer_indicator_timer[ni] = 0.0f;
                        v.npc_prayer_lock_tick[ni] = -1;
                    }
                }

            }

            /* Sync viewer attack_target with player's backend target */
            v.attack_target = v.state.player.attack_target_idx;
            /* Auto-clear if target NPC died */
            if (v.state.player.attack_target_idx >= 0) {
                FcNpc* tn = &v.state.npcs[v.state.player.attack_target_idx];
                if (!tn->active || tn->is_dead) {
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

        /* OSRS overhead lifetimes are measured in 20 ms client cycles. */
        float overhead_dt = GetFrameTime();
        fc_click_feedback_update(&v.click_feedback, overhead_dt);
        for (int i = 0; i < MAX_HITSPLATS; i++) {
            if (v.hitsplats[i].active) {
                v.hitsplats[i].seconds_left -= overhead_dt;
                if (v.hitsplats[i].seconds_left <= 0.0f)
                    v.hitsplats[i].active = 0;
            }
        }
        if (v.player_healthbar_timer > 0.0f) {
            v.player_healthbar_timer -= overhead_dt;
            if (v.player_healthbar_timer < 0.0f)
                v.player_healthbar_timer = 0.0f;
        }
        for (int i = 0; i < FC_MAX_NPCS; i++) {
            if (v.npc_healthbar_timer[i] > 0.0f) {
                v.npc_healthbar_timer[i] -= overhead_dt;
                if (v.npc_healthbar_timer[i] < 0.0f)
                    v.npc_healthbar_timer[i] = 0.0f;
            }
        }

        /* Update visual projectiles */
        {
            float dt = GetFrameTime();
            float visual_dt = policy_replay_anim_dt(&v, dt);
            update_visual_actor_targets(&v);
            fc_visual_actor_set_movement_blocked(
                &v.visual_scene.player,
                visual_sequence_blocks_movement(
                    &v, player_visual_action_sequence(&v)));
            for (int i = 0; i < FC_MAX_NPCS; i++) {
                uint16_t action_seq = 0;
                if (v.state.npcs[i].is_dead ||
                    v.state.npcs[i].died_this_tick) {
                    int type = v.state.npcs[i].npc_type;
                    if (type > 0 && type < 9) action_seq = NPC_ANIM_DEATH[type];
                } else if (v.npc_attack_visual_timer[i] > 0.0f) {
                    action_seq = npc_attack_animation_id(
                        v.state.npcs[i].npc_type,
                        v.npc_attack_visual_style[i]);
                }
                fc_visual_actor_set_movement_blocked(
                    &v.visual_scene.npcs[i],
                    visual_sequence_blocks_movement(&v, action_seq));
            }
            if (!v.paused || v.policy_pipe)
                fc_visual_scene_update(&v.visual_scene, visual_dt);
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
            if (v.prayer_flick_visual_timer > 0.0f) {
                v.prayer_flick_visual_timer -= dt;
                if (v.prayer_flick_visual_timer < 0.0f)
                    v.prayer_flick_visual_timer = 0.0f;
            }
            for (int i = 0; i < FC_MAX_NPCS; i++) {
                if (v.npc_prayer_indicator_timer[i] > 0.0f) {
                    v.npc_prayer_indicator_timer[i] -= dt;
                    if (v.npc_prayer_indicator_timer[i] < 0.0f)
                        v.npc_prayer_indicator_timer[i] = 0.0f;
                }
            }
            if (v.objects)
                fc_animated_atlas_update(&v.objects->atlas, dt);
            if (v.object_anim_models)
                fc_animated_atlas_update(&v.object_anim_models->atlas, dt);
            if (v.projectile_models)
                fc_animated_atlas_update(&v.projectile_models->atlas, visual_dt);
            if (v.npc_models)
                fc_animated_atlas_update(&v.npc_models->atlas, visual_dt);
            if (v.player_model)
                fc_animated_atlas_update(&v.player_model->atlas, visual_dt);
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (v.projectiles[i].active) {
                    if (update_visual_projectile(&v, &v.projectiles[i], dt)) {
                        /* The client retains an impact for at most three game
                         * ticks, but a shorter spotanim ends after one pass. */
                        float effect_secs = projectile_effect_duration(
                            &v, v.projectiles[i].impact_spot_id, 90.0f);
                        spawn_spot_effect(&v, v.projectiles[i].impact_spot_id,
                                          v.projectiles[i].x,
                                          v.projectiles[i].y,
                                          v.projectiles[i].z,
                                          effect_secs, v.projectiles[i].color,
                                          v.projectiles[i].radius * 1.4f,
                                          0.0f);
                        if (v.projectiles[i].has_deferred_hitsplat) {
                            spawn_hitsplat(
                                &v,
                                v.projectiles[i].hitsplat_actor_kind,
                                v.projectiles[i].hitsplat_actor_slot,
                                v.projectiles[i].hitsplat_world_x,
                                v.projectiles[i].hitsplat_world_y,
                                v.projectiles[i].hitsplat_world_z,
                                v.projectiles[i].hitsplat_damage);
                        }
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
            FcVisualPose player_pose =
                fc_visual_scene_player_pose(&v.visual_scene);
            const PlayerVisualProfile* profile =
                player_visual_profile(v.active_loadout);
            uint16_t pose_seq = profile->idle_anim;
            switch (player_pose.locomotion) {
                case FC_VISUAL_LOCOMOTION_TURN:
                    pose_seq = profile->turn_anim;
                    break;
                case FC_VISUAL_LOCOMOTION_WALK_BACK:
                    pose_seq = profile->walk_back_anim;
                    break;
                case FC_VISUAL_LOCOMOTION_WALK_LEFT:
                    pose_seq = profile->walk_left_anim;
                    break;
                case FC_VISUAL_LOCOMOTION_WALK_RIGHT:
                    pose_seq = profile->walk_right_anim;
                    break;
                case FC_VISUAL_LOCOMOTION_RUN:
                    pose_seq = profile->run_anim;
                    break;
                case FC_VISUAL_LOCOMOTION_WALK_FORWARD:
                    pose_seq = profile->walk_anim;
                    break;
                default:
                    pose_seq = profile->idle_anim;
                    break;
            }

            /* OSRS applies movement pose and action sequence independently.
             * In policy replay, movement and attack can be valid on the same
             * tick, so attack visuals are driven by the actual attack event. */
            uint16_t action_seq = player_visual_action_sequence(&v);

            if (pose_seq != v.player_pose_anim_seq &&
                player_profile_is_movement_sequence(
                    profile, v.player_pose_anim_seq) &&
                player_profile_is_movement_sequence(profile, pose_seq)) {
                retarget_anim_track_preserving_phase(
                    v.anim_cache, pose_seq,
                    &v.player_pose_anim_seq, &v.player_pose_anim_frame,
                    &v.player_pose_anim_timer);
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
                if (action_fb && pose &&
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
                    v.npc_prayer_indicator_timer[ni] = 0.0f;
                    v.npc_prayer_lock_tick[ni] = -1;
                    v.npc_healthbar_timer[ni] = 0.0f;
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
                    v.npc_action_anim_seq[ni] = 0;
                    v.npc_action_anim_frame[ni] = 0;
                    v.npc_action_anim_timer[ni] = 0.0f;
                }

                /* NPC pose and action are independent client animation tracks. */
                uint16_t pose_seq = (n->npc_type > 0 && n->npc_type < 9)
                    ? NPC_ANIM_IDLE[n->npc_type] : 0;
                int npc_moved = v.visual_scene.npcs[ni].moving;
                if (npc_moved && n->npc_type > 0 && n->npc_type < 9)
                    pose_seq = NPC_ANIM_WALK[n->npc_type];

                uint16_t action_seq = 0;
                if (n->is_dead || n->died_this_tick) {
                    action_seq = (n->npc_type > 0 && n->npc_type < 9)
                        ? NPC_ANIM_DEATH[n->npc_type] : 0;
                } else if (v.npc_attack_visual_timer[ni] > 0.0f) {
                    action_seq = npc_attack_animation_id(
                        n->npc_type, v.npc_attack_visual_style[ni]);
                }

                AnimSequence* pose = advance_anim_track(
                    v.anim_cache, pose_seq,
                    &v.npc_anim_seq[ni], &v.npc_anim_frame[ni],
                    &v.npc_anim_timer[ni], anim_dt);
                AnimSequence* action = NULL;
                if (action_seq != 0) {
                    action = advance_anim_track_once(
                        v.anim_cache, action_seq,
                        &v.npc_action_anim_seq[ni],
                        &v.npc_action_anim_frame[ni],
                        &v.npc_action_anim_timer[ni], anim_dt);
                } else {
                    v.npc_action_anim_seq[ni] = 0;
                    v.npc_action_anim_frame[ni] = 0;
                    v.npc_action_anim_timer[ni] = 0.0f;
                }

                int applied = 0;
                if (action) {
                    AnimFrameData* action_fd =
                        &action->frames[v.npc_action_anim_frame[ni]].frame;
                    AnimFrameBase* action_fb = anim_get_framebase(
                        v.anim_cache, action_fd->framebase_id);
                    if (action_fb && pose && action->interleave_count > 0 &&
                        action->interleave_order) {
                        AnimFrameData* pose_fd =
                            &pose->frames[v.npc_anim_frame[ni]].frame;
                        AnimFrameBase* pose_fb = anim_get_framebase(
                            v.anim_cache, pose_fd->framebase_id);
                        if (pose_fb) {
                            anim_apply_frame_interleaved(
                                v.npc_anim_states[ni], nme->base_verts,
                                pose_fd, pose_fb, action_fd, action_fb,
                                action->interleave_order,
                                action->interleave_count);
                            applied = 1;
                        }
                    } else if (action_fb) {
                        anim_apply_frame(v.npc_anim_states[ni], nme->base_verts,
                                         action_fd, action_fb);
                        applied = 1;
                    }
                }
                if (!applied && pose) {
                    AnimFrameData* pose_fd =
                        &pose->frames[v.npc_anim_frame[ni]].frame;
                    AnimFrameBase* pose_fb = anim_get_framebase(
                        v.anim_cache, pose_fd->framebase_id);
                    if (pose_fb)
                        anim_apply_frame(v.npc_anim_states[ni], nme->base_verts,
                                         pose_fd, pose_fb);
                }

                if (v.npc_attack_visual_timer[ni] > 0.0f) {
                    v.npc_attack_visual_timer[ni] -= anim_dt;
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
        draw_runec_side_overrides(&v);
        draw_runec_console(&v);
        draw_test_overlay(&v);
        draw_click_cross(&v);

        EndDrawing();
    }

    if (v.pray_melee_tex.id > 0) UnloadTexture(v.pray_melee_tex);
    if (v.pray_missiles_tex.id > 0) UnloadTexture(v.pray_missiles_tex);
    if (v.pray_magic_tex.id > 0) UnloadTexture(v.pray_magic_tex);
    if (v.hitsplat_zero_tex.id > 0) UnloadTexture(v.hitsplat_zero_tex);
    if (v.hitsplat_damage_tex.id > 0) UnloadTexture(v.hitsplat_damage_tex);
    if (v.hitsplat_heal_tex.id > 0) UnloadTexture(v.hitsplat_heal_tex);
    if (v.hitsplat_prayer_drain_tex.id > 0)
        UnloadTexture(v.hitsplat_prayer_drain_tex);
    if (v.healthbar_full_tex.id > 0) UnloadTexture(v.healthbar_full_tex);
    if (v.healthbar_empty_tex.id > 0) UnloadTexture(v.healthbar_empty_tex);
    for (int i = 0; i < FC_CLICK_CROSS_FRAME_COUNT * 2; i++) {
        if (v.click_cross_tex[i].id > 0)
            UnloadTexture(v.click_cross_tex[i]);
    }
    if (v.tex_pray_melee_on.id > 0) UnloadTexture(v.tex_pray_melee_on);
    if (v.tex_pray_melee_off.id > 0) UnloadTexture(v.tex_pray_melee_off);
    if (v.tex_pray_range_on.id > 0) UnloadTexture(v.tex_pray_range_on);
    if (v.tex_pray_range_off.id > 0) UnloadTexture(v.tex_pray_range_off);
    if (v.tex_pray_magic_on.id > 0) UnloadTexture(v.tex_pray_magic_on);
    if (v.tex_pray_magic_off.id > 0) UnloadTexture(v.tex_pray_magic_off);
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
    fc_minimap_scene_free(&v.minimap_scene);
    if (v.terrain && v.terrain->loaded) { UnloadModel(v.terrain->model); free(v.terrain->heightmap); free(v.terrain); }
    fc_osrs_text_shutdown();
    runec_ui_shutdown(&v.ui);
    CloseWindow();
    return 0;
}
