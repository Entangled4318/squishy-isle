"""Export all game art to C. Usage: python3 export_game.py OUTDIR

Writes OUTDIR/game_assets.c/.h and preview PNGs of every BG screen.
"""
import os
import sys

import numpy as np

from gba import new, blit, rgb15, save_scaled, W, H
from palette import C, FLAVOR_ORDER, FLAVOR_NAME
from gbaconv import bg, obj, bgr555, CWriter
from squishies import AREAS, SPECIES
import squishy_export
import mockups
import props
import areas
from pip import frames as pip_frames

OUT = sys.argv[1] if len(sys.argv) > 1 else 'build'
PILL_W = 104          # name pill on the shelf (top right)


def palette_and_lookup(images):
    cs = set()
    for im in images:
        cs |= {tuple(int(v) for v in p) for p in im[..., :3][im[..., 3] > 0]}
    cs = sorted(cs)
    if len(cs) > 15:
        raise ValueError(f'{len(cs)} colors')
    lookup = {c: i + 1 for i, c in enumerate(cs)}
    pal = [0] * 16
    for c, i in lookup.items():
        pal[i] = bgr555(c)
    return lookup, pal


def pad_to(img, w, h):
    out = new(w, h)
    blit(out, img, 0, 0)
    return out


def map_rows(m, w_tiles, h_tiles, stride=32):
    rows = []
    for r in range(h_tiles):
        rows += m[r * w_tiles:(r + 1) * w_tiles] + [0] * (stride - w_tiles)
    return rows


def export_multi_bg(cw, prefix, images, max_pals=15):
    """Several full-screen images sharing one tile set and palette set."""
    stack = np.concatenate(images, axis=0)
    b = bg(stack, pal_bank=0, max_pals=max_pals)
    th = H // 8
    cw.u32_bytes(f'{prefix}_tiles', b['tiles'])
    cw.u16(f'{prefix}_pal', [c for p in b['palettes'] for c in p])
    for i in range(len(images)):
        part = b['map'][i * th * 30:(i + 1) * th * 30]
        cw.u16(f'{prefix}_map{i}', map_rows(part, 30, th))
    cw.define(f'{prefix.upper()}_PALS', len(b['palettes']))
    for i, im in enumerate(images):
        save_scaled(im, os.path.join(OUT, f'preview_{prefix}{i}.png'), 2)
    print(f'{prefix}: {b["ntiles"]} tiles, {len(b["palettes"])} palettes')
    return b


def closeup_background():
    img = mockups.sunburst(rgb15('#fff6d8'), rgb15('#ffeab8'), rgb15('#fffbee'), cy=74, glow_r=48)
    blit(img, mockups.stage(), 68, 110)
    p = props.pill(128, 15, C['ui_bg'], C['ui_pk'], C['ui_ink'], C['white'])
    blit(img, p, 56, 141)
    return img


def open_background():
    img = mockups.sunburst(rgb15('#f3edff'), rgb15('#e7ddfb'), rgb15('#fbf8ff'), cy=74, glow_r=48)
    blit(img, mockups.stage(), 68, 110)
    return img


