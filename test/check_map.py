"""Checks the meadow collision grid with the game's own feet test (viewer.c
blocked(): a 11x6 px feet box), and that every container spot of every area
can be reached. Usage: check_map.py OUTDIR"""
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



def reachable_spots(area):
    """Every container spot in every area: Pip, walking from the area's start
    (all gates open), can stand where the game counts a touch (viewer.c
    box_hit with TOUCH 6, as the harness seek uses), and each spot's box
    footprint lies on walkable ground, not in the sky or the sea."""
    from collections import deque
    W, H = area.w, area.h

    def sol(x, y):
        return x < 0 or y < 0 or x >= W or y >= H or area.solid[y // C, x // C]

    def walk(x, y, box=None):
        if sol(x - FEET_W, y - FEET_H) or sol(x + FEET_W, y - FEET_H) or sol(x - FEET_W, y) or sol(x + FEET_W, y):
            return False
        if box and x + FEET_W >= box[0] - 7 and x - FEET_W <= box[0] + 7 and y >= box[1] - 6 and y - FEET_H <= box[1]:
            return False
        return True

    seen = np.zeros((H, W), bool)
    sx, sy = area.spawn
    q = deque([(sx, sy)])
    seen[sy, sx] = True
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < W and 0 <= ny < H and not seen[ny, nx] and walk(nx, ny):
                seen[ny, nx] = True
                q.append((nx, ny))
    bad = []
    for (dx0, dy0, dw, dh, _) in area.doors:        # the door zone (viewer.c: up to 12 px below the door)
        if not any(seen[y, x] for y in range(dy0, min(H, dy0 + dh + 12)) for x in range(dx0, dx0 + dw)):
            bad.append(('door', dx0, dy0))
    for (bx, by) in area.spots:
        on_ground = all(not sol(x, y) for x in (bx - 6, bx, bx + 6) for y in (by - 5, by))
        touch = any(seen[y, x] and walk(x, y, (bx, by))
                    for y in range(max(0, by - 14), min(H, by + 10)) for x in range(max(0, bx - 16), min(W, bx + 17))
                    if x + FEET_W + 4 >= bx - 7 and x - FEET_W - 4 <= bx + 7 and y + 4 >= by - 6 and y - FEET_H - 4 <= by)
        if not (on_ground and touch):
            bad.append((bx, by))
    return bad


for build in (areas.meadow, areas.woods, areas.shore, areas.clouds):
    ar = build()
    bad = reachable_spots(ar)
    check(not bad, f'{ar.name}: Pip can reach all {len(ar.spots)} container spots (none off the ground) and '
                   f'{len(ar.doors)} house door {bad or ""}')

img = areas.collision_preview(a)
Image.fromarray(img[..., :3]).resize((a.w * 2, a.h * 2), Image.NEAREST).save(os.path.join(OUT, 'map_collision.png'))
if fails:
    sys.exit(1)
print('all map checks passed')
