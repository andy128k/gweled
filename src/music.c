

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
	    usleep(10000);
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

