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

#ifndef _SOUND_H_
#define _SOUND_H_
#include <glib.h>

typedef enum e_gweled_sound_effects
{
    CLICK_EVENT,
    SWAP_EVENT
} gweled_sound_effects;

void sound_init(GdkScreen *);
void sound_music_play(GtkWidget *);
void sound_music_stop(void);
void sound_effect_play(gweled_sound_effects);
void sound_destroy(void);
gboolean sound_get_enabled();

#endif
