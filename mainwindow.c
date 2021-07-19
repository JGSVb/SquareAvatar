#include "mainwindow.h"
#include "cairo.h"
#include "editor.h"
#include "utils.h"

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>

static void load_change_cb(Editor * editor, GtkButton * button){
	gtk_widget_set_sensitive(GTK_WIDGET(button), editor_get_loaded(editor));
}

static void open_button_clicked(GtkButton * button, gpointer * data){

	(void)button;

	GtkDialog * dialog = data[0];
	Editor * editor = data[1];

	switch(gtk_dialog_run(dialog)){

		case(GTK_RESPONSE_OK):
			editor_load_image_from_filename(editor, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
			break;

		case(GTK_RESPONSE_CANCEL):
			break;
	}

	gtk_widget_hide(GTK_WIDGET(dialog));

}

static void save_button_clicked(GtkButton * button, gpointer * data){

	(void)button;

	GtkDialog * dialog = data[0];
	Editor * editor = data[1];

	GtkDialog 		 * export_dialog = data[2];
	GtkSpinButton  * size_spin = data[3];
	GtkCheckButton * inscribed_check = data[4];

	GdkPixbuf * out;

	if((gtk_dialog_run(dialog) == GTK_RESPONSE_OK) && (gtk_dialog_run(export_dialog) == GTK_RESPONSE_OK)){

		int size = gtk_spin_button_get_value_as_int(size_spin);
		gboolean inscribed_square = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(inscribed_check));

		out = editor_get_final_image(editor, size, inscribed_square);
		gdk_pixbuf_save(out, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)), "png", NULL, NULL);
		g_object_unref(out);

	}

	gtk_widget_hide(GTK_WIDGET(dialog));
	gtk_widget_hide(GTK_WIDGET(export_dialog));

}

static void remove_button_clicked(GtkButton * button, Editor * editor){
	(void)button;
	editor_unload_image(editor);
}


GtkWidget * main_window_new(GtkApplication * app){

	GObject		 * window;
	GObject		 * save_button;
	GObject		 * open_button;
	GObject		 * remove_button;
	GObject 	 * image_open_dialog;
	GObject 	 * image_save_dialog;
	GObject		 * export_dialog;
	GObject		 * size_spin;
	GObject		 * inscribed_check;
	GtkBuilder * builder;
	GtkWidget  * editor;
	gpointer 	 * user_data;

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "mainwindow.ui", NULL);

	image_open_dialog = gtk_builder_get_object(builder, "image-open-dialog");
	image_save_dialog = gtk_builder_get_object(builder, "image-save-dialog");
	open_button = gtk_builder_get_object(builder, "open-button");
	save_button = gtk_builder_get_object(builder, "save-button");
	export_dialog = gtk_builder_get_object(builder, "export-options-dialog");
	remove_button = gtk_builder_get_object(builder, "remove-button");
	size_spin = gtk_builder_get_object(builder, "size-spin");
	inscribed_check = gtk_builder_get_object(builder, "inscribed-check");
	window = gtk_builder_get_object(builder, "window");

	gtk_window_set_application(GTK_WINDOW(window), app);


	// NOTE(save_button): desativar
	// NOTE(remove_button): desativar
	{
		gtk_widget_set_sensitive(GTK_WIDGET(save_button), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(remove_button), FALSE);
	}

	editor = editor_new();
	gtk_container_add(GTK_CONTAINER(window), editor);

	// NOTE(open_button): Conectar sinais
	{
		user_data = malloc(sizeof(gpointer) * 2);
		user_data[0] = image_open_dialog;
		user_data[1] = editor;
		g_signal_connect(open_button, "clicked", G_CALLBACK(open_button_clicked),
				user_data);
	}

	// NOTE(save_button): Conectar sinais
	{
		user_data = malloc(sizeof(gpointer) * 5);
		user_data[0] = image_save_dialog;
		user_data[1] = editor;
		user_data[2] = export_dialog;
		user_data[3] = size_spin;
		user_data[4] = inscribed_check;
		g_signal_connect(save_button, "clicked", G_CALLBACK(save_button_clicked),
				user_data);

		g_signal_connect(editor, "image-unloaded", G_CALLBACK(load_change_cb), save_button);
		g_signal_connect(editor, "image-loaded", G_CALLBACK(load_change_cb),   save_button);
	}

	// NOTE(remove_button): Conectar sinais
	{
		g_signal_connect(editor, "image-unloaded", G_CALLBACK(load_change_cb), remove_button);
		g_signal_connect(editor, "image-loaded", G_CALLBACK(load_change_cb),   remove_button);
		g_signal_connect(remove_button, "clicked", G_CALLBACK(remove_button_clicked), editor);
	}


	return GTK_WIDGET(window);

}
