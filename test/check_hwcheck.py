"""Checks for the hardware check ROM run. Usage: check_hwcheck.py OUTDIR

Converts screenshots to 4x PNGs and fails loudly when a check does not hold.
"""
import glob
import os
import re
import struct
import sys
import wave

from PIL import Image

OUT = sys.argv[1]
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


# screenshots -> png
shots = {}
for ppm in sorted(glob.glob(os.path.join(OUT, '*.ppm'))):
    im = Image.open(ppm).convert('RGB')
    name = os.path.splitext(os.path.basename(ppm))[0]
    shots[name] = im
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(OUT, name + '.png'))

# logs
log1 = open(os.path.join(OUT, 'boot1.log')).read()
log2 = open(os.path.join(OUT, 'boot2.log')).read()
check('boot: save=NEW boots=1' in log1, 'first boot starts a new save')
check('boot: save=OK boots=2' in log2, 'second boot loads the save (boots=2)')
m = re.search(r'squishes=(\d+)', log2)
check(m and int(m.group(1)) >= 2, 'squish count survives a restart')
check('flavor Sparkle' in log1, 'L/R cycles through all flavors')
check('ERROR' not in log1 + log2 and 'failed' not in log1 + log2, 'no save write errors')

# save file
sav = open(os.path.join(OUT, 'test.sav'), 'rb').read()
check(len(sav) >= 512, f'save file written ({len(sav)} bytes)')
check(b'SQIS' in sav[:0x200], 'save magic present in SRAM slots')

# pixels
boot = shots['01_first_boot']
check(boot.getpixel((2, 2)) != boot.getpixel((3, 2)), 'corner 1px checker pattern intact')
bg = boot.getpixel((120, 90))
squish = shots['02_squish']
hop = shots['03_hop']


def bbox_changed(a, b):
    diff = [(x, y) for y in range(20, 140) for x in range(140, 236) if a.getpixel((x, y)) != b.getpixel((x, y))]
    return len(diff)


check(bbox_changed(boot, squish) > 200, 'squish changes the bunny sprite')
check(bbox_changed(boot, hop) > 200, 'hop moves the bunny')
check(shots['04_strawberry'].getpixel((188, 80)) != boot.getpixel((188, 80)), 'flavor palette swap changes colors')
lit = shots['07_buttons_lit']
check(lit.getpixel((17, 149)) != boot.getpixel((17, 149)), 'held button icon lights up')

# audio: RMS per 1/30 s window
w = wave.open(os.path.join(OUT, 'hwcheck_sfx.wav'))
rate = w.getframerate()
data = w.readframes(w.getnframes())
samples = struct.unpack('<%dh' % (len(data) // 2), data)
win = rate // 30 * 2
rms = []
for i in range(0, len(samples) - win, win):
    chunk = samples[i:i + win]
    rms.append((sum(s * s for s in chunk) / len(chunk)) ** 0.5)
loud = [r > 300 for r in rms]
segments = sum(1 for i in range(1, len(loud)) if loud[i] and not loud[i - 1])
peak = max(rms) if rms else 0
print(f'audio: {len(rms)} windows, {segments} sound events, peak RMS {peak:.0f}')
check(segments >= 4, 'squeak, boing, chimes and tune are audible')
check(peak < 12000, 'sound is not harsh (peak RMS under 12000)')

# pitch checks: the squeak must rise, the tune must hit its notes
try:
    import numpy as np
    d = np.array(samples, dtype=float).reshape(-1, 2).mean(axis=1)

    def peak_hz(t0, t1):
        seg = d[int(t0 * rate):int(t1 * rate)]
        seg = seg - seg.mean()
        spec = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), n=16384))
        fr = np.fft.rfftfreq(16384, 1 / rate)
        m = fr > 300
        return fr[m][np.argmax(spec[m])]

    early, late = peak_hz(0.0, 0.017), peak_hz(0.083, 0.117)
    check(early < 700 and late > 1150, f'squeak rises ({early:.0f} Hz to {late:.0f} Hz)')
    win = rate // 60
    heard = set()
    for i in range(0, len(d) - win, win):
        seg = d[i:i + win] - d[i:i + win].mean()
        if np.sqrt((seg ** 2).mean()) < 300:
            continue
        f = peak_hz(i / rate, (i + win) / rate)
        for note in (523, 659, 784, 1047):
            if abs(f - note) < note * 0.03:
                heard.add(note)
    check(heard == {523, 659, 784, 1047}, f'tune plays C5 E5 G5 C6 (heard {sorted(heard)})')
except ImportError:
    print('SKIP pitch checks (numpy missing)')

if fails:
    sys.exit(f'{len(fails)} check(s) failed')
print('all checks passed')
