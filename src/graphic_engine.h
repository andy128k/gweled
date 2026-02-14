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

#ifndef _GRAPHIC_ENGINE_H
#define _GRAPHIC_ENGINE_H

#include <glib.h>
#include "sge_core.h"

#define GWELED_TYPE_STAGE gweled_stage_get_type ()
G_DECLARE_FINAL_TYPE (GweledStage, gweled_stage, GWELED, STAGE, GtkWidget)

GweledStage*
gweled_stage_new (void);

void
gweled_stage_set_layer_visibility (GweledStage* stage, T_SGELayer layer, gboolean visibility);
void
gweled_stage_set_layer_opacity (GweledStage* stage, T_SGELayer layer, gdouble opacity);

void
gweled_stage_create_game_message (GweledStage *stage, const gchar *message, guint timing);

T_SGEObject*
gweled_stage_create_score_message (GweledStage *stage, const gchar *message, double msg_x, double msg_y);

void
gweled_gems_fall_into_place (gboolean new_board_animation);

#endif
