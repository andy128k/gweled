/*     ____
      /____\
      \    /    Gweled
       \  /
        \/

(C) 2003-2005 Sebastien Delestaing <sebdelestaing@free.fr>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>

#include <libgnome/gnome-score.h>

#include <pthread.h>
#include <mikmod.h>

#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"
#include "music.h"

// GLOBALS
GtkBuilder *gweled_xml;
GtkWidget *g_main_window;
GtkWidget *g_pref_window;
GtkWidget *g_score_window;

GtkWidget *g_drawing_area;
GtkWidget *g_progress_bar;
GtkWidget *g_score_label;
GtkWidget *g_bonus_label;
GdkPixmap *g_buffer_pixmap = NULL;
GRand *g_random_generator;

GweledPrefs prefs;
pthread_t thread;

/*MODULE *module;*/
SAMPLE *swap_sfx, *click_sfx;

/*static pthread_t thread;*/

void save_preferences(void)
{
	gchar *filename;
	FILE *stream;

	filename = g_strconcat(g_get_user_config_dir(), "/.gweled", NULL);
	stream = fopen(filename, "w");
	if(stream)
	{
		fwrite(&prefs, sizeof(GweledPrefs), 1, stream);
		fclose(stream);
	}

	g_free(filename);
}

void load_preferences(void)
{
	char *filename;
	FILE *stream;

	filename = g_strconcat(g_get_user_config_dir(), "/.gweled", NULL);

	stream = fopen(filename, "r");
	if(stream)
	{
		fread(&prefs, sizeof(GweledPrefs), 1, stream);
		fclose(stream);
	}
	else
	{
		prefs.tile_width = 48;
		prefs.tile_height = 48;
		prefs.timer_mode = FALSE;
		prefs.music_on = TRUE;

		save_preferences();
	}
	g_free(filename);
}

void init_pref_window(void)
{
	GtkWidget *radio_button = NULL;

	if (prefs.timer_mode == FALSE)
		radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "easyRadiobutton"));
	else
		radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "hardRadiobutton"));
	if (radio_button)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);

	radio_button = NULL;
	switch (prefs.tile_width) {
	case 32:
		radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "smallRadiobutton"));
		break;
	case 48:
		radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "mediumRadiobutton"));
		break;
	case 64:
		radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "largeRadiobutton"));
		break;
	}
	if (radio_button)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);

	//Is the music playing at start ?
	radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "music_checkbutton"));
	if (prefs.music_on)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), FALSE);
}

void show_hiscores (int newscore_rank)
{
 	// Note (newscore_rank := 0) is a rather kludgy hack that tells us we're
	//  calling this from the menu, not at the end of a game, so just display
 	//  the list, don't try to highlight the new score. Else if
 	//  (1 <= newscore_rank <= 10), then let the user know they've just made
 	//  a new high score.

 	GtkWidget *box;
 	GtkWidget *label;
 	char **names;
 	gfloat *scores;
 	time_t *scoretimes;
 	int i, nb_scores;
 	char *buffer;

 	gboolean close_score_list = TRUE;

 	nb_scores = gnome_score_get_notable ("gweled", "easy", &names, &scores, &scoretimes);

 // FIXME: that's a temporary solution to display the timed game highscores
     if (prefs.timer_mode)
 	   	nb_scores = gnome_score_get_notable ("gweled", "timed", &names, &scores, &scoretimes);
 	else
 	   	nb_scores = gnome_score_get_notable ("gweled", "easy", &names, &scores, &scoretimes);

 	if (nb_scores)
	{
 		buffer = g_strdup_printf ("label27");
 		label = GTK_WIDGET (gtk_builder_get_object (gweled_xml, buffer));
 		g_free (buffer);
 		if (label)
 			if (newscore_rank > 0)
 				gtk_label_set_markup (GTK_LABEL (label), "<span color=\"red\" weight=\"bold\">You made the highscore list!</span>");
 			else
 				gtk_label_set_markup (GTK_LABEL (label), "<span weight=\"bold\">Highscores</span>");

 		for (i = 0; i < 10 && i < nb_scores; i++)
		{
 			buffer = g_strdup_printf ("label%d", i + 28);
 			label = GTK_WIDGET (gtk_builder_get_object (gweled_xml, buffer));
 			g_free (buffer);
 			if (label)
			{
 				if (i == newscore_rank - 1)
 					buffer = g_strdup_printf ("<span weight=\"bold\">%d.</span>", i + 1);
 				else
 					buffer = g_strdup_printf ("%d.", i + 1);
 				gtk_label_set_markup (GTK_LABEL (label), buffer);
 				g_free (buffer);
 			}

 			buffer = g_strdup_printf ("nameLabel%02d", i + 1);
 			label = GTK_WIDGET (gtk_builder_get_object (gweled_xml, buffer));
 			g_free (buffer);
 			if (label)
 				if(i < nb_scores)
				{
 					if (i == newscore_rank - 1)
					{
						buffer = g_strdup_printf ("<span weight=\"bold\">%s</span>", names[i]);
 						gtk_label_set_markup (GTK_LABEL (label), buffer);
 						g_free (buffer);
 					}
					else
 						gtk_label_set_text (GTK_LABEL (label), names[i]);
 					g_free (names[i]);
 				}
				else
 					gtk_label_set_text (GTK_LABEL (label), "");

 			buffer = g_strdup_printf ("scoreLabel%02d", i + 1);
 			label = GTK_WIDGET (gtk_builder_get_object (gweled_xml, buffer));
 			g_free (buffer);
 			if (label)
 				if(i < nb_scores)
				{
 					if (i == newscore_rank - 1)
 						buffer = g_strdup_printf ("<span weight=\"bold\">%.0f</span>", scores[i]);
 					else
 						buffer = g_strdup_printf ("%.0f", scores[i]);
 					gtk_label_set_markup (GTK_LABEL (label), buffer);
 					g_free (buffer);
 				}
				else
 					gtk_label_set_text (GTK_LABEL (label), "");
 		}

 		for (; i < nb_scores; i++)
 			g_free (names[i]);

 		g_free (names);
 		g_free (scores);
 		g_free (scoretimes);

 		gtk_widget_show (g_score_window);
 	}
	else
	{
 		box = gtk_message_dialog_new (GTK_WINDOW (g_main_window),
 					      GTK_DIALOG_DESTROY_WITH_PARENT,
 					      GTK_MESSAGE_INFO,
 					      GTK_BUTTONS_OK,
 					      "No highscores recorded yet");
 		gtk_dialog_run (GTK_DIALOG (box));
 		gtk_widget_destroy (box);
 	}
}

