# Squishy Isle

A calm collecting game for toddlers, made for Game Boy Advance and played on
an emulator (Trimui Brick). Walk around a pastel island, open surprise
containers, squish the mochi friends inside and fill your shelf.
No battles, no fail states, no reading needed.

![Contact sheet](mockups/contact_sheet.png)

- Game design: [docs/DESIGN.md](docs/DESIGN.md)
- Mockups (4x, 960x640): [mockups/](mockups/)
- All 80 squishies: [mockups/squishy_sheet.png](mockups/squishy_sheet.png)

## Status

Phase 1 (mockups) is done. The ROM build comes next.

## Rebuild the mockups

Needs Python 3 with numpy and Pillow.

```
pip install numpy pillow
python3 tools/export_mockups.py
```

The art is procedural and hand-placed pixel data in `tools/`. Every color
is 15-bit and the renderer checks GBA limits (layers, palettes, colors per
tile and sprite), so the same data will feed the ROM.
