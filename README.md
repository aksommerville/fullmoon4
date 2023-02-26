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

- [x] Cave graphics: ladder, entrance, water
- [x] Village graphics: huts, fountain, fences
- [x] Illumination
- [x] Cheese wind-down sound and visual feedback
- [x] Fire nozzle
- [x] Treadle-n-gate
- [x] Quickie touch input, just for verification on mobile.
- [x] Music timing is not tolerable. It's worse on mobile than desktop. Find a better way.
- - duh! We can schedule note starts in the future, they don't have to be at (context.currentTime).
- - [x] Reproduce the problem reliably.
- - Oh hey, we were also failing to apply sound effect velocity as level. oops
- - [x] Occasional loud notes, what the hell...
- - - Not the same notes every time. Eye of Newt rhythm guitar does it reliably (due to so many notes).
- - - Due to reading (gain.value) in the same pass where we set it with setValueAtTime -- it hadn't been committed yet.
- - [x] Still hearing noise sometimes on note releases. Most apparent in Tangled Vine.
- - - [x] I bet this is due to attack and release ramps overlapping. Confirm.
- - - - That does happen (esp Eye of Newt), but it's not most of the faulty notes.
- - - Important to stake at the predetermined sustain level, not node.gain.value. (it might not be there yet)
- - [x] Short notes are now cutting off a little funny.
- - - [x] Toil and Trouble
- - - [x] Eye of Newt, i think i'm hearing it in the bass. 
- - xxx Eye of Newt, bump up drums
- - xxx Tangled Vine, ''
- - [x] No, dipshit! Drum levels are low because you deferred their gain but not their start time.
- [x] Round-off jitter has returned: Try walking east into the west octopus tree in the eye-of-newt map.
- - ...both octopus trees actually, which happen to be at x=8 and x=16.
- [x] treasure: Sound when picking up an already-have item
- [x] treadle: sounds
- [x] Need a better solution for shovel holes. eg dig in the packed earth in the village, it shows a grass outline
- - Making it work on disparate backgrounds, we'd need some external list of holes. It can't be done directly on the map or cellphysics.
- - For now, I'll just forbid digging on the packed earth.

### March

- [ ] Spells.
- [ ] Refine items.
- - [x] Corn: Eliminate. Same thing as Seed.
- - [ ] Seed: Sprite on actuation, and summon the bird or create a plant.
- - [ ] Feather: Actuate blocks and ...whatever else a feather does.
- - [ ] Coin: throw
- - [ ] Pitcher: pickup from environment
- - [ ] Pitcher: advance plants when poured. And put out fires. and what else?
- - [ ] Pitcher: visual feedback on pickup
- - [ ] Compass: How to decide on target location? see src/app/fmn_secrets.c
- - [ ] Violin: Chart UI
- - [ ] Violin: Encode song
- - [ ] Shovel: Needs a new face tile, show some effort.
- [ ] Sprites:
- - [ ] Sawblades like v1
- - [ ] Alphablocks
- - [ ] Firewall
- - [ ] Chalk-aware sentinel
- - [ ] Raccoon, throws acorns
- - [ ] Rat, pounce and bite. He'll miss if you're flying.

### April

- [ ] Graphics.
- [ ] Maps for demo.
- [ ] Map gamepads necessary for demo (mind that they will be different per browser and os).
- [ ] Ensure audio doesn't get stuck on after an exception. like, actually handle exceptions nicely.
- [ ] Persist plants and sketches across map loads.
- [ ] UI for saved game management.
- [x] Physics flag to control whether chalk is allowed? Consider the black octopus tree, chalk looks weird like it's floating in air.
- [ ] Fine-tune song endings so they loop seamlessly.

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
- [ ] Make up a new item to take Corn's place.
- [ ] Maps for full game.
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls. Currently 1..50 ms per Clock.js
- [ ] Detect heavy drops in requestAnimationFrame rate and stop music if too low.
- [ ] Rekajigger Constants.js, use actual inlinable constants.
- [ ] Filter resources by qualifier, see src/tool/mkdata/packArchive.js
- [ ] Translation.
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
