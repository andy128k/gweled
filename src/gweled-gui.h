/* Gweled
 *
 * Copyright (C) 2021 Daniele Napolitano <dnax88@gmail.com>
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
#include <clutter/clutter.h>

#define LOOKUP_WIDGET(widget_name) GTK_WIDGET (gtk_builder_get_object (gweled_ui->builder, widget_name))

typedef struct
{
  GtkBuilder *builder;

  GtkWidget *main_window,
            *g_clutter,
            *g_welcome_box,
            *g_progress_bar,
            *g_score_label,
            *g_pref_sounds_button,
            *g_main_game_stack,
            *g_headerbar,
            *g_new_game_btn,
            *g_pause_game_btn;

  ClutterActor *g_stage;

  GtkMenuButton *g_menu_button;

  GMenuModel *headermenu;

} GuiContext;

void
gweled_ui_init(GApplication *app);

void
gweled_ui_resize (gint size, gboolean update);

void
gweled_setup_game_window(gboolean playing);

void
welcome_screen_visibility (gboolean value);

gint
show_hiscores (gint pos, gboolean endofgame);

#endif
