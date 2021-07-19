#include <math.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "editor.h"
#include "utils.h"

// XXX: Mai god mai god, GObject é complicado demais
// quem teve a ideia de colocar orientação a objetos no c :rage:
// https://code.woboq.org/gtk/gtk/gtk/gtkbutton.c.html essa source ajudou muito a entender algumas coisas

// TODO: Implementar métodos de `dispose` e `finalize`
// TODO: Gestures

G_DEFINE_TYPE_WITH_PRIVATE(Editor, editor, GTK_TYPE_DRAWING_AREA)

static void editor_realize(GtkWidget * widget);

static gboolean editor_draw(GtkWidget * editor, cairo_t * cr);
static gboolean editor_motion_notify_event(GtkWidget * editor, GdkEventMotion * event);
static gboolean editor_button_press_release_event(GtkWidget * editor, GdkEventButton * event);
static gboolean editor_scroll_event(GtkWidget * editor, GdkEventScroll * event);

static int editor_no_draw_image_move(Editor * editor, double x, double y);
static int editor_no_draw_image_scale(Editor * editor, int width, int height, GdkInterpType interp);
static int editor_no_draw_mask_move(Editor * editor, double x, double y);
static int editor_no_draw_change_mask_radius(Editor * editor, float radius);
static int editor_no_draw_change_mask_size(Editor * editor, int size);

static gboolean editor_do_mask_radius_scroll(GtkWidget * widget, GdkEventScroll * event);
static gboolean editor_do_image_zoom(GtkWidget * widget, GdkEventScroll * event);

static void editor_ajust_image_size_and_position_to_make_everything_beautiful(Editor * editor);

static GdkPixbuf * editor_no_image_loaded_pixbuf(int width, int height);

static void editor_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void editor_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static int editor_load_pixbuf(Editor * editor, GdkPixbuf * pb);

static int editor_zoom(Editor * editor, float zoom, double x, double y);


enum {
	PROP_IMAGE_LOADED = 1,
	PROP_COUNT,
};

enum {
	SIGNAL_IMAGE_LOADED,
	SIGNAL_IMAGE_UNLOADED,
	SIGNAL_COUNT,
};

static GParamSpec * props[PROP_COUNT] = {NULL, };
static guint signals[SIGNAL_COUNT]; 

static void editor_private_init(EditorPrivate * priv){

	priv->image.loaded = FALSE;

	priv->image.original = priv->image.transformed = NULL;
	priv->image.aspect_ratio = 0.0f;
	priv->image.o_width = priv->image.o_height = 0;

	priv->image.t_width = priv->image.t_height = 0;
	priv->image.x = priv->image.y = 0.0f;
	priv->image.real_x = priv->image.real_y = 0;

	priv->mask.x = priv->mask.y = 0.0f;
	priv->mask.real_x = priv->mask.real_y = 0;
	priv->mask.size = 150;
	priv->mask.radius = 0.6;

	priv->mouse.x = priv->mouse.y = 0.0f;
	priv->mouse.button = 0;

}

static void editor_class_init(EditorClass * klass){

	/* Eu até pensei em implementar propriedades para o tamanho da imagem, posição, mas depois cheguei a conclusão que por enquanto
	 * não será necessário para o que eu estou a fazer então apenas deixei para lá :thumbsup: */

	GObjectClass * gobject_class;
	GtkWidgetClass * widget_class;

	widget_class = GTK_WIDGET_CLASS(klass);
	gobject_class = G_OBJECT_CLASS(klass);

	widget_class->draw = editor_draw;
	widget_class->motion_notify_event = editor_motion_notify_event;
	widget_class->button_press_event = editor_button_press_release_event;
	widget_class->button_release_event = editor_button_press_release_event;
	widget_class->scroll_event = editor_scroll_event;

	gobject_class->set_property = editor_set_property;
	gobject_class->get_property = editor_get_property;

	props[PROP_IMAGE_LOADED] = g_param_spec_boolean("image-loaded",
			NULL,
			NULL,
			FALSE,
			G_PARAM_READABLE);

	g_object_class_install_properties(gobject_class, PROP_COUNT, props);

	signals[SIGNAL_IMAGE_LOADED] = g_signal_newv("image-loaded",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			NULL,
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			0,
			NULL);

	signals[SIGNAL_IMAGE_UNLOADED] = g_signal_newv("image-unloaded",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			NULL,
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			0,
			NULL);

}


