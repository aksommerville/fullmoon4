/* HintsUi.js
 * Panel content with advice on how to play.
 */
 
import { Dom } from "../util/Dom.js";

export class HintsUi {
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
    
    this.dom.spawn(this.element, "H2", "General Advice");
    this.dom.spawn(this.element, "DIV", ["advice"], "Pencil and paper are recommended. For the free demo version, it shouldn't matter so much.");
    this.dom.spawn(this.element, "DIV", ["advice"], "Remember the Home Spell! Then if you get trapped, you can always return to the beginning.");
    this.dom.spawn(this.element, "DIV", ["advice"], "Lost? Throw seeds on the ground and a bird will show you where to go.");
    this.dom.spawn(this.element, "DIV", ["advice"], "But take it with a grain of salt, I mean he's just a bird, what does he know?");
    
    this.dom.spawn(this.element, "H2", "For Non-Native English Speakers");
    this.dom.spawn(this.element, "DIV", ["advice"], "...hopefully you've managed to translate this page!");
    this.dom.spawn(this.element, "DIV", ["advice"], "There's one puzzle that requires a firm understanding of English.");
    this.dom.spawn(this.dom.spawn(this.element, "DIV", ["spoiler"]), "IMG", { src: "./img/english-words.png" });
  }
}
