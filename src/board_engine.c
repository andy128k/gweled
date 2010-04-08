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

/* for memset and strlen */
#include <string.h>
/* for fabs() */
#include <math.h>

#include <gtk/gtk.h>
#include <mikmod.h>

#include <libgnome/gnome-score.h>

#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"

#define FIRST_BONUS_AT	100	// needs tweaking
#define NB_BONUS_GEMS	8	// same
#define TOTAL_STEPS_FOR_TIMER	60	// seconds

void gweled_remove_gems_and_update_score (void);

enum {
	_IDLE,
	_FIRST_GEM_CLICKED,
	_SECOND_GEM_CLICKED,
	_ILLEGAL_MOVE,
	_MARK_ALIGNED_GEMS,
	_BOARD_REFILLING
};

typedef struct s_alignment {
	gint x;
	gint y;
	gint direction;
	gint length;
} T_Alignment;

SAMPLE *swap_sfx;

gint gi_score, gi_current_score, gi_game_running;

gint gi_total_gems_removed;
gint gi_gems_removed_per_move;

gint gi_bonus_multiply;
gint gi_previous_bonus_at;
gint gi_next_bonus_at;
gint gi_trigger_bonus;
guint g_steps_for_timer;

gint gi_gem_clicked = 0;
gint gi_x_click = 0;
gint gi_y_click = 0;

gint gi_gem_dragged = 0;
gint gi_x_drag = 0;
gint gi_y_drag = 0;

gint gpc_game_board[BOARD_WIDTH][BOARD_HEIGHT];
gint gi_nb_of_tiles[7];

gboolean g_do_not_score;

