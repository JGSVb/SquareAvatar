#include <gtk/gtk.h>

#include "application.h"
#include "mainwindow.h"

static void activate(GtkApplication * app, gpointer user_data);

GtkApplication * application_new(){

	GtkApplication * app;
	app = gtk_application_new("dev.vieira.square_avatar", G_APPLICATION_FLAGS_NONE);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	return app;
}

static void activate(GtkApplication * app, gpointer user_data){

	(void)user_data;

	GtkWidget * window;
	window = main_window_new(app);
	gtk_widget_show_all(window);

	return;
}
