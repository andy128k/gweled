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

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "sge_utils.h"

#define GWELED_PIXMAPS_BASEPATH DATA_DIRECTORY G_DIR_SEPARATOR_S "pixmaps" G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S

GdkPixbuf *
sge_load_svg_to_pixbuf (char *filename, int width, int height)
{
	char *full_pathname;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;

	full_pathname = g_strconcat(GWELED_PIXMAPS_BASEPATH, filename, NULL);
	if (g_file_test(full_pathname, G_FILE_TEST_IS_REGULAR)) {
		pixbuf = gdk_pixbuf_new_from_file_at_size (full_pathname, width,
						   height, &error);
		if (pixbuf == NULL)
			g_error_free (error);

	} else
		g_warning ("%s not found", filename);

	g_free (full_pathname);

	return pixbuf;
}

GdkPixbuf *
sge_load_file_to_pixbuf (char *filename)
{
	gchar *full_pathname;
	GdkPixbuf *pixbuf = NULL;

	full_pathname = g_strconcat(GWELED_PIXMAPS_BASEPATH, filename, NULL);
	if (g_file_test(full_pathname, G_FILE_TEST_IS_REGULAR)) {
		pixbuf = gdk_pixbuf_new_from_file (full_pathname, NULL);
		g_free (full_pathname);
	} else
		g_warning ("%s not found\n", filename);

	return pixbuf;
}