T_SGEObject *g_gem_objects[BOARD_WIDTH][BOARD_HEIGHT];
unsigned char gpc_bit_n[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

static gint gi_state = _IDLE;
static GList *g_alignment_list;

extern GRand *g_random_generator;
extern GtkWidget *g_progress_bar;
extern GtkWidget *g_score_label;
extern GtkWidget *g_bonus_label;

extern gint gi_gems_pixbuf[7];
extern gint gi_cursor_pixbuf;

extern GweledPrefs prefs;

gchar
get_new_tile (void)
{
	int i;
	int min, max, min_index, max_index, previous_min_index;

	min_index = 0;
	previous_min_index = 0;
	max_index = 0;
	min = gi_nb_of_tiles[0];
	max = gi_nb_of_tiles[0];
	for (i = 0; i < 7; i++) {
		if (gi_nb_of_tiles[i] < min) {
			min = gi_nb_of_tiles[i];
			min_index = i;
			previous_min_index = min_index;
		}
		if (gi_nb_of_tiles[i] > max) {
			max = gi_nb_of_tiles[i];
			max_index = i;
		}
	}

//  i = (gint)g_rand_int_range(g_random_generator, 0, 4);
	i = (gint) g_rand_int_range (g_random_generator, 0, 2);

	switch (i) {
	case 0:
		return g_rand_int_range (g_random_generator, 0, 2) ? min_index : previous_min_index;
	default:
		return (max_index + (gchar) g_rand_int_range (g_random_generator, 1, 7)) % 7;
	}
}

void
gweled_start_new_game (void)
{
	gint i, j, i_deleted;

	gi_score = 0;
	gi_current_score = 0;
	gi_gems_removed_per_move = 0;
	gi_bonus_multiply = 3;
	gi_previous_bonus_at = 0;
	gi_next_bonus_at = FIRST_BONUS_AT;
	gi_trigger_bonus = 0;
	g_steps_for_timer = FIRST_BONUS_AT / TOTAL_STEPS_FOR_TIMER;

	if(prefs.timer_mode)
		gi_total_gems_removed = FIRST_BONUS_AT / 2;
	else
		gi_total_gems_removed = 0;

	gtk_progress_bar_set_fraction ((GtkProgressBar *) g_progress_bar, 0.0);
	gtk_label_set_markup ((GtkLabel *) g_score_label, "<span weight=\"bold\">000000</span>");
	gtk_label_set_markup ((GtkLabel *) g_bonus_label, "<span weight=\"bold\">x1</span>");

	memset (gi_nb_of_tiles, 0, 7 * sizeof (int));

	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
		{
			gpc_game_board[i][j] = get_new_tile ();
			gi_nb_of_tiles[gpc_game_board[i][j]]++;
			g_gem_objects[i][j] =
			    sge_create_object (i * prefs.tile_width,
							(j - BOARD_HEIGHT) * prefs.tile_height,
							1,
							gi_gems_pixbuf[gpc_game_board[i][j]]);
		}

	g_do_not_score = TRUE;
	while(gweled_check_for_alignments ()) {
		gweled_remove_gems_and_update_score ();
		gweled_refill_board();
	};
	g_do_not_score = FALSE;

//test pattern for a known bug
/*
gpc_game_board[0][7] = 0;
gpc_game_board[1][7] = 0;
gpc_game_board[2][7] = 1;
gpc_game_board[3][7] = 0;
gpc_game_board[4][7] = 1;
gpc_game_board[5][7] = 1;
*/

	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
			g_gem_objects[i][j] = sge_create_object (i * prefs.tile_width, (j - BOARD_HEIGHT) * prefs.tile_height, 1,
													 gi_gems_pixbuf[gpc_game_board[i][j]]);

	gweled_gems_fall_into_place ();

	gi_game_running = -1;
	gi_state = _MARK_ALIGNED_GEMS;
}

gint
gweled_is_part_of_an_alignment (gint x, gint y)
{
	gint i, result;

	result = 0;
	for (i = x - 2; i <= x; i++)
		if (i >= 0 && i + 2 < BOARD_WIDTH)
			if (gpc_bit_n[gpc_game_board[i][y]] &
			     gpc_bit_n[gpc_game_board[i + 1][y]] &
			     gpc_bit_n[gpc_game_board[i + 2][y]]) {
				result |= 1;	// is part of an horizontal alignment
				break;
			}

	for (i = y - 2; i <= y; i++)
		if (i >= 0 && i + 2 < BOARD_HEIGHT)
		if (gpc_bit_n[gpc_game_board[x][i]] &
		     gpc_bit_n[gpc_game_board[x][i + 1]] &
		     gpc_bit_n[gpc_game_board[x][i + 2]]) {
				result |= 2;	// is part of a vertical alignment
				break;
		}

	return result;
}

gboolean
gweled_check_for_moves_left (int *pi, int *pj)
{
	gint i, j;

	for (j = BOARD_HEIGHT - 1; j >= 0; j--)
		for (i = BOARD_WIDTH - 1; i >= 0; i--) {
			if (i > 0) {
				gweled_swap_gems (i - 1, j, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i - 1, j, i, j);
					goto move_found;
				}
				gweled_swap_gems (i - 1, j, i, j);
			}
			if (i < 7) {
				gweled_swap_gems (i + 1, j, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i + 1, j, i, j);
					goto move_found;
				}
				gweled_swap_gems (i + 1, j, i, j);
			}
			if (j > 0) {
				gweled_swap_gems (i, j - 1, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i, j - 1, i, j);
					goto move_found;
				}
				gweled_swap_gems (i, j - 1, i, j);
			}
			if (j < 7) {
				gweled_swap_gems (i, j + 1, i, j);
				if (gweled_is_part_of_an_alignment (i, j)) {
					gweled_swap_gems (i, j + 1, i, j);
					goto move_found;
				}
				gweled_swap_gems (i, j + 1, i, j);
			}
		}
	return FALSE;

move_found:
	if (pi && pj) {
		*pi = i;
		*pj = j;
	}
	return TRUE;
}

void
gweled_swap_gems (gint x1, gint y1, gint x2, gint y2)
{
	gint i;
	T_SGEObject * object;

	object = g_gem_objects[x1][y1];
	g_gem_objects[x1][y1] = g_gem_objects[x2][y2];
	g_gem_objects[x2][y2] = object;
	i = gpc_game_board[x1][y1];
	gpc_game_board[x1][y1] = gpc_game_board[x2][y2];
	gpc_game_board[x2][y2] = i;
}
void
gweled_refill_board (void)
{
	gint i, j, k;

	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
			if (gpc_game_board[i][j] == -1)
			{
				for (k = j; k > 0; k--)
				{
					gpc_game_board[i][k] =
					    gpc_game_board[i][k - 1];
					g_gem_objects[i][k] =
					    g_gem_objects[i][k - 1];
				}
				gpc_game_board[i][0] = get_new_tile ();
				gi_nb_of_tiles[gpc_game_board[i][0]]++;

				// make sure the new tile appears outside of the screen (1st row is special-cased)
				if (j && g_gem_objects[i][1])
					g_gem_objects[i][0] = sge_create_object (i * prefs.tile_width,
											g_gem_objects[i][1]->y - prefs.tile_height,
											1,
											gi_gems_pixbuf[gpc_game_board[i][0]]);
				else
					g_gem_objects[i][0] = sge_create_object (i * prefs.tile_width,
											-prefs.tile_height,
											1,
											gi_gems_pixbuf[gpc_game_board[i][0]]);
			}
}

