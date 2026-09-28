# Squishy Isle: handoff for the next session

Read this first. It says what exists, how to build and test it, the traps
already found, and what comes next.

## Goal

A calm collecting game for a toddler (2 to 4 years), made as a real Game
Boy Advance ROM. The owner plays it on a Trimui Brick (GBA emulator) and on
desktop mGBA, and copies ROMs to the Brick by FTP. Walk around a pastel
island, open surprise containers, squish the mochi friends inside, fill the
shelf. No battles, no fail states, no reading needed. Full design:
[DESIGN.md](DESIGN.md).

## Working rules the owner set

- Score each part X/10 and iterate until it is 8.5 or higher. Report
  scores honestly, including the first pass and what was fixed.
- Send screenshots in chat (from the real emulator, not mockups).
- Work step by step. Give an overview, then ask before starting each step.
- Build to a high standard.

## Status

| Step | State |
| --- | --- |
| Mockups (8 screens, 80 squishies) | Done. `mockups/`, scorecard in DESIGN.md |
| 1. Toolchain, header, test harness, hardware check ROM | Done. Tested on the Brick: boots, all buttons, sound, save all work |
| 2. Art in the ROM: walkable meadow, Squishy Shelf, close-up | Done. `release/squishy-isle.gba`. Tested on desktop mGBA and the Brick: works |
| 3. Game loop in the meadow | In progress. Sub-step 1 of 7 done (save v2 and rolls); see "Step 3 progress" |
| 4. Open / reveal / squish polish, shelf with real collection | To do |
| 5. Music and sound set | To do |
| 6. Shore, Woods, Cloud Hill, unlocks, title, parent reset | To do |
| 7. QA and final ROM with Brick instructions | To do |

## Owner notes

- Budget: the owner is on limited credit. Stop after each sub-step with a
  short report (score, screenshot when useful) and ask before going on.
  Commit, push and update this file at the end of every sub-step, so a new
  chat can pick up from the exact place.
- Known issue (fix later): invisible blocks in the meadow make Pip walk
  around empty-looking ground. Likely the 26 px "tall object" collision
  above bases, or collision cells that reach past the art. Check the
  collision grid against the art in `tools/areas.py`. The collision
  preview shows solid cells under the small flower tufts beside the path
  and by the cottage; those are the likely cause.

## Step 3 progress

Owner decisions: no duplicate friends and no hearts; each new game
shuffles the order; on-screen text is fine for parents and early readers;
a title with Continue / New game (guarded) is wanted. Plan, one sub-step
per turn:

1. Save v2 and rolls. **Done.** `rom/src/collection.c/.h`: found array
   (0/1), opens, roll state, follower line of 3. A roll picks only friends
   not found yet (Sparkle weight 1 vs 4, so they come late) and returns -1
   when the area is full. `collection_new_game(seed)` clears friends and
   reshuffles. Saves after each change; boot loads and counts boots. A
   version 1 save is replaced by a fresh one. Tests: `test/test_collection.c`
   (host) and `check_save.py` (reboots on the mGBA core).
2. Gift boxes on the map. **Done.** In `viewer.c`: up to 3 boxes (fewer
   when fewer friends are left) on free `meadow_spots`, 5 colors, never two
   of one color; the first box of a game sits at `MEADOW_FIRST_SPOT` below
   the cottage, later ones prefer spots off screen and 48 px from Pip. A
   box hops every few seconds, a twinkle circles it, a chime plays as Pip
   comes near. Boxes are solid; touching one shows a bouncing A button and
   A or B opens it. Guide arrow after 15 s without an open (`ARROW_DELAY`),
   always to the nearest box: at the screen edge in 8 directions, or above
   the box when it is on screen. World sprites are depth sorted by y.
   `open_box()` rolls the friend and goes to the open screen. Touch zone
   is 6 px around the box footprint (toddler friendly). Tests:
   `step3_boxes.txt`, `check_step3.py`.
3. Open screen. **Done.** `rom/src/open.c`: lavender sunburst and cushion
   (BG1), 64 px box in the meadow box's color (body and lid sprites share
   one affine matrix, so they wobble together; `affine_rot_scale` takes 256
   steps per turn). Big A button and 3 hearts (14 px) show progress. Press
   1 and 2 (A or B, 14 frame cooldown) jump higher with rising chimes and
   a boing; press 3 pops: lid flies off spinning, sparkle ring, puffs,
   `sfx_pop` and `tune_pop`. The friend is added and saved at the pop. No
   press for 4 s (`AUTO_WAIT`) starts self-presses every 70 frames. For now
   it returns to the meadow 70 frames after the pop; sub-step 4 puts the
   reveal there.
4. Reveal and squish: confetti, star burst, name pill; 3 squishes or a
   wait returns to the meadow and saves.
5. Intro and title: logo intro (skippable), Continue / New game, guarded
   start-over screen (No default, hold A 3 s for Yes).
6. Followers and the friend pen (owner request): found friends live in a
   fenced pen left of the cottage and roam inside it. Talking to something
   at the pen (sign or NPC) opens a picker to choose up to 3 followers.
   The newest friend follows Pip by default. This replaces the earlier
   "wander the meadow" and "pick in the close-up" ideas.
7. Found counter (sprites) and the real shelf (silhouettes).
8. Full-loop test with reboot, scoring pass, screenshots, release ROM.

