#include <gnome.h>
#include <librsvg/rsvg.h>

#include "sge_utils.h"

GdkPixbuf *
sge_load_svg_to_pixbuf (GnomeProgram * program, char *filename, int width,
			int height)
{
	gchar *full_pathname;
	GdkPixbuf *pixbuf = NULL;
	GError *error;

	full_pathname = gnome_program_locate_file (program,
						   GNOME_FILE_DOMAIN_APP_PIXMAP,
						   filename, TRUE, NULL);
	if (full_pathname) {
		pixbuf =
		    rsvg_pixbuf_from_file_at_size (full_pathname, width,
						   height, &error);
		g_free (full_pathname);
		if (pixbuf == NULL) {
			g_free (error);
		}
	} else
		printf ("%s not found\n", filename);

	return pixbuf;
}

GdkPixbuf *
sge_load_file_to_pixbuf (GnomeProgram * program, char *filename)
{
	gchar *full_pathname;
	GdkPixbuf *pixbuf = NULL;

	full_pathname = gnome_program_locate_file (program,
						   GNOME_FILE_DOMAIN_APP_PIXMAP,
						   filename, TRUE, NULL);
	if (full_pathname) {
		pixbuf = gdk_pixbuf_new_from_file (full_pathname, NULL);
		g_free (full_pathname);
	} else
		printf ("%s not found\n", filename);

	return pixbuf;
}
