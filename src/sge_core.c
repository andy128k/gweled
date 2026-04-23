/* Gweled - Seb's Graphic Engine (SGE)
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

/* Levels:
 * 0 -> board background
 * 1 -> gems
 * 2 -> cursor
 * 3 -> text
 */


#include <gtk/gtk.h>
#include <math.h>

#include "gweled-gui.h"
#include "board_engine.h"
#include "graphic_engine.h"
#include "sge_core.h"

static const char * const PixbufStrings[] = {
    "\e[1;37mgem\e[0;0m",
    "\e[1;34mgem\e[0;0m",
    "\e[1;33mgem\e[0;0m",
    "\e[1;35mgem\e[0;0m",
    "\e[1;31mgem\e[0;0m",
    "\e[1;93mgem\e[0;0m",
    "\e[1;92mgem\e[0;0m",
    "bg",
    "\e[1mcursor\e[0;0m"
};
#define PIXBUF_ID_TO_STRING(val) ((val) >= 0 && (val) < sizeof(PixbufStrings) ? PixbufStrings[val] : "UNKNOWN")

GList *g_object_list;

// LOCAL VARS
static GdkPixbuf **g_pixbufs;
static gint gi_nb_pixbufs;

static T_SGEObject *object_bouncing = NULL;

#define HINT_GEM_BOUNCING_TIMEOUT_SECS  2

GdkPixbuf *
sge_get_pixbuf(gint index) {
    return g_pixbufs[index];
}

gint
sge_register_pixbuf (GdkPixbuf * pixbuf, int index)
{
	int i;

	if (index == -1) {
		for (i = 0; i < gi_nb_pixbufs; i++)
			if (g_pixbufs[i] == 0)
				break;
		if (i == gi_nb_pixbufs) {
			gi_nb_pixbufs++;
			g_pixbufs =
			    (GdkPixbuf **) g_realloc (g_pixbufs,
						      sizeof (GdkPixbuf *)
						      * gi_nb_pixbufs);
		}
		g_pixbufs[i] = pixbuf;
	} else {
		i = index;
		g_object_unref (g_pixbufs[i]);
		g_pixbufs[i] = pixbuf;
	}

	return i;
}

static void
on_animation_done (gpointer object) {
    SGE_OBJECT(object)->animation_data.animation_type = SGE_ANIM_NONE;
}

void
sge_destroy_object (gpointer obj, gpointer user_data)
{
    T_SGEObject *object = SGE_OBJECT(obj);

    g_signal_handlers_disconnect_by_func (object->animation, (gpointer) on_animation_done, object);

    g_clear_object (&object->animation);
    if (object->text_data) {
        g_clear_pointer (&object->text_data->string, g_free);
    }
    g_clear_pointer (&object->text_data, g_free);

    g_object_list = g_list_remove (g_object_list, object);
    g_free(object);
}

void
sge_destroy_object_on_level (gpointer object, gpointer user_data)
{
    T_SGELayer level = GPOINTER_TO_INT(user_data);
    // destroy only objects in the specified level
    if(SGE_OBJECT(object)->layer != level)
        return;

    sge_destroy_object(object, NULL);
}

void
sge_destroy_all_objects (void)
{
    GList *copy = g_list_copy (g_object_list);
    g_list_foreach (copy, sge_destroy_object, NULL);
    g_list_free (copy);
}

void
sge_destroy_all_objects_on_level (T_SGELayer level)
{
    GList *copy = g_list_copy (g_object_list);
    g_list_foreach (copy, sge_destroy_object_on_level, GINT_TO_POINTER(level));
    g_list_free (copy);
}


static void
sge_destroy_on_transition_ended (AdwAnimation *animation G_GNUC_UNUSED, gpointer object) {
    sge_destroy_object (SGE_OBJECT(object), NULL);
}

// animations/effects

static void
object_set_animation_value_cb (gdouble value G_GNUC_UNUSED, gpointer user_data) {
    gtk_widget_queue_draw (GTK_WIDGET (user_data));
}