The old notes below mention hearts and "rolls lean toward new"; the owner
replaced those with the rules above.

## What step 3 should do (as promised to the owner)

- Gift boxes appear at the map's container spots (`meadow_spots`).
- Walking into one fades to the open screen: the box wobbles, each A (or B)
  press makes it jump higher with a rising note, the third press pops it.
  If the child does not press, it opens by itself after a short wait.
- Reveal: the friend lands on the cushion, NEW badge for new friends, name
  pill. Each press squishes; after 3 squishes or a short wait it goes back
  to the meadow.
- The newest friend follows Pip; found friends wander the meadow.
- Collection state saved automatically (two-slot SRAM save, bump
  `SAVE_VERSION`). Rolls lean toward friends not found yet; the first 5
  opens are always new. Duplicates add hearts (3 hearts = crown later).
- Found counter on screen as sprites (the meadow uses all 16 BG palettes).
- The shelf shows the real collection (silhouettes for missing friends).

## Repository layout

- `docs/` design doc and this handoff.
- `mockups/` 4x mockup screenshots and the full squishy sheet.
- `release/` the current ROM and step screenshots.
- `tools/` Python art pipeline (numpy, Pillow). All art is procedural or
  hand-placed pixel data; every color is 15-bit.
  - `squishies.py` 16 species as soft shapes, rendered natively at 16/32/64.
  - `world.py`, `props.py`, `pip.py`, `logo.py`, `font.py` world art,
    containers and UI, player sprite, bubble logo, 5x7 font.
  - `mockups.py` mockup screens; also holds shared builders
    (`shelf_background`, `sunburst`, `stage`, `selection_frame`).
  - `areas.py` full-size maps: ground and overlay layers, 8x8 collision
    grid, spawn, container spots, doors.
  - `gbaconv.py` image to GBA tiles, maps, palettes (with a strong palette
    packer); `squishy_export.py` squishy tiles with shared area palettes;
    `export_game.py`, `export_hwcheck.py`, `export_font.py` write the C
    data; `gbafix.py` writes the cartridge header.
- `rom/` C source, `Makefile`, `gba.ld`.
  - `crt0.s` startup and IRQ handler; `system.c` input, OAM, debug log;
    `scene.c` scene manager with white fades; `text.c` variable-width text
    strips; `sound.c` PSG effects and a small melody sequencer; `save.c`
    two-slot SRAM save; `squishy.c` sprite helpers.
  - Scenes: `viewer.c` (meadow walking), `shelf.c`, `closeup.c`,
    `hwcheck.c` (hold L + R + SELECT at boot).
- `test/` headless mGBA harness (`harness.c`), scripts and checks.

## Build and test

The container needs these each new session:

    sudo apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi libmgba-dev
    pip install numpy pillow
    make -C rom            # builds rom/build/squishy_isle.gba
    make -C rom test       # all headless checks on the real mGBA core

`test/harness` runs a ROM with a script (`wait N`, `hold KEYS N`,
`tap KEYS`, `shot NAME`, `audio NAME` / `audio end`, `dump NAME`), prints
the game's `dbg()` log lines, writes PPM screenshots, WAV audio and a
memory dump. Checks read the log, pixels and audio pitch.

## Traps already found (do not repeat)

- The network policy blocks devkitPro and GitHub release downloads. The
  build uses Ubuntu's `gcc-arm-none-eabi` with our own crt0, linker script
  and `gbafix.py`. The header logo bytes are verified by their CRC-16
  (0xCF56), so the ROM boots even with a real BIOS file.
- Game code is `SQIS` (not in mGBA's override table). `SRAM_V113` is kept
  in ROM for save detection.
- BG palette budget: the meadow uses 16 of 16 palettes and 909 of 1024
  tiles. Tiles that mix color families (green tree + pink tree, trees or
  lily pads + water) cost extra palettes. Lily pads and reeds use grass
  greens for that reason. Check every new map with `bg(..., max_pals=16)`.
- Text strips hold at most 64 tiles and must be declared `EWRAM_BSS`. A
  strip that was too big once overran memory and corrupted VRAM; a guard
  and regression checks exist now.
- The hardware pitch sweep overflowed and silenced the squeak; the squeak
  uses a per-frame pitch curve instead.
- OAM powers up as 128 visible sprites at (0,0); `scene_run` hides them.
- Scene fades ignore input for about 10 frames; test scripts wait 20
  frames before the first press.
- Map objects split at Pip's height (26 px): upper part on the overlay BG
  (over Pip), lower part on the ground BG, and tall objects block 26 px
  above their base. This keeps Pip correctly in front of or behind trees.
- Squishies share one palette slot layout per area (`ROM_MODE` render:
  two-shade accents, tongue uses the blush color), so 5 palettes cover a
  whole shelf page. The Sparkle flavor's glints are separate twinkle
  sprites.

## VRAM use per scene

- Meadow: tiles in charblocks 0-1, ground map SB 24-27 (BG2, 64x64),
  overlay map SB 28-31 (BG1, 64x64, priority 0), sprites priority 1.
- Shelf and close-up: art on BG1 (CB 0, SB 30), text on BG0 (CB 2,
  SB 31, palette bank 15).
- Sprites: 1D mapping; each scene loads its own OBJ tiles and palettes.
