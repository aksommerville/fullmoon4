/* MapCommandsModal.js
 * Pastes the command tokens back together and lets you edit as plain text.
 * Can't really improve on that, interface-wise :)
 */
 
import { Dom } from "/js/util/Dom.js";

export class MapCommandsModal {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    // Caller should set.
    // Dirties won't come at high frequency. Just zero or one times per modal.
    this.onDirty = () => {};
    
    this.map = null;
    
    this.buildUi();
  }
  
  setup(map) {
    this.map = map;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "TEXTAREA", ["text"]);
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "OK", "on-click": () => this.onSubmit() });
  }
  
  populateUi() {
    if (this.map) {
      this.element.querySelector(".text").value = this.map.encodeCommands();
    } else {
      this.element.querySelector(".text").value = "";
    }
  }
  
  onSubmit() {
    if (this.map) {
      const text = this.element.querySelector(".text").value;
      this.map.decodeCommands(text);
      this.onDirty();
    }
    this.dom.popModal(this);
  }
}
