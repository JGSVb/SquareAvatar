#!/usr/bin/env python3

import os

from PIL import Image, ImageChops
from math import pi as PI

import cairo
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GObject, GdkPixbuf, Gdk



def mask_image(mask_image : Image.Image, image : Image.Image):

    mask_image = mask_image.convert("L")
    image = image.convert("RGBA")

    r,g,b,alpha = image.split()
    alpha = ImageChops.multiply(alpha, mask_image)

    out = Image.merge("RGBA", (r,g,b,alpha))

    return out


def mask_image_from_filename(mask_filename : str, image_filename : str):

    m = Image.open(mask_filename)
    i = Image.open(image_filename)

    return mask_image(m, i)


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

def cairo_rounded_rectangle(ctx : cairo.Context, x : float, y : float, width : float, height : float, radius : float):
    # https://www.cairographics.org/samples/rounded_rectangle/
    # Cairo é complicado então utilizei esse código :thumbsup:

    degrees = PI / 180

    x -= width / 2
    y -= height / 2

    ctx.new_sub_path()
    ctx.arc(x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees)
    ctx.arc(x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees)
    ctx.arc(x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees)
    ctx.arc(x + radius, y + radius, radius, 180 * degrees, 270 * degrees)
    ctx.close_path()

def cairo_relative_rounded_rectangle(ctx : cairo.Context, x : float, y : float, width : float, height : float, radius : float):
    cairo_rounded_square(ctx, x, y, width, height, min(width, height) * radius / 2)

def cairo_relative_rounded_square(ctx : cairo.Context, x : float, y : float, size : float, radius : float):
    cairo_rounded_rectangle(ctx, x, y, size, size, size * radius / 2)

def gdk_rectangle_new(x : int, y : int, width : int, height : int) -> Gdk.Rectangle:
    r = Gdk.Rectangle()
    r.x = x
    r.y = y
    r.width = width
    r.height = height

    return r

