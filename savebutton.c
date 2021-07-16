#include <gtk/gtk.h>
#include "savebutton.h"
#include "editor.h"

G_DEFINE_TYPE_WITH_PRIVATE(SaveButton, save_button, GTK_TYPE_BUTTON)

enum {
	PROP_EDITOR = 1,
	PROP_COUNT,
};

static GParamSpec * props[PROP_COUNT] = {NULL, };
static void save_button_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void save_button_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void save_button_clicked(GtkButton * button);
static void save_button_editor_loaded_unloaded(Editor * editor, SaveButton * button);
static void save_button_set_editor(SaveButton * save_button, Editor * editor);
static gboolean save_button_get_editor_loaded(SaveButton * button);

/*
void save_button_destroy(GtkWidget * widget){

	SaveButtonPrivate * priv = SAVE_BUTTON(widget)->priv;

	if((priv->handler_id_1 != 0) && (priv->handler_id_2 != 0)){
		g_signal_handler_disconnect(priv->editor, priv->handler_id_1);
		g_signal_handler_disconnect(priv->editor, priv->handler_id_2);
	}

	gtk_widget_destroy(widget);

}
*/

void save_button_class_init(SaveButtonClass * klass){

	GObjectClass * gobject_class = G_OBJECT_CLASS(klass);
	GtkButtonClass * button_class = GTK_BUTTON_CLASS(klass);

	gobject_class->set_property = save_button_set_property;
	gobject_class->get_property = save_button_get_property;



	props[PROP_EDITOR] = g_param_spec_object(
			"editor",
			NULL,
			NULL,
			TYPE_EDITOR,
			G_PARAM_READWRITE);

	g_object_class_install_properties(gobject_class, PROP_COUNT, props);

	button_class->clicked = save_button_clicked;

}

void save_button_init(SaveButton * self){

	SaveButtonPrivate * priv = save_button_get_instance_private(self);
	self->priv = priv;

	GtkButton * button = GTK_BUTTON(self);

	gtk_button_set_image(button, gtk_image_new_from_icon_name("document-save-symbolic", GTK_ICON_SIZE_BUTTON));

}

GtkWidget * save_button_new(Editor * editor){

	SaveButton * save_button = g_object_new(TYPE_SAVE_BUTTON, NULL);
	save_button_set_editor(save_button, editor);

	return GTK_WIDGET(save_button);
}

static void save_button_set_editor(SaveButton * save_button, Editor * editor){

	GtkWidget * widget = GTK_WIDGET(save_button);
	SaveButtonPrivate * priv = save_button->priv;

	g_return_if_fail(priv->editor == NULL);

	priv->editor = editor;

	gtk_widget_set_sensitive(widget, editor_get_loaded(editor));

	priv->handler_id_1 = g_signal_connect(G_OBJECT(editor), "image-loaded", G_CALLBACK(save_button_editor_loaded_unloaded), save_button);
	priv->handler_id_2 = g_signal_connect(G_OBJECT(editor), "image-unloaded", G_CALLBACK(save_button_editor_loaded_unloaded), save_button);

}

static void save_button_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec){

	SaveButton * save_button = SAVE_BUTTON(object);
	SaveButtonPrivate * priv = save_button->priv;

	switch(prop_id){

		case(PROP_EDITOR):
			g_value_set_object(value, priv->editor);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}

}

static void save_button_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec){

	SaveButton * save_button = SAVE_BUTTON(object);
	SaveButtonPrivate * priv = save_button->priv;

	switch(prop_id){

		case(PROP_EDITOR):
			priv->editor = EDITOR(g_value_get_object(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}


}

static void save_button_clicked(GtkButton * button){

	GtkWidget * dialog;
	GtkFileFilter * filter;
	GtkFileChooser * file_chooser;

	dialog = gtk_file_chooser_dialog_new(
			"Salvar Avatar",
			GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"Salvar", GTK_RESPONSE_OK,
			"Cancelar", GTK_RESPONSE_CANCEL,
			NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Imagens");
	gtk_file_filter_add_mime_type(filter, "image/*");

	file_chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_do_overwrite_confirmation(file_chooser, TRUE);
	gtk_file_chooser_add_filter(file_chooser, filter);

	guint response = gtk_dialog_run(GTK_DIALOG(dialog));

	switch(response){
		case(GTK_RESPONSE_OK):
		case(GTK_RESPONSE_CANCEL):
			break;
	}

	gtk_widget_destroy(dialog);

}

Editor * save_button_get_editor(SaveButton * button){

	return button->priv->editor;

}

static void save_button_editor_loaded_unloaded(Editor * editor, SaveButton * button){
	(void)editor;
	gtk_widget_set_sensitive(GTK_WIDGET(button), save_button_get_editor_loaded(button));

}

static gboolean save_button_get_editor_loaded(SaveButton * button){
	return editor_get_loaded(save_button_get_editor(button));
}
