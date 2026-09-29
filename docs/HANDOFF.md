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
- Work step by step. Give an overview, then go ahead with the coding.
- For music, send the piano roll `check_music.py` writes (`roll_KEY.png`).
- Commit and push after each task and update this file. Merge to main
  when a sub-step is done, so the owner can download
  `release/squishy-isle.gba`.
- Ask the owner one question at a time.
- Build to a high standard.

## Status

| Step | State |
| --- | --- |
| Mockups (8 screens, 80 squishies) | Done. `mockups/`, scorecard in DESIGN.md |
| 1. Toolchain, header, test harness, hardware check ROM | Done. Tested on the Brick: boots, all buttons, sound, save all work |
| 2. Art in the ROM: walkable meadow, Squishy Shelf, close-up | Done. `release/squishy-isle.gba`. Tested on desktop mGBA and the Brick: works |
| 3. Game loop in the meadow | Done. Brick feedback fixed (3.9, 3.10); Brick re-test queued; see "Step 3 progress" |
| 4. Open / reveal / squish polish, shelf with real collection | Done inside step 3 (3.3, 3.4, 3.7) |
| 5. Music and sound set | Done; see "Step 5 progress". Brick checks queued |
| 6. Woods, Shore, Cloud Hill, gates, mailbox, basket | Done (6.1 to 6.9); see "Step 6 progress". Brick checks queued |
| 7. QA and final ROM with Brick instructions | To do |

## Owner notes

