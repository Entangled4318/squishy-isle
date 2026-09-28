"""Checks the full meadow loop over two reboots: new game, 5 friends, reboot,
15 more, reboot with a full meadow. Usage: check_loop.py OUTDIR ELF"""
import os
import re
import struct
import subprocess
import sys

from PIL import Image

OUT, ELF = sys.argv[1], sys.argv[2]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


la, lb, lc, ld = (open(os.path.join(OUT, 'loop_%s.log' % p)).read() for p in 'abcd')
for n in ('f01_after5', 'f02_continue', 'f03_full_meadow', 'f04_reboot_full', 'f05_full_pen', 'f06_full_shelf', 'f07_continue_spot'):
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

grass = ((181, 231, 165), (156, 214, 148))
for n in ('f03_full_meadow', 'f04_reboot_full'):
    m = Image.open(os.path.join(OUT, n + '.ppm')).convert('RGB')
    ink = sum(1 for y in range(2, 16) for x in range(6, 50) if m.getpixel((x, y)) not in grass)
    check(ink > 60, f'{n}: the found counter shows ({ink} px)')

if fails:
    sys.exit(1)
print('all loop checks passed')
