/* Gweled
 *
 * Copyright (C) 2003-2005 Sebastien Delestaing <sebastien.delestaing@wanadoo.fr>
 * Copyright (C) 2010 Daniele Napolitano <dnax88@gmail.com>
 * Copyright (C) 2026 Andrey Kutejko <andy128k@gmail.com>
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

#include "config.h"

/* for memset and strlen */
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>

#include "board_engine.h"
#include "sge_core.h"
#include "sound.h"

#define FIRST_BONUS_AT          100  // needs tweaking
#define NB_BONUS_GEMS           8    // same
#define TOTAL_STEPS_FOR_TIMER   180  // seconds
#define HINT_TIMEOUT            15   // seconds

typedef enum e_game_state {
    _IDLE,
    _FIRST_GEM_CLICKED,
    _SECOND_GEM_CLICKED,
    _ILLEGAL_MOVE,
    _MARK_ALIGNED_GEMS,
    _BOARD_REFILLING
} T_GameState;

typedef enum e_alignment_dir
{
    T_ALIGN_HORIZONTAL,
    T_ALIGN_VERTICAL
} T_AlignmentDir;

typedef struct s_alignment {
    gint x;
    gint y;
    T_AlignmentDir direction;
    gint length;
} T_Alignment;

static unsigned char gpc_bit_n[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

extern gint gi_cursor_pixbuf;

struct _GweledEngine {
    GObject parent;
};

typedef struct _GweledEnginePrivate {
    gweled_game_mode mode;
    gboolean show_hints;
    gboolean play_sounds;

    GRand *random_generator;

    gint score;
    gint current_score;
    gboolean is_game_running;
    gboolean is_game_paused;

    gfloat total_gems_removed;
    gint score_per_move;

    guint hint_timeout;

    gint bonus_multiply;
    gint previous_bonus_at;
    gint next_bonus_at;
    gint level;
    gfloat steps_for_timer;

    GweledStage *stage;

    gint game_board[BOARD_WIDTH][BOARD_HEIGHT];
    T_SGEObject *gem_objects[BOARD_WIDTH][BOARD_HEIGHT];
    gint number_of_tiles[7];

    gboolean do_not_score;

    T_GameState state;

    GList *alignments;

    guint board_engine_id;

    gboolean gem_clicked;
    gint x_click;
    gint y_click;
} GweledEnginePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GweledEngine, gweled_engine, G_TYPE_OBJECT)

enum
{
    PROP_SHOW_HINTS = 1,
    PROP_PLAY_SOUNDS,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gweled_engine_set_hints_active (GweledEngine *engine, gboolean yn);

void
gweled_engine_set_stage (GweledEngine *engine, GweledStage *stage) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    priv->stage = stage;
}

gweled_game_mode
gweled_engine_get_mode (GweledEngine *engine) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    return priv->mode;
}

void
gweled_engine_set_mode (GweledEngine *engine, gweled_game_mode mode) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    priv->mode = mode;
}

static gint
get_new_tile (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    int min_index = 0;
    int previous_min_index = 0;
    int max_index = 0;
    int min = priv->number_of_tiles[0];
    int max = priv->number_of_tiles[0];
    for (int i = 0; i < 7; i++) {
        if (priv->number_of_tiles[i] < min) {
            min = priv->number_of_tiles[i];
            min_index = i;
            previous_min_index = min_index;
        }
        if (priv->number_of_tiles[i] > max) {
            max = priv->number_of_tiles[i];
            max_index = i;
        }
    }

    int random = g_rand_int_range (priv->random_generator, 0, 2);
    switch (random) {
    case 0:
        return g_rand_int_range (priv->random_generator, 0, 2) ? min_index : previous_min_index;
    default:
        return (max_index + (gchar) g_rand_int_range (priv->random_generator, 1, 7)) % 7;
    }
}

static gint
gweled_is_part_of_an_alignment (GweledEngine *engine, gint x, gint y)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint i;

    gint result = 0;
    for (i = x - 2; i <= x; i++)
        if (i >= 0 && i + 2 < BOARD_WIDTH)
            if (gpc_bit_n[priv->game_board[i][y]] &
                gpc_bit_n[priv->game_board[i + 1][y]] &
                gpc_bit_n[priv->game_board[i + 2][y]]
            ) {
                result |= 1;	// is part of an horizontal alignment
                break;
            }

    for (i = y - 2; i <= y; i++)
        if (i >= 0 && i + 2 < BOARD_HEIGHT)
            if (gpc_bit_n[priv->game_board[x][i]] &
                gpc_bit_n[priv->game_board[x][i + 1]] &
                gpc_bit_n[priv->game_board[x][i + 2]]
            ) {
                result |= 2;	// is part of a vertical alignment
                break;
            }

    return result;
}

