/* gweled-scores.h
 *
 * Copyright (C) 2023 Daniele Napolitano <dnax88@gmail.com>
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

#ifndef _GWELED_SCORES_H_
#define _GWELED_SCORES_H_

#include <glib.h>
#include <glib/gi18n.h>

#include <libgnome-games-support.h>

#define GAME_SCORE_CAT_NORMAL 0
#define GAME_SCORE_CAT_TIMED 1

void
gweled_hiscores_show();

void
gweled_hiscores_show_and_add(guint score, guint category);

void
gweled_init_scores(GtkWindow *parent_window);

#endif
