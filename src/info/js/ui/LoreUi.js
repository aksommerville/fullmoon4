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
    let imageSideToggle = true;
    for (const content of [

      "./img/look-i-found-a-feather.png",
      "You are Dot Vine, the witch.",
      "Normally you would never hurt anything, except sometimes, and especially when the anything is a werewolf.",
      "<span></span>Werewolfs! They're the <i>worst</i>!",
      "clear",
      "./img/cast-a-spell.png",
      "Witches use magic.",
      "Sometimes that means casting a spell, sometimes it's a song, or maybe just a particular way of waggling a feather.",
      "Find as much treasure as you can! Everything is useful.",

    ]) {
      switch (content[0]) {
        case '.': this.dom.spawn(this.element, "IMG", [imageSideToggle ? "float-left" : "float-right"], { src: content }); imageSideToggle = !imageSideToggle; break;
        case '<': this.dom.spawn(this.element, "DIV", ["content"]).innerHTML = content; break;
        case 'c': this.dom.spawn(this.element, "BR", { clear: "left" }); break;
        default: this.dom.spawn(this.element, "DIV", ["content"], content); break;
      }
    }
  }
}
