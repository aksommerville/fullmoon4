/* ImageSelectModal.js
 * Same idea as ImageAllUi, but designed to display in a modal popup.
 */
 
import { Dom } from "../util/Dom.js";
import { ResService } from "../util/ResService.js";

export class ImageSelectModal {
  static getDependencies() {
    return [HTMLElement, Dom, ResService];
  }
  constructor(element, dom, resService) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    
    this.onChoose = (res) => {};
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const scroller = this.dom.spawn(this.element, "DIV", ["scroller"]);
    for (const res of this.resService.toc) {
      if (res.type !== "image") continue;
      const row = this.dom.spawn(scroller, "DIV", ["row"], { "on-click": () => this.onClick(res) });
      const img = this.dom.spawn(row, "IMG", ["preview"], { src: res.path });
      const vitals = this.dom.spawn(row, "DIV", ["vitals"]);
      this.dom.spawn(vitals, "DIV", `id: ${res.id}`);
      this.dom.spawn(vitals, "DIV", `name: ${res.name}`);
      this.dom.spawn(vitals, "DIV", `q: ${res.q}`);
      this.dom.spawn(vitals, "DIV", `lang: ${res.lang}`);
      if (res.object && res.object.complete) {
        this.dom.spawn(vitals, "DIV", `size: ${res.object.naturalWidth},${res.object.naturalHeight}`);
      }
    }
  }
  
  onClick(res) {
    this.onChoose(res);
    this.dom.popModal(this);
  }
}
