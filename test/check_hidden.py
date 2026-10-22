"""Checks the way out of the hidden screens (step 9.7). Usage: check_hidden.py OUTDIR
hidden_exit_hw.log / hidden_exit_snd.log: in the hardware check and in the
sound test, holding L + R for 1 s goes on to the title, with the save loaded."""
import os
import re
import sys

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


for name, leave in (('hw', 'hwcheck leave'), ('snd', 'jukebox leave')):
    log = open(os.path.join(OUT, f'hidden_exit_{name}.log')).read()
    after = log.split(leave, 1)[1] if leave in log else ''
    check(re.search(r'scene title options=2 found=3', after) and 'title start continue' in after,
          f'{name}: holding L + R leaves for the title; the save loads and Continue works')

if fails:
    sys.exit(1)
print('all hidden screen checks passed')
