# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

And tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### May

- [x] Beta test.
- [ ] Track travel. Use for crow guidance, and report coverage at the end. Flag maps eg border as "not participating in coverage".
- - Or use a "set gsbit" map command, for selected maps only? Coverage reporting is not a big priority.
- [ ] Scripts to analyze logs.
- [ ] Metal
- [x] editor:ImageAllUi: Show names.
- [ ] Can I make rubber trees that you can squeeze between them by pushing?
- [x] Westward blowback seems slower than eastward, or is that my imagination? ...imagination
- [ ] minsyn drums too loud
- [ ] pushblock: Don't make a sound if we can tell we won't be moving.
- [ ] wildflower: Option to flip horizontally
- [ ] Native fiddle tool.
- - [x] http: WebSocket
- - [ ] inotify helper
- - [x] Report peak and RMS continuously over WebSocket.
- - [x] MIDI-In. Manage at the web app, and call `POST /api/midi`.
- - [ ] MIDI-In and PCM/synth config at command line? Would be cool to have some functionality without the web app.
- - [ ] Broad support for level analysis -- I'd like to replace `assist` with this.
- - [ ] Show me which instruments are used by each song.
- - [ ] Possible to populate the instruments menu with the selected program on channel 14?
- [x] web: Handle high-frequency monitors. We use requestAnimationFrame but then assume 60 Hz.
- [x] Disable shovel underlay while pumpkinned.
- [x] Moonsong's chalkboard should be chalkable, make it work.
- [x] Gravestones too, why aren't they?
- [ ] Can we move the rabbit? He teaches a song but users will not have the violin at that point.
- [x] Put some pushblocks near the snow globe, to show it off.
- [x] Beach SW, the rain spell educator looks like it has something to do with the lambda block but it doesn't.
- [ ] Is the panda too distracting, at Seamonster Pong?
- [ ] Playing first_frost via fiddle, with screwing around on ch 14, the bells timing is off.
- [ ] Chalk alphabet education.
- [ ] chalkguard: fireworks
- [ ] "Bring matches" watchduck at the cave entrance, demo.
- [x] Village transmogrification platform is shovellable; shouldn't be.
- [x] Lambda block: Some kind of visual change while attracted, so you can tell something happened, when you're blocking its motion.

### Full Maps

- [ ] Village/forest boundary, need big tree on the village side.
- [ ] Furnish Dot's house.
- [ ] Big tree edges don't line up.
- [ ] image castleext: Water-into-trees, 4 tiles needed.
- [x] Decide where each treasure goes. Filling in with Cheese.
- [ ] desert: tiles for forest boundary
- [ ] image steppe: Entirely temporary
- [ ] steppe/beach boundary in blowback border
- [ ] Sea should be the same color everywhere, pick one.
- [ ] Church: join path at welcome mat.
- [x] village: wattle-and-daub houses need an extensible roof

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

- [ ] Tree-shake resources when packing.
- [ ] MacOS: Build two independent app bundles. Demo and Full.
- [ ] MacOS: App icon
- [ ] linux: PulseAudio
- [x] Eliminate transparency from map images, incorporate the background in each tile.
- [ ] Dismissing Hello menu (bigpc and web), we assume to switch to song 3. That won't be true forever. Use the map's song.
- [ ] macos (on iMac only), crash on quit. Can we cause it to close the window instead? The implicit quit on window close doesn't crash.
- [ ] macos (on iMac only), glViewport needs to scale by NSScreen.backingScale.
- [ ] inmgr: Real input mapping.
- [ ] InputManager.js: Must handle unconfigured devices connected before launching config modal.
- [ ] UI for saved game management.
- [ ] Soft render.
- [ ] stdsyn. Or drop it, maybe minsyn is adequate?
- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [x] MapAllUi: Populate tattle.
- - [ ] Delete maps (and resources in general)
- - [ ] ImageAllUi: Show names
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
- [ ] verify: Map flag commands eg ANCILLARY must come before sprites and doors.
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
- [ ] Other platforms:
- - [ ] Web wrapper eg Electron
- - [ ] Windows
- - [ ] Pippin. Doubtful but I have to know.
- - [ ] PicoSystem
- - [ ] Thumby
- - [ ] PocketStar
- - [ ] Playdate
- - [ ] Wii
- - [ ] Modern consoles?

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
