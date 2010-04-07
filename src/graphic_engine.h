#ifndef _GRAPHIC_ENGINE_H
#define _GRAPHIC_ENGINE_H

#define BOARD_WIDTH   8
#define BOARD_HEIGHT  8
#define FONT_WIDTH    24
#define FONT_HEIGHT   20

// FUNCTIONS

void gweled_draw_message(gchar *in_message);
void gweled_draw_game_message(gchar *in_message, double timing);
void gweled_draw_message_at(gchar *in_message, gint msg_x, gint msg_y);
int gweled_gems_fall_into_place(void);
T_SGEObject *gweled_draw_character(int x, int y, int layer, char character);

#endif