void
delete_alignment_from_board (gpointer alignment_pointer, gpointer user_data)
{
	gint i, i_total_score;
	int xsize, ysize, xhotspot, yhotspot, xpos, ypos;
	char *buffer;
	T_Alignment *alignment;
	T_SGEObject *object;

	alignment = (T_Alignment *) alignment_pointer;
// delete alignment
	if (alignment->direction == 1)	// horizontal
	{
		xhotspot = (alignment->x * prefs.tile_width + alignment->length * prefs.tile_width / 2);
		yhotspot = (alignment->y * prefs.tile_height + prefs.tile_height / 2);
		for (i = alignment->x; i < alignment->x + alignment->length; i++) {
			if (gpc_game_board[i][alignment->y] != -1) {
				gi_gems_removed_per_move++;
				gi_nb_of_tiles[gpc_game_board[i][alignment->y]]--;
				gpc_game_board[i][alignment->y] = -1;
			}
		}
	} else {
		xhotspot = (alignment->x * prefs.tile_width + prefs.tile_width / 2);
		yhotspot = (alignment->y * prefs.tile_height + alignment->length * prefs.tile_height / 2);
		for (i = alignment->y; i < alignment->y + alignment->length; i++) {
			if (gpc_game_board[alignment->x][i] != -1) {
				gi_gems_removed_per_move++;
				gi_nb_of_tiles[gpc_game_board[alignment->x][i]]--;
				gpc_game_board[alignment->x][i] = -1;
			}
		}
	}
//compute score
	if (alignment->length == 1)	//bonus mode
		i_total_score = 10 * g_rand_int_range (g_random_generator, 1, 2);
	else
		i_total_score = 10 * (gi_gems_removed_per_move - 2) * (gi_bonus_multiply >> 1);

	if (g_do_not_score == FALSE) {
		gi_score += i_total_score;
//display score
		buffer = g_strdup_printf ("%d", i_total_score);
		xsize = strlen (buffer) * FONT_WIDTH;
		ysize = FONT_HEIGHT;
		for (i = 0; i < strlen (buffer); i++) {
			xpos = xhotspot - xsize / 2 + i * FONT_WIDTH;
			ypos = yhotspot - ysize / 2;
			object = gweled_draw_character (xpos, ypos, 4, buffer[i]);
			object->vy = -1.0;
			sge_object_set_lifetime (object, 50);	//1s
		}
		g_free (buffer);
	}
}

void
gweled_remove_gems_and_update_score (void)
{
	g_list_foreach (g_alignment_list, delete_alignment_from_board, NULL);
}

void
take_down_alignment (gpointer object, gpointer user_data)
{
	gint i;
	T_Alignment *alignment;

	alignment = (T_Alignment *) object;

	if (alignment->direction == 1)	// horizontal
		for (i = alignment->x; i < alignment->x + alignment->length; i++)
			sge_destroy_object (g_gem_objects[i][alignment->y], NULL);
	else
		for (i = alignment->y; i < alignment->y + alignment->length; i++)
			sge_destroy_object (g_gem_objects[alignment->x][i], NULL);
}

void
gweled_take_down_deleted_gems (void)
{
	g_list_foreach (g_alignment_list, take_down_alignment, NULL);
}

void
destroy_alignment (gpointer object, gpointer user_data)
{
	g_alignment_list = g_list_remove (g_alignment_list, object);
}

void
destroy_all_alignments (void)
{
	g_list_foreach (g_alignment_list, destroy_alignment, NULL);
}

void
gweled_delete_gems_for_bonus (void)
{
	gint i;
	T_Alignment * alignment;

	destroy_all_alignments ();
	for (i = 0; i < NB_BONUS_GEMS; i++) {
		alignment = (T_Alignment *) g_malloc (sizeof (T_Alignment));
		alignment->x = g_rand_int_range (g_random_generator, 0, 7);
		alignment->y = g_rand_int_range (g_random_generator, 0, 7);
		alignment->direction = 1;
		alignment->length = 1;
		g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
	}
}


