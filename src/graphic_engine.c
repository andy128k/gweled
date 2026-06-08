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

/* for exit() */
#include <stdlib.h>
/* for fabs() */
#include <math.h>

#include <gtk/gtk.h>

#include "graphic_engine.h"
#include "sge_core.h"
#include "sge_utils.h"
#include "board_engine.h"

extern GList *g_object_list;

gint gi_tiles_bg_pixbuf = -1;
gint gi_gems_pixbuf[7] = {-1, -1, -1, -1, -1, -1, -1};
gint gi_cursor_pixbuf = -1;

struct _GweledStage {
    GtkWidget parent;
};

typedef struct _GweledStagePrivate {
    T_SGELayerState layers[4];

    gint x_press;
    gint y_press;
} GweledStagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GweledStage, gweled_stage, GTK_TYPE_WIDGET)

static void
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
gweled_stage_set_layer_visibility (GweledStage* stage, T_SGELayer layer, gboolean visibility) {
    GweledStagePrivate *priv = gweled_stage_get_instance_private (stage);

    priv->layers[layer].visible = visibility;
}

void
gweled_stage_set_layer_opacity (GweledStage* stage, T_SGELayer layer, gdouble opacity) {
    GweledStagePrivate *priv = gweled_stage_get_instance_private (stage);

    priv->layers[layer].opacity = opacity;
}

T_SGEObject *
gweled_stage_create_score_message (GweledStage *stage, const gchar *message, double msg_x, double msg_y)
{
    T_SGEObject *object;
    T_SGETextData *text_data = g_new0 (T_SGETextData, 1);

    text_data->string = g_strdup (message);
    text_data->relative_font_size = 65;
    text_data->text_color = COLOR_CREATE(0xff, 0xd7, 0x00);
    text_data->outline_color = COLOR_CREATE(0x00, 0x00, 0x00);

    object = sge_create_score_text_object (GTK_WIDGET (stage), msg_x, msg_y, TEXT_LAYER, text_data);

    return object;
}

void
gweled_stage_create_game_message (GweledStage *stage, const gchar *message, guint lifetime)
{
    T_SGEObject *object;
    T_SGETextData *text_data = g_new0 (T_SGETextData, 1);

    text_data->string = g_strdup (message);
    text_data->relative_font_size = 82;
    text_data->text_color = COLOR_CREATE(0xff, 0xd7, 0x00);
    text_data->outline_color = COLOR_CREATE(0x00, 0x00, 0x00);

    object = sge_create_fullscreen_text_object (GTK_WIDGET (stage), TEXT_LAYER, text_data);

    sge_object_zoomin (object, 500, ADW_EASE_OUT_BACK);

    if (lifetime > 0)
        sge_object_fadeout(object, lifetime, 500);
}

static void
button_pressed (GtkGestureClick* gesture G_GNUC_UNUSED,
                gint n_press G_GNUC_UNUSED,
                gdouble x,
                gdouble y,
                gpointer user_data
) {
    GweledStage *stage = GWELED_STAGE (user_data);
    GweledStagePrivate *priv = gweled_stage_get_instance_private (stage);

    // Stop hint gem bouncing
    sge_stop_bouncing();

    int width = gtk_widget_get_width (GTK_WIDGET (stage));
    int height = gtk_widget_get_height (GTK_WIDGET (stage));
    int tile_size = MIN (width / BOARD_WIDTH, height / BOARD_HEIGHT);

    priv->x_press = floor(x) / tile_size;
    priv->y_press = floor(y) / tile_size;

    g_signal_emit_by_name (stage, "clicked", priv->x_press, priv->y_press);
}

static void
button_released (GtkGestureClick* gesture G_GNUC_UNUSED,
                 gint n_press G_GNUC_UNUSED,
                 gdouble x,
                 gdouble y,
                 gpointer user_data
) {
    GweledStage *stage = GWELED_STAGE (user_data);
    GweledStagePrivate *priv = gweled_stage_get_instance_private (stage);

    int width = gtk_widget_get_width (GTK_WIDGET (stage));
    int height = gtk_widget_get_height (GTK_WIDGET (stage));
    int tile_size = MIN (width / BOARD_WIDTH, height / BOARD_HEIGHT);

    gint x_release = floor(x) / tile_size;
    gint y_release = floor(y) / tile_size;

    if (priv->x_press != x_release || priv->y_press != y_release) {
        g_signal_emit_by_name (stage, "clicked", x_release, y_release);
    }
}

static void
gweled_stage_size_allocate (GtkWidget* widget G_GNUC_UNUSED,
                            int width,
                            int height,
                            int baseline G_GNUC_UNUSED)
{
    int tile_size = MIN (width / BOARD_WIDTH, height / BOARD_HEIGHT);

    // Reload SVG at new sizes.
    if (gi_cursor_pixbuf == -1 || gdk_pixbuf_get_width (GDK_PIXBUF (sge_get_pixbuf (gi_cursor_pixbuf))) != tile_size) {
        gweled_load_pixmaps (tile_size);
    }
}

