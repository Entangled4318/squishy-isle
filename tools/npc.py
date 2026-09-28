"""Momo, the sleepy panda who lies across the way to the next area until
the child has found enough friends there (owner request, step 6).

One character for every gate, so the map needs one sprite palette for it
(it shares the small shadow's OBJ palette: the maps use all 16)."""
import numpy as np

from gba import new, from_ascii
from palette import C
from squishies import Species, Part, Ellipse, Poly, Mochi, mirror, render

# Soft pastel panda: vanilla body, grey-lilac ears and eye patches (light
# enough that the ink eyes still read), a sky-blue nightcap, a pom in the
# body's own colors (14 colors in all: one OBJ palette after the shadow).
PANDA = Species('momo', 'Momo', [
    Part(Ellipse(15, 26, 8.5), z=0, mat='panda2'),
    Part(mirror(Ellipse(15, 26, 8.5)), z=0, mat='panda2'),
    Part(Mochi(32, 42, 27, 20), z=1),
    Part(Ellipse(21.5, 40.5, 6.5, 5.2, rot=-25), z=2, mat='panda2', line=None, clip=2),
    Part(mirror(Ellipse(21.5, 40.5, 6.5, 5.2, rot=-25)), z=2, mat='panda2', line=None, clip=2),
    Part(Poly([(30, 24), (52, 26), (60, 6)], 2.5, R=3), z=3, mat='sky2'),
    Part(Ellipse(58, 8, 4.8), z=4, mat='bodylt'),
], eyes=(21.5, 40.5), mouth=None, nose=47, blush=(12, 48), shine=(17, 31, 4, 2.2, -35))


def frames():
    """32x32 frames: asleep and awake."""
    return {'sleep': render(PANDA, 'vanilla', 32, expr='sleep'), 'awake': render(PANDA, 'vanilla', 32)}


def zz():
    """Two 8x8 tiles, a small and a big z, in the panda's ink."""
    ink = C['ink2']
    small = from_ascii(['kkk', '..k', '.k.', 'kkk'], {'k': ink})
    big = from_ascii(['kkkkk', '...k.', '..k..', '.k...', 'kkkkk'], {'k': ink})
    out = []
    for z in (small, big):
        t = new(8, 8)
        t[1:1 + z.shape[0], 1:1 + z.shape[1]] = z
        out.append(t)
    return out


if __name__ == '__main__':
    from gba import save_scaled
    f = frames()
    row = np.concatenate([f['sleep'], f['awake']] + [np.pad(z, ((0, 24), (0, 0), (0, 0))) for z in zz()], axis=1)
    cols = {tuple(int(v) for v in p) for p in row[..., :3][row[..., 3] > 0]}
    print(len(cols), 'colors')
    save_scaled(row, 'momo.png', 8)