static void
gweled_swap_gems (GweledEngine *engine, gint x1, gint y1, gint x2, gint y2)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    T_SGEObject * object;
    object = priv->gem_objects[x1][y1];
    priv->gem_objects[x1][y1] = priv->gem_objects[x2][y2];
    priv->gem_objects[x2][y2] = object;

    gint i;
    i = priv->game_board[x1][y1];
    priv->game_board[x1][y1] = priv->game_board[x2][y2];
    priv->game_board[x2][y2] = i;
}

static void
set_int (int *ptr, int value) {
    if (ptr) *ptr = value;
}

static gboolean
gweled_check_for_moves_left (GweledEngine *engine, int *pi, int *pj)
{
    for (int j = 0; j < BOARD_HEIGHT; j++)
        for (int i = 0; i < BOARD_WIDTH; i++) {
            if (i > 0) {
                gweled_swap_gems (engine, i - 1, j, i, j);
                gint alignments = gweled_is_part_of_an_alignment (engine, i, j);
                gweled_swap_gems (engine, i - 1, j, i, j);
                if (alignments) {
                    set_int (pi, i - 1);
                    set_int (pj, j);
                    return TRUE;
                }
            }
            if (i < 7) {
                gweled_swap_gems (engine, i + 1, j, i, j);
                gint alignments = gweled_is_part_of_an_alignment (engine, i, j);
                gweled_swap_gems (engine, i + 1, j, i, j);
                if (alignments) {
                    set_int (pi, i + 1);
                    set_int (pj, j);
                    return TRUE;
                }
            }
            if (j > 0) {
                gweled_swap_gems (engine, i, j - 1, i, j);
                gint alignments = gweled_is_part_of_an_alignment (engine, i, j);
                gweled_swap_gems (engine, i, j - 1, i, j);
                if (alignments) {
                    set_int (pi, i);
                    set_int (pj, j - 1);
                    return TRUE;
                }
            }
            if (j < 7) {
                gweled_swap_gems (engine, i, j + 1, i, j);
                gint alignments = gweled_is_part_of_an_alignment (engine, i, j);
                gweled_swap_gems (engine, i, j + 1, i, j);
                if (alignments) {
                    set_int (pi, i);
                    set_int (pj, j + 1);
                    return TRUE;
                }
            }
        }
    return FALSE;
}

static void
gweled_refill_board (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint i, j, k;
    gint last_tile = -1;
    gint same_tile_count = 1;
    g_debug("gweled_refill_board():");

    for (i = 0; i < BOARD_WIDTH; i++)
        for (j = 0; j < BOARD_HEIGHT; j++)
            if (priv->game_board[i][j] == -1) {
                for (k = j; k > 0; k--) {
                    priv->game_board[i][k] = priv->game_board[i][k - 1];
                    priv->gem_objects[i][k] = priv->gem_objects[i][k - 1];
                }
                priv->game_board[i][0] = get_new_tile (engine);

                // Keeps count of gems of the same type in a row
                if (last_tile == priv->game_board[i][0])
                    same_tile_count++;
                else
                    same_tile_count = 1;

                // If we have at least 3 gems in a row, let's change it.
                if (same_tile_count >= 3) {
                    g_debug("##### 3 gems in a row!!\n");
                    do  {
                        priv->game_board[i][0] = get_new_tile (engine);
                    } while (last_tile == priv->game_board[i][0]);
                }

                last_tile = priv->game_board[i][0];

                priv->number_of_tiles[priv->game_board[i][0]]++;

                // make sure the new tile appears outside of the screen (1st row is special-cased)
                if (j && priv->gem_objects[i][1])
                    priv->gem_objects[i][0] = sge_create_object (GTK_WIDGET (priv->stage),
                                            i,
                                            priv->gem_objects[i][1]->y - 1,
                                            GEMS_LAYER,
                                            priv->game_board[i][0]);
                else
                    priv->gem_objects[i][0] = sge_create_object (GTK_WIDGET (priv->stage),
                                            i,
                                            -1,
                                            GEMS_LAYER,
                                            priv->game_board[i][0]);
            }
}

