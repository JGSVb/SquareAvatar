#pragma once
#include <cairo/cairo.h>
#include <gtk/gtk.h>

#define PI 3.14159265359
#define MAP_TO_RANGE(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define cairo_set_source_gray_tone(cr, b) cairo_set_source_rgb(cr, b, b, b)

void cairo_rounded_rectangle(cairo_t * cr, double x, double y, double width, double height, float radius);
GtkWidget * gtk_label_new_with_markup(gchar * markup);
