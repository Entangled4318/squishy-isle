"""Checks Cloud Hill (step 6.5). Usage: check_clouds.py OUTDIR
clouds.log: 10 friends in the meadow, woods and shore, Pip on the shore and
the way up not built: Momo's scene plays on the shore, the cloud steps lead
to Cloud Hill, a capsule, the shelf, Momo asleep in its bed and awake with
hearts, the pen, back down to the shore and up again. clouds_continue.log:
Continue starts on Cloud Hill."""
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


def img(name):
    im = Image.open(os.path.join(OUT, name + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, name + '.png'))
    return im


def count(im, rgb, box=None):
    """Pixels of a color, at 5 bits a channel within 1 step (mGBA's output
    differs by a step from the 15-bit values)."""
    q = tuple(v >> 3 for v in rgb)
    reg = im.crop(box) if box else im
    return sum(1 for p in reg.getdata() if all(abs((v >> 3) - w) <= 1 for v, w in zip(p, q)))


hexc = lambda h: tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))
log = open(os.path.join(OUT, 'clouds.log')).read()
st = re.search(r'\[game f(\d+)\] gate scene 3 start helpers=3', log)
dn = re.search(r'\[game f(\d+)\] gate scene 3 done', log)
check(st and dn and int(st.group(1)) < 80, '10 shore friends: Momo is tickled awake on the shore at once')
check('exit to area 3' in log and 'scene meadow pip=24,204 area=3' in log, 'the cloud steps lead up to Cloud Hill')
check(re.search(r'song (?:start|resume) 7\n\[game f\d+\] scene meadow pip=24,204 area=3', log) is not None,
      'Cloud Hill plays its own song')
pops = [int(v) for v in re.findall(r'open pop friend (\d+)', log)]
check(pops and all(60 <= p < 80 for p in pops), f'a capsule holds a Cloud Hill friend (ids {pops})')
check('scene shelf page=3' in log, 'START on Cloud Hill opens the shelf on the Cloud Hill page')
check('momo home near' in log, 'Momo wakes up in its bed when Pip comes close')
check('npc near' not in log.split('gate scene 3 done')[1], 'no sleepy Momo blocks a way once every way is open')
check('exit to area 2' in log and 'scene meadow pip=452,150 area=2' in log, 'the cloud steps lead back down to the shore')
cont = open(os.path.join(OUT, 'clouds_continue.log')).read()
check(re.search(r'scene meadow pip=\d+,\d+ area=3', cont) is not None, 'Continue after power off starts on Cloud Hill')

patch, quilt, sky = hexc('#8f84aa'), hexc('#bde7ff'), hexc('#c4c0f2')
shore = img('c01_shore_scene')
check(count(shore, patch) > 10, 'Momo sleeps at the way up on the shore')
arrive = img('c02_clouds_arrive')
check(count(arrive, hexc('#cfc8f6')) + count(arrive, hexc('#dbd0f8')) + count(arrive, hexc('#e8d6f6')) > 3000,
      'Cloud Hill shows its lavender sky')
after = img('c04_clouds_after')
check(count(after, hexc('#e4dcf2'), (184, 0, 240, 22)) > 3, 'the counter pill shows the capsule icon')
asleep = img('c06a_momo_asleep')
check(count(asleep, patch) > 10 and count(asleep, quilt) > 150, 'Momo sleeps tucked in under its blanket')
home = img('c06_momo_home')
check(count(home, patch) > 10, 'Momo is there when Pip comes close')
for n in ('c03_touch_capsule', 'c05_shelf_clouds', 'c07_pen', 'c08_shore_back', 'c09_continue'):
    img(n)
if fails:
    sys.exit(1)
print('all cloud hill checks passed')
