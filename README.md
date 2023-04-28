# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

And tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### April

- [ ] Graphics touch-up.
- - [ ] Village interiors
- - [ ] Village exterior
- [x] gate: Don't blink when changing state immediately after map transition (eg moonsong's house)
- - This might mean forcing treadles to set zero state at unload.

### May

- [ ] Door turn-around. If you immediately reverse direction after passing thru a door with blocked edges (house doors, not ladders), you should re-enter it.
- - [ ] Can we effect that based on pressure against the solid cell?
- [ ] Beta test.
- [ ] Pretty up the public web page.
- [ ] Bonus secret 2-player mode: Second player is a ghost that can make wind.
- [ ] Track travel. Use for crow guidance, and report coverage at the end. Flag maps eg border as "not participating in coverage".
- - Or use a "set gsbit" map command, for selected maps only? Coverage reporting is not a big priority.
- [ ] Two sets of map resource: Demo and Full. I think other resources don't matter, let tree-shaking handle it.
- [ ] Tree-shake resources when packing.
- [ ] Scripts to analyze logs.
- [ ] Is it too late to move rendering business into the client? I'm terrified of adding new render backends.

### After GDEX

- [ ] Dismissing Hello menu (bigpc and web), we assume to switch to song 3. That won't be true forever. Use the map's song.
- [ ] macos (on iMac only), crash on quit. Can we cause it to close the window instead? The implicit quit on window close doesn't crash.
- [ ] macos (on iMac only), glViewport needs to scale by NSScreen.backingScale.
- [ ] inmgr: Real input mapping.
- [ ] InputManager.js: Must handle unconfigured devices connected before launching config modal.
- [ ] UI for saved game management.
- [x] Building resources is too slow. Rewrite all those Node tools in C.
- [ ] Soft render.
- [ ] stdsyn. Or drop it, maybe minsyn is adequate?
- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [ ] MapAllUi: Populate tattle.
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
- [ ] Web input: Key events are triggering the pause button when focussed. Why isn't it just space bar?
- [ ] Touch input.
- [ ] Input configuration.
- - [ ] UI for mapping.
- [ ] Extra mappable input actions, eg hard pause and fullscreen toggle. I specifically don't want these for the demo, so punt.
- [ ] Consider WebGL for rendering. CanvasRenderingContext2D is not performing to my hopes.
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
