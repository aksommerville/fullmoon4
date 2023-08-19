# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### Remaining major features. Finish before 31 August.

- [ ] Web: Persist Preferences. And sneak language into that too.
- [ ] Web: Interactive input config.
- [ ] bigpc: respect fmn_platform_settings.scaler
- [x] macos (on iMac only), crash on quit. Can we cause it to close the window instead? The implicit quit on window close doesn't crash.
- - On closer examination, it is impossible to reproduce. Must have been due to cosmic rays interacting with swamp gas.
- [x] macos (on iMac only), glViewport needs to scale by NSScreen.backingScale.
- [ ] MacOS: Build two independent app bundles. Demo and Full.

### Little bugs and narrative concerns, after playtest 3 August.

- [x] Credits: Lower text is not erasing each time. (soft render, windows and linux)
- [x] Also, Dot and Wolf render incorrectly when slightly offscreen during the yellow floor of credits.
- [x] Soft render: Wand tattle arrows not using alpha. Up looks fine, all other directions are mixed up.
- [ ] Mu block: (A) and (B) directions must be different! As is you can tickle 5 times in one direction.
- [ ] credits: Sound effect on delivering the winter clothes
- [ ] FIRE/MOON chalkguard: Didn't open the first time I entered MOON. Clear and redo, and it worked. etc/notes/20230806-chalkguard.save (saved after proceeding)
- [ ] Hard-coded device name "System Keyboard". Can that be moved to a string resource for translation?
- [ ] Look for other hard-coded text.
- [ ] Make the farmer emerge faster. Should be hard not to see him.
- [ ] Move items around: Compass to Swamp. Umbrella and Wand to Beach? Violin to music shop. Shovel must come earlier
- [ ] North pole: Shuffle land bridges south so no blowback while on dry land.
- [ ] Can we make the beach sign more legible?
- [ ] Violin is still too unforgiving.
- [ ] Can the Swamp entrance be made more prominent?
- [ ] Free seeds at Dot's house.
- [ ] Snow: Interior corners missing one pixel of outline.
- [ ] Music shop: Couple wrong tiles in the wall.
- [ ] Static educator: Use treadles instead of connect-the-blocks.
- [ ] Static educator: Make feather image more obvious.
- [ ] Static educator: Don't use numbers on the alphablock slide. Use a cloverleaf of arrows.
- [ ] Can push alphablock via intermediate pushblock.
- [ ] Swap wand trap: Allow to get the Umbrella, don't trap them so tight.
- [ ] Desert dead space, do something with it.
- [ ] Move Seamonster Pong near the Umbrella. In the Swamp?
- [ ] Music teacher: Engage from anywhere, not just right in front of me.
- [ ] More hat trolls outside. Make it very difficult to get by without the Hat.
- [ ] Put Hat in the Mountains.
- [ ] "R" chalk, allow negative slope.
- [ ] "R" chalk: Accept narrow top bubble.
- [ ] Castle pumpkin icon, make more legible.
- [ ] Possible to stand inside the dragon's head. Fire should hit you there.
- [ ] Ensure feather actuates dragons on all exposed parts (possibly beyond the sprite).
- [ ] Lambda and charmed pushblock: Hesitate briefly at cell boundaries.
- [ ] All blocks: Fuzz movement off-axis, make easier to squeeze in tight corridors.
- [ ] Positive and negative beeps from alphablocks as you encode.
- [ ] Castle 2f, the one with gamma, lambda, and water: Add a conveyor in the vertical corridor so lambda can't escape that way.
- [ ] Firewall: Respect invisibility.
- [ ] "Don't look" sign for firewall, nobody gets it on their own.
- [ ] Konami code easter egg.
- [ ] horz/vert indicator showed null initially.
- [ ] Spawn in a wall after being killed by panda. (Macbook 2023-08-11T16:05)
- [ ] Speed up the velocity envelope's release a little?
- [ ] Cut violin to 2 beats/measure or cut tempo, so we can play the click for every possible note slot.
- [ ] Completed full version with VICTORY splash, said 10:14. Continued and completed fast with CREDITS splash, said 13:something, that can't be right. (reran with VICTORY, and 13:36)

### Further critical features and bugs. Finish before 15 September.

- [ ] macos: "Quit" from Hello makes app unresponsive but doesn't actually quit.
- [x] Castle 3f, west of the OTP read, the upper stompbox spoils the whole puzzle.
- [ ] Orphans should keep their winter clothes after your first victory.
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
- [ ] vcs: I'm still getting music problems on startup, the first few notes get clobbered.

### TODO: Not critical.

- [ ] After a 45-minute session: bigpc_quit, clock stats: Final game time 1800 ms (1800 ms real time). overflow=0 underflow=0 fault=0 wrap=0 cpu=0.036068
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
