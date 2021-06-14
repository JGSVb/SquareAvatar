import os

import cairo
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GdkPixbuf, Gdk

from utils import *

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

        self.__mask_size : float           = 120
        self.__mask_size_increment : int   = 5
        self.__border_radius : float           = 0.30
        self.__border_radius_increment : float = 0.05
        self.__mask_x = self.__mask_y      = 0

        self.__image_zoom : float = 0.10

        self.__image_pixbuf : GdkPixbuf.Pixbuf = None
        self.__image_width                 = self.__image_height                 = 0
        self.__image_width_ratio           = self.__image_height_ratio           = 0.0
        self.__image_surface_x             = self.__image_surface_y              = 0.0
        self.__image_surface_width         = self.__image_surface_height         = 0.0

        self.__widget_width = self.__widget_height = 0

        self.scroll_action_dispatcher = {
            Gdk.ModifierType.CONTROL_MASK: self.do_circle_radius_change,
            Gdk.ModifierType.SHIFT_MASK:   self.do_border_radius_change,
            0:                             self.do_image_zoom
        }

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

        self.__mask_size = min(self.__widget_width, self.__widget_height) / 2 - 40
        self.__mask_x, self.__mask_y = self.__widget_width / 2, self.__widget_height / 2

    def check_mask_before_set(self, ix, iy, iw, ih, mx, my, ms) -> bool:

        image_rect = gdk_rectangle_new(ix, iy, iw, ih)
        mask_rect = gdk_rectangle_new(mx - ms / 2, my - ms / 2, ms, ms)

        res = mask_rect.intersect(image_rect)

        if res[0] and res[1].equal(mask_rect):
            return True

        return False

    def update_widget_dimension(self):
        self.__widget_width = self.get_allocated_width()
        self.__widget_height = self.get_allocated_height()

    def update_mouse_pos(self):
        self.__mouse_x, self.__mouse_y = self.get_pointer()

    def button_event(self, widget : Gtk.Widget, event : Gdk.EventButton):

        self.__button_pressed = int(event.type) == int(Gdk.EventType.BUTTON_PRESS)
        self.__button_value = event.button

        self.update_mouse_pos()


    def do_circle_radius_change(self, event : Gdk.EventScroll):

        s = self.__mask_size

        if event.direction == Gdk.ScrollDirection.UP:
            s += self.__mask_size_increment
        elif event.direction == Gdk.ScrollDirection.DOWN:
            s -= self.__mask_size_increment
        else:
            return False

        if self.check_mask_before_set(self.__image_surface_x, self.__image_surface_y, self.__image_surface_width, self.__image_surface_height, self.__mask_x, self.__mask_y, s):
            self.__mask_size = s

            return True

        return False

    def do_image_zoom(self, event : Gdk.EventScroll):

        x, y = self.__image_surface_x, self.__image_surface_y

        if event.direction == Gdk.ScrollDirection.UP:

            wb, hb = self.__image_surface_width * ( 1 + self.__image_zoom ), self.__image_surface_height * ( 1 + self.__image_zoom )

            x -= (self.__mouse_x - self.__image_surface_x) * (self.__image_zoom)
            y -= (self.__mouse_y - self.__image_surface_y) * (self.__image_zoom)

        elif event.direction == Gdk.ScrollDirection.DOWN:

            wb, hb = self.__image_surface_width * ( 1 - self.__image_zoom ), self.__image_surface_height * ( 1 - self.__image_zoom )

            x -= (self.__mouse_x - self.__image_surface_x) * (1 - self.__image_zoom - 1)
            y -= (self.__mouse_y - self.__image_surface_y) * (1 - self.__image_zoom - 1)

        else:
            return False

        if self.check_mask_before_set(x, y, wb, hb, self.__mask_x, self.__mask_y, self.__mask_size):

            self.__image_surface_x = x
            self.__image_surface_y = y

            self.image_surface_scale(wb, hb)

            return True
        return False

    def do_border_radius_change(self, event : Gdk.EventScroll):
        if event.direction == Gdk.ScrollDirection.UP:
            self.__border_radius += self.__border_radius_increment
        elif event.direction == Gdk.ScrollDirection.DOWN:
            self.__border_radius -= self.__border_radius_increment
        else:
            return False

        # 0 <= border_radius <= 1
        self.__border_radius = max(min(self.__border_radius, 1), 0)

        return True

    def scroll_event(self, widget : Gtk.Widget, event : Gdk.EventScroll):

        self.update_mouse_pos()

        if event.state in self.scroll_action_dispatcher and self.scroll_action_dispatcher[event.state](event):
            self.queue_draw()

    def mouse_move_event(self, widget : Gtk.Widget, event : Gdk.EventMotion):

        if self.__button_pressed == 1 and self.__button_value == 2:

            x = self.__image_surface_x + event.x - self.__mouse_x
            y = self.__image_surface_y + event.y - self.__mouse_y

            if self.check_mask_before_set(x, y, self.__image_surface_width, self.__image_surface_height, self.__mask_x, self.__mask_y, self.__mask_size):
                self.__image_surface_x = x
                self.__image_surface_y = y

                self.queue_draw()

            self.update_mouse_pos()

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
            cairo_rounded_square(ctx, self.__mask_x, self.__mask_y,
                                      self.__mask_size,
                                      self.__border_radius)
            ctx.fill()

            # Imagem
            ctx.set_operator(cairo.OPERATOR_MULTIPLY)
            ctx.set_source_surface(self.__image_surface, self.__image_surface_x, self.__image_surface_y)
            ctx.paint()