def main():
    os.makedirs(OUT, exist_ok=True)
    cw = CWriter('game_assets')
    cw.h.append('\ntypedef struct {\n    const char *name;\n    uint8_t area;\n'
                '    const uint32_t *t16, *t32, *t64;\n} SpeciesInfo;\n\n')

    # ---------------- squishies
    species_rows = []
    for ai, area in enumerate(AREAS):
        ex = squishy_export.area_export(area[0])
        cw.u16(f'sq_pal_{area[0]}', [c for fl in FLAVOR_ORDER for c in ex['palettes'][fl]])
        for sp in area[3]:
            for size in (16, 32, 64):
                cw.u32_bytes(f'sq_{sp}_{size}', b''.join(ex['tiles'][sp][size]))
            species_rows.append((sp, ai))
    sil = squishy_export.area_export('meadow')['silhouettes']
    cw.u16('sq_sil_pal', [c for r in range(4) for c in sil[r]])
    cw.c.append('const SpeciesInfo species_info[16] = {\n' + ',\n'.join(
        f'    {{"{SPECIES[sp].name}", {ai}, sq_{sp}_16, sq_{sp}_32, sq_{sp}_64}}' for sp, ai in species_rows)
        + '\n};\n')
    cw.h.append('extern const SpeciesInfo species_info[16];\n')
    cw.c.append('const uint16_t *const sq_area_pals[4] = {' +
                ', '.join(f'sq_pal_{a[0]}' for a in AREAS) + '};\n')
    cw.h.append('extern const uint16_t *const sq_area_pals[4];\n')
    cw.c.append('const char *const flavor_names[5] = {' +
                ', '.join(f'"{FLAVOR_NAME[f]}"' for f in FLAVOR_ORDER) + '};\n')
    cw.h.append('extern const char *const flavor_names[5];\n')
    cw.c.append('const char *const area_names[4] = {' + ', '.join(f'"{a[1]}"' for a in AREAS) + '};\n')
    cw.h.append('extern const char *const area_names[4];\n')
    for k, v in (('SQ_FRAMES16_IDLE', 0), ('SQ_FRAMES16_SQUISH', 1), ('SQ32_IDLE', 0), ('SQ32_OPEN', 1),
                 ('SQ64_IDLE', 0), ('SQ64_BLINK', 1), ('SQ64_OPEN', 2), ('SQ64_SQUISH', 3)):
        cw.define(k, v)

    # ---------------- shelf pages (one per active tab)
    pages = [mockups.shelf_background(i, pill_w=PILL_W) for i in range(4)]
    export_multi_bg(cw, 'shelf', pages)
    for k, v in mockups.SHELF.items():
        cw.define(f'SHELF_{k.upper()}', v)
    cw.define('SHELF_PILL_W', PILL_W)

    # ---------------- close-up screen
    export_multi_bg(cw, 'closeup', [closeup_background()])

    # ---------------- open screen: box in 5 colors (closed body, open body, lid)
    export_multi_bg(cw, 'openbg', [open_background()])
    for color in ('pink', 'lav', 'mint', 'yellow', 'sky'):
        parts = [props.box64(color, 'body'), props.box64(color, 'open'), props.box64(color, 'lid')]
        lk, pal = palette_and_lookup(parts)
        cw.u32_bytes(f'box64_{color}_tiles', b''.join(obj(p, lk) for p in parts))
        cw.u16(f'box64_{color}_pal', pal)
    cw.h.append('extern const uint32_t *const box64_tiles[5];\nextern const uint16_t *const box64_pal[5];')
    cw.c.append('const uint32_t *const box64_tiles[5] = {' + ', '.join(f'box64_{c}_tiles' for c in ('pink', 'lav', 'mint', 'yellow', 'sky')) + '};')
    cw.c.append('const uint16_t *const box64_pal[5] = {' + ', '.join(f'box64_{c}_pal' for c in ('pink', 'lav', 'mint', 'yellow', 'sky')) + '};')
    abig = [pad_to(props.a_button_big(False), 32, 32), pad_to(props.a_button_big(True), 32, 32)]
    lk, pal = palette_and_lookup(abig)
    cw.u32_bytes('abig_tiles', b''.join(obj(a, lk) for a in abig))
    cw.u16('abig_pal', pal)
    heart_off = mockups.hearts_progress(0, 1)
    x2 = lambda im: np.repeat(np.repeat(im, 2, axis=0), 2, axis=1)
    bits = [pad_to(x2(props.HEART), 16, 16), pad_to(x2(heart_off), 16, 16), pad_to(mockups.motion_arcs('left'), 8, 8),
            pad_to(mockups.puff(), 16, 8)]
    lk, pal = palette_and_lookup(bits)
    cw.u32_bytes('openui_tiles', b''.join(obj(b, lk) for b in bits))
    cw.u16('openui_pal', pal)
    cw.define('OPENUI_HEART_ON', 0)
    cw.define('OPENUI_HEART_OFF', 4)
    cw.define('OPENUI_ARC', 8)
    cw.define('OPENUI_PUFF', 9)

    # ---------------- reveal: confetti and big star
    conf = [pad_to(props.confetti_piece(i), 8, 8) for i in range(12)]
    lk, pal = palette_and_lookup(conf)
    cw.u32_bytes('confetti_tiles', b''.join(obj(c, lk) for c in conf))
    cw.u16('confetti_pal', pal)
    cw.define('CONFETTI_N', len(conf))
    star = new(16, 16)
    blit(star, props.SPARK_HUGE, 2, 2)
    lk, pal = palette_and_lookup([star])
    cw.u32_bytes('star16_tiles', obj(star, lk))
    cw.u16('star16_pal', pal)

    # ---------------- UI sprites
    frame = pad_to(mockups.selection_frame(), 64, 32)
    lk, pal = palette_and_lookup([frame])
    cw.u32_bytes('ui_frame_tiles', obj(frame, lk))
    cw.u16('ui_frame_pal', pal)

    sparks = [props.SPARK_TINY, props.SPARK_SMALL, props.SPARK_BIG]
    cells = []
    for sp in sparks:
        f = new(8, 8)
        blit(f, sp, (8 - sp.shape[1]) // 2, (8 - sp.shape[0]) // 2)
        cells.append(f)
    heart = new(8, 8)
    blit(heart, props.HEART, 0, 0)
    cells.append(heart)
    lk, pal = palette_and_lookup(cells)
    cw.u32_bytes('ui_small_tiles', b''.join(obj(c, lk) for c in cells))
    cw.u16('ui_small_pal', pal)
    cw.define('UI_TWINKLE0', 0)
    cw.define('UI_HEART', 3)

    shadow = props.shadow(64, 8, rgb15('#c9789a'))[:, :32]     # left half; right half is h-flipped
    cw.u32_bytes('ui_shadow_tiles', obj(shadow, {rgb15('#c9789a'): 1}))
    cw.u16('ui_shadow_pal', [0, bgr555(rgb15('#c9789a'))] + [0] * 14)

    cw.u16('ui_text_pal', [0, bgr555(C['ui_ink']), bgr555(C['white']), bgr555(C['ui_line'])] + [0] * 12)

    # ---------------- Pip
    pf = pip_frames()
    order = ['down0', 'down1', 'down2', 'up0', 'up1', 'up2', 'right0', 'right1']
    lk, pal = palette_and_lookup([pf[k] for k in order])
    cw.u32_bytes('pip_tiles', b''.join(obj(pf[k], lk) for k in order))
    cw.u16('pip_pal', pal)
    feet = max(int(np.nonzero(pf[k][..., 3].any(axis=1))[0][-1]) for k in order)
    cw.define('PIP_FEET_ROW', feet)
    small_shadow = props.shadow(12, 4, rgb15('#6a5a88'))
    sh = new(16, 8)
    blit(sh, small_shadow, 2, 2)
    cw.u32_bytes('shadow16_tiles', obj(sh, {rgb15('#6a5a88'): 1}))
    cw.u16('shadow16_pal', [0, bgr555(rgb15('#6a5a88'))] + [0] * 14)

    # ---------------- meadow sprites: gift boxes, A bubble, guide arrow
    from gbaconv import tile4
    idx = props.box16_index()
    cw.u32_bytes('box16_tiles', b''.join(tile4(idx[ty:ty + 8, tx:tx + 8]) for ty in (0, 8) for tx in (0, 8)))
    box_pals = []
    for color in ('pink', 'lav', 'mint', 'yellow', 'sky'):
        box_pals += [0] + [bgr555(c) for c in props.box16_palette(color)] + [0] * (15 - len(props.BOX16_KEYS))
    cw.u16('box16_pal', box_pals)
    cw.define('BOX_COLORS', 5)

    abtn = pad_to(props.a_button(), 16, 32)
    lk, pal = palette_and_lookup([abtn])
    cw.u32_bytes('abubble_tiles', obj(abtn, lk))
    cw.u16('abubble_pal', pal)

    arrows = [props.guide_arrow(d) for d in ('right', 'up', 'upright')]
    lk, pal = palette_and_lookup(arrows)
    cw.u32_bytes('arrow_tiles', b''.join(obj(a, lk) for a in arrows))
    cw.u16('arrow_pal', pal)
    save_scaled(np.concatenate(arrows + [pad_to(props.box16(c), 16, 16) for c in ('pink', 'lav', 'mint', 'yellow', 'sky')], axis=1),
                os.path.join(OUT, 'meadow_sprites.png'), 8)

    # ---------------- areas
    for area in (areas.meadow(),):
        export_area(cw, area)
    cw.save(OUT)


def export_area(cw, a):
    stack = np.concatenate([a.ground, a.overlay], axis=0)
    b = bg(stack, pal_bank=0, max_pals=16)
    if b['ntiles'] > 1024:
        raise ValueError(f'{a.name}: {b["ntiles"]} tiles > 1024')
    tw, th = a.w // 8, a.h // 8

    def sb64(entries):
        # 64x64 map in 4 screenblocks: (0,0) (1,0) (0,1) (1,1)
        out = [0] * 4096
        for ty in range(th):
            for tx in range(tw):
                sbn = (ty // 32) * 2 + (tx // 32)
                out[sbn * 1024 + (ty % 32) * 32 + (tx % 32)] = entries[ty * tw + tx]
        return out

    n = tw * th
    cw.u32_bytes(f'{a.name}_tiles', b['tiles'])
    cw.u16(f'{a.name}_pal', [c for p in b['palettes'] for c in p])
    cw.u16(f'{a.name}_ground', sb64(b['map'][:n]))
    cw.u16(f'{a.name}_overlay', sb64(b['map'][n:]))
    solid = a.solid.astype(np.uint8).flatten().tolist()
    cw.c.append(f'const uint8_t {a.name}_solid[{len(solid)}] = {{' + ','.join(map(str, solid)) + '};\n')
    cw.h.append(f'extern const uint8_t {a.name}_solid[{len(solid)}];\n')
    up = a.name.upper()
    cw.define(f'{up}_W', a.w)
    cw.define(f'{up}_H', a.h)
    cw.define(f'{up}_PALS', len(b['palettes']))
    cw.define(f'{up}_SPAWN_X', a.spawn[0])
    cw.define(f'{up}_SPAWN_Y', a.spawn[1])
    spots = [v for p in a.spots for v in p]
    cw.u16(f'{a.name}_spots', spots)
    cw.define(f'{up}_FIRST_SPOT', a.first_spot)
    cw.define(f'{up}_NSPOTS', len(a.spots))
    # palette slots holding the water shimmer color, for runtime cycling
    target = bgr555(C['w_lt'])
    slots = [bi * 16 + ci for bi, p in enumerate(b['palettes']) for ci, c in enumerate(p) if ci and c == target]
    cw.u16(f'{a.name}_shimmer', slots or [0])
    cw.define(f'{up}_NSHIMMER', len(slots))
    lo, hi = C['w_lt'], C['w_hi']
    mid = tuple((x + y) // 2 for x, y in zip(lo, hi))
    cw.u16('water_shimmer_cycle', [bgr555(lo), bgr555(mid), bgr555(hi), bgr555(mid)])
    doors = [v for d in a.doors for v in d[:4]]
    cw.u16(f'{a.name}_doors', doors)
    cw.define(f'{up}_NDOORS', len(a.doors))
    full = a.ground.copy()
    m = a.overlay[..., 3] > 0
    full[m] = a.overlay[m]
    save_scaled(full, os.path.join(OUT, f'preview_{a.name}.png'), 2)
    print(f'{a.name}: {b["ntiles"]} tiles, {len(b["palettes"])} palettes')


if __name__ == '__main__':
    main()
