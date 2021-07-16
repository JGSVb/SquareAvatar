#pragma once
#include <gtk/gtk.h>

#include "editor.h"

#define TYPE_SAVE_BUTTON (save_button_get_type())
#define SAVE_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_SAVE_BUTTON, SaveButton))

typedef struct _SaveButtonClass 	SaveButtonClass;
typedef struct _SaveButton 				SaveButton;
typedef struct _SaveButtonPrivate SaveButtonPrivate;

struct _SaveButtonClass {
	GtkButtonClass parent_class;
};

struct _SaveButton {
	GtkButton parent_instance;
	SaveButtonPrivate * priv;
};

struct _SaveButtonPrivate {
	Editor * editor;
	gulong handler_id_1;
	gulong handler_id_2;
};

GType save_button_get_type(void);
GtkWidget * save_button_new(Editor * editor);
Editor * save_button_get_editor(SaveButton * button);
