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

- [x] Sound effect for map-reset.
- [ ] Coloring book, violin pic: Make the hat less dicky.
- [ ] Fresh graphics
- - [ ] Combining trees, but larger than one tile.
- - [ ] Caves.
- - [ ] Village.
- [x] Enter a map over water, then get hurt twice to force a reset -- you get stuck toggling between the two maps.

### March

- [ ] Spells.
- [ ] Refine items.
- - [x] Corn: Eliminate. Same thing as Seed.
- - [ ] Seed: Sprite on actuation, and summon the bird or create a plant.
- - [ ] Cheese: Visual feedback. Maybe a second sound effect when it ends?
- - [ ] Feather: Actuate blocks and ...whatever else a feather does.
- - [ ] Coin: throw
- - [ ] Pitcher: pickup from environment
- - [ ] Pitcher: advance plants when poured. And put out fires. and what else?
- - [ ] Pitcher: visual feedback on pickup
- - [ ] Match: Use global illumination_time for something. Darkened caves etc.
- - [ ] Compass: How to decide on target location? see src/app/fmn_secrets.c
- - [ ] Violin: Chart UI
- - [ ] Violin: Encode song
- - [ ] Shovel: Needs a new face tile, show some effort.
- [ ] Sprites:
- - [ ] Sawblades like v1
- - [ ] Alphablocks
- - [ ] Firewall
- - [ ] Chalk-aware sentinel
- - [ ] Fire nozzle
- - [ ] Treadle-n-gate
- - [ ] Raccoon, throws acorns
- - [ ] Rat, pounce and bite. He'll miss if you're flying.

### April

- [ ] Graphics.
- [ ] Maps for demo.
- [ ] Map gamepads necessary for demo (mind that they will be different per browser and os).
- [ ] Ensure audio doesn't get stuck on after an exception. like, actually handle exceptions nicely.
- [ ] Persist plants and sketches across map loads.
- [ ] UI for saved game management.

### May

- [ ] Order merch.
- [ ] Beta test.
- [ ] Pretty up the public web page.
- [ ] Lively intro splash.
- [ ] Bonus secret 2-player mode: Second player is a ghost that can make wind.

### After GDEX

- [ ] Make up a new item to take Corn's place.
- [ ] Touch input.
- [ ] Input configuration.
- - [ ] Generalize mapping, see src/www/js/game/InputManager.js
- - [ ] Persist gamepad and keyboard mapping.
- - [ ] UI for mapping.
- [ ] Extra mappable input actions, eg hard pause and fullscreen toggle. I specifically don't want these for the demo, so punt.
- [ ] Consider WebGL for rendering. CanvasRenderingContext2D is not performing to my hopes.
- [ ] Package web app in a native wrapper eg Electron.
- [ ] Native platforms.
- - [ ] Linux
- - [ ] MacOS
- - [ ] Windows
- - [ ] Tiny
- - [ ] Pippin
- - [ ] PicoSystem
- - [ ] Thumby
- - [ ] PocketStar
- - [ ] Playdate
- - [ ] 68k Mac
- - [ ] iOS
- - [ ] Android
- - [ ] Wii
- - [ ] Modern consoles?
- - [ ] SNES? I know it won't happen but you got to try
- - [ ] node or deno? Shame to write all this Javascript and use it for just one platform...
- [ ] Unit tests.
- - [ ] General-purpose test runner.
- - [ ] C tests
- - [ ] JS platform tests
- [ ] Automation against headless native build.
- [ ] Maps for full game.
- [ ] Translation.
- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [ ] MapAllUi: Populate tattle.
- - [ ] Delete maps (and resources in general)
- - [ ] SpriteUi: Helper to browse for image and tile. (eg ImageAllUi in a modal)
- - [ ] Helper to analyze map songs, ensure no map could have a different song depending on entry point
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls. Currently 1..50 ms per Clock.js
- [ ] Filter resources by qualifier, see src/tool/mkdata/packArchive.js
- [ ] Detect heavy drops in requestAnimationFrame rate and stop music if too low.
- [ ] Move Map decoding out of DataService into a class of its own.
- [ ] Finish everything by Halloween.
- [ ] verify: Check neighbor edges cell by cell