- Brick test queue: the owner cannot test ROMs on the Brick for now.
  Keep working in the emulator with headless tests, and add every item
  that needs a Brick check (feel, sound, music) to this list. Ask the
  owner to run the list when they can test again. Pending now: step
  6.9 (walk up into each house door: the room opens; the gifts read at
  4x in every flavor, the silhouettes read as "not yet"; the arrow and
  hop feel good; the letter's gift matches the friend), step
  6.8 (the woods trail, shore walkway and cloud bridges read as straight
  paths; the shore fences make the walkway the only way east, so it is
  clear Momo blocks it; a long play session feels good), step
  6.7 (after a new friend the mailbox flag waves and an envelope bobs;
  A by the mailbox: the envelope drops, the letter slides up, the
  friend squeaks; the flag goes down after; snack time at the basket:
  a treat pops out, the followers hop over and squeak, the pace feels
  cosy, not slow; with no followers Pip hops), step
  6.6 (open an acorn, a seashell and a capsule: each reads at 4x, the
  cap, top shell and dome fly off, the pearl and the hollow nut read),
  step
  6.5 (with 10 shore friends Momo wakes on the shore and waddles up the
  cloud steps; the east edge leads up to Cloud Hill and back down; the
  sky, rainbow and candy trees read well at 4x; the stars and moon
  twinkle softly, not flashy; Momo sleeps tucked in on its cloud bed and
  hops with hearts when Pip comes close; the Cloud Hill song plays; the
  capsules read on the white cloud), step
  6.4 (with 10 woods friends Momo wakes in the woods and the east trail
  leads onto the shore boardwalk; the sea glints; shells read as
  shells; the shore song plays), step
  6.3 (Momo the panda sleeps on the meadow bridge; walking up shows
  the "n/10" bubble; on the 10th meadow friend the friends tickle Momo
  awake and it waddles off east: pace feels right, not too long; Momo
  reads as a friendly panda at 4x), step
  6.2 (with 10 meadow friends walk east over the bridge into the Berry
  Woods and back; with fewer the log blocks the bridge; the woods feel
  different from the meadow; acorns read as acorns; woods song plays;
  Continue after power off starts in the woods), step
  5.6 (chimes, squeaks and boings are clear over every song but not
  harsh; the title letter chimes are not too loud), step
  5.5 (sound test, hold L + R + START at power-on: Shore, Woods and
  Cloud Hill tunes each sound calm and different from the meadow; the
  woods reed and the cloud bell are pleasant on the speaker; loops are
  seamless), step
  5.4 B (shelf music box: tinkly, not too high or sharp on the speaker,
  cursor sounds still clear over it; back on the map the meadow song
  goes on where it was, no pop or click at that moment), step
  5.3 B (meadow lullaby: starts when the meadow shows, slow and sleepy
  3/4 rocking, softer than the title song, melody clear over the bass,
  bass audible on the Brick speaker (lowest note F2, 87 Hz), loop at
  58 s is seamless; also in the sound test),
  step 5.4 A (open screen: tip-toe tune under the press chimes, the
  chimes sound in tune with it, it stops at the pop; the new friend
  jingle when the friend lands, not louder than the pop), step 5.3
  (title music: starts after the hello jingle, bright and bouncy,
  melody clearly over the bass, loop at 34 s is seamless, stops when
  the game starts; also in the sound test), step
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
- Next: step 7 (QA and the final ROM). Step 6 is done; ask the owner
  before starting step 7.
  Owner (step 6 session 2): do all of step 6 without stopping (merge
  after each sub-step), then ask before step 7. Owner
  answers for 6.7: mailbox = a letter with a happy message from the
  newest friend (flag up after each new friend; A reads it; no new mail
  re-reads the last one); picnic basket = snack time (A opens the lid, a
  treat pops out, a different one each time; every follower hops over
  and shares it with a squeak; with no followers Pip eats it with a
  hop; any number of times, short cooldown). Step 6 scope: Shore, Woods, Cloud Hill, each
  with 20 unique friends, a way to get there, a pen the size of the
  meadow's; play each area's song from step 5.5. Then step 7 (QA and
  the final ROM). Do not wait for the Brick
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
3. Title and meadow music. Task A **done**: "Title: Hello island"
   (`song('title', ...)` in `music.py`): C major, 112 bpm, 16 bars
   (A climbs to a high C with a dotted skip, B is stepwise and leads
   back; bar 16 ends on G for a clean loop), bell lead with a slow fade
   (`lead_decay` 10), plucked oom-pah bass, 34 s a loop. Starts after
   the hello jingle (`finish_intro`), stops when the game starts
   (`start_game`). Harmony review: lead notes on beats vs the bass; the
   3 left are a 4-3 suspension and chord roots over a fifth in the bass.
   `music.py` now checks every bar is 16 ticks (`bar=`). Found and fixed:
   the music slowed by a frame whenever the game missed a VBlank; the
   IRQ handler now counts VBlanks (`vbl_count`) and `scene_run` runs one
   `sound_tick` per VBlank that passed (at most 8). Click limit per wave
   (the bell is spikier than the sine, so its smallest step is 13%
   bigger). `check_music.py` writes `roll_KEY.png` (notes as written,
   pitch heard in the emulator). Mix: bass/lead loudness 0.47 (check
   0.25..0.65). Score 8.5 (first pass 7: 4 clashing beat notes, slow
   first loop, bass too loud at 0.70). Screenshot
   `docs/step5_3_title_roll.png`. Task B **done**: "Meadow: Sleepy
   clover" (`song('meadow', ...)`): F major waltz, 3/4 (`bar=12`, 12
   ticks a bar), 75 bpm (tick 12), 24 bars A B A' (A rises by broken
   chords, B steps down in pairs and rests on F, A' climbs to a high D
   and settles on A), 57.6 s a loop. Hollow (flute) lead at 50% with a
   slow fade on long notes (`lead_decay` 40); bass rocks root, fifth,
   third on a 50% square, soft pluck that rings about half a beat
   (`bass_vol` 4, `bass_step` 7). `music_play(SONG_MEADOW)` in the
   meadow's `enter()`; it plays on through the shelf and close-up (no
   restart); `open.c` stops it (5.4 gives the box its own tune). The
   sound engine logs `song start N` / `song stop N`; `check_loop.py`
   checks the song per scene. `check_music.py` draws bar lines from the
   song's `bar`. `step3_boxes.txt` records the arrow wait on channel 1
   only (`solo 1`, then 4 frames for the filter to settle), since the
   music now plays there. Mix: peak 5889 (title 8808), bass/lead 0.42.
   Score 8.5 (first pass 7.5: lead louder than the title song, lead click
   step on the limit, bass plucks died by mid-beat; fixed by lead 75% to
   50%, longer bass ring, a 4-3 suspension in bar 18). Piano roll
   `docs/step5_3_meadow_roll.png`.
