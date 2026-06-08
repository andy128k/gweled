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

#define GWELED_SOUND_BASEPATH DATA_DIRECTORY G_DIR_SEPARATOR_S "sounds" G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S

/* Play sound fx */
void
sound_effect_play(GweledSoundEffects effect)
{
    const gchar *effect_path;
    switch (effect) {
    case CLICK_EVENT:
        effect_path = GWELED_SOUND_BASEPATH "click.ogg";
        break;
    case SWAP_EVENT:
        effect_path = GWELED_SOUND_BASEPATH "swap.ogg";
        break;
    case EXPLODE_EVENT:
        effect_path = GWELED_SOUND_BASEPATH "explode.ogg";
        break;
    default:
        g_warning("Unknown sound effect %d", effect);
        return;
    }

    GtkMediaStream *stream = gtk_media_file_new_for_filename (effect_path);
    gtk_media_stream_play (stream);
    g_timeout_add_once (1000, g_object_unref, stream);
}
