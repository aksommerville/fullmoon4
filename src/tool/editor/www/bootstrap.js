import { Injector } from "/js/util/Injector.js";
import { Dom } from "/js/util/Dom.js";
import { RootUi } from "/js/ui/RootUi.js";
import { ResService } from "/js/util/ResService.js";

window.addEventListener("load", () => {
  const injector = new Injector(window);
  const dom = injector.getInstance(Dom);
  const resService = injector.getInstance(ResService);
  
  /* It's probably OK to skip this initial wait-for-load, we're pretty reactive all around.
   * But I figure initial load is a time when the user is OK with a little delay,
   * and having everything loaded before the main app even starts is good for my mental health.
   */
  document.body.innerHTML = "";
  resService.whenLoaded().then(() => {
    const root = dom.spawnController(document.body, RootUi);
  }).catch(error => {
    console.error(error);
    if ((error instanceof Error) && error.message) error = error.message;
    document.body.innerText = `Error loading resources\n${JSON.stringify(error)}`;
  });
});