// FIXME!!!
//
// if we have the following pattern:
//
// xxoxoo
//
// and swap the 2 central gems:
//
// xxxooo <- this is counted as 1 alignment of 6
//
// giving a score of 40 appearing in the middle rather than 10 + 40 (combo bonus).
// However the fix implies a significant change in the function below for
// a bug that is unlikely to happen. I will fix it. Just... not now.
gboolean
gweled_check_for_alignments (void)
{
	gint i, j, i_nb_aligned, start_x, start_y;
	T_Alignment *alignment;

	destroy_all_alignments ();

// make a list of vertical alignments
	i_nb_aligned = 0;

	for (i = 0; i < BOARD_WIDTH; i++) {
		for (j = 0; j < BOARD_HEIGHT; j++)
			if ((gweled_is_part_of_an_alignment (i, j) & 2) == 2) {
				// record the origin of the alignment
				if (i_nb_aligned == 0) {
					start_x = i;
					start_y = j;
				}
				i_nb_aligned++;
			} else {
				// we found one, let's remember it for later use
				if (i_nb_aligned > 2) {
					alignment = (T_Alignment *)g_malloc (sizeof (T_Alignment));
					alignment->x = start_x;
					alignment->y = start_y;
					alignment->direction = 2;
					alignment->length = i_nb_aligned;
					g_alignment_list = g_list_append(g_alignment_list, (gpointer) alignment);
				}
				i_nb_aligned = 0;
			}

		// end of column
		if (i_nb_aligned > 2) {
			alignment = (T_Alignment *)g_malloc (sizeof (T_Alignment));
			alignment->x = start_x;
			alignment->y = start_y;
			alignment->direction = 2;
			alignment->length = i_nb_aligned;
			g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
		}
		i_nb_aligned = 0;
	}

// make a list of horizontal alignments
	i_nb_aligned = 0;

	for (j = 0; j < BOARD_HEIGHT; j++) {
		for (i = 0; i < BOARD_WIDTH; i++)
			if ((gweled_is_part_of_an_alignment (i, j) & 1) == 1) {
				// record the origin of the alignment
				if (i_nb_aligned == 0) {
					start_x = i;
					start_y = j;
				}
				i_nb_aligned++;
			} else {
				// if we found one, let's remember it for later use
				if (i_nb_aligned > 2) {
					alignment = (T_Alignment *)g_malloc (sizeof (T_Alignment));
					alignment->x = start_x;
					alignment->y = start_y;
					alignment->direction = 1;
					alignment->length = i_nb_aligned;
					g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
				}
				i_nb_aligned = 0;
			}

		// end of row
		if (i_nb_aligned > 2) {
			alignment = (T_Alignment *) g_malloc (sizeof (T_Alignment));
			alignment->x = start_x;
			alignment->y = start_y;
			alignment->direction = 1;
			alignment->length = i_nb_aligned;
			g_alignment_list = g_list_append (g_alignment_list, (gpointer) alignment);
		}
		i_nb_aligned = 0;
	}

	return (g_list_length (g_alignment_list) != 0);
}