static void
object_animate_move (T_SGEObject * object,
                     gint dest_x,
                     gint dest_y,
                     gint delay,
                     gint speed,
                     AdwEasing easing
) {
    object->animation_data.animation_type = SGE_ANIM_MOVE;
    object->animation_data.data.from_x = object->x;
    object->animation_data.data.from_y = object->y;
    object->animation_data.data.to_x   = dest_x;
    object->animation_data.data.to_y   = dest_y;

    object->x = dest_x;
    object->y = dest_y;

    adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (object->animation), 0.0);
    adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (object->animation), 1.0);
    adw_timed_animation_set_duration (ADW_TIMED_ANIMATION (object->animation), speed);
    adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (object->animation), easing);

    if (delay)
        g_timeout_add_once (delay, (GSourceOnceFunc) adw_animation_play, object->animation);
    else
        adw_animation_play (ADW_ANIMATION (object->animation));
}

// used for gems swapping
void
sge_object_move_to (T_SGEObject * object, gint dest_x, gint dest_y)
{
    object_animate_move (object, dest_x, dest_y, 0, 200, ADW_EASE_IN_OUT_CUBIC);
}

void
sge_object_fall_to (T_SGEObject * object, gint y_pos, gint delay, gint speed)
{
    object_animate_move (object, object->x, y_pos, delay, speed, ADW_EASE_OUT_CUBIC);
}

// Used only for the game start animation
void
sge_object_fall_to_with_effect (T_SGEObject * object, gint y_pos, gint delay)
{
    object_animate_move (object, object->x, y_pos, delay, 500, ADW_EASE_IN_QUAD);
}

//other useful stuff

gboolean
sge_object_is_moving (T_SGEObject * object)
{
    return adw_animation_get_state (object->animation) == ADW_ANIMATION_PLAYING;
}

gboolean
sge_objects_are_moving_on_layer (T_SGELayer layer)
{
	T_SGEObject *object;
    gboolean moving = FALSE;

	for (size_t i = 0; i < g_list_length (g_object_list); i++) {
		object = SGE_OBJECT (g_list_nth_data (g_object_list, i));
		if (object->layer == layer && sge_object_is_moving (object)) {
			moving = TRUE;
            break;
        }
	}

	return moving;
}

// fadeout the object and then destroy it
void sge_object_fadeout (T_SGEObject *object, guint delay_secs, guint duration)
{
    object->animation_data.animation_type = SGE_ANIM_OPACITY;

    adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (object->animation), 255.0);
    adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (object->animation), 0.0);
    adw_timed_animation_set_duration (ADW_TIMED_ANIMATION (object->animation), duration);
    adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (object->animation), ADW_LINEAR);

    g_signal_connect_after (object->animation, "done",
		    G_CALLBACK (sge_destroy_on_transition_ended),
		    object);

    g_timeout_add_once (delay_secs * 1000, (GSourceOnceFunc) adw_animation_play, object->animation);
}

// Destroy the gem with some effects
void sge_gem_destroy (T_SGEObject *object)
{
    if (object == NULL) return;

    object->animation_data.animation_type = SGE_ANIM_ZOOM_OUT;

    adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (object->animation), 0.0);
    adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (object->animation), 3.0);
    adw_timed_animation_set_duration (ADW_TIMED_ANIMATION (object->animation), 240);
    adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (object->animation), ADW_LINEAR);

    g_signal_connect_after (object->animation, "done",
		    G_CALLBACK (sge_destroy_on_transition_ended),
		    object);

    adw_animation_play (object->animation);
}

void
sge_object_fly_away (T_SGEObject *object)
{
    object->animation_data.animation_type = SGE_ANIM_FLY_AWAY;

    adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (object->animation), 0.0);
    adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (object->animation), 1.0);
    adw_timed_animation_set_duration (ADW_TIMED_ANIMATION (object->animation), 800);
    adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (object->animation), ADW_LINEAR);

    g_signal_connect_after (object->animation, "done",
		    G_CALLBACK (sge_destroy_on_transition_ended),
		    object);

    adw_animation_play (object->animation);
}

void
sge_object_zoomin (T_SGEObject *object, guint msecs, AdwEasing mode)
{
    object->animation_data.animation_type = SGE_ANIM_SCALE;

    adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (object->animation), 0.8);
    adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (object->animation), 1.0);
    adw_timed_animation_set_duration (ADW_TIMED_ANIMATION (object->animation), msecs);
    adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (object->animation), mode);

    g_timeout_add_once (50, (GSourceOnceFunc) adw_animation_play, object->animation);
}

