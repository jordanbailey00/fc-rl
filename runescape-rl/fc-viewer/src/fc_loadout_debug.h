#ifndef FC_LOADOUT_DEBUG_H
#define FC_LOADOUT_DEBUG_H
#include "fc_types.h"
#include "fc_player_init.h"
#include "raylib.h"

/* Explicit, playable-viewer-only setup. No training adapter calls this module. */
enum { FC_LOADOUT_DEBUG_BOOK_0, FC_LOADOUT_DEBUG_BOOK_1, FC_LOADOUT_DEBUG_BOOK_2,
       FC_LOADOUT_DEBUG_BOOK_3, FC_LOADOUT_DEBUG_RUNES,
       FC_LOADOUT_DEBUG_CLEAR_INVENTORY, FC_LOADOUT_DEBUG_CLEAR_EQUIPMENT,
       FC_LOADOUT_DEBUG_LEVEL, /* legacy playable reset only */
       FC_LOADOUT_DEBUG_LOADOUT_BASE,
       FC_LOADOUT_DEBUG_COUNT = FC_LOADOUT_DEBUG_LOADOUT_BASE + FC_NUM_LOADOUTS };
enum { FC_LOADOUT_KIT_MAGIC, FC_LOADOUT_KIT_RANGED, FC_LOADOUT_KIT_MELEE };
typedef struct {
    int kit;
    int dropdown; /* 0 closed, 1 style, 2 tier, 3 spellbook */
    int confirm;  /* pending destructive button, or 0 */
} FcLoadoutDebugUi;
const char *fc_loadout_debug_action(FcState *state, int playable, int action);
void fc_loadout_debug_draw(const FcState *state, int playable,
                          const FcLoadoutDebugUi *ui, Rectangle body);
int fc_loadout_debug_hit(Rectangle body, Vector2 mouse, FcLoadoutDebugUi *ui);
#endif
