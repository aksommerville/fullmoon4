/* PlayUi.js
 * Panel content with the embedded game.
 */
 
import { Dom } from "../util/Dom.js";

export class PlayUi {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const flexor = this.dom.spawn(this.element, "DIV", ["flexor"]);
    const iframe = this.dom.spawn(flexor, "IFRAME", { src: "./iframe-placeholder.html", width: "640", height: "384" });
    this.dom.spawn(flexor, "DIV", "Click to pause (fullscreen, input config, etc)");
    this.dom.spawn(flexor, "DIV", "Default controls: Arrows, Z, X");
  }
}
