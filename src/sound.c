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

#include <canberra-gtk.h>
#include <glib.h>

#include "sound.h"
#include "board_engine.h"

/* The time interval in milliseconds between music play iterations */
#define MS_BETWEEN_PLAY 500

static gboolean is_playing;
static gboolean sound_available;
static ca_context *context = NULL;

/* Structure for repeatable sounds */
typedef struct {
  GtkWidget *widget;
  guint play_interval;
} GweledRepeatableSound;


void sound_init(GdkScreen *screen)
{
    if(sound_available == TRUE)
	return;
    
    if (screen == NULL)
	screen = gdk_screen_get_default();

    if(context == NULL)
    context = ca_gtk_context_get_for_screen(screen);

    g_print("Initializing canberra-gtk context for display %s on screen %d\n", 
	gdk_display_get_name(gdk_screen_get_display(screen)),
	gdk_screen_get_number(screen));

    if (!context){
	sound_available = FALSE;
	return;
	}

    sound_available = TRUE;
}


void sound_music_play(GtkWidget *game_window)
{
    /* declare necessary variables */
    GweledRepeatableSound *repeatable_sound;
    char sound_name[] = "autonom.ogg";
    char path[] = DATADIR"/sounds/gweled/autonom.ogg";
    int play_status;
    ca_proplist *music_proplist;
    
    /* allocate memory and fill the repeatable_sound */
    repeatable_sound = g_slice_new0(GweledRepeatableSound);
    repeatable_sound->widget = game_window;
    repeatable_sound->play_interval = MS_BETWEEN_PLAY;

    /* create the property list */
    ca_proplist_create (&music_proplist);

    /* fill the property list */
    ca_gtk_proplist_set_for_widget(music_proplist, game_window);
    ca_proplist_sets(music_proplist, CA_PROP_MEDIA_NAME, sound_name);
    ca_proplist_sets(music_proplist, CA_PROP_MEDIA_FILENAME, path);
    ca_proplist_sets(music_proplist, CA_PROP_MEDIA_ROLE, "game");

    /* play the music and get the status */
    play_status = ca_context_play_full(context, 0, music_proplist, music_finished_playing_cb, repeatable_sound);

    /* print the status of the playback to the terminal */
    g_print("libcanberra playing sound %s [file %s]: %s\n", sound_name, path, 			ca_strerror (play_status));
}

/* Stop playing music*/ 
void sound_music_stop()
{
	ca_context_cancel(context, 0);
}

/* Callback when music finished playing */
static void music_finished_playing_cb(ca_context *c, uint32_t id, int errno, gpointer user_data){

    GweledRepeatableSound *repeatable_sound = user_data;

    /* Check to see if the playback was successful */
    if(errno != CA_SUCCESS){
    g_print("Error:%s", ca_strerror(errno));
    return;
    }

    /* set up loop to play music */
    g_timeout_add(repeatable_sound->play_interval, playing_timeout_cb, 			user_data);	
}

/* Callback to replay music */
static gboolean playing_timeout_cb(gpointer data)
{
    GweledRepeatableSound *repeatable_sound = data;

    sound_music_play(repeatable_sound->widget);

}


/* Play sound fx */
void sound_effect_play(gweled_sound_effects effect)
{
    char click_name[] = "click.ogg";
    char swap_name[] = "swap.ogg";
    char click_path[] = DATADIR"/sounds/gweled/click.ogg";
    char swap_path[] = DATADIR"/sounds/gweled/swap.ogg";
    int play_status;

    if (sound_available == FALSE)
        return;

    switch (effect) {
        case CLICK_EVENT:
            play_status = ca_gtk_play_for_event(gtk_get_current_event(), 0,
			CA_PROP_MEDIA_FILENAME, click_name,
			CA_PROP_MEDIA_FILENAME, click_path,
			CA_PROP_EVENT_ID, "click-event",
			CA_PROP_EVENT_DESCRIPTION, "click event happened",
			NULL);
            g_print("libcanberra playing sound %s [file: %s]; %s\n", click_name, 			click_path, ca_strerror(play_status));
            break;
        case SWAP_EVENT:
            play_status = ca_context_play(context, 0,
			CA_PROP_MEDIA_FILENAME, swap_name,
			CA_PROP_MEDIA_FILENAME, swap_path,
			CA_PROP_EVENT_ID, "swap-event",
			CA_PROP_EVENT_DESCRIPTION, "swap event happened",
			NULL);
            g_print("libcanberra playing sound %s [file: %s]; %s\n", swap_name, 			swap_path, ca_strerror(play_status));
            break;
    }
}

void sound_destroy()
{
    if (sound_available) {
        sound_music_stop();
	}

	ca_context_destroy(context);
	
        sound_available = FALSE;
}

gboolean sound_get_enabled()
{
    return sound_available;
}
