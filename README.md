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

- Phase 1, mockups: done.
- ROM step 1, toolchain and hardware check ROM: done.
  [release/squishy-isle-hwcheck.gba](release/squishy-isle-hwcheck.gba)
  tests display, sprites, squash, flavor palette swaps, every button,
  sound and the battery save.

## Try the hardware check ROM

1. Open `release/squishy-isle-hwcheck.gba` in mGBA, or copy it to the
   folder that holds your other GBA games on the handheld.
2. A squishes, B hops, L and R change flavor, START plays a tune. Each
   button lights up at the bottom while you hold it.
3. Quit and start it again. "Boots" goes up by one and "Save" reads
   "working" when the battery save works.
4. The "Emulator" line shows "mGBA" or "other" (for example gpSP).
5. The corners hold 1-pixel checkerboards. If they look uneven or blurry,
   turn on integer scaling in the emulator.

## Build the ROM

Needs Ubuntu 24.04 packages and Python 3 with numpy and Pillow:

```
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi libmgba-dev
pip install numpy pillow
make -C rom          # builds rom/build/squishy_isle.gba
make -C rom test     # runs the headless mGBA tests
```

The tests run the ROM on the real mGBA core, press buttons from a script,
take screenshots, record audio (and check the pitches) and restart with the
same save file to prove the save survives.

## Rebuild the mockups

Needs Python 3 with numpy and Pillow.

```
pip install numpy pillow
python3 tools/export_mockups.py
```

The art is procedural and hand-placed pixel data in `tools/`. Every color
is 15-bit and the renderer checks GBA limits (layers, palettes, colors per
tile and sprite), so the same data will feed the ROM.
