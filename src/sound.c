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
#include <pthread.h>
#include <glib.h>

#include "sound.h"
#include "board_engine.h"

static pthread_t thread;

static gboolean is_playing;
static gboolean sound_available;
static ca_context *context = NULL;

void sound_init(GdkScreen *screen)
{
    if(sound_available == TRUE)
	return;
    
    if (screen == NULL)
	screen = gdk_screen_get_default();

    if(context = NULL)
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
    ca_proplist *p = NULL;
    ca_proplist_create(&p);
    ca_gtk_proplist_set_for_widget (p, game_window);


    char sound_name[] = "autonom.ogg";
    char path[] = DATADIR"/sounds/gweled/autonom.ogg";
    int play_status;

    play_status = ca_gtk_play_for_widget (game_window, 0, CA_PROP_MEDIA_NAME, 						sound_name, CA_PROP_MEDIA_FILENAME, path,
					NULL);

    g_print("libcanberra playing sound %s [file %s]: %s\n", sound_name, path, 			ca_strerror (play_status));
}

/* Stop playing music*/ 
void sound_music_stop()
{
	ca_context_cancel(context, 0);
}

// play sound fx
/*void sound_play_sample(gweled_sound_samples sample)
{
    if (sound_available == FALSE)
        return;

    switch (sample) {
        case CLICK_EVENT:
            if(click_sfx)
                Sample_Play(click_sfx, 0, 0);
            break;
        case SWAP_EVENT:
            if(swap_sfx)
                Sample_Play(swap_sfx, 0, 0);
            break;
    }
}

void sound_destroy()
{
    if (sound_available) {
        sound_music_stop();

        pthread_cancel(thread);
        pthread_join(thread, NULL);

        sound_unload_samples();

        MikMod_DisableOutput();
	    MikMod_Exit();

	    sound_available = FALSE;
	}
}*/

gboolean sound_get_enabled()
{
    return sound_available;
}
