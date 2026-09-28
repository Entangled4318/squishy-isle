"""Checks the friend pen and the follower line. Usage: check_pen.py OUTDIR"""
import os
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


log = open(os.path.join(OUT, 'pen.log')).read()
check('friends pen=7 follow=3' in log, '10 friends: the 3 followers walk with Pip, the other 7 live in the pen')
p1, p2 = shot('p01_walk_line'), shot('p02_pen')
grass = lambda p: p in ((181, 231, 165), (156, 214, 148))
behind = sum(1 for y in range(70, 95) for x in range(126, 180) if not grass(p1.getpixel((x, y))))
check(behind > 250, f'the followers line up behind Pip ({behind} px)')
moved = sum(1 for y in range(25, 80) for x in range(25, 90) if p1.getpixel((x, y)) != p2.getpixel((x, y)))
check(moved > 40, f'friends waddle around inside the pen ({moved} px changed)')

if fails:
    sys.exit(1)
print('all pen checks passed')
