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

#include <mikmod.h>
#include <pthread.h>
#include <glib.h>

#include "music.h"
#include "board_engine.h"

extern GweledPrefs prefs;
extern pthread_t thread;

static MODULE *module;
static gboolean is_playing;

void music_init()
{
    /* register all the drivers */
    MikMod_RegisterDriver(&drv_AF);
    MikMod_RegisterDriver(&drv_esd);
    MikMod_RegisterDriver(&drv_alsa);
    MikMod_RegisterDriver(&drv_oss);
    MikMod_RegisterDriver(&drv_nos);

    /* register all the module loaders */
    MikMod_RegisterAllLoaders();

    /* initialize the library */
    if (MikMod_Init(""))
	{
        fprintf(stderr, "Could not initialize sound, reason: %s\n", MikMod_strerror(MikMod_errno));
        //return; don't fail on sound problems
    }
    is_playing = FALSE;

}

void music_thread(void *ptr)
{
	while (1) {
	    g_usleep(10000);
		MikMod_Update();
    }
}

void music_play()
{
	if (!music_isplaying())
	{
	    // load module
	    module = Player_Load(DATADIR "/sounds/gweled/autonom.s3m", 64, 0);
    	if (module) {
    	    Player_Start(module);
			Player_SetVolume(64);
			pthread_create(&thread, NULL, (void *)&music_thread, NULL);
			prefs.music_on = TRUE;
			is_playing = TRUE;

    	} else
    	    fprintf(stderr, "Could not load module, reason: %s\n", MikMod_strerror(MikMod_errno));
	}
}

void music_stop()
{
	if (music_isplaying()){
    	if (module)
		{
	    	Player_Stop();
	    	Player_Free(module);
			pthread_cancel(thread);
			pthread_join(thread, NULL);
	    	prefs.music_on = FALSE;
	    	is_playing = FALSE;
		}
	}

}

gboolean music_isplaying()
{
	return is_playing;
}