static void
delete_alignment_from_board (gpointer alignment_pointer, gpointer user_data)
{
    GweledEngine *engine = GWELED_ENGINE (user_data);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint i, i_total_score;
    gint gi_gems_removed = 0;
    double xhotspot, yhotspot, xpos, ypos;
    char *buffer;
    T_Alignment *alignment;
    T_SGEObject *object;

    alignment = (T_Alignment *) alignment_pointer;
    // delete alignment
    if (alignment->direction == T_ALIGN_HORIZONTAL)	// horizontal
    {
        xhotspot = alignment->x + alignment->length / 2.0;
        yhotspot = alignment->y + 0.5;
        for (i = alignment->x; i < alignment->x + alignment->length; i++) {
            if (priv->game_board[i][alignment->y] != -1) {
                gi_gems_removed++;
                priv->number_of_tiles[priv->game_board[i][alignment->y]]--;
                priv->game_board[i][alignment->y] = -1;
            }
        }
    } else {
        xhotspot = alignment->x + 0.5;
        yhotspot = alignment->y + alignment->length / 2.0;
        for (i = alignment->y; i < alignment->y + alignment->length; i++) {
            if (priv->game_board[alignment->x][i] != -1) {
                gi_gems_removed++;
                priv->number_of_tiles[priv->game_board[alignment->x][i]]--;
                priv->game_board[alignment->x][i] = -1;
            }
        }
    }

    //compute score
    if (alignment->length == 1) {	//bonus mode
        i_total_score = 10 * g_rand_int_range (priv->random_generator, 1, 2);
    } else {
        i_total_score = 10 * (priv->bonus_multiply >> 1) * (alignment->length - 2) + priv->score_per_move;
        if (priv->do_not_score == TRUE)
            priv->score_per_move = i_total_score;
    }

    if (priv->do_not_score == FALSE) {
        priv->total_gems_removed += gi_gems_removed;

        g_debug("Score: %d Gems removed: %d [tot:%.2f] %i:%i, dir %i, length:%i\n", i_total_score, gi_gems_removed, priv->total_gems_removed, alignment->x, alignment->y, alignment->direction, alignment->length);

        priv->score += i_total_score;

        // display score
        buffer = g_strdup_printf ("%d", i_total_score);
        xpos = xhotspot - 1;
        ypos = yhotspot - 0.5;
        object = gweled_stage_create_score_message (priv->stage, buffer, xpos, ypos);
        sge_object_zoomin (object, 500, ADW_EASE_OUT_BOUNCE);
        sge_object_fly_away (object);
        g_free (buffer);
    }
}

static void
gweled_remove_gems_and_update_score (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    g_list_foreach (priv->alignments, delete_alignment_from_board, engine);
}

static void
take_down_alignment (gpointer object, gpointer user_data)
{
    T_Alignment *alignment = (T_Alignment *) object;
    GweledEngine *engine = GWELED_ENGINE (user_data);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    if (alignment->direction == T_ALIGN_HORIZONTAL)	{
        // horizontal
        for (int i = alignment->x; i < alignment->x + alignment->length; i++) {
            sge_gem_destroy (priv->gem_objects[i][alignment->y]);
            priv->gem_objects[i][alignment->y] = NULL;
        }
    } else {
        for (int i = alignment->y; i < alignment->y + alignment->length; i++) {
            sge_gem_destroy (priv->gem_objects[alignment->x][i]);
            priv->gem_objects[alignment->x][i] = NULL;
        }
    }
}

static void
gweled_take_down_deleted_gems (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    g_list_foreach (priv->alignments, take_down_alignment, engine);
}

static void
destroy_all_alignments (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    g_list_free_full (g_steal_pointer (&priv->alignments), g_free);
}

