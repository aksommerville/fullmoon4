/* PressAKeyModal.js
 */
 
import { Dom } from "../util/Dom.js";

export class PressAKeyModal {
  static getDependencies() {
    return [HTMLElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    
    this.onchoose = (keycode) => {};
    
    this.btnid = 0;
    this.keyDownListener = e => this.onKeyDown(e);
    this.window.addEventListener("keydown", this.keyDownListener);
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    if (this.keyDownListener) {
      this.window.removeEventListener("keydown", this.keyDownListener);
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "DIV", ["message"]);
  }
  
  setBtnid(btnid) {
    this.btnid = btnid;
    this.element.querySelector(".message").innerText = `Press a key for: ${this.reprBtnid(btnid)}`;
  }
  
  reprBtnid(btnid) {
    switch (btnid) {
      case InputManager.INPUT_LEFT: return "LEFT";
      case InputManager.INPUT_RIGHT: return "RIGHT";
      case InputManager.INPUT_UP: return "UP";
      case InputManager.INPUT_DOWN: return "DOWN";
      case InputManager.INPUT_USE: return "USE";
      case InputManager.INPUT_MENU: return "CHOOSE";
    }
    return btnid;
  }
  
  onKeyDown(event) {
    if (!event.code) return;
    event.stopPropagation();
    event.preventDefault();
    this.onchoose(event.code);
    this.dom.popModal(this);
  }
}
