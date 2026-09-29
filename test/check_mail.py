"""Checks the mailbox and the picnic basket (step 6.7). Usage: check_mail.py OUTDIR
mail.log: no letter before the first new friend; after it the flag is up
and an envelope bobs over the mailbox; A reads the new friend's letter
(the flag goes down), A again re-reads it; snack time at the basket twice,
a different treat each time, with the 3 followers. mail_solo.log: with no
followers Pip eats the treat."""
import os
import re
import sys

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))
from gba import rgb15                          # noqa: E402  (the ROM's 15-bit color for a hex)

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


def count(im, hexes, box):
    qs = {tuple(v >> 3 for v in rgb15(h)) for h in hexes}
    return sum(1 for p in im.crop(box).getdata() if tuple(v >> 3 for v in p) in qs)


log = open(os.path.join(OUT, 'mail.log')).read()
first_pop = log.index('open pop friend')
check('at mailbox' not in log[:first_pop], 'no letter before the first new friend: no A bubble at the mailbox')
friend = int(re.search(r'open pop friend (\d+)', log).group(1))
check(f'at mailbox pip=276,117 new=1' in log, 'after a new friend the mailbox has a new letter')
check(f'scene letter friend {friend} new 1' in log, f'A reads the letter from the newest friend ({friend})')
check('letter open' in log and 'letter squish 1' in log and 'letter close' in log,
      'the letter opens, A squishes the friend, B goes back')
check(f'scene letter friend {friend} new 0' in log, 'no new mail: A re-reads the last letter')
snacks = re.findall(r'snack (\w+) followers=(\d)', log)
check(len(snacks) == 2 and snacks[0][0] != snacks[1][0] and all(n == '3' for _, n in snacks),
      f'snack time twice, a different treat each time, with the followers ({snacks})')
check(log.count('snack done') == 2, 'each snack ends and Pip can walk again')
solo = open(os.path.join(OUT, 'mail_solo.log')).read()
check(re.search(r'snack \w+ followers=0', solo) is not None and 'snack done' in solo, 'with no followers Pip has the snack')

up, down = img('m01_flag_up'), img('m06_flag_down')     # Pip at 276,132: the mailbox at screen 120,56
flag = ('#ffe890', '#f5c563')
check(count(up, flag, (120, 28, 146, 52)) >= 20, 'the flag is up while the letter waits')
check(count(down, flag, (120, 28, 146, 52)) <= 6, 'the flag is down once the letter is read')
ink = ('#c25a82',)
check(count(up, ink, (108, 4, 132, 32)) > 15 and count(down, ink, (108, 4, 132, 32)) < 3,
      'an envelope bobs over the mailbox only while the letter waits')
env, letter = img('m03_envelope'), img('m04_letter')
check(count(env, ('#ffd3df', '#ffc2d3', '#f2a0ba'), (0, 0, 240, 160)) > 400, 'a pink envelope drops in and opens')
check(count(letter, ('#fffaf0',), (0, 0, 240, 160)) > 12000, 'the letter card fills the screen')
check(count(letter, ('#fffaf0',), (30, 50, 90, 110)) < 1500, 'the friend waves from its round frame')
snack = img('m08_snack')
check(count(snack, ('#ff6f86', '#f0c890', '#b8805a', '#4a3a5c', '#5fa870', '#ff9fbd'), (80, 60, 120, 95)) > 8,
      'the treat sits on the picnic blanket')
# step 8.2: each new friend's letter waits its turn
q = open(os.path.join(OUT, 'mail_queue.log')).read()
pops = [int(v) for v in re.findall(r'open pop friend (\d+)', q)]
reads = re.findall(r'scene letter friend (\d+) new (\d) msg \d+ waiting (\d)', q)
check(len(pops) == 2 and len(reads) == 3, f'two new friends, three visits to the mailbox ({pops}, {reads})')
if len(pops) == 2 and len(reads) == 3:
    check([int(r[0]) for r in reads[:2]] == pops and reads[0][1:] == ('1', '1') and reads[1][1:] == ('1', '0'),
          'their letters come oldest first; the flag stays up until the last one is read')
    check(reads[2] == (str(pops[1]), '0', '0'), 'none waiting: A re-reads the last letter')
fu, fd = img('mq_flag_up'), img('mq_flag_down')
box = (116, 44, 152, 78)                  # Pip below the mailbox (goto mail, 276,110): the flag at screen ~125..141, 54..70
check(count(fu, flag, box) >= 20 and count(fd, flag, box) <= 6,
      f'the flag is up between the two letters, down after ({count(fu, flag, box)}, {count(fd, flag, box)} px)')

for n in ('m02_mail_bubble', 'm05_letter_squish', 'm07_basket_bubble', 'm09_snack2', 'm10_pip_snack'):
    img(n)
if fails:
    sys.exit(1)
print('all mailbox and basket checks passed')
