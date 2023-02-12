/* HeaderUi.js
 * Global header bar.
 */
 
import { Dom } from "../util/Dom.js";

export class HeaderUi {
  static getDependencies() {
    return [HTMLElement, Dom, "discriminator"];
  }
  constructor(element, dom, discriminator) {
    this.element = element;
    this.dom = dom;
    this.discriminator = discriminator;
    
    this.onReset = () => {};
    this.onFullscreen = () => {};
    this.onPause = () => {};
    this.onResume = () => {};
    this.onDebugPause = () => {}; // toggle
    this.onDebugStep = () => {};
    
    this.ready = false;
    
    this.buildUi();
  }
  
  setReady(ready) {
    if (!!ready === !!this.ready) return;
    this.ready = !!ready;
    for (const element of this.element.querySelectorAll("input")) {
      element.disabled = !this.ready;
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "Reset", disabled: "disabled", "on-click": () => this.onReset() });
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "Fullscreen", disabled: "disabled", "on-click": () => this.onFullscreen() });
    const pauseId = `HeaderUi-${this.discriminator}-pause`;
    this.dom.spawn(this.element, "INPUT", ["toggle", "pause"], { type: "checkbox", id: pauseId, disabled: "disabled", "on-change": () => this.onPauseToggled() });
    this.dom.spawn(this.dom.spawn(this.element, "LABEL", { for: pauseId }), "DIV", "Pause");
    
    // Debug controls, not for production:
    this.dom.spawn(this.element, "DIV", ["spacer"]);
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "||", "on-click": () => this.onDebugPause() });
    this.dom.spawn(this.element, "INPUT", { type: "button", value: ">|", "on-click": () => this.onDebugStep() });
  }
  
  onPauseToggled() {
    const input = this.element.querySelector("input.pause");
    if (input.checked) {
      this.onPause();
    } else {
      this.onResume();
    }
  }
  
  reset() {
    this.element.querySelector("input.pause").checked = false;
  }
}
