# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Target platform is web browsers only.
There will be a "platform" layer in Javascript, and an "app" layer in C compiled to WebAssembly.

^ meaning "*initial* target platform". I am definitely making a Linux-native version, and yknow, as many others as we can.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

### February

### March

- [x] Spells.
- - [x] Rain
- - [x] Wind
- - [x] Slow motion
- - [x] Teleport home
- - [x] 4 or 5 other teleport spells? Depends what the world looks like.
- - [x] Opening
- - [x] Invisibility
- - [x] (song) Reveille
- - [x] (song) Lullabye
- - [x] (song) Bloom
- - [x] (song) Revelations
- - [x] (song) Come Hither
- - [x] (song) Go Thither
- [x] Refine items.
- - [x] Seed: Summon a bird.
- - [x] Pitcher: advance plants when poured
- - [x] Shovel: Needs a new face tile, show some effort.
- - [x] Violin: Should linger a bit after completion, and I think nix the input so she doesn't start walking.
- [ ] Sprites:
- - [x] Sawblades like v1
- - [ ] Firewall
- - [ ] Chalk-aware sentinel
- - [ ] Raccoon, throws acorns
- - [ ] Rat, pounce and bite. He'll miss if you're flying.
- - [ ] Rabbit that teaches you a song.
- - [ ] Farmer that plants and waters a seed.
- [ ] Bloom plants on a timer.
- [ ] Move the collect-item logic somewhere shareable, it's already duped in a few places.
- [ ] Prevent wind and rain from happening indoors.
- [ ] Initial wind per map.
- [ ] Compass: How to decide on target location? see src/app/fmn_secrets.c

### April

- [ ] Graphics.
- [ ] Maps for demo.
- [ ] Map gamepads necessary for demo (mind that they will be different per browser and os).
- [ ] Ensure audio doesn't get stuck on after an exception. like, actually handle exceptions nicely.
- [ ] Persist plants and sketches across map loads.
- [ ] UI for saved game management.
- [ ] Fine-tune song endings so they loop seamlessly.
- [ ] Chrome Linux, I get 1-pixel artifacts when scaled up. Visible when the violin chart is up.
- - Would it help to scale up only by integers? That's probably a good idea in any case.
- - We're using straight `object-fit:contain` on a canvas that fills the entire available space. Will be tricky.
- [ ] Finalize sound effects.
- [ ] Bonfire: Fizzle and smoke when extinguished.

### May

- [ ] Order merch.
- - [ ] Coloring book. (printingcenterusa.com says about $1/ea for 250)
- - [ ] Stickers and pins: Stickermule (they do t-shirts too but too expensive)
- - [ ] T-shirts. (Bolt is cheapest I've found, and they did good with Plunder Squad. UberPrints looks ok but more expensive)
- - [ ] Notebooks
- - [ ] Pencils
- - Probly no need for thumb drives if we've only got the demo.
- [ ] Beta test.
- [ ] Pretty up the public web page.
- [ ] Lively intro splash.
- [ ] Bonus secret 2-player mode: Second player is a ghost that can make wind.

### After GDEX

- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [ ] MapAllUi: Populate tattle.
- - [ ] Delete maps (and resources in general)
- - [ ] SpriteUi: Helper to browse for image and tile. (eg ImageAllUi in a modal)
- - [ ] Helper to analyze map songs, ensure no map could have a different song depending on entry point
- - - Consider using `verify` instead of `editor` for this.
- [ ] Build-time support for enums and such? Thinking FMN_SPRITE_STYLE_ especially, it's a pain to add one.
- [ ] ^ or if not all that, we at least need a repository of named `gs` fields.
- [ ] verify: Check neighbor edges cell by cell
- [ ] verify: Tile 0x0f must be UNSHOVELLABLE in all tile sheets.
- [ ] verify: Song channel 14 reserved for violin, songs must not use.
- [ ] verify: Teleport targets must set song.
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
- [ ] Move Map decoding out of DataService into a class of its own.
- [ ] Unit tests.
- - [ ] General-purpose test runner.
- - [ ] C tests
- - [ ] JS platform tests
- [ ] Automation against headless native build.
- [ ] Other platforms:
- - [ ] Web wrapper eg Electron
- - [ ] Linux
- - [ ] Tiny
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
- - [ ] SNES? I know it won't happen but you got to try
- - [ ] node or deno? Shame to write all this Javascript and use it for just one platform...

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
