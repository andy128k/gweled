/* Gweled - Seb's Graphic Engine (SGE)
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

/* Levels:
 * 0 -> board background
 * 1 -> gems
 * 2 -> cursor
 * 3 -> text
 */


#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <math.h>

#include "gweled-gui.h"
#include "board_engine.h"
#include "graphic_engine.h"
#include "sound.h"
#include "sge_core.h"

#define CLUTTER_TIMELINE_DURATION  200

// LOCAL VARS
static GList *g_object_list;
static GdkPixbuf **g_pixbufs;
static gint gi_nb_pixbufs;

extern gint gi_gem_clicked;
extern gint gi_x_click;
extern gint gi_y_click;

extern gint gi_gem_dragged;
extern gint gi_x_drag;
extern gint gi_y_drag;

extern GuiContext *gweled_ui;

extern GweledPrefs prefs;

ClutterActor *g_gameboard = NULL;

ClutterActor *g_actor_layers[5] = {NULL, NULL, NULL, NULL, NULL};

T_SGEObject *object_bouncing = NULL;
guint object_bounce_timout_id = 0;

#define HINT_GEM_BOUNCING_TIMEOUT_SECS  2

GdkPixbuf *
sge_get_pixbuf(gint index) {
    return g_pixbufs[index];
}

// helper functions
gint
compare_by_layer (gconstpointer a, gconstpointer b)
{
	return SGE_OBJECT(a)->layer - SGE_OBJECT(b)->layer;
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



void
sge_destroy_object (gpointer object, gpointer user_data)
{
	if (SGE_OBJECT(object)->actor) {

        if (SGE_OBJECT(object)->layer == TEXT_LAYER) {
            g_object_unref(CLUTTER_CANVAS (clutter_actor_get_content (SGE_OBJECT(object)->actor)));
            g_free(SGE_OBJECT(object)->text_data);
        }
	    clutter_actor_destroy (SGE_OBJECT(object)->actor);
        SGE_OBJECT(object)->actor = NULL;
	}

    if (SGE_OBJECT(object)->effect_handler_id) {
        g_source_remove(SGE_OBJECT(object)->effect_handler_id);
    }

	g_object_list = g_list_remove (g_object_list, object);
	g_free(object);
    object = NULL;
}

void
sge_destroy_object_on_level (gpointer object, gpointer user_data)
{
    // destroy only objects in the specified level
    if(SGE_OBJECT(object)->layer != GPOINTER_TO_INT(user_data))
        return;

    sge_destroy_object(object, NULL);
}

void
sge_destroy_all_objects (void)
{
	g_list_foreach (g_object_list, sge_destroy_object, NULL);
}

void
sge_destroy_all_objects_on_level (T_SGELayer level)
{
	g_list_foreach (g_object_list, sge_destroy_object_on_level, GINT_TO_POINTER(level));
}


void
sge_destroy_on_transition_ended (ClutterActor *actor,
                         char         *name,
                         gboolean      is_finished,
                         gpointer      object) {
                         
    SGE_OBJECT(object)->animating = FALSE;
    sge_destroy_object (SGE_OBJECT(object), NULL);
}

static void
sge_destroy_on_specific_transition_ended (ClutterTimeline *timeline,
                     gboolean         is_finished,
                     gpointer         user_data){

    sge_destroy_object (SGE_OBJECT(user_data), NULL);
}

void
sge_finished_animation (ClutterActor *actor,
                        gpointer      object) {

    SGE_OBJECT(object)->animating = FALSE;
    g_signal_handler_disconnect (actor, SGE_OBJECT(object)->animating_handler_id);
}

// animations/effects

// used for gems swapping
void
sge_object_move_to (T_SGEObject * object, gint dest_x, gint dest_y)
{
  object->x = dest_x;
  object->y = dest_y;

  object->animating = TRUE;
  
  clutter_actor_save_easing_state (object->actor);
  clutter_actor_set_easing_mode(object->actor, CLUTTER_EASE_IN_OUT_CUBIC);
  clutter_actor_set_easing_duration (object->actor, 200);
  clutter_actor_set_position (object->actor, dest_x * prefs.tile_size, dest_y * prefs.tile_size);
  clutter_actor_restore_easing_state (object->actor);

  object->animating_handler_id = g_signal_connect (object->actor, "transitions-completed",
		    G_CALLBACK (sge_finished_animation),
		    object);
}

void
sge_object_fall_to (T_SGEObject * object, gint y_pos, gint delay, gint speed)
{
    object->animating = TRUE;

    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode(object->actor, CLUTTER_EASE_OUT_CUBIC);
    clutter_actor_set_easing_duration (object->actor, speed);
    clutter_actor_set_easing_delay (object->actor, delay);
    clutter_actor_set_position (object->actor, object->x * prefs.tile_size, y_pos * prefs.tile_size);
    clutter_actor_restore_easing_state (object->actor);

    object->y = y_pos;

    object->animating_handler_id = g_signal_connect (object->actor, "transitions-completed",
		    G_CALLBACK (sge_finished_animation),
		    object);
}

// Used only for the game start animation
void
sge_object_fall_to_with_effect (T_SGEObject * object, gint y_pos, gint delay)
{
    object->y = y_pos;
  
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode(object->actor, CLUTTER_EASE_IN_QUAD);
    clutter_actor_set_easing_duration (object->actor, 500);
    clutter_actor_set_easing_delay (object->actor, delay);
    clutter_actor_set_position (object->actor, object->x * prefs.tile_size, object->y * prefs.tile_size);
    clutter_actor_restore_easing_state (object->actor);
}

//other useful stuff

gboolean
sge_object_is_moving (T_SGEObject * object)
{
    return object->animating;
}

gboolean
sge_objects_are_moving_on_layer (T_SGELayer layer)
{
	gint i;
	T_SGEObject *object;
    gboolean moving = FALSE;

	for (i = 0; i < g_list_length (g_object_list); i++) {
		object = SGE_OBJECT (g_list_nth_data (g_object_list, i));
		if (object->layer == layer && sge_object_is_moving (object)) {
			moving = TRUE;
            break;
        }
	}

	return moving;
}

void sge_set_layer_visibility (T_SGELayer layer, gboolean visibility)
{
    if (visibility) {
        clutter_actor_set_opacity(g_actor_layers[layer], 255);
    } else {
        clutter_actor_set_opacity(g_actor_layers[layer], 0);
    }
}

void sge_set_layer_opacity (T_SGELayer layer, guint8 opacity)
{
    clutter_actor_set_opacity(g_actor_layers[layer], opacity);
}


// fadeout the object and then destroy it
void sge_object_fadeout (T_SGEObject *object, guint delay_secs, guint duration)
{
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode(object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration (object->actor, duration);
    clutter_actor_set_easing_delay(object->actor, delay_secs * 1000);
    clutter_actor_set_opacity (object->actor, 0);

    ClutterTransition *transition = clutter_actor_get_transition (object->actor, "opacity");
    g_signal_connect (transition, "stopped",
	        G_CALLBACK (sge_destroy_on_specific_transition_ended),
	        object);
    
    clutter_actor_restore_easing_state (object->actor);
}

void
sge_zoomout_on_transition_ended (ClutterActor *actor,
                                 char         *name,
                                 gboolean      is_finished,
                                 gpointer      object) {

    sge_object_zoomout (SGE_OBJECT(object));
}

// Destroy the gem with some effects
void sge_gem_destroy (T_SGEObject *object)
{
    if (object == NULL) return;

    object->animating = TRUE;


    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_EASE_OUT_CUBIC);
    clutter_actor_set_easing_duration (object->actor, 80);
    clutter_actor_set_scale (object->actor, 1.2, 1.2);
    clutter_actor_restore_easing_state (object->actor);

    object->animating_handler_id = g_signal_connect (object->actor, "transition-stopped",
		    G_CALLBACK (sge_zoomout_on_transition_ended),
		    object);
}

// zoomout the object and then destroy it
void sge_object_zoomout (T_SGEObject *object)
{
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration (object->actor, 150);
    clutter_actor_set_scale (object->actor, 0, 0);
    clutter_actor_restore_easing_state (object->actor);
    
    g_signal_handler_disconnect (object->actor, object->animating_handler_id);

    g_signal_connect (object->actor, "transition-stopped",
		    G_CALLBACK (sge_destroy_on_transition_ended), 
		    object);
}

void
sge_object_fly_away (T_SGEObject *object)
{
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration (object->actor, 800);
    clutter_actor_set_position (object->actor,
                                clutter_actor_get_x (object->actor),
                                clutter_actor_get_y (object->actor) - (prefs.tile_size/3)
                                );
    clutter_actor_restore_easing_state (object->actor);
    
    // After 700ms start to fade away.
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_delay(object->actor, 700);
    clutter_actor_set_easing_duration (object->actor, 100);
    clutter_actor_set_opacity(object->actor, 0);

    ClutterTransition *transition = clutter_actor_get_transition (object->actor, "opacity");
    g_signal_connect (transition, "stopped",
	        G_CALLBACK (sge_destroy_on_specific_transition_ended),
	        object);

    clutter_actor_restore_easing_state (object->actor);
}

void
sge_object_zoomin (T_SGEObject *object, guint msecs, ClutterAnimationMode mode)
{
    clutter_actor_set_scale (object->actor, 0.8, 0.8);
    
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, mode);
    clutter_actor_set_easing_delay (object->actor, 50);
    clutter_actor_set_easing_duration (object->actor, msecs);
    clutter_actor_set_scale (object->actor, 1, 1);
    clutter_actor_restore_easing_state (object->actor);
}

