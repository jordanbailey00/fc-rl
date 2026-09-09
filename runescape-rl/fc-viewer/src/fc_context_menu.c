#include "fc_context_menu.h"

FcNpcMenuInfo fc_menu_npc_info(int npc_type) {
    /* Fight Cave NPC display levels, not attack-skill levels or Inferno healers. */
    static const FcNpcMenuInfo npcs[] = {
        {"", 0}, {"Tz-Kih", 22}, {"Tz-Kek", 45}, {"Tz-Kek", 22},
        {"Tok-Xil", 90}, {"Yt-MejKot", 180}, {"Ket-Zek", 360},
        {"TzTok-Jad", 702}, {"Yt-HurKot", 108}
    };
    if (npc_type < 0 || npc_type >= (int)(sizeof(npcs) / sizeof(npcs[0]))) npc_type = 0;
    return npcs[npc_type];
}

uint32_t fc_menu_level_color(int player_level, int npc_level) {
    /* Client3 entry/client.c:getCombatLevelColorTag and defines.h RGB ramps. */
    int difference = player_level - npc_level;
    if (difference < -9) return 0xff0000;
    if (difference < -6) return 0xff3000;
    if (difference < -3) return 0xff7000;
    if (difference < 0) return 0xffb000;
    if (difference > 9) return 0x00ff00;
    if (difference > 6) return 0x40ff00;
    if (difference > 3) return 0x80ff00;
    if (difference > 0) return 0xc0ff00;
    return 0xffff00;
}

FcMenuLayout fc_menu_layout(int x, int y, int screen_width, int screen_height,
                             int text_width, int rows) {
    if (rows < 0) rows = 0;
    FcMenuLayout menu = {0, 0, text_width + 12,
                         FC_MENU_HEADER_HEIGHT + rows * FC_MENU_ROW_HEIGHT + 4};
    if (menu.width < 1) menu.width = 1;
    if (screen_width > 0 && menu.width > screen_width) menu.width = screen_width;
    menu.x = x - menu.width / 2;
    menu.y = y;
    if (menu.x + menu.width > screen_width) menu.x = screen_width - menu.width;
    if (menu.y + menu.height > screen_height) menu.y = screen_height - menu.height;
    if (menu.x < 0) menu.x = 0;
    if (menu.y < 0) menu.y = 0;
    return menu;
}

int fc_menu_contains(FcMenuLayout menu, int x, int y, int margin) {
    return x >= menu.x - margin && x < menu.x + menu.width + margin &&
           y >= menu.y - margin && y < menu.y + menu.height + margin;
}

int fc_menu_action_at(FcMenuLayout menu, int rows, int x, int y) {
    if (!fc_menu_contains(menu, x, y, 0)) return -1;
    int offset = y - menu.y - FC_MENU_HEADER_HEIGHT;
    if (offset < 0) return -1;
    int row = offset / FC_MENU_ROW_HEIGHT;
    return row < rows ? row : -1;
}