static void editor_init(Editor * self){

	EditorPrivate * priv;
	GtkWidget * widget;

	self->priv = editor_get_instance_private(self);
	priv = self->priv;

	widget = GTK_WIDGET(self);

	gtk_widget_add_events(widget,
			GDK_POINTER_MOTION_MASK |
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			GDK_SCROLL_MASK |
			GDK_BUTTON1_MASK |
			GDK_FOCUS_CHANGE_MASK);

	gtk_widget_set_can_focus(widget, TRUE);
	gtk_widget_set_focus_on_click(widget, TRUE);

	g_signal_connect(G_OBJECT(self), "realize", G_CALLBACK(editor_realize), NULL);

	editor_private_init(priv);
	editor_unload_image(self);
}


GtkWidget * editor_new(void){
	return g_object_new(TYPE_EDITOR, NULL);
}

// Getters e setters do gobject

static void editor_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec){
	
	EditorPrivate * priv = EDITOR(object)->priv;
	(void)priv;
	(void)value;

	switch(prop_id){

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}

}

static void editor_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec){

	EditorPrivate * priv = EDITOR(object)->priv;

	switch(prop_id){

		case(PROP_IMAGE_LOADED):
			g_value_set_boolean(value, priv->image.loaded);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}

}

/*
 *
 * MÉTODOS
 * MÉTODOS
 *
 * */


static int editor_load_pixbuf(Editor * editor, GdkPixbuf * pb){

	if(pb == NULL)
		return -1;

	EditorPrivate * priv = editor->priv;

	if(priv->image.original    != NULL) g_object_unref(priv->image.original);
	if(priv->image.transformed != NULL) g_object_unref(priv->image.transformed);

	priv->image.original = pb;

	priv->image.o_width = gdk_pixbuf_get_width(pb);
	priv->image.o_height = gdk_pixbuf_get_height(pb);
	priv->image.aspect_ratio = (double)priv->image.o_width / priv->image.o_height;

	priv->image.transformed = gdk_pixbuf_copy(pb);
	priv->image.t_width = priv->image.o_width;
	priv->image.t_height = priv->image.o_height;
	priv->image.x = priv->image.y = 0.0f;
	priv->image.real_x = priv->image.real_y = 0;

	if(gtk_widget_get_realized(GTK_WIDGET(editor))) editor_fit_image(editor);

	return 0;

}

int editor_unload_image(Editor * editor){

	GdkPixbuf * no_image = editor_no_image_loaded_pixbuf(1024, 1024);

	if(editor_load_pixbuf(editor, no_image) == -1)
		return -1;

	editor->priv->image.loaded = FALSE;
	g_signal_emit(editor, signals[SIGNAL_IMAGE_UNLOADED], 0);

	return 0;

}


int editor_load_image_from_pixbuf(Editor * editor, GdkPixbuf * pb){

	if(editor_load_pixbuf(editor, pb) == -1){
		return -1;
	}

	editor->priv->image.loaded = TRUE;
	g_signal_emit(editor, signals[SIGNAL_IMAGE_LOADED], 0);

	return 0;
}

int editor_load_image_from_filename(Editor * editor, const char * filename){

	return editor_load_image_from_pixbuf(editor, gdk_pixbuf_new_from_file(filename, NULL));
	
}

gboolean editor_get_loaded(Editor * editor){

	return editor->priv->image.loaded;

}

static void editor_ajust_image_size_and_position_to_make_everything_beautiful(Editor * editor){
	
	EditorPrivate * priv = editor->priv;

	if(priv->image.loaded == FALSE) {

		GtkWidget * widget = GTK_WIDGET(editor);

		int wid, hei;
		wid = gtk_widget_get_allocated_width(widget);
		hei = gtk_widget_get_allocated_height(widget);

		editor_no_draw_image_move(editor,
				MAP_TO_RANGE(priv->image.x, -priv->image.t_width, wid),
				MAP_TO_RANGE(priv->image.y, -priv->image.t_height, hei));

		return;

	}

	if(MIN(priv->image.t_width, priv->image.t_height) < priv->mask.size){

		if(priv->image.t_width < priv->image.t_height){

			editor_no_draw_image_scale(editor,
					priv->mask.size,
					(double)priv->mask.size * ((double)1 / priv->image.aspect_ratio), GDK_INTERP_BILINEAR);
		} else {

			editor_no_draw_image_scale(editor,
					(double)priv->mask.size * priv->image.aspect_ratio,
					priv->mask.size, GDK_INTERP_BILINEAR);
		}

	}

	editor_no_draw_image_move(editor,
		MAP_TO_RANGE(priv->image.x, -(priv->image.t_width  - priv->mask.size - priv->mask.x), priv->mask.x),
		MAP_TO_RANGE(priv->image.y, -(priv->image.t_height - priv->mask.size - priv->mask.y), priv->mask.y)
	);

}

