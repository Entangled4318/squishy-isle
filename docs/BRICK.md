# Squishy Isle on the Trimui Brick

Version 1.0. File: `release/squishy-isle.gba` (617 KB). The version shows
small at the bottom right of the title screen, so you can see which build
is on the Brick.

## Install or update

1. Download `release/squishy-isle.gba` from the `main` branch.
2. Copy it by FTP into the GBA ROM folder on the Brick's SD card (on the
   stock firmware this is `Roms/GBA`; custom firmwares use a similar GBA
   folder).
3. Keep the file name `squishy-isle.gba`. The emulator names the save
   after the ROM, so an update with the same name keeps the collection.
   Overwrite the old file; do not keep two copies with different names.

Saves from older builds load. A save from before step 3 (version 1) is
too old and starts a new game.

## Emulator settings

- Core: use the same GBA core as for the earlier Brick tests (the step 1
  hardware check passed on it). The game is built and tested on mGBA, so
  pick mGBA if the firmware offers a choice. gpSP should also work.
- Screen: integer scale, 4x (960 x 640 on the 1024 x 768 screen), no
  smoothing filter. This keeps every pixel sharp.
- Leave fast-forward and cheats off. Audio needs no change.

## Saves

- The game saves by itself: after each new friend, after a letter is
  read, after a follower change, when Pip goes through a door or to
  another area, and when Pip stands still for 1 second.
- The save is 32 KB battery SRAM with two copies. If the power goes off
  during a write, the copy before it loads (tested at every byte).
- Most GBA cores write the save file to the SD card when you leave the
  game. Leave the game with the Brick's menu before you switch it off.
- To back up the collection, copy the save file by FTP. It has the ROM's
  name with `.srm` or `.sav` at the end, in the emulator's save folder.
- Save states are not needed. If the firmware resumes from a state, that
  is fine. Loading an older state brings back an older collection.

## For grown-ups

- New game is only on the title screen. With friends on the save it asks
  "Start over?" with No first, and Yes needs A held for 3 seconds.
- Nothing in play deletes friends. SELECT does nothing.
- Hardware check: hold L + R + SELECT while the game starts. It shows the
  buttons, a sound, the save and the boot count.
- Sound test: hold L + R + START while the game starts. LEFT / RIGHT
  choose a song, A plays, B stops, SELECT plays a jingle, L boings,
  UP / DOWN play effects.

## How to play (for the parent)

Walk Pip with the D-pad. Walk into a gift box, acorn, seashell or capsule
and press A (or B): it wobbles, jumps and pops open by itself if nobody
presses. A new friend lands; A squishes it. B makes Pip hop. START opens
the shelf (L / R turn the pages, A shows a friend big). The heart sign by
the pen picks who follows Pip. The mailbox holds a letter from the newest
friend, the picnic basket gives snack time, and each house holds the
gifts the friends sent. Ten friends in an area wake Momo the panda, who
opens the way to the next area. Twenty friends start a parade.

## Brick test checklist

Tick each line on the Brick. Items marked (ear) need the Brick's speaker.
The emulator tests cannot judge these.

Start and save

- [ ] The ROM starts; the title letters drop in with a chime each; any
      button skips.
- [ ] Continue after power off: Pip starts where he last stood still,
      with the same friends and followers.
- [ ] Hold L + R + START at start: the sound test opens. Hold
      L + R + SELECT: the hardware check opens.

Map and walking

- [ ] Walking feels smooth; no invisible walls; Pip walks close beside
      bushes and past the pen corners.
- [ ] B makes Pip hop, the followers hop after him; the "hup" is soft,
      also when B is mashed. (ear)
- [ ] After 5 s without a find, a blinking arrow next to Pip points to
      the nearest container, with no chime.
- [ ] The counter pill (top right) reads at 4x and hops on a new friend.

Containers, open and reveal

- [ ] Each container reads at 4x: gift box, acorn, seashell, capsule.
- [ ] Open screen: 3 presses pop it; with no press it opens by itself.
      The lid, cap, top shell or dome flies off.
- [ ] The friend lands with stars and confetti; 3 squishes or a short
      wait and it hops away.

Friends, pen and shelf

- [ ] The first 3 friends of an area follow Pip; the rest waddle in the
      pen without piling up.
- [ ] The heart sign opens the picker in every area; the big heart badges
      mark the followers; A adds or removes one.
- [ ] START then A on a friend opens it big; 3 squishes go back.

Mailbox, snack, houses

- [ ] After a new friend the mailbox flag waves and an envelope bobs; A by
      the mailbox opens the letter; the friend's gift shows on it.
- [ ] Snack time at the basket: a treat pops out, the followers hop over
      and squeak; the pace feels cosy.
- [ ] Walk up into each house door: the room shows that area's gifts;
      long names such as "From Strawberry Shroom" fit the pill.

Areas and Momo

- [ ] With 10 friends Momo wakes (the tickle scene), and the way on opens:
      meadow to woods, woods to shore, shore to Cloud Hill.
- [ ] The west exits of the woods and the shore always take Pip back.
- [ ] Momo sleeps in its cloud bed at the end and hops with hearts when
      Pip comes close.

Parade

- [ ] The 20th friend of an area starts the parade: 20 friends march past
      and hop, hearts and twinkles drift down. It feels festive, not long
      (about 7 s). It plays once per area.

Music and sound (ear)

- [ ] Each area has its own calm song; title, shelf music box, open tune
      and reveal jingle sound right; loops are seamless.
- [ ] The bass is audible on the speaker (lowest note F2, 87 Hz).
- [ ] Chimes, squeaks and boings are clear over every song, not harsh.
- [ ] No pop or click when a song resumes after the shelf or a box.

Long play

- [ ] A long session (30 minutes or more) with button mashing: nothing
      sticks, no slowdown, the music keeps time.

Tell me what fails, with the area and what you did, and I will fix it.
