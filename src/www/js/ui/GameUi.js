/* GameUi.js
 * Variable-size container for the main canvas.
 */
 
import { Dom } from "../util/Dom.js";
import { InputManager } from "../game/InputManager.js";
import { Constants } from "../game/Constants.js";

export class GameUi {
  static getDependencies() {
    return [HTMLElement, Dom, InputManager, Window, Constants];
  }
  constructor(element, dom, inputManager, window, constants) {
    this.element = element;
    this.dom = dom;
    this.inputManager = inputManager;
    this.window = window;
    this.constants = constants;
    
    this.running = false;
    
    this.resizeObserver = new this.window.ResizeObserver(e => this.onResize(e));
    this.resizeObserver.observe(this.element);
    
    this.buildUi();
    
    this.inputManager.registerTouchListeners(this.element);
  }
  
  onRemoveFromDom() {
    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const canvas = this.dom.spawn(this.element, "CANVAS", ["gameView"]);
  }
  
  // There is no corresponding "exitFullscreen"; browser takes care of it.
  enterFullscreen() {
    this.element.querySelector(".gameView").requestFullscreen();
  }
  
  getCanvas() {
    return this.element.querySelector(".gameView");
  }
  
  setRunning(running) {
    running = !!running;
    if (running === this.running) return;
    if (this.running = running) {
      this.element.querySelector(".gameView").classList.add("running");
    } else {
      this.element.querySelector(".gameView").classList.remove("running");
    }
  }
  
  onResize(events) {
    if (!events || (events.length < 1)) return;
    const event = events[events.length - 1];
    const fullw = event.contentRect.width;
    const fullh = event.contentRect.height;
    const fbw = this.constants.COLC * this.constants.TILESIZE;
    const fbh = this.constants.ROWC * this.constants.TILESIZE;
    
    // If anything is zero, get out.
    if (!fullw || !fullh || !fbw || !fbh) return;
    
    // Find the largest integer scale of (fb) that fits in (full).
    const scale = Math.min(Math.floor(fullw / fbw), Math.floor(fullh / fbh));
    
    // If the element is smaller than the framebuffer, that's kind of crazy, just stop doing things.
    if (scale < 1) return;
    
    const dstw = fbw * scale;
    const dsth = fbh * scale;
    const canvas = this.element.querySelector(".gameView");
    canvas.style.width = `${dstw}px`;
    canvas.style.height = `${dsth}px`;
  }
}
