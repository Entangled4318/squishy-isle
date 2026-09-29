"""Stress test (step 7.1): a toddler mashes buttons on the real mGBA core.

Usage: mash.py OUTDIR ROM [soak]

Each run starts from a save (blank, mid game with a gate scene due, the
shore with the Cloud Hill scene due, one friend left in every area, all 80
found), boots, and hands the buttons to the harness `mash` driver: random
taps, chords, held directions, A mashing, long idle spells, and walks to
boxes and places. The harness watches every frame (the game's frame
counter goes on, the scene is a known one, the CPU runs from BIOS, ROM or
IWRAM). This script then checks the log and the save:

- no fault, no hang, no save write error
- the open screen and the reveal always end by themselves in time
  (each press restarts their wait, so the limit allows three waits)
- every box that opens reaches the open screen (none lost on the way)
- friends count up one at a time, never twice the same friend
- a new game (which wipes friends) happens only from a save with none
- the save on disk is valid and consistent (areas, gates, lines, mail)

`make test` runs the short set; `make -C test soak` runs many seeds, long.
"""
import os
import re
import struct
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
OUT, ROM = sys.argv[1], sys.argv[2]
SOAK = len(sys.argv) > 3 and sys.argv[3] == 'soak'
HARNESS = os.path.join(HERE, 'build', 'harness')

OPEN_MAX = 3 * 240 + 70 + 30             # each press restarts AUTO_WAIT (3 presses), POP_TIME, a fade (open.c)
REVEAL_MAX = 60 + 3 * 300 + 40 + 30 + 30  # drop, each squish restarts REVEAL_WAIT, calm after the 3rd, hop away, fade (closeup.c)

A_ALL = ','.join(str(i) for i in range(80))
SAVES = [   # name, make_save.py args (FOUND, FOLLOWERS[, GATES[, AREA]]); None = blank
    ('blank', None),
    ('gate', [','.join(map(str, list(range(10)) + list(range(20, 28)))), '0,1,2', '0', '1']),
    ('shore', [','.join(map(str, list(range(20)) + list(range(20, 36)) + list(range(40, 50)))), '0,1', '3', '2']),
    ('last', [','.join(str(i) for i in range(80) if i % 20 != 19), '0,1', '-', '0']),
    ('full', [A_ALL, '0,1,2', '-', '3']),
]
SEEDS = [1, 2, 3, 4, 5, 6] if SOAK else [7]
FRAMES = 60000 if SOAK else 15000

fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


def checksum_old(b):
    s = 0x1234ABCD
    for v in b:
        s = ((s << 5) + (s >> 27) + v) & 0xFFFFFFFF
    return s


def checksum_ok(b, c):
    return c == zlib.crc32(bytes(b)) & 0xFFFFFFFF or c == checksum_old(b)


def load_save(path):
    """The newest valid slot, as a dict, or None."""
    data = open(path, 'rb').read()
    best = None
    for slot in (0, 1):
        b = data[slot * 0x100:slot * 0x100 + 144]
        magic, ver, seq = struct.unpack_from('<IHH', b, 0)
        if magic != 0x53495153 or ver != 2 or not checksum_ok(b[:140], struct.unpack_from('<I', b, 140)[0]):
            continue
        if best is None or ((seq - best['seq']) & 0xFFFF) < 0x8000:
            best = {'seq': seq, 'found': list(b[16:96]), 'opens': struct.unpack_from('<I', b, 96)[0],
                    'followers': list(b[104:107]), 'pip': struct.unpack_from('<HH', b, 108),
                    'area': b[112], 'gates': b[113], 'lines': [list(b[114 + k * 3:117 + k * 3]) for k in range(3)],
                    'mail': b[123], 'mail_new': b[124]}
    return best


def save_problems(s):
    p = []
    f = s['found']
    in_area = [sum(f[a * 20:a * 20 + 20]) for a in range(4)]
    if any(v not in (0, 1) for v in f): p.append('found holds a value other than 0 / 1')
    if s['opens'] != sum(f): p.append(f"opens {s['opens']} != found {sum(f)}")
    if s['area'] > 3 or (s['area'] > 0 and in_area[s['area'] - 1] < 10): p.append(f"in area {s['area']}, not open")
    for a in (1, 2, 3):
        if s['gates'] & (1 << (a - 1)) and in_area[a - 1] < 10: p.append(f'gate {a} built but not open')
    for a, line in enumerate([s['followers']] + s['lines']):
        ids = [v - 1 for v in line if v]
        if line != [v for v in line if v] + [0] * (3 - len(ids)): p.append(f'line {a} has a gap {line}')
        if len(set(ids)) != len(ids): p.append(f'line {a} has a friend twice {line}')
        for i in ids:
            if not (a * 20 <= i < a * 20 + 20) or not f[i]: p.append(f'line {a} holds friend {i} (not found here)')
    if s['mail'] and not f[s['mail'] - 1]: p.append(f"mail from friend {s['mail'] - 1}, not found")
    if s['mail_new'] not in (0, 1) or (s['mail_new'] and not s['mail']): p.append('mail flag without a letter')
    x, y = s['pip']
    if x >= 480 or y >= 320: p.append(f'Pip saved at {x},{y}, off the map')
    return p


