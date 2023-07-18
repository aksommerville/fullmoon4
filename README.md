# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### Broad areas. Finish before 21 July.

- [x] Mountains and caves.
- [ ] Forest.
- [ ] Review village.
- [ ] Make two more teleport spells: Steppe and Desert
- [ ] All maps we expect to need must exist at this point. Not necessarily populated.
- [x] Editor: Mountain rocks are not joining correctly, at SE and SW when an irrelevant diagonal present. Tileprops looks kosher. ?
- - Have to filter out zero weights before counting bits.
- [x] Buried door must be allowed to have gsbit zero, meaning it doesn't stay open after leaving screen. (as is, it is open without digging)
- - Use gsbit 68

### Populate all interactions. Finish before 18 August.

- [ ] Mountains: Hat trolls aren't deadly enough. Add static hazards for them to knock you into? More of them? Faster missiles?
- [ ] Static hazards. Both shovellable and non.
- [ ] Can I make an Umbrella guard more flexible than firewall? Rapid-fire missiles?
- [ ] Castle.
- [ ] Village, mostly education.
- - [ ] Song tutorials in the village: Show ghost notes, if you stand in the right place.
- - [ ] Orphans.
- [ ] Mountains.
- [ ] Swamp.
- [ ] Forest.
- - [ ] Ensure HOME is pumpkinproof; a pumpkin can leave the castle, to just the first screen outside. (or guard inside the castle? or make the exterior multihome?)
- [ ] Beach.
- [ ] Desert.
- [ ] Steppe.
- [ ] Gameplay should now be complete.

### Final graphics. Finish before 1 September.

- [ ] Village/forest boundary, need big tree on the village side.
- [ ] Furnish Dot's house.
- [ ] Big tree edges don't line up.
- [ ] image castleext: Water-into-trees, 4 tiles needed.
- [ ] desert: tiles for forest boundary
- [ ] Church: join path at welcome mat.
- [ ] Castle basement: Make the church attachment look like the church as a hint.

### Further critical features and bugs. Finish before 15 September.

- [ ] End credits.
- [ ] Settings menu.
- - [ ] Fullscreen
- - [ ] Driver selection
- - [ ] Driver settings
- - [ ] Music mute
- - [ ] Input config
- - [ ] Language
- - [ ] Zap saved game. And show details.
- [ ] I broke auto-bloom somewhere; plants are now sprouting after entering the map. Which is actually not bad. Maybe keep it this way and drop the old logic instead of fixing?
- [ ] UI sound effects, eg change menu selection.
- [ ] web: Sound effects too quiet relative to music.
- [ ] Violin: Fuzz playback? Look for matches that allow off-by-one somehow.
- [ ] macos (on iMac only), crash on quit. Can we cause it to close the window instead? The implicit quit on window close doesn't crash.
- [ ] macos (on iMac only), glViewport needs to scale by NSScreen.backingScale.
- [ ] MacOS: Build two independent app bundles. Demo and Full.
- [ ] Metal
- [ ] inmgr: Real input mapping.
- [ ] InputManager.js: Must handle unconfigured devices connected before launching config modal.
- [ ] Touch input.
- [ ] Input configuration.
- - [ ] UI for mapping.
- [ ] Windows
- [ ] minsyn drums too loud

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
