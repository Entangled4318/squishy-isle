"""Checks for step 3 (game loop in the meadow). Usage: check_step3.py OUTDIR"""
import os
import re
import sys

from PIL import Image

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


def shot(name):
    im = Image.open(os.path.join(OUT, name + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, name + '.png'))
    return im


def count(im, box, pred):
    return sum(1 for p in im.crop(box).get_flattened_data() if pred(p))


log = open(os.path.join(OUT, 'boxes.log')).read()
frames = {k: int(f) for f, k in re.findall(r'\[game f(\d+)\] (arrow on box \d+|box 0 open)', log)}

# ---- boxes appear
placed = re.findall(r'box (\d) at (\d+),(\d+)', log)
check(len(placed) >= 3 and ('0', '200', '150') in placed[:3], 'three boxes out; the first sits in view below the cottage')
spots = [(x, y) for _, x, y in placed[:3]]
check(len(set(spots)) == 3, 'the three boxes use different spots')

# ---- touch and open
m = re.search(r'touch box 0 pip=(\d+),(\d+)', log)
check(m is not None, 'walking into the box counts as touching it')
m = re.search(r'box 0 open friend (\d+) found 1 pip=(\d+),(\d+)', log)
check(m is not None and 0 <= int(m.group(1)) < 20, 'A opens it and a meadow friend joins the collection')
check(m is not None and int(m.group(2)) >= 212, f'the box is solid (Pip stopped at x={m and m.group(2)}, box edge 207)')
check(re.search(r'\[game f(\d+)\] box 0 at (?!200,150)', log) is not None, 'a new box appears at another spot after the open')
opened = frames.get('box 0 open', 0)

# ---- guide arrow
arrows = re.findall(r'\[game f(\d+)\] arrow on box (\d)', log)
check(arrows and 880 <= int(arrows[0][0]) - opened <= 920, f'arrow appears 15 s after the last open ({arrows and int(arrows[0][0]) - opened} frames)')
check(len(arrows) >= 2 and arrows[0][1] != arrows[1][1], 'arrow switches to a nearer box while walking')

yellow = lambda p: p == (255, 222, 123)          # arrow fill #ffe07a on screen
pink_a = lambda p: p[0] > 220 and 120 < p[1] < 190 and 140 < p[2] < 200

b1 = shot('b01_first_box')
check(count(b1, (70, 95, 110, 125), lambda p: p[2] > 200 and p[0] > 170 and p[1] < 210) > 30, 'first box is visible (lavender) near Pip')
b2 = shot('b02_touch_a_bubble')
check(count(b2, (100, 40, 140, 80), pink_a) > 25, 'touching shows the A bubble above the box')
b3 = shot('b03_open_burst')
check(count(b3, (80, 40, 140, 120), lambda p: p[0] > 240 and p[1] > 235 and p[2] > 150 and p[2] < 230) > 6, 'the box pops in a sparkle burst')
b4 = shot('b04_after_open')
check(count(b4, (0, 0, 240, 160), yellow) < 10, 'no arrow before the wait is over')
b5 = shot('b05_arrow')
check(count(b5, (0, 0, 30, 160), yellow) > 20, 'arrow at the left edge points to the box off screen')
shot('b06_arrow_walking')
b7 = shot('b07_arrow_over_box')
check(count(b7, (170, 60, 200, 95), yellow) > 20, 'arrow floats above the box once it is on screen')

# ---- the friend is saved
reboot = open(os.path.join(OUT, 'boxes_reboot.log')).read()
check('save: loaded v2 boots=2 found=1' in reboot, 'the found friend survives a reboot')

if fails:
    sys.exit(1)
print('all step 3 checks passed')
