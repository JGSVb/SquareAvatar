#include <gtk/gtk.h>

#include "application.h"

int main(int argc, char ** argv){

	int status;

	GtkApplication * app;
	app = application_new();

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}