static int editor_no_draw_image_move(Editor * editor, double x, double y){

	EditorPrivate * priv = editor->priv;

	if(priv->image.transformed == NULL) return -1;

	priv->image.x = x;
	priv->image.y = y;

	priv->image.real_x = round(priv->image.x);
	priv->image.real_y = round(priv->image.y);

	return 0;

}

static int editor_no_draw_image_scale(Editor * editor, int width, int height, GdkInterpType interp){

	EditorPrivate * priv = editor->priv;

	if((width <= 0) || (height <= 0)) return -1;
	if(priv->image.original == NULL) return -1;

	priv->image.t_width  = width;
	priv->image.t_height = height;

	GdkPixbuf * scaled = gdk_pixbuf_scale_simple(priv->image.original, width, height, interp);

	g_object_unref(priv->image.transformed);
	priv->image.transformed = scaled;

	return 0;
}

int editor_fit_image(Editor * editor){

	EditorPrivate * priv = editor->priv;
	if(priv->image.original == NULL) return -1;

	GtkWidget * widget = GTK_WIDGET(editor);

	int wid_wid = gtk_widget_get_allocated_width(widget);
	int wid_hei = gtk_widget_get_allocated_height(widget);

	int width, height;
	double x, y;

	// imagem quadrada
	if(priv->image.o_width == priv->image.o_height){

		int size = MIN(wid_wid, wid_hei);
		width = height = size;

	// imagem horizontal
	} else if(priv->image.o_width > priv->image.o_height){

		width = wid_wid;
		height = (double)wid_wid * (1.0f / priv->image.aspect_ratio);

	// imagem vertical
	} else {

		height = wid_hei;
		width = (double)height * priv->image.aspect_ratio;
	}

	x = (double)wid_wid / 2 - (double)width / 2;
	y = (double)wid_hei / 2 - (double)height / 2;

	g_return_val_if_fail(editor_no_draw_image_scale(editor, width, height, GDK_INTERP_BILINEAR) == 0, -1);
	g_return_val_if_fail(editor_no_draw_image_move(editor, x, y) == 0, -1);

	g_debug("Widget tem tamanho alocado de %dx%d", wid_wid, wid_hei);
	g_debug("Imagem ajustada para %dx%d+%f+%f", width, height, x, y);

	gtk_widget_queue_draw(widget);

	return 0;

}

int editor_zoom(Editor * editor, float zoom, double x, double y){

	EditorPrivate * priv = editor->priv;
	if(priv->image.original == NULL) return -1;

	GtkWidget * widget = GTK_WIDGET(editor);

	int dest_height = priv->image.t_height + priv->image.o_height * zoom;
	int dest_width  = dest_height * priv->image.aspect_ratio; // manter o aspeto

	if(editor_no_draw_image_move(editor,            
			                 	priv->image.x - (x - priv->image.x) * ((double)dest_width  / priv->image.t_width  - 1),
			                 	priv->image.y - (y - priv->image.y) * ((double)dest_height / priv->image.t_height - 1)) != 0)
		return -1;

	if(editor_no_draw_image_scale(editor, dest_width, dest_height, GDK_INTERP_BILINEAR) != 0)
		return -1;

	gtk_widget_queue_draw(widget);

	return 0;

}

static int editor_no_draw_mask_move(Editor * editor, double x, double y){

	EditorPrivate * priv = editor->priv;
	priv->mask.x = x;
	priv->mask.y = y;
	priv->mask.real_x = round(x);
	priv->mask.real_y = round(y);

	return 0;
}

static int editor_no_draw_change_mask_radius(Editor * editor, float radius){

	EditorPrivate * priv = editor->priv;

	priv->mask.radius = MAP_TO_RANGE(radius, 0, 1);


	return 0;

}


static int editor_no_draw_change_mask_size(Editor * editor, int size){

	editor->priv->mask.size = size;

	return 0;
}

