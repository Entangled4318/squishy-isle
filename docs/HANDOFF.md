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
| 3. Game loop in the meadow | Done. Brick feedback fixed (3.9, 3.10); Brick re-test queued; see "Step 3 progress" |
| 4. Open / reveal / squish polish, shelf with real collection | Done inside step 3 (3.3, 3.4, 3.7) |
| 5. Music and sound set | In progress; see "Step 5 progress" |
| 6. Shore, Woods, Cloud Hill, unlocks, title, parent reset | To do |
| 7. QA and final ROM with Brick instructions | To do |

## Owner notes

- Brick test queue: the owner cannot test ROMs on the Brick for now.
  Keep working in the emulator with headless tests, and add every item
  that needs a Brick check (feel, sound, music) to this list. Ask the
  owner to run the list when they can test again. Pending now: step
  5.2 (hold L + R + START at power-on: the sound test opens; A plays
  the scale test song, B stops; notes sound clean with no tick at each
  note; SELECT plays the jingle and the song goes on after it; L boings
  and the bass comes back; UP / DOWN chime and squeak over the music),
  step
  5.1 (found counter pill at the top right with the gift box icon, as
  in the mockups; the count reads well at 4x), step
  3.10 (dark shelf frame, no chime when the arrow shows, reading the
  pen sign from the top), plus the 3.9 list: walk past the pen's bottom-left corner and through
  rocks, walk close beside bushes, walk the stream shore north to
  south, arrow next to Pip after 5 s (blinks), no chime near boxes,
  counter over trees, first 3 friends keep following, START then A on
  a friend after a catch (cursor starts on a found friend; centred,
  3 squishes close it), power off and
  Continue (Pip starts where he stood).
- Next: step 5 music and sound (owner request: title music, meadow
  music, a different tune per area, a tune for opening boxes), then
  step 6 (3 more areas with 20 unique friends each, a way to get there,
  a pen like the meadow's; mailbox and picnic basket do something: ask
  the owner one question each), then step 7. Do not wait for the Brick
  re-test; queue Brick checks above instead. Music can only be judged by
  ear on real hardware, so check it headless (pitch, tempo, loop,
  volume, no clicks) and queue it.
- Budget: the owner is on limited credit. Stop after each sub-step with a
  short report (score, screenshot when useful) and ask before going on.
  Commit, push and update this file at the end of every sub-step, so a new
  chat can pick up from the exact place.
- Fixed in 6a: the "invisible blocks" were small tulip clumps placed as
  solid. They are walk-through now. Owner should confirm on the Brick.

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
   press for 4 s (`AUTO_WAIT`) starts self-presses every 70 frames. 70
   frames after the pop it goes to the reveal.
4. Reveal and squish. **Done.** `scene_reveal` is a mode of `closeup.c`
   (yellow sunburst, cushion, name pill): the friend drops in from above,
   lands with a bounce, squeak and `tune_hello`, a ring of big stars and
   confetti raining gently behind it (sprite priority 1, so the face and
   name stay clear). A or B squishes (after 20 frames). 3 squishes, or 5 s
   without one (`REVEAL_WAIT`), and it hops up and away, back to the
   meadow. The shelf close-up is unchanged. Note: confetti uses
   `game_rand()`, so box placement after a reveal depends on it; tests
   must not rely on exact later box spots.
5. Intro and title. **Done.** `rom/src/title.c`, boot goes here. Flat
   background from the title mockup (sky, island, cast shadows baked in),
   11 logo letters as sprites (`logo_table`: tile, x, y, shape, size,
   palette) that drop in one by one with a chime each (~2 s, any button
   skips). Friends hop one at a time, Pip stands in the middle. Menu:
   "Play" on a save with no friends (new game), else "Continue" (default)
   and "New game"; heart cursor plus A button by the chosen option; B
   makes the letters hop. New game with friends opens the confirm mode:
   "Start over? / Your friends will go home.", "No" default, "Yes, hold
   A" fills 6 hearts over 3 s (`HOLD_TIME`); letting go empties them; B
   or A on No backs out. Start over calls `collection_new_game()` and
   `meadow_reset()`. The title must not call `game_rand()` (it would
   shift the saved random sequence). Tests: `title_new.txt`,
   `title_reset.txt`, `check_title.py`; the older scripts now tap A twice
   at the start to pass the title.
