#!/usr/bin/env python3


from PIL import Image, ImageChops
from math import pi as PI

import cairo
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from avatar_editor import AvatarEditor



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

if __name__ == "__main__":
    main()