GdkPixbuf * editor_get_final_image(Editor * editor, int size, gboolean inscribed){
	
	g_return_val_if_fail(size > 0, NULL);

	EditorPrivate * priv = editor->priv;
	GdkPixbuf * out;
	int diameter, x, y;
	cairo_surface_t * surface;
	cairo_surface_t * image_surface;
	cairo_t * cr;


	// NOTE(image): Fazer uma imagem quadrada de tamanho definido
	{
		int width, height;
		double fac;
		GdkPixbuf * scaled, * cropped;

		fac 	 = (double)size / priv->mask.size;

		x			 = (priv->mask.x - priv->image.x) * fac;
		y			 = (priv->mask.y - priv->image.y) * fac;

		height = priv->image.t_height * fac;
		width  = height * priv->image.aspect_ratio;

		scaled = gdk_pixbuf_scale_simple(priv->image.original, width, height, GDK_INTERP_HYPER);
		cropped = gdk_pixbuf_new_subpixbuf(scaled, x, y, size, size);
		g_object_unref(scaled);

		image_surface = gdk_cairo_surface_create_from_pixbuf(cropped, 0, NULL);
	}

	// NOTE: Colocar a imagem como se fosse dentro de um círculo caso `inscribed` seja TRUE
	// Caso contrário, apenas colocar a imagem normalmente
	// Preparar desenho
	{

		diameter = LERP(priv->mask.radius, size * M_SQRT2, size);
		int radius = diameter / 2;

		if(inscribed){

			y = x = radius - size / 2;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, diameter, diameter);

		} else {
			x = y = 0;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
		}

		cr = cairo_create(surface);

		cairo_set_source_surface(cr, image_surface, x, y);
		cairo_rounded_rectangle(cr, x, y, size, size, priv->mask.radius);
		cairo_clip(cr);
		cairo_paint(cr);

	}

	cairo_destroy(cr);

	if(inscribed)
		out = gdk_pixbuf_get_from_surface(surface, 0, 0, diameter, diameter);
	else
		out = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);

	cairo_surface_destroy(surface);

	return out;
}
/*
 *
 *	CALLBACKS
 *	CALLBACKS
 *
 * */

static gboolean editor_draw(GtkWidget * widget, cairo_t * cr){

	Editor * editor;
	EditorPrivate * priv;
	cairo_surface_t * surface;

	editor = EDITOR(widget);
	priv = editor->priv;


	// deixar a máscara numa posição bonita e agradável
	// confia na estética :sunglasses:
	if(priv->image.loaded == TRUE){

		int width, height;

		width = gtk_widget_get_allocated_width(widget);
		height = gtk_widget_get_allocated_height(widget);

		editor_no_draw_change_mask_size(editor, MIN(width, height) * ((double)3/4));
		editor_no_draw_mask_move(editor,
				(double)width / 2 - (double)priv->mask.size / 2,
				(double)height / 2 - (double)priv->mask.size / 2);

	}

	editor_ajust_image_size_and_position_to_make_everything_beautiful(editor);

	surface = gdk_cairo_surface_create_from_pixbuf(priv->image.transformed, 0, NULL);

	// fundo
	cairo_set_source_rgb(cr, 0.115, 0.12, 0.12);
	cairo_paint(cr);

	// borda
	cairo_set_source_gray_tone(cr, 0.05);
	cairo_rectangle(cr, priv->image.real_x - 1, priv->image.real_y - 1, priv->image.t_width + 2, priv->image.t_height + 2);
	cairo_fill(cr);

	if(priv->image.loaded == FALSE){

		cairo_set_source_surface(cr, surface, priv->image.real_x, priv->image.real_y);
		cairo_paint(cr);
		
		return FALSE;
	}

	// mask
	cairo_set_source_gray_tone(cr, 0.1);
	cairo_rectangle(cr, priv->image.real_x, priv->image.real_y, priv->image.t_width, priv->image.t_height);
	cairo_fill(cr);

	cairo_set_source_gray_tone(cr, 1);
	cairo_rounded_rectangle(cr,
			priv->mask.real_x,
			priv->mask.real_y,
			priv->mask.size, priv->mask.size,
			priv->mask.radius);
	cairo_fill(cr);

	// pintar imagem com aquela coisinha de multiplicar para deixar ela mais escura
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_MULTIPLY);
	cairo_set_source_surface(cr, surface, priv->image.real_x, priv->image.real_y);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_surface_destroy(surface);

	return FALSE;
	
}