static void
gweled_delete_gems_for_bonus (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint i;
    T_Alignment * alignment;

    destroy_all_alignments (engine);
    for (i = 0; i < NB_BONUS_GEMS; i++) {
        alignment = g_new0 (T_Alignment, 1);
        alignment->x = g_rand_int_range (priv->random_generator, 0, 7);
        alignment->y = g_rand_int_range (priv->random_generator, 0, 7);
        alignment->direction = T_ALIGN_HORIZONTAL;
        alignment->length = 1;
        priv->alignments = g_list_append (priv->alignments, (gpointer) alignment);
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
static gboolean
gweled_check_for_alignments (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint i, j, i_nb_aligned, start_x, start_y;
    T_Alignment *alignment;

    destroy_all_alignments (engine);

    // make a list of vertical alignments
    i_nb_aligned = 0;

    for (i = 0; i < BOARD_WIDTH; i++) {
        for (j = 0; j < BOARD_HEIGHT; j++)
            if ((gweled_is_part_of_an_alignment (engine, i, j) & 2) == 2) {
                // record the origin of the alignment
                if (i_nb_aligned == 0) {
                    start_x = i;
                    start_y = j;
                }
                i_nb_aligned++;
            } else {
                // we found one, let's remember it for later use
                if (i_nb_aligned > 2) {
                    alignment = g_new0 (T_Alignment, 1);
                    alignment->x = start_x;
                    alignment->y = start_y;
                    alignment->direction = T_ALIGN_VERTICAL;
                    alignment->length = i_nb_aligned;
                    priv->alignments = g_list_append(priv->alignments, (gpointer) alignment);
                }
                i_nb_aligned = 0;
            }

        // end of column
        if (i_nb_aligned > 2) {
            alignment = g_new0 (T_Alignment, 1);
            alignment->x = start_x;
            alignment->y = start_y;
            alignment->direction = T_ALIGN_VERTICAL;
            alignment->length = i_nb_aligned;
            priv->alignments = g_list_append (priv->alignments, (gpointer) alignment);
        }
        i_nb_aligned = 0;
    }

    // make a list of horizontal alignments
    i_nb_aligned = 0;

    for (j = 0; j < BOARD_HEIGHT; j++) {
        for (i = 0; i < BOARD_WIDTH; i++)
            if ((gweled_is_part_of_an_alignment (engine, i, j) & 1) == 1) {
                // record the origin of the alignment
                if (i_nb_aligned == 0) {
                    start_x = i;
                    start_y = j;
                }
                i_nb_aligned++;
            } else {
                // if we found one, let's remember it for later use
                if (i_nb_aligned > 2) {
                    alignment = g_new0 (T_Alignment, 1);
                    alignment->x = start_x;
                    alignment->y = start_y;
                    alignment->direction = T_ALIGN_HORIZONTAL;
                    alignment->length = i_nb_aligned;
                    priv->alignments = g_list_append (priv->alignments, (gpointer) alignment);
                }
                i_nb_aligned = 0;
            }

        // end of row
        if (i_nb_aligned > 2) {
            alignment = g_new0 (T_Alignment, 1);
            alignment->x = start_x;
            alignment->y = start_y;
            alignment->direction = T_ALIGN_HORIZONTAL;
            alignment->length = i_nb_aligned;
            priv->alignments = g_list_append (priv->alignments, (gpointer) alignment);
        }
        i_nb_aligned = 0;
    }

    return (g_list_length (priv->alignments) != 0);
}

static void
gweled_fill_new_board (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint i, j;

    memset (priv->number_of_tiles, 0, 7 * sizeof (int));

    for (i = 0; i < BOARD_WIDTH; i++)
        for (j = 0; j < BOARD_HEIGHT; j++)
        {
            priv->game_board[i][j] = get_new_tile (engine);
            priv->number_of_tiles[priv->game_board[i][j]]++;
            priv->gem_objects[i][j] = sge_create_object (GTK_WIDGET (priv->stage),
                            i,
                            (j - BOARD_HEIGHT),
                            GEMS_LAYER,
                            priv->game_board[i][j]);
        }

    priv->do_not_score = TRUE;

    while (gweled_check_for_alignments (engine)) {
        gweled_remove_gems_and_update_score (engine);
        gweled_refill_board (engine);
    };
    priv->do_not_score = FALSE;

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
            priv->gem_objects[i][j] = sge_create_object (GTK_WIDGET (priv->stage),
                                                    i,
                                                    (j - BOARD_HEIGHT),
                                                    GEMS_LAYER,
                                                    priv->game_board[i][j]);

}

static void
gweled_gems_fall_into_place (GweledEngine *engine, gboolean new_board_animation);

static gboolean
gweled_gems_ready_to_fall_check (gpointer user_data)
{
    GweledEngine *engine = GWELED_ENGINE (user_data);

    if (sge_objects_are_moving_on_layer (GEMS_LAYER)) {
        return G_SOURCE_CONTINUE;
    } else {
        gweled_gems_fall_into_place (engine, FALSE);
        return G_SOURCE_REMOVE;
    }
}

static gboolean
gweled_gems_ready_to_fall_check_new_board_animation (gpointer user_data)
{
    GweledEngine *engine = GWELED_ENGINE (user_data);

    if (sge_objects_are_moving_on_layer (GEMS_LAYER)) {
        return G_SOURCE_CONTINUE;
    } else {
        gweled_gems_fall_into_place (engine, TRUE);
        return G_SOURCE_REMOVE;
    }
}

static void
gweled_gems_fall_into_place (GweledEngine *engine, gboolean new_board_animation)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint i, j;
    gint delay_incr = 0;
    gint max_height = 0;

    // Avoid gems falling if there are something moving/animating.
    if (sge_objects_are_moving_on_layer (GEMS_LAYER)) {
        g_timeout_add (10,
            new_board_animation ? gweled_gems_ready_to_fall_check_new_board_animation : gweled_gems_ready_to_fall_check,
            engine);
        return;
    }

    for (i = BOARD_WIDTH - 1; i >= 0; i--) {
        delay_incr = 0;
        for (j = BOARD_HEIGHT - 1; j >= 0; j--) {

            if (priv->gem_objects[i][j]->y == j)
                continue;

            if (max_height < j)
                max_height = j;

            if (new_board_animation)
                sge_object_fall_to_with_effect (priv->gem_objects[i][j], j,
                                    (i * 100) + ((BOARD_HEIGHT - j) * 50));
            else
                sge_object_fall_to (priv->gem_objects[i][j], j,
                                    // delay incremental
                                    delay_incr * 25,
                                    // trying to have the same speed regardless the destination
                                    100 + 20 * max_height);

            delay_incr++;
        }
    }
}

void
gweled_engine_set_pause (GweledEngine *engine, gboolean value)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    priv->is_game_paused = value;
    g_signal_emit_by_name (engine, "paused", priv->is_game_paused);

    if (value) {
        gweled_stage_create_game_message (priv->stage, _("Paused"), 0);

        gweled_stage_set_layer_visibility (priv->stage, GEMS_LAYER, FALSE);
        gweled_stage_set_layer_visibility (priv->stage, EFFECTS_LAYER, FALSE);
        gweled_engine_set_hints_active (engine, FALSE);
    } else {
        gweled_stage_set_layer_visibility (priv->stage, GEMS_LAYER, TRUE);
        gweled_stage_set_layer_visibility (priv->stage, EFFECTS_LAYER, TRUE);
        sge_destroy_all_objects_on_level(TEXT_LAYER);
        gweled_engine_respawn (engine);
    }
}

