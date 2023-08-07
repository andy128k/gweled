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

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

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

GtkWidget *g_clutter;
GtkWidget *g_welcome_box;
GtkWidget *g_progress_bar;
GtkWidget *g_score_label;
GtkWidget *g_bonus_label;
GtkWidget *g_pref_sounds_button;
GtkWidget *g_main_game_stack;
GtkWidget *g_headerbar;
GtkWidget *g_new_game;
GtkWidget *g_pause_game_btn;

ClutterActor *g_stage;

GtkMenuButton *g_menu_button;
GMenuModel *headermenu;

//GdkPixmap *g_buffer_pixmap = NULL;
GRand *g_random_generator;

GamesScores *highscores;

static const GamesScoresCategory scorecats[] = {
  {"Normal", NC_("game type", "Normal")  },
  {"Timed",  NC_("game type", "Timed") }
};


extern GweledPrefs prefs;
extern GSettings *settings;

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
on_new_game_activate_cb (GtkWidget *button, gpointer user_data)
{
	GtkWidget *dialog;
	gint response;

	if (is_game_running()) {
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

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(g_headerbar), "");

    gweled_stop_game ();

    welcome_screen_visibility (TRUE);
}

void
on_pause_activate_cb (GtkWidget *button, gpointer user_data)
{

    if(!is_game_running()) return;

    if ( board_get_pause() ) {
	    board_set_pause(FALSE);

	}
    else {
	    board_set_pause(TRUE);

	}
}

void
on_game_mode_start_clicked (GtkButton * button, gpointer game_mode)
{
    prefs.game_mode = GPOINTER_TO_UINT (game_mode);

    switch (prefs.game_mode) {
        case NORMAL_MODE:
            gtk_header_bar_set_subtitle(GTK_HEADER_BAR(g_headerbar), _("Normal"));
            break;
        case TIMED_MODE:
            gtk_header_bar_set_subtitle(GTK_HEADER_BAR(g_headerbar), _("Timed"));
            break;
        case ENDLESS_MODE:
            gtk_header_bar_set_subtitle(GTK_HEADER_BAR(g_headerbar), _("Endless"));
            break;
    }

    welcome_screen_visibility(FALSE);
    
    gweled_draw_board (prefs.tile_size);
    gweled_setup_game_window(TRUE);
    gweled_start_new_game ();

    respawn_board_engine_loop();
}


