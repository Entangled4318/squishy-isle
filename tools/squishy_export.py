"""Squishy sprites for the ROM with flavor palette swaps.

Each species is rendered once with a synthetic "role" flavor whose seven
ramp colors are unique. Pixels of those colors become fixed palette slots,
so one set of tiles serves all five flavors: the ROM only swaps the 16-color
palette. Face and accent colors get the remaining slots.
"""
from gba import colors_of, from5
from gbaconv import bgr555, obj
from palette import FL, FLAVOR_ORDER
from squishies import SPECIES, render

ROLE = [from5((1, 1, k)) for k in range(1, 8)]
FL['_role'] = ROLE

# (expression, squash) for each stored animation frame
FRAMES = {
    'idle': ('happy', (1.0, 1.0)),
    'squish': ('squish', (1.17, 0.74)),
    'open': ('happy_open', (1.0, 1.0)),
    'stretch': ('happy_open', (0.9, 1.08)),
}


def export(species, size, frames=('idle', 'squish')):
    """Returns (tiles per frame, {flavor: 16-entry BGR555 palette})."""
    imgs = [render(SPECIES[species], '_role', size, expr=FRAMES[f][0], squash=FRAMES[f][1])
            for f in frames]
    colors = set()
    for im in imgs:
        colors |= colors_of(im)
    roles = [i for i, c in enumerate(ROLE) if c in colors]
    fixed = sorted(colors - set(ROLE))
    slots = [('role', i) for i in roles] + [('fixed', c) for c in fixed]
    if len(slots) > 15:
        raise ValueError(f'{species}: {len(slots)} colors')
    lookup = {}
    for n, (kind, v) in enumerate(slots, 1):
        lookup[ROLE[v] if kind == 'role' else v] = n
    tiles = [obj(im, lookup) for im in imgs]
    pals = {}
    for fl in FLAVOR_ORDER:
        pal = [0] * 16
        for n, (kind, v) in enumerate(slots, 1):
            pal[n] = bgr555(FL[fl][v] if kind == 'role' else v)
        pals[fl] = pal
    return tiles, pals