gboolean
gweled_engine_is_paused (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    return priv->is_game_paused;
}

gboolean
gweled_engine_is_game_running (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    return priv->is_game_running;
}

static gboolean
hint_callback (gpointer data)
{
    GweledEngine *engine = GWELED_ENGINE (data);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gint x, y;

    if (priv->is_game_running) {
        gweled_check_for_moves_left (engine, &x, &y);
        g_debug("hint_callback: x:%d, y%d\n", x, y);
        sge_object_bounce (priv->gem_objects[x][y]);
    }

    return TRUE;
}

static void
gweled_engine_emit_score_changed (GweledEngine *engine) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    g_signal_emit_by_name (engine, "score-changed", priv->current_score);
}

static void
gweled_engine_emit_level_changed (GweledEngine *engine) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    g_signal_emit_by_name (engine, "level-changed", priv->level);
}

static void
gweled_engine_emit_progress (GweledEngine *engine) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gdouble progress = (gdouble) (priv->total_gems_removed - priv->previous_bonus_at)
                     / (gdouble) (priv->next_bonus_at - priv->previous_bonus_at);
    progress = CLAMP (progress, 0.0, 1.0);

    g_signal_emit_by_name (engine, "progress", progress);
}

gboolean
gweled_game_over_callback (gpointer data)
{
    GweledEngine *engine = GWELED_ENGINE (data);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    g_signal_emit_by_name (engine, "game-over", priv->score, (guint) priv->mode);
    return FALSE;
}

