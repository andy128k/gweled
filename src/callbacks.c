#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glade/glade.h>
#include <mikmod.h>

#include "callbacks.h"

#include "sge_core.h"
#include "board_engine.h"
#include "graphic_engine.h"

extern gi_game_running;
extern gint gi_score;
extern gboolean g_do_not_score;

extern GladeXML *gweled_xml;
extern GtkWidget *g_main_window;
extern GtkWidget *g_pref_window;
extern GtkWidget *g_score_window;
extern GtkWidget *g_drawing_area;
extern GdkPixmap *g_buffer_pixmap;
extern GtkWidget *g_appbar;

extern gint gi_gem_clicked;
extern gint gi_x_click;
extern gint gi_y_click;

extern gint gi_gem_dragged;
extern gint gi_x_drag;
extern gint gi_y_drag;

extern GweledPrefs prefs;

SAMPLE *click_sfx;

static gint gi_dragging = 0;

void
on_new1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *box;
	gint response;

	if (gi_game_running) {
		box = gtk_message_dialog_new (GTK_WINDOW (g_main_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_MESSAGE_QUESTION,
					      GTK_BUTTONS_YES_NO,
					      _("Do you really want to abort this game ?"));

		gtk_dialog_set_default_response (GTK_DIALOG (box),
						 GTK_RESPONSE_NO);
		response = gtk_dialog_run (GTK_DIALOG (box));
		gtk_widget_destroy (box);

		if (response != GTK_RESPONSE_YES)
			return;
	}

	sge_destroy_all_objects ();
	gweled_draw_board ();
	gweled_start_new_game ();
}

void
on_scores1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	// See comments in show_hiscores() for meaning of 0 as argument
 	show_hiscores (0);
}

void
on_quit1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_main_quit ();
}

void
on_preferences1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_widget_show (g_pref_window);
}
void
on_about1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	static GtkWidget *about = NULL;
	GdkPixbuf *pixbuf = NULL;
	const gchar *authors[] = { "Sebastien Delestaing <sebdelestaing@free.fr>", NULL };
	const gchar *documenters[] = { NULL };
	const gchar *translator_credits = _("translator_credits");

	if (about != NULL)
	{
		gdk_window_raise (about->window);
		gdk_window_show (about->window);
		return;
	}

	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_DATA_DIR "/gweled/gweled_logo.png", NULL);

	about = gnome_about_new(_("Gweled"), VERSION,
			"Copyright \xc2\xa9 2003-2005 Sebastien Delestaing",
			_("This is a GNOME port of the PalmOS/Windows/Java game \"Bejeweled\" (aka \"Diamond Mine\")."),
			(const char **)authors,
			(const char **)documenters,
			strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
			pixbuf);

	gtk_window_set_icon_from_file(GTK_WINDOW(about), PACKAGE_DATA_DIR "/gweled/gweled_icon.png", NULL);

	if (pixbuf != NULL)
		gdk_pixbuf_unref (pixbuf);
	
	g_signal_connect (G_OBJECT (about), "destroy",
			G_CALLBACK (gtk_widget_destroyed), &about);
	g_object_add_weak_pointer (G_OBJECT (about), (void**)&(about));

	gtk_widget_show(about);
}

gboolean
drawing_area_expose_event_cb (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
	gdk_draw_drawable (GDK_DRAWABLE (widget->window),
			   widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			   g_buffer_pixmap, event->area.x, event->area.y,
			   event->area.x, event->area.y, event->area.width,
			   event->area.height);

	return FALSE;
}

gboolean
drawing_area_button_event_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	static int x_press = -1;
	static int y_press = -1;
	static int x_release = -1;
	static int y_release = -1;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		x_press = event->x / prefs.tile_width;
		y_press = event->y / prefs.tile_height;
		if (gi_game_running) {
			gi_x_click = x_press;
			gi_y_click = y_press;
			gi_gem_clicked = -1;
			gi_dragging = -1;

			if(click_sfx)
				Sample_Play(click_sfx, 0, 0);						

		} else {
			on_new1_activate (NULL, NULL);
		}
		break;

	case GDK_BUTTON_RELEASE:
		gi_dragging = 0;
		gi_gem_dragged = 0;		
		x_release = event->x / prefs.tile_width;
		y_release = event->y / prefs.tile_height;
		if( (x_release < 0) ||
			(x_release > BOARD_WIDTH - 1) ||
			(y_release < 0) ||
			(y_release > BOARD_HEIGHT - 1) )
			break;
		if( (x_press != x_release) ||
			(y_press != y_release))
		{
			if (gi_game_running) {
				gi_x_click = x_release;
				gi_y_click = y_release;
				gi_gem_clicked = -1;
			}
		}
		break;

	default:
		break;
	}

	return FALSE;
}

