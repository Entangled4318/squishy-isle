"""Full-size area maps for the ROM.

Each area returns two layers and a collision grid:
  ground  - terrain, decor and the lower part of every object (under Pip)
  overlay - the upper part of tall objects (tree tops, roofs), over Pip
  solid   - 8x8 px cells Pip cannot enter
The split height equals Pip's height, and tall objects block that same
height above their base, so Pip is covered by a tree top only when he is
really behind it.
"""
import math

import numpy as np

from gba import new, blit, rgb15
from palette import C
import world
from world import (render_terrain, MEADOW_KINDS, shape_labels, Capsule, Oval, SmoothUnion, SDF,
                   tree, cottage, bush, tree_shadow)
import props

PIP_H = 26          # sprite height that must be covered correctly
CELL = 8


class Area:
    def __init__(self, name, w, h):
        self.name, self.w, self.h = name, w, h
        self.ground = None
        self.overlay = new(w, h)
        self.solid = np.zeros((h // CELL, w // CELL), bool)
        self.spawn = (w // 2, h // 2)
        self.spots = []          # container spots (x, y of the sprite's bottom centre)
        self.doors = []          # (x, y, w, h, target)

    def block(self, x0, y0, x1, y1):
        """Mark pixel rect [x0,x1) x [y0,y1) solid (rounded to cells)."""
        c0, r0 = max(0, int(x0) // CELL), max(0, int(y0) // CELL)
        c1, r1 = min(self.w // CELL, -(-int(x1) // CELL)), min(self.h // CELL, -(-int(y1) // CELL))
        self.solid[r0:r1, c0:c1] = True

    def place(self, img, x, y, block_w=None, block_inset=0, tall=None):
        """Put an object with its top-left at (x, y). Pixels higher than
        PIP_H above the base go to the overlay. block_w: width of the solid
        footprint (centred), block height = min(PIP_H, object height)."""
        h, w = img.shape[:2]
        rows = np.nonzero(img[..., 3].any(axis=1))[0]
        if not len(rows):
            return
        base = y + rows[-1] + 1
        split = base - PIP_H
        for yy in range(h):
            Y = y + yy
            if Y < 0 or Y >= self.h:
                continue
            target = self.overlay if Y < split else self.ground
            row = img[yy:yy + 1]
            blit(target, row, x, Y)
        if block_w:
            top = base - min(PIP_H if tall is None else tall, rows[-1] - rows[0] + 1)
            cx = x + w / 2
            self.block(cx - block_w / 2, top + block_inset, cx + block_w / 2, base)

    def decor(self, img, x, y):
        blit(self.ground, img, x, y)


class Stream(SDF):
    def __init__(self, x):
        self.x = x

    def d(self, X, Y):
        return (self.x + 4 * np.sin(Y / 23.0) + 2 * np.sin(Y / 7.0)) - X


def bridge(h=28):
    """Wooden bridge across the stream (planks run horizontally)."""
    w = 44
    img = new(w, h)
    for y in range(h):
        for x in range(w):
            if y in (0, h - 1):
                c = C['wd_ink']
            elif y in (1, h - 2):
                c = C['wd_dk2']
            elif x % 6 == 5:
                c = C['wd_dk']
            else:
                c = C['wd_lt'] if (x // 6) % 2 == 0 else C['wd_base']
            img[y, x, :3] = c
            img[y, x, 3] = 255
    # rails
    for x in range(w):
        for y in (3, h - 4):
            img[y, x, :3] = C['wd_dk2'] if x % 6 else C['wd_ink']
    return img


def meadow():
    W_, H_ = 480, 320
    a = Area('meadow', W_, H_)
    path = SmoothUnion(10,
                       Capsule(232, 90, 232, 196, 15),
                       Capsule(232, 200, 470, 200, 15),
                       Capsule(232, 200, 84, 200, 13),
                       Capsule(84, 200, 84, 300, 13))
    pond = Oval(362, 96, 50, 32, n=2.3)
    lab = shape_labels(W_, H_, 'g', [('p', path), ('w', pond), ('w', Stream(454))])
    ground, lab = render_terrain(None, MEADOW_KINDS, lab=lab)
    a.ground = ground
    # water is solid; the bridge opens the stream again
    water = lab == 'w'
    for r in range(H_ // CELL):
        for c in range(W_ // CELL):
            if water[r * CELL:(r + 1) * CELL, c * CELL:(c + 1) * CELL].mean() > 0.3:
                a.solid[r, c] = True

    # ---- ground decor
    for name, x, y in (
            ('patch_pink', 150, 120), ('patch_yellow', 40, 130), ('patch_mix', 300, 160),
            ('patch_pink', 180, 270), ('patch_yellow', 330, 280), ('patch_mix', 20, 250),
            ('patch_pink', 410, 250), ('patch_yellow', 270, 130), ('tuft', 120, 150),
            ('tuft', 380, 170), ('tuft_small', 200, 240), ('tuft', 60, 90), ('clover', 150, 230),
            ('clover', 400, 140), ('tuft_small', 300, 90), ('tuft', 250, 270), ('patch_mix', 110, 290),
            ('tuft_small', 30, 190), ('clover', 360, 240), ('tuft', 180, 150), ('patch_pink', 370, 300)):
        a.decor(world.decor(name), x, y)
    for (x, y, f) in ((336, 80, False), (372, 102, True), (386, 78, False), (344, 108, False)):
        a.decor(world.lily_pad(f), x, y)
    a.decor(world.picnic_blanket(), 292, 232)
    a.decor(world.door_mat(), 226, 99)

    # ---- trees (big first so small ones overlap nicely); shadows go under
    trees = [
        ('green', True, -14, -34), ('green', False, 34, -12), ('green', False, 70, -20),
        ('blossom', True, 108, -40), ('blossom', False, 156, -16),
        ('green', True, 276, -38), ('green', False, 324, -14), ('blossom', False, 358, -24),
        ('blossom', True, 392, -36), ('green', False, 424, 6),
        ('green', True, -20, 26), ('blossom', False, -10, 84), ('green', True, -22, 130),
        ('blossom', False, 10, 286), ('green', True, 150, 284), ('blossom', False, 196, 292),
        ('blossom', True, 236, 290), ('blossom', False, 366, 290), ('green', True, 404, 280),
        ('blossom', False, 30, 226), ('blossom', False, 118, 236), ('green', False, 404, 120),
    ]
    for kind, big, x, y in trees:
        t = tree(kind, big)
        a.decor(tree_shadow(t.shape[1]), x, y + t.shape[0] - 6)
    for kind, big, x, y in sorted(trees, key=lambda t: t[3] + tree(t[0], t[1]).shape[0]):
        t = tree(kind, big)
        a.place(t, x, y, block_w=t.shape[1] - 8)

    # ---- cottage, garden, props
    a.place(cottage(), 200, 34, block_w=48)
    a.doors.append((224, 90, 16, 10, 'home'))
    a.place(world.decor('tulips'), 122, 70)
    a.place(world.decor('tulips'), 138, 72)
    a.place(world.decor('tulips'), 154, 70)
    a.place(world.fence(3), 118, 82, block_w=48, tall=10)
    a.place(world.mailbox(), 268, 74, block_w=10, tall=8)
    a.place(world.signpost(world.mini_shell()), 414, 164, block_w=10, tall=8)
    a.place(world.basket(), 322, 230, block_w=12, tall=8)
    for kind, x, y in (('green', 180, 104), ('blossom', 300, 116), ('green', 24, 170), ('blossom', 150, 244),
                       ('green', 330, 140)):
        a.place(bush(kind), x, y, block_w=12, tall=8)
    for big, x, y in ((True, 312, 150), (False, 176, 178), (True, 418, 300), (False, 60, 60)):
        r = world.rock(big)
        a.place(r, x, y, block_w=r.shape[1] - 2, tall=6)
    for x, y in ((56, 112), (388, 150), (262, 300)):
        a.decor(world.mushrooms(), x, y)
    for x, y in ((200, 102), (250, 104), (262, 212), (196, 214), (110, 212), (330, 186)):
        a.place(world.decor('tulips'), x, y, block_w=10, tall=6)
    for name, x, y in (('patch_mix', 60, 150), ('patch_yellow', 350, 196), ('patch_pink', 124, 170),
                       ('tuft', 220, 250), ('patch_mix', 420, 226), ('tuft', 100, 110)):
        a.decor(world.decor(name), x, y)
    a.place(world.reeds(), 318, 74)
    a.place(world.reeds(), 402, 104)
    br = bridge()
    a.decor(br, 436, 186)
    a.solid[186 // CELL:(186 + 28) // CELL, 432 // CELL:] = False
    a.solid[186 // CELL, 432 // CELL:] = True          # rails
    a.solid[(186 + 27) // CELL, 432 // CELL:] = True
    a.block(472, 0, 480, 320)                          # map edge beyond the stream

    a.spawn = (232, 124)
    a.spots = [(64, 160), (150, 212), (286, 180), (200, 150), (120, 120), (330, 210), (400, 220),
               (270, 260), (100, 260), (380, 166)]
    return a


def collision_preview(a):
    img = a.ground.copy()
    ov = a.overlay[..., 3] > 0
    img[ov] = a.overlay[ov]
    for r in range(a.solid.shape[0]):
        for c in range(a.solid.shape[1]):
            if a.solid[r, c]:
                sub = img[r * CELL:(r + 1) * CELL, c * CELL:(c + 1) * CELL, :3].astype(int)
                sub[:] = (sub * 0.5 + np.array([255, 60, 90]) * 0.5).astype(int)
                img[r * CELL:(r + 1) * CELL, c * CELL:(c + 1) * CELL, :3] = sub
    for (x, y) in a.spots:
        img[y - 2:y + 2, x - 2:x + 2, :3] = (40, 40, 255)
    return img
