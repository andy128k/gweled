/* gweled-scores.c
 *
 * Copyright (C) 2023 Daniele Napolitano <dnax88@gmail.com>
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

#include <libgnome-games-support.h>

#include <gweled-scores.h>


typedef struct
{
  gchar *key;
  gchar *name;
} key_value;


static const key_value gweled_scorecats[] = {
  {"normal", NC_("game type", "Normal")  },
  {"timed",  NC_("game type", "Timed") }
};

static GamesScoresContext *high_scores;

const gchar *
category_name_from_key (const gchar *key)
{
  int i;
  for (i = 0; i < G_N_ELEMENTS (gweled_scorecats); ++i) {
    if (g_strcmp0 (gweled_scorecats[i].key, key) == 0)
      return g_dpgettext2(NULL, "game type", gweled_scorecats[i].name);
  }
  return NULL;
}

static GamesScoresCategory *
get_scores_category_from_key (const gchar *key, void* user_data)
{
  const gchar *name = category_name_from_key (key);
  if (name == NULL)
    return NULL;
  return games_scores_category_new (key, name);
}


static void
add_score_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GamesScoresContext *context = GAMES_SCORES_CONTEXT (source_object);
  GError *error = NULL;

  games_scores_context_add_score_finish (context, res, &error);
  if (error != NULL) {
    g_warning ("Failed to add score: %s", error->message);
    g_error_free (error);
  }
}

void
gweled_hiscores_show()
{
    games_scores_context_run_dialog (high_scores);
}

void
gweled_hiscores_show_and_add(guint score, guint game_type)
{
    GamesScoresCategory *category;

    category = get_scores_category_from_key (gweled_scorecats[game_type].key, NULL);

    games_scores_context_add_score (high_scores,
                                    score,
                                    category,
                                    NULL,
                                    add_score_cb,
                                    NULL);
    gweled_hiscores_show();
}

void
gweled_init_scores(GtkWindow *parent_window)
{
    high_scores = games_scores_context_new_with_icon_name("gweled",
                                           /* Translators: label displayed on the scores dialog, preceding a difficulty. */
                                           _("Game type:"),
                                           GTK_WINDOW (parent_window),
                                           get_scores_category_from_key,
                                           NULL, GAMES_SCORES_STYLE_POINTS_GREATER_IS_BETTER,
                                           APPLICATION_ID);

}

