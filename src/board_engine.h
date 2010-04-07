#ifndef _BOARD_ENGINE_H_
#define _BOARD_ENGINE_H_

typedef struct s_gweled_prefs
{
	gboolean timer_mode;
	gint tile_width;
	gint tile_height;
}GweledPrefs;

extern gint gi_game_running;
extern gboolean gweled_in_timer_mode;

// FUNCTIONS
void gweled_init_glyphs(void);
void gweled_load_pixmaps(void);
void gweled_start_new_game(void);

void gweled_draw_board(void);
void gweled_draw_gems(void);

void gweled_draw_single_board(gint i, gint j);
void gweled_draw_single_gem(gint i, gint j);

void gweled_swap_gems(gint x1, gint y1, gint x2, gint y2);
void gweled_refill_board(void);
gboolean gweled_check_for_moves_left(int *pi, int *pj);

gboolean board_engine_loop(gpointer data);

#endif
