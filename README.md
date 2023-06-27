# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

And tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### May

- [ ] Track travel. Use for crow guidance, and report coverage at the end. Flag maps eg border as "not participating in coverage".
- - Or use a "set gsbit" map command, for selected maps only? Coverage reporting is not a big priority.
- [x] Scripts to analyze logs.
- [ ] Metal
- [ ] Can I make rubber trees that you can squeeze between them by pushing?
- [ ] minsyn drums too loud
- [ ] Native fiddle tool.
- - [ ] MIDI-In and PCM/synth config at command line? Would be cool to have some functionality without the web app.
- - [ ] Broad support for level analysis -- I'd like to replace `assist` with this.
- - [ ] Mysterious segfault. Refreshed browser while song running and fiddle C code had changed.
- xxx Can we move the rabbit? He teaches a song but users will not have the violin at that point. meh
- xxx Is the panda too distracting, at Seamonster Pong? meh
- [ ] Playing first_frost via fiddle, with screwing around on ch 14, the bells timing is off.
- - Also noticing in blood_for_silver, as i build up inotify. (no midi in, just data changes)
- - These, and the segfault above, might be due to borrowed songs in minsyn:   if (!(DRIVER->song=midi_file_new_borrow(src,srcc))) return -1;
- - Problem is the live game depends on that serial address to detect redundant requests.
- - [ ] Can we move the redundant-song check up to the platform level, and make it undefined at the synth level?
- - I want songs to continue playing across archive reloads, in fiddle. (in fact this is very important)
- [ ] Measure full memory usage after all songs and sound effects have played -- I'm concerned it might be too high for the Pi.

### Full Maps

- [ ] Village/forest boundary, need big tree on the village side.
- [ ] Furnish Dot's house.
- [ ] Big tree edges don't line up.
- [ ] image castleext: Water-into-trees, 4 tiles needed.
- [ ] desert: tiles for forest boundary
- [ ] image steppe: Entirely temporary
- [ ] steppe/beach boundary in blowback border
- [ ] Church: join path at welcome mat.

#### Major areas

- [ ] Castle (int) -- seven_circles
- [ ] Village N -- snowglobe
- [ ] Forest C -- toil_and_trouble
- [ ] Swamp SW -- gloom_for_company
- [ ] Beach SE -- eye_of_newt
- [ ] Steppe N -- first_frost
- [ ] Desert E -- blood_for_silver
- [ ] Mountains NW -- A Stone to Take My Place?

### After GDEX