6. Followers and the friend pen (owner request). Split in two for budget.
   6a **Done.** Pen left of the cottage in `tools/areas.py` (fence all
   round, `world.fence_side`, heart sign `world.mini_heart` in the shell
   icon's colors so it costs no palette; meadow is at 16/16 palettes, 963
   tiles). `a.pen` (feet area) and `a.sign` export as `MEADOW_PEN_*`,
   `MEADOW_SIGN_*`. In `viewer.c`: followers (save `followers[]`, newest
   first, max 3) walk Pip's recent steps (`TRAIL_GAP` 14 steps apart);
   every other found meadow friend waddles in the pen (local `pen_rand`,
   not `game_rand`). 16 px friend sprites, palettes 10..14, depth sorted.
   Box spot (64,160) moved to (60,178) below the pen. The small tulip
   clumps are no longer solid (the owner's "invisible blocks").
   Tests: `make_save.py` builds a save with chosen friends, `pen.txt`,
   `check_pen.py`.
   6b **Done.** Pip within ~20 px of the pen sign (`at_sign` in
   `viewer.c`, only when a meadow friend is found and no box is touched)
   shows the A bubble over the sign; A opens the shelf in pick mode
   (`shelf_pick` in `game.h`). Pick mode: meadow page only, real
   collection (silhouettes for friends not found, "Find me!"), header
   "Who follows Pip?" until the frame moves, a small heart on each
   follower. A on a follower removes it (`follower_remove`), A on another
   found friend adds it at the front (`follower_add`, the oldest drops
   when 3 are chosen), each change saves. B or START goes back and the
   meadow rebuilds the line. L/R, SELECT and the close-up are off in pick
   mode. Tests: `pick.txt`, `check_pick.py`. Screenshot
   `docs/step3_6b_picker.png`. Score 8.5/10: works and reads well; the
   8 px hearts are small, a bigger badge could come with step 7 art.
7. Found counter and real shelf. **Done.** Counter in `viewer.c`: heart
   plus "10/20" at the top left, text drawn by `strip_print` straight into
   OBJ tiles (`T_COUNT`, 64x32, palette 15 = `ui_text_pal`), because the
   meadow has no free BG palette. It hops twice when the count went up
   since the last meadow visit. Shelf: `collected()` is now
   `friend_found()` on every page (demo pattern and SELECT removed);
   missing friends are silhouettes named "Find me!". `step2.txt` now runs
   on a save with the Shore page found (`make_save.py`), since it opens
   Matcha Octo. Screenshot `docs/step3_7_counter.png`. Score 8.5/10.
7b. Bigger friend pen (owner request). **Done.** Map: outer fence
   56..176 x 98..182, `fence(7)`, feet area 64..168 x 118..168 (104x50,
   was 52x24), sign at (187,183) by the path; box spots (60,206),
   (210,150), (140,88); rock, mushrooms, tufts moved. Meadow: 16/16
   palettes, 950 tiles. Growing the pen left or lower needs a 17th
   palette (the big tree at the left edge). Roaming in `viewer.c`: each
   new target is the roomiest of `PEN_TRIES` (10) random spots (farthest
   from the other friends' spots and targets); a friend does not step
   closer than `PEN_ROOM` (13 px) to a neighbor, sidesteps on one axis
   first, and picks a new target after 30 blocked steps. Tests: walks
   fixed for the new sign and box spots (`pen.txt`, `pick.txt`,
   `step3_boxes.txt`, `check_step3.py`); `pen20.txt` / `check_pen20.py`
   (20 found, 17 in the pen) read the pen array from the IWRAM part of
   `dump` (symbol addresses and `sizeof(Roamer)` come from `nm -S`) and
   check: all inside, at least 8 of 17 move every 2 s, at most 1 pair
   under 10 px. Screenshot `docs/step3_7b_pen20.png`. Score 8.5/10
   (first pass 6: up to 9 piled pairs; second 7.5: jams, fixed by the
   sidestep). Known flaws: the top rail touches the cottage's tulip
   fence (looks doubled; fixed in 8 by removing the tulip fence); the found
   counter sits over the top rail when the whole pen is on screen; at
   20/20 the 16 px sprites still overlap a little at the edges.
   Each future area (Shore, Woods, Cloud Hill, 20 friends each) gets its
   own pen of this size.
8. Full loop, scoring, release. **Done.** Harness `seek N` walks Pip to
   the nearest box: it waits for the meadow scene (`current` and
   `pending` both `scene_meadow_view`, `fade_dir` 0), builds a distance
   field on `meadow_solid` (other boxes and the door blocked) and holds
   the keys that go downhill until `touch_box` >= 0. Symbols come from
   `arm-none-eabi-nm -S` on the ROM's .elf. `loop_a/b/c.txt` +
   `check_loop.py`: new game, 5 friends, reboot, 15 more (every third box
   opens itself), reboot with a full meadow; checks no repeats, saves
   survive, no boxes once full, 17 in the pen and 3 followers, 20/20.
   Scoring pass (from the loop and test screenshots): boxes 8.5, open
   8.5, reveal 8.7, arrow 8.5, counter 8.5, picker 8.5, title 8.6, pen
   8.5. Two fixes: the follower line stacked on Pip's feet after a
   reboot or a reveal (7/10); `trail_line()` now lays it out behind Pip,
   away from where Pip faces, trying the other sides when blocked (8.5).
   The cottage tulip fence doubled the pen's top rail (map 8/10); it is
   removed, the tulips stay (8.5; meadow 942 tiles, 16 palettes).
   Release ROM updated; screenshots `release/step3_screens.png`,
   `docs/step3_8_line.png`. `make -C rom test` runs 9 suites in ~10 s.
   A step 2 save on the Brick is version 1 and is replaced by a new game.

9. Brick feedback fixes (owner test of step 3). Plan, one task per turn:
   1. **Done.** Map and walking: collision cells 4 px (`areas.CELL`,
      exported as `MEADOW_CELL_SHIFT`; harness `seek` derives the cell
      size from the grid size); the bush by the pen's bottom-left post
      is gone; rocks are walk-through; bushes, mailbox, signs, basket
      block only their base (block_w 8..10, tall 6). `check_map.py`
      tests the lane left of the pen, rocks, and room beside bushes with
      the game's feet box; writes `map_collision.png`. Owner add-on:
      the stream shore is a straight line (`Stream(460)`, no wave), the
      small tree at the top right moved to x 410, bridge rails block
      only over the water, so Pip walks the whole shore north to south
      (lane x 445..454, checked). Meadow 903 tiles.
   2. **Done.** `hud_spr()` draws the counter, A bubble and arrow at
      OBJ priority 0 (over the tree-top overlay BG); box twinkles stay at
      priority 1. The near-box chime and `Box.near` are gone (the arrow
      still chimes once when it appears). Arrow: `ARROW_R` 26 px from
      Pip's middle toward the nearest box, 8 directions, blinks
      `ARROW_BLINK` 60 frames on / 60 off (starts on), shows after
      `ARROW_DELAY` 5 s without an open. `check_step3.py` checks the 5 s,
      the arrow next to Pip and the blink (two shots 1 s apart).
   3. **Done.** `collection_add` calls `follower_join` (joins the end
      only while the line has room), so the first 3 found keep
      following; the sign picker still uses `follower_add`. Save has
      `pip_x, pip_y` in 4 of the old reserved bytes (same layout, still
      version 2; 0,0 = house). `save_pos()` in `viewer.c` writes it when
      Pip has stood still `POS_SAVE_WAIT` (1 s) after moving and before
      every scene change (box, door, START, sign); `restore_pos()` puts
      Pip there on the first meadow visit after boot if walkable; new
      game clears it. Tests: `test_collection.c` (first 3 stay, free
      place fills, new game clears, layout), `loop_c/d.txt` +
      `check_loop.py` (Continue at the last box, then at the last still
      spot).
   4. **Done.** Close-up after a catch: `closeup.c` shares `leave_t`
      with the reveal, which ends at 30 (hop away, lift 210 px, the
      sprite y wrapped under the cushion); `enter()` now resets it.
      3 squishes (`SQUISHES`) go back to the shelf like B. Found while
      testing: A that opens a box also set `shelf_pick` when Pip stood by
      the sign (open_box clears `touch_box`, then the sign check ran in
      the same frame; its `scene_go` was ignored during the fade), so
      the next START opened the picker. The meadow now stops the frame
      after a box opens and ignores buttons while a fade runs
      (`scene_fading()`). Tests in `loop_a.txt` / `check_loop.py`: START
      after a catch opens the plain shelf, close-up sprite y 28 (was 72),
      3 squishes return to the shelf.
   5. **Done.** 10 suites pass (`check_map.py` is new). Scores: map and
      walking 8.5, shore 8.5, overlays and arrow 8.5, followers and
      Continue spot 8.5, close-up 8.5. Release ROM updated;
      `release/step3_9_screens.png`, `docs/step3_9_collision.png`.
      Owner follow-up: the shelf (not pick mode) keeps its cursor
      between visits, but when it sits on a friend not found yet it moves
      to the first found friend of the page, so START then A opens a
      close-up (`check_loop.py` checks it).
   Owner feature requests for later: title, meadow, per-area and open
   music (step 5); mailbox and picnic basket do something (step 6, ask
   the owner one question each); 3 more maps with their own friends
   (step 6).

10. Second Brick feedback. **Done.** Shelf selection frame is a dark
   2 px outline (`#2e1a2a`) with a light inner edge (`selection_frame`
   in `mockups.py`), was gold. The guide arrow appears without a chime
   (`check_step3.py` records the wait: peak 0, was 6701). The pen sign
   zone reaches 18 px above its base, so Pip can read it from the top;
   the A bubble rises over Pip's head there (`sign_top.txt`,
   `check_pick.py`). 10 suites pass. Release ROM updated.
   (Step 5.2 added `check_music.py`: 11 suites now.)

The old notes below mention hearts and "rolls lean toward new"; the owner
replaced those with the rules above.

## Step 5 progress

Plan agreed with the owner, one sub-step per turn, stop after each task:

1. Found counter pill. **Done.** Owner request: match the mockups. A
   rounded pill at the top right (`COUNT_PILL_W` 52 px, 4 px from the
   edge) with the area's container icon inside its left end. Owner
   follow-up: the icon is smaller, 12 px hand-drawn `props.icon12()`
   (box, shell, acorn, capsule), fully inside the pill; exported in
   `count_pill_tiles` / `count_pill_pal`, 16 tiles and one palette per
   area, ink is color 1). The count is printed at run time into a 32x16
   OBJ strip, centred after the icon (`COUNT_TEXT_X` 18). Two 32x16
   sprites draw the pill (64x16 is not a GBA sprite size). Still hidden
   while the area has 0 friends, still hops twice on a new friend.
   `check_loop.py` and `check_pick.py` look for the pill fill and ink at
   the top right. Screenshot `docs/step5_1_counter.png`. Score 8.5 (first
   pass 7.5: "20/20" was left-aligned and touched the right end; the
   12 px shell took 3 drafts, the first read as a cupcake).
