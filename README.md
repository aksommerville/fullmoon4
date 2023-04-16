# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

And tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### April

- [x] Linux DRM platform, why not.
- - [x] minsyn
- [ ] After that works, try for MacOS too. Web works on the MacBook but it's a strain.
- [ ] Handle gamepad hats. eg Black-on-black gamepad on the MacBook.
- [ ] Web and gl2 renderers: Can we draw the hero on top, after pan transitions? So she's not cut in half.
- [x] bigpc: Stop music while violin up
- [x] On Pi, I'm hearing timing flaws in the songs, enough to be a distraction. Problem with midi_file?
- [ ] Try moving the desert entrance south to mitigate the fast-switch of songs.
- [x] raspi: Charms in treasure menu aren't rotating left of center
- [x] raspi: Feather down, it's using the wrong transform or something.
- - Using BCM hacks on the nuc, I do get the feather thing but do not get the charms things.
- [ ] Drop the BCM hacks condition, we can do it the bcm way everywhere.
- [x] Make each song use its own instruments, no more sharing.
- [x] minsyn: Are notes playing an octave high or something? Sounds much higher than in WebAudio.
- [x] And sound effects are low. The hell? ...oops, denormalized rates backward from 64k.
- [ ] gl2: Fade out at game end
- [ ] minpc+web: Stop music at the moment the werewolf dies.
- [x] minsyn, notes aborting or something? it's noticeable in Eye of Newt.
- - There's a Meta End of Track right there, and the effect moves when I move the event.
- - Meta 0xff, we read that as real-time System Reset, oops.
- [ ] Give the rabbit a pond.
- [ ] bigpc: map analysis (crow)
- [x] bigpc: Suppress duplicate sound effects (eg bell, first stroke is noticeably louder)

### May

- [ ] Trick floor: Would it help to gently suck the hero into the center of each tile? to prevent overstepping, i mean
- [ ] Raccoon mind control: Don't wait for the decision cycle, make it immediate.
- [ ] Order merch.
- - [ ] Stickers and pins: Stickermule (they do t-shirts too but too expensive)
- - [ ] T-shirts. (Bolt is cheapest I've found, and they did good with Plunder Squad. UberPrints looks ok but more expensive)
- - [ ] Baseball cards: Get in touch with Lucas, maybe we can do a full-GDEX set of cards? Emailed 2023-03-11. Gotprint.com: $35/250. 875x1225px
- - Probly no need for thumb drives if we've only got the demo.
- - [ ] Big banner. Try sewing this instead of buying one.
- - [ ] Stuffed werewolf.
- - [ ] Feathers with sharp pins at the tip so we can impale the werewolf with them.
- [ ] Beta test.
- [ ] Pretty up the public web page.
- [ ] Lively intro splash.
- [ ] Bonus secret 2-player mode: Second player is a ghost that can make wind.
- [ ] Correct song and sound effect levels. Automated analysis?
- [ ] Track travel. Use for crow guidance, and report coverage at the end. Flag maps eg border as "not participating in coverage".
- - Or use a "set gsbit" map command, for selected maps only? Coverage reporting is not a big priority.
- [ ] Make up a new item to take Corn's place, and one for the zero slot. Fill all 16 slots. (mind that item zero comes into play then)
- - [ ] Membership Hat: Certain monsters won't attack while you're wearing it, but it's an item so you can't use anything else.
- [ ] Two sets of map resource: Demo and Full. I think other resources don't matter, let tree-shaking handle it.
- [ ] Tree-shake resources when packing.

### After GDEX

- [ ] inmgr: Real input mapping.
- [ ] UI for saved game management.
- [ ] Building resources is too slow. Rewrite all those Node tools in C.
- [ ] Soft render.
- [ ] stdsyn. Or drop it, maybe minsyn is adequate?
- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [ ] MapAllUi: Populate tattle.
- - [ ] Delete maps (and resources in general)
- - [ ] ImageAllUi: Show names
- [ ] Build-time support for enums and such? Thinking FMN_SPRITE_STYLE_ especially, it's a pain to add one.
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
- [ ] Remove hard-coded teleport targets, store in the archive (fmn_spell_cast).
- [ ] Maps for full game.
- [ ] Pumpkin protection: Ensure she can't travel too far as a pumpkin, and can't get trapped in the reachable area.
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls. Currently 1..50 ms per Clock.js
- [ ] Rekajigger Constants.js, use actual inlinable constants.
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
- - [ ] Tiny. I don't think it will work, but prove it.
- - [ ] MacOS
- - [ ] Windows
- - [ ] iOS
- - [ ] Android
- - [ ] Pippin
- - [ ] PicoSystem
- - [ ] Thumby
- - [ ] PocketStar
- - [ ] Playdate
- - [ ] 68k Mac
- - [ ] Wii
- - [ ] Modern consoles?
- - [ ] node or deno? Shame to write all this Javascript and use it for just one platform...

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