static void
gweled_stage_snapshot (GtkWidget* widget, GtkSnapshot* snapshot) {
    GweledStagePrivate *priv = gweled_stage_get_instance_private (GWELED_STAGE (widget));

    int width = gtk_widget_get_width (widget);
    int height = gtk_widget_get_height (widget);

    int tile_size = MIN (width / BOARD_WIDTH, height / BOARD_HEIGHT);

    graphene_rect_t board_bounds = GRAPHENE_RECT_INIT (0, 0, tile_size * BOARD_WIDTH, tile_size * BOARD_HEIGHT);
    gtk_snapshot_push_clip (snapshot, &board_bounds);

    GdkTexture* bg_texture = gdk_texture_new_for_pixbuf (GDK_PIXBUF(sge_get_pixbuf(gi_tiles_bg_pixbuf)));
    graphene_rect_t bounds = GRAPHENE_RECT_INIT (0, 0, tile_size * 2, tile_size * 2);
    for (int x = 0; x < BOARD_WIDTH; x += 2)
        for (int y = 0; y < BOARD_HEIGHT; y += 2) {
            gtk_snapshot_save (snapshot);
            graphene_point_t offset = { x * tile_size, y * tile_size };
            gtk_snapshot_translate (snapshot, &offset);
            gtk_snapshot_append_texture (snapshot, bg_texture, &bounds);
            gtk_snapshot_restore (snapshot);
        }

    for (T_SGELayer layer = BOARD_LAYER; layer <= TEXT_LAYER; ++layer) {
        if (!priv->layers[layer].visible)
            continue;

        gtk_snapshot_push_opacity (snapshot, priv->layers[layer].opacity);

        for (guint i = 0; i < g_list_length (g_object_list); i++) {
            T_SGEObject *object = SGE_OBJECT (g_list_nth_data (g_object_list, i));
            if (object->layer != layer)
                continue;

            double value = adw_animation_get_value (object->animation);
            double scale, scale_x, scale_y;
            switch (object->animation_data.animation_type) {
            case SGE_ANIM_MOVE:
                gtk_snapshot_save (snapshot);
                gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (
                    (object->animation_data.data.from_x - object->animation_data.data.to_x) * (1.0 - value) * tile_size,
                    (object->animation_data.data.from_y - object->animation_data.data.to_y) * (1.0 - value) * tile_size
                ));
                sge_object_snapshot (widget, object, snapshot, 1.0, 1.0, tile_size);
                gtk_snapshot_restore (snapshot);
                break;
            case SGE_ANIM_OPACITY:
                gtk_snapshot_push_opacity (snapshot, value);
                sge_object_snapshot (widget, object, snapshot, 1.0, 1.0, tile_size);
                gtk_snapshot_pop (snapshot);
                break;
            case SGE_ANIM_BLINK:
                gtk_snapshot_push_opacity (snapshot,
                                           fabs (value - 0.5) * 0.8 + 0.6);
                sge_object_snapshot (widget, object, snapshot, 1.0, 1.0, tile_size);
                gtk_snapshot_pop (snapshot);
                break;
            case SGE_ANIM_BOUNCE:
                gtk_snapshot_save (snapshot);
                if (value < 350) {
                    value = value / 350.0;
                    gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, value * tile_size * 0.07));
                    scale_x = 1.0 + 0.1 * value; // scale from 1.0 to 1.1
                    scale_y = 1.0 - 0.1 * value; // scale from 1.0 to 0.9
                } else {
                    value = (value - 350.0) / 150.0;
                    gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, (1.0 - value) * tile_size * 0.07));
                    scale_x = 1.1 - 0.1 * value; // scale from 1.1 to 1.0
                    scale_y = 0.9 + 0.1 * value; // scale from 0.9 to 1.0
                }
                sge_object_snapshot (widget, object, snapshot, scale_x, scale_y, tile_size);
                gtk_snapshot_restore (snapshot);
                break;
            case SGE_ANIM_SCALE:
                sge_object_snapshot (widget, object, snapshot, value, value, tile_size);
                break;
            case SGE_ANIM_FLY_AWAY:
                gtk_snapshot_push_opacity (snapshot,
                                           MIN (8.0 - 8.0 * value, 1.0)); // After 700ms start to fade away
                gtk_snapshot_save (snapshot);
                gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, - value * tile_size / 3.0));
                sge_object_snapshot (widget, object, snapshot, 1.0, 1.0, tile_size);
                gtk_snapshot_restore (snapshot);
                gtk_snapshot_pop (snapshot);
                break;
            case SGE_ANIM_ZOOM_OUT:
                gtk_snapshot_save (snapshot);
                scale = value < 1.0
                    ? 1.0 + adw_easing_ease (ADW_EASE_OUT_CUBIC, value) * 0.2 // growth 1->1.2
                    : (3.0 - value) / 2.0 * 1.2; // shrinks 1.2->0
                sge_object_snapshot (widget, object, snapshot, scale, scale, tile_size);
                gtk_snapshot_restore (snapshot);
                break;
            case SGE_ANIM_NONE:
            default:
                sge_object_snapshot (widget, object, snapshot, 1.0, 1.0, tile_size);
            }
        }

        gtk_snapshot_pop (snapshot);
    }

    gtk_snapshot_pop (snapshot);
}

static void
gweled_stage_class_init (GweledStageClass *klass) {
    GTK_WIDGET_CLASS (klass)->size_allocate = gweled_stage_size_allocate;
    GTK_WIDGET_CLASS (klass)->snapshot = gweled_stage_snapshot;

    g_signal_new ("clicked",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        NULL,
        G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
}

static void
gweled_stage_init (GweledStage *stage) {
    GweledStagePrivate *priv = gweled_stage_get_instance_private (stage);

    priv->x_press = -1;
    priv->y_press = -1;
    for (int i = 0; i < 4; i++) {
        priv->layers[i].visible = TRUE;
        priv->layers[i].opacity = 1.0;
    }

    gtk_widget_set_can_focus (GTK_WIDGET (stage), TRUE);

    GtkGesture *click = gtk_gesture_click_new ();
    g_signal_connect (click, "pressed", G_CALLBACK (button_pressed), stage);
    g_signal_connect (click, "released", G_CALLBACK (button_released), stage);
    gtk_widget_add_controller (GTK_WIDGET (stage), GTK_EVENT_CONTROLLER (click));
}