2. Music engine. Task A **done**: `tools/music.py` holds the songs as
   note-name text ("C5:4", "r:2", "|" bar lines), checks every note is
   within 12 cents on the hardware rates and both voices have the same
   length, and writes `build/music_data.c/.h` (rate tables, 3 lead waves
   `sine`/`bell`/`hollow`, `songs[]`) plus `music_songs.json` for tests.
   `sound.c`: `music_play(id)` (no restart if already playing),
   `music_stop()`, `music_current()`, `music_mute`. Lead on channel 3
   (volume 100/75/50/25 %, optional decay, gap frames), bass on channel 2
   (square envelope). A jingle (`song_play`) pauses the song, which goes
   on at the next note after it; the boing takes channel 2 and the bass
   comes back at its next note. Sound test scene `jukebox.c`: hold
   L + R + START at power-on; LEFT/RIGHT choose, A play, B stop, SELECT
   jingle, L boing. One test song ("Scale test"). Checked by hand in the
   emulator: pitches right (C5 524 Hz, C6 1041 Hz), loops on time, the
   song resumes after a jingle. Task B **done**: harness `solo N` (hear
   only PSG channel N; `core->enableAudioChannel` crashes in this libmgba
   build, so it sets `gba->audio.psg.forceDisableCh` directly).
   `check_music.py` drives the sound test for every song in
   `music_songs.json`, records lead and bass alone (2 loops) and the mix,
   and checks pitch (YIN, 25 cents), exact tempo from the `music loop`
   log lines, loop 2 = loop 1, no clipping, bass under the lead, clicks,
   and that the song goes on after a jingle and the boing. Found and
   fixed: every lead note switched hard on and off, a DC jump of 0.8 of
   the note's loudness (a click per note start and end; PSG channels
   only output positive values). The lead now fades in and out one
   volume level per frame and changes pitch without a restart while it
   sounds (largest step 0.41, the floor with 4 volume levels). The bass
   should use a fading envelope (`bass_step`), not a hard cut
   (`bass_gap` 0); a plucked start steps sqrt(d/(1-d)) for duty d.
   Score 8.5 (first pass 6.5: clicks). Limits: tempo is whole frames per
   tick (8 = 112 bpm, 10 = 90, 12 = 75 in quarter notes); the lead has
   4 volume levels. Screenshot `docs/step5_2_sound_test.png`.
