"""Checks the way to Berry Woods (step 6.2). Usage: check_woods.py OUTDIR
woods_shut.log: 9 meadow friends, the log on the bridge blocks the way.
woods.log: 10 friends, over the bridge, an acorn, the shelf, back, again.
woods_continue.log: after a reboot, Continue starts in the woods.
gate.log: 10 friends, gate scene not played yet: it plays, then the way is open.
gate_again.log: after a reboot it does not play again."""
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


def gba(h):
    """A #rrggbb color as the emulator shows it (15-bit, expanded)."""
    v = [int(h[i:i + 2], 16) for i in (1, 3, 5)]
    return tuple(((c * 31 + 127) // 255 << 3) | ((c * 31 + 127) // 255 >> 2) for c in v)


def shot(n):
    im = Image.open(os.path.join(OUT, n + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, n + '.png'))
    return im


def count(im, box, color):
    x0, y0, x1, y1 = box
    return sum(1 for y in range(y0, y1) for x in range(x0, x1) if im.getpixel((x, y)) == color)


shut = open(os.path.join(OUT, 'woods_shut.log')).read()
m = re.search(r'\[walkto\] stopped at (\d+),(\d+)', shut)
check('exit to area' not in shut and m and int(m.group(1)) <= 446,
      f'9 meadow friends: the log stops Pip on the bridge (at x {m and m.group(1)}, log from 450)')
log = open(os.path.join(OUT, 'woods.log')).read()
check('exit to area 1' in log and 'scene meadow pip=20,200 area=1' in log,
      '10 meadow friends: over the bridge into the woods, arriving on the west trail')
pops = [int(v) for v in re.findall(r'open pop friend (\d+)', log)]
check(pops and all(20 <= p < 40 for p in pops), f'an acorn holds a woods friend (ids {pops})')
check('scene shelf page=1' in log, 'START in the woods opens the shelf on the woods page')
check('exit to area 0' in log and 'scene meadow pip=450,200 area=0' in log, 'the west trail leads back over the bridge')
after = log[log.index('scene meadow pip=450,200 area=0'):]
check('friends pen=7 follow=3' in after, 'back in the meadow its own 3 followers are there (the woods friend stays in the woods)')
woods_lines = re.findall(r'area=1\n\[game f\d+\] friends pen=(\d+) follow=(\d+)', log)
check(('0', '1') in woods_lines, f'in the woods the new friend follows Pip ({woods_lines[-2:]})')
cont = open(os.path.join(OUT, 'woods_continue.log')).read()
check('scene meadow pip=20,200 area=1' in cont, 'after a reboot, Continue starts in the woods where Pip stood')
songs = re.findall(r'song (?:start|resume) (\d+)\n\[game f\d+\] scene meadow pip=\d+,\d+ area=(\d)', log)
check(('6', '1') in songs and ('1', '0') in songs, f'the woods play their own song, the meadow its own ({songs[:4]})')

w1, w2 = shot('w01_log_shut'), shot('w02_bridge_open')
gate = (206, 70, 228, 108)                      # the log's place on screen (camera at 240,108 in both)
ink = gba('#704c52')
check(count(w1, gate, ink) >= 20 and count(w2, gate, ink) == 0,
      f'the log shows on the bridge while shut and is gone once open ({count(w1, gate, ink)} / {count(w2, gate, ink)} px)')
w5, w8 = shot('w05_woods_after'), shot('w08_meadow_back')
pill = (180, 2, 236, 18)
cap = gba('#c49276')
check(count(w5, pill, cap) >= 4 and count(w8, pill, cap) == 0,
      f'the woods counter shows an acorn, the meadow counter does not ({count(w5, pill, cap)} / {count(w8, pill, cap)} px)')
gl = open(os.path.join(OUT, 'gate.log')).read()
st = re.search(r'\[game f(\d+)\] gate scene 1 start helpers=(\d)', gl)
dn = re.search(r'\[game f(\d+)\] gate scene 1 done', gl)
check(st and dn and st.group(2) == '3' and 280 <= int(dn.group(1)) - int(st.group(1)) <= 330,
      f'10th friend: the gate scene plays once with 3 helpers, about 5 s ({dn and st and int(dn.group(1)) - int(st.group(1))} frames)')
check(st and int(st.group(1)) - int(re.search(r'\[game f(\d+)\] scene meadow', gl).group(1)) < 20,
      'it starts as soon as the meadow shows')
check('exit to area 1' in gl, 'after the scene the bridge leads to the woods')
check('gate scene' not in open(os.path.join(OUT, 'gate_again.log')).read(), 'after a reboot the scene does not play again (saved)')
g3, g4, g5 = shot('g03_push'), shot('g04_roll'), shot('g05_cheer')
at = (206, 60, 228, 98)                         # the log's place with the camera on it
below = (200, 98, 240, 160)
check(count(g3, at, ink) >= 20, f'the log lies on the bridge while the friends push ({count(g3, at, ink)} px)')
check(count(g4, at, ink) < count(g3, at, ink) // 2 and count(g4, below, ink) >= 10,
      f'then it tumbles off the way ({count(g4, at, ink)} px left, {count(g4, below, ink)} px below)')
check(count(g5, at, ink) == 0 and count(g5, below, ink) == 0, 'and it is gone')
for n in ('g01_pan', 'g02_helpers', 'g06_back'):
    shot(n)
for n in ('w03_woods_arrive', 'w04_touch_acorn', 'w06_shelf_woods', 'w07_pen_sign', 'w09_continue_woods'):
    shot(n)

if fails:
    sys.exit(1)
print('all woods checks passed')
