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
#include <gnome.h>
#include <glib.h>
#include <glade/glade.h>
#include <pthread.h>
#include <mikmod.h>

#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"

// GLOBALS
GnomeProgram *g_program;
GladeXML *gweled_xml;
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

MODULE *module;
SAMPLE *swap_sfx, *click_sfx;

static pthread_t thread;

void save_preferences(void)
{
	gchar *filename;
	FILE *stream;

	filename = g_strconcat(g_get_home_dir(), "/.gweled", NULL);
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

	filename = g_strconcat(g_get_home_dir(), "/.gweled", NULL);

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

		save_preferences();	
	}
	g_free(filename);
}

void init_pref_window(void)
{
	GtkWidget *radio_button = NULL;

	if (prefs.timer_mode == FALSE)
		radio_button = glade_xml_get_widget(gweled_xml, "easyRadiobutton");
	else
		radio_button = glade_xml_get_widget(gweled_xml, "hardRadiobutton");
	if (radio_button)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);

	radio_button = NULL;
	switch (prefs.tile_width) {
	case 32:
		radio_button = glade_xml_get_widget(gweled_xml, "smallRadiobutton");
		break;
	case 48:
		radio_button = glade_xml_get_widget(gweled_xml, "mediumRadiobutton");
		break;
	case 64:
		radio_button = glade_xml_get_widget(gweled_xml, "largeRadiobutton");
		break;
	}
	if (radio_button)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);
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
 		label = glade_xml_get_widget(gweled_xml, buffer);
 		g_free (buffer);
 		if (label)
 			if (newscore_rank > 0)
 				gtk_label_set_markup (GTK_LABEL (label), "<span color=\"red\" weight=\"bold\">You made the highscore list!</span>");
 			else
 				gtk_label_set_markup (GTK_LABEL (label), "<span weight=\"bold\">Highscores</span>");
 		
 		for (i = 0; i < 10 && i < nb_scores; i++)
		{
 			buffer = g_strdup_printf ("label%d", i + 28);
 			label = glade_xml_get_widget(gweled_xml, buffer);
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
 			label = glade_xml_get_widget(gweled_xml, buffer);
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
 			label = glade_xml_get_widget(gweled_xml, buffer);
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
 					      _("No highscores recorded yet"));
 		gtk_dialog_run (GTK_DIALOG (box));
 		gtk_widget_destroy (box);
 	}
}
 

void mikmod_thread(void *ptr)
{
	while (1) {
	    usleep(10000);
		MikMod_Update();
    }
}
	
int main (int argc, char **argv)
{
	guint board_engine_id;

	gnome_score_init ("gweled");

	g_program =
	    gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc,
				argv, GNOME_PARAM_APP_DATADIR,
				PACKAGE_DATA_DIR, NULL);

    /* register all the drivers */
    MikMod_RegisterAllDrivers();

    /* register all the module loaders */
    MikMod_RegisterAllLoaders();

    /* initialize the library */
    if (MikMod_Init(""))
	{
        fprintf(stderr, "Could not initialize sound, reason: %s\n", MikMod_strerror(MikMod_errno));
        //return; don't fail on sound problems
    }

	sge_init ();

	g_random_generator = g_rand_new_with_seed (time (NULL));

	gweled_xml = glade_xml_new(PACKAGE_DATA_DIR "/gweled/gweled.glade", NULL, NULL);
	g_pref_window = glade_xml_get_widget(gweled_xml, "preferencesDialog");
	g_main_window = glade_xml_get_widget(gweled_xml, "gweledApp");
	g_score_window = glade_xml_get_widget(gweled_xml, "highscoresDialog");
	g_progress_bar = glade_xml_get_widget(gweled_xml, "bonusProgressbar");
	g_score_label = glade_xml_get_widget(gweled_xml, "scoreLabel");
	g_bonus_label = glade_xml_get_widget(gweled_xml, "bonusLabel");
	g_drawing_area = glade_xml_get_widget(gweled_xml, "boardDrawingarea");

	load_preferences();
	init_pref_window();

	glade_xml_signal_autoconnect(gweled_xml);

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

    // load module
    module = Player_Load(PACKAGE_DATA_DIR "/sounds/gweled/autonom.s3m", 64, 0);

	// load sound fx
    swap_sfx = Sample_Load(PACKAGE_DATA_DIR "/sounds/gweled/swap.wav");
    if (!swap_sfx) {
        fprintf(stderr, "Could not load swap.wav, reason: %s\n", MikMod_strerror(MikMod_errno));
        //MikMod_Exit();
        //return; nope, this is not critical
    }
    click_sfx = Sample_Load(PACKAGE_DATA_DIR "/sounds/gweled/click.wav");
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

    if (module) {
        Player_Start(module);
		Player_SetVolume(64);
		pthread_create(&thread, NULL, (void *)&mikmod_thread, NULL);

    } else
        fprintf(stderr, "Could not load module, reason: %s\n", MikMod_strerror(MikMod_errno));

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

    if (module)
	{
		pthread_cancel(thread);
		pthread_join(thread, NULL);
	    Player_Stop();
	    Player_Free(module);
	}
	if(swap_sfx)
		Sample_Free(swap_sfx);
	if(click_sfx)
		Sample_Free(swap_sfx);

	MikMod_Exit();


	return 0;
}

