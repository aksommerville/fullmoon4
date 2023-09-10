/* PlayUi.js
 * Panel content with the embedded game.
 */
 
import { Dom } from "../util/Dom.js";

export class PlayUi {
  static getDependencies() {
    return [HTMLElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const flexor = this.dom.spawn(this.element, "DIV", ["flexor"]);
    const iframe = this.dom.spawn(flexor, "IFRAME", { src: "./iframe-placeholder.html", width: "640", height: "384" });
    this.dom.spawn(flexor, "DIV", "Default controls: Arrows, Z, X");
  }
}