int main (int argc, char **argv)
{
	guint board_engine_id;
	GError* error = NULL;

	gnome_score_init ("gweled");

	gtk_window_set_default_icon_name ("gweled");
    g_set_application_name("Gweled");

    gtk_init(&argc, &argv);

    music_init ();
	sge_init ();

	g_random_generator = g_rand_new_with_seed (time (NULL));

    gweled_xml = gtk_builder_new ();
    if (!gtk_builder_add_from_file (gweled_xml, PACKAGE_DATA_DIR "/gweled.ui", &error))
    {
        g_warning ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
    g_pref_window = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "preferencesDialog"));
    g_main_window = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "gweledApp"));
    g_score_window = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "highscoresDialog"));
    g_progress_bar = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "bonusProgressbar"));
    g_score_label = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "scoreLabel"));
    g_bonus_label = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "bonusLabel"));
    g_drawing_area = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "boardDrawingarea"));

	load_preferences();
	init_pref_window();

	gtk_builder_connect_signals(gweled_xml, NULL);

	gtk_widget_add_events (GTK_WIDGET (g_drawing_area),
			       GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_MOTION_MASK);

	gtk_window_set_resizable (GTK_WINDOW (g_main_window), FALSE);
	gtk_widget_show (g_main_window);

	gtk_widget_set_size_request (GTK_WIDGET (g_drawing_area),
				     BOARD_WIDTH * prefs.tile_width,
				     BOARD_HEIGHT * prefs.tile_height);

	g_buffer_pixmap =
	    gdk_pixmap_new (g_drawing_area->window,
			    BOARD_WIDTH * prefs.tile_width,
			    BOARD_HEIGHT * prefs.tile_height, -1);

	gweled_init_glyphs ();
	gweled_load_pixmaps ();
	gweled_load_font ();

	gi_game_running = 0;

	// load sound fx
    swap_sfx = Sample_Load(DATADIR "/sounds/gweled/swap.wav");
    if (!swap_sfx) {
        fprintf(stderr, "Could not load swap.wav, reason: %s\n", MikMod_strerror(MikMod_errno));
        //MikMod_Exit();
        //return; nope, this is not critical
    }
    click_sfx = Sample_Load(DATADIR "/sounds/gweled/click.wav");
    if (!click_sfx) {
        fprintf(stderr, "Could not load click.wav, reason: %s\n", MikMod_strerror(MikMod_errno));
        //MikMod_Exit();
        //return;
    }

    MikMod_SetNumVoices(-1, 4);
	Voice_SetVolume(0, 255);
	Voice_SetVolume(1, 255);
	Voice_SetVolume(2, 255);
	Voice_SetVolume(3, 255);

    MikMod_EnableOutput();

	if (prefs.music_on)
		music_play();

	board_engine_id = gtk_timeout_add (100, board_engine_loop, NULL);
	sge_set_drawing_area (g_drawing_area, g_buffer_pixmap,
			      BOARD_WIDTH * prefs.tile_width,
			      BOARD_HEIGHT * prefs.tile_height);

	gweled_draw_board ();
	gweled_draw_message ("gweled");

	gtk_main ();

    MikMod_DisableOutput();

	sge_destroy ();
	gtk_timeout_remove (board_engine_id);
	g_rand_free (g_random_generator);
	g_object_unref(G_OBJECT(gweled_xml));

	music_stop();

	if(swap_sfx)
		Sample_Free(swap_sfx);
	if(click_sfx)
		Sample_Free(click_sfx);

	MikMod_Exit();


	return 0;
}

