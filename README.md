# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Target platform is web browsers only.
There will be a "platform" layer in Javascript, and an "app" layer in C compiled to WebAssembly.

^ meaning "*initial* target platform". I am definitely making a Linux-native version, and yknow, as many others as we can.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

And tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### March

- [ ] Ricochet missiles with the umbrella is fun. Do more with this.
- - [x] Influence ricochet direction according to collision position on the tangent axis.
- - [ ] Gag where monsters are trying to destroy something and you're trying to protect it.
- - xxx Anything that ejects a missile should get stunned when it gets sent back.
- - - [x] Or catch it! Raccoon should do this. "Oh it just wanted to play catch"
- - [ ] Use ricochet to hit a switch or something.
- - [x] Make her less likely to get smacked in the face. ("could it be the umbrella'd edge?" as opposed to "which edge?")

### April

- [ ] Compass and crow: How to decide on target location? see src/app/fmn_secrets.c
- [ ] Graphics.
- [ ] Maps for demo.
- [ ] Map gamepads necessary for demo (mind that they will be different per browser and os).
- [ ] UI for saved game management.
- [ ] Chrome Linux, I get 1-pixel artifacts when scaled up. Visible when the violin chart is up.
- - Would it help to scale up only by integers? That's probably a good idea in any case.
- - We're using straight `object-fit:contain` on a canvas that fills the entire available space. Will be tricky.
- [ ] Finalize sound effects.
- [ ] Hero's shadow when flying goes under the conveyor belts, but over would look more natural. Can anything be done?

### May

- [ ] Order merch.
- - [ ] Coloring book. (printingcenterusa.com says about $1/ea for 250)
- - [ ] Stickers and pins: Stickermule (they do t-shirts too but too expensive)
- - [ ] T-shirts. (Bolt is cheapest I've found, and they did good with Plunder Squad. UberPrints looks ok but more expensive)
- - [ ] Notebooks
- - [ ] Pencils
- - [ ] Baseball cards: Get in touch with Lucas, maybe we can do a full-GDEX set of cards? Emailed 2023-03-11. Gotprint.com: $35/250. 875x1225px
- - Probly no need for thumb drives if we've only got the demo.
- [ ] Beta test.
- [ ] Pretty up the public web page.
- [ ] Lively intro splash.
- [ ] Bonus secret 2-player mode: Second player is a ghost that can make wind.
- [ ] The Cheat Book. A nice looking "WARNING SPOILERS" notebook we can leave out for demo players.

### After GDEX

- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [ ] MapAllUi: Populate tattle.
- - [ ] Delete maps (and resources in general)
- [ ] Build-time support for enums and such? Thinking FMN_SPRITE_STYLE_ especially, it's a pain to add one.
- [ ] verify: analyze map songs, ensure no map could have a different song depending on entry point
- [ ] verify: Check neighbor edges cell by cell (be mindful of firewall)
- [ ] verify: Tile 0x0f must be UNSHOVELLABLE in all tile sheets.
- [ ] verify: Song channel 14 reserved for violin, songs must not use.
- [ ] verify: Teleport targets must set song.
- [ ] verify: Firewall must be on edge, with at least two vacant cells, and a solid on both ends.
- [ ] verify: Chalk duplicates
- [ ] verify: Resources named by sprite config, eg chalkguard strings
- [ ] verify: 'indoors' should be the same for all edge neighbors, should change only when passing thru a door
- [ ] Remove hard-coded teleport targets, store in the archive (fmn_spell_cast).
- [ ] Make up a new item to take Corn's place.
- [ ] Maps for full game.
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls. Currently 1..50 ms per Clock.js
- [ ] Detect heavy drops in requestAnimationFrame rate and stop music if too low.
- [ ] Rekajigger Constants.js, use actual inlinable constants.
- [ ] Filter resources by qualifier, see src/tool/mkdata/packArchive.js
- [ ] Translation.
- [ ] Web input: Key events are triggering the pause button when focussed. Why isn't it just space bar?
- [ ] Touch input.
- [ ] Input configuration.
- - [ ] Generalize mapping, see src/www/js/game/InputManager.js
- - [ ] Persist gamepad and keyboard mapping.
- - [ ] UI for mapping.
- [ ] Extra mappable input actions, eg hard pause and fullscreen toggle. I specifically don't want these for the demo, so punt.
- [ ] Finish everything by Halloween. (to this point in the list at least)
- [ ] Consider WebGL for rendering. CanvasRenderingContext2D is not performing to my hopes.
- [x] Move Map decoding out of DataService into a class of its own.
- [ ] Unit tests.
- - [ ] General-purpose test runner.
- - [ ] C tests
- - [ ] JS platform tests
- [ ] Automation against headless native build.
- [ ] Other platforms:
- - [ ] Web wrapper eg Electron
- - [ ] Linux
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
