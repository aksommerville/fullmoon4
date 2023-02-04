// Our server will do some trickery to serve /js/ from the game's own sources.
import { Injector } from "./js/util/Injector.js";
import { Dom } from "./js/util/Dom.js";
import { RootUi } from "./RootUi.js";

window.addEventListener("load", () => {
  const injector = new Injector(window);
  const dom = injector.getInstance(Dom);
  const root = dom.spawnController(document.body, RootUi);
  // Instantiate Synthesizer et al.
  // Load data.
  // Open a WebSocket connection to our server.
  // Implement the MIDI shovel in src/tool/server/main.js
  // Implement WebSocket output in Midevil.
});
