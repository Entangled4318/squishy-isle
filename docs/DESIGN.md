# Squishy Isle: game design

A calm collecting game for a toddler, built for Game Boy Advance and played on
an emulator (Trimui Brick Pro). You explore a small pastel island, open
surprise containers, meet squishy mochi friends and fill your shelf.
No battles. No fail states. No reading needed.

## Who it is for

A child of about 2 to 4 years. The child cannot read, presses buttons at
random, loses focus fast and loves the same surprise again and again. A
parent may sit alongside and read names out loud.

## Design rules

1. Every press gives a happy result. No button is wrong. Nothing can be lost.
2. Pictures, color, sound and motion carry all meaning. Text is only for the
   parent.
3. The loop is short, and the surprise repeats: wobble, pop, squish, keep.
4. Calm and soft. Pastel colors, gentle music, no flashing, no timers.

## Premise

On Squishy Isle, small mochi friends called squishies love hide and seek.
Every morning they curl up inside gift boxes, seashells, acorns and toy
capsules all over the island. Pip, a small kid in a bunny hoodie, finds
them, gives each one a gentle squish hello and brings them home to the
Squishy Shelf.

## Core loop (30 to 60 seconds)

1. **Explore.** Walk Pip around the area. Containers sparkle and chime.
2. **Open.** Touch a container. The view zooms in. Each press of A (or B)
   makes it wobble and jump higher, with a rising note. On the third
   press it pops. If the child does not press, it opens by itself after a
   short wait, so a very young child is never stuck.
3. **Meet.** A squishy bounces out with confetti. New friends get a star
   burst. Every press squishes it (squash, squeak, hearts). After three
   squishes or a short wait, it hops into Pip's bag.
4. **Play.** Back in the world, the new friend follows Pip. Friends you
   already found wander the area, so the island fills with life as the
   collection grows.

## Session loop (5 to 15 minutes)

Each area has its own container and four species. When the child finds six
friends in an area, the squishies build the way to the next area (bridge,
boardwalk, stairs, rainbow). The shelf (Start) shows every friend found.

## Long loop

Every species comes in five flavors: Vanilla, Strawberry, Matcha, Taro and a
rare, shimmering Sparkle. Duplicates are never wasted: each repeat adds a
heart, and three hearts give that friend a tiny crown on the shelf. A full
shelf page starts a squishy parade.

Rolls lean toward friends the child does not have yet, so new faces come
often. The first five opens are always new.

## Content

| Area | Container | Squishies |
| --- | --- | --- |
| Blossom Meadow | Gift box | Bunny, Kitty, Bear, Chick |
| Seashell Shore | Seashell | Whale, Octo, Seal, Crab |
| Berry Woods | Acorn | Frog, Fox, Dino, Shroom |
| Cloud Hill | Capsule machine | Cloud, Star, Planet, Uni |

4 areas x 4 species x 5 flavors = 80 squishies.

## Controls

| Button | Action |
| --- | --- |
| D-pad | Walk, move shelf cursor |
| A | Open, squish, choose |
| B | Hop (in the world), also opens and squishes |
| Start | Shelf |
| L / R | Shelf pages |
| Select | Nothing (safe) |

Toddler proofing: soft reset combo is off, there is no delete option in play,
the game saves by itself after each new friend. A parent can erase the save
by holding L + R + Select for 5 seconds on the title screen.

## Screens

Title, world (4 areas), open, reveal, squish, shelf, bridge building,
parade. Mockups of the first eight are in `mockups/`.

## Mockup scorecard

Scored against: toddler readability, charm, cohesion with the rest of the
set, and whether the GBA can really show it. First pass and final pass.

| Part | First | Final | Main fixes |
| --- | --- | --- | --- |
| Squishy art (16 species x 5 flavors) | 7.0 | 8.7 | Cloud read as a swirl, whale tail, chick wings, dino, unicorn palette overflow, planet ring and noses at 16 px |
| Pip (player) | 7.5 | 8.5 | Symmetric arms, bigger shiny eyes, smile, cleaner side view |
| Title | 7.5 | 8.6 | Cast stands on the island, logo letters as bouncing sprites, foam, sparkles |
| Blossom Meadow | 7.0 | 8.5 | Oval pond, smooth path, bigger flower beds, mailbox, picnic, sign |
| Open (wobble) | 6.5 | 8.5 | Box sits on a cushion, lavender backdrop for contrast, dithered glow, big A prompt, heart progress |
| Reveal | 7.0 | 8.7 | Friend lands on the cushion instead of on the box, bigger confetti |
| Squish | 6.5 | 8.6 | Native squash art instead of affine smear, clear > < face, hearts, puffs |
| Shelf | 7.5 | 8.6 | Pastel rows per species, tinted silhouettes, gold frame, locked tabs dimmed |
| Seashell Shore | 7.0 | 8.5 | Striped umbrella, sandcastle contrast, palm trunks, boardwalk, sailboat |
| Cloud Hill | 7.0 | 8.5 | Puffy scalloped clouds, cotton-candy trees, rainbow, capsule machine |

Every screen passes the checks in `tools/scene.py`: at most 4 BG layers,
16 BG palettes and 16 OBJ palettes, 15 colors per sprite and per 8x8 tile.
The meadow is the tightest at 14 of 16 BG palettes.

## Known limits of the mockups

- They come from the pipeline's GBA-faithful renderer, not from an
  emulator. The ROM uses the same art data, but motion, timing and sound
  are not shown.
- The spinning sunburst needs an affine background (Mode 1) in the ROM.
- The squish uses stored squash frames plus a small affine wobble, because
  pure affine scaling smears the face at 64 px.

## Audio

Soft chiptune on the four GBA tone channels: a slow lullaby for each area,
a music box theme on the shelf. Sound effects: wobble boing, pop, sparkle
chime, one squeak per species (pitch changes by flavor), new friend jingle.

## Technical plan

- C, Mode 0. BG3 sky and far layer, BG2 ground, BG1 tree tops above Pip,
  BG0 HUD. Sprites for Pip, squishies, containers, sparkles and confetti.
- Squash and stretch uses OBJ affine matrices, so the squish is real
  hardware scaling of the 64x64 art.
- Art is made by the Python pipeline in `tools/`. The same data drives
  the mockups and the ROM, so the screenshots are what the ROM shows.
- All colors are 15-bit and every 8x8 tile and sprite obeys the 16-color
  limit. The mockup renderer checks this.
- Save: 32 KB SRAM with a magic value and checksum.
- Screen: 240x160. On the Trimui Brick (1024x768) use 4x integer scale
  (960x640) for sharp pixels.