gboolean
drawing_area_motion_event_cb (GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
	if (gi_dragging) {
		gi_x_drag = event->x / prefs.tile_width;
		gi_y_drag = event->y / prefs.tile_height;
		gi_gem_dragged = -1;
	}
	
	return FALSE;
}

gboolean
on_app1_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	gtk_main_quit ();
	return FALSE;
}

gboolean
on_preferencesDialog_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	gtk_widget_hide (widget);
	return TRUE;
}

gboolean
on_highscoresDialog_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	gtk_widget_hide (widget);
	return TRUE;
}

void
on_closebutton1_clicked (GtkButton * button, gpointer user_data)
{
	save_preferences();
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void
on_closebutton2_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void
on_easyRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton))
	{
		prefs.timer_mode = FALSE;
		if (gi_game_running) {
			sge_destroy_all_objects ();
			gweled_draw_board ();
			gweled_start_new_game ();
		}
	}
}

void
on_hardRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton))
	{
		prefs.timer_mode = TRUE;
		if (gi_game_running) {
			sge_destroy_all_objects ();
			gweled_draw_board ();
			gweled_start_new_game ();
		}
	}
}
/*
void
on_hintButton_clicked (GtkButton * button, gpointer user_data)
{
	if (gi_game_running) {
		gweled_check_for_moves_left (&gi_x_click, &gi_y_click);
		gi_gem_clicked = -1;
		g_do_not_score = TRUE;
	}
}
*/
void
on_smallRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.tile_width = 32;
		prefs.tile_height = 32;

		gweled_reload_pixmaps ();

		gtk_widget_set_size_request (GTK_WIDGET (g_drawing_area),
					     BOARD_WIDTH * prefs.tile_width,
					     BOARD_HEIGHT * prefs.tile_height);
		g_object_unref (G_OBJECT (g_buffer_pixmap));
		g_buffer_pixmap = gdk_pixmap_new (g_drawing_area->window,
				    BOARD_WIDTH * prefs.tile_width,
				    BOARD_HEIGHT * prefs.tile_height, -1);
		sge_set_drawing_area (g_drawing_area, g_buffer_pixmap,
				      BOARD_WIDTH * prefs.tile_width,
				      BOARD_HEIGHT * prefs.tile_height);

		gweled_reload_pixmaps ();
	}
}

void
on_mediumRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.tile_width = 48;
		prefs.tile_height = 48;

		gweled_reload_pixmaps ();

		gtk_widget_set_size_request (GTK_WIDGET (g_drawing_area),
					     BOARD_WIDTH * prefs.tile_width,
					     BOARD_HEIGHT * prefs.tile_height);
		g_object_unref (G_OBJECT (g_buffer_pixmap));
		g_buffer_pixmap = gdk_pixmap_new (g_drawing_area->window,
				    BOARD_WIDTH * prefs.tile_width,
				    BOARD_HEIGHT * prefs.tile_height, -1);

		sge_set_drawing_area (g_drawing_area, g_buffer_pixmap,
				      BOARD_WIDTH * prefs.tile_width,
				      BOARD_HEIGHT * prefs.tile_height);
	}
}

void
on_largeRadiobutton_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		prefs.tile_width = 64;
		prefs.tile_height = 64;

		gweled_reload_pixmaps ();

		gtk_widget_set_size_request (GTK_WIDGET (g_drawing_area),
					     BOARD_WIDTH * prefs.tile_width,
					     BOARD_HEIGHT * prefs.tile_height);
		g_object_unref (G_OBJECT (g_buffer_pixmap));
		g_buffer_pixmap = gdk_pixmap_new (g_drawing_area->window,
				    BOARD_WIDTH * prefs.tile_width,
				    BOARD_HEIGHT * prefs.tile_height, -1);

		sge_set_drawing_area (g_drawing_area, g_buffer_pixmap,
				      BOARD_WIDTH * prefs.tile_width,
				      BOARD_HEIGHT * prefs.tile_height);
	}
}
