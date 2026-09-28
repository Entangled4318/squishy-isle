"""Checks the open screen per container (step 6.6). Usage: check_open.py OUTDIR
open_<area>.log: one container opened in the woods, on the shore and on
Cloud Hill: the open screen shows that area's container (acorn, seashell,
capsule), the lid (cap, top shell, dome) flies off and the inside shows."""
import os
import re
import sys

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))
import props                                   # noqa: E402

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


def img(name):
    im = Image.open(os.path.join(OUT, name + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, name + '.png'))
    return im


def count(im, rgb, box=(80, 40, 160, 125)):
    """Pixels of a color (already 15-bit, as in the ROM) in the container's place."""
    q = tuple(v >> 3 for v in rgb)
    return sum(1 for p in im.crop(box).getdata() if tuple(v >> 3 for v in p) == q)


def colors(fn, color, part):
    im = fn(color, part)
    return {tuple(int(v) for v in p) for p in im[..., :3][im[..., 3] > 0]}


AREAS = {'woods': (1, props.acorn64, ('cream', 'pink', 'mint', 'lav', 'gold'), 'acorn'),
         'shore': (2, props.shell64, ('pink', 'peach', 'mint', 'lav', 'yellow'), 'seashell'),
         'clouds': (3, props.capsule64, tuple(props.CAPSULE_COLORS), 'capsule')}
for name, (area, fn, cols, what) in AREAS.items():
    log = open(os.path.join(OUT, f'open_{name}.log')).read()
    m = re.search(r'scene open color (\d) friend (\d+) area (\d)', log)
    check(m and int(m.group(3)) == area, f'{name}: the open screen knows the area')
    if not m:
        continue
    color = cols[int(m.group(1))]
    pop = re.search(r'open pop friend (\d+)', log)
    check(pop and area * 20 <= int(pop.group(1)) < area * 20 + 20, f'{name}: the {what} holds a friend of this area')
    closed, popped = img(f'o_{name}_closed'), img(f'o_{name}_pop')
    img(f'o_{name}_jump')
    img(f'o_{name}_open')
    lid_only = colors(fn, color, 'lid') - colors(fn, color, 'open')          # colors only the lid has
    inside = colors(fn, color, 'open') - colors(fn, color, 'body') - colors(fn, color, 'lid')
    every = colors(fn, color, 'all') | colors(fn, color, 'open')
    n_all = sum(count(closed, c) for c in every)
    check(n_all > 600, f'{name}: a {color} {what} waits on the cushion ({n_all} pixels)')
    n_lid = sum(count(closed, c) for c in lid_only)
    near = sum(count(popped, c) for c in lid_only)
    check(n_lid >= 10 and near <= n_lid // 4, f'{name}: the lid flew off at the pop ({n_lid} to {near} pixels in lid colors)')
    if inside:
        n_in = sum(count(popped, c) for c in inside)
        check(n_in > 10, f'{name}: the inside shows after the pop ({n_in} pixels)')
if fails:
    sys.exit(1)
print('all open screen checks passed')
