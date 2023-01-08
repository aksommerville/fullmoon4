# Full Moon

Fourth time's the charm!

Target platform is web browsers only.
There will be a "platform" layer in Javascript, and an "app" layer in C compiled to WebAssembly.

## TODO

- [ ] Clock.js "hard" vs "soft" pause. Do we really need both?
- [ ] Consider WebGL for rendering. CanvasRenderingContext2D is not performing to my hopes.
- [ ] Native platform for Linux.
- [ ] Unit tests.
- [ ] Automation against headless native build.
- [ ] Synthesizer.
- [ ] Input configuration.
- [ ] Gamepad input.
- [ ] Touch input.
- [ ] UI for saved game management.
- [ ] Data editor.
- [ ] Can we reduce the wasm exports? There's only like 3 symbols that actually need exported.
- [ ] Graphics should live in src/data, not src/www. Punting for now because that complicates the dev server.