void sge_object_reset_effects (T_SGEObject *object)
{
    if (object->actor == NULL) return;

    clutter_actor_remove_all_transitions (object->actor);

    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration(object->actor, 0);
    clutter_actor_set_opacity(object->actor, 255);
    clutter_actor_set_position (object->actor, object->x * prefs.tile_size, object->y * prefs.tile_size);
    clutter_actor_set_scale (object->actor, 1, 1);
    clutter_actor_restore_easing_state (object->actor);
}

gboolean
sge_object_exists (T_SGEObject *object)
{
    if(g_list_index(g_object_list, object) == -1)
        return FALSE;
    else
        return TRUE;
}

static gboolean
board_input_event (ClutterActor *stage,
                   ClutterEvent *event,
                   gpointer      dummy G_GNUC_UNUSED)
{
    gfloat x, y;
    static gint x_press = -1;
    static gint y_press = -1;
    static gint x_release = -1;
    static gint y_release = -1;
    static gboolean gi_dragging = FALSE;
     
    // resume game on click
    if (is_game_running() && board_get_pause() == TRUE ) {
        board_set_pause(FALSE);
        // not handle this click
        return FALSE;
    }

    // in pause mode don't accept events
    if (board_get_pause())
        return FALSE;

    // only handle left button
    if (event->type == CLUTTER_BUTTON_PRESS && clutter_event_get_button (event) != CLUTTER_BUTTON_PRIMARY)
        return FALSE;

    // Stop hint gem bouncing
    sge_stop_bouncing();
    
    // Can't use the actor position due to possible gems transformations.
    clutter_event_get_coords (event, &x, &y);
    
    g_debug("Board input! %i:%i [%.2lfx%.2lf] event_type:%i\n", gi_x_click, gi_y_click, x, y, event->type);

    switch (event->type) {
        case CLUTTER_BUTTON_PRESS:
        case CLUTTER_TOUCH_BEGIN:
            gi_x_click = x_press = round(x) / prefs.tile_size;
            gi_y_click = y_press = round(y) / prefs.tile_size;

            gi_gem_clicked = -1;

            sound_effect_play (CLICK_EVENT);

            if (event->type == CLUTTER_TOUCH_BEGIN) {
                gi_dragging = TRUE;
            }

            respawn_board_engine_loop();

            break;

        case CLUTTER_BUTTON_RELEASE:
        case CLUTTER_TOUCH_END:
        case CLUTTER_TOUCH_UPDATE:
        
            if (event->type == CLUTTER_TOUCH_UPDATE && gi_dragging == FALSE) {
                break;
            }
        
		    gi_gem_dragged = 0;

            x_release = round(x) / prefs.tile_size;
            y_release = round(y) / prefs.tile_size;

            if (x_press != x_release || y_press != y_release) {
                gi_x_click = x_release;
                gi_y_click = y_release;
			    gi_gem_clicked = -1;
                gi_dragging = FALSE;
            }

            respawn_board_engine_loop();

            break;

        default:
            break;
    }

	return CLUTTER_EVENT_STOP;
}


