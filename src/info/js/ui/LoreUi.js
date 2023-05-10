/* LoreUi.js
 * Panel content with pictures and stories about Dot's world.
 */
 
import { Dom } from "../util/Dom.js";

export class LoreUi {
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
    //TODO Fill this space with stories and pictures.
    this.dom.spawn(this.element, "DIV", "Dot Vine is a witch.");
    this.dom.spawn(this.element, "DIV", "Witches use magic and stuff.");
    this.dom.spawn(this.element, "DIV", "Why would this pleasant, peaceful witch want to kill a werewolf?");
  }
}
