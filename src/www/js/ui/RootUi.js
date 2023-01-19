/* RootUi.js
 * Top level view controller.
 */
 
import { Dom } from "../util/Dom.js";
import { HeaderUi } from "./HeaderUi.js";
import { GameUi } from "./GameUi.js";
import { FooterUi } from "./FooterUi.js";
import { ErrorModal } from "./ErrorModal.js";
import { Runtime } from "../game/Runtime.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom, Window, Runtime];
  }
  constructor(element, dom, window, runtime) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.runtime = runtime;
    
    this.header = null;
    this.footer = null;
    this.game = null;
    
    this.buildUi();
    
    this.reset();
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
}
