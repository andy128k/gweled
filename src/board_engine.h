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

#include <glib.h>
#include "graphic_engine.h"

#define BOARD_WIDTH   8
#define BOARD_HEIGHT  8

typedef enum e_gweled_game_mode
{
    NORMAL_MODE,
    TIMED_MODE,
    ENDLESS_MODE
} gweled_game_mode;

typedef struct s_gweled_gamestate
{
    gint gpc_game_board[BOARD_WIDTH][BOARD_HEIGHT];
    gweled_game_mode game_mode;
    gint gi_score;
    gfloat gi_total_gems_removed;
    gint gi_bonus_multiply;
    gint gi_previous_bonus_at;
    gint gi_next_bonus_at;
    gint gi_level;
    gfloat g_steps_for_timer;

} GweledGameState;

#define GWELED_TYPE_ENGINE gweled_engine_get_type ()
G_DECLARE_FINAL_TYPE (GweledEngine, gweled_engine, GWELED, ENGINE, GObject)

void
gweled_engine_set_stage (GweledEngine *engine, GweledStage *stage);

gweled_game_mode
gweled_engine_get_mode (GweledEngine *engine);
void
gweled_engine_set_mode (GweledEngine *engine, gweled_game_mode game_mode);

void
gweled_engine_start_new_game (GweledEngine *engine);

void
gweled_engine_set_pause (GweledEngine *engine, gboolean value);

gboolean
gweled_engine_is_paused (GweledEngine *engine);

gboolean
gweled_engine_is_game_running (GweledEngine *engine);

void
gweled_engine_respawn (GweledEngine *engine);

GweledGameState*
gweled_engine_get_current_game (GweledEngine *engine);

void
gweled_engine_set_previous_game (GweledEngine *engine, GweledGameState *game);

void
gweled_engine_stop_game (GweledEngine *engine);

void
gweled_engine_handle_click (GweledEngine *engine, gint x, gint y);

#endif
