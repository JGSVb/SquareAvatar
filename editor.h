#pragma once
#include <gtk/gtk.h>

#define TYPE_EDITOR (editor_get_type())
#define EDITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_EDITOR, Editor))

typedef struct _EditorClass 	EditorClass;
typedef struct _Editor 				Editor;
typedef struct _EditorPrivate EditorPrivate;

struct _EditorClass {
	GtkDrawingAreaClass parent_class;
};

struct _Editor {

	GtkDrawingArea parent_instance;
	EditorPrivate * priv;

};

struct _EditorPrivate {

	struct {
		gboolean loaded;

		GdkPixbuf * original;
		int o_width, o_height;
		double aspect_ratio;

		GdkPixbuf * transformed;
		int t_width, t_height;
		double x, y;
		int real_x, real_y;
	} image;

	struct {
		int size;
		float radius;
		double x, y;
		int real_x, real_y;
	} mask;

	struct {
		double x, y;
		int button;
	} mouse;

};

GType editor_get_type(void);

GtkWidget * editor_new(void);
int editor_unload_image(Editor * editor);
int editor_load_image_from_pixbuf(Editor * editor, GdkPixbuf * pb);
int editor_load_image_from_filename(Editor * editor, const char * filename);
int editor_fit_image(Editor * editor);
gboolean editor_get_loaded(Editor * editor);
