/* for exit() */
#include <stdlib.h>
/* for strlen() */
#include <string.h>

#include <gtk/gtk.h>
#include "sge_core.h"
#include "sge_utils.h"

#include "board_engine.h"
#include "graphic_engine.h"

extern gint gi_game_running;
extern gchar gpc_game_board[BOARD_WIDTH][BOARD_HEIGHT];
extern GRand *g_random_generator;
extern GdkPixbuf *g_gems_pixbuf[7];
extern GtkWidget *g_drawing_area;
extern GdkPixmap *g_buffer_pixmap;
extern T_SGEObject *g_gem_objects[BOARD_WIDTH][BOARD_HEIGHT];

extern GweledPrefs prefs;
signed char gpc_font_glyphs[256];
gint gi_tiles_bg_pixbuf[2];
gint gi_gems_pixbuf[7];
gint gi_charset_pixbuf[50];
gint gi_cursor_pixbuf;

void
gweled_load_font (void)
{
	GdkPixbuf *pixbuf;
	int i;

	pixbuf =
	    sge_load_file_to_pixbuf ("gweled/font_24_20.png");
	if (pixbuf) {
		for (i = 0; i < 50; i++)
			gi_charset_pixbuf[i] =
			    sge_register_pixbuf (gdk_pixbuf_new_subpixbuf
						 (pixbuf, i * FONT_WIDTH,
						  0, FONT_WIDTH,
						  FONT_HEIGHT), -1);
	} else
		exit (-1);
}

void
gweled_load_pixmaps (void)
{
	gchar *filename, *full_pathname;
	GdkPixbuf *pixbuf;
	int i;

	for (i = 0; i < 7; i++) {
		filename = g_strdup_printf ("gweled/gem%02d.svg", i + 1);
		pixbuf =
		    sge_load_svg_to_pixbuf (filename,
					    prefs.tile_width, prefs.tile_height);
		if (pixbuf == NULL)
			exit (-1);
		gi_gems_pixbuf[i] = sge_register_pixbuf (pixbuf, -1);

		g_free (filename);
	}

	pixbuf =
	    sge_load_svg_to_pixbuf ("gweled/tile_odd.svg",
				    prefs.tile_width, prefs.tile_height);
	if (pixbuf == NULL)
		exit (-1);
	gi_tiles_bg_pixbuf[0] = sge_register_pixbuf (pixbuf, -1);

	pixbuf =
	    sge_load_svg_to_pixbuf ("gweled/tile_even.svg",
				    prefs.tile_width, prefs.tile_height);
	if (pixbuf == NULL)
		exit (-1);
	gi_tiles_bg_pixbuf[1] = sge_register_pixbuf (pixbuf, -1);

	pixbuf =
	    sge_load_svg_to_pixbuf ("gweled/cursor.svg",
				    prefs.tile_width, prefs.tile_height);
	if (pixbuf == NULL)
		exit (-1);
	gi_cursor_pixbuf = sge_register_pixbuf (pixbuf, -1);
}

// FIXME: load and reload functions ? yeah right...
void
gweled_reload_pixmaps (void)
{
	gchar *filename, *full_pathname;
	GdkPixbuf *pixbuf;
	int i;

	for (i = 0; i < 7; i++) {
		filename = g_strdup_printf ("gweled/gem%02d.svg", i + 1);
		pixbuf =
		    sge_load_svg_to_pixbuf (filename,
					    prefs.tile_width, prefs.tile_height);
		if (pixbuf == NULL)
			exit (-1);
		sge_register_pixbuf (pixbuf, gi_gems_pixbuf[i]);

		g_free (filename);
	}

	pixbuf =
	    sge_load_svg_to_pixbuf ("gweled/tile_odd.svg",
				    prefs.tile_width, prefs.tile_height);
	if (pixbuf == NULL)
		exit (-1);
	sge_register_pixbuf (pixbuf, gi_tiles_bg_pixbuf[0]);

	pixbuf =
	    sge_load_svg_to_pixbuf ("gweled/tile_even.svg",
				    prefs.tile_width, prefs.tile_height);
	if (pixbuf == NULL)
		exit (-1);
	sge_register_pixbuf (pixbuf, gi_tiles_bg_pixbuf[1]);

	pixbuf =
	    sge_load_svg_to_pixbuf ("gweled/cursor.svg",
				    prefs.tile_width, prefs.tile_height);
	if (pixbuf == NULL)
		exit (-1);
	sge_register_pixbuf (pixbuf, gi_cursor_pixbuf);
}

