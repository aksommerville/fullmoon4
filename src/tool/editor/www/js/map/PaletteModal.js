/* PaletteModal.js
 * Shows the full tilesheet so you can click on one of the tiles and select it.
 * We work with MapService to read and write global state directly.
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { MapService } from "/js/map/MapService.js";

/* OK actually, having PaletteModal interact with the global MapService was probably a mistake.
 * SpriteUi needs it too, and who knows, there might be other use cases in the future.
 * When instantiating, you can provide an override PaletteModalConfig to turn off that global interaction.
 */
export class PaletteModalConfig {
  constructor(isolated, callback, imageId, tileId) {
    this.isolated = isolated ?? false;
    this.callback = callback || (() => {});
    this.imageId = imageId || 0;
    this.tileId = tileId ?? -1;
  }
}

export class PaletteModal {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, MapService, PaletteModalConfig];
  }
  constructor(element, dom, resService, mapService, config) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.mapService = mapService;
    
    this.config = config;
    if (this.config.isolated) {
      this.imageId = this.config.imageId;
      this.tileId = this.config.tileId;
    } else {
      this.imageId = this.mapService.paletteImageId;
      this.tileId = this.mapService.selectedTile;
    }
    
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
    if ((this.tileId >= 0) && (this.tileId <= 0xff)) {
      const canvas = this.dom.spawn(this.element, "CANVAS");
      canvas.width = 16;
      canvas.height = 16;
      const ctx = canvas.getContext("2d");
      ctx.globalAlpha = 0.5;
      ctx.fillStyle = "#ff0";
      ctx.fillRect(this.tileId & 15, (this.tileId >> 4) & 15, 1, 1);
    }
  }
  
  onClick(e) {
    const tileId = this.tileIdFromEvent(e);
    if (tileId >= 0) {
      if (this.config.isolated) {
        this.config.callback(tileId);
      } else {
        this.mapService.setSelectedTile(tileId);
      }
    }
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