gboolean
board_engine_loop (gpointer user_data)
{
    GweledEngine *engine = GWELED_ENGINE (user_data);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    static gint x1, y1, x2, y2, time_slice = 0;
    static T_SGEObject *cursor[2] = { NULL, NULL };
    gchar msg_buffer[200];

    time_slice++;

    const gchar* state[] = {"_IDLE", "_FIRST_GEM_CLICKED", "_SECOND_GEM_CLICKED",
                            "_ILLEGAL_MOVE", "_MARK_ALIGNED_GEMS", "_BOARD_REFILLING"};

    // progressive score
    if (priv->current_score < priv->score)
    {
        priv->current_score += 10;
        gweled_engine_emit_score_changed (engine);
    }

    /* Let's first check if we are in timer mode, and penalize the player if necessary */
    if (priv->mode == TIMED_MODE && priv->is_game_running && !priv->is_game_paused  && (time_slice % 25 == 0))
    {
        priv->total_gems_removed -= priv->steps_for_timer;

        if (priv->total_gems_removed <= priv->previous_bonus_at) {

            gweled_stage_create_game_message (priv->stage, _("Time's up!"), 0);
            gweled_stage_set_layer_opacity (priv->stage, GEMS_LAYER, 0.5);

            // Removes any gem that is still activated
            sge_object_blink_stop (priv->gem_objects[priv->x_click][priv->y_click]);
            gweled_stage_set_layer_visibility (priv->stage, EFFECTS_LAYER, FALSE);

            priv->is_game_running = FALSE;
            priv->is_game_paused = TRUE;
            priv->state = _IDLE;

            if (priv->score > 0)
                g_timeout_add_seconds (1, gweled_game_over_callback, engine);

        }

        gweled_engine_emit_progress (engine);
    }

    g_debug("Current state: %s\n", state[priv->state]);

    if (priv->hint_timeout && priv->gem_clicked) {
        gweled_engine_set_hints_active (engine, FALSE);
    }

    switch (priv->state) {
    case _IDLE:
        if (priv->gem_clicked) {
            x1 = priv->x_click;
            y1 = priv->y_click;
            priv->state = _FIRST_GEM_CLICKED;

            // Stop any previous effects
            sge_object_stop_effect (priv->gem_objects[priv->x_click][priv->y_click]);

            sge_object_blink_start (priv->gem_objects[priv->x_click][priv->y_click]);

            if (cursor[0])
                sge_destroy_object (cursor[0], NULL);
            cursor[0] = sge_create_object (GTK_WIDGET (priv->stage), x1, y1, EFFECTS_LAYER, gi_cursor_pixbuf);
            priv->gem_clicked = FALSE;
        }

        break;

    case _FIRST_GEM_CLICKED:
        if (priv->gem_clicked) {
            x2 = priv->x_click;
            y2 = priv->y_click;
            priv->gem_clicked = FALSE;
            if (((x1 == x2) && (abs (y1 - y2) == 1)) ||
                ((y1 == y2) && (abs (x1 - x2) == 1))) {
                // If the player clicks an adjacent gem, try to swap
                sge_object_blink_stop (priv->gem_objects[x1][y1]);
                // swap gems
                sge_object_move_to (priv->gem_objects[x1][y1],
                        x2,
                        y2);
                sge_object_move_to (priv->gem_objects[x2][y2],
                        x1,
                        y1);
                // swap cursors
                sge_object_move_to (cursor[0],
                        x2,
                        y2);

                if (priv->play_sounds) {
                    sound_effect_play (SWAP_EVENT);
                }

                priv->state = _SECOND_GEM_CLICKED;
            } else if((x1 == x2) && (y1 == y2)) {
                // If the player clicks the selected gem, deselect it
                if(cursor[0]) {
                    sge_destroy_object(cursor[0], NULL);
                    cursor[0] = NULL;
                }
                sge_object_blink_stop (priv->gem_objects[x1][y1]);
                priv->state = _IDLE;
                priv->gem_clicked = FALSE;
            } else {
                // If the player clicks anywhere else, make that the first selection
                sge_object_blink_stop (priv->gem_objects[x1][y1]);
                sge_object_blink_start (priv->gem_objects[x2][y2]);
                x1 = x2;
                y1 = y2;
                if (cursor[0])
                    sge_destroy_object (cursor[0], NULL);
                cursor[0] = sge_create_object (GTK_WIDGET (priv->stage), x1, y1, EFFECTS_LAYER, gi_cursor_pixbuf);
            }
        }
        break;

    case _SECOND_GEM_CLICKED:
        if (!sge_object_is_moving (priv->gem_objects[x1][y1]) && !sge_object_is_moving (priv->gem_objects[x2][y2])) {
            gweled_swap_gems (engine, x1, y1, x2, y2);
            if (!gweled_is_part_of_an_alignment (engine, x1, y1) && !gweled_is_part_of_an_alignment (engine, x2, y2)) {
                // re-swap gems
                sge_object_move_to (priv->gem_objects[x1][y1],
                        x2,
                        y2);
                sge_object_move_to (priv->gem_objects[x2][y2],
                        x1,
                        y1);
                // re-swap cursors
                sge_object_move_to (cursor[0],
                        x1,
                        y1);

                priv->state = _ILLEGAL_MOVE;
            } else {
                priv->score_per_move = 0;
                priv->state = _MARK_ALIGNED_GEMS;
            }
            // fadeout cursors
            if (cursor[0])
                sge_object_fadeout (cursor[0], 0, 200);
            cursor[0] = NULL;
            cursor[1] = NULL;
        }
        break;

    case _ILLEGAL_MOVE:
        if (!sge_object_is_moving (priv->gem_objects[x1][y1]) && !sge_object_is_moving (priv->gem_objects[x2][y2])) {
            gweled_swap_gems (engine, x1, y1, x2, y2);
            priv->state = _IDLE;
        }
        break;

    case _MARK_ALIGNED_GEMS:
        if (gweled_check_for_alignments (engine) == TRUE) {
            gweled_take_down_deleted_gems (engine);
            gweled_remove_gems_and_update_score (engine);

            if (priv->play_sounds) {
                sound_effect_play (EXPLODE_EVENT);
            }

            if (priv->mode != ENDLESS_MODE) {
                gweled_engine_emit_progress (engine);
            }

            priv->state = _BOARD_REFILLING;
            gweled_refill_board (engine);
            gweled_gems_fall_into_place (engine, FALSE);
        } else {
            if (gweled_check_for_moves_left (engine, NULL, NULL) == FALSE) {
                if (priv->mode == ENDLESS_MODE || priv->mode == TIMED_MODE) {

                    gweled_stage_create_game_message (priv->stage, _("No moves left!"), 2);

                    sge_destroy_all_objects_on_level(GEMS_LAYER);
                    gweled_fill_new_board (engine);

                    gweled_gems_fall_into_place (engine, FALSE);
                    priv->state = _MARK_ALIGNED_GEMS;
                } else {
                    // Game over

                    gweled_stage_create_game_message (priv->stage, _("No moves left!"), 0);
                    gweled_stage_set_layer_opacity (priv->stage, GEMS_LAYER, 0.5);
                    priv->is_game_running = FALSE;
                    priv->is_game_paused = TRUE;
                    priv->state = _IDLE;

                    if (priv->score > 0)
                        g_timeout_add_seconds (1, gweled_game_over_callback, engine);
                }
            } else {
                priv->do_not_score = FALSE;
                priv->state = _IDLE;
            }
        }
        break;

    case _BOARD_REFILLING:
        if (!sge_objects_are_moving_on_layer (GEMS_LAYER)) {
            if (priv->total_gems_removed >= priv->next_bonus_at && priv->mode != ENDLESS_MODE) {
                priv->previous_bonus_at = priv->next_bonus_at;
                priv->next_bonus_at *= 2;

                if (priv->mode == TIMED_MODE)
                    priv->steps_for_timer = (priv->next_bonus_at - priv->previous_bonus_at) / TOTAL_STEPS_FOR_TIMER + 1;

                // draw bonus message and new level in game
                priv->bonus_multiply++;
                priv->level++;
                gweled_engine_emit_level_changed (engine);
                g_sprintf (msg_buffer, _("Bonus x%d"), priv->bonus_multiply >> 1);
                gweled_stage_create_game_message (priv->stage, msg_buffer, 2);

                gweled_delete_gems_for_bonus (engine);
                gweled_take_down_deleted_gems (engine);
                gweled_remove_gems_and_update_score (engine);

                if (priv->mode == TIMED_MODE)
                    priv->total_gems_removed = (priv->next_bonus_at + priv->previous_bonus_at) / 2;

                gweled_engine_emit_progress (engine);

                gweled_refill_board (engine);
                gweled_gems_fall_into_place (engine, FALSE);
                priv->do_not_score = TRUE;
            } else {
                priv->state = _MARK_ALIGNED_GEMS;
            }
        }
        break;
    default:
        break;
    }

    if (priv->state == _IDLE && !priv->gem_clicked && !priv->hint_timeout && priv->show_hints)
        gweled_engine_set_hints_active (engine, TRUE);

    if ((priv->state == _IDLE || priv->state == _FIRST_GEM_CLICKED) && priv->current_score == priv->score && (priv->is_game_paused || priv->mode != TIMED_MODE))
    {
        priv->board_engine_id = 0;
        //g_debug("Board engine timer stopped");
        return FALSE;
    }
    return TRUE;
}