4. Box-opening tune (open screen), reveal jingle; shelf music box theme
   (DESIGN.md) unless the owner says no. Task A **done**: "Open: What's
   inside?" (`song('open', ...)`), a tip-toe loop in C major pentatonic
   (the press chimes C6/E6 and the pop chime A6 always fit), 112 bpm,
   4 bars (8.5 s), plucked bell notes with rests (`lead_decay` 4), short
   oom-pah bass. `open.c` plays it on enter and stops it right after the
   pop's log line; the G major ta-da (`tune_pop`) follows. "Reveal: New
   friend!" (`song('reveal', ...)`, `loop=False`), 150 bpm, 2 bars
   (3.2 s), climbing C arpeggio home to a held high C, played when the
   friend lands (`closeup.c`, replaces `tune_hello` there; the title
   keeps `tune_hello`). Engine fix: a lead attack from silence starts at
   25% at most (at `lead_level` 0 it jumped to 50%, a click).
   `check_loop.py` checks the open tune per scene, its stop at the pop,
   the jingle at each landing, and the whole box 1 audio (`loop_open`,
   peak 18589 at the pop, from the old pop sounds). Scores: open tune
   8.5, reveal jingle 8.5 (first pass 7: click on the first note, and at
   100% it peaked at 10615 against the meadow's 5889; now 75%, 8793).
   Piano rolls `docs/step5_4_open_reveal_roll.png`. Task B **done**:
   "Shelf: Music box" (`song('shelf', ...)`), G major, 90 bpm (tick 10),
   16 bars A B (43 s), bell lead that dies fast (`lead_decay` 8) like a
   music box comb, bass voice plays a soft Alberti pattern in eighths
   (`alberti()` helper in `music.py` builds it from chords). `shelf.c`
   plays it on enter (plain shelf and sign picker); the close-up keeps
   it. Resume: `music_play()` keeps where a looping song was when another
   song takes over (`kept[]` in `sound.c`) and goes on from there, the
   current note struck again for its frames left; `music_stop()` saves
   nothing, so the open tune and the title start over.
   `music_play_from_start()` is for the sound test. Log: `song resume N`.
   `check_loop.py`: songs per scene, the meadow song starts once and
   resumes after every box and shelf trip. Mix: peak 7198, bass/lead
   0.34. Score 8.5 first pass (checks clean, harmony reviewed per beat);
   resume 8.5. Piano roll `docs/step5_4_shelf_roll.png`.
5. Shore, Woods and Cloud Hill tunes (played once step 6 builds them).
   **Done.** Each area has its own key, meter and lead sound (meadow is F
   3/4 hollow): "Shore: Sea breeze" (`song('shore', ...)`) D major 6/8
   (`bar=12`, two dotted beats), tick 10, 24 bars A B A' (48 s), sine
   lead, bass rocks root - fifth per dotted beat. "Woods: Acorn trail"
   (`song('woods', ...)`) A minor turning to C major, 4/4, tick 11
   (82 bpm), 16 bars (47 s), new `reed` wave (odd harmonics, clarinet
   like), plucked walking bass in quarters, ends on E7 to loop. "Cloud
   Hill: Floating up" (`song('cloud', ...)`) Eb major, 4/4, tick 12
   (75 bpm), 16 bars (51 s), bell with a slow fade (`lead_decay` 24),
   wide leaps, bass rolls root - fifth - octave - fifth. All leads at
   50% or decaying from 75%, like the meadow. Mix (lead loudness,
   bass/lead): shore 1786 / 0.38, woods 1876 / 0.33, cloud 1609 / 0.33.
   Scores: shore 8.5 (first 7.5: lead 2655, louder than the meadow; bass
   0.51), woods 8.5 (first 7: bass 0.24 under a loud reed), cloud 8.5
   (first 7.5: Ab6 is 20 cents off on the wave channel and `music.py`
   refused it; bar 9 rewritten). Step 6: call `music_play(SONG_SHORE)`
   etc. in each area's `enter()`; each area song resumes like the
   meadow's. Until then they are in the sound test. Piano rolls
   `docs/step5_5_area_rolls.png`. Note: high Ab6 (and other notes above
   ~1500 Hz) can miss the 12-cent limit on the wave channel; `music.py`
   says so at build time.
6. Mix pass, all suites, release ROM, Brick queue, merge. **Done.**
   Song loudness (90th percentile of 100 ms loudness of the mix): title
   2627, reveal 2583, cloud 2471, shelf 2190, meadow 2161, open 2101,
   shore 2051, woods 2020, so the songs sit within about 2 dB; title and
   jingle a little brighter on purpose. Effects against them: the chime
   was 1621, under the music (first pass 7/10); `sfx_chime` envelope
   10 to 14 (2377) and `sfx_squeak` 11 to 13 (3143); boing 3207. Same
   tone, only louder. `check_music.py` now checks each effect is at least
   the songs' typical loudness. Box 1 audio peak 19868 (no clipping).
   Sound test screen with 8 songs `docs/step5_6_sound_test.png` (the
   status line is for the selected song). Score 8.5. 11 suites pass.

## Step 6 progress

Owner decisions: the areas open in a line, Meadow, Woods, Shore, Cloud
Hill (Woods first, the owner's pick); each opens when the area before it
has 10 friends (`GATE_NEED`); until then the way is visibly blocked.
Plan, one sub-step at a time, merge after each:

1. Area framework. **Done.** `AREAS` in `squishies.py` is in play order
   (friend ids: meadow 0-19, woods 20-39, shore 40-59, cloud 60-79;
   shelf tabs and counter icons follow it). `export_game.py` writes an
   `AreaMap` per built area (`area_maps[]`, `AREA_MAPS`; must follow the
   play order); the old `meadow_*` arrays and `MEADOW_*` defines stay
   because the harness `seek` reads them. `viewer.c` reads everything
   from `A = &area_maps[game_save.area]`: maps, collision, spots, pen,
   sign, doors, shimmer, the area's 4 species and 5 palettes, counter
   icon, and its song (`area_song[]`). Boxes reset when the area
   changes; the first box of each area sits at its `first_spot`. Save
   (still version 2, same 144 bytes, `_Static_assert`): `area`, `gates`
   (bit n: the way into area n + 1 is built) and `lines[3][3]`, the
   follower lines of areas 1..3 (the meadow's stays `followers`, so old
   saves keep their line). `follower_get(area, i)`; add/join/remove find
   the line from the friend id (`friend_area`). `area_open(a)`. The shelf
   opens on the current area's page; the picker uses that area's line.
   Tests: `test_collection.c` (separate lines, the woods open at 10, new
   game goes home), `check_step2.py` (page 1 is the Woods, Matcha Fox).
   Screenshot `docs/step6_1_shelf_order.png`. Score 8.5 (refactor, no
   visible change besides the tab order; first pass had a wrong line
   index, `& 3 % 3`, caught in review before the build).
2. Berry Woods. **Done.** `areas.woods()` (480x320): mossy floor
   (`WOODS_KINDS`, `mg_*`), earth trail (`dt_*`) from the west edge
   through a small clearing to the east edge, a path north; a ring of
   green and autumn trees (`tree('autumn')`, `au_*`) snapped to the 8 px
   grid so copies share tiles; berry bushes, ferns, fallen leaves,
   stumps, logs, mushrooms (`world.berry_bush/fern/leaves/log`). Pen the
   meadow's size at 300..420 x 200..284, sign left of it. 12 acorn spots
   (none under a canopy or on solid ground: checked). 698 tiles, 9
   palettes. Map data: `Area.exits` (x, y, w, h, to area, arrive x, y)
   and `Area.gates` (x, y, w, h, to area), exported into `AreaMap`.
   Meadow: exit at the bridge's east end, gate log across it; its sign
   by the bridge shows an acorn (`mini_acorn`, the 3 sign colors, else
   the meadow needs 17 palettes). Woods: west exit back, east exit and
   gate toward the shore. `viewer.c`: a gate is shut while
   `!area_open(to)` or the map is not built (`to >= AREA_MAPS`); shut
   gates are solid and draw a 16x32 log sprite (`gate_tiles`, colors in
   the shadow's OBJ palette after its 1 color: the map uses all 16);
   walking into an open exit sets `travel_to` and calls the new
   `scene_reload()` (fade out, enter the same scene); `enter()` puts Pip
   at the arrival, stores `game_save.area` and the spot. Containers per
   area: `cont16_tiles` / `cont16_pal` (4 tiles and 5 palettes an area;
   `shared_index()` builds one tile set from color variants). Acorns keep
   a brown cap and vary the nut (cream, pink, mint, lav, gold). Harness:
   `seek` works in any area (reads `game_save.area` at offset 112, a
   `_Static_assert` guards it, and the area's `<name>_solid/spots/doors/
   gates`; shut gates count as solid); new `walkto X Y N`. Found and
   fixed: followers outside the meadow drew garbage (species not offset
   by the area); `wait_meadow` ran before the symbols were loaded, so
   the first `walkto`/`seek` of a run waited its whole budget.
   Tests: `woods_shut.txt`, `woods.txt`, `woods_continue.txt`,
   `check_woods.py` (log stops Pip at 9 friends, crossing at 10, acorn
   gives a woods friend, woods shelf page, back to the meadow with its
   own followers, Continue in the woods, songs per area, log sprite shown
   and gone, acorn counter). 12 suites. Screenshots
   `docs/step6_2_woods.png`, `docs/step6_2_woods_pen.png`. Scores: map
   8.5 (first 7: open like the meadow, clearing a dirt blob, a log in the
   pen, trees over the pen rail), travel and gates 8.5, acorns 8.5
   (first 7: pastel caps looked like cupcakes). Known: the log gate on
   the bridge stands like a post (it lies across the way; 6.3 rolls it
   away); the open screen still shows a gift box in the woods (6.6).
3. Gate scene. **Done.** In `viewer.c` (`gs`, `GS_*` frame marks):
   when a gate's area is open but its `game_save.gates` bit is not set,
   the map starts the scene on its first frame (after the 10th friend's
   reveal, or on Continue). The camera glides to the log (50 frames),
   3 helpers (pen friends first, then followers) hop in from the west,
   push 3 times (squished frame, squeaks, the log nudges), the log
   tumbles south with a boing (flips every 6 frames), sparkles and the
   pop ta-da, happy hops, the camera glides back to Pip; 304 frames in
   all, input ignored. Then the bit is set and saved. A gate blocks
   while it is shut or its scene has not played (`gate_there`); the
   meadow counts as built (`gate_built(0)`, found when the way back from
   the woods stayed shut). `make_save.py` takes GATES (default: every
   open way built, so older tests skip the scene). Tests: `gate.txt`,
   `gate_again.txt` in `check_woods.py` (plays once with 3 helpers in
   ~5 s, starts at once, the way opens after, log on the bridge, then
   below it, then gone; no replay after a reboot). Screenshots
   `docs/step6_3_gate_scene.png`. Score 8.5 (the log drops into the
   stream with no splash; a splash could come with step 7 polish).
   3b. Momo instead of logs (owner: "an NPC blocking the path, for all
   maps"). **Done.** `tools/npc.py`: Momo, a sleepy pastel panda in a
   sky-blue nightcap, drawn with the squishy renderer at 32 px (asleep
   with the new `expr='sleep'` eyes, awake), plus two z tiles; 13 colors
   in the shadow's OBJ palette (`npc_tiles`, `NPC_AWAKE`, `NPC_ZZ`;
   `T_NPC` 160). One character for every gate: Momo lies across the
   first way not yet opened, z's drifting up, breathing. Pip within
   34 px: Momo opens its eyes (`sfx_blip`, log `npc near`) and a bubble
   (the counter pill: area icon plus "7/10", `st_npc` at `T_NPC_TXT`)
   floats over it. The gate scene: friends hop in and tickle Momo
   (squeaks, wriggle) until it wakes (chime, boing), hops for joy three
   times, then waddles off toward the area's exit for that gate while
   sparkles and the ta-da play. Gate rects widened to Momo's 28 px. The
   story reads: Momo moves on and sleeps across the next way. Checks in
   `check_woods.py` use Momo's patch color `#8f84aa` (the helpers share
   the vanilla outline). Screenshots `docs/step6_3b_momo.png`. Score 8.5
   (first pass 7: 15 colors, pale patches read as a cat, nose and mouth
   stacked into "=").
4. Seashell Shore. **Done.** `areas.shore()` (480x320) from the 07
   mockup: wavy sea along the north (`Sea` SDF, solid), sand, a
   boardwalk from the west edge (arrival from the woods at 20,200), tide
   pool with rocks, palms, umbrella and towel, sandcastle, bucket, beach
   ball, starfish, tiny shells, footprints, two sailboats. Pen at
   290..410 x 184..268 with its sign. East exit to Cloud Hill with Momo
   (to area 3, not built yet, so Momo sleeps there and shows n/10).
   498 tiles, 8 palettes. Seashells in pink, peach, mint, lav, yellow
   (`props.shell` got mint and yellow). Per-area glint: `Area.shimmer`
   names the color (`sea_lt` on the shore) and each area exports its
   own `<name>_shimmer_cycle` (in `AreaMap`). Woods and shore east exits
   widened to 12 px (a 6 px exit at the map edge left one reachable
   column); Momo's bubble is clamped to the screen near map edges.
   Tests: `shore.txt`, `check_shore.py` (Momo's woods scene at 10 woods
   friends, onto the boardwalk, a shell gives a shore friend, shelf page
   2, shore song, Momo at the way on with 1/10, back to the woods).
   13 suites. Screenshots `docs/step6_4_shore.png`. Score 8.5 (first
   pass 8: Momo's bubble cut at the edge, a palm over Momo).
5. Cloud Hill. **Done.** `areas.clouds()` (480x320) from the 08
   mockup: a lavender to pink sky over the whole map (`world.sky_map`,
   bands repeat across so rows share tiles), a rainbow behind the big
   island, soft far clouds, a moon and stars. Walkable = cloud
   (`CloudBlob` islands joined by `puff_bridge()` chains of small puffs;
   a cell is solid unless 70% of it is cloud). Big island: capsule
   machine (`world.gacha`) under the rainbow, candy trees, lollipops, pen
   the meadow's size at 282..402 x 168..252 with its sign. A garden
   island south-west, Momo's island north-east. West: cloud steps down
   to the shore (exit 12 px wide, arrive on the shore at 452,150); on the
   shore, 4 cloud puffs at the east edge show the way up (arrive 24,204).
   Stars and moon use `st_lt`, the area's glint color, so they twinkle.
   682 tiles, 14 palettes. Capsules in 5 colors (`props.CAPSULE_COLORS`,
   one index image). Momo's happy end: `Area.momo` / `AreaMap.momo_x,y`
   is its bed (`world.cloud_bed`, BG: mattress and pillow); once every
   way is open Momo sleeps there tucked in under a sky-blue blanket
   sprite (`npc.blanket()`, 64x32, Momo's palette, `NPC_BLANKET`, drawn
   in front of Momo), z's rising. Pip within 40 px: Momo wakes, hops
   twice every 48 frames, hearts float up, a chime (log `momo home
   near`). `T_NPC_TXT` moved to 232 (Momo now takes 66 tiles). Fixed on
   the way: a save with no position (0,0) put Pip at the meadow's spawn
   in any area; `restore_pos` now uses the area's spawn.
   `make_save.py` takes AREA (5th argument; GATES may be `-`). Tests:
   `clouds.txt`, `clouds_continue.txt`, `check_clouds.py` (Momo's shore
   scene, up the steps, cloud song, a capsule gives a cloud friend, shelf
   page 3, Momo asleep under the blanket and awake, no gate Momo left,
   back down, Continue on Cloud Hill, sky and pill colors within one
   5-bit step: mGBA's output differs by a step). 14 suites. Screenshots
   `docs/step6_5_clouds.png`, map `docs/step6_5_clouds_map.png`. Scores:
   map 8.5 (first 7: flat capsule bridges, the sign under the machine,
   a bridge neck too narrow for Pip's feet; second 8: the west exit at
   6 px), Momo's bed 8.5 (first 7: Momo sat on the bed and the counter
   pill hid its z's; a first blanket read as a blue tub).
6. Open screen per container. **Done.** `props.acorn64`, `shell64`,
   `capsule64` (like `box64`: parts `body`, `open`, `lid` in one 64x64
   frame, bottom row 61). Acorn: pastel nut, brown cap with stem and
   scales is the lid, open shows the hollow nut. Seashell: a scallop fan
   (the lid) over a ridged dish; open shows the pearly inside and a
   pearl. Capsule: colored dome with a rim band (the lid) on a white
   half; open shows the inside. Each in its area's 5 colors, same order
   as the map containers. `export_game.py` writes `cont64_tiles[20]` /
   `cont64_pal[20]` (area * 5 + color, the meadow's gift boxes first;
   `box64_*` are gone). `open.c` loads `cont64_*[area * 5 + open_color]`
   and draws the lid before the body, so it is in front (an acorn cap
   overlaps its nut). The lavender backdrop stays for every area (any
   tint clashes with one of the 5 colors; the outlines keep the lavender
   ones clear). Log: `scene open color C friend F area A` (area at the
   end, older checks read the start). Tests: `open_woods/shore/clouds.txt`,
   `check_open.py` (the area's container in the seed's color waits on the
   cushion, its lid colors are gone after the pop, the inside shows, the
   friend is from that area; colors compared exactly, the art is already
   15-bit). 15 suites. Screenshots `docs/step6_6_open.png`, art sheet
   `docs/step6_6_containers64.png`. Scores: acorn 8.5, seashell 8.5
   (first 7.5: a tiny pearl on the rim, a small fan), capsule 8.5.
7. Mailbox and picnic basket (owner answers above). **Done.**
   Mailbox: the map's mailbox has no flag now (`world.mailbox(flag=False)`);
   the flag is a sprite, up (waving) while `game_save.mail_new`, down
   after reading, and an envelope bobs over the mailbox while a letter
   waits. Save (still v2, 144 bytes, in the old padding): `mail` (newest
   friend id + 1, set with `mail_new` by `collection_add`), cleared by new
   game. By the mailbox (the sign's zone, `near_prop`, only once a letter
   exists) the A bubble shows; A opens `scene_letter` (`letter.c`): a
   pink envelope drops in, wobbles, opens, the card slides up out of it
   (BG1 paper + BG0 text scroll together, BG2 backdrop stays; the card
   starts 96 px low because the 256 px map wraps and only rows 160..255
   are blank; BG0/BG1 show from the flap), then the friend pops into its
   round frame with a squeak, hearts and `tune_hello`. Text on the ruled
   lines: "Dear Pip," / one of 12 two-line messages (`(id * 5) % 12`) /
   "Love," / the friend's name. A squishes (3 go back), B or START go
   back, any press while it opens skips to the letter. Reading clears
   `mail_new`; with no new mail A re-reads the last letter. Basket: A
   pops a treat (8 kinds in `props.SNACKS`, the next in turn each time,
   the first random) out of the basket onto the blanket; the followers
   hop over to seats left, behind and in front of it (clear of Pip and
   the basket), each takes a bite with a squeak and a heart, a twinkle
   when it is gone, they hop back into line (134 frames). With no
   followers Pip hops twice and eats it. Pip waits during snack time;
   60 frames cooldown; no guide arrow meanwhile. Sprites: `ui_extra_tiles`
   (8 snacks, envelope, flag up / down, 16x16) share the sparkle palette
   (`ui_small_pal`, now 15 colors; the maps use all 16 OBJ palettes);
   the flag uses the flower yellows, not the arrow's (tests find the
   arrow by its yellow). `AreaMap.mail_x/y, basket_x/y`. Meadow 902
   tiles, 15 palettes. Tests: `mail.txt`, `mail_solo.txt`,
   `check_mail.py` (no bubble before a letter, the newest friend's
   letter, squish, back, re-read, flag up / down and envelope pixels,
   envelope and card, two different treats with 3 followers, Pip alone),
   `test_collection.c` (mail set by a new friend, not by a known one,
   cleared by new game, offset 123). 16 suites. Screenshot
   `docs/step6_7_mail_snack.png`. Scores: mailbox and letter 8.5 (first
   7: the card's bottom showed at the top while the envelope dropped; the
   flag was 4 px), snack time 8.5 (first 7.5: the right seat stood on
   Pip and the basket; the arrow drew over the treat).
8. Full 80-friend loop, straight paths, scores, release. **Done.**
   `make_full_loop.py` writes the script (the Makefile runs it): a new
   game, 20 containers per area in play order (every fourth opens by
   itself, every fourth friend leaves on its own), the way on after each
   area, the four shelf pages, Momo's bed; `full_b.txt` reboots with 80.
   `check_full.py`: 80 opens counting 1..80, no repeats, areas fill in
   order, each friend from the area it was opened in, each gate scene
   once after that area's 10th friend and the walk on, no containers
   once an area is full, containers and friends that act by themselves,
   each area's song, Momo home, Continue on Cloud Hill with 80 and no
   containers, every shelf page full. ~55,000 frames, ~17 s. The first
   run found 79/80: Cloud Hill spot (138,280) was in the sky.
   `check_map.py` now checks every container spot of every area is on
   the ground and reachable from the area's start (it also flagged
   (392,146) and (330,272)); they moved to (376,150), (100,274),
   (344,262). Owner requests: every path is straight (horizontal or
   vertical), inside maps too. Woods trail: in at y 200 to the
   clearing, north along x 240 to the big old tree, east at y 120 to
   the shore (a stump and a berry bush moved off it). Shore: the
   diagonal cloud steps are one straight cloud walkway east
   (`cloud_puff(72, 16)`, level with the exit); low white fences along
   the east and west edges, so the boardwalk and the walkway (Momo
   sleeps across it) are the only ways off: Momo could already not be
   walked around (the edge was an invisible wall), now it reads that
   way. Cloud Hill: the arrival path, the garden bridge and Momo's
   bridge are straight. Woods 615 tiles / 11 palettes, shore 539 / 9,
   Cloud Hill 683 / 14. 17 suites pass. Screenshot
   `docs/step6_8_full_loop.png`. Scores: full loop 8.5 (first run 7:
   an unreachable container), straight paths 8.5, shore fences 8.5.
9. Houses and gifts (owner request after 6.8, before step 7). **Done.**
   Gifts: `tools/gifts.py`, one 16x16 keepsake per species (tulip, yarn,
   honey pot, bell; crown, leaf, egg, mushroom lamp; sailboat, locket,
   beach ball, bucket; umbrella, star wand, rocket, gem), drawn in the
   friends' palette slots (1-5 ramp, 6 outline, 7 blush, 8 ink, 9
   white), so each shows in its friend's flavor with no new palette
   (`gift_tiles`, 16 species x 4 tiles). Gift = friend found (no new
   save data). The letter shows the friend's gift at twice size, bottom
   right of the card (affine matrix 1). Houses (`world.mushroom_house`,
   `beach_hut`, `cloud_cottage`; the meadow keeps its cottage), each
   with a door in `Area.doors`: woods at the north end of the straight
   path (the big old tree and one autumn tree gone), shore up the beach,
   Cloud Hill on the big island's west side (a candy tree moved).
   Walking up into a door opens `scene_house` (`house.c`, owner: not
   the shelf; START still opens the shelf). Room per area
   (`tools/rooms.py`, `room_art[4]`): themed wallpaper (hearts, leaves,
   waves, stars), wood floor, window, a plant and a floor cushion, a
   rug with Pip seen from behind, the name pill at the top, and four
   wall shelves: species 0 top left, 1 top right, 2 bottom left, 3
   bottom right, 5 flavors each (`ROOM_*` positions). Gifts not found
   are silhouettes in the shelf's tint (`sq_sil_pal`). A bouncing arrow
   (the guide arrow flipped) starts on the newest friend's gift if it
   lives here, else the first found; LEFT / RIGHT run along both shelves
   of a row, UP / DOWN switch rows; the pill says "From Matcha Bunny" or
   "Find me!". A: the gift hops with its friend's squeak and hearts; a
   missing one wiggles with a blip. B or START: back out of the door.
   Tiles and palettes: woods 656 / 13, shore 588 / 11, Cloud Hill 734 /
   13; rooms 136..173 tiles, 2 palettes each. Tests: `house_meadow/
   woods/shore/clouds.txt`, `check_house.py` (each door opens its room,
   the gift count, the arrow on a found gift, hop and wiggle, back out,
   vanilla colors on a found gift, the pill text, a silhouette),
   `check_map.py` (every house door reachable), `check_step2.py` (the
   cottage door opens its room). 18 suites. Screenshots
   `docs/step6_9_rooms.png`, `docs/step6_9_houses.png` (a full meadow
   room and the three new houses). Scores: gifts 8.5 (first 7.5: the
   honey pot label read as an eye, the locket chain as a dotted line),
   houses 8.5, rooms 8.5 (first 8: the letter showed a Cloud Hill gift,
   a byte offset used on a u32 array).

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
- Music runs from `scene_run` once per VBlank that passed (`vbl_count`
  from the IRQ handler), not once per loop pass: a slow frame must not
  slow the song. `hwcheck.c` has its own loop and still ticks once.
- A new `dbg()` line can split two log lines that a check expects
  together (`open press 3` then `open pop`); log after them.
- Switching `solo` in the harness while sound plays leaves a filter
  transient of ~2400 for a few frames; wait 4 frames before `audio`.
- Scene fades ignore input for about 10 frames; test scripts wait 20
  frames before the first press.
- Moving a box spot, the sign or the pen breaks the scripted walks in
  `step3_boxes.txt`, `pen.txt`, `pick.txt`, `pen20.txt` and the spot in
  `check_step3.py`. Probe Pip's position with a `dump` (IWRAM, `pip_x` /
  `pip_y` from `nm`) instead of guessing frame counts.
- Test randomness follows boot time: a new game's seed uses
  `frame_count` at the press, so a ROM that reaches the title one frame
  later (f3, not f2) shuffles differently and `step3_boxes.txt` walks
  past the wrong boxes. Boot time grows with zeroed RAM (`.sbss`, 47 KB):
  the letter's 5 text strips (10 KB) did it; one shared strip fixed it.
  Keep big buffers shared, and check `boot_only.txt` still shows
  "scene title" at f2.
- Pointer offsets into exported `u32` tile arrays count words, not
  bytes: a 16x16 sprite (4 tiles, 128 bytes) is 32 words (the letter
  once showed the wrong gift). `dma3_copy32` sizes are in bytes.
- The harness keeps walks out of a house door's zone (the door, and up
  to 12 px below it for the game, 16 for the harness), so a `walkto`
  goal inside it counts as unreachable and Pip is nudged straight at
  the goal, which can walk him into a door. Aim below the zone and
  `hold UP` to enter.
- An exit at the map edge must be at least 12 px wide: Pip's feet box
  stops 5 px from the edge, so a 6 px exit leaves one reachable column.
- A small prop that puts sky, cloud edge and its own colors in one 8x8
  tile can cost 2 BG palettes (Cloud Hill lollipop: 14 to 16). Check
  `bg(...)` after every placement near an edge.
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
