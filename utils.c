#include "utils.h"
#include <cairo/cairo.h>
#include <math.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void cairo_rounded_rectangle(cairo_t * cr, double x, double y, double width, double height, float radius){
	// https://www.cairographics.org/samples/rounded_rectangle/

	double degrees = M_PI / 180.0;
	double P_radius = radius * MIN(width, height) / 2;

	cairo_new_sub_path(cr);
	cairo_arc(cr, x + width - P_radius, y + P_radius, P_radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr, x + width - P_radius, y + height - P_radius, P_radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr, x + P_radius, y + height - P_radius, P_radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr, x + P_radius, y + P_radius, P_radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);

}

GtkWidget * gtk_label_new_with_markup(gchar * markup){
	GtkWidget * w = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(w), markup);
	return w;
}