void
gweled_engine_respawn (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    if(!priv->board_engine_id)
        priv->board_engine_id = g_timeout_add (50, board_engine_loop, engine);
}

void
gweled_engine_start_new_game (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    priv->is_game_paused = FALSE;
    priv->score = 0;
    priv->current_score = 0;
    priv->score_per_move = 0;
    priv->bonus_multiply = 3;
    priv->level = 1;
    priv->previous_bonus_at = 0;
    priv->next_bonus_at = FIRST_BONUS_AT;
    priv->steps_for_timer = FIRST_BONUS_AT / (float) TOTAL_STEPS_FOR_TIMER;

    if (priv->mode == TIMED_MODE) {
        priv->total_gems_removed = FIRST_BONUS_AT / 2;
    } else {
        priv->total_gems_removed = 0;
    }
    gweled_engine_emit_progress (engine);

    if (priv->mode != ENDLESS_MODE) {
        gweled_engine_emit_level_changed (engine);
    }

    gweled_engine_set_hints_active (engine, FALSE);

    gweled_engine_emit_score_changed (engine);

    gweled_fill_new_board (engine);

    gweled_gems_fall_into_place (engine, TRUE);

    gweled_engine_respawn (engine);

    priv->is_game_running = TRUE;
    priv->state = _IDLE;
}

GweledGameState*
gweled_engine_get_current_game (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    GweledGameState *game;
    int i, j;

    game = g_malloc( sizeof(GweledGameState) );

    game->game_mode = priv->mode;
    game->gi_score = priv->score;
    game->gi_total_gems_removed = priv->total_gems_removed;
    game->gi_bonus_multiply = priv->bonus_multiply;
    game->gi_previous_bonus_at = priv->previous_bonus_at;
    game->gi_next_bonus_at = priv->next_bonus_at;
    game->gi_level = priv->level;
    game->g_steps_for_timer = priv->steps_for_timer;

    for (i = 0; i < BOARD_WIDTH; i++)
        for (j = 0; j < BOARD_HEIGHT; j++)
            game->gpc_game_board[i][j] = priv->game_board[i][j];

    return game;
}

