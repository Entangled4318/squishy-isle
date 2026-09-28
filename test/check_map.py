"""Checks the meadow collision grid with the game's own feet test (viewer.c
blocked(): a 11x6 px feet box). Usage: check_map.py OUTDIR"""
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))
import areas  # noqa: E402

OUT = sys.argv[1]
FEET_W, FEET_H = 5, 5
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


a = areas.meadow()
C = areas.CELL


def solid(x, y):
    if x < 0 or y < 0 or x >= a.w or y >= a.h:
        return True
    return a.solid[y // C, x // C]


def blocked(x, y):
    return solid(x - FEET_W, y - FEET_H) or solid(x + FEET_W, y - FEET_H) or solid(x - FEET_W, y) or solid(x + FEET_W, y)


check(C == 4, f'collision cells are 4 px ({C})')
# pen's bottom-left post: Pip can walk north to south past it on the left
px0, py1 = 56, 182
lanes = [x for x in range(8, px0) if all(not blocked(x, y) for y in range(py1 - 40, py1 + 16))]
check(len(lanes) > 0, f'a free north-south lane left of the pen ({lanes[:1]}..{lanes[-1:]})')
# the stream shore is straight and walkable from the north edge to the south edge
shore = [x for x in range(400, a.w) if all(not blocked(x, y) for y in range(FEET_H + 1, a.h - 8))]
check(len(shore) > 0, f'Pip can walk the whole shore north to south (lane x {shore[:1]}..{shore[-1:]})')
# rocks are walk-through
for big, x, y in ((True, 312, 150), (False, 196, 204), (True, 418, 300), (False, 60, 60)):
    r = areas.world.rock(big)
    cx, base = x + r.shape[1] // 2, y + r.shape[0] - 1
    check(not blocked(cx, base) and not blocked(cx, base - 3), f'rock at {x},{y} is walk-through')
# free-standing bushes: Pip can stand right beside the art (feet box within 3 px of
# its edge); the bushes at 180,104 and 150,244 touch the pen fence and a tree
for kind, x, y in (('blossom', 300, 116), ('green', 330, 140)):
    w = areas.bush(kind).shape[1]
    base = y + 15
    left_ok = not blocked(x - FEET_W - 3, base)
    right_ok = not blocked(x + w + FEET_W + 2, base)
    check(left_ok and right_ok, f'bush at {x},{y}: Pip fits right beside it on both sides')

img = areas.collision_preview(a)
Image.fromarray(img[..., :3]).resize((a.w * 2, a.h * 2), Image.NEAREST).save(os.path.join(OUT, 'map_collision.png'))
if fails:
    sys.exit(1)
print('all map checks passed')