void
gweled_init_glyphs (void)
{
	int i;

	for (i = 0; i < 127; i++)
		if (g_ascii_isupper (i))
			gpc_font_glyphs[i] = i - 'A';
		else if (g_ascii_isdigit (i))
			gpc_font_glyphs[i] = 29 + i - '0';
		else
			switch (i) {
			case '(':
				gpc_font_glyphs[i] = 26;
				break;
			case ')':
				gpc_font_glyphs[i] = 27;
				break;
			case '-':
				gpc_font_glyphs[i] = 28;
				break;
			case '.':
				gpc_font_glyphs[i] = 39;
				break;
			case ':':
				gpc_font_glyphs[i] = 40;
				break;
			case ',':
				gpc_font_glyphs[i] = 41;
				break;
			case '\'':
				gpc_font_glyphs[i] = 42;
				break;
			case '"':
				gpc_font_glyphs[i] = 43;
				break;
			case '?':
				gpc_font_glyphs[i] = 44;
				break;
			case '!':
				gpc_font_glyphs[i] = 45;
				break;
			case '#':	// "!!!"
				gpc_font_glyphs[i] = 46;
				break;
			case '_':	// "..."
				gpc_font_glyphs[i] = 47;
				break;
			case '*':	// "ang"
				gpc_font_glyphs[i] = 48;
				break;
			default:
				gpc_font_glyphs[i] = -1;
				break;
			}
}

void
gweled_draw_board (void) 
{
	gint i, j;
		for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
			sge_create_object (i * prefs.tile_width,
					   j * prefs.tile_height, 0,
					   gi_tiles_bg_pixbuf[(i + j) % 2]);
}

T_SGEObject *
gweled_draw_character (int x, int y, int layer, char character)
{
	if (gpc_font_glyphs[character] != -1)
		return sge_create_object (x, y, layer,
					  gi_charset_pixbuf[gpc_font_glyphs
							    [character]]);
	else
		return NULL;
}
void
gweled_draw_message_at (gchar * in_message, gint msg_x, gint msg_y)
{
	int i;
	gchar *message;

	message = g_ascii_strup (in_message, -1);

	for (i = 0; i < strlen (message); i++)
		gweled_draw_character (msg_x + i * FONT_WIDTH, msg_y, 1,
				       message[i]);

	free (message);
}
void
gweled_draw_message (gchar * in_message)
{
	gint msg_x, msg_y, msg_w;

	msg_w = FONT_WIDTH * strlen (in_message);
	msg_x = (BOARD_WIDTH * prefs.tile_width - msg_w) >> 1;
	msg_y = (BOARD_HEIGHT * prefs.tile_height - FONT_HEIGHT) >> 1;

	gweled_draw_message_at (in_message, msg_x, msg_y);
} void
gweled_draw_game_message (gchar * in_message, double timing)
{
	int i;
	gint msg_x, msg_y, msg_w;
	T_SGEObject *p_object;
	gchar *message;

	msg_w = FONT_WIDTH * strlen (in_message);
	msg_x = (BOARD_WIDTH * prefs.tile_width - msg_w) >> 1;
	msg_y = (BOARD_HEIGHT * prefs.tile_height - FONT_HEIGHT) >> 1;

	message = g_ascii_strup (in_message, -1);

	for (i = 0; i < strlen (message); i++)
		if (gpc_font_glyphs[message[i]] != -1) {
			p_object =
			    sge_create_object (msg_x + i * FONT_WIDTH,
					       msg_y, 3,
					       gi_charset_pixbuf
					       [gpc_font_glyphs
						[message[i]]]);
			sge_object_set_lifetime (p_object,
						 (int) (50.0 * timing));
		}
	free (message);
}

int
gweled_gems_fall_into_place (void)
{
	gint i, j;
	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
			sge_object_fall_to (g_gem_objects[i][j],
					    j * prefs.tile_height);
}


