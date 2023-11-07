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

#include <config.h>

#include <gsound.h>

#include "sound.h"
#include "board_engine.h"

#define GWELED_SOUND_BASEPATH DATA_DIRECTORY G_DIR_SEPARATOR_S "sounds" G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S

static gboolean sound_available;
static GSoundContext *sound_ctx = NULL;

static const char* GweledSounds[NUM_SOUND_EFFECTS][2] = {
    {"click", GWELED_SOUND_BASEPATH "click.ogg"},
    {"swap", GWELED_SOUND_BASEPATH "swap.ogg"}
};

extern GweledPrefs prefs;

void
sound_init()
{
    int i;
    GError *error = NULL;


    if (sound_available == TRUE)
	    return;

    if (sound_ctx == NULL) {
        sound_ctx = gsound_context_new (NULL, &error);
        if (error != NULL) goto sound_errors;

        gsound_context_open (sound_ctx, &error);
        if (error != NULL) goto sound_errors;

        for (i = 0; i < NUM_SOUND_EFFECTS; i ++) {
            error = NULL;
            gsound_context_cache(sound_ctx, &error,
                                 GSOUND_ATTR_EVENT_ID,
                                 GweledSounds[i][SOUND_NAME],
                                 GSOUND_ATTR_MEDIA_FILENAME,
                                 GweledSounds[i][SOUND_PATH],
                                 GSOUND_ATTR_MEDIA_ROLE, "game",
                                 NULL);
            if (error != NULL)  {
                g_warning("File [%s] %i: %s\n", GweledSounds[i][SOUND_PATH], error->code, error->message);
                g_error_free (error);
            }
        }
    }

    g_print("Sound init OK\n");
    sound_available = TRUE;
    return;

    sound_errors:
        g_print("Sound error %i: %s\n", error->code, error->message);
        g_error_free (error);
        sound_available = FALSE;
}

/* Play sound fx */
void
sound_effect_play(GweledSoundEffects effect)
{
    GError *error = NULL;

    if (!(prefs.sounds_on && sound_available))
        return;

    gsound_context_play_simple (sound_ctx, NULL, &error,
                                GSOUND_ATTR_EVENT_ID, GweledSounds[effect][SOUND_NAME],
                                GSOUND_ATTR_MEDIA_FILENAME, GweledSounds[effect][SOUND_PATH],
                                GSOUND_ATTR_MEDIA_ROLE, "game",
                                GSOUND_ATTR_CANBERRA_CACHE_CONTROL, "permanent",
                                NULL);

    if (error != NULL)  {
        g_warning("File [%s] %i: %s\n", GweledSounds[effect][SOUND_PATH], error->code, error->message);
        g_error_free (error);
        return;
    }
}

