/* RootUi.js
 * Top level view controller.
 */
 
import { Dom } from "../util/Dom.js";
import { GameUi } from "./GameUi.js";
import { MenuUi } from "./MenuUi.js";
import { ErrorModal } from "./ErrorModal.js";
import { Runtime } from "../game/Runtime.js";
import { DataService } from "../game/DataService.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom, Window, Runtime, DataService];
  }
  constructor(element, dom, window, runtime, dataService) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.runtime = runtime;
    this.dataService = dataService;
    
    this.menu = null;
    this.game = null;
    this.ready = false;
    
    this.buildUi();
    
    this.element.addEventListener("click", (e) => this.onClick(e));
    
    /* If either DataService or WasmLoader fails during its preload, we should report it immediately.
     */
    this.runtime.preloadAtConstruction
      .then(() => this.reset())
      .catch(e => this.onError(e));
      
    this.runtime.onError = e => this.onError(e);
    this.runtime.onForcedPause = () => this.onForcedPause();
    this.runtime.requestFullscreen = (state) => this.enterFullscreen(state);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.game = this.dom.spawnController(this.element, GameUi);
    this.menu = this.dom.spawnController(this.element, MenuUi);
    this.menu.onfullscreen = () => this.enterFullscreen(true);
    this.menu.ondismiss = () => this.dismissMenu();
    this.runtime.setRenderTarget(this.game.getCanvas());
  }
  
  reset() {
    this.runtime.reset()
      .then(() => this.onLoaded())
      .catch(e => this.onError(e));
  }
  
  onLoaded() {
    this.game.setRunning(true);
  }
  
  onError(e) {
    this.game.setRunning(false);
    console.error(`RootUi.onError`, e);
    const modal = this.dom.spawnModal(ErrorModal);
    modal.setup(e);
  }
  
  onForcedPause() {
    // This event comes from Runtime, so we don't notify them.
    this.game.setRunning(false);
  }
  
  dataLoaded() {
    this.ready = true;
  }
  
  onClick(e) {
    if (this.runtime.running) {
      this.game.setRunning(false);
      this.runtime.pause();
      this.menu.show(true);
    } else {
      this.game.setRunning(true);
      this.runtime.resume();
      this.menu.show(false);
    }
  }
  
  dismissMenu() {
    this.game.setRunning(true);
    this.runtime.resume();
    this.menu.show(false);
  }
  
  enterFullscreen(state) {
    if (state) {
      this.dom.document.body.requestFullscreen().then(
        () => this.runtime.setFullscreen(true)
      ).catch(
        err => console.error(err)
      );
    } else {
      this.dom.document.exitFullscreen().then(
        () => this.runtime.setFullscreen(false)
      ).catch(
        err => console.error(err)
      );
    }
  }
}
