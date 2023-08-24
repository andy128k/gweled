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

static gint gi_dragging = 0;

ClutterActor *g_gameboard = NULL;

ClutterActor *g_actor_layers[5] = {NULL, NULL, NULL, NULL, NULL};

ClutterTimeline *timeline;


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
	    clutter_actor_destroy (SGE_OBJECT(object)->actor);
        SGE_OBJECT(object)->actor = NULL;
	}
	g_object_list = g_list_remove (g_object_list, object);
	g_free(object);
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
                         
    sge_destroy_object (SGE_OBJECT(object), NULL);
}

void
sge_fadeout_on_transition_ended (ClutterActor *actor,
                                 char         *name,
                                 gboolean      is_finished,
                                 gpointer      object) {
       sge_object_fadeout(SGE_OBJECT(object), 0);
}

// animations/effects

// used for gems swapping
void
sge_object_move_to (T_SGEObject * object, gint dest_x, gint dest_y)
{
  object->x = dest_x;
  object->y = dest_y;

  sge_object_reset_effects (object);
  
  clutter_actor_save_easing_state (object->actor);
  clutter_actor_set_easing_mode(object->actor, CLUTTER_EASE_IN_OUT_CUBIC);
  clutter_actor_set_easing_duration (object->actor, 200);
  clutter_actor_set_position (object->actor, dest_x * prefs.tile_size, dest_y * prefs.tile_size);
  clutter_actor_restore_easing_state (object->actor);
}

gboolean
sge_object_expired (gpointer data)
{
    sge_destroy_object(SGE_OBJECT(data), NULL);
    return FALSE;
}

void
sge_object_set_lifetime (T_SGEObject * object, gint seconds)
{
    g_timeout_add_seconds (seconds, sge_object_expired, object);
}
void

sge_object_fall_to (T_SGEObject * object, gint y_pos)
{
  sge_object_fall_to_with_delay (object, y_pos, 0);
}
void
sge_object_fall_to_with_delay (T_SGEObject * object, gint y_pos, gint delay)
{
  
  object->y = y_pos;
  
  clutter_actor_save_easing_state (object->actor);
  clutter_actor_set_easing_mode(object->actor, CLUTTER_EASE_OUT_ELASTIC);
  clutter_actor_set_easing_duration (object->actor, 500);
  clutter_actor_set_easing_delay (object->actor, delay);
  clutter_actor_set_position (object->actor,
                              clutter_actor_get_x (object->actor),
                              y_pos * prefs.tile_size);
  clutter_actor_restore_easing_state (object->actor);
}

//other useful stuff

gboolean
sge_object_is_moving (T_SGEObject * object)
{
	gfloat cx, cy;
	clutter_actor_get_position(object->actor, &cx, &cy);
	return ((object->x * prefs.tile_size != cx) || (object->y * prefs.tile_size != cy));
}

gboolean
sge_objects_are_moving (void)
{
	gint i;
	T_SGEObject *object;

	for (i = 0; i < g_list_length (g_object_list); i++) {
		object =
		    (T_SGEObject *) g_list_nth_data (g_object_list, i);
		if (sge_object_is_moving (object))
			return TRUE;
	}
	return FALSE;
}

