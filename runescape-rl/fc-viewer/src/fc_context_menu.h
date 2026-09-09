#ifndef FC_CONTEXT_MENU_H
#define FC_CONTEXT_MENU_H

#include <stdint.h>

/* RuneC's client-style geometry, shared by menu drawing and hit testing. */
#define FC_MENU_HEADER_HEIGHT 24
#define FC_MENU_ROW_HEIGHT 19
#define FC_MENU_DISMISS_MARGIN 10

typedef struct { int x, y, width, height; } FcMenuLayout;
typedef struct { const char *name; int level; } FcNpcMenuInfo;

FcNpcMenuInfo fc_menu_npc_info(int npc_type);
uint32_t fc_menu_level_color(int player_level, int npc_level);

FcMenuLayout fc_menu_layout(int x, int y, int screen_width, int screen_height,
                             int text_width, int rows);
int fc_menu_contains(FcMenuLayout menu, int x, int y, int margin);
int fc_menu_action_at(FcMenuLayout menu, int rows, int x, int y);

#endif
