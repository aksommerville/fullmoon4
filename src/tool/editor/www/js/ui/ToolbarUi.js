/* ToolbarUi.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";

export class ToolbarUi {
  static getDependencies() {
    return [HTMLElement, Dom, ResService];
  }
  constructor(element, dom, resService) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
  }
}