def log_problems(log, start_found):
    p = []
    lines = log.splitlines()
    seen, total = set(), start_found
    for i, ln in enumerate(lines):
        if 'save: write FAILED' in ln: p.append('a save write failed')
        if 'title start new' in ln:
            seen, total = set(), 0
        m = re.search(r'box \d open friend (-?\d+)', ln)
        if m:
            nxt = next((l for l in lines[i + 1:] if '[game' in l and 'scene ' in l), '')
            if 'scene open' not in nxt: p.append(f'a box opened but the open screen did not follow: {ln.strip()}')
            if m.group(1) == '-1': p.append(f'a box held no friend: {ln.strip()}')
        m = re.search(r'open pop friend (\d+) found (\d+)', ln)
        if m:
            fid, n = int(m.group(1)), int(m.group(2))
            if fid in seen: p.append(f'friend {fid} came twice')
            seen.add(fid)
            if n != total + 1: p.append(f'count jumped from {total} to {n}')
            total = n
    return p, total


def run(name, args, seed):
    tag = f'mash_{name}_{seed}'
    sav = os.path.join(OUT, tag + '.sav')
    if os.path.exists(sav): os.remove(sav)
    start = 0
    if args:
        subprocess.run([sys.executable, os.path.join(HERE, 'make_save.py'), sav] + args, check=True)
        start = len([v for v in args[0].split(',') if v])
    script = os.path.join(OUT, tag + '.txt')
    # boot, and for a save with friends pick Continue (a toddler can still reach New game while mashing)
    open(script, 'w').write(f'wait 20\ntap A\nwait 15\ntap A\nwait 25\nmash {seed} {FRAMES}\nshot {tag}\n')
    log = subprocess.run([HARNESS, ROM, sav, script, OUT], capture_output=True, text=True, check=True).stdout
    open(os.path.join(OUT, tag + '.log'), 'w').write(log)
    summary = {m.group(1): (int(m.group(2)), int(m.group(3)))
               for m in re.finditer(r'\[mash\] scene (\w+) visits (\d+) stay_max (\d+)', log)}
    m = re.search(r'faults (\d+) hang_max (\d+)', log)
    faults, hang = (int(m.group(1)), int(m.group(2))) if m else (-1, -1)
    lp, total = log_problems(log, start)
    s = load_save(sav)
    sp = save_problems(s) if s else ['no valid save slot']
    visits = ' '.join(f'{k}:{v[0]}' for k, v in summary.items() if v[0])
    wipes = len(re.findall(r'title start new', log)) if start else 0
    check(faults == 0 and hang < 10, f'{tag}: no fault, no hang (faults {faults}, longest still {hang} frames)')
    check(summary.get('open', (0, 0))[1] <= OPEN_MAX and summary.get('reveal', (0, 0))[1] <= REVEAL_MAX,
          f"{tag}: open and reveal end by themselves (longest {summary.get('open', (0, 0))[1]}/{OPEN_MAX}, "
          f"{summary.get('reveal', (0, 0))[1]}/{REVEAL_MAX} frames)")
    check(not lp, f'{tag}: log consistent, friends {start} -> {total}' + (': ' + '; '.join(lp[:3]) if lp else ''))
    check(not sp, f'{tag}: save valid and consistent' + (': ' + '; '.join(sp[:3]) if sp else ''))
    check(wipes == 0, f'{tag}: no new game over a save with friends ({wipes})')
    print(f'     {tag}: {visits}')
    return summary


if __name__ == '__main__':
    totals = {}
    for seed in SEEDS:
        for name, args in SAVES:
            for k, v in run(name, args, seed).items():
                totals[k] = totals.get(k, 0) + v[0]
    reached = [k for k in ('meadow_view', 'open', 'reveal', 'shelf', 'closeup', 'letter', 'house') if totals.get(k)]
    check(len(reached) == 7, f"every play scene was mashed: {' '.join(f'{k}:{totals.get(k, 0)}' for k in totals)}")
    if fails:
        print(f'{len(fails)} mash check(s) FAILED')
        sys.exit(1)
    print('all mash checks passed')
