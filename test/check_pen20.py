"""Checks 17 friends in the big pen: all inside, moving, not piled up.
Reads the pen array from IWRAM dumps. Usage: check_pen20.py OUTDIR ELF"""
import math
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


sym, size = {}, {}
for line in subprocess.check_output(['arm-none-eabi-nm', '-S', ELF], text=True).splitlines():
    p = line.split()
    if len(p) == 4:
        sym[p[3]], size[p[3]] = int(p[0], 16), int(p[1], 16)
STRIDE = size['pen'] // 20                   # sizeof(Roamer): pen[] holds MAX_PEN = 20
hdr = open('../rom/build/game_assets.h').read()
PEN = [int(re.search(r'#define MEADOW_PEN_%s (\d+)' % k, hdr).group(1)) for k in ('X0', 'Y0', 'X1', 'Y1')]
IWRAM = 0x400 + 0x400 + 0x18000 + 0x400      # dump layout: IO, palettes, VRAM, OAM, IWRAM


def roamers(name):
    b = open(os.path.join(OUT, name + '.bin'), 'rb').read()
    at = lambda a: IWRAM + a - 0x03000000
    n = struct.unpack_from('<i', b, at(sym['n_pen']))[0]
    return [struct.unpack_from('<hh', b, at(sym['pen']) + STRIDE * i) for i in range(n)]   # x, y of each Roamer


for n in ('q01_pen_full', 'q02_pen_later'):
    im = Image.open(os.path.join(OUT, n + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, n + '.png'))

log = open(os.path.join(OUT, 'pen20.log')).read()
check('friends pen=17 follow=3' in log, '20 friends: 3 follow Pip, 17 live in the pen')
samples = [roamers('q_pen%d' % k) for k in range(6)]
check(all(len(s) == 17 for s in samples), 'the pen array holds 17 friends in every sample')
inside = all(PEN[0] <= x <= PEN[2] and PEN[1] <= y <= PEN[3] for s in samples for x, y in s)
check(inside, 'every friend stays inside the pen feet area')
moved = min(sum(1 for a, b in zip(samples[k], samples[k + 1]) if a != b) for k in range(5))
check(moved >= 8, f'friends keep roaming, no jam (at least {moved} of 17 moved in every 2 s)')
worst_pairs, worst_min = 0, 99.0
for s in samples:
    d = [math.dist(s[i], s[j]) for i in range(len(s)) for j in range(i + 1, len(s))]
    worst_pairs = max(worst_pairs, sum(1 for v in d if v < 10))
    worst_min = min(worst_min, min(d))
print(f'     closest pair {worst_min:.1f} px, at most {worst_pairs} pairs under 10 px in one sample')
check(worst_pairs <= 1, 'no pile: at most 1 pair of friends closer than 10 px at a time')

if fails:
    sys.exit(1)
print('all pen20 checks passed')