- [x] Spawn on hazard
- [x] Text on inventory menu
- [x] Horz/vert instead of spinwise/widdershins for turning puzzle.
- [ ] The church's altar area looks like a stage; can we make something happen when you play the violin up there?
- [x] Minimum time at treasure menu, prevent accidental dismiss.
- [x] '' game over menu.
- [x] Chalk: clear all
- [x] Chalk: highlight active endpoints
- [ ] More involved tutorial for violin?
- [x] Firewall at 720x400 visible aliasing. (texture filter?)
- [ ] Show spell while encoding.
- [ ] Make most animals pumpkinable, make a generic pumpkin.
- [ ] Snow, when you use the snowglobe.
- [ ] Rabbit: sync song to metronome
- [ ] Violin: Fuzz playback? Look for matches that allow off-by-one somehow.
- [ ] Visible rests in rabbit song. And graffiti song in the pub. (and anywhere else we teach a song)
- [ ] Wind spell tutorial: The circle doesn't make sense, find something better.
- [ ] Trick floor: Discrete movement somehow?
- [ ] Trick floor: Timeout fires.
- [ ] Trick floor: Shorter path, say 9 steps?
- [ ] fmn_hero_return_to_map_entry: Make soulballs coalesce on the new position.
- [ ] Pumpkins can get trapped near the wand. Maybe better to prevent pumpkins from leaving the village?
- [ ] Seamonster should be ticklish (all monsters should).
- [x] Extend forests to prevent flying around the island; eliminate the northern arc of the demo world.
- [ ] Farmer: Tolerate other plant states. Esp pre-dug hole.
- [ ] Bloom plants while on screen?
- [ ] Ghost of plant when you dig it up.
- [ ] Cow: Black vertical stripes over the eyes and a big pink nose, look it up.
- [ ] Snowglobe: Minimum interval between strokes? Or at least between sound effects.
- [ ] Hearts or something above blocks and raccoons when enchanted.
- [ ] _Enchanting Adventures: The Witch's Quest_ minigame.
- [ ] fmn_game_load_map(): Try to get the correct hero position, right now it's reading randomly from the sprite list. (doesn't matter whether before or after the transition).
- [x] See paper notes from GDEX. Lots of bugs and improvements.
- [ ] Tree-shake resources when packing.
- [ ] MacOS: Build two independent app bundles. Demo and Full.
- [ ] genioc: Moved CPU load reporting to bigpc, we can drop it from genioc I think.
- [ ] Dismissing Hello menu (bigpc and web), we assume to switch to song 3. That won't be true forever. Use the map's song.
- [ ] macos (on iMac only), crash on quit. Can we cause it to close the window instead? The implicit quit on window close doesn't crash.
- [ ] macos (on iMac only), glViewport needs to scale by NSScreen.backingScale.
- [ ] inmgr: Real input mapping.
- [ ] InputManager.js: Must handle unconfigured devices connected before launching config modal.
- [ ] UI for saved game management.
- [ ] stdsyn. Or drop it, maybe minsyn is adequate?
- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [ ] Delete maps (and resources in general)
- [ ] Build-time support for enums and such? Thinking FMN_SPRITE_STYLE_ especially, it's a pain to add one.
- [ ] Rekajigger Constants.js, use actual inlinable constants.
- [ ] Map dark, indoors, blowback... Can we make a "map flags" field to contain these?
- [ ] verify: analyze map songs, ensure no map could have a different song depending on entry point
- [ ] verify: Check neighbor edges cell by cell (be mindful of firewall). Open no-neighbor edges must have blowback.
- [ ] verify: Tile 0x0f must be UNSHOVELLABLE in all tile sheets.
- [ ] verify: Song channel 14 reserved for violin, songs must not use.
- [ ] verify: Teleport targets must set song.
- [ ] verify: Firewall must be on edge, with at least two vacant cells, and a solid on both ends.
- [ ] verify: Chalk duplicates
- [ ] verify: Resources named by sprite config, eg chalkguard strings
- [ ] verify: 'indoors' should be the same for all edge neighbors, should change only when passing thru a door
- [ ] verify: buried_treasure and buried_door. shovellable, etc
- [ ] verify: Map flag commands eg ANCILLARY, also "sketch" important, must come before sprites and doors.
- [ ] verify: Map tilesheet must be before neighbors, for crow's edge detection.
- [ ] Remove hard-coded teleport targets, store in the archive (fmn_spell_cast).
- [ ] Summoning the crow potentially examines every command in every map, every time. Can we pre-index all that by itemid?
- [ ] Maps for full game.
- [ ] Pumpkin protection: Ensure she can't travel too far as a pumpkin, and can't get trapped in the reachable area.
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls. Currently 1..50 ms per Clock.js
- [ ] Filter resources by qualifier, see src/tool/mkdata/packArchive.js
- [ ] Translation.
- [ ] Touch input.
- [ ] Input configuration.
- - [ ] UI for mapping.
- [ ] Extra mappable input actions, eg hard pause and fullscreen toggle. I specifically don't want these for the demo, so punt.
- [ ] Consider WebGL for rendering. CanvasRenderingContext2D is not performing to my hopes.
- - 2023-05-07: A quick test (no actual rendering, just clear the framebuffer), and WebGL is no better.
- [ ] Unit tests.
- - [ ] General-purpose test runner.
- - [ ] C tests
- - [ ] JS platform tests
- [ ] Automation against headless native build.
- [ ] Windows

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
