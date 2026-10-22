"""Checks the ways out of the shelf (step 9.3). Usage: check_exits.py OUTDIR
shelf_exits.log: DOWN past the bottom row of the shelf goes back to the map;
after B in a close-up, three close-ups in a row ended with A alone go back to
the map (the first two go back to the shelf)."""
import os
import re
import sys

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


log = open(os.path.join(OUT, 'shelf_exits.log')).read()
scenes = re.findall(r'\] scene (shelf|closeup|meadow)', log)
check(re.search(r'shelf leave \(down\)\n(?:(?!.*\] scene ).*\n)*.*\] scene meadow', log) is not None,
      'DOWN past the bottom row of the shelf goes back to the map')
after = scenes[scenes.index('meadow', 1) + 1:] if scenes.count('meadow') >= 2 else []
want = ['shelf', 'closeup', 'shelf', 'closeup', 'shelf', 'closeup', 'shelf', 'closeup', 'meadow']
check(after == want, f'after B, the 3rd close-up in a row ended with A alone goes back to the map ({" ".join(after)})')
check(log.count('closeup streak: back to the map') == 1, 'only the 3rd close-up of the streak goes to the map')

if fails:
    sys.exit(1)
print('all exit checks passed')
