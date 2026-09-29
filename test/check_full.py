"""Checks the full 80-friend loop (step 6.8). Usage: check_full.py OUTDIR
full.log: a new game; 20 containers in each area in play order, every way
opened by Momo's gate scene after 10 friends; the whole shelf; Momo's bed.
full_b.log: power off and Continue with all 80 found."""
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


log = open(os.path.join(OUT, 'full.log')).read()
check('save: new v2 boots=1 found=0' in log, 'a new game starts')
check('seek] timeout' not in log, 'Pip reaches every container in every area (no seek timeout)')
pops = [(int(f), int(n)) for f, n in re.findall(r'open pop friend (\d+) found (\d+)', log)]
check([n for _, n in pops] == list(range(1, 81)), f'80 opens count up 1 to 80 ({len(pops)} pops)')
check(sorted(f for f, _ in pops) == list(range(80)), 'all 80 friends, each once: no repeats')
order = [f // 20 for f, _ in pops]
check(order == sorted(order), 'the areas fill in play order: meadow, woods, shore, Cloud Hill')
areas_of = [int(a) for a in re.findall(r'scene open color \d friend \d+ area (\d)', log)]
check(len(areas_of) == 80 and all(areas_of[i] == pops[i][0] // 20 for i in range(80)),
      "each container holds a friend of the area it stands in (and shows that area's container)")
for to in (1, 2, 3):
    st = [m.start() for m in re.finditer(rf'gate scene {to} start helpers=3', log)]
    tenth = log.index(f'found {(to - 1) * 20 + 10}\n')
    check(len(st) == 1 and st[0] > tenth and f'gate scene {to} done' in log,
          f'Momo wakes once at the way into area {to}, after the 10th friend there')
    check(f'exit to area {to}' in log and log.index(f'exit to area {to}') > log.index(f'gate scene {to} done'),
          f'Pip walks on into area {to} once the way is open')
for a in range(4):
    last = log.index(f'found {a * 20 + 20}\n')
    nxt = log.find('scene meadow', log.find('exit to area', last)) if a < 3 else len(log)
    seg = log[last:nxt if nxt > 0 else len(log)]
    check(re.search(r'box \d at', seg) is None, f'no new container in area {a} once its 20 are found')
waited = log.count('reveal leave (waited)')
auto = log.count('(auto)')
check(waited >= 18 and auto >= 54, f'containers left alone open themselves ({auto // 3}), friends leave on their own ({waited})')
songs = re.findall(r'song (?:start|resume) (\d)\n\[game f\d+\] scene meadow pip=\d+,\d+ area=(\d)', log)
want = {'0': '1', '1': '6', '2': '5', '3': '7'}
check(songs and all(want[a] == s for s, a in songs) and {a for _, a in songs} == set(want),
      'every area plays its own song')
check('momo home near' in log, 'Momo sleeps in its cloud bed at the end and wakes for Pip')

b = open(os.path.join(OUT, 'full_b.log')).read()
check('save: loaded v2 boots=2 found=80 opens=80' in b, 'Continue after power off: all 80 friends are still there')
check(re.search(r'scene meadow pip=\d+,\d+ area=3', b) is not None and re.search(r'box \d at', b) is None,
      'Continue starts on Cloud Hill, with no containers left anywhere')


def img(name):
    im = Image.open(os.path.join(OUT, name + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, name + '.png'))
    return im


for p in range(4):                            # a full shelf page: no silhouettes ("Find me!") left
    im = img(f'full_shelf{p}')
    grey = sum(1 for px in im.crop((8, 22, 232, 160)).getdata()
               if max(px) - min(px) < 12 and 120 < px[0] < 230)
    check(grey < 400, f'shelf page {p} is full (grey silhouette pixels {grey})')
for n in ('full_a0_first', 'full_a1_first', 'full_a2_first', 'full_a3_first', 'full_a0_done', 'full_a1_done',
          'full_a2_done', 'full_a3_done', 'full_momo_home', 'full_pen_clouds', 'full_continue', 'full_no_boxes'):
    img(n)
if fails:
    sys.exit(1)
print('all full loop checks passed')
