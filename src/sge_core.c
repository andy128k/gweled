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

#include "board_engine.h"
#include "graphic_engine.h"
#include "sound.h"
#include "sge_core.h"

#define ACCELERATION	1.0
#define SGE_TIMER_INTERVAL  20

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

extern GtkWidget *g_clutter;
extern ClutterActor *g_stage;

extern GweledPrefs prefs;

static gint gi_dragging = 0;

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
sge_destroy_all_objects_on_level (int level)
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
       sge_object_fadeout(SGE_OBJECT(object));
}

// animations/effects

// used for gems swapping
void
sge_object_move_to (T_SGEObject * object, gint dest_x, gint dest_y)
{
  object->x = dest_x;
  object->y = dest_y;
  
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
sge_objects_are_moving_on_layer (int layer)
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

void sge_set_layer_visibility (int layer, gboolean visibility)
{
    if (visibility) {
        clutter_actor_set_opacity(g_actor_layers[layer], 255);
    } else {
        clutter_actor_set_opacity(g_actor_layers[layer], 0);
    }
}


// fadeout the object and then destroy it
void sge_object_fadeout (T_SGEObject *object)
{
    clutter_actor_save_easing_state (object->actor);
    clutter_actor_set_easing_mode(object->actor, CLUTTER_LINEAR);
    clutter_actor_set_easing_duration (object->actor, 200);
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
    clutter_actor_set_pivot_point (object->actor, 0.5, 0.5);
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

void sge_object_blink_start (T_SGEObject *object)
{    
    object->blink = TRUE;
    clutter_timeline_start(timeline);
}

void sge_object_blink_stop (T_SGEObject *object)
{
    object->blink = FALSE;
    clutter_timeline_pause(timeline);
    clutter_actor_set_opacity (object->actor, 255);
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
on_gem_clicked (ClutterClickAction    *action,
                ClutterActor          *actor,
                gpointer               user_data)
{
    gfloat x, y;
     
     
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
    if (clutter_click_action_get_button (action) != 1)
        return FALSE;
    
	 
    clutter_actor_get_position (actor, &x, &y);
	 
	 
	gi_x_click = round(x) / prefs.tile_size;
    gi_y_click = round(y) / prefs.tile_size;
     
    g_print("Clicked! %i:%i %s btn:%i\n", gi_x_click, gi_y_click, clutter_actor_get_name(actor), clutter_click_action_get_button (action));
     
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
    
    for (i = 0; i < g_list_length (g_object_list); i++) {
    
		object = (T_SGEObject *) g_list_nth_data (g_object_list, i);
		gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (object->actor),
                                         GDK_PIXBUF(g_pixbufs[object->pixbuf_id]), &error);
		clutter_actor_set_size (CLUTTER_ACTOR(object->actor),
                            size,
                            size);
                            
        clutter_actor_set_position (CLUTTER_ACTOR (object->actor),
                                    object->x * size,
                                    object->y * size);
    }
    
    // Resize levels.
    for (i = 0; i < 5; i++) {
	 
        clutter_actor_set_size (g_actor_layers[i],
                                BOARD_WIDTH * size,
                                BOARD_HEIGHT * size);
                                
        clutter_actor_set_clip(g_actor_layers[i], 0, 0, BOARD_WIDTH * size, BOARD_HEIGHT * size);
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
	}

}


//objects creation/destruction

T_SGEObject *
sge_create_object (gint x, gint y, gint layer, gint pixbuf_id)
{

    GError *error;
    ClutterAction *action;
    
    g_print("sge_create_object %i at %i:%i layer:%i\n", pixbuf_id, x, y, layer);
    g_print("sge_create_object %i at %i:%i layer:%i\n", pixbuf_id, x * prefs.tile_size, y * prefs.tile_size, layer);
    
    T_SGEObject * object;
	object = (T_SGEObject *) g_malloc (sizeof (T_SGEObject));
    object->x = x;
    object->y = y;
    object->pixbuf_id = pixbuf_id;

    object->y_delay = 0;

    object->blink = FALSE;

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
                                x * prefs.tile_size,
                                y * prefs.tile_size);
    
    clutter_actor_add_child (g_actor_layers[layer],
                                 CLUTTER_ACTOR (object->actor));
                                 
    clutter_actor_show (object->actor);                            
        

    // Gems clickabe.
    if (layer == 1) {
        clutter_actor_set_reactive (object->actor, TRUE);
        
        action = clutter_click_action_new();
        clutter_actor_add_action (object->actor, action);
        
        g_signal_connect (action, "clicked", G_CALLBACK (on_gem_clicked), NULL);
	}
	

    g_object_list = g_list_append (g_object_list, (gpointer) object);

	return object;
}

// Temporary, for text
T_SGEObject *
sge_create_object_simple (gint x, gint y, gint layer, gint pixbuf_id)
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
	
	// Layers create.
	for (i = 0; i < 5; i++) {
	    g_actor_layers[i] = clutter_actor_new();
	    
	    clutter_actor_set_name(g_actor_layers[i], g_strdup_printf ("Level %d\n", i));
	    clutter_actor_set_easing_mode(g_actor_layers[i], CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (g_actor_layers[i], 200);
        clutter_actor_show (g_actor_layers[i]);
        
        clutter_actor_set_position(g_actor_layers[i], 0, 0);
        clutter_actor_set_size (g_actor_layers[i],
                                BOARD_WIDTH * prefs.tile_size,
                                BOARD_HEIGHT * prefs.tile_size);
                                
        clutter_actor_set_clip(g_actor_layers[i], 0, 0, BOARD_WIDTH * prefs.tile_size, BOARD_HEIGHT * prefs.tile_size);
        
        clutter_actor_add_child (g_stage, g_actor_layers[i]);
        clutter_actor_add_constraint (g_actor_layers[i], clutter_align_constraint_new (g_stage, CLUTTER_ALIGN_BOTH, 0));
	}
    
    
    /* Create a timeline to manage animation */
    timeline = clutter_timeline_new (200);
    clutter_timeline_set_repeat_count (timeline, -1);

    /* fire a callback for frame change */
    g_signal_connect (timeline, "new-frame",  G_CALLBACK (sge_clutter_frame_cb), NULL);
    
    
    clutter_timeline_pause(timeline);

    /* and start it */
    clutter_timeline_start (timeline);
	
}
