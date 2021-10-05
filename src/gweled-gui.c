/* gweled-gui.c
 *
 * Copyright (C) 2021 Daniele Napolitano
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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include "graphic_engine.h"
#include "board_engine.h"
#include "sound.h"
#include "games-scores.h"
#include "games-scores-dialog.h"
#include "main.h"

#include "gweled-gui.h"

// GLOBALS
GtkBuilder *builder;
GtkWidget *g_main_window;
GtkWidget *g_pref_window;
GtkWidget *g_score_window;

GtkWidget *g_drawing_area;
GtkWidget *g_progress_bar;
GtkWidget *g_score_label;
GtkWidget *g_bonus_label;
GtkWidget *g_menu_pause;
GtkWidget *g_pref_sounds_button;
GtkWidget *g_alignment_welcome;
GtkWidget *g_new_game;
GtkWidget *g_pause_game_btn;

GtkMenuButton *g_menu_button;
GMenuModel *headermenu;

//GdkPixmap *g_buffer_pixmap = NULL;
GRand *g_random_generator;

GamesScores *highscores;

static const GamesScoresCategory scorecats[] = {
  {"Normal", NC_("game type", "Normal")  },
  {"Timed",  NC_("game type", "Timed") }
};


extern gint gi_game_running;
extern GweledPrefs prefs;

#define GWELED_RESOURCE_BASE "/org/gweled/"


// Callbacks
static void
on_about_activate_cb (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	const gchar *authors[] = {
	    "Sebastien Delestaing <sebdelestaing@free.fr>",
	    "Daniele Napolitano <dnax88@gmail.com>",
	    "Wesley Ellis",
	    NULL
	};

	const gchar *translator_credits = _("translator-credits");

    gtk_show_about_dialog (GTK_WINDOW(g_main_window),
        "authors", authors,
	    "translator-credits", strcmp("translator-credits", translator_credits) ? translator_credits : NULL,
        "comments", _("A puzzle game with gems"),
        "copyright", "Copyright © 2003-2005 Sebastien Delestaing\nCopyright © 2010-2021 Daniele Napolitano",
        "version", VERSION,
        "license-type", GTK_LICENSE_GPL_3_0,
        "website", "https://gweled.org",
	    "logo-icon-name", "gweled",
        NULL);
}

void
on_scores_activate (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
 	show_hiscores (0, FALSE);
}

void
on_preferences_activate (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	if(gi_game_running && !board_get_pause()) 
	    board_set_pause(TRUE);
	
	gtk_widget_show (g_pref_window);
}

void
on_prefs_tilesize_toggled_cb (GtkToggleButton *togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.tile_size = GPOINTER_TO_INT (user_data);

		gweled_set_objects_size (prefs.tile_size);
	}

    if(!gi_game_running)
        welcome_screen_visibility(TRUE);
}

void
on_sounds_checkbutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.sounds_on = TRUE;
		if(sound_get_enabled() == FALSE) {
	        sound_init(gdk_screen_get_default());
		    if(sound_get_enabled() == FALSE) {
	            gtk_widget_set_sensitive(g_pref_sounds_button, FALSE);
	        }
		}
	}
	else prefs.sounds_on = FALSE;
}

void
on_hints_checkbutton_toggled (GtkToggleButton *togglebutton, gpointer *data)
{
    if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.hints_off = TRUE;
		gweled_set_hints_active(FALSE);
	}
	else {
	    prefs.hints_off = FALSE;
	    gweled_set_hints_active(TRUE);
	}
}


void
on_prefs_close_cb (GtkWidget *widget, gpointer user_data)
{
	save_preferences();

    // unpause the game if running
    if(gi_game_running)
	    board_set_pause(FALSE);

	gtk_widget_hide (g_pref_window);
}


void
init_pref_window(GtkWidget *window)
{
	GtkWidget *radio_button = NULL;

	switch (prefs.tile_size) {
	    case 32:
		    radio_button = GTK_WIDGET (gtk_builder_get_object (builder, "smallRadiobutton"));
		    break;
	    case 48:
		    radio_button = GTK_WIDGET (gtk_builder_get_object (builder, "mediumRadiobutton"));
		    break;
	    case 64:
		    radio_button = GTK_WIDGET (gtk_builder_get_object (builder, "largeRadiobutton"));
		    break;
	}
	
	if (radio_button)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);
		
	g_signal_connect(gtk_builder_get_object (builder, "smallRadiobutton"), "toggled",
                     G_CALLBACK(on_prefs_tilesize_toggled_cb), GINT_TO_POINTER(32));
    g_signal_connect(gtk_builder_get_object (builder, "mediumRadiobutton"), "toggled",
                     G_CALLBACK(on_prefs_tilesize_toggled_cb), GINT_TO_POINTER(48));
    g_signal_connect(gtk_builder_get_object (builder, "largeRadiobutton"), "toggled",
                     G_CALLBACK(on_prefs_tilesize_toggled_cb), GINT_TO_POINTER(64));

	// Sounds
	radio_button = GTK_WIDGET (gtk_builder_get_object (builder, "sounds_checkbutton"));
	if (prefs.sounds_on)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_button), FALSE);

	g_signal_connect(radio_button, "toggled",
                     G_CALLBACK(on_sounds_checkbutton_toggled), NULL);
	
	// HINTS
	radio_button = GTK_WIDGET(gtk_builder_get_object(builder, "hints_checkbutton"));
	if(prefs.hints_off) {
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), TRUE);
	} else {
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), FALSE);
	}
	
	g_signal_connect(radio_button, "toggled",
                     G_CALLBACK(on_hints_checkbutton_toggled), NULL);
	
	g_signal_connect(window, "delete-event",
                     G_CALLBACK(on_prefs_close_cb), NULL);
}


void welcome_screen_visibility (gboolean value)
{
    gtk_widget_set_visible (g_alignment_welcome, value);
    gtk_widget_set_visible (g_drawing_area, !value);

    if(value == TRUE) {
    
        // Hide Play/Pause buttons.
        gtk_widget_hide(GTK_WIDGET(g_new_game));
        gtk_widget_hide(GTK_WIDGET(g_pause_game_btn));

        // set the label with value for word wrap
        gtk_widget_set_size_request( GTK_WIDGET (gtk_builder_get_object (builder, "labelDescNormal")), BOARD_WIDTH * prefs.tile_size - 30, -1);
        gtk_widget_set_size_request( GTK_WIDGET (gtk_builder_get_object (builder, "labelDescTimed")), BOARD_WIDTH * prefs.tile_size - 30, -1);
        gtk_widget_set_size_request( GTK_WIDGET (gtk_builder_get_object (builder, "labelDescEndless")), BOARD_WIDTH * prefs.tile_size - 30, -1);

        // if window is small, reduce spaces
        if(prefs.tile_size < 48) {

            gtk_box_set_spacing( GTK_BOX (gtk_builder_get_object (builder, "vboxWelcome")), 10);
            gtk_widget_hide(GTK_WIDGET (gtk_builder_get_object (builder, "scoreLabel2")));
            gtk_box_set_spacing( GTK_BOX (gtk_builder_get_object (builder, "hbox2")), 0);
            gtk_container_set_border_width( GTK_CONTAINER (gtk_builder_get_object (builder, "vboxWelcome")), 0);

        }
        else if(prefs.tile_size > 48) {

            gtk_box_set_spacing( GTK_BOX (gtk_builder_get_object (builder, "vboxWelcome")), 70);
            gtk_widget_show(GTK_WIDGET (gtk_builder_get_object (builder, "scoreLabel2")));
            gtk_box_set_spacing( GTK_BOX (gtk_builder_get_object (builder, "hbox2")), 12);
            gtk_container_set_border_width( GTK_CONTAINER (gtk_builder_get_object (builder, "vboxWelcome")), 30);

        }
        else {

            gtk_box_set_spacing( GTK_BOX (gtk_builder_get_object (builder, "vboxWelcome")), 40);
            gtk_widget_show(GTK_WIDGET (gtk_builder_get_object (builder, "scoreLabel2")));
            gtk_box_set_spacing( GTK_BOX (gtk_builder_get_object (builder, "hbox2")), 12);
            gtk_container_set_border_width( GTK_CONTAINER (gtk_builder_get_object (builder, "vboxWelcome")), 12);

        }
    }
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
      gtk_dialog_add_buttons (GTK_DIALOG (sorrydialog), _("_Quit"),
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

void
on_new_game_activate_cb (GtkWidget *button, gpointer user_data)
{
	GtkWidget *dialog;
	gint response;

	if (gi_game_running) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (g_main_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_MESSAGE_QUESTION,
					      GTK_BUTTONS_YES_NO,
					      _("Do you really want to abort this game?"));

		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
						 GTK_RESPONSE_NO);
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (response != GTK_RESPONSE_YES)
			return;
	}

    gweled_stop_game ();

    welcome_screen_visibility (TRUE);
}

void
on_game_mode_start_clicked (GtkButton * button, gpointer game_mode)
{
    prefs.game_mode = GPOINTER_TO_UINT (game_mode);
    welcome_screen_visibility(FALSE);

    gweled_draw_board ();
    gweled_start_new_game ();
    respawn_board_engine_loop();
    gtk_widget_set_sensitive(g_pause_game_btn, TRUE);
    
    
    gtk_widget_show(GTK_WIDGET(g_new_game));
    gtk_widget_show(GTK_WIDGET(g_pause_game_btn));
}


void
on_pause_activate_cb (GtkWidget *button, gpointer user_data)
{

    if(!gi_game_running) return;

    if ( board_get_pause() ) {
	    board_set_pause(FALSE);

	}
    else {
	    board_set_pause(TRUE);

	}
}


void
on_window_unfocus_cb (GtkWidget *widget,
                      GdkEvent  *event,
                      gpointer   user_data)
{
    if (gi_game_running && prefs.game_mode == TIMED_MODE && board_get_pause() == FALSE )
        board_set_pause(TRUE);
}


void
gweled_ui_destroy(GtkWidget *window, gpointer user_data)
{
    GtkWidget *dialog;
    gint response;

    if (gi_game_running) {
        dialog = gtk_message_dialog_new (GTK_WINDOW (g_main_window),
                          GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_QUESTION,
                          GTK_BUTTONS_YES_NO,
                          _("Do you want to save the current game?"));

        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                         GTK_RESPONSE_NO);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        if (response == GTK_RESPONSE_YES)
            save_current_game();
        else {
            gchar *filename;

            filename = g_strconcat(g_get_user_config_dir(), "/gweled.saved-game", NULL);
            unlink(filename);
        }
    }
    
    g_rand_free (g_random_generator);
    
    gtk_widget_destroy(g_main_window);

	gtk_main_quit ();
}

void
gweled_ui_init (GApplication *app)
{
	GtkWidget *box;
	gchar     *filename;
	gint       response;
	GtkBuilder *menu_builder;
    
    GError    *error = NULL;
    
    highscores = games_scores_new ("gweled",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "game type", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

	//sge_init ();

	g_random_generator = g_rand_new_with_seed (time (NULL));

    builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource  (builder, GWELED_RESOURCE_BASE "ui/main.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
    g_pref_window = GTK_WIDGET (gtk_builder_get_object (builder, "preferencesDialog"));
    g_main_window = GTK_WIDGET (gtk_builder_get_object (builder, "gweledApp"));
    g_score_window = GTK_WIDGET (gtk_builder_get_object (builder, "highscoresDialog"));
    g_progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "bonusProgressbar"));
    g_score_label = GTK_WIDGET (gtk_builder_get_object (builder, "scoreLabel"));
    g_drawing_area = GTK_WIDGET (gtk_builder_get_object (builder, "boardDrawingarea"));
    g_menu_pause = GTK_WIDGET (gtk_builder_get_object (builder, "pause1"));
    g_pref_sounds_button = GTK_WIDGET (gtk_builder_get_object (builder, "sounds_checkbutton"));
    g_alignment_welcome = GTK_WIDGET (gtk_builder_get_object (builder, "alignmentWelcome"));
    g_menu_button = GTK_MENU_BUTTON (gtk_builder_get_object (builder, "menu_button"));
    
    g_new_game = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_button"));
    g_pause_game_btn = GTK_WIDGET (gtk_builder_get_object (builder, "pause_button"));

	// Header button events
	g_signal_connect(G_OBJECT(g_new_game), "clicked",
                     G_CALLBACK(on_new_game_activate_cb), NULL);
    g_signal_connect(G_OBJECT(g_pause_game_btn), "clicked",
                     G_CALLBACK(on_pause_activate_cb), NULL);
                     
                     
    // Game mode button events
    g_signal_connect(gtk_builder_get_object (builder, "buttonNormal"), "clicked",
                     G_CALLBACK(on_game_mode_start_clicked), GINT_TO_POINTER(NORMAL_MODE));
    g_signal_connect(gtk_builder_get_object (builder, "buttonTimed"), "clicked",
                     G_CALLBACK(on_game_mode_start_clicked), GINT_TO_POINTER(TIMED_MODE));
    g_signal_connect(gtk_builder_get_object (builder, "buttonEndless"), "clicked",
                     G_CALLBACK(on_game_mode_start_clicked), GINT_TO_POINTER(ENDLESS_MODE));                 
                 
	
    // Main window events
    g_signal_connect(G_OBJECT(g_main_window), "delete-event",
                     G_CALLBACK(gweled_ui_destroy), NULL);
                 
    g_signal_connect(G_OBJECT(g_main_window), "focus-out-event",
                     G_CALLBACK(on_window_unfocus_cb), NULL);
     
    g_signal_connect(G_OBJECT(g_main_window), "unmap-event",
                     G_CALLBACK(on_window_unfocus_cb), NULL);
                     
                     
    load_preferences();
	init_pref_window(g_pref_window);

	gtk_builder_connect_signals(builder, NULL);

	gtk_widget_add_events (GTK_WIDGET (g_drawing_area),
			       GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_MOTION_MASK);

	gtk_window_set_resizable (GTK_WINDOW (g_main_window), FALSE);


    welcome_screen_visibility(TRUE);

    gtk_widget_set_size_request (g_drawing_area,
                                 BOARD_WIDTH * prefs.tile_size,
			                     BOARD_HEIGHT * prefs.tile_size);

    gtk_widget_set_size_request (g_alignment_welcome,
                                 BOARD_WIDTH * prefs.tile_size,
			                     BOARD_HEIGHT * prefs.tile_size);

    /*g_buffer_pixmap = gdk_pixmap_new (gtk_widget_get_parent_window(g_drawing_area),
			    BOARD_WIDTH * prefs.tile_size,
			    BOARD_HEIGHT * prefs.tile_size, -1);*/

	gweled_init_glyphs ();
	gweled_load_pixmaps ();
	gweled_load_font ();

	gi_game_running = 0;

	if(prefs.sounds_on) {
	    sound_init(gdk_screen_get_default());
	    if(sound_get_enabled() == FALSE) {
	        gtk_widget_set_sensitive(g_pref_sounds_button, FALSE);
	    }
	}

	/*sge_set_drawing_area (g_drawing_area, g_buffer_pixmap,
			      BOARD_WIDTH * prefs.tile_size,
			      BOARD_HEIGHT * prefs.tile_size);
*/
    // check for previous saved game
    filename = g_strconcat(g_get_user_config_dir(), "/gweled.saved-game", NULL);

	if(g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
	    box = gtk_message_dialog_new (GTK_WINDOW (g_main_window),
                          GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_QUESTION,
                          GTK_BUTTONS_YES_NO,
                          _("There is a game saved, do you want restore it?"));

        gtk_dialog_set_default_response (GTK_DIALOG (box),
                         GTK_RESPONSE_NO);
        response = gtk_dialog_run (GTK_DIALOG (box));
        gtk_widget_destroy (box);

        if (response == GTK_RESPONSE_YES)
            load_previous_game();
        else
            unlink(filename);
	}
	
	g_free(filename);	
	
	// Menu
	menu_builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource (menu_builder, GWELED_RESOURCE_BASE "ui/headerbar-menu.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }

    headermenu = G_MENU_MODEL(gtk_builder_get_object (menu_builder, "headermenu"));

    // Menu actions.
    const GActionEntry entries[] = {
        { "about", on_about_activate_cb },
        { "preferences", on_preferences_activate },
        { "scores", on_scores_activate }
    };

    GActionGroup *actions = G_ACTION_GROUP( g_simple_action_group_new () );
    gtk_widget_insert_action_group (g_main_window, "app", actions);
    g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), g_main_window);
    gtk_menu_button_set_menu_model(g_menu_button, headermenu);
    g_object_unref(G_OBJECT(menu_builder));
	
	gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(g_main_window));
    gtk_widget_show (g_main_window);
}

