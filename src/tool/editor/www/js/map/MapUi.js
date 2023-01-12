/* MapUi.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { MapService } from "/js/map/MapService.js";
import { ResService } from "/js/util/ResService.js";

export class MapUi {
  static getDependencies() {
    return [HTMLElement, Dom, MapService, ResService];
  }
  constructor(element, dom, mapService, resService) {
    this.element = element;
    this.dom = dom;
    this.mapService = mapService;
    this.resService = resService;
    
    this.buildUi();
  }
  
  setup(mapId, args) {
    console.log(`TODO MapUi.setup`, { mapId, args });
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.element.innerText = "MapUi";
  }
}
