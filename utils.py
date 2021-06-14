from math import pi as PI

import cairo
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk

def image_chooser_dialog(title = "Escolher Imagem", parent = None):

    dialog = Gtk.FileChooserDialog(
        title = title,
        parent = parent,
        action = Gtk.FileChooserAction.OPEN)

    dialog.add_buttons(
        Gtk.STOCK_CANCEL,
        Gtk.ResponseType.CANCEL,
        Gtk.STOCK_OPEN,
        Gtk.ResponseType.OK
    )

    file_filter = Gtk.FileFilter()
    file_filter.set_name("Imagens")
    file_filter.add_mime_type("image/*")

    dialog.add_filter(file_filter)

    dialog.run()
    filename = dialog.get_filename()
    dialog.destroy()
    return filename

def __cairo_rounded_rectangle(ctx : cairo.Context, x : float, y : float, width : float, height : float, radius : float):
    # https://www.cairographics.org/samples/rounded_rectangle/
    # Cairo é complicado então utilizei esse código :thumbsup:

    degrees = PI / 180

    ctx.new_sub_path()
    ctx.arc(x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees)
    ctx.arc(x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees)
    ctx.arc(x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees)
    ctx.arc(x + radius, y + radius, radius, 180 * degrees, 270 * degrees)
    ctx.close_path()

def cairo_rounded_rectangle(ctx : cairo.Context, x : float, y : float, width : float, height : float, radius : float):
    __cairo_rounded_square(ctx, x, y, width, height, min(width, height) * radius / 2)

def cairo_rounded_square(ctx : cairo.Context, x : float, y : float, size : float, radius : float):
    __cairo_rounded_rectangle(ctx, x - size / 2, y - size / 2, size, size, size * radius / 2)

def gdk_rectangle_new(x : int, y : int, width : int, height : int) -> Gdk.Rectangle:
    r = Gdk.Rectangle()
    r.x = x
    r.y = y
    r.width = width
    r.height = height

    return r
