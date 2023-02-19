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

- [ ] Music. We don't need every song yet, just like 3 or so.
- - [x] Seven Circles of a Witch's Soul
- - - [ ] Do something more interesting with the bassline, geez this is boring.
- - [x] Eye of Newt
- - - [ ] Cleaner lead, and less distorted during the twirly half.
- - - [ ] In the chromatic circling, use an instrument we can reduce the attack for all but first note.
- - [ ] Blood for Silver
- [ ] Make up a new item to take Corn's place.
- [x] Return to map entry point if injured while another injury in progress. No more ping-ponging between hazards.
- [ ] `make verify` or something, to auto-validate resources:
- - [ ] Song must start with a Program Change for each channel before any notes.
- - [ ] Song events should be tracked by channel.
- - [ ] Map neighbors, song, tilesheet.
- - [ ] Sprite controller, tilesheet.
- - [ ] No unused resources.
- - [ ] Image dimensions and colorspace.
- [x] Able to skip thru left wall in the Seven-Circles zone, with cheese+broom. Limit velocity.
- - [x] Devise some kind of assurance that final velocity is never more than 0.5 m/frame, whatever a frame is.
- [x] Resume broom over hole. Rejects due to "action in progress". Can we let it thru?

### March

- [ ] Spells.
- [ ] Refine items.
- - [ ] Corn: Eliminate. Same thing as Seed.
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

### April

- [ ] Graphics.
- [ ] Maps for demo.
- [ ] Coloring book, violin pic: Make the hat less dicky.
- [ ] Map gamepads necessary for demo (mind that they will be different per browser and os).
- [ ] Ensure audio doesn't get stuck on after an exception. like, actually handle exceptions nicely.
- [ ] Persist plants and sketches across map loads.
- [ ] UI for saved game management.

### May

- [ ] Order merch.
- [ ] Beta test.
- [ ] Pretty up the public web page.

### After GDEX

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
