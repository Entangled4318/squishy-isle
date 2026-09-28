"""Checks the follower picker at the pen sign. Usage: check_pick.py OUTDIR"""
import os
import sys

from PIL import Image

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


for n in ('k01_sign', 'k02_picker', 'k03_added', 'k04_meadow'):
    im = Image.open(os.path.join(OUT, n + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, n + '.png'))

log = open(os.path.join(OUT, 'pick.log')).read()
check('at sign' in log, 'Pip reaches the pen sign and the A bubble shows')
check('scene shelf page=0 pick=1' in log, 'A at the sign opens the picker on the meadow page')
check('pick remove 17' in log, 'A on a follower removes it')
check('pick add 15' in log, 'A on another found friend adds it')
check('pick done 15 6 12' in log, 'the new friend walks first, the others keep their order')
check(log.count('friends pen=7 follow=3') == 2, 'back in the meadow the line is rebuilt (3 follow, 7 in the pen)')
m = Image.open(os.path.join(OUT, 'k04_meadow.ppm')).convert('RGB')
ink = sum(1 for y in range(2, 16) for x in range(6, 50) if m.getpixel((x, y)) in ((255, 255, 255), (247, 247, 247)) or sum(m.getpixel((x, y))) < 300)
check(ink > 60, f'the found counter shows at the top left ({ink} px)')

if fails:
    sys.exit(1)
print('all pick checks passed')
