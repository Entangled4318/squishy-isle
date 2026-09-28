"""Checks the full meadow loop over two reboots: new game, 5 friends, reboot,
15 more, reboot with a full meadow. Usage: check_loop.py OUTDIR ELF"""
import os
import re
import struct
import subprocess
import sys

import numpy as np
from PIL import Image

OUT, ELF = sys.argv[1], sys.argv[2]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


la, lb, lc, ld = (open(os.path.join(OUT, 'loop_%s.log' % p)).read() for p in 'abcd')
for n in ('f01_after5', 'f02_continue', 'f03_full_meadow', 'f04_reboot_full', 'f05_full_pen', 'f06_full_shelf', 'f07_continue_spot', 'f08_closeup_after_catch', 'f09_back_on_shelf'):
    im = Image.open(os.path.join(OUT, n + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, n + '.png'))

check('seek] timeout' not in la + lb, 'Pip reaches every box (no seek timeout)')
pops = [(int(f), int(n)) for f, n in re.findall(r'open pop friend (\d+) found (\d+)', la + lb)]
check([n for _, n in pops] == list(range(1, 21)), f'20 opens count up 1 to 20 ({len(pops)} pops)')
check(sorted(f for f, _ in pops) == list(range(20)), 'all 20 meadow friends, no repeats')
check(la.count('reveal leave (waited)') >= 1 and lb.count('reveal leave (waited)') >= 4,
      'boxes left alone open by themselves and the friend leaves on its own')
check('save: new v2 boots=1 found=0' in la, 'part A starts a new game')
check('save: loaded v2 boots=2 found=5 opens=5' in lb, 'reboot 1: the 5 friends are still there')
check('save: loaded v2 boots=3 found=20 opens=20' in lc, 'reboot 2: all 20 friends are still there')
last_pop = lb.rindex('found 20')
check(re.search(r'box \d at', lb[last_pop:]) is None, 'no new box after the 20th friend')
check(re.search(r'box \d at', lc) is None, 'no box after a reboot with a full meadow')
last_open = re.findall(r'open friend \d+ pip=(\d+,\d+)', lb)[-1]
check(f'restore pip={last_open}' in lc, f'reboot 2: Continue starts where Pip opened the last box ({last_open})')
saved = re.findall(r'save pip=(\d+,\d+)', lc)
check(saved and f'restore pip={saved[-1]}' in ld, f'reboot 3: Continue starts where Pip last stood still ({saved and saved[-1]})')
check('restore pip' not in la, 'a new game starts at the house')
cu = open(os.path.join(OUT, 'f_closeup.bin'), 'rb').read()[0x400 + 0x400 + 0x18000:]
cu_y = cu[0]                                   # OAM 0 = the friend (affine, double size)
check('scene shelf page=0 pick=0' in la, 'START after a catch opens the shelf, not the follower picker')
check(20 <= cu_y <= 36, f'shelf close-up after a catch: the friend sits on the cushion (sprite y {cu_y}, 28 expected)')
check(re.search(r'scene shelf page=0 pick=0\n\[game f\d+\] scene closeup', la) is not None, 'the shelf cursor starts on a found friend: START then A opens a close-up')
check(re.search(r'closeup squish 3\n\[game f\d+\] scene shelf', la) is not None, '3 squishes in the close-up go back to the shelf')
playing, bad = -1, []                          # music per scene: meadow song on the map and shelf, none on the box
for ln in la.splitlines():
    m = re.search(r'song (start|resume|stop) (\d+)|scene (\w+)', ln)
    if not m:
        continue
    if m.group(1):
        playing = -1 if m.group(1) == 'stop' else int(m.group(2))
    elif playing != {'meadow': 1, 'open': 2, 'shelf': 4, 'closeup': 4}.get(m.group(3), playing):
        bad.append(f'{m.group(3)} with song {playing}')
check(not bad and la.count('song start 4') == 1, f'songs per scene: meadow on the map, open tune on the box, music box on the shelf and close-up {bad[:3]}')
check(la.count('song start 1') == 1 and la.count('song resume 1') >= 5, f'the meadow song starts once and goes on where it was after each box and the shelf '
      f'({la.count("song start 1")} starts, {la.count("song resume 1")} resumes)')
check(la.count('scene reveal') >= 5 and len(re.findall(r'open pop.*\n.*song stop 2', la)) + len(re.findall(r'song stop 2\n.*open pop', la)) >= 5,
      'the open tune stops at the pop')
check(len(re.findall(r'reveal landed', la)) == len(re.findall(r'reveal landed\n\[game f\d+\] song start 3|song start 3\n\[game f\d+\] reveal landed', la)) >= 5,
      'the new friend jingle plays when the friend lands')
import wave
_w = wave.open(os.path.join(OUT, 'loop_open.wav'))
_a = np.frombuffer(_w.readframes(_w.getnframes()), dtype=np.int16)
check(len(_a) > 0 and 2000 < np.abs(_a).max() < 30000,
      f'box 1 (open tune, chimes, boing, pop, jingle): sound and no clipping (peak {np.abs(_a).max() if len(_a) else 0})')
trips = re.findall(r'scene shelf.*?scene meadow', la, re.S)
check(trips and all(t.count('song ') == 2 and 'song stop 4' in t and 'song resume 1' in t for t in trips),
      f'the music box plays on through the close-up; back on the map only the meadow song resumes ({len(trips)} trips)')
check('friends pen=17 follow=3' in lc, 'full meadow: 3 friends follow Pip, 17 live in the pen')

sym, size = {}, {}
for line in subprocess.check_output(['arm-none-eabi-nm', '-S', ELF], text=True).splitlines():
    p = line.split()
    if len(p) == 4:
        sym[p[3]], size[p[3]] = int(p[0], 16), int(p[1], 16)
b = open(os.path.join(OUT, 'f_pen.bin'), 'rb').read()
IW = 0x400 + 0x400 + 0x18000 + 0x400 - 0x03000000
n_pen = struct.unpack_from('<i', b, IW + sym['n_pen'])[0]
check(n_pen == 17, f'the pen array holds 17 friends after the reboot ({n_pen})')

PILL, INK = (255, 247, 247), (140, 90, 123)   # counter pill fill and text ink
for n in ('f03_full_meadow', 'f04_reboot_full'):
    m = Image.open(os.path.join(OUT, n + '.ppm')).convert('RGB')
    fill = sum(1 for y in range(2, 18) for x in range(184, 236) if m.getpixel((x, y)) == PILL)
    ink = sum(1 for y in range(2, 18) for x in range(184, 236) if m.getpixel((x, y)) == INK)
    check(fill > 200 and ink > 80, f'{n}: the found counter pill shows at the top right ({fill} fill, {ink} ink px)')

if fails:
    sys.exit(1)
print('all loop checks passed')