void sge_object_reset_effects (T_SGEObject *object)
{
    if (object->animation != NULL) {
        adw_timed_animation_set_repeat_count (ADW_TIMED_ANIMATION (object->animation), 1);
        adw_animation_skip (object->animation);
    }
}

gboolean
sge_object_exists (T_SGEObject *object)
{
    if(g_list_index(g_object_list, object) == -1)
        return FALSE;
    else
        return TRUE;
}

void
sge_object_stop_effect (T_SGEObject *object) {
    sge_object_reset_effects (object);
}

void sge_object_blink_start (T_SGEObject *object)
{
    object->animation_data.animation_type = SGE_ANIM_BLINK;

    adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (object->animation), 0.0);
    adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (object->animation), 1.0);
    adw_timed_animation_set_duration (ADW_TIMED_ANIMATION (object->animation), 400);
    adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (object->animation), ADW_LINEAR);
    adw_timed_animation_set_repeat_count (ADW_TIMED_ANIMATION (object->animation), 0);

    adw_animation_play (ADW_ANIMATION (object->animation));
}

void sge_object_blink_stop (T_SGEObject *object)
{
    sge_object_stop_effect (object);
}

void sge_object_bounce (T_SGEObject *object)
{
    object_bouncing = object;

    object->animation_data.animation_type = SGE_ANIM_BOUNCE;

    adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (object->animation), 0.0);
    adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (object->animation), 500.0);
    adw_timed_animation_set_duration (ADW_TIMED_ANIMATION (object->animation), 500);
    adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (object->animation), ADW_EASE_IN_OUT_QUAD);
    adw_timed_animation_set_repeat_count (ADW_TIMED_ANIMATION (object->animation), 4);

    adw_animation_play (ADW_ANIMATION (object->animation));
}

void sge_stop_bouncing (void)
{
    if (object_bouncing) {
        sge_object_stop_effect (object_bouncing);
        object_bouncing = NULL;
    }
}

//objects creation/destruction

T_SGEObject *
sge_create_object (GtkWidget *widget, gint x, gint y, T_SGELayer layer, gint pixbuf_id)
{
    g_print("sge_create_object %s at %i:%i layer:%i\n", PIXBUF_ID_TO_STRING(pixbuf_id), x, y, layer);
    
    T_SGEObject * object;
    object = g_new0 (T_SGEObject, 1);
    object->x = x;
    object->y = y;
    object->pixbuf_id = pixbuf_id;

    object->layer = layer;

    object->text_data = NULL;

    object->width = 1;
    object->height = 1;

    AdwAnimationTarget* target = adw_callback_animation_target_new (object_set_animation_value_cb, widget, NULL);
    object->animation = adw_timed_animation_new (widget, 0.0, 1.0, 1, target);
    object->animation_data.animation_type = SGE_ANIM_NONE;
    g_signal_connect_swapped (object->animation, "done",
                              G_CALLBACK (on_animation_done), object);

    g_object_list = g_list_append (g_object_list, (gpointer) object);

    return object;
}

static void
generic_text_canvas_draw(GtkWidget* widget, GtkSnapshot *snapshot, int width, int height, T_SGETextData *text_data) {

    gfloat board_width = gtk_widget_get_width (widget);

    // Set the text in the default layout
    PangoContext* context = gtk_widget_get_pango_context(widget);
    PangoFontDescription *desc = pango_context_get_font_description (context);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    pango_font_description_set_absolute_size(desc, board_width * text_data->relative_font_size);

    PangoLayout *layout = gtk_widget_create_pango_layout (widget, text_data->string);
    pango_layout_set_font_description(layout, desc);

    // Text centered and layout at full width.
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_width (layout, width * PANGO_SCALE);

    // Center vertically the text.
    gint lw, lh;
    pango_layout_get_size(layout, &lw, &lh);
    gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, height / 2.0 - (double)lh / PANGO_SCALE / 2.0));

    GskPathBuilder *builder = gsk_path_builder_new ();
    gsk_path_builder_add_layout (builder, layout);
    GskPath *path = gsk_path_builder_free_to_path (builder);

    GskStroke *stroke = gsk_stroke_new (board_width / 120.0);
    gsk_stroke_set_line_cap (stroke, GSK_LINE_CAP_ROUND);
    gtk_snapshot_append_stroke (snapshot,
                                path,
                                stroke,
                                &(GdkRGBA) {
                                    COLOR_RED (text_data->outline_color) / 255.0,
                                    COLOR_GREEN (text_data->outline_color) / 255.0,
                                    COLOR_BLUE (text_data->outline_color) / 255.0,
                                  1.0 });

    gtk_snapshot_append_fill (snapshot,
                              path,
                              GSK_FILL_RULE_WINDING,
                              &(GdkRGBA) {
                                  COLOR_RED (text_data->text_color) / 255.0,
                                  COLOR_GREEN (text_data->text_color) / 255.0,
                                  COLOR_BLUE (text_data->text_color) / 255.0,
                                  1.0 });

    // Free resources
    gsk_stroke_free (stroke);
    g_object_unref(layout);
}


