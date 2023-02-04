# Full Moon

Fourth time's the charm!

Target platform is web browsers only.
There will be a "platform" layer in Javascript, and an "app" layer in C compiled to WebAssembly.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

### January

- [x] Hero physics and animation.
- [x] If you drop the dpad during a door transition, she should stop moving. As is, you get the deceleration tail on the far end.
- [x] Serve on the public web.
- [x] Sprite data format.
- [x] Occasional jitter in sprite collisions, seems to happen only at specific places. Pushblocks on map 1 expose it.
- - Push a pushblock east or south when it's in position 4 or 8. Not 12 or 16, and never west or north.

### February

- [ ] Items.
- [ ] Synthesizer.
- - [x] Channel volume
- - [ ] Pitch wheel
- - [ ] Sound effects -- I'm thinking a new platform hook, just fmn_play_sound(id) or so
- - [ ] Do we need percussion? How is that going to work?
- - [x] Tooling to use midevil and edit instrument definitions on the fly.
- - - [x] midevil: Output to WebSocket
- - - [x] Synthesizer: Receive MIDI events via WebSocket
- - - [x] Alternate audio-only web app
- - - [x] Node WebSocket server to shovel MIDI between web apps.
- - - - [x] Also notice changes to instruments and recompile as needed.
- - [ ] Change song per map header
- [ ] Gamepad input.
- [ ] UI for saved game management.

### March

- [x] Hazards.
- [ ] Spells.
- [ ] Music. We don't need every song yet, just like 3 or so.
- [ ] Preprocess MIDI files, eg removing Logic's trailing delays.
- - Maybe not a big deal? We're going to touch up with midevil in any case.

### April

- [ ] Graphics.
- [ ] Maps for demo.
- [ ] Pretty up the public web page.

### After GDEX

- [x] Can we reduce the wasm exports? There's only like 3 symbols that actually need exported.
- [ ] Touch input.
- [ ] Input configuration.
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
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls.
- [ ] Filter resources by qualifier, see src/tool/mkdata/packArchive.js
