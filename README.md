# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### TODO: Map changes

- [ ] "Don't look" sign for firewall, nobody gets it on their own.
- [ ] Move items around: Compass to Swamp. Umbrella and Wand to Beach? Violin to music shop. Shovel must come earlier
- [ ] North pole: Shuffle land bridges south so no blowback while on dry land.
- [ ] Can we make the beach sign more legible?
- [ ] Can the Swamp entrance be made more prominent?
- [ ] Swamp wand trap: Allow to get the Umbrella, don't trap them so tight.
- [ ] Desert dead space, do something with it.
- [ ] Move Seamonster Pong near the Umbrella. In the Swamp?
- [ ] More hat trolls outside. Make it very difficult to get by without the Hat.
- [ ] Put Hat in the Mountains.

### TODO: Critical bugs, roughly sorted by effort

- [ ] Settings: Can get "scaler=''" by pressing Left.
- [ ] "R" chalk, allow negative slope.
- [ ] "R" chalk: Accept narrow top bubble.
- [ ] Mu block: (A) and (B) directions must be different! As is you can tickle 5 times in one direction.
- [ ] Make the farmer emerge faster. Should be hard not to see him.
- [ ] credits: Sound effect on delivering the winter clothes
- [ ] Free seeds at Dot's house.
- [ ] treadle-driven firenozzles, can we ensure they bump back to the OFF state instantly when re-entering the map? (eg 2 screens south of home)
- [ ] Music teacher: Engage from anywhere, not just right in front of me.
- [ ] Snow: Interior corners missing one pixel of outline.
- [ ] Music shop: Couple wrong tiles in the wall.
- [ ] Castle pumpkin icon, make more legible.
- [ ] Speed up the velocity envelope's release a little?
- [ ] Violin is still too unforgiving. Rewrite songs with only 2 beats/measure. (ie every metronome click)
- [ ] Can push alphablock via intermediate pushblock.
- [ ] Positive and negative beeps from alphablocks as you encode.
- [ ] Static educator: Use treadles instead of connect-the-blocks.
- [ ] Static educator: Make feather image more obvious.
- [ ] Static educator: Don't use numbers on the alphablock slide. Use a cloverleaf of arrows.
- [ ] Konami code easter egg.
- [ ] Orphans should keep their winter clothes after your first victory.
- [ ] Ensure feather actuates dragons on all exposed parts (possibly beyond the sprite).
- [ ] Possible to stand inside the dragon's head. Fire should hit you there.
- [ ] Castle 2f, the one with gamma, lambda, and water: Add a conveyor in the vertical corridor so lambda can't escape that way. And move button to more convenient spot.
- [ ] Confirm tolltroll acknowledges invisible. And everything else sight-oriented.
- [ ] Firewall: Respect invisibility.
- [ ] Lambda and charmed pushblock: Hesitate briefly at cell boundaries.
- [ ] All blocks: Fuzz movement off-axis, make easier to squeeze in tight corridors.
- [ ] horz/vert indicator showed null initially.
- [ ] UI sound effects, eg change menu selection.
- [ ] Ensure that pitcher, when it shows highlight under a bonfire, will actually hit the bonfire! I've seen it fail.
- [ ] Completed full version with VICTORY splash, said 10:14. Continued and completed fast with CREDITS splash, said 13:something, that can't be right. (reran with VICTORY, and 13:36)
- [ ] FIRE/MOON chalkguard: Didn't open the first time I entered MOON. Clear and redo, and it worked. etc/notes/20230806-chalkguard.save (saved after proceeding)
- [ ] Spawn in a wall after being killed by panda. (Macbook 2023-08-11T16:05)
- [ ] minsyn drums too loud
- [ ] Hard-coded device name "System Keyboard". Can that be moved to a string resource for translation?
- [ ] Look for other hard-coded text.
- [ ] web: Sound effects too quiet relative to music.
- [ ] Web reported play time runs short -- it doesn't match what gets saved.
- [ ] web: Sometimes images don't load after a rebuild.
- [ ] InputManager.js: Must handle unconfigured devices connected before launching config modal. (note: This was some time ago; reassess)
- [ ] vcs: I'm still getting music problems on startup, the first few notes get clobbered.
- [ ] Windows: Must sign executable.
- [ ] MacOS: Must sign executable.

### TODO: Not critical.

- [ ] Touch input.
- [ ] web incfg with hats: Coded but not tested. Validate on a Mac.
- [ ] After a 45-minute session: bigpc_quit, clock stats: Final game time 1800 ms (1800 ms real time). overflow=0 underflow=0 fault=0 wrap=0 cpu=0.036068
- [ ] web: Language selection, figure out how that works
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
