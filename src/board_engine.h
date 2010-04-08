/* Gweled
 *
 * Copyright (C) 2003-2005 Sebastien Delestaing <sebastien.delestaing@wanadoo.fr>
 * Copyright (C) 2010 Daniele Napolitano <dnax88@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _BOARD_ENGINE_H_
#define _BOARD_ENGINE_H_

typedef struct s_gweled_prefs
{
	gboolean timer_mode;
	gboolean music_on;
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
