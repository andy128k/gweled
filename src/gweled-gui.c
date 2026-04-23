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

static guint layout_idle_id = 0;

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
                gtk_widget_set_visible (gweled_ui->g_pause_game_btn, TRUE);
                break;
            case TIMED_MODE:
                gtk_header_bar_set_subtitle(GTK_HEADER_BAR(gweled_ui->g_headerbar), _("Timed"));
                gtk_widget_set_visible (gweled_ui->g_progress_bar, TRUE);
                gtk_widget_set_visible (gweled_ui->g_pause_game_btn, TRUE);
                break;
            case ENDLESS_MODE:
                gtk_header_bar_set_subtitle(GTK_HEADER_BAR(gweled_ui->g_headerbar), _("Endless"));
                gtk_widget_set_visible (gweled_ui->g_progress_bar, FALSE);
                gtk_widget_set_visible (gweled_ui->g_pause_game_btn, FALSE);
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
        "Crunchpix (explode sound)",
        NULL
    };

    const gchar *translator_credits = _("translator-credits");

    gtk_show_about_dialog (GTK_WINDOW(gweled_ui->main_window),
        "authors", authors,
        "translator-credits", strcmp("translator-credits", translator_credits) ? translator_credits : NULL,
        "comments", _("A puzzle game with gems"),
        "copyright", "Copyright © 2003-2005 Sebastien Delestaing\nCopyright © 2010-2025 Daniele Napolitano",
        "version", VERSION,
        "license-type", GTK_LICENSE_GPL_2_0,
        "website", "https://gweled.org",
        "logo-icon-name", APPLICATION_ID,
        NULL);
}

void
on_scores_activate (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{

  gweled_hiscores_show();
}

static void
new_game_cb () {
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(gweled_ui->g_headerbar), "");
    gweled_stop_game ();
    welcome_screen_visibility (TRUE);
}

static void
abort_game_cb (GtkDialog *dialog, int response_id, gpointer user_data) {
    gtk_widget_destroy (GTK_WIDGET (dialog));
    if (response_id == GTK_RESPONSE_YES)
        new_game_cb ();
}

void
on_new_game_activate_cb (GtkWidget *button, gpointer user_data)
{
    if (is_game_running()) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (gweled_ui->main_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_MESSAGE_QUESTION,
					      GTK_BUTTONS_YES_NO,
					      _("Do you really want to abort this game?"));

        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
						 GTK_RESPONSE_NO);
        g_signal_connect (dialog, "response", G_CALLBACK (abort_game_cb), NULL);
        gtk_window_present (GTK_WINDOW (dialog));
    } else {
        new_game_cb ();
    }
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

gboolean
board_start (gpointer data)
{
    gweled_start_new_game ();
    clutter_main ();
    respawn_board_engine_loop();
    return FALSE;
}

static void
on_game_mode_start_clicked (GtkButton * button, gpointer game_mode)
{
    prefs.game_mode = GPOINTER_TO_UINT (game_mode);
    welcome_screen_visibility(FALSE);
    gweled_setup_game_window(TRUE);

    // Waiting for the clutter stage to be realized.
    g_timeout_add (50, board_start, NULL);
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
    gchar msg_buffer[7];
    g_snprintf (msg_buffer, 7, "%06d", score);
    gtk_label_set_markup (GTK_LABEL(gweled_ui->g_score_label), msg_buffer);
}


gboolean
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
    
    sge_destroy ();
    gtk_widget_destroy (window);

    return FALSE;
}


static gboolean
sync_layout_idle (gpointer data)
{
    gint tilesize, ts_x, ts_y;
    GtkWidget *widget = GTK_WIDGET (data);
    ClutterActor *stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (widget));

    gint w = gtk_widget_get_allocated_width  (widget);
    gint h = gtk_widget_get_allocated_height (widget);

    ts_x = w / BOARD_WIDTH;
    ts_y = h / BOARD_HEIGHT;
    tilesize = MIN (ts_x, ts_y);

    if (tilesize < 1) tilesize = 1;

    // If no changements do nothing
    if (tilesize != prefs.tile_size) {

        clutter_actor_set_size (stage,
                            BOARD_WIDTH * tilesize,
                            BOARD_HEIGHT * tilesize);

        clutter_actor_set_position (stage, 0, 0);

        prefs.tile_size = tilesize;
        gweled_set_objects_size (tilesize);
    }

    layout_idle_id = 0;
    return G_SOURCE_REMOVE;
}


static void
on_stage_size_allocate (GtkWidget *widget, GtkAllocation *a, gpointer user_data)
{
    // Trying to avoid strange board movements on window resize.
    if (layout_idle_id == 0) {
        layout_idle_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                             50,
                                          sync_layout_idle,
                                          g_object_ref (widget),
                                          (GDestroyNotify) g_object_unref);
    }
}

void
gweled_ui_window_present() {
    gtk_window_present_with_time (GTK_WINDOW (gweled_ui->main_window), gtk_get_current_event_time());
}

static void
restore_game_cb (GtkDialog *dialog, int response_id, gpointer user_data) {
    gtk_widget_destroy (GTK_WIDGET (dialog));

    gboolean start_previous_game = FALSE;
    if (response_id == GTK_RESPONSE_YES) {
        GweledGameState *previous_game = load_previous_game();
        gweled_set_previous_game(previous_game);
        g_free(previous_game);
        start_previous_game = TRUE;
    }

    remove_saved_game();

    welcome_screen_visibility(!start_previous_game);
    gweled_setup_game_window(start_previous_game);
}

void
gweled_ui_init (GApplication *app)
{
	GtkWidget *game_frame;
	GtkBuilder *menu_builder;
	GAction *action;
    
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

    gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(gweled_ui->main_window));

    // Clutter scene
    gweled_ui->g_clutter = gtk_clutter_embed_new ();
    gweled_ui->g_stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (gweled_ui->g_clutter));

    // Set the board size at the default tile size.
    gtk_widget_set_size_request(gweled_ui->g_main_game_stack,
                                BOARD_WIDTH * prefs.tile_size,
                                BOARD_HEIGHT * prefs.tile_size);

    clutter_stage_set_no_clear_hint (CLUTTER_STAGE(gweled_ui->g_stage), TRUE);
    gtk_container_add(GTK_CONTAINER(game_frame), gweled_ui->g_clutter);

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

    g_signal_connect_after (G_OBJECT (gweled_ui->g_clutter), "size-allocate",
                    G_CALLBACK (on_stage_size_allocate), gweled_ui->g_stage);

    
    g_print("Size %d x %d; tile size: %d\n", BOARD_WIDTH * prefs.tile_size, BOARD_HEIGHT * prefs.tile_size, prefs.tile_size);
			                     
	sge_init ();
	gweled_load_pixmaps (prefs.tile_size);
    gweled_set_objects_size (prefs.tile_size);
	
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

    gweled_init_scores(GTK_WINDOW(gweled_ui->main_window));

    gtk_widget_show_all (gweled_ui->main_window);

    gtk_widget_set_visible (gweled_ui->g_progress_bar, FALSE);

    // check for previous saved game
    if(is_present_saved_game()) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (gweled_ui->main_window),
                          GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_QUESTION,
                          GTK_BUTTONS_YES_NO,
                          _("There is a game saved, do you want restore it?"));

        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                         GTK_RESPONSE_NO);
        g_signal_connect (dialog, "response", G_CALLBACK (restore_game_cb), NULL);
        gtk_window_present (GTK_WINDOW (dialog));
    } else {
        // Setup default window mode.
        welcome_screen_visibility(TRUE);
        gweled_setup_game_window(FALSE);
    }
}

