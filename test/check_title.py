"""Checks for the title: intro, menu, and the guarded start-over. Usage: check_title.py OUTDIR"""
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


def shot(name):
    im = Image.open(os.path.join(OUT, name + '.ppm')).convert('RGB')
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, name + '.png'))
    return im


def count(im, box, pred):
    return sum(1 for p in im.crop(box).get_flattened_data() if pred(p))


def frames(log, pat):
    return [int(f) for f in re.findall(r'\[game f(\d+)\] ' + pat, log)]


white = lambda p: p == (255, 255, 247)       # C["white"] on screen
heart_on = lambda p: p == (255, 156, 189)

# ---- fresh save
log = open(os.path.join(OUT, 'title_new.log')).read()
check('scene title options=1 found=0' in log, 'a fresh save boots to the title with one option')
done = [int(v) for v in re.findall(r'title intro done at (\d+)', log)]
check(done and 120 <= done[0] <= 140, f'the logo intro plays by itself in about 2 s ({done and done[0]} frames)')
check('title start new' in log and 'scene meadow' in log, 'A on "Play" starts a new game in the meadow')
t1, t2, t3 = shot('t01_intro'), shot('t02_menu_play'), shot('t03_letters_hop')
logo = lambda im: count(im, (40, 0, 200, 80), lambda p: p[0] < 250 and not (p[2] > 200 and p[0] > 190 and p[1] > 180))
check(logo(t1) < logo(t2) - 400, 'the letters drop in one by one (fewer in the first frames)')
check(count(t2, (100, 136, 140, 152), white) > 15, '"Play" shows centred at the bottom')
check(sum(1 for y in range(0, 80) for x in range(40, 200) if t2.getpixel((x, y)) != t3.getpixel((x, y))) > 300,
      'B makes the letters hop')

# ---- save with friends, start over
log = open(os.path.join(OUT, 'title_reset.log')).read()
check('scene title options=2 found=2' in log, 'a save with friends shows Continue and New game')
t4 = shot('t04_menu_continue')
check(count(t4, (20, 136, 60, 156), heart_on) > 20, 'Continue is chosen by default (heart cursor on the left)')
t5 = shot('t05_new_game')
check(count(t5, (110, 136, 160, 156), heart_on) > 20, 'the D-pad moves the cursor to New game')
confirm = frames(log, r'title confirm$') or frames(log, r'title confirm\n')
nos = frames(log, r'title confirm no')
check(len(nos) >= 2, 'A on the default "No" and B both back out of the start-over screen')
t6 = shot('t06_confirm_no')
check(count(t6, (40, 20, 200, 70), white) > 150, 'the start-over screen asks in words for the parent')
t7, t8 = shot('t07_hold_part'), shot('t08_released')
filled = count(t7, (60, 66, 180, 90), heart_on)
check(20 < filled < 120, f'holding A on "Yes" fills the hearts bit by bit ({filled} px after 1 s)')
check(count(t8, (60, 66, 180, 90), heart_on) == 0 and 'title hold released at 6' in log,
      'letting go of A empties the hearts again; nothing is erased')
starts = frames(log, r'title hold start')
new = frames(log, r'title start new')
check(len(new) == 1 and starts and 175 <= new[0] - starts[-1] <= 185,
      f'only a full 3 s hold starts over ({new and starts and new[0] - starts[-1]} frames)')
shot('t09_new_game_meadow')
reboot = open(os.path.join(OUT, 'reset_reboot.log')).read()
check('found=0' in reboot and 'options=1' in reboot, 'after starting over, the save is empty on the next boot')

ver = sum(1 for y in range(146, 158) for x in range(216, 240)            # step 7.5: "v1.0" at the bottom right
          if all(abs(c - d) <= 4 for c, d in zip(t4.getpixel((x, y))[:3], (0x8a, 0x5a, 0x7a))))
check(20 <= ver <= 80, f'the version shows small at the bottom right of the title ({ver} ink pixels)')
if fails:
    sys.exit(1)
print('all title checks passed')
