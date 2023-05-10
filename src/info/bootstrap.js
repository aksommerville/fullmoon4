import { Injector } from "./js/util/Injector.js";
import { Dom } from "./js/util/Dom.js";
import { HomeTabber } from "./js/ui/HomeTabber.js";

window.addEventListener("load", () => {
  const injector = new Injector(window);
  const dom = injector.getInstance(Dom);
  const homeTabber = dom.spawnController(dom.document.body.querySelector(".insert-content-here"), HomeTabber);
});
