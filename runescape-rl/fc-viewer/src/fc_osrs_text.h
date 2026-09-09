#ifndef FC_OSRS_TEXT_H
#define FC_OSRS_TEXT_H

#include "raylib.h"

#define FC_OSRS_MENU_FONT_SIZE 16.0f

int fc_osrs_text_init(void);
void fc_osrs_text_shutdown(void);
Font fc_osrs_menu_font(void);
void fc_osrs_draw_text(const char* text, int x, int y, int font_size,
                       Color color);
int fc_osrs_measure_text(const char* text, int font_size);

#endif /* FC_OSRS_TEXT_H */
