"""Checks for step 3 (game loop in the meadow). Usage: check_step3.py OUTDIR"""
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


log = open(os.path.join(OUT, 'boxes.log')).read()
frames = {k: int(f) for f, k in re.findall(r'\[game f(\d+)\] (arrow on box \d+|box 0 open)', log)}

# ---- boxes appear
placed = re.findall(r'box (\d) at (\d+),(\d+)', log)
check(len(placed) >= 3 and ('0', '200', '150') in placed[:3], 'three boxes out; the first sits in view below the cottage')
spots = [(x, y) for _, x, y in placed[:3]]
check(len(set(spots)) == 3, 'the three boxes use different spots')

# ---- touch and open
m = re.search(r'touch box 0 pip=(\d+),(\d+)', log)
check(m is not None, 'walking into the box counts as touching it')
m = re.search(r'box 0 open friend (\d+) pip=(\d+),(\d+)', log)
check(m is not None and 0 <= int(m.group(1)) < 20, 'A opens it with a meadow friend inside')
check(m is not None and int(m.group(2)) >= 212, f'the box is solid (Pip stopped at x={m and m.group(2)}, box edge 207)')
check(re.search(r'\[game f(\d+)\] box 0 at (?!200,150)', log) is not None, 'a new box appears at another spot after the open')

# ---- open screen, pressed by the child
fr = lambda pat: [int(f) for f in re.findall(r'\[game f(\d+)\] ' + pat, log)]
enter = fr(r'scene open color 1')
presses = fr(r'open press \d$') or fr(r'open press \d\n')
child = re.findall(r'open press (\d)\n', log)
check(enter and child[:3] == ['1', '2', '3'], 'the open screen shows the meadow box color; A, B, A count as 3 presses')
check(re.search(r'open press 3\n\[game f\d+\] open pop friend (\d+) found 1', log) is not None, 'the third press pops it and the friend joins the collection')
back = fr(r'scene meadow')
check(len(back) >= 2, 'after the pop the game returns to the meadow')

# ---- reveal
names = re.findall(r'scene reveal (\w+ \w+)', log)
check(len(names) == 2, f'each pop leads to the reveal screen ({names})')
check(re.search(r'reveal squish 1\n.*\n?.*reveal squish 2\n.*\n?.*reveal squish 3', log) is not None and 'reveal leave (squished)' in log,
      'A, B, A squish the friend 3 times, then it hops away')
landed = fr(r'reveal landed')
waited = fr(r'reveal leave \(waited\)')
check(len(landed) == 2 and waited and 290 <= waited[0] - landed[1] <= 320,
      f'with no press the friend hops away after 5 s ({waited and landed and waited[0] - landed[-1]} frames)')
check(len(back) >= 3, 'both reveals end back in the meadow')

# ---- open screen, nobody presses
auto = fr(r'open press \d \(auto\)')
enter2 = fr(r'scene open color 3')
check(len(auto) == 3 and enter2 and 230 <= auto[0] - enter2[0] <= 250, f'with no press the box starts opening itself after 4 s ({auto and enter2 and auto[0] - enter2[0]} frames)')
check(len(auto) == 3 and all(60 <= auto[i + 1] - auto[i] <= 80 for i in range(2)), 'then presses itself about every 1.2 s until it pops')
friends = re.findall(r'open pop friend (\d+) found (\d)', log)
check(len(friends) == 2 and friends[0][0] != friends[1][0] and friends[1][1] == '2', 'the second box holds a different friend')

# ---- guide arrow
arrows = [(int(f), b) for f, b in re.findall(r'\[game f(\d+)\] arrow on box (\d)', log)]
check(arrows and 880 <= arrows[0][0] - back[1] <= 920, f'arrow appears 15 s after coming back from the open ({arrows and arrows[0][0] - back[1]} frames)')
check(len(arrows) >= 2 and arrows[0][1] != arrows[1][1], 'arrow switches to a nearer box while walking')

yellow = lambda p: p == (255, 222, 123)          # arrow fill #ffe07a on screen
pink_a = lambda p: p[0] > 220 and 120 < p[1] < 190 and 140 < p[2] < 200

b1 = shot('b01_first_box')
check(count(b1, (70, 95, 110, 125), lambda p: p[2] > 200 and p[0] > 170 and p[1] < 210) > 30, 'first box is visible (lavender) near Pip')
b2 = shot('b02_touch_a_bubble')
check(count(b2, (100, 40, 140, 80), pink_a) > 25, 'touching shows the A bubble above the box')
heart_on = lambda p: p == (255, 156, 189)
o1 = shot('o01_open_idle')
check(count(o1, (95, 60, 150, 125), lambda p: p[2] > 200 and p[0] < 225 and p[1] < 210) > 400, 'open screen: big lavender box on the cushion')
check(count(o1, (110, 130, 170, 155), heart_on) == 0, 'open screen: no hearts filled before a press')
o2 = shot('o02_jump1')
check(count(o2, (110, 130, 170, 155), heart_on) > 20, 'open screen: first press fills a heart')
o3 = shot('o03_jump2')
check(count(o3, (110, 130, 170, 155), heart_on) > count(o2, (110, 130, 170, 155), heart_on) + 20, 'open screen: second press fills another')
o4 = shot('o04_pop')
o5 = shot('o05_pop_sparkles')
bow = lambda p: p[1] > 200 and p[0] > 240 and p[2] < 140
check(count(o1, (95, 55, 145, 85), bow) > 40 and count(o5, (95, 55, 145, 85), bow) == 0,
      'after the pop the lid (yellow bow) has flown off the box')
check(count(o5, (110, 130, 170, 155), heart_on) == 0, 'the A button and hearts go away after the pop')
b4 = shot('b04_after_open')
check(count(b4, (0, 0, 240, 160), yellow) < 10, 'no arrow before the wait is over')
b5 = shot('b05_arrow')
check(count(b5, (0, 0, 30, 160), yellow) > 20, 'arrow at the left edge points to the box off screen')
b7 = shot('b07_arrow_over_box')
pts = [(x, y) for y in range(160) for x in range(240) if yellow(b7.getpixel((x, y)))]
check(len(pts) > 20 and min(p[0] for p in pts) > 24 and max(p[0] for p in pts) < 216 and min(p[1] for p in pts) > 16,
      'arrow floats above a box on screen (not at the edge) once one is in view')

ink = lambda p: p[0] < 170 and p[1] < 120 and p[2] < 160
r1 = shot('r01_landed_stars')
check(count(r1, (88, 60, 152, 120), lambda p: p[1] > p[0] + 20) > 600 if 'Matcha' in names[0] else
      count(r1, (88, 60, 152, 120), lambda p: not (p[0] > 240 and p[1] > 200)) > 600, 'reveal: the friend sits big on the cushion')
check(count(r1, (60, 140, 180, 156), ink) > 40, 'reveal: the name pill shows the friend name')
r2 = shot('r02_squish')
check(count(r2, (40, 20, 200, 110), lambda p: p == (255, 156, 189)) > 10, 'reveal: squishing sends hearts up')
shot('r03_leaving')
shot('r04_reveal_waiting')
shot('b09_back_again')

# ---- the friend is saved
reboot = open(os.path.join(OUT, 'boxes_reboot.log')).read()
check('save: loaded v2 boots=2 found=2' in reboot, 'both found friends survive a reboot')
for n in ('b06_arrow_walking', 'b08_touch_second', 'o06_waiting'):
    shot(n)

if fails:
    sys.exit(1)
print('all step 3 checks passed')
