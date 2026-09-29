"""Checks for the step 2 run (meadow walking, door, shelf, close-up)."""
import os
import re
import sys

OUT = sys.argv[1]
log = open(os.path.join(OUT, 'step2.log')).read()
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


check('scene meadow pip=232,124' in log, 'game boots into the meadow at the cottage')
m = re.search(r'meadow start pip=(\d+),(\d+)', log)
check(m and int(m.group(1)) == 232 and int(m.group(2)) > 150, 'Pip walks down (feet y > 150)')
check('scene shelf' in log, 'START opens the shelf')
check('shelf page=1' in log, 'R turns to the Berry Woods page (play order)')
check('scene closeup Matcha Fox' in log, 'A opens the chosen friend (Matcha Fox)')
check('closeup squish' in log, 'A squishes in the close-up')
check(log.count('scene meadow') >= 2, 'B returns to the meadow')
check('door 0' in log and 'scene house area 0' in log, 'walking up into the cottage door opens its room (owner: not the shelf)')
last = re.findall(r'meadow start pip=(\d+),(\d+)', log)[-1]
check(int(last[0]) > 330 and 128 <= int(last[1]) <= 140,
      f'pond blocks Pip walking up (stopped at {last[0]},{last[1]})')
# the close-up after a meadow visit must render cleanly (regression: text
# strip overflow once corrupted VRAM and showed black gaps)
from PIL import Image
for name in ('s04_closeup', 's05_squish_hearts'):
    im = Image.open(os.path.join(OUT, name + '.ppm')).convert('RGB')
    black = sum(1 for p in im.getdata() if p == (0, 0, 0))
    check(black == 0, f'{name}: background intact ({black} black pixels)')
im = Image.open(os.path.join(OUT, 's04_closeup.ppm')).convert('RGB')
greenish = sum(1 for p in im.crop((88, 60, 152, 124)).getdata() if p[1] > p[0] + 20 and p[1] > p[2])
check(greenish > 300, f'close-up shows the Matcha (green) friend ({greenish} green pixels)')

if fails:
    sys.exit(f'{len(fails)} check(s) failed')
print('all checks passed')