3. Title and meadow music.
4. Box-opening tune (open screen), reveal jingle; shelf music box theme
   (DESIGN.md) unless the owner says no.
5. Shore, Woods and Cloud Hill tunes (played once step 6 builds them).
6. Mix pass, all suites, release ROM, Brick queue, merge.

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

    sudo apt-get update      # needed: a stale index gives 404s
    sudo apt-get install -y --no-install-recommends gcc-arm-none-eabi libnewlib-arm-none-eabi libmgba-dev
    pip install numpy pillow
    make -C rom            # builds rom/build/squishy_isle.gba
    make -C rom test       # all headless checks on the real mGBA core

`test/harness` runs a ROM with a script (`wait N`, `hold KEYS N`,
`tap KEYS`, `shot NAME`, `audio NAME` / `audio end`, `dump NAME`,
`seek N`), prints
the game's `dbg()` log lines, writes PPM screenshots, WAV audio and a
memory dump. Checks read the log, pixels and audio pitch.

## Traps already found (do not repeat)

- The network policy blocks devkitPro and GitHub release downloads. The
  build uses Ubuntu's `gcc-arm-none-eabi` with our own crt0, linker script
  and `gbafix.py`. The header logo bytes are verified by their CRC-16
  (0xCF56), so the ROM boots even with a real BIOS file.
- Game code is `SQIS` (not in mGBA's override table). `SRAM_V113` is kept
  in ROM for save detection.
- BG palette budget: the meadow uses 16 of 16 palettes and 950 of 1024
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
- Moving a box spot, the sign or the pen breaks the scripted walks in
  `step3_boxes.txt`, `pen.txt`, `pick.txt`, `pen20.txt` and the spot in
  `check_step3.py`. Probe Pip's position with a `dump` (IWRAM, `pip_x` /
  `pip_y` from `nm`) instead of guessing frame counts.
- The scene manager leaves `pending` equal to `current` after a switch
  (it is not cleared to NULL); test drivers must compare, not test 0.
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
