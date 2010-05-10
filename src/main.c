/*      ____
 *     /____\
 *     \    /    Gweled
 *      \  /
 *       \/
 *
 * Copyright (C) 2003-2005 Sebastien Delestaing <sebdelestaing@free.fr>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "games-scores.h"
#include "games-scores-dialog.h"

#include <pthread.h>
#include <mikmod.h>

#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"
#include "music.h"
#include "main.h"

// GLOBALS
GtkBuilder *gweled_xml;
GtkWidget *g_main_window;
GtkWidget *g_pref_window;
GtkWidget *g_score_window;

GtkWidget *g_drawing_area;
GtkWidget *g_progress_bar;
GtkWidget *g_score_label;
GtkWidget *g_bonus_label;
GtkWidget *g_menu_pause;
GdkPixmap *g_buffer_pixmap = NULL;
GRand *g_random_generator;

guint board_engine_id;

GweledPrefs prefs;
pthread_t thread;

static const GamesScoresCategory scorecats[] = {
  {"Normal", NC_("game type", "Normal")  },
  {"Timed",  NC_("game type", "Timed") }
};

GamesScores *highscores;

SAMPLE *swap_sfx, *click_sfx;

extern gint gi_game_running;

void save_preferences(void)
{
	gchar *filename, *configstr;
	GKeyFile *config;
	FILE *configfile;
	GError *error = NULL;

	filename = g_strconcat(g_get_user_config_dir(), "/gweled.conf", NULL);

    config = g_key_file_new();

	g_key_file_set_integer(config, "General", "tile_width", prefs.tile_width);
	g_key_file_set_integer(config, "General", "tile_height", prefs.tile_height);
	g_key_file_set_boolean(config, "General", "timer_mode", prefs.timer_mode);
	g_key_file_set_boolean(config, "General", "music_on", prefs.music_on);
	g_key_file_set_boolean(config, "General", "sounds_on", prefs.sounds_on);

    configstr = g_key_file_to_data(config, NULL, NULL);

	configfile = fopen(filename, "w");

	if(configfile != NULL) {
	    fprintf(configfile, configstr, NULL);
	    fclose(configfile);
	    g_free(configstr);

	} else {
	    g_printerr("Error writing config file\n");
    }

    g_key_file_free(config);
	g_free(filename);
}

void load_preferences(void)
{
	char *filename;
	GKeyFile *config;
	GError *error = NULL;

	filename = g_strconcat(g_get_user_config_dir(), "/gweled.conf", NULL);

	config = g_key_file_new();
	g_key_file_load_from_file(config, filename,
		G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error);

    if(error == NULL && g_key_file_has_group(config, "General")) {

	    prefs.tile_width = g_key_file_get_integer(config, "General", "tile_width", NULL);
	    prefs.tile_height = g_key_file_get_integer(config, "General", "tile_height", NULL);
	    prefs.timer_mode = g_key_file_get_boolean(config, "General", "timer_mode", NULL);
	    prefs.music_on = g_key_file_get_boolean(config, "General", "music_on", NULL);
	    prefs.sounds_on = g_key_file_get_boolean(config, "General", "sounds_on", NULL);

    } else {
        if (error) {
            g_printerr("Error loading config file: %s\n", error->message);
            g_error_free (error);
        }

		prefs.tile_width = 48;
		prefs.tile_height = 48;
		prefs.timer_mode = FALSE;
		prefs.music_on = TRUE;
		prefs.sounds_on = TRUE;

		save_preferences();
	}
	g_key_file_free(config);
	g_free(filename);

}

void init_pref_window(void)
{
	GtkWidget *radio_button = NULL;

    // Game type
	if (prefs.timer_mode == FALSE)
		radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "easyRadiobutton"));
	else
		radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "hardRadiobutton"));
	if (radio_button)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);

    // Board size
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

	// Sounds
	radio_button = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "sounds_checkbutton"));
	if (prefs.sounds_on)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), FALSE);
}

gint
show_hiscores (gint pos, gboolean endofgame)
{
  gchar *message;
  static GtkWidget *scoresdialog = NULL;
  static GtkWidget *sorrydialog = NULL;
  GtkWidget *dialog;
  gint result;

  if (endofgame && (pos <= 0)) {
    if (sorrydialog != NULL) {
      gtk_window_present (GTK_WINDOW (sorrydialog));
    } else {
      sorrydialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (g_main_window),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_INFO,
							GTK_BUTTONS_NONE,
							"<b>%s</b>\n%s",
							_
							("Game over!"),
							_
							("Great work, but unfortunately your score did not make the top ten."));
      gtk_dialog_add_buttons (GTK_DIALOG (sorrydialog), GTK_STOCK_QUIT,
			      GTK_RESPONSE_REJECT, _("_New Game"),
			      GTK_RESPONSE_ACCEPT, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (sorrydialog),
				       GTK_RESPONSE_ACCEPT);
      gtk_window_set_title (GTK_WINDOW (sorrydialog), "");
    }
    dialog = sorrydialog;
  } else {

    if (scoresdialog != NULL) {
      gtk_window_present (GTK_WINDOW (scoresdialog));
    } else {
      scoresdialog = games_scores_dialog_new (GTK_WINDOW (g_main_window), highscores, _("Gweled Scores"));
      games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
						    (scoresdialog),
						    _("Game type:"));
    }

    if (pos > 0) {
      games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (scoresdialog),
				       pos);
      message = g_strdup_printf ("<b>%s</b>\n\n%s",
				 _("Congratulations!"),
				 pos == 1 ? _("Your score is the best!") :
                                 _("Your score has made the top ten."));
      games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
				       message);
      g_free (message);
    } else {
      games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
				       NULL);
    }

    if (endofgame) {
      games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog),
				       GAMES_SCORES_QUIT_BUTTON |
				       GAMES_SCORES_NEW_GAME_BUTTON);
    } else {
      games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog), 0);
    }
    dialog = scoresdialog;
  }

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  return result;
}

void mikmod_thread(void *ptr)
{
	while (1) {
	    g_usleep(10000);
		MikMod_Update();
    }
}

int main (int argc, char **argv)
{
	GError* error = NULL;

	/* gettext */
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    // needed for scores handling
    setgid_io_init();

	g_set_application_name("Gweled");

	gtk_init(&argc, &argv);

	highscores = games_scores_new ("gweled",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "game type", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

	gtk_window_set_default_icon_name ("gweled");

    music_init ();
	sge_init ();

	g_random_generator = g_rand_new_with_seed (time (NULL));

    gweled_xml = gtk_builder_new ();
    if (!gtk_builder_add_from_file (gweled_xml, PACKAGE_DATA_DIR "/gweled.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
    g_pref_window = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "preferencesDialog"));
    g_main_window = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "gweledApp"));
    g_score_window = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "highscoresDialog"));
    g_progress_bar = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "bonusProgressbar"));
    g_score_label = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "scoreLabel"));
    g_drawing_area = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "boardDrawingarea"));
    g_menu_pause = GTK_WIDGET (gtk_builder_get_object (gweled_xml, "pause1"));

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
        g_warning("Could not load swap.wav, reason: %s", MikMod_strerror(MikMod_errno));

    }
    click_sfx = Sample_Load(DATADIR "/sounds/gweled/click.wav");
    if (!click_sfx) {
        g_warning("Could not load click.wav, reason: %s", MikMod_strerror(MikMod_errno));
    }

    MikMod_SetNumVoices(-1, 4);
	Voice_SetVolume(0, 255);
	Voice_SetVolume(1, 255);
	Voice_SetVolume(2, 255);
	Voice_SetVolume(3, 255);

    MikMod_EnableOutput();

    pthread_create(&thread, NULL, (void *)&music_thread, NULL);

	if (prefs.music_on)
		music_play();

	sge_set_drawing_area (g_drawing_area, g_buffer_pixmap,
			      BOARD_WIDTH * prefs.tile_width,
			      BOARD_HEIGHT * prefs.tile_height);

	gweled_draw_board ();
	gweled_draw_message ("gweled");

	gtk_main ();

    MikMod_DisableOutput();

	sge_destroy ();
	if(board_engine_id)
	    g_source_remove (board_engine_id);
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

