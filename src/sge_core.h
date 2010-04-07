#ifndef _SGE_CORE_H_
#define _SGE_CORE_H_

typedef struct s_sge_object
{
	gdouble x;
	gdouble y;
	gdouble vx;
	gdouble vy;
	gdouble ax;
	gdouble ay;
	gint dest_x;
	gint dest_y;
	gint width;
	gint height;
	gint layer;
	gint lifetime;
	gint needs_drawing;
	gint pixbuf_id;	
	GdkPixmap *pre_rendered;
	gint (*stop_condition)(struct s_sge_object *);
	GFunc stop_callback;
}T_SGEObject;

void sge_init(void);
void sge_destroy(void);
void sge_set_drawing_area(GtkWidget *drawing_area, GdkPixmap *pixmap_buffer, gint width, gint height);

gint sge_register_pixbuf(GdkPixbuf *pixbuf, int index);

T_SGEObject *sge_create_object(gint x, gint y, gint layer, gint pixbuf_id);//GdkPixbuf *pixbuf);
void sge_destroy_object(gpointer object, gpointer user_data);
void sge_destroy_all_objects(void);

void sge_object_set_lifetime(T_SGEObject *object, gint lifetime);
void sge_object_take_down(T_SGEObject *object);
void sge_object_move_to(T_SGEObject *object, gint dest_x, gint dest_y);
void sge_objects_fall_to(T_SGEObject *object, gint dest_y);

gboolean sge_object_is_moving(T_SGEObject *object);
gboolean sge_objects_are_moving(void);
gboolean sge_objects_are_moving_on_layer(int layer);

void sge_invalidate_layer(int layer);

#endif
