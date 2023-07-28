# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

2023-07-23, observations on playthru:

- Farming is not obvious. Maybe bring the shovel forward? Or somehow lead from shovel to farming.
- Violin is acquired late, and is not really useful at all.
- - Add a musical gate in the castle, for sure.
- - It's not overpowering like the Broom or Snowglobe, no reason not to provide it early.
x Need the hat trolls deadly enough that it's unlikely to get past without wearing the hat.
x Need a beach sign from the third screen, otherwise one assumes you're supposed to walk toward the swamp or mountains.
- Play time just over 30 min, and that's about where I want it. :)

### Populate all interactions. Finish before 18 August.

- [x] Shuffle items. See playthru notes above.
- [x] Mountains: Hat trolls aren't deadly enough. Add static hazards for them to knock you into? More of them? Faster missiles?
- [x] Static hazards. Both shovellable and non. ...nah we don't need shovellable or solid
- xxx Can I make an Umbrella guard more flexible than firewall? Rapid-fire missiles? ...no particular need for this
- [x] Castle.
- - xxx Some kind of musical gate.
- [x] Village, mostly education.
- - [x] Song tutorials in the village: Show ghost notes, if you stand in the right place.
- - [x] Orphans.
- - [x] Purchase hat.
- - [x] Library: 1f=3 alphablock books. 2f=chalk HAND/BIRD FIRE/MOON. (current spell books are placeholders; remove)
- - [x] Library: World map.
- - [x] Magic school 1f: Charm challenge. Make a racoon walk thru fire to open the door.
- - [x] Magic school 2f: Spell books Slowmo and Rain (two you would already know by this point). Ladder up guarded by a mu block.
- - [x] Magic school 3f: Spell books Invisibility and Opening. Both critical, and both only taught here. --teacher, not book, for invisibility
- - [x] Dept of Agriculture: Plainly readable help: Water=>Seed, Milk=>Cheese, Sap=>Match, Honey=>Coin, Song of Blooming, Rain Spell. (none of this is unique)
- - xxx Dept of Agriculture basement: Guide to killing werewolfs? ...nah let this remain a mystery
- [x] Mountains.
- [x] Swamp.
- - [x] Put a trick floor just west of the maze exit.
- [x] Forest.
- - [x] Ensure HOME is pumpkinproof; a pumpkin can leave the castle, to just the first screen outside. (or guard inside the castle? or make the exterior multihome?)
- [ ] Beach.
- - [ ] Bury some treasure. You just got a shovel and compass, let's use them!
- [ ] Desert. Some dead space in the middle, do something with it.
- [ ] Steppe. Needs some hazards. A new seamonster for the north.
- [ ] Steppe is ridiculously easy if you get the broom first. Are we confident that that's not likely? Maybe broom should go back there.
- [ ] Need to lead more, to the bell. Not obvious at all.
- [ ] I still think farming is not obvious enough.
- [ ] Nerf the shovel puzzle a little.
- [ ] Use skyleton somewhere.
- [ ] '' duck
- [ ] '' rat
- [ ] Ghosts at the graveyard.
- [ ] Gameplay should now be complete.

### Final graphics, etc. Finish before 1 September.

- [ ] Magic school: Toy castle, door opens when you cast spell of opening.
- [ ] bucklock: Fireworks, sound effect, when unlocking.
- [x] Caves: Illumination sprite so the doors show up while dark.
- [ ] Village/forest boundary, need big tree on the village side.
- [ ] Furnish Dot's house.
- [ ] Big tree edges don't line up.
- [ ] image castleext: Water-into-trees, 4 tiles needed.
- [ ] desert: tiles for forest boundary
- [ ] Church: join path at welcome mat.
- [ ] Castle basement: Make the church attachment look like the church as a hint.
- [ ] swamp: forest edge
- [ ] Hat shop: mannequins
- [ ] Compass: make easier to read?

### Further critical features and bugs. Finish before 15 September.

- [ ] treadle-driven firenozzles, can we ensure they bump back to the OFF state instantly when re-entering the map?
- [ ] Ensure that pitcher, when it shows highlight under a bonfire, will actually hit the bonfire! I've seen it fail.
- [ ] Arriving at swamp the first time, seems I can't guess wrong in the first mazelet. Is that my imagination? Or is it not getting set?
- [ ] Crow guidance for full version. fmn_secrets.c:fmn_secrets_get_guide_dir()
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
- xxx Violin: Fuzz playback? Look for matches that allow off-by-one somehow. ...too complicated, and we're educating better now so i think not a problem.
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
