"""Music checks on the real mGBA core. Usage: check_music.py OUTDIR ROM

For every song in music_songs.json (written next to the ROM by
tools/music.py) this drives the sound test screen (L + R + START at boot)
and records the lead (channel 3) and the bass (channel 2) alone, two loops
each, then the full mix. It checks:
  pitch   every note held 6+ frames is within 25 cents of its target
  tempo   the game logs each loop exactly one song length apart
  loop    the second loop sounds like the first (loudness envelope)
  volume  the mix never clips, the bass sits under the lead
  clicks  no voice jumps by more than one volume step in a frame
Also checks that a jingle pauses the song and that the song goes on after
the jingle and after the boing.
"""
import json
import os
import re
import subprocess
import sys
import wave

import numpy as np

OUT, ROM = sys.argv[1], sys.argv[2]
HERE = os.path.dirname(os.path.abspath(__file__))
HARNESS = os.path.join(HERE, 'build', 'harness')
SONGS = json.load(open(os.path.join(os.path.dirname(ROM), 'music_songs.json')))
FPS = 16777216 / 280896
# clicks: largest DC jump in a frame / the voice's full loudness (99th
# percentile). PSG channels only output positive values, so any volume
# change moves the DC level. A click is a jump bigger than the smallest
# step the channel can make, with 20% margin:
#   lead: one wave volume level (25%) from its full level:
#         (0.25 / full level) * mean / std of the wave (0.47 for a sine at 75%)
#   bass: a square note starting from silence, sqrt(d / (1 - d)) for duty d
#         (0.58 at 25%): a plucked attack. A hard cut is bigger.
DUTY = (0.125, 0.25, 0.5)
LEVEL = (1.0, 0.75, 0.5, 0.25)
fails = []


def check(cond, msg):
    print(('PASS ' if cond else 'FAIL ') + msg)
    if not cond:
        fails.append(msg)


def run(name, lines):
    path = os.path.join(OUT, name + '.txt')
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    log = subprocess.run([HARNESS, ROM, os.path.join(OUT, 'music.sav'), path, OUT],
                         capture_output=True, text=True, check=True).stdout
    open(os.path.join(OUT, name + '.log'), 'w').write(log)
    return log


def load(name):
    w = wave.open(os.path.join(OUT, name + '.wav'))
    a = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).reshape(-1, 2)[:, 0].astype(float)
    return a, w.getframerate()


