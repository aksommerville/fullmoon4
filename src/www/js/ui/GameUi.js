/* GameUi.js
 * Variable-size container for the main canvas.
 */
 
import { Dom } from "../util/Dom.js";
import { InputManager } from "../game/InputManager.js";

export class GameUi {
  static getDependencies() {
    return [HTMLElement, Dom, InputManager];
  }
  constructor(element, dom, inputManager) {
    this.element = element;
    this.dom = dom;
    this.inputManager = inputManager;
    
    this.running = false;
    
    this.buildUi();
    
    this.inputManager.registerTouchListeners(this.element);
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
}