static gboolean editor_button_press_release_event(GtkWidget * widget, GdkEventButton * event){

	EditorPrivate * priv;
	Editor * editor = EDITOR(widget);
	priv = editor->priv;

	gtk_widget_grab_focus(widget);

	priv->mouse.button = event->type;

	return FALSE;

}

static gboolean editor_motion_notify_event(GtkWidget * widget, GdkEventMotion * event){

	Editor * editor;
	EditorPrivate * priv;

	editor = EDITOR(widget);
	priv = editor->priv;


	if(priv->mouse.button == GDK_BUTTON_PRESS){

		editor_no_draw_image_move(editor,
				priv->image.x + event->x - priv->mouse.x,
				priv->image.y + event->y - priv->mouse.y);

		gtk_widget_queue_draw(widget);
	}

	priv->mouse.x = event->x;
	priv->mouse.y = event->y;

	return FALSE;

}

static gboolean editor_do_mask_radius_scroll(GtkWidget * widget, GdkEventScroll * event){

	Editor * editor = EDITOR(widget);
	EditorPrivate * priv = editor->priv;

	if(event->direction == GDK_SCROLL_UP)
		editor_no_draw_change_mask_radius(editor, priv->mask.radius + 0.05);
	else if (event->direction == GDK_SCROLL_DOWN)
		editor_no_draw_change_mask_radius(editor, priv->mask.radius - 0.05);

	gtk_widget_queue_draw(widget);

	return FALSE;
}

static gboolean editor_do_image_zoom(GtkWidget * widget, GdkEventScroll * event){

	Editor * editor = EDITOR(widget);
	EditorPrivate * priv = editor->priv;

	if(event->direction == GDK_SCROLL_UP)
		editor_zoom(editor, 0.1, priv->mouse.x, priv->mouse.y);
	else if (event->direction == GDK_SCROLL_DOWN)
		editor_zoom(editor, -0.1, priv->mouse.x, priv->mouse.y);

	return FALSE;
}

static gboolean editor_scroll_event(GtkWidget * widget, GdkEventScroll * event){

	if((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
		editor_do_mask_radius_scroll(widget, event);
	else if (event->state == 0)
		editor_do_image_zoom(widget, event);

	return FALSE;
}

static void editor_realize(GtkWidget * widget){

	editor_fit_image(EDITOR(widget));

}


/*
 * MISC
 * MISC
 *
 * */


static GdkPixbuf * editor_no_image_loaded_pixbuf(int width, int height){

	GdkPixbuf * pb;
	cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
	cairo_t * cr = cairo_create(surface);
	PangoLayout * layout = pango_cairo_create_layout(cr);
	PangoFontDescription * desc;
	double border_width = MIN(width, height) * 0.025;

	// fundo
	cairo_set_source_gray_tone(cr, 0.15);
	cairo_paint(cr);

	// font description
	desc = pango_font_description_new();
	pango_font_description_set_family(desc, "Roboto");
	pango_font_description_set_size(desc, MIN(width, height) * 0.05 * PANGO_SCALE);
	pango_layout_set_font_description(layout, desc);

	// layout
	pango_layout_set_justify(layout, TRUE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD);

	pango_layout_set_width(layout,  ((double)width  - border_width * 2) * PANGO_SCALE);
	pango_layout_set_height(layout, ((double)height - border_width * 2) * PANGO_SCALE);

	pango_layout_set_indent(layout, 4);

	pango_layout_set_markup(layout,
			"<span weight =\"bold\" style=\"italic\" size=\"x-large\">\tOops ...</span>\n\n\n"
			"<span weight=\"light\">Parece que não existe nenhuma imagem carregada. Meu deus, que problemão!\n\n"
			"<s>Pango é complicado demais, não recomendo ...</s></span>", -1);

	// draw
	cairo_set_source_gray_tone(cr, 0.85);
	cairo_move_to(cr, border_width, border_width);

	pango_cairo_update_layout(cr, layout);
	pango_cairo_show_layout(cr, layout);
	
	/* Agora que já está tudo desenhado, já não é necessário
	 * o `layout` nem o `cr` */
	// Preferi limpar o `font_description` aqui
	pango_font_description_free(desc);
	g_object_unref(layout);
	cairo_destroy(cr);

	pb = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
	cairo_surface_destroy(surface);

	return pb;
}
