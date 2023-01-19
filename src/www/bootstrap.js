import { Injector } from "./js/util/Injector.js";
import { Dom } from "./js/util/Dom.js";
import { RootUi } from "./js/ui/RootUi.js";

window.addEventListener("load", () => {
  const injector = new Injector(window);
  const dom = injector.getInstance(Dom);
  document.body.innerHTML = "";
  const root = dom.spawnController(document.body, RootUi);
});
