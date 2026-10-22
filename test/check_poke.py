"""Checks A on the map with nothing else to do (step 9.4). Usage: check_poke.py OUTDIR
poke.log: A squishes the nearest follower; by a sleeping Momo, A tickles it.
poke_pen.log: with no followers, A by the pen fence squishes a pen friend;
far from every friend, A only ticks (no poke)."""
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
    return Image.open(os.path.join(OUT, name + '.ppm')).convert('RGB')


log = open(os.path.join(OUT, 'poke.log')).read()
pokes = re.findall(r'poke (friend|momo) (\d+)', log)
check(pokes and pokes[0][0] == 'friend' and int(pokes[0][1]) in (0, 1), f'A near the followers squishes one of them ({pokes[:1]})')
check(pokes.count(('momo', '0')) == 2, 'A by sleeping Momo tickles it, every time')
a, b = shot('pk_follow_squish'), shot('pk_follow_heart')
diff = sum(1 for p, q in zip(a.getdata(), b.getdata()) if p != q)
check(diff > 40, f'the squished follower and its heart change the picture ({diff} px)')

log = open(os.path.join(OUT, 'poke_pen.log')).read()
pen = [int(i) for i in re.findall(r'poke friend (\d+)', log)]
check(len(pen) == 1 and 0 <= pen[0] < 9, f'with no followers, A by the fence squishes a pen friend; far away it does not ({pen})')

if fails:
    sys.exit(1)
print('all poke checks passed')