void
sge_objects_resize (gint size) {
    GError *error = NULL;
    int i;
    T_SGEObject *object;
    
    g_print("sge_objects_resize at %i\n", size);

    clutter_actor_set_size (g_gameboard,
                            BOARD_WIDTH * size,
                            BOARD_HEIGHT * size);

    // Resize levels.
    for (i = 0; i < 5; i++) {

        clutter_actor_set_size (g_actor_layers[i],
                                BOARD_WIDTH * size,
                                BOARD_HEIGHT * size);

        clutter_actor_set_clip(g_actor_layers[i], 0, 0, BOARD_WIDTH * size, BOARD_HEIGHT * size);
    }

    for (i = 0; i < g_list_length (g_object_list); i++) {
    
		object = SGE_OBJECT (g_list_nth_data (g_object_list, i));

        if (object->layer == TEXT_LAYER) {
            clutter_actor_set_size (object->actor, BOARD_WIDTH * size, BOARD_HEIGHT * size);

            clutter_canvas_set_size (CLUTTER_CANVAS(clutter_actor_get_content(object->actor)), BOARD_WIDTH * size, BOARD_HEIGHT * size);
        }
        else {

		    gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (object->actor),
                                             GDK_PIXBUF(g_pixbufs[object->pixbuf_id]), &error);
		    clutter_actor_set_size (object->actor, size, size);

            clutter_actor_set_position (object->actor,
                                        object->x * size,
                                        object->y * size);
        }
    }
    
}

