/* RootUi.js
 * Top level view controller.
 */
 
import { Dom } from "../util/Dom.js";
import { HeaderUi } from "./HeaderUi.js";
import { GameUi } from "./GameUi.js";
import { FooterUi } from "./FooterUi.js";
import { ErrorModal } from "./ErrorModal.js";
import { Runtime } from "../game/Runtime.js";
import { DataService } from "../game/DataService.js";
import { InputConfigurationModal } from "./InputConfigurationModal.js";

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
    
    this.header = null;
    this.footer = null;
    this.game = null;
    this.ready = false;
    
    this.buildUi();
    
    /* If either DataService or WasmLoader fails during its preload, we should report it immediately.
     */
    this.runtime.preloadAtConstruction
      .then(() => this.dataLoaded())
      .catch(e => this.onError(e));
      
    this.runtime.onError = e => this.onError(e);
    this.runtime.onForcedPause = () => this.onForcedPause();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    this.header = this.dom.spawnController(this.element, HeaderUi);
    this.game = this.dom.spawnController(this.element, GameUi);
    this.footer = this.dom.spawnController(this.element, FooterUi);
    
    this.header.onReset = () => this.reset();
    this.header.onFullscreen = () => this.game.enterFullscreen();
    this.header.onPause = () => {
      this.game.setRunning(false);
      this.runtime.pause();
    };
    this.header.onResume = () => {
      this.game.setRunning(true);
      this.runtime.resume();
    };
    this.header.onConfigureInput = () => this.onConfigureInput();
    this.header.onDebugPause = () => this.runtime.debugPauseToggle();
    this.header.onDebugStep = () => this.runtime.debugStep();
    if (this.ready) this.header.setReady(true);
    
    this.runtime.setRenderTarget(this.game.getCanvas());
  }
  
  reset() {
    this.header.reset();
    this.header.blurResetButton(); // debatable
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
    this.header.setPaused(true);
  }
  
  dataLoaded() {
    this.ready = true;
    this.header.setReady(true);
  }
  
  onConfigureInput() {
    this.game.setRunning(false);
    this.runtime.pause();
    this.header.setPaused(true);
    const modal = this.dom.spawnModal(InputConfigurationModal);
  }
}
