"""Checks the houses and gifts (step 6.9). Usage: check_house.py OUTDIR
house_<area>.log: walking up into each area's house door opens its room:
that area's gifts, the found ones in their friend's colors, the others
as silhouettes; the arrow starts on a found gift, A makes it hop (a
squeak), a gift not found yet only wiggles, B goes back out of the door."""
import os
import re
import sys

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))
from gba import rgb15                          # noqa: E402

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


def count(im, hexes, box):
    qs = {tuple(v >> 3 for v in rgb15(h)) for h in hexes}
    return sum(1 for p in im.crop(box).getdata() if tuple(v >> 3 for v in p) in qs)


FOUND = {'meadow': (0, 7), 'woods': (1, 5), 'shore': (2, 5), 'clouds': (3, 5)}   # area, gifts found there (see Makefile)
VANILLA = ('#a37e68', '#fcedd3', '#f0d8b6', '#fff8ea')                          # the vanilla ramp and outline
for name, (area, n) in FOUND.items():
    log = open(os.path.join(OUT, f'house_{name}.log')).read()
    m = re.search(r'scene house area (\d) gifts (\d+) sel (\d+)', log)
    check('door 0' in log and m and int(m.group(1)) == area, f'{name}: walking up into the house door opens its room')
    if not m:
        continue
    check(int(m.group(2)) == n, f'{name}: the room holds the {n} gifts of the friends found here ({m.group(2)})')
    check(int(m.group(3)) == area * 20, f'{name}: the arrow starts on a gift that is there ({m.group(3)})')
    picks = [(int(i), int(f)) for i, f in re.findall(r'house pick (\d+) found (\d)', log)]
    check(len(picks) == 2 and picks[0][1] == 1 and picks[1][1] == 0 and all(area * 20 <= i < area * 20 + 20 for i, _ in picks),
          f'{name}: A on a gift makes it hop, on a missing one it wiggles ({picks})')
    check('house leave' in log and re.search(rf'house leave\n\[game f\d+\] scene meadow pip=\d+,\d+ area={area}', log),
          f'{name}: B goes back out of the door')
    room = Image.open(os.path.join(OUT, f'h_{name}_room.ppm')).convert('RGB')
    room.resize((960, 640), Image.NEAREST).save(os.path.join(OUT, f'h_{name}_room.png'))
    check(count(room, VANILLA, (11, 42, 29, 60)) > 20, f'{name}: the first gift shows in its friend\'s (vanilla) colors')
    check(count(room, ('#8a5a7a',), (56, 3, 184, 18)) > 20, f'{name}: the pill names who sent the gift')
    back = Image.open(os.path.join(OUT, f'h_{name}_back.ppm')).convert('RGB')
    back.resize((960, 640), Image.NEAREST).save(os.path.join(OUT, f'h_{name}_back.png'))
meadow = Image.open(os.path.join(OUT, 'h_meadow_room.ppm')).convert('RGB')
check(count(meadow, ('#f7d3de',), (47, 42, 65, 60)) > 20 and count(meadow, VANILLA, (47, 42, 65, 60)) == 0,
      'a gift not found yet is a silhouette in its shelf\'s tint')
if fails:
    sys.exit(1)
print('all house checks passed')