class AvatarEditor(Gtk.DrawingArea):
    def __init__(self, *args, **kwargs):
        Gtk.DrawingArea.__init__(self, can_focus = True)

        self.add_events(
            Gdk.EventMask.BUTTON_PRESS_MASK |
            Gdk.EventMask.BUTTON_RELEASE_MASK |
            Gdk.EventMask.POINTER_MOTION_MASK |
            Gdk.EventMask.SCROLL_MASK |
            Gdk.EventMask.KEY_PRESS_MASK
        )

        self.__avatar_image : str                 = ""
        self.__image_surface : cairo.ImageSurface = None

        self.__button_pressed : bool = False
        self.__button_value : int    = -1
        self.__mouse_x : float       = -1
        self.__mouse_y : float       = -1

        self.__circle_radius : float           = 120
        self.__circle_radius_increment : int   = 5
        self.__border_radius : float           = 0.30
        self.__border_radius_increment : float = 0.05
        self.__circle_x = self.__circle_y      = 0

        self.__image_zoom : float = 0.10

        self.__image_pixbuf : GdkPixbuf.Pixbuf = None
        self.__image_width                 = self.__image_height                 = 0
        self.__image_width_ratio           = self.__image_height_ratio           = 0.0
        self.__image_surface_x             = self.__image_surface_y              = 0.0
        self.__image_surface_width         = self.__image_surface_height         = 0.0

        self.__widget_width = self.__widget_height = 0

        self.connect("button-press-event", self.button_event)
        self.connect("button-release-event", self.button_event)
        self.connect("motion-notify-event", self.mouse_move_event)
        self.connect("scroll-event", self.scroll_event)

        self.connect("draw", self.draw)

    def get_avatar_image(self):
        return self.__avatar_image

    def set_avatar_image_from_filename(self, filename):

        if not os.path.isfile(filename):
            raise FileNotFoundError

        self.__avatar_image = filename

        self.__set_image_pixbuf(GdkPixbuf.Pixbuf.new_from_file(filename))
        self.__set_image_surface(Gdk.cairo_surface_create_from_pixbuf(self.__image_pixbuf, 0, None), 0, 0)

    def __set_image_pixbuf(self, pixbuf : GdkPixbuf.Pixbuf):

        self.__image_pixbuf = pixbuf

        self.__image_width = self.__image_pixbuf.get_width()
        self.__image_height = self.__image_pixbuf.get_height()

        self.__image_width_ratio = self.__image_width / self.__image_height
        self.__image_height_ratio = self.__image_height / self.__image_width

    def __set_image_surface(self, surface : cairo.Surface, x : float, y : float):

        self.__image_surface = surface

        self.__image_surface_width = self.__image_surface.get_width()
        self.__image_surface_height = self.__image_surface.get_height()

        self.__image_surface_x = x
        self.__image_surface_y = y

    def image_surface_scale(self, width : float, height : float) -> (float, float):

        scaled = self.__image_pixbuf.copy()
        scaled = scaled.scale_simple(width, height, GdkPixbuf.InterpType.TILES)
        surface = Gdk.cairo_surface_create_from_pixbuf(scaled, 0, None)

        wb, hb = self.__image_surface_width, self.__image_surface_height

        self.__set_image_surface(surface, self.__image_surface_x, self.__image_surface_y)

        return wb, hb

    def image_surface_proportional_scale(self, fac : float) -> (float, float):
        return self.image_surface_scale(self.__image_surface_width * fac, self.__image_surface_height * fac)

    def center_image_surface(self):

        scaled = self.__image_pixbuf.copy()

        # Imagem é horizontal
        if self.__image_width > self.__image_height:
            width = self.__widget_width
            height = width * self.__image_height_ratio
            self.__image_surface_y = self.__widget_height / 2 - height / 2

        # Imagem é vertical ou quadrada
        else:
            height = self.__widget_height
            width = height * self.__image_width_ratio
            self.__image_surface_x = self.__widget_width / 2 - width / 2

        self.image_surface_scale(width, height)

    def do_realize(self):
        Gtk.DrawingArea.do_realize(self)

        self.update_widget_dimension()
        self.center_image_surface()

        self.__circle_radius = min(self.__widget_width, self.__widget_height) / 2 - 40
        self.__circle_x, self.__circle_y = self.__widget_width / 2, self.__widget_height / 2

    def update_widget_dimension(self):
        self.__widget_width = self.get_allocated_width()
        self.__widget_height = self.get_allocated_height()

    def update_mouse_pos(self):
        self.__mouse_x, self.__mouse_y = self.get_pointer()

    def button_event(self, widget : Gtk.Widget, event : Gdk.EventButton):

        self.__button_pressed = int(event.type) == int(Gdk.EventType.BUTTON_PRESS)
        self.__button_value = event.button

        self.update_mouse_pos()


    """
    Ações relacionadas com evento de scrolling:

        Aumentar o radio do círculo;
        dar zoom na imagem;
        aumentar o raio da borda.
    """

    def do_circle_radius_change(self, event : Gdk.EventScroll):
        if event.direction == Gdk.ScrollDirection.UP:
            self.__circle_radius += self.__circle_radius_increment
        elif event.direction == Gdk.ScrollDirection.DOWN:
            self.__circle_radius -= self.__circle_radius_increment

    def do_image_zoom(self, event : Gdk.EventScroll):
        if event.direction == Gdk.ScrollDirection.UP:

            wb, hb = self.image_surface_proportional_scale(1 + self.__image_zoom)

            self.__image_surface_x -= (self.__mouse_x - self.__image_surface_x) * (self.__image_zoom)
            self.__image_surface_y -= (self.__mouse_y - self.__image_surface_y) * (self.__image_zoom)

        elif event.direction == Gdk.ScrollDirection.DOWN:

            wb, hb = self.image_surface_proportional_scale(1 - self.__image_zoom)

            self.__image_surface_x -= (self.__mouse_x - self.__image_surface_x) * (1 - self.__image_zoom - 1)
            self.__image_surface_y -= (self.__mouse_y - self.__image_surface_y) * (1 - self.__image_zoom - 1)

    def do_border_radius_change(self, event : Gdk.EventScroll):
        if event.direction == Gdk.ScrollDirection.UP:
            self.__border_radius += self.__border_radius_increment

        elif event.direction == Gdk.ScrollDirection.DOWN:
            self.__border_radius -= self.__border_radius_increment

        # 0 <= border_radius <= 1
        self.__border_radius = max(min(self.__border_radius, 1), 0)

    def scroll_event(self, widget : Gtk.Widget, event : Gdk.EventScroll):

        self.update_mouse_pos()

        if event.state == Gdk.ModifierType.CONTROL_MASK:
            self.do_circle_radius_change(event)

        elif event.state == Gdk.ModifierType.SHIFT_MASK:
            self.do_border_radius_change(event)

        elif event.state == 0:
            self.do_image_zoom(event)

        self.queue_draw()

    def mouse_move_event(self, widget : Gtk.Widget, event : Gdk.EventMotion):

        if self.__button_pressed == 1 and self.__button_value == 2:

            x = self.__image_surface_x + event.x - self.__mouse_x
            y = self.__image_surface_y + event.y - self.__mouse_y

            # TODO: Ainda não funciona bem, resolver depois da escola

            image_rect = gdk_rectangle_new(x, y, self.__image_surface_width, self.__image_surface_height)
            mask_rect = gdk_rectangle_new(self.__circle_x, self.__circle_y, self.__circle_radius, self.__circle_radius)

            if mask_rect.intersect(image_rect)[1].equal(mask_rect):

                self.__image_surface_x = x
                self.__image_surface_y = y

            self.update_mouse_pos()
            self.queue_draw()

    def draw(self, widget : Gtk.Widget, ctx : cairo.Context):

        self.update_widget_dimension()

        if self.__image_surface:

            # Fundo
            ctx.set_source_rgb(0.1, 0.1, 0.1)
            ctx.rectangle(0, 0, self.__widget_width, self.__widget_height)
            ctx.fill()

            # Quadrado de escurecimento
            ctx.set_source_rgba(0.20, 0.23, 0.28)
            ctx.rectangle(self.__image_surface_x, self.__image_surface_y, self.__image_surface_width, self.__image_surface_height)
            ctx.fill()

            # Buraco no quadrado
            ctx.set_source_rgb(1, 1, 1)
            cairo_relative_rounded_square(ctx, self.__circle_x, self.__circle_y,
                                          self.__circle_radius,
                                          self.__border_radius)
            ctx.fill()

            # Imagem
            ctx.set_operator(cairo.OPERATOR_MULTIPLY)
            ctx.set_source_surface(self.__image_surface, self.__image_surface_x, self.__image_surface_y)
            ctx.paint()


class MainWindow(Gtk.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, title="Square Avatar", **kwargs)

        self.set_default_size(500, 300)

        self.drawing = AvatarEditor()
        self.drawing.set_avatar_image_from_filename("/home/asv/Imagens/fotos_de_perfil/fu4xjtknusw31 (1).jpg")
        self.add(self.drawing)


class Application(Gtk.Application):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            application_id = "dev.vieira.square_gif",
            **kwargs
        )


    def do_activate(self):

        window = MainWindow(application = self)
        window.show_all()


def main():

    app = Application()
    app.run()
    # out = mask_image_from_filename(CONFIG.mask_filename, os.path.join(CONFIG.input_folder, file))
    # out.show()

if __name__ == "__main__":
    main()
