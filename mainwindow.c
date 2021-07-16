#include "mainwindow.h"
#include "editor.h"
#include "utils.h"
#include "savebutton.h"

#include <gtk/gtk.h>

static void clicked(GtkButton * button, Editor * editor){
	(void)button;
	if(editor_get_loaded(editor))
		editor_unload_image(editor);
	else
		editor_load_image_from_filename(editor, "./avatar.png");
}

GtkWidget * main_window_new(GtkApplication * app){

	GtkWidget * header;
	header = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), MAIN_WINDOW_TITLE);

	
	GtkWidget * window;
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), MAIN_WINDOW_TITLE);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);

	gtk_window_set_titlebar(GTK_WINDOW(window), header);

	GtkWidget * editor;
	editor = editor_new();

	gtk_container_add(GTK_CONTAINER(window), editor);

	GtkWidget * save;
	save = save_button_new(EDITOR(editor));
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), save);

	GtkWidget * test;
	test = gtk_button_new_with_label("Botão de test :sunglasses:");
	g_signal_connect(G_OBJECT(test), "clicked", G_CALLBACK(clicked), editor);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), test);

	return window;

}
