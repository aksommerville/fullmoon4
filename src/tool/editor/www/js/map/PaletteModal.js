/* PaletteModal.js
 * Shows the full tilesheet so you can click on one of the tiles and select it.
 * We work with MapService to read and write global state directly.
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { MapService } from "/js/map/MapService.js";

export class PaletteModal {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, MapService];
  }
  constructor(element, dom, resService, mapService) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.mapService = mapService;
    
    this.imageId = this.mapService.paletteImageId;
    this.image = this.resService.getResourceObject("image", this.imageId);
    
    this.buildUi();
    this.element.addEventListener("click", e => this.onClick(e));
  }
  
  buildUi() {
    this.element.innerHTML = "";
    if (this.image) {
      const image = this.image.cloneNode();
      this.element.appendChild(image);
    } else {
      const table = this.dom.spawn(this.element, "TABLE");
      for (let row=0; row<16; row++) {
        const tr = this.dom.spawn(table, "TR");
        for (let col=0; col<16; col++) {
          this.dom.spawn(tr, "TD", (row * 16 + col).toString(16).padStart(2, '0'));
        }
      }
    }
  }
  
  onClick(e) {
    const tileId = this.tileIdFromEvent(e);
    if (tileId >= 0) this.mapService.setSelectedTile(tileId);
    this.dom.popModal(this);
  }
  
  tileIdFromEvent(e) {
    const bounds = this.element.getBoundingClientRect();
    const x = e.clientX - bounds.x;
    const y = e.clientY - bounds.y;
    if ((x < 0) || (x >= bounds.width) || (y < 0) || (y >= bounds.height)) return -1;
    const col = Math.floor((x * 16) / bounds.width);
    const row = Math.floor((y * 16) / bounds.height);
    return (row << 4) | col;
  }
}
