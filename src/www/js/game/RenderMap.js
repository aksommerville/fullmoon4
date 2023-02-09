/* RenderMap.js
 * Subservient to Renderer, manages the background images that update infrequently.
 */
 
import { Dom } from "../util/Dom.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";
import { RenderBasics } from "./RenderBasics.js";
 
export class RenderMap {
  static getDependencies() {
    return [Dom, Constants, Globals, DataService, RenderBasics];
  }
  constructor(dom, constants, globals, dataService, renderBasics) {
    this.dom = dom;
    this.constants = constants;
    this.globals = globals;
    this.dataService = dataService;
    this.renderBasics = renderBasics;
    
    this.dirty = false;
    this.awaitingTilesheet = false;
    this.canvas = this.dom.createElement("CANVAS", {
      width: this.constants.COLC * this.constants.TILESIZE,
      height: this.constants.COLC * this.constants.TILESIZE,
    });
  }
  
  setDirty() {
    this.dirty = true;
  }
  
  // Redraws if needed, and returns the map image.
  update() {
    if (this.dirty) {
      this._render(this.canvas);
      this.dirty = false;
    }
    return this.canvas;
  }
  
  /* Private.
   *********************************************************************/
   
  _render(dst) {
    const ctx = dst.getContext("2d");
    const cellv = this.globals.g_map;
    let cellp = 0;
    const imageId = this.globals.g_maptsid[0];
    const tilesheet = this.dataService.getImage(imageId);
    
    // Black out and wait for the tilesheet if it's null or incomplete.
    // This is normal for the first few frames after startup, shouldn't happen after that.
    if (!tilesheet || !tilesheet.complete) {
      ctx.fillStyle = "#000";
      ctx.fillRect(0, 0, dst.width, dst.height);
      if (tilesheet && !this.awaitingTilesheet) {
        this.awaitingTilesheet = true;
        tilesheet.addEventListener("load", () => {
          this.setDirty();
          this.awaitingTilesheet = false;
        });
      }
      return;
    }
    this.awaitingTilesheet = false;
    
    const tilesize = this.constants.TILESIZE;
    for (let dsty=tilesize>>1, yi=this.constants.ROWC; yi-->0; dsty+=tilesize) {
      for (let dstx=tilesize>>1, xi=this.constants.COLC; xi-->0; dstx+=tilesize, cellp++) {
        this.renderBasics.tile(ctx, dstx, dsty, tilesheet, 0);
        if (cellv[cellp]) {
          this.renderBasics.tile(ctx, dstx, dsty, tilesheet, cellv[cellp], 0);
        }
      }
    }
  }
}

RenderMap.singleton = true;
