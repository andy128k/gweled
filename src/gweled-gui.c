/* gweled-gui.c
 *
 * Copyright (C) 2021 Daniele Napolitano <dnax88@gmail.com>
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
#include <glib/gprintf.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

#include "graphic_engine.h"
#include "board_engine.h"
#include "gweled-scores.h"
#include "main.h"
#include "gweled-gui.h"

// GLOBALS
GuiContext* gweled_ui;

GRand *g_random_generator;

extern GweledPrefs prefs;
extern GSettings *settings;

#define GWELED_RESOURCE_BASE "/org/gweled/"

void
gweled_setup_game_window(gboolean playing)
{
    if (playing) {
        gtk_widget_set_sensitive(gweled_ui->g_pause_game_btn, TRUE);
        gtk_widget_show(gweled_ui->g_new_game_btn);
        gtk_widget_show(gweled_ui->g_pause_game_btn);

        // Sets window subtitle and progress bar visibility
        switch (prefs.game_mode) {
            case NORMAL_MODE:
                gtk_header_bar_set_subtitle(GTK_HEADER_BAR(gweled_ui->g_headerbar), _("Normal"));
                gtk_widget_set_visible (gweled_ui->g_progress_bar, TRUE);
                break;
            case TIMED_MODE:
                gtk_header_bar_set_subtitle(GTK_HEADER_BAR(gweled_ui->g_headerbar), _("Timed"));
                gtk_widget_set_visible (gweled_ui->g_progress_bar, TRUE);
                break;
            case ENDLESS_MODE:
                gtk_header_bar_set_subtitle(GTK_HEADER_BAR(gweled_ui->g_headerbar), _("Endless"));
                gtk_widget_set_visible (gweled_ui->g_progress_bar, FALSE);
                break;
        }

    }
    else {
        // Hide Play/Pause buttons.
        gtk_widget_hide(gweled_ui->g_new_game_btn);
        gtk_widget_hide(gweled_ui->g_pause_game_btn);
    }
}


void
welcome_screen_visibility (gboolean value)
{
    if (value) {
        gweled_setup_game_window(FALSE);

        gtk_widget_set_visible (gweled_ui->g_progress_bar, FALSE);

        // Set welcome screen visible
        gtk_stack_set_visible_child_name (GTK_STACK (gweled_ui->g_main_game_stack), "welcome_screen");
    }
    else {
        gtk_stack_set_visible_child_name (GTK_STACK (gweled_ui->g_main_game_stack), "game_scene");
    }
}


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

    gtk_show_about_dialog (GTK_WINDOW(gweled_ui->main_window),
        "authors", authors,
	    "translator-credits", strcmp("translator-credits", translator_credits) ? translator_credits : NULL,
        "comments", _("A puzzle game with gems"),
        "copyright", "Copyright © 2003-2005 Sebastien Delestaing\nCopyright © 2010-2021 Daniele Napolitano",
        "version", VERSION,
        "license-type", GTK_LICENSE_GPL_2_0,
        "website", "https://gweled.org",
	    "logo-icon-name", PACKAGE_NAME,
        NULL);
}

void
on_scores_activate (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{

  gweled_hiscores_show();
}

void
on_new_game_activate_cb (GtkWidget *button, gpointer user_data)
{
	GtkWidget *dialog;
	gint response;

	if (is_game_running()) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (gweled_ui->main_window),
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

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(gweled_ui->g_headerbar), "");

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
    welcome_screen_visibility(FALSE);
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

void
gweled_set_current_score (gint score)
{
    gchar msg_buffer[6];
    g_sprintf (msg_buffer, "%06d", score);
	gtk_label_set_markup (GTK_LABEL(gweled_ui->g_score_label), msg_buffer);
}


void
gweled_ui_destroy(GtkWidget *window, gpointer user_data)
{
    GtkWidget *dialog;
    gint response;

    if (is_game_running()) {
        dialog = gtk_message_dialog_new (GTK_WINDOW (gweled_ui->main_window),
                          GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_QUESTION,
                          GTK_BUTTONS_YES_NO,
                          _("Do you want to save the current game?"));

        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                         GTK_RESPONSE_NO);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        if (response == GTK_RESPONSE_YES)
            save_current_game(gweled_get_current_game());
        else {
            remove_saved_game();
        }
    }
    
    g_rand_free (g_random_generator);
    
    gtk_widget_destroy(gweled_ui->main_window);

	gtk_main_quit ();
}


static gboolean
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    gint tilesize, ts_x, ts_y;

    g_print("configure_event_cb %dx%d %d\n", event->width, event->height, event->send_event);

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

    // If no changements do nothing
    if (tilesize == prefs.tile_size) return FALSE;

    gweled_set_objects_size (tilesize);

    prefs.tile_size = tilesize;

    return TRUE;
}

void
gweled_ui_init (GApplication *app)
{
	GtkWidget *box, *game_frame;
	gint       response;
	GtkBuilder *menu_builder;
	GAction *action;
    gboolean start_previous_game = FALSE;
    
    GError    *error = NULL;

	g_random_generator = g_rand_new_with_seed (time (NULL));

    gweled_ui = g_malloc( sizeof(GuiContext) );
      
    gweled_ui->builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource  (gweled_ui->builder, GWELED_RESOURCE_BASE "ui/main.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
    gweled_ui->main_window = LOOKUP_WIDGET ("gweledApp");
    gweled_ui->g_welcome_box = LOOKUP_WIDGET ("vboxWelcome");
    gweled_ui->g_progress_bar = LOOKUP_WIDGET ("bonusProgressbar");
    gweled_ui->g_score_label = LOOKUP_WIDGET ("scoreLabel");
    gweled_ui->g_main_game_stack = LOOKUP_WIDGET ("main_game_stack");
    gweled_ui->g_menu_button = GTK_MENU_BUTTON (LOOKUP_WIDGET ("menu_button"));
    
    gweled_ui->g_headerbar = LOOKUP_WIDGET ("headerbar");
    gweled_ui->g_new_game_btn = LOOKUP_WIDGET ("new_game_button");
    gweled_ui->g_pause_game_btn = LOOKUP_WIDGET ("pause_button");

    game_frame = LOOKUP_WIDGET ("game_frame");

    gtk_window_set_default_size (GTK_WINDOW (gweled_ui->main_window),
                               BOARD_WIDTH * prefs.tile_size, BOARD_HEIGHT * prefs.tile_size);

    gtk_widget_realize (gweled_ui->main_window);

    // Clutter scene
    gweled_ui->g_clutter = gtk_clutter_embed_new ();
    gweled_ui->g_stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (gweled_ui->g_clutter));

    // Minimum size of tiles of 48px
    gtk_widget_set_size_request(gweled_ui->g_clutter,
                                BOARD_WIDTH * 48,
                                BOARD_HEIGHT * 48);

    clutter_actor_set_size  (gweled_ui->g_stage,
                            BOARD_WIDTH * prefs.tile_size,
                            BOARD_HEIGHT * prefs.tile_size);

    clutter_stage_set_use_alpha(CLUTTER_STAGE(gweled_ui->g_stage), TRUE);
    clutter_actor_set_background_color(gweled_ui->g_stage, CLUTTER_COLOR_Transparent);

    gtk_container_add(GTK_CONTAINER(game_frame), gweled_ui->g_clutter);

    gtk_widget_set_can_focus (gweled_ui->g_clutter, TRUE);

	// Header button events
	g_signal_connect(G_OBJECT(gweled_ui->g_new_game_btn), "clicked",
                     G_CALLBACK(on_new_game_activate_cb), NULL);
    g_signal_connect(G_OBJECT(gweled_ui->g_pause_game_btn), "clicked",
                     G_CALLBACK(on_pause_activate_cb), NULL);


    // Game mode button events
    g_signal_connect(LOOKUP_WIDGET ("buttonNormal"), "clicked",
                     G_CALLBACK(on_game_mode_start_clicked), GINT_TO_POINTER(NORMAL_MODE));
    g_signal_connect(LOOKUP_WIDGET ("buttonTimed"), "clicked",
                     G_CALLBACK(on_game_mode_start_clicked), GINT_TO_POINTER(TIMED_MODE));
    g_signal_connect(LOOKUP_WIDGET ("buttonEndless"), "clicked",
                     G_CALLBACK(on_game_mode_start_clicked), GINT_TO_POINTER(ENDLESS_MODE));                 
                 
	
    // Main window events
    g_signal_connect(G_OBJECT(gweled_ui->main_window), "delete-event",
                     G_CALLBACK(gweled_ui_destroy), NULL);
                 
    g_signal_connect(G_OBJECT(gweled_ui->main_window), "focus-out-event",
                     G_CALLBACK(on_window_unfocus_cb), NULL);
     
    g_signal_connect(G_OBJECT(gweled_ui->main_window), "unmap-event",
                     G_CALLBACK(on_window_unfocus_cb), NULL);

    g_signal_connect (G_OBJECT (gweled_ui->g_clutter), "configure_event",
                    G_CALLBACK (configure_event_cb), NULL);

    
    g_print("Size %d x %d; tile size: %d\n", BOARD_WIDTH * prefs.tile_size, BOARD_HEIGHT * prefs.tile_size, prefs.tile_size);
			                     
	sge_init ();
	
	gweled_load_pixmaps (prefs.tile_size);
	
	// Menu
	menu_builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource (menu_builder, GWELED_RESOURCE_BASE "ui/headerbar-menu.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }

    gweled_ui->headermenu = G_MENU_MODEL(gtk_builder_get_object (menu_builder, "headermenu"));

    // Menu actions.
    const GActionEntry entries[] = {
        { "about", on_about_activate_cb },
        { "scores", on_scores_activate }
    };

    GActionGroup *win_actions = G_ACTION_GROUP( g_simple_action_group_new () );

    action = g_settings_create_action(settings, "sound");
    g_action_map_add_action (G_ACTION_MAP (win_actions), action);

    action = g_settings_create_action(settings, "hints");
    g_action_map_add_action (G_ACTION_MAP (win_actions), action);

    GActionGroup *actions = G_ACTION_GROUP( g_simple_action_group_new () );

    gtk_widget_insert_action_group (gweled_ui->main_window, "win", win_actions);
    gtk_widget_insert_action_group (gweled_ui->main_window, "app", actions);

    g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), gweled_ui->main_window);
    gtk_menu_button_set_menu_model(gweled_ui->g_menu_button, gweled_ui->headermenu);
    g_object_unref(G_OBJECT(menu_builder));
	
	gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(gweled_ui->main_window));

    gweled_init_scores(GTK_WINDOW(gweled_ui->main_window));

    gtk_widget_show_all (gweled_ui->main_window);

    gtk_widget_set_visible (gweled_ui->g_progress_bar, FALSE);

    // check for previous saved game
	if(is_present_saved_game()) {
	    box = gtk_message_dialog_new (GTK_WINDOW (gweled_ui->main_window),
                          GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_QUESTION,
                          GTK_BUTTONS_YES_NO,
                          _("There is a game saved, do you want restore it?"));

        gtk_dialog_set_default_response (GTK_DIALOG (box),
                         GTK_RESPONSE_NO);
        response = gtk_dialog_run (GTK_DIALOG (box));
        gtk_widget_destroy (box);

        if (response == GTK_RESPONSE_YES) {
            GweledGameState *previous_game = load_previous_game();
            gweled_set_previous_game(previous_game);
            g_free(previous_game);
            start_previous_game = TRUE;
        }

        remove_saved_game();
	}

    // Setup default window mode.
    welcome_screen_visibility(!start_previous_game);
    gweled_setup_game_window(start_previous_game);
}


