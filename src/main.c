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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <clutter-gtk/clutter-gtk.h>

#include "gweled-gui.h"

//#include "sge_core.h"
#include "board_engine.h"
#include "main.h"

// Globals
guint board_engine_id;

GweledPrefs prefs;


void
save_preferences(void)
{
	gchar *filename, *configstr;
	GKeyFile *config;
	FILE *configfile;

	filename = g_strconcat(g_get_user_config_dir(), "/gweled.conf", NULL);

    config = g_key_file_new();

	g_key_file_set_integer(config, "General", "tile_size", prefs.tile_size);
	g_key_file_set_boolean(config, "General", "sounds_on", prefs.sounds_on);
	g_key_file_set_boolean(config, "General", "hints_off", prefs.hints_off);

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

	    prefs.tile_size = g_key_file_get_integer(config, "General", "tile_size", NULL);
	    prefs.sounds_on = g_key_file_get_boolean(config, "General", "sounds_on", NULL);
	    prefs.hints_off = g_key_file_get_boolean(config, "General", "hints_off", NULL);

	    if(prefs.tile_size <= 32)
	        prefs.tile_size = 32;
	    else if(prefs.tile_size >= 64)
	        prefs.tile_size = 64;
	    else
	        prefs.tile_size = 48;

    } else {
        if (error) {
            g_printerr("Error loading config file: %s\n", error->message);
            g_error_free (error);
        }

		prefs.tile_size = 48;
		prefs.sounds_on = TRUE;
		prefs.hints_off = FALSE;

		save_preferences();
	}
	g_key_file_free(config);
	g_free(filename);

}

void save_current_game(void)
{
    GweledGameState game;
    gchar *filename;
    FILE *stream;

    game = gweled_get_current_game();

    filename = g_strconcat(g_get_user_config_dir(), "/gweled.saved-game", NULL);

    stream = fopen(filename, "w");

    if(stream)
    {
        fwrite(&game, sizeof(GweledGameState), 1, stream);
        fclose(stream);
    }
}

void
load_previous_game()
{
    gchar *filename;
    FILE *stream;
    GweledGameState game;
    gint ret;

    filename = g_strconcat(g_get_user_config_dir(), "/gweled.saved-game", NULL);

    stream = fopen(filename, "r");

    if(stream)
    {
        ret = fread(&game, sizeof(GweledGameState), 1, stream);
        fclose(stream);

        if(ret == 1)
            gweled_set_previous_game(game);
    }

}

static void
gweled_activate_cb (GApplication *app, gpointer user_data)
{
    g_set_application_name("Gweled");

    gtk_window_set_default_icon_name ("gweled");
    
    g_object_set (gtk_settings_get_default (),
                    "gtk-application-prefer-dark-theme", TRUE,
                    NULL);
    
    /* Initialize the GUI */
    gweled_ui_init(app);

    /* Enter in the main loop */
    gtk_main();
}


int main (int argc, char **argv)
{
	GtkApplication *app;
	GError *error = NULL;
	int status;
	
	/* gettext */
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    // needed for scores handling
    setgid_io_init();
	
	app = gtk_application_new ("org.gweled", G_APPLICATION_FLAGS_NONE);
	
	g_signal_connect (app, "activate", G_CALLBACK (gweled_activate_cb), NULL);
	
    if (gtk_clutter_init_with_args (&argc, &argv,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &error) != CLUTTER_INIT_SUCCESS)
    {
    
        if (error) {
            g_critical ("Unable to initialize Clutter-GTK: %s", error->message);
            g_error_free (error);
            return EXIT_FAILURE;
          }
          else
            g_error ("Unable to initialize Clutter-GTK");
    }
    
    /* calling gtk_clutter_init* multiple times should be safe */
    g_assert (gtk_clutter_init (NULL, NULL) == CLUTTER_INIT_SUCCESS);

	status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

	//sge_destroy ();
	if(board_engine_id)
	    g_source_remove (board_engine_id);

	return status;
}

