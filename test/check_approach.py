"""Walking up to Momo on Cloud Hill (step 8.5, Brick test: "the path seems
too short and requires precise movement"). Usage: check_approach.py OUTDIR ROM

A toddler pushes toward Momo: from 39 spots on the big island around the
pen's top (x 280..400, y 140..164), hold UP+RIGHT for 5 s with no steering.
Pip must reach Momo's bed every time (the "momo home near" log line). In
v1.3 he got there from 12 of 31 spots: the bridge lane was 18 px, a notch at
the island edge and a candy tree pocket stopped him, and stair steps of the
cloud edge stopped a diagonal walk."""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT, ROM = sys.argv[1], sys.argv[2]
HARNESS = os.path.join(HERE, 'build', 'harness')
found = ','.join(str(i) for a in (0, 1, 2) for i in range(a * 20, a * 20 + 10)) + ',60,61'
script = os.path.join(OUT, 'approach.txt')
open(script, 'w').write('wait 20\ntap A\nwait 15\ntap A\nwait 40\nhold UP+RIGHT 300\nwait 10\n')
ok, tried, missed = 0, 0, []
for y in (140, 152, 164):
    for x in range(280, 401, 10):
        sav = os.path.join(OUT, 'approach.sav')
        subprocess.run([sys.executable, os.path.join(HERE, 'make_save.py'), sav, found, '', '-', '3', '-', f'{x},{y}'], check=True)
        log = subprocess.run([HARNESS, ROM, sav, script, OUT], capture_output=True, text=True).stdout
        if f'restore pip={x},{y}' not in log:
            continue                              # not a walkable spot: the game puts Pip at the area's start
        tried += 1
        if 'momo home near' in log:
            ok += 1
        else:
            missed.append((x, y))
good = tried >= 30 and ok == tried
print(('PASS ' if good else 'FAIL ') +
      f'holding UP+RIGHT toward Momo from {tried} spots by the pen, Pip reaches Momo every time ({ok}; missed {missed or "none"})')
sys.exit(0 if good else 1)
