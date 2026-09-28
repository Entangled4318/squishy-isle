"""Checks the way to Seashell Shore (step 6.4). Usage: check_shore.py OUTDIR
shore.log: 10 meadow and 10 woods friends, the way to the shore not built:
Momo's scene plays in the woods, then east onto the shore boardwalk, a
shell, the shelf, Momo at the way to Cloud Hill, back to the woods."""
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


log = open(os.path.join(OUT, 'shore.log')).read()
st = re.search(r'\[game f(\d+)\] gate scene 2 start helpers=3', log)
dn = re.search(r'\[game f(\d+)\] gate scene 2 done', log)
arrive = re.search(r'\[game f(\d+)\] scene meadow pip=20,200 area=1', log)
check(st and dn and arrive and int(st.group(1)) - int(arrive.group(1)) < 20,
      '10 woods friends: Momo is tickled awake in the woods as soon as Pip arrives')
check('exit to area 2' in log and 'scene meadow pip=20,200 area=2' in log, 'the east trail leads onto the shore boardwalk')
pops = [int(v) for v in re.findall(r'open pop friend (\d+)', log)]
check(pops and all(40 <= p < 60 for p in pops), f'a seashell holds a shore friend (ids {pops})')
check('scene shelf page=2' in log, 'START on the shore opens the shelf on the shore page')
check(re.search(r'song (?:start|resume) 5\n\[game f\d+\] scene meadow pip=20,200 area=2', log) is not None,
      'the shore plays its own song')
check('npc near 0 found 1' in log, 'Momo sleeps at the way to Cloud Hill and shows 1/10')
check('exit to area 1' in log and 'scene meadow pip=456,120 area=1' in log, 'the boardwalk leads back to the woods east trail')
for n in ('s01_woods_scene', 's02_shore_arrive', 's03_touch_shell', 's04_shore_after', 's05_shelf_shore',
          's06_momo_shore', 's07_woods_back'):
    im = Image.open(os.path.join(OUT, n + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, n + '.png'))

if fails:
    sys.exit(1)
print('all shore checks passed')
