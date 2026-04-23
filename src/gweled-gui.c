/* gweled-gui.c
 *
 * Copyright (C) 2021 Daniele Napolitano <dnax88@gmail.com>
 * Copyright (C) 2026 Andrey Kutejko <andy128k@gmail.com>
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
#include <adwaita.h>

#include "graphic_engine.h"
#include "board_engine.h"
#include "gweled-scores.h"
#include "main.h"
#include "gweled-gui.h"

// GLOBALS
extern GweledPrefs prefs;
extern GSettings *settings;

#define GWELED_RESOURCE_BASE "/org/gweled/"

struct _GweledWindow {
    GtkApplicationWindow parent;
};

typedef struct _GweledWindowPrivate {
    AdwWindowTitle *window_title;
    GtkWidget *welcome_box;
    GtkProgressBar *progress_bar;
    GtkWidget *score_label;
    GtkWidget *main_game_stack;
    GtkAspectFrame *game_frame;
    GweledStage *stage;
    GtkButton *new_game_button;
    GtkButton *pause_button;
    GtkMenuButton *menu_button;

    gchar *last_progress_bar_text;
} GweledWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GweledWindow, gweled_window, ADW_TYPE_APPLICATION_WINDOW)

GweledStage *
gweled_window_get_stage (GweledWindow *window) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    return priv->stage;
}

static void
gweled_setup_game_window(GweledWindow *window, gboolean playing)
{
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    if (playing) {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->pause_button), TRUE);
        gtk_widget_set_visible (GTK_WIDGET (priv->new_game_button), TRUE);
        gtk_widget_set_visible (GTK_WIDGET (priv->pause_button), TRUE);

        // Sets window subtitle and progress bar visibility
        switch (prefs.game_mode) {
            case NORMAL_MODE:
                adw_window_title_set_subtitle (priv->window_title, _("Normal"));
                gtk_widget_set_visible (GTK_WIDGET (priv->progress_bar), TRUE);
                gtk_widget_set_visible (GTK_WIDGET (priv->pause_button), TRUE);
                break;
            case TIMED_MODE:
                adw_window_title_set_subtitle (priv->window_title, _("Timed"));
                gtk_widget_set_visible (GTK_WIDGET (priv->progress_bar), TRUE);
                gtk_widget_set_visible (GTK_WIDGET (priv->pause_button), TRUE);
                break;
            case ENDLESS_MODE:
                adw_window_title_set_subtitle (priv->window_title, _("Endless"));
                gtk_widget_set_visible (GTK_WIDGET (priv->progress_bar), FALSE);
                gtk_widget_set_visible (GTK_WIDGET (priv->pause_button), FALSE);
                break;
        }

    }
    else {
        // Hide Play/Pause buttons.
        gtk_widget_set_visible (GTK_WIDGET (priv->new_game_button), FALSE);
        gtk_widget_set_visible (GTK_WIDGET (priv->pause_button), FALSE);
    }
}


static void
welcome_screen_visibility (GweledWindow *window, gboolean value)
{
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    if (value) {
        gweled_setup_game_window(window, FALSE);

        gtk_widget_set_visible (GTK_WIDGET (priv->progress_bar), FALSE);

        // Set welcome screen visible
        gtk_stack_set_visible_child_name (GTK_STACK (priv->main_game_stack), "welcome_screen");
    }
    else {
        gtk_stack_set_visible_child_name (GTK_STACK (priv->main_game_stack), "game_scene");
    }
}


// Callbacks

static void
on_hamburger_activate_cb (GtkWidget* widget,
                          const char* action_name G_GNUC_UNUSED,
                          GVariant* parameter G_GNUC_UNUSED)
{
    GweledWindow *window = GWELED_WINDOW (widget);
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    gtk_menu_button_popup (priv->menu_button);
}

static void
on_about_activate_cb (GtkWidget* widget,
                      const char* action_name G_GNUC_UNUSED,
                      GVariant* parameter G_GNUC_UNUSED)
{
    const gchar *developers[] = {
        "Sebastien Delestaing <sebdelestaing@free.fr>",
        "Daniele Napolitano <dnax88@gmail.com>",
        "Wesley Ellis",
        "Crunchpix (explode sound)",
        "Andrey Kutejko <andy128k@gmail.com>",
        NULL
    };

    const gchar *translator_credits = _("translator-credits");

    adw_show_about_dialog (widget,
        "developers", developers,
        "translator-credits", strcmp("translator-credits", translator_credits) ? translator_credits : NULL,
        "comments", _("A puzzle game with gems"),
        "copyright", "Copyright © 2003-2005 Sebastien Delestaing\nCopyright © 2010-2025 Daniele Napolitano\nCopyright © 2026 Andrey Kutejko",
        "version", VERSION,
        "license-type", GTK_LICENSE_GPL_2_0,
        "website", "https://gweled.org",
        "application-name", _("Gweled"),
        "application-icon", APPLICATION_ID,
        NULL);
}

static void
on_scores_activate (GtkWidget* widget,
                    const char* action_name G_GNUC_UNUSED,
                    GVariant* parameter G_GNUC_UNUSED)
{
    gweled_hiscores_show (GTK_WINDOW (widget));
}

static void
new_game_cb (GweledWindow *window) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    adw_window_title_set_subtitle (priv->window_title, "");
    gweled_stop_game ();
    welcome_screen_visibility (window, TRUE);
}

static void
abort_game_cb (AdwAlertDialog* dialog G_GNUC_UNUSED,
               gchar* response,
               gpointer user_data)
{
    GweledWindow *window = GWELED_WINDOW (user_data);

    if (g_strcmp0 (response, "yes") == 0)
        new_game_cb (window);
}

void
on_new_game_activate_cb (GtkWidget *button G_GNUC_UNUSED, gpointer user_data)
{
    GweledWindow *window = GWELED_WINDOW (user_data);

    if (is_game_running()) {
        AdwDialog *dialog = adw_alert_dialog_new (_("Abort game?"),
                                                  _("Do you really want to abort this game?"));
        adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                        "no", _("_No"),
                                        "yes", _("_Yes"),
                                        NULL);
        adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "no");
        adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "no");

        g_signal_connect (dialog, "response", G_CALLBACK (abort_game_cb), window);

        adw_dialog_present (dialog, GTK_WIDGET (window));
    } else {
        new_game_cb (window);
    }
}

static void
on_pause_activate_cb (GtkWidget* widget G_GNUC_UNUSED,
                      const char* action_name G_GNUC_UNUSED,
                      GVariant* parameter G_GNUC_UNUSED)
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
    respawn_board_engine_loop();
    return FALSE;
}

static void
on_game_mode_start_clicked (GtkWidget* widget,
                            const char* action_name G_GNUC_UNUSED,
                            GVariant* parameter)
{
    GweledWindow *window = GWELED_WINDOW (widget);
    const char *game_mode = g_variant_get_string (parameter, NULL);

    if (g_strcmp0 (game_mode, "normal") == 0)
        prefs.game_mode = NORMAL_MODE;
    else if (g_strcmp0 (game_mode, "timed") == 0)
        prefs.game_mode = TIMED_MODE;
    else if (g_strcmp0 (game_mode, "endless") == 0)
        prefs.game_mode = ENDLESS_MODE;
    else {
        g_warning ("Unknown game mode %s", game_mode);
        prefs.game_mode = NORMAL_MODE;
    }
    welcome_screen_visibility(window, FALSE);
    gweled_setup_game_window (window, TRUE);

    // Waiting for the clutter stage to be realized.
    g_timeout_add (50, board_start, NULL);
}

void
on_window_unfocus_cb (GtkEventControllerFocus* controller G_GNUC_UNUSED,
                      gpointer user_data G_GNUC_UNUSED)
{
    if (is_game_running() && prefs.game_mode == TIMED_MODE && board_get_pause() == FALSE )
        board_set_pause(TRUE);
}

void
gweled_window_set_current_score (GweledWindow *window, gint score) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    gchar msg_buffer[7];
    g_snprintf (msg_buffer, 7, "%06d", score);
    gtk_label_set_markup (GTK_LABEL(priv->score_label), msg_buffer);
}

void
gweled_window_set_pause_enabled (GweledWindow *window, gboolean enabled) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    gtk_widget_set_visible (GTK_WIDGET (priv->pause_button), enabled);
}

void
gweled_window_set_paused (GweledWindow *window, gboolean paused) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    if (paused) {
        gtk_button_set_label (priv->pause_button, _("_Resume"));

        g_clear_pointer (&priv->last_progress_bar_text, g_free);
        priv->last_progress_bar_text = g_strdup (gtk_progress_bar_get_text (priv->progress_bar));
        gtk_progress_bar_set_text (priv->progress_bar, _("Paused"));
    } else {
        gtk_button_set_label (priv->pause_button, _("_Pause"));

        if (priv->last_progress_bar_text != NULL) {
            gtk_progress_bar_set_text (priv->progress_bar, priv->last_progress_bar_text);
            g_clear_pointer (&priv->last_progress_bar_text, g_free);
        }
    }

    gtk_widget_queue_draw (GTK_WIDGET (priv->stage));
}

void
gweled_window_set_progress (GweledWindow *window, gdouble progress) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    gtk_progress_bar_set_fraction (priv->progress_bar, progress);
}

void
gweled_window_set_level (GweledWindow *window, gint level) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    gchar *text_buffer = g_strdup_printf (_("Level %d"), level);
    gtk_progress_bar_set_text (priv->progress_bar, text_buffer);
    g_free (text_buffer);
}

void
gweled_window_reset_progress (GweledWindow *window) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    gtk_progress_bar_set_text (priv->progress_bar, "");
    gtk_progress_bar_set_fraction (priv->progress_bar, 0.0);
}


static void
save_game_cb (AdwAlertDialog* dialog G_GNUC_UNUSED,
              gchar* response,
              gpointer user_data)
{
    GweledWindow *window = GWELED_WINDOW (user_data);

    if (g_strcmp0 (response, "yes") == 0)
        save_current_game(gweled_get_current_game());
    else
        remove_saved_game();

    gtk_window_destroy (GTK_WINDOW (window));
}

static gboolean
on_close_request(GtkWidget *window, gpointer user_data G_GNUC_UNUSED)
{
    if (!is_game_running()) {
        return FALSE;
    }

    AdwDialog *dialog = adw_alert_dialog_new (_("Save game?"),
                                              _("Do you want to save the current game?"));
    adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                    "no", _("_No"),
                                    "yes", _("_Yes"),
                                    NULL);
    adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "no");
    adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "no");

    g_signal_connect (dialog, "response", G_CALLBACK (save_game_cb), window);

    adw_dialog_present (dialog, GTK_WIDGET (window));

    return TRUE;
}

static void
gweled_window_dispose (GObject *obj)
{
    sge_destroy ();
    G_OBJECT_CLASS (gweled_window_parent_class)->dispose (obj);
}

void
gweled_window_restore_game (GweledWindow *window)
{
    GweledGameState *previous_game = load_previous_game();
    gweled_set_previous_game(previous_game);
    g_free(previous_game);

    welcome_screen_visibility (window, FALSE);
    gweled_setup_game_window (window, TRUE);
}

GweledWindow *
gweled_window_new (AdwApplication *app)
{
    GAction *action;

    GweledWindow *window = g_object_new (GWELED_TYPE_WINDOW, "application", app, NULL);

    sge_init ();
	
    GActionGroup *win_actions = G_ACTION_GROUP( g_simple_action_group_new () );

    action = g_settings_create_action(settings, "sound");
    g_action_map_add_action (G_ACTION_MAP (win_actions), action);

    action = g_settings_create_action(settings, "hints");
    g_action_map_add_action (G_ACTION_MAP (win_actions), action);

    gtk_widget_insert_action_group (GTK_WIDGET (window), "win", win_actions);

    gweled_init_scores();

    // Setup default window mode.
    welcome_screen_visibility (window, TRUE);
    gweled_setup_game_window (window, FALSE);

    return window;
}

static void
gweled_window_class_init (GweledWindowClass *klass) {
    GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);

    G_OBJECT_CLASS (klass)->dispose = gweled_window_dispose;

    // Template
    GWELED_TYPE_STAGE; // preload type
    gtk_widget_class_set_template_from_resource (wc, GWELED_RESOURCE_BASE "ui/main.ui");
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, window_title);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, welcome_box);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, progress_bar);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, score_label);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, main_game_stack);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, game_frame);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, stage);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, new_game_button);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, pause_button);
    gtk_widget_class_bind_template_child_internal_private (wc, GweledWindow, menu_button);

    // Menu actions
    gtk_widget_class_install_action (wc, "app.about", NULL, on_about_activate_cb);
    gtk_widget_class_install_action (wc, "app.scores", NULL, on_scores_activate);
    gtk_widget_class_install_action (wc, "win.hamburger", NULL, on_hamburger_activate_cb);

    // Game actions
    gtk_widget_class_install_action (wc, "win.new-game", "s", on_game_mode_start_clicked);
    gtk_widget_class_install_action (wc, "win.pause", NULL, on_pause_activate_cb);
}

static void
gweled_window_init (GweledWindow *window) {
    GweledWindowPrivate *priv = gweled_window_get_instance_private (window);

    gtk_widget_init_template (GTK_WIDGET (window));

    // Set the board size at the default tile size.
    gtk_widget_set_size_request (priv->main_game_stack,
                                 BOARD_WIDTH * 64,
                                 BOARD_HEIGHT * 64);

    // Header button events
    g_signal_connect (priv->new_game_button, "clicked",
                      G_CALLBACK (on_new_game_activate_cb), window);

    // Main window events
    g_signal_connect (window, "close-request",
                      G_CALLBACK (on_close_request), NULL);

    GtkEventController* focus_controller = gtk_event_controller_focus_new ();
    g_signal_connect (focus_controller, "leave",
                      G_CALLBACK (on_window_unfocus_cb), NULL);
    gtk_widget_add_controller (GTK_WIDGET (window), GTK_EVENT_CONTROLLER (focus_controller));
}
