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

#include <config.h>
#include <gtk/gtk.h>

#include "sound.h"
#include "board_engine.h"

#define GWELED_SOUND_BASEPATH DATA_DIRECTORY G_DIR_SEPARATOR_S "sounds" G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S

static GtkMediaStream *click, *swap, *explode;

extern GweledPrefs prefs;

void
sound_init(void)
{
    click   = gtk_media_file_new_for_filename (GWELED_SOUND_BASEPATH "click.ogg");
    swap    = gtk_media_file_new_for_filename (GWELED_SOUND_BASEPATH "swap.ogg");
    explode = gtk_media_file_new_for_filename (GWELED_SOUND_BASEPATH "explode.ogg");
}

/* Play sound fx */
void
sound_effect_play(GweledSoundEffects effect)
{
    if (!prefs.sounds_on)
        return;

    switch (effect) {
    case CLICK_EVENT:
        gtk_media_stream_play (click);
        break;
    case SWAP_EVENT:
        gtk_media_stream_play (swap);
        break;
    case EXPLODE_EVENT:
        gtk_media_stream_play (explode);
        break;
    default:
        g_warning("Unknown sound effect %d", effect);
    }
}

