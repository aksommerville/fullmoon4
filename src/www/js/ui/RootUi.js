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
    
    /* We don't really interact with DataService, but we do create an initial delay, waiting for user input.
     * It's crazy to not load all the data during that delay.
     */
    this.dataService.load()
      .then(() => this.dataLoaded())
      .catch(e => this.onError(e));
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    this.header = this.dom.spawnController(this.element, HeaderUi);
    this.game = this.dom.spawnController(this.element, GameUi);
    this.footer = this.dom.spawnController(this.element, FooterUi);
    
    this.header.onReset = () => this.reset();
    this.header.onFullscreen = () => this.game.enterFullscreen();
    this.header.onPause = () => this.runtime.pause();
    this.header.onResume = () => this.runtime.resume();
    this.header.onDebugPause = () => this.runtime.debugPauseToggle();
    this.header.onDebugStep = () => this.runtime.debugStep();
    if (this.ready) this.header.setReady(true);
    
    this.runtime.setRenderTarget(this.game.getCanvas());
  }
  
  reset() {
    this.header.reset();
    this.runtime.reset()
      .then(() => this.onLoaded())
      .catch(e => this.onError(e));
  }
  
  onLoaded() {
  }
  
  onError(e) {
    console.log(`RootUi.onError`, e);
    const modal = this.dom.spawnModal(ErrorModal);
    modal.setup(e);
  }
  
  dataLoaded() {
    this.ready = true;
    this.header.setReady(true);
  }
}