gboolean
sge_object_effects_cb (gpointer data)
{
    T_SGEObject *object = SGE_OBJECT(data);

    if (object->effect == NONE) {
        object->effect_status = TRUE;
        sge_object_reset_effects (object);
        return FALSE;
    }

    clutter_actor_save_easing_state (object->actor);

    if (object->effect == BLINK) {

        clutter_actor_set_easing_mode(object->actor, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (object->actor, 200);
        if (object->effect_status) {
            clutter_actor_set_opacity (object->actor, 155);
            object->effect_status = FALSE;
        }
        else {
            clutter_actor_set_opacity (object->actor, 255);
            object->effect_status = TRUE;
        }

        object->effect_handler_id = g_timeout_add (200, sge_object_effects_cb, object);
    }

    if (object->effect == BOUNCE) {
            clutter_actor_set_easing_mode(object->actor, CLUTTER_EASE_IN_OUT_QUAD);
            if (object->effect_status) {
                clutter_actor_set_easing_duration (object->actor, 350);
                clutter_actor_set_position  (object->actor,
                                            object->x * prefs.tile_size,
                                            object->y * prefs.tile_size + (prefs.tile_size * 0.07));
                clutter_actor_set_scale  (object->actor, 1.1, 0.9);

                object->effect_status = FALSE;

                object->effect_handler_id = g_timeout_add (350, sge_object_effects_cb, object);
            }
            else {
                clutter_actor_set_easing_duration (object->actor, 150);
                clutter_actor_set_position (object->actor, object->x * prefs.tile_size, object->y * prefs.tile_size);
                clutter_actor_set_scale (object->actor, 1, 1);

                object->effect_status = TRUE;

                object->effect_handler_id = g_timeout_add (150, sge_object_effects_cb, object);
            }
    }

    clutter_actor_restore_easing_state (object->actor);

    return FALSE;
}

static void
sge_object_start_effect (T_SGEObject *object, T_SGEEffect effect)
{
    // Sets the effect
    object->effect = effect;
    // Reset the effect repetition status
    object->effect_status = TRUE;
    sge_object_effects_cb(object);
}

void
sge_object_stop_effect (T_SGEObject *object) {
    object->effect = NONE;
    if (object->effect_handler_id) {
        g_source_remove(object->effect_handler_id);
        object->effect_handler_id = 0;
    }
    sge_object_reset_effects (object);
}

void sge_object_blink_start (T_SGEObject *object)
{
    sge_object_start_effect (object, BLINK);
}

void sge_object_blink_stop (T_SGEObject *object)
{
    sge_object_stop_effect (object);
}

gboolean
sge_object_stop_bounce_cb (gpointer data)
{
    T_SGEObject *object = SGE_OBJECT(data);
    sge_object_stop_effect (object);
    object_bouncing = NULL;
    object_bounce_timout_id = 0;
    return FALSE;
}

void sge_object_bounce (T_SGEObject *object)
{
    sge_object_start_effect (object, BOUNCE);
    object_bouncing = object;
    object_bounce_timout_id = g_timeout_add_seconds (HINT_GEM_BOUNCING_TIMEOUT_SECS, sge_object_stop_bounce_cb, object);
}

void sge_stop_bouncing ()
{
    if (object_bouncing != NULL) {
        sge_object_stop_effect (object_bouncing);
        object_bouncing = NULL;
    }

    if (object_bounce_timout_id > 0) {
        g_source_remove(object_bounce_timout_id);
        object_bounce_timout_id = 0;
    }
}

//objects creation/destruction

T_SGEObject *
sge_create_object (gint x, gint y, T_SGELayer layer, gint pixbuf_id)
{

    GError *error;
    
    g_print("sge_create_object %i at %i:%i -> %i:%i layer:%i\n", pixbuf_id, x, y, x * prefs.tile_size, y * prefs.tile_size, layer);
    
    T_SGEObject * object;
	object = (T_SGEObject *) g_malloc (sizeof (T_SGEObject));
    object->x = x;
    object->y = y;
    object->pixbuf_id = pixbuf_id;

    object->y_delay = 0;

    object->animating = FALSE;

    object->effect = NONE;
    object->effect_handler_id = 0;

	object->layer = layer;

    object->text_data = NULL;

	object->width = gdk_pixbuf_get_width (g_pixbufs[pixbuf_id]);
    object->height = gdk_pixbuf_get_height (g_pixbufs[pixbuf_id]);
    
   
    // Clutter actors.
    object->actor = gtk_clutter_texture_new ();
    gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (object->actor),
                                         GDK_PIXBUF(g_pixbufs[pixbuf_id]), &error);

    clutter_actor_set_size (CLUTTER_ACTOR(object->actor),
                            object->width,
                            object->height);
                            
    clutter_actor_set_position (CLUTTER_ACTOR (object->actor),
                                x * prefs.tile_size,
                                y * prefs.tile_size);
    
    clutter_actor_add_child (g_actor_layers[layer],
                                 CLUTTER_ACTOR (object->actor));
                                 
    clutter_actor_show (object->actor);                            
        

    if (layer == GEMS_LAYER) {
        clutter_actor_set_pivot_point (object->actor, 0.5, 0.5);
	}

    g_object_list = g_list_append (g_object_list, (gpointer) object);

	return object;
}