void
gweled_engine_set_previous_game (GweledEngine *engine, GweledGameState *game)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    int i, j;

    priv->mode = game->game_mode;
    priv->score = game->gi_score;
    priv->total_gems_removed = game->gi_total_gems_removed;
    priv->bonus_multiply = game->gi_bonus_multiply;
    priv->previous_bonus_at = game->gi_previous_bonus_at;
    priv->next_bonus_at = game->gi_next_bonus_at;
    priv->level = game->gi_level;
    priv->steps_for_timer = game->g_steps_for_timer;
    priv->current_score = priv->score;

    if (priv->mode != ENDLESS_MODE) {
        gweled_engine_emit_progress (engine);
        gweled_engine_emit_level_changed (engine);
    }

    gweled_engine_emit_score_changed (engine);

    sge_destroy_all_objects ();

    for (i = 0; i < BOARD_WIDTH; i++)
        for (j = 0; j < BOARD_HEIGHT; j++) {
            priv->game_board[i][j] = game->gpc_game_board[i][j];
            priv->gem_objects[i][j] = sge_create_object (GTK_WIDGET (priv->stage), i, j, GEMS_LAYER, priv->game_board[i][j]);
        }

    priv->is_game_running = TRUE;
    priv->state = _MARK_ALIGNED_GEMS;

    gweled_engine_respawn (engine);

    priv->is_game_running = TRUE;
    priv->is_game_paused = FALSE;
}

void
gweled_engine_stop_game (GweledEngine *engine)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    gweled_engine_set_pause (engine, FALSE);
    g_source_remove (priv->board_engine_id);
    priv->board_engine_id = 0;

    priv->is_game_running = FALSE;
    sge_destroy_all_objects ();
}

static void
gweled_engine_set_hints_active (GweledEngine *engine, gboolean yn)
{
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    if (yn) {
        if (!priv->hint_timeout) {
            priv->hint_timeout = g_timeout_add_seconds (HINT_TIMEOUT, hint_callback, engine);
        }
    } else {
        if (priv->hint_timeout) {
            g_source_remove (priv->hint_timeout);
            priv->hint_timeout = 0;
        }
    }
}

void
gweled_engine_handle_click (GweledEngine *engine, gint x, gint y) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    // resume game on click
    if (gweled_engine_is_game_running (engine) && gweled_engine_is_paused(engine)) {
        gweled_engine_set_pause (engine, FALSE);
        // skip this click
        return;
    }

    // in pause mode don't accept events
    if (gweled_engine_is_paused(engine))
        return;

    priv->x_click = x;
    priv->y_click = y;
    priv->gem_clicked = TRUE;

    if (priv->play_sounds) {
        sound_effect_play (CLICK_EVENT);
    }
    gweled_engine_respawn (engine);
}

static void
gweled_engine_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    GweledEngine *engine = GWELED_ENGINE (object);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    switch (prop_id) {
    case PROP_SHOW_HINTS:
        priv->show_hints = g_value_get_boolean (value);
        gweled_engine_set_hints_active (engine, priv->show_hints);
        break;
    case PROP_PLAY_SOUNDS:
        priv->play_sounds = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gweled_engine_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    GweledEngine *engine = GWELED_ENGINE (object);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    switch (prop_id) {
    case PROP_SHOW_HINTS:
        g_value_set_boolean (value, priv->show_hints);
        break;
    case PROP_PLAY_SOUNDS:
        g_value_set_boolean (value, priv->play_sounds);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gweled_engine_dispose (GObject *object) {
    GweledEngine *engine = GWELED_ENGINE (object);
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    g_clear_pointer (&priv->random_generator, g_rand_free);
}

static void
gweled_engine_class_init (GweledEngineClass *klass) {
    G_OBJECT_CLASS (klass)->dispose = gweled_engine_dispose;
    G_OBJECT_CLASS (klass)->set_property = gweled_engine_set_property;
    G_OBJECT_CLASS (klass)->get_property = gweled_engine_get_property;

    obj_properties[PROP_SHOW_HINTS] = g_param_spec_boolean ("show-hints", NULL, NULL, TRUE, G_PARAM_READWRITE);
    obj_properties[PROP_PLAY_SOUNDS] = g_param_spec_boolean ("play-sounds", NULL, NULL, TRUE, G_PARAM_READWRITE);
    g_object_class_install_properties (G_OBJECT_CLASS (klass), N_PROPERTIES, obj_properties);

    g_signal_new ("progress",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        NULL,
        G_TYPE_NONE, 1, G_TYPE_DOUBLE);

    g_signal_new ("score-changed",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        NULL,
        G_TYPE_NONE, 1, G_TYPE_INT);

    g_signal_new ("level-changed",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        NULL,
        G_TYPE_NONE, 1, G_TYPE_INT);

    g_signal_new ("paused",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        NULL,
        G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

    g_signal_new ("game-over",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        NULL,
        G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_UINT);
}

static void
gweled_engine_init (GweledEngine *engine) {
    GweledEnginePrivate *priv = gweled_engine_get_instance_private (engine);

    priv->mode = NORMAL_MODE;
    priv->show_hints = TRUE;
    priv->play_sounds = TRUE;
    priv->random_generator = g_rand_new_with_seed (time (NULL));
    priv->is_game_running = FALSE;
    priv->state = _IDLE;
    priv->gem_clicked = FALSE;
}