gboolean
sge_objects_are_moving_on_layer (T_SGELayer layer)
{
	gint i;
	T_SGEObject *object;

	for (i = 0; i < g_list_length (g_object_list); i++) {
		object =
		    (T_SGEObject *) g_list_nth_data (g_object_list, i);
		if (object->layer == layer)
			if (sge_object_is_moving (object))
				return TRUE;
	}
	return FALSE;
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
void sge_object_fadeout (T_SGEObject *object, guint delay_secs)
{
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode(object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration (object->actor, 200);
    clutter_actor_set_easing_delay(object->actor, delay_secs * 1000);
    clutter_actor_set_opacity (object->actor, 0);
    clutter_actor_restore_easing_state (object->actor);
    
    g_signal_connect (object->actor, "transition-stopped",
		    G_CALLBACK (sge_destroy_on_transition_ended), 
		    object);
    
}

// zoomout the object and then destroy it
void sge_object_zoomout (T_SGEObject *object)
{
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration (object->actor, 200);
    clutter_actor_set_scale  (object->actor, 0, 0);
    clutter_actor_restore_easing_state (object->actor);
    
    g_signal_connect (object->actor, "transition-stopped",
		    G_CALLBACK (sge_destroy_on_transition_ended), 
		    object);
}

void
sge_object_fly_away (T_SGEObject *object)
{    
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration (object->actor, 1000);
    clutter_actor_set_position (object->actor,
                                clutter_actor_get_x (object->actor),
                                clutter_actor_get_y (object->actor) - (prefs.tile_size/2)
                                );
    clutter_actor_restore_easing_state (object->actor);
    
    // After 800ms start to fade away.
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_delay(object->actor, 800);
    clutter_actor_set_easing_duration (object->actor, 200);
    clutter_actor_set_opacity(object->actor, 0);
    clutter_actor_restore_easing_state (object->actor);
    
    g_signal_connect (object->actor, "transition-stopped",
		    G_CALLBACK (sge_destroy_on_transition_ended), 
		    object);
}

void sge_object_reset_effects (T_SGEObject *object)
{
    if (object->actor == NULL) return;

    clutter_actor_remove_all_transitions (object->actor);

    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode (object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration(object->actor, COGL_PIXEL_FORMAT_ARGB_2101010);
    clutter_actor_set_opacity(object->actor, 255);
    clutter_actor_set_position (object->actor, object->x * prefs.tile_size, object->y * prefs.tile_size);
    clutter_actor_set_size (object->actor, prefs.tile_size, prefs.tile_size);
    clutter_actor_restore_easing_state (object->actor);

    clutter_timeline_pause(timeline);
}

void sge_object_blink_start (T_SGEObject *object)
{    
    object->blink = TRUE;
    clutter_timeline_start(timeline);
}

void sge_object_blink_stop (T_SGEObject *object)
{
    object->blink = FALSE;
    clutter_timeline_pause(timeline);
    sge_object_reset_effects(object);
}

gboolean
sge_object_stop_bounce (gpointer data)
{
    T_SGEObject *object = SGE_OBJECT(data);

    object->bounce = FALSE;
    sge_object_reset_effects(object);

    return FALSE;
}

void sge_object_bounce (T_SGEObject *object)
{
    object->bounce = TRUE;
    clutter_timeline_start(timeline);
    g_timeout_add_seconds (2, sge_object_stop_bounce, object);
}

void sge_object_animate (T_SGEObject *object, gboolean repeat)
{
    object->animation = TRUE;
    object->animation_repeat = repeat;
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
on_gem_clicked (ClutterActor *stage,
                  ClutterEvent *event,
                  gpointer      dummy G_GNUC_UNUSED)
{
    gfloat x, y;
    g_print("on_gem_clicked\n");
     
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
    if (clutter_event_get_button (event) != 1)
        return FALSE;
    
    // Can't use the actor position due to possible transformations.
    clutter_event_get_coords  (event, &x, &y);
	 
	gi_x_click = round(x) / prefs.tile_size;
    gi_y_click = round(y) / prefs.tile_size;
     
    g_print("Clicked! %i:%i [%.2lfx%.2lf] btn:%i\n", gi_x_click, gi_y_click, x, y, clutter_event_get_button (event));
     
    gi_gem_clicked = -1;
    gi_dragging = -1;
	 
	respawn_board_engine_loop();
	 
	 
	return FALSE;
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
    
		object = (T_SGEObject *) g_list_nth_data (g_object_list, i);

        if (CLUTTER_IS_TEXT(object->actor)) continue;

		gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (object->actor),
                                         GDK_PIXBUF(g_pixbufs[object->pixbuf_id]), &error);
		clutter_actor_set_size (object->actor, size, size);
                            
        clutter_actor_set_position (object->actor,
                                    object->x * size,
                                    object->y * size);
    }
    
}



/* Timeline handler */
static void
sge_clutter_frame_cb (ClutterTimeline *timeline, 
	  gint             msecs,
	  gpointer         data)
{
    gint            i;
    guint           progress = clutter_timeline_get_progress (timeline) * 360.0f;
    static gboolean fade = TRUE;
    static gboolean bounce_anim = TRUE;
    T_SGEObject *object;

    if (progress < 360)
        return;

    for (i = 0; i < g_list_length (g_object_list); i++) {
		object = (T_SGEObject *) g_list_nth_data (g_object_list, i);
		if (object->blink) {
		    clutter_actor_save_easing_state (object->actor);
            clutter_actor_set_easing_mode(object->actor, CLUTTER_LINEAR);
            clutter_actor_set_easing_duration (object->actor, 100);
            if (fade) {
                clutter_actor_set_opacity (object->actor, 155);
                fade = FALSE;
            }
            else {
                clutter_actor_set_opacity (object->actor, 255);
                fade = TRUE;
            }
            clutter_actor_restore_easing_state (object->actor);
        }

        if (object->bounce) {
		    clutter_actor_save_easing_state (object->actor);
            clutter_actor_set_easing_mode(object->actor, CLUTTER_EASE_IN_OUT_QUAD);
            if (bounce_anim) {
                clutter_actor_set_easing_duration (object->actor, 500);
                clutter_actor_set_position  (object->actor,
                                            object->x * prefs.tile_size - (prefs.tile_size * 0.05),
                                            object->y * prefs.tile_size + (prefs.tile_size * 0.2));
                clutter_actor_set_size (object->actor, prefs.tile_size * 1.1, prefs.tile_size * 0.9);

                bounce_anim = FALSE;
            }
            else {
                clutter_actor_set_easing_duration (object->actor, 250);
                clutter_actor_set_position (object->actor, object->x * prefs.tile_size, object->y * prefs.tile_size);
                clutter_actor_set_size (object->actor, prefs.tile_size, prefs.tile_size);

                bounce_anim = TRUE;
            }
            clutter_actor_restore_easing_state (object->actor);
        }
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

    object->blink = FALSE;

    object->bounce = FALSE;

    object->animation = FALSE;
    object->animation_iter = 1;
    object->animation_repeat = TRUE;

	object->layer = layer;

	object->width = gdk_pixbuf_get_width (g_pixbufs[pixbuf_id]);
    object->height = gdk_pixbuf_get_height (g_pixbufs[pixbuf_id]);
    
   
    // Clutter actors.
    object->actor = gtk_clutter_texture_new ();
    gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (object->actor),
                                         GDK_PIXBUF(g_pixbufs[pixbuf_id]), &error);

    clutter_actor_set_name(object->actor, g_strdup_printf ("pixbuf #%d", pixbuf_id));
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

// Temporary, for text
T_SGEObject *
sge_create_object_simple (gint x, gint y, T_SGELayer layer, gint pixbuf_id)
{
    GError *error;
    
    g_print("sge_create_object_simple %i at %i:%i layer:%i\n", pixbuf_id, x, y, layer);
    
    T_SGEObject * object;
	object = (T_SGEObject *) g_malloc (sizeof (T_SGEObject));
    object->x = x;
    object->y = y;
    object->pixbuf_id = pixbuf_id;

    object->y_delay = 0;

    object->blink = FALSE;
    object->bounce = FALSE;

    object->animation = FALSE;
    object->animation_iter = 1;
    object->animation_repeat = TRUE;

	object->layer = layer;

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
                                x,
                                y);
    
    clutter_actor_add_child (g_actor_layers[layer],
                                 CLUTTER_ACTOR (object->actor));
                                 
    clutter_actor_show (object->actor);

    g_object_list = g_list_append (g_object_list, (gpointer) object);

	return object;
}


// Temporary, for text
T_SGEObject *
sge_create_text_object (T_SGELayer layer, const gchar *string, const gchar *color_string)
{

    g_print("sge_create_text_object \"%s\" layer:%i\n", string, layer);

    T_SGEObject * object;
	object = (T_SGEObject *) g_malloc (sizeof (T_SGEObject));
    object->x = 0;
    object->y = 0;
    object->pixbuf_id = 0;

    object->y_delay = 0;

    object->blink = FALSE;
    object->bounce = FALSE;

    object->animation = FALSE;
    object->animation_iter = 1;
    object->animation_repeat = TRUE;

	object->layer = layer;

    // Text features
    PangoContext* context = gtk_widget_get_pango_context(gweled_ui->g_clutter);
    PangoFontDescription *systemFontDesc = pango_context_get_font_description (context);
    gint systemFontSize = pango_font_description_get_size(systemFontDesc); // In pango units (1/1024th of a point)

    PangoFontDescription *font_desc = pango_font_description_copy(systemFontDesc);
    pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
    pango_font_description_set_size(font_desc, systemFontSize * 2);

    // Color handling.
    ClutterColor *c_color = clutter_color_alloc();
    clutter_color_from_string (c_color, color_string);

    // Clutter actor.
    object->actor = clutter_text_new_full(pango_font_description_to_string (font_desc), string, c_color);

    clutter_color_free(c_color);

    // Clutter text alignment and sizing
    clutter_text_set_line_alignment (CLUTTER_TEXT(object->actor), PANGO_ALIGN_CENTER);
    clutter_text_set_line_wrap (CLUTTER_TEXT(object->actor), TRUE);

    clutter_actor_set_width (object->actor, BOARD_WIDTH * prefs.tile_size);
    object->width = clutter_actor_get_width (object->actor);
    object->height = clutter_actor_get_height (object->actor);

    // Horizontally and vertically centerered,
    clutter_actor_add_constraint (object->actor, clutter_align_constraint_new (g_gameboard, CLUTTER_ALIGN_BOTH, 0.5));

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
                      G_CALLBACK (on_gem_clicked),
                      NULL);

	// Layers create.
	for (i = 0; i < 5; i++) {
	    g_actor_layers[i] = clutter_actor_new();
	    
	    clutter_actor_set_name(g_actor_layers[i], g_strdup_printf ("Level %d\n", i));
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

    /* Create a timeline to manage animation */
    timeline = clutter_timeline_new (CLUTTER_TIMELINE_DURATION);
    clutter_timeline_set_repeat_count (timeline, -1);

    /* fire a callback for frame change */
    g_signal_connect (timeline, "new-frame",  G_CALLBACK (sge_clutter_frame_cb), NULL);

    /* and start it */
    clutter_timeline_start (timeline);
}