void
on_window_unfocus_cb (GtkWidget *widget,
                      GdkEvent  *event,
                      gpointer   user_data)
{
    if (is_game_running() && prefs.game_mode == TIMED_MODE && board_get_pause() == FALSE )
        board_set_pause(TRUE);
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
gweled_setup_game_window(gboolean playing)
{
    if (playing) {
        gtk_widget_set_sensitive(g_pause_game_btn, TRUE);
        gtk_widget_show(GTK_WIDGET(g_new_game));
        gtk_widget_show(GTK_WIDGET(g_pause_game_btn));
    }
    else {
        // Hide Play/Pause buttons.
        gtk_widget_hide(GTK_WIDGET(g_new_game));
        gtk_widget_hide(GTK_WIDGET(g_pause_game_btn));
    }
}


void
welcome_screen_visibility (gboolean value)
{
    if(value == FALSE) {
        gtk_stack_set_visible_child_name (GTK_STACK (g_main_game_stack), "game_scene");
        return;
    }
    
    gweled_setup_game_window(FALSE);
    
    // Set welcome screen visible
    gtk_stack_set_visible_child_name (GTK_STACK (g_main_game_stack), "welcome_screen");
}

void
gweled_ui_destroy(GtkWidget *window, gpointer user_data)
{
    GtkWidget *dialog;
    gint response;

    if (is_game_running()) {
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


static gboolean
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
 int tilesize, ts_x, ts_y;
  int i;

  g_print("configure_event_cb %d\n", event->width);

  if (event->width == 1) return FALSE;

  /* Compute the new tile size based on the size of the
   * drawing area, rounded down. */
  ts_x = event->width / BOARD_WIDTH;
  ts_y = event->height / BOARD_HEIGHT;
  if (ts_x * BOARD_WIDTH > event->width)
    ts_x--;
  if (ts_y * BOARD_HEIGHT > event->height)
    ts_y--;
  tilesize = MIN (ts_x, ts_y);

  if (tilesize == prefs.tile_size) return FALSE;


   clutter_actor_set_size (CLUTTER_ACTOR (g_stage),
                          BOARD_WIDTH * tilesize,
                          BOARD_HEIGHT * tilesize);


   gweled_set_objects_size (tilesize);

   prefs.tile_size = tilesize;


  return TRUE;
}

void
gweled_ui_init (GApplication *app)
{
	GtkWidget *box, *game_frame;
	gchar     *filename;
	gint       response;
	GtkBuilder *menu_builder;
	GAction *action;
    gboolean start_previous_game = FALSE;
    
    GError    *error = NULL;
    
    highscores = games_scores_new ("gweled",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "game type", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

	g_random_generator = g_rand_new_with_seed (time (NULL));
      
    builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource  (builder, GWELED_RESOURCE_BASE "ui/main.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
    g_main_window = GTK_WIDGET (gtk_builder_get_object (builder, "gweledApp"));
    g_welcome_box = GTK_WIDGET (gtk_builder_get_object (builder, "vboxWelcome"));
    g_progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "bonusProgressbar"));
    g_score_label = GTK_WIDGET (gtk_builder_get_object (builder, "scoreLabel"));
    g_main_game_stack = GTK_WIDGET (gtk_builder_get_object (builder, "main_game_stack"));
    g_menu_button = GTK_MENU_BUTTON (gtk_builder_get_object (builder, "menu_button"));
    
    g_headerbar = GTK_WIDGET (gtk_builder_get_object (builder, "headerbar"));
    g_new_game = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_button"));
    g_pause_game_btn = GTK_WIDGET (gtk_builder_get_object (builder, "pause_button"));

    game_frame = GTK_WIDGET (gtk_builder_get_object (builder, "game_frame"));

    // Clutter scene
    g_clutter = gtk_clutter_embed_new ();
    g_stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (g_clutter));


    clutter_actor_set_size  (g_stage,
                            BOARD_WIDTH * prefs.tile_size,
                            BOARD_HEIGHT * prefs.tile_size);

    clutter_stage_set_user_resizable (CLUTTER_STAGE(g_stage), FALSE);



    clutter_actor_set_background_color (g_stage, CLUTTER_COLOR_LightSkyBlue);


    gtk_window_set_default_size (GTK_WINDOW (g_main_window),
                               BOARD_WIDTH * prefs.tile_size, BOARD_HEIGHT * prefs.tile_size);

    gtk_widget_realize (g_main_window);

    gtk_container_add(GTK_CONTAINER(game_frame), GTK_WIDGET(g_clutter));

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

    g_signal_connect (G_OBJECT (g_clutter), "configure_event",
                    G_CALLBACK (configure_event_cb), NULL);

    
    g_print("Size %d x %d; tile size: %d\n", BOARD_WIDTH * prefs.tile_size, BOARD_HEIGHT * prefs.tile_size, prefs.tile_size);
			                     
	sge_init ();
	
	gweled_init_glyphs ();
	gweled_load_pixmaps (prefs.tile_size);
	gweled_load_font ();

    // Init sound
	if(prefs.sounds_on) {
	    sound_init(gdk_screen_get_default());
	}


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

        if (response == GTK_RESPONSE_YES) {
            load_previous_game();
            start_previous_game = TRUE;
        }
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
        { "scores", on_scores_activate }
    };

    GActionGroup *win_actions = G_ACTION_GROUP( g_simple_action_group_new () );

    action = g_settings_create_action(settings, "tile-size");
    g_action_map_add_action (G_ACTION_MAP (win_actions), action);

    action = g_settings_create_action(settings, "sound");
    g_action_map_add_action (G_ACTION_MAP (win_actions), action);

    action = g_settings_create_action(settings, "hints");
    g_action_map_add_action (G_ACTION_MAP (win_actions), action);

    GActionGroup *actions = G_ACTION_GROUP( g_simple_action_group_new () );

    gtk_widget_insert_action_group (g_main_window, "win", win_actions);
    gtk_widget_insert_action_group (g_main_window, "app", actions);

    g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), g_main_window);
    gtk_menu_button_set_menu_model(g_menu_button, headermenu);
    g_object_unref(G_OBJECT(menu_builder));
	
	gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(g_main_window));

    gtk_widget_show_all (g_main_window);

    // Setup default window mode.
    welcome_screen_visibility(!start_previous_game);
    gweled_setup_game_window(start_previous_game);
}

