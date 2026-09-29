"""Step 7.4: every text the game prints fits the strip it is printed into.
Usage: check_text.py BUILD_DIR   (rom/build: font_data.c/.h)

Widths come from the ROM's own font (5x7 variable width, 1 px between
letters, text.c text_width). Strips and their widths are the ones in the
scene sources; a text wider than its strip is cut at both ends (centred)
or at the right (left aligned). The house pill once cut six "From
Strawberry ..." names (121 px in 112)."""
import os
import re
import sys

BUILD = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


src = open(os.path.join(BUILD, 'font_data.c')).read()
first = int(re.search(r'FONT_FIRST\s+(\d+)', open(os.path.join(BUILD, 'font_data.h')).read()).group(1))
widths = [int(v, 0) for v in re.search(r'font_widths\[\d*\][^{]*\{([^}]*)\}', src).group(1).split(',') if v.strip()]


def text_width(s):
    return sum(widths[ord(c) - first] + 1 for c in s) - 1 if s else 0


FLAVORS = ['Vanilla', 'Strawberry', 'Matcha', 'Taro', 'Sparkle']
SPECIES = ['Bunny', 'Kitty', 'Bear', 'Chick', 'Frog', 'Fox', 'Dino', 'Shroom',
           'Whale', 'Octo', 'Seal', 'Crab', 'Cloud', 'Star', 'Planet', 'Uni']
NAMES = [f'{f} {s}' for s in SPECIES for f in FLAVORS]
LETTER = ['Dear Pip,', 'Love,', 'Thank you for', 'finding me!', 'I love my', 'new home!', 'You give the',
          'best squishes!', "Let's play", 'again soon!', 'I made a new', 'friend today!', 'Hugs and',
          'squishes for you!', 'I had a yummy', 'snack today!', 'You are my', 'best friend!', 'I found a',
          'shiny pebble!', 'Today was a', 'happy day!', 'I did a big', 'happy hop!', 'Come and visit', 'me soon!']

STRIPS = [   # where (source: strip_init), width in px, the texts printed there
    ('shelf name pill (shelf.c, 13 tiles)', 13 * 8, NAMES + ['Find me!', 'Who follows Pip?']),
    ('reveal and close-up name (closeup.c, 16 tiles)', 16 * 8, NAMES),
    ('house pill (house.c, 16 tiles)', 16 * 8, ['From ' + n for n in NAMES] + ['Find me!']),
    ('letter lines (letter.c, 13 tiles, left aligned)', 13 * 8, NAMES + LETTER),
    ('title options (title.c, 12 tiles)', 12 * 8, ['Play', 'Continue', 'New game', 'No', 'Yes, hold A']),
    ('title confirm lines (title.c, 16 and 24 tiles)', 16 * 8, ['Start over?']),
    ('title confirm line 2 (title.c, 24 tiles)', 24 * 8, ['Your friends will go home.']),
    ('map counter (viewer.c, 4 tiles after the icon)', 4 * 8, [f'{n}/20' for n in range(21)] + [f'{n}/10' for n in range(11)]),
]
for where, room, texts in STRIPS:
    widest = max(texts, key=text_width)
    over = [t for t in texts if text_width(t) > room - 2]    # 1 px clear on each side
    check(not over, f'{where}: widest "{widest}" {text_width(widest)} of {room} px' + (f', too wide: {over[:4]}' if over else ''))
if fails:
    sys.exit(1)
print('all text checks passed')