static gboolean
generic_text_canvas_draw(ClutterCanvas *canvas, cairo_t *cr, int width, int height, gpointer user_data) {

    PangoFontDescription *desc;
    PangoLayout *layout;
    T_SGETextData *text_data = (T_SGETextData*) user_data;
    gint lw, lh;
    gfloat board_width = BOARD_WIDTH * prefs.tile_size;

    // Clean the surface.
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    // Half vertically
    cairo_translate(cr, 0, height / 2.0);

    // Set the text in the default layout
    PangoContext* context = gtk_widget_get_pango_context(gweled_ui->g_clutter);
    desc = pango_context_get_font_description (context);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    pango_font_description_set_absolute_size(desc, board_width * text_data->relative_font_size);

    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text_data->string, -1);
    pango_layout_set_font_description(layout, desc);

    // Text centered and layout at full width.
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_width (layout, width * PANGO_SCALE);

    // Center vertically the text.
    pango_layout_get_size(layout, &lw, &lh);
    cairo_move_to(cr, 0, -((double)lh / PANGO_SCALE) / 2.0);

    pango_cairo_layout_path(cr, layout);

    // External text border
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgb(cr, COLOR_RED (text_data->outline_color) / 255.0,
                             COLOR_GREEN (text_data->outline_color) / 255.0,
                             COLOR_BLUE (text_data->outline_color) / 255.0);
    cairo_set_line_width(cr, board_width / 120.0);
    cairo_stroke_preserve(cr);

    // Text internal color
    cairo_set_source_rgb(cr, COLOR_RED (text_data->text_color) / 255.0,
                             COLOR_GREEN (text_data->text_color) / 255.0,
                             COLOR_BLUE (text_data->text_color) / 255.0);
    cairo_fill(cr);

    // Free resources
    g_object_unref(layout);

    return TRUE;
}


T_SGEObject *
sge_create_score_text_object (gint x, gint y, T_SGELayer layer, T_SGETextData *text_data)
{
    T_SGEObject * object;
	object = (T_SGEObject *) g_malloc (sizeof (T_SGEObject));
    object->x = x;
    object->y = y;
    object->pixbuf_id = 0;

    object->animating = FALSE;

    object->y_delay = 0;

    object->effect_handler_id = 0;

	object->layer = layer;

    object->width = prefs.tile_size * 2;
    object->height = prefs.tile_size;

    // Text features
    ClutterContent * canvas;

    object->text_data = text_data;

    // Crea un actor per il testo
    canvas = clutter_canvas_new();
    g_signal_connect(canvas, "draw", G_CALLBACK(generic_text_canvas_draw), (gpointer) object->text_data);

    // Text size: width -> two times the gem size, height -> gem size
    clutter_canvas_set_size (CLUTTER_CANVAS(canvas), object->width, object->height);

    object->actor = clutter_actor_new();
    clutter_actor_set_content (object->actor, canvas);

    clutter_actor_set_size (CLUTTER_ACTOR(object->actor),
                            object->width,
                            object->height);

    clutter_actor_set_position (CLUTTER_ACTOR (object->actor),
                                x,
                                y);

    clutter_actor_set_pivot_point (object->actor, 0.5, 0.5);

    clutter_actor_add_child (g_actor_layers[layer],
                             CLUTTER_ACTOR (object->actor));

    clutter_actor_show (object->actor);

    g_object_list = g_list_append (g_object_list, (gpointer) object);

	return object;
}


