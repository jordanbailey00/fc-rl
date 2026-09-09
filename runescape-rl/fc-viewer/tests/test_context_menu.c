#include "fc_context_menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
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
