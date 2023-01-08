/* FooterUi.js
 * Global footer bar.
 */
 
import { Dom } from "/js/util/Dom.js";

export class FooterUi {
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
    this.dom.spawn(this.element, "DIV", ["copyright"], "\u00a9 2023 AK Sommerville");
    this.dom.spawn(this.element, "A", { href: "https://github.com/aksommerville/fullmoon4" }, "GitHub");
  }
}
