/* ImageAllUi.js
 * List of image resources.
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";

export class ImageAllUi {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, Window];
  }
  constructor(element, dom, resService, window) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.window = window;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    for (const res of this.resService.toc) {
      if (res.type !== "image") continue;
      const card = this.dom.spawn(this.element, "DIV", ["card"], { "on-click": () => this.onClickImage(res.id) });
      this.dom.spawn(card, "IMG", { src: res.path });
      this.dom.spawn(card, "DIV", `${res.id}: ${res.name}`);
    }
  }
  
  onClickImage(id) {
    this.window.location = `#image/${id}`;
  }
}
