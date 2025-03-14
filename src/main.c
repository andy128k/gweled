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
#include <clutter-gtk/clutter-gtk.h>

#include "gweled-gui.h"
#include "board_engine.h"
#include "graphic_engine.h"
#include "sound.h"
#include "main.h"

#define SAVED_GAME_FILENAME "gweled.saved-game"
#define SAVED_GAME_HEADER "gweled"

// Globals
guint board_engine_id;

GweledPrefs prefs;
GSettings *settings;


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
	prefs.tile_size = 64;
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
is_present_saved_game()
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
load_previous_game()
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

void remove_saved_game()
{
    gchar *filename;
    filename = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S SAVED_GAME_FILENAME, NULL);
    if (g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)){
        unlink(filename);
    }
    g_free(filename);
}

static void
gweled_activate_cb (GApplication *app, gpointer user_data)
{
    gweled_ui_window_present();
}

static void
gweled_startup_cb (GApplication *app, gpointer user_data)
{
    gtk_window_set_default_icon_name (PACKAGE_NAME);
    
    g_object_set (gtk_settings_get_default (),
                    "gtk-application-prefer-dark-theme", TRUE,
                    NULL);
    
    load_preferences();

    /* Initialize the GUI */
    gweled_ui_init(app);

    // Init sound system
    sound_init ();
}

int main (int argc, char **argv)
{
	GtkApplication *app;
	int status;
	
	/* gettext */
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    app = gtk_application_new (APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (gweled_activate_cb), NULL);
    g_signal_connect (app, "startup", G_CALLBACK (gweled_startup_cb), NULL);

    if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    {
        GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_NONE,
                                                "%s", "Unable to initialize Clutter.");
        gtk_window_set_title (GTK_WINDOW (dialog), g_get_application_name ());
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        exit (EXIT_FAILURE);
    }
    
    settings = g_settings_new (APPLICATION_ID);
    g_signal_connect (settings, "changed", G_CALLBACK (gweled_setting_changed), NULL);

    g_set_application_name("Gweled");

	status = g_application_run (G_APPLICATION (app), argc, argv);

    g_object_run_dispose (G_OBJECT (app));
    g_clear_object (&app);

	return status;
}

