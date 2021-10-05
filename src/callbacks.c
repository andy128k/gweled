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


#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "main.h"
#include "sound.h"
#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"

extern gint gi_game_running;
extern gint gi_score;
extern gboolean g_do_not_score;

extern GtkBuilder *gweled_xml;
extern GtkWidget *g_main_window;
extern GtkWidget *g_pref_window;
extern GtkWidget *g_score_window;
extern GtkWidget *g_drawing_area;
//extern GdkPixmap *g_buffer_pixmap;
extern GtkWidget *g_pause_game;
extern GtkWidget *g_pref_music_button;
extern GtkWidget *g_pref_sounds_button;

extern gint gi_gem_clicked;
extern gint gi_x_click;
extern gint gi_y_click;

extern gint gi_gem_dragged;
extern gint gi_x_drag;
extern gint gi_y_drag;

extern guint board_engine_id;
extern GweledPrefs prefs;

static gint gi_dragging = 0;

static gboolean game_mode_changed = FALSE;


gboolean
drawing_area_expose_event_cb (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
	/*gdk_draw_drawable (GDK_DRAWABLE (gtk_widget_get_parent_window(widget)),
			   widget->style->fg_gc[gtk_widget_get_state (widget)],
			   g_buffer_pixmap, event->area.x, event->area.y,
			   event->area.x, event->area.y, event->area.width,
			   event->area.height);*/

	return FALSE;
}

gboolean
drawing_area_button_event_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	static int x_press = -1;
	static int y_press = -1;
	static int x_release = -1;
	static int y_release = -1;

    // resume game on click
    if(gi_game_running && board_get_pause() == TRUE && event->type == GDK_BUTTON_RELEASE ) {
        board_set_pause(FALSE);
        // not handle this click
        return FALSE;
    }

    // in pause mode don't accept events
    if(board_get_pause())
        return FALSE;

    // only handle left button
    if(event->button != 1)
        return FALSE;

    respawn_board_engine_loop();

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		x_press = event->x / prefs.tile_size;
		y_press = event->y / prefs.tile_size;
		if (gi_game_running) {
			gi_x_click = x_press;
			gi_y_click = y_press;
			gi_gem_clicked = -1;
			gi_dragging = -1;

			if(prefs.sounds_on)
			    sound_effect_play(CLICK_EVENT);
		}
		break;

	case GDK_BUTTON_RELEASE:
		gi_dragging = 0;
		gi_gem_dragged = 0;
		x_release = event->x / prefs.tile_size;
		y_release = event->y / prefs.tile_size;
		if( (x_release < 0) ||
			(x_release > BOARD_WIDTH - 1) ||
			(y_release < 0) ||
			(y_release > BOARD_HEIGHT - 1) )
			break;
		if( (x_press != x_release) ||
			(y_press != y_release))
		{
			if (gi_game_running) {
				gi_x_click = x_release;
				gi_y_click = y_release;
				gi_gem_clicked = -1;
			}
		}
		break;

	default:
		break;
	}

	return FALSE;
}

gboolean
drawing_area_motion_event_cb (GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
	if (gi_dragging) {
		gi_x_drag = event->x / prefs.tile_size;
		gi_y_drag = event->y / prefs.tile_size;
		gi_gem_dragged = -1;
	}

	return FALSE;
}




