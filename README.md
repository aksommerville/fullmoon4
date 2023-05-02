# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

And tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### 2023-04-29 render-redesign

- [x] Glue thru bigpc.
- [x] Client-side render logic. Mostly copy gl2, it's phrased similarly.
- [x] Weather.
- [x] Menus.
- [x] Violin.
- [x] Plants and sketches.
- [x] Chalk menu.
- [ ] gl2: upload image, deferring until I'm sure what it's for.
- [ ] Web glue.
- [ ] Web Renderer abstraction. Be ready for selectable CanvasRenderingContext2D/WebGL.
- [ ] bigpc cleanup -- remove the three "old API" renderer hooks.
- [ ] gl2 cleanup
- [ ] fmn_map_dirty() -- not the platform's problem anymore, remove it. Transition stuff too.
- - [ ] This was previously handling flower times and syncing plants and sketches. That needs to happen some other way now.
- [ ] !!! I'm rendering the "from" state of transitions during update, not render. Find a way to make that OK.
- [x] gl2 did a lot of decals upside-down due to y axis direction confusing. Don't force client to do that, account for it inside gl2.
- [x] ^ decal and recal are updated, review mintile and maxtile ...we never draw tiles from a framebuffer, that would be weird. no worries.
- [ ] Re-figure-out the idle warning, how to communicate it to client.
- [ ] fmn_render_map.c: plants and sketches
- [ ] Redesign sprites to interact more closely with render? Maybe not worth the effort, it's a lot of effort.
- [x] mintile are consistently showing up 1 pixel low of expected (see treasure chest on initial map, should butt against foot of trees. vertices leave render correctly, y==24)
- - ...it was the decal-flip thing, actually the *background* was in the wrong place.
- [ ] Verify sprites that weren't reachable in the first pass, see fmn_render_sprites.c
- [ ] Suspend updates during transition and menu.
- [ ] If we proceed as is, the game clock will start including transition and menu times. Is this OK? Try it and decide.
- [ ] Add a platform hook for changing song. Menus should use that.
- [ ] Deprecate fontosaur and implement text via mintile.

### May

- [ ] Werewolf turns left to face the hero, after she's dead.
- [ ] Beta test.
- [ ] Pretty up the public web page.
- [ ] Track travel. Use for crow guidance, and report coverage at the end. Flag maps eg border as "not participating in coverage".
- - Or use a "set gsbit" map command, for selected maps only? Coverage reporting is not a big priority.
- [ ] Two sets of map resource: Demo and Full. I think other resources don't matter, let tree-shaking handle it.
- [ ] Tree-shake resources when packing.
- [ ] Scripts to analyze logs.
- [ ] Is it too late to move rendering business into the client? I'm terrified of adding new render backends.

### After GDEX

- [ ] Eliminate transparency from map images, incorporate the background in each tile.
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