T_SGEObject *
sge_create_fullscreen_text_object (T_SGELayer layer, T_SGETextData *text_data)
{
    T_SGEObject * object;
	object = (T_SGEObject *) g_malloc (sizeof (T_SGEObject));
    object->x = 0;
    object->y = 0;
    object->pixbuf_id = 0;

    object->animating = FALSE;

    object->y_delay = 0;

    object->effect_handler_id = 0;

	object->layer = layer;

    object->width = BOARD_WIDTH * prefs.tile_size;
    object->height = BOARD_HEIGHT * prefs.tile_size;

    // Text features
    ClutterContent * canvas;

    object->text_data = text_data;

    // Crea un actor per il testo
    canvas = clutter_canvas_new();
    g_signal_connect(canvas, "draw", G_CALLBACK(generic_text_canvas_draw), (gpointer) object->text_data);
    clutter_canvas_set_size (CLUTTER_CANVAS(canvas), object->width, object->width);

    object->actor = clutter_actor_new();
    clutter_actor_set_content (object->actor, canvas);

    clutter_actor_set_size (CLUTTER_ACTOR(object->actor),
                            object->width,
                            object->height);

    clutter_actor_set_pivot_point (object->actor, 0.5, 0.5);

    clutter_actor_add_child (g_actor_layers[layer],
                             CLUTTER_ACTOR (object->actor));

    clutter_actor_show (object->actor);

    g_object_list = g_list_append (g_object_list, (gpointer) object);

	return object;
}

void
sge_destroy (void)
{
	int i;

    sge_destroy_all_objects();

    clutter_actor_destroy_all_children (g_gameboard);
    clutter_actor_destroy (g_gameboard);

	for (i = 0; i < gi_nb_pixbufs; i++)
		g_object_unref (g_pixbufs[i]);
	g_free (g_pixbufs);

}

// creation/destruction
void
sge_init (void)
{
	int i;

	gi_nb_pixbufs = 0;
	g_pixbufs = NULL;

    // Create a parent actor that's autoalign on the stage
    g_gameboard = clutter_actor_new();
    clutter_actor_set_size (g_gameboard,
                            BOARD_WIDTH * prefs.tile_size,
                            BOARD_HEIGHT * prefs.tile_size);
    clutter_actor_add_constraint (g_gameboard, clutter_align_constraint_new (gweled_ui->g_stage, CLUTTER_ALIGN_BOTH, 0.5));
    clutter_actor_add_child (gweled_ui->g_stage, g_gameboard);
    clutter_actor_show (g_gameboard);

    clutter_actor_set_reactive (g_gameboard, TRUE);
    g_signal_connect (g_gameboard, "button-press-event",
                      G_CALLBACK (board_input_event),
                      NULL);
    g_signal_connect (g_gameboard, "button-release-event",
                      G_CALLBACK (board_input_event),
                      NULL);
    g_signal_connect (g_gameboard, "touch-event",
                      G_CALLBACK (board_input_event),
                      NULL);

	// Layers create.
	for (i = 0; i < 5; i++) {
	    g_actor_layers[i] = clutter_actor_new();
	    
	    clutter_actor_set_easing_mode(g_actor_layers[i], CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (g_actor_layers[i], 200);
        clutter_actor_set_position(g_actor_layers[i], 0, 0);
        clutter_actor_set_size (g_actor_layers[i],
                                BOARD_WIDTH * prefs.tile_size,
                                BOARD_HEIGHT * prefs.tile_size);
                                
        clutter_actor_set_clip(g_actor_layers[i], 0, 0, BOARD_WIDTH * prefs.tile_size, BOARD_HEIGHT * prefs.tile_size);
        
        clutter_actor_add_child (g_gameboard, g_actor_layers[i]);

        clutter_actor_show (g_actor_layers[i]);
	}
}
