# Full Moon

Fourth time's the charm!

Target platform is web browsers only.
There will be a "platform" layer in Javascript, and an "app" layer in C compiled to WebAssembly.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

### February

- [x] Item possession and quantity
- [ ] Injury must end the active item, and don't begin while injured.
- [ ] Suppress movement during certain item actions (eg pitcher)
- [ ] Synthesizer.
- - [ ] Pitch wheel
- - [ ] Sound effects -- I'm thinking a new platform hook, just fmn_play_sound(id) or so
- - [ ] Do we need percussion? How is that going to work?
- - [x] Change song per map header
- - [x] Defer song start until first interaction. ...forcing to click Reset first, i'm ok with that.
- - [x] Prevent rapid-fire sound effects, eg bouncing between two hazards.
- [ ] UI for saved game management.
- [ ] (A) to dismiss menu -- zap input state on resuming game.

### March

- [ ] Spells.
- [ ] Music. We don't need every song yet, just like 3 or so.
- [ ] Refine items.
- - [ ] Corn, seed: Decide whether these are the same thing. Sprite on actuation, and summon the bird or create a plant.
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
- [ ] Pretty up the public web page.
- [ ] Coloring book, violin pic: Make the hat less dicky.
- [ ] Map gamepads necessary for demo (mind that they will be different per browser and os).
- [ ] Ensure audio doesn't get stuck on after an exception. like, actually handle exceptions nicely.
- [ ] Persist plants and sketches across map loads.

### After GDEX

- [ ] Touch input.
- [ ] Input configuration.
- - [ ] Generalize mapping, see src/www/js/game/InputManager.js
- - [ ] Persist gamepad and keyboard mapping.
- - [ ] UI for mapping.
- [ ] Extra mappable input actions, eg hard pause and fullscreen toggle. I specifically don't want these for the demo, so punt.
- [ ] Clock.js "hard" vs "soft" pause. Do we really need both?
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
- - [ ] 68k Mac
- - [ ] iOS
- - [ ] Android
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
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls.
- [ ] Filter resources by qualifier, see src/tool/mkdata/packArchive.js
- [ ] Detect heavy drops in requestAnimationFrame rate and stop music if too low.
- [ ] Move Map decoding out of DataService into a class of its own.
- [ ] Can we pack CSS and favicon into index.html? Just to eliminate the initial HTTP calls.