def frames(a, sr):
    """Per-frame DC level and loudness (std)."""
    spf = sr / FPS
    k = int(len(a) // spf)
    seg = [a[int(i * spf):int((i + 1) * spf)] for i in range(k)]
    return np.array([s.mean() for s in seg]), np.array([s.std() for s in seg])


def yin(x, sr, fmin=55, fmax=2200):
    """Fundamental frequency by the YIN difference function."""
    x = x - x.mean()
    tmin, tmax = int(sr / fmax), int(sr / fmin)
    n = len(x) - tmax
    if n < tmax:
        return 0.0
    d = np.array([np.sum((x[:n] - x[t:t + n]) ** 2) for t in range(tmax + 1)])
    cmnd = d.copy()
    cmnd[0] = 1
    cmnd[1:] = d[1:] * np.arange(1, tmax + 1) / np.maximum(np.cumsum(d[1:]), 1e-9)
    t = tmin
    while t < tmax and cmnd[t] > 0.15:
        t += 1
    if t >= tmax:
        t = tmin + int(np.argmin(cmnd[tmin:]))
    while t + 1 < tmax and cmnd[t + 1] < cmnd[t]:
        t += 1
    if 0 < t < tmax:
        a, b, c = cmnd[t - 1], cmnd[t], cmnd[t + 1]
        den = a - 2 * b + c
        t = t + (0.5 * (a - c) / den if den else 0)
    return sr / t


def hz(m):
    return 440.0 * 2 ** ((m - 69) / 12)


def timeline(events, tick):
    """(midi, start frame, length in frames) for each note of one loop."""
    out, f = [], 0
    for m, n in events:
        out.append((m, f, n * tick))
        f += n * tick
    return out, f


def onset(ac, full):
    i = int(np.argmax(ac > 0.2 * full))
    return i


def voice_checks(key, s, voice, a, sr, loops):
    tick = s['tick']
    notes, length = timeline(s[voice], tick)
    dc, ac = frames(a, sr)
    full = np.median(ac[ac > 0.5 * ac.max()]) if ac.max() > 0 else 0
    check(full > 300, f'{key} {voice}: the voice sounds (loudness {full:.0f})')
    if full <= 300:
        return None
    first = next(f for m, f, _ in notes if m)          # a voice may start with a rest
    t0 = onset(ac, full) - first
    spf = sr / FPS
    worst, bad = 0.0, []
    gap = s['lead_gap'] if voice == 'lead' else s['bass_gap']
    for loop in range(loops):
        for m, f, ln in notes:
            if not m or ln < 6:
                continue
            f0 = t0 + loop * length + f + 3
            f1 = t0 + loop * length + f + ln - gap - 1
            if f1 - f0 < 3:
                continue
            got = yin(a[int(f0 * spf):int(f1 * spf)], sr)
            cents = 1200 * np.log2(got / hz(m)) if got > 0 else 9999
            worst = max(worst, abs(cents))
            if abs(cents) > 25:
                bad.append(f'note {m} at frame {f + loop * length}: {got:.0f} Hz')
    check(not bad, f'{key} {voice}: every note in tune over {loops} loop(s) (worst {worst:.1f} cents)'
          + (f' {bad[:3]}' if bad else ''))
    loud = np.percentile(ac, 99)                     # the voice at full volume (fading voices are quiet most of the time)
    jump = np.abs(np.diff(dc)).max() / loud
    if voice == 'lead':
        w = np.array(s['wave_samples'], float)
        limit = round(1.2 * 0.25 / LEVEL[s['lead_level']] * w.mean() / w.std(), 2)
    else:
        d = DUTY[s['bass_duty']]
        limit = round(1.2 * (d / (1 - d)) ** 0.5, 2)
    check(jump <= limit, f'{key} {voice}: no clicks (largest step {jump:.2f} of the voice, limit {limit})')
    if loops >= 2:
        e1 = ac[t0:t0 + length]
        e2 = ac[t0 + length:t0 + 2 * length]
        r = float(np.corrcoef(e1, e2)[0, 1]) if len(e2) == len(e1) and e1.std() > 0 else 1.0
        check(r > 0.95, f'{key} {voice}: loop 2 matches loop 1 (envelope r={r:.3f})')
    return full, a[int(t0 * spf):int((t0 + length) * spf)], (a, sr, t0)


def piano_roll(key, s, recs):
    """OUTDIR/roll_KEY.png: the notes as written (bars) and the pitch heard
    in the emulator (dots, every 4 frames), one loop."""
    from PIL import Image, ImageDraw
    _, length = timeline(s['lead'], s['tick'])
    ms = [m for v in ('lead', 'bass') for m, _ in s[v] if m]
    lo, hi = min(ms) - 2, max(ms) + 2
    px, row = 0.5, 6
    W, H = int(length * px) + 20, (hi - lo + 1) * row + 20
    im = Image.new('RGB', (W, H), (255, 250, 247))
    d = ImageDraw.Draw(im)
    for m in range(lo, hi + 1):
        if m % 12 == 0:
            d.line([(10, 10 + (hi - m) * row + row // 2), (W - 10, 10 + (hi - m) * row + row // 2)], fill=(230, 215, 225))
    for f in range(0, length + 1, s['tick'] * s.get('bar', 16)):
        d.line([(10 + f * px, 10), (10 + f * px, H - 10)], fill=(225, 210, 220))
    colors = {'lead': ((255, 196, 214), (176, 64, 110)), 'bass': ((214, 200, 246), (84, 64, 150))}
    for v in ('lead', 'bass'):
        notes, _ = timeline(s[v], s['tick'])
        for m, f, ln in notes:
            if m:
                y = 10 + (hi - m) * row
                d.rectangle([10 + f * px, y, 10 + (f + ln) * px - 1, y + row - 1], fill=colors[v][0])
        if v in recs:
            a, sr, t0 = recs[v]
            spf = sr / FPS
            for f in range(0, length, 4):
                seg = a[int((t0 + f) * spf):int((t0 + f + 4) * spf)]
                if len(seg) < 100 or seg.std() < 300:
                    continue
                got = yin(seg, sr)
                if got <= 0:
                    continue
                m = 69 + 12 * np.log2(got / 440)
                y = 10 + (hi - m) * row + row / 2
                d.ellipse([10 + (f + 2) * px - 1.5, y - 1.5, 10 + (f + 2) * px + 1.5, y + 1.5], fill=colors[v][1])
    im.save(os.path.join(OUT, f'roll_{key}.png'))


def song_script(index, s, voice_solo, name, loops):
    notes, length = timeline(s['lead'], s['tick'])
    lines = ['hold L+R+START 5', 'wait 30'] + ['tap RIGHT'] * index
    lines += [f'solo {voice_solo}', f'audio {name}', 'wait 10', 'tap A', f'wait {length * loops + 30}',
              'audio end', 'tap B', 'wait 10']
    return lines, length


for key, s in SONGS.items():
    loops = 2 if s['loop'] else 1
    rms, recs = {}, {}
    for voice, ch in (('lead', 3), ('bass', 2)):
        name = f'mus_{key}_{voice}'
        lines, length = song_script(s['index'], s, ch, name, loops)
        log = run(name, lines)
        if s['loop']:
            play = [int(f) for f in re.findall(r'\[game f(\d+)\] music play', log)]
            lp = [int(f) for f in re.findall(r'\[game f(\d+)\] music loop', log)]
            ok = len(play) == 1 and len(lp) >= 2 and all(b - a == length for a, b in zip([play[0] + 1] + lp, lp))
            if voice == 'lead':
                check(ok, f'{key}: tempo exact, a loop every {length} frames ({s["tick"]} frames per tick)'
                      f' (play f{play[0] if play else "?"}, loops {lp[:3]})')
        a, sr = load(name)
        res = voice_checks(key, s, voice, a, sr, loops)
        if res:
            rms[voice] = float(np.sqrt(np.mean((res[1] - res[1].mean()) ** 2)))
            recs[voice] = res[2]
    piano_roll(key, s, recs)
    name = f'mus_{key}_mix'
    lines, length = song_script(s['index'], s, 0, name, 1)
    run(name, lines)
    a, sr = load(name)
    peak = np.abs(a).max()
    check(peak < 30000, f'{key} mix: no clipping (peak {peak:.0f})')
    if 'lead' in rms and 'bass' in rms:
        ratio = rms['bass'] / rms['lead']
        check(0.25 <= ratio <= 0.65, f'{key} mix: bass under the lead (bass/lead loudness {ratio:.2f}, 0.25..0.65)')

# a jingle pauses the song; after it, and after the boing, the song goes on
first = next(iter(SONGS.values()))
lines = ['hold L+R+START 5', 'wait 30'] + ['tap RIGHT'] * first['index'] + [
    'solo 3', 'audio mus_fx_lead', 'wait 10', 'tap A', 'wait 60', 'tap SELECT', 'wait 150', 'wait 90',
    'audio end', 'tap B', 'wait 10', 'solo 2', 'audio mus_fx_bass', 'tap A', 'wait 100', 'tap L', 'wait 200',
    'audio end']
run('mus_fx', lines)
a, sr = load('mus_fx_lead')
dc, ac = frames(a, sr)
t = int(np.argmax(ac > 0.2 * ac.max()))
jingle = ac[t + 66:t + 66 + 80]        # the jingle (hello) plays from about here
after = ac[t + 66 + 110:t + 66 + 170]  # the song again
check(jingle.max() > 0 and (after > 0.2 * ac.max()).mean() > 0.6,
      f'jingle: the song goes on after it (sounding {100 * (after > 0.2 * ac.max()).mean():.0f}% of frames after)')
a, sr = load('mus_fx_bass')
dc, ac = frames(a, sr)
before, after = ac[:100].max(), ac[100 + 30:].max()     # the boing plays at frame ~100 for 16 frames
check(after > 0.7 * before, f'boing: the bass notes come back after it (peak {after:.0f}, before {before:.0f})')

# mix: every effect is at least as loud as the songs usually are (the 90th
# percentile of 100 ms loudness of each looping song's mix), so the music
# never hides a chime, squeak or boing
def loud100(a, sr):
    n = int(sr / 10)
    return np.array([a[i:i + n].std() for i in range(0, len(a) - n, n // 4)])


song_loud = {}
for key, s in SONGS.items():
    if s['loop']:
        a, sr = load(f'mus_{key}_mix')
        r = loud100(a[int(sr * 0.4):], sr)
        song_loud[key] = float(np.percentile(r[r > 50], 90))
run('mus_levels', ['hold L+R+START 5', 'wait 30', 'audio mus_levels', 'wait 10', 'tap UP', 'wait 60',
                   'tap DOWN', 'wait 60', 'tap L', 'wait 60', 'audio end'])
a, sr = load('mus_levels')
spf = sr / FPS
top = max(song_loud.values())
typical = float(np.median(list(song_loud.values())))
for name, f0 in (('chime', 10), ('squeak', 71), ('boing', 132)):
    fx = loud100(a[int(f0 * spf):int((f0 + 55) * spf)], sr).max()
    check(fx >= typical, f'mix: the {name} is not hidden by the music (loudness {fx:.0f}, songs {typical:.0f} typical, {top:.0f} top)')

if fails:
    sys.exit(1)
print('all music checks passed')