void
sge_object_snapshot (GtkWidget* widget,
                     T_SGEObject *object,
                     GtkSnapshot *snapshot,
                     double scale_x,
                     double scale_y,
                     int tile_size)
{
    gtk_snapshot_save (snapshot);

    gtk_snapshot_translate (snapshot,
                            &GRAPHENE_POINT_INIT ((object->x + object->width / 2.0) * tile_size,
                                                  (object->y + object->height / 2.0) * tile_size));
    gtk_snapshot_scale (snapshot, scale_x, scale_y);
    gtk_snapshot_translate (snapshot,
                            &GRAPHENE_POINT_INIT (- object->width / 2.0 * tile_size,
                                                  - object->height / 2.0 * tile_size));

    graphene_rect_t bounds = GRAPHENE_RECT_INIT (0, 0, object->width * tile_size, object->height * tile_size);
    if (object->pixbuf_id >= 0) {
        GdkTexture* texture = gdk_texture_new_for_pixbuf (GDK_PIXBUF(g_pixbufs[object->pixbuf_id]));
        gtk_snapshot_append_texture (snapshot, texture, &bounds);
    }
    if (object->text_data) {
        generic_text_canvas_draw (widget, snapshot, object->width * tile_size, object->height * tile_size, object->text_data);
    }

    gtk_snapshot_restore (snapshot);
}


T_SGEObject *
sge_create_score_text_object (GtkWidget *widget, double x, double y, T_SGELayer layer, T_SGETextData *text_data)
{
    T_SGEObject * object;
    object = g_new0 (T_SGEObject, 1);
    object->x = x;
    object->y = y;
    object->pixbuf_id = -1;

    object->layer = layer;

    object->width = 2;
    object->height = 1;

    object->text_data = text_data;

    AdwAnimationTarget* target = adw_callback_animation_target_new (object_set_animation_value_cb, widget, NULL);
    object->animation = adw_timed_animation_new (widget, 0.0, 1.0, 1, target);
    object->animation_data.animation_type = SGE_ANIM_NONE;
    g_signal_connect_swapped (object->animation, "done",
                              G_CALLBACK (on_animation_done), object);

    g_object_list = g_list_append (g_object_list, (gpointer) object);

    return object;
}


T_SGEObject *
sge_create_fullscreen_text_object (GtkWidget *widget, T_SGELayer layer, T_SGETextData *text_data)
{
    T_SGEObject * object;
    object = g_new0 (T_SGEObject, 1);
    object->x = 0;
    object->y = 0;
    object->pixbuf_id = -1;

    object->layer = layer;

    object->width = BOARD_WIDTH;
    object->height = BOARD_HEIGHT;

    object->text_data = text_data;

    AdwAnimationTarget* target = adw_callback_animation_target_new (object_set_animation_value_cb, widget, NULL);
    object->animation = adw_timed_animation_new (widget, 0.0, 1.0, 1, target);
    object->animation_data.animation_type = SGE_ANIM_NONE;
    g_signal_connect_swapped (object->animation, "done",
                              G_CALLBACK (on_animation_done), object);

    g_object_list = g_list_append (g_object_list, (gpointer) object);

    return object;
}

void
sge_destroy (void)
{
	int i;

    sge_destroy_all_objects();

    for (i = 0; i < gi_nb_pixbufs; i++)
		g_object_unref (g_pixbufs[i]);
	g_free (g_pixbufs);

}

// creation/destruction
void
sge_init (void)
{
	gi_nb_pixbufs = 0;
	g_pixbufs = NULL;
}
