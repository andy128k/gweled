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

/* for exit() */
#include <stdlib.h>
/* for strlen() */
#include <string.h>

#include <gtk/gtk.h>

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

#include "sge_core.h"
#include "sge_utils.h"

#include "board_engine.h"
#include "gweled-gui.h"
#include "graphic_engine.h"

extern gchar gpc_game_board[BOARD_WIDTH][BOARD_HEIGHT];
extern GRand *g_random_generator;
extern GdkPixbuf *g_gems_pixbuf[7];

extern GuiContext *gweled_ui;

extern T_SGEObject *g_gem_objects[BOARD_WIDTH][BOARD_HEIGHT];

extern ClutterActor *g_actor_layers[5];

extern GweledPrefs prefs;
extern gint size;

gint gi_tiles_bg_pixbuf = -1;
gint gi_gems_pixbuf[7] = {-1, -1, -1, -1, -1, -1, -1};
gint gi_cursor_pixbuf = -1;

gboolean ready_to_fall = FALSE;

void
gweled_load_pixmaps (gint size)
{
	gchar *filename;
	GdkPixbuf *pixbuf;
	int i;

	for (i = 0; i < 7; i++) {
		filename = g_strdup_printf ("gem%02d.svg", i + 1);
		pixbuf = sge_load_svg_to_pixbuf (filename, size, size);
		if (pixbuf == NULL)
			exit (-1);
		gi_gems_pixbuf[i] = sge_register_pixbuf (pixbuf, gi_gems_pixbuf[i]);

		g_free (filename);
	}

    pixbuf = sge_load_svg_to_pixbuf ("board_bg.svg", size * 2, size * 2);

	if (pixbuf == NULL)
		exit (-1);
	gi_tiles_bg_pixbuf = sge_register_pixbuf (pixbuf, gi_tiles_bg_pixbuf);

	pixbuf = sge_load_svg_to_pixbuf ("cursor.svg", size, size);

	if (pixbuf == NULL)
		exit (-1);
	gi_cursor_pixbuf = sge_register_pixbuf (pixbuf, gi_cursor_pixbuf);
}

void
gweled_draw_board (gint size)
{

	ClutterActor *tmp;
	GError *error;
    GValue val = {0,};

    tmp = gtk_clutter_texture_new ();
    gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                         GDK_PIXBUF(sge_get_pixbuf(gi_tiles_bg_pixbuf)), &error);

    g_value_init (&val, G_TYPE_BOOLEAN);
    g_value_set_boolean ( &val, TRUE);

    g_object_set_property (G_OBJECT (tmp), "repeat-y", &val);
    g_object_set_property (G_OBJECT (tmp), "repeat-x", &val);

    clutter_actor_set_size (CLUTTER_ACTOR(tmp), BOARD_WIDTH * size, BOARD_HEIGHT * size);
    clutter_actor_set_position (CLUTTER_ACTOR (tmp), 0, 0);
    clutter_actor_show (CLUTTER_ACTOR (tmp));

    clutter_actor_add_child (g_actor_layers[BOARD_LAYER], CLUTTER_ACTOR (tmp));
}

T_SGEObject *
gweled_draw_score_message (gchar * in_message, T_SGELayer layer, gint msg_x, gint msg_y)
{
	T_SGEObject *object;
    T_SGETextData *text_data = (T_SGETextData *) g_malloc (sizeof (T_SGETextData));

    text_data->string = in_message;
    text_data->relative_font_size = 65;
    text_data->text_color = COLOR_CREATE(0xff, 0xd7, 0x00);
    text_data->outline_color = COLOR_CREATE(0x00, 0x00, 0x00);

    object = sge_create_score_text_object (msg_x, msg_y, TEXT_LAYER, text_data);

    return object;
}

void
gweled_draw_game_message (const gchar * message, guint lifetime)
{
	T_SGEObject *object;
    T_SGETextData *text_data = (T_SGETextData *) g_malloc (sizeof (T_SGETextData));

    text_data->string = message;
    text_data->relative_font_size = 82;
    text_data->text_color = COLOR_CREATE(0xff, 0xd7, 0x00);
    text_data->outline_color = COLOR_CREATE(0x00, 0x00, 0x00);

    object = sge_create_fullscreen_text_object (TEXT_LAYER, text_data);

    sge_object_zoomin (object, 500, CLUTTER_EASE_OUT_BACK);

    if (lifetime > 0)
        sge_object_fadeout(object, lifetime, 500);

}

//const gchar* gems[] = {"\e[1;37;40mWH", "\e[1;36;40mBL", "\e[0;33;40mAR", "\e[1;35;40mVI", "\e[1;31;40mRO", "\e[1;33;40mYE", "\e[1;32;40mGR"};

gboolean
gweled_gems_ready_to_fall_check (gpointer data)
{
    if (sge_objects_are_moving_on_layer (GEMS_LAYER))
        g_timeout_add (10, gweled_gems_ready_to_fall_check, data);
    else
        gweled_gems_fall_into_place (GPOINTER_TO_INT(data));

    return FALSE;
}

void
gweled_gems_fall_into_place (gboolean new_board_animation)
{
	gint i, j;
    gint delay_incr = 0;
    gint max_height = 0;

    // Avoid gems falling if there are something moving/animating.
    if (sge_objects_are_moving_on_layer (GEMS_LAYER)) {
        g_timeout_add (10, gweled_gems_ready_to_fall_check, GINT_TO_POINTER(new_board_animation));
        return;
    }

    for (i = BOARD_WIDTH - 1; i >= 0; i--) {
        delay_incr = 0;
        for (j = BOARD_HEIGHT - 1; j >= 0; j--) {

            if (g_gem_objects[i][j]->y == j) continue;

            if (max_height < j)
                max_height = j;

            if (new_board_animation)
                sge_object_fall_to_with_effect (g_gem_objects[i][j], j,
                                    (i * 100) + ((BOARD_HEIGHT - j) * 50));
            else
                sge_object_fall_to (g_gem_objects[i][j], j,
                                    // delay incremental
                                    delay_incr * 25,
                                    // trying to have the same speed regardless the destination
                                    100 + 20 * max_height);

            delay_incr++;
        }
    }

}

void
gweled_set_objects_size (gint size)
{
	g_print("gweled_set_objects_size %d\n", size);

    // Destroy board background texture
    clutter_actor_remove_all_children (g_actor_layers[BOARD_LAYER]);

    // Reload SVG at new sizes.
    gweled_load_pixmaps (size);

    // Draw the board background texture
    gweled_draw_board(size);

    // Resize all the Clutter objects
    sge_objects_resize(size);

}
