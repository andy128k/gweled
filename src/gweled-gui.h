/* gweled-gui.h
 *
 * Copyright (C) 2021 Daniele Napolitano <dnax88@gmail.com>
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

#ifndef _GWELED_GUI_H_
#define _GWELED_GUI_H_

#include <glib.h>
#include <gtk/gtk.h>
#include <adwaita.h>
#include "graphic_engine.h"

#define GWELED_TYPE_WINDOW gweled_window_get_type ()
G_DECLARE_FINAL_TYPE (GweledWindow, gweled_window, GWELED, WINDOW, AdwApplicationWindow)

GweledWindow *
gweled_window_new (AdwApplication *app);

GweledStage *
gweled_window_get_stage (GweledWindow *window);

void
gweled_window_restore_game (GweledWindow *window);

void
gweled_window_set_current_score (GweledWindow *window, gint score);

void
gweled_window_set_pause_enabled (GweledWindow *window, gboolean enabled);
void
gweled_window_set_paused (GweledWindow *window, gboolean paused);

void
gweled_window_set_progress (GweledWindow *window, gdouble progress);
void
gweled_window_set_level (GweledWindow *window, gint level);
void
gweled_window_reset_progress (GweledWindow *window);

#endif
