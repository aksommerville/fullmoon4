# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### Final graphics, etc. Finish before 13 August.

- [x] Demo: Mountains default tile changed. Needs some edging.
- [x] Demo: Church teleport spell.
- [x] Magic school: Toy castle, door opens when you cast spell of opening. ...meh, just use a regular gate.
- [x] bucklock: Fireworks, sound effect, when unlocking.
- [x] datan: Generate tile usage report.
- [x] Steppe: hole tile.
- [x] Village/forest boundary, need big tree on the village side.
- [ ] Furnish Dot's house.
- [x] Big tree edges don't line up.
- [x] image castleext: Water-into-trees, 4 tiles needed.
- [x] desert: tiles for forest boundary
- [x] Church: join path at welcome mat.
- [x] Castle basement: Make the church attachment look like the church as a hint.
- [x] swamp: forest edge
- [x] "Draw door" book in Goody's room.
- [x] Goody should face left.
- [ ] Compass: make easier to read
- [x] Choose-a-door room.
- [ ] Black out unused tiles.

### Remaining major features. Finish before 31 August.

- [ ] End credits.
- [ ] Settings menu.
- - [ ] Fullscreen
- - [ ] Driver selection
- - [ ] Driver settings
- - [ ] Music mute
- - [ ] Input config
- - [ ] Language
- - [ ] Zap saved game. And show details.
- [ ] macos (on iMac only), crash on quit. Can we cause it to close the window instead? The implicit quit on window close doesn't crash.
- [ ] macos (on iMac only), glViewport needs to scale by NSScreen.backingScale.
- [ ] MacOS: Build two independent app bundles. Demo and Full.
- [ ] Windows

### Further critical features and bugs. Finish before 15 September.

- [ ] Web reported play time runs short -- it doesn't match what gets saved.
- [ ] treadle-driven firenozzles, can we ensure they bump back to the OFF state instantly when re-entering the map?
- [ ] Ensure that pitcher, when it shows highlight under a bonfire, will actually hit the bonfire! I've seen it fail.
- [ ] Arriving at swamp the first time, seems I can't guess wrong in the first mazelet. Is that my imagination? Or is it not getting set?
- [ ] Crow guidance for full version. fmn_secrets.c:fmn_secrets_get_guide_dir()
- [ ] UI sound effects, eg change menu selection.
- [ ] web: Sound effects too quiet relative to music.
- [ ] Metal
- [ ] inmgr: Real input mapping.
- [ ] InputManager.js: Must handle unconfigured devices connected before launching config modal.
- [ ] Touch input.
- [ ] Input configuration.
- - [ ] UI for mapping.
- [ ] minsyn drums too loud
- [ ] Confirm tolltroll acknowledges invisible. And everything else sight-oriented.

### TODO: Not critical.

- [ ] web: Language selection, figure out how that works
- [ ] Soft render: Alpha problem with arrows in wand feedback.
- [ ] Sound effect for coin toss
- [ ] _Enchanting Adventures: The Witch's Quest_ minigame.
- [ ] Tree-shake resources when packing.
- [ ] stdsyn. Or drop it, maybe minsyn is adequate?
- [ ] Summoning the crow potentially examines every command in every map, every time. Can we pre-index all that by itemid?
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls. Currently 1..50 ms per Clock.js
- [ ] Extra mappable input actions, eg hard pause and fullscreen toggle. I specifically don't want these for the demo, so punt.
- [ ] Consider WebGL for rendering. CanvasRenderingContext2D is not performing to my hopes.
- - 2023-05-07: A quick test (no actual rendering, just clear the framebuffer), and WebGL is no better.
- [ ] Unit tests.
- - [ ] General-purpose test runner.
- - [ ] C tests
- - [ ] JS platform tests
- [ ] Automation against headless native build.
- [ ] Native fiddle tool.
- - [ ] MIDI-In and PCM/synth config at command line? Would be cool to have some functionality without the web app.
- - [ ] Broad support for level analysis -- I'd like to replace `assist` with this.
- - [ ] Mysterious segfault. Refreshed browser while song running and fiddle C code had changed.
- [ ] Playing first_frost via fiddle, with screwing around on ch 14, the bells timing is off.
- - Also noticing in blood_for_silver, as i build up inotify. (no midi in, just data changes)
- - These, and the segfault above, might be due to borrowed songs in minsyn:   if (!(DRIVER->song=midi_file_new_borrow(src,srcc))) return -1;
- - Problem is the live game depends on that serial address to detect redundant requests.
- - [ ] Can we move the redundant-song check up to the platform level, and make it undefined at the synth level?
- - I want songs to continue playing across archive reloads, in fiddle. (in fact this is very important)
- [ ] Measure full memory usage after all songs and sound effects have played -- I'm concerned it might be too high for the Pi.

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
