/* HeaderUi.js
 * Global header bar.
 */
 
import { Dom } from "/js/util/Dom.js";

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
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "Reset", "on-click": () => this.onReset() });
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "Fullscreen", "on-click": () => this.onFullscreen() });
    const pauseId = `HeaderUi-${this.discriminator}-pause`;
    this.dom.spawn(this.element, "INPUT", ["toggle", "pause"], { type: "checkbox", id: pauseId, "on-change": () => this.onPauseToggled() });
    this.dom.spawn(this.dom.spawn(this.element, "LABEL", { for: pauseId }), "DIV", "Pause");
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
