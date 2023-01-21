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

### February

- [ ] Items.
- [ ] Synthesizer.
- [ ] Gamepad input.
- [ ] UI for saved game management.

### March

- [ ] Hazards.
- [ ] Spells.
- [ ] Music. We don't need every song yet, just like 3 or so.

### April

- [ ] Graphics.
- [ ] Maps for demo.
- [ ] Pretty up the public web page.

### After GDEX

- [ ] Can we reduce the wasm exports? There's only like 3 symbols that actually need exported.
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
