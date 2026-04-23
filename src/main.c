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

#include "config.h"

#include <stdio.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gweled-gui.h"
#include "board_engine.h"
#include "graphic_engine.h"
#include "sound.h"
#include "main.h"

#define SAVED_GAME_FILENAME "gweled.saved-game"
#define SAVED_GAME_HEADER "gweled"

// Globals
GweledWindow* g_main_window;
guint board_engine_id;

GweledPrefs prefs;
GSettings *settings;

extern GRand *g_random_generator;

void
gweled_setting_changed (GSettings* self,
                        gchar* key,
                        gpointer user_data
) {
    g_print("Settings changed: %s\n", key);

    if (g_strcmp0 (key, "sound") == 0) {
    	prefs.sounds_on = g_settings_get_boolean (self, "sound");
    }

    if (g_strcmp0 (key, "hints") == 0) {
    	prefs.hints_off = !g_settings_get_boolean (self, "hints");
		gweled_set_hints_active(!prefs.hints_off);
    }
}

void load_preferences(void)
{
	prefs.sounds_on = g_settings_get_boolean (settings, "sound");
	prefs.hints_off = !g_settings_get_boolean (settings, "hints");
}

gboolean
validate_saved_game_file()
{
    gchar *filename;
    gchar header_buf[strlen(SAVED_GAME_HEADER)];

    filename = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S SAVED_GAME_FILENAME, NULL);

    // Check if file exists
    if (!g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
        return FALSE;

    FILE *fp = fopen(filename, "rb");

    // Check file size
    fseek(fp, 0, SEEK_END);
    glong file_size = ftell(fp);
    rewind(fp);

    if (file_size != strlen(SAVED_GAME_HEADER) + sizeof(GweledGameState)) {
        g_warning("Saved game file \"%s\": size mismatch, ignoring.", filename);
        g_free(filename);
        fclose(fp);
        return FALSE;
    }

    fread(header_buf, strlen(SAVED_GAME_HEADER), 1, fp);
    if (strncmp(header_buf, SAVED_GAME_HEADER, strlen(SAVED_GAME_HEADER)) != 0) {
        g_warning("Saved game file \"%s\": Unable to find file header, ignoring.", filename);
        g_free(filename);
        fclose(fp);
        return FALSE;
    }

    fclose(fp);

    return TRUE;
}


// check for previous saved game
gboolean
is_present_saved_game(void)
{
    return validate_saved_game_file();
}

void
save_current_game(GweledGameState *game)
{
    gchar *filename;
    FILE *stream;

    filename = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S SAVED_GAME_FILENAME, NULL);

    stream = fopen(filename, "wb");

    if(stream)
    {
        fwrite(SAVED_GAME_HEADER, strlen(SAVED_GAME_HEADER), 1, stream);
        fwrite(game, sizeof(GweledGameState), 1, stream);
        fclose(stream);
    }
    else {
        g_warning ("Unable to write save game file.");
    }

    g_free(filename);
}

GweledGameState*
load_previous_game(void)
{
    gchar *filename;
    FILE *stream;
    GweledGameState *game;
    gint ret;

    game = g_malloc( sizeof(GweledGameState) );

    filename = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S SAVED_GAME_FILENAME, NULL);

    stream = fopen(filename, "rb");

    if (stream)
    {
        fseek(stream, strlen(SAVED_GAME_HEADER), SEEK_SET);
        ret = fread(game, sizeof(GweledGameState), 1, stream);
        fclose(stream);
        g_free(filename);

        if(ret == 1)
            return game;
    }

    g_free(filename);

    return NULL;
}

void remove_saved_game(void)
{
    gchar *filename;
    filename = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S SAVED_GAME_FILENAME, NULL);
    if (g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)){
        unlink(filename);
    }
    g_free(filename);
}

static void
gweled_startup_cb (GApplication *app, gpointer user_data G_GNUC_UNUSED)
{
    gtk_window_set_default_icon_name (PACKAGE_NAME);

    adw_style_manager_set_color_scheme (adw_style_manager_get_default (),
                                        ADW_COLOR_SCHEME_PREFER_DARK);
    
    load_preferences();

    gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.hamburger",
                                           (char const * const[]){ "F10", NULL });
    gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.pause",
                                           (char const * const[]){ "<Control>p", NULL });

    // Init sound system
    sound_init ();

    g_random_generator = g_rand_new_with_seed (time (NULL));
}

static void
restore_game_cb (AdwAlertDialog* dialog G_GNUC_UNUSED,
                 gchar* response,
                 gpointer user_data)
{
    GweledWindow *window = GWELED_WINDOW (user_data);

    if (g_strcmp0 (response, "yes") == 0) {
        gweled_window_restore_game (window);
    }
    remove_saved_game();
}

static void
gweled_activate_cb (GApplication *app, gpointer user_data G_GNUC_UNUSED)
{
    GweledWindow *window = gweled_window_new (ADW_APPLICATION (app));

    g_main_window = window;

    gtk_window_present (GTK_WINDOW (window));

    // check for previous saved game
    if (is_present_saved_game()) {
        AdwDialog *dialog = adw_alert_dialog_new (_("Restore game?"),
                                                  _("There is a game saved, do you want restore it?"));
        adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                        "no", _("_No"),
                                        "yes", _("_Yes"),
                                        NULL);
        adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "no");
        adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "no");

        g_signal_connect (dialog, "response", G_CALLBACK (restore_game_cb), window);

        adw_dialog_present (dialog, GTK_WIDGET (window));
    }
}

static void
gweled_shutdown_cb (GApplication *app G_GNUC_UNUSED, gpointer user_data G_GNUC_UNUSED)
{
    g_rand_free (g_random_generator);
}

int main (int argc, char **argv)
{
    /* gettext */
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    AdwApplication *app = adw_application_new (APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "startup", G_CALLBACK (gweled_startup_cb), NULL);
    g_signal_connect (app, "activate", G_CALLBACK (gweled_activate_cb), NULL);
    g_signal_connect (app, "shutdown", G_CALLBACK (gweled_shutdown_cb), NULL);

    settings = g_settings_new (APPLICATION_ID);
    g_signal_connect (settings, "changed", G_CALLBACK (gweled_setting_changed), NULL);

    g_set_application_name (_("Gweled"));

    int status = g_application_run (G_APPLICATION (app), argc, argv);

    g_clear_object (&app);

    return status;
}

