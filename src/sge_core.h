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

#ifndef _SGE_CORE_H_
#define _SGE_CORE_H_

#include <gtk/gtk.h>
#include <clutter/clutter.h>

typedef enum e_sge_layers
{
    BOARD_LAYER,
    GEMS_LAYER,
    EFFECTS_LAYER,
    TEXT_LAYER
} T_SGELayer;

typedef enum e_sge_effect
{
    NONE,
    BLINK,
    BOUNCE,
} T_SGEEffect;

typedef uint32_t T_SGEColor;

typedef struct s_sge_text_data
{
  const gchar *string;
  guint relative_font_size;
  T_SGEColor text_color;
  T_SGEColor outline_color;
} T_SGETextData;


#define COLOR_CREATE(r,g,b)	(((r)<<16) | ((g)<<8) | (b))
#define COLOR_RED(col)		(((col)>>16) & 0xff)
#define COLOR_GREEN(col)	(((col)>>8) & 0xff)
#define COLOR_BLUE(col)		((col)&0xff)

typedef struct s_sge_object
{
	gint x;
	gint y;
	gint y_delay;

	gint width;
	gint height;
	T_SGELayer layer;

  gboolean animating;
  gulong animating_handler_id;

  T_SGEEffect effect;
  guint effect_handler_id;
  gboolean effect_status;

	gint pixbuf_id;

	T_SGETextData *text_data;
	ClutterActor *actor;
	
} T_SGEObject;

#define SGE_OBJECT(obj)  ((T_SGEObject *) obj)

void sge_init(void);
void sge_destroy(void);

gint sge_register_pixbuf(GdkPixbuf *pixbuf, int index);

GdkPixbuf *
sge_get_pixbuf(gint);

void
sge_objects_resize (gint size);

T_SGEObject *
sge_create_object (gint x, gint y, T_SGELayer layer, gint pixbuf_id);

void sge_destroy_object(gpointer object, gpointer user_data);
void sge_destroy_all_objects_on_level(T_SGELayer level);
void sge_destroy_all_objects(void);

void sge_object_move_to(T_SGEObject *object, gint dest_x, gint dest_y);
void sge_object_fall_to (T_SGEObject * object, gint y_pos, gint delay, gint speed);
void sge_object_fall_to_with_effect (T_SGEObject * object, gint y_pos, gint delay);

gboolean sge_object_is_moving(T_SGEObject *object);
gboolean sge_objects_are_moving_on_layer(T_SGELayer layer);

void sge_set_layer_visibility (T_SGELayer layer, gboolean visibility);
void sge_set_layer_opacity (T_SGELayer layer, guint8 opacity);

void sge_object_fadeout (T_SGEObject *object, guint delay_secs, guint duration);
void sge_object_zoomout (T_SGEObject *object);
void sge_gem_destroy (T_SGEObject *object);

void
sge_object_fly_away (T_SGEObject *object);

void
sge_object_zoomin (T_SGEObject *object, guint msecs, ClutterAnimationMode mode);

void sge_object_blink_start (T_SGEObject *object);
void sge_object_blink_stop (T_SGEObject *object);

void sge_object_bounce (T_SGEObject *object);

void sge_object_reset_effects (T_SGEObject *object);

gboolean sge_object_exists (T_SGEObject *object);

T_SGEObject *
sge_create_fullscreen_text_object (T_SGELayer layer, T_SGETextData *text_data);

T_SGEObject *
sge_create_score_text_object (gint x, gint y, T_SGELayer layer, T_SGETextData *text_data);

#endif