gboolean
board_engine_loop (gpointer data)
{
	static gint x1, y1, x2, y2, time_slice = 0;
	static T_SGEObject *cursor[2] = { NULL, NULL };
	gchar msg_buffer[200];
	gint hiscore_rank;

	time_slice++;

// progressive score
	if(gi_current_score < gi_score)
	{
		gi_current_score += 10;
		sprintf (msg_buffer, "<span weight=\"bold\">%06d</span>", gi_current_score);
		gtk_label_set_markup ((GtkLabel *) g_score_label, msg_buffer);
	}

/* Let's first check if we are in timer mode, and penalize the player if necessary */
	if (prefs.timer_mode && gi_game_running && (time_slice % 10 == 0))
	{
		gi_total_gems_removed -= g_steps_for_timer;
		if (gi_total_gems_removed <= gi_previous_bonus_at) {
			gweled_draw_message ("time's up #");
			gi_game_running = 0;
 			hiscore_rank = gnome_score_log ((gfloat) gi_score, "timed", TRUE);
 			if (hiscore_rank > 0)
 				show_hiscores (hiscore_rank);
 			show_hiscores (gi_score);
			g_do_not_score = FALSE;
			gi_state = _IDLE;
		} else
			gtk_progress_bar_set_fraction ((GtkProgressBar *)
						       g_progress_bar,
						       (float)(gi_total_gems_removed -gi_previous_bonus_at)
						       / (float)(gi_next_bonus_at - gi_previous_bonus_at));
	}

	switch (gi_state) {
	case _IDLE:
		if (gi_gem_clicked) {
			x1 = gi_x_click;
			y1 = gi_y_click;
			gi_state = _FIRST_GEM_CLICKED;
			if (cursor[0])
				sge_destroy_object (cursor[0], NULL);
			cursor[0] = sge_create_object (prefs.tile_width * x1,
					prefs.tile_height * y1,
					2, gi_cursor_pixbuf);
			gi_gem_clicked = 0;
			gi_gem_dragged = 0;
		}
		break;

	case _FIRST_GEM_CLICKED:
		if (gi_gem_clicked) {
			x2 = gi_x_click;
			y2 = gi_y_click;
			gi_gem_clicked = 0;
			if (((x1 == x2) && (fabs (y1 - y2) == 1)) ||
			    ((y1 == y2) && (fabs (x1 - x2) == 1))) {
				// If the player clicks an adjacent gem, try to swap
				if (cursor[1])
					sge_destroy_object (cursor[1], NULL);
				cursor[1] = sge_create_object (prefs.tile_width * x2,
						prefs.tile_height * y2,
						2, gi_cursor_pixbuf);
				sge_object_move_to (g_gem_objects[x1][y1],
						x2 * prefs.tile_width,
						y2 * prefs.tile_height);
				sge_object_move_to (g_gem_objects[x2][y2],
						x1 * prefs.tile_width,
						y1 * prefs.tile_height);
				if(swap_sfx)
					Sample_Play(swap_sfx, 0, 0);
				gi_state = _SECOND_GEM_CLICKED;
			} else if((x1 == x2) && (y1 == y2)) {
				if (cursor[1])
					sge_destroy_object (cursor[1], NULL);
				cursor[1] = NULL;
				// If the player clicks the selected gem, deselect it
				if(cursor[0])
					sge_destroy_object(cursor[0], NULL);
				gi_state = _IDLE;
				gi_gem_clicked = 0;
			} else {
				if (cursor[1])
					sge_destroy_object (cursor[1], NULL);
				cursor[1] = NULL;
				// If the player clicks anywhere else, make that the first selection
				x1 = x2;
				y1 = y2;
				if (cursor[0])
					sge_destroy_object (cursor[0], NULL);
				cursor[0] = sge_create_object (prefs.tile_width * x1,
						prefs.tile_height * y1,
						2, gi_cursor_pixbuf);
			}
		}else if(gi_gem_dragged)
		{
			//printf("gem dragged\n");
			if (cursor[1])
					sge_destroy_object (cursor[1], NULL);
				cursor[1] = sge_create_object (prefs.tile_width * gi_x_drag,
						prefs.tile_height * gi_y_drag,
						2, gi_cursor_pixbuf);
		}
		break;

	case _SECOND_GEM_CLICKED:
		if (!sge_object_is_moving (g_gem_objects[x1][y1]) && !sge_object_is_moving (g_gem_objects[x2][y2])) {
			gweled_swap_gems (x1, y1, x2, y2);
			if (!gweled_is_part_of_an_alignment (x1, y1) && !gweled_is_part_of_an_alignment (x2, y2)) {
				//gweled_draw_game_message("illegal move !", 1.0);
				sge_object_move_to (g_gem_objects[x1][y1],
						x2 * prefs.tile_width,
						y2 * prefs.tile_height);
				sge_object_move_to (g_gem_objects[x2][y2],
						x1 * prefs.tile_width,
						y1 * prefs.tile_height);
				gi_state = _ILLEGAL_MOVE;
			} else {
				if (cursor[0])
					sge_destroy_object (cursor[0], NULL);
				if (cursor[1])
					sge_destroy_object (cursor[1], NULL);
				cursor[0] = NULL;
				cursor[1] = NULL;
				gi_gems_removed_per_move = 0;
				gi_state = _MARK_ALIGNED_GEMS;
			}
		}
		break;

	case _ILLEGAL_MOVE:
		if (!sge_object_is_moving (g_gem_objects[x1][y1]) && !sge_object_is_moving (g_gem_objects[x2][y2])) {
			if (cursor[0])
				sge_destroy_object (cursor[0], NULL);
			if (cursor[1])
				sge_destroy_object (cursor[1], NULL);
			cursor[0] = NULL;
			cursor[1] = NULL;
			gweled_swap_gems (x1, y1, x2, y2);
			gi_state = _IDLE;
		}
		break;

	case _MARK_ALIGNED_GEMS:
		if (gweled_check_for_alignments () == TRUE) {
			gweled_take_down_deleted_gems ();
			gweled_remove_gems_and_update_score ();
			if (g_do_not_score == FALSE)
				gi_total_gems_removed += gi_gems_removed_per_move;
			if (gi_total_gems_removed <= gi_next_bonus_at)
				gtk_progress_bar_set_fraction ((GtkProgressBar *) g_progress_bar, (float) (gi_total_gems_removed - gi_previous_bonus_at) / (float) (gi_next_bonus_at - gi_previous_bonus_at));
			else
				gtk_progress_bar_set_fraction ((GtkProgressBar *) g_progress_bar, 1.0);
			//sprintf (msg_buffer, "<span weight=\"bold\">%06d</span>", gi_score);
			//gtk_label_set_markup ((GtkLabel *) g_score_label, msg_buffer);
			gweled_refill_board ();
			gweled_gems_fall_into_place ();
			gi_state = _BOARD_REFILLING;
		} else {
			if (gweled_check_for_moves_left (NULL, NULL) == FALSE) {
				if ((gi_next_bonus_at == FIRST_BONUS_AT) || (prefs.timer_mode)) {
					gint i, j;

					gweled_draw_game_message ("no moves left #", 1.0);
					memset (gi_nb_of_tiles, 0, 7 * sizeof (int));

					for (i = 0; i < BOARD_WIDTH; i++)
						for (j = 0; j < BOARD_HEIGHT; j++) {
							sge_destroy_object (g_gem_objects[i][j], NULL);
							gpc_game_board[i][j] = get_new_tile();
							gi_nb_of_tiles[gpc_game_board[i][j]]++;
						}
					g_do_not_score = TRUE;
					while(gweled_check_for_alignments ()) {
						gweled_remove_gems_and_update_score ();
						gweled_refill_board();
					};
					g_do_not_score = FALSE;
					for (i = 0; i < BOARD_WIDTH; i++)
						for (j = 0; j < BOARD_HEIGHT; j++) {
							g_gem_objects[i][j] = sge_create_object
								(i * prefs.tile_width,
							    	(j - BOARD_HEIGHT) * prefs.tile_height,
								1, gi_gems_pixbuf[gpc_game_board[i][j]]);
						}
					gweled_gems_fall_into_place ();
					gi_state = _MARK_ALIGNED_GEMS;
				} else {
					gweled_draw_message ("no moves left #");
					gi_game_running = 0;
					hiscore_rank = gnome_score_log ((gfloat) gi_score, "easy", TRUE);
 					if (hiscore_rank > 0)
 						show_hiscores (hiscore_rank);
					g_do_not_score = FALSE;
					gi_state = _IDLE;
				}
			} else {
				g_do_not_score = FALSE;
				gi_state = _IDLE;
			}
		}
		break;

	case _BOARD_REFILLING:
		if (!sge_objects_are_moving_on_layer (1)) {
			if (gi_total_gems_removed >= gi_next_bonus_at) {
				gi_previous_bonus_at = gi_next_bonus_at;
				gi_next_bonus_at *= 2;
				if (prefs.timer_mode)
					g_steps_for_timer = (gi_next_bonus_at - gi_previous_bonus_at) / TOTAL_STEPS_FOR_TIMER + 1;
				gi_bonus_multiply++;
				sprintf (msg_buffer, "bonus x%d", gi_bonus_multiply >> 1);
				gweled_draw_game_message (msg_buffer, 1.0);
				gweled_delete_gems_for_bonus ();
				gweled_take_down_deleted_gems ();
				gweled_remove_gems_and_update_score ();
				if (prefs.timer_mode)
					gi_total_gems_removed = (gi_next_bonus_at + gi_previous_bonus_at) / 2;
				gtk_progress_bar_set_fraction ((GtkProgressBar *) g_progress_bar,
					(float) (gi_total_gems_removed - gi_previous_bonus_at) /
					(float) (gi_next_bonus_at - gi_previous_bonus_at));
				//sprintf (msg_buffer, "<span weight=\"bold\">%06d</span>", gi_score);
				//gtk_label_set_markup ((GtkLabel *)g_score_label, msg_buffer);
				sprintf (msg_buffer, "<span weight=\"bold\">x%d</span>", gi_bonus_multiply >> 1);
				gtk_label_set_markup ((GtkLabel *)g_bonus_label, msg_buffer);
				gweled_refill_board ();
				gweled_gems_fall_into_place ();
				g_do_not_score = TRUE;
			} else {
				gi_state = _MARK_ALIGNED_GEMS;
			}
		}
		break;
	default:
		break;
	}
	return TRUE;
}
