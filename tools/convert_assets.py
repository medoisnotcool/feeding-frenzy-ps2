#!/usr/bin/env python3
"""Prepare only the art used by this PS2 port as power-of-two RGBA textures.
Run from the project root. Requires Pillow: python3 -m pip install Pillow
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets" / "source"
OUT = ROOT / "assets" / "converted"
OUT.mkdir(parents=True, exist_ok=True)

def fit_canvas(im, size):
    im = im.convert("RGBA")
    im.thumbnail(size, Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.alpha_composite(im, ((size[0]-im.width)//2, (size[1]-im.height)//2))
    return canvas

def background(filename, outname):
    im = Image.open(SRC / filename).convert("RGBA")
    # The source images are 4:3-ish; a 256x256 texture is inexpensive in GS VRAM.
    im = im.resize((256, 256), Image.Resampling.LANCZOS)
    im.save(OUT / (outname + ".preview.png"))
    im.tobytes()
    (OUT / (outname + ".rgba")).write_bytes(im.tobytes())

def sprite(filename, crop, size, outname):
    im = Image.open(SRC / filename).convert("RGBA").crop(crop)
    im = fit_canvas(im, size)
    im.save(OUT / (outname + ".preview.png"))
    (OUT / (outname + ".rgba")).write_bytes(im.tobytes())

background("mainMenuBG.png", "menu_bg")
background("ground1.png", "reef_bg")
background("lost_city.png", "city_bg")
# The following frame rectangles match the original project's sprite-sheet layout.
sprite("fish_ferenzy.png", (0, 0, 208, 86), (256, 128), "logo")
# Player: first frame of angel_fish, row 1 (original main.cpp uses a 15 x 4 sheet).
sprite("angel_fish.png", (2, 107, 125, 211), (128, 128), "player")
# Minnow: first swimming frame from the 15 x 2 sheet.
sprite("Minow.png", (2, 1, 65, 48), (64, 64), "prey")
# Lionfish: original NPC sheet uses a 14 x 4 grid; use its first frame.
sprite("lionfish.png", (2, 2, 182, 161), (128, 128), "predator")
print("Converted PS2 art textures in", OUT)
