"""Checks the game save across boots. Usage: check_save.py OUTDIR"""
import os
import struct
import sys

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


def log(name):
    return open(os.path.join(OUT, name + '.log')).read()


check('save: new v2 boots=1 found=0' in log('save1'), 'first boot starts a new version 2 save')
check('save: loaded v2 boots=2 found=0' in log('save2'), 'second boot loads it (boots=2)')
check('save: reset v2 boots=1' in log('savev1'), 'an old version 1 save is replaced by a fresh one')
check('save: loaded v2 boots=2' in log('savev1b'), 'the replaced save loads on the next boot')
check(all('FAILED' not in log(n) for n in ('save1', 'save2', 'savev1', 'savev1b')), 'no save write errors')

sav = open(os.path.join(OUT, 'game.sav'), 'rb').read()
slots = [struct.unpack_from('<IHH', sav, off) for off in (0, 0x100)]
check(all(s[0] == 0x53495153 and s[1] == 2 for s in slots), 'both SRAM slots hold version 2 saves after two boots')
check(sorted(s[2] for s in slots) == [1, 2], 'slots alternate (seq 1 and 2)')

if fails:
    sys.exit(1)
print('all save checks passed')
