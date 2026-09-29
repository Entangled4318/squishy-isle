"""Momo's heart meter (step 8.4). Usage: check_momo.py OUTDIR
meter_meadow/woods/shore.log: Pip beside a sleeping Momo with 7, 3 and 9
friends in that area. The bubble shows one filled heart per friend, the
counter pill hides meanwhile, and the bubble clears the signs (check_map.py
checks every spot Pip can wake Momo from)."""
import os
import re
import struct
import sys

from PIL import Image

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


hdr = open(os.path.join(os.path.dirname(__file__), '..', 'rom', 'build', 'game_assets.h')).read()
define = {k: int(v) for k, v in re.findall(r'#define (METER_\w+|COUNT_PILL_W) (\d+)', hdr)}
T_METER, T_COUNT = 448, 128                      # viewer.c
for name, found in (('meadow', 7), ('woods', 3), ('shore', 9)):
    log = open(os.path.join(OUT, f'meter_{name}.log')).read()
    d = open(os.path.join(OUT, f'meter_{name}.bin'), 'rb').read()
    oam = [struct.unpack_from('<3H', d, 0x18800 + i * 8) for i in range(128)]
    shown = [a2 & 0x3FF for a0, a1, a2 in oam if not a0 & 0x200]
    hearts = shown.count(T_METER + define['METER_HEART'])
    check(f'npc near 0 found {found}' in log, f'{name}: Momo wakes as Pip comes close, {found} friends found')
    check(T_METER in shown and T_METER + define['METER_TAIL'] in shown and hearts == found,
          f'{name}: the heart meter shows {hearts} filled hearts of 10 (want {found})')
    check(T_COUNT not in shown, f'{name}: the counter pill hides while the meter shows')
    im = Image.open(os.path.join(OUT, f'meter_{name}.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, f'meter_{name}.png'))

if fails:
    sys.exit(1)
print('all Momo meter checks passed')
