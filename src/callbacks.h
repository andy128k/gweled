#include <gtk/gtk.h>


void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_scores1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_undo1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_help1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
drawing_area_expose_event_cb           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
drawing_area_button_press_event_cb     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_app1_delete_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_window1_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_button4_activate                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button4_released                    (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_window2_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_button5_released                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button4_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button5_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_closebutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_closebutton2_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_radiobutton5_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_radiobutton6_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_button2_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_radiobutton7_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_radiobutton8_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_radiobutton9_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_newButton_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_hintButton_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_easyRadiobutton_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_hardRadiobutton_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_smallRadiobutton_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_mediumRadiobutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_largeRadiobutton_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_music_checkbutton_toggled		    (GtkToggleButton *togglebutton,
                                         gpointer  		 user_data);
